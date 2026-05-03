
#ifndef PLATFORM_H
#define PLATFORM_H

#include "tools.h"


////////////////////////////////////////////////////////////////////////////////////////////////////
// Dynamic library interface

#if PLATFORM_WINDOWS
#define ENGINE_API extern "C" __declspec(dllexport)
#else
#define ENGINE_API extern "C"
#endif


////////////////////////////////////////////////////////////////////////////////////////////////////
// Thread IDs

#define WORK_QUEUE_WORKER_COUNT 8

enum ThreadID
{
	THREAD_ID_UPDATE,
	THREAD_ID_AUDIO,
	THREAD_ID_WORKER_0,
	THREAD_ID_WORKER_LAST = THREAD_ID_WORKER_0 + WORK_QUEUE_WORKER_COUNT - 1,
};

#if PLATFORM_WINDOWS || PLATFORM_LINUX
#define USE_UPDATE_THREAD 1
#else
#define USE_UPDATE_THREAD 0
#endif

#if PLATFORM_LINUX || PLATFORM_WINDOWS
#define USE_AUDIO_THREAD 1
#elif PLATFORM_ANDROID
#define USE_AUDIO_THREAD 0 // Do not set to 1, Android invokes AAudioFillAudioBuffer from its own audio thread
#endif




////////////////////////////////////////////////////////////////////////////////////////////////////
// Window and input

#if USE_XCB
#	include <xcb/xcb.h>
#	include <stdlib.h> // free
#endif



enum Key
{
	K_NULL,
	K_LEFT, K_RIGHT, K_UP, K_DOWN,
	K_ESCAPE,
	K_SPACE,
	K_BACKSPACE,
	K_DELETE,
	K_RETURN,
	K_TAB,
	K_CONTROL,
	K_SHIFT,
	K_ALT,
	K_0, K_1, K_2,
	K_3, K_4, K_5,
	K_6, K_7, K_8, K_9,
	K_A, K_B, K_C, K_D,
	K_E, K_F, K_G, K_H,
	K_I, K_J, K_K, K_L,
	K_M, K_N, K_O, K_P,
	K_Q, K_R, K_S, K_T,
	K_U, K_V, K_W, K_X,
	K_Y, K_Z,
	K_COUNT,
};

enum MouseButton
{
	MOUSE_BUTTON_LEFT,
	MOUSE_BUTTON_RIGHT,
	MOUSE_BUTTON_MIDDLE,
	MOUSE_BUTTON_COUNT,
};

enum KeyState
{
	KEY_STATE_IDLE,
	KEY_STATE_PRESS,
	KEY_STATE_PRESSED,
	KEY_STATE_RELEASE,
};

enum ButtonState
{
	BUTTON_STATE_IDLE,
	BUTTON_STATE_PRESS,
	BUTTON_STATE_PRESSED,
	BUTTON_STATE_RELEASE,
};

enum TouchState
{
	TOUCH_STATE_IDLE,
	TOUCH_STATE_PRESS,
	TOUCH_STATE_PRESSED,
	TOUCH_STATE_RELEASE,
};

struct Keyboard
{
	KeyState keys[K_COUNT];
};

struct Mouse
{
	i32 x, y;
	i32 dx, dy;
	i32 wx, wy; // wheel x and y
	ButtonState buttons[MOUSE_BUTTON_COUNT];
};

#define MAX_INPUT_CHARS 16

struct Chars
{
	char chars[MAX_INPUT_CHARS];
	u32 charCount;
	bool shift;
	bool ctrl;
	bool alt;
};

struct Touch
{
	f32 x0, y0;
	f32 x, y;
	f32 dx, dy;
	TouchState state;
};

inline bool KeyPress(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESS;
}

inline bool KeyPressed(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESSED;
}

inline bool KeyRelease(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_RELEASE;
}

inline bool ButtonPress(ButtonState state)
{
	return state == BUTTON_STATE_PRESS;
}

inline bool ButtonPressed(ButtonState state)
{
	return state == BUTTON_STATE_PRESSED;
}

inline bool ButtonRelease(ButtonState state)
{
	return state == BUTTON_STATE_RELEASE;
}

inline bool MouseMoved(const Mouse &mouse)
{
	return mouse.dx != 0 || mouse.dy != 0;
}

inline bool MouseButtonPress(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPress(mouse.buttons[button]);
}

