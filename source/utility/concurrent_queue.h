#ifndef _CONCURRENT_QUEUE_H
#define _CONCURRENT_QUEUE_H

#include <SDL_mutex.h>
#include "dense_queue.h"

/**
 * @struct ConcurrentQueue
 * ConcurrentQueue provides functionality for thread-safe enqueue and dequeue operations.
 * @see http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
 * @see http://stackoverflow.com/questions/15278343/c11-thread-safe-queue
 */
struct alignas(64) ConcurrentQueue
{
	SDL_mutex*	lock = nullptr;
	SDL_cond*	cond = nullptr;
	
	DenseQueue	queue;
	
	// padding added for 64 byte total size, to avoid potential false sharing of the ConcurrentQueue's cache line
	u8			_padding[24] = {};

	// Functions 

	explicit ConcurrentQueue() {}
	
	explicit ConcurrentQueue(
		u16 elementSizeB,
		u32 capacity,
		void* buffer = nullptr,
		u8 assertOnFull = 1)
		: queue(elementSizeB, capacity, buffer, assertOnFull)
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
	 * @param[in]	zero	if val is nullptr, pass true to zero the new item memory
	 * @returns pointer to new item, or nullptr if the container is full
	 */
	void* push(void* inData, bool zero = true);

	/**
	 * Copies {count} items from inData into the queue.
	 * @param[in]	inData	container of items to be copied into queue
	 * @param[in]	count	number of objects to push
	 * @param[in]	zero	if val is nullptr, pass true to zero the new item memory
	 * @returns pointer to first new item, or nullptr if the container is full
	 */
	void* push_n(void* inData, u32 count, bool zero = true);

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
	 * @param[in]	max			maximum number of items to pop; may be related to capacity in
	 * 							outData to avoid overflowing the buffer
	 * @returns number of items popped
	 */
	u32 try_pop_all(void* outData, u32 max);

	/**
	 * Pops several items from the queue, or returns immediately without waiting if the list is
	 * empty. Items are pushed into the @c pushTo queue, up to the available capacity of the queue. 
	 * @param[in]	pushTo		DenseQueue to push the popped items into
	 * @returns number of items popped / pushed
	 */
	u32 try_pop_all_push(DenseQueue& pushTo);

	/**
	 * Pops an item from the queue, or returns immediately without waiting if the list is empty
	 * only if the provided predicate function evaluates to true.
	 * @param[out]	outData	   memory location to copy item into, only modified if true is returned
	 * @param[in]	p_		   predicate must return bool and accept a single param of type void*
	 * @returns true if pop succeeds, false if queue is empty or predicate returns false
	 */
	typedef bool UnaryPredicate(void*);
	bool try_pop_if(void* outData, UnaryPredicate* p_);

	/**
	 * Pops several items from the queue, or returns immediately without waiting if the list is
	 * empty. Items are popped only while the provided predicate function evaluates to true.
	 * @param[out]	outData		The popped items are copied to the provided address
	 * @param[in]	max			The maximum number of items to pop; may be related to capacity
	 * 							in outData to avoid overflowing the buffer
	 * @param[in]	p_			predicate must return bool and accept a single param of type void*
	 * @returns number of items popped
	 */
	u32 try_pop_while(void* outData, u32 max, UnaryPredicate* p_);

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
	 * @returns capacity of the queue
	 */
	u32 capacity() { return queue.capacity; }

	void init(
		u16 elementSizeB,
		u32 capacity,
		void* buffer = nullptr,
		u8 assertOnFull = 1);

	void deinit();
};

static_assert(sizeof(ConcurrentQueue) == 64, "ConcurrentQueue expected to be 64 bytes");


void* ConcurrentQueue::push(void* inData, bool zero)
{
	SDL_LockMutex(lock);
	
	void* addr = queue.push_back(inData, zero);
	
	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
	return addr;
}


