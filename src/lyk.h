#ifndef LYK_H
#define LYK_H

#include <stddef.h>

#include "string.h"



#define MAX_PROJECTS 64
#define PROJECT_MAX_NOTES 256
#define PROJECT_MAX_NAME_LEN 64


enum view_e {
    VIEW_PROJECTS,
    VIEW_PROJECT_NOTES,

    MAX_VIEWS
};


struct note_t {
    int severity;
    struct string_t title;
    struct string_t desc;
};

struct project_t {
    char          name[PROJECT_MAX_NAME_LEN];
    struct note_t notes[PROJECT_MAX_NOTES];
    size_t        num_notes;
    size_t        name_len;
};

struct lyk_t {
    struct string_t list_path;
    bool running;
    enum view_e   view;
    int           view_cursor;
    int           view_cursor_max;

    int           term_width;
    int           term_height;

    char* selected_input_buffer;
    size_t selected_input_buffer_memsize;

    struct project_t* curr_project;
    struct project_t projects[MAX_PROJECTS];
    size_t num_projects;
};


void free_project(struct project_t* prj);
void free_projects(struct lyk_t* lyk);
void read_project_notes(struct lyk_t* lyk); 


void push_new_project(struct lyk_t* lyk, const char* project_name);
void delete_project(struct lyk_t* lyk, const char* project_name);

void push_new_project_note
(
    struct lyk_t* lyk,
    const char* project_name,
    const char* note_title,
    const char* note_desc,
    int note_severity
);


#endif
