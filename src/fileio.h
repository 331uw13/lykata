#ifndef LYK_FILEIO_H
#define LYK_FILEIO_H

#include <sys/mman.h>



bool file_exists(const char* path);
bool dir_exists(const char* path);

bool map_file(const char* path, char** out, size_t* out_size); 
bool mkdir_p(const char* path, mode_t perm);


#endif
