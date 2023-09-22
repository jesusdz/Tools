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

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"


struct Pipeline
{
	VkPipelineLayout layout;
	VkPipeline handle;
};

struct Buffer
{
	VkBuffer buffer;
	VkDeviceMemory memory;
};

struct Image
{
	VkImage image;
	VkFormat format;
	VkDeviceMemory memory;
};

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct VertexTransforms
{
	float4x4 model;
	float4x4 view;
	float4x4 proj;
};

static const Vertex cubeVertices[] = {
	// front
	{{-0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 0.0f, 1.0f}, {0.0f, 0.0f}},
	// back
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 1.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {1.0f, 0.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 0.0f, -1.0f}, {0.0f, 0.0f}},
	// right
	{{ 0.5f, -0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	// left
	{{-0.5f, -0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f,  0.5f,  0.5f}, {-1.0f, 0.0f, 0.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {-1.0f, 0.0f, 0.0f}, {0.0f, 0.0f}},
	// top
	{{-0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
	{{ 0.5f,  0.5f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{ 0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{-0.5f,  0.5f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	// bottom
	{{ 0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 1.0f}},
	{{-0.5f, -0.5f,  0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, -0.5f, -0.5f}, {0.0f, -1.0f, 0.0f}, {0.0f, 0.0f}},
};

static const u16 cubeIndices[] = {
	0,  1,  2,  2,  3,  0,  // front
	4,  5,  6,  6,  7,  4,  // back
	8,  9,  10, 10, 11, 8,  // right
	12, 13, 14, 14, 15, 12, // left
	16, 17, 18, 18, 19, 16, // top
	20, 21, 22, 22, 23, 20, // bottom
};

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

struct Camera
{
	float3 position;
	float2 orientation; // yaw and pitch
};

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

	VkImage depthImage;
	VkDeviceMemory depthImageMemory;
	VkImageView depthImageView;

	uint32_t graphicsQueueFamilyIndex;
	uint32_t presentQueueFamilyIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkDescriptorPool descriptorPool;

#if USE_IMGUI
	VkDescriptorPool imGuiDescriptorPool;
#endif

	u32 currentFrame;

	VkCommandPool commandPool;
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];

	VkCommandPool transientCommandPool;

	VkPipelineCache pipelineCache;

	// TODO: Temporary stuff hardcoded here
	VkRenderPass renderPass;
	VkDescriptorSetLayout descriptorSetLayout;
	Pipeline pipeline;

	VkBuffer vertexBuffer;
	VkDeviceMemory vertexBufferMemory;
	VkBuffer indexBuffer;
	VkDeviceMemory indexBufferMemory;
	VkBuffer uniformBuffers[MAX_FRAMES_IN_FLIGHT];
	VkDeviceMemory uniformBuffersMemory[MAX_FRAMES_IN_FLIGHT];
	void *uniformBuffersMapped[MAX_FRAMES_IN_FLIGHT];

	Image textureImage;
	VkImageView textureImageView;
	VkSampler textureSampler;

	VkDescriptorSet descriptorSets[MAX_FRAMES_IN_FLIGHT];

	struct
	{
		bool debugReportCallbacks;
	} support;

	VkDebugReportCallbackEXT debugReportCallback;

	Camera camera;
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

u32 FindMemoryTypeIndex(GfxDevice &gfxDevice, u32 memoryTypeBits, VkMemoryPropertyFlags memoryFlags)
{
	VkPhysicalDeviceMemoryProperties physicalDeviceMemoryProperties;
	vkGetPhysicalDeviceMemoryProperties(gfxDevice.physicalDevice, &physicalDeviceMemoryProperties);

	u32 memoryTypeIndex = -1;
	VkMemoryPropertyFlags requiredMemoryProperties = memoryFlags;
	for (u32 i = 0; i < physicalDeviceMemoryProperties.memoryTypeCount; ++i)
	{
		if ( (memoryTypeBits & (1 << i)) &&
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
	return memoryTypeIndex;
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

	const u32 memoryTypeIndex = FindMemoryTypeIndex(gfxDevice, memoryRequirements.memoryTypeBits, memoryFlags);

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

VkCommandBuffer BeginTransientCommandBuffer(GfxDevice &gfxDevice)
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

	return commandBuffer;
}

void EndTransientCommandBuffer(GfxDevice &gfxDevice, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(gfxDevice.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(gfxDevice.graphicsQueue);

	vkFreeCommandBuffers(gfxDevice.device, gfxDevice.transientCommandPool, 1, &commandBuffer);
}

void CopyBuffer(GfxDevice &gfxDevice, VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfxDevice);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = 0;
	copyRegion.dstOffset = 0;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndTransientCommandBuffer(gfxDevice, commandBuffer);
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

Pipeline CreatePipeline(const GfxDevice &gd, Arena &arena)
{
	Arena scratch = MakeSubArena(arena);

	FilePath vertexShaderPath = MakePath("shaders/vertex.spv");
	FilePath fragmentShaderPath = MakePath("shaders/fragment.spv");
	FileOnMemory *vertexFile = PushFile( scratch, vertexShaderPath.str );
	if ( !vertexFile ) {
		LOG( Error, "Could not open shader file %s.\n", vertexShaderPath.str );
		QUIT_ABNORMALLY();
	}
	FileOnMemory *fragmentFile = PushFile( scratch, fragmentShaderPath.str );
	if ( !fragmentFile ) {
		LOG( Error, "Could not open shader file %s.\n", fragmentShaderPath.str );
		QUIT_ABNORMALLY();
	}

	VkShaderModule vertexShaderModule = CreateShaderModule( gd.device, vertexFile->data, vertexFile->size );
	VkShaderModule fragmentShaderModule = CreateShaderModule( gd.device, fragmentFile->data, fragmentFile->size );

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

	VkVertexInputAttributeDescription attributeDescriptions[3] = {};
	attributeDescriptions[0].binding = 0;
	attributeDescriptions[0].location = 0;
	attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[0].offset = offsetof(Vertex, pos);
	attributeDescriptions[1].binding = 0;
	attributeDescriptions[1].location = 1;
	attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
	attributeDescriptions[1].offset = offsetof(Vertex, normal);
	attributeDescriptions[2].binding = 0;
	attributeDescriptions[2].location = 2;
	attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
	attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

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
	//rasterizerCreateInfo.cullMode = VK_CULL_MODE_NONE;
	rasterizerCreateInfo.cullMode = VK_CULL_MODE_BACK_BIT;
	rasterizerCreateInfo.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
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

	VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {};
	depthStencilCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
	depthStencilCreateInfo.depthTestEnable = VK_TRUE;
	depthStencilCreateInfo.depthWriteEnable = VK_TRUE;
	depthStencilCreateInfo.depthCompareOp = VK_COMPARE_OP_LESS;
	depthStencilCreateInfo.depthBoundsTestEnable = VK_FALSE;
	depthStencilCreateInfo.minDepthBounds = 0.0f;
	depthStencilCreateInfo.maxDepthBounds = 1.0f;
	depthStencilCreateInfo.stencilTestEnable = VK_FALSE;
	depthStencilCreateInfo.front = {}; // Optional
	depthStencilCreateInfo.back = {}; // Optional

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
	pipelineLayoutCreateInfo.pSetLayouts = &gd.descriptorSetLayout;
	pipelineLayoutCreateInfo.pushConstantRangeCount = 0; // Optional
	pipelineLayoutCreateInfo.pPushConstantRanges = nullptr; // Optional

	VkPipelineLayout pipelineLayout;
	VK_CHECK_RESULT( vkCreatePipelineLayout(gd.device, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {};
	graphicsPipelineCreateInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
	graphicsPipelineCreateInfo.stageCount = ARRAY_COUNT(shaderStages);
	graphicsPipelineCreateInfo.pStages = shaderStages;
	graphicsPipelineCreateInfo.pVertexInputState = &vertexInputCreateInfo;
	graphicsPipelineCreateInfo.pInputAssemblyState = &inputAssemblyCreateInfo;
	graphicsPipelineCreateInfo.pViewportState = &viewportStateCreateInfo;
	graphicsPipelineCreateInfo.pRasterizationState = &rasterizerCreateInfo;
	graphicsPipelineCreateInfo.pMultisampleState = &multisamplingCreateInfo;
	graphicsPipelineCreateInfo.pDepthStencilState = &depthStencilCreateInfo;
	graphicsPipelineCreateInfo.pColorBlendState = &colorBlendingCreateInfo;
	graphicsPipelineCreateInfo.pDynamicState = &dynamicStateCreateInfo;
	graphicsPipelineCreateInfo.layout = pipelineLayout;
	graphicsPipelineCreateInfo.renderPass = gd.renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	graphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

	VkPipeline pipelineHandle;
	VK_CHECK_RESULT( vkCreateGraphicsPipelines( gd.device, gd.pipelineCache, 1, &graphicsPipelineCreateInfo, VULKAN_ALLOCATORS, &pipelineHandle ) );

	vkDestroyShaderModule(gd.device, vertexShaderModule, VULKAN_ALLOCATORS);
	vkDestroyShaderModule(gd.device, fragmentShaderModule, VULKAN_ALLOCATORS);

	Pipeline pipeline = {};
	pipeline.layout = pipelineLayout;
	pipeline.handle = pipelineHandle;
	return pipeline;
}

Image CreateImage(GfxDevice &gfxDevice, u32 width, u32 height, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags memoryFlags)
{
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = 1;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	VkImage image;
	VK_CHECK_RESULT( vkCreateImage(gfxDevice.device, &imageCreateInfo, VULKAN_ALLOCATORS, &image) );

	VkMemoryRequirements memoryRequirements;
	vkGetImageMemoryRequirements(gfxDevice.device, image, &memoryRequirements);

	VkMemoryAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	allocInfo.allocationSize = memoryRequirements.size;
	allocInfo.memoryTypeIndex = FindMemoryTypeIndex(gfxDevice, memoryRequirements.memoryTypeBits, memoryFlags);

	VkDeviceMemory imageMemory;
	VK_CHECK_RESULT( vkAllocateMemory(gfxDevice.device, &allocInfo, VULKAN_ALLOCATORS, &imageMemory) );

	vkBindImageMemory(gfxDevice.device, image, imageMemory, 0);

	Image imageStruct = { image, format, imageMemory};
	return imageStruct;
}

VkImageView CreateImageView(GfxDevice &gfxDevice, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags)
{
    VkImageViewCreateInfo viewInfo = {};
    viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
    viewInfo.image = image;
    viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
    viewInfo.format = format;
	viewInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
	viewInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
    viewInfo.subresourceRange.aspectMask = aspectFlags;
    viewInfo.subresourceRange.baseMipLevel = 0;
    viewInfo.subresourceRange.levelCount = 1;
    viewInfo.subresourceRange.baseArrayLayer = 0;
    viewInfo.subresourceRange.layerCount = 1;

    VkImageView imageView;
    VK_CHECK_RESULT( vkCreateImageView(gfxDevice.device, &viewInfo, nullptr, &imageView) );

    return imageView;
}

bool HasStencilComponent(VkFormat format)
{
	const bool hasStencil =
		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT;
	return hasStencil;
}

void TransitionImageLayout(GfxDevice &gfxDevice, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfxDevice);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = 1;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;

	if (newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_DEPTH_BIT;
		if (HasStencilComponent(format)) {
			barrier.subresourceRange.aspectMask |= VK_IMAGE_ASPECT_STENCIL_BIT;
		}
	} else {
		barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	}

	VkPipelineStageFlags sourceStage;
	VkPipelineStageFlags destinationStage;

	if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = 0;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else {
		INVALID_CODE_PATH();
	}

	vkCmdPipelineBarrier(commandBuffer,
		sourceStage,
		destinationStage,
		0,          // 0 or VK_DEPENDENCY_BY_REGION_BIT
		0, NULL,    // Memory barriers
		0, NULL,    // Buffer barriers
		1, &barrier // Image barriers
		);

	EndTransientCommandBuffer(gfxDevice, commandBuffer);
}

void CopyBufferToImage(GfxDevice &gfxDevice, VkBuffer buffer, VkImage image, u32 width, u32 height)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfxDevice);

	VkBufferImageCopy region{};
	region.bufferOffset = 0;
	region.bufferRowLength = 0;
	region.bufferImageHeight = 0;

	region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	region.imageSubresource.mipLevel = 0;
	region.imageSubresource.baseArrayLayer = 0;
	region.imageSubresource.layerCount = 1;

	region.imageOffset = {0, 0, 0};
	region.imageExtent = { width, height, 1 };

	vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
			);

	EndTransientCommandBuffer(gfxDevice, commandBuffer);
}

void CreateTextureImage(GfxDevice &gfxDevice)
{
	int texWidth, texHeight, texChannels;
	FilePath imagePath = MakePath("assets/image.png");
	stbi_uc* originalPixels = stbi_load(imagePath.str, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	stbi_uc* pixels = originalPixels;
	if ( !pixels )
	{
		LOG(Error, "stbi_load failed to load %s\n", imagePath.str);
		static stbi_uc constPixels[] = {255, 0, 255, 255};
		pixels = constPixels;
		texWidth = texHeight = 1;
		texChannels = 4;
	}

	u32 imageSize = texWidth * texHeight * 4;

	// Create a staging buffer backed with locally accessible memory
	Buffer stagingBuffer = CreateBuffer(
			gfxDevice,
			imageSize,
			VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );

	// Fill staging buffer memory
	void* dstData;
	VK_CHECK_RESULT( vkMapMemory(gfxDevice.device, stagingBuffer.memory, 0, imageSize, 0, &dstData) );
	MemCopy(dstData, pixels, imageSize);
	vkUnmapMemory(gfxDevice.device, stagingBuffer.memory);

	if ( originalPixels )
	{
		stbi_image_free(originalPixels);
	}

	Image image = CreateImage(gfxDevice,
			texWidth, texHeight,
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);

	TransitionImageLayout(gfxDevice, image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL);
	CopyBufferToImage(gfxDevice, stagingBuffer.buffer, image.image, texWidth, texHeight);
	TransitionImageLayout(gfxDevice, image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL);

	vkDestroyBuffer( gfxDevice.device, stagingBuffer.buffer, VULKAN_ALLOCATORS );
	vkFreeMemory( gfxDevice.device, stagingBuffer.memory, VULKAN_ALLOCATORS );

	gfxDevice.textureImage = image;
}

void CreateTextureImageView(GfxDevice &gfxDevice)
{
	gfxDevice.textureImageView = CreateImageView(gfxDevice, gfxDevice.textureImage.image, gfxDevice.textureImage.format, VK_IMAGE_ASPECT_COLOR_BIT);
}

void CreateTextureSampler(GfxDevice &gfxDevice)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(gfxDevice.physicalDevice, &properties);

	VkSamplerCreateInfo samplerCreateInfo = {};
	samplerCreateInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
	samplerCreateInfo.magFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.minFilter = VK_FILTER_LINEAR;
	samplerCreateInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
	samplerCreateInfo.anisotropyEnable = VK_TRUE;
	samplerCreateInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
	samplerCreateInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
	samplerCreateInfo.unnormalizedCoordinates = VK_FALSE;
	samplerCreateInfo.compareEnable = VK_FALSE; // For PCF shadows for instance
	samplerCreateInfo.compareOp = VK_COMPARE_OP_ALWAYS;
	samplerCreateInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
	samplerCreateInfo.mipLodBias = 0.0f;
	samplerCreateInfo.minLod = 0.0f;
	samplerCreateInfo.maxLod = 0.0f;

	VkSampler textureSampler;
	VK_CHECK_RESULT( vkCreateSampler(gfxDevice.device, &samplerCreateInfo, VULKAN_ALLOCATORS, &textureSampler) );

	gfxDevice.textureSampler = textureSampler;
}

VkFormat FindSupportedFormat(GfxDevice &gfxDevice, const VkFormat candidates[], u32 candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (u32 i = 0; i < candidateCount; ++i)
	{
		VkFormat format = candidates[i];
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(gfxDevice.physicalDevice, format, &properties);

		if (tiling == VK_IMAGE_TILING_LINEAR && (features & properties.linearTilingFeatures) == features)
		{
			return format;
		}
		else if (tiling == VK_IMAGE_TILING_OPTIMAL && (features & properties.optimalTilingFeatures) == features)
		{
			return format;
		}
	}

	INVALID_CODE_PATH();
	return VK_FORMAT_MAX_ENUM;
}

VkFormat FindDepthFormat(GfxDevice &gfxDevice)
{
	const VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return FindSupportedFormat(gfxDevice, candidates, ARRAY_COUNT(candidates),
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
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


	// Depth buffer
	VkFormat depthFormat = FindDepthFormat(gfxDevice);
	Image depthImage = CreateImage(gfxDevice,
			gfxDevice.swapchainExtent.width, gfxDevice.swapchainExtent.height,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	VkImageView depthImageView = CreateImageView(gfxDevice, depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT);
	TransitionImageLayout(gfxDevice, depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL);
	gfxDevice.depthImage = depthImage.image;
	gfxDevice.depthImageMemory = depthImage.memory;
	gfxDevice.depthImageView = depthImageView;


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
		const VkImage image = gfxDevice.swapchainImages[i];
		const VkFormat format = gfxDevice.swapchainFormat;
		gfxDevice.swapchainImageViews[i] = CreateImageView(gfxDevice, image, format, VK_IMAGE_ASPECT_COLOR_BIT);
	}


	// Framebuffer
	for ( u32 i = 0; i < gfxDevice.swapchainImageCount; ++i )
	{
		VkImageView attachments[] = { gfxDevice.swapchainImageViews[i], gfxDevice.depthImageView };

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
	GfxDevice gd = {};

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

	VK_CHECK_RESULT ( vkCreateInstance( &instanceCreateInfo, VULKAN_ALLOCATORS, &gd.instance ) );


	// Load the instance-related Vulkan function pointers
	volkLoadInstanceOnly(gd.instance);


	// Report callback
	if ( vkCreateDebugReportCallbackEXT )
	{
		gd.support.debugReportCallbacks = true;

		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		//debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
		debugReportCallbackCreateInfo.pfnCallback = VulkanDebugReportCallback;
		debugReportCallbackCreateInfo.pUserData = 0;

		VK_CHECK_RESULT( vkCreateDebugReportCallbackEXT( gd.instance, &debugReportCallbackCreateInfo, VULKAN_ALLOCATORS, &gd.debugReportCallback) );
	}


#if VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = window.connection;
	surfaceCreateInfo.window = window.window;
	VK_CHECK_RESULT( vkCreateXcbSurfaceKHR( gd.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gd.surface ) );
#elif VK_USE_PLATFORM_ANDROID_KHR
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = window.window; // ANativeWindow
	VK_CHECK_RESULT( vkCreateAndroidSurfaceKHR( gd.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gd.surface ) );
#elif VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = window.hInstance;
	surfaceCreateInfo.hwnd = window.hWnd;
	VK_CHECK_RESULT( vkCreateWin32SurfaceKHR( gd.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gd.surface ) );
#endif


	// List of physical devices
	uint32_t physicalDeviceCount = 0;
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( gd.instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CHECK_RESULT( vkEnumeratePhysicalDevices( gd.instance, &physicalDeviceCount, physicalDevices ) );

	const char *requiredDeviceExtensionNames[] = {
		VK_KHR_SWAPCHAIN_EXTENSION_NAME,
	};


	// Data to discover from the physical device selection
	bool suitableDeviceFound = false;

	// Physical device selection
	for (u32 i = 0; i < physicalDeviceCount; ++i)
	{
		VkPhysicalDevice physicalDevice = physicalDevices[i];

		Arena scratch2 = MakeSubArena(scratch);

		#if !PLATFORM_ANDROID
		// We only want dedicated GPUs
		VkPhysicalDeviceProperties properties;
		vkGetPhysicalDeviceProperties( physicalDevice, &properties );
		if ( properties.deviceType != VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU )
			continue;
		#endif

		VkPhysicalDeviceFeatures features;
		vkGetPhysicalDeviceFeatures( physicalDevice, &features );
		// Check any needed features here
		if ( features.samplerAnisotropy == VK_FALSE )
			continue;

		// Check the available queue families
		uint32_t queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilies = PushArray( scratch2, VkQueueFamilyProperties, queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilies );

		uint32_t gfxFamilyIndex = -1;
		uint32_t presentFamilyIndex = -1;
		for ( uint32_t i = 0; i < queueFamilyCount; ++i )
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, gd.surface, &presentSupport );
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
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, NULL ) );
		VkExtensionProperties *deviceExtensions = PushArray( scratch2, VkExtensionProperties, deviceExtensionCount );
		VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

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
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, gd.surface, &surfaceFormatCount, NULL);
		if ( surfaceFormatCount == 0 )
			continue;
		VkSurfaceFormatKHR *surfaceFormats = PushArray( scratch2, VkSurfaceFormatKHR, surfaceFormatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, gd.surface, &surfaceFormatCount, surfaceFormats);

		gd.swapchainFormat = VK_FORMAT_MAX_ENUM;
		for ( u32 i = 0; i < surfaceFormatCount; ++i )
		{
			if ( surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB &&
					surfaceFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
			{
				gd.swapchainFormat = surfaceFormats[i].format;
				gd.swapchainColorSpace = surfaceFormats[i].colorSpace;
				break;
			}
		}
		if ( gd.swapchainFormat == VK_FORMAT_MAX_ENUM )
		{
			gd.swapchainFormat = surfaceFormats[0].format;
			gd.swapchainColorSpace = surfaceFormats[0].colorSpace;
		}

		// Swapchain present mode
		uint32_t surfacePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, gd.surface, &surfacePresentModeCount, NULL);
		if ( surfacePresentModeCount == 0 )
			continue;
		VkPresentModeKHR *surfacePresentModes = PushArray( scratch2, VkPresentModeKHR, surfacePresentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, gd.surface, &surfacePresentModeCount, surfacePresentModes);

#if USE_SWAPCHAIN_MAILBOX_PRESENT_MODE
		gd.swapchainPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		for ( u32 i = 0; i < surfacePresentModeCount; ++i )
		{
			if ( surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				gd.swapchainPresentMode = surfacePresentModes[i];
			}
		}
		if ( gd.swapchainPresentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			gd.swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
		gd.swapchainPresentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

		// At this point, we know this device meets all the requirements
		suitableDeviceFound = true;
		gd.physicalDevice = physicalDevice;
		gd.graphicsQueueFamilyIndex = gfxFamilyIndex;
		gd.presentQueueFamilyIndex = presentFamilyIndex;
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
	queueCreateInfos[0].queueFamilyIndex = gd.graphicsQueueFamilyIndex;
	queueCreateInfos[0].queueCount = queueCount;
	queueCreateInfos[0].pQueuePriorities = queuePriorities;
	queueCreateInfos[1].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[1].queueFamilyIndex = gd.presentQueueFamilyIndex;
	queueCreateInfos[1].queueCount = queueCount;
	queueCreateInfos[1].pQueuePriorities = queuePriorities;

#if 0
	uint32_t deviceExtensionCount;
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( gd.physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CHECK_RESULT( vkEnumerateDeviceExtensionProperties( gd.physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

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

	VkPhysicalDeviceFeatures requiredPhysicalDeviceFeatures = {};
	requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = ARRAY_COUNT(queueCreateInfos);
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = NULL;
	deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames;
	deviceCreateInfo.pEnabledFeatures = &requiredPhysicalDeviceFeatures;

	result = vkCreateDevice( gd.physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &gd.device );
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "vkCreateDevice failed!\n");
		return false;
	}

	// Load all the remaining device-related Vulkan function pointers
	volkLoadDevice(gd.device);


	// Retrieve queues
	vkGetDeviceQueue(gd.device, gd.graphicsQueueFamilyIndex, 0, &gd.graphicsQueue);

	vkGetDeviceQueue(gd.device, gd.presentQueueFamilyIndex, 0, &gd.presentQueue);


	// Create render passes
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = gd.swapchainFormat;
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

	VkAttachmentDescription depthAttachmentDesc = {};
	depthAttachmentDesc.format = FindDepthFormat(gd);
	depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
	depthAttachmentDesc.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
	depthAttachmentDesc.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
	depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
	depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	VkAttachmentReference depthAttachmentRef = {};
	depthAttachmentRef.attachment = 1;
	depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

	const VkAttachmentDescription attachments[] = { colorAttachmentDesc, depthAttachmentDesc };

	VkSubpassDescription subpassDesc = {};
	subpassDesc.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
	subpassDesc.colorAttachmentCount = 1;
	subpassDesc.pColorAttachments = &colorAttachmentRef;
	subpassDesc.pDepthStencilAttachment = &depthAttachmentRef;

	VkSubpassDependency subpassDependency = {};
	subpassDependency.srcSubpass = VK_SUBPASS_EXTERNAL;
	subpassDependency.dstSubpass = 0;
	subpassDependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.srcAccessMask = 0;
	subpassDependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	subpassDependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

	VkRenderPassCreateInfo renderPassCreateInfo = {};
	renderPassCreateInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
	renderPassCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
	renderPassCreateInfo.pAttachments = attachments;
	renderPassCreateInfo.subpassCount = 1;
	renderPassCreateInfo.pSubpasses = &subpassDesc;
	renderPassCreateInfo.dependencyCount = 1;
	renderPassCreateInfo.pDependencies = &subpassDependency;

	VK_CHECK_RESULT( vkCreateRenderPass( gd.device, &renderPassCreateInfo, VULKAN_ALLOCATORS, &gd.renderPass ) );


	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = NULL;
	VK_CHECK_RESULT( vkCreatePipelineCache( gd.device, &pipelineCacheCreateInfo, VULKAN_ALLOCATORS, &gd.pipelineCache ) );


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
	VK_CHECK_RESULT( vkCreateDescriptorPool( gd.device, &descriptorPoolCreateInfo, VULKAN_ALLOCATORS, &gd.imGuiDescriptorPool ) );
#endif


	// Create descriptor set layout
	// TODO: This shouldn't be part of the device initialization
	VkDescriptorSetLayoutBinding bindings[] = {
		{
			0, // binding
			VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, // descriptorType
			1, // descriptorCount
			VK_SHADER_STAGE_VERTEX_BIT, // stageFlags
			NULL // pImmutableSamplers
		},
		{
			1, // binding
			VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, // descriptorType
			1, // descriptorCount
			VK_SHADER_STAGE_FRAGMENT_BIT, // stageFlags
			NULL // pImmutableSamplers
		}
	};

	VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
	descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
	descriptorSetLayoutCreateInfo.bindingCount = ARRAY_COUNT(bindings);
	descriptorSetLayoutCreateInfo.pBindings = bindings;
	VK_CHECK_RESULT( vkCreateDescriptorSetLayout(gd.device, &descriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &gd.descriptorSetLayout) );

	// Create pipeline
	// TODO: This shouldn't be part of the device initialization
	// but let's put it here for now
	gd.pipeline = CreatePipeline(gd, scratch);


	// Command pool
	VkCommandPoolCreateInfo commandPoolCreateInfo = {};
	commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	commandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
	commandPoolCreateInfo.queueFamilyIndex = gd.graphicsQueueFamilyIndex;

	VK_CHECK_RESULT( vkCreateCommandPool(gd.device, &commandPoolCreateInfo, VULKAN_ALLOCATORS, &gd.commandPool) );


	// Command buffer
	VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
	commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	commandBufferAllocInfo.commandPool = gd.commandPool;
	commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	commandBufferAllocInfo.commandBufferCount = MAX_FRAMES_IN_FLIGHT;

	VK_CHECK_RESULT( vkAllocateCommandBuffers( gd.device, &commandBufferAllocInfo, gd.commandBuffers) );


	// Transient command pool
	VkCommandPoolCreateInfo transientCommandPoolCreateInfo = {};
	transientCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transientCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transientCommandPoolCreateInfo.queueFamilyIndex = gd.graphicsQueueFamilyIndex;

	VkCommandPool transientCommandPool;
	VK_CHECK_RESULT( vkCreateCommandPool(gd.device, &transientCommandPoolCreateInfo, VULKAN_ALLOCATORS, &gd.transientCommandPool) );


	// Create swapchain
	CreateSwapchain( gd, window );


	// Synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		VK_CHECK_RESULT( vkCreateSemaphore( gd.device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gd.imageAvailableSemaphores[i] ) );
		VK_CHECK_RESULT( vkCreateSemaphore( gd.device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gd.renderFinishedSemaphores[i] ) );
		VK_CHECK_RESULT( vkCreateFence( gd.device, &fenceCreateInfo, VULKAN_ALLOCATORS, &gd.inFlightFences[i] ) );
	}


	// Create a vertex buffer
	Buffer vertexBuffer = CreateVertexBuffer(gd, cubeVertices, sizeof(cubeVertices));
	gd.vertexBuffer = vertexBuffer.buffer;
	gd.vertexBufferMemory = vertexBuffer.memory;

	// Create a index buffer
	Buffer indexBuffer = CreateIndexBuffer(gd, cubeIndices, sizeof(cubeIndices));
	gd.indexBuffer = indexBuffer.buffer;
	gd.indexBufferMemory = indexBuffer.memory;

	// Create uniform buffers
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		Buffer uniformBuffer = CreateBuffer(
			gd,
			sizeof(VertexTransforms),
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT );
		gd.uniformBuffers[i] = uniformBuffer.buffer;
		gd.uniformBuffersMemory[i] = uniformBuffer.memory;

		vkMapMemory(gd.device, gd.uniformBuffersMemory[i], 0, sizeof(VertexTransforms), 0, &gd.uniformBuffersMapped[i]);
	}

	// Create texture
	CreateTextureImage(gd);
	CreateTextureImageView(gd);
	CreateTextureSampler(gd);

	// Create Descriptor Pool
	VkDescriptorPoolSize poolSizes[] = {
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<u32>(MAX_FRAMES_IN_FLIGHT) },
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, static_cast<u32>(MAX_FRAMES_IN_FLIGHT) }
	};

	VkDescriptorPoolCreateInfo poolCreateInfo = {};
	poolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
	poolCreateInfo.poolSizeCount = ARRAY_COUNT(poolSizes);
	poolCreateInfo.pPoolSizes = poolSizes;
	poolCreateInfo.maxSets = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);
	//poolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;

	VK_CHECK_RESULT( vkCreateDescriptorPool(gd.device, &poolCreateInfo, VULKAN_ALLOCATORS, &gd.descriptorPool) );

	// DescriptorSets
	VkDescriptorSetLayout descriptorSetLayouts[MAX_FRAMES_IN_FLIGHT] = {};
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i) descriptorSetLayouts[i] = gd.descriptorSetLayout;
	VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
	descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
	descriptorSetAllocateInfo.descriptorPool = gd.descriptorPool;
	descriptorSetAllocateInfo.descriptorSetCount = static_cast<u32>(MAX_FRAMES_IN_FLIGHT);
	descriptorSetAllocateInfo.pSetLayouts = descriptorSetLayouts;
	VK_CHECK_RESULT( vkAllocateDescriptorSets(gd.device, &descriptorSetAllocateInfo, gd.descriptorSets) );

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		VkDescriptorBufferInfo bufferInfo = {};
		bufferInfo.buffer = gd.uniformBuffers[i];
		bufferInfo.offset = 0;
		bufferInfo.range = sizeof(VertexTransforms);

		VkDescriptorImageInfo imageInfo = {};
		imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo.imageView = gd.textureImageView;
		imageInfo.sampler = gd.textureSampler;

		VkWriteDescriptorSet descriptorWrites[2] = {};

		descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[0].dstSet = gd.descriptorSets[i];
		descriptorWrites[0].dstBinding = 0;
		descriptorWrites[0].dstArrayElement = 0;
		descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		descriptorWrites[0].descriptorCount = 1;
		descriptorWrites[0].pBufferInfo = &bufferInfo;
		descriptorWrites[0].pImageInfo = NULL;
		descriptorWrites[0].pTexelBufferView = NULL;

		descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
		descriptorWrites[1].dstSet = gd.descriptorSets[i];
		descriptorWrites[1].dstBinding = 1;
		descriptorWrites[1].dstArrayElement = 0;
		descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		descriptorWrites[1].descriptorCount = 1;
		descriptorWrites[1].pBufferInfo = NULL;
		descriptorWrites[1].pImageInfo = &imageInfo;
		descriptorWrites[1].pTexelBufferView = NULL;

		vkUpdateDescriptorSets(gd.device, ARRAY_COUNT(descriptorWrites), descriptorWrites, 0, NULL);
	}

	// Camera
	gd.camera.position = {0, 1, 2};
	gd.camera.orientation = {0, -0.45};

	// Copy the temporary device into the output parameter
	gfxDevice = gd;

	return true;
}

void WaitDeviceIdle(GfxDevice &gfxDevice)
{
	vkDeviceWaitIdle( gfxDevice.device );
}

void CleanupSwapchain(GfxDevice &gfxDevice)
{
	WaitDeviceIdle(gfxDevice);

	vkDestroyImageView(gfxDevice.device, gfxDevice.depthImageView, VULKAN_ALLOCATORS);
	vkDestroyImage(gfxDevice.device, gfxDevice.depthImage, VULKAN_ALLOCATORS);
	vkFreeMemory(gfxDevice.device, gfxDevice.depthImageMemory, VULKAN_ALLOCATORS);

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

	vkDestroySampler( gfxDevice.device, gfxDevice.textureSampler, VULKAN_ALLOCATORS );

	vkDestroyImageView( gfxDevice.device, gfxDevice.textureImageView, VULKAN_ALLOCATORS );
	vkDestroyImage( gfxDevice.device, gfxDevice.textureImage.image, VULKAN_ALLOCATORS );
	vkFreeMemory( gfxDevice.device, gfxDevice.textureImage.memory, VULKAN_ALLOCATORS );

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

	vkDestroyPipeline( gfxDevice.device, gfxDevice.pipeline.handle, VULKAN_ALLOCATORS );

	vkDestroyPipelineLayout( gfxDevice.device, gfxDevice.pipeline.layout, VULKAN_ALLOCATORS );

	vkDestroyDescriptorSetLayout( gfxDevice.device, gfxDevice.descriptorSetLayout, VULKAN_ALLOCATORS );

	vkDestroyPipelineCache( gfxDevice.device, gfxDevice.pipelineCache, VULKAN_ALLOCATORS );

	vkDestroyCommandPool( gfxDevice.device, gfxDevice.transientCommandPool, VULKAN_ALLOCATORS );
	vkDestroyCommandPool( gfxDevice.device, gfxDevice.commandPool, VULKAN_ALLOCATORS );

	vkDestroyDescriptorPool( gfxDevice.device, gfxDevice.descriptorPool, VULKAN_ALLOCATORS );

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

float3 ForwardDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 forward = { -Sin(yaw)*Cos(pitch), Sin(pitch), -Cos(yaw)*Cos(pitch) };
	return forward;
}

float3 RightDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const float3 right = { Cos(yaw), 0.0f, -Sin(yaw) };
	return right;
}

float4x4 ViewMatrixFromCamera(const Camera &camera)
{
	const float3 forward = ForwardDirectionFromAngles(camera.orientation);
	const float3 vrp = Add(camera.position, forward);
	constexpr float3 up = {0, 1, 0};
	const float4x4 res = LookAt(vrp, camera.position, up);
	return res;
}

bool RenderGraphics(GfxDevice &gfxDevice, Window &window, f32 deltaSeconds)
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
	static f32 angle = 0.0f;
	angle += 45.0f * deltaSeconds;
	const f32 ar = static_cast<f32>(window.width) / static_cast<f32>(window.height);
	const float orthox = ar > 1.0f ? ar : 1.0f;
	const float orthoy = ar > 1.0f ? 1.0f : 1.0f/ar;
	VertexTransforms vertexTransforms;
	vertexTransforms.model = Rotate({0, 1, 0}, angle);
	//vertexTransforms.view = LookAt({0, 0, 0}, {0, 1, 2}, {0, 1, 0});
	vertexTransforms.view = ViewMatrixFromCamera(gfxDevice.camera);
	//vertexTransforms.proj = Orthogonal(-orthox, orthox, -orthoy, orthoy, -10, 10);
	vertexTransforms.proj = Perspective(60.0f, ar, 0.1f, 1000.0f);
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

	VkClearValue clearValues[2] = {};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = gfxDevice.renderPass;
	renderPassBeginInfo.framebuffer = gfxDevice.swapchainFramebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = gfxDevice.swapchainExtent;
	renderPassBeginInfo.clearValueCount = ARRAY_COUNT(clearValues);
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass( commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxDevice.pipeline.handle );

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(gfxDevice.swapchainExtent.height);
	viewport.width = static_cast<float>(gfxDevice.swapchainExtent.width);
	viewport.height = -static_cast<float>(gfxDevice.swapchainExtent.height);
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

	vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, gfxDevice.pipeline.layout, 0, 1, &gfxDevice.descriptorSets[frameIndex], 0, NULL);

	//vkCmdDraw(commandBuffer, ARRAY_COUNT(cubeVertices), 1, 0, 0);
	vkCmdDrawIndexed(commandBuffer, ARRAY_COUNT(cubeIndices), 1, 0, 0, 0);

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


#if 0
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
#endif


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

	Clock lastFrameClock = GetClock();

	// Application loop
	while ( 1 )
	{
		const Clock currentFrameClock = GetClock();
		const f32 deltaSeconds = GetSecondsElapsed(lastFrameClock, currentFrameClock);
		lastFrameClock = currentFrameClock;

		ProcessWindowEvents(window);

#define USE_CAMERA_MOVEMENT 1
#if USE_CAMERA_MOVEMENT

		// Camera rotation
		float2 angles = gfxDevice.camera.orientation;
		if (MouseButtonPressed(window.mouse, MOUSE_BUTTON_LEFT))
		{
			const f32 deltaYaw = - window.mouse.dx * ToRadians * 0.2f;
			const f32 deltaPitch = - window.mouse.dy * ToRadians * 0.2f;
			angles.x = angles.x + deltaYaw;
			angles.y = Clamp(angles.y + deltaPitch, -Pi * 0.49, Pi * 0.49);
		}
		gfxDevice.camera.orientation = angles;

		// Movement direction
		float3 dir = { 0, 0, 0 };
		if ( KeyPressed(window.keyboard, KEY_W) ) { dir = Add(dir, ForwardDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, KEY_S) ) { dir = Add(dir, Negate( ForwardDirectionFromAngles(angles) )); }
		if ( KeyPressed(window.keyboard, KEY_D) ) { dir = Add(dir, RightDirectionFromAngles(angles)); }
		if ( KeyPressed(window.keyboard, KEY_A) ) { dir = Add(dir, Negate( RightDirectionFromAngles(angles) )); }
		dir = NormalizeIfNotZero(dir);

		// Accelerated translation
		static constexpr f32 MAX_SPEED = 100.0f;
		static constexpr f32 ACCELERATION = 50.0f;
		static float3 speed = { 0, 0, 0 };
		const float3 speed0 = speed;

		// Apply acceleration, then limit speed
		speed = Add(speed, Mul(dir, ACCELERATION * deltaSeconds));
		speed = Length(speed) > MAX_SPEED ?  Mul( Normalize(speed), MAX_SPEED) : speed;

		// Based on speed, translate camera position
		const float3 translation = Add( Mul(speed0, deltaSeconds), Mul(speed, 0.5f * deltaSeconds) );
		gfxDevice.camera.position = Add(gfxDevice.camera.position, translation);

		// Apply deceleration
		speed = Mul(speed, 0.9);
#endif

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

		RenderGraphics(gfxDevice, window, deltaSeconds);
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

