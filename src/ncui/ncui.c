#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include <ncurses.h>

#include "ncui.h"


#define NCUI_CLEAR_EVENTS (struct ncui_events_t) { 0, 0 }

struct ncui_t ncui_init(uint16_t max_elements_rows, uint16_t max_elements_columns) {
    struct ncui_t ncui = (struct ncui_t) {
        .update_active = false,
        .max_elements_rows = max_elements_rows,
        .max_elements_columns = max_elements_columns,
        .elements_map = NULL,
        .cursor_x = 0,
        .cursor_y = 0,
        .cursor_max_x = max_elements_columns - 1,
        .cursor_max_y = max_elements_rows - 1,
        .events = NCUI_CLEAR_EVENTS
    };

    ncui.elements_rows_max_columns 
        = NCUI_MEM_ALLOC(max_elements_rows * sizeof *ncui.elements_rows_max_columns);

    ncui.elements_map 
        = NCUI_MEM_ALLOC(max_elements_rows * sizeof **ncui.elements_map);
    
    for(uint16_t row = 0; row < max_elements_rows; row++) {
        ncui.elements_map[row] 
            = NCUI_MEM_ALLOC(max_elements_columns * sizeof **ncui.elements_map);
    }



    return ncui;
}


void free_ncui(struct ncui_t* ncui) {
    for(uint16_t row = 0; row < ncui->max_elements_rows; row++) {
        NCUI_MEM_FREE(ncui->elements_map[row]);
        ncui->elements_map[row] = NULL;
    }

    NCUI_MEM_FREE(ncui->elements_map);
    ncui->elements_map = NULL;

    NCUI_MEM_FREE(ncui->elements_rows_max_columns);
    ncui->elements_rows_max_columns = NULL;
}


void ncui_new_update_begin(struct ncui_t* ncui) {

    for(uint16_t row = 0; row < ncui->max_elements_rows; row++) {
        for(uint16_t col = 0; col < ncui->max_elements_columns; col++) {
            ncui->elements_map[row][col].elem_type_id = NCUI_NULL_ELEM_ID;
        }
    }
   
    for(uint16_t row = 0; row < ncui->max_elements_rows; row++) {
        ncui->elements_rows_max_columns[row] = 0;
    }
    
    ncui->update_active = true;
}


void ncui_clear_events(struct ncui_t* ncui) {
    ncui->events = NCUI_CLEAR_EVENTS;
}

void ncui_set_cursor_max_x(struct ncui_t* ncui, uint16_t new_max_x) { 
    ncui->cursor_max_x = new_max_x;
    if(ncui->cursor_max_x >= ncui->max_elements_columns) {
        ncui->cursor_max_x = ncui->max_elements_columns - 1;
    }
}
void ncui_set_cursor_max_y(struct ncui_t* ncui, uint16_t new_max_y) {
    ncui->cursor_max_y = new_max_y;
    if(ncui->cursor_max_y >= ncui->max_elements_rows) {
        ncui->cursor_max_y = ncui->max_elements_rows - 1;
    }
}

void ncui_set_cursor_max_xy(struct ncui_t* ncui, uint16_t new_max_x, uint16_t new_max_y) {
    ncui_set_cursor_max_x(ncui, new_max_x);
    ncui_set_cursor_max_y(ncui, new_max_y);
}


void ncui_event_move(struct ncui_t* ncui, int16_t offset_x, int16_t offset_y) {
    ncui->cursor_x += (offset_x < 0 && ncui->cursor_x == 0) ? 0 : offset_x;
    ncui->cursor_y += (offset_y < 0 && ncui->cursor_y == 0) ? 0 : offset_y;
    
    if(ncui->cursor_x > ncui->cursor_max_x) {
        ncui->cursor_x = ncui->cursor_max_x;
    }

    if(ncui->cursor_y > ncui->cursor_max_y) {
        ncui->cursor_y = ncui->cursor_max_y;
    }


    uint16_t row_max_column = ncui->elements_rows_max_columns[ncui->cursor_y];
    if(ncui->cursor_x > row_max_column) {
        ncui->cursor_x = row_max_column;
    }
}

void ncui_event_char_input(struct ncui_t* ncui, char ch) {
    if(ch >= 0x20 && ch <= 0x7E) {
        ncui->events.char_input = ch;
    
        struct ncui_element_t* cursor_elem
            = &ncui->elements_map[ncui->cursor_y][ncui->cursor_x];

        if(cursor_elem->elem_type_id != NCUI_INPUTBOX_ELEM_ID) {
            return;
        }

        char* buffer = cursor_elem->inputbox_elem.buffer;
        const size_t buffer_len = strlen(buffer);
        if(buffer_len+1 >= cursor_elem->inputbox_elem.buffer_memsize) {
            return;
        }

        buffer[buffer_len] = ch;

    }
}

