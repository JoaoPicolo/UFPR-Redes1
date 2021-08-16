#include "kermit.h"

typedef struct Node_t { 
    message_t message; 
    struct Node_t* next; 
} Node_t;

typedef struct {
    int size;
    Node_t *start, *end; 
} Queue_t; 

Queue_t* createQueue();

Node_t* newNode(message_t m);

void enQueue(Queue_t* q, message_t m);

void deQueue(Queue_t* q);