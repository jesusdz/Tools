#include <stdio.h>

#if _WIN32
#define GAMEAPI extern "C" __declspec(dllexport)
#else
#define GAMEAPI extern "C"
#endif

GAMEAPI void initialize()
{
	printf("game initialize()\n");
}

GAMEAPI void update()
{
	printf("game update()\n");
}

GAMEAPI void finalize()
{
	printf("game finalize()\n");
}

