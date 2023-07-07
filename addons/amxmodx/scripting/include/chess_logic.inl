/*
    перед поиском клетки для хода фигуры:
        проверить: нашему королю сейчас шах?
            если шах от 2ух фигур:
                возможен только ход королём (бей или беги, так же проверять с помощью симуляции)
                если фигура которой ищем ходы не король, сразу ставим 0 возможных ходов
                иначе если это король, ищем возможные ходы
            если шах от 1ой фигуры:
                может ли наша фигура: 
                    съесть атакующего?
                        если да, записываем этот ход
                    закрыть собой линию атаки на короля?
                    (симулируем ходы, каждый раз проверяя закрыли ли короля, если да, одобрям такие ходы)
            иначе, когда нет шаха, для каждой клетки-кандидатa на возможный ход:
                нужно проверить, ход на клетку приведёт к шаху союзного короля?
                (симулируем ход, проверяем шах или нет)
                    если да, отклоняем клетку
                    если шаха не будет, одобряем клетку

        после хода фигуры:
            стираем кеш данных союзного короля
            проверить, получил ли оппонент шах
                если да, то обновляем кеш короля оппонента:
                    шах:ДА, отКого[2]: {фигyра1, -1}

*/

// TODO: review all getters that i made, without safety checks they throw errors everywhere...

#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <chess>
#include <chess_history.inl>

#define TURN_CHECK 0

#define set_list(%1,%2,%3) g_PiecesList[boardIndex][%1][%2] = %3
#define set_matrix(%1,%2,%3) g_PiecesMatrix[boardIndex][%1][%2] = %3

#define get(%1,%2) (g_PiecesList[boardIndex][%1][%2])
#define get_at(%1,%2,%3) (g_PiecesList[boardIndex][get_piece(%1, %2)][%3])
#define get_piece(%1,%2) (g_PiecesMatrix[boardIndex][%1][%2])
#define get_piece_data(%1,%2) (g_PiecesList[boardIndex][g_PiecesMatrix[boardIndex][%1][%2]])
#define get_piece_data_by_id(%1) (g_PiecesList[boardIndex][%1])

#define is_square_empty(%1,%2) (get_piece(%1, %2) == InvalidID)


#define is_square_exist(%1,%2) (%1 >= 0 && %1 < BOARD_ROWS && %2 >= 0 && %2 < BOARD_COLUMNS)
#define is_valid_turn(%1,%2) (TURN_CHECK == 0 ? true : get(get_piece(%1, %2), Color) == g_iTurn[boardIndex])
#define is_ally_at(%1,%2,%3) (!is_square_empty(%1, %2) && get(get_piece(%1, %2), Color) == %3)
#define is_opponent_at(%1,%2,%3) (!is_square_empty(%1, %2) && get(get_piece(%1, %2), Color) != %3)
#define is_king_at(%1,%2) (!is_square_empty(%1, %2) && get(get_piece(%1, %2), Rank) == King)
#define is_rook_moved(%1,%2) (ArrayFindValue(g_aMovedRooks[boardIndex], get_at(%1, %2, Id)) != -1)
#define is_rook_moved_by_id(%1) (ArrayFindValue(g_aMovedRooks[boardIndex], %1) != -1)


new g_PiecesMatrix[MAX_BOARDS][BOARD_ROWS][BOARD_COLUMNS]; // makes connection from X,Y to pieceId
new g_PiecesList[MAX_BOARDS][PIECE_COUNT][PieceStruct];    // makes connection from pieceId to PieceStruct and thus to X,Y
new g_Boards[MAX_BOARDS][BoardStruct];
// new g_aChecks[MAX_BOARDS]; // TODO: init with Arrays. these will be updated each move
new g_iLongMovedPawnID[MAX_BOARDS]; // keeps id of the pawn that did double move and no one else moved after that (for En Passant capture)
new bool:g_bHasKingMoved[MAX_BOARDS][Color]; 
new Array:g_aMovedRooks[MAX_BOARDS]; 
new g_iTurn[MAX_BOARDS];

is_wait_for_promote_choice(boardIndex, &pawnIndex = -1) {
    if(!is_history_empty(boardIndex) && history_get_var_from_last(boardIndex, _:MoveType) == _:Promote) {
        pawnIndex = history_get_var_from_last(boardIndex, MovingPieceID);
        return true;
    }
    return false;
}

