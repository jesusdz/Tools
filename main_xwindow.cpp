#include "tools.h"


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


struct XWindow
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
};

#if USE_WINAPI
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
#endif

XWindow CreateXWindow()
{
	// X11

#if USE_XCB

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
		XCB_EVENT_MASK_KEY_PRESS       | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS    | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION  |
		XCB_EVENT_MASK_ENTER_WINDOW    | XCB_EVENT_MASK_LEAVE_WINDOW   |
		XCB_EVENT_MASK_STRUCTURE_NOTIFY
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

	// Get window info at this point
	xcb_get_geometry_cookie_t cookie= xcb_get_geometry( connection, window );
	xcb_get_geometry_reply_t *reply = xcb_get_geometry_reply( connection, cookie, NULL );
	LOG(Info, "Created window size (%u, %u)\n", reply->width, reply->height);

	XWindow xwindow = {};
	xwindow.connection = connection;
	xwindow.window = window;
	xwindow.width = reply->width;
	xwindow.height = reply->height;
	return xwindow;

#endif

#if USE_WINAPI

	// Register the window class.
	const char CLASS_NAME[]  = "Sample Window Class";
	HINSTANCE hInstance = GetModuleHandle(NULL);

	WNDCLASS wc = { };
	wc.lpfnWndProc   = WindowProc;
	wc.hInstance     = hInstance;
	wc.lpszClassName = CLASS_NAME;
	RegisterClass(&wc);

	HWND hWnd = CreateWindowEx(
			0,                              // Optional window styles.
			CLASS_NAME,                     // Window class
			"Learn to Program Windows",     // Window text
			WS_OVERLAPPEDWINDOW,            // Window style

			// Size and position
			CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

			NULL,       // Parent window
			NULL,       // Menu
			hInstance,  // Instance handle
			NULL        // Additional application data
			);

	if (hWnd == NULL)
	{
		// TODO: Handle error
	}

	ShowWindow(hWnd, SW_SHOW);

	XWindow window = {};
	window.hInstance = hInstance;
	window.hWnd = hWnd;
	return window;

#endif

}

void CloseXWindow(XWindow &window)
{
#if USE_XCB
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#elif USE_WINAPI
	DestroyWindow(window.hWnd);
#endif
}

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
	LOG(Warning, "VulkanDebugReportCallback was called.\n");
	LOG(Warning, " - pLayerPrefix: %s.\n", pLayerPrefix);
	LOG(Warning, " - pMessage: %s.\n", pMessage);
	return VK_FALSE;
}

#define VULKAN_ALLOCATORS NULL
#define MAX_SWAPCHAIN_IMAGE_COUNT 3
#define MAX_FRAMES_IN_FLIGHT 2

#define VK_CHECK_RESULT( call ) \
	if ( call != VK_SUCCESS ) \
	{ \
		LOG(Error, "Vulkan call failed:.\n"); \
		LOG(Error, " - " #call "\n"); \
		return false; \
	}

struct GfxDevice
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	VkSurfaceKHR surface;

	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	VkColorSpaceKHR swapchainColorSpace;
	VkPresentModeKHR swapchainPresentMode;
	u32 swapchainImageCount;
	VkImage swapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkImageView swapchainImageViews[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkFramebuffer swapchainFramebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];
	bool shouldRecreateSwapchain;

	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	u32 currentFrame;

	// TODO: Temporary stuff hardcoded here
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

	struct
	{
		bool debugReportCallbacks;
	} support;

	VkDebugReportCallbackEXT debugReportCallback;
};

VkShaderModule CreateShaderModule( VkDevice device, byte *data, u32 size )
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const uint32_t*>(data);

	VkShaderModule shaderModule;
	if ( vkCreateShaderModule(device, &createInfo, VULKAN_ALLOCATORS, &shaderModule) != VK_SUCCESS )
	{
		LOG(Error, "Error in CreateShaderModule.\n");
		shaderModule = VK_NULL_HANDLE;
	}

	return shaderModule;
}

