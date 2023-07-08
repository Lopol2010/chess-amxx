// Credits: R3X@Box System
#include <amxmodx>
#include <amxmisc>
#include <fakemeta>
#include <engine>
#include <xs>
#include <box_with_boxes_stocks>

#define BOX_THINK_TIMER 0.001
#define BOX_VISUAL_THINK_TIMER 0.3

// TODO: settings for delay and distance in menu
#define MOVE_DELAY 0.05
#define MOVE_STEP 1.0

#define PEV_TYPE pev_netname
#define PEV_TYPE_INDEX pev_iuser4
#define PEV_ID pev_message
#define PEV_BOARD_INDEX pev_iuser3
#define PEV_PIECE_ID pev_iuser4

#define m_flNextAttack 83

#define BOX_CLASSNAME "bwb"
#define ANCHOR_CLASSNAME "bwb_anchor"
#define SELECTED_ANCHOR_CLASSNAME "bwb_selected_anchor"

new Float:BOARD_SIZE[3] = { 64.00, 64.00, 4.4 };
new Float:SQUARE_SIZE = 8.00;
new Float:BOARD_CORNER[3];
// new Float:FIRST_SQUARE_OFFSET[3] = { 5.8, 5.8, -2.0 };
new Float:FIRST_SQUARE_OFFSET[3] = { 0.0, ... };

new g_iBoardEntity[MAX_BOARDS];
new g_iPieceEntities[MAX_BOARDS][BOARD_ROWS][BOARD_COLUMNS];
new g_iPromotionMenuEntities[MAX_BOARDS][4];

new const g_szModel[] = "sprites/cnt1.spr";
new const g_szChessPiecesModel[] = "models/chess_pieces.mdl";
new const g_szChessBoardModel[] = "models/chess_board.mdl";

// angelina.mdl
new g_iSpriteLine;
new g_iGlowEnt;

// new Float:g_fDistance[33];
// new g_iCatched[33];
// new g_iMarked[33];

new g_iSelectedBox[33];

new bool:g_bEditMode[33];

new giUNIQUE = 1;

enum _:TypeStruct {
    Type[32],
    BoxColor[3]
};

new type_info[TypeStruct];
new Array:g_aTypes;

enum Forwards {
    StartTouch,
    StopTouch,
    FrameTouch,
    BoxCreated,
    BoxDeleted,
    InvalidTouch
};

new g_hForwards[Forwards];

enum _:HistoryStruct {
    BoxEnt,
    Float:AbsMins[3],
    Float:AbsMaxs[3]
};
// new Array:g_aHistory;

enum _:TracelineResult {
    TR_RESULT_NONE,
    TR_RESULT_BOARD,
    TR_RESULT_PROMOTION_MENU_ITEM,
};


public native_create_box(plugin, params)
{
    enum {
        arg_type = 1,
        arg_origin,
        arg_mins,
        arg_maxs
    };

    new type[32];
    get_string(arg_type, type, charsmax(type));
    new Float:origin[3];
    get_array_f(arg_origin, origin, sizeof(origin));
    new Float:mins[3];
    get_array_f(arg_mins, mins, sizeof(mins));
    new Float:maxs[3];
    get_array_f(arg_maxs, maxs, sizeof(maxs));

    create_box(origin, type);
}
public native_get_type_index(plugin, params)
{
    enum {
        arg_type = 1,
    };

    new type[32];
    get_string(arg_type, type, charsmax(type));

    return ArrayFindString(g_aTypes, type);
}
public native_get_box_type(plugin, params)
{
    enum {
        arg_box = 1,
        arg_type,
        arg_len
    };

    new box = get_param(arg_box);

    new type[32];
    pev(box, PEV_TYPE, type, charsmax(type));
    set_string(arg_type, type, get_param(arg_len));
}
load_types()
{
    // new INIParser:parser = INI_CreateParser();

    // INI_SetParseEnd(parser, "ini_parse_end");
    // INI_SetReaders(parser, "ini_key_value", "ini_new_section");
    // new bool:result = INI_ParseFile(parser, "addons/amxmodx/configs/box_with_boxes/types.ini");
    
    // if(!result) {
    //     // TODO
    //     log_amx("Failed to parse ini file");
    // }
}
// public ini_new_section(INIParser:handle, const section[], bool:invalid_tokens, bool:close_bracket, bool:extra_tokens, curtok, any:data)
// {
//     if(type_info[Type]) {
//         push_type();
//     }
    
//     copy(type_info[Type], charsmax(type_info[Type]), section);

//     return true;
// }
// public ini_key_value(INIParser:handle, const key[], const value[], bool:invalid_tokens, bool:equal_token, bool:quotes, curtok, any:data)
// {
//     new k[32];
//     copy(k, charsmax(k), key);
//     remove_quotes(k);
//     if(equal(k, "color")) {
//         new color[3];
//         parse_color(value, color);
//         type_info[BoxColor] = color;
//     }

//     return true;
// }
// public ini_parse_end(INIParser:handle, bool:halted, any:data)
// {
//     if(type_info[Type]) {
//         push_type();
//     }

//     INI_DestroyParser(handle);
// }

// push_type()
// {
//     new index = ArrayFindString(g_aTypes, type_info[Type]);
//     if(index == -1) {
//         ArrayPushArray(g_aTypes, type_info);
//     }
// }

stock parse_color(const string[], color[3])
{
    new r[4], g[4], b[4];
    parse(string, r, charsmax(r), g, charsmax(g), b, charsmax(b));
    color[0] = str_to_num(r);
    color[1] = str_to_num(g);
    color[2] = str_to_num(b);
}


