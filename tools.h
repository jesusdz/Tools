#include <assert.h> // assert
#include <stdlib.h> // atof
#include <stdio.h>  // printf, FILE, etc



////////////////////////////////////////////////////////////////////////////////////////////////////
// Platform definitions

#if _WIN32
#	define PLATFORM_WINDOWS 1
#elif __linux__
#	define PLATFORM_LINUX 1
#elif __APPLE__
#	define PLATFORM_APPLE 1
#else
#	error "Unsupported platform"
#endif

#if PLATFORM_WINDOWS
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#include <WindowsX.h>
#endif



////////////////////////////////////////////////////////////////////////////////////////////////////
// Base types and definitions

typedef char i8;
typedef short int i16;
typedef int i32;
typedef int i32;
typedef long long int i64;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef	unsigned char byte;

#define internal static

#define KB(x) (1024ul * x)
#define MB(x) (1024ul * KB(x))
#define GB(x) (1024ul * MB(x))
#define TB(x) (1024ul * GB(x))

#define ASSERT(expression) assert(expression)
#define INVALID_CODE_PATH() ASSERT(0 && "Invalid code path")
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))
#define LOG(channel, fmt, ...) printf(fmt, ##__VA_ARGS__)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings

struct String
{
	const char* str;
	u32 size;
};

String MakeString(const char *str, u32 size)
{
	String string = { str, size };
	return string;
}

u32 StrLen(char *str)
{
	u32 len = 0;
	while (*str++) ++len;
	return len;
}

void StrCopy(char *dst, const String& src_string)
{
	u32 size = src_string.size;
	const char *src = src_string.str;
	while (size-- > 0) *dst++ = *src++;
	dst[src_string.size] = '\0';
}

void StrCopy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	*dst = 0;
}

void StrCat(char *dst, const char *src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
}

bool StrEq(const String &s1, const String &s2)
{
	if ( s1.size != s2.size ) return false;

	const char *str1 = s1.str;
	const char *str2 = s2.str;
	const char *str2end = s2.str + s2.size;

	while ( *str1 == *str2 && str2 != str2end )
	{
		str1++;
		str2++;
	}

	return str2 == str2end;
}

bool StrEq(const String &s11, const char *s2)
{
	const char *s1 = s11.str;
	u32 count = s11.size;
	while ( count > 0 && *s1 == *s2 )
	{
		s1++;
		s2++;
		count--;
	}
	return count == 0 && *s2 == 0;
}

bool StrEq(const char *s1, const char *s2)
{
	while ( *s1 == *s2 && *s1 )
	{
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

f32 StrToFloat(const String &s)
{
	char buf[256] = {};
	ASSERT(s.size + 1 < ARRAY_COUNT(buf));
	StrCopy(buf, s);
	f32 number = atof(buf);
	return number;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory

#if PLATFORM_LINUX || PLATFORM_APPLE

#include<sys/mman.h>
#include<sys/stat.h>

void* AllocateVirtualMemory(u32 size)
{
	void* baseAddress = 0;
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_PRIVATE | MAP_ANONYMOUS;
	int fd = -1;
	off_t offset = 0;
	void *allocatedMemory = mmap(baseAddress, size, prot, flags, fd, offset);
	ASSERT( allocatedMemory != MAP_FAILED && "Failed to allocate memory." );
	return allocatedMemory;
}

#elif PLATFORM_WINDOWS

void* AllocateVirtualMemory(u32 size)
{
	void *data = VirtualAlloc(0, size, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	return data;
}

#endif

void MemSet(void *ptr, u32 size)
{
	byte *bytePtr = (byte*)ptr;
	while (size-- > 0) *bytePtr++ = 0;
}

void MemCopy(void *dst, const void *src, u32 size)
{
	const byte *pSrc = (byte*) src;
	const byte *pEnd = pSrc + size;
	byte *pDst = (byte*) dst;
	while (pSrc != pEnd) *pDst++ = *pSrc++;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Arena

struct Arena
{
	byte* base;
	u32 used;
	u32 size;
};

Arena MakeArena(byte* base, u32 size)
{
	ASSERT(base != NULL && "MakeArena needs a non-null base pointer.");
	ASSERT(size > 0 && "MakeArena needs a greater-than-zero size.");
	Arena arena = {};
	arena.base = base;
	arena.size = size;
	arena.used = 0;
	return arena;
}

Arena MakeSubArena(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "MakeSubArena of bounds of the memory arena.");
	Arena subarena = {};
	subarena.base = arena.base + arena.used;
	subarena.size = size;
	subarena.used = 0;
	return subarena;
}

Arena MakeSubArena(Arena &arena)
{
	u32 remainingSize = arena.size - arena.used;
	Arena subarena = MakeSubArena(arena, remainingSize);
	return subarena;
}

byte* PushSize(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "PushSize of bounds of the memory arena.");
	byte* head = arena.base + arena.used;
	arena.used += size;
	return head;
}

void ResetArena(Arena &arena)
{
	// This tells the OS we don't need these pages
	//madvise(arena.base, arena.size, MADV_DONTNEED);
	arena.used = 0;
}

void PrintArenaUsage(Arena &arena)
{
	printf("Memory Arena Usage:\n");
	printf("- size: %u B / %u kB\n", arena.size, arena.size/1024);
	printf("- used: %u B / %u kB\n", arena.used, arena.used/1024);
}

#define ZeroStruct( pointer ) MemSet(pointer, sizeof(*pointer) )
#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * count)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

struct FileOnMemory
{
	byte *data;
	u32 size;
};

u32 GetFileSize(const char *filename)
{
	u32 size = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fclose(f);
	}
	return size;
}

u32 ReadEntireFile(const char *filename, void *bytes, u32 bytesSize)
{
	u32 bytesRead = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		bytesRead = fread(bytes, sizeof(unsigned char), bytesSize, f);
		ASSERT(bytesRead <= bytesSize);
		((byte*)bytes)[bytesRead] = 0; // TODO: Revisit this, it's dangerous to write beyond bytesSize
		fclose(f);
	}
	return bytesRead;
}

FileOnMemory *PushFile( Arena& arena, const char *filename )
{
	FileOnMemory *file = 0;

	u32 size = GetFileSize( filename );
	if ( size > 0 )
	{
		Arena backupArena = arena;

		byte *data = PushArray( arena, byte, size + 1 ); // +1 for final zero put by ReadEntireFile
		u32 bytesRead = ReadEntireFile( filename, data, size );
		if ( bytesRead == size )
		{
			file = PushStruct( arena, FileOnMemory );
			file->data = data;
			file->size = size;
		}
		else
		{
			arena = backupArena;
		}
	}

	return file;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Math

inline u32 Min( u32 a, u32 b ) { return a < b ? a : b; }
inline u32 Max( u32 a, u32 b ) { return a > b ? a : b; }
inline u32 Clamp( u32 v, u32 min, u32 max ) { return Min( Max( v, min ), max ); }



////////////////////////////////////////////////////////////////////////////////////////////////////
// Windows

#if PLATFORM_LINUX
#	define USE_XCB 1
#elif PLATFORM_WINDOWS
#	define USE_WINAPI 1
#endif


#if USE_XCB
#	include <xcb/xcb.h>
#	define VK_USE_PLATFORM_XCB_KHR
#elif USE_WINAPI
#	define VK_USE_PLATFORM_WIN32_KHR
#endif

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"


enum WindowFlags
{
	WindowFlags_Resized = 1 << 0,
	WindowFlags_Exiting = 1 << 1,
};

struct Window
{
#if USE_XCB
	xcb_connection_t *connection;
	xcb_window_t window;
#elif USE_WINAPI
	HINSTANCE hInstance;
	HWND hWnd;
#endif
	u32 width;
	u32 height;
	u32 flags;
};

#if USE_WINAPI
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

bool InitializeWindow(Window &window)
{
	const int WINDOW_WIDTH = 640;
	const int WINDOW_HEIGHT = 480;

#if USE_XCB

	// Connect to the X server
	xcb_connection_t *xcbConnection = xcb_connect(NULL, NULL);

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
	xcb_create_window(
		xcbConnection,                 // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		xcbWindow,                     // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		WINDOW_WIDTH, WINDOW_HEIGHT,   // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	// Map the window to the screen
	xcb_map_window(xcbConnection, xcbWindow);

	// Flush the commands before continuing
	xcb_flush(xcbConnection);

	// Get window info at this point
	xcb_get_geometry_cookie_t cookie= xcb_get_geometry( xcbConnection, xcbWindow );
	xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply( xcbConnection, cookie, NULL );
	LOG(Info, "Created window size (%u, %u)\n", reply->width, reply->height);

	window.connection = xcbConnection;
	window.window = xcbWindow;
	window.width = reply->width;
	window.height = reply->height;

#endif

#if USE_WINAPI

	// Register the window class.
	const char CLASS_NAME[]  = "Sample Window Class";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = { };
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	ATOM atom = RegisterClassA(&wc);

	if (atom == 0)
	{
		// TODO: Handle error
		return false;
	}

	HWND hWnd = CreateWindowExA(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			"Learn to Program Windows",     // Window text
			WS_OVERLAPPEDWINDOW,            // Window style
			CW_USEDEFAULT, CW_USEDEFAULT,   // Position
			WINDOW_WIDTH, WINDOW_HEIGHT,    // Size
			NULL,                           // Parent window
			NULL,                           // Menu
			hInstance,                      // Instance handle
			NULL                            // Additional application data
			);

	if (hWnd == NULL)
	{
		// TODO: Handle error
		return false;
	}

	ShowWindow(hWnd, SW_SHOW);

	window.hInstance = hInstance;
	window.hWnd = hWnd;

#endif

	return true;
}

void CleanupWindow(Window &window)
{
#if USE_XCB
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#elif USE_WINAPI
	if ( !DestroyWindow(window.hWnd) )
	{
		LOG(Warning, "Error in DestroyWindow.\n");
	}
#endif
}

#if USE_WINAPI
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
	{
		case WM_SIZE:
			{
				int width = LOWORD(lParam);  // Macro to get the low-order word.
				int height = HIWORD(lParam); // Macro to get the high-order word.
				LOG( Info, "Window resized (%d, %d)\n", width, height );

				// Respond to the message:
				// TODO: Modify window size
				break;
			}

		case WM_LBUTTONDOWN:
		case WM_LBUTTONUP:
		case WM_RBUTTONDOWN:
		case WM_RBUTTONUP:
			{
				// MK_CONTROL	The CTRL key is down.
				// MK_LBUTTON	The left mouse button is down.
				// MK_MBUTTON	The middle mouse button is down.
				// MK_RBUTTON	The right mouse button is down.
				// MK_SHIFT	The SHIFT key is down.
				// MK_XBUTTON1	The XBUTTON1 button is down.
				// MK_XBUTTON2	The XBUTTON2 button is down.
				// if (wParam & MK_CONTROL) { ... }

				bool left = uMsg == WM_LBUTTONUP || uMsg == WM_LBUTTONDOWN;
				bool down = uMsg == WM_LBUTTONDOWN || uMsg == WM_RBUTTONDOWN;
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);

				UINT button = GET_XBUTTON_WPARAM(wParam);
				LOG( Info, "Mouse %s button %s at position (%d, %d)\n", left ? "LEFT" : "RIGHT", down ? "DOWN" : "UP", xPos, yPos );
				break;
			}

		case WM_MOUSEMOVE:
			{
				int xPos = GET_X_LPARAM(lParam);
				int yPos = GET_Y_LPARAM(lParam);
				//LOG( Info, "Mouse at position (%d, %d)\n", xPos, yPos );
				break;
			}

		case WM_MOUSEHOVER:
		case WM_MOUSELEAVE:
			{
				// These events are disabled by default. See documentation if needed:
				// https://learn.microsoft.com/en-us/windows/win32/learnwin32/other-mouse-operations
				LOG( Info, "Mouse %s the window\n", uMsg == WM_MOUSEHOVER ? "entered" : "left" );
				break;
			}

		case WM_DESTROY:
			{
				// This inserts a WM_QUIT message in the queue, which will in turn cause
				// GetMessage to return zero. We will exit the main loop when that happens.
				// On the other hand, PeekMessage has to handle WM_QUIT messages explicitly.
				PostQuitMessage(0);
				return 0;
			}
    }
    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
#endif



#if USE_XCB
void PrintModifiers (uint32_t mask)
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



void ProcessWindowEvents(Window &window)
{
	window.flags = 0;

#if USE_XCB
	xcb_generic_event_t *event;

	while ( (event = xcb_poll_for_event(window.connection)) != 0 )
	{
		switch ( event->response_type & ~0x80 )
		{
			case XCB_CONFIGURE_NOTIFY:
				{
					const xcb_configure_notify_event_t *ev = (const xcb_configure_notify_event_t *)event;

					if ( window.width != ev->width || window.height != ev->height )
					{
						window.width = ev->width;
						window.height = ev->height;
						LOG(Info, "Window %ld was resized. New size is (%d, %d)\n", ev->window, window.width, window.height);
						window.flags |= WindowFlags_Resized;
					}
					break;
				}

			case XCB_BUTTON_PRESS:
				{
					xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
					PrintModifiers(ev->state);

					switch (ev->detail) {
						case 4:
							LOG(Info, "Wheel Button up in window %ld, at coordinates (%d,%d)\n",
									ev->event, ev->event_x, ev->event_y);
							break;
						case 5:
							LOG(Info, "Wheel Button down in window %ld, at coordinates (%d,%d)\n",
									ev->event, ev->event_x, ev->event_y);
							break;
						default:
							LOG(Info, "Button %d pressed in window %ld, at coordinates (%d,%d)\n",
									ev->detail, ev->event, ev->event_x, ev->event_y);
					}
					break;
				}

			case XCB_BUTTON_RELEASE:
				{
					xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
					PrintModifiers(ev->state);

					LOG(Info, "Button %d released in window %ld, at coordinates (%d,%d)\n",
							ev->detail, ev->event, ev->event_x, ev->event_y);
					break;
				}

			case XCB_MOTION_NOTIFY:
				{
					xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;

					// NOTE: This is commented out to avoid excessive verbosity
					//LOG(Info, "Mouse moved in window %ld, at coordinates (%d,%d)\n",
					//		ev->event, ev->event_x, ev->event_y);
					break;
				}

			case XCB_ENTER_NOTIFY:
				{
					xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;

					LOG(Info, "Mouse entered window %ld, at coordinates (%d,%d)\n",
							ev->event, ev->event_x, ev->event_y);
					break;
				}

			case XCB_LEAVE_NOTIFY:
				{
					xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;

					LOG(Info, "Mouse left window %ld, at coordinates (%d,%d)\n",
							ev->event, ev->event_x, ev->event_y);
					break;
				}

			case XCB_KEY_PRESS:
				{
					xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
					PrintModifiers(ev->state);

					LOG(Info, "Key pressed in window %ld\n", ev->event);
					break;
				}

			case XCB_KEY_RELEASE:
				{
					xcb_key_release_event_t *ev = (xcb_key_release_event_t *)event;
					PrintModifiers(ev->state);

					LOG(Info, "Key released in window %ld\n", ev->event);

					if ( ev->detail == 9 ) // 9 is code for Escape
					{
						window.flags |= WindowFlags_Exiting;
					}
					break;
				}

			default:
				/* Unknown event type, ignore it */
				LOG(Info, "Unknown window event: %d\n", event->response_type);
				break;
		}

		free(event);
	}

#endif

#if USE_WINAPI

	MSG msg = { };
	while ( PeekMessageA( &msg, NULL, 0, 0, PM_REMOVE ) )
	{
		if ( LOWORD( msg.message ) == WM_QUIT )
		{
			window.flags |= WindowFlags_Exiting;
		}
		else
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}

#endif
}
