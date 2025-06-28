#define ENGINE_FUNCTION_POINTERS 1
#include "engine.h"

GAMEAPI void GameStart()
{
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

