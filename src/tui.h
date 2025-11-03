#ifndef LYK_TUI_UTILS_H
#define LYK_TUI_UTILS_H

#include "lyk.h"


// Extra ncurses color pairs
#define COLOR_CURSOR 8   
#define COLOR_DARK 9
#define COLOR_VERY_DARK 10


#define USER_ACTION_DECLINED 0
#define USER_ACTION_CONFIRMED 1
int confirm_user_action(struct lyk_t* lyk, int message_color_pair, const char* message_fmt, ...);


#endif
