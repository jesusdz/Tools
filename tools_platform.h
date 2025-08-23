#ifndef PLATFORM_H
#define PLATFORM_H

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

#if PLATFORM_WINDOWS
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

#if PLATFORM_LINUX
#	define USE_XCB 1
#elif PLATFORM_ANDROID
#	define USE_ANDROID 1
#elif PLATFORM_WINDOWS
#	define USE_WINAPI 1
#endif


#if USE_XCB
#	include <xcb/xcb.h>
#	include <stdlib.h> // free

void XcbReportError( int xcbErrorCode, const char *context )
{
	static const char *xcbErrorMessages[] = {
		"NO_ERROR",
		"XCB_CONN_ERROR",                   // 1
		"XCB_CONN_CLOSED_EXT_NOTSUPPORTED", // 2
		"XCB_CONN_CLOSED_MEM_INSUFFICIENT", // 3
		"XCB_CONN_CLOSED_REQ_LEN_EXCEED",   // 4
		"XCB_CONN_CLOSED_PARSE_ERR",        // 5
		"XCB_CONN_CLOSED_INVALID_SCREEN",   // 6
		"XCB_CONN_CLOSED_FDPASSING_FAILED", // 7
	};
	LOG(Error, "Xcb error (%s): %s\n", context, xcbErrorMessages[xcbErrorCode]);
}

void XcbReportGenericError( xcb_connection_t *conn, xcb_generic_error_t *err, const char *context )
{
	// TODO: Find a better way to report XCB generic errors
	LOG(Error, "Xcb generic error (%s)\n", context);
}

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

bool KeyPress(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESS;
}

bool KeyPressed(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_PRESSED;
}

bool KeyRelease(const Keyboard &keyboard, Key key)
{
	ASSERT(key < K_COUNT);
	return keyboard.keys[key] == KEY_STATE_RELEASE;
}

bool ButtonPress(ButtonState state)
{
	return state == BUTTON_STATE_PRESS;
}

bool ButtonPressed(ButtonState state)
{
	return state == BUTTON_STATE_PRESSED;
}

bool ButtonRelease(ButtonState state)
{
	return state == BUTTON_STATE_RELEASE;
}

bool MouseMoved(const Mouse &mouse)
{
	return mouse.dx != 0 || mouse.dy != 0;
}

bool MouseButtonPress(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPress(mouse.buttons[button]);
}

bool MouseButtonPressed(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonPressed(mouse.buttons[button]);
}

bool MouseButtonRelease(const Mouse &mouse, MouseButton button)
{
	ASSERT(button < MOUSE_BUTTON_COUNT);
	return ButtonRelease(mouse.buttons[button]);
}

bool MouseButtonChanged(const Mouse &mouse, MouseButton button)
{
	return MouseButtonPress(mouse, button) || MouseButtonRelease(mouse, button);
}

bool MouseChanged(const Mouse &mouse)
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

#if PLATFORM_WINDOWS
typedef HRESULT FP_DirectSoundCreate( LPGUID lpGuid, LPDIRECTSOUND* ppDS, LPUNKNOWN  pUnkOuter );
#endif

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

#define MAX_SCRATCH_ARENAS 8

struct Platform
{
	// To be configured by the client app

	u32 globalMemorySize = MB(64);
	u32 frameMemorySize = MB(16);
	u32 stringMemorySize = KB(16);
	u32 dataMemorySize = MB(16);

	bool (*InitCallback)(Platform &);
	void (*UpdateCallback)(Platform &);
	void (*RenderGraphicsCallback)(Platform &);
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
};

static Platform *sPlatform = nullptr;

u32 AcquireScratchArena(Arena &outArena)
{
	// Max MAX_SCRATCH_ARENAS attempts to get a scratch arena
	for (u32 i = 0; i < MAX_SCRATCH_ARENAS; ++i)
	{
		// Get the first bit not set
		const u32 oldValue = sPlatform->scratchArenaLockMask;
		const u32 index = FBZ(oldValue);
		ASSERT(index < MAX_SCRATCH_ARENAS);

		// Try to change it
		const u32 newValue = oldValue | (1<<index);
		if (AtomicSwap(&sPlatform->scratchArenaLockMask, oldValue, newValue))
		{
			Arena &arena = sPlatform->scratchArenas[index];
			if (!arena.base)
			{
				const u32 size = MB(1);
				arena.base = (byte*)AllocateVirtualMemory(size);
				arena.size = size;
			}
			outArena = arena;
			outArena.used = 0;
			return index;
		}
	}
	INVALID_CODE_PATH();
	return U32_MAX;
}

void ReleaseScratchArena(u32 index)
{
	const u32 oldValue = sPlatform->scratchArenaLockMask;
	const u32 newValue = oldValue & ~(1<<index);
	const bool swapped = AtomicSwap(&sPlatform->scratchArenaLockMask, oldValue, newValue);
	ASSERT(swapped);
}

struct Scratch
{
	Arena arena;
	u32 lockedBit;

	Scratch() { lockedBit = AcquireScratchArena(arena); }
	~Scratch() { ReleaseScratchArena(lockedBit); }
};


bool IsAbsolutePath(const char *path)
{
#if PLATFORM_LINUX || PLATFORM_ANDROID
	const bool res = *path == '/';
	return res;
#elif PLATFORM_WINDOWS
	const bool res = path[1] == ':' && path[0] >= 'A' && path[0] <= 'Z';
	return res;
#else
#error "Missing implementation"
#endif
}

void CanonicalizePath(char *path)
{
	struct PathPart
	{
		char *str;
		int len;
	};

	PathPart parts[32] = {};
	u32 partCount = 0;

	char *ptr = path;

	PathPart *currentPart = &parts[partCount++];
	currentPart->str = ptr;

	const bool addRootSeparator = (*ptr == '/');

	// Split path in parts
	// NOTE: This replaces separators by '\0' for easier string comparisons later
	while (*ptr) {
		if (*ptr == '/') {
			*ptr = 0;
			if (currentPart->len > 0) {
				ASSERT(partCount < ARRAY_COUNT(parts));
				currentPart = &parts[partCount++];
			}
			currentPart->str = ptr + 1;
		} else {
			currentPart->len++;
		}
		ptr++;
	}

	//LOG(Debug, "Directory parts:\n");
	//for (u32 i = 0; i < partCount; ++i) {
	//	LOG(Debug, "- %.*s\n", parts[i].len,  parts[i].str);
	//}

	// Canonicalize
	u32 finalParts[32] = {};
	u32 finalPartCount = 0;
	for (u32 partIndex = 0; partIndex < partCount; ++partIndex)
	{
		const PathPart &part = parts[partIndex];
		if ( StrEq(part.str, ".") ) {
			// Do nothing
		} else if ( StrEq(part.str, "..") ) {
			// Remove previous part
			ASSERT(finalPartCount > 0);
			finalPartCount--;
		} else if ( part.len > 0 ) {
			finalParts[finalPartCount++] = partIndex;
		}
	}

	// Copy string back to buffer
	ptr = path;
	for (u32 i = 0; i < finalPartCount; ++i)
	{
		const u32 partIndex = finalParts[i];
		const PathPart &part = parts[partIndex];
		if ( i > 0 || addRootSeparator ) {
			*ptr++ = '/';
		}
		for (u32 c = 0; c < part.len; ++c) {
			*ptr++ = part.str[c];
		}
	}
	*ptr = 0;
}

