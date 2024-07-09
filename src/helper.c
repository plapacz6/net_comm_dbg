#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
// #include <sys/stat.h>
// #include <sys/types.h>

#include "helper.h"

void check_fd_mode(int fd) {
    printf("file descriptor %d opened in mode: ", fd);
    int fd_mode = fcntl(fd, F_GETFL);
    switch(fd_mode) {
        case O_RDONLY : {
            printf("%s\n", "O_RDONLY");
            break;
        }
        case O_WRONLY : {
            printf("%s\n", "O_WRONLY");
            break;
        }        
        case O_RDWR : {
            printf("%s\n", "O_RDWR");
            break;
        }        
        case (O_WRONLY | O_APPEND) : {
            printf("%s\n", "O_WRONLY | O_APPEND");
            break;
        }        
        case (O_RDWR | O_APPEND) : {
            printf("%s\n", "O_RDWR | O_APPEND");
            break;
        }                
        default : {
            printf("%d : %s\n", fd_mode, "defaut: unknon");
            break;
        }        
    }
}