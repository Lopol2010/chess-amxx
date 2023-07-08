// TODO: threefold repetition
// TODO: 50 move rule
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <chess>
#include <chess_logic.inl>
#include <chess_3d.inl>

#define ACCESS_FLAG ADMIN_LEVEL_B

#define PLUGIN "Chess Game"
#define AUTHOR "Lopol2010"
#define VERSION "0.0.1"

// these for debug
#pragma semicolon 1

new g_iSelectedPiece[MAX_BOARDS][33];
new g_iBoardPlayers[MAX_BOARDS][33];
new g_iPlacingBoard[33] = { -1, ... };
new g_bPromotionMenuOpen[MAX_BOARDS];

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);

    register_clcmd("chess", "cmd_chess");
    // register_clcmd("chess_admin_menu", "cmd_admin_menu", ACCESS_FLAG);
    register_clcmd("chess_delete_all", "cmd_delete_all", ACCESS_FLAG);

    // register_think(BOX_CLASSNAME, "fwd_box_think");

    register_forward( FM_CmdStart, "FwdCmdStart" );
    register_forward(FM_PlayerPreThink, "fwd_player_prethink", 1);


    // chess_render_init();
    // start_new_game();
    for(new i = 0; i < MAX_BOARDS; i++) {
        for(new j = 0; j < 33; j++) {
            g_iSelectedPiece[i][j] = InvalidID;
        }
    }
}

public plugin_precache()
{
    precache_model(g_szModel);
    precache_model(g_szChessPiecesModel);
    precache_model(g_szChessBoardModel);

    g_iSpriteLine = precache_model("sprites/white.spr");
}
public plugin_cfg()
{
}

public client_disconnected(id)
{
    if(g_iPlacingBoard[id] != -1) {
        delete_board(g_iPlacingBoard[id]);
        g_iPlacingBoard[id] = -1;
    }

    for(new i = 0; i < MAX_BOARDS; i++) {
        if(g_iBoardPlayers[i][id] == id || g_iBoardPlayers[i][id] == id) {
            delete_board(i);
            g_iBoardPlayers[i][id] = -1;
            g_iBoardPlayers[i][id] = -1;
            g_iSelectedPiece[i][id] = InvalidID;
            g_iSelectedPiece[i][id] = InvalidID;
        }
    }

}

public delete_board(i) {
    new boardEnt = g_iBoardEntity[i];
    if(task_exists(boardEnt)) remove_task(boardEnt);
    if(is_valid_ent(boardEnt)) remove_entity(boardEnt);
    deinitialize_board(i);
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {
            new pieceEnt = g_iPieceEntities[i][x][y];
            if(is_valid_ent(pieceEnt)) remove_entity(pieceEnt);
        }
    }
    delete_promotion_menu(i);
    g_bPromotionMenuOpen[i] = false;
}

