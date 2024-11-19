#include "server_info.h"

static server_info si;

void init_server_info() {
    si.current_conections = 0;
    si.hist_conections = 0;
    si.record_concurrent_conections = 0;
    si.bytes_received = 0;
    si.bytes_sent = 0;
    si.total_bytes_transfered = 0;
}

void new_conection_update() {
    si.current_conections++;
    si.hist_conections++;

    if (si.current_conections > si.record_concurrent_conections) {
        si.record_concurrent_conections = si.current_conections;
    }
}

void close_conection_update() {
    si.current_conections--;
}

void bytes_sent_update(size_t bytes) {
    si.bytes_sent += bytes;

    si.total_bytes_transfered += bytes;
}

void bytes_received_update(size_t bytes) {
    si.bytes_received += bytes;
    si.total_bytes_transfered += bytes;
} 