void InitializeDirectories(Platform &platform, int argc, char **argv)
{
	char buffer[MAX_PATH_LENGTH];

#if PLATFORM_ANDROID

	// TODO: Don't hardcode this path here and get it from Android API.
	DataDir = "/sdcard/Android/data/com.tools.game/files";
	BinDir = DataDir;
	ProjectDir = "";

#else

#if PLATFORM_LINUX
	char *workingDir = getcwd(buffer, ARRAY_COUNT(buffer));
#elif PLATFORM_WINDOWS
	char *workingDir = _getcwd(buffer, ARRAY_COUNT(buffer));
#else
#error "Missing implementation"
#endif

	StrReplace(workingDir, '\\', '/'); // Make all separators '/'

	char exeDir[MAX_PATH_LENGTH] = {};
	if (argc > 0)
	{
		StrReplace(argv[0], '\\', '/'); // Make all separators '/'
		const char *exePath = argv[0];
		const char *lastSeparator = StrCharR(exePath, '/');
		const u32 length = lastSeparator ? lastSeparator - exePath : 0;
		StrCopyN(exeDir, exePath, length);
	}

	char directory[MAX_PATH_LENGTH];
	if ( !IsAbsolutePath(exeDir) )
	{
		StrCopy(directory, workingDir);
		StrCat(directory, "/");
	}
	StrCat(directory, exeDir);
	CanonicalizePath(directory);

	DataDir = PushString(platform.stringArena, directory);
	BinDir = DataDir;

	StrCat(directory, "/..");
	CanonicalizePath(directory);
	ProjectDir = PushString(platform.stringArena, directory);

	StrCopy(directory, ProjectDir);
	StrCat(directory, "/assets");
	AssetDir = PushString(platform.stringArena, directory);

#endif

	LOG(Info, "Directories:\n");
	LOG(Info, "- BinDir: %s\n", BinDir);
	LOG(Info, "- DataDir: %s\n", DataDir);
	LOG(Info, "- AssetDir: %s\n", AssetDir);
	LOG(Info, "- ProjectDir: %s\n", ProjectDir);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Generic platform events

void SendPlatformEvent(Platform &platform, PlatformEvent event)
{
	ASSERT(platform.eventHead - platform.eventTail < ARRAY_COUNT(platform.events));
	
	const i32 eventIndex = platform.eventHead % ARRAY_COUNT(platform.events);
	platform.events[eventIndex] = event;

	AtomicIncrement(&platform.eventHead);
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Specific platform events

#if USE_XCB && 0
void PrintModifiers(uint32_t mask)
{
	const char **mod, *mods[] = {
		"Shift", "Lock", "Ctrl", "Alt",
		"Mod2", "Mod3", "Mod4", "Mod5",
		"Button1", "Button2", "Button3", "Button4", "Button5"
	};
	LOG(Info, "Modifier mask: ");
	for (mod = mods ; mask; mask >>= 1, mod++)
		if (mask & 1)
			LOG(Info, *mod);
	putchar ('\n');
}
#endif



#if USE_XCB

#include "xcb_key_mappings.h"

#if USE_IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API bool ImGui_ImplXcb_HandleInputEvent(xcb_generic_event_t *event);
#endif

void XcbWindowProc(Window &window, xcb_generic_event_t *event)
{
#if USE_IMGUI
	if (ImGui_ImplXcb_HandleInputEvent(event))
		return;
#endif

	u32 eventType = event->response_type & ~0x80;

	switch ( eventType )
	{
		case XCB_KEY_PRESS:
		case XCB_KEY_RELEASE:
			{
				// NOTE: xcb_key_release_event_t is an alias of xcb_key_press_event_t
				xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
				u32 keyCode = ev->detail;
				ASSERT( keyCode < ARRAY_COUNT(XcbKeyMappings) );
				u32 mapping = XcbKeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				KeyState state = eventType == XCB_KEY_PRESS ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
				window.keyboard.keys[ mapping ] = state;
				break;
			}

		case XCB_BUTTON_PRESS:
		case XCB_BUTTON_RELEASE:
			{
				// NOTE: xcb_button_release_event_t is an alias of xcb_button_press_event_t
				xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
				ButtonState state = eventType == XCB_BUTTON_PRESS ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
				switch (ev->detail) {
					case 1: window.mouse.buttons[ MOUSE_BUTTON_LEFT ] = state; break;
					case 2: window.mouse.buttons[ MOUSE_BUTTON_MIDDLE ] = state; break;
					case 3: window.mouse.buttons[ MOUSE_BUTTON_RIGHT ] = state; break;
					case 4: if (eventType == XCB_BUTTON_PRESS) { window.mouse.wy -= 1; } break;// // wheel up
					case 5: if (eventType == XCB_BUTTON_PRESS) { window.mouse.wy += 1; } break;// // wheel down
					case 6: if (eventType == XCB_BUTTON_PRESS) { window.mouse.wx -= 1; } break;// // wheel left
					case 7: if (eventType == XCB_BUTTON_PRESS) { window.mouse.wx += 1; } break;// // wheel right
					default:;
				}
				break;
			}

		case XCB_MOTION_NOTIFY:
			{
				xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;
				window.mouse.dx = static_cast<i32>(ev->event_x) - static_cast<i32>(window.mouse.x);
				window.mouse.dy = static_cast<i32>(ev->event_y) - static_cast<i32>(window.mouse.y);
				window.mouse.x = ev->event_x;
				window.mouse.y = ev->event_y;
				break;
			}

		case XCB_ENTER_NOTIFY:
			{
				xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;
				//LOG(Info, "Mouse entered window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_LEAVE_NOTIFY:
			{
				xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;
				//LOG(Info, "Mouse left window %ld, at coordinates (%d,%d)\n",
				//		ev->event, ev->event_x, ev->event_y);
				break;
			}

		case XCB_CONFIGURE_NOTIFY:
			{
				const xcb_configure_notify_event_t *ev = (const xcb_configure_notify_event_t *)event;
				if ( window.width != ev->width || window.height != ev->height )
				{
					window.width = ev->width;
					window.height = ev->height;
					window.flags |= WindowFlags_WasResized;
				}
				break;
			}

		case XCB_CLIENT_MESSAGE:
			{
				const xcb_client_message_event_t *ev = (const xcb_client_message_event_t *)event;
				if ( ev->data.data32[0] == window.closeAtom )
				{
					window.flags |= WindowFlags_WillDestroy;
					window.flags |= WindowFlags_Exit;
				}
				break;
			}

		case XCB_MAP_NOTIFY:
			// TODO: Handle this event
			break;

		case XCB_REPARENT_NOTIFY:
			// TODO: Handle this event
			break;

		default:
			/* Unknown event type, ignore it */
			LOG(Info, "Unknown window event: %d\n", event->response_type);
			break;
	}

#if USE_IMGUI
	const Mouse &mouse = window.mouse;

	ImGuiIO& io = ImGui::GetIO();
	if (MouseChanged(mouse))
	{
		if (MouseMoved(mouse))
		{
			io.AddMousePosEvent(mouse.x, mouse.y);
		}
		int button = -1;
		bool press = true;
		if (MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) { button = 0; press = true; }
		if (MouseButtonPress(mouse, MOUSE_BUTTON_RIGHT)) { button = 1; press = true; }
		if (MouseButtonPress(mouse, MOUSE_BUTTON_MIDDLE)) { button = 2; press = true; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_LEFT)) { button = 0; press = false; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_RIGHT)) { button = 1; press = false; }
		if (MouseButtonRelease(mouse, MOUSE_BUTTON_MIDDLE)) { button = 2; press = false; }
		if ( button != -1 )
		{
			io.AddMouseButtonEvent(button, press);
			io.AddMouseSourceEvent(ImGuiMouseSource_Mouse);
		}
	}

	// TODO: When we detect the mouse leves the window
	// io.AddMousePosEvent(-FLT_MAX, -FLT_MAX);

	// Clear mouse/keyboard events if handled by ImGui
	if ( io.WantCaptureMouse ) window.mouse = {};
	if ( io.WantCaptureKeyboard ) window.keyboard = {};
#endif // USE_IMGUI
}

#elif USE_ANDROID

/**
 * Process the next main command.
 * enum NativeAppGlueAppCmd {
 *   UNUSED_APP_CMD_INPUT_CHANGED = 0
 *   APP_CMD_INIT_WINDOW = 1
 *   APP_CMD_TERM_WINDOW = 2
 *   APP_CMD_WINDOW_RESIZED = 3
 *   APP_CMD_WINDOW_REDRAW_NEEDED = 4
 *   APP_CMD_CONTENT_RECT_CHANGED = 5
 *   APP_CMD_GAINED_FOCUS = 6
 *   APP_CMD_LOST_FOCUS = 7
 *   APP_CMD_CONFIG_CHANGED = 8
 *   APP_CMD_LOW_MEMORY = 9
 *   APP_CMD_START = 10
 *   APP_CMD_RESUME = 11
 *   APP_CMD_SAVE_STATE = 12
 *   APP_CMD_PAUSE = 13
 *   APP_CMD_STOP = 14
 *   APP_CMD_DESTROY = 15
 *   APP_CMD_WINDOW_INSETS_CHANGED = 16
 * }
 */
void AndroidHandleAppCommand(struct android_app *app, int32_t cmd)
{
	Platform *platform = (Platform*)app->userData;

	switch (cmd)
	{
		case APP_CMD_INIT_WINDOW:
			// The window is being shown, get it ready.
			ASSERT(app->window != NULL);
			if (app->window && app->window != platform->window.nativeWindow)
			{
				platform->window.nativeWindow = app->window;
				platform->window.flags |= WindowFlags_WasCreated;
			}
			break;
		case APP_CMD_TERM_WINDOW:
			// The window is being hidden or closed, clean it up.
			platform->window =  {};
			platform->window.flags |= WindowFlags_WillDestroy;
			break;
		case APP_CMD_WINDOW_RESIZED:
			{
				int32_t newWidth = ANativeWindow_getWidth(app->window);
				int32_t newHeight = ANativeWindow_getHeight(app->window);
				if ( newWidth != platform->window.width || newHeight != platform->window.height )
				{
					platform->window.width = newWidth;
					platform->window.height = newHeight;
					platform->window.flags |= WindowFlags_WasResized;
				}
			}
			break;
		//case APP_CMD_WINDOW_REDRAW_NEEDED: break;
		//case APP_CMD_CONTENT_RECT_CHANGED: break;
		//case APP_CMD_GAINED_FOCUS: break;
		//case APP_CMD_LOST_FOCUS: break;
		//case APP_CMD_CONFIG_CHANGED: break;
		//case APP_CMD_LOW_MEMORY: break;
		//case APP_CMD_START: break;
		//case APP_CMD_RESUME: break;
		//case APP_CMD_SAVE_STATE: break;
		case APP_CMD_PAUSE:
			{
				// Gets activated at APP_CMD_INIT_WINDOW
			}
			break;
		//case APP_CMD_STOP: break;
		//case APP_CMD_DESTROY: break;
		//case APP_CMD_WINDOW_INSETS_CHANGED: break;
		default:
			//LOG( Info, "UNKNOWN ANDROID COMMAND: %d\n", cmd);
			break;
	}
	//LOG( Info, "ANDROID APP COMMAND: %d\n", cmd);
}

int32_t AndroidHandleInputEvent(struct android_app *app, AInputEvent *event)
{
	Platform *platform = (Platform*)app->userData;

	if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		const int32_t actionAndPointer = AMotionEvent_getAction( event );
		const uint32_t action = actionAndPointer & AMOTION_EVENT_ACTION_MASK;
		const uint32_t pointerIndex = (actionAndPointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
		const uint32_t pointerCount = AMotionEvent_getPointerCount(event);
		const float x = AMotionEvent_getX(event, pointerIndex);
		const float y = AMotionEvent_getY(event, pointerIndex);

		if (pointerId < ARRAY_COUNT(platform->window.touches))
		{
			Touch *touches = platform->window.touches;

			switch( action )
			{
				case AMOTION_EVENT_ACTION_DOWN:
				case AMOTION_EVENT_ACTION_POINTER_DOWN:
					{
						touches[pointerId].state = TOUCH_STATE_PRESS;
						touches[pointerId].x0 = x;
						touches[pointerId].y0 = y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_UP:
				case AMOTION_EVENT_ACTION_POINTER_UP:
					{
						touches[pointerId].state = TOUCH_STATE_RELEASE;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
				case AMOTION_EVENT_ACTION_MOVE:
					// On move ements, we are meant to handle all pointers in the gesture
					for (u32 pointerIndex = 0; pointerIndex < pointerCount; ++pointerIndex)
					{
						const float x = AMotionEvent_getX(event, pointerIndex);
						const float y = AMotionEvent_getY(event, pointerIndex);
						const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
						touches[pointerId].dx = x - touches[pointerId].x;
						touches[pointerId].dy = y - touches[pointerId].y;
						touches[pointerId].x = x;
						touches[pointerId].y = y;
					}
					break;
			}
		}
		return 1;
	}
	return 0;
}

#elif USE_WINAPI

#include "win32_key_mappings.h"

#if USE_IMGUI
#define IMGUI_DEFINE_MATH_OPERATORS
#include "imgui/imgui.h"
// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
#endif

LRESULT CALLBACK Win32WindowProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	ASSERT(sPlatform);
	Platform &platform = *sPlatform;

#if !USE_UPDATE_THREAD
	Window *window = &platform.window;
#endif

	bool processMouseEvents = true;
	bool processKeyboardEvents = true;
#if USE_IMGUI
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}
	if (ImGui::GetCurrentContext() != nullptr)
	{
		const ImGuiIO& io = ImGui::GetIO();
		processKeyboardEvents = !io.WantCaptureKeyboard;
		processMouseEvents = !io.WantCaptureMouse;
	}
#endif

	switch (uMsg)
	{
		case WM_KEYDOWN:
		case WM_KEYUP:
			if ( processKeyboardEvents )
			{
				WPARAM keyCode = wParam;
				ASSERT( keyCode < ARRAY_COUNT(Win32KeyMappings) );
				const Key mapping = Win32KeyMappings[ keyCode ];
				ASSERT( mapping < K_COUNT );
				const KeyState state = uMsg == WM_KEYDOWN ? KEY_STATE_PRESS : KEY_STATE_RELEASE;
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeKeyPress,
					.keyPress = {
						.code = mapping,
						.state = state,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->keyboard.keys[ mapping ] = state;
#endif
			}
			break;

		case WM_SYSCHAR:
			// If this message is not handled the default window procedure will
			// play a system notification sound when Alt+Enter is pressed.
			break;

		case WM_LBUTTONDOWN:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_LEFT,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
#else
				//int xPos = GET_X_LPARAM(lParam);
				//int yPos = GET_Y_LPARAM(lParam);
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_LEFT] = BUTTON_STATE_PRESS;
#endif
			}
			break;
		case WM_LBUTTONUP:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_LEFT,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_LEFT] = BUTTON_STATE_RELEASE;
#endif
			}
			break;
		case WM_RBUTTONDOWN:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_RIGHT,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_RIGHT] = BUTTON_STATE_PRESS;
