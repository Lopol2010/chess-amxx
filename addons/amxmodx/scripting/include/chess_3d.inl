#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>

#define PEV_BOARD_INDEX pev_iuser3
#define PEV_PIECE_ID pev_iuser4

#define m_flNextAttack 83

new Float:BOARD_SIZE[3] = { 64.00, 64.00, 4.4 };
new Float:SQUARE_SIZE = 8.00;

new Float:g_fBoardCorner[MAX_BOARDS][3];
new g_iBoardEntity[MAX_BOARDS];
new g_iPieceEntities[MAX_BOARDS][BOARD_ROWS][BOARD_COLUMNS];
new g_iPromotionMenuEntities[MAX_BOARDS][4];

new const g_szModel[] = "sprites/cnt1.spr";
new const g_szChessPiecesModel[] = "models/chess_pieces.mdl";
new const g_szChessBoardModel[] = "models/chess_board.mdl";

// these for debug
new g_iSpriteLine;
new g_iGlowEnt;

enum _:TracelineResult {
    TR_RESULT_NONE,
    TR_RESULT_BOARD,
    TR_RESULT_PROMOTION_MENU_ITEM,
};

create_board_entity(const Float:origin[3], boardIndex)
{
    new ent = create_entity("func_wall");
    g_iBoardEntity[boardIndex] = ent;

    DispatchSpawn(ent);

    set_pev(ent, PEV_BOARD_INDEX, boardIndex);

    entity_set_model(ent, g_szChessBoardModel);
    set_pev(ent, pev_origin, origin);
    set_rendering(ent, kRenderFxNone, 155, 111, 0, kRenderNormal, 255);

    set_pev(ent, pev_solid, SOLID_NOT);
    set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);

    new Float:mins[3]; xs_vec_div_scalar(BOARD_SIZE, -2.0, mins);
    new Float:maxs[3]; xs_vec_div_scalar(BOARD_SIZE, 2.0, maxs);
    entity_set_size(ent, mins, maxs);

    return ent;
}

create_promotion_menu_item_entity(const Float:origin[3])
{
    new ent = create_entity("func_wall");

    DispatchSpawn(ent);

    entity_set_model(ent, g_szChessPiecesModel);
    set_pev(ent, pev_origin, origin);
    set_rendering(ent, kRenderFxNone, 155, 111, 0, kRenderNormal, 255);

    set_pev(ent, pev_solid, SOLID_BBOX);
    
    set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
    entity_set_size(ent, Float:{ -3.0, -3.0, 0.0 }, Float:{ 3.0, 3.0, 6.0 });

    return ent;
}

create_glow(const class[] = "glow")
{
    new ent = create_entity("info_target");

    DispatchSpawn(ent);


    entity_set_model(ent, g_szModel);
    entity_set_size(ent, Float:{ -1.0, -1.0, -1.0 }, Float:{ 1.0, 1.0, 1.0 });

    set_pev(ent, pev_solid, SOLID_TRIGGER);

    set_pev(ent, pev_scale, 0.05);

    set_rendering(ent, kRenderFxPulseFast, 0, 250, 0, kRenderTransAdd, 2555);

    return ent;
}