show_bwb_menu(id)
{
    new menu = menu_create("Box with Boxes", "bwb_menu_handler");

    menu_additem(menu, "Create box", "1");

    if(g_iSelectedBox[id]) {
        new type[32], index[32];
        pev(g_iSelectedBox[id], PEV_ID, index, charsmax(index));
        pev(g_iSelectedBox[id], PEV_TYPE, type, charsmax(type));
        menu_additem(menu, fmt("Select next \y[Current: %s, Type: %s]", index, type), "2");
    } else {
        menu_additem(menu, "Select next", "2");
    }

    menu_additem(menu, "Select the nearest", "3");
    
    menu_additem(menu, "Move to selected box", "4");
    menu_additem(menu, "Change box type", "5");
    menu_additem(menu, fmt("Noclip \r%s", pev(id, pev_movetype) == MOVETYPE_NOCLIP ? "[ON]" : "[OFF]"), "6");

    menu_addblank2(menu);
    menu_addblank2(menu);

    menu_additem(menu, "Remove selected box", "7");
    menu_additem(menu, "Exit", "8");

    menu_setprop(menu, MPROP_PERPAGE, 0);

    menu_display(id, menu);

    if(!g_bEditMode[id]) {
        g_bEditMode[id] = true;
        switch_edit_mode(id);
    }

    return PLUGIN_HANDLED;
}
public bwb_menu_handler(id, menu, item)
{
    if(item == MENU_EXIT) {
        menu_destroy(menu);
        g_bEditMode[id] = false;
        switch_edit_mode(id);
        return PLUGIN_HANDLED;
    }

    new item_info[8];
    menu_item_getinfo(menu, item, _, item_info, charsmax(item_info));
    menu_destroy(menu);

    new index = str_to_num(item_info);

    switch(index) {
        case 1: {
            // new Float:origin[3];
            // pev(id, pev_origin, origin);
            new Float:origin[3];
            // TracelineThroughAnchor(id, id, origin);
            new Float:t[3] = { 33.1, 33.1, 1.75 };
            origin[2] += t[2] + 2.0;
            new ent = create_box(origin);

            if(pev_valid(g_iSelectedBox[id])) {
                // remove_task(g_iSelectedBox[id]);
                // remove_anchors(g_iSelectedBox[id]);
                remove_selected_anchor(id);
            }
            
            g_iSelectedBox[id] = ent;

            if(g_bEditMode[id]) {
                
                set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent, .flags = "b");
                // create_anchors(ent);
                // create_selected_anchor(id, g_iSelectedBox[id]);
            }
        }
        case 2: {
            new ent = g_iSelectedBox[id] ? g_iSelectedBox[id] : -1;
            new start = ent;
            new found;

            while(!(start == -1 && ent == 0) && !found) {
                while((ent = find_ent_by_class(ent, BOX_CLASSNAME))) {

                    /* if(pev_valid(g_iSelectedBox[id])) {
                        remove_task(g_iSelectedBox[id]);
                        remove_anchors(g_iSelectedBox[id]);
                    } */

                    g_iSelectedBox[id] = ent;
                    found = true;
                    break;
                }
            }

            if(found) {
                remove_selected_anchor(id);
                if(g_bEditMode[id]) {
                    create_selected_anchor(id, ent);

                    /* set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent, .flags = "b");
                    create_anchors(ent); */
                }
            }
        }
        case 3: {
            new nearest;
            new Float:ndist, Float:dist;

            new ent = -1;
            while((ent = find_ent_by_class(ent, BOX_CLASSNAME))) {
                dist = entity_range(id, ent);
                if(!nearest || dist < ndist) {
                    ndist = dist;
                    nearest = ent;
                }
            }

            if(nearest) {
                remove_selected_anchor(id);
                if(g_bEditMode[id]) {
                    /* if(pev_valid(g_iSelectedBox[id])) {
                        remove_task(g_iSelectedBox[id]);
                        remove_anchors(g_iSelectedBox[id]);
                    } */

                    create_selected_anchor(id, nearest);

                    /* set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", nearest, .flags = "b");
                    create_anchors(nearest); */
                }

                g_iSelectedBox[id] = nearest;
            }
        }
        case 4: {
            if(pev_valid(g_iSelectedBox[id])) {
                new ent = g_iSelectedBox[id];
                new Float:origin[3];
                pev(ent, pev_origin, origin);
                set_pev(id, pev_origin, origin);
                remove_selected_anchor(id);
            }
        }
        case 5: {
            if(pev_valid(g_iSelectedBox[id])) {
                // show_type_menu(id);
                return PLUGIN_HANDLED;
            }
        }
        case 6: {
            set_pev(id, pev_movetype, pev(id, pev_movetype) == MOVETYPE_NOCLIP ? MOVETYPE_WALK : MOVETYPE_NOCLIP);
        }
        case 7: {
            if(pev_valid(g_iSelectedBox[id])) {
                new ent = g_iSelectedBox[id];
                remove_task(ent);
                // remove_anchors(ent);
                // remove_selected_anchor(id);

                // remove_box(ent);
                g_iSelectedBox[id] = 0;
            }
        }
        case 8: {
            g_bEditMode[id] = false;
            switch_edit_mode(id);
            return PLUGIN_HANDLED;
        }
    }

    show_bwb_menu(id);

    return PLUGIN_HANDLED;
}

// show_type_menu(id)
// {
//     new type[32], index[32];
//     pev(g_iSelectedBox[id], PEV_ID, index, charsmax(index));
//     pev(g_iSelectedBox[id], PEV_TYPE, type, charsmax(type));

//     new menu = menu_create(fmt("Type menu^nCurrent box: %s, type: %s", index, type), "type_menu_handler");

//     for(new i, size = ArraySize(g_aTypes); i < size; i++) {
//         ArrayGetArray(g_aTypes, i, type_info);
//         menu_additem(menu, type_info[Type]);
//     }

//     menu_display(id, menu);
// }

public type_menu_handler(id, menu, item)
{
    if(item == MENU_EXIT) {
        menu_destroy(menu);
        if(is_user_connected(id)) {
            show_bwb_menu(id);
        }
        return PLUGIN_HANDLED;
    }

    new item_name[32];
    menu_item_getinfo(menu, item, _, _, _, item_name, charsmax(item_name));
    menu_destroy(menu);

    if(pev_valid(g_iSelectedBox[id])) {
        set_pev(g_iSelectedBox[id], PEV_TYPE, item_name);
    }

    // TODO: forward type changed

    show_bwb_menu(id);

    return PLUGIN_HANDLED;
}

switch_edit_mode(id)
{
    if(g_bEditMode[id]) {
        new ent = -1;
        while((ent = find_ent_by_class(ent, BOX_CLASSNAME))) {
            set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent, .flags = "b");
            // create_anchors(ent);
        }

        if(pev_valid(g_iSelectedBox[id])) {
            /* ent = g_iSelectedBox[id];
            set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent, .flags = "b");
            create_anchors(ent); */

            // create_selected_anchor(id, g_iSelectedBox[id]);
        }
    } else {
        new ent = -1;
        while((ent = find_ent_by_class(ent, BOX_CLASSNAME))) {
            remove_task(ent);
            // remove_anchors(ent);
        }
        if(pev_valid(g_iSelectedBox[id])) {
            /* new ent = g_iSelectedBox[id];
            remove_task(ent);
            remove_anchors(ent); */

            // remove_selected_anchor(id);
        }
    }
}

public cmd_undo(id)
{
    if(!g_bEditMode[id]) {
        return PLUGIN_CONTINUE;
    }

    // box_history_pop();

    return PLUGIN_HANDLED;
}

