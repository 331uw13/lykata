#include <string.h>

#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <errno.h>

#include <pwd.h>
#include <ncurses.h>
#include <json-c/json.h>


#include "lyk.h"
#include "fileio.h"
#include "tui.h"


#define LIST_FILE_DIR ".lyk/"
#define LIST_FILENAME "project-notes.json"




struct lyk_t* lyk_init() {

    // For user home directory.
    // TODO: Check env variable first.
    struct passwd* pw = getpwuid(getuid());
    if(!pw) {
        fprintf(stderr, "Failed to get home dir.\n");
        return NULL;
    }

    struct lyk_t* lyk = malloc(sizeof *lyk);
    lyk->list_path = create_string();
    lyk->running = true;
    lyk->view = VIEW_PROJECTS;
    lyk->curr_project = NULL;

    lyk->ncui = ncui_init
    (
        32,  // Max elements rows.
        8   // Max elements columns.
    );

    //printf("%li\n", sizeof(lyk));

    // Create path to note list json.
    string_move(&lyk->list_path, pw->pw_dir, strlen(pw->pw_dir));
    if(string_lastbyte(&lyk->list_path) != '/') {
        string_pushbyte(&lyk->list_path, '/');
    }

    string_append(&lyk->list_path, LIST_FILE_DIR, strlen(LIST_FILE_DIR));
    if(!dir_exists(lyk->list_path.bytes)) {
        printf("\"%s\" Doesnt exist, create it? (yes/no): ", lyk->list_path.bytes);
        fflush(stdout);

        char input[6] = { 0 };
        read(STDIN_FILENO, input, sizeof(input));

        if((input[0] != 'y') && (input[0] != 'Y')) {
            lyk_quit(lyk);
            return NULL;
        }
        if(!mkdir_p(lyk->list_path.bytes, S_IRWXU)) {
            fprintf(stderr, "Failed to create directory (or its parent dirs) \"%s\" | %s\n",
                    lyk->list_path.bytes, strerror(errno));
            lyk_quit(lyk);
            return NULL;
        }
    }

    string_append(&lyk->list_path, LIST_FILENAME, strlen(LIST_FILENAME));    
    if(!file_exists(lyk->list_path.bytes)) {
        if(creat(lyk->list_path.bytes, S_IRUSR | S_IWUSR) < 0) {
            fprintf(stderr, "Failed to create file \"%s\" | %s\n", 
                    lyk->list_path.bytes, strerror(errno));
            lyk_quit(lyk);
            return NULL;
        }
    }

    return lyk;
}


void lyk_quit(struct lyk_t* lyk) {
    string_free(&lyk->list_path); 
    free_projects(lyk);
    free_ncui(&lyk->ncui);
    free(lyk);
}

void free_project(struct project_t* prj) {
    for(size_t i = 0; i < prj->num_notes; i++) {
        string_free(&prj->notes[i].title);
        string_free(&prj->notes[i].desc);
    }
    prj->num_notes = 0;
}

void free_projects(struct lyk_t* lyk) {
    for(size_t i = 0; i < lyk->num_projects; i++) {
        free_project(&lyk->projects[i]);
    }
    lyk->num_projects = 0;
} 


static int cmp_note_severity(const void* p1, const void* p2) {
    struct note_t* n1 = (struct note_t*)p1;
    struct note_t* n2 = (struct note_t*)p2;

    return (n1->severity < n2->severity);
}

void read_project_notes(struct lyk_t* lyk) {
    char* file = NULL;
    size_t file_size = 0;

    free_projects(lyk);
    map_file(lyk->list_path.bytes, &file, &file_size);

    if(file_size == 0) {
        return;
    }

    struct json_object* objects = json_tokener_parse(file);
    json_object_object_foreach(objects, key, val) {
        struct project_t* proj = &lyk->projects[lyk->num_projects++];

        memset(proj->name, 0, PROJECT_MAX_NAME_LEN);
        memcpy(proj->name, key, strlen(key));

        size_t arr_len = json_object_array_length(val);
       
        for(size_t i = 0; i < arr_len; i++) {
            struct note_t* note = &proj->notes[proj->num_notes];
            struct json_object* elem_obj = json_object_array_get_idx(val, i);
     
            note->title = create_string();
            note->desc = create_string();

            struct json_object* severity_obj = json_object_object_get(elem_obj, "severity");
            int severity = json_object_get_int(severity_obj);

            struct json_object* title_obj = json_object_object_get(elem_obj, "title");
            const char* title = json_object_get_string(title_obj);

            struct json_object* desc_obj = json_object_object_get(elem_obj, "desc");
            const char* desc = json_object_get_string(desc_obj);

            note->severity = severity;
            string_move(&note->title, (char*)title, strlen(title));
            string_move(&note->desc, (char*)desc, strlen(desc));
            note->desc_open = false;

            proj->num_notes++;
            if(proj->num_notes >= PROJECT_MAX_NOTES) {
                //asm("int3");
                break;
            }
        }
    
        // Sort notes by their severity.
        if(proj->num_notes > 0) {
            qsort(proj->notes, proj->num_notes, sizeof *proj->notes, cmp_note_severity);
        }
    }
        
    json_object_put(objects);
    munmap(file, file_size);




}



