#ifndef __KERMIT__
#define __KERMIT__

#define MAX_DATA 15
#define MAX_SEQ 256
#define BUFFER_SIZE 1000

// Marker
#define MARKER 126 // 126 == 0X7E == 01111110

// Types
#define CD_T 0
#define LS_T 1
#define VER_T 2
#define LINHA_T 3
#define LINHAS_T 4
#define EDIT_T 5
#define EXIT_T 6
#define ACK_T 8
#define NACK_T 9
#define LIF_T 10
#define LSC_T 11
#define CA_T 12
#define END_T 13
#define ENDL_T 14
#define ERRO_T 15

// Errors
#define PERMISSION_E 1
#define DIR_E 2
#define ARQ_E 3
#define LINHA_E 4

typedef struct {
    unsigned char marker;
    unsigned char data_size:4;
    unsigned char sequence;
    unsigned char type:4;
    unsigned char data[MAX_DATA];
    unsigned char parity;
} message_t;

message_t createMessage();

void changeReader();

void setHeader(message_t* response, int type);

void errorHeader(message_t* response, int error);

void verticalParity(message_t* message);

int checkParity(message_t* message);

// IPC definitions
void createFile();

void removeFile();

int serverRead();

void changePermission(char new_p);

#endif