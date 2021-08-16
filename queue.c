#include <stdlib.h> 

#include "queue.h"
#include "kermit.h"

Queue_t* createQueue() { 
    Queue_t* q = (Queue_t*)malloc(sizeof(Queue_t));
    q->size = 0;
    q->start = q->end = NULL; 
    return q; 
}

Node_t* newNode(message_t m) {
    Node_t* temp = (Node_t*)malloc(sizeof(Node_t)); 
    temp->message = m; 
    temp->next = NULL; 
    return temp; 
}

void enQueue(Queue_t* q, message_t m) { 
    // Create a new LL node 
    Node_t* temp = newNode(m); 
  
    // If queue is empty, then new node is start and end both 
    if (q->end == NULL) { 
        q->start = q->end = temp;
        q->size++;
        return; 
    }
  
    // Add the new node at the end of queue and change end 
    q->end->next = temp; 
    q->end = temp;
    q->size++;
}

void deQueue(Queue_t* q) { 
    // If queue is empty, return NULL. 
    if (q->start == NULL) 
        return; 
  
    // Store previous start and move start one node ahead 
    Node_t* temp = q->start; 
  
    q->start = q->start->next; 
  
    // If start becomes NULL, then change end also as NULL 
    if (q->start == NULL) 
        q->end = NULL; 
    
    q->size--;
    free(temp); 
} 