// debug func
public show_board_grid(ent)
{
    new Float:absmins[3], Float:absmaxs[3], Float:mins[3], Float:maxs[3], Float:origin[3];
    pev(ent, pev_absmin, absmins);
    pev(ent, pev_absmax, absmaxs);
    pev(ent, pev_mins, mins);
    pev(ent, pev_maxs, maxs);
    pev(ent, pev_origin, origin);

    new color[3] = {0,255,0};

    _draw_line(color, absmins[0], absmaxs[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 011 001
    _draw_line(color, absmaxs[0], absmins[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 101 001
    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmaxs[2]); // 111 011
    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmaxs[0], absmins[1], absmaxs[2]); // 111 101

    new Float:x = SQUARE_SIZE;
    new Float:y = SQUARE_SIZE;
    for(new i = 1; i < 8; i++) {
        _draw_line(color, absmins[0] + x + 1, absmaxs[1], absmaxs[2], absmins[0] + x + 1, absmins[1], absmaxs[2]); // 011 001
        _draw_line(color, absmaxs[0], absmins[1] + y + 1, absmaxs[2], absmins[0], absmins[1] + y + 1, absmaxs[2]); // 101 001
        // client_print(0, print_chat, "%d %f", i, absmaxs[0] - (i * (8)));

        x += (SQUARE_SIZE);
        y += (SQUARE_SIZE);
    }
}

_draw_line(color[3], Float:from_x, Float:from_y, Float:z1, Float:to_x, Float:to_y, Float:z2)
{
    new Float:start[3];
    start[0] = from_x;
    start[1] = from_y;
    start[2] = z1;

    new Float:stop[3];
    stop[0] = to_x;
    stop[1] = to_y;
    stop[2] = z2;

    // draw_line(start, stop, color, g_iSpriteLine);
}

TracelineResult:Traceline(id, ent, Float:hit[3], &x, &y, &boardIndex, &promotion_menu_item_entity, &new_rank)
{
    new Float:fVec[3];
    pev(id, pev_v_angle, fVec);
    angle_vector(fVec, ANGLEVECTOR_FORWARD, fVec);
    
    xs_vec_mul_scalar(fVec, 500.0, fVec);

    new Float:fOrigin[3];
    pev(id, pev_origin, fOrigin);
    
    new Float:fView[3];
    pev(id, pev_view_ofs, fView);
    
    xs_vec_add(fOrigin, fView, fOrigin);
    xs_vec_add(fOrigin, fVec, fVec);

    new Float:start[3], Float:end[3];

    xs_vec_copy(fOrigin, start);
    xs_vec_copy(fVec, end);


    // SNIPPET SOURCE: https://forums.alliedmods.net/showpost.php?p=1399838&postcount=5
    // Traceline through multiple obstacles, in this case through player and anchor, then hit what's behind them.
    new iTraceHit; // hitted entity
    new iEntToIgnore = id; // this would change at every trace
    new iTraceHandle = create_tr2(); // trace handle
    new iMaxTraces = 3, iCurTraceNum = 0;

    while(engfunc(EngFunc_TraceLine, start, end, IGNORE_MONSTERS | IGNORE_MISSILE, iEntToIgnore, iTraceHandle)) // will always return 1, see engfunc.cpp
    {

        iTraceHit = get_tr2(iTraceHandle, TR_pHit); // getting hitted entity
        
        if(get_global_float(GL_trace_fraction) >= 1.0) // the traceline finished at the original end position, so we will stop here
            break;

        if(iTraceHit != ent)
            break;
        
        // the next traceline will start at the end of the last one
        iEntToIgnore = iTraceHit;
        get_tr2(iTraceHandle, TR_vecEndPos, start);

        if(iMaxTraces <= iCurTraceNum)
            break;
        else
            iCurTraceNum ++;
    }

    get_tr2(iTraceHandle, TR_vecEndPos, hit); // out of the loop, this will get the last position of the last traceline. you can use a beam effect or something if you want

    new ent = get_tr2(iTraceHandle, TR_pHit, hit);
    new resultType = TR_RESULT_NONE;
    if(is_valid_ent(ent)) {
        for(new i = 0; i < MAX_BOARDS && resultType == TR_RESULT_NONE; i ++) {
            if(ent == g_iBoardEntity[i]) {
                boardIndex = pev(ent, PEV_BOARD_INDEX);
                resultType = TR_RESULT_BOARD;
                // server_print("ent %d %d", ent, g_iBoardEntity[i]);
            } else {
                for(new j = 0; j < 4; j ++) {
                    new menuItemEnt = g_iPromotionMenuEntities[i][j];
                    if(ent == menuItemEnt /* && is_valid_ent(menuItemEnt) */) {
                        promotion_menu_item_entity = menuItemEnt;
                        new_rank = model_id_to_rank(pev(menuItemEnt, pev_body));
                        boardIndex = pev(menuItemEnt, PEV_BOARD_INDEX);
                        resultType = TR_RESULT_PROMOTION_MENU_ITEM;
                    }
                }
            }
        }

        for(new i = 0; i < MAX_BOARDS && resultType == TR_RESULT_NONE; i ++) {
            for(new j = 0; j < BOARD_ROWS && resultType == TR_RESULT_NONE; j++) {
                for(new l = 0; l < BOARD_COLUMNS && resultType == TR_RESULT_NONE; l++) {
                    new p = g_iPieceEntities[i][j][l];
                    if(ent == p) {
                        boardIndex = pev(p, PEV_BOARD_INDEX);
                        resultType = TR_RESULT_BOARD;
                        // ent = g_iBoardEntity[found];
                    }
                }
            }
        }

        switch(resultType) {
            case TR_RESULT_NONE: {
                return TracelineResult:TR_RESULT_NONE;
            }
            case TR_RESULT_BOARD: {
                x = floatround((hit[0] - (g_fBoardCorner[boardIndex][0])) / SQUARE_SIZE, floatround_floor);
                y = floatround((hit[1] - (g_fBoardCorner[boardIndex][1])) / SQUARE_SIZE, floatround_floor);

                // new Float:p[3];
                // get_tr2(ptr, TR_vecEndPos, p);
                // entity_set_origin(g_iGlowEnt, hit);
                // entity_set_origin(g_iGlowEnt, hit);
                // server_print("you looking at board: %d, x: %d y: %d", boardIndex, x, y);
                
                if(x < 0 || y < 0 || x > BOARD_ROWS-1 || y > BOARD_COLUMNS-1) return TracelineResult:TR_RESULT_NONE;
                return TracelineResult:TR_RESULT_BOARD;
            }
            case TR_RESULT_PROMOTION_MENU_ITEM: {
                return TracelineResult:TR_RESULT_PROMOTION_MENU_ITEM;
            }
        }
    }


    // for(new i = 0; i < 8; i ++) {
    //     for(new i = 0; i < 8; i ++) {

    //     }
    // }

    free_tr2(iTraceHandle); // freeing the tracehandle 
    
    // xs_vec_copy(hit, end);
    // end[2] += 500.0;
    // Create_Line(ent, hit, end);
    return TracelineResult:TR_RESULT_NONE;
}

render_pieces(boardIndex) {

    new piece[PieceStruct], pieceEnt;
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {
            pieceEnt = g_iPieceEntities[boardIndex][x][y];
            if(!is_valid_ent(pieceEnt)) continue;

            piece = get_piece_data_by_id(pev(pieceEnt, PEV_PIECE_ID));
            if(piece[Status] == Captured) {
                remove_entity(pieceEnt);
                g_iPieceEntities[boardIndex][x][y] = -1;
                // server_print("piece entity deleted");
                continue;
            }
        }
    }
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {
            pieceEnt = g_iPieceEntities[boardIndex][x][y];
            if(!is_valid_ent(pieceEnt)) continue;

            piece = get_piece_data_by_id(pev(pieceEnt, PEV_PIECE_ID));

            // is there promoted pawn
            if(model_id_to_rank(pev(pieceEnt, pev_body)) == Pawn
            && piece[Rank] != Pawn) {
                // server_print("render: promoted pawn");
                set_pev(pieceEnt, pev_body, rank_to_model_id(piece[Rank]));
                continue;
            }

            if(x != piece[Row] || y != piece[Column]) {
                new Float:origin[3];
                // pev(ent, pev_origin, origin);
                origin[0] = g_fBoardCorner[boardIndex][0] + piece[Row] * SQUARE_SIZE + (SQUARE_SIZE/2);
                origin[1] = g_fBoardCorner[boardIndex][1] + piece[Column] * SQUARE_SIZE + (SQUARE_SIZE/2);
                origin[2] = g_fBoardCorner[boardIndex][2]; 
                set_pev(pieceEnt, pev_origin, origin);

                g_iPieceEntities[boardIndex][x][y] = -1;
                g_iPieceEntities[boardIndex][piece[Row]][piece[Column]] = pieceEnt;
                continue;
            }
        }
    }
}

