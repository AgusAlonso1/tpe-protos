#include "pop3_parser.h"
#include "buffer.h"

/** -------------------------- Definición de funciones static  -------------------------- **/
static enum command_states get_verb(const uint8_t character, struct pop3_command_parser * parser);
static enum command_states get_arg1(const uint8_t character, struct pop3_command_parser * parser);
static enum command_states handle_separator(const uint8_t character);
static enum command_states handle_cr(const uint8_t character);



/** Inicializamos el parser del comando **/
void initialize_command_parser(struct pop3_command_parser * parser) {
    parser->state = verb;
    memset(parser->command, 0, sizeof(*(parser->command)));
    parser->bytes_read = 0;
}

/** Conseguimos el verbo del comando **/
static enum command_states get_verb(const uint8_t character, struct pop3_command_parser * parser) {
    enum command_states next;
    switch (character) {
        case (CR):
            next = cr;
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
    enum command_states next;
    switch (character) {
        case (CR):
            next = cr;
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
        case (CR):
            next = cr;
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

static enum command_states handle_cr(const uint8_t character) {
    return (character == EOL) ? done : error;
}

/** Control general del parseo del comando **/
enum command_states feed_character(const uint8_t character, struct pop3_command_parser * parser) {
    switch (parser->state) {
        case verb:
            return parser->state = get_verb(character, parser);
        case separator:
            return parser->state = handle_separator(character);
        case arg1:
            return parser->state = get_arg1(character, parser);
        case arg2:
            return parser->state = get_arg2(character, parser);
        case cr:
            return parser->state = handle_cr(character);
        default:
            return parser->state = error;
    }
}

/** Se fija si se termino de procesar el comando **/
bool parsing_finished(const enum command_states state, bool * errors) {
    if (state >= error && errors != 0) {
        * errors = true;
    }
    return state >= done;
}

/**  Procesa un comando POP3 desde un buffer **/
enum command_states consume_command(buffer * buffer, struct pop3_command_parser * parser, bool * errors) {
    enum command_states state = parser->state;
    bool finished = false;

    while (buffer_can_read(buffer) && !finished) {
        const uint8_t character = buffer_read(buffer);
        state = feed_character(character, parser);

        finished = parsing_finished(state, errors);
    }

    return state;
}

