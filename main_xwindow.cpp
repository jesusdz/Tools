#include "tools.h"

#define USE_XCB
#if defined(USE_XCB)
#include <xcb/xcb.h>
#else
# error "Platform implementation for a Windowing system is missing"
#endif

#define VK_USE_PLATFORM_XCB_KHR
#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#include <unistd.h>

struct XWindow
{
#if defined(USE_XCB)
	xcb_connection_t *connection;
	xcb_window_t window;
#endif
	u32 width;
	u32 height;
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
		XCB_EVENT_MASK_KEY_PRESS       | XCB_EVENT_MASK_KEY_RELEASE    |
		XCB_EVENT_MASK_BUTTON_PRESS    | XCB_EVENT_MASK_BUTTON_RELEASE |
		XCB_EVENT_MASK_POINTER_MOTION  |
		XCB_EVENT_MASK_ENTER_WINDOW    | XCB_EVENT_MASK_LEAVE_WINDOW   |
		XCB_EVENT_MASK_RESIZE_REDIRECT
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

}

void CloseXWindow(XWindow &window)
{
#if defined(USE_XCB)
	xcb_destroy_window(window.connection, window.window);
	xcb_disconnect(window.connection);
#endif
}

#if defined(USE_XCB)
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
	VkDevice device;

	VkSurfaceKHR surface;
	VkSwapchainKHR swapchain;
	VkFormat swapchainFormat;
	VkExtent2D swapchainExtent;
	u32 swapchainImageCount;
	VkImage swapchainImages[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkImageView swapchainImageViews[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkFramebuffer swapchainFramebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	VkQueue gfxQueue;
	VkQueue presentQueue;

	// TODO: Temporary stuff hardcoded here
	VkRenderPass renderPass;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

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
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
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


	// XCB Surface
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = xwindow.connection;
	surfaceCreateInfo.window = xwindow.window;
	VkSurfaceKHR surface;
	VK_CHECK_RESULT( vkCreateXcbSurfaceKHR( instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &surface ) );


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

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	VkSurfaceFormatKHR surfaceFormat;
	VkPresentModeKHR surfacePresentMode;
	VkExtent2D surfaceExtent;


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

		vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &surfaceCapabilities);

		// Swapchain extension
		if ( surfaceCapabilities.currentExtent.width != UINT32_MAX )
		{
			surfaceExtent = surfaceCapabilities.currentExtent;
		}
		else
		{
			surfaceExtent.width = Clamp( xwindow.width, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount );
			surfaceExtent.height = Clamp( xwindow.height, surfaceCapabilities.minImageCount, surfaceCapabilities.maxImageCount );
		}

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
	VkQueue gfxQueue;
	vkGetDeviceQueue(device, graphicsQueueFamilyIndex, 0, &gfxQueue);

	VkQueue presentQueue;
	vkGetDeviceQueue(device, presentQueueFamilyIndex, 0, &presentQueue);


	// Swapchain creation
	uint32_t imageCount = surfaceCapabilities.minImageCount + 1;
	if ( surfaceCapabilities.maxImageCount > 0 )
		imageCount = Min( imageCount, surfaceCapabilities.maxImageCount );

	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = surfaceFormat.format;
	swapchainCreateInfo.imageColorSpace = surfaceFormat.colorSpace;
	swapchainCreateInfo.imageExtent = surfaceExtent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // we will render directly on it
	//swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // for typical engines with several render passes before

	uint32_t queueFamilyIndices[] = { graphicsQueueFamilyIndex, presentQueueFamilyIndex };

	if ( graphicsQueueFamilyIndex != presentQueueFamilyIndex )
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
	swapchainCreateInfo.presentMode = surfacePresentMode;
	swapchainCreateInfo.clipped = VK_TRUE; // Don't care about pixels obscured by other windows
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;

	VkSwapchainKHR swapchain;
	VK_CHECK_RESULT( vkCreateSwapchainKHR(device, &swapchainCreateInfo, VULKAN_ALLOCATORS, &swapchain) );


	// Get the swapchain images
	vkGetSwapchainImagesKHR( device, swapchain, &gfxDevice.swapchainImageCount, NULL );
	ASSERT( gfxDevice.swapchainImageCount <= ARRAY_COUNT(gfxDevice.swapchainImages) );
	vkGetSwapchainImagesKHR( device, swapchain, &gfxDevice.swapchainImageCount, gfxDevice.swapchainImages );


	// Create image views
	gfxDevice.swapchainImageCount = gfxDevice.swapchainImageCount;
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		VkImageViewCreateInfo imageViewCreateInfo = {};
		imageViewCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
		imageViewCreateInfo.image = gfxDevice.swapchainImages[i];
		imageViewCreateInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
		imageViewCreateInfo.format = surfaceFormat.format;
		imageViewCreateInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
		imageViewCreateInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		imageViewCreateInfo.subresourceRange.baseMipLevel = 0;
		imageViewCreateInfo.subresourceRange.levelCount = 1;
		imageViewCreateInfo.subresourceRange.baseArrayLayer = 0;
		imageViewCreateInfo.subresourceRange.layerCount = 1;

		VK_CHECK_RESULT( vkCreateImageView(device, &imageViewCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.swapchainImageViews[i] ) );
	}


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

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = 0.0f;
	viewport.width = (float) surfaceExtent.width;
	viewport.height = (float) surfaceExtent.height;
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = surfaceExtent;

	VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {};
	viewportStateCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
	viewportStateCreateInfo.viewportCount = 1;
	viewportStateCreateInfo.scissorCount = 1;

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


	// Framebuffer
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		VkImageView attachments[] = { gfxDevice.swapchainImageViews[i] };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass;
		framebufferCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = surfaceExtent.width;
		framebufferCreateInfo.height = surfaceExtent.height;
		framebufferCreateInfo.layers = 1;

		VK_CHECK_RESULT( vkCreateFramebuffer( device, &framebufferCreateInfo, VULKAN_ALLOCATORS, &gfxDevice.swapchainFramebuffers[i]) );
	}


	// Fill GfxDevice
	gfxDevice.instance = instance;
	gfxDevice.device = device;
	gfxDevice.surface = surface;
	gfxDevice.gfxQueue = gfxQueue;
	gfxDevice.presentQueue = presentQueue;
	gfxDevice.swapchain = swapchain;
	gfxDevice.swapchainFormat = surfaceFormat.format;
	gfxDevice.swapchainExtent = surfaceExtent;
	gfxDevice.renderPass = renderPass;
	gfxDevice.pipelineLayout = pipelineLayout;
	gfxDevice.graphicsPipeline = graphicsPipeline;
	return true;
}

