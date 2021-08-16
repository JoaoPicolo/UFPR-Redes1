#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <math.h>
#include <errno.h>

#include "raw_socket.h"
#include "kermit.h"
#include "queue.h"

int executeCD(message_t* message, message_t* response) {
    char* path = (char*)message->data;

    if(!checkParity(message)) {
        setHeader(response, NACK_T);
        return 0;
    }

    if (chdir(path) != 0) {
        errorHeader(response, errno);
        verticalParity(response);
        return 0;
    }

    setHeader(response, ACK_T);
    return 1;
}

int executeLS(message_t* message, Queue_t* responses) {
    DIR *d;
    struct dirent *dir;
    char* path = (char*)message->data;

    message_t response = createMessage();
    response.type = LSC_T;

    if(!checkParity(message)) {
        setHeader(&response, NACK_T);
        enQueue(responses, response);
        return 0;
    }

    d = opendir(path);
    // Reads dir contents
    if (d) {
        int total = 0;
        while((dir = readdir(d)) != NULL) {
            total += strlen(dir->d_name) + 2;
        }
        closedir(d);

        d = opendir(path);
        char* ls_response = malloc(total * sizeof(char));
        int len = 0;
        int i = 0;
        while((dir = readdir(d)) != NULL) {
            len = strlen(dir->d_name);
            ls_response[i] = ' ';
            i++;
            ls_response[i] = dir->d_type + MARKER;
            i++;
            for(int j = 0; j < len; j++) {
                ls_response[i] = dir->d_name[j];
                i++;
            }
        }

        int size = strlen(ls_response);
        int resp_idx = 0;
        int sequence = 0;

        
        for(int i = 1; i < size; i++) { //  Eliminates space at beggining
            if(resp_idx == (MAX_DATA - 1)) {
                response.data[resp_idx] = '\0';
                response.data_size = resp_idx;
                verticalParity(&response);
                enQueue(responses, response);

                response = createMessage();
                response.type = LSC_T;
                sequence++;
                sequence = sequence % MAX_SEQ;
                response.sequence = sequence;

                resp_idx = 0;
            }

            response.data[resp_idx] = ls_response[i];
            resp_idx++;
        }

        // Captures last response
        response.data[resp_idx] = '\0';
        response.data_size = resp_idx;
        verticalParity(&response);
        enQueue(responses, response);

        // Creates end of transmission
        response = createMessage();
        response.type = END_T;
        sequence++;
        sequence = sequence % MAX_SEQ;
        response.sequence = sequence;
        verticalParity(&response);
        enQueue(responses, response);
        
        free(ls_response);
    }
    else {
        errorHeader(&response, errno);
        verticalParity(&response);
        enQueue(responses, response);
        return 0;
    }

    return 1;
}

FILE* openFile(message_t* message, Queue_t* responses) {
    message_t response = createMessage();

    if(!checkParity(message)) {
        setHeader(&response, NACK_T);
        enQueue(responses, response);
        return NULL;
    }

    FILE *file_pointer;
    char* path = (char*)message->data;
    file_pointer = fopen(path, "r");

    if(file_pointer == NULL) {
        errorHeader(&response, errno);
        verticalParity(&response);
        enQueue(responses, response);
        return NULL;
    }

    return file_pointer;
}

int getEndLine(FILE* file_p) {
    int count = 0;
    char c;
    for (c = getc(file_p); c != EOF; c = getc(file_p)) 
        if (c == '\n') count++; 

    fseek(file_p, 0, SEEK_SET);

    return count;
}