void create_new_project(struct lyk_t* lyk, const char* project_name) {
    char* file = NULL;
    size_t file_size = 0;

    if(!map_file(lyk->list_path.bytes, &file, &file_size)) {
        return;
    }

    if(file_size == 0) {
        return;
    }
    
    struct json_object* objects = json_tokener_parse(file);
    struct json_object* notes_arr = json_object_new_array();

    json_object_object_add(objects, project_name, notes_arr);

    int fd = open(lyk->list_path.bytes, O_WRONLY | O_TRUNC);
    if(fd > 0) {
        const char* updated_json = json_object_to_json_string_ext(objects, 
                JSON_C_TO_STRING_PLAIN | 
                JSON_C_TO_STRING_PRETTY | 
                JSON_C_TO_STRING_SPACED);

        write(fd, updated_json, strlen(updated_json));
        close(fd);
    }
    // TODO: Report errors.

    json_object_put(objects);
    munmap(file, file_size);

    // Update files.
    read_project_notes(lyk);
}

void create_new_project_note
(
    struct lyk_t* lyk,
    const char* project_name,
    const char* note_title,
    const char* note_desc,
    int note_severity
){
    char* file = NULL;
    size_t file_size = 0;

    if(!map_file(lyk->list_path.bytes, &file, &file_size)) {
        return;
    }

    if(file_size == 0) {
        return;
    }
    
    struct json_object* objects = json_tokener_parse(file);

    struct json_object* notes_arr = json_object_object_get(objects, project_name);
    if(!notes_arr) {
        goto out;
    }


    struct json_object* note_obj = json_object_new_object();
    
    json_object_object_add(note_obj, "severity", json_object_new_int(note_severity));
    json_object_object_add(note_obj, "title", json_object_new_string(note_title));
    json_object_object_add(note_obj, "desc", json_object_new_string(note_desc));

    json_object_array_add(notes_arr, note_obj);
    
    int fd = open(lyk->list_path.bytes, O_WRONLY | O_TRUNC);
    if(fd > 0) {
        const char* updated_json = json_object_to_json_string_ext(objects, 
                JSON_C_TO_STRING_PLAIN | 
                JSON_C_TO_STRING_PRETTY | 
                JSON_C_TO_STRING_SPACED);

        write(fd, updated_json, strlen(updated_json));
        close(fd);
    }
    // TODO: Report errors.

out:
    json_object_put(objects);
    munmap(file, file_size);

    read_project_notes(lyk);
}

void delete_project(struct lyk_t* lyk, const char* project_name) {
    char* file = NULL;
    size_t file_size = 0;

    if(!map_file(lyk->list_path.bytes, &file, &file_size)) {
        return;
    }

    if(file_size == 0) {
        return;
    }
    
    struct json_object* objects = json_tokener_parse(file);
    json_object_object_del(objects, project_name);

    int fd = open(lyk->list_path.bytes, O_WRONLY | O_TRUNC);
    if(fd > 0) {
        const char* updated_json = json_object_to_json_string_ext(objects, 
                JSON_C_TO_STRING_PLAIN | 
                JSON_C_TO_STRING_PRETTY | 
                JSON_C_TO_STRING_SPACED);

        write(fd, updated_json, strlen(updated_json));
        close(fd);
    }
    // TODO: Report errors.

    json_object_put(objects);
    munmap(file, file_size);

    // Update files.
    read_project_notes(lyk);
}

/*
void delete_selected_project(struct lyk_t* lyk) {
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
*/

