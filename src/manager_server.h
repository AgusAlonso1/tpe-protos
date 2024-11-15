#ifndef MANAGER_SERVER_H 
#define MANAGER_SERVER_H

#include "selector.h"

/** Manager Protocol
 *  
 *  Request:
 * 
 *  +---------+--------+--------+
 *  | VERSION |        | METRIC |
 *  +---------+--------+--------+
 *  |    1    |        |   1    |
 *  +---------+--------+--------+
 *
 *  VERSION: La version del protocolo
 *  METRIC: El numero de metrica que se desea solicitar
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

#define MAX_REQUEST_LEN 100

void manager_passive_accept(struct selector_key sk);

#endif 
