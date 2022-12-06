#include "tools.h"

#define USE_XCB
#if defined(USE_XCB)
#include <xcb/xcb.h>
#else
# error "Platform implementation for a Windowing system is missing"
#endif

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#include <unistd.h>

struct XWindow
{
#if defined(USE_XCB)
	xcb_connection_t *connection;
	xcb_window_t window;
#endif
};

XWindow CreateXWindow()
{
	// X11

#if defined(USE_XCB)

	// Connect to the X server
	xcb_connection_t *connection = xcb_connect(NULL, NULL);

	// Get the first screen
	const xcb_setup_t *setup = xcb_get_setup(connection);
	xcb_screen_iterator_t iter = xcb_setup_roots_iterator(setup);
	xcb_screen_t * screen = iter.data;

	// Configure events to capture
	uint32_t mask = XCB_CW_BACK_PIXEL | XCB_CW_EVENT_MASK;
	uint32_t values[2] = {
		screen->black_pixel,
		XCB_EVENT_MASK_KEY_PRESS      | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS   | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION | XCB_EVENT_MASK_EXPOSURE       |
		XCB_EVENT_MASK_ENTER_WINDOW   | XCB_EVENT_MASK_LEAVE_WINDOW 
	};

	// Create a window
	xcb_window_t window = xcb_generate_id(connection);
	xcb_create_window(
		connection,                    // xcb connection
		XCB_COPY_FROM_PARENT,          // depth
		window,                        // window id
		screen->root,                  // parent window
		0, 0,                          // x, y
		150, 150,                      // width, height
		0,                             // bnorder_width
		XCB_WINDOW_CLASS_INPUT_OUTPUT, // class
		screen->root_visual,           // visual
		mask, values);                 // value_mask, value_list

	// Map the window to the screen
	xcb_map_window(connection, window);
	
	// Flush the commands before continuing
	xcb_flush(connection);

	XWindow xwindow = {};
	xwindow.connection = connection;
	xwindow.window = window;
	return xwindow;
#endif

}

void CloseXWindow(XWindow &window)
{
#if defined(USE_XCB)
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#endif
}

#if defined(USE_XCB)
void
print_modifiers (uint32_t mask)
{
	const char **mod, *mods[] = {
		"Shift", "Lock", "Ctrl", "Alt",
		"Mod2", "Mod3", "Mod4", "Mod5",
		"Button1", "Button2", "Button3", "Button4", "Button5"
	};
	printf ("Modifier mask: ");
	for (mod = mods ; mask; mask >>= 1, mod++)
		if (mask & 1)
			printf(*mod);
	putchar ('\n');
}
#endif


VkBool32 VulkanDebugReportCallback(
		VkDebugReportFlagsEXT                       flags,
		VkDebugReportObjectTypeEXT                  objectType,
		uint64_t                                    object,
		size_t                                      location,
		int32_t                                     messageCode,
		const char*                                 pLayerPrefix,
		const char*                                 pMessage,
		void*                                       pUserData)
{
	printf("VulkanDebugReportCallback was called.\n");
	printf(" - pLayerPrefix: %s.\n", pLayerPrefix);
	printf(" - pMessage: %s.\n", pMessage);
	return VK_FALSE;
}

#define VULKAN_ALLOCATORS NULL

