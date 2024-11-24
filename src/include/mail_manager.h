#ifndef MAIL_MANAGER_H
#define MAIL_MANAGER_H

#include <stddef.h>
#include <stdbool.h>


typedef struct mail_message {
    char * path_identifier;
    size_t size; //(octates)
    int index;
    bool deleted;
    struct mail_message * next;
} mail_message;

typedef struct mail_manager {
    mail_message * first_message;

    size_t messages_count;
    size_t messages_size; //(octates)
} mail_manager;


mail_message * create_mail_message(const char * path, size_t size, int index);
void free_mail_message(struct mail_message * message);
int create_mail_manager(struct mail_manager *manager);
bool add_mail_message(struct mail_manager * manager, mail_message * message);
void reset_deleted_mail_messages(struct mail_manager * manager);
void cleanup_deleted_messages(struct mail_manager * manager);


#endif