#define TOOLS_WINDOW
#include "tools.h"

#if USE_XCB
#	define VK_USE_PLATFORM_XCB_KHR 1
#elif USE_ANDROID
#	define VK_USE_PLATFORM_ANDROID_KHR 1
#elif USE_WINAPI
#	define VK_USE_PLATFORM_WIN32_KHR 1
#else
#	error "Configure the proper platform for Vulkan"
#endif

// This extension removes a VK_ERROR_INCOMPATIBLE_DRIVER when calling vkCreateInstance
// on some platforms implementing Vulkan on top of another API.
// An example of this could be Apple machines with MoltenVK on top of Metal.
#define USE_VK_EXT_PORTABILITY_ENUMERATION 0

#define VOLK_IMPLEMENTATION
#include "volk/volk.h"

#if USE_IMGUI
// Unity build Dear ImGui files
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"

#if PLATFORM_WINDOWS
#include "imgui/imgui_impl_win32.cpp"
#elif PLATFORM_LINUX
#include "imgui/imgui_impl_xcb.cpp"
#else
#error "Missing ImGui implementation file"
#endif

// Undefining VK_NO_PROTOTYPES here to avoid ImGui to retrieve Vulkan functions again.
#undef VK_NO_PROTOTYPES
#include "imgui/imgui_impl_vulkan.cpp"
#endif


struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Vertex
{
	float2 pos;
	float3 color;
};

struct VertexTransforms
{
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

static const Vertex vertices[] = {
	{{-0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}},
	{{ 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}},
	{{ 0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}},
	{{-0.5f,  0.5f}, {1.0f, 1.0f, 1.0f}}
};

static const u16 indices[] = { 0, 1, 2, 2, 3, 0 };

static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
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
#if PLATFORM_ANDROID
#define MAX_SWAPCHAIN_IMAGE_COUNT 5
#else
#define MAX_SWAPCHAIN_IMAGE_COUNT 3
#endif
#define MAX_FRAMES_IN_FLIGHT 2

static void CheckVulkanResult(VkResult result)
{
	if (result == VK_SUCCESS)
		return;
	LOG(Error, "[vulkan] VkResult = %d\n", result);
	if (result < VK_SUCCESS)
		QUIT_ABNORMALLY();
}

#define VK_CHECK_RESULT( call ) CheckVulkanResult( call )

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

#if USE_IMGUI
	VkDescriptorPool imGuiDescriptorPool;
#endif

	u32 currentFrame;

	VkPipelineCache pipelineCache;

	// TODO: Temporary stuff hardcoded here
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	VkPipelineLayout pipelineLayout;
	VkPipeline graphicsPipeline;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

	VkCommandPool transientCommandPool;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
	void *uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];

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

Buffer CreateBuffer(GfxDevice &gfxDevice, u32 size, VkBufferUsageFlags bufferUsageFlags, VkMemoryPropertyFlags memoryFlags)
{
	// Vertex buffers
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = bufferUsageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VK_CHECK_RESULT( vkCreateBuffer(gfxDevice.device, &bufferCreateInfo, VULKAN_ALLOCATORS, &buffer) );


	// Memory for the buffer
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(gfxDevice.device, buffer, &memoryRequirements);

	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(gfxDevice.physicalDevice, &physicalDeviceMemoryProperties);

	uint32_t memoryTypeIndex = -1;
	VkMemoryPropertyFlags requiredMemoryProperties = memoryFlags;
	for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		if ( (memoryRequirements.memoryTypeBits & (1 << i)) &&
				((physicalDeviceMemoryProperties.memoryTypes[i].propertyFlags & requiredMemoryProperties) == requiredMemoryProperties) )
		{
			memoryTypeIndex = i;
		}
	}
	if( memoryTypeIndex == -1 )
	{
		LOG(Error, "Could not find a proper memory type for the buffer.\n");
		QUIT_ABNORMALLY();
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = memoryRequirements.size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory;
	VK_CHECK_RESULT( vkAllocateMemory(gfxDevice.device, &memoryAllocateInfo, VULKAN_ALLOCATORS, &memory) );

	VkDeviceSize offset = 0;
	VK_CHECK_RESULT( vkBindBufferMemory(gfxDevice.device, buffer, memory, offset) );

	Buffer gfxBuffer = {
		buffer,
		memory
	};
	return gfxBuffer;
}