// public FwdCmdStart(id, uc_handle)
public fwd_player_prethink(id)
{
    static Button, OldButtons;
    // Button = get_uc(uc_handle, UC_Buttons);
    // OldButtons = pev(id, pev_oldbuttons);
    Button = pev(id, pev_button);
    OldButtons = pev(id, pev_oldbuttons);
 
    // start with loop + traceline in order to prevent attack animation 
    // otherwise when doing move in chess you would see unfinished attacks
    for(new i = 0; i < MAX_BOARDS; i++) {
        if(g_iBoardPlayers[i][id] == id) {

            new Float:origin[3];
            new x, y, boardIndex, promotion_menu_item_entity, new_rank;
            switch(Traceline(id, g_iGlowEnt, origin, x, y, boardIndex, promotion_menu_item_entity, new_rank)) {

                case TR_RESULT_NONE: { 
                    // server_print("traceline result none!"); 

                    if(g_iPlacingBoard[id] != -1) {
                        new boardEnt = g_iBoardEntity[g_iPlacingBoard[id]];
                        set_pev(boardEnt, pev_origin, origin);
                        if((Button & IN_ATTACK) && !(OldButtons & IN_ATTACK)) {
                            set_pev(boardEnt, pev_solid, SOLID_BBOX);
                            create_board_entities_dbg(g_iPlacingBoard[id]);
                            g_iPlacingBoard[id] = -1;
                        }
                    }
                }
                case TR_RESULT_BOARD: { 
                    // player looking at board, prevent attack animation
                    set_pdata_float(id, m_flNextAttack, 1.0);
                    if((Button & IN_ATTACK) && !(OldButtons & IN_ATTACK)) {
                        if(!g_bPromotionMenuOpen[boardIndex]) {
                            // server_print("g_iSelectedPiece[boardIndex][id] %d", g_iSelectedPiece[boardIndex][id]);
                            if(g_iSelectedPiece[boardIndex][id] != InvalidID) {
                                new px, py;
                                px = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex][id]][Row];
                                py = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex][id]][Column];
                                // server_print("move: %d %d -> %d %d", px, py, x, y);
                                if(do_move(boardIndex, px, py, x, y)) {
                                    // server_print("move success");
                                    render_pieces(boardIndex);
                                    new pawnIndex;
                                    if(is_wait_for_promote_choice(boardIndex, pawnIndex)) {
                                        // server_print("is_wait_for_promote_choice: true, pawnIndex: %d", pawnIndex);
                                        if(!g_bPromotionMenuOpen[boardIndex]) {
                                            // server_print("g_bPromotionMenuOpen: false");
                                            create_promotion_menu(boardIndex, pawnIndex);
                                            g_bPromotionMenuOpen[boardIndex] = true;
                                        }
                                    }

                                } else {
                                    // server_print("move failed");
                                    client_print(id, print_chat, "move failed!");
                                }
                                g_iSelectedPiece[boardIndex][id] = InvalidID;
                            } else {
                                g_iSelectedPiece[boardIndex][id] = g_PiecesMatrix[boardIndex][x][y];
                            }
                        }
                    }

                    return;
                }
                case TR_RESULT_PROMOTION_MENU_ITEM: { 
                    // player chosing pieces, prevent attack animation
                    set_pdata_float(id, m_flNextAttack, 1.0);
                    if(!((Button & IN_ATTACK) && !(OldButtons & IN_ATTACK))) return;

                    new pawnId = pev(promotion_menu_item_entity, PEV_PIECE_ID);
                    set_list(pawnId, Rank, new_rank);
                    print_piece(boardIndex, pawnId);
                    
                    delete_promotion_menu(boardIndex);
                    g_bPromotionMenuOpen[boardIndex] = false;
                    render_pieces(boardIndex);
                }

            }

            // g_iSelectedPiece[boardIndex][id] = InvalidID;
        }
    }
   
    if((Button & IN_ATTACK) && (OldButtons & IN_ATTACK))
    {
        // Player is holding down right click
    }
    
    if(!(Button & IN_ATTACK) && (OldButtons & IN_ATTACK))
    {
        // Player releases right click
    }

} 

// start_new_game() {
//     new boardIndex = init_new_board();
//     // g_iGlowEnt = create_glow();
//     create_board_entities_dbg(boardIndex);
//     // render_pieces(boardIndex);
// }

public cmd_chess(id, level, cid)
{
    if(!cmd_access(id, level, cid, 1)) {
        return PLUGIN_HANDLED;
    }

    show_chess_menu(id);

    return PLUGIN_HANDLED;
} 

