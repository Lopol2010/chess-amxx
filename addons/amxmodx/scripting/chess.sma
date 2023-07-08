// Credits: R3X@Box System
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

#pragma semicolon 1

// TODO: this should be per player + per board (e.g. player can select multiple pieces if each of them is on different board)
// TODO: clear on client_disconnect
new g_iSelectedPiece[MAX_BOARDS] = { InvalidID, ... };
new g_iBoardPlayers[MAX_BOARDS][2];
new g_iPlacingBoard[33] = { -1, ... };
new g_bPromotionMenuOpen[MAX_BOARDS];

public plugin_init()
{
    register_plugin(PLUGIN, VERSION, AUTHOR);

    register_clcmd("chess", "cmd_chess");
    // register_clcmd("chess_admin_menu", "cmd_admin_menu", ACCESS_FLAG);
    register_clcmd("chess_delete_all", "cmd_delete_all", ACCESS_FLAG);

    register_think(BOX_CLASSNAME, "fwd_box_think");

    register_forward( FM_CmdStart, "FwdCmdStart" );
    // register_forward(FM_TraceLine, "fwd_trace_line", 1);
    register_forward(FM_PlayerPreThink, "fwd_player_prethink", 1);

    // register_touch(BOX_CLASSNAME, "*", "fwd_box_touch");
    // g_hForwards[StartTouch] = CreateMultiForward("bwb_box_start_touch", ET_STOP, FP_CELL, FP_CELL, FP_CELL);
    // g_hForwards[StopTouch] = CreateMultiForward("bwb_box_stop_touch", ET_STOP, FP_CELL, FP_CELL, FP_CELL);
    // g_hForwards[FrameTouch] = CreateMultiForward("bwb_box_touch", ET_STOP, FP_CELL, FP_CELL, FP_CELL);
    // g_hForwards[BoxCreated] = CreateMultiForward("bwb_box_created", ET_STOP, FP_CELL, FP_STRING);
    // g_hForwards[BoxDeleted] = CreateMultiForward("bwb_box_deleted", ET_STOP, FP_CELL, FP_STRING);
    // g_hForwards[InvalidTouch] = CreateMultiForward("bwb_box_invalid_touch", ET_STOP, FP_CELL, FP_CELL, FP_CELL);

    // chess_render_init();
    // start_new_game();
}

public plugin_precache()
{
    precache_model(g_szModel);
    precache_model(g_szChessPiecesModel);
    precache_model(g_szChessBoardModel);

    g_iSpriteLine = precache_model("sprites/white.spr");
}
public plugin_natives()
{
    register_library("box_with_boxes");

    g_aTypes = ArrayCreate(TypeStruct, 1);

    register_native("bwb_create_box", "native_create_box");
    // register_native("bwb_register_box_type", "native_register_box_type");
    register_native("bwb_get_type_index", "native_get_type_index");
    register_native("bwb_get_box_type", "native_get_box_type");
}
public plugin_cfg()
{
    load_types();
}