inline bool MouseButtonPressed(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPressed(mouse.buttons[button]);
}

inline bool MouseButtonRelease(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonRelease(mouse.buttons[button]);
}

inline bool MouseButtonChanged(const Mouse &mouse, MouseButton button)
{
	return MouseButtonPress(mouse, button) || MouseButtonRelease(mouse, button);
}

inline bool MouseChanged(const Mouse &mouse)
{
	return MouseMoved(mouse)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_LEFT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_RIGHT)
		|| MouseButtonChanged(mouse, MOUSE_BUTTON_MIDDLE);
}

enum WindowFlags
{
	WindowFlags_WasCreated  = 1 << 0,
	WindowFlags_WillDestroy = 1 << 1,
	WindowFlags_WasResized  = 1 << 2,
	WindowFlags_Exit        = 1 << 3,
};

struct Window
{
#if USE_XCB
	xcb_connection_t *connection;
	xcb_window_t window;
	xcb_atom_t closeAtom;
#elif USE_ANDROID
	ANativeWindow *nativeWindow;
#elif USE_WINAPI
	HINSTANCE hInstance;
	HWND hWnd;
#endif
	u32 width;
	u32 height;
	u32 flags;

	Keyboard keyboard;
	Mouse mouse;
	Chars chars;
	Touch touches[2];
};

struct Gamepad
{
	union {
		struct {
			ButtonState start;
			ButtonState back;
			ButtonState up;
			ButtonState down;
			ButtonState left;
			ButtonState right;
			ButtonState a;
			ButtonState b;
			ButtonState x;
			ButtonState y;
			ButtonState leftShoulder;
			ButtonState rightShoulder;
		};
		ButtonState buttons[12];
	};
	float leftTrigger;
	float rightTrigger;
	float2 leftAxis;
	float2 rightAxis;
};

struct Input
{
#if PLATFORM_WINDOWS
	DynamicLibrary library;
#elif PLATFORM_LINUX
	int fd; // file descriptor
#endif

	Gamepad gamepad;
};

struct AudioDevice
{
	// Config
	u16 channelCount;
	u16 bytesPerSample;
	u16 samplesPerSecond;
	u16 bufferSize;
	u16 latencyFrameCount;
	u16 latencySampleCount;
	u16 safetyBytes;
	u32 runningSampleIndex;

#if PLATFORM_WINDOWS
	DynamicLibrary library;
	LPDIRECTSOUNDBUFFER buffer;
#elif PLATFORM_LINUX
	DynamicLibrary library;
	snd_pcm_t *pcm;
#elif PLATFORM_ANDROID
	AAudioStream *stream;
#endif

	bool initialized;
	bool isPlaying;
	bool soundIsValid;

	i16 *outputSamples;
};

struct SoundBuffer
{
	u16 samplesPerSecond;
	u16 sampleCount;
	i16* samples;
};

enum PlatformEventType
{
	PlatformEventTypeWindowWasCreated,
	PlatformEventTypeWindowWillDestroy,
	PlatformEventTypeWindowResize,
	PlatformEventTypeKeyPress,
	PlatformEventTypeMouseClick,
	PlatformEventTypeMouseMove,
	PlatformEventTypeMouseWheel,
	PlatformEventTypeQuit,
	PlatformEventTypeCount,
};

struct PlatformEventWindowResize
{
	u16 width, height;
};

struct PlatformEventKeyPress
{
	Key code;
	KeyState state;
};

struct PlatformEventMouseClick
{
	MouseButton button;
	ButtonState state;
};

struct PlatformEventMouseMove
{
	i16 x, y;
};

struct PlatformEventMouseWheel
{
	i16 dx, dy;
};

struct PlatformEvent
{
	PlatformEventType type;
	union
	{
		PlatformEventWindowResize windowResize;
		PlatformEventKeyPress keyPress;
		PlatformEventMouseClick mouseClick;
		PlatformEventMouseMove mouseMove;
		PlatformEventMouseWheel mouseWheel;
	};
};

struct PlatformConfig
{
#if PLATFORM_ANDROID
	struct android_app *androidApp;
#endif // PLATFORM_ANDROID
};

typedef void (*PFN_PlatformQuit)();
typedef u32  (*PFN_AcquireScratchArena)(Arena &outArena);
typedef void (*PFN_ReleaseScratchArena)(u32 index);