#endif
			}
			break;
		case WM_RBUTTONUP:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_RIGHT,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_RIGHT] = BUTTON_STATE_RELEASE;
#endif
			}
			break;
		case WM_MBUTTONDOWN:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_MIDDLE,
						.state = BUTTON_STATE_PRESS,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_MIDDLE] = BUTTON_STATE_PRESS;
#endif
			}
			break;
		case WM_MBUTTONUP:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseClick,
					.mouseClick = {
						.button = MOUSE_BUTTON_MIDDLE,
						.state = BUTTON_STATE_RELEASE,
					},
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.buttons[MOUSE_BUTTON_MIDDLE] = BUTTON_STATE_RELEASE;
#endif
			}
			break;

		case WM_MOUSEWHEEL:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseWheel,
					.mouseWheel = { .dy = -GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA },
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.wy -= GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
#endif
			}
			break;

		case WM_MOUSEHWHEEL:
			if ( processMouseEvents )
			{
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseWheel,
					.mouseWheel = { .dx = GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA },
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.wx += GET_WHEEL_DELTA_WPARAM(wParam)/WHEEL_DELTA;
#endif
			}
			break;

		case WM_MOUSEMOVE:
			if ( processMouseEvents )
			{
				i16 xPos = GET_X_LPARAM(lParam);
				i16 yPos = GET_Y_LPARAM(lParam);
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeMouseMove,
					.mouseMove = { .x = xPos, .y = yPos }
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				window->mouse.dx = xPos - window->mouse.x;
				window->mouse.dy = yPos - window->mouse.y;
				window->mouse.x = xPos;
				window->mouse.y = yPos;
#endif
				//LOG( Info, "Mouse at position (%d, %d)\n", xPos, yPos );
			}
			break;

		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
			{
				// These events are disabled by default. See documentation if needed:
				// https://learn.microsoft.com/en-us/windows/win32/learnwin32/other-mouse-operations
				//LOG( Info, "Mouse %s the window\n", uMsg == WM_MOUSEHOVER ? "entered" : "left" );
				break;
			}

		case WM_SIZE:
			{
				const u16 width = LOWORD(lParam);
				const u16 height = HIWORD(lParam);
#if USE_UPDATE_THREAD
				const PlatformEvent event = {
					.type = PlatformEventTypeWindowResize,
					.windowResize = { .width = width, .height = height },
				};
				SendPlatformEvent(platform, event);
#else
				ASSERT(window);
				if ( window->width != width || window->height != height )
				{
					window->width = Max(width, 0);
					window->height = Max(height, 0);
					window->flags |= WindowFlags_WasResized;
				}
#endif
				break;
			}

		case WM_SYSCOMMAND:
			{
				WPARAM param = ( wParam & 0xFFF0 );
				AudioDevice &audio = sPlatform->audio;

				if (param == SC_MINIMIZE)
				{
					if ( audio.initialized && audio.isPlaying ) {
						sPlatform->audio.buffer->Stop();
						audio.isPlaying = false;
					}
				}
				else if (param == SC_RESTORE)
				{
					if ( audio.initialized && !audio.isPlaying ) {
						sPlatform->audio.buffer->Play(0, 0, DSBPLAY_LOOPING);
						audio.isPlaying = true;
						sPlatform->audio.soundIsValid = false;
					}
				}

				return DefWindowProc(hWnd, uMsg, wParam, lParam);
			};

		case WM_CLOSE:
			{
				// If we want to show a dialog to ask the user for confirmation before
				// closing the window, it should be done here. Zero should be returned
				// to indicate that we handled this message.
				// Otherwise, calling DefWindowProc will internally call DestroyWindow
				// and will internally send the WM_DESTROY message.
#if USE_UPDATE_THREAD
				const PlatformEvent event = { .type = PlatformEventTypeWindowWillDestroy, };
				SendPlatformEvent(platform, event);
#endif
				DestroyWindow(hWnd);
				break;
			}

		case WM_DESTROY:
			{
				// This inserts a WM_QUIT message in the queue, which will in turn cause
				// GetMessage to return zero. We will exit the main loop when that happens.
				// On the other hand, PeekMessage has to handle WM_QUIT messages explicitly.
				#if USE_UPDATE_THREAD
				const PlatformEvent event = { .type = PlatformEventTypeQuit, };
				SendPlatformEvent(platform, event);
				#else
				window->flags |= WindowFlags_WillDestroy;
				#endif
				PostQuitMessage(0);
				break;
			}

		default:
			return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	return 0;
}

#endif