#define VK_CHECK_RESULT( call ) \
	if ( call != VK_SUCCESS ) \
	{ \
		printf("Vulkan call failed:.\n"); \
		printf(" - " #call "\n"); \
		return false; \
	}

struct GfxDevice
{
	VkInstance instance;
	VkDevice device;

	struct
	{
		bool debugReportCallbacks;
	} support;

	VkDebugReportCallbackEXT debugReportCallback;
};

bool InitializeGraphics(Arena &arena, XWindow window, GfxDevice &gfxDevice)
{
	Arena scratch = MakeSubArena(arena);

	// Initialize Volk -- load basic Vulkan function pointers
	VkResult result = volkInitialize();
	if ( result != VK_SUCCESS )
	{
		printf("The Vulkan loader was not found in the system.\n");
		return false;
	}

	VkApplicationInfo applicationInfo = { VK_STRUCTURE_TYPE_APPLICATION_INFO };
	applicationInfo.pApplicationName = "Vulkan application";
	applicationInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.pEngineName = "Vulkan engine";
	applicationInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
	applicationInfo.apiVersion = VK_API_VERSION_1_1;

	uint32_t instanceLayerCount;
	VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &instanceLayerCount, NULL ) );
	VkLayerProperties *instanceLayers = PushArray(scratch, VkLayerProperties, instanceLayerCount);
	VK_CHECK_RESULT( vkEnumerateInstanceLayerProperties( &instanceLayerCount, instanceLayers ) );

	const char *wantedInstanceLayerNames[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	const char *enabledInstanceLayerNames[ARRAY_COUNT(wantedInstanceLayerNames)];
	uint32_t enabledInstanceLayerCount = 0;

	printf("Instance layers:\n");
	for (u32 i = 0; i < instanceLayerCount; ++i)
	{
		bool enabled = false;

		const char *iteratedLayerName = instanceLayers[i].layerName;
		for (u32 j = 0; j < ARRAY_COUNT(wantedInstanceLayerNames); ++j)
		{
			const char *wantedLayerName = wantedInstanceLayerNames[j];
			if ( StrEq( iteratedLayerName, wantedLayerName ) )
			{
				enabledInstanceLayerNames[enabledInstanceLayerCount++] = wantedLayerName;
				enabled = true;
			}
		}

		printf("%c %s\n", enabled?'*':'-', instanceLayers[i].layerName);
	}

	uint32_t instanceExtensionCount;
	VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, NULL ) );
	VkExtensionProperties *instanceExtensions = PushArray(scratch, VkExtensionProperties, instanceExtensionCount);
	VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, instanceExtensions ) );

	const char *wantedInstanceExtensionNames[] = {
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		//VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // This one is newer, only supported from vulkan 1.1
	};
	const char *enabledInstanceExtensionNames[ARRAY_COUNT(wantedInstanceExtensionNames)];
	uint32_t enabledInstanceExtensionCount = 0;

	printf("Instance extensions:\n");
	for (u32 i = 0; i < instanceExtensionCount; ++i)
	{
		bool enabled = false;

		const char *iteratedExtensionName = instanceExtensions[i].extensionName;
		for (u32 j = 0; j < ARRAY_COUNT(wantedInstanceExtensionNames); ++j)
		{
			const char *wantedExtensionName = wantedInstanceExtensionNames[j];
			if ( StrEq( iteratedExtensionName, wantedExtensionName ) )
			{
				enabledInstanceExtensionNames[enabledInstanceExtensionCount++] = wantedExtensionName;
				enabled = true;
			}
		}

		printf("%c %s\n", enabled?'*':'-', instanceExtensions[i].extensionName);
	}

	VkInstanceCreateInfo instanceCreateInfo = { VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO };
	instanceCreateInfo.pApplicationInfo = &applicationInfo;
	instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
	instanceCreateInfo.enabledLayerCount = enabledInstanceLayerCount;
	instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayerNames;
	instanceCreateInfo.enabledExtensionCount = enabledInstanceExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensionNames;

	VkInstance instance = {};
	VK_CHECK_RESULT ( vkCreateInstance( &instanceCreateInfo, VULKAN_ALLOCATORS, &instance ) );

	// Load the instance-related Vulkan function pointers
	volkLoadInstanceOnly(instance);

	if ( vkCreateDebugReportCallbackEXT )
	{
		gfxDevice.support.debugReportCallbacks = true;

		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		//debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
		debugReportCallbackCreateInfo.pfnCallback = VulkanDebugReportCallback;
		debugReportCallbackCreateInfo.pUserData = 0;

		VK_CHECK_RESULT( vkCreateDebugReportCallbackEXT( instance, &debugReportCallbackCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.debugReportCallback) );
	}

	uint32_t physicalDeviceCount = 0;
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, physicalDevices ) );

	bool suitableDeviceFound;
	VkPhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamilyIndex;

	for (u32 i = 0; i < physicalDeviceCount; ++i)
	{
		VkPhysicalDevice device = physicalDevices[i];

		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties( device, &properties );
		if ( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
			continue;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( device, &features );
		// Check any needed features here

		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilies = PushArray( scratch, VkQueueFamilyProperties, queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies );

		uint32_t gfxFamilyIndex = -1;
		for ( uint32_t i = 0; i < queueFamilyCount; ++i )
		{
			if ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				gfxFamilyIndex = i;
				break;
			}
		}

		if ( gfxFamilyIndex != -1 )
		{
			suitableDeviceFound = true;
			physicalDevice = device;
			graphicsQueueFamilyIndex = gfxFamilyIndex;
			break;
		}
	}

	if ( !suitableDeviceFound )
	{
		printf("Could not find any suitable GFX device.\n");
		return false;
	}

	uint32_t queueCount = 1;
	float queuePriorities[1] = { 1.0f };

	VkDeviceQueueCreateInfo queueCreateInfo = {};
	queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	//queueCreateInfo.flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
	queueCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfo.queueCount = queueCount;
	queueCreateInfo.pQueuePriorities = queuePriorities;

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

	uint32_t deviceExtensionCount;
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

	const char *wantedDeviceExtensionNames[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};
	const char *enabledDeviceExtensionNames[ARRAY_COUNT(wantedDeviceExtensionNames)];
	uint32_t enabledDeviceExtensionCount = 0;

	printf("Device extensions:\n");
	for (u32 i = 0; i < deviceExtensionCount; ++i)
	{
		bool enabled = false;

		const char *iteratedExtensionName = deviceExtensions[i].extensionName;
		for (u32 j = 0; j < ARRAY_COUNT(wantedDeviceExtensionNames); ++j)
		{
			const char *wantedExtensionName = wantedDeviceExtensionNames[j];
			if ( StrEq( iteratedExtensionName, wantedExtensionName ) )
			{
				enabledDeviceExtensionNames[enabledDeviceExtensionCount++] = wantedExtensionName;
				enabled = true;
			}
		}

		printf("%c %s\n", enabled?'*':'-', deviceExtensions[i].extensionName);
	}

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = 1;
	deviceCreateInfo.pQueueCreateInfos = &queueCreateInfo;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = NULL;
	deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	VkDevice device = {};
	result = vkCreateDevice( physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &device );
	if ( result != VK_SUCCESS )
	{
		printf("vkCreateDevice failed!\n");
		return false;
	}

	// Load all the remaining device-related Vulkan function pointers
	volkLoadDevice(device);


