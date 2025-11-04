#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>

#include <ncurses.h>

#include "lyk.h"
#include "tui.h"



#define DELETE_SELECTED_CH 'x'

char PROJECT_NAME_BUFFER[32] = { 0 };
char NOTE_TITLE_BUFFER[64] = { 0 };
char NOTE_DESC_BUFFER[256] = { 0 };
char NOTE_SEVERITY_BUFFER[8] = { 0 };




inline void create_color_pair(int pair, int r, int g, int b) {
    init_color(pair, r, g, b);
    init_pair(pair, pair, -1); 
}

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

    create_color_pair(COLOR_DARK, 400, 400, 400);
    create_color_pair(COLOR_VERY_DARK, 200, 200, 200);

    create_color_pair(COLOR_INDICATOR_VERY_HIGH, 1000, 100, 400);  // >= 90
    create_color_pair(COLOR_INDICATOR_HIGH, 1000, 300, 100);       // >= 70
    create_color_pair(COLOR_INDICATOR_MEDIUM, 1000, 500, 100);     // >= 50
    create_color_pair(COLOR_INDICATOR_LOW_MEDIUM, 700, 800, 100);  // >= 30
    create_color_pair(COLOR_INDICATOR_LOW, 500, 1000, 100);        // >= 20
    create_color_pair(COLOR_INDICATOR_VERY_LOW, 500, 500, 500);    // < 20
}

//void handle_delete_selected(struct lyk_t* lyk);

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

            /*
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
            */

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

    uint32_t proj_index = lyk->ncui.cursor_y - 2;
    if(proj_index >= lyk->num_projects) {
        goto out;
    }

    proj = &lyk->projects[proj_index];

out:
    return proj;
}

/*
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
*/

#define LISTEN_NEXT_INPUT 1
#define SKIP_NEXT_INPUT 2

int draw_view__projects(struct lyk_t* lyk) {
    ncui_new_update_begin(&lyk->ncui);
    ncui_set_cursor_max_y(&lyk->ncui, 1 + lyk->num_projects);


    if(ncui_button(&lyk->ncui,
                (struct ncui_elem_mappos_t){ 0, 1 },
                (struct ncui_elem_drawpos_t){ 2, 4 },
                "`-> [Add]", " <Enter>", COLOR_PAIR(COLOR_DARK) | A_BLINK)) {

        if(strlen(PROJECT_NAME_BUFFER) > 0) {
            create_new_project(lyk, PROJECT_NAME_BUFFER);
            memset(PROJECT_NAME_BUFFER, 0, sizeof(PROJECT_NAME_BUFFER));
        }
    }

    ncui_inputbox(&lyk->ncui, 
            (struct ncui_elem_mappos_t){ 0, 0 },
            (struct ncui_elem_drawpos_t){ 2, 3 },
            ",- Project name: ",
            PROJECT_NAME_BUFFER, sizeof(PROJECT_NAME_BUFFER));

    for(size_t i = 0; i < lyk->num_projects; i++) {
        struct project_t* proj = &lyk->projects[i];
        if(ncui_button(&lyk->ncui,
                    (struct ncui_elem_mappos_t){ 0, 2 + i },
                    (struct ncui_elem_drawpos_t){ 4, 6 + i },
                    proj->name, NULL, 0)) {

            lyk->curr_project = proj;
            lyk->view = VIEW_PROJECT_NOTES;
            lyk->ncui.cursor_y = 0;
            return SKIP_NEXT_INPUT;
        }

        if(lyk->ncui.cursor_y-2 == (int64_t)i) {
            if(ncui_button(&lyk->ncui,
                        (struct ncui_elem_mappos_t){ 1, 2 + i },
                        (struct ncui_elem_drawpos_t){ 5 + strlen(proj->name), 6 + i },
                        "[x]", NULL, 0)) {

                int result = confirm_user_action(lyk, 0, 
                        "Are you sure you want to delete \"%s\" notes?\n"
                        "This action cannot be undone", proj->name);
    
                if(result == USER_ACTION_CONFIRMED) {
                    delete_project(lyk, proj->name);
                    if(lyk->ncui.cursor_y >= lyk->ncui.cursor_max_y) {
                        lyk->ncui.cursor_y = lyk->ncui.cursor_max_y - 1;
                    }
                }
                return SKIP_NEXT_INPUT;
            }
        }
    }

    return LISTEN_NEXT_INPUT;
}