get_king_id(boardIndex, color) {
    for(new i = 0; i < sizeof g_PiecesList[]; i++) 
        if(get(i, Rank) == King && get(i, Color) == color) 
            return i;
    return -1;
}
// get_opposite_color(color) return color == White ? Black : White;
// is_contain_move(Array:array, MoveStruct:move) {
//     if(!array) return false;
//     new moveTmp[MoveStruct];
//     for(new i = 0; i < ArraySize(array); i++) {
//         moveTmp = ArrayGetArray(array, i, moveTmp);
        
//         for(new i = 0; i < MoveStruct; i++) {
//             if(move[i] == moveTmp[i]) {

//             }
//         }

//     }
// }

get_uninitialized_board()
{
    for(new i = 0; i < MAX_BOARDS; i++) {
        if(!g_Boards[i][IsInitialized]) {
            return i;
        }
    }
    return -1;
}

deinitialize_board(boardIndex) {
    // if(!g_Boards[boardIndex][IsInitialized]) {
    //     log_amx("ERROR: Already deinitialized board!");
    // }
    g_Boards[boardIndex][IsInitialized] = false;
}

            // TODO: should clean board from past games (after game ended with any reason)
init_new_board()
{
    new boardIndex = get_uninitialized_board();
    if(boardIndex == -1) 
        return boardIndex;
    g_Boards[boardIndex][IsInitialized] = true;
    if(!g_aMovedRooks[boardIndex])
        g_aMovedRooks[boardIndex] = ArrayCreate();

    new pieceId = 0;
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {

            set_matrix(x, y, InvalidID) // clear previous data
            // ignore mid rows 2,3,4,5
            if(y > 1 && y < 6) continue;

            
            new piece[PieceStruct];
            piece[Row] = x;
            piece[Column] = y;
            piece[Id] = pieceId;

            // set pieces color
            if(y <= 1) {
                piece[Color] = White;
            } else {
                piece[Color] = Black;
            }

            // set pawns
            if(y == 1 || y == 6) {
                piece[Rank] = Pawn;
            }
            if((y == 0 || y == 7)) {
                // set bishops
                if(x == 2 || x == 5) {
                    piece[Rank] = Bishop;
                }
                // set knights
                if(x == 1 || x == 6) {
                    piece[Rank] = Knight;
                }
                // set rooks
                if(x == 0 || x == 7) {
                    piece[Rank] = Rook;
                }
                // set queens
                if(x == 3) {
                    piece[Rank] = Queen;
                }
                // set kings
                if(x == 4) {
                    piece[Rank] = King;
                }
            }

            g_PiecesList[boardIndex][pieceId] = piece;
            set_matrix(x, y, pieceId) 
            pieceId ++;
        }
    }
    return boardIndex;
}

// logic_update(boardIndex, from_x, from_y, to_x, to_y) {

// }

do_move(boardIndex, from_x, from_y, to_x, to_y) {
    new movingPieceId = get_piece(from_x, from_y);
    if(!is_valid_move(boardIndex, movingPieceId, from_x, from_y, to_x, to_y)) return false;

    new move[MoveStruct];
    // client_print(0, print_chat, "before crtmove MovingPieceID %d", move[MovingPieceID]);
    create_move(move, boardIndex, movingPieceId, from_x, from_y, to_x, to_y);
    // client_print(0, print_chat, "after crtmove MovingPieceID %d", move[MovingPieceID]);

    // additional validation (king check)
    print_board(boardIndex);
    new color = get(movingPieceId, Color);
    do_move_unvalidated(boardIndex, movingPieceId, from_x, from_y, to_x, to_y, move);
    print_board(boardIndex);
    if(is_king_checked(boardIndex, color)) {
        undo_last_move(boardIndex);
        client_print(0, print_chat, "king checked color %d", color);
        print_board(boardIndex);
        return false;
    }
    print_board(boardIndex);

    return true;
}

