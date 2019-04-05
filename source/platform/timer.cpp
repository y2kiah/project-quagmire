#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
#define ASSERT_TIMER_INITIALIZED	assert(gCountsPerSecond != 0 && "High performance timer not initialized")
#else
#define ASSERT_TIMER_INITIALIZED
#endif

global i64 gCountsPerSecond = 0;
global f64 gSecondsPerCount = 0.0;

#ifdef _WIN32

inline i64 getPerformanceFrequency() {
	i64 freq = 0;
	SetThreadAffinityMask(GetCurrentThread(), 1);

	// get high performance counter frequency
	BOOL result = QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
	return (result == 0 ? 0 : freq);
}

inline i64 getPerformanceCounter() {
	ASSERT_TIMER_INITIALIZED;
	i64 now = 0;
	BOOL result = QueryPerformanceCounter((LARGE_INTEGER*)&now);
	return (result == 0 ? -1 : now);
}

#else

inline i64 getPerformanceFrequency() {
	return (i64)SDL_GetPerformanceFrequency();
}

inline i64 getPerformanceCounter() {
	ASSERT_TIMER_INITIALIZED;
	return (i64)SDL_GetPerformanceCounter();
}

#endif



i64 timer_queryCounts()
{
	return getPerformanceCounter();
}

i64 timer_queryCountsSince(i64 startCounts)
{
	return (timer_queryCounts() - startCounts);
}

f64 timer_querySecondsSince(i64 startCounts)
{
	return (timer_queryCountsSince(startCounts) * gSecondsPerCount);
}

f64 timer_queryMillisSince(i64 startCounts)
{
	return (timer_querySecondsSince(startCounts) * 1000.0);
}

f64 timer_secondsBetween(i64 startCounts, i64 stopCounts)
{
	ASSERT_TIMER_INITIALIZED;

	return (stopCounts - startCounts) * gSecondsPerCount;
}

f64 timer_millisBetween(i64 startCounts, i64 stopCounts)
{
	return timer_secondsBetween(startCounts, stopCounts) * 1000.0;
}

bool initHighPerfTimer()
{
	// get high performance counter frequency
	gCountsPerSecond = getPerformanceFrequency();
	if (gCountsPerSecond == 0) {
		//debugPrintf("Timer::initTimer: QueryPerformanceFrequency failed (error %d)\n", GetLastError());
		return false;
	}

	gSecondsPerCount = 1.0 / (f64)(gCountsPerSecond);

	// test counter function
	if (getPerformanceCounter() == -1) {
		//debugPrintf("Timer::initTimer: QueryPerformanceCounter failed (error %d)\n", GetLastError());
		return false;
	}
	
	return true;
}

// Member Functions

void Timer::init()
{
	ASSERT_TIMER_INITIALIZED;

	countsPerSecond = gCountsPerSecond;
	secondsPerCount = gSecondsPerCount;
	countsPerMs = countsPerSecond / 1000;
}

i64 Timer::start()
{
	if (countsPerSecond == 0) {
		init();
	}
	
	countsPassed = 0;
	millisPassed = 0;
	secondsPassed = 0;
	
	startCounts = getPerformanceCounter();

	stopCounts = startCounts;
	return startCounts;
}

i64 Timer::stop()
{
	// query the current counts from QPC
	stopCounts = getPerformanceCounter();

	// get time passed since start
	countsPassed = max(stopCounts - startCounts, 0LL);

	secondsPassed = (f64)(countsPassed) * secondsPerCount;
	millisPassed = secondsPassed * 1000.0;

	return countsPassed;
}

i64 Timer::queryCountsPassed()
{
	i64 counts = stop();
	startCounts = stopCounts;

	return counts;
}

i64 Timer::queryCurrentCounts()
{
	ASSERT_TIMER_INITIALIZED;

	i64 now = getPerformanceCounter();
	return max(now - startCounts, 0LL);
}

f64 Timer::queryCurrentSeconds()
{
	return (queryCurrentCounts() * secondsPerCount);
}
