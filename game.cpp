#define ENGINE_FUNCTION_POINTERS 1
#include "engine.h"

#if _WIN32
#define GAMEAPI extern "C" __declspec(dllexport)
#else
#define GAMEAPI extern "C"
#endif

GAMEAPI void GameStart(const EngineAPI &api)
{
	// Initialize engine global function pointers
	InitEngineAPI(api);

	LogDebug("- GameStart!\n");
}

GAMEAPI void GameUpdate()
{
	LogDebug("- GameUpdate!\n");
}

GAMEAPI void GameStop()
{
	LogDebug("- GameStop!\n");
}