bool CreateSwapchain(GfxDevice &gfxDevice, XWindow &xwindow)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfxDevice.physicalDevice, gfxDevice.surface, &surfaceCapabilities);

	// Swapchain extent
	if ( surfaceCapabilities.currentExtent.width != 0xFFFFFFFF )
	{
		gfxDevice.swapchainExtent = surfaceCapabilities.currentExtent;
	}
	else
	{
		gfxDevice.swapchainExtent.width = Clamp( xwindow.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width );
		gfxDevice.swapchainExtent.height = Clamp( xwindow.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height );
	}

	// We want to update the XWindow size just in case the swapchain recreation was
	// requested from the Vulkan driver before being notified by the window manager.
	xwindow.width = gfxDevice.swapchainExtent.width;
	xwindow.height = gfxDevice.swapchainExtent.height;


	// Image count
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if ( surfaceCapabilities.maxImageCount > 0 )
		imageCount = Min( imageCount, surfaceCapabilities.maxImageCount );


	// Swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = gfxDevice.surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = gfxDevice.swapchainFormat;
	swapchainCreateInfo.imageColorSpace = gfxDevice.swapchainColorSpace;
	swapchainCreateInfo.imageExtent = gfxDevice.swapchainExtent; // TODO: Calculate extent each time
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // we will render directly on it
	//swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // for typical engines with several render passes before

	uint32_t queueFamilyIndices[] = {
		gfxDevice.graphicsQueueFamilyIndex,
		gfxDevice.presentQueueFamilyIndex
	};

	if ( gfxDevice.graphicsQueueFamilyIndex != gfxDevice.presentQueueFamilyIndex )
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		swapchainCreateInfo.queueFamilyIndexCount = ARRAY_COUNT(queueFamilyIndices);
		swapchainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
	}
	else
	{
		swapchainCreateInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
		swapchainCreateInfo.queueFamilyIndexCount = 0; // Optional
		swapchainCreateInfo.pQueueFamilyIndices = NULL; // Optional
	}

	swapchainCreateInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR; // Ignore, no compositing over other windows
	swapchainCreateInfo.presentMode = gfxDevice.swapchainPresentMode;
	swapchainCreateInfo.clipped = VK_TRUE; // Don't care about pixels obscured by other windows
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	VkSwapchainKHR swapchain;
	VK_CHECK_RESULT( vkCreateSwapchainKHR(gfxDevice.device, &swapchainCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.swapchain) );


	// Get the swapchain images
	vkGetSwapchainImagesKHR( gfxDevice.device, gfxDevice.swapchain, &gfxDevice.swapchainImageCount, NULL );
	ASSERT( gfxDevice.swapchainImageCount <= ARRAY_COUNT(gfxDevice.swapchainImages) );
	vkGetSwapchainImagesKHR( gfxDevice.device, gfxDevice.swapchain, &gfxDevice.swapchainImageCount, gfxDevice.swapchainImages );


	// Create image views
	gfxDevice.swapchainImageCount = gfxDevice.swapchainImageCount;
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = gfxDevice.swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = gfxDevice.swapchainFormat;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT( vkCreateImageView(gfxDevice.device, &imageViewCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.swapchainImageViews[i] ) );
	}


	// Framebuffer
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		VkImageView attachments[] = { gfxDevice.swapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = gfxDevice.renderPass;
		framebufferCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = gfxDevice.swapchainExtent.width;
		framebufferCreateInfo.height = gfxDevice.swapchainExtent.height;
		framebufferCreateInfo.layers = 1;

		VK_CHECK_RESULT( vkCreateFramebuffer( gfxDevice.device, &framebufferCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.swapchainFramebuffers[i]) );
	}

	return true;
}

