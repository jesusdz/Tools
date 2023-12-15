#include "main_vulkan.cpp"

void android_main(struct android_app* app)
{
	Platform platform = {};

	// Memory
	platform.globalMemorySize = MB(64);
	platform.frameMemorySize = MB(16);

	// Callbacks
	platform.InitCallback = EngineInit;
	platform.UpdateCallback = EngineUpdate;
	platform.CleanupCallback = EngineCleanup;
	platform.WindowInitCallback = EngineWindowInit;
	platform.WindowCleanupCallback = EngineWindowCleanup;
	platform.androidApp = app;

	// User data
	Graphics gfx = {};
	platform.userData = &gfx;

	PlatformRun(platform);
}
