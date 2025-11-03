#ifndef NCURSES_USER_INTERFACE_UTILITY_H
#define NCURSES_USER_INTERFACE_UTILITY_H


#ifndef NCUI_MEM_ALLOC
#define NCUI_MEM_ALLOC(size) malloc(size)
#endif

#ifndef NCUI_MEM_FREE
#define NCUI_MEM_FREE(ptr) free(ptr)
#endif


#include <stdint.h>
#include <stdbool.h>

#define NCUI_NULL_ELEM_ID 0
#define NCUI_BUTTON_ELEM_ID 1
#define NCUI_INPUTBOX_ELEM_ID 2



struct ncui_style_t {
    int unselected_attr;
    int selected_attr;
    int selected_inputbox_buffer_attr;
    int unselected_inputbox_buffer_attr;
    int inputbox_cursor_char;
    int inputbox_cursor_attr;
};


struct ncui_inputbox_storage_t {
    char* buffer;
    size_t buffer_memsize;
};


struct ncui_elem_mappos_t {
    uint16_t x;
    uint16_t y;
};

struct ncui_elem_drawpos_t {
    uint16_t x;
    uint16_t y;
};

struct ncui_element_t {
    uint8_t elem_type_id;


    // Some elements may require to store some values.
    // TODO: Use union if more need to be added.

    struct ncui_inputbox_storage_t inputbox_elem;

};

struct ncui_events_t {
    int key_input;
    char char_input;
};

struct ncui_t {
    bool update_active;

    uint16_t cursor_y;
    uint16_t cursor_x;
    uint16_t cursor_max_x;
    uint16_t cursor_max_y;

    struct ncui_style_t style;
    struct ncui_events_t events;

    uint16_t*               elements_rows_max_columns;
    struct ncui_element_t** elements_map;
    uint16_t max_elements_rows;
    uint16_t max_elements_columns;
};


struct ncui_t ncui_init(uint16_t max_elements_rows, uint16_t max_elements_columns);
void          free_ncui(struct ncui_t* ncui);

void ncui_set_cursor_max_x(struct ncui_t* ncui, uint16_t new_max_x);
void ncui_set_cursor_max_y(struct ncui_t* ncui, uint16_t new_max_y);
void ncui_set_cursor_max_xy(struct ncui_t* ncui, uint16_t new_max_x, uint16_t new_max_y);

void ncui_new_update_begin(struct ncui_t* ncui);

void ncui_clear_events        (struct ncui_t* ncui);
void ncui_event_move          (struct ncui_t* ncui, int16_t offset_x, int16_t offset_y);
void ncui_event_char_input    (struct ncui_t* ncui, char ch);
void ncui_event_key_press     (struct ncui_t* ncui, int key);



// Elements:

bool ncui_button
(
    struct ncui_t* ncui,
    struct ncui_elem_mappos_t map_pos,
    struct ncui_elem_drawpos_t draw_pos,
    const char* label,
    const char* selected_info, int selected_info_attr
);

bool ncui_inputbox
(
    struct ncui_t* ncui,
    struct ncui_elem_mappos_t map_pos,
    struct ncui_elem_drawpos_t draw_pos,
    const char* label,
    char* buffer,
    size_t buffer_memsize
);






#endif
