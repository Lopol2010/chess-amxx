#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <chess>

load_position_from_fen(boardIndex, szFen[], len)
{
    new pieceId = 0;
    new x = 0, y = 7;
    new spaces = 0;
    new num[10];
    new numStartIdx;

    arrayset(g_bMovedRooks[boardIndex], true, 4);

    for(new i = 0; i < len; i++) {
        new char_l = char_to_lower(szFen[i]);

        if(szFen[i] == ' ') spaces ++; 

        switch(spaces) {
            case 0: {

                new piece[PieceStruct];
                // server_print("x %d y %d %c %c", x, y, szFen[i], char_l);

                piece[Row] = x;
                piece[Column] = y;
                piece[Id] = pieceId;
                piece[Status] = Battle;
                piece[Color] = is_char_upper(szFen[i]) ? White : Black;

                switch(char_l) {
                    case 'p': piece[Rank] = Pawn;
                    case 'n': piece[Rank] = Knight;
                    case 'b': piece[Rank] = Bishop;
                    case 'r': piece[Rank] = Rook;
                    case 'q': piece[Rank] = Queen;
                    case 'k': piece[Rank] = King;
                }
                if(piece[Rank] != None) 
                    g_PiecesList[boardIndex][pieceId] = piece;

                switch(char_l) {
                    case 'r', 'p', 'n', 'b', 'q', 'k': {
                        set_matrix(x, y, pieceId) 
                        x ++;
                        pieceId ++;
                    }
                    case '1'..'8': {
                        new skip = str_to_num(fmt("%s", szFen[i]));
                        // server_print("%s", szFen[i]);
                        for(new j = x; j < x+skip; j ++) {
                            // server_print("%d %d %d %d", x, y, j, skip);
                            set_matrix(j, y, InvalidID) // clear squares that should be empty 
                        }
                        x += skip;
                    }
                    case '/': {
                        y --;
                        x = 0;
                    }
                }
                if(x > 7) x = 0;
            }
            case 1: {
                g_iTurn[boardIndex] = szFen[i] == 'w' ? White : Black;
            }
            case 2: {
                switch(szFen[i]) {
                    case '-': {
                        g_bHasKingMoved[boardIndex][White] = true;
                        g_bHasKingMoved[boardIndex][Black] = true;
                    }
                    case 'K': g_bMovedRooks[boardIndex][0] = false;
                    case 'Q': g_bMovedRooks[boardIndex][0] = false;
                    case 'k': g_bMovedRooks[boardIndex][0] = false;
                    case 'q': g_bMovedRooks[boardIndex][0] = false;
                }
            }
            case 3: {
                // server_print("szFen[i] %c", szFen[i]);
                if(szFen[i] != '-' && szFen[i] != ' ') {
                    new x;
                    switch(szFen[i]) {
                        case 'a': x = 0;
                        case 'b': x = 1;
                        case 'c': x = 2;
                        case 'd': x = 3;
                        case 'e': x = 4;
                        case 'f': x = 5;
                        case 'g': x = 6;
                        case 'h': x = 7;
                    }

                    new y = str_to_num(fmt("%s", szFen[i + 1]));
                    i ++;

                    if(y == 2) y = 3;
                    if(y == 5) y = 4;

                    g_iLongMovedPawnID[boardIndex] = get_at(x, y, Id);
                }
            }
            case 4: {
                if(numStartIdx == 0) numStartIdx = i;
                num[i - numStartIdx] = szFen[i];
                if(szFen[i + 1] == ' ') {
                    numStartIdx = 0;
                    g_iHalfmoveClock[boardIndex] = str_to_num(num);
                    // server_print(" g_iHalfmoveClock = %s", num);
                    arrayset(num, 0, sizeof num);
                }
            }
            case 5: {
                if(numStartIdx == 0) numStartIdx = i;
                num[i - numStartIdx] = szFen[i];
                if(szFen[i + 1] == 0) {
                    numStartIdx = 0;
                    g_iFullmoveClock[boardIndex] = str_to_num(num);
                    // server_print(" g_iFullmoveClock = %s", num);
                    arrayset(num, 0, sizeof num);
                    spaces ++;
                }
            }
        }

    }
}
