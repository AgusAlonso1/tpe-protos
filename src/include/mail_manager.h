#ifndef MAIL_MANAGER_H
#define MAIL_MANAGER_H

#include <stddef.h>
#include <stdbool.h>
#include "buffer.h"

#define CUR "cur"
#define NEW "new"

typedef struct mail_message {
    char * path_identifier;
    size_t number;
    size_t size; //(octates)
    bool deleted;
} mail_message;

typedef struct mail_manager {
    char * mail_drop;
    mail_message * messages_array;
    size_t messages_capacity;           /** tamaño del array **/
    size_t messages_count;              /** cantidad de mensajes **/
    size_t messages_size; //(octates)
} mail_manager;



void free_mail_manager(struct mail_manager * manager);
struct mail_manager * create_mail_manager(char * mail_drop, char * username);
bool add_mail_message(struct mail_manager * manager, const char * path, size_t size);
void reset_deleted_mail_messages(struct mail_manager * manager, size_t * size, int * messages_count);
bool delete_mail_message(struct mail_manager * manager, int index);
void cleanup_deleted_messages(struct mail_manager * manager);
int execute_and_read(const char* file_path, char** output);
FILE * get_message_content(struct mail_manager * manager, int message_number, char * transformation);
FILE * retrieve_message(struct mail_manager * manager, int message_number, size_t * octets, char * transformation);


#endif