public client_disconnect(id)
{
    if(g_iPlacingBoard[id] != -1) {
        // TODO: delete board
        delete_board(g_iPlacingBoard[id]);
        g_iPlacingBoard[id] = -1;
    }

    for(new i = 0; i < MAX_BOARDS; i++) {
        if(g_iBoardPlayers[i][0] == id || g_iBoardPlayers[i][1] == id) {
            // TODO: delete board
            delete_board(i);
            g_iBoardPlayers[i][0] = -1;
            g_iBoardPlayers[i][1] = -1;
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
 
//  client_print(0, print_chat, "%d %d", Button, IN_ATTACK2);
    if((Button & IN_ATTACK) && !(OldButtons & IN_ATTACK))
    {
        // Player presses right click
        new Float:origin[3];
        // pev(id, pev_origin, origin);
            // client_print (0, print_chat, " 123 %d %d ", 1, 1);
        new x, y, boardIndex, promotion_menu_item_entity, new_rank;
        switch(Traceline(id, g_iGlowEnt, origin, x, y, boardIndex, promotion_menu_item_entity, new_rank)) {

            case TR_RESULT_NONE: { }
            case TR_RESULT_BOARD: { 
                if(g_bPromotionMenuOpen[boardIndex]) {
                    server_print("you should choose promotion option!");
                } else {
                    // set_pev(id, pev_flNextAttack, 10.0);
                    set_pdata_float(id, m_flNextAttack, 1.0);

                    // client_print (0, print_chat, " 123 %d %d ", x, y);
                    if(g_iSelectedPiece[boardIndex] != InvalidID) {
                        new px, py;
                        px = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex]][Row];
                        py = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex]][Column];
                        console_print(0, "move: %d %d %d %d", px, py, x, y);
                        if(do_move(boardIndex, px, py, x, y)) {
                            console_print(0, "move success");
                            render_pieces(boardIndex);
                            new pawnIndex;
                            if(is_wait_for_promote_choice(boardIndex, pawnIndex)) {
                                server_print("is_wait_for_promote_choice: true, pawnIndex: %d", pawnIndex);
                                if(!g_bPromotionMenuOpen[boardIndex]) {
                                    server_print("g_bPromotionMenuOpen: false");
                                    create_promotion_menu(boardIndex, pawnIndex);
                                    g_bPromotionMenuOpen[boardIndex] = true;
                                }
                            }

                        } else {
                            console_print(0, "move failed");
                        }
                        g_iSelectedPiece[boardIndex] = InvalidID;
                        return;
                    }
                    g_iSelectedPiece[boardIndex] = g_PiecesMatrix[boardIndex][x][y];
                    return;
                }
            }
            case TR_RESULT_PROMOTION_MENU_ITEM: { 
                new pawnId = pev(promotion_menu_item_entity, PEV_PIECE_ID);
                set_list(pawnId, Rank, new_rank);
                print_piece(boardIndex, pawnId);
                
                delete_promotion_menu(boardIndex);
                g_bPromotionMenuOpen[boardIndex] = false;
                render_pieces(boardIndex);
            }

        }
        g_iSelectedPiece[boardIndex] = InvalidID;

        if(g_iPlacingBoard[id] != -1) {

            create_board_entities_dbg(g_iPlacingBoard[id], origin);
            new boardEnt = g_iBoardEntity[g_iPlacingBoard[id]];
            server_print("boardEnt %d", boardEnt);
            // set_pev(boardEnt, pev_origin, origin);
            // set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", boardEnt, .flags = "b");
            g_iPlacingBoard[id] = -1;
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

// public fwd_player_prethink(id)
// {

//     return FMRES_IGNORED;
// }

// TODO: restrict board to be usable only by 2 players
start_new_game() {

    new boardIndex = init_new_board();
    // g_iGlowEnt = create_glow();
    create_board_entities_dbg(boardIndex);
    // render_pieces(boardIndex);
}

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
    new menu = menu_create( "\rLook at this Player Menu!:", "choose_opponent_menu_handler" );

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
    server_print("A game of chess started with players: %n | %n", playerOne, playerTwo);
    client_print(playerOne, print_chat, "A game of chess against: %n", playerTwo);
    client_print(playerTwo, print_chat, "A game of chess against: %n", playerOne);
    
    g_iGlowEnt = create_glow();
    new boardIndex = init_new_board();
    server_print("board created %d", boardIndex);
    g_iPlacingBoard[playerTwo] = boardIndex;
    g_iBoardPlayers[boardIndex][0] = playerOne;
    g_iBoardPlayers[boardIndex][1] = playerTwo;

 }

public cmd_delete_all(id, level, cid)
{
    if(!cmd_access(id, level, cid, 1)) {
        return PLUGIN_HANDLED;
    }

    for(new i = 0; i < MAX_BOARDS; i++) {
        if(!g_Boards[i][IsInitialized]) continue;
        g_iBoardPlayers[i][0] = -1;
        g_iBoardPlayers[i][1] = -1;
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