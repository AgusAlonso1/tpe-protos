#ifndef MANAGER_CLIENT_H
#define MANAGER_CLIENT_H

#include <stdint.h>
#include <stddef.h>

uint8_t get_command_id(char * command);
void print_data_info(uint8_t command_id, size_t data);

#endif