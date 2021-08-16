#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <arpa/inet.h>

#include "raw_socket.h"
#include "kermit.h"
#include "queue.h"

// Global const
#define MAX_ARGS 4
#define LINE_LENGTH 5
// Colors
#define RESET "\033[0m"
#define KBLU "\033[1;34m"

void printCurrentDir() {
    char client_dir[128];
    getcwd(client_dir, 128);

    char* token = strrchr(client_dir, '/');
    int length = strlen(token);
    char* directory = malloc(length);
    memcpy(directory, token+1, length);

    printf(KBLU);
    printf("%s", directory);
    printf(RESET);
    printf("$ ");

    free(directory);
}

void initializeArgs(char* input, char* argv[]) {
    int index = 0;
    int size = strlen(input) - 2;
    char* aux = malloc(strlen(input) * sizeof(char));
    strcpy(aux, input);

    char* token = strtok(input, " ");
    while( token != NULL ) {
        if(index == 3) {
            if(strcmp(argv[0], "edit") == 0) {
                int c = 0;
                int i = 0;
                while(c < 3) {
                    if(aux[i] == 32) c++;
                    i++;
                }
                i++;
                c = 0;
                argv[index] = malloc((size - i) * sizeof(char));
                while(i < size) {
                    argv[index][c] = aux[i];
                    c++;
                    i++;
                }
                index++;
                break;
            }
        }
        argv[index] = malloc(strlen(token) + 1);
        token[strcspn(token, "\n")] = 0;
        strcpy(argv[index], token);
        index++;
    
      token = strtok(NULL, " ");
    }

    // Sets remaining space to null
    for(int i = index; i < MAX_ARGS; i++) {
        argv[i] = malloc(sizeof(NULL));
        argv[i] = NULL;
    }
}

void freeArgs(char* argv[]) {
    for(int index = 0; index < MAX_ARGS; index++) {
        free(argv[index]);
    }
}

void parseArgs(char* input, char* argv[]) {
    int index = 0;
    int size = strlen(input) - 2;
    char* aux = malloc(strlen(input) * sizeof(char));
    strcpy(aux, input);

    char* token = strtok(input, " ");
    while( token != NULL ) {
        if(index == 3) {
            if(strcmp(argv[0], "edit") == 0) {
                int c = 0;
                int i = 0;
                while(c < 3) {
                    if(aux[i] == 32) c++;
                    i++;
                }
                i++;
                c = 0;
                free(argv[index]);
                argv[index] = malloc((size - i) * sizeof(char));
                while(i < size) {
                    argv[index][c] = aux[i];
                    c++;
                    i++;
                }
                index++;
                break;
            }
        }
        argv[index] = realloc(argv[index], strlen(token) + 1);
        token[strcspn(token, "\n")] = 0;
        strcpy(argv[index], token);
        index++;
    
        token = strtok(NULL, " ");
    }

    // Sets remaining space to null
    for(int i = index; i < MAX_ARGS; i++) {
        argv[i] = realloc(argv[i], sizeof(NULL));
        argv[i] = NULL;
    }
}

void executeLCD(char* path) {
    if (chdir(path) != 0) {
        printf("%s: no such directory\n", path);
    }
}

void executeLLS(char* path) {
    DIR *d;
    struct dirent *dir;
    int i = 1;

    // Opens dir
    if(path == NULL)
        d = opendir(".");
    else
        d = opendir(path);
    
    // Reads dir contents
    if (d) {
        while((dir = readdir(d)) != NULL) {
            if(i % LINE_LENGTH != 0) {
                if(dir->d_type == 4) { // if is a dir
                    printf(KBLU);
                    printf("%-25s", dir->d_name);
                    printf(RESET);
                }
                else { // if is a file
                    printf("%-25s", dir->d_name);
                }
            }
            else {
                if(dir->d_type == 4) { // if is a dir
                    printf(KBLU);
                    printf("%-25s\n", dir->d_name);
                    printf(RESET);
                }
                else { // if is a file
                    printf("%-25s\n", dir->d_name);
                }
            }
            i++;
        }
        closedir(d);
    }
    else {
        printf("%s: no such directory\n", path);
    }
    printf("\n");
}

void printError(message_t* response) {
    int error_code = response->data[0];

    if(error_code == PERMISSION_E) {
        printf("Permission denied\n");
    }
    else if(error_code == DIR_E) {
        printf("No such directory on server-side\n");
    }
    else if(error_code == ARQ_E) {
        printf("No such file on server-side\n");
    }
    else if(error_code == LINHA_E){
        printf("No such line on the provided file\n");
    }
}

