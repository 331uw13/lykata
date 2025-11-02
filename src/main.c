#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <errno.h>
#include <stdbool.h>

#include <ncurses.h>
//#include <json-c/json.h>

#include "string.h"
#include "fileio.h"
#include "lyk.h"
#include "tui.h"

#define LIST_FILE_DIR ".lyk/"
#define LIST_FILENAME "project-notes.json"

char PROJECT_NAME_BUFFER[32] = { 0 };




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


void handle_enter_key(struct lyk_t* lyk);
void handle_delete_key(struct lyk_t* lyk);


void input_handler(struct lyk_t* lyk) {

    int ch = getch();

    switch(ch) {
        case 0x09:
            lyk->running = false;
            break;

            
        case KEY_LEFT:
            break;

        case KEY_RIGHT:
            break;


        case KEY_UP:
            if(lyk->view_cursor > 0) {
                lyk->view_cursor--;
                lyk->selected_input_buffer = NULL;
                lyk->selected_input_buffer_memsize = 0;
            }
            break;

        case KEY_DOWN:
            if(lyk->view_cursor+1 < lyk->view_cursor_max) {
                lyk->view_cursor++;
                lyk->selected_input_buffer = NULL;
                lyk->selected_input_buffer_memsize = 0;
            }
            break;

        case KEY_BACKSPACE:
            if(lyk->selected_input_buffer
            && lyk->selected_input_buffer_memsize) {
                const size_t buffer_len = strlen(lyk->selected_input_buffer);
                lyk->selected_input_buffer[(buffer_len > 0) ? buffer_len-1 : 0] = 0;
            }
            break;

        case 'x':
            handle_delete_key(lyk);
            break;

        case 0x0A:
            handle_enter_key(lyk);
            break;

        default:
            if(ch >= 0x20 && ch <= 0x7E) {
                if(lyk->selected_input_buffer
                && lyk->selected_input_buffer_memsize) {
                    const size_t buffer_len = strlen(lyk->selected_input_buffer);
                    if(buffer_len+1 < lyk->selected_input_buffer_memsize) {
                        lyk->selected_input_buffer[buffer_len] = ch;
                        //asm("int3");
                    }
                }
            }   
            break;
    }
}

// VIEW_PROJECTS Cursor IDs:
#define INPUT__PROJECT_NAME 0
#define BUTTON__ADD_PROJECT 1

// VIEW_PROJECT_NOTES Cursor IDs:
// ...

void handle_delete_key(struct lyk_t* lyk) {
    if(lyk->view == VIEW_PROJECTS) {

        // There are 2 elements before project name list:
        // INPUT__PROJECT_NAME and BUTTON__ADD_PROJECT.
        if(lyk->view_cursor < 2) {
            return;
        }

        int64_t proj_index = lyk->view_cursor - 2;
        if(proj_index >= (int64_t)lyk->num_projects) {
            return;
        }

        struct project_t* project = &lyk->projects[proj_index];

        if(confirm_user_action(lyk, COLOR_RED, 
                    "Delete \"%s\" notes?\n"
                    "This action cannot be undone!",
                    project->name) == USER_ACTION_DECLINED) {
            return;
        }

        delete_project(lyk, project->name);
        if((int64_t)lyk->view_cursor-2 >= (int64_t)lyk->num_projects) {
            lyk->view_cursor = lyk->num_projects + 2 - 1;
        }

    }
    else
    if(lyk->view == VIEW_PROJECT_NOTES) {
    
    }

}

void handle_enter_key(struct lyk_t* lyk) {
    //asm("int3");
    if(lyk->view == VIEW_PROJECTS) {
        
        if(lyk->view_cursor == BUTTON__ADD_PROJECT) {

            const size_t project_name_len = strlen(PROJECT_NAME_BUFFER);
            if(project_name_len > 0) {
                push_new_project(lyk, PROJECT_NAME_BUFFER);
                memset(PROJECT_NAME_BUFFER, 0, project_name_len);
            }
        }
        else
        if(lyk->view_cursor > BUTTON__ADD_PROJECT) {
            // Some of project names was selected.

            int64_t proj_index = lyk->view_cursor - 2;
            if(proj_index >= (int64_t)lyk->num_projects) {
                return;
            }

            lyk->curr_project = &lyk->projects[proj_index];
        
            lyk->view = VIEW_PROJECT_NOTES;
        }
    }
    else
    if((lyk->view == VIEW_PROJECT_NOTES) && (lyk->curr_project)) {
       


    }
}