void ncui_event_key_press(struct ncui_t* ncui, int key) {
    ncui->events.key_input = key;

    if(key == KEY_BACKSPACE) {
        struct ncui_element_t* cursor_elem
            = &ncui->elements_map[ncui->cursor_y][ncui->cursor_x];

        if(cursor_elem->elem_type_id != NCUI_INPUTBOX_ELEM_ID) {
            return;
        }

        char* buffer = cursor_elem->inputbox_elem.buffer;
        const size_t buffer_len = strlen(buffer);
        
        buffer[(buffer_len > 0) ? buffer_len-1 : 0] = 0;
    }
}


static void ncui_update_elem_row_max_column
(
    struct ncui_t* ncui,
    struct ncui_elem_mappos_t map_pos
){
    uint16_t* row_max_column = &ncui->elements_rows_max_columns[map_pos.y];

    if(*row_max_column < map_pos.x) {
        *row_max_column = map_pos.x;
    }
}


bool ncui_button
(
    struct ncui_t* ncui,
    struct ncui_elem_mappos_t map_pos,
    struct ncui_elem_drawpos_t draw_pos,
    const char* label,
    const char* selected_info, int selected_info_attr
){
    if(!ncui) {
        return false;
    }

    if(!label) {
        return false;
    }

    if((map_pos.x >= ncui->max_elements_columns)
    || (map_pos.y >= ncui->max_elements_rows)) {
        return false;
    }

    ncui_update_elem_row_max_column(ncui, map_pos);
    ncui->elements_map[map_pos.y][map_pos.x].elem_type_id = NCUI_BUTTON_ELEM_ID;

    const size_t label_len = strlen(label);
    const bool is_selected = 
        (map_pos.x == ncui->cursor_x && map_pos.y == ncui->cursor_y);

    int label_attr = (is_selected) 
        ? ncui->style.selected_attr
        : ncui->style.unselected_attr;

    attron(label_attr);
    mvaddstr(draw_pos.y, draw_pos.x, label);
    attroff(label_attr);

    if(is_selected) {
        attron(selected_info_attr);
        mvaddstr(draw_pos.y, draw_pos.x + label_len, selected_info);
        attroff(selected_info_attr);
    }

    return (is_selected && ncui->events.key_input == 0x0A);
}

bool ncui_inputbox
(
    struct ncui_t* ncui,
    struct ncui_elem_mappos_t map_pos,
    struct ncui_elem_drawpos_t draw_pos,
    const char* label,
    char* buffer,
    size_t buffer_memsize
){
    if(!ncui) {
        return false;
    }

    if(!label) {
        return false;
    }

    if(!buffer || !buffer_memsize) {
        return false;
    }

    if((map_pos.x >= ncui->max_elements_columns)
    || (map_pos.y >= ncui->max_elements_rows)) {
        return false;
    }

    ncui_update_elem_row_max_column(ncui, map_pos);
    struct ncui_element_t* elem = &ncui->elements_map[map_pos.y][map_pos.x];

    elem->elem_type_id = NCUI_INPUTBOX_ELEM_ID;
    elem->inputbox_elem.buffer = buffer;
    elem->inputbox_elem.buffer_memsize = buffer_memsize;


    const size_t label_len = strlen(label);
    const bool is_selected = 
        (map_pos.x == ncui->cursor_x && map_pos.y == ncui->cursor_y);

    int label_attr = (is_selected) 
        ? ncui->style.selected_attr
        : ncui->style.unselected_attr;

    attron(label_attr);
    mvaddstr(draw_pos.y, draw_pos.x, label);
    attroff(label_attr);


    int buffer_attr = (is_selected) 
        ? ncui->style.selected_inputbox_buffer_attr
        : ncui->style.unselected_inputbox_buffer_attr;

    attron(buffer_attr);
    mvaddstr(draw_pos.y, draw_pos.x + label_len, buffer);
    attroff(buffer_attr);

    if(is_selected) {
        attron(ncui->style.inputbox_cursor_attr);
        mvaddch(draw_pos.y, draw_pos.x + label_len + strlen(buffer),
                ncui->style.inputbox_cursor_char);
        attroff(ncui->style.inputbox_cursor_attr);
    }

    return false;
}