void mountCD(char* path, message_t* message) {
    int path_size = strlen(path);

    if(path_size > MAX_DATA) {
        printf("Invalid path: maximum allowed path has length %d\n", MAX_DATA);
    }
    else {
        message->data_size = path_size;
        message->sequence = 0;
        message->type = CD_T;
        
        for(int i = 0; i < path_size; i++) {
            message->data[i] = path[i];
        }

        verticalParity(message);
    }
}

void mountLS(char* path, message_t* message) {
    if(path == NULL) {
        path = malloc(sizeof(char));
        path[0] = '.';
    }

    int path_size = strlen(path);

    if(path_size > MAX_DATA) {
        printf("Invalid path: maximum allowed path has length %d\n", MAX_DATA);
    }

    message->data_size = path_size;
    message->sequence = 0;
    message->type = LS_T;

    for(int i = 0; i < path_size; i++) {
        message->data[i] = path[i];
    }

    verticalParity(message);
}

void printLS(char* ls_response) {
    char* token = strtok(ls_response, " ");
    char type;
    int i = 1;

    while( token != NULL ) {
        type = token[0] - MARKER;
        token = token + 1;
        if(i % LINE_LENGTH != 0) {
            if(type == 4) { // if is a dir
                printf(KBLU);
                printf("%-25s", token);
                printf(RESET);
            }
            else { // if is a file
                printf("%-25s", token);
            }
        }
        else {
            if(type == 4) { // if is a dir
                printf(KBLU);
                printf("%-25s\n", token);
                printf(RESET);
            }
            else { // if is a file
                printf("%-25s\n", token);
            }
        }

        i++;
        token = strtok(NULL, " ");
    }
    printf("\n");
}

void mountVer(char* file_name, message_t* message) {
    if(file_name == NULL) {
        printf("Please enter a valid file name.");
    }

    int name_size = strlen(file_name);

    if(name_size > MAX_DATA) {
        printf("Invalid file: maximum allowed file name has length %d\n", MAX_DATA);
    }

    message->data_size = name_size;
    message->sequence = 0;
    message->type = VER_T;

    for(int i = 0; i < name_size; i++) {
        message->data[i] = file_name[i];
    }

    verticalParity(message);
}

void mountLinha(char* line, char* file_name, Queue_t* messages) {
    if(file_name == NULL) {
        printf("Please enter a valid file name.");
    }
    if(line == NULL) {
        printf("Please enter a valid line.");
    }

    int name_size = strlen(file_name);
    int numb_size = strlen(line);

    if(name_size > MAX_DATA) {
        printf("Invalid file: maximum allowed file name has length %d\n", MAX_DATA);
    }

    message_t message = createMessage();
    message.data_size = name_size;
    message.sequence = 0;
    message.type = LINHA_T;
    for(int i = 0; i < name_size; i++) {
        message.data[i] = file_name[i];
    }
    verticalParity(&message);
    enQueue(messages, message);

    message = createMessage();
    message.data_size = numb_size;
    message.sequence = 1;
    message.type = LIF_T;
    for(int i = 0; i < numb_size; i++) {
        message.data[i] = line[i];
    }

    verticalParity(&message);
    enQueue(messages, message);
}

int mountLinhas(char* start_line, char* end_line, char* file_name, Queue_t* messages) {
    if(file_name == NULL) {
        printf("Please enter a valid file name.");
    }
    if(start_line == NULL) {
        printf("Please enter a valid start line.");
    }
    if(end_line == NULL) {
        printf("Please enter a valid end line.");
    }

    int name_size = strlen(file_name);
    int start_size = strlen(start_line);
    int end_size = strlen(end_line);

    if(name_size > MAX_DATA) {
        printf("Invalid file: maximum allowed file name has length %d\n", MAX_DATA);
    }

    message_t message = createMessage();
    message.data_size = name_size;
    message.sequence = 0;
    message.type = LINHAS_T;
    for(int i = 0; i < name_size; i++) {
        message.data[i] = file_name[i];
    }
    verticalParity(&message);
    enQueue(messages, message);

    message = createMessage();
    message.data_size = start_size + end_size + 1;
    message.sequence = 1;
    message.type = LIF_T;
    int i;
    for(i = 0; i < start_size; i++) {
        message.data[i] = start_line[i];
    }

    int line = atoi((char*)message.data);
    message.data[i] = ' ';
    i++;
    for(int j = 0; j < end_size; j++) {
        message.data[i] = end_line[j];
        i++;
    }

    verticalParity(&message);
    enQueue(messages, message);

    return line;
}

