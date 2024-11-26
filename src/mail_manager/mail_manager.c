#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include "../include/mail_manager.h"
#include <sys/wait.h>
#include <errno.h>
#include <sys/stat.h>
#include <dirent.h>


#define BUFFER_SIZE 4096
#define ARRAY_CAP 10

#define ERROR (-1)
#define SUCCESS 0

static FILE *open_and_process_message_file(char *filepath, char *transformation);

void free_mail_manager(struct mail_manager *manager) {
    if(!manager) {
        return;
    }

    if(manager->mail_drop) {
        free(manager->mail_drop);
    }

    if(manager->messages_array) {
        for(size_t i = 0; i < manager->messages_count; i++) {
            mail_message * message = &manager->messages_array[i];
            if(message) {
                free(message->path_identifier);
                free(message);
            }
        }
        free(manager->messages_array);
    }
}

struct mail_manager * create_mail_manager(char * mail_drop, char * username) {
    struct mail_manager *manager = malloc(sizeof(struct mail_manager));
    if(!manager) {
        return NULL;
    }

    /** Inicializamos los atributos del manager **/
    manager->messages_array = NULL;
    manager->messages_count = 0;
    manager->messages_capacity = 0;
    manager->messages_size = 0;

    /** Construir ruta del mail_drop **/
    char *mail_drop_path;
    if(asprintf(&mail_drop_path, "%s/%s/", mail_drop, username) == -1) {
        free(manager);
        return NULL;
    }

    /** Verificar que mail_drop_path es un directorio **/
    struct stat mail_drop_stat;
    if(stat(mail_drop_path, &mail_drop_stat) == -1 || !S_ISDIR(mail_drop_stat.st_mode)) {
        free(mail_drop_path);
        free(manager);
        errno = ENOTDIR;
        return NULL;
    }

    manager->mail_drop = strdup(mail_drop_path);

    /** Abrir el directorio principal **/
    DIR *mail_drop_dir = opendir(mail_drop_path);
    if(!mail_drop_dir) {
        free(manager->mail_drop);
        free(manager);
        return NULL;
    }

    struct dirent *mail_drop_entry;
    while((mail_drop_entry = readdir(mail_drop_dir)) != NULL) {
        /** Solo procesamos el directorio new **/
        if(mail_drop_entry->d_type == DT_DIR && (strcmp(mail_drop_entry->d_name, NEW) == 0)) {

            char *subdir_path;
            if(asprintf(&subdir_path, "%s%s", mail_drop_path, mail_drop_entry->d_name) == -1) {
                free_mail_manager(manager);
                closedir(mail_drop_dir);
                return NULL;
            }

            DIR *subdir = opendir(subdir_path);
            if(!subdir) {
                free(subdir_path);
                continue;
            }

            struct dirent *subdir_entry;
            while((subdir_entry = readdir(subdir)) != NULL) {
                if (subdir_entry->d_type == DT_REG) {
                    /** Conseguimos el path del archivo **/
                    char *message_path;
                    if(asprintf(&message_path, "%s/%s", subdir_path, subdir_entry->d_name) == -1) {
                        free(subdir_path);
                        closedir(subdir);
                        free_mail_manager(manager);
                        closedir(mail_drop_dir);
                        return NULL;
                    }

                    /** Conseguimos el size del archivo **/
                    struct stat message_stat;
                    if(stat(message_path, &message_stat) == -1) {
                        free(message_path);
                        free(subdir_path);
                        closedir(subdir);
                        free_mail_manager(manager);
                        closedir(mail_drop_dir);
                        return NULL;
                    }

                    /** Añadimos el message al array **/
                    if(!add_mail_message(manager, message_path, message_stat.st_size)) {
                        free(message_path);
                        free(subdir_path);
                        closedir(subdir);
                        free_mail_manager(manager);
                        closedir(mail_drop_dir);
                        return NULL;
                    }

                    free(message_path);
                }
            }

            free(subdir_path);
            closedir(subdir);
        }
    }

    closedir(mail_drop_dir);
    return manager;
}


bool add_mail_message(struct mail_manager * manager, const char * path, size_t size) {
    if(!manager || !path) {
        return false;
    }

    if(manager->messages_count == manager->messages_capacity) {
        size_t new_capacity = manager->messages_capacity + ARRAY_CAP;
        mail_message * new_array = realloc(manager->messages_array, new_capacity * sizeof(mail_message));
        if(!new_array) {
            return false;
        }

        manager->messages_array = new_array;
        manager->messages_capacity = new_capacity;
    }

    mail_message * new_message = &manager->messages_array[manager->messages_count];
    new_message->path_identifier = strdup(path);
    if(!new_message->path_identifier) {
        return false;
    }

    new_message->size = size;
    new_message->deleted = false;
    new_message->number = manager->messages_count + 1;

    manager->messages_count++;
    manager->messages_size += size;

    return true;
}