create_board_entity(const Float:origin[3], const class[] = "box")
{
    new ent = create_entity("func_wall");

    entity_set_string(ent, EV_SZ_classname, BOX_CLASSNAME);
    set_pev(ent, PEV_TYPE, class);

    // new index = ArrayFindString(g_aTypes, class);

    // set_pev(ent, PEV_TYPE_INDEX, index);

    // set_pev(ent, PEV_ID, fmt("Box#%d", giUNIQUE++));

    DispatchSpawn(ent);

    entity_set_model(ent, g_szChessBoardModel);
    set_pev(ent, pev_origin, origin);
    set_rendering(ent, kRenderFxNone, 155, 111, 0, kRenderNormal, 255);
    // set_rendering(ent, kRenderFxPulseFast, 0, 150, 0, kRenderTransAdd, 150);

    // set_pev(ent, pev_effects, EF_NODRAW);
    set_pev(ent, pev_solid, SOLID_BBOX);
    
    set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
    // set_pev(ent, pev_movetype, MOVETYPE_NONE);
                           
    // set_pev(ent, pev_enemy, 1);

    // set_pev(ent, pev_nextthink, get_gametime() + 0.1);

    // touch info
    // new Array:a = ArrayCreate(1, 1);
    // set_pev(ent, pev_iuser3, a);

    new Float:mins[3]; 
    xs_vec_div_scalar(BOARD_SIZE, -2.0, mins);
    new Float:maxs[3]; xs_vec_div_scalar(BOARD_SIZE, 2.0, maxs);
    entity_set_size(ent, mins, maxs);

    return ent;
}

create_promotion_menu_item_entity(const Float:origin[3])
{
    new ent = create_entity("func_wall");

    // entity_set_string(ent, EV_SZ_classname, BOX_CLASSNAME);
    // set_pev(ent, PEV_TYPE, class);

    // new index = ArrayFindString(g_aTypes, class);

    // set_pev(ent, PEV_TYPE_INDEX, index);

    // set_pev(ent, PEV_ID, fmt("Box#%d", giUNIQUE++));

    DispatchSpawn(ent);

    entity_set_model(ent, g_szChessPiecesModel);
    set_pev(ent, pev_origin, origin);
    // set_pev(ent, pev_body, 0);
    set_rendering(ent, kRenderFxNone, 155, 111, 0, kRenderNormal, 255);
    // set_rendering(ent, kRenderFxPulseFast, 0, 150, 0, kRenderTransAdd, 150);

    // set_pev(ent, pev_effects, EF_NODRAW);
    set_pev(ent, pev_solid, SOLID_BBOX);
    
    set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
    // set_pev(ent, pev_movetype, MOVETYPE_NONE);
                           
    // set_pev(ent, pev_enemy, 1);

    // set_pev(ent, pev_nextthink, get_gametime() + 0.1);

    // touch info
    // new Array:a = ArrayCreate(1, 1);
    // set_pev(ent, pev_iuser3, a);

    // new Float:mins[3]; 
    // xs_vec_div_scalar(BOARD_SIZE, -2.0, mins);
    // new Float:maxs[3]; xs_vec_div_scalar(BOARD_SIZE, 2.0, maxs);
    entity_set_size(ent, Float:{ -3.0, -3.0, 0.0 }, Float:{ 3.0, 3.0, 6.0 });

    return ent;
}

create_box(const Float:origin[3], const class[] = "box")
{
    new ent = create_entity("func_wall");

    entity_set_string(ent, EV_SZ_classname, BOX_CLASSNAME);
    set_pev(ent, PEV_TYPE, class);

    new index = ArrayFindString(g_aTypes, class);

    set_pev(ent, PEV_TYPE_INDEX, index);

    set_pev(ent, PEV_ID, fmt("Box#%d", giUNIQUE++));

    DispatchSpawn(ent);

    entity_set_model(ent, g_szModel);

    set_pev(ent, pev_effects, EF_NODRAW);
    set_pev(ent, pev_solid, SOLID_BBOX);
    
    // set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
    set_pev(ent, pev_movetype, MOVETYPE_NONE);
                           
    // set_pev(ent, pev_enemy, 1);

    set_pev(ent, pev_nextthink, get_gametime() + 0.1);

    // touch info
    new Array:a = ArrayCreate(1, 1);
    set_pev(ent, pev_iuser3, a);

    entity_set_origin(ent, origin);
    new Float:mins[3]; xs_vec_div_scalar(BOARD_SIZE, -2.0, mins);
    new Float:maxs[3]; xs_vec_div_scalar(BOARD_SIZE, 2.0, maxs);
    entity_set_size(ent, mins, maxs);

    // new ret;
    // ExecuteForward(g_hForwards[BoxCreated], ret, ent, class);

    return ent;
}

create_glow(const class[] = "glow")
{
    new ent = create_entity("info_target");

    entity_set_string(ent, EV_SZ_classname, class);
    set_pev(ent, PEV_TYPE, class);

    DispatchSpawn(ent);


    entity_set_model(ent, g_szModel);
    entity_set_size(ent, Float:{ -1.0, -1.0, -1.0 }, Float:{ 1.0, 1.0, 1.0 });

    // set_pev(ent, pev_solid, SOLID_BBOX);
    set_pev(ent, pev_solid, SOLID_TRIGGER);
    // set_pev(ent, pev_solid, SOLID_SLIDEBOX);
    
    // set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);

    // set_pev(ent, pev_iuser4, vertex);

    set_pev(ent, pev_scale, 0.05);

    set_rendering(ent, kRenderFxPulseFast, 0, 250, 0, kRenderTransAdd, 2555);

    return ent;
}

// create_anchors(box)
// {
//     new Float:mins[3], Float:maxs[3];
//     pev(box, pev_absmin, mins);
//     pev(box, pev_absmax, maxs);

//     create_anchor_entity(box, 0b000, mins[0], mins[1], mins[2]);
//     create_anchor_entity(box, 0b001, mins[0], mins[1], maxs[2]);
//     create_anchor_entity(box, 0b010, mins[0], maxs[1], mins[2]);
//     create_anchor_entity(box, 0b011, mins[0], maxs[1], maxs[2]);
//     create_anchor_entity(box, 0b100, maxs[0], mins[1], mins[2]);
//     create_anchor_entity(box, 0b101, maxs[0], mins[1], maxs[2]);
//     create_anchor_entity(box, 0b110, maxs[0], maxs[1], mins[2]);
//     create_anchor_entity(box, 0b111, maxs[0], maxs[1], maxs[2]);

//     /* new Float:x, Float:y, Float:z;
//     for(new i = 0b000; i <= 0b111; i++) {
//         x = i & (1 << 0) ? maxs[0] : mins[0];
//         y = i & (1 << 1) ? maxs[1] : mins[1];
//         z = i & (1 << 2) ? maxs[2] : mins[2];

//         create_anchor_entity(box, i, x, y, z);
//     } */
// }