do_move_unvalidated(boardIndex, movingPieceId, from_x, from_y, to_x, to_y, move[MoveStruct]) {
    // TODO: castling, capture, move, vzyatie na prohode...
    switch(move[MoveType]) {
        case Move: {
            set_matrix(to_x, to_y, movingPieceId)
            set_matrix(from_x, from_y, InvalidID) 
            set_list(movingPieceId, Row, to_x)
            set_list(movingPieceId, Column, to_y)
        }
        case PawnLongMove: {
            g_iLongMovedPawnID[boardIndex] = movingPieceId;
            set_matrix(to_x, to_y, movingPieceId)
            set_matrix(from_x, from_y, InvalidID) 
            set_list(movingPieceId, Row, to_x)
            set_list(movingPieceId, Column, to_y)
        }
        case Capture: {
            new capturedPieceIndex = get_piece(to_x, to_y);

            set_matrix(to_x, to_y, movingPieceId)
            set_matrix(from_x, from_y, InvalidID) 
            set_list(movingPieceId, Row, to_x)
            set_list(movingPieceId, Column, to_y)

            set_list(capturedPieceIndex, Status, Captured)
        }
        case CaptureOnPassing: {
            set_matrix(to_x, to_y, movingPieceId)
            set_matrix(from_x, from_y, InvalidID) 
            set_list(movingPieceId, Row, to_x)
            set_list(movingPieceId, Column, to_y)

            new cx = move[Row2];
            new cy = move[Column2];
// client_print(0, print_chat, "cxcy %d %d", cx, cy);
            new capturedPieceIndex = get_piece(cx, cy);
            set_list(capturedPieceIndex, Status, Captured)
            set_matrix(cx, cy, InvalidID)
        }
        case Castling: {
            new kx = move[EndRow];
            new ky = move[EndColumn];
            new rx = move[Row2];
            new ry = move[Column2];
            new rookId = move[OtherPieceID];
            set_matrix(kx, ky, movingPieceId)
            set_matrix(rx, ry, rookId) 
            set_list(movingPieceId, Row, kx)
            set_list(movingPieceId, Column, ky)
            set_list(rookId, Row, rx)
            set_list(rookId, Column, ry)
            set_matrix(from_x, from_y, InvalidID) 
            set_matrix(to_x, to_y, InvalidID) 
        }
        case Promote: {
            set_matrix(to_x, to_y, movingPieceId)
            set_matrix(from_x, from_y, InvalidID) 
            set_list(movingPieceId, Row, to_x)
            set_list(movingPieceId, Column, to_y)

            new capturedPieceIndex = move[OtherPieceID];
            if(capturedPieceIndex != InvalidID) {
                set_list(capturedPieceIndex, Status, Captured)
                set_matrix(to_x, to_y, InvalidID)
            }
        }
    }
    // TODO: possible bug, maybe after castling rook is not marked as moved
    if(get(movingPieceId, Rank) == Rook) {
        if(!is_rook_moved_by_id(movingPieceId)) 
            ArrayPushCell(g_aMovedRooks[boardIndex], movingPieceId);
    }

    if(get(movingPieceId, Rank) == King) {
        new color = get(movingPieceId, Color);
        g_bHasKingMoved[boardIndex][color] = true; 
    }

    if(move[MoveType] != PawnLongMove) {
        g_iLongMovedPawnID[boardIndex] = InvalidID;
    }
    
    g_iTurn[boardIndex] = g_iTurn[boardIndex] == White ? Black : White;

    history_push(boardIndex, move);

    return true;
}

undo_last_move(boardIndex) {
    new deletedMove[MoveStruct];
    new newLastMove[MoveStruct];
    history_pop(boardIndex, deletedMove);
    console_print(0, "movingPieceId: %d, deletedMove[OTHERpiecID] %d", deletedMove[MovingPieceID], deletedMove[OtherPieceID]);
    // TODO: this fails at 1st deletedMove...
    new isHistoryEmpty = is_history_empty(boardIndex);
    if(!isHistoryEmpty)
        history_get_last(boardIndex, newLastMove);
    undo_move(boardIndex, deletedMove, newLastMove, !isHistoryEmpty);
}