void draw_view__projects(struct lyk_t* lyk) {
    
    lyk->view_cursor_max = 2 + lyk->num_projects;

    draw_input_field(INPUT__PROJECT_NAME,
            lyk, 2, 1, PROJECT_NAME_BUFFER, sizeof(PROJECT_NAME_BUFFER), "Project name: ");
    
    draw_button(BUTTON__ADD_PROJECT,
            lyk, 3, 1, "[Add]", "<Enter>", A_BLINK);


    for(size_t i = 0; i < lyk->num_projects; i++) {
        struct project_t* proj = &lyk->projects[i];
        if((size_t)lyk->view_cursor == 2+i) {
            mvaddch(5+i, 3, '>');
        }
        draw_button(2+i, lyk, 5+i, 5, proj->name, " <Enter> 'view notes', <X> 'delete'", A_DIM);
    }

}

void draw_view__project_notes(struct lyk_t* lyk) {
    if(!lyk->curr_project) {
        return;
    }

    for(size_t i = 0; i < lyk->curr_project->num_notes; i++) {
        struct note_t* note = &lyk->curr_project->notes[i];

        mvaddstr(5+i, 2, note->title.bytes);
    }

}


void interaction_loop(struct lyk_t* lyk) {
    while(lyk->running) {
        getmaxyx(stdscr, lyk->term_height, lyk->term_width);
        move(0, 0);
        clrtobot();
    
        if(lyk->view == VIEW_PROJECTS) {
            draw_view__projects(lyk);
        }
        else
        if(lyk->view == VIEW_PROJECT_NOTES) {
            draw_view__project_notes(lyk);
        }
       

        mvprintw(0, 0, "Project Notes  -  "
                "Move with arrow keys  -  "
                "Press [TAB] to exit  %i/%i [%p %li]",
                lyk->view_cursor,
                lyk->view_cursor_max,
                lyk->selected_input_buffer,
                lyk->selected_input_buffer_memsize);

        move(1, 0);
        attron(COLOR_PAIR(COLOR_DARK));
        hline('-', lyk->term_width);
        attroff(COLOR_PAIR(COLOR_DARK));


        input_handler(lyk);
    }
}

int main() {

    // For user home directory.
    // TODO: Check env variable first.
    struct passwd* pw = getpwuid(getuid());
    if(!pw) {
        fprintf(stderr, "Failed to get home dir.\n");
        return 1;
    }

    struct lyk_t lyk;
    lyk.list_path = create_string();
    lyk.running = true;
    lyk.view = VIEW_PROJECTS;
    lyk.view_cursor = 0;
    lyk.selected_input_buffer = NULL;
    lyk.selected_input_buffer_memsize = 0;
    lyk.curr_project = NULL;

    // Create path to note list json.
    string_move(&lyk.list_path, pw->pw_dir, strlen(pw->pw_dir));
    if(string_lastbyte(&lyk.list_path) != '/') {
        string_pushbyte(&lyk.list_path, '/');
    }

    string_append(&lyk.list_path, LIST_FILE_DIR, strlen(LIST_FILE_DIR));
    if(!dir_exists(lyk.list_path.bytes)) {
        printf("\"%s\" Doesnt exist, create it? (yes/no): ", lyk.list_path.bytes);
        fflush(stdout);

        char input[6] = { 0 };
        read(STDIN_FILENO, input, sizeof(input));

        if((input[0] != 'y') && (input[0] != 'Y')) {
            string_free(&lyk.list_path);
            return 0;
        }
        if(!mkdir_p(lyk.list_path.bytes, S_IRWXU)) {
            fprintf(stderr, "Failed to create directory (or its parent dirs) \"%s\" | %s\n",
                    lyk.list_path.bytes, strerror(errno));
            string_free(&lyk.list_path);
            return 1;
        }
    }

    string_append(&lyk.list_path, LIST_FILENAME, strlen(LIST_FILENAME));    
    if(!file_exists(lyk.list_path.bytes)) {
        if(creat(lyk.list_path.bytes, S_IRUSR | S_IWUSR) < 0) {
            fprintf(stderr, "Failed to create file \"%s\" | %s\n", 
                    lyk.list_path.bytes, strerror(errno));
            string_free(&lyk.list_path);
            return 1;
        }
    }


    //delete_project(&lyk, "project_name_B");
    //push_new_project(&lyk, "test_project_name");

    //push_new_project_note(&lyk, "test_project_name", "note title goes here", "no descripotinhaha", 40);
    //push_new_project_note(&lyk, "test_project_name", "toinen asia", "no desc", 80);

    
    read_project_notes(&lyk);
    
    init_curses();
    interaction_loop(&lyk);
    endwin();

    string_free(&lyk.list_path); 
    free_projects(&lyk);
    
    return 0;
}


