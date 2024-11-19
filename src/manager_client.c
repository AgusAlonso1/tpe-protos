#include <string.h>
#include <stdio.h>


#define NORMAL_AMOUNT_OF_ARGS 3
#define HELP_AMOUNT_OF_ARGS 2

#define HELP_ARG_IDX 1
#define BIN_ARG_IDX 0

#define HELP_FLAG "-h"


#define USAGE_MSG "usage: %s [token] [command]\n\
commads:\n\
chistory - History of connections\n\
ccurrent - Current connections\n\
crecord  - Record of concurrent connections\n\
bytess   - Bytes send\n\
bytesr   - Bytes recieved\n\
bytest   - Bytes total\n"

/** Manager Protocol - Client
 *
 * Usage: 
 * 
 * $> manager [TOKEN] [COMMAND]
 * 
 * Command:
 * 
 * chistory - History of connections
 * ccurrent - Current connections
 * crecord  - Record of concurrent connections
 * bytess   - Bytes send
 * bytesr   - Bytes recieved
 * bytest   - Bytes total
 * 

*/

static char * error_msg;

int main(int argc, char * argv[]) {
    if (argc = HELP_AMOUNT_OF_ARGS && strcmp(argv[HELP_ARG_IDX], HELP_FLAG)) {
        fprintf(stdout, USAGE_MSG, BIN_ARG_IDX);
        return;
    }

    if (argc != NORMAL_AMOUNT_OF_ARGS) {
        error_msg = "djwjkdwo"; 
        goto finally;
    }




finally:
}