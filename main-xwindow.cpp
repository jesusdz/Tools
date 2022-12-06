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

#define DEFAULT_VK_ALLOCATION_CALLBACKS NULL

#define VK_CHECK_RESULT( call ) \
	if ( call != VK_SUCCESS ) \
	{ \
		printf("Vulkan call failed:.\n"); \
		printf(" - " #call "\n"); \
		return false; \
	}

bool InitializeGraphics(Arena &arena, XWindow window)
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
	applicationInfo.pApplicationName = "Example application";
	applicationInfo.applicationVersion = 0;
	applicationInfo.pEngineName = "Example engine";
	applicationInfo.engineVersion = 0;
	applicationInfo.apiVersion = 0;

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
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME 
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
	instanceCreateInfo.enabledLayerCount = enabledInstanceLayerCount;
	instanceCreateInfo.ppEnabledLayerNames = enabledInstanceLayerNames;
	instanceCreateInfo.enabledExtensionCount = enabledInstanceExtensionCount;
	instanceCreateInfo.ppEnabledExtensionNames = enabledInstanceExtensionNames;

	VkInstance instance = {};
	VK_CHECK_RESULT ( vkCreateInstance(
			&instanceCreateInfo,
			DEFAULT_VK_ALLOCATION_CALLBACKS,
			&instance ) );

	// Load the instance-related Vulkan function pointers
	volkLoadInstanceOnly(instance);

	if ( vkCreateDebugReportCallbackEXT )
	{
		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		//debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
		debugReportCallbackCreateInfo.pfnCallback = VulkanDebugReportCallback;
		debugReportCallbackCreateInfo.pUserData = 0;

		VkDebugReportCallbackEXT reportCallback;
		VK_CHECK_RESULT( vkCreateDebugReportCallbackEXT(
					instance,
					&debugReportCallbackCreateInfo,
					DEFAULT_VK_ALLOCATION_CALLBACKS,
					&reportCallback) );
	}

	VkPhysicalDevice physicalDevices[5];
	uint32_t physicalDeviceCount = 0;
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, NULL ) );
	assert( physicalDeviceCount < ARRAY_COUNT( physicalDevices ) );
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, physicalDevices ) );

	VkPhysicalDevice physicalDevice = physicalDevices[0];
	// TODO: Investigate how to do this selection better

	VkQueueFamilyProperties queueFamilyProperties[5];
	uint32_t queueFamilyPropertyCount = 0;
	vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice,
			&queueFamilyPropertyCount,
			NULL);
	ASSERT( queueFamilyPropertyCount < ARRAY_COUNT( queueFamilyProperties ) );
	vkGetPhysicalDeviceQueueFamilyProperties(
			physicalDevice,
			&queueFamilyPropertyCount,
			queueFamilyProperties);
	uint32_t queueFamilyIndex = 9999;
	for ( uint32_t i = 0; i < queueFamilyPropertyCount; ++i )
	{
		if ( queueFamilyProperties[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
		{
			// TODO: Investigate how to make this selection better
			queueFamilyIndex = i;
			break;
		}
	}

	uint32_t queueCount = 1;
	float queuePriorities[1] = { 1.0f };

	VkDeviceQueueCreateInfo queueCreateInfos[1];
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	//queueCreateInfos[0].flags = VK_DEVICE_QUEUE_CREATE_PROTECTED_BIT;
	queueCreateInfos[0].queueFamilyIndex = queueFamilyIndex;
	queueCreateInfos[0].queueCount = queueCount;
	queueCreateInfos[0].pQueuePriorities = queuePriorities;
	uint32_t queueCreateInfoCount = 0; // TODO: Crash if enabling a queue here
	// TODO

	const char *enabledDeviceLayerNames[2];
	uint32_t enabledDeviceLayerCount = 0;
	// TODO

	const char *enabledDeviceExtensionNames[2];
	uint32_t enabledDeviceExtensionCount = 0;
	// TODO

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = enabledDeviceLayerCount;
	deviceCreateInfo.ppEnabledLayerNames = enabledDeviceLayerNames;
	deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames;
	deviceCreateInfo.pEnabledFeatures = NULL;

	VkDevice device = {};
	result = vkCreateDevice(
			physicalDevice,
			&deviceCreateInfo,
			DEFAULT_VK_ALLOCATION_CALLBACKS,
			&device );
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
	vkCreateXcbSurfaceKHR(vkInstance, &createInfo, DEFAULT_VK_ALLOCATION_CALLBACKS, &surface);
#endif


	return true;
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
	if ( !InitializeGraphics(arena, window) )
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

	// Close window
	CloseXWindow(window);
	return 1;
}

