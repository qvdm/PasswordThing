#ifndef QUEUE_H
#define QUEUE_H
#include "hardware.h"

// define default capacity of the queue
#define MAXQUEUE 256

// Class for queue
class Queue
{

  public:
	Queue();
    ~Queue();
	byte dequeue();
	bool enqueue(byte x);
	bool isEmpty();
	bool isFull();
    int size();

  private:
	byte arr[MAXQUEUE];
	int front=0;		// front points to front element in the queue (if any)
	int rear=0;		// rear points to last element in the queue
	int count=0;		// current size of the queue


};

#endif