undo_move(boardIndex, deletedMove[MoveStruct], newLastMove[MoveStruct], hasNewLastMove) {
    new from_x = deletedMove[StartRow], from_y = deletedMove[StartColumn];
    new to_x = deletedMove[EndRow], to_y = deletedMove[EndColumn];

    new movingPieceId = deletedMove[MovingPieceID];
    print_piece(boardIndex, movingPieceId);
    server_print("undo_move called");

    // TODO: promote, castling, capture, deletedMove, vzyatie na prohode...
    switch(deletedMove[MoveType]) {
        case Move: {
            set_matrix(to_x, to_y, InvalidID)
            set_matrix(from_x, from_y, movingPieceId) 
            set_list(movingPieceId, Row, from_x)
            set_list(movingPieceId, Column, from_y)
        }
        case PawnLongMove: {
            set_matrix(to_x, to_y, InvalidID)
            set_matrix(from_x, from_y, movingPieceId) 
            set_list(movingPieceId, Row, from_x)
            set_list(movingPieceId, Column, from_y)
        }
        case Capture: {
            new capturedPieceIndex = deletedMove[OtherPieceID];

            set_matrix(to_x, to_y, capturedPieceIndex)
            set_matrix(from_x, from_y, movingPieceId) 
            // client_print(0, print_chat, "c %d crank %d m %d mrank %d ", capturedPieceIndex, movingPieceId,
            //     g_PiecesList[boardIndex][capturedPieceIndex][Rank],
            //     g_PiecesList[boardIndex][movingPieceId][Rank]
            // );
            set_list(movingPieceId, Row, from_x)
            set_list(movingPieceId, Column, from_y)

            set_list(capturedPieceIndex, Row, to_x)
            set_list(capturedPieceIndex, Column, to_y)

            set_list(capturedPieceIndex, Status, Battle)
        }
        case CaptureOnPassing: {
            set_matrix(to_x, to_y, InvalidID);
            set_matrix(from_x, from_y, movingPieceId);
            set_list(movingPieceId, Row, from_x);
            set_list(movingPieceId, Column, from_y);

            new cx = deletedMove[Row2];
            new cy = deletedMove[Column2];
// client_print(0, print_chat, "cxcy %d %d", cx, cy);
            new capturedPieceIndex = deletedMove[OtherPieceID];
            set_list(capturedPieceIndex, Status, Battle);
            set_matrix(cx, cy, capturedPieceIndex);
        }
        case Castling: {
            new rx = deletedMove[Row2];
            new ry = deletedMove[Column2];
            new rookId = deletedMove[OtherPieceID];
            set_matrix(from_x, from_x, movingPieceId);
            set_matrix(to_x, to_x, rookId);
            set_list(movingPieceId, Row, from_x);
            set_list(movingPieceId, Column, from_y);
            set_list(rookId, Row, to_x);
            set_list(rookId, Column, to_y);
        }
        case Promote: {
            set_matrix(to_x, to_y, InvalidID)
            set_matrix(from_x, from_y, movingPieceId) 
            set_list(movingPieceId, Row, from_x)
            set_list(movingPieceId, Column, from_y)

            new capturedPieceIndex = deletedMove[OtherPieceID];
            if(capturedPieceIndex != InvalidID) {
                set_list(capturedPieceIndex, Status, Battle);
                set_matrix(to_x, to_y, capturedPieceIndex);
            }
        }
    }

    if(movingPieceId == Rook 
    && deletedMove[IsFirstMove]) {
        new removeIndex = ArrayFindValue(g_aMovedRooks[boardIndex], movingPieceId);
        ArrayDeleteItem(g_aMovedRooks[boardIndex], removeIndex);
    }

    if(movingPieceId == King
    && deletedMove[IsFirstMove]) {
        new color = get(movingPieceId, Color);
        g_bHasKingMoved[boardIndex][color] = false; 
    }

    g_iLongMovedPawnID[boardIndex] = InvalidID;
    if(hasNewLastMove) {
        if(newLastMove[MoveType] == PawnLongMove) {
            g_iLongMovedPawnID[boardIndex] = newLastMove[MovingPieceID];
        }
    }
    
    g_iTurn[boardIndex] = get(movingPieceId, Color) == Black ? Black : White;

}

bool:is_valid_move(boardIndex, movingPieceId, from_x, from_y, to_x, to_y) {
    server_print("is_valid_move called");
    if(!is_square_exist(from_x, from_y) 
        || !is_square_exist(to_x, to_y)
        || is_square_empty(from_x, from_y)
        || !is_valid_turn(from_x, from_y)
        || !is_valid_piece_move(boardIndex, movingPieceId, from_x, from_y, to_x, to_y)
        || is_castling(boardIndex, movingPieceId, from_x, from_y, to_x, to_y)
        && !is_valid_castling(boardIndex, movingPieceId, from_x, from_y, to_x, to_y)
    ) {
        server_print("is_valid_move: false");
        return false;
    }

    server_print("is_valid_move: true");
    return true;
}

bool:is_castling(boardIndex, movingPieceId, from_x, from_y, to_x, to_y) {

    if(
        get_at(from_x, from_y, Rank) == King
        && is_ally_at(to_x, to_y, get_at(from_x, from_y, Color))
        && get_at(to_x, to_y, Rank) == Rook
    ) { 
        server_print("is_castling: true");
        return true; 
    }

    server_print("is_castling: false");
    return false;
}

