#include <stdarg.h>
#include <ncurses.h>
#include <string.h>

#include "tui.h"


#define USER_ACTION_DECLINED 0
#define USER_ACTION_CONFIRMED 1
int confirm_user_action(struct lyk_t* lyk, int message_color_pair, const char* message_fmt, ...) {
    int choise = -1;

    char message[512] = { 0 };
    
    va_list args;
    va_start(args);
    const size_t formatted_length = vsnprintf(message, sizeof(message)-1, message_fmt, args);
    va_end(args);


    // Separate message by new line character
    // because mvaddstr resets the column. (Im not sure if ncurses has function who doesnt do that)
    char message_lines[8][64] = { 0 };
    uint32_t num_message_lines = 0;
    uint32_t curr_message_idx = 0;

    uint32_t longest_message_line = 0;

    char message_ch = 0;
    for(size_t i = 0; i < formatted_length; i++) {
        message_ch = message[i];
        if((message_ch == '\n') || (i+1 >= formatted_length)) {
            num_message_lines++;

            if(curr_message_idx > longest_message_line) {
                longest_message_line = curr_message_idx;
            }

            if(num_message_lines >= (sizeof(message_lines) / sizeof(*message_lines))) {
                break;
            }

            curr_message_idx = 0;
            continue;
        }

        message_lines[num_message_lines][curr_message_idx] = message_ch;
        
        curr_message_idx++;
        if(curr_message_idx >= sizeof(*message_lines)) {
            break; // TODO: Report this error!
        }
    }
    

    // Selected option.
    int selected = 0;

    const int selected_attrs = COLOR_PAIR(COLOR_YELLOW) | A_BOLD;
    const int unselected_attrs = COLOR_PAIR(COLOR_DARK);

    while(choise == -1) {

        getmaxyx(stdscr, lyk->term_height, lyk->term_width);
        int pos_x = lyk->term_width / 2 - (longest_message_line / 2);
        int pos_y = lyk->term_height / 2;
   
        move(0, 0);
        clrtobot();
        addstr("Change selected option with arrow keys. Confirm by pressing <Enter>");
       
        attron(COLOR_PAIR(COLOR_DARK));
        move(pos_y-2, pos_x-1);
        hline('-', longest_message_line+2);
        move(pos_y + num_message_lines+3, pos_x-1);
        hline('-', longest_message_line+2);
        
        attron(COLOR_PAIR(COLOR_DARK));

        attron(COLOR_PAIR(message_color_pair));
        for(uint32_t i = 0; i < num_message_lines; i++) {
            mvaddstr(pos_y, pos_x, message_lines[i]);
            pos_y++;
        }
        attroff(COLOR_PAIR(message_color_pair));
        pos_y--;


        attron((selected == 0) ? selected_attrs : unselected_attrs);
        mvaddstr(pos_y+2, pos_x+4, "[No]");
        attroff((selected == 0) ? selected_attrs : unselected_attrs);
        
        attron((selected == 1) ? selected_attrs : unselected_attrs);
        mvaddstr(pos_y+2, pos_x+11, "[Yes]");
        attroff((selected == 1) ? selected_attrs : unselected_attrs);

        attron(COLOR_PAIR(COLOR_DARK));
        mvaddstr(pos_y+2, pos_x+9, "-");
        attroff(COLOR_PAIR(COLOR_DARK));

        int input = getch();
        switch(input) {


            case KEY_LEFT:
                selected = 0;
                break;

            case KEY_RIGHT:
                selected = 1;
                break;

            case 0x0A: // Enter
                choise = selected;
                break;
        }
    }

    return choise;
}

void draw_outline(int y, int x, int w) {
    move(y, x);
    hline('-', w);
    move(y+2, x);
    hline('-', w);

    move(y, x-1);
    addch('.');
    
    move(y, x+w);
    addch('.');

    move(y+2, x-1);
    addch('\'');
    
    move(y+2, x+w);
    addch('\'');

    move(y+1, x-1);
    addch('|');

    move(y+1, x+w);
    addch('|');
}

void draw_input_field
(
    int cursor_id,
    struct lyk_t* lyk,
    int y,
    int x,
    char* buffer,
    size_t buffer_memsize,
    const char* label
){
    bool active = (lyk->view_cursor == cursor_id);
    int label_color = active ? COLOR_GREEN : COLOR_DARK;

    const size_t buffer_len = strlen(buffer);
    const size_t label_len = strlen(label);

    mvaddstr(y, x + label_len, buffer);
    
    attron(COLOR_PAIR(label_color));
    mvaddstr(y, x, label);
    attroff(COLOR_PAIR(label_color));

    if(active) {
        attron(COLOR_PAIR(COLOR_GREEN) | A_BLINK);
        mvaddch(y, x + label_len + buffer_len, '_');
        attroff(COLOR_PAIR(COLOR_GREEN) | A_BLINK);
        lyk->selected_input_buffer = buffer;
        lyk->selected_input_buffer_memsize = buffer_memsize;
    }
}

void draw_button
(
    int cursor_id,
    struct lyk_t* lyk,
    int y,
    int x,
    const char* label,
    const char* active_key,
    int active_key_attr
){
    bool active = (lyk->view_cursor == cursor_id);
    int label_color = active ? COLOR_YELLOW : COLOR_DARK;

    const size_t label_len = strlen(label);

    attron(COLOR_PAIR(label_color) | A_BOLD);
    mvaddstr(y, x, label);
    attroff(COLOR_PAIR(label_color) | A_BOLD);

    if(active && active_key) {
        attron(COLOR_PAIR(COLOR_DARK) | active_key_attr);
        mvaddstr(y, x + label_len + 1, active_key);
        attroff(COLOR_PAIR(COLOR_DARK) | active_key_attr);
    }   
}