void mountEdit(char* line, char* file_name, char* new_text, Queue_t* messages) {
    if(new_text == NULL) {
        printf("Please enter a valid text.");
    }
    if(line == NULL) {
        printf("Please enter a valid line.");
    }
    if(file_name == NULL) {
        printf("Please enter a valid file path.");
    }

    int name_size = strlen(file_name);

    if(name_size > MAX_DATA) {
        printf("Invalid file: maximum allowed file name has length %d\n", MAX_DATA);
    }

    message_t message = createMessage();
    message.data_size = name_size;
    message.sequence = 0;
    message.type = EDIT_T;
    for(int i = 0; i < name_size; i++) {
        message.data[i] = file_name[i];
    }
    verticalParity(&message);
    enQueue(messages, message);

    message = createMessage();
    message.data_size = name_size;
    message.sequence = 1;
    message.type = LIF_T;
    for(int i = 0; i < name_size; i++) {
        message.data[i] = line[i];
    }
    verticalParity(&message);
    enQueue(messages, message);

    int sequence = 2;
    message = createMessage();
    message.sequence = sequence;
    message.type = CA_T;
    int size = strlen(new_text);
    int resp_idx = 0;
    for(int i = 0; i < size; i++) {
        if(resp_idx == (MAX_DATA - 1)) {
            message.data[resp_idx] = '\0';
            message.data_size = resp_idx;
            verticalParity(&message);
            enQueue(messages, message);

            message = createMessage();
            message.type = CA_T;
            sequence++;
            sequence = sequence % MAX_SEQ;
            message.sequence = sequence;
            resp_idx = 0;
        }

        message.data[resp_idx] = new_text[i];
        resp_idx++;
    }

    // Captures last response
    message.data[resp_idx] = '\0';
    message.data_size = resp_idx;
    verticalParity(&message);
    enQueue(messages, message);

    // Creates end of transmission
    message = createMessage();
    message.type = END_T;
    sequence++;
    sequence = sequence % MAX_SEQ;
    message.sequence = sequence;
    verticalParity(&message);
    enQueue(messages, message);
}

void mountExit(message_t* message) {
    message->type = EXIT_T;
}