bool:is_valid_castling(boardIndex, movingPieceId, from_x, from_y, to_x, to_y) {
    new bool:isCastling;
    if(
        get_at(from_x, from_y, Rank) == King
        && !g_bHasKingMoved[boardIndex][get_at(from_x, from_y, Color)]
        && is_ally_at(to_x, to_y, get_at(from_x, from_y, Color))
        && get_at(to_x, to_y, Rank) == Rook
        && !is_rook_moved_by_id(get_at(to_x, to_y, Id))
    ) {
        new dir = to_x == 0 ? -1 : 1;
        new kx = from_x;
        for(new i = 0; i < 2; i ++) {
            kx += dir;
            if(is_square_empty(kx, to_y) 
            && !is_square_under_attack(boardIndex, kx, to_y, get_at(from_x, from_y, Color))) {
                isCastling = true;
            } else {
                isCastling = false;
                break;
            }
        }
    }
    server_print("is_valid_castling returns: %b", isCastling);
    return isCastling;
}

bool:is_king_checked(boardIndex, color) {
    // TODO: verify that id is found
    new kingId = get_king_id(boardIndex, color);
    new kingData[PieceStruct]; 
    kingData = get_piece_data_by_id(kingId);
    new kx = kingData[Row], ky = kingData[Column];
    return is_square_under_attack(boardIndex, kx, ky, kingData[Color]);
}

bool:is_square_under_attack(boardIndex, kx, ky, color) {
    {
        server_print("is_square_under_attack called");
        for(new i = -1; i <= 1; i++) {
            for(new j = -1; j <= 1; j++) {
                new x = kx+i;
                new y = ky+j;
                // console_print(0, "kat %d %d, csqr %d %d diagonal ", kx, ky, x, y);

                // console_print(0, "exists %b x %d y %d protected ", is_square_exist(x, y), x, y);
                if(is_square_exist(x, y) 
                   && is_opponent_at(x, y, color)
                   && get_at(x, y, Rank) == King) {
                        console_print(0, "CHECK AT ^^");
                        return true;
                }
            }
        }
    }

    // pawn, queen and bishop here (diagonals)
    {
        new dirs[][] = {
            { 1,  1},
            { 1, -1},
            {-1, -1},
            {-1,  1}
        };
        new offsets[4][2];
        offsets = dirs;
        new diagonalIndex = 0;
        new bool:is_protected[4];
        // console_print(0, "is bishops check king?");
        for(new i = 0; i < 28; i++) {
            diagonalIndex = i % 4;
            new x = kx+offsets[diagonalIndex][0];
            new y = ky+offsets[diagonalIndex][1];
            // console_print(0, "kat %d %d, csqr %d %d", kx, ky, x, y);

            offsets[diagonalIndex][0] += dirs[diagonalIndex][0];
            offsets[diagonalIndex][1] += dirs[diagonalIndex][1];

            // diagonal is protected
            // console_print(0, "exists %b x %d y %d protected %d", is_square_exist(x, y), x, y, is_protected[diagonalIndex]);
            if(is_protected[diagonalIndex]) continue;
            if(!is_square_exist(x, y) 
            || is_ally_at(x, y, color)
            // assuming that i > 3 guarantees that pawn is too far to check the king
            || (is_opponent_at(x, y, color) 
                && (get_at(x, y, Rank) == Pawn && i > 3
                    || (get_at(x, y, Rank) != Queen
                        && get_at(x, y, Rank) != Bishop
                        && get_at(x, y, Rank) != Pawn)))) {
                is_protected[diagonalIndex] = true;
                // console_print(0, "PROTECTED PAWWWWWWWWWWWWWWNNNNNN^^");
                continue;
            }

            // console_print(0, "PAWWWWWWWWWWWWWWNNNNNN^^");
            if(is_opponent_at(x, y, color)) {

                if( get_at(x, y, Rank) == Bishop 
                    || get_at(x, y, Rank) == Queen) {
                    console_print(0, "CHECK AT ^^");
                    return true;
                }

                if(get_at(x, y, Rank) == Pawn) {
                    // new pawnDir = color == White ? 1 : -1;
                    if(color == White && dirs[diagonalIndex][1] == 1
                    || color == Black && dirs[diagonalIndex][1] == -1) {
                        console_print(0, "CHECK AT ^^");
                        return true;
                    }
                }
            }
        }
    }

     // queen and rook here (verticals and horizonals)
    {
        new dirs[][] = {
            { 1,  0},
            {-1,  0},
            { 0,  1},
            { 0, -1}
        };
        new offsets[4][2];
        offsets = dirs;
        new diagonalIndex = 0;
        new is_protected[4];
        // console_print(0, "is rook check king?");
        for(new i = 0; i < 28; i++) {
            diagonalIndex = i % 4;
            new x = kx+offsets[diagonalIndex][0];
            new y = ky+offsets[diagonalIndex][1];
            // console_print(0, "kat %d %d, csqr %d %d diagonal %d", kx, ky, x, y, diagonalIndex);

            offsets[diagonalIndex][0] += dirs[diagonalIndex][0];
            offsets[diagonalIndex][1] += dirs[diagonalIndex][1];

            // diagonal is protected
            // console_print(0, "exists %b x %d y %d protected %d", is_square_exist(x, y), x, y, is_protected[diagonalIndex]);
            if(is_protected[diagonalIndex]) continue;
            if(!is_square_exist(x, y) 
            || is_ally_at(x, y, color)
            || (is_opponent_at(x, y, color) 
                && (get_at(x, y, Rank) != Queen
                    && get_at(x, y, Rank) != Rook))) {
                is_protected[diagonalIndex] = true;
                // console_print(0, "protected");
                continue;
            }

            if(is_opponent_at(x, y, color)) {

                if( get_at(x, y, Rank) == Rook 
                    || get_at(x, y, Rank) == Queen) {
                    console_print(0, "CHECK AT ^^");
                    return true;
                }
            }
        }
    }

    // knights here!
    {
        new dirs[][] = {
            { 2,  1},
            { 2, -1},
            {-2,  1},
            {-2, -1},
            { 1,  2},
            { 1, -2},
            {-1,  2},
            {-1, -2}
        };
        new offsets[8][2];
        offsets = dirs;
        new diagonalIndex = 0;
        new is_protected[8];
        // console_print(0, "is knight check king?");
        for(new i = 0; i < 8; i++) {
            diagonalIndex = i % 8;
            new x = kx+offsets[diagonalIndex][0];
            new y = ky+offsets[diagonalIndex][1];
            // console_print(0, "kat %d %d, csqr %d %d diagonal %d", kx, ky, x, y, diagonalIndex);

            offsets[diagonalIndex][0] += dirs[diagonalIndex][0];
            offsets[diagonalIndex][1] += dirs[diagonalIndex][1];

            // diagonal is protected
            // console_print(0, "exists %b x %d y %d protected %d", is_square_exist(x, y), x, y, is_protected[diagonalIndex]);
            if(is_protected[diagonalIndex]) continue;
            if(!is_square_exist(x, y) 
            || is_ally_at(x, y, color)
            || (is_opponent_at(x, y, color) 
                && get_at(x, y, Rank) != Knight)) {
                is_protected[diagonalIndex] = true;
                // console_print(0, "protected");
                continue;
            }

            if(is_opponent_at(x, y, color)) {

                if( get_at(x, y, Rank) == Knight) {
                    console_print(0, "CHECK AT ^^");
                    return true;
                }
            }
        }
    }
    server_print("is_square_under_attack returned false");
    return false;
}