void CleanupGraphics(GfxDevice &gfxDevice)
{
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		vkDestroyFramebuffer( gfxDevice.device, gfxDevice.swapchainFramebuffers[i], VULKAN_ALLOCATORS );
	}

	vkDestroyPipeline( gfxDevice.device, gfxDevice.graphicsPipeline, VULKAN_ALLOCATORS );

	vkDestroyPipelineLayout( gfxDevice.device, gfxDevice.pipelineLayout, VULKAN_ALLOCATORS );

	vkDestroyRenderPass( gfxDevice.device, gfxDevice.renderPass, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		vkDestroyImageView(gfxDevice.device, gfxDevice.swapchainImageViews[i], VULKAN_ALLOCATORS);
	}

	vkDestroySwapchainKHR(gfxDevice.device, gfxDevice.swapchain, VULKAN_ALLOCATORS);

	vkDestroyDevice(gfxDevice.device, VULKAN_ALLOCATORS);

	vkDestroySurfaceKHR(gfxDevice.instance, gfxDevice.surface, VULKAN_ALLOCATORS);

	if ( gfxDevice.support.debugReportCallbacks )
	{
		vkDestroyDebugReportCallbackEXT( gfxDevice.instance, gfxDevice.debugReportCallback, VULKAN_ALLOCATORS );
	}

	vkDestroyInstance(gfxDevice.instance, VULKAN_ALLOCATORS);

	ZeroStruct( &gfxDevice );
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
		LOG(Error, "InitializeGraphics failed!\n");
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
			switch ( event->response_type & ~0x80 )
			{
				case XCB_RESIZE_REQUEST:
					{
						xcb_resize_request_event_t *ev = (xcb_resize_request_event_t *)event;

						LOG(Info, "Window %ld was resized. New size is (%d, %d)\n", ev->window, ev->width, ev->height);
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

						LOG(Info, "Mouse moved in window %ld, at coordinates (%d,%d)\n",
								ev->event, ev->event_x, ev->event_y);
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
	}

	CleanupGraphics(gfxDevice);

	CloseXWindow(window);

	return 1;
}

