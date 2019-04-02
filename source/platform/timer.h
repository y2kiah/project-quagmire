struct Timer {
	// Timing Variables
	i64		startCounts = 0;
	i64		stopCounts = 0;
	i64		countsPassed = 0;	// high res timer, set by QPC on windows
	f64		millisPassed = 0.0;
	f64		secondsPassed = 0.0;
	
	// Configuration Variables
	i64		countsPerSecond = 0;
	i64		countsPerMs = 0;
	f64		secondsPerCount = 0.0;

	// Functions
	void	init();
	i64		start();
	i64		stop();
	void	reset() { startCounts = stopCounts = countsPassed = 0; millisPassed = secondsPassed = 0.0; }
	i64		queryCountsPassed();
	i64		queryCurrentCounts();
	f64		queryCurrentSeconds();
	
	explicit Timer() { init(); }
};

i64	    timer_queryCounts();
i64	    timer_queryCountsSince(i64 startCounts);
f64	    timer_querySecondsSince(i64 startCounts);
f64	    timer_queryMillisSince(i64 startCounts);
f64	    timer_secondsBetween(i64 startCounts, i64 stopCounts);
f64	    timer_millisBetween(i64 startCounts, i64 stopCounts);
bool	initHighPerfTimer();