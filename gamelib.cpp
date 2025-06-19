#include <stdio.h>

#if _WIN32
#define GAMEAPI extern "C" __declspec(dllexport)
#else
#define GAMEAPI extern "C"
#endif

GAMEAPI void initialize()
{
	printf("gamelib initialize()\n");
}

GAMEAPI void update()
{
	printf("gamelib update()\n");
}

GAMEAPI void finalize()
{
	printf("gamelib finalize()\n");
}