show_chess_menu(id)
{
    new menu = menu_create("Chess", "chess_menu_handler");

    menu_additem(menu, "Choose opponent", "1");
    // menu_addblank2(menu);
    // menu_additem(menu, "Create board", "2");
    // menu_additem(menu, "Board setting #2", "3");
    // menu_additem(menu, "Board setting #3", "4");

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public chess_menu_handler(id, menu, item)
{
    if(item == MENU_EXIT) {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new item_info[8];
    menu_item_getinfo(menu, item, _, item_info, charsmax(item_info));
    menu_destroy(menu);

    new index = str_to_num(item_info);

    switch(index) {
        case 1: {
            choose_opponent_menu(id);
        }
    }

    return PLUGIN_HANDLED;
}

public choose_opponent_menu( id )
 {
    //Create a variable to hold the menu
    new menu = menu_create( "\rChoose player to request a game!:", "choose_opponent_menu_handler" );

    //We will need to create some variables so we can loop through all the players
    new players[32], pnum, tempid;

    //Some variables to hold information about the players
    new szName[32], szUserId[32];

    //Fill players with available players
    get_players( players, pnum, "a" ); // flag "a" because we are going to add health to players, but this is just for this specific case

    //Start looping through all players
    for ( new i; i<pnum; i++ )
    {
        //Save a tempid so we do not re-index
        tempid = players[i];

        //Get the players name and userid as strings
        get_user_name( tempid, szName, charsmax( szName ) );
        //We will use the data parameter to send the userid, so we can identify which player was selected in the handler
        formatex( szUserId, charsmax( szUserId ), "%d", get_user_userid( tempid ) );

        //Add the item for this player
        menu_additem( menu, szName, szUserId, 0 );
    }

    //We now have all players in the menu, lets display the menu
    menu_display( id, menu, 0 );
 }

public choose_opponent_menu_handler( id, menu, item )
 {
    //Do a check to see if they exited because menu_item_getinfo ( see below ) will give an error if the item is MENU_EXIT
    if ( item == MENU_EXIT )
    {
        menu_destroy( menu );
        return PLUGIN_HANDLED;
    }

    //now lets create some variables that will give us information about the menu and the item that was pressed/chosen
    new szData[6], szName[64];
    new _access, item_callback;
    //heres the function that will give us that information ( since it doesnt magicaly appear )
    menu_item_getinfo( menu, item, _access, szData,charsmax( szData ), szName,charsmax( szName ), item_callback );

    //Get the userid of the player that was selected
    new userid = str_to_num( szData );

    //Try to retrieve player index from its userid
    new player = find_player( "kh", userid ); // flag "k" : find player from userid

    //If player == 0, this means that the player's userid cannot be found
    //If the player is still alive ( we had retrieved alive players when formating the menu but some players may have died before id could select an item from the menu )
    if ( player && is_user_alive( player ) )
    {
        you_challenged_menu(player, id);
    }

    menu_destroy( menu );
    return PLUGIN_HANDLED;
 }

// when someone select you as opponent you'll see this menu
you_challenged_menu(id, challenger)
{
    new szChallengerName[32], szChallengerIndex[32];
    get_user_name( challenger, szChallengerName, charsmax( szChallengerName ) );
    formatex( szChallengerIndex, charsmax( szChallengerIndex ), "%d", challenger );

    new menu = menu_create(fmt("You are challenged to play chess by: ^r%s", szChallengerName), "you_challenged_menu_handler");

    menu_additem(menu, "Accept", szChallengerIndex);
    menu_additem(menu, "Decline");

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public you_challenged_menu_handler( id, menu, item )
 {
    //Do a check to see if they exited because menu_item_getinfo ( see below ) will give an error if the item is MENU_EXIT
    if ( item == MENU_EXIT )
    {
        menu_destroy( menu );
        return PLUGIN_HANDLED;
    }


    switch(item) {
        case 0: {
            new szData[6], szName[64];
            new _access, item_callback;

            menu_item_getinfo( menu, item, _access, szData,charsmax( szData ), szName,charsmax( szName ), item_callback );
            new id2 = str_to_num( szData );
            challenge_accepted(id, id2);
        }
        case 1: {
        }
    }

    menu_destroy( menu );
    return PLUGIN_HANDLED;
 }

 challenge_accepted(playerOne, playerTwo) {
    if(get_uninitialized_board() == -1) {
        client_print(playerOne, print_chat, "game aborted: max boards limit reached");
        client_print(playerTwo, print_chat, "game aborted: max boards limit reached");
        return;
    }
    // server_print("A game of chess started with players: %n | %n", playerOne, playerTwo);
    client_print(playerOne, print_chat, "A game of chess against: %n", playerTwo);
    client_print(playerTwo, print_chat, "A game of chess against: %n", playerOne);
    
    g_iGlowEnt = create_glow();
    new boardIndex = init_new_board();
    // server_print("board created %d", boardIndex);
    g_iPlacingBoard[playerTwo] = boardIndex;
    g_iBoardPlayers[boardIndex][playerOne] = playerOne;
    g_iBoardPlayers[boardIndex][playerTwo] = playerTwo;

    // create_board_entities_dbg(g_iPlacingBoard[playerTwo]);
    new ent = create_board_entity(Float:{0.0, 0.0, 0.0}, boardIndex);
 }

public cmd_delete_all(id, level, cid)
{
    if(!cmd_access(id, level, cid, 1)) {
        return PLUGIN_HANDLED;
    }

    for(new i = 0; i < MAX_BOARDS; i++) {
        if(!g_Boards[i][IsInitialized]) continue;
        g_iBoardPlayers[i][id] = -1;
        g_iBoardPlayers[i][id] = -1;
        delete_board(i);
    }

    return PLUGIN_HANDLED;
} 


public cmd_chess_admin_menu(id, level, cid)
{
    if(!cmd_access(id, level, cid, 1)) {
        return PLUGIN_HANDLED;
    }

    show_admin_menu(id);

    return PLUGIN_HANDLED;
} 

// TODO: manage boards?
show_admin_menu(id)
{
    new menu = menu_create("Chess Admin Menu", "chess_admin_menu_handler");

    menu_additem(menu, "Delete boards", "1");
    // menu_addblank2(menu);
    // menu_additem(menu, "Create board", "2");
    // menu_additem(menu, "Board setting #2", "3");
    // menu_additem(menu, "Board setting #3", "4");

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public chess_admin_menu_handler(id, menu, item)
{
    if(item == MENU_EXIT) {
        menu_destroy(menu);
        return PLUGIN_HANDLED;
    }

    new item_info[8];
    menu_item_getinfo(menu, item, _, item_info, charsmax(item_info));
    menu_destroy(menu);

    new index = str_to_num(item_info);

    switch(index) {
        case 1: {
            // choose_opponent_menu(id);
        }
    }

    return PLUGIN_HANDLED;
}