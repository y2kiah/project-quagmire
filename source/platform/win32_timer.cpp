#if defined(_WIN32)

#if defined(QUAGMIRE_SLOWCHECKS) && QUAGMIRE_SLOWCHECKS != 0
#define ASSERT_TIMER_INITIALIZED	assert(qpc_countsPerSecond != 0 && "High performance timer not initialized")
#else
#define ASSERT_TIMER_INITIALIZED
#endif

static i64 qpc_countsPerSecond = 0;
static f64 qpc_secondsPerCount = 0.0;


i64 timer_queryCounts()
{
	ASSERT_TIMER_INITIALIZED;

	i64 now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now;
}

i64 timer_queryCountsSince(i64 startCounts)
{
	return (timer_queryCounts() - startCounts);
}

f64 timer_querySecondsSince(i64 startCounts)
{
	return (timer_queryCountsSince(startCounts) * qpc_secondsPerCount);
}

f64 timer_queryMillisSince(i64 startCounts)
{
	return (timer_querySecondsSince(startCounts) * 1000.0);
}

f64 timer_secondsBetween(i64 startCounts, i64 stopCounts)
{
	ASSERT_TIMER_INITIALIZED;

	return (stopCounts - startCounts) * qpc_secondsPerCount;
}

f64 timer_millisBetween(i64 startCounts, i64 stopCounts)
{
	return timer_secondsBetween(startCounts, stopCounts) * 1000.0;
}

bool initHighPerfTimer()
{
	SetThreadAffinityMask(GetCurrentThread(), 1);

	// get high performance counter frequency
	BOOL result = QueryPerformanceFrequency((LARGE_INTEGER *)&qpc_countsPerSecond);
	if (result == 0 || qpc_countsPerSecond == 0) {
		//debugPrintf("Timer::initTimer: QueryPerformanceFrequency failed (error %d)\n", GetLastError());
		return false;
	}

	qpc_secondsPerCount = 1.0 / (f64)(qpc_countsPerSecond);

	// test counter function
	i64 dummy = 0;
	result = QueryPerformanceCounter((LARGE_INTEGER *)&dummy);
	if (result == 0) {
		//debugPrintf("Timer::initTimer: QueryPerformanceCounter failed (error %d)\n", GetLastError());
		return false;
	}
	
	return true;
}

// Member Functions

void Timer::init()
{
	ASSERT_TIMER_INITIALIZED;

	countsPerSecond = qpc_countsPerSecond;
	secondsPerCount = qpc_secondsPerCount;
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

	QueryPerformanceCounter((LARGE_INTEGER *)&startCounts);

	stopCounts = startCounts;
	return startCounts;
}

i64 Timer::stop()
{
	ASSERT_TIMER_INITIALIZED;

	// query the current counts from QPC
	QueryPerformanceCounter((LARGE_INTEGER *)&stopCounts);

	// get time passed since start
	countsPassed = __max(stopCounts - startCounts, 0LL);

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

	i64 now = 0;
	QueryPerformanceCounter((LARGE_INTEGER *)&now);
	return now - startCounts;
}

f64 Timer::queryCurrentSeconds()
{
	return (queryCurrentCounts() * secondsPerCount);
}

#endif