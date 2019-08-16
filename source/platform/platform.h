#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "platform_api.h"
#include "../input/platform_input.h"

typedef i32 GameUpdateAndRenderFunc(
		GameMemory* gameMemory,
		PlatformApi* platform,
		input::PlatformInput* input,
		SDLApplication* app,
		i64 realTime,
		i64 countsPassed,
		i64 countsPerMs,
		u64 frame);

typedef i32 GameOnLoadFunc(
		GameMemory* gameMemory,
		PlatformApi* platformApi,
		SDLApplication* app);

typedef void GameOnUnloadFunc(
		GameMemory* gameMemory,
		PlatformApi* platformApi,
		SDLApplication* app);

typedef void GameOnExitFunc(
		GameMemory* gameMemory,
		PlatformApi* platformApi,
		SDLApplication* app);

#ifdef _WIN32

#include "Windows.h"


struct GameCode
{
	HMODULE		gameDLL;
	FILETIME	dllLastWriteTime;

	GameUpdateAndRenderFunc*	updateAndRender;
	GameOnLoadFunc*				onLoad;
	GameOnUnloadFunc*			onUnload;
	GameOnExitFunc*				onExit;
	//game_get_sound_samples *getSoundSamples;

	bool isValid;
};

#else

struct GameCode
{
	void*		gameDLL;
	time_t		dllLastWriteTime;

	GameUpdateAndRender *updateAndRender;
	//game_get_sound_samples *getSoundSamples;

	bool isValid;
};

#endif

struct GameContext
{
	SDLApplication*			app;
	GameCode				gameCode;
	GameMemory				gameMemory;
	PlatformMemory			platformMemory;
	input::PlatformInput	input;
	volatile u32			done;
};

#endif