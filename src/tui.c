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
    char message_lines[8][96] = { 0 };
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

            if(num_message_lines >= 
                    (sizeof(message_lines) / sizeof(*message_lines))) {
                break;
            }

            curr_message_idx = 0;
            continue;
        }

        message_lines[num_message_lines][curr_message_idx] = message_ch;
        
        curr_message_idx++;
        if(curr_message_idx >= sizeof(*message_lines)) {
            num_message_lines++;

            if(num_message_lines >= 
                    (sizeof(message_lines) / sizeof(*message_lines))) {
                break;
            }
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

