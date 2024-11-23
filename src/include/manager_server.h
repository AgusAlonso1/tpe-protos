#ifndef MANAGER_SERVER_H 
#define MANAGER_SERVER_H

#define VERSION 0x0
#define REQUEST_RESPONSE_LEN 10
#define TOKEN_LEN 8

#define VERSION_OFFSET 0

#define AUTH_TOKEN_OFFSET 1
#define COMMAND_OFFSET 9

#define STATUS_OFFSET 1
#define DATA_OFFSET 2

#define MANAGER_PORT 8086


#include "selector.h"

/** Manager Protocol - Server
 *  
 *  Request:
 * 
 *  +---------+----------------------------+
 *  | VERSION |    AUTHTOKEN     | COMMAND |
 *  +---------+----------------------------+
 *  |    1    |        8         |    1    |
 *  +---------+----------------------------+
 *
 *  VERSION: La version del protocolo
 *  AUTHTOKEN: Token de autorizacion
 *  COMMAND: El numero de metrica que se desea solicitar
 *  
 *  Response:
 *   
 *  +---------+--------+------+
 *  | VERSION | STATUS | DATA |
 *  +---------+--------+------+
 *  |    1    |   1    |  8   |
 *  +---------+--------+------+
 *
 *  VERSION: La version del protocolo
 *  STATUS: Codigo de respuesta del protocolo, 0x0 OK o 0x1 ERROR
 *  DATA: En caso de una respuesta OK, el valor numerico de la metrica solicitado
 *
 */

typedef enum command_id {
    HIST_CONECTIONS = 0x0,
    CURRENT_CONECTIONS = 0x1,
    BYTES_SEND = 0x2,
    BYTES_RECEIVED = 0x3,
    RECORD_CONCURRENT_CONECTIONS = 0x4,
    TOTAL_BYTES_TRANSFERED = 0x5
} command_id;

typedef enum response_status {
    OK = 0x0,
    UNAUTHORIZED = 0x1,
    INVALID_COMMAND = 0x2,
    INVALID_VERSION = 0x3,
    BAD_REQUEST = 0x4,
} response_status;

void manager_handle_connection(struct selector_key * sk);

#endif 