void CopyBuffer(GfxDevice &gfxDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = gfxDevice.transientCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(gfxDevice.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(gfxDevice.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(gfxDevice.graphicsQueue);

	vkFreeCommandBuffers(gfxDevice.device, gfxDevice.transientCommandPool, 1, &commandBuffer);
}

Buffer CreateBufferWithData(GfxDevice &gfxDevice, const void *data, u32 size, VkBufferUsageFlags usage)
{
	// Create a staging buffer backed with locally accessible memory
	Buffer stagingBuffer = CreateBuffer(
			gfxDevice,
			size,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	// Fill staging buffer memory
	void* dstData;
	VK_CHECK_RESULT( vkMapMemory(gfxDevice.device, stagingBuffer.memory, 0, size, 0, &dstData) );
	MemCopy(dstData, data, (size_t) size);
	vkUnmapMemory(gfxDevice.device, stagingBuffer.memory);

	// Create a buffer in device local memory
	Buffer finalBuffer = CreateBuffer(
			gfxDevice,
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	// Copy contents from the staging to the final buffer
	CopyBuffer(gfxDevice, stagingBuffer.buffer, finalBuffer.buffer, size);

	vkDestroyBuffer( gfxDevice.device, stagingBuffer.buffer, VULKAN_ALLOCATORS );
	vkFreeMemory( gfxDevice.device, stagingBuffer.memory, VULKAN_ALLOCATORS );

	return finalBuffer;
}

Buffer CreateVertexBuffer(GfxDevice &gfxDevice, const void *data, u32 size )
{
	Buffer vertexBuffer = CreateBufferWithData(gfxDevice, data, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	return vertexBuffer;
}


Buffer CreateIndexBuffer(GfxDevice &gfxDevice, const void *data, u32 size )
{
	Buffer indexBuffer = CreateBufferWithData(gfxDevice, data, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	return indexBuffer;
}

bool CreateSwapchain(GfxDevice &gfxDevice, Window &window)
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
		gfxDevice.swapchainExtent.width = Clamp( window.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width );
		gfxDevice.swapchainExtent.height = Clamp( window.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height );
	}

	// We want to update the Window size just in case the swapchain recreation was
	// requested from the Vulkan driver before being notified by the window manager.
	window.width = gfxDevice.swapchainExtent.width;
	window.height = gfxDevice.swapchainExtent.height;


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

bool InitializeGraphics(Arena &arena, Window window, GfxDevice &gfxDevice)
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
#if USE_VK_EXT_PORTABILITY_ENUMERATION
		VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME,
#endif
		VK_KHR_SURFACE_EXTENSION_NAME,
#if VK_USE_PLATFORM_XCB_KHR
		VK_KHR_XCB_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_ANDROID_KHR
		VK_KHR_ANDROID_SURFACE_EXTENSION_NAME,
#elif VK_USE_PLATFORM_WIN32_KHR
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
#if USE_VK_EXT_PORTABILITY_ENUMERATION
	instanceCreateInfo.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
#endif
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


#if VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = window.connection;
	surfaceCreateInfo.window = window.window;
	VkSurfaceKHR surface = {};
	VK_CHECK_RESULT( vkCreateXcbSurfaceKHR( instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &surface ) );
#elif VK_USE_PLATFORM_ANDROID_KHR
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = window.window; // ANativeWindow
	VkSurfaceKHR surface = {};
	VK_CHECK_RESULT( vkCreateAndroidSurfaceKHR( instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &surface ) );
#elif VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = window.hInstance;
	surfaceCreateInfo.hwnd = window.hWnd;
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

		#if !PLATFORM_ANDROID
		// We only want dedicated GPUs
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties( device, &properties );
		if ( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
			continue;
		#endif

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


	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = NULL;
	VkPipelineCache pipelineCache = {};
	VK_CHECK_RESULT( vkCreatePipelineCache( device, &pipelineCacheCreateInfo, VULKAN_ALLOCATORS, &pipelineCache ) );


	// Create descriptor pools
#if USE_IMGUI
	// Create Descriptor Pool
	// The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
	// If you wish to load e.g. additional textures you may need to alter pools sizes.
	VkDescriptorPoolSize descriptorPoolSizes[] =
	{
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
	};
	VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
	descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
	descriptorPoolCreateInfo.maxSets = 1;
	descriptorPoolCreateInfo.poolSizeCount = ARRAY_COUNT(descriptorPoolSizes);
	descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
	VkDescriptorPool imGuiDescriptorPool = {};
	VK_CHECK_RESULT( vkCreateDescriptorPool( device, &descriptorPoolCreateInfo, VULKAN_ALLOCATORS, &imGuiDescriptorPool ) );
#endif


	// Create descriptor set layout
	// TODO: This shouldn't be part of the device initialization
	VkDescriptorSetLayoutBinding binding = {};
	binding.binding = 0;
	binding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
	binding.descriptorCount = 1;
	binding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;
	binding.pImmutableSamplers = NULL;

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = 1;
	descriptorSetLayoutCreateInfo.pBindings = &binding;
	VkDescriptorSetLayout descriptorSetLayout;
	VK_CHECK_RESULT( vkCreateDescriptorSetLayout(device, &descriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &descriptorSetLayout) );

	// Create pipeline
	// TODO: This shouldn't be part of the device initialization
	// but let's put it here for now
#if PLATFORM_ANDROID
	// TODO: Implement functions to get the base path for asset data per platform
	const char *vertexShaderFilename = "/sdcard/Android/data/com.tools.game/files/shaders/vertex.spv";
	const char *fragmentShaderFilename = "/sdcard/Android/data/com.tools.game/files/shaders/fragment.spv";
#else
	const char *vertexShaderFilename = "shaders/vertex.spv";
	const char *fragmentShaderFilename = "shaders/fragment.spv";
#endif
	FileOnMemory *vertexFile = PushFile( scratch, vertexShaderFilename );
	if ( !vertexFile ) {
		LOG( Error, "Could not open shader file %s.\n", vertexShaderFilename );
		return false;
	}
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

	// TODO: Like I said before, vertex description shouldn't go here
	VkVertexInputBindingDescription bindingDescription = {};
	bindingDescription.binding = 0;
	bindingDescription.stride = sizeof(Vertex);
	bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[2] = {};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, color);

	VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {};
	vertexInputCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
	vertexInputCreateInfo.vertexBindingDescriptionCount = 1;
	vertexInputCreateInfo.pVertexBindingDescriptions = &bindingDescription; // Optional
	vertexInputCreateInfo.vertexAttributeDescriptionCount = ARRAY_COUNT(attributeDescriptions);
	vertexInputCreateInfo.pVertexAttributeDescriptions = attributeDescriptions; // Optional

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
	pipelineLayoutCreateInfo.setLayoutCount = 1;
	pipelineLayoutCreateInfo.pSetLayouts = &descriptorSetLayout;
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

	CreateSwapchain( gfxDevice, window );


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


	// Transient command pool
	VkCommandPoolCreateInfo transientCommandPoolCreateInfo = {};
	transientCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transientCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transientCommandPoolCreateInfo.queueFamilyIndex = graphicsQueueFamilyIndex;

	VkCommandPool transientCommandPool;
	VK_CHECK_RESULT( vkCreateCommandPool(device, &transientCommandPoolCreateInfo, VULKAN_ALLOCATORS, &transientCommandPool) );


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
	gfxDevice.descriptorSetLayout = descriptorSetLayout;
	gfxDevice.pipelineLayout = pipelineLayout;
	gfxDevice.graphicsPipeline = graphicsPipeline;
	gfxDevice.commandPool = commandPool;
	gfxDevice.transientCommandPool = transientCommandPool;
	gfxDevice.pipelineCache = pipelineCache;
#if USE_IMGUI
	gfxDevice.imGuiDescriptorPool = imGuiDescriptorPool;
#endif


	// Create a vertex buffer
	Buffer vertexBuffer = CreateVertexBuffer(gfxDevice, vertices, sizeof(vertices));
	gfxDevice.vertexBuffer = vertexBuffer.buffer;
	gfxDevice.vertexBufferMemory = vertexBuffer.memory;

	// Create a index buffer
	Buffer indexBuffer = CreateIndexBuffer(gfxDevice, indices, sizeof(indices));
	gfxDevice.indexBuffer = indexBuffer.buffer;
	gfxDevice.indexBufferMemory = indexBuffer.memory;

	// Create uniform buffers
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		Buffer uniformBuffer = CreateBuffer(
			gfxDevice,
			sizeof(VertexTransforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		gfxDevice.uniformBuffers[i] = uniformBuffer.buffer;
		gfxDevice.uniformBuffersMemory[i] = uniformBuffer.memory;

		vkMapMemory(gfxDevice.device, gfxDevice.uniformBuffersMemory[i], 0, sizeof(VertexTransforms), 0, &gfxDevice.uniformBuffersMapped[i]);
	}


	return true;
}

void WaitDeviceIdle(GfxDevice &gfxDevice)
{
	vkDeviceWaitIdle( gfxDevice.device );
}

void CleanupSwapchain(GfxDevice &gfxDevice)
{
	WaitDeviceIdle(gfxDevice);

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
	WaitDeviceIdle( gfxDevice );

	CleanupSwapchain( gfxDevice );

	vkDestroyBuffer( gfxDevice.device, gfxDevice.indexBuffer, VULKAN_ALLOCATORS );
	vkFreeMemory( gfxDevice.device, gfxDevice.indexBufferMemory, VULKAN_ALLOCATORS );
	vkDestroyBuffer( gfxDevice.device, gfxDevice.vertexBuffer, VULKAN_ALLOCATORS );
	vkFreeMemory( gfxDevice.device, gfxDevice.vertexBufferMemory, VULKAN_ALLOCATORS );

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(gfxDevice.device, gfxDevice.uniformBuffers[i], NULL);
		vkFreeMemory(gfxDevice.device, gfxDevice.uniformBuffersMemory[i], NULL);
	}

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		vkDestroySemaphore( gfxDevice.device, gfxDevice.imageAvailableSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroySemaphore( gfxDevice.device, gfxDevice.renderFinishedSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroyFence( gfxDevice.device, gfxDevice.inFlightFences[i], VULKAN_ALLOCATORS );
	}

	vkDestroyCommandPool( gfxDevice.device, gfxDevice.transientCommandPool, VULKAN_ALLOCATORS );
	vkDestroyCommandPool( gfxDevice.device, gfxDevice.commandPool, VULKAN_ALLOCATORS );

	vkDestroyPipeline( gfxDevice.device, gfxDevice.graphicsPipeline, VULKAN_ALLOCATORS );

	vkDestroyPipelineLayout( gfxDevice.device, gfxDevice.pipelineLayout, VULKAN_ALLOCATORS );

	vkDestroyDescriptorSetLayout( gfxDevice.device, gfxDevice.descriptorSetLayout, VULKAN_ALLOCATORS );

	vkDestroyPipelineCache( gfxDevice.device, gfxDevice.pipelineCache, VULKAN_ALLOCATORS );

#if USE_IMGUI
	vkDestroyDescriptorPool( gfxDevice.device, gfxDevice.imGuiDescriptorPool, VULKAN_ALLOCATORS );
#endif

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

bool RenderGraphics(GfxDevice &gfxDevice, Window &window)
{
	// TODO: create as many fences as swap images to improve synchronization
	u32 frameIndex = gfxDevice.currentFrame;

	// Swapchain sync
	vkWaitForFences( gfxDevice.device, 1, &gfxDevice.inFlightFences[frameIndex], VK_TRUE, UINT64_MAX );

	uint32_t imageIndex;
	VkResult acquireResult = vkAcquireNextImageKHR( gfxDevice.device, gfxDevice.swapchain, UINT64_MAX, gfxDevice.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex );

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		gfxDevice.shouldRecreateSwapchain = true;
		return false;
	}
	else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		LOG( Error, "vkAcquireNextImageKHR failed.\n" );
		return false;
	}

	vkResetFences( gfxDevice.device, 1, &gfxDevice.inFlightFences[frameIndex] );


	// Update uniform data
	VertexTransforms vertexTransforms;
	vertexTransforms.model = Eye();
	vertexTransforms.view = Eye();
	vertexTransforms.proj = Eye();
	MemCopy(gfxDevice.uniformBuffersMapped[frameIndex], &vertexTransforms, sizeof(vertexTransforms) );


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

	VkBuffer vertexBuffers[] = { gfxDevice.vertexBuffer };
	VkDeviceSize offsets[] = { 0 };
	vkCmdBindVertexBuffers(commandBuffer, 0, ARRAY_COUNT(vertexBuffers), vertexBuffers, offsets);

	vkCmdBindIndexBuffer(commandBuffer, gfxDevice.indexBuffer, 0, VK_INDEX_TYPE_UINT16);

	//vkCmdDraw(commandBuffer, ARRAY_COUNT(vertices), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(indices), 1, 0, 0, 0);

#if USE_IMGUI
	// Record dear imgui primitives into command buffer
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
#endif

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
		gfxDevice.shouldRecreateSwapchain = true;
	}
	else if (presentResult != VK_SUCCESS)
	{
		LOG( Error, "vkQueuePresentKHR failed.\n" );
		return false;
	}

	gfxDevice.currentFrame = ( gfxDevice.currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;


	static Clock clock0 = GetClock();
	Clock clock1 = GetClock();
	float deltaSeconds = GetSecondsElapsed(clock0, clock1);
	clock0 = clock1;

	static float elapsedSeconds = 0.0f;
	static u64 frameCount = 0;
	elapsedSeconds += deltaSeconds;
	frameCount++;
	if ( elapsedSeconds > 1.0f )
	{
		float framesPerSecond = frameCount / elapsedSeconds;
		LOG(Info, "FPS: %f\n", framesPerSecond);
		elapsedSeconds = 0.0f;
		frameCount = 0;
	}


	return true;
}



int main(int argc, char **argv)
{
	// Create Window
	Window window = {};
	if ( !InitializeWindow(window) )
	{
		LOG(Error, "InitializeWindow failed!\n");
		return -1;
	}

	// Allocate base memory
	u32 baseMemorySize = MB(64);
	byte *baseMemory = (byte*)AllocateVirtualMemory(baseMemorySize);
	Arena arena = MakeArena(baseMemory, baseMemorySize);

	// Initialize graphics
	GfxDevice gfxDevice = {};
	if ( !InitializeGraphics(arena, window, gfxDevice) )
	{
		LOG(Error, "InitializeGraphics failed!\n");
		return -1;
	}

#if USE_IMGUI
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
#if USE_WINAPI
	ImGui_ImplWin32_Init(window.hWnd);
#elif USE_XCB
	ImGui_ImplXcb_Init(window.connection, window.window);
#else
#error "Missing codepath"
#endif

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gfxDevice.instance;
	init_info.PhysicalDevice = gfxDevice.physicalDevice;
	init_info.Device = gfxDevice.device;
	init_info.QueueFamily = gfxDevice.graphicsQueueFamilyIndex;
	init_info.Queue = gfxDevice.graphicsQueue;
	init_info.PipelineCache = gfxDevice.pipelineCache;
	init_info.DescriptorPool = gfxDevice.imGuiDescriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = gfxDevice.swapchainImageCount;
	init_info.ImageCount = gfxDevice.swapchainImageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = VULKAN_ALLOCATORS;
	init_info.CheckVkResultFn = CheckVulkanResult;
	ImGui_ImplVulkan_Init(&init_info, gfxDevice.renderPass);

	// Upload Fonts
	{
		// Use any command buffer
		VkCommandPool commandPool = gfxDevice.commandPool;
		VkCommandBuffer commandBuffer = gfxDevice.commandBuffers[0];

		VK_CHECK_RESULT( vkResetCommandPool(gfxDevice.device, commandPool, 0) );
		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CHECK_RESULT( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		VkSubmitInfo endInfo = {};
		endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		endInfo.commandBufferCount = 1;
		endInfo.pCommandBuffers = &commandBuffer;
		VK_CHECK_RESULT( vkEndCommandBuffer(commandBuffer) );
		VK_CHECK_RESULT( vkQueueSubmit(gfxDevice.graphicsQueue, 1, &endInfo, VK_NULL_HANDLE) );

		WaitDeviceIdle(gfxDevice);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
#endif

	// Application loop
	while ( 1 )
	{
		ProcessWindowEvents(window);

#if USE_IMGUI
		// Start the Dear ImGui frame
		ImGui_ImplVulkan_NewFrame();
#if USE_WINAPI
		ImGui_ImplWin32_NewFrame();
#elif USE_XCB
		ImGui_ImplXcb_NewFrame();
#else
#error "Missing codepath"
#endif
		ImGui::NewFrame();

		// Create some fancy UI
		static bool checked = true;
		static f32 floatValue = 0.0f;
		static float clear_color[3] = {};
		static u32 counter = 0;
		ImGui::Begin("Hello, world!");
		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)
		ImGui::Checkbox("Example checkbox", &checked);          // Edit bools storing our window open/close state
		ImGui::SliderFloat("float", &floatValue, 0.0f, 1.0f);   // Edit 1 float using a slider from 0.0f to 1.0f
		ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color
		if (ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);
		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
		ImGui::End();

		// Generate the draw data
		ImGui::Render();
#endif

		if ( window.flags & WindowFlags_Resized || gfxDevice.shouldRecreateSwapchain)
		{
			CleanupSwapchain(gfxDevice);
			CreateSwapchain(gfxDevice, window);
			gfxDevice.shouldRecreateSwapchain = false;
		}
		if ( window.flags & WindowFlags_Exiting )
		{
			break;
		}
		if ( window.keyboard.keys[KEY_ESCAPE] == KEY_STATE_PRESSED )
		{
			break;
		}

		RenderGraphics(gfxDevice, window);
	}

	WaitDeviceIdle(gfxDevice);

#if USE_IMGUI
	ImGui_ImplVulkan_Shutdown();
#if USE_WINAPI
	ImGui_ImplWin32_Shutdown();
#elif USE_XCB
	ImGui_ImplXcb_Shutdown();
#else
#error "Missing codepath"
#endif
	ImGui::DestroyContext();
#endif

	CleanupGraphics(gfxDevice);

	CleanupWindow(window);

	PrintArenaUsage(arena);

	return 1;
}