create_board_entities_dbg(boardIndex) {


    new ent = g_iBoardEntity[boardIndex];

    new Float:origin[3];
    pev(ent, pev_origin, origin);

    g_fBoardCorner[boardIndex][0] = origin[0] - BOARD_SIZE[0]/2.0;
    g_fBoardCorner[boardIndex][1] = origin[1] - BOARD_SIZE[1]/2.0;
    g_fBoardCorner[boardIndex][2] = origin[2] + BOARD_SIZE[2];

    // create entities for pieces
    new piece[PieceStruct];
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {
            new pieceId = g_PiecesMatrix[boardIndex][x][y];
            if(pieceId == InvalidID) continue;
            piece = g_PiecesList[boardIndex][pieceId];
            new glowEnt = create_glow();
            g_iPieceEntities[boardIndex][x][y] = glowEnt;

            origin[0] = g_fBoardCorner[boardIndex][0] + x * SQUARE_SIZE + (SQUARE_SIZE/2);
            origin[1] = g_fBoardCorner[boardIndex][1] + y * SQUARE_SIZE + (SQUARE_SIZE/2);
            origin[2] = g_fBoardCorner[boardIndex][2];
            set_pev(glowEnt, pev_origin, origin);
            set_pev(glowEnt, PEV_BOARD_INDEX, boardIndex);
            set_pev(glowEnt, PEV_PIECE_ID, piece[Id]);

            entity_set_model(glowEnt, g_szChessPiecesModel);
            new Float:angles[3];
            pev(glowEnt, pev_angles, angles);
            if(piece[Color] == White) 
                angles[1] += 90.0;
            else
                angles[1] -= 90.0;
            set_pev(glowEnt, pev_angles, angles);
            set_pev(glowEnt, pev_body, rank_to_model_id(piece[Rank]));
            if(piece[Color] != White) 
                set_pev(glowEnt, pev_skin, 1);
            set_rendering(glowEnt, kRenderFxNone, 155, 111, 0, kRenderNormal, 255);
        }
    }

    return ent;
}

