#ifndef POP_3
#define POP_3
#include "selector.h"

void pop3_passive_accept(struct selector_key * sk);

void pop3_write(struct selector_key * sk);
void pop3_read(struct selector_key * sk);
void pop3_close(struct selector_key * sk);
void pop3_block(struct selector_key * sk);

#endif
