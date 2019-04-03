/**
* @struct concurrent_queue
* concurrent_queue provides functionality for thread-safe enqueue and dequeue operations.
* @see http://www.justsoftwaresolutions.co.uk/threading/implementing-a-thread-safe-queue-using-condition-variables.html
* @see http://stackoverflow.com/questions/15278343/c11-thread-safe-queue
*/
struct concurrent_queue {
	SDL_mutex*	lock = nullptr;
	SDL_cond*	cond = nullptr;
	
	dense_queue queue;
	
	// padding added for 64 byte total size, to avoid potential false sharing of the concurrent_queue's cache line
	u8     		_padding[16] = {};


	/**
	* Constructors
	*/
	explicit concurrent_queue(u16 elementSizeB,
							  u32 capacity,
							  void* buffer = nullptr) :
		queue(elementSizeB, capacity, buffer)
	{
		assert(sizeof(concurrent_queue) == 64);

		lock = SDL_CreateMutex();
		cond = SDL_CreateCond();
	}

	~concurrent_queue() {
		SDL_DestroyMutex(lock);
		SDL_DestroyCond(cond);
	}

	/**
	* Thread-safe push onto the queue. Also updates the condition variable so any threads
	* locked in waitPop will take the mutex and process the pop.
	* @param	inData	item to be moved into queue
	*/
	void push(void* inData);

	/**
	* Copies all items in container inData into the queue.
	* @param	inData	container of items to be moved into queue
	* @param    count   number of objects to push
	*/
	void push_all(void* inData, int count);

	/**
	* Pops an item from the queue, or returns immediately without waiting if the list is
	* empty. Most likely would use this in the main thread to pop items pushed from a worker
	* thread.
	* @param	outData	   memory location to move item into, only modified if true is returned
	* @returns true if pop succeeds, false if queue is empty
	*/
	bool try_pop(void** outData);

	/**
	* Pops an item from the queue, or waits for specified timeout period for an item.
	* Most likely would use this in the main thread to pop items pushed from a worker thread.
	* @param	outData	   memory location to move item into, only modified if true is returned
	* @param	timeoutMS  timeout period in milliseconds
	* @returns true if pop succeeds, false if queue is empty for duration
	*/
	bool try_pop(void** outData, u32 timeoutMS);

	/**
	* Pops several items from the queue, or returns immediately without waiting if the list is
	* empty. Items are popped only up to a max count passed in.
	* @param	outData	The popped items are emplaced into the provided container.
	* @param	max		maximum number of items to pop, or 0 (default) for unlimited
	* @returns number of items popped (and emplaced in outData)
	*/
	int try_pop_all(void** outData, int max = 0);

	/**
	* Pops an item from the queue, or returns immediately without waiting if the list is empty
	* only if the provided predicate function evaluates to true.
	* @param	outData	   memory location to move item into, only modified if true is returned
	* @param	p_		   predicate must return bool and accept a single param of type void*
	* @returns true if pop succeeds, false if queue is empty or predicate returns false
	*/
	typedef bool (*pfUnaryPredicate)(void*);
	bool try_pop_if(void** outData, pfUnaryPredicate p_);

	/**
	* Pops several items from the queue, or returns immediately without waiting if the list is
	* empty. Items are popped only while the provided predicate function evaluates to true.
	* @param	outData	The popped items are emplaced into the provided container.
	* @param	p_		   predicate must return bool and accept a single param of type void*
	* @returns number of items popped (and emplaced in outData)
	*/
	int try_pop_all_if(void** outData, pfUnaryPredicate p_);

	/**
	* Waits indefinitely for a condition variable that indicates data is available in the
	* queue. Most likely would use this in a worker thread to execute tasks pushed from a
	* client thread.
	* @param	outData	   memory location to move item into
	*/
	void wait_pop(void** outData);

	/**
	* Concurrency-safe clear the queue
	*/
	void clear();

	/**
	* Concurrency-safe check of whether the queue is empty
	* @returns whether the queue is empty: i.e. whether its size is zero
	*/
	bool empty();

	/**
	* unsafe_size is not concurrency-safe and can produce incorrect results if called
	* concurrently with calls to push*, pop*, and empty methods.
	* @returns size of the queue
	*/
	size_t unsafe_size() { return queue.length; }

	/**
	* unsafe_capacity is not concurrency-safe and can produce incorrect results if called
	* concurrently with calls to push*, pop*, and empty methods.
	* @returns capacity of the queue
	*/
	size_t unsafe_capacity() { return queue.capacity; }
};


void concurrent_queue::push(void* inData)
{
	SDL_LockMutex(lock);
	
	queue.push_back(inData);
	
	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
}


void concurrent_queue::push_all(void* inData, int count)
{
	void* src = inData;
	
	SDL_LockMutex(lock);

	for (int c = 0; c < count; ++c) {
		queue.push_back(src);
		src = (void*)((uintptr_t)src + queue.elementSizeB);
	}

	SDL_UnlockMutex(lock);
	SDL_CondSignal(cond);
}


bool concurrent_queue::try_pop(void** outData)
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


bool concurrent_queue::try_pop(void** outData, u32 timeoutMS)
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


int concurrent_queue::try_pop_all(void** outData, int max)
{
	int numPopped = 0;
	void** dst = outData;

	SDL_LockMutex(lock);

	while (!queue.empty() &&
		(max <= 0 || numPopped < max))
	{
		queue.pop_front(dst);
		dst = (void**)((uintptr_t)dst + queue.elementSizeB);
		
		++numPopped;
	}

	SDL_UnlockMutex(lock);
	
	return numPopped;
}


bool concurrent_queue::try_pop_if(void** outData, pfUnaryPredicate p_)
{
	SDL_LockMutex(lock);

	bool result = (!queue.empty() && p_(queue.front()));
	if (result) {
		queue.pop_back(outData);
	}

	SDL_UnlockMutex(lock);

	return result;
}


int concurrent_queue::try_pop_all_if(void** outData, pfUnaryPredicate p_)
{
	int numPopped = 0;
	void** dst = outData;

	SDL_LockMutex(lock);

	while (!queue.empty() && p_(queue.front())) {
		queue.pop_front(dst);
		dst = (void**)((uintptr_t)dst + queue.elementSizeB);

		++numPopped;
	}

	SDL_UnlockMutex(lock);

	return numPopped;
}


void concurrent_queue::wait_pop(void** outData)
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


void concurrent_queue::clear()
{
	SDL_LockMutex(lock);

	queue.clear();

	SDL_UnlockMutex(lock);
}


bool concurrent_queue::empty()
{
	SDL_LockMutex(lock);
	
	bool result = queue.empty();

	SDL_UnlockMutex(lock);
	
	return result;
}
