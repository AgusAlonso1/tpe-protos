#ifndef POP_3
#define POP_3
#include "selector.h"
#include "pop3_parser.h"
#include "utils.h"


#define BUFFER_SIZE 2048
#define USER "USER"
#define PASS "PASS"
#define QUIT "QUIT"
#define STAT "STAT"
#define LIST "LIST"
#define RETR "RETR"
#define DELE "DELE"
#define NOOP "NOOP"
#define RSET "RSET"



void pop3_passive_accept(struct selector_key * sk);
void pop3_write(struct selector_key * sk);
void pop3_read(struct selector_key * sk);
void pop3_close(struct selector_key * sk);
void pop3_block(struct selector_key * sk);
void pop3_done(struct selector_key * sk);

#endif
