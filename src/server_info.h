#ifndef SERVER_INFO_H
#define SERVER_INFO_H

#include <stddef.h>

typedef struct server_info {
    size_t hist_conections;
    size_t current_conections;
    size_t bytes_sent;
    size_t bytes_received;
    size_t record_concurrent_conections;
    size_t total_bytes_transfered;
} server_info;

void init_server_info(server_info * si);
void new_conection_update(server_info * si);
void close_conection_update(server_info * si);
void bytes_sent_update(server_info * si, size_t bytes);
void bytes_received(server_info * server_info, size_t bytes); 

#endif

