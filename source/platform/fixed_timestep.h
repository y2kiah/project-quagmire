struct UpdateInfo {
	i64		virtualTime;	// interpolation of real time with each step of update loop, keeps in sync with real time, useful for checking against timestamped events 
	i64		gameTime;		// current game time from 0, useful for logical game time checks, sensitive to gameSpeed, should be reset to 0 when game is restarted
	i64		deltaCounts;	// update frame clock counts
	u64		frame;			// frame counter, starts at 0
	f32		deltaMs;		// update frame time in milliseconds
	f32		deltaT;			// update frame time in seconds
	f32		gameSpeed;		// rate multiplier of gameplay, 1.0 is normal
};


struct FixedTimestep {
	typedef void (*pfUpdate)(UpdateInfo&);

	i64		accumulator = 0;
	i64 	gameTime = 0;
	i64 	virtualTime = 0;

	i64 	deltaCounts;
	f32		deltaMs;


	explicit FixedTimestep(f32 deltaMs,
						   i64 countsPerMs) :
		deltaMs{ deltaMs },
		deltaCounts{ (i64)(deltaMs * countsPerMs) }
	{}

	f32 tick(i64 realTime,
			 i64 countsPassed,
			 u64 frame,
			 f32 gameSpeed,
			 pfUpdate update)
	{
		UpdateInfo ui = { virtualTime, gameTime, deltaCounts, frame, deltaMs, deltaMs / 1000.0f, gameSpeed };

		accumulator += (i64)(countsPassed * gameSpeed);

		while (accumulator >= deltaCounts) {
			
			update(ui);

			gameTime += deltaCounts;
			virtualTime += deltaCounts;
			accumulator -= deltaCounts;
		}
		virtualTime = realTime;

		f32 interpolation = (f32)accumulator / (f32)deltaCounts;
		return interpolation;
	}
};
