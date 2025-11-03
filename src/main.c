#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
//#include <errno.h>
//#include <stdbool.h>

#include <ncurses.h>
//#include <json-c/json.h>

#include "lyk.h"
#include "tui.h"



#define DELETE_SELECTED_CH 'x'

char PROJECT_NAME_BUFFER[32] = { 0 };
char NOTE_TITLE_BUFFER[64] = { 0 };
char NOTE_DESC_BUFFER[256] = { 0 };
char NOTE_SEVERITY_BUFFER[8] = { 0 };



#define PRINT_JSON(objects)\
    printf("%s\n", json_object_to_json_string_ext(objects, JSON_C_TO_STRING_SPACED | JSON_C_TO_STRING_PRETTY))


void init_curses() {
    initscr();
    raw();    // Do not generate any interupt signal.
    noecho(); // Do not print characters that user types.
    keypad(stdscr, 1); // Enable arrow, backspace keys etc...
    curs_set(0); // Hide cursor.
    start_color();
    use_default_colors();

    init_pair(COLOR_BLACK,   COLOR_BLACK, -1);
    init_pair(COLOR_RED,     COLOR_RED, -1);
    init_pair(COLOR_GREEN,   COLOR_GREEN, -1);
    init_pair(COLOR_YELLOW,  COLOR_YELLOW, -1);
    init_pair(COLOR_BLUE,    COLOR_BLUE, -1);
    init_pair(COLOR_MAGENTA, COLOR_MAGENTA, -1);
    init_pair(COLOR_CYAN,    COLOR_CYAN, -1);
    init_pair(COLOR_WHITE,   COLOR_WHITE, -1);
    init_pair(COLOR_CURSOR,  COLOR_BLACK, COLOR_CYAN);

    init_color(COLOR_DARK, 400, 400, 400);
    init_pair(COLOR_DARK, COLOR_DARK, -1);
    
    init_color(COLOR_VERY_DARK, 200, 200, 200);
    init_pair(COLOR_VERY_DARK, COLOR_VERY_DARK, -1);
}

void handle_delete_selected(struct lyk_t* lyk);

void input_handler(struct lyk_t* lyk) {

    int input = getch();

    switch(input) {
        case 0x09: // Tab
            lyk->running = false;
            break;
            
        case KEY_LEFT:
            ncui_event_move(&lyk->ncui, -1, 0);
            break;

        case KEY_RIGHT:
            ncui_event_move(&lyk->ncui, 1, 0);
            break;

        case KEY_UP:
            ncui_event_move(&lyk->ncui, 0, -1);
            break;

        case KEY_DOWN:
            ncui_event_move(&lyk->ncui, 0, 1);
            break;

        case KEY_BACKSPACE:
            ncui_event_key_press(&lyk->ncui, input);
            break;

        case 'x':
            {
                struct ncui_element_t* cursor_elem
                    = &lyk->ncui.elements_map[lyk->ncui.cursor_y][lyk->ncui.cursor_x];

                if(cursor_elem->elem_type_id == NCUI_INPUTBOX_ELEM_ID) {
                    ncui_event_char_input(&lyk->ncui, input);
                }
                else {
                    handle_delete_selected(lyk);
                }
            }
            break;

        case 0x0A: // Enter
            ncui_event_key_press(&lyk->ncui, input);
            break;

        default:
            ncui_event_char_input(&lyk->ncui, input);
            break;
    }
}

struct project_t* get_selected_project(struct lyk_t* lyk) {
    struct project_t* proj = NULL;

    // There are 2 ui elements before project names.
    // FIXME: This is horrible...
    //        Add some kind of tracking system for this.

    int proj_index = lyk->ncui.cursor_y - 2;
    if((proj_index < 0) || (proj_index >= lyk->num_projects)) {
        goto out;
    }

    proj = &lyk->projects[proj_index];

out:
    return proj;
}


