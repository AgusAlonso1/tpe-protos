#ifndef MANAGER_SERVER_H 
#define MANAGER_SERVER_H

#include "selector.h"

/** Manager Protocol
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

enum commands {
    HIST_CONECTIONS = 0x0,
    CURRENT_CONECTIONS = 0x1,
    BYTES_SEND = 0x2,
    BYTES_RECEIVED = 0x3,
    RECORD_CONCURRENT_CONECTIONS = 0x4,
    TOTAL_BYTES_TRANSFERED = 0x5
};

enum response_status {
    OK = 0x0,
    UNAUTHORIZED = 0x1,
    INVALID_COMMAND = 0x2,
    INVALID_VERSION = 0x3,
    BAD_REQUEST = 0x4,
};

void manager_passive_accept(struct selector_key * sk);

#endif 