#if 0
	VkXcbSurfaceCreateInfoKHR createInfo = { VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR };
	createInfo.flags = 0;
	VkSurfaceKHR surface;
	vkCreateXcbSurfaceKHR(vkInstance, &createInfo, VULKAN_ALLOCATORS, &surface);
#endif


	return true;
}

void CleanupGraphics(GfxDevice &gfxDevice)
{
	vkDestroyDevice(gfxDevice.device, VULKAN_ALLOCATORS);

	if ( gfxDevice.support.debugReportCallbacks )
	{
		vkDestroyDebugReportCallbackEXT( gfxDevice.instance, gfxDevice.debugReportCallback, VULKAN_ALLOCATORS );
	}

	vkDestroyInstance(gfxDevice.instance, VULKAN_ALLOCATORS);
}

struct Application
{
};

internal Application app;


int main(int argc, char **argv)
{
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	Arena arena = MakeArena(baseMemory, baseMemorySize);

	// Create Window
	XWindow window = CreateXWindow();

	// Initialize graphics
	GfxDevice gfxDevice = {};
	if ( !InitializeGraphics(arena, window, gfxDevice) )
	{
		printf("InitializeGraphics failed!\n");
		return -1;
	}
	
	// Input loop
#if defined(USE_XCB)
	xcb_generic_event_t *event;
#endif

	bool gGlobalExit = false;

	while ( !gGlobalExit )
	{
#if defined(USE_XCB)
		while ( (event = xcb_poll_for_event(window.connection)) != 0 )
		{
			printf("Event received.\n");
			switch ( event->response_type & ~0x80 )
			{

				case XCB_EXPOSE:
					{
						xcb_expose_event_t *ev = (xcb_expose_event_t *)event;

						printf ("Window %ld exposed. Region to be redrawn at location (%d,%d), with dimension (%d,%d)\n",
								ev->window, ev->x, ev->y, ev->width, ev->height);
						break;
					}

				case XCB_BUTTON_PRESS:
					{
						xcb_button_press_event_t *ev = (xcb_button_press_event_t *)event;
						print_modifiers(ev->state);

						switch (ev->detail) {
							case 4:
								printf ("Wheel Button up in window %ld, at coordinates (%d,%d)\n",
										ev->event, ev->event_x, ev->event_y);
								break;
							case 5:
								printf ("Wheel Button down in window %ld, at coordinates (%d,%d)\n",
										ev->event, ev->event_x, ev->event_y);
								break;
							default:
								printf ("Button %d pressed in window %ld, at coordinates (%d,%d)\n",
										ev->detail, ev->event, ev->event_x, ev->event_y);
						}
						break;
					}

				case XCB_BUTTON_RELEASE:
					{
						xcb_button_release_event_t *ev = (xcb_button_release_event_t *)event;
						print_modifiers(ev->state);

						printf ("Button %d released in window %ld, at coordinates (%d,%d)\n",
								ev->detail, ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_MOTION_NOTIFY:
					{
						xcb_motion_notify_event_t *ev = (xcb_motion_notify_event_t *)event;

						printf ("Mouse moved in window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}
				case XCB_ENTER_NOTIFY:
					{
						xcb_enter_notify_event_t *ev = (xcb_enter_notify_event_t *)event;

						printf ("Mouse entered window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_LEAVE_NOTIFY:
					{
						xcb_leave_notify_event_t *ev = (xcb_leave_notify_event_t *)event;

						printf ("Mouse left window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
						break;
					}

				case XCB_KEY_PRESS:
					{
						xcb_key_press_event_t *ev = (xcb_key_press_event_t *)event;
						print_modifiers(ev->state);

						printf ("Key pressed in window %ld\n", ev->event);
						break;
					}

				case XCB_KEY_RELEASE:
					{
						xcb_key_release_event_t *ev = (xcb_key_release_event_t *)event;
						print_modifiers(ev->state);

						printf ("Key released in window %ld\n", ev->event);

						if ( ev->detail == 9 ) // 9 is code for Escape
						{
							gGlobalExit = true;
						}
						break;
					}

				default:
					/* Unknown event type, ignore it */
					printf("Unknown event: %d\n", event->response_type);
					break;
			}

			free(event);
		}
#endif
	}

	CleanupGraphics(gfxDevice);

	CloseXWindow(window);

	return 1;
}