bool InitializeWindow(
		Window &window,
		u32 width = 640,
		u32 height = 480,
		const char *title = "Example window"
		)
{
	ZeroStruct(&window);
	window.width = width;
	window.height = height;

#if USE_XCB

	// Connect to the X server
	xcb_connection_t *xcbConnection = xcb_connect(NULL, NULL);

	int xcbConnError = xcb_connection_has_error(xcbConnection);
	if ( xcbConnError > 0 )
	{
		XcbReportError(xcbConnError, "xcb_connect");
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Get the first screen
	const xcb_setup_t *setup = xcb_get_setup(xcbConnection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen = iter.data;

	// Configure events to capture
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {
		screen->black_pixel,
		XCB_EVENT_MASK_KEY_PRESS       | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS    | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION  |
		XCB_EVENT_MASK_ENTER_WINDOW    | XCB_EVENT_MASK_LEAVE_WINDOW   |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
	};

	// Create a window
	xcb_window_t xcbWindow = xcb_generate_id(xcbConnection);
	xcb_void_cookie_t createWindowCookie = xcb_create_window_checked(
		xcbConnection,                 // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		xcbWindow,                     // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		width, height,                 // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	xcb_generic_error_t *createWindowError = xcb_request_check(xcbConnection, createWindowCookie);
	if ( createWindowError )
	{
		XcbReportGenericError(xcbConnection, createWindowError, "xcb_create_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Handle close event
	// TODO: Handle xcb_intern_atom errors
	xcb_intern_atom_cookie_t protocolCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 1, 12, "WM_PROTOCOLS");
	xcb_intern_atom_reply_t *protocolReply =
		xcb_intern_atom_reply( xcbConnection, protocolCookie, 0);
	xcb_intern_atom_cookie_t closeCookie = // handle error
		xcb_intern_atom_unchecked( xcbConnection, 0, 16, "WM_DELETE_WINDOW");
	xcb_intern_atom_reply_t *closeReply =
		xcb_intern_atom_reply( xcbConnection, closeCookie, 0);
	u8 dataFormat = 32;
	u32 dataLength = 1;
	void *data = &closeReply->atom;
	xcb_change_property( // handle error
			xcbConnection,
			XCB_PROP_MODE_REPLACE,
			xcbWindow,
			protocolReply->atom, XCB_ATOM_ATOM,
			dataFormat, dataLength, data);
	xcb_atom_t closeAtom = closeReply->atom;
	free(protocolReply);
	free(closeReply);

	// Map the window to the screen
	xcb_void_cookie_t mapWindowCookie = xcb_map_window_checked(xcbConnection, xcbWindow);
	xcb_generic_error_t *mapWindowError = xcb_request_check(xcbConnection, mapWindowCookie);
	if ( mapWindowError )
	{
		XcbReportGenericError(xcbConnection, mapWindowError, "xcb_map_window_checked");
		xcb_destroy_window(xcbConnection, xcbWindow);
		xcb_disconnect(xcbConnection);
		return false;
	}

	// Flush the commands before continuing
	xcb_flush(xcbConnection);

	// Get window info at this point
	xcb_get_geometry_cookie_t cookie= xcb_get_geometry( xcbConnection, xcbWindow ); // handle error
	xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply( xcbConnection, cookie, NULL ); // handle error

	window.connection = xcbConnection;
	window.window = xcbWindow;
	window.closeAtom = closeAtom;
	window.width = reply->width;
	window.height = reply->height;

	window.flags = WindowFlags_WasCreated;

#endif

#if USE_WINAPI

	// Register the window class.
	const char CLASS_NAME[]  = "Sample Window Class";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = {};
	wc.style         = CS_HREDRAW | CS_VREDRAW;
	wc.lpfnWndProc   = Win32WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.hCursor       = LoadCursor(NULL, IDC_ARROW);
	ATOM classAtom = RegisterClassA(&wc);

	if (classAtom == 0)
	{
		Win32ReportError("InitializeWindow - RegisterClassA");
		return false;
	}

	// Given the desired client window size, get the full size
	RECT windowRect = { 0, 0, (int)width, (int)height };
	AdjustWindowRect( &windowRect, WS_OVERLAPPEDWINDOW, FALSE );
	int fullWidth = windowRect.right - windowRect.left;
	int fullHeight = windowRect.bottom - windowRect.top;

	// Ignore display scaling
	SetProcessDPIAware();

	HWND hWnd = CreateWindowExA(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			title,                          // Window text
			WS_OVERLAPPEDWINDOW,            // Window style
			CW_USEDEFAULT, CW_USEDEFAULT,   // Position
			fullWidth, fullHeight,          // Size
			NULL,                           // Parent window
			NULL,                           // Menu
			hInstance,                      // Instance handle
			NULL                            // Additional application data
			);

	if (hWnd == NULL)
	{
		Win32ReportError("InitializeWindow - CreateWindowExA");
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);

	window.hInstance = hInstance;
	window.hWnd = hWnd;

	window.flags = WindowFlags_WasCreated;

#endif

	return true;
}


void CleanupWindow(Window &window)
{
#if USE_XCB
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#elif USE_WINAPI
	DestroyWindow(window.hWnd);
#endif
}


void TransitionInputStatesSinceLastFrame(Window &window)
{
	// Transition key states
	for ( u32 i = 0; i < K_COUNT; ++i ) {
		if ( window.keyboard.keys[i] == KEY_STATE_PRESS ) {
			window.keyboard.keys[i] = KEY_STATE_PRESSED;
		} else if ( window.keyboard.keys[i] == KEY_STATE_RELEASE ) {
			window.keyboard.keys[i] = KEY_STATE_IDLE;
		}
	}

	// Transition mouse button states
	for ( u32 i = 0; i < MOUSE_BUTTON_COUNT; ++i ) {
		if ( window.mouse.buttons[i] == BUTTON_STATE_PRESS ) {
			window.mouse.buttons[i] = BUTTON_STATE_PRESSED;
		} else if ( window.mouse.buttons[i] == BUTTON_STATE_RELEASE ) {
			window.mouse.buttons[i] = BUTTON_STATE_IDLE;
		}
	}

	window.mouse.dx = 0;
	window.mouse.dy = 0;
	window.mouse.wx = 0;
	window.mouse.wy = 0;

	window.chars.charCount = 0;

	// Transition touch states
	for ( u32 i = 0; i < ARRAY_COUNT(window.touches); ++i ) {
		if ( window.touches[i].state == TOUCH_STATE_PRESS ) {
			window.touches[i].state = TOUCH_STATE_PRESSED;
		} else if ( window.touches[i].state == TOUCH_STATE_RELEASE ) {
			window.touches[i].state = TOUCH_STATE_IDLE;
		}
		window.touches[i].dx = 0.0f;
		window.touches[i].dy = 0.0f;
	}
}


void UpdateKeyModifiers(Window &window)
{
	// Update key modifiers
	window.chars.shift = KeyPressed(window.keyboard, K_SHIFT);
	window.chars.ctrl = KeyPressed(window.keyboard, K_CONTROL);
	window.chars.alt = KeyPressed(window.keyboard, K_ALT);

	for ( u32 i = 0; i < K_COUNT; ++i )
	{
		if ( KeyPress(window.keyboard, (Key)i) )
		{
			char character = 0;

			if ( i >= K_A && i <= K_Z ) {
				character = window.chars.shift ? 'A' + (i - K_A) : 'a' + (i - K_A);
			} else if ( i >= K_0 && i <= K_9 ) {
				character = '0' + (i - K_0);
			} else if ( i == K_SPACE ) {
				character = ' ';
			}

			if (character)
			{
				window.chars.chars[window.chars.charCount++] = character;
			}
		}
	}
}


void PlatformUpdateEventLoop(Platform &platform)
{
	Window &window = platform.window;

#if !USE_UPDATE_THREAD
	TransitionInputStatesSinceLastFrame(window);
#endif

#if USE_XCB

	xcb_generic_event_t *event;
	while ( (event = xcb_poll_for_event(window.connection)) != 0 )
	{
		XcbWindowProc(window, event);
		free(event);
	}

#elif USE_ANDROID

	// Read all pending events.
	int ident;
	int events;
	struct android_poll_source* source;

	const int kWaitForever = -1;
	const int kDontWait = 0;
	while ((ident=ALooper_pollAll(kDontWait, NULL, &events, (void**)&source)) >= 0)
	{
		// Process this event.
		if (source != NULL)
		{
			source->process(platform.androidApp, source);
		}

		// Check if we are exiting.
		if (platform.androidApp->destroyRequested != 0)
		{
			LOG(Info, "androidApp->destroyRequesteds\n");
			window.flags |= WindowFlags_Exit;
		}
	}

#elif USE_WINAPI

	BOOL ret = 1;
	MSG msg = { };
#if USE_UPDATE_THREAD
	while ( (ret = GetMessage( &msg, NULL, 0, 0 )) != 0 ) // ret == 0 means WM_QUIT
#else
	while ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
#endif
	{
		if ( ret == -1 )
		{
			LOG(Error, "Fatal error returned by GetMessage\n");
			break;
		}
		else if ( LOWORD( msg.message ) == WM_QUIT )
		{
			window.flags |= WindowFlags_Exit;
			break;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#if USE_UPDATE_THREAD
	platform.mainThreadRunning = false;
#endif

#endif

#if !USE_UPDATE_THREAD
	UpdateKeyModifiers(window);
#endif
}

#if PLATFORM_WINDOWS

#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_STATE *state)
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD userIndex, XINPUT_VIBRATION *vibration)

typedef X_INPUT_GET_STATE(XInputGetState_t);
typedef X_INPUT_SET_STATE(XInputSetState_t);

X_INPUT_GET_STATE(XInputGetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

X_INPUT_SET_STATE(XInputSetStateStub)
{
	return ERROR_INVALID_FUNCTION;
}

XInputGetState_t *FP_XInputGetState = XInputGetStateStub;
XInputSetState_t *FP_XInputSetState = XInputSetStateStub;

#endif // PLATFORM_WINDOWS

bool InitializeGamepad(Platform &platform)
{
	LOG(Info, "Input system initialization:\n");

	Input &input = platform.input;

#if PLATFORM_WINDOWS

	const char *libraryNames[] = {
		"xinput1_4.dll",
		"xinput9_1_0.dll",
		"xinput1_3.dll",
	};

	const char *libraryName = nullptr;
	for (u32 i = 0; i < ARRAY_COUNT(libraryNames); ++i) {
		libraryName = libraryNames[i];
		input.library = OpenLibrary(libraryName);
		if (input.library) {
			break;
		}
	}

	if (input.library)
	{
		LOG(Info, "- Loaded %s successfully\n", libraryName);

		XInputGetState_t* getState = (XInputGetState_t*)LoadSymbol(input.library, "XInputGetState");
		XInputSetState_t* setState = (XInputSetState_t*)LoadSymbol(input.library, "XInputSetState");

		if ( getState != nullptr ) {
			LOG(Info, "- XInputGetState symbol loaded successfully\n");
			FP_XInputGetState = getState;
		} else {
			LOG(Warning, "- Error loading XInputGetState symbol\n");
			FP_XInputGetState = XInputGetStateStub;
		}
		if ( setState != nullptr ) {
			LOG(Info, "- XInputSetState symbol loaded successfully\n");
			FP_XInputSetState = setState;
		} else {
			LOG(Warning, "- Error loading XInputSetState\n");
			FP_XInputSetState = XInputSetStateStub;
		}
	}

#elif PLATFORM_LINUX

	input.fd = -1;

	char path[MAX_PATH_LENGTH] = {};
	char name[MAX_PATH_LENGTH] = {};
	const char *dirName = "/dev/input";
	bool found = false;

	Dir dir = {};

	if ( OpenDir(dir, dirName) )
	{
		DirEntry entry = {};

		while ( !found && ReadDir(dir, entry) )
		{
			SPrintf(path, "%s/%s", dirName, entry.name);

			if (StrEqN(entry.name, "event", 5))
			{
				int fd = open(path, O_RDONLY | O_NONBLOCK);
				if (fd < 0) {
					continue;
				}

				if ( ioctl(fd, EVIOCGNAME(sizeof(name)), name) != -1 )
				{
					LOG(Info, "- Device path: %s\n", path);
					LOG(Info, "- Device name: %s\n", name);
					input.fd = fd;
					found = true;
				}
				else
				{
					close(fd);
				}
			}
		}

		CloseDir(dir);
	}

	return found;

#else

	LOG(Info, "- Missing implementation\n");

#endif

	return false;
}

#if PLATFORM_WINDOWS

static ButtonState ButtonStateFromXInput(WORD prevButtonMask, WORD currButtonMask, WORD buttonBit)
{
	const u32 wasDown = ( prevButtonMask & buttonBit ) ? 1 : 0;
	const u32 isDown = ( currButtonMask & buttonBit ) ? 1 : 0;
	constexpr ButtonState stateMatrix[2][2] = {
		// isUp,  isDown
		{ BUTTON_STATE_IDLE , BUTTON_STATE_PRESS }, // wasUp
		{ BUTTON_STATE_RELEASE ,BUTTON_STATE_PRESSED }, // wasDown
	};
	const ButtonState state = stateMatrix[wasDown][isDown];
	return state;
}

static f32 TriggerFromXInput(BYTE trigger)
{
	const f32 res = (f32)trigger/255.0f;
	return res;
}

static f32 AxisFromXInput(SHORT axis, SHORT deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

#endif // PLATFORM_WINDOWS

#if PLATFORM_LINUX
static ButtonState ButtonStateFromEvent(i32 value)
{
	const ButtonState res = (value == 1) ? BUTTON_STATE_PRESS : BUTTON_STATE_RELEASE;
	return res;
}

#define GAMEPAD_LEFT_THUMB_DEADZONE  7849
#define GAMEPAD_RIGHT_THUMB_DEADZONE 8689

static f32 AxisFromEvent(i32 axis, i32 deadzoneThreshold)
{
	f32 normalizedAxis = 0.0f;
	if (axis < -deadzoneThreshold) {
		normalizedAxis = (f32)(axis + deadzoneThreshold)/(32768.0f - deadzoneThreshold);
	} else if (axis > deadzoneThreshold) {
		normalizedAxis = (f32)(axis - deadzoneThreshold)/(32767.0f - deadzoneThreshold);
	}
	return normalizedAxis;
}

static f32 TriggerFromEvent(i32 value)
{
	//ASSERT(value >= 0 && value < 256);
	static f32 maxValue = 256.0f;
	maxValue = Max(maxValue, (f32)value); // TODO(jesus): Query per-device limits with ioctl
	const f32 res = (f32)value/maxValue;
	return res;
}

static ButtonState DPadStateFromEvent(ButtonState prevState, i32 expectedValue, i32 value)
{
	ButtonState state = prevState;
	if (expectedValue == value) {
		state = BUTTON_STATE_PRESS;
	} else if (prevState != BUTTON_STATE_IDLE) {
		state = BUTTON_STATE_RELEASE;
	}
	return state;
}
#endif // PLATFORM_LINUX

void UpdateGamepad(Platform &platform)
{
	Gamepad &gamepad = platform.input.gamepad;

#if PLATFORM_WINDOWS

	for ( DWORD i = 0; i < XUSER_MAX_COUNT; ++i )
	{
		XINPUT_STATE controllerState;
		if (FP_XInputGetState(i, &controllerState) == ERROR_SUCCESS)
		{
			static XINPUT_GAMEPAD prevXPad = {};

			//int packetNumber = controllerState.dwPacketNumber;
			const XINPUT_GAMEPAD &xpad = controllerState.Gamepad;

			gamepad.start = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_START);
			gamepad.back = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_BACK);
			gamepad.up = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_UP);
			gamepad.down = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_DOWN);
			gamepad.left = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_LEFT);
			gamepad.right = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_DPAD_RIGHT);
			gamepad.a = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_A);
			gamepad.b = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_B);
			gamepad.x = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_X);
			gamepad.y = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_Y);
			gamepad.leftShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_LEFT_SHOULDER);
			gamepad.rightShoulder = ButtonStateFromXInput(prevXPad.wButtons, xpad.wButtons, XINPUT_GAMEPAD_RIGHT_SHOULDER);
			gamepad.leftTrigger = TriggerFromXInput(xpad.bLeftTrigger);
			gamepad.rightTrigger = TriggerFromXInput(xpad.bRightTrigger);
			gamepad.leftAxis.x = AxisFromXInput(xpad.sThumbLX, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.leftAxis.y = AxisFromXInput(xpad.sThumbLY, XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE);
			gamepad.rightAxis.x = AxisFromXInput(xpad.sThumbRX, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);
			gamepad.rightAxis.y = AxisFromXInput(xpad.sThumbRY, XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE);

			prevXPad = xpad;

			// Only one gamepad supported
			break;
		}
		else
		{
			// Controller not available
		}
	}

