#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include "kermit.h"

message_t createMessage() {
    message_t message;

    message.marker = MARKER;
    message.data_size = 0;
    message.sequence = 0;
    message.type = 0;
    for(int i = 0; i < MAX_DATA; i++) {
        message.data[i] = 0;
    }
    message.parity = 0;

    return message;
}

void setHeader(message_t* response, int type) {
    response->type = type;
}

void errorHeader(message_t* response, int error) {
    response->type = ERRO_T; // ERROR
    response->data_size = 1;
    response->data[0] = error;
}

void verticalParity(message_t* message) {
    int size = message->data_size;
    unsigned char parity;

    parity = message->data_size ^ message->sequence ^ message->type;
    for(int i = 0; i < size; i++) {
        parity ^= message->data[i];
    }
    
    message->parity = parity;
}

int checkParity(message_t* message) {
    int size = message->data_size;
    unsigned char parity;

    parity = message->data_size ^ message->sequence ^ message->type;
    for(int i = 0; i < size; i++) {
        parity ^= message->data[i];
    }
    
    if(message->parity == parity)
        return 1;
    
    return 0;
}

// Interprocess Communication
void createFile() {
    FILE *file_pointer;
    file_pointer = fopen("/home/picolo/Documents/ufpr/redes1/temporaryIPC.txt", "w");
    fputc('y', file_pointer);
    fclose(file_pointer);
}

void removeFile() {
    remove("/home/picolo/Documents/ufpr/redes1/temporaryIPC.txt");
}

int serverRead() {
    FILE *file_pointer;
    file_pointer = fopen("/home/picolo/Documents/ufpr/redes1/temporaryIPC.txt", "r");

    char permission = fgetc(file_pointer);
    fclose(file_pointer);

    if(permission == 'y') {
        return 1;    
    }

    return 0;
}

void changePermission(char new_p) {
    FILE *file_pointer;
    file_pointer = fopen("/home/picolo/Documents/ufpr/redes1/temporaryIPC.txt", "r+");
    fputc(new_p, file_pointer);
    fclose(file_pointer);
}