int main() {
    char input[1024];
    char* argv[MAX_ARGS];
    int socket_id = ConexaoRawSocket("lo");
    message_t message, response;
    Queue_t* messages;

    printCurrentDir();
    fgets(input, sizeof(input), stdin);
    initializeArgs(input, argv);

    while(1) { 
        message = createMessage();
        response = createMessage();

        char* command = argv[0];
        if(strcmp(command, "cd") == 0) {
            mountCD(argv[1], &message);
            sendMessage(socket_id, &message, &response);
            if(response.type == ERRO_T) {
                printError(&response);
            }
            else {
                printf("Directory change to '%s' on server-side\n", argv[1]);
            }

            message = createMessage();
            message.type = ACK_T;

            sendResponse(socket_id, &message);
        }
        else if(strcmp(command, "lcd") == 0) {
            executeLCD(argv[1]);
        }
        else if(strcmp(command, "ls") == 0) {
            int or_size = 1;
            int sequence = 0;
            char* ls_response = malloc(or_size);
            char* data;
            mountLS(argv[1], &message);
            while(response.type != END_T) {
                sendMessage(socket_id, &message, &response);
                if(response.type == ERRO_T) {
                    printError(&response);
                    break;
                }
                else if(response.type == LSC_T) {
                    data = (char*)response.data;
                    or_size += response.data_size;
                    ls_response = realloc(ls_response, or_size);
                    strcat(ls_response, data);
                }
                message = createMessage();
                message.type = ACK_T;
                sequence++;
                sequence = sequence % MAX_SEQ;
                message.sequence = sequence;
            }
            if(response.type == END_T) {
                sendResponse(socket_id, &message);
                printLS(ls_response);
            }
            free(ls_response);
        }
        else if(strcmp(command, "lls") == 0) {
            executeLLS(argv[1]);
        }
        else if(strcmp(command, "ver") == 0) {
            mountVer(argv[1], &message);
            int sequence = 0;
            char* data;
            int line = 1;
            int print = 1;
            while(response.type != END_T) {
                response.type = ACK_T;
                while((response.type != ENDL_T) && (response.type != END_T)) {
                    sendMessage(socket_id, &message, &response);

                    if(response.type == ERRO_T) {
                        printError(&response);
                        break;
                    }
                    else if(response.type == CA_T) {
                        data = (char*)response.data;
                        if(print){
                            printf("%d ", line);
                            print = 0;
                        }
                        printf("%s", data);
                    }

                    message = createMessage();
                    message.type = ACK_T;
                    sequence++;
                    sequence = sequence % MAX_SEQ;
                    message.sequence = sequence;
                }
                line ++;
                print = 1;
                printf("\n");
                
                if(response.type == ERRO_T) {
                    break;
                }
            }
            if(response.type == END_T) {
                sendResponse(socket_id, &message);
            }
        }
        else if(strcmp(command, "linha") == 0) {
            messages = createQueue();
            mountLinha(argv[1], argv[2], messages);
            
            int sequence = 1;
            char* data;
            message = messages->start->message;
            while(response.type != END_T) {
                sendMessage(socket_id, &message, &response);

                if(response.type == ERRO_T) {
                    printError(&response);
                    break;
                }
                else if(response.type == ACK_T) {
                    deQueue(messages);
                    message = createMessage();
                    message = messages->start->message;
                }
                else if(response.type == CA_T) {
                    data = (char*)response.data;
                    printf("%s", data);

                    message = createMessage();
                    message.type = ACK_T;
                    sequence++;
                    sequence = sequence % MAX_SEQ;
                    message.sequence = sequence;  
                }
            }
            printf("\n");
            if(response.type == END_T) {
                message = createMessage();
                message.type = ACK_T;
                sequence++;
                sequence = sequence % MAX_SEQ;
                message.sequence = sequence;  
                sendResponse(socket_id, &message);
            }
            free(messages);
        }
        else if(strcmp(command, "linhas") == 0) {
            messages = createQueue();
            int line = mountLinhas(argv[1], argv[2], argv[3], messages);
            int sequence = 1;
            char* data;
            int print = 1;
            message = messages->start->message;
            while(response.type != END_T) {
                response.type = ACK_T;
                while((response.type != ENDL_T) && (response.type != END_T)) {
                    sendMessage(socket_id, &message, &response);

                    if(response.type == ERRO_T) {
                        printError(&response);
                        break;
                    }
                    else if(response.type == ACK_T) {
                        deQueue(messages);
                        message = createMessage();
                        message = messages->start->message;
                    }
                    else if(response.type == CA_T) {
                        data = (char*)response.data;
                        if(print){
                            printf("%d ", line);
                            print = 0;
                        }
                        printf("%s", data);

                        message = createMessage();
                        message.type = ACK_T;
                        sequence++;
                        sequence = sequence % MAX_SEQ;
                        message.sequence = sequence;  
                    }
                    else if(response.type == ENDL_T) {
                        message = createMessage();
                        message.type = ACK_T;
                        sequence++;
                        sequence = sequence % MAX_SEQ;
                        message.sequence = sequence;  
                    }
                }
                line++;
                print = 1;
                printf("\n");
                
                if(response.type == ERRO_T) {
                    break;
                }
            }
            if(response.type == END_T) {
                message = createMessage();
                message.type = ACK_T;
                sequence++;
                sequence = sequence % MAX_SEQ;
                message.sequence = sequence;
                sendResponse(socket_id, &message);
            }
            free(messages);
        }
        else if(strcmp(command, "edit") == 0) {
            messages = createQueue();
            mountEdit(argv[1], argv[2], argv[3], messages);
            message = messages->start->message;
            while(message.type != END_T) {
                sendMessage(socket_id, &message, &response);

                if(response.type == ERRO_T) {
                    printError(&response);
                    break;
                }
                
                deQueue(messages);
                message = messages->start->message;
            }
            
            if(message.type == END_T) {
                sendMessage(socket_id, &message, &response);
            }
            free(messages);
        }
        else if(strcmp(command, "exit") == 0) {
            mountExit(&message);
            sendResponse(socket_id, &message);
            break;
        }
        else {
            printf("%s: command not found\n", command);
        }
    
        printCurrentDir();
        fgets(input, sizeof(input), stdin);
        parseArgs(input, argv);
    }
    
    freeArgs(argv);
    close(socket_id);

    return 0;
}