bool InitializeGraphics(Arena &arena, XWindow xwindow, GfxDevice &gfxDevice)
{
	Arena scratch = MakeSubArena(arena);


	// Initialize Volk -- load basic Vulkan function pointers
	VkResult result = volkInitialize();
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "The Vulkan loader was not found in the system.\n");
		return false;
	}


	// Instance creation
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

	LOG(Info, "Instance layers:\n");
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

		LOG(Info, "%c %s\n", enabled?'*':' ', instanceLayers[i].layerName);
	}

	uint32_t instanceExtensionCount;
	VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, NULL ) );
	VkExtensionProperties *instanceExtensions = PushArray(scratch, VkExtensionProperties, instanceExtensionCount);
	VK_CHECK_RESULT( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, instanceExtensions ) );

	const char *wantedInstanceExtensionNames[] = {
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
		VK_KHR_SURFACE_EXTENSION_NAME,
#if USE_XCB
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#endif
#if USE_WINAPI
		VK_KHR_WIN32_SURFACE_EXTENSION_NAME,
#endif
		VK_EXT_DEBUG_REPORT_EXTENSION_NAME,
		//VK_EXT_DEBUG_UTILS_EXTENSION_NAME, // This one is newer, only supported from vulkan 1.1
	};
	const char *enabledInstanceExtensionNames[ARRAY_COUNT(wantedInstanceExtensionNames)];
	uint32_t enabledInstanceExtensionCount = 0;

	LOG(Info, "Instance extensions:\n");
	for (u32 i = 0; i < instanceExtensionCount; ++i)
	{
		bool enabled = false;

		const char *availableExtensionName = instanceExtensions[i].extensionName;
		for (u32 j = 0; j < ARRAY_COUNT(wantedInstanceExtensionNames); ++j)
		{
			const char *wantedExtensionName = wantedInstanceExtensionNames[j];
			if ( StrEq( availableExtensionName, wantedExtensionName ) )
			{
				enabledInstanceExtensionNames[enabledInstanceExtensionCount++] = wantedExtensionName;
				enabled = true;
			}
		}

		LOG(Info, "%c %s\n", enabled?'*':' ', instanceExtensions[i].extensionName);
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


	// Report callback
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


#if USE_XCB
	// XCB Surface
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = xwindow.connection;
	surfaceCreateInfo.window = xwindow.window;
	VkSurfaceKHR surface = {};
	VK_CHECK_RESULT( vkCreateXcbSurfaceKHR( instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &surface ) );
#else
	// WIN32 Surface
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = xwindow.hInstance;
	surfaceCreateInfo.hwnd = xwindow.hWnd;
	VkSurfaceKHR surface = {};
	VK_CHECK_RESULT( vkCreateWin32SurfaceKHR( instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &surface ) );
#endif


	// List of physical devices
	uint32_t physicalDeviceCount = 0;
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( instance, &physicalDeviceCount, physicalDevices ) );

	const char *requiredDeviceExtensionNames[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};


	// Data to discover from the physical device selection
	bool suitableDeviceFound = false;

	VkPhysicalDevice physicalDevice;
	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;

	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR surfacePresentMode;


	// Physical device selection
	for (u32 i = 0; i < physicalDeviceCount; ++i)
	{
		VkPhysicalDevice device = physicalDevices[i];

		Arena scratch2 = MakeSubArena(scratch);

		// We only want dedicated GPUs
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties( device, &properties );
		if ( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
			continue;

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( device, &features );
		// Check any needed features here

		// Check the available queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilies = PushArray( scratch2, VkQueueFamilyProperties, queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( device, &queueFamilyCount, queueFamilies );

		uint32_t gfxFamilyIndex = -1;
		uint32_t presentFamilyIndex = -1;
		for ( uint32_t i = 0; i < queueFamilyCount; ++i )
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( device, i, surface, &presentSupport );
			if ( presentSupport )
			{
				presentFamilyIndex = i;
			}

			if ( queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT )
			{
				gfxFamilyIndex = i;
			}
		}

		// We don't want a device that does not support both queue types
		if ( gfxFamilyIndex == -1 || presentFamilyIndex == -1 )
			continue;

		// Check if this physical device has all the required extensions
		uint32_t deviceExtensionCount;
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( device, NULL, &deviceExtensionCount, NULL ) );
		VkExtensionProperties *deviceExtensions = PushArray( scratch2, VkExtensionProperties, deviceExtensionCount );
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( device, NULL, &deviceExtensionCount, deviceExtensions ) );

		uint32_t foundDeviceExtensionCount = 0;

		for (u32 j = 0; j < ARRAY_COUNT(requiredDeviceExtensionNames); ++j)
		{
			const char *requiredExtensionName = requiredDeviceExtensionNames[j];
			bool found = false;

			for (u32 i = 0; i < deviceExtensionCount; ++i)
			{
				const char *availableExtensionName = deviceExtensions[i].extensionName;
				if ( StrEq( availableExtensionName, requiredExtensionName ) )
				{
					foundDeviceExtensionCount++;
					found = true;
					break;
				}
			}

			if ( !found )
			{
				break;
			}
		}

		// We only want devices with all the required extensions
		if ( foundDeviceExtensionCount < ARRAY_COUNT(requiredDeviceExtensionNames) )
			continue;

		// Swapchain format
		uint32_t surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, NULL);
		if ( surfaceFormatCount == 0 )
			continue;
		VkSurfaceFormatKHR *surfaceFormats = PushArray( scratch2, VkSurfaceFormatKHR, surfaceFormatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &surfaceFormatCount, surfaceFormats);

		surfaceFormat.format = VK_FORMAT_MAX_ENUM;
		for ( u32 i = 0; i < surfaceFormatCount; ++i )
		{
			if ( surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB &&
				 surfaceFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
			{
				surfaceFormat = surfaceFormats[i];
				break;
			}
		}
		if ( surfaceFormat.format == VK_FORMAT_MAX_ENUM )
			surfaceFormat = surfaceFormats[0];

		// Swapchain present mode
		uint32_t surfacePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &surfacePresentModeCount, NULL);
		if ( surfacePresentModeCount == 0 )
			continue;
		VkPresentModeKHR *surfacePresentModes = PushArray( scratch2, VkPresentModeKHR, surfacePresentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &surfacePresentModeCount, surfacePresentModes);

