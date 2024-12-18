#ifndef SERVER_INFO_H
#define SERVER_INFO_H

#include <stddef.h>

#define MAX_CONECTIONS 500

typedef struct server_info {
    size_t hist_conections;
    size_t current_conections;
    size_t bytes_sent;
    size_t bytes_received;
    size_t record_concurrent_conections;
    size_t total_bytes_transfered;
    size_t max_conections;
} server_info;

void init_server_info();
void new_conection_update();
void close_conection_update();
void bytes_sent_update(size_t bytes);
void bytes_received_update(size_t bytes);
void max_conections_update(size_t max_conections);

size_t get_hist_conections();
size_t get_current_conections();
size_t get_record_concurrent_conections();
size_t get_bytes_sent();
size_t get_bytes_received();
size_t get_total_bytes_transfered();
size_t get_max_conections();

#endif

