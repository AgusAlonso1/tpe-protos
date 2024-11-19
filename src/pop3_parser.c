#include "pop3_parser.h"
#include "buffer.h"

/** -------------------------- Definición de funciones static  -------------------------- **/
static enum command_states get_verb(uint8_t character, struct pop3_command_parser * parser);
static enum command_states get_arg1(uint8_t character, struct pop3_command_parser * parser);
static enum command_states handle_separator(uint8_t character);
static enum command_states handle_eol(uint8_t character);



/** Inicializamos el parser del comando **/
extern void initialize_command_parser(struct pop3_command_parser * parser) {
    printf("Entramos en initialize_command_parser\n");
    if (parser->command == NULL) {
        parser->command = malloc(sizeof(*(parser->command)));
        if (parser->command == NULL) {
            fprintf(stderr, "Error: Failed to allocate memory for command\n");
            return;
        }
    }
    parser->state = verb;
    memset(parser->command, 0, sizeof(*(parser->command)));
    parser->bytes_read = 0;
}

/** Conseguimos el verbo del comando **/
static enum command_states get_verb(const uint8_t character, struct pop3_command_parser * parser) {
    enum command_states next;
    switch (character) {
        case (EOL):
            next = eol;
            break;
        case (SPACE):
            next = separator;
            break;
        default:
            next = verb;
            if (parser->bytes_read < sizeof(parser->command->verb) - 1) {
                parser->command->verb[parser->bytes_read++] = character;
            }
    }
    if (next != verb) {
        parser->command->verb[parser->bytes_read] = EOS;
        parser->bytes_read = 0;
    }
    return next;
}


/** Conseguimos el primer argumento del comando **/
static enum command_states get_arg1(const uint8_t character, struct pop3_command_parser * parser) {
    printf("Entramos en get_arg1\n");
    enum command_states next;
    switch (character) {
        case (EOL):
            next = eol;
            break;
        case (SPACE):
            next = arg2;
            break;
        default:
            next = arg1;
            if (parser->bytes_read < sizeof(parser->command->arg1) - 1)
                parser->command->arg1[parser->bytes_read++] = character;
    }
    if (next != arg1) {
        parser->command->arg1[parser->bytes_read] = EOS;
        parser->bytes_read = 0;
    }
    return next;
}

/** Conseguimos el segundo argumento del comando **/
static enum command_states get_arg2(const uint8_t character, struct pop3_command_parser * parser) {
    enum command_states next;
    switch (character) {
        case (EOL):
            next = eol;
            break;
        default:
            next = arg2;
            if (parser->bytes_read < sizeof(parser->command->arg2) - 1) {
                parser->command->arg2[parser->bytes_read++] = character;
            }
    }
    if (next != arg2) {
        parser->command->arg2[parser->bytes_read] = EOS;
        parser->bytes_read = 0;
    }
    return next;
}

static enum command_states handle_separator(const uint8_t character) {
    return (character == SPACE) ? arg1 : error;
}

static enum command_states handle_eol(const uint8_t character) {
    return (character == EOL) ? done : error;
}

/** Control general del parseo del comando **/
extern enum command_states feed_character(const uint8_t character, struct pop3_command_parser * parser) {
    switch (parser->state) {
        case verb:
            return parser->state = get_verb(character, parser);
        case separator:
            return parser->state = handle_separator(character);
        case arg1:
            return parser->state = get_arg1(character, parser);
        case arg2:
            return parser->state = get_arg2(character, parser);
        case eol:
            return parser->state = handle_eol(character);
        default:
            return parser->state = error;
    }
}

/** Se fija si se termino de procesar el comando **/
extern bool parsing_finished(const enum command_states state, bool * errors) {
    if (state >= error && errors != 0) {
        * errors = true;
    }
    return state >= done;
}

/**  Procesa un comando POP3 desde un buffer **/
extern enum command_states consume_command(buffer * buffer, struct pop3_command_parser * parser, bool * errors) {
    enum command_states state = parser->state;
    bool finished = false;

    while (buffer_can_read(buffer) && !finished) {
        const uint8_t character = buffer_read(buffer);
        state = feed_character(character, parser);

        parser->state = state;
        finished = parsing_finished(state, errors);

        if(character == SPACE || character == EOL) {
            state = feed_character(character, parser);

            parser->state = state;
            finished = parsing_finished(state, errors);
        }
    }

    return state;
}