bool:is_valid_piece_move(boardIndex, movingPieceId, from_x, from_y, to_x, to_y) {
    server_print("is_valid_piece_move called")
    new piece[PieceStruct];
    piece = get_piece_data_by_id(movingPieceId);
    print_piece(boardIndex, movingPieceId);
    switch(piece[Rank]) {
        case Pawn: {
            // offsets relative to white pawn's position
            new moves[4][2] = {{0, 2}, {0, 1}, {-1, 1}, {1, 1}}; 
            new direction = piece[Color] == White ? 1 : -1;
            new found = -1;
            for(new i = 0; i < sizeof moves; i++) {
                if(to_x == from_x + moves[i][0] 
                && to_y == from_y + moves[i][1] * direction) {
                    found = i;
                    break;
                }
            }
            if(found == -1) return false;
            if(is_ally_at(to_x, to_y, piece[Color])) return false;

            server_print("pawn move validation");

            switch(found) {
                case 0: 
                        // double move 
                        if((from_y == 1 || from_y == 6) 
                        && is_square_empty(from_x, from_y + direction) 
                        && is_square_empty(from_x, from_y + direction * 2))
                            return true;
                case 1: 
                        // forward move
                        if(found == 1 && is_square_empty(to_x, to_y))
                            return true;
                case 2..3: {

                        server_print("is_opponent_at: %b, is_king_at: %b", is_opponent_at(to_x, to_y, piece[Color]), is_king_at(to_x, to_y));
                        // diagonal move
                        if(is_can_perform_en_passant(boardIndex, movingPieceId, from_y, to_x)
                        || (is_opponent_at(to_x, to_y, piece[Color]) 
                            && !is_king_at(to_x, to_y)))
                            return true;
                }
            }

            server_print("pawn move is invalid");
        }
        case Bishop: {
            new dist_x = abs(to_x - from_x);
            new dist_y = abs(to_y - from_y);
            // check if it's a diagonal move, assuming valid x,y
            if(dist_x != dist_y) return false;

            new direction_x = to_x - from_x < 0 ? -1 : 1;
            new direction_y = to_y - from_y < 0 ? -1 : 1;

            new mx = from_x + direction_x;
            new my = from_y + direction_y;
            for(new i = 0; i < dist_x; i++) {

                if(is_ally_at(mx, my, piece[Color])
                || is_king_at(mx, my))
                    return false;
                if(to_x == mx && to_y == my)
                    return true;
                mx += direction_x;
                my += direction_y;
            }
        }

        case Knight: {
            new moves[8][2] = {
                {1, 2}, {2, 1}, {-1, -2}, {-2, -1},
                {-1, 2}, {-2, 1}, {1, -2}, {2, -1}
            }; 
            new found = -1;
            for(new i = 0; i < sizeof moves; i++) {
                if(to_x == from_x + moves[i][0] 
                && to_y == from_y + moves[i][1]) {
                    found = i;
                    break;
                }
            }
            if(found == -1) return false;

            if(is_ally_at(to_x, to_y, piece[Color])
            || is_king_at(to_x, to_y))
                return false;
            return true;
        }

        case Rook: {
            new dist_x = abs(to_x - from_x);
            new dist_y = abs(to_y - from_y);
            // return if not vertical or horizontal move 
            if(from_x != to_x && from_y != to_y) return false;

            new direction_x = to_x - from_x < 0 ? -1 : 1;
            new direction_y = to_y - from_y < 0 ? -1 : 1;
            direction_x = dist_x == 0 ? 0 : direction_x;
            direction_y = dist_y == 0 ? 0 : direction_y;

            new mx = from_x + direction_x;
            new my = from_y + direction_y;
            // client_print(0, print_chat, "rookmove %d %d mxmy:%d %d", direction_x, direction_y, mx, my);
            // client_print(0, print_chat, "dist_x", );
            for(new i = 0; i < dist_x + dist_y; i++) {

                if(is_ally_at(mx, my, piece[Color])
                || is_king_at(mx, my))
                    return false;
                if(to_x == mx && to_y == my)
                    return true;
                mx += direction_x;
                my += direction_y;
            }
        }

        case Queen: {

            new dist_x = abs(to_x - from_x);
            new dist_y = abs(to_y - from_y);
            // return if not vertical, horizontal or diagonal move 
            if((from_x != to_x && from_y != to_y) && (dist_x != dist_y)) return false;

            new direction_x = to_x - from_x < 0 ? -1 : 1;
            new direction_y = to_y - from_y < 0 ? -1 : 1;
            direction_x = dist_x == 0 ? 0 : direction_x;
            direction_y = dist_y == 0 ? 0 : direction_y;

            new mx = from_x + direction_x;
            new my = from_y + direction_y;
            // client_print(0, print_chat, "rookmove %d %d mxmy:%d %d", direction_x, direction_y, mx, my);
            // client_print(0, print_chat, "dist_x", );
            for(new i = 0; i < dist_x + dist_y; i++) {

                if(is_ally_at(mx, my, piece[Color])
                || is_king_at(mx, my))
                    return false;
                if(to_x == mx && to_y == my)
                    return true;
                mx += direction_x;
                my += direction_y;
            }
        }

        case King: {
            
            for(new i = -1; i <= 1; i++) {
                for(new o = -1; o <= 1; o++) {
                    if(from_x + i == to_x && from_y + o == to_y) {
                        if(is_ally_at(to_x, to_y, piece[Color]))
                            return false;
                        return true;
                    }
                    // for castling
                    if((to_x == 0 && to_y == 0 
                    || to_x == 7 && to_y == 0 
                    || to_x == 0 && to_y == 7 
                    || to_x == 7 && to_y == 7)
                    && get_at(to_x, to_y, Rank) == Rook) {
                        return true;
                    }
                }
            }
        }
    }
    return false;
}