create_promotion_menu(boardIndex, pawnIndex) {

    new piece[PieceStruct];
    piece = get_piece_data_by_id(pawnIndex);
    new px = piece[Row];
    new py = piece[Column];
    // server_print("create_promotion_menu at %d %d, pawnIndex %d", px, py, pawnIndex);
    new pieceEnt = g_iPieceEntities[boardIndex][px][py];
    new pieceColor = piece[Color];

    new Float:origin[3];
    pev(pieceEnt, pev_origin, origin);
    for(new x = 0; x < 4; x++) {

        origin[2] += 12.0;

        new menuItemEnt = create_promotion_menu_item_entity(origin);

        g_iPromotionMenuEntities[boardIndex][x] = menuItemEnt;

        set_pev(menuItemEnt, PEV_BOARD_INDEX, boardIndex);
        set_pev(menuItemEnt, pev_solid, SOLID_BBOX);
        set_pev(menuItemEnt, PEV_PIECE_ID, piece[Id]);
        // set_pev(menuItemEnt, pev_scale, 1.0);

        new Float:angles[3];
        pev(menuItemEnt, pev_angles, angles);
        angles[1] = pieceColor == White ? -90.0 : 90.0;
        set_pev(menuItemEnt, pev_angles, angles);

        if(pieceColor != White) set_pev(menuItemEnt, pev_skin, 1);

        set_pev(menuItemEnt, pev_body, 4-x);
    }
}

delete_promotion_menu(boardIndex) {
    for(new x = 0; x < 4; x++) {
        if(is_valid_ent(g_iPromotionMenuEntities[boardIndex][x]))
            remove_entity(g_iPromotionMenuEntities[boardIndex][x]);
        g_iPromotionMenuEntities[boardIndex][x] = 0;
    }
}

model_id_to_rank(body_id) {
    new rank = 0;
    switch(body_id) {
        case 0: rank = Pawn;
        case 1: rank = Knight;
        case 2: rank = Bishop;
        case 3: rank = Rook;
        case 4: rank = Queen;
        case 5: rank = King;
    }
    return rank;
}

rank_to_model_id(rank) {
    new body_id = 1;
    switch(rank) {
        case Pawn: body_id = 0;
        case Knight: body_id = 1;
        case Bishop: body_id = 2;
        case Rook: body_id = 3;
        case Queen: body_id = 4;
        case King: body_id = 5;
    }
    return body_id;
}