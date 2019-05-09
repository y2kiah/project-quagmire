#ifndef _PLATFORM_H
#define _PLATFORM_H

#include "platform_api.h"
#include "../input/platform_input.h"

typedef u8 GameUpdateAndRenderFunc(
		GameMemory* gameMemory,
		PlatformApi* platform,
		input::PlatformInput* input,
		SDLApplication* app,
		i64 realTime,
		i64 countsPassed,
		i64 countsPerMs,
		u64 frame);

typedef u8 GameOnLoadFunc(
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

	GameUpdateAndRenderFunc*	updateAndRender = nullptr;
	GameOnLoadFunc*				onLoad = nullptr;
	GameOnExitFunc*				onExit = nullptr;
	//game_get_sound_samples *getSoundSamples;

	bool isValid = false;
};

#else

struct GameCode
{
	void*		gameDLL;
	time_t		dllLastWriteTime;

	GameUpdateAndRender *updateAndRender = nullptr;
	//game_get_sound_samples *getSoundSamples;

	bool isValid = false;
};

#endif

struct GameContext {
	SDLApplication*			app = nullptr;
	GameCode				gameCode = {};
	GameMemory				gameMemory = {};
	PlatformMemory			platformMemory;
	input::PlatformInput	input = {};
	std::atomic_bool		done; // TODO: consider using SDL atomics instead of std to avoid the template
};

#endif