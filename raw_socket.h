#ifndef __RAWSOCKET__
#define __RAWSOCKET__

#include "kermit.h"

int ConexaoRawSocket(char *device);

void sendResponse(int socket_id, message_t* response) ;

int recvResponse(int socket_id, message_t* response);

void sendMessage(int socket_id, message_t* message, message_t* response);

void recvMessage(int socket_id, message_t* message, int control);

#endif