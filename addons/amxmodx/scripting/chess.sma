// TODO: threefold repetition
// TODO: draw
// TODO: in menu add buttons for draw and surrender
// TODO: 
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

new g_iPlayerColor[MAX_BOARDS][33];
new g_iSelectedPiece[MAX_BOARDS][33];
new g_iBoardPlayers[MAX_BOARDS][33];
new g_iPlacingBoard[33] = { -1, ... };
new g_bPromotionMenuOpen[MAX_BOARDS];
new g_iGamesNum;

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

// public client_connect(id) {
//     // TEST SETUP
//     new boardIndex = init_new_board_test();
//     new ent = create_board_entity(Float:{0.0, 0.0, 0.0}, boardIndex);
//     set_pev(ent, pev_solid, SOLID_BBOX);

//     g_iGlowEnt = create_glow();
//     // server_print("board created %d", boardIndex);
//     // g_iPlacingBoard[id] = boardIndex;
//     g_iBoardPlayers[boardIndex][id] = id;
//     g_iBoardPlayers[boardIndex][id] = id;

//     g_iPlayerColor[boardIndex][id] = 0;
//     // g_iPlayerColor[boardIndex][id] = !g_iPlayerColor[boardIndex][playerOne];

//     g_iGamesNum ++;
//     create_board_entities_dbg(boardIndex);
//     render_pieces(boardIndex);
//     print_board(boardIndex);
// }

public client_disconnected(id)
{
    if(g_iPlacingBoard[id] != -1) {
        delete_board(g_iPlacingBoard[id]);
        g_iPlacingBoard[id] = -1;
    }

    for(new i = 0; i < MAX_BOARDS; i++) {
        if(g_iBoardPlayers[i][id] != 0) {
            delete_board(i);
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
            g_iPieceEntities[i][x][y] = 0;
        }
    }
    delete_promotion_menu(i);
    g_bPromotionMenuOpen[i] = false;
    g_iGamesNum --;

    g_iBoardEntity[i] = 0;
    g_iPromotionMenuEntities[i] = {0, 0, 0, 0};

    for(new j = 0; j < 33; j++) {
        g_iBoardPlayers[i][j] = 0;
        g_iSelectedPiece[i][j] = InvalidID;
        g_iPlayerColor[i][j] = 0;
    }
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
        if(g_iBoardPlayers[i][id] != 0) {

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
                            g_iGamesNum ++;
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
                                if(g_iPlayerColor[boardIndex][id] == g_iTurn[boardIndex] 
                                    || g_iBoardPlayers[boardIndex][id] == id // play against self
                                ) {

                                    new px, py;
                                    px = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex][id]][Row];
                                    py = g_PiecesList[boardIndex][g_iSelectedPiece[boardIndex][id]][Column];
                                    // server_print("move: %d %d -> %d %d", px, py, x, y);
                                    if(do_move(boardIndex, px, py, x, y)) {
                                        // server_print("move success");
                                        render_pieces(boardIndex);

                                        if(is_fifty_move_rule_draw(boardIndex)) {
                                            on_draw(boardIndex, id);
                                        }

                                        new pawnIndex;
                                        if(is_wait_for_promote_choice(boardIndex, pawnIndex)) {
                                            // server_print("is_wait_for_promote_choice: true, pawnIndex: %d", pawnIndex);
                                            if(!g_bPromotionMenuOpen[boardIndex]) {
                                                // server_print("g_bPromotionMenuOpen: false");
                                                create_promotion_menu(boardIndex, pawnIndex);
                                                g_bPromotionMenuOpen[boardIndex] = true;
                                            }
                                        } else {
                                            new color = get_at(x, y, Color);
                                            new is_check;
                                            new mate = is_mate(boardIndex, get_opposite_color(color), is_check);
                                            // server_print("is_mate: %b, is_check %b", mate, is_check);
                                            // server_print("color moved %d", color);
                                            if(mate) on_mate(boardIndex, id, get_opposite_color(color));
                                            if(!is_check && is_stalemate(boardIndex, get_opposite_color(color))
                                            || count_position_for_threefold_rule(boardIndex) == 3) 
                                                on_draw(boardIndex, id);

                                        }

                                    } else {
                                        // server_print("move failed");
                                        client_print(id, print_chat, "move failed!");
                                    }
                                    g_iSelectedPiece[boardIndex][id] = InvalidID;
                                } else {
                                    client_print(id, print_chat, "not your turn!");
                                }
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

                    new color = get(pawnId, Color);
                    new is_check;
                    new mate = is_mate(boardIndex, get_opposite_color(color), is_check);
                    if(mate) on_mate(boardIndex, id, get_opposite_color(color));
                    if(!is_check && is_stalemate(boardIndex, get_opposite_color(color))
                    || count_position_for_threefold_rule(boardIndex) == 3) 
                        on_draw(boardIndex, id);

                    
                    // server_print("is_mate(after promotion): %b", mate);
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

