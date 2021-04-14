#include <stdio.h>
#include <stdlib.h>
#include "myQueue.h"

Queue* createQueue() {
	Queue* q = (Queue*) malloc(sizeof(Queue));
	q->front = NULL;
	q->rear = NULL;
	return q;
}
void Enqueue(Queue* q, void* x) {
	Node* temp = (Node*) malloc(sizeof(Node));
	temp->data = x;
	temp->next = NULL;
	if(q->front == NULL && q->rear == NULL) {
		q->front = q->rear = temp;
		return;
	}
	q->rear->next = temp;
	q->rear = temp;
}

void* Dequeue(Queue* q) {
	Node* temp = q->front;
	if(q->front == NULL) return NULL;
	if(q->front == q->rear) {
		q->front = q->rear = NULL;
	}
	else {
		q->front = temp->next;
	}
	void* tempData = temp->data;
	free(temp);
	return tempData;
}

bool isEmpty(Queue* q) {
	if(q->front == NULL) {
		return true;
	}
	return false;
}

