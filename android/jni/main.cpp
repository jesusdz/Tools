#define USE_VULKAN 1
//#define USE_OPENGL 1

#if USE_VULKAN
#include "main_vulkan.cpp"
#elif USE_OPENGL
#include <android/sensor.h>
#include <android/log.h>
#include <android_native_app_glue.h>
#include <EGL/egl.h>
#include <GLES/gl.h>
#endif

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

    ASensorManager* sensorManager;
    const ASensor* accelerometerSensor;
    ASensorEventQueue* sensorEventQueue;

#if USE_OPENGL
    EGLDisplay display;
    EGLSurface surface;
    EGLContext context;
#elif USE_VULKAN
	Arena arena;
	Arena frameArena;
	Window window;
	Graphics gfx;
	bool initialized;
#endif
    int32_t width;
    int32_t height;
    SavedState state;
};

/**
 * Initialize a graphics context for the current display.
 */
static int engine_init_display(Engine* engine)
{
#if USE_OPENGL

    // initialize OpenGL ES and EGL

    /*
     * Here specify the attributes of the desired configuration.
     * Below, we select an EGLConfig with at least 8 bits per color
     * component compatible with on-screen windows
     */
    const EGLint attribs[] = {
            EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
            EGL_BLUE_SIZE, 8,
            EGL_GREEN_SIZE, 8,
            EGL_RED_SIZE, 8,
            EGL_NONE
    };
    EGLint w, h, dummy, format;
    EGLint numConfigs;
    EGLConfig config;
    EGLSurface surface;
    EGLContext context;

    EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);

    eglInitialize(display, 0, 0);

    /* Here, the application chooses the configuration it desires.
     * find the best match if possible, otherwise use the very first one
     */
    eglChooseConfig(display, attribs, nullptr,0, &numConfigs);
    auto supportedConfigs = new EGLConfig[numConfigs];
    assert(supportedConfigs);
    eglChooseConfig(display, attribs, supportedConfigs, numConfigs, &numConfigs);
    assert(numConfigs);
    auto i = 0;
    for (; i < numConfigs; i++) {
        auto& cfg = supportedConfigs[i];
        EGLint r, g, b, d;
        if (eglGetConfigAttrib(display, cfg, EGL_RED_SIZE, &r)   &&
            eglGetConfigAttrib(display, cfg, EGL_GREEN_SIZE, &g) &&
            eglGetConfigAttrib(display, cfg, EGL_BLUE_SIZE, &b)  &&
            eglGetConfigAttrib(display, cfg, EGL_DEPTH_SIZE, &d) &&
            r == 8 && g == 8 && b == 8 && d == 0 ) {

            config = supportedConfigs[i];
            break;
        }
    }
    if (i == numConfigs) {
        config = supportedConfigs[0];
    }

    /* EGL_NATIVE_VISUAL_ID is an attribute of the EGLConfig that is
     * guaranteed to be accepted by ANativeWindow_setBuffersGeometry().
     * As soon as we picked a EGLConfig, we can safely reconfigure the
     * ANativeWindow buffers to match, using EGL_NATIVE_VISUAL_ID. */
    eglGetConfigAttrib(display, config, EGL_NATIVE_VISUAL_ID, &format);
    surface = eglCreateWindowSurface(display, config, engine->app->window, NULL);
    context = eglCreateContext(display, config, NULL, NULL);

    if (eglMakeCurrent(display, surface, surface, context) == EGL_FALSE) {
        LOG(Warning, "Unable to eglMakeCurrent");
        return -1;
    }

    eglQuerySurface(display, surface, EGL_WIDTH, &w);
    eglQuerySurface(display, surface, EGL_HEIGHT, &h);

    engine->display = display;
    engine->context = context;
    engine->surface = surface;
    engine->width = w;
    engine->height = h;

    // Check openGL on the system
    auto opengl_info = {GL_VENDOR, GL_RENDERER, GL_VERSION, GL_EXTENSIONS};
    for (auto name : opengl_info) {
        auto info = glGetString(name);
        LOG(Info, "OpenGL Info: %s", info);
    }
    // Initialize GL state.
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
    glEnable(GL_CULL_FACE);
    glDisable(GL_DEPTH_TEST);

#elif USE_VULKAN

	engine->window.window = engine->app->window;

	// Initialize graphics
	if ( !InitializeGraphics(engine->arena, engine->window, engine->gfx) )
	{
		LOG(Error, "InitializeGraphics failed!");
		return -1;
	}

	engine->initialized = true;
	engine->width = engine->gfx.swapchain.extent.width;
	engine->height = engine->gfx.swapchain.extent.height;
	engine->window.width = engine->width;
	engine->window.height = engine->height;

