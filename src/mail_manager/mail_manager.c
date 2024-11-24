#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>
#include "../include/mail_manager.h"

#define ERROR (-1)
#define SUCCESS 0


// crear mail_message ✅
//crear mail_manager ✅
// agregar un mail_message al manager ✅
// marcar como eliminado ✅
// elimiar las marcadas como eliminados ✅
// resetear los eliminados ✅
// abrir un mail_message seleccionado --> en proceso con transformaciones
// recorrer e imprimir la lista


mail_message * create_mail_message(const char * path, size_t size, int index) {
    mail_message * new_message = malloc(sizeof(mail_message));
    if (!new_message) {
        return NULL;
    }
    new_message->path_identifier = strdup(path);
    if (!new_message->path_identifier) {
        free(new_message);
        return NULL;
    }
    new_message->size = size;
    new_message->index = index;
    new_message->deleted = false;
    new_message->next = NULL;
    return new_message;
}


void free_mail_message(mail_message * message) {
    if (message) {
        free(message->path_identifier);
        free(message);
    }
}


int create_mail_manager(struct mail_manager *manager) {
    if (!manager) {
        return ERROR;
    }
    manager->first_message = NULL;
    manager->messages_count = 0;
    manager->messages_size = 0;

    return SUCCESS;
}

bool add_mail_message(struct mail_manager * manager, mail_message * message) {
    if(!manager || !message) {
        return false;
    }

    if(manager->first_message == NULL) {
        manager->first_message = message;
    } else {
        mail_message * current = manager->first_message;
        while (current->next != NULL) {
            current = current->next;
        }
        current->next = message;
    }

    manager->messages_count++;
    manager->messages_size += message->size;
    message->next = NULL;

    return true;
}

bool delete_mail_message(struct mail_manager * manager, int index) {
    if(!manager) {
        return false;
    }
    mail_message * current = manager->first_message;
    while(current != NULL) {
        if(current->index == index && !current->deleted) {
            current->deleted = true;
            return true;
        }
        current = current->next;
    }
    return false;
}

void reset_deleted_mail_messages(struct mail_manager * manager) {
    if(!manager) {
        return;
    }
    mail_message * current = manager->first_message;
    while(current != NULL) {
        if(current->deleted) {
            current->deleted = false;
        }
        current = current->next;
    }
}

void cleanup_deleted_messages(struct mail_manager * manager) {
    if (!manager) {
        return;
    }

    mail_message *current = manager->first_message;
    while (current != NULL) {
        if (current->deleted) {
            remove(current->path_identifier);
        }
        current = current->next;
    }

    current = manager->first_message;
    while (current != NULL) {
        mail_message *temp = current;
        current = current->next;
        free_mail_message(temp);
    }

    free(manager);
}


