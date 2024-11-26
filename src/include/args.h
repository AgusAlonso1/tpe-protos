#ifndef ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8
#define ARGS_H_kFlmYm1tW9p5npzDr2opQJ9jM8

#include <stdbool.h>
#include <stdlib.h>

#define MAX_USERS 10

typedef struct user {
    char *name;
    char *pass;
} user;

typedef struct pop3args {
    char * pop3_addr;
    unsigned short pop3_port;

    char * mng_addr;
    unsigned short mng_port;

    char * mail_dirs_path;
    char * tranformation_bin;

    struct user users[MAX_USERS];
    size_t users_len;
} pop3args;

/**
 * Interpreta la linea de comandos (argc, argv) llenando
 * args con defaults o la selección humana. Puede cortar
 * la ejecución.
 */
void init_server_args(int argc, char **argv);

bool exists_user(char * username, char * password);
bool exists_user_name(char * name);

char * get_transformation_bin();
char * get_mail_dirs_path();
char * get_manager_addr();
unsigned short get_manager_port();
char * get_pop3_addr();
unsigned short get_pop3_port();

#endif

