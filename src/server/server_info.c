#include <server_info.h>

static server_info si;

void init_server_info() {
    si.current_conections = 0;
    si.hist_conections = 0;
    si.record_concurrent_conections = 0;
    si.bytes_received = 0;
    si.bytes_sent = 0;
    si.total_bytes_transfered = 0;
    si.max_conections = MAX_CONECTIONS;
}

void max_conections_update(size_t max_conections) {
    si.max_conections = max_conections;
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

size_t get_hist_conections() {
    return si.hist_conections;
}

size_t get_current_conections() {
    return si.current_conections;
}

size_t get_record_concurrent_conections() {
    return si.record_concurrent_conections;
}

size_t get_bytes_sent() {
    return si.bytes_sent;
}

size_t get_bytes_received() {
    return si.bytes_received;    
}

size_t get_total_bytes_transfered() {
    return si.total_bytes_transfered;
}

size_t get_max_conections() {
    return si.max_conections;
}
