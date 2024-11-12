#ifndef UTILS_H
#define UTILS_H

#define ERROR_CODE -1

#define SOCKET_CREATION_ERROR_MSG "Error in creation of socket."
#define SOCKET_BINDING_ERROR_MSG "Error in binding of socket."
#define SOCKET_LISTENING_ERROR_MSG "Error in listening of socket."
#define SELECTOR_SETTING_PASSIVE_SOCKET_NIO_ERROR_MSG "Error setting socket NIO."
#define SELECTOR_INIT_ERROR_MSG "Error in selector initialization."
#define SELECTOR_REGISTER_ERROR_MSG "Error registering file descriptor to selector."
#define SELECTOR_SELECT_ERROR_MSG "Error in selector select."

void validate(int code, char * msg);

#endif

