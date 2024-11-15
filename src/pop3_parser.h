#ifndef POP3_PARSER_H
#define POP3_PARSER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum pop3_command {
    CMD_USER,
    CMD_PASS,
    CMD_STAT,
    CMD_LIST,
    CMD_RETR,
    CMD_DELE,
    CMD_NOOP,
    CMD_QUIT,
    CMD_UNKNOWN
};

struct pop3_parser {
    char ** args;
    size_t num_args;
    enum pop3_command current_command;
};

void free_pop3_parser(struct pop3_parser *parser);

enum pop3_command parse_pop3_command(const char *input);
int validate_pop3_command(enum pop3_command cmd, const struct pop3_parser *parser);
void process_pop3_command(struct pop3_parser *parser, const char *input);


#endif