// create_anchor_entity(box, vertex, Float:x, Float:y, Float:z)
// {
//     new Float:origin[3];
//     origin[0] = x;
//     origin[1] = y;
//     origin[2] = z;

//     new ent = create_entity("info_target");
//     entity_set_string(ent, EV_SZ_classname, ANCHOR_CLASSNAME);


//     entity_set_model(ent, g_szModel);
//     entity_set_origin(ent, origin);

//     entity_set_size(ent, Float:{ -3.0, -3.0, -3.0 }, Float:{ 3.0, 3.0, 3.0 });

//     set_pev(ent, pev_solid, SOLID_BBOX);
//     set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
//     set_pev(ent, pev_owner, box);

//     set_pev(ent, pev_iuser4, vertex);

//     set_pev(ent, pev_scale, 0.25);

//     set_rendering(ent, kRenderFxPulseFast, 0, 150, 0, kRenderTransAdd, 255);
// }

// remove_anchors(box)
// {
//     new ent = -1;
//     while((ent = find_ent_by_owner(ent, ANCHOR_CLASSNAME, box))) {
//         remove_entity(ent);
//     }
// }

// get_anchor(box, num)
// {
//     new ent = 0;
//     new a = -1;
//     while((a = find_ent_by_owner(a, ANCHOR_CLASSNAME, box))) {
//         if(pev(a, pev_iuser4) == num) {
//             ent = a;
//             break;
//         }
//     }
//     return ent;
// }

create_selected_anchor(id, box)
{
    new ent = create_entity("info_target");
    entity_set_string(ent, EV_SZ_classname, SELECTED_ANCHOR_CLASSNAME);

    new Float:origin[3];
    pev(box, pev_origin, origin);

    entity_set_model(ent, g_szModel);
    entity_set_origin(ent, origin);

    entity_set_size(ent, Float:{ -3.0, -3.0, -3.0 }, Float:{ 3.0, 3.0, 3.0 });

    set_pev(ent, pev_solid, SOLID_BBOX);
    set_pev(ent, pev_movetype, MOVETYPE_NOCLIP);
    set_pev(ent, pev_owner, box);

    set_pev(ent, pev_iuser3, id);

    set_pev(ent, pev_scale, 0.4);

    set_rendering(ent, kRenderFxPulseFast, 150, 150, 0, kRenderTransAdd, 255);
}
remove_selected_anchor(id)
{
    new ent = -1;
    while((ent = find_ent_by_class(ent, SELECTED_ANCHOR_CLASSNAME))) {
        if(pev(ent, pev_iuser3) == id) {
            remove_entity(ent);
        }
    }
}

// remove_box(box)
// {
//     new type[32];
//     pev(box, PEV_TYPE, type, charsmax(type));

//     new ret;
//     ExecuteForward(g_hForwards[BoxDeleted], ret, box, type);

//     new Array:a = Array:pev(box, pev_iuser3);
//     ArrayDestroy(a);
//     remove_entity(box);

//     new size = ArraySize(g_aHistory);

//     if(!size) {
//         return;
//     }

//     new history_info[HistoryStruct];

//     for(new i = size - 1; i >= 0; i--) {
//         ArrayGetArray(g_aHistory, i, history_info);
//         if(box == history_info[BoxEnt]) {
//             ArrayDeleteItem(g_aHistory, i);
//         }
//     }
// }

