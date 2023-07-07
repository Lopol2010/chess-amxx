#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <chess>

#define MAX_HITORY_LENGTH 300

// enum _:HistoryEntryType {
//   Move,
//   PawnLongMove, // when pawn moves 2 squares 
//   Capture,
//   CaptureOnPassing,
//   Castling,
//   Promote
// }

// enum _:HistoryEntry {
//     HistoryEntryType:Type,

//     MovingPieceId,
//     FromTo[4],

//     // These "Action" squares should store:
//     // if Move: none
//     // if Capture: [0] is id of captured piece, [1,2] is captured position 
//     // if Castling: [0] is rook id, [1..4] rook's FromTo 
//     // if Poromote: [0] is old Rank, [1] is new rank
//     ActionData[5],
// }

// new g_aHistory[MAX_BOARDS][MAX_HITORY_LENGTH][HistoryEntry];
// new g_aHistoryLength[MAX_BOARDS];
new Array:g_aHistory[MAX_BOARDS];
// new g_aHistoryLength[MAX_BOARDS];

// history_push(boardIndex, MoveTypeEnum:type, _MovingPieceID, _OtherPieceID, from_x, from_y, to_x, to_y, extra_x = -1, extra_y = -1, rank = -1)
history_push(boardIndex, entry[MoveStruct])
{
//     enum _:MoveStruct {
//   MoveTypeEnum:MoveType,
//   MovingPieceID,
//   OtherPieceID, // for rook castling, capture
//   // moving piece start position
//   StartRow,
//   StartColumn,
//   // moving piece end position
//   EndRow,
//   EndColumn,
//   // rook position after castling OR captured piece position
//   Row2,
//   Column2,
//   // captured piece rank OR pawn's new rank after promotion
//   Rank
// }
    if(!g_aHistory[boardIndex]) g_aHistory[boardIndex] = ArrayCreate(sizeof entry);
    
    // new entry[HistoryEntry];

    // entry[Type] = type;
    // entry[MovingPieceID] = _MovingPieceID;
    // entry[OtherPieceID] = _OtherPieceID;
    // entry[StartRow] = from_x;
    // entry[StartColumn] = from_y;
    // entry[EndRow] = to_x;
    // entry[EndColumn] = to_y;
    // entry[Row2] = extra_x;
    // entry[Column2] = extra_y;
    // entry[Rank] = rank;

    new index = ArrayPushArray(g_aHistory[boardIndex], entry, sizeof entry);
    console_print(0, "history new index: %d, entry[pieceid] %d", index, entry[MovingPieceID]);
}

history_pop(boardIndex, entry[MoveStruct])
{
    ArrayGetArray(g_aHistory[boardIndex], ArraySize(g_aHistory[boardIndex])-1, entry, sizeof entry);
    ArrayDeleteItem(g_aHistory[boardIndex], ArraySize(g_aHistory[boardIndex])-1);
}

history_get_last(boardIndex, entry[MoveStruct])
{
    ArrayGetArray(g_aHistory[boardIndex], ArraySize(g_aHistory[boardIndex])-1, entry);
}

history_get_var_from_last(boardIndex, var)
{
    new entry[MoveStruct];
    ArrayGetArray(g_aHistory[boardIndex], ArraySize(g_aHistory[boardIndex])-1, entry);
    return entry[var];
}

bool:is_history_empty(boardIndex)
{
    return _:g_aHistory[boardIndex] == 0 || ArraySize(g_aHistory[boardIndex]) == 0;
}

// history_push(boardIndex, HistoryEntryType:type, pieceId, from_x, from_y, to_x, to_y)
// {
//     if(!g_aHistory[boardIndex]) g_aHistory[boardIndex] = ArrayCreate();
    
//     new entry[HistoryEntry];
//     // entry = g_aHistory[boardIndex][g_aHistoryLength];
//     entry[Type] = type;
//     entry[MovingPieceId] = pieceId;
//     entry[FromTo][0] = from_x;
//     entry[FromTo][1] = from_y;
//     entry[FromTo][2] = to_x;
//     entry[FromTo][3] = to_y;

//     entry[ActionData] = data;

//     ArrayPushArray(g_aHistory[boardIndex], entry, sizeof entry);
//     // g_aHistory[boardIndex][g_aHistoryLength] = entry;
//     // g_aHistoryLength ++;
// }

// history_pop()
// {
    // g_aHistoryLength --;
    // TODO: is this valid cleaning?
    // g_aHistory[boardIndex][g_aHistoryLength] = 0;
// }