#if USE_SWAPCHAIN_MAILBOX_PRESENT_MODE
		surfacePresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		for ( u32 i = 0; i < surfacePresentModeCount; ++i )
		{
			if ( surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				surfacePresentMode = surfacePresentModes[i];
			}
		}
		if ( surfacePresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
		surfacePresentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

		// At this point, we know this device meets all the requirements
		suitableDeviceFound = true;
		physicalDevice = device;
		graphicsQueueFamilyIndex = gfxFamilyIndex;
		presentQueueFamilyIndex = presentFamilyIndex;
		break;
	}

	if ( !suitableDeviceFound )
	{
		LOG(Error, "Could not find any suitable GFX device.\n");
		return false;
	}


	// Device creation
	uint32_t queueCount = 1;
	float queuePriorities[1] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
	queueCreateInfos[0].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[0].queueFamilyIndex = graphicsQueueFamilyIndex;
	queueCreateInfos[0].queueCount = queueCount;
	queueCreateInfos[0].pQueuePriorities = queuePriorities;
	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = presentQueueFamilyIndex;
	queueCreateInfos[1].queueCount = queueCount;
	queueCreateInfos[1].pQueuePriorities = queuePriorities;

#if 0
	uint32_t deviceExtensionCount;
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

	// We don't need this loop anymore unless we want to print this device extensions
	const char *enabledDeviceExtensionNames[ARRAY_COUNT(requiredDeviceExtensionNames)];
	uint32_t enabledDeviceExtensionCount = 0;

	LOG(Info, "Device extensions:\n");
	for (u32 i = 0; i < deviceExtensionCount; ++i)
	{
		bool enabled = false;

		const char *availableExtensionName = deviceExtensions[i].extensionName;
		for (u32 j = 0; j < ARRAY_COUNT(requiredDeviceExtensionNames); ++j)
		{
			const char *requiredExtensionName = requiredDeviceExtensionNames[j];
			if ( StrEq( availableExtensionName, requiredExtensionName ) )
			{
				enabledDeviceExtensionNames[enabledDeviceExtensionCount++] = requiredExtensionName;
				enabled = true;
			}
		}

		LOG(Info, "%c %s\n", enabled?'*':' ', deviceExtensions[i].extensionName);
	}