#elif PLATFORM_LINUX

	if (platform.input.fd != -1)
	{
		// Update button states
		for (u32 i = 0; i < ARRAY_COUNT(gamepad.buttons); ++i) {
			if (gamepad.buttons[i] == BUTTON_STATE_PRESS) {
				gamepad.buttons[i] = BUTTON_STATE_PRESSED;
			} else if (gamepad.buttons[i] == BUTTON_STATE_RELEASE) {
			}
		}

		ssize_t size = 0;
		input_event event;

		while ( (size = read(platform.input.fd, &event, sizeof(event))) != -1 )
		{
			const u32 type = event.type;
			const u32 code = event.code;
			const i32 value = event.value;

			if (type == EV_KEY) {
				switch (code) {
					case BTN_START: gamepad.start = ButtonStateFromEvent(value); break;
					case BTN_SELECT: gamepad.back = ButtonStateFromEvent(value); break;
					//case BTN_MODE: codeStr = ButtonStateFromEvent(value); break;
					case BTN_TL: gamepad.leftShoulder = ButtonStateFromEvent(value); break;
					case BTN_TR: gamepad.rightShoulder = ButtonStateFromEvent(value); break;
					case BTN_A: gamepad.a = ButtonStateFromEvent(value); break;
					case BTN_B: gamepad.b = ButtonStateFromEvent(value); break;
					case BTN_X: gamepad.x = ButtonStateFromEvent(value); break;
					case BTN_Y: gamepad.y = ButtonStateFromEvent(value); break;
					//case BTN_THUMBL: codeStr = ButtonStateFromEvent(gamepad., value); break;
					//case BTN_THUMBR: codeStr = ButtonStateFromEvent(gamepad., value); break;
					default:;
				}
			} else if (type == EV_ABS) {
				switch (code) {
					case ABS_X: gamepad.leftAxis.x = AxisFromEvent(value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_Y: gamepad.leftAxis.y = AxisFromEvent(-value, GAMEPAD_LEFT_THUMB_DEADZONE); break;
					case ABS_RX: gamepad.rightAxis.x = AxisFromEvent(value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_RY: gamepad.rightAxis.y = AxisFromEvent(-value, GAMEPAD_RIGHT_THUMB_DEADZONE); break;
					case ABS_Z: gamepad.leftTrigger = TriggerFromEvent(value); break;
					case ABS_RZ: gamepad.rightTrigger = TriggerFromEvent(value); break;
					case ABS_HAT0X:
						gamepad.left = DPadStateFromEvent(gamepad.left, -1, value);
						gamepad.right = DPadStateFromEvent(gamepad.right, 1, value);
						break;
					case ABS_HAT0Y:
						gamepad.up = DPadStateFromEvent(gamepad.up, -1, value);
						gamepad.down = DPadStateFromEvent(gamepad.down, 1, value);
						break;
				}
			} else if (type == EV_MSC) {
				// MSC event (not sure what this is)
			} else if (type == EV_SYN) {
				// Synchronization event (not sure what this is)
			} else {
				LOG(Warning, "- Unknown event type\n");
			}
		}

		if (errno != EAGAIN)
		{
			LOG(Warning, "Error reading gamepad input\n");
		}
	}

#endif
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Audio

#if PLATFORM_WINDOWS

void Win32FillAudioBuffer(AudioDevice &audio, DWORD writeOffset, DWORD writeSize, const i16 *audioSamples);

#elif PLATFORM_LINUX

typedef const char * SND_STRERROR (int errnum);
typedef int SND_PCM_OPEN(snd_pcm_t **pcmp, const char * name, snd_pcm_stream_t stream, int	mode );
typedef int SND_PCM_HW_PARAMS_MALLOC(snd_pcm_hw_params_t **ptr);
typedef int SND_PCM_HW_PARAMS_ANY(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_SET_ACCESS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_access_t _access);
typedef int SND_PCM_HW_PARAMS_SET_FORMAT(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_format_t val);
typedef int SND_PCM_HW_PARAMS_SET_CHANNELS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int val);
typedef int SND_PCM_HW_PARAMS_SET_RATE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR(snd_pcm_t *pcm, snd_pcm_hw_params_t *params, snd_pcm_uframes_t *val, int *dir);
typedef int SND_PCM_HW_PARAMS(snd_pcm_t *pcm, snd_pcm_hw_params_t *params);
typedef int SND_PCM_HW_PARAMS_GET_CHANNELS(const snd_pcm_hw_params_t *params, unsigned int *channelCount);
typedef int SND_PCM_HW_PARAMS_GET_RATE(const snd_pcm_hw_params_t *params, unsigned int *sampleRate, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_FORMAT(const snd_pcm_hw_params_t *params, snd_pcm_format_t *format);
typedef int SND_PCM_HW_PARAMS_GET_ACCESS(const snd_pcm_hw_params_t *params, snd_pcm_access_t *access);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_TIME(const snd_pcm_hw_params_t *params, unsigned int *val, int *dir);
typedef int SND_PCM_HW_PARAMS_GET_PERIOD_SIZE(const snd_pcm_hw_params_t *params, snd_pcm_uframes_t *frames, int *dir);
typedef int SND_PCM_AVAIL_DELAY(snd_pcm_t *pcm, snd_pcm_sframes_t *availp, snd_pcm_sframes_t *delayp);
typedef snd_pcm_sframes_t SND_PCM_WRITEI(snd_pcm_t *pcm, const void *buffer, snd_pcm_uframes_t size);
typedef int SND_PCM_RECOVER(snd_pcm_t *pcm, int err, int silent);
typedef int SND_PCM_PREPARE(snd_pcm_t *pcm);
typedef int SND_PCM_CLOSE(snd_pcm_t *pcm);
typedef int SND_PCM_DRAIN(snd_pcm_t *pcm);

SND_STRERROR* FP_snd_strerror;
SND_PCM_OPEN* FP_snd_pcm_open;
SND_PCM_HW_PARAMS_MALLOC* FP_snd_pcm_hw_params_malloc;
SND_PCM_HW_PARAMS_ANY* FP_snd_pcm_hw_params_any;
SND_PCM_HW_PARAMS_SET_ACCESS* FP_snd_pcm_hw_params_set_access;
SND_PCM_HW_PARAMS_SET_FORMAT* FP_snd_pcm_hw_params_set_format;
SND_PCM_HW_PARAMS_SET_CHANNELS* FP_snd_pcm_hw_params_set_channels;
SND_PCM_HW_PARAMS_SET_RATE_NEAR* FP_snd_pcm_hw_params_set_rate_near;
SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR* FP_snd_pcm_hw_params_set_period_size_near;
SND_PCM_HW_PARAMS* FP_snd_pcm_hw_params;
SND_PCM_HW_PARAMS_GET_CHANNELS* FP_snd_pcm_hw_params_get_channels;
SND_PCM_HW_PARAMS_GET_RATE* FP_snd_pcm_hw_params_get_rate;
SND_PCM_HW_PARAMS_GET_FORMAT* FP_snd_pcm_hw_params_get_format;
SND_PCM_HW_PARAMS_GET_ACCESS* FP_snd_pcm_hw_params_get_access;
SND_PCM_HW_PARAMS_GET_PERIOD_TIME* FP_snd_pcm_hw_params_get_period_time;
SND_PCM_HW_PARAMS_GET_PERIOD_SIZE* FP_snd_pcm_hw_params_get_period_size;
SND_PCM_AVAIL_DELAY* FP_snd_pcm_avail_delay;
SND_PCM_WRITEI* FP_snd_pcm_writei;
SND_PCM_RECOVER* FP_snd_pcm_recover;
SND_PCM_PREPARE* FP_snd_pcm_prepare;
SND_PCM_CLOSE* FP_snd_pcm_close;
SND_PCM_DRAIN* FP_snd_pcm_drain;

#elif PLATFORM_ANDROID

aaudio_data_callback_result_t AAudioFillAudioBuffer(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames);

#endif // PLATFORM_LINUX

#if PLATFORM_WINDOWS
void Win32FillAudioBuffer(AudioDevice &audio, DWORD writeOffset, DWORD writeSize, const i16 *audioSamples)
{
	ASSERT(writeSize <= audio.bufferSize);

	void *region1;
	DWORD region1Size;
	void *region2;
	DWORD region2Size;

	if (audio.buffer->Lock(writeOffset, writeSize, &region1, &region1Size, &region2, &region2Size, 0) == DS_OK)
	{
		const i16 *srcSample = audioSamples;

		i16 *dstSample = (i16*)region1;
		const u32 region1SampleCount = region1Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region1SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		dstSample = (i16*)region2;
		const u32 region2SampleCount = region2Size / (audio.bytesPerSample * audio.channelCount);
		for (u32 i = 0; i < region2SampleCount; ++i)
		{
			*dstSample++ = *srcSample++;
			*dstSample++ = *srcSample++;
			audio.runningSampleIndex++;
		}

		audio.buffer->Unlock(region1, region1Size, region2, region2Size);
	}
	else
	{
		LOG(Warning, "Failed to Lock sound buffer.\n");
		audio.soundIsValid = false;
	}
}
#elif PLATFORM_ANDROID
aaudio_data_callback_result_t AAudioFillAudioBuffer(AAudioStream *stream, void *userData, void *audioData, int32_t numFrames)
{
	Platform &platform = *(Platform*)userData;
	AudioDevice &audio = platform.audio;

	SoundBuffer soundBuffer = {};
	soundBuffer.samplesPerSecond = audio.samplesPerSecond;
	soundBuffer.sampleCount = numFrames;
	soundBuffer.samples = (i16*)audioData;
	platform.RenderAudioCallback(platform, soundBuffer);

	return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#endif // PLATFORM_WINDOWS

void UpdateAudio(Platform &platform, float secondsSinceFrameBegin)
{
	AudioDevice &audio = platform.audio;

#if PLATFORM_WINDOWS
	DWORD playCursor;
	DWORD writeCursor;
	if (audio.buffer->GetCurrentPosition(&playCursor, &writeCursor) == DS_OK)
	{
		// Audio just started playing
		if (!audio.soundIsValid) {
			audio.runningSampleIndex = writeCursor / (audio.bytesPerSample * audio.channelCount);
			audio.soundIsValid = true;
		}

		const DWORD byteToLock = (audio.runningSampleIndex * audio.bytesPerSample * audio.channelCount) % audio.bufferSize;

		const u32 gameUpdateHz = 30;
		const f32 targetSecondsPerFrame = 1.0f / (f32)gameUpdateHz;

		const DWORD expectedBytesPerFrame = (audio.samplesPerSecond * audio.channelCount * audio.bytesPerSample) / gameUpdateHz;
		const f32 secondsLeftUntilFlip = targetSecondsPerFrame - secondsSinceFrameBegin;
		const DWORD expectedBytesUntilFlip = (DWORD)((secondsLeftUntilFlip/targetSecondsPerFrame)*(f32)expectedBytesPerFrame);
		const DWORD expectedFrameBoundaryByte = playCursor + expectedBytesPerFrame;

		DWORD safeWriteCursor = writeCursor;
		if (safeWriteCursor < playCursor) {
			safeWriteCursor += audio.bufferSize;
		}
		ASSERT(safeWriteCursor >= playCursor);

		const bool audioCardIsLowLatency = safeWriteCursor < expectedFrameBoundaryByte;

		DWORD targetCursor = 0;
		if (audioCardIsLowLatency) {
			targetCursor = expectedFrameBoundaryByte + expectedBytesPerFrame;
		} else {
			targetCursor = writeCursor + expectedBytesPerFrame + audio.safetyBytes;
		}
		targetCursor = targetCursor % audio.bufferSize;

		DWORD bytesToWrite = 0;
		if (byteToLock > targetCursor) {
			bytesToWrite = targetCursor + (audio.bufferSize - byteToLock);
		} else {
			bytesToWrite = targetCursor - byteToLock;
		}

		#if 0 // Debug code to print audio latency
		// Latency calculation
		DWORD unwrappedWriteCursor = writeCursor;
		if (writeCursor < playCursor) {
			unwrappedWriteCursor += audio.bufferSize;
		}
		const DWORD latencySize = unwrappedWriteCursor - playCursor;
		const DWORD latencySamples = latencySize / ( audio.bytesPerSample * audio.channelCount );
		const f32 latencySeconds = (f32)latencySamples / (f32)audio.samplesPerSecond;
		LOG(Debug, "latency: %u bytes (%fs)\n", latencySize, latencySeconds);
		#endif

		SoundBuffer soundBuffer = {};
		soundBuffer.samplesPerSecond = audio.samplesPerSecond;
		soundBuffer.sampleCount = bytesToWrite / (audio.bytesPerSample * audio.channelCount);
		soundBuffer.samples = audio.outputSamples;
		platform.RenderAudioCallback(platform, soundBuffer);

		Win32FillAudioBuffer(audio, byteToLock, bytesToWrite, soundBuffer.samples);
	}
	else
	{
		LOG(Warning, "Failed to GetCurrentPosition for sound buffer.\n");
	}
#elif PLATFORM_LINUX

	for (u32 i = 0; i < 2; ++i)
	{
		snd_pcm_sframes_t availableFrames;
		snd_pcm_sframes_t delayFrames;
		int res = FP_snd_pcm_avail_delay(audio.pcm, &availableFrames, &delayFrames);
		//LOG(Debug, "avail %u / delay %u\n", availableFrames, delayFrames);

		if ( res == 0 )
		{
			// TODO(jesus): Underruns seem to be fixed increasing this time window (but we add latency to reproduce new sounds)
			const float time = 2.0f / 30.0f; // Two times what's needed to render two game frames
			const snd_pcm_uframes_t maxFramesToRender = audio.samplesPerSecond * time;

			snd_pcm_uframes_t framesToRender = maxFramesToRender - delayFrames;

			framesToRender = framesToRender < availableFrames ? framesToRender : availableFrames;

			if ( framesToRender > 0 )
			{
				SoundBuffer soundBuffer = {};
				soundBuffer.samplesPerSecond = audio.samplesPerSecond;
				soundBuffer.sampleCount = framesToRender;
				soundBuffer.samples = audio.outputSamples;
				platform.RenderAudioCallback(platform, soundBuffer);

				res = FP_snd_pcm_writei(audio.pcm, soundBuffer.samples, framesToRender);

				if ( res < 0 )
				{
					LOG(Error, "Error calling snd_pcm_writei: %s\n", FP_snd_strerror(res));
				}
			}
		}
		else
		{
			LOG(Error, "Error calling snd_pcm_avail_delay: %s\n", FP_snd_strerror(res));
		}

		// Error recovery
		if (res == -EPIPE || res == -ESTRPIPE || res == -EINTR) {
			FP_snd_pcm_recover(audio.pcm, res, 0);
			continue;
		}

		// All good or unrecoverable error (no need to try a second time)
		break;
	}

#elif PLATFORM_ANDROID

	// NOTE(jesus): AAudio makes an async call to AAudioFillAudioBuffer.

#endif
}

#if USE_AUDIO_THREAD
static THREAD_FUNCTION(AudioThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	ASSERT( sPlatform );
	Platform &platform = *sPlatform;

	Clock lastClock = GetClock();

	while (1)
	{
		if ( platform.audio.isPlaying )
		{
			Clock currentClock = GetClock();
			const f32 secondsSinceLastIteration = GetSecondsElapsed(lastClock, currentClock);
			lastClock = currentClock;

			UpdateAudio(platform, secondsSinceLastIteration);

			SleepMillis(10);
		}
	}

	return 0;
}
#endif // USE_AUDIO_THREAD

bool InitializeAudio(Platform &platform)
{
	LOG(Info, "Sound system initialization:\n");

	if ( platform.RenderAudioCallback == nullptr )
	{
		LOG(Info, "- RenderAudioCallback not provided, sound system not required\n");
		return true;
	}

	AudioDevice &audio = platform.audio;
	const Window &window = platform.window;

	const u16 gameUpdateHz = 30;

	// Audio configuration
	audio.channelCount = 2;
	audio.bytesPerSample = 2; // 4 in HH
	audio.samplesPerSecond = 48000; // per channel
	audio.bufferSize = audio.channelCount * audio.samplesPerSecond * audio.bytesPerSample;
	audio.latencyFrameCount = 3;
	audio.latencySampleCount = audio.latencyFrameCount * audio.samplesPerSecond / gameUpdateHz;
	audio.safetyBytes = (audio.samplesPerSecond * audio.bytesPerSample * audio.channelCount)/audio.latencyFrameCount;

	// Allocate buffer to output samples from the engine
	audio.outputSamples = (i16*)AllocateVirtualMemory(audio.bufferSize);

#if PLATFORM_WINDOWS

	audio.library = OpenLibrary("dsound.dll");

	if (audio.library)
	{
		LOG(Info, "- Loaded dsound.dll successfully\n");

		FP_DirectSoundCreate* CreateAudioDevice = (FP_DirectSoundCreate*)LoadSymbol(audio.library, "DirectSoundCreate");

		LPDIRECTSOUND directSound;
		if (CreateAudioDevice && SUCCEEDED(CreateAudioDevice(0, &directSound, 0)))
		{
			LOG(Info, "- Audio device created successfully\n");

			if (SUCCEEDED(directSound->SetCooperativeLevel(window.hWnd, DSSCL_PRIORITY)))
			{
				// We create a primary buffer just to set the wanted format and avoid the API resample sounds to whatever rate it uses by default
				DSBUFFERDESC primaryBufferDesc = {};
				primaryBufferDesc.dwSize = sizeof(primaryBufferDesc);
				primaryBufferDesc.dwFlags = DSBCAPS_PRIMARYBUFFER;
				LPDIRECTSOUNDBUFFER primaryBuffer;
				if (SUCCEEDED(directSound->CreateSoundBuffer(&primaryBufferDesc, &primaryBuffer, 0)))
				{
					LOG(Info, "- Primary buffer created successfully\n");

					WAVEFORMATEX waveFormat = {};
					waveFormat.wFormatTag = WAVE_FORMAT_PCM;
					waveFormat.nChannels = audio.channelCount;
					waveFormat.nSamplesPerSec = audio.samplesPerSecond;
					waveFormat.nBlockAlign = audio.channelCount * audio.bytesPerSample;
					waveFormat.nAvgBytesPerSec = audio.samplesPerSecond * waveFormat.nBlockAlign;
					waveFormat.wBitsPerSample = audio.bytesPerSample * 8;
					waveFormat.cbSize = 0;
					if (SUCCEEDED(primaryBuffer->SetFormat(&waveFormat)))
					{
						LOG(Info, "- Primary buffer format set successfully\n");

						// After setting the primary buffer format, we create the secondary buffer where we will be actually writing to
						DSBUFFERDESC secondaryBufferDesc = {};
						secondaryBufferDesc.dwSize = sizeof(secondaryBufferDesc);
						// TODO(jesus): Set the DSBCAPS_GETCURRENTPOSITION2 flag?
						// TODO(jesus): Set the DSBCAPS_GLOBALFOCUS flag?
						secondaryBufferDesc.dwFlags = 0;
						secondaryBufferDesc.dwBufferBytes = audio.bufferSize;
						secondaryBufferDesc.lpwfxFormat = &waveFormat;
						LPDIRECTSOUNDBUFFER secondaryBuffer;
						if (SUCCEEDED(directSound->CreateSoundBuffer(&secondaryBufferDesc, &secondaryBuffer, 0)))
						{
							LOG(Info, "- Secondary buffer created successfully\n");

							audio.buffer = secondaryBuffer;

							MemSet(audio.outputSamples, audio.bufferSize, 0);
							Win32FillAudioBuffer(sPlatform->audio, 0, sPlatform->audio.bufferSize, audio.outputSamples);

							if (SUCCEEDED(audio.buffer->Play(0, 0, DSBPLAY_LOOPING)))
							{
								LOG(Info, "- Secondary buffer is playing...\n");
								audio.initialized = true;
								audio.isPlaying = true;
								audio.soundIsValid = false;
							}
							else
							{
								LOG(Error, "- Error playing secondaryBuffer.\n");
							}
						}
						else
						{
							LOG(Error, "- Error calling CreateSoundBuffer for secondaryBuffer\n");
						}
					}
					else
					{
						LOG(Error, "- Error setting primary buffer format\n");
					}
				}
				else
				{
					LOG(Error, "- Error calling CreateSoundBuffer for primaryBuffer\n");
				}
			}
			else
			{
				LOG(Error, "- Error setting DirectSound priority cooperative level\n");
			}
		}
		else
		{
			LOG(Error, "- Error loading DirectSoundCreate symbol\n");
		}
	}
	else
	{
		LOG(Error, "- Error loading dsound.dll\n");
	}

#elif PLATFORM_LINUX

	audio.library = OpenLibrary("libasound.so");

	if (audio.library)
	{
		DynamicLibrary alsa = audio.library;

		// Load functions
		FP_snd_strerror = (SND_STRERROR*) LoadSymbol(alsa, "snd_strerror");
		FP_snd_pcm_open = (SND_PCM_OPEN*) LoadSymbol(alsa, "snd_pcm_open");
		FP_snd_pcm_hw_params_malloc = (SND_PCM_HW_PARAMS_MALLOC*) LoadSymbol(alsa, "snd_pcm_hw_params_malloc");
		FP_snd_pcm_hw_params_any = (SND_PCM_HW_PARAMS_ANY*) LoadSymbol(alsa, "snd_pcm_hw_params_any");
		FP_snd_pcm_hw_params_set_access = (SND_PCM_HW_PARAMS_SET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_access");
		FP_snd_pcm_hw_params_set_format = (SND_PCM_HW_PARAMS_SET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_set_format");
		FP_snd_pcm_hw_params_set_channels = (SND_PCM_HW_PARAMS_SET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_set_channels");
		FP_snd_pcm_hw_params_set_rate_near = (SND_PCM_HW_PARAMS_SET_RATE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_rate_near");
		FP_snd_pcm_hw_params_set_period_size_near = (SND_PCM_HW_PARAMS_SET_PERIOD_SIZE_NEAR*) LoadSymbol(alsa, "snd_pcm_hw_params_set_period_size_near");
		FP_snd_pcm_hw_params = (SND_PCM_HW_PARAMS*) LoadSymbol(alsa, "snd_pcm_hw_params");
		FP_snd_pcm_hw_params_get_channels = (SND_PCM_HW_PARAMS_GET_CHANNELS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_channels");
		FP_snd_pcm_hw_params_get_rate = (SND_PCM_HW_PARAMS_GET_RATE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_rate");
		FP_snd_pcm_hw_params_get_format = (SND_PCM_HW_PARAMS_GET_FORMAT*) LoadSymbol(alsa, "snd_pcm_hw_params_get_format");
		FP_snd_pcm_hw_params_get_access = (SND_PCM_HW_PARAMS_GET_ACCESS*) LoadSymbol(alsa, "snd_pcm_hw_params_get_access");
		FP_snd_pcm_hw_params_get_period_time = (SND_PCM_HW_PARAMS_GET_PERIOD_TIME*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_time");
		FP_snd_pcm_hw_params_get_period_size = (SND_PCM_HW_PARAMS_GET_PERIOD_SIZE*) LoadSymbol(alsa, "snd_pcm_hw_params_get_period_size");
		FP_snd_pcm_avail_delay = (SND_PCM_AVAIL_DELAY*) LoadSymbol(alsa, "snd_pcm_avail_delay");
		FP_snd_pcm_writei = (SND_PCM_WRITEI*) LoadSymbol(alsa, "snd_pcm_writei");
		FP_snd_pcm_recover = (SND_PCM_RECOVER*) LoadSymbol(alsa, "snd_pcm_recover");
		FP_snd_pcm_prepare = (SND_PCM_PREPARE*) LoadSymbol(alsa, "snd_pcm_prepare");
		FP_snd_pcm_close = (SND_PCM_CLOSE*) LoadSymbol(alsa, "snd_pcm_close");
		FP_snd_pcm_drain = (SND_PCM_DRAIN*) LoadSymbol(alsa, "snd_pcm_drain");

		// Open PCM device
		int res = FP_snd_pcm_open(&audio.pcm, "default", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
		if (res == 0)
		{
			int dir = 0; // direction of approximate values
			unsigned int sampleRate = audio.samplesPerSecond; // frames/second (CD quality)
			unsigned int channelCount = audio.channelCount;
			unsigned int bytesPerSample = audio.bytesPerSample;
			snd_pcm_uframes_t frames = 32; // period size of 32 frames?

			// Allocate and configure hardware parameters
			snd_pcm_hw_params_t *params;
			FP_snd_pcm_hw_params_malloc(&params);
			FP_snd_pcm_hw_params_any(audio.pcm, params); // default values
			FP_snd_pcm_hw_params_set_channels(audio.pcm, params, channelCount);
			FP_snd_pcm_hw_params_set_rate_near(audio.pcm, params, &sampleRate, &dir);
			FP_snd_pcm_hw_params_set_format(audio.pcm, params, SND_PCM_FORMAT_S16_LE); // 16 bit little endian
			FP_snd_pcm_hw_params_set_access(audio.pcm, params, SND_PCM_ACCESS_RW_INTERLEAVED);
			FP_snd_pcm_hw_params_set_period_size_near(audio.pcm, params, &frames, &dir);

			// Write the parameters to the driver
			res = FP_snd_pcm_hw_params(audio.pcm, params);
			if (res == 0)
			{
				LOG(Info, "- PCM is playing...\n");
				audio.initialized = true;
				audio.isPlaying = true;
				audio.soundIsValid = false;
			}
			else
			{
				LOG(Error, "- Error setting PCM HW parameters: %s\n", FP_snd_strerror(res));
			}

			unsigned int finalSampleRate = 0;
			FP_snd_pcm_hw_params_get_rate(params, &finalSampleRate, &dir);
		}
		else
		{
			LOG(Error, "- Error opening PCM device: %s\n", FP_snd_strerror(res));
		}
	}
	else
	{
		LOG(Error, "- Error loading libasound.so\n");
	}

#elif PLATFORM_ANDROID

	AAudioStreamBuilder *builder;
	aaudio_result_t result = AAudio_createStreamBuilder(&builder);
	if ( result == AAUDIO_OK )
	{
		const int32_t deviceId = 0;
		AAudioStreamBuilder_setDeviceId(builder, deviceId);
		AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
		AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
		AAudioStreamBuilder_setSampleRate(builder, audio.samplesPerSecond);
		AAudioStreamBuilder_setChannelCount(builder, audio.channelCount);
		AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_I16);
		AAudioStreamBuilder_setBufferCapacityInFrames(builder, audio.samplesPerSecond/30);
		AAudioStreamBuilder_setDataCallback(builder, AAudioFillAudioBuffer, &platform);

		AAudioStream *stream;
		result = AAudioStreamBuilder_openStream(builder, &stream);
		if ( result == AAUDIO_OK )
		{
			LOG(Info, "- AAudioStream created successfully!\n");

			// TODO(jesus): Perform checks?
			// aaudio_format_t dataFormat = AAudioStream_getDataFormat(stream);
			// if (dataFormat == AAUDIO_FORMAT_PCM_I16) { }

			result = AAudioStream_requestStart(stream);
			if ( result == AAUDIO_OK )
			{
				LOG(Info, "- AAudioStream is playing...\n");
				audio.stream = stream;
				audio.initialized = true;
				audio.isPlaying = true;
			}
			else
			{
				LOG(Error, "- Error starting AAudioStream\n");
			}
		}
		else
		{
			LOG(Error, "- Error creating an AAudioStream\n");
		}

		AAudioStreamBuilder_delete(builder);
	}
	else
	{
		LOG(Error, "- Error creating an AAudioStreamBuilder\n");
	}

#else
#error "Missing implementation" 
#endif // PLATFORM_WINDOWS

#if USE_AUDIO_THREAD
	if ( audio.initialized )
	{
		static const ThreadInfo threadInfo = {
			.globalIndex = THREAD_ID_AUDIO,
		};
		if ( !CreateDetachedThread(AudioThread, threadInfo) )
		{
			audio.initialized = false;
		}
	}
#endif // USE_AUDIO_THREAD

	return audio.initialized;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Work queue abstraction

struct WorkQueueEntry
{
	WorkQueueCallback *callback;
	void *data;
};

struct WorkQueue
{
	Semaphore semaphore;
	volatile_i64 head;
	volatile_i64 tail;

	WorkQueueEntry entries[128] = {};
};

static WorkQueue workQueue;

static void WorkQueuePush(WorkQueueEntry entry)
{
	ASSERT(workQueue.head - workQueue.tail < ARRAY_COUNT(workQueue.entries));
	ASSERT(entry.callback);
	ASSERT(entry.data);

	const u32 index = workQueue.head % ARRAY_COUNT(workQueue.entries);
	workQueue.entries[index] = entry;

	FullWriteBarrier();

	AtomicIncrement(&workQueue.head);

	SignalSemaphore(workQueue.semaphore);
}

static bool WorkQueueEmpty()
{
	const bool empty = workQueue.head == workQueue.tail;
	return empty;
}

static bool WorkQueueProcess(const ThreadInfo &threadInfo)
{
	bool thereIsPendingWork = false;

	const u64 tail = workQueue.tail;

	if (tail < workQueue.head)
	{
		if ( AtomicSwap(&workQueue.tail, tail, tail+1) )
		{
			FullReadBarrier();

			const u32 workIndex = tail % ARRAY_COUNT(workQueue.entries);
			WorkQueueEntry &entry = workQueue.entries[workIndex];
			entry.callback(threadInfo, entry.data);
		}

		thereIsPendingWork = true;
	}

	return thereIsPendingWork;
}

static THREAD_FUNCTION(WorkQueueThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	while (1)
	{
		if ( !WorkQueueProcess(*threadInfo) )
		{
			WaitSemaphore(workQueue.semaphore);
		}
	}

	return 0;
}

#if 0 // Threading test code
static void PrintString(const ThreadInfo &threadInfo, void *data)
{
	const char *work = (const char *)data;
	LOG(Debug, "Thread %u work: %s\n", threadInfo.globalIndex, work);
}

static void WorkQueuePushString(const char *str)
{
	WorkQueueEntry entry = {
		.callback = PrintString,
		.data = (void*)str
	};
	WorkQueuePush(entry);
}
#endif

bool InitializeWorkQueue(Platform &platform)
{
	static ThreadInfo threadInfos[WORK_QUEUE_WORKER_COUNT];
	constexpr u32 threadCount = ARRAY_COUNT(threadInfos);

	const u32 iniCount = 0;
	const u32 maxCount = threadCount;
	if ( !CreateSemaphore( workQueue.semaphore, iniCount, maxCount ) )
	{
		return false;
	}

	workQueue.head = 0;
	workQueue.tail = 0;

	for (u32 i = 0; i < threadCount; ++i)
	{
		ThreadInfo &threadInfo = threadInfos[i];
		threadInfo.globalIndex = THREAD_ID_WORKER_0 + i;
		if ( !CreateDetachedThread(WorkQueueThread, threadInfo) )
		{
			return false;
		}
	}

	return true;

#if 0 // Threading test code
	WorkQueuePushString("A0");
	WorkQueuePushString("A1");
	WorkQueuePushString("A2");
	WorkQueuePushString("A3");
	WorkQueuePushString("A4");
	WorkQueuePushString("A5");
	WorkQueuePushString("A6");
	WorkQueuePushString("A7");
	WorkQueuePushString("A8");
	WorkQueuePushString("A9");
	WorkQueuePushString("B0");
	WorkQueuePushString("B1");
	WorkQueuePushString("B2");
	WorkQueuePushString("B3");
	WorkQueuePushString("B4");
	WorkQueuePushString("B5");
	WorkQueuePushString("B6");
	WorkQueuePushString("B7");
	WorkQueuePushString("B8");
	WorkQueuePushString("B9");
	WorkQueuePushString("C0");
	WorkQueuePushString("C1");
	WorkQueuePushString("C2");
	WorkQueuePushString("C3");
	WorkQueuePushString("C4");
	WorkQueuePushString("C5");
	WorkQueuePushString("C6");
	WorkQueuePushString("C7");
	WorkQueuePushString("C8");
	WorkQueuePushString("C9");

	ThreadInfo threadInfo = { 16 };
	while (!WorkQueueEmpty()) {
		WorkQueueProcess(threadInfo);
	}
#endif

	return true;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Update thread

#if USE_UPDATE_THREAD

static void ProcessPlatformEvents(Platform &platform)
{
	Window &window = platform.window;

	TransitionInputStatesSinceLastFrame(window);

	while (platform.eventTail < platform.eventHead)
	{
		const i64 uncappedIndex = AtomicPreIncrement(&platform.eventTail);
		const i32 eventIndex = uncappedIndex % ARRAY_COUNT(platform.events);
		const PlatformEvent &event = platform.events[eventIndex];

		switch (event.type)
		{
			case PlatformEventTypeWindowWasCreated:
			{
				platform.WindowInitCallback(platform);
				platform.windowInitialized = true;
				break;
			};
			case PlatformEventTypeWindowWillDestroy:
			{
				platform.windowInitialized = false;
				platform.WindowCleanupCallback(platform);
				CleanupWindow(platform.window);
				break;
			};
			case PlatformEventTypeWindowResize:
			{
				const u16 width = event.windowResize.width;
				const u16 height = event.windowResize.height;
				if ( window.width != width || window.height != height )
				{
					window.width = Max(width, 0);
					window.height = Max(height, 0);
					window.flags |= WindowFlags_WasResized;
				}
				break;
			};
			case PlatformEventTypeMouseClick:
			{
				window.mouse.buttons[event.mouseClick.button] = event.mouseClick.state;
				break;
			};
			case PlatformEventTypeMouseMove:
			{
				window.mouse.dx = event.mouseMove.x - window.mouse.x;
				window.mouse.dy = event.mouseMove.y - window.mouse.y;
				window.mouse.x = event.mouseMove.x;
				window.mouse.y = event.mouseMove.y;
				break;
			};
			case PlatformEventTypeMouseWheel:
			{
				window.mouse.wx += event.mouseWheel.dx;
				window.mouse.wy += event.mouseWheel.dy;
				break;
			};
			case PlatformEventTypeQuit:
			{
				platform.updateThreadRunning = false;
				break;
			};
		}
	}

	UpdateKeyModifiers(window);
}

static THREAD_FUNCTION(UpdateThread) // void *WorkQueueThread(void* arguments)
{
	const ThreadInfo *threadInfo = (const ThreadInfo *)arguments;

	ASSERT( sPlatform );
	Platform &platform = *sPlatform;

	Clock lastClock = GetClock();

	while ( platform.updateThreadRunning )
	{
		ResetArena(platform.frameArena);

		ProcessPlatformEvents(platform);

		if ( platform.windowInitialized )
		{
			UpdateGamepad(platform);

			platform.UpdateCallback(platform);

			platform.RenderGraphicsCallback(platform);

			platform.window.flags = 0;
		}
		else
		{
			Yield();
		}
	}

	SignalSemaphore(platform.updateThreadFinished);

	return 0;
}
#endif // USE_AUDIO_THREAD

bool InitializeUpdateThread(Platform &platform)
{
	bool ok = true;

#if USE_UPDATE_THREAD
	platform.updateThreadRunning = true;

	if ( !CreateSemaphore( platform.updateThreadFinished, 0, 1 ) )
	{
		return false;
	}

	static const ThreadInfo threadInfo = {
		.globalIndex = THREAD_ID_UPDATE,
	};
	if ( !CreateDetachedThread(UpdateThread, threadInfo) )
	{
		ok = false;
	}
#endif // USE_UPDATE_THREAD

	return ok;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform

bool PlatformInitialize(Platform &platform, int argc, char **argv)
{
	ASSERT( platform.globalMemorySize > 0 );
	ASSERT( platform.frameMemorySize > 0 );
	ASSERT( platform.InitCallback );
	ASSERT( platform.UpdateCallback );
	ASSERT( platform.RenderGraphicsCallback );
	ASSERT( platform.CleanupCallback );
	ASSERT( platform.WindowInitCallback );
	ASSERT( platform.WindowCleanupCallback );

	byte *globalMemory = (byte*)AllocateVirtualMemory(platform.globalMemorySize);
	platform.globalArena = MakeArena(globalMemory, platform.globalMemorySize);

	byte *frameMemory = (byte*)AllocateVirtualMemory(platform.frameMemorySize);
	platform.frameArena = MakeArena(frameMemory, platform.frameMemorySize);

	byte *stringMemory = (byte*)AllocateVirtualMemory(platform.stringMemorySize);
	platform.stringArena = MakeArena(stringMemory, platform.stringMemorySize);
	platform.stringInterning = StringInterningCreate(&platform.stringArena);

	byte *dataMemory = (byte*)AllocateVirtualMemory(platform.dataMemorySize);
	platform.dataArena = MakeArena(dataMemory, platform.dataMemorySize);

#if PLATFORM_ANDROID
	ASSERT( platform.androidApp );
	platform.androidApp = platform.androidApp;
	platform.androidApp->onAppCmd = AndroidHandleAppCommand;
	platform.androidApp->onInputEvent = AndroidHandleInputEvent;
	platform.androidApp->userData = &platform;
#endif // PLATFORM_ANDROID

	InitializeDirectories(platform, argc, argv);

	sPlatform = &platform;

	return true;
}

bool PlatformRun(Platform &platform)
{
	if ( !InitializeWindow(platform.window) )
	{
		return false;
	}

	const PlatformEvent event = { .type = PlatformEventTypeWindowWasCreated };
	SendPlatformEvent(platform, event);

	if ( InitializeGamepad(platform) )
	{
		// Do nothing
	}

	if ( !InitializeAudio(platform) )
	{
		return false;
	}

	if ( !InitializeWorkQueue(platform) )
	{
		return false;
	}

	if ( !platform.InitCallback(platform) )
	{
#if PLATFORM_ANDROID
		ANativeActivity_finish(platform.androidApp->activity);
#endif // PLATFORM_ANDROID
		return false;
	}

	if ( !InitializeUpdateThread(platform) )
	{
		return false;
	}

	Clock lastFrameClock = GetClock();

	const Clock firstFrameClock = GetClock();

	platform.mainThreadRunning = true;

	while ( platform.mainThreadRunning )
	{
		const Clock currentFrameBeginClock = GetClock();
		platform.deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameBeginClock);
		platform.totalSeconds = GetSecondsElapsed(firstFrameClock, currentFrameBeginClock);
		lastFrameClock = currentFrameBeginClock;

		PlatformUpdateEventLoop(platform);

#if !USE_UPDATE_THREAD
		ResetArena(platform.frameArena);

		if ( platform.window.flags & WindowFlags_Exit )
		{
			platform.mainThreadRunning = false;
			continue;
		}
		if ( platform.window.flags & WindowFlags_WasCreated )
		{
			platform.WindowInitCallback(platform);
			platform.windowInitialized = true;
		}
		if ( platform.window.flags & WindowFlags_WillDestroy )
		{
			platform.WindowCleanupCallback(platform);
			CleanupWindow(platform.window);
			platform.windowInitialized = false;
		}

		UpdateGamepad(platform);

		if ( platform.windowInitialized )
		{
			platform.UpdateCallback(platform);
		}

		platform.RenderGraphicsCallback(platform);

		platform.window.flags = 0;
#endif // !USE_UPDATE_THREAD

#if !USE_AUDIO_THREAD
		if ( platform.audio.isPlaying )
		{
			const f32 secondsSinceFrameBegin = GetSecondsElapsed(currentFrameBeginClock, GetClock());
			UpdateAudio(platform, secondsSinceFrameBegin);
		}
#endif // USE_AUDIO_THREAD
	}

#if USE_UPDATE_THREAD
	WaitSemaphore(platform.updateThreadFinished);
#endif

	if ( platform.windowInitialized )
	{
		platform.WindowCleanupCallback(platform);
	}

	platform.CleanupCallback(platform);
	// TODO: Cleanup window and audio

	return false;
}

#endif // PLATFORM_H