bool delete_mail_message(struct mail_manager * manager, int index) {
    if(!manager || index < 0 || index >= manager->messages_count) {
        return false;
    }

    if(manager->messages_array[index].deleted == false) {
        manager->messages_array[index].deleted = true;
        return true;
    }

    return false;
}

void reset_deleted_mail_messages(struct mail_manager * manager, size_t * size, int * messages_count) {
    if(!manager) {
        return;
    }

    for(size_t i = 0; i < manager->messages_count; i++) {
        if(manager->messages_array[i].deleted) {
            * size += manager->messages_array[i].size;
            * messages_count += 1;
            manager->messages_array[i].deleted = false;
        }
    }
}

void cleanup_deleted_messages(struct mail_manager * manager) {
    if(!manager) {
        return;
    }

    char *cur_dir = NULL;
    if (asprintf(&cur_dir, "%s/cur", manager->mail_drop) == ERROR) {
        return; 
    }
    struct stat st = {0};
    if (stat(cur_dir, &st) == -1) {
        if (mkdir(cur_dir, 0755) == -1) {
            perror("mkdir");
            free(cur_dir);
            return; 
        }
    }
    free(cur_dir);


    for(size_t i = 0; i < manager->messages_count; i++) {
        if(manager->messages_array[i].deleted) {
            char * old_path = strdup(manager->messages_array[i].path_identifier);
            char * new_path = NULL;

            const char * file_name = strrchr(old_path, BARRA);
            file_name++;

            if(asprintf(&new_path, "%s/cur/%s", manager->mail_drop, file_name) == ERROR) {
                free(old_path);
            } else {

                if(rename(old_path, new_path) == 0) {
                    free(manager->messages_array[i].path_identifier);
                    manager->messages_array[i].path_identifier = new_path;
                } else {
                    free(new_path);
                }
                free(old_path);
            }
        }
    }
}

FILE * retrieve_message(struct mail_manager * manager, int message_number, size_t * octets, char * transformation) {
    if(!manager || message_number < 0 || message_number > manager->messages_count) {
        return false;
    }

    *octets = manager->messages_array[message_number - 1].size;
    return get_message_content(manager, message_number, transformation);
}

FILE * get_message_content(struct mail_manager * manager, int message_number, char * transformation) {

    if (manager == NULL || message_number < 1 || message_number > manager->messages_size) {
        return NULL;
    }

    if (manager->messages_array[message_number - 1].deleted) {
        return NULL;
    }

    int message_path_length = strlen(manager->messages_array[message_number - 1].path_identifier) + 1;
    char path[message_path_length];

    strcpy(path, manager->messages_array[message_number - 1].path_identifier);

    FILE * message_file = open_and_process_message_file(path, transformation);

    // if (message_file != NULL) {
    //     * estimated_message_size = manager->messages_array[message_number - 1].size;
    // }

    return message_file;
}

FILE * open_and_process_message_file(char * filepath, char * transformation) {
    const char * command = strdup(transformation);

    int command_length = strlen(command) + 1 + strlen(filepath) + 1; 
    char execute_command[command_length];

    snprintf(execute_command, sizeof(execute_command), "%s %s", command, filepath);

    int pipefd[2];
    if (pipe(pipefd) == ERROR) {
        return NULL;
    }

    pid_t pid = fork();
    if (pid == ERROR) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);

        execlp("sh", "sh", "-c", execute_command, (char *) NULL);

        exit(EXIT_FAILURE);
    } else {
        close(pipefd[1]);

        FILE * file = fdopen(pipefd[0], "r");

        if (file != NULL) {
            return file;
        } else {
            close(pipefd[0]);
            return NULL;
        }
    }
}

//Erores que encontre:
//2. Si tiro dos user seguidos y dsp pongo pass y dsp list se buggea ✅
//3. Comandos con todos los atributos pegados se buggea ✅ 
//4. agragar /n despues de leer el mail (retreive) ✅ 
//5. Checkear que queden todos los mensajes igual a pop3 (tomi)
//6. El mensaje de error se guarda y se imprime mas de una vez ✅
//7. El argumento -d para el maildir no funciona ✅ 
//8. checkear en el rfc como se maneja cuando pones el pass mal.  ✅ El rfc no lo especifica (por mi lo dejamos como esta. Total seria decision del que lo implementa) ✅
//9. mandar los mensajes deleted al directorio cur ✅
//10. ver si el segundo argumento de pass acepta espacios (lo dice el rfc)(PD: no se como se haria para pasar un " " como argumento) (vale)
//11. Cuando borro un mensaje, el list lo sigue incluyendo (vale)
//12. Si el list mas de un argumento y el primer argumento es valido, retorna OK (Creo q deberiamos sacar poder recibir mar de un arg) (vale)
//13. Hacer logs (tomi)