on_mate(boardIndex, winnerId, color_in_mate) {
    client_print_color(0, print_team_red, "[^4CHESS^1] Winner ^4%n ^1: Loser ^3%n", winnerId, g_iBoardPlayers[winnerId]);
    delete_board(boardIndex);
}

on_draw(boardIndex, lastMovedPlayerId) {
    client_print_color(0, print_team_grey, "[^4CHESS^1] Draw: ^3%n^1 vs. ^3%n", lastMovedPlayerId, g_iBoardPlayers[lastMovedPlayerId]);
    delete_board(boardIndex);
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
    new menu = menu_create("Chess [0.0.1-alpha]", "chess_menu_handler");

    menu_additem(menu, "Choose opponent", "1");
    // menu_addblank2(menu);
    menu_additem(menu, fmt("Games [\r%d\w]", g_iGamesNum), "2");
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
            if(g_iPlacingBoard[id] == -1) 
                choose_opponent_menu(id);
            else
                client_print(id, print_chat, "you should place current board");
        }
        case 2: {
            if(g_iGamesNum > 0)
                show_games_menu(id);
        }
    
    }

    return PLUGIN_HANDLED;
}

show_games_menu(id)
{
    new menu = menu_create("Current chess games", "games_menu_handler");

    new szItem[100], itemInfo[3], playerNum;
    for(new i = 0; i < MAX_BOARDS; i++) {
        // if(g_Boards)
        for(new j = 1; j < 33; j++) {
            if(g_iBoardPlayers[i][j] > 0) {
                if(playerNum == 0) {
                    strcat(szItem, fmt("%n", g_iBoardPlayers[i][j]), sizeof szItem);
                } else {
                    strcat(szItem, fmt(" vs. %n", g_iBoardPlayers[i][j]), sizeof szItem);
                }
                itemInfo[playerNum] = j;
                playerNum ++;
            }
        }

        if(playerNum == 1) {
            strcat(szItem, fmt(" plays alone"), sizeof szItem);
            itemInfo[1] = itemInfo[0];
        }

        if(playerNum > 0) {
            itemInfo[2] = i;
            menu_additem(menu, szItem, itemInfo);
        }
        playerNum = 0;
        arrayset(szItem, 0, sizeof szItem);
        arrayset(itemInfo, 0, sizeof itemInfo);
    }

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public games_menu_handler( id, menu, item )
 {
    //Do a check to see if they exited because menu_item_getinfo ( see below ) will give an error if the item is MENU_EXIT
    if ( item == MENU_EXIT )
    {
        menu_destroy( menu );
        return PLUGIN_HANDLED;
    }


    new szData[3], szName[64];
    new _access, item_callback;

    menu_item_getinfo( menu, item, _access, szData,charsmax( szData ), szName,charsmax( szName ), item_callback );

    show_player_menu(id, szData);

    menu_destroy( menu );
    return PLUGIN_HANDLED;
 }

show_player_menu(id, szData[3])
{
    new boardIndex = szData[2];
    new menu = menu_create(fmt("Chess player menu [Board %d]", boardIndex), "player_menu_handler");


    menu_additem(menu, "-");
    menu_additem(menu, "-");

    menu_additem(menu, "Draw", szData);
    menu_additem(menu, "Surrender", szData);

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public player_menu_handler( id, menu, item )
 {
    //Do a check to see if they exited because menu_item_getinfo ( see below ) will give an error if the item is MENU_EXIT
    if ( item == MENU_EXIT )
    {
        menu_destroy( menu );
        return PLUGIN_HANDLED;
    }


    new szData[3], szName[64];
    new _access, item_callback;

    menu_item_getinfo( menu, item, _access, szData,charsmax( szData ), szName,charsmax( szName ), item_callback );
// server_print("item %d", item);
    if(id == szData[0] || id == szData[1]) {
        if(item == 2) { // draw
            // should ask other player
            new playerId = id != szData[0] ? szData[0] : szData[1];
            show_ask_for_draw_menu(playerId, szData);
        } else if(item == 3) { // surrender
            new winnerId = id == szData[0] ? szData[0] : szData[1];
            on_mate(szData[2], winnerId, get_opposite_color(g_iPlayerColor[szData[2]][winnerId]));
        }
    }

    menu_destroy( menu );
    return PLUGIN_HANDLED;
 }

 show_ask_for_draw_menu(id, szData[3])
{
    new menu = menu_create(fmt("Opponent requested DRAW:"), "ask_for_draw_menu_handler");


    menu_additem(menu, "-");
    menu_additem(menu, "-");
    menu_additem(menu, "Accept");
    menu_additem(menu, "Decline");

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    return PLUGIN_HANDLED;
}

public ask_for_draw_menu_handler( id, menu, item )
 {
    //Do a check to see if they exited because menu_item_getinfo ( see below ) will give an error if the item is MENU_EXIT
    if ( item == MENU_EXIT )
    {
        menu_destroy( menu );
        return PLUGIN_HANDLED;
    }

    new szData[3], szName[64];
    new _access, item_callback;

    menu_item_getinfo( menu, item, _access, szData,charsmax( szData ), szName,charsmax( szName ), item_callback );

    if(item == 2) { // draw
        on_draw(szData[2], id);
    } else if(item == 3) { // surrender
        // TODO: notify draw is declined
    }

    menu_destroy( menu );
    return PLUGIN_HANDLED;
 }

public choose_opponent_menu( id )
 {
    //Create a variable to hold the menu
    new menu = menu_create( "Choose player to request a game!:", "choose_opponent_menu_handler" );

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

    new menu = menu_create(fmt("You are invited to play chess by: \r%s", szChallengerName), "you_challenged_menu_handler");

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
    new boardIndex = init_new_board();
    if(boardIndex == -1) {
        client_print(playerOne, print_chat, "game aborted: max boards limit reached");
        client_print(playerTwo, print_chat, "game aborted: max boards limit reached");
        return;
    }
    // server_print("A game of chess started with players: %n | %n", playerOne, playerTwo);
    client_print(playerOne, print_chat, "A game of chess against: %n", playerTwo);
    client_print(playerTwo, print_chat, "A game of chess against: %n", playerOne);
    
    g_iGlowEnt = create_glow();
    // server_print("board created %d", boardIndex);
    g_iPlacingBoard[playerTwo] = boardIndex;
    g_iBoardPlayers[boardIndex][playerOne] = playerTwo;
    g_iBoardPlayers[boardIndex][playerTwo] = playerOne;

    g_iPlayerColor[boardIndex][playerOne] = random_num(0, 1);
    g_iPlayerColor[boardIndex][playerTwo] = !g_iPlayerColor[boardIndex][playerOne];
    // server_print("%d", g_iPlayerColor[boardIndex][playerOne]);

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