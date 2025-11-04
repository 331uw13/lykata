#ifndef LYK_H
#define LYK_H

#include <stddef.h>

#include "string.h"
#include "ncui/ncui.h"


#define MAX_PROJECTS 64
#define PROJECT_MAX_NOTES 256
#define PROJECT_MAX_NAME_LEN 64


enum view_e {
    VIEW_PROJECTS,
    VIEW_PROJECT_NOTES,

    MAX_VIEWS
};


struct note_t {
    int             severity;
    struct string_t title;
    struct string_t desc; // Description.
    bool            desc_open;
};

struct project_t {
    char          name[PROJECT_MAX_NAME_LEN];
    struct note_t notes[PROJECT_MAX_NOTES];
    size_t        num_notes;
    size_t        name_len;
};

struct lyk_t {
    struct ncui_t ncui;

    bool running;
    struct string_t list_path;
    
    enum view_e   view;

    int           term_width;
    int           term_height;

    struct project_t* curr_project;
    struct project_t projects[MAX_PROJECTS];
    size_t num_projects;
};


struct lyk_t* lyk_init();
void          lyk_quit(struct lyk_t* lyk);

void free_project(struct project_t* prj);
void free_projects(struct lyk_t* lyk);
void read_project_notes(struct lyk_t* lyk); 

void create_new_project(struct lyk_t* lyk, const char* project_name);
void delete_project(struct lyk_t* lyk, const char* project_name);
void delete_project_note(struct lyk_t* lyk, const char* project_name, const char* note_title);

void create_new_project_note
(
    struct lyk_t* lyk,
    const char* project_name,
    const char* note_title,
    const char* note_desc,
    int note_severity
);


#endif