int enQueueLine(char* buffer, int sequence, Queue_t* responses, int type) {
    message_t response = createMessage();
    response.type = CA_T;
    response.sequence = sequence;
    int size = strlen(buffer) - 1;
    int resp_idx = 0;

    for(int i = 0; i < size; i++) {
        if(resp_idx == (MAX_DATA - 1)) {
            response.data[resp_idx] = '\0';
            response.data_size = resp_idx;
            verticalParity(&response);
            enQueue(responses, response);

            response = createMessage();
            response.type = CA_T;
            sequence++;
            sequence = sequence % MAX_SEQ;
            response.sequence = sequence;
            resp_idx = 0;
        }

        response.data[resp_idx] = buffer[i];
        resp_idx++;
    }

    // Captures last response
    response.data[resp_idx] = '\0';
    response.data_size = resp_idx;
    verticalParity(&response);
    enQueue(responses, response);

    // Creates end of transmission
    response = createMessage();
    response.type = type;
    sequence++;
    sequence = sequence % MAX_SEQ;
    response.sequence = sequence;
    verticalParity(&response);
    enQueue(responses, response);

    return sequence + 1;
}

void executeEdit(FILE* file_p, int line, char* file_name, char* new_line) {
    FILE* temp_p;
    
    strcat(new_line, "\n");
    strcat(new_line, "\0");

    temp_p = fopen("edit.temp", "w");
    int count = 0;
    char buffer[BUFFER_SIZE];

    while((fgets(buffer, BUFFER_SIZE, file_p)) != NULL) {
        count++;
        if (count == line)
            fputs(new_line, temp_p);
        else
            fputs(buffer, temp_p);
    }


    fclose(file_p);
    fclose(temp_p);
    remove(file_name);
    rename("edit.temp", file_name);
}

