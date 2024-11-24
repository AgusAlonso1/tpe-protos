#ifndef POP_3
#define POP_3
#include "selector.h"
#include "pop3_parser.h"
#include "mail_manager.h"
#include "utils.h"


#define BUFFER_SIZE 2048
#define USER "USER"         /** Arguments: none
                                 Restrictions: none
                                 Possible Responses:
                                    +OK
                                Examples:
                                    C: QUIT
                                    S: +OK dewey POP3 server signing off
                              **/

#define PASS "PASS"           /** Arguments: none
                                 Restrictions: none
                                 Possible Responses:
                                    +OK
                                Examples:
                                    C: QUIT
                                    S: +OK dewey POP3 server signing off
                              **/

#define QUIT "QUIT"          /** Arguments: none
                                 Restrictions: none
                                 Possible Responses:
                                     +OK
                                     -ERR some deleted messages not removed
                                 Examples:
                                     C: QUIT
                                     S: +OK dewey POP3 server signing off (maildrop empty)
                                        ...
                              **/

#define STAT "STAT"          /** Arguments: none
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK nn mm
                                 Examples:
                                     C: STAT
                                     S: +OK 2 320
                              **/

#define LIST "LIST"           /** Arguments: a message-number (optional), which, if present, may NOT
                                             refer to a message marked as deleted
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK scan listing follows
                                     -ERR no such message
                                 Examples:
                                     C: LIST
                                     S: +OK 2 messages (320 octets)
                                     S: 1 120
                              **/

#define RETR "RETR"           /** Arguments: a message-number (required) which may NOT refer to a
                                             message marked as deleted
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK message follows
                                     -ERR no such message
                                 Examples:
                                     C: RETR 1
                                     S: +OK 120 octets
                                     S: <the POP3 server sends the entire message here>
                                     S: .
                              **/

#define DELE "DELE"           /** Arguments: a message-number (required) which may NOT refer to a
                                             message marked as deleted
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK message deleted
                                     -ERR no such message
                                 Examples:
                                     C: DELE 1
                                     S: +OK message 1 deleted
                                        ...
                                     C: DELE 2
                                     S: -ERR message 2 already deleted
                              **/

#define NOOP "NOOP"          /** Arguments: none
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK
                                 Examples:
                                     C: NOOP
                                     S: +OK

                                -> The POP3 server does nothing, it merely replies with a
                                   positive response.
                               **/

#define RSET "RSET"           /** Arguments: none
                                 Restrictions: may only be given in the TRANSACTION state
                                 Possible Responses:
                                     +OK
                                 Examples:
                                     C: RSET
                                     S: +OK maildrop has 2 messages (320 octets)

                                -> If any messages have been marked as deleted by the POP3
                                   server, they are unmarked.
                              **/




void pop3_passive_accept(struct selector_key * sk);
void pop3_write(struct selector_key * sk);
void pop3_read(struct selector_key * sk);
void pop3_close(struct selector_key * sk);
void pop3_block(struct selector_key * sk);
void pop3_done(struct selector_key * sk);

#endif
