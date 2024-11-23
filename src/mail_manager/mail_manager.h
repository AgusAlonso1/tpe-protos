#ifndef MAIL_MANAGER_H
#define MAIL_MANAGER_H

#include <stddef.h>

typedef struct mail_manager_cdt {

    mail_message * first_message;

    size_t messages_count;
    size_t messages_size; //(octates)

} mail_manager_cdt;

typedef struct mail_message {

    char * path_identifier;
    size_t size; //(octates)
    int index;
    bool deleted;
    mail_message * next;

} mail_message;

//LIST
//OK 2 messages (320 octates)
#endif