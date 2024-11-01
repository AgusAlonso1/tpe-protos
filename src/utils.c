#include "utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

void validate(int code, char * msg) {
    if (code == ERROR_CODE) {
        perror(msg);
        exit(errno);
    }
}
