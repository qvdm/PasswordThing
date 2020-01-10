/*
 * Name: Queue.cpp
 * 
 * Purpose: Provides a simple queue implementation
 * 
 * Provides:
 *   enQueue
 *   deQueue
 *   Size
 *   isFull
 *   isEmpty
 * 
 * Private: 
  *
 * Operation:
 *   Just a Queue.  No idea where I got the code from originally, but an alsmost identical version appears in various tutorials around the web.  
 * 
 */

#ifndef MAINT


#include "queue.h"

// CTOR
Queue::Queue()
{
	front = 0;
	rear = -1;
	count = 0;
}

Queue::~Queue() {}


// Remove front element from the queue
byte Queue::dequeue()
{
	// check for queue underflow
	if (isEmpty())
	{
      return 0;
	}
    byte c = arr[front];
	front = (front+1) % MAXQUEUE;
	count--;
    return c;
}

// Add an item to the queue
bool Queue::enqueue(byte item)
{
	// check for queue overflow
	if (isFull())
	{
      return false;
	}

	rear = (rear + 1) % MAXQUEUE;
	arr[rear] = item;
	count++;
	return true;
}

// Return the size of the queue
int Queue::size()
{
	return count;
}

// Check if the queue is empty or not
bool Queue::isEmpty()
{
	return (size() == 0);
}

// Check if the queue is full or not
bool Queue::isFull()
{
	return (size() == MAXQUEUE);
}

#endif