// Visual
public show_board_grid(ent)
{
    new Float:absmins[3], Float:absmaxs[3], Float:mins[3], Float:maxs[3], Float:origin[3];
    pev(ent, pev_absmin, absmins);
    pev(ent, pev_absmax, absmaxs);
    pev(ent, pev_mins, mins);
    pev(ent, pev_maxs, maxs);
    pev(ent, pev_origin, origin);

    new color[3];
    get_type_color(ent, color);

    // _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmaxs[2]); // 011 001

    // _draw_line(color, absmins[0], absmins[1], absmins[2], absmaxs[0], absmaxs[1], absmaxs[2]); // 000 111

    // _draw_line(color, absmins[0], absmins[1], absmaxs[2], absmins[0], absmins[1], absmins[2]); // 001 000
    // _draw_line(color, absmins[0], absmaxs[1], absmins[2], absmins[0], absmins[1], absmins[2]); // 010 000

    _draw_line(color, absmins[0], absmaxs[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 011 001

    // _draw_line(color, absmins[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmins[2]); // 011 010
    // _draw_line(color, absmaxs[0], absmins[1], absmins[2], absmins[0], absmins[1], absmins[2]); // 100 000

    _draw_line(color, absmaxs[0], absmins[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 101 001

    // _draw_line(color, absmaxs[0], absmins[1], absmaxs[2], absmaxs[0], absmins[1], absmins[2]); // 101 100
    // _draw_line(color, absmaxs[0], absmaxs[1], absmins[2], absmins[0], absmaxs[1], absmins[2]); // 110 010
    // _draw_line(color, absmaxs[0], absmaxs[1], absmins[2], absmaxs[0], absmins[1], absmins[2]); // 110 100

    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmaxs[2]); // 111 011
    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmaxs[0], absmins[1], absmaxs[2]); // 111 101

    // _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmaxs[0], absmaxs[1], absmins[2]); // 111 110

    // draw grid
    // for(new i = 1; i < 8; i++) {
    //     _draw_line(color, absmins[0] + i*8 + 1, absmaxs[1], absmaxs[2], absmins[0] + i*8 + 1, absmins[1], absmaxs[2]); // 011 001
    //     client_print(0, print_chat, "%d %f", i, absmaxs[0] - (i * (8)));
    // }
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
public box_visual_think(ent)
{
    new Float:absmins[3], Float:absmaxs[3], Float:mins[3], Float:maxs[3], Float:origin[3];
    pev(ent, pev_absmin, absmins);
    pev(ent, pev_absmax, absmaxs);
    pev(ent, pev_mins, mins);
    pev(ent, pev_maxs, maxs);
    pev(ent, pev_origin, origin);

    new color[3];
    get_type_color(ent, color);

    // _draw_line(color, absmins[0], absmins[1], absmins[2], absmaxs[0], absmaxs[1], absmaxs[2]); // 000 111

    // _draw_line(color, absmins[0], absmins[1], absmaxs[2], absmins[0], absmins[1], absmins[2]); // 001 000
    // _draw_line(color, absmins[0], absmaxs[1], absmins[2], absmins[0], absmins[1], absmins[2]); // 010 000

    _draw_line(color, absmins[0], absmaxs[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 011 001

    // _draw_line(color, absmins[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmins[2]); // 011 010
    // _draw_line(color, absmaxs[0], absmins[1], absmins[2], absmins[0], absmins[1], absmins[2]); // 100 000

    _draw_line(color, absmaxs[0], absmins[1], absmaxs[2], absmins[0], absmins[1], absmaxs[2]); // 101 001

    // _draw_line(color, absmaxs[0], absmins[1], absmaxs[2], absmaxs[0], absmins[1], absmins[2]); // 101 100
    // _draw_line(color, absmaxs[0], absmaxs[1], absmins[2], absmins[0], absmaxs[1], absmins[2]); // 110 010
    // _draw_line(color, absmaxs[0], absmaxs[1], absmins[2], absmaxs[0], absmins[1], absmins[2]); // 110 100

    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmins[0], absmaxs[1], absmaxs[2]); // 111 011
    _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmaxs[0], absmins[1], absmaxs[2]); // 111 101

    // _draw_line(color, absmaxs[0], absmaxs[1], absmaxs[2], absmaxs[0], absmaxs[1], absmins[2]); // 111 110

    // draw grid
    // for(new i = 1; i < 8; i++) {
    //     _draw_line(color, absmins[0] + i*8 + 1, absmaxs[1], absmaxs[2], absmins[0] + i*8 + 1, absmins[1], absmaxs[2]); // 011 001
    //     client_print(0, print_chat, "%d %f", i, absmaxs[0] - (i * (8)));
    // }
    new Float:x = 8.0;
    new Float:y = 8.0;
    for(new i = 1; i < 8; i++) {
        _draw_line(color, absmins[0] + x + 1, absmaxs[1], absmaxs[2], absmins[0] + x + 1, absmins[1], absmaxs[2]); // 011 001
        _draw_line(color, absmaxs[0], absmins[1] + y + 1, absmaxs[2], absmins[0], absmins[1] + y + 1, absmaxs[2]); // 101 001
        // client_print(0, print_chat, "%d %f", i, absmaxs[0] - (i * (8)));

        x += (8.0);
        y += (8.0);
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

    draw_line(start, stop, color, g_iSpriteLine);
}

get_type_color(ent, color[3])
{
    if(!pev_valid(ent))
        return 0;

    new type[32];
    pev(ent, PEV_TYPE, type, charsmax(type));

    new r = ArrayFindString(g_aTypes, type);

    if(r != -1) {
        ArrayGetArray(g_aTypes, r, type_info);
        color[0] = type_info[BoxColor][0];
        color[1] = type_info[BoxColor][1];
        color[2] = type_info[BoxColor][2];
    } else {
        color[0] = 50;
        color[1] = 255;
        color[2] = 50;
    }

    return 1;
}


// public FwdCmdStart( client, ucHandle ) 
// {
//     if( !g_bEditMode[client]
//     || !is_user_alive( client )
//     || pev( client, pev_movetype ) != MOVETYPE_NOCLIP
//     || !( pev( client, pev_button ) & IN_FORWARD ) ) {
//         return FMRES_IGNORED;
//     }
    
//     static Float:fForward, Float:fSide;
//     get_uc( ucHandle, UC_ForwardMove, fForward );
//     get_uc( ucHandle, UC_SideMove, fSide );
    
//     if( fForward == 0.0 && fSide == 0.0 ) {
//         return FMRES_IGNORED;
//     }
    
//     static Float:fMaxSpeed;
//     pev( client, pev_maxspeed, fMaxSpeed );
    
//     new Float:fWalkSpeed = fMaxSpeed * 0.52;
//     if( floatabs( fForward ) <= fWalkSpeed
//     && floatabs( fSide ) <= fWalkSpeed ) {
//         static Float:vOrigin[ 3 ];
//         pev( client, pev_origin, vOrigin );
        
//         static Float:vAngle[ 3 ];
//         pev( client, pev_v_angle, vAngle );
//         engfunc( EngFunc_MakeVectors, vAngle );
//         global_get( glb_v_forward, vAngle );
        
//         vOrigin[ 0 ] += ( vAngle[ 0 ] * 8.0 );
//         vOrigin[ 1 ] += ( vAngle[ 1 ] * 8.0 );
//         vOrigin[ 2 ] += ( vAngle[ 2 ] * 8.0 );
        
//         engfunc( EngFunc_SetOrigin, client, vOrigin );
//     }
    
//     return FMRES_IGNORED;
// }
// public fwd_trace_line(const Float:v1[], const Float:v2[], fNoMonsters, id, ptr)
// {
//     if(!is_user_alive(id)) {
//         return FMRES_IGNORED;
//     }

//     if(!g_bEditMode[id]) {
//         return FMRES_IGNORED;
//     }

//     new ent = get_tr2(ptr, TR_pHit);

//     // if(!is_valid_ent(ent)) {
//     //     anchor_unmark(id, g_iMarked[id]);
//     //     return FMRES_IGNORED;
//     // }

//     // new Float:p[3];
//     // get_tr2(ptr, TR_vecEndPos, p);
//     // entity_set_origin(g_iGlowEnt, p);

//     // if(is_valid_ent(g_iCatched[id])) {
//     //     if( pev(id, pev_button) & IN_ATTACK ) {
//     //         anchor_move_process(id, g_iCatched[id]);
//     //     } else {
//     //         anchor_move_uninit(id, g_iCatched[id]);
//     //     }
//     // } else {
//     //     new classname[32];
//     //     pev(ent, pev_classname, classname, charsmax(classname));
//     //     if(equal(classname, ANCHOR_CLASSNAME) || equal(classname, SELECTED_ANCHOR_CLASSNAME)) {
//     //         if( pev(id, pev_button) & IN_ATTACK ) {
//     //             anchor_move_init(id, ent);
//     //         } else {
//     //             anchor_mark(id, ent);
//     //         }
//     //     } else {
//     //         anchor_unmark(id, g_iMarked[id]);
//     //     }
//     // }

//     return FMRES_IGNORED;
// }

// box_part(ent)
// {
//     new classname[32];
//     pev(ent, pev_classname, classname, charsmax(classname));
//     if(equal(classname, ANCHOR_CLASSNAME) || equal(classname, SELECTED_ANCHOR_CLASSNAME)) {
//         return true;
//     }
//     return false;
// }

// Float:get_end_point_distance(id, ent, Float:origin[3], Float:vec[3], Float:endp[3])
// {
//     new Float:start[3], Float:end[3];
//     xs_vec_copy(origin, start);

//     xs_vec_mul_scalar(vec, 9999.9, end);
//     xs_vec_add(end, origin, end);

//     while(engfunc(EngFunc_TraceLine, start, end, IGNORE_MONSTERS | IGNORE_MISSILE, ent, 0)) {
//         new hit = get_tr2(0, TR_pHit);
//         if(hit == id) {
//             xs_vec_add(start, vec, start);
//             continue;
//         }
//         if(pev_valid(hit) && box_part(hit)) {
//             xs_vec_add(start, vec, start);
//             continue;
//         }
//         break;
//     }

//     new Float:normal[3];
//     get_tr2(0, TR_vecPlaneNormal, normal);
//     xs_vec_mul_scalar(normal, 2.0, normal);

//     get_tr2(0, TR_vecEndPos, end);

//     xs_vec_add(end, normal, endp);

//     return get_distance_f(origin, end);
// }

// anchor_move_process(id, ent)
// {
//     if( g_iCatched[id] != ent ) {
//         anchor_move_init(id, ent);
//     }

//     new Float:vec[3];
//     pev(id, pev_v_angle, vec);
//     angle_vector(vec, ANGLEVECTOR_FORWARD, vec);

//     new buttons = pev(id, pev_button);
//     static Float:last_change;
//     new Float:gt = get_gametime();
//     if(buttons & IN_USE && gt > last_change + MOVE_DELAY) {
//         g_fDistance[id] -= MOVE_STEP;
//         last_change = gt;
//     }

//     if(buttons & IN_RELOAD && gt > last_change + MOVE_DELAY) {
//         g_fDistance[id] += MOVE_STEP;
//         last_change = gt;
//     }

//     new Float:origin[3];
//     pev(id, pev_origin, origin);

//     new Float:view_ofs[3];
//     pev(id, pev_view_ofs, view_ofs);
//     xs_vec_add(origin, view_ofs, origin);

//     new Float:endp[3];
//     new Float:d = get_end_point_distance(id, ent, origin, vec, endp);

//     if(d > g_fDistance[id]) {
//         xs_vec_mul_scalar(vec, g_fDistance[id], vec);
//         xs_vec_add(origin, vec, vec);
//     } else {
//         vec = endp;

//         if(last_change == gt) {
//             g_fDistance[id] = d;
//         }
//     }

//     set_pev(ent, pev_origin, vec);

//     new classname[32];
//     pev(ent, pev_classname, classname, charsmax(classname));

//     new box = pev(ent, pev_owner);

//     if(equal(classname, SELECTED_ANCHOR_CLASSNAME)) {
//         box_update_origin(box, vec);
//         return;
//     }

//     new num1 = pev(ent, pev_iuser4);

//     new num2 = (~num1) & 0b111;
//     new ent2 = get_anchor(box, num2);

//     new Float:vec2[3];
//     pev(ent2, pev_origin, vec2);

//     box_update_size(box, vec, vec2, num1);
// }
// box_update_size(box, const Float:vec[3], const Float:vec2[3], anchor = -1)
// {
//     new Float:mins[3];
//     mins[0] = floatmin(vec[0], vec2[0]);
//     mins[1] = floatmin(vec[1], vec2[1]);
//     mins[2] = floatmin(vec[2], vec2[2]);

//     new Float:maxs[3];
//     maxs[0] = floatmax(vec[0], vec2[0]);
//     maxs[1] = floatmax(vec[1], vec2[1]);
//     maxs[2] = floatmax(vec[2], vec2[2]);

//     anchor != 0b000 && box_update_anchors_entity(box, 0b000, mins[0], mins[1], mins[2]);
//     anchor != 0b001 && box_update_anchors_entity(box, 0b001, mins[0], mins[1], maxs[2]);
//     anchor != 0b010 && box_update_anchors_entity(box, 0b010, mins[0], maxs[1], mins[2]);
//     anchor != 0b011 && box_update_anchors_entity(box, 0b011, mins[0], maxs[1], maxs[2]);
//     anchor != 0b100 && box_update_anchors_entity(box, 0b100, maxs[0], mins[1], mins[2]);
//     anchor != 0b101 && box_update_anchors_entity(box, 0b101, maxs[0], mins[1], maxs[2]);
//     anchor != 0b110 && box_update_anchors_entity(box, 0b110, maxs[0], maxs[1], mins[2]);
//     anchor != 0b111 && box_update_anchors_entity(box, 0b111, maxs[0], maxs[1], maxs[2]);

//     /* new Float:x, Float:y, Float:z;
//     for(new i = 0b000; i <= 0b111; i++) {
//         if(anchor == i) {
//             continue;
//         }

//         x = i & (1 << 0) ? maxs[0] : mins[0];
//         y = i & (1 << 1) ? maxs[1] : mins[1];
//         z = i & (1 << 2) ? maxs[2] : mins[2];

//         box_update_anchors_entity(box, i, x, y, z);
//     } */

//     new Float:origin[3];
//     xs_vec_add(maxs, mins, origin);
//     xs_vec_mul_scalar(origin, 0.5, origin);

//     xs_vec_sub(maxs, origin, maxs);
//     xs_vec_sub(mins, origin, mins);

//     entity_set_origin(box, origin);
//     entity_set_size(box, mins, maxs);

//     new sanchor = -1;
//     if((sanchor = find_ent_by_owner(sanchor, SELECTED_ANCHOR_CLASSNAME, box))) {
//         entity_set_origin(sanchor, origin);
//     }
// }
// box_update_origin(box, Float:vec[3])
// {
//     entity_set_origin(box, vec);

//     new Float:mins[3], Float:maxs[3];
//     pev(box, pev_absmin, mins);
//     pev(box, pev_absmax, maxs);

//     box_update_anchors_entity(box, 0b000, mins[0], mins[1], mins[2]);
//     box_update_anchors_entity(box, 0b001, mins[0], mins[1], maxs[2]);
//     box_update_anchors_entity(box, 0b010, mins[0], maxs[1], mins[2]);
//     box_update_anchors_entity(box, 0b011, mins[0], maxs[1], maxs[2]);
//     box_update_anchors_entity(box, 0b100, maxs[0], mins[1], mins[2]);
//     box_update_anchors_entity(box, 0b101, maxs[0], mins[1], maxs[2]);
//     box_update_anchors_entity(box, 0b110, maxs[0], maxs[1], mins[2]);
//     box_update_anchors_entity(box, 0b111, maxs[0], maxs[1], maxs[2]);

//     /* new Float:x, Float:y, Float:z;
//     for(new i = 0b000; i <= 0b111; i++) {
//         x = i & (1 << 0) ? maxs[0] : mins[0];
//         y = i & (1 << 1) ? maxs[1] : mins[1];
//         z = i & (1 << 2) ? maxs[2] : mins[2];

//         box_update_anchors_entity(box, i, x, y, z);
//     } */
// }
// box_update_anchors_entity(box, num, Float:x, Float:y, Float:z)
// {
//     new ent = get_anchor(box, num);

//     if(!is_valid_ent(ent)) {
//         return;
//     }

//     new Float:origin[3];
//     origin[0] = x;
//     origin[1] = y;
//     origin[2] = z;

//     entity_set_origin(ent, origin);
// }
// anchor_mark(id, ent)
// {
//     g_iMarked[id] = ent;
//     set_pev(ent, pev_scale, 0.35);
// }
// anchor_unmark(id, ent)
// {
//     g_iMarked[id] = 0;
//     if(pev_valid(ent)) {
//         set_pev(ent, pev_scale, 0.25);
//     }
// }
// anchor_move_init(id, ent)
// {
//     new Float:origin[3];
//     pev(id, pev_origin, origin);

//     new Float:view_ofs[3];
//     pev(id, pev_view_ofs, view_ofs);
//     xs_vec_add(origin, view_ofs, origin);

//     new Float:eorigin[3];
//     pev(ent, pev_origin, eorigin);

//     g_fDistance[id] = get_distance_f(origin, eorigin);
//     g_iCatched[id] = ent;

//     set_rendering(ent, kRenderFxGlowShell, 255, 0, 0, kRenderTransAdd, 255);

//     new box = pev(ent, pev_owner);

//     if(g_iSelectedBox[id] != box) {
//         g_iSelectedBox[id] = box;
//         show_bwb_menu(id);
//     } else {
//         if(!find_ent_by_owner(-1, SELECTED_ANCHOR_CLASSNAME, box)) {
//             create_selected_anchor(id, box);
//         }
//     }

//     box_history_push(box);
// }
// anchor_move_uninit(id, ent)
// {
//     g_fDistance[id] = 0.0;
//     g_iCatched[id] = 0;

//     set_rendering(ent, kRenderFxNone, 0, 150, 0, kRenderTransAdd, 255);
// }

// History
// box_history_push(box)
// {
//     new history_info[HistoryStruct];
//     history_info[BoxEnt] = box;
//     new Float:mins[3], Float:maxs[3];
//     pev(box, pev_absmin, mins);
//     pev(box, pev_absmax, maxs);
//     history_info[AbsMins] = mins;
//     history_info[AbsMaxs] = maxs;

//     ArrayPushArray(g_aHistory, history_info);
// }
// box_history_pop()
// {
//     new size = ArraySize(g_aHistory);
//     if(!size) {
//         return;
//     }

//     new history_info[HistoryStruct];
//     ArrayGetArray(g_aHistory, size - 1, history_info);

//     new box = history_info[BoxEnt];

//     ArrayDeleteItem(g_aHistory, size - 1);

//     if(!pev_valid(box)) {
//         return;
//     }

//     new Float:mins[3], Float:maxs[3];
//     for(new i; i < 3; i++) {
//         mins[i] = history_info[AbsMins][i];
//         maxs[i] = history_info[AbsMaxs][i];
//     }

//     box_update_size(box, mins, maxs);
// }

// Touch Mechanic

public fwd_box_touch(box, ent)
{
    new Array:a = Array:pev(box, pev_iuser3);

    new index = pev(box, PEV_TYPE_INDEX);

    if(ArrayFindValue(a, ent) == -1) {
        ArrayPushCell(a, ent);
        box_start_touch(box, ent, index);
    }

    new ret;
    ExecuteForward(g_hForwards[FrameTouch], ret, box, ent, index);

    return PLUGIN_CONTINUE;
}

public fwd_box_think(box)
{
    // new Array:a = Array:pev(box, pev_iuser3);
    // new ent, index;
    // index = pev(box, PEV_TYPE_INDEX);

    // for(new i = ArraySize(a) - 1; i >= 0; i--) {
    //     ent = ArrayGetCell(a, i);

    //     if(!pev_valid(ent)) {
    //         // box_invalid_touch(box, ent, index);
    //         // ArrayDeleteItem(a, i);
    //         continue;
    //     }

    //     new collision = intersect(box, ent);

    //     new flags = pev(ent, pev_flags);
    //     new Float:v[3];
    //     pev(ent, pev_velocity, v);

    //     // if(!collision || ent > MaxClients && flags & FL_ONGROUND && vector_length(v) == 0.0) {
    //         // box_end_touch(box, ent, index);
    //         // ArrayDeleteItem(a, i);
    //     // }
    // }

    set_pev(box, pev_nextthink, get_gametime() + BOX_THINK_TIMER);
}

// Forwards
box_start_touch(box, ent, index)
{
    new ret;
    ExecuteForward(g_hForwards[StartTouch], ret, box, ent, index);
}
// box_end_touch(box, ent, index) {
//     new ret;
//     ExecuteForward(g_hForwards[StopTouch], ret, box, ent, index);
// }
// box_invalid_touch(box, ent, index)
// {
//     new ret;
//     ExecuteForward(g_hForwards[InvalidTouch], ret, box, ent, index);
// }

// TracelineThroughAnchor(id, ent, Float:hit[3], &x = 0, &y = 0)
// {
//     new Float:fVec[3];
//     pev(id, pev_v_angle, fVec);
//     angle_vector(fVec, ANGLEVECTOR_FORWARD, fVec);
    
//     xs_vec_mul_scalar(fVec, 500.0, fVec);

//     new Float:fOrigin[3];
//     pev(id, pev_origin, fOrigin);
    
//     new Float:fView[3];
//     pev(id, pev_view_ofs, fView);
    
//     xs_vec_add(fOrigin, fView, fOrigin);
//     xs_vec_add(fOrigin, fVec, fVec);

//     new Float:start[3], Float:end[3];

//     xs_vec_copy(fOrigin, start);
//     xs_vec_copy(fVec, end);


//     // SNIPPET SOURCE: https://forums.alliedmods.net/showpost.php?p=1399838&postcount=5
//     // Traceline through multiple obstacles, in this case through player and anchor, then hit what's behind them.
//     new iTraceHit; // hitted entity
//     new iEntToIgnore = id; // this would change at every trace
//     new iTraceHandle = create_tr2(); // trace handle
//     new iMaxTraces = 3, iCurTraceNum = 0;

//     while(engfunc(EngFunc_TraceLine, start, end, IGNORE_MONSTERS | IGNORE_MISSILE, iEntToIgnore, iTraceHandle)) // will always return 1, see engfunc.cpp
//     {

//         iTraceHit = get_tr2(iTraceHandle, TR_pHit); // getting hitted entity
        
//         if(get_global_float(GL_trace_fraction) >= 1.0) // the traceline finished at the original end position, so we will stop here
//             break;

//         if(iTraceHit != ent)
//             break;
        
//         // the next traceline will start at the end of the last one
//         iEntToIgnore = iTraceHit;
//         get_tr2(iTraceHandle, TR_vecEndPos, start);

//         if(iMaxTraces <= iCurTraceNum)
//             break;
//         else
//             iCurTraceNum ++;
//     }

//     get_tr2(iTraceHandle, TR_vecEndPos, hit); // out of the loop, this will get the last position of the last traceline. you can use a beam effect or something if you want

//     new brd = get_tr2(iTraceHandle, TR_pHit, hit); // out of the loop, this will get the last position of the last traceline. you can use a beam effect or something if you want
//     if(is_valid_ent(brd)) {

//         new Float:absmins[3], Float:absmaxs[3], Float:mins[3], Float:maxs[3], Float:origin[3];
//         pev(brd, pev_absmin, absmins);
//         pev(brd, pev_absmax, absmaxs);

//         // new x = 0, y = 0;
//         x = floatround(floatabs((floatabs(absmins[0]+1) - floatabs(hit[0]))) / 8.0, floatround_ceil) ;
//         y = floatround(floatabs((floatabs(absmins[1]+1) - floatabs(hit[1]))) / 8.0, floatround_ceil) ;
//         // client_print(0, print_chat, "%d %d", x,y);
//     }


//     // for(new i = 0; i < 8; i ++) {
//     //     for(new i = 0; i < 8; i ++) {

//     //     }
//     // }

//     free_tr2(iTraceHandle); // freeing the tracehandle 
    
//     // xs_vec_copy(hit, end);
//     // end[2] += 500.0;
//     // Create_Line(ent, hit, end);
// }

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
            } else {
                for(new j = 0; j < 4; j ++) {
                    new menuItemEnt = g_iPromotionMenuEntities[i][j];
                    if(ent == menuItemEnt) {
                        promotion_menu_item_entity = menuItemEnt;
                        new_rank = model_id_to_rank(pev(menuItemEnt, pev_body));
                        server_print("j %d, new_rank %d", j, new_rank)
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
                x = floatround((hit[0] - (BOARD_CORNER[0] + FIRST_SQUARE_OFFSET[0])) / SQUARE_SIZE, floatround_floor);
                y = floatround((hit[1] - (BOARD_CORNER[1] + FIRST_SQUARE_OFFSET[1])) / SQUARE_SIZE, floatround_floor);

                // new Float:p[3];
                // get_tr2(ptr, TR_vecEndPos, p);
                entity_set_origin(g_iGlowEnt, hit);
                server_print("you looking at board: %d", boardIndex);
                
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
                server_print("piece entity deleted");
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
                server_print("render: promoted pawn");
                set_pev(pieceEnt, pev_body, rank_to_model_id(piece[Rank]));
                continue;
            }

            if(x != piece[Row] || y != piece[Column]) {
                new Float:origin[3];
                // pev(ent, pev_origin, origin);
                origin[0] = BOARD_CORNER[0] + FIRST_SQUARE_OFFSET[0] + piece[Row] * SQUARE_SIZE + (SQUARE_SIZE/2);
                // console_print(0, "%f %d", origin[0], x);
                origin[1] = BOARD_CORNER[1] + FIRST_SQUARE_OFFSET[1] + piece[Column] * SQUARE_SIZE + (SQUARE_SIZE/2);
                origin[2] = BOARD_CORNER[2] + FIRST_SQUARE_OFFSET[2];
                // client_print(0, print_chat, "render %d %d %d %d %d", piece[Row], piece[Column], x, y, piece[Rank]);
                set_pev(pieceEnt, pev_origin, origin);

                g_iPieceEntities[boardIndex][x][y] = -1;
                g_iPieceEntities[boardIndex][piece[Row]][piece[Column]] = pieceEnt;
                continue;
            }
        }
    }
}

create_board_entities_dbg(boardIndex, Float:origin[3] = { 0.0, 0.0, 0.0 }) {
    // new Float:origin[3];
    // TracelineThroughAnchor(id, id, origin);
    // origin[0] = 3.0;
    // origin[1] = -97.0;
    // origin[2] += 5.0;

    BOARD_CORNER[0] = origin[0] - BOARD_SIZE[0]/2.0;
    BOARD_CORNER[1] = origin[1] - BOARD_SIZE[1]/2.0;
    BOARD_CORNER[2] = origin[2] + BOARD_SIZE[2];

    // new ent_lines = create_box(origin);
    // new id = 0;
    // g_iSelectedBox[id] = ent_lines;
    // set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent_lines, .flags = "b");

    new ent = create_board_entity(origin);
    set_pev(ent, PEV_BOARD_INDEX, boardIndex);
    g_iBoardEntity[boardIndex] = ent;
    // set_task(BOX_VISUAL_THINK_TIMER, "show_board_grid", ent, .flags = "b");

    // if(pev_valid(g_iSelectedBox[id])) {
    //     // remove_task(g_iSelectedBox[id]);
    //     // remove_anchors(g_iSelectedBox[id]);
    //     remove_selected_anchor(id);
    // }
    
    // g_iSelectedBox[id] = ent;
    // set_task(BOX_VISUAL_THINK_TIMER, "box_visual_think", ent, .flags = "b");

    // create entities for pieces
    new piece[PieceStruct];
    for(new x = 0; x < BOARD_COLUMNS; x++) {
        for(new y = 0; y < BOARD_ROWS; y++) {
            // console_print(0, "%d ", g_PiecesMatrix[boardIndex][x][y]);
            new pieceId = g_PiecesMatrix[boardIndex][x][y];
            if(pieceId == InvalidID) continue;
            piece = g_PiecesList[boardIndex][pieceId];
            // new x = piece[Row];
            // new y = piece[Column];
            new glowEnt = create_glow();
            g_iPieceEntities[boardIndex][x][y] = glowEnt;

            new Float:origin[3];
            pev(ent, pev_origin, origin);
            origin[0] = BOARD_CORNER[0] + x * SQUARE_SIZE + (SQUARE_SIZE/2) + FIRST_SQUARE_OFFSET[0];
            // console_print(0, "%f %d", origin[0], x);
            origin[1] = BOARD_CORNER[1] + y * SQUARE_SIZE + (SQUARE_SIZE/2) + FIRST_SQUARE_OFFSET[1];
            origin[2] = BOARD_CORNER[2] + FIRST_SQUARE_OFFSET[2];
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
    // BOARD_CORNER[0] = origin[0] - BOARD_SIZE[0]/2.0;
    // BOARD_CORNER[1] = origin[1] - BOARD_SIZE[1]/2.0;
    // BOARD_CORNER[2] = origin[2] + BOARD_SIZE[2];

    new boardEnt = g_iBoardEntity[boardIndex];

    new piece[PieceStruct];
    piece = get_piece_data_by_id(pawnIndex);
    new px = piece[Row];
    new py = piece[Column];
    server_print("create_promotion_menu at %d %d, pawnIndex %d", px, py, pawnIndex);
    new pieceEnt = g_iPieceEntities[boardIndex][px][py];
    new pieceColor = piece[Color];
    new pieceRank = piece[Rank];

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

chess_render_init() {
}