int main() {
    int socket_id = ConexaoRawSocket("lo");
    message_t message, response, last_message, last_response;
    Queue_t* responses;
    unsigned char command;
    createFile();

    last_response = createMessage();
    last_message = createMessage();

    while(1) {
        message = createMessage();
        
        if(serverRead()) {
            printf("Server reading\n");
            recvMessage(socket_id, &message, 0);
            command = message.type;
        }
        else {
            command = 16; // Impossible command
        }

        if(command == CD_T) {
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                response = createMessage();
                executeCD(&message, &response);
                last_message = message;
                last_response = response;
                sendResponse(socket_id, &response);
            }
            else {
                sendResponse(socket_id, &last_response);
            }
            // Receives ACK from client
            // and clear buffer for own message
            while(1) {
                if(serverRead()) {
                    for(int i = 0; i < 3; i++) {
                        recvMessage(socket_id, &message, 1);
                    }
                    if(message.type != ACK_T) {
                        sendResponse(socket_id, &last_response);
                    }
                    else {
                        last_message = message;
                        break;
                    }
                }
            }
        }
        else if(command == LS_T) {
            responses = createQueue();
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                executeLS(&message, responses);
                last_message = message;
                last_response = responses->start->message;
                sendResponse(socket_id, &last_response);

                if(last_response.type == NACK_T || last_response.type == ERRO_T) {
                    deQueue(responses);
                    // Clears own response from buffer
                    recvMessage(socket_id, &message, 1);
                }

                while(responses->size > 0) {
                    if(serverRead()) {
                        if(responses->size != 1) {
                            recvMessage(socket_id, &message, 1);
                            recvMessage(socket_id, &message, 0);
                        }
                        else {
                            for(int i = 0; i < 3; i++) {
                                recvMessage(socket_id, &message, 1);
                            }
                            if(message.sequence == last_message.sequence) {
                                sendResponse(socket_id, &last_response);   
                            }
                        }

                        last_message = message;
                        int message_seq = message.sequence % MAX_SEQ;
                        if(message_seq != 0) message_seq--;
                        else message_seq = MAX_SEQ - 1;

                        if((message.type == ACK_T) && (message_seq == (last_response.sequence  % MAX_SEQ))) {
                            deQueue(responses);
                            if(responses->size > 0) {
                                last_response = responses->start->message;
                                sendResponse(socket_id, &last_response);
                            }
                        }
                        else {
                            sendResponse(socket_id, &last_response);
                        }
                    }
                }
            }
            else {
                sendResponse(socket_id, &last_response);
            }

            free(responses);
        }
        else if(command == VER_T) {
            responses = createQueue();
            int sequence = 0;
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                FILE* file_p = openFile(&message, responses);
                last_message = message;
                
                if(file_p == NULL) { // NACK or ERROR
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);
                    deQueue(responses);
                    // Clears own response from buffer
                    recvMessage(socket_id, &message, 1);
                }
                else {
                    char buffer[BUFFER_SIZE];
                    int message_seq;
                    while(fgets(buffer, BUFFER_SIZE, file_p) != NULL) {
                        sequence = enQueueLine(buffer, sequence, responses, ENDL_T);

                        last_response = responses->start->message;
                        sendResponse(socket_id, &last_response);

                        while(responses->size > 0) {
                            if(serverRead()) {
                                if(responses->size != 1) {
                                    recvMessage(socket_id, &message, 1);
                                    recvMessage(socket_id, &message, 0);
                                }
                                else {
                                    for(int i = 0; i < 3; i++) {
                                        recvMessage(socket_id, &message, 1);
                                    }
                                    if(message.sequence == last_message.sequence) {
                                        sendResponse(socket_id, &last_response);   
                                    }
                                }

                                last_message = message;
                                message_seq = message.sequence % MAX_SEQ;
                                if(message_seq != 0) message_seq--;
                                else message_seq = MAX_SEQ - 1;

                                if((message.type == ACK_T) && (message_seq == (last_response.sequence  % MAX_SEQ))) {
                                    deQueue(responses);
                                    if(responses->size > 0) {
                                        last_response = responses->start->message;
                                        sendResponse(socket_id, &last_response);
                                    }
                                }
                                else {
                                    sendResponse(socket_id, &last_response);
                                }
                            }
                        }
                    }
                    response = createMessage();
                    sequence = sequence % MAX_SEQ;
                    response.sequence = sequence;
                    response.type = END_T;
                    verticalParity(&response);
                    last_response = response;
                    sendResponse(socket_id, &last_response);
                    // Clears own response from buffer
                    for(int i = 0; i < 3; i++) {
                        recvMessage(socket_id, &message, 1);
                    }

                    fclose(file_p);
                }
            }
            else {
                sendResponse(socket_id, &last_response);
            }
            free(responses);
        }
        else if(command == LINHA_T) {
            responses = createQueue();
            int sequence = 1;
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                FILE* file_p = openFile(&message, responses);
                last_message = message;
                
                if(file_p == NULL) { // NACK or ERROR
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);
                    deQueue(responses);
                    // Clears own response from buffer
                    recvMessage(socket_id, &message, 1);
                }
                else {
                    int end_line = getEndLine(file_p);
                    int line;
                    response = createMessage();
                    response.type = ACK_T;
                    enQueue(responses, response);
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);

                    while(responses->size > 0) {
                        if(serverRead()) {
                            if((responses->size != 1) || (last_response.type == ACK_T)) {
                                recvMessage(socket_id, &message, 1);
                                recvMessage(socket_id, &message, 0);
                            }
                            else {
                                for(int i = 0; i < 3; i++) {
                                    recvMessage(socket_id, &message, 1);
                                }
                                if(message.sequence == last_message.sequence) {
                                    sendResponse(socket_id, &last_response);   
                                }
                            }

                            last_message = message;
                            int message_seq = message.sequence % MAX_SEQ;
                            if(message_seq != 0) message_seq--;
                            else message_seq = MAX_SEQ - 1;

                            if((message.type == ACK_T) && (message_seq == (last_response.sequence  % MAX_SEQ))) {
                                deQueue(responses);
                                if(responses->size > 0) {
                                    last_response = responses->start->message;
                                    sendResponse(socket_id, &last_response);
                                }
                            }
                            else if(message.type == LIF_T) {
                                char buffer[BUFFER_SIZE];
                                int i = 0;
                                line = atoi((char*)message.data);

                                if((line > end_line) || (line < 1)) {
                                    response = createMessage();
                                    sequence++;
                                    sequence = sequence % MAX_SEQ;
                                    response.sequence = sequence;
                                    errorHeader(&response, 4);
                                    verticalParity(&response);
                                    sendResponse(socket_id, &response);
                                    recvMessage(socket_id, &message, 1);
                                    break;
                                }
                                else {
                                    while(i < line) {
                                        fgets(buffer, BUFFER_SIZE, file_p);
                                        i++;
                                    }
                                    deQueue(responses);
                                    sequence = enQueueLine(buffer, sequence, responses, END_T);
                                    last_response = responses->start->message;
                                    sendResponse(socket_id, &last_response);
                                }
                            }
                            else {
                                sendResponse(socket_id, &last_response);
                            }
                        }
                    }
                    fclose(file_p);
                }
            }
            else {
                sendResponse(socket_id, &last_response);
            }
            free(responses);
        }
        else if(command == LINHAS_T) {
            responses = createQueue();
            int lines[2];
            int line = 0;
            int sequence = 1;
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                FILE* file_p = openFile(&message, responses);
                last_message = message;
                
                if(file_p == NULL) { // NACK or ERROR
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);
                    deQueue(responses);
                    // Clears own response from buffer
                    recvMessage(socket_id, &message, 1);
                }
                else {
                    int end_line = getEndLine(file_p);
                    response = createMessage();
                    response.type = ACK_T;
                    enQueue(responses, response);
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);

                    while(responses->size > 0) {
                        if(serverRead()) {
                            if((responses->size != 1) || (last_response.type == ACK_T) || (last_response.type == ENDL_T)) {
                                recvMessage(socket_id, &message, 1);
                                recvMessage(socket_id, &message, 0);
                            }
                            else {
                                for(int i = 0; i < 3; i++) {
                                    recvMessage(socket_id, &message, 1);
                                }
                                if(message.sequence == last_message.sequence) {
                                    sendResponse(socket_id, &last_response);   
                                }
                            }

                            last_message = message;
                            int message_seq = message.sequence % MAX_SEQ;
                            if(message_seq != 0) message_seq--;
                            else message_seq = MAX_SEQ - 1;

                            if((message.type == ACK_T) && (message_seq == (last_response.sequence  % MAX_SEQ))) {
                                deQueue(responses);
                                if(responses->size > 0) {
                                    last_response = responses->start->message;
                                    sendResponse(socket_id, &last_response);
                                }
                                else if(last_response.type ==  ENDL_T) {
                                    line++;
                                    if(line > lines[1]) { // Last line
                                        response = createMessage();
                                        sequence = sequence % MAX_SEQ;
                                        response.sequence = sequence;
                                        response.type = END_T;
                                        verticalParity(&response);
                                        last_response = response;
                                        enQueue(responses, response);
                                        sendResponse(socket_id, &last_response);
                                    }
                                    else {
                                        char buffer[BUFFER_SIZE];
                                        fgets(buffer, BUFFER_SIZE, file_p);
                                        deQueue(responses);
                                        sequence = enQueueLine(buffer, sequence, responses, ENDL_T);
                                        last_response = responses->start->message;
                                        sendResponse(socket_id, &last_response);
                                    }
                                    
                                }
                            }
                            else if(message.type == LIF_T) {
                                char buffer[BUFFER_SIZE];
                                int i = 0;
                                char* input = (char*)message.data;
                                char* token = strtok(input, " ");
                                while( token != NULL ) {
                                    lines[i] = atoi(token);
                                    i++;
                                    token = strtok(NULL, " ");
                                }

                                if((lines[0] > end_line) || (lines[0] < 1) || (lines[1] > end_line) || (lines[1] < 1)) {
                                    response = createMessage();
                                    sequence++;
                                    sequence = sequence % MAX_SEQ;
                                    response.sequence = sequence;
                                    errorHeader(&response, 4);
                                    verticalParity(&response);
                                    sendResponse(socket_id, &response);
                                    recvMessage(socket_id, &message, 1);
                                    break;
                                }
                                else {
                                    line = lines[0];
                                    i = 0;
                                    while(i < line) {
                                        fgets(buffer, BUFFER_SIZE, file_p);
                                        i++;
                                    }
                                    deQueue(responses);
                                    sequence = enQueueLine(buffer, sequence, responses, ENDL_T);
                                    last_response = responses->start->message;
                                    sendResponse(socket_id, &last_response);
                                }
                            }
                            else {
                                sendResponse(socket_id, &last_response);
                            }
                        }
                    }
                    fclose(file_p);
                }
            }
            else {
                sendResponse(socket_id, &last_response);
            }
            free(responses);
        }
        else if(command == EDIT_T) {
            responses = createQueue();
            if((last_message.parity != message.parity) || last_response.type == NACK_T) {
                FILE* file_p = openFile(&message, responses);
                last_message = message;
                
                if(file_p == NULL) { // NACK or ERROR
                    last_response = responses->start->message;
                    sendResponse(socket_id, &last_response);
                    deQueue(responses);
                    // Clears own response from buffer
                    recvMessage(socket_id, &message, 1);
                }
                else {
                    int sequence = 0;
                    int line;
                    char buffer[BUFFER_SIZE];
                    memset(buffer, 0, BUFFER_SIZE);

                    int end_line = getEndLine(file_p);
                    char* file_name = malloc(message.data_size * sizeof(char));
                    strcpy(file_name, (char*)message.data);

                    response = createMessage();
                    response.type = ACK_T;
                    sendResponse(socket_id, &response);

                    while(message.type != END_T) {
                        if(serverRead()) {
                            recvMessage(socket_id, &message, 1);
                            recvMessage(socket_id, &message, 0);
                            last_message = message;
                            if(!checkParity(&message)) {
                                response = createMessage();
                                sequence++;
                                sequence = sequence % MAX_SEQ;
                                response.sequence = sequence;
                                setHeader(&response, NACK_T);
                                sendResponse(socket_id, &response);
                                continue;
                            }
                            if(message.type == LIF_T) {
                                line = atoi((char*)message.data);

                                if((line > end_line) || (line < 1)) {
                                    response = createMessage();
                                    sequence++;
                                    sequence = sequence % MAX_SEQ;
                                    response.sequence = sequence;
                                    errorHeader(&response, 4);
                                    verticalParity(&response);
                                    sendResponse(socket_id, &response);
                                    recvMessage(socket_id, &message, 1);
                                    break;
                                }
                                else {
                                    response = createMessage();
                                    sequence++;
                                    sequence = sequence % MAX_SEQ;
                                    response.sequence = sequence;
                                    response.type = ACK_T;
                                    sendResponse(socket_id, &response);
                                }
                            }
                            else if(message.type == CA_T) {
                                char* data = (char*)message.data;
                                strcat(buffer, data);
                                
                                response = createMessage();
                                sequence++;
                                sequence = sequence % MAX_SEQ;
                                response.sequence = sequence;
                                response.type = ACK_T;
                                sendResponse(socket_id, &response);
                            }


                        }
                    }
                    if(message.type == END_T) {
                        executeEdit(file_p, line, file_name, buffer);
                        response = createMessage();
                        sequence++;
                        sequence = sequence % MAX_SEQ;
                        response.sequence = sequence;
                        response.type = ACK_T;
                        sendResponse(socket_id, &response);
                        recvMessage(socket_id, &message, 1);
                    }

                    free(file_name);
                }
            }
            else {
                sendResponse(socket_id, &last_response);
            }
            free(responses);
        }
        else if(command == EXIT_T) {
            break;
        }
        else {
            printf("Server waiting to read\n");   
        }

        sleep(2);
    }

    removeFile();
    close(socket_id);
    return 0;
}