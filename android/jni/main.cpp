#include "main_vulkan.cpp"

Graphics &GetPlatformGraphics(Platform &platform)
{
	Graphics *gfx = (Graphics*)platform.userData;
	ASSERT(gfx != NULL);
	return *gfx;
}

void PlatformWindowInit(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	if ( !InitializeGraphics(platform.globalArena, platform.window, gfx) )
	{
		LOG(Error, "InitializeGraphics failed!");
		return;
	}

	InitializeScene(gfx);
}

void PlatformWindowCleanup(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	CleanupGraphics(gfx);
}

void android_main(struct android_app* app)
{
	// Create platform
	Platform platform = {};
	PlatformConfig config = {};
	config.globalMemorySize = MB(64);
	config.frameMemorySize = MB(16);
	config.androidApp = app;
	if ( !PlatformInit(config, platform) )
	{
		LOG(Error, "PlatformInit failed!\n");
		return;
	}

	Window &window = platform.window;
	Arena &globalArena = platform.globalArena;
	Arena &frameArena = platform.frameArena;

	Graphics gfx = {};
	platform.userData = &gfx;
	platform.WindowInitCallback = PlatformWindowInit;
	platform.WindowCleanupCallback = PlatformWindowCleanup;

	Clock lastFrameClock = GetClock();

	// Application loop
	while ( 1 )
	{
		const Clock currentFrameClock = GetClock();
		const f32 deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameClock);
		lastFrameClock = currentFrameClock;

		PlatformUpdate(platform);

#if USE_CAMERA_MOVEMENT
		AnimateCamera(window, gfx.camera, deltaSeconds);
#endif

		if ( window.flags & WindowFlags_Exiting )
		{
			break;
		}
		if ( window.keyboard.keys[KEY_ESCAPE] == KEY_STATE_PRESSED )
		{
			break;
		}

		if ( gfx.initialized )
		{
			RenderGraphics(gfx, window, frameArena, deltaSeconds);
			ResetArena(frameArena);
		}
	}
}