#else
	const char **enabledDeviceExtensionNames = requiredDeviceExtensionNames;
	uint32_t enabledDeviceExtensionCount = ARRAY_COUNT(requiredDeviceExtensionNames);
#endif

	VkPhysicalDeviceFeatures physicalDeviceFeatures = {};

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = ARRAY_COUNT(queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = NULL;
	deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames;
	deviceCreateInfo.pEnabledFeatures = &physicalDeviceFeatures;

	VkDevice device;
	result = vkCreateDevice( physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &device );
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "vkCreateDevice failed!\n");
		return false;
	}


	// Load all the remaining device-related Vulkan function pointers
	volkLoadDevice(device);


	// Retrieve queues
	VkQueue graphicsQueue;
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &graphicsQueue);

	VkQueue presentQueue;
	vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);


	// Create render passes
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = surfaceFormat.format;
	colorAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	colorAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	colorAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
	colorAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	colorAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	colorAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	colorAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

	VkAttachmentReference colorAttachmentRef = {};
	colorAttachmentRef.attachment = 0;
	colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = 1;
	renderPassCreateInfo.pAttachments = &colorAttachmentDesc;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;

	VkRenderPass renderPass = {};
	VK_CHECK_RESULT( vkCreateRenderPass( device, &renderPassCreateInfo, VULKAN_ALLOCATORS, &renderPass ) );


	// Create pipeline
	// TODO: This shouldn't be part of the device initialization
	// but let's put it here for now
	const char *vertexShaderFilename = "shaders/vertex.spv";
	FileOnMemory *vertexFile = PushFile( scratch, vertexShaderFilename );
	if ( !vertexFile ) {
		LOG( Error, "Could not open shader file %s.\n", vertexShaderFilename );
		return false;
	}
	const char *fragmentShaderFilename = "shaders/fragment.spv";
	FileOnMemory *fragmentFile = PushFile( scratch, fragmentShaderFilename );
	if ( !fragmentFile ) {
		LOG( Error, "Could not open shader file %s.\n", fragmentShaderFilename );
		return false;
	}

	VkShaderModule vertexShaderModule = CreateShaderModule( device, vertexFile->data, vertexFile->size );
	VkShaderModule fragmentShaderModule = CreateShaderModule( device, fragmentFile->data, fragmentFile->size );

	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
	vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageCreateInfo.module = vertexShaderModule;
	vertexShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
	fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = fragmentShaderModule;
	fragmentShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 0;
	vertexInputCreateInfo.pVertexBindingDescriptions = NULL; // Optional
	vertexInputCreateInfo.vertexAttributeDescriptionCount = 0;
	vertexInputCreateInfo.pVertexAttributeDescriptions = NULL; // Optional

	VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {};
	inputAssemblyCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
	inputAssemblyCreateInfo.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
	inputAssemblyCreateInfo.primitiveRestartEnable = VK_FALSE;

	VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {};
	dynamicStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
	dynamicStateCreateInfo.dynamicStateCount = ARRAY_COUNT(dynamicStates);
	dynamicStateCreateInfo.pDynamicStates = dynamicStates;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;
	// NOTE: We don't set values for the viewport and scissor
	// rect because they will be set dynamically using commands

	VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {};
	rasterizerCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
	rasterizerCreateInfo.depthClampEnable = VK_FALSE;
	rasterizerCreateInfo.rasterizerDiscardEnable = VK_FALSE;
	rasterizerCreateInfo.polygonMode = VK_POLYGON_MODE_FILL;
	rasterizerCreateInfo.lineWidth = 1.0f;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_CLOCKWISE;
	rasterizerCreateInfo.depthBiasEnable = VK_FALSE;
	rasterizerCreateInfo.depthBiasConstantFactor = 0.0f; // Optional
	rasterizerCreateInfo.depthBiasClamp = 0.0f; // Optional
	rasterizerCreateInfo.depthBiasSlopeFactor = 0.0f; // Optional

	VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {};
	multisamplingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
	multisamplingCreateInfo.sampleShadingEnable = VK_FALSE;
	multisamplingCreateInfo.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
	multisamplingCreateInfo.minSampleShading = 1.0f; // Optional
	multisamplingCreateInfo.pSampleMask = nullptr; // Optional
	multisamplingCreateInfo.alphaToCoverageEnable = VK_FALSE; // Optional
	multisamplingCreateInfo.alphaToOneEnable = VK_FALSE; // Optional

	VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {};
	colorBlendAttachmentState.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
	// Color replace
	colorBlendAttachmentState.blendEnable = VK_FALSE;
	colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD; // Optional
	colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE; // Optional
	colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
	colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD; // Optional
	// Alpha blending
	//colorBlendAttachmentState.blendEnable = VK_TRUE;
	//colorBlendAttachmentState.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
	//colorBlendAttachmentState.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
	//colorBlendAttachmentState.colorBlendOp = VK_BLEND_OP_ADD;
	//colorBlendAttachmentState.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
	//colorBlendAttachmentState.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
	//colorBlendAttachmentState.alphaBlendOp = VK_BLEND_OP_ADD;

	VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {};
	colorBlendingCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
	colorBlendingCreateInfo.logicOpEnable = VK_FALSE;
	colorBlendingCreateInfo.logicOp = VK_LOGIC_OP_COPY; // Optional
	colorBlendingCreateInfo.attachmentCount = 1;
	colorBlendingCreateInfo.pAttachments = &colorBlendAttachmentState;
	colorBlendingCreateInfo.blendConstants[0] = 0.0f; // Optional
	colorBlendingCreateInfo.blendConstants[1] = 0.0f; // Optional
	colorBlendingCreateInfo.blendConstants[2] = 0.0f; // Optional
	colorBlendingCreateInfo.blendConstants[3] = 0.0f; // Optional

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = 0; // Optional
	pipelineLayoutCreateInfo.pSetLayouts = nullptr; // Optional
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

	VkPipelineLayout pipelineLayout;
	VK_CHECK_RESULT( vkCreatePipelineLayout(device, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = ARRAY_COUNT(shaderStages);
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = nullptr; // Optional
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	graphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

	VkPipeline graphicsPipeline;
	VkPipelineCache pipelineCache = VK_NULL_HANDLE;
	VK_CHECK_RESULT( vkCreateGraphicsPipelines( device, pipelineCache, 1, &graphicsPipelineCreateInfo, VULKAN_ALLOCATORS, &graphicsPipeline ) );

	vkDestroyShaderModule(device, vertexShaderModule, VULKAN_ALLOCATORS);
	vkDestroyShaderModule(device, fragmentShaderModule, VULKAN_ALLOCATORS);


	// Swapchain creation
	gfxDevice.physicalDevice = physicalDevice;
	gfxDevice.device = device;
	gfxDevice.surface = surface;
	gfxDevice.swapchainFormat = surfaceFormat.format;
	gfxDevice.swapchainColorSpace = surfaceFormat.colorSpace;
	gfxDevice.swapchainPresentMode = surfacePresentMode;
	gfxDevice.graphicsQueueFamilyIndex = graphicsQueueFamilyIndex;
	gfxDevice.presentQueueFamilyIndex = presentQueueFamilyIndex;
	gfxDevice.renderPass = renderPass;

	CreateSwapchain( gfxDevice, xwindow );


	// Command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

	VkCommandPool commandPool;
	VK_CHECK_RESULT( vkCreateCommandPool(device, &commandPoolCreateInfo, VULKAN_ALLOCATORS, &commandPool) );


	// Command buffer
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = commandPool;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	VK_CHECK_RESULT( vkAllocateCommandBuffers( device, &commandBufferAllocInfo, gfxDevice.commandBuffers) );


	// Synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		VK_CHECK_RESULT( vkCreateSemaphore( device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.imageAvailableSemaphores[i] ) );
		VK_CHECK_RESULT( vkCreateSemaphore( device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.renderFinishedSemaphores[i] ) );
		VK_CHECK_RESULT( vkCreateFence( device, &fenceCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.inFlightFences[i] ) );
	}


	// Fill GfxDevice
	gfxDevice.instance = instance;
	gfxDevice.graphicsQueue = graphicsQueue;
	gfxDevice.presentQueue = presentQueue;
	gfxDevice.pipelineLayout = pipelineLayout;
	gfxDevice.graphicsPipeline = graphicsPipeline;
	gfxDevice.commandPool = commandPool;
	return true;
}