int draw_view__project_notes(struct lyk_t* lyk) {


    if(!lyk->curr_project) {
        mvprintw(10, 10, "[ ERROR: No project pointer was set ]");
        goto out;
    }

    mvprintw(3, 30, "%s  -  %li notes", lyk->curr_project->name, lyk->curr_project->num_notes);

    ncui_new_update_begin(&lyk->ncui);
    ncui_set_cursor_max_y(&lyk->ncui, 4 + lyk->curr_project->num_notes);


    if(ncui_button(&lyk->ncui,
                (struct ncui_elem_mappos_t){ 0, 0 },
                (struct ncui_elem_drawpos_t){ 2, 3 },
                "[Back]", " <Enter>", COLOR_PAIR(COLOR_DARK) | A_BLINK)) {
        lyk->view = VIEW_PROJECTS;
        lyk->ncui.cursor_y = 0;
        return SKIP_NEXT_INPUT;
    }

    ncui_inputbox(&lyk->ncui, 
            (struct ncui_elem_mappos_t){ 0, 1 },
            (struct ncui_elem_drawpos_t){ 4, 5 },
            ",- Note title: ",
            NOTE_TITLE_BUFFER, sizeof(NOTE_TITLE_BUFFER));

    ncui_inputbox(&lyk->ncui,
            (struct ncui_elem_mappos_t){ 0, 2 },
            (struct ncui_elem_drawpos_t){ 4, 6 },
            "|- Note desc: ",
            NOTE_DESC_BUFFER, sizeof(NOTE_DESC_BUFFER));

    ncui_inputbox(&lyk->ncui,
            (struct ncui_elem_mappos_t){ 0, 3 },
            (struct ncui_elem_drawpos_t){ 4, 7 },
            "|- Note severity: ",
            NOTE_SEVERITY_BUFFER, sizeof(NOTE_SEVERITY_BUFFER));

    if(ncui_button(&lyk->ncui,
            (struct ncui_elem_mappos_t){ 0, 4 },
            (struct ncui_elem_drawpos_t){ 4, 8 },
            "`-> [Add]", " <Enter>", COLOR_PAIR(COLOR_DARK) | A_BLINK)) {
        const size_t note_title_len = strlen(NOTE_TITLE_BUFFER);
        const size_t note_desc_len = strlen(NOTE_DESC_BUFFER);
        const size_t note_severity_len = strlen(NOTE_SEVERITY_BUFFER);

        if(note_title_len && note_desc_len && note_severity_len) {
    
            int note_severity = atoi(NOTE_SEVERITY_BUFFER);
            note_severity = (note_severity < 0) ? 0 : (note_severity > 100) ? 100 : note_severity;
            
            create_new_project_note(lyk,
                    lyk->curr_project->name,
                    NOTE_TITLE_BUFFER,
                    NOTE_DESC_BUFFER,
                    note_severity);

            memset(NOTE_TITLE_BUFFER, 0, note_title_len);
            memset(NOTE_DESC_BUFFER, 0, note_desc_len);
            memset(NOTE_SEVERITY_BUFFER, 0, note_severity_len);
        }
    }

    int note_draw_y = 10;

    for(size_t i = 0; i < lyk->curr_project->num_notes; i++) {
        struct note_t* note = &lyk->curr_project->notes[i];

        //int note_draw_y = 10 + i;
        int indicator_attr = 0;

        if(note->severity >= 90) {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_VERY_HIGH);
        }
        else
        if(note->severity >= 70) {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_HIGH);
        }
        else
        if(note->severity >= 50) {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_MEDIUM);
        }
        else
        if(note->severity >= 30) {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_LOW_MEDIUM);
        }
        else
        if(note->severity >= 20) {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_LOW);
        }
        else {
            indicator_attr = COLOR_PAIR(COLOR_INDICATOR_VERY_LOW);
        }

        indicator_attr |= A_BOLD;

        attron(indicator_attr);
        mvprintw(note_draw_y, 3, "(%i)", note->severity);
        attroff(indicator_attr);


        // Open/Close description button.
        if(ncui_button(&lyk->ncui,
                (struct ncui_elem_mappos_t){ 0, 5+i },
                (struct ncui_elem_drawpos_t){ 14, note_draw_y },
                note->title.bytes, NULL, 0)) {
            note->desc_open = !note->desc_open;
        } 


        if(lyk->ncui.cursor_y-5 == (int64_t)i) {
            // Delete note button.
            if(ncui_button(&lyk->ncui,
                    (struct ncui_elem_mappos_t){ 1, 5+i },
                    (struct ncui_elem_drawpos_t){ 14 + note->title.size + 1, note_draw_y },
                    "[x]", NULL, 0)) {
        
                int result = confirm_user_action(lyk, COLOR_RED, 
                        "Are you sure you want to delete\n"
                        "\"%s\" ?\n"
                        "This action cannot be undone!", note->title.bytes);
                
                if(result == USER_ACTION_CONFIRMED) {
                    delete_project_note(lyk, lyk->curr_project->name, note->title.bytes);
                    if(lyk->ncui.cursor_y >= lyk->ncui.cursor_max_y) {
                        lyk->ncui.cursor_y = lyk->ncui.cursor_max_y - 1;
                    }
                }
                 
                return SKIP_NEXT_INPUT;
            }
        }
               
        mvaddch(note_draw_y, 12, note->desc_open ? 'v' : '>');
    
        note_draw_y++;

        if(note->desc_open) {
        
            mvaddstr(note_draw_y, 16, note->desc.bytes);
            note_draw_y++;

        }
    }


out:
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