void handle_delete_selected(struct lyk_t* lyk) {
    if(lyk->view == VIEW_PROJECTS) {
        struct project_t* proj = get_selected_project(lyk);
        if(!proj) {
            return;
        }

        int result = confirm_user_action(lyk, COLOR_RED, 
                "Are you sure you want to delete \"%s\" project notes?\n"
                "This action cannot be undone!", proj->name);
    
        if(result == USER_ACTION_CONFIRMED) {
            delete_project(lyk, proj->name);
            if(lyk->ncui.cursor_y >= lyk->ncui.cursor_max_y) {
                lyk->ncui.cursor_y = lyk->ncui.cursor_max_y - 1;
            }
        }
    }
    else
    if(lyk->view == VIEW_PROJECT_NOTES) {
    
    }
}


#define LISTEN_NEXT_INPUT 1
#define SKIP_NEXT_INPUT 2

int draw_view__projects(struct lyk_t* lyk) {
    ncui_new_update_begin(&lyk->ncui);
    ncui_set_cursor_max_y(&lyk->ncui, 1 + lyk->num_projects);


    if(ncui_button(&lyk->ncui,
                (struct ncui_elem_mappos_t){ 0, 1 },
                (struct ncui_elem_drawpos_t){ 2, 4 },
                "[Add]", " <Enter>", COLOR_PAIR(COLOR_DARK) | A_BLINK)) {
     
        create_new_project(lyk, PROJECT_NAME_BUFFER);
        memset(PROJECT_NAME_BUFFER, 0, sizeof(PROJECT_NAME_BUFFER));
    }

    ncui_inputbox(&lyk->ncui, 
            (struct ncui_elem_mappos_t){ 0, 0 },
            (struct ncui_elem_drawpos_t){ 2, 3 },
            "Project name: ",
            PROJECT_NAME_BUFFER, sizeof(PROJECT_NAME_BUFFER));

    for(size_t i = 0; i < lyk->num_projects; i++) {
        struct project_t* proj = &lyk->projects[i];
        if(ncui_button(&lyk->ncui,
                    (struct ncui_elem_mappos_t){ 0, 2 + i },
                    (struct ncui_elem_drawpos_t){ 4, 6 + i },
                    proj->name, " -  Enter: 'view', x: 'delete'", COLOR_PAIR(COLOR_DARK) | A_DIM)) {

            lyk->curr_project = proj;
            lyk->view = VIEW_PROJECT_NOTES;

            return SKIP_NEXT_INPUT;
        }
    }

    return LISTEN_NEXT_INPUT;
}


int draw_view__project_notes(struct lyk_t* lyk) {

    return LISTEN_NEXT_INPUT;
}


void interaction_loop(struct lyk_t* lyk) {
    while(lyk->running) {
        getmaxyx(stdscr, lyk->term_height, lyk->term_width);
        move(0, 0);
        clrtobot();
   
        int listen_input_opt = 0;

        if(lyk->view == VIEW_PROJECTS) {
            listen_input_opt = draw_view__projects(lyk);
        }
        else
        if(lyk->view == VIEW_PROJECT_NOTES) {
            listen_input_opt = draw_view__project_notes(lyk);
        }
       

        mvprintw(0, 0, "Project Notes  -  "
                "Move with arrow keys  -  "
                "Press [TAB] to exit");
        move(1, 0);
        attron(COLOR_PAIR(COLOR_DARK));
        hline('-', lyk->term_width);
        attroff(COLOR_PAIR(COLOR_DARK));


        ncui_clear_events(&lyk->ncui);
        if(listen_input_opt == LISTEN_NEXT_INPUT) {
            input_handler(lyk);
        }
    }
}

int main() {
    struct lyk_t* lyk = lyk_init();

    lyk->ncui.style = (struct ncui_style_t) {
        .selected_attr    = COLOR_PAIR(COLOR_YELLOW) | A_BOLD,
        .unselected_attr  = COLOR_PAIR(COLOR_DARK),

        .selected_inputbox_buffer_attr   = 0,
        .unselected_inputbox_buffer_attr = COLOR_PAIR(COLOR_DARK),

        .inputbox_cursor_char = '|',
        .inputbox_cursor_attr = COLOR_PAIR(COLOR_GREEN) | A_BLINK
    };

    
    read_project_notes(lyk);
    
    init_curses();
    interaction_loop(lyk);
    endwin();

    lyk_quit(lyk);

    return 0;
}