#endif

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
#if USE_OPENGL

    if (engine->display == NULL) {
        // No display.
		LOG(Warning, "No display");
        return;
    }

    // Just fill the screen with a color.
	glClearColor(0.5f, 0.5f, 0.5f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    eglSwapBuffers(engine->display, engine->surface);

#elif USE_VULKAN

	RenderGraphics(engine->gfx, engine->window, engine->frameArena, engine->state.deltaSeconds);

#endif
}

/**
 * Tear down the graphics context currently associated with the display.
 */
static void engine_term_display(Engine* engine)
{
#if USE_OPENGL
    if (engine->display != EGL_NO_DISPLAY) {
        eglMakeCurrent(engine->display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (engine->context != EGL_NO_CONTEXT) {
            eglDestroyContext(engine->display, engine->context);
        }
        if (engine->surface != EGL_NO_SURFACE) {
            eglDestroySurface(engine->display, engine->surface);
        }
        eglTerminate(engine->display);
    }
    engine->display = EGL_NO_DISPLAY;
    engine->context = EGL_NO_CONTEXT;
    engine->surface = EGL_NO_SURFACE;
#elif USE_VULKAN

	engine->initialized = false;
	CleanupGraphics(engine->gfx);

#endif
}

/**
 * Process the next input event.
 */
static int32_t engine_handle_input(struct android_app* app, AInputEvent* event) {
    Engine* engine = (Engine*)app->userData;
    if (AInputEvent_getType(event) == AINPUT_EVENT_TYPE_MOTION) {
        engine->state.x = AMotionEvent_getX(event, 0);
        engine->state.y = AMotionEvent_getY(event, 0);
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
					engine_draw_frame(engine);
				}
            }
            break;
        case APP_CMD_TERM_WINDOW:
            // The window is being hidden or closed, clean it up.
            engine_term_display(engine);
            break;
        case APP_CMD_GAINED_FOCUS:
            // When our app gains focus, we start monitoring the accelerometer.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_enableSensor(engine->sensorEventQueue,
                                               engine->accelerometerSensor);
                // We'd like to get 60 events per second (in us).
                ASensorEventQueue_setEventRate(engine->sensorEventQueue,
                                               engine->accelerometerSensor,
                                               (1000L/60)*1000);
            }
            break;
        case APP_CMD_LOST_FOCUS:
            // When our app loses focus, we stop monitoring the accelerometer.
            // This is to avoid consuming battery while not being used.
            if (engine->accelerometerSensor != NULL) {
                ASensorEventQueue_disableSensor(engine->sensorEventQueue,
                                                engine->accelerometerSensor);
            }
            engine_draw_frame(engine);
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

#if USE_VULKAN
	// Allocate base memory
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	engine.arena = MakeArena(baseMemory, baseMemorySize);

	// Frame allocator
	u32 frameMemorySize = MB(16);
	byte *frameMemory = (byte*)AllocateVirtualMemory(frameMemorySize);
	engine.frameArena = MakeArena(frameMemory, frameMemorySize);
#endif

    app->userData = &engine;
    app->onAppCmd = engine_handle_cmd;
    app->onInputEvent = engine_handle_input;
    engine.app = app;

    // Prepare to monitor accelerometer
    engine.sensorManager = ASensorManager_getInstanceForPackage("com.tools.game");
    engine.accelerometerSensor = ASensorManager_getDefaultSensor(
                                        engine.sensorManager,
                                        ASENSOR_TYPE_ACCELEROMETER);
    engine.sensorEventQueue = ASensorManager_createEventQueue(
                                    engine.sensorManager,
                                    app->looper, LOOPER_ID_USER,
                                    NULL, NULL);

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

        // Read all pending events.
        int ident;
        int events;
        struct android_poll_source* source;

		const int kWaitForever = -1;
		const int kDontWait = 0;
        while ((ident=ALooper_pollAll(kDontWait, NULL, &events, (void**)&source)) >= 0)
		{
            // Process this event.
            if (source != NULL) {
                source->process(app, source);
            }

            // If a sensor has data, process it now.
            if (ident == LOOPER_ID_USER) {
                if (engine.accelerometerSensor != NULL) {
                    ASensorEvent event;
                    while (ASensorEventQueue_getEvents(engine.sensorEventQueue,
                                                       &event, 1) > 0) {
						#if 0
                        LOG(Info, "accelerometer: x=%f y=%f z=%f",
                             event.acceleration.x, event.acceleration.y,
                             event.acceleration.z);
						#endif
                    }
                }
            }

            // Check if we are exiting.
            if (app->destroyRequested != 0) {
				engine.window.flags |= WindowFlags_Exiting;
            }
        }


#if USE_VULKAN
		ProcessWindowEvents(engine.window);

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
#endif

		engine_draw_frame(&engine);
    }

	engine_term_display(&engine);
}
