#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <chess>

// TODO? #define MAX_HITORY_LENGTH 300

new Array:g_aHistory[MAX_BOARDS];

history_push(boardIndex, entry[MoveStruct])
{
    if(!g_aHistory[boardIndex]) g_aHistory[boardIndex] = ArrayCreate(sizeof entry);
    
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
