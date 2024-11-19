#ifndef POP3_PARSER_H
#define POP3_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include "buffer.h"


#define CR '\r'
#define SPACE ' '
#define EOL '\n'
#define EOS '\0'
#define MAX_VERB_LENGTH 5
#define MAX_ARG_LENGTH 256


/** Estructura general de comandos POP3:
 *      USER <username>
 *      PASS <password>
 *      QUIT
 *      STAT
 *      LIST
 *      RETR <msg number>
 *      DELE <msg number>
 *      RSET
 *      TOP <msg> <lines>
 **/
struct command {
    char verb[MAX_VERB_LENGTH];        /** Ningún verbo del comando tiene mas de 4 caracteres **/
    char arg1[MAX_ARG_LENGTH];
    char arg2[MAX_ARG_LENGTH];    /** A lo sumo vamos a tener dos argumentos (comando TOP) **/
};

/** Estados del parsing **/
enum command_states {
    verb,
    separator,
    arg1,
    arg2,
    eol,
    cr,
    done,
    error
};

/** Estructura para guardar tanto el comando parseado como le estado actual del parsing; también los bytes leídos **/
struct pop3_command_parser {
    struct command * command;
    enum command_states state;
    size_t bytes_read;
};

void initialize_command_parser(struct pop3_command_parser * parser);
bool parsing_finished(enum command_states state, bool * errored);
enum command_states consume_command(buffer * buffer, struct pop3_command_parser * parser, bool * errors);
enum command_states feed_character(uint8_t character, struct pop3_command_parser * parser);

#endif