struct PlatformAPI
{
	PFN_PlatformQuit        PlatformQuit;
	PFN_AcquireScratchArena AcquireScratchArena;
	PFN_ReleaseScratchArena ReleaseScratchArena;
};

#define MAX_SCRATCH_ARENAS 8

struct Platform
{
	i32 argc;
	char **argv;

	u32 globalMemorySize = MB(64);
	u32 frameMemorySize = MB(16);
	u32 stringMemorySize = KB(16);
	u32 dataMemorySize = MB(16);

	bool (*PreInitCallback)(Platform &);
	bool (*InitCallback)(Platform &);
	void (*UpdateCallback)(Platform &);
	void (*RenderGraphicsCallback)(Platform &);
	void (*PreRenderAudioCallback)(Platform &);
	void (*RenderAudioCallback)(Platform &, SoundBuffer &soundBuffer);
	void (*CleanupCallback)(Platform &);
	bool (*WindowInitCallback)(Platform &);
	void (*WindowCleanupCallback)(Platform &);

	void *userData;

#if PLATFORM_ANDROID
	struct android_app *androidApp;
#endif // PLATFORM_ANDROID

	// Platform components

	Arena globalArena;
	Arena frameArena;
	Arena stringArena;
	Arena dataArena;

	Arena scratchArenas[MAX_SCRATCH_ARENAS];
	volatile_u32 scratchArenaLockMask;

	DynamicLibrary engineLib;

	StringInterning stringInterning;
	Window window;
	Input input;
	AudioDevice audio;
	f32 deltaSeconds;
	f32 totalSeconds;

	volatile_i64 eventTail;
	volatile_i64 eventHead;
	PlatformEvent events[128];

	bool mainThreadRunning;
	bool updateThreadRunning;
	bool windowInitialized;

	Semaphore updateThreadFinished;

	// Directories (set during pre-initialization, before any engine callbacks)
	const char *BinDir;
	const char *DataDir;
	const char *AssetDir;
	const char *ProjectDir;

	// API exposed to the engine
	PlatformAPI api;
};

#endif // PLATFORM_H



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform API

#if defined(PLATFORM_API)
#ifndef PLATAFORM_API_INCLUDED
#define PLATAFORM_API_INCLUDED

extern Platform *sPlatform;

#define GlobalArena sPlatform->globalArena
#define FrameArena sPlatform->frameArena
#define StringArena sPlatform->stringArena
#define DataArena sPlatform->dataArena

extern const char *BinDir;
extern const char *DataDir;
extern const char *AssetDir;
extern const char *ProjectDir;

extern PFN_PlatformQuit        PlatformQuit;
extern PFN_AcquireScratchArena AcquireScratchArena;
extern PFN_ReleaseScratchArena ReleaseScratchArena;

inline void SetPlatformAPI(Platform &platform)
{
	sPlatform = &platform;

	BinDir       = platform.BinDir;
	DataDir      = platform.DataDir;
	AssetDir     = platform.AssetDir;
	ProjectDir   = platform.ProjectDir;

	PlatformQuit        = platform.api.PlatformQuit;
	AcquireScratchArena = platform.api.AcquireScratchArena;
	ReleaseScratchArena = platform.api.ReleaseScratchArena;
}

struct Scratch
{
	Arena arena;
	u32 lockedBit;

	Scratch() { lockedBit = AcquireScratchArena(arena); }
	~Scratch() { ReleaseScratchArena(lockedBit); }
};

#endif //  PLATAFORM_API_INCLUDED
#endif // PLATFORM_API



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform API implementation

#if defined(PLATFORM_API_IMPLEMENTATION)
#ifndef PLATFORM_API_IMPLEMENTATION_INCLUDED
#define PLATFORM_API_IMPLEMENTATION_INCLUDED

Platform *sPlatform = nullptr;

const char *BinDir = "";
const char *DataDir = "";
const char *AssetDir = "";
const char *ProjectDir = "";

PFN_PlatformQuit        PlatformQuit        = nullptr;
PFN_AcquireScratchArena AcquireScratchArena = nullptr;
PFN_ReleaseScratchArena ReleaseScratchArena = nullptr;

#endif // PLATFORM_API_IMPLEMENTATION_INCLUDED

#endif // PLATFORM_API_IMPLEMENTATION

