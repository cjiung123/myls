#ifndef MYQUEUE_H_
#define MYQUEUE_H_
#include <stdbool.h>
typedef struct Node_s Node;
struct Node_s {
	void* data;
	Node* next;
};

typedef struct Queue_s Queue;
struct Queue_s {
	Node* front;
	Node* rear;
};

Queue* createQueue();
void Enqueue(Queue* q, void* x);
void* Dequeue(Queue* q);
bool isEmpty(Queue* q);
#endif /* MYQUEUE_H_ */
