#include "pop3_parser.h"

enum pop3_command parse_pop3_command(const char * input) {
    if (strncasecmp(input, "USER", 4) == 0) {
        return CMD_USER;
    } else if (strncasecmp(input, "PASS", 4) == 0) {
        return CMD_PASS;
    } else if (strncasecmp(input, "STAT", 4) == 0) {
        return CMD_STAT;
    } else if (strncasecmp(input, "LIST", 4) == 0) {
        return CMD_LIST;
    } else if (strncasecmp(input, "RETR", 4) == 0) {
        return CMD_RETR;
    } else if (strncasecmp(input, "DELE", 4) == 0) {
        return CMD_DELE;
    } else if (strncasecmp(input, "NOOP", 4) == 0) {
        return CMD_NOOP;
    } else if (strncasecmp(input, "QUIT", 4) == 0) {
        return CMD_QUIT;
    } else {
        return CMD_UNKNOWN;
    }
}

int validate_pop3_command(enum pop3_command cmd, const struct pop3_parser *parser) {
    switch (cmd) {
        case CMD_USER:
        case CMD_PASS:
            return parser->num_args == 1;
        case CMD_STAT:
        case CMD_LIST:
        case CMD_NOOP:
        case CMD_QUIT:
            return parser->num_args == 0;
        case CMD_RETR:
        case CMD_DELE:
            return parser->num_args == 1 && atoi(parser->args[0]) > 0;
        default:
            return 0;
    }
}

void process_pop3_command(struct pop3_parser * parser, const char * input) {
    if (parser->args != NULL) {
        for (size_t i = 0; i < parser->num_args; i++) {
            free(parser->args[i]);
        }
        free(parser->args);
    }

    parser->args = NULL;
    parser->num_args = 0;

    enum pop3_command cmd = parse_pop3_command(input);
    parser->current_command = cmd;

    const char *cmd_end = strchr(input, ' ');
    if (cmd_end != NULL) {
        char *args_str = strdup(cmd_end + 1);
        char *token = strtok(args_str, " \r\n");
        while (token != NULL) {
            parser->args = realloc(parser->args, sizeof(char *) * (parser->num_args + 1));
            parser->args[parser->num_args] = strdup(token);
            parser->num_args++;
            token = strtok(NULL, " \r\n");
        }
        free(args_str);
    }

    if (!validate_pop3_command(cmd, parser)) {
        printf("Comando inválido o mal formado\n");
        parser->current_command = CMD_UNKNOWN;
    }
}

void free_pop3_parser(struct pop3_parser *parser) {
    if (parser->args != NULL) {
        for (size_t i = 0; i < parser->num_args; i++) {
            free(parser->args[i]);
        }
        free(parser->args);
    }
}

