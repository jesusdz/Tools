#include <stdio.h>

extern "C"
void initialize()
{
	printf("gamelib initialize()\n");
}

extern "C"
void update()
{
	printf("gamelib update()\n");
}

extern "C"
void finalize()
{
	printf("gamelib finalize()\n");
}