void* ConcurrentQueue::push_n(void* inData, u32 count, bool zero)
{
	SDL_LockMutex(lock);

	void* addr = queue.push_back_n(count, inData, zero);

	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
	return addr;
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


u32 ConcurrentQueue::try_pop_all(void* outData, u32 max)
{
	SDL_LockMutex(lock);

	u32 numPopped = queue.pop_front_n(max, outData);

	SDL_UnlockMutex(lock);
	
	return numPopped;
}


u32 ConcurrentQueue::try_pop_all_push(DenseQueue& pushTo)
{
	SDL_LockMutex(lock);

	u32 totalPopped = 0;
	while (!queue.empty() && !pushTo.full()) {
		u32 numPopped = queue.pop_front_n(pushTo.maxContiguous(), pushTo.nextBack());
		if (numPopped > 1) {
			pushTo.push_back_n(numPopped, nullptr, false);
		}
		else if (numPopped == 1) {
			pushTo.push_back(nullptr, false);
		}
		totalPopped += numPopped;
	}

	SDL_UnlockMutex(lock);

	return totalPopped;
}


bool ConcurrentQueue::try_pop_if(void* outData, UnaryPredicate* p_)
{
	SDL_LockMutex(lock);

	bool result = (!queue.empty() && p_(queue.front()));
	if (result) {
		queue.pop_back(outData);
	}

	SDL_UnlockMutex(lock);

	return result;
}


u32 ConcurrentQueue::try_pop_while(void* outData, u32 max, UnaryPredicate* p_)
{
	SDL_LockMutex(lock);

	// get number of items that pass the predicate to pop
	u32 numPopped = 0;
	for (u32 i = 0;
		 i < queue.length && i < max && p_(queue[i]);
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


void ConcurrentQueue::init(
	u16 elementSizeB,
	u32 capacity,
	void* buffer,
	u8 assertOnFull)
{
	queue.init(elementSizeB, capacity, buffer, assertOnFull);
	
	lock = SDL_CreateMutex();
	cond = SDL_CreateCond();
}


void ConcurrentQueue::deinit()
{
	SDL_DestroyMutex(lock);
	SDL_DestroyCond(cond);
	lock = nullptr;
	cond = nullptr;

	queue.deinit();
}


// Helper Macros

// Macro for defining a type-safe ConcurrentQueue wrapper that avoids void* and elementSizeB in the api
#define ConcurrentQueueTyped(Type, name) \
	struct name {\
		enum { TypeSize = sizeof(Type) };\
		ConcurrentQueue _q;\
		name() {}\
		explicit name(u32 capacity, void* buffer = nullptr, u8 assertOnFull = 1)\
			: _q(TypeSize, capacity, buffer, assertOnFull) {}\
		Type* push(Type* inData)					{ return (Type*)_q.push((void*)inData); }\
		Type* push_n(Type* inData, u32 count) 		{ return (Type*)_q.push_n((void*)inData, count); }\
		bool try_pop(Type* outData) 				{ return _q.try_pop((void*)outData); }\
		bool try_pop(Type* outData, u32 timeoutMS)	{ return _q.try_pop((void*)outData, timeoutMS); }\
		u32 try_pop_all(Type* outData, u32 max = 0)	{ return _q.try_pop_all((void*)outData, max); }\
		u32 try_pop_all_push(DenseQueue& pushTo)	{ return _q.try_pop_all_push(pushTo); }\
		bool try_pop_if(Type* outData, ConcurrentQueue::UnaryPredicate* p_)\
													{ return _q.try_pop_if((void*)outData, p_); }\
		u32 try_pop_while(Type* outData, u32 max, ConcurrentQueue::UnaryPredicate* p_)\
													{ return _q.try_pop_while((void*)outData, max, p_); }\
		void wait_pop(Type* outData) 				{ _q.wait_pop((void*)outData); }\
		void clear() 								{ return _q.clear(); }\
		bool empty() 								{ return _q.empty(); }\
		u32 unsafe_size() 							{ return _q.unsafe_size(); }\
		u32 capacity() 								{ return _q.capacity(); }\
		void init(u32 capacity, void* buffer = nullptr, u8 assertOnFull = 1)\
													{ _q.init(TypeSize, capacity, buffer, assertOnFull); }\
		void deinit() 								{ _q.deinit(); }\
		Type* data()								{ return (Type*)_q.queue.items; }\
	};

#endif