#ifndef UTILS_H
#define UTILS_H

#define ERROR_CODE -1

#define SOCKET_CREATION_ERROR_MSG "Error in creation of socket."
#define SOCKET_BINDING_ERROR_MSG "Error in binding of socket."
#define SOCKET_LISTENING_ERROR_MSG "Error in listening of socket."

void validate(int code, char * msg);

#endif

