#ifndef _CONCURRENT_QUEUE_H
#define _CONCURRENT_QUEUE_H

#include <SDL2/SDL_mutex.h>
#include "dense_queue.h"

/**
 * @struct ConcurrentQueue
 * ConcurrentQueue provides functionality for thread-safe enqueue and dequeue operations.
 * @see http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 * @see http://stackoverflow.com/questions/15278343/c11-thread-safe-queue
 */
struct ConcurrentQueue {
	SDL_mutex*	lock = nullptr;
	SDL_cond*	cond = nullptr;
	
	DenseQueue	queue;
	
	// padding added for 64 byte total size, to avoid potential false sharing of the ConcurrentQueue's cache line
	u8			_padding[24] = {};


	/**
	* Constructors
	*/
	explicit ConcurrentQueue(u16 elementSizeB,
							 u32 capacity,
							 void* buffer = nullptr) :
		queue(elementSizeB, capacity, buffer)
	{
		lock = SDL_CreateMutex();
		cond = SDL_CreateCond();
	}

	~ConcurrentQueue() {
		SDL_DestroyMutex(lock);
		SDL_DestroyCond(cond);
	}

	/**
	* Thread-safe push onto the queue. Also updates the condition variable so any threads
	* locked in waitPop will take the mutex and process the pop.
	* @param[in]	inData	item to be copied into queue
	*/
	void push(void* inData);

	/**
	* Copies all items in container inData into the queue.
	* @param[in]	inData	container of items to be copied into queue
	* @param[in]	count	number of objects to push
	*/
	void push_all(void* inData, int count);

	/**
	* Pops an item from the queue, or returns immediately without waiting if the list is
	* empty. Most likely would use this in the main thread to pop items pushed from a worker
	* thread.
	* @param[out]	outData		memory location to copy item into, only modified if true is returned
	* @returns true if pop succeeds, false if queue is empty
	*/
	bool try_pop(void* outData);

	/**
	* Pops an item from the queue, or waits for specified timeout period for an item.
	* Most likely would use this in the main thread to pop items pushed from a worker thread.
	* @param[out]	outData		memory location to copy item into, only modified if true is returned
	* @param[in]	timeoutMS	timeout period in milliseconds
	* @returns true if pop succeeds, false if queue is empty for duration
	*/
	bool try_pop(void* outData, u32 timeoutMS);

	/**
	* Pops several items from the queue, or returns immediately without waiting if the list is
	* empty. Items are popped only up to a max count passed in.
	* @param[out]	outData		The popped items are copied to the provided address.
	* @param[in]	max			maximum number of items to pop, or 0 (default) for unlimited
	* @returns number of items popped
	*/
	u32 try_pop_all(void* outData, int max = 0);

	/**
	* Pops an item from the queue, or returns immediately without waiting if the list is empty
	* only if the provided predicate function evaluates to true.
	* @param[out]	outData	   memory location to copy item into, only modified if true is returned
	* @param[in]	p_		   predicate must return bool and accept a single param of type void*
	* @returns true if pop succeeds, false if queue is empty or predicate returns false
	*/
	typedef bool (*pfUnaryPredicate)(void*);
	bool try_pop_if(void* outData, pfUnaryPredicate p_);

	/**
	* Pops several items from the queue, or returns immediately without waiting if the list is
	* empty. Items are popped only while the provided predicate function evaluates to true.
	* @param[out]	outData		The popped items are copied to the provided address
	* @param[in]	p_			predicate must return bool and accept a single param of type void*
	* @returns number of items popped
	*/
	u32 try_pop_all_if(void* outData, pfUnaryPredicate p_);

	/**
	* Waits indefinitely for a condition variable that indicates data is available in the
	* queue. Most likely would use this in a worker thread to execute tasks pushed from a
	* client thread.
	* @param[out]	outData		memory location to move item into
	*/
	void wait_pop(void* outData);

	/**
	* Concurrency-safe clear the queue
	*/
	void clear();

	/**
	* Concurrency-safe check of whether the queue is empty
	* @returns true if the queue is empty
	*/
	bool empty();

	/**
	* unsafe_size is not concurrency-safe and can produce incorrect results if called
	* concurrently with calls to push*, pop*, and empty methods.
	* @returns size of the queue
	*/
	u32 unsafe_size() { return queue.length; }

	/**
	* unsafe_capacity is not concurrency-safe and can produce incorrect results if called
	* concurrently with calls to push*, pop*, and empty methods.
	* @returns capacity of the queue
	*/
	u32 unsafe_capacity() { return queue.capacity; }
};

static_assert(sizeof(ConcurrentQueue) == 64, "ConcurrentQueue expected to be 64 bytes");


void ConcurrentQueue::push(void* inData)
{
	SDL_LockMutex(lock);
	
	queue.push_back(inData);
	
	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
}


void ConcurrentQueue::push_all(void* inData, int count)
{
	SDL_LockMutex(lock);

	queue.push_back_n(count, inData);

	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
}


bool ConcurrentQueue::try_pop(void* outData)
{
	assert(outData);

	SDL_LockMutex(lock);

	bool result = !queue.empty();
	if (result) {
		queue.pop_front(outData);
	}

	SDL_UnlockMutex(lock);

	return result;
}


bool ConcurrentQueue::try_pop(void* outData, u32 timeoutMS)
{
	SDL_LockMutex(lock);

	// continue waiting while the queue is empty, even on spurious wakeup
	// stop waiting if timeout is returned
	while (SDL_CondWaitTimeout(cond, lock, timeoutMS) == 0 && queue.empty()) {
		continue;
	}

	bool result = !queue.empty();
	if (result) {
		queue.pop_front(outData);
	}

	SDL_UnlockMutex(lock);

	return result;
}


u32 ConcurrentQueue::try_pop_all(void* outData, int max)
{
	SDL_LockMutex(lock);

	u32 numPopped = queue.pop_front_n(max, outData);

	SDL_UnlockMutex(lock);
	
	return numPopped;
}


bool ConcurrentQueue::try_pop_if(void* outData, pfUnaryPredicate p_)
{
	SDL_LockMutex(lock);

	bool result = (!queue.empty() && p_(queue.front()));
	if (result) {
		queue.pop_back(outData);
	}

	SDL_UnlockMutex(lock);

	return result;
}


u32 ConcurrentQueue::try_pop_all_if(void* outData, pfUnaryPredicate p_)
{
	SDL_LockMutex(lock);

	// get number of items that pass the predicate to pop
	u32 numPopped = 0;
	for (u32 i = 0;
		 i < queue.length && p_(queue[i]);
		 ++i)
	{
		++numPopped;
	}

	if (numPopped > 0) {
		numPopped = queue.pop_front_n(numPopped, outData);
	}

	SDL_UnlockMutex(lock);

	return numPopped;
}


void ConcurrentQueue::wait_pop(void* outData)
{
	SDL_LockMutex(lock);

	// continue waiting while the queue is empty, even on spurious wakeup
	while (queue.empty()) {
		SDL_CondWait(cond, lock);
	}

	if (!queue.empty()) {
		queue.pop_front(outData);
	}

	SDL_UnlockMutex(lock);
}


void ConcurrentQueue::clear()
{
	SDL_LockMutex(lock);

	queue.clear();

	SDL_UnlockMutex(lock);
}


bool ConcurrentQueue::empty()
{
	SDL_LockMutex(lock);
	
	bool result = queue.empty();

	SDL_UnlockMutex(lock);
	
	return result;
}

#endif