#include "main_vulkan.cpp"

#include <jni.h>
#include <initializer_list>
#include <errno.h>
#include <cassert>
#include <string.h>

/**
 * Our saved state data.
 */
struct SavedState {
    int32_t x;
    int32_t y;
	f32 deltaSeconds;
};

/**
 * Shared state for our app.
 */
struct Engine {
	struct android_app* app;

	Arena arena;
	Arena frameArena;
	Window window;
	Graphics gfx;
	bool initialized;

    SavedState state;
};

/**
 * Initialize a graphics context for the current display.
 */
static int engine_init_display(Engine* engine)
{
	engine->window.window = engine->app->window;

	// Initialize graphics
	if ( !InitializeGraphics(engine->arena, engine->window, engine->gfx) )
	{
		LOG(Error, "InitializeGraphics failed!");
		return -1;
	}

	engine->initialized = true;
    return 0;
}

/**
 * Just the current frame in the display.
 */
static void engine_draw_frame(Engine* engine)
{
	if ( !engine->initialized )
	{
		return;
	}

	RenderGraphics(engine->gfx, engine->window, engine->frameArena, engine->state.deltaSeconds);
}

/**
 * Tear down the graphics context currently associated with the display.
 */
static void engine_term_display(Engine* engine)
{
	engine->initialized = false;
	CleanupGraphics(engine->gfx);
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event)
{
	Engine* engine = (Engine*)app->userData;

    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION)
	{
		const int32_t actionAndPointer = AMotionEvent_getAction( event );
		const uint32_t action = actionAndPointer & AMOTION_EVENT_ACTION_MASK;
		const uint32_t pointerIndex = (actionAndPointer & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT;
		const uint32_t pointerId = AMotionEvent_getPointerId(event, pointerIndex);
		const uint32_t pointerCount = AMotionEvent_getPointerCount(event);
		const float x = AMotionEvent_getX(event, pointerIndex);
		const float y = AMotionEvent_getY(event, pointerIndex);

		if (pointerId < ARRAY_COUNT(engine->window.touches))
		{
			Touch *touches = engine->window.touches;

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

/**
 * Process the next main command.
 */
static void engine_handle_cmd(struct android_app* app, int32_t cmd) {
    Engine* engine = (Engine*)app->userData;
    switch (cmd) {
        case APP_CMD_SAVE_STATE:
            // The system has asked us to save our current state.  Do so.
            engine->app->savedState = malloc(sizeof(SavedState));
            *((SavedState*)engine->app->savedState) = engine->state;
            engine->app->savedStateSize = sizeof(SavedState);
            break;
        case APP_CMD_INIT_WINDOW:
            // The window is being shown, get it ready.
            if (engine->app->window != NULL)
			{
                int status = engine_init_display(engine);
				if ( status == 0 )
				{
					//engine_draw_frame(engine);
				}
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            break;
        case APP_CMD_LOST_FOCUS:
            break;
    }
}

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* app) {

	LOG(Info, "Hello native activity!");

    Engine engine = {};

	// Allocate base memory
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	engine.arena = MakeArena(baseMemory, baseMemorySize);

	// Frame allocator
	u32 frameMemorySize = MB(16);
	byte *frameMemory = (byte*)AllocateVirtualMemory(frameMemorySize);
	engine.frameArena = MakeArena(frameMemory, frameMemorySize);

    app->userData = &engine;
    app->onAppCmd = engine_handle_cmd;
    app->onInputEvent = engine_handle_input;

    engine.app = app;
	engine.window.app = app;

    if (app->savedState != NULL) {
        // We are starting with a previous saved state; restore from it.
        engine.state = *(SavedState*)app->savedState;
    }

	Clock lastFrameClock = GetClock();

    // Application loop
    while ( 1 )
	{
		Clock currentFrameClock = GetClock();
		engine.state.deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameClock);
		lastFrameClock = currentFrameClock;

		ProcessWindowEvents(engine.window);

#if USE_CAMERA_MOVEMENT
		AnimateCamera(engine.window, engine.gfx.camera, engine.state.deltaSeconds);
#endif

		if ( engine.window.flags & WindowFlags_Resized )
		{
			engine.gfx.swapchain.shouldRecreate = true;
		}
		if ( engine.window.flags & WindowFlags_Exiting )
		{
			break;
		}
		if ( engine.window.keyboard.keys[KEY_ESCAPE] == KEY_STATE_PRESSED )
		{
			break;
		}

		engine_draw_frame(&engine);
    }

	engine_term_display(&engine);
}
