#ifndef ENGINE_H
#define ENGINE_H

enum LogChannel
{
	LogChannelDebug,
	LogChannelInfo,
	LogChannelWarning,
	LogChannelError,
};

// Function typedefs
typedef void (*FP_Log)(LogChannel channel, const char *format, ...);

struct EngineAPI
{
	FP_Log Log;
};

#if ENGINE_FUNCTION_POINTERS

// Function pointers
FP_Log Log;

// Macros
#define LogDebug(...) Log(LogChannelDebug, __VA_ARGS__)
#define LogInfo(...) Log(LogChannelInfo, __VA_ARGS__)
#define LogWarning(...) Log(LogChannelWarning, __VA_ARGS__)
#define LogError(...) Log(LogChannelError, __VA_ARGS__)

#if _WIN32
#define GAMEAPI extern "C" __declspec(dllexport)
#else
#define GAMEAPI extern "C"
#endif

GAMEAPI void GameInitAPI(const EngineAPI &api)
{
	Log = api.Log;
}

#endif // ENGINE_FUNCTION_POINTERS

#endif // ENGINE_H