//TODO: make sure structure is filled correctly, 
// TODO: bug with king, possibly due to struct not filled correctly, or some shit with history ...
create_move(move[MoveStruct], boardIndex, movingPieceId, from_x, from_y, to_x, to_y) {
    move[StartRow] = from_x;
    move[StartColumn] = from_y;
    move[EndRow] = to_x;
    move[EndColumn] = to_y;
    move[MovingPieceID] = movingPieceId;
    console_print(0, "crtmove MovingPieceID %d", movingPieceId);
    print_piece(boardIndex, movingPieceId);

    new piece[PieceStruct];
    console_print(0, " cm %d %d ", BOARD_ROWS, BOARD_COLUMNS);
    console_print(0, " cm %d ", boardIndex);
    console_print(0, " cm %d",  g_PiecesMatrix[boardIndex][from_x][from_y]);
    piece = get_piece_data(from_x, from_y);

    if(is_can_perform_en_passant(boardIndex, movingPieceId, from_y, to_x)) {
        move[MoveType] = CaptureOnPassing;
        move[Row2] = to_x;
        move[Column2] = from_y;
        move[OtherPieceID] = get_piece(to_x, from_y);
        return;
    }

    // TODO: insert promote branch here (order is important)
    if(piece[Rank] == Pawn && (to_y == 0 || to_y == 7)) {
        move[MoveType] = Promote;
        move[CapturedRank] = None;
        if(!is_square_empty(to_x, to_y))
            move[OtherPieceID] = get_piece(to_x, to_y);
        server_print("create_move promote");
        return;
    }

    if(piece[Rank] == Rook
    && !is_rook_moved_by_id(movingPieceId)
    || piece[Rank] == King
    && !g_bHasKingMoved[boardIndex][piece[Color]]) {
        move[IsFirstMove] = true;
    }

    if(is_castling(boardIndex, movingPieceId, from_x, from_y, to_x, to_y)) {
        new dir;
        if(to_x == 0) dir = -1;
        if(to_x == 7) dir = 1;

        move[MoveType] = Castling;
        move[EndRow] = from_x + dir * 2;
        move[EndColumn] = to_y;
        move[Row2] = from_x + dir;
        move[Column2] = to_y;
        move[OtherPieceID] = get_piece(to_x, to_y);
        server_print("create_move castling, dir: %d", dir);
        return;
    }

    if(is_square_empty(to_x, to_y)) {
        new dist = abs(to_y - from_y);
        new is_pawn_long_move = dist == 2 && piece[Rank] == Pawn;
        if(is_pawn_long_move) {
            move[MoveType] = PawnLongMove;
        } else {
            move[MoveType] = Move;
        }
        return;
    }

    if(is_opponent_at(to_x, to_y, piece[Color])) {
        move[MoveType] = Capture;
        new capturedPieceId = get_piece(to_x, to_y);
        // move[CapturedRank] = get(capturedPieceId, Rank);
        move[OtherPieceID] = capturedPieceId;
        server_print("create_move Capture");
        return;
    }

}
print_piece(boardIndex, id) {
    new piece[PieceStruct];
    console_print(0, "pp %d", id);
    piece = get_piece_data_by_id(id);
    console_print(0, "id:%d rank:%d color:%d row:%d column:%d status:%d", piece[Id], piece[Rank], piece[Color], piece[Row], piece[Column], piece[Status]);
}
print_board(boardIndex) {
    new str[BOARD_ROWS] = {'*', ...};
    server_print("--------------------");
    for(new y = BOARD_ROWS-1; y >= 0 ; y--) {
        for(new x = 0; x < BOARD_COLUMNS; x++) {
            if(!is_square_empty(x, y)) {
                switch(get_at(x, y, Rank)) {
                    case King: str[x] = 'k';
                    case Queen: str[x] = 'q';
                    case Knight: str[x] = 'n';
                    case Rook: str[x] = 'r';
                    case Bishop: str[x] = 'b';
                    case Pawn: str[x] = 'p';
                }
                // server_print("print_board x: %d", x);
            }
        }
        server_print("%s", str);
        arrayset(str, '*', sizeof str);
    }
    server_print("--------------------");
}

// returns whether pawn at x,y can perform En Passant capture by moving at px,py
is_can_perform_en_passant(boardIndex, movingPieceId, y, px) {
    if(is_square_empty(px, y)) return false;
    new capturingPieceColor = get(movingPieceId, Color);
    return get(movingPieceId, Rank) == Pawn
        && get(get_piece(px, y), Rank) == Pawn
        && is_opponent_at(px, y, capturingPieceColor)
        && g_iLongMovedPawnID[boardIndex] == get_piece(px, y);
}
