#ifndef ENGINE_H
#define ENGINE_H

typedef void (*FP_LOG)(int, const char *format, ...);

struct EngineAPI
{
	FP_LOG LOG;
};

#if ENGINE_FUNCTION_POINTERS
FP_LOG LOG;

static void InitEngineAPI(const EngineAPI &api)
{
	LOG = api.LOG;
}
#endif // ENGINE_FUNCTION_POINTERS

#endif // ENGINE_H
