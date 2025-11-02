#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/mman.h>
#include <string.h>

#include "fileio.h"


bool file_exists(const char* path) {
    return (access(path, F_OK) == 0);
}

bool dir_exists(const char* path) {
    struct stat sb;
    if(lstat(path, &sb) != 0) {
        return false;
    }
    return (sb.st_mode & S_IFDIR);
}


bool map_file(const char* path, char** out, size_t* out_size) {
    bool result = false;

    int fd = open(path, O_RDONLY);
    struct stat sb;

    if(fd < 0) {
        goto out;
    }

    if(fstat(fd, &sb) < 0) {
        goto out;
    }

    *out_size = sb.st_size;

    if(sb.st_size == 0) {
        return true;
    }

    if(out) {
        *out = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        if(!*out) {
            goto out;
        }
    }

    result = true;

out:

    if(fd > 0) {
        close(fd);
    }

    return result;
}

bool mkdir_p(const char* path, mode_t perm) {
    bool result = false;

    if(!path) {
        goto out;
    }

    const size_t path_length = strlen(path);

    char buffer[256] = { 0 };
    size_t buffer_idx = 0;

    for(size_t i = 0; i < path_length; i++) {
        char ch = path[i];

        buffer[buffer_idx++] = ch;
        if(buffer_idx >= sizeof(buffer)) {
            goto out;
        }

        if(((i > 0) && (ch == '/'))
        || (i+1 >= path_length)) {
            if(!dir_exists(buffer)) {
                if(mkdir(buffer, perm) != 0) {
                    goto out;
                }
            }
        }
    }

    result = true;

out:
    return result;
}