void CleanupSwapchain(GfxDevice &gfxDevice)
{
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		vkDestroyFramebuffer( gfxDevice.device, gfxDevice.swapchainFramebuffers[i], VULKAN_ALLOCATORS );
	}

	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		vkDestroyImageView(gfxDevice.device, gfxDevice.swapchainImageViews[i], VULKAN_ALLOCATORS);
	}

	vkDestroySwapchainKHR(gfxDevice.device, gfxDevice.swapchain, VULKAN_ALLOCATORS);
}

void CleanupGraphics(GfxDevice &gfxDevice)
{
	vkDeviceWaitIdle( gfxDevice.device );

	CleanupSwapchain( gfxDevice );

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		vkDestroySemaphore( gfxDevice.device, gfxDevice.imageAvailableSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroySemaphore( gfxDevice.device, gfxDevice.renderFinishedSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroyFence( gfxDevice.device, gfxDevice.inFlightFences[i], VULKAN_ALLOCATORS );
	}

	vkDestroyCommandPool( gfxDevice.device, gfxDevice.commandPool, VULKAN_ALLOCATORS );

	vkDestroyPipeline( gfxDevice.device, gfxDevice.graphicsPipeline, VULKAN_ALLOCATORS );

	vkDestroyPipelineLayout( gfxDevice.device, gfxDevice.pipelineLayout, VULKAN_ALLOCATORS );

	vkDestroyRenderPass( gfxDevice.device, gfxDevice.renderPass, VULKAN_ALLOCATORS );

	vkDestroyDevice(gfxDevice.device, VULKAN_ALLOCATORS);

	vkDestroySurfaceKHR(gfxDevice.instance, gfxDevice.surface, VULKAN_ALLOCATORS);

	if ( gfxDevice.support.debugReportCallbacks )
	{
		vkDestroyDebugReportCallbackEXT( gfxDevice.instance, gfxDevice.debugReportCallback, VULKAN_ALLOCATORS );
	}

	vkDestroyInstance(gfxDevice.instance, VULKAN_ALLOCATORS);

	ZeroStruct( &gfxDevice );
}

bool RenderGraphics(GfxDevice &gfxDevice, XWindow &xwindow)
{
	// TODO: create as many fences as swap images to improve synchronization
	u32 frameIndex = gfxDevice.currentFrame;

	// Swapchain sync
	vkWaitForFences( gfxDevice.device, 1, &gfxDevice.inFlightFences[frameIndex], VK_TRUE, UINT64_MAX );

	uint32_t imageIndex;
	VkResult acquireResult = vkAcquireNextImageKHR( gfxDevice.device, gfxDevice.swapchain, UINT64_MAX, gfxDevice.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex );

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		vkDeviceWaitIdle(gfxDevice.device);
		CleanupSwapchain(gfxDevice);
		return CreateSwapchain(gfxDevice, xwindow);
	}
	else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		LOG( Error, "vkAcquireNextImageKHR failed.\n" );
		return false;
	}

	vkResetFences( gfxDevice.device, 1, &gfxDevice.inFlightFences[frameIndex] );


	// Record commands
	VkCommandBuffer commandBuffer = gfxDevice.commandBuffers[frameIndex];

	VkCommandBufferResetFlags resetFlags = 0;
	vkResetCommandBuffer( commandBuffer, resetFlags );

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // Optional
	commandBufferBeginInfo.pInheritanceInfo = NULL; // Optional
	VK_CHECK_RESULT( vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) );

	VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = gfxDevice.renderPass;
	renderPassBeginInfo.framebuffer = gfxDevice.swapchainFramebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = gfxDevice.swapchainExtent;
	renderPassBeginInfo.clearValueCount = 1;
	renderPassBeginInfo.pClearValues = &clearColor;

	vkCmdBeginRenderPass( commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxDevice.graphicsPipeline );

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = static_cast<float>(gfxDevice.swapchainExtent.width);
	viewport.height = static_cast<float>(gfxDevice.swapchainExtent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = gfxDevice.swapchainExtent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	vkCmdDraw(commandBuffer, 3, 1, 0, 0);

	vkCmdEndRenderPass(commandBuffer);

	VK_CHECK_RESULT( vkEndCommandBuffer( commandBuffer ) );


	// Submit commands
	VkSemaphore waitSemaphores[] = { gfxDevice.imageAvailableSemaphores[frameIndex] };
	VkSemaphore signalSemaphores[] = { gfxDevice.renderFinishedSemaphores[frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT };
	// NOTE: With the following, the render pass drawing on the swapchain image should have
	// an external subpass dependency to handle the case of waiting for this stage before
	// the initial render pass transition.
	//VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = ARRAY_COUNT(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CHECK_RESULT( vkQueueSubmit( gfxDevice.graphicsQueue, 1, &submitInfo, gfxDevice.inFlightFences[frameIndex] ) );


	// Presentation
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &gfxDevice.swapchain;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL; // Optional

	VkResult presentResult = vkQueuePresentKHR( gfxDevice.presentQueue, &presentInfo );

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR || gfxDevice.shouldRecreateSwapchain)
	{
		gfxDevice.shouldRecreateSwapchain = false;
		vkDeviceWaitIdle(gfxDevice.device);
		CleanupSwapchain(gfxDevice);
		CreateSwapchain(gfxDevice, xwindow);
	}
	else if (presentResult != VK_SUCCESS)
	{
		LOG( Error, "vkQueuePresentKHR failed.\n" );
		return false;
	}

	gfxDevice.currentFrame = ( gfxDevice.currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;

	return true;
}

struct Application
{
};

internal Application app;


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
		LOG(Error, "InitializeGraphics failed!\n");
		return -1;
	}

	// Input loop
#if USE_XCB
	xcb_generic_event_t *event;
#endif

	bool gGlobalExit = false;

	while ( !gGlobalExit )
	{
#if USE_XCB

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
							gfxDevice.shouldRecreateSwapchain = true;
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
							gGlobalExit = true;
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
				gGlobalExit = true;
			}
			else
			{
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}

#endif

		if ( !gGlobalExit )
		{
			RenderGraphics(gfxDevice, window);
		}

	}

	CleanupGraphics(gfxDevice);

	CloseXWindow(window);

	return 1;
}

