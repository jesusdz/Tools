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

#define SPV_ASSERT ASSERT
#define SPV_PRINTF(...) LOG(Info, ##__VA_ARGS__)
#define SPV_IMPLEMENTATION
#define SPV_PRINT_FUNCTIONS
#include "tools_spirv.h"

#include "shaders/structs.hlsl"


#define VULKAN_ALLOCATORS NULL


#if PLATFORM_ANDROID
#define MAX_SWAPCHAIN_IMAGE_COUNT 5
#else
#define MAX_SWAPCHAIN_IMAGE_COUNT 3
#endif
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_TEXTURES 4
#define MAX_SAMPLERS 4
#define MAX_PIPELINES 8
#define MAX_SHADER_REFLECTIONS 8
#define MAX_MATERIALS 4
#define MAX_ENTITIES 32
#define MAX_SHADER_BINDINGS 8

#define DESCRIPTOR_SET_GLOBAL 0
#define DESCRIPTOR_SET_MATERIAL 1


enum HeapType
{
	HeapType_General,
	HeapType_RTs,
	HeapType_Staging,
	HeapType_Dynamic,
	HeapType_Readback,
	HeapType_COUNT,
};

struct Heap
{
	HeapType type;
	u32 size;
	u32 memoryTypeIndex;
	VkDeviceMemory memory;
	u8* data;
	u32 used;
};

struct Alloc
{
	HeapType heap;
	u64 offset;
	u64 size;
};

struct ShaderBinding
{
	u8 set;
	u8 binding;
	SpvType type;
	SpvStageFlags stageFlags;
	const char *name;
};

struct ShaderReflection
{
	ShaderBinding bindings[MAX_SHADER_BINDINGS];
	u8 bindingCount;
};

typedef u32 ShaderReflectionH;

struct Pipeline
{
	VkDescriptorSetLayout globalDescriptorSetLayout;
	VkDescriptorSetLayout materialDescriptorSetLayout;
	VkPipelineLayout layout;
	VkPipeline handle;
};

typedef u32 PipelineH;

struct Buffer
{
	VkBuffer buffer;
	Alloc alloc;
};

struct Image
{
	VkImage image;
	VkFormat format;
	Alloc alloc;
};

struct Texture
{
	Image image;
	u32 mipLevels;
	VkImageView imageView;
};

typedef u32 TextureH;

struct ShaderSource
{
	u8 *data;
	u64 dataSize;
};

struct ShaderModule
{
	VkShaderModule handle;
};

struct Material
{
	PipelineH pipelineH;
	ShaderReflectionH shaderReflectionH;
	TextureH albedoTexture;
	f32 uvScale;
	u32 bufferOffset;
};

typedef u32 MaterialH;

struct Sampler
{
	VkSampler sampler;
};

typedef u32 SamplerH;

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct SwapchainInfo
{
	VkFormat format;
	VkColorSpaceKHR colorSpace;
	VkPresentModeKHR presentMode;
};

struct Swapchain
{
	VkSwapchainKHR handle;
	VkExtent2D extent;
	u32 imageCount;
	VkImage images[MAX_SWAPCHAIN_IMAGE_COUNT];
	VkImageView imageViews[MAX_SWAPCHAIN_IMAGE_COUNT];
	float preRotationDegrees;
	bool outdated;
};

struct RenderTargets
{
	Image depthImage;
	VkImageView depthImageView;
	VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];
};

struct Camera
{
	float3 position;
	float2 orientation; // yaw and pitch
};

struct Entity
{
	float3 position;
	float scale;
	bool visible;
	Buffer *vertexBuffer;
	Buffer *indexBuffer;
	u16 materialIndex;
};

struct Alignment
{
	u32 uniformBufferOffset;
	u32 optimalBufferCopyOffset;
	u32 optimalBufferCopyRowPitch;
	// u32 nonCoherentAtomSize;
	// VkExtent3D minImageTransferGranularity;
};

struct Graphics
{
	VkInstance instance;
	VkPhysicalDevice physicalDevice;
	VkDevice device;

	Alignment alignment;

	u32 graphicsQueueFamilyIndex;
	u32 presentQueueFamilyIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
	VkCommandPool transientCommandPool;

	VkRenderPass renderPass;

	VkSurfaceKHR surface;
	SwapchainInfo swapchainInfo;
	Swapchain swapchain;

	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkFence inFlightFences[MAX_FRAMES_IN_FLIGHT];

	VkPipelineCache pipelineCache;

	Heap heaps[HeapType_COUNT];

	u32 currentFrame;

	// TODO: Temporary stuff hardcoded here

	RenderTargets renderTargets;

	Buffer stagingBuffer;
	u32 stagingBufferOffset;

	Buffer cubeVertices;
	Buffer cubeIndices;
	Buffer planeVertices;
	Buffer planeIndices;

	Buffer globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	Buffer entityBuffer[MAX_FRAMES_IN_FLIGHT];
	Buffer materialBuffer;

	SamplerH textureSampler;

	Sampler samplers[MAX_SAMPLERS];
	u32 samplerCount;

	Texture textures[MAX_TEXTURES];
	u32 textureCount;

	Pipeline pipelines[MAX_PIPELINES];
	u32 pipelineCount;

	ShaderReflection shaderReflections[MAX_SHADER_REFLECTIONS];
	u32 shaderReflectionCount;

	Material materials[MAX_MATERIALS];
	u32 materialCount;

	VkDescriptorPool globalDescriptorPool;
	VkDescriptorPool materialDescriptorPool;
#if USE_IMGUI
	VkDescriptorPool imGuiDescriptorPool;
#endif

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	VkDescriptorSet globalDescriptorSets[MAX_MATERIALS][MAX_FRAMES_IN_FLIGHT];
	// Updated once at the beginning for each material
	VkDescriptorSet materialDescriptorSets[MAX_MATERIALS];

	struct
	{
		bool debugReportCallbacks;
	} support;

	VkDebugReportCallbackEXT debugReportCallback;

	Camera camera;

	Entity entities[MAX_ENTITIES];
	u32 entityCount;

	bool deviceInitialized;
	bool sceneInitialized;
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

static const Vertex planeVertices[] = {
	{{-0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 0.0f}},
	{{ 0.5f, 0.0f,  0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 0.0f}},
	{{ 0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {1.0f, 1.0f}},
	{{-0.5f, 0.0f, -0.5f}, {0.0f, 1.0f, 0.0f}, {0.0f, 1.0f}},
};

static const u16 planeIndices[] = {
	0, 1, 2, 2, 3, 0,
};


const char *VkPhysicalDeviceTypeToString( VkPhysicalDeviceType type )
{
	static const char *toString[] = {
		"VK_PHYSICAL_DEVICE_TYPE_OTHER", // 0
		"VK_PHYSICAL_DEVICE_TYPE_INTEGRATED_GPU", // 1
		"VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU", // 2
		"VK_PHYSICAL_DEVICE_TYPE_VIRTUAL_GPU", // 3
		"VK_PHYSICAL_DEVICE_TYPE_CPU", // 4
	};
	ASSERT( type < ARRAY_COUNT(toString) );
	return toString[type];
}

#define CONCAT_FLAG(flag) if ( flags & flag ) StrCat(outString, #flag "|" );

const char *VkMemoryPropertyFlagsToString( VkMemoryPropertyFlags flags, char *outString )
{
	*outString = 0;
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT)
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_COHERENT_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_HOST_CACHED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_LAZILY_ALLOCATED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_PROTECTED_BIT);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_COHERENT_BIT_AMD);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_DEVICE_UNCACHED_BIT_AMD);
	CONCAT_FLAG(VK_MEMORY_PROPERTY_RDMA_CAPABLE_BIT_NV);
	return outString;
}

const char *VkMemoryHeapFlagsToString( VkMemoryHeapFlags flags, char *outString )
{
	*outString = 0;
	CONCAT_FLAG(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
	CONCAT_FLAG(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
	return outString;
}

const char *HeapTypeToString(HeapType heapType)
{
	static const char *toString[] = {
		"HeapType_General",
		"HeapType_RTs",
		"HeapType_Staging",
		"HeapType_Dynamic",
		"HeapType_Readback",
	};
	CT_ASSERT( ARRAY_COUNT(toString) == HeapType_COUNT );
	ASSERT( heapType < ARRAY_COUNT(toString) );
	return toString[heapType];
}

#undef CONCAT_FLAG


static VKAPI_ATTR VkBool32 VKAPI_CALL VulkanDebugReportCallback(
		VkDebugReportFlagsEXT       flags,
		VkDebugReportObjectTypeEXT  objectType,
		uint64_t                    object,
		size_t                      location,
		int32_t                     messageCode,
		const char*                 pLayerPrefix,
		const char*                 pMessage,
		void*                       pUserData)
{
	LOG(Warning, "VulkanDebugReportCallback was called.\n");
	LOG(Warning, " - pLayerPrefix: %s.\n", pLayerPrefix);
	LOG(Warning, " - pMessage: %s.\n", pMessage);
	return VK_FALSE;
}

static const char *VkResultToString(VkResult result)
{
	switch (result)
	{
		case VK_SUCCESS: return "VK_SUCCESS";
		case VK_NOT_READY: return "VK_NOT_READY";
		case VK_TIMEOUT: return "VK_TIMEOUT";
		case VK_EVENT_SET: return "VK_EVENT_SET";
		case VK_EVENT_RESET: return "VK_EVENT_RESET";
		case VK_INCOMPLETE: return "VK_INCOMPLETE";
		case VK_ERROR_OUT_OF_HOST_MEMORY: return "VK_ERROR_OUT_OF_HOST_MEMORY";
		case VK_ERROR_OUT_OF_DEVICE_MEMORY: return "VK_ERROR_OUT_OF_DEVICE_MEMORY";
		case VK_ERROR_INITIALIZATION_FAILED: return "VK_ERROR_INITIALIZATION_FAILED";
		case VK_ERROR_DEVICE_LOST: return "VK_ERROR_DEVICE_LOST";
		case VK_ERROR_MEMORY_MAP_FAILED: return "VK_ERROR_MEMORY_MAP_FAILED";
		case VK_ERROR_LAYER_NOT_PRESENT: return "VK_ERROR_LAYER_NOT_PRESENT";
		case VK_ERROR_EXTENSION_NOT_PRESENT: return "VK_ERROR_EXTENSION_NOT_PRESENT";
		case VK_ERROR_FEATURE_NOT_PRESENT: return "VK_ERROR_FEATURE_NOT_PRESENT";
		case VK_ERROR_INCOMPATIBLE_DRIVER: return "VK_ERROR_INCOMPATIBLE_DRIVER";
		case VK_ERROR_TOO_MANY_OBJECTS: return "VK_ERROR_TOO_MANY_OBJECTS";
		case VK_ERROR_FORMAT_NOT_SUPPORTED: return "VK_ERROR_FORMAT_NOT_SUPPORTED";
		case VK_ERROR_FRAGMENTED_POOL: return "VK_ERROR_FRAGMENTED_POOL";
		case VK_ERROR_UNKNOWN: return "VK_ERROR_UNKNOWN";
		default: break;
	}
	return "VK_ERROR_UNHANDLED";
}

static void CheckVulkanResult(VkResult result, const char *callString)
{
	if (result == VK_SUCCESS)
		return;
	LOG(Error, "[vulkan] VkResult error:\n");
	LOG(Error, "[vulkan] - errorCode: %d\n", result);
	LOG(Error, "[vulkan] - errorString: %s\n", VkResultToString(result));
	LOG(Error, "[vulkan] - callString: %s\n", callString);
	if (result < VK_SUCCESS)
		QUIT_ABNORMALLY();
}

#if USE_IMGUI
static void CheckVulkanResultImGui(VkResult result)
{
	CheckVulkanResult(result, "ImGui");
}
#endif

#define VK_CALL( call ) CheckVulkanResult( call, #call )

VkShaderModule CreateShaderModule( VkDevice device, byte *data, u32 size )
{
	VkShaderModuleCreateInfo createInfo{};
	createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
	createInfo.codeSize = size;
	createInfo.pCode = reinterpret_cast<const u32*>(data);

	VkShaderModule shaderModule;
	if ( vkCreateShaderModule(device, &createInfo, VULKAN_ALLOCATORS, &shaderModule) != VK_SUCCESS )
	{
		LOG(Error, "Error in CreateShaderModule.\n");
		shaderModule = VK_NULL_HANDLE;
	}

	return shaderModule;
}

VkMemoryPropertyFlags HeapTypeToVkMemoryPropertyFlags( HeapType heapType )
{
	static const VkMemoryPropertyFlags flags[] = {
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // HeapType_General,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, // HeapType_RTs,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // HeapType_Staging,
		VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT | VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, // HeapType_Dynamic,
		VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_CACHED_BIT, // HeapType_Readback,
	};
	CT_ASSERT( ARRAY_COUNT(flags) == HeapType_COUNT );
	return flags[heapType];
};

Heap CreateHeap(const Graphics &gfx, HeapType heapType, u32 size, bool mapMemory)
{
	u32 memoryTypeIndex = -1;

	VkMemoryPropertyFlags requiredFlags = HeapTypeToVkMemoryPropertyFlags( heapType );

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(gfx.physicalDevice, &memoryProperties);

	for ( u32 i = 0; i < memoryProperties.memoryTypeCount; ++i )
	{
		const VkMemoryType &memoryType = memoryProperties.memoryTypes[i];

		if ( ( memoryType.propertyFlags & requiredFlags ) == requiredFlags )
		{
			memoryTypeIndex = i;
			break;
		}
	}

	VkMemoryAllocateInfo memoryAllocateInfo = {};
	memoryAllocateInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
	memoryAllocateInfo.allocationSize = size;
	memoryAllocateInfo.memoryTypeIndex = memoryTypeIndex;

	VkDeviceMemory memory;
	VK_CALL( vkAllocateMemory(gfx.device, &memoryAllocateInfo, VULKAN_ALLOCATORS, &memory) );

	u8 *data = 0;
	if ( mapMemory )
	{
		VK_CALL( vkMapMemory(gfx.device, memory, 0, size, 0, (void**)&data) );
	}

	Heap heap = {};
	heap.type = heapType;
	heap.size = size;
	heap.memoryTypeIndex = memoryTypeIndex;
	heap.memory = memory;
	heap.data = data;
	return heap;
}

Buffer CreateBuffer(Graphics &gfx, u32 size, VkBufferUsageFlags bufferUsageFlags, Heap &memoryHeap)
{
	// Buffer
	VkBufferCreateInfo bufferCreateInfo = {};
	bufferCreateInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferCreateInfo.size = size;
	bufferCreateInfo.usage = bufferUsageFlags;
	bufferCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

	VkBuffer buffer;
	VK_CALL( vkCreateBuffer(gfx.device, &bufferCreateInfo, VULKAN_ALLOCATORS, &buffer) );

	// Memory
	VkDeviceMemory memory = memoryHeap.memory;
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(gfx.device, buffer, &memoryRequirements);
	VkDeviceSize offset = AlignUp( memoryHeap.used, memoryRequirements.alignment );
	ASSERT( offset + memoryRequirements.size <= memoryHeap.size );
	memoryHeap.used = offset + memoryRequirements.size;

	VK_CALL( vkBindBufferMemory(gfx.device, buffer, memory, offset) );

	Alloc alloc = {
		memoryHeap.type,
		offset,
		memoryRequirements.size,
	};

	Buffer gfxBuffer = {
		buffer,
		alloc,
	};

	return gfxBuffer;
}

VkCommandBuffer BeginTransientCommandBuffer(const Graphics &gfx)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = gfx.transientCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(gfx.device, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	return commandBuffer;
}

void EndTransientCommandBuffer(Graphics &gfx, VkCommandBuffer commandBuffer)
{
	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(gfx.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
	vkQueueWaitIdle(gfx.graphicsQueue);

	vkFreeCommandBuffers(gfx.device, gfx.transientCommandPool, 1, &commandBuffer);

	gfx.stagingBufferOffset = 0;
}

void CopyBufferToBuffer(Graphics &gfx, VkBuffer srcBuffer, u32 srcOffset, VkBuffer dstBuffer, u32 dstOffset, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfx);

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

	EndTransientCommandBuffer(gfx, commandBuffer);
}

struct StagedData
{
	VkBuffer buffer;
	u32 offset;
};

StagedData StageData(Graphics &gfx, const void *data, u32 size)
{
	StagedData staging = {};
	staging.buffer = gfx.stagingBuffer.buffer;
	staging.offset = gfx.stagingBuffer.alloc.offset + gfx.stagingBufferOffset;

	Heap &stagingHeap = gfx.heaps[HeapType_Staging];
	void* stagingData = stagingHeap.data + staging.offset;
	MemCopy(stagingData, data, (size_t) size);

	gfx.stagingBufferOffset = AlignUp(gfx.stagingBufferOffset + size, gfx.alignment.optimalBufferCopyOffset);

	return staging;
}

Buffer CreateStagingBuffer(Graphics &gfx)
{
	Heap &stagingHeap = gfx.heaps[HeapType_Staging];
	Buffer stagingBuffer = CreateBuffer(gfx, stagingHeap.size, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, stagingHeap);
	return stagingBuffer;
}

Buffer CreateBufferWithData(Graphics &gfx, const void *data, u32 size, VkBufferUsageFlags usage)
{
	StagedData staged = StageData(gfx, data, size);

	// Create a buffer in device local memory
	Buffer finalBuffer = CreateBuffer(
			gfx,
			size,
			VK_BUFFER_USAGE_TRANSFER_DST_BIT | usage,
			gfx.heaps[HeapType_General]);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(gfx, staged.buffer, staged.offset, finalBuffer.buffer, 0, size);

	return finalBuffer;
}

Buffer CreateVertexBuffer(Graphics &gfx, const void *data, u32 size )
{
	Buffer vertexBuffer = CreateBufferWithData(gfx, data, size, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	return vertexBuffer;
}


Buffer CreateIndexBuffer(Graphics &gfx, const void *data, u32 size )
{
	Buffer indexBuffer = CreateBufferWithData(gfx, data, size, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	return indexBuffer;
}

static VkDescriptorType SpvDescriptorTypeToVulkan(SpvType type)
{
	switch (type) {
		case SpvTypeImage: return VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
		case SpvTypeSampler: return VK_DESCRIPTOR_TYPE_SAMPLER;
		case SpvTypeSampledImage: return VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
		case SpvTypeUniformBuffer: return VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
		case SpvTypeStorageBuffer: return VK_DESCRIPTOR_TYPE_STORAGE_BUFFER; // Should be DYNAMIC if we use dynamic offsets
		default: INVALID_CODE_PATH();
	}
	return VK_DESCRIPTOR_TYPE_MAX_ENUM;
}

static VkShaderStageFlags SpvStageFlagsToVulkan(u8 stageFlags)
{
	VkShaderStageFlags vkStageFlags = 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsVertexBit ) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsFragmentBit ) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	return vkStageFlags;
}

ShaderSource GetShaderSource(Arena &arena, const char *filename)
{
	FilePath shaderPath = MakePath(filename);
	DataChunk *chunk = PushFile( arena, shaderPath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open shader file %s.\n", shaderPath.str );
		QUIT_ABNORMALLY();
	}
	ShaderSource shaderSource = { chunk->data, chunk->size };
	return shaderSource;
}

ShaderModule CreateShaderModule(const Graphics &gfx, const ShaderSource &source)
{
	VkShaderModule moduleHandle = CreateShaderModule( gfx.device, source.data, source.dataSize );

	ShaderModule shaderModule = {};
	shaderModule.handle = moduleHandle;
	return shaderModule;
}

void DestroyShaderModule(const Graphics &gfx, const ShaderModule &module)
{
	vkDestroyShaderModule(gfx.device, module.handle, VULKAN_ALLOCATORS);
}

ShaderReflectionH CreateShaderReflection( Graphics &gfx, Arena scratch, const ShaderSource &vertexSource, const ShaderSource &fragmentSource )
{
	ShaderReflection reflection = {};

	void *tempMem = scratch.base + scratch.used;
	const u32 tempMemSize = scratch.size - scratch.used;

	SpvParser parserForVertex = SpvParserInit( vertexSource.data, vertexSource.dataSize );
	SpvParser parserForFragment = SpvParserInit( fragmentSource.data, fragmentSource.dataSize );
	SpvDescriptorSetList spvDescriptorList = {};
	SpvParseDescriptors( &parserForVertex, &spvDescriptorList, tempMem, tempMemSize );
	SpvParseDescriptors( &parserForFragment, &spvDescriptorList, tempMem, tempMemSize );

	for (u32 setIndex = 0; setIndex < SPV_MAX_DESCRIPTOR_SETS; ++setIndex)
	{
		for (u32 bindingIndex = 0; bindingIndex < SPV_MAX_DESCRIPTORS_PER_SET; ++bindingIndex)
		{
			SpvDescriptor &descriptor = spvDescriptorList.sets[setIndex].bindings[bindingIndex];

			if ( descriptor.type != SpvTypeNone )
			{
				ASSERT( reflection.bindingCount < ARRAY_COUNT(reflection.bindings) );
				ShaderBinding &shaderBinding = reflection.bindings[reflection.bindingCount++];
				shaderBinding.binding = descriptor.binding;
				shaderBinding.set = setIndex;
				shaderBinding.type = (SpvType)descriptor.type;
				shaderBinding.stageFlags = descriptor.stageFlags;
				shaderBinding.name = InternString( descriptor.name );
				//LOG(Info, "Descriptor name: %s\n", descriptor.name);
			}
		}
	}

	ASSERT( gfx.shaderReflectionCount < ARRAY_COUNT(gfx.shaderReflections) );
	ShaderReflectionH shaderReflectionHandle = gfx.shaderReflectionCount++;
	gfx.shaderReflections[shaderReflectionHandle] = reflection;
	return shaderReflectionHandle;
}

const ShaderReflection &GetShaderReflection( const Graphics &gfx, ShaderReflectionH handle )
{
	const ShaderReflection &reflection = gfx.shaderReflections[handle];
	return reflection;
}

PipelineH CreatePipeline(Graphics &gfx, Arena &arena, const ShaderModule &vertexModule, const ShaderModule &fragmentModule, const ShaderReflection &shaderReflection)
{
	Arena scratch = MakeSubArena(arena);

	VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {};
	vertexShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	vertexShaderStageCreateInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
	vertexShaderStageCreateInfo.module = vertexModule.handle;
	vertexShaderStageCreateInfo.pName = "main";

	VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {};
	fragmentShaderStageCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
	fragmentShaderStageCreateInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
	fragmentShaderStageCreateInfo.module = fragmentModule.handle;
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

	// Pipeline layout
	VkDescriptorSetLayout descriptorSetLayouts[SPV_MAX_DESCRIPTOR_SETS] = {};
	u32 descriptorSetLayoutCount = 0;

	for (u32 set = 0; set < SPV_MAX_DESCRIPTOR_SETS; ++set)
	{
		VkDescriptorSetLayoutBinding bindings[SPV_MAX_DESCRIPTORS_PER_SET] = {};
		u32 bindingCount = 0;

		for (u32 bindingIndex = 0; bindingIndex < shaderReflection.bindingCount; ++bindingIndex)
		{
			const ShaderBinding &shaderBinding = shaderReflection.bindings[bindingIndex];

			if ( shaderBinding.set == set )
			{
				VkDescriptorSetLayoutBinding &binding = bindings[bindingCount++];
				binding.binding = shaderBinding.binding;
				binding.descriptorType = SpvDescriptorTypeToVulkan((SpvType)shaderBinding.type);
				binding.descriptorCount = 1;
				binding.stageFlags = SpvStageFlagsToVulkan(shaderBinding.stageFlags);
				binding.pImmutableSamplers = NULL;
			}
		}

		if (bindingCount > 0)
		{
			VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {};
			descriptorSetLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
			descriptorSetLayoutCreateInfo.bindingCount = bindingCount;
			descriptorSetLayoutCreateInfo.pBindings = bindings;
			VK_CALL( vkCreateDescriptorSetLayout(gfx.device, &descriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &descriptorSetLayouts[descriptorSetLayoutCount++]) );
		}
	}

	VkDescriptorSetLayout globalDescriptorSetLayout = descriptorSetLayouts[0];
	VkDescriptorSetLayout materialDescriptorSetLayout = descriptorSetLayouts[1];

	VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {};
	pipelineLayoutCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
	pipelineLayoutCreateInfo.setLayoutCount = descriptorSetLayoutCount;
	pipelineLayoutCreateInfo.pSetLayouts = descriptorSetLayouts;

	VkPipelineLayout pipelineLayout;
	VK_CALL( vkCreatePipelineLayout(gfx.device, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

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
	graphicsPipelineCreateInfo.renderPass = gfx.renderPass;
	graphicsPipelineCreateInfo.subpass = 0;
	graphicsPipelineCreateInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
	graphicsPipelineCreateInfo.basePipelineIndex = -1; // Optional

	VkPipeline vkPipelineHandle;
	VK_CALL( vkCreateGraphicsPipelines( gfx.device, gfx.pipelineCache, 1, &graphicsPipelineCreateInfo, VULKAN_ALLOCATORS, &vkPipelineHandle ) );

	ASSERT( gfx.pipelineCount < ARRAY_COUNT(gfx.pipelines) );
	PipelineH pipelineHandle = gfx.pipelineCount++;
	gfx.pipelines[pipelineHandle].globalDescriptorSetLayout = globalDescriptorSetLayout;
	gfx.pipelines[pipelineHandle].materialDescriptorSetLayout = materialDescriptorSetLayout;
	gfx.pipelines[pipelineHandle].layout = pipelineLayout;
	gfx.pipelines[pipelineHandle].handle = vkPipelineHandle;
	return pipelineHandle;
}

const Pipeline &GetPipeline(const Graphics &gfx, PipelineH handle)
{
	const Pipeline &pipeline = gfx.pipelines[handle];
	return pipeline;
}

Image CreateImage(const Graphics &gfx, u32 width, u32 height, u32 mipLevels, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, Heap &memoryHeap)
{
	// Image
	VkImageCreateInfo imageCreateInfo = {};
	imageCreateInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
	imageCreateInfo.imageType = VK_IMAGE_TYPE_2D;
	imageCreateInfo.extent.width = width;
	imageCreateInfo.extent.height = height;
	imageCreateInfo.extent.depth = 1;
	imageCreateInfo.mipLevels = mipLevels;
	imageCreateInfo.arrayLayers = 1;
	imageCreateInfo.format = format;
	imageCreateInfo.tiling = tiling;
	imageCreateInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	imageCreateInfo.usage = usage;
	imageCreateInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;
	imageCreateInfo.samples = VK_SAMPLE_COUNT_1_BIT;
	imageCreateInfo.flags = 0;

	VkImage image;
	VK_CALL( vkCreateImage(gfx.device, &imageCreateInfo, VULKAN_ALLOCATORS, &image) );

	// Memory
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(gfx.device, image, &memoryRequirements);
	VkDeviceMemory memory = memoryHeap.memory;
	VkDeviceSize offset = AlignUp( memoryHeap.used, memoryRequirements.alignment );
	ASSERT( offset + memoryRequirements.size < memoryHeap.size );
	memoryHeap.used = offset + memoryRequirements.size;

	VK_CALL( vkBindImageMemory(gfx.device, image, memory, offset) );

	Alloc alloc = {
		memoryHeap.type,
		offset,
		memoryRequirements.size,
	};
	Image imageStruct = {
		image,
		format,
		alloc,
	};
	return imageStruct;
}

VkImageView CreateImageView(const Graphics &gfx, VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, u32 mipLevels)
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
	viewInfo.subresourceRange.levelCount = mipLevels;
	viewInfo.subresourceRange.baseArrayLayer = 0;
	viewInfo.subresourceRange.layerCount = 1;

	VkImageView imageView;
	VK_CALL( vkCreateImageView(gfx.device, &viewInfo, VULKAN_ALLOCATORS, &imageView) );
	return imageView;
}

SamplerH CreateSampler(Graphics &gfx)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(gfx.physicalDevice, &properties);

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
	samplerCreateInfo.maxLod = VK_LOD_CLAMP_NONE;

	VkSampler vkSampler;
	VK_CALL( vkCreateSampler(gfx.device, &samplerCreateInfo, VULKAN_ALLOCATORS, &vkSampler) );

	Sampler sampler = {vkSampler};

	ASSERT( gfx.samplerCount < ARRAY_COUNT( gfx.samplers ) );
	SamplerH samplerHandle = gfx.samplerCount++;
	gfx.samplers[samplerHandle] = sampler;
	return samplerHandle;
}

const Sampler &GetSampler(const Graphics &gfx, SamplerH handle)
{
	const Sampler &sampler = gfx.samplers[handle];
	return sampler;
}

bool HasStencilComponent(VkFormat format)
{
	const bool hasStencil =
		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
		format == VK_FORMAT_D24_UNORM_S8_UINT;
	return hasStencil;
}

void TransitionImageLayout(Graphics &gfx, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfx);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.oldLayout = oldLayout;
	barrier.newLayout = newLayout;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.image = image;
	barrier.subresourceRange.baseMipLevel = 0;
	barrier.subresourceRange.levelCount = mipLevels;
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

	EndTransientCommandBuffer(gfx, commandBuffer);
}

void CopyBufferToImage(Graphics &gfx, VkBuffer buffer, u32 bufferOffset, VkImage image, u32 width, u32 height)
{
	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfx);

	VkBufferImageCopy region{};
	region.bufferOffset = bufferOffset;
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

	EndTransientCommandBuffer(gfx, commandBuffer);
}

void GenerateMipmaps(Graphics &gfx, VkImage image, VkFormat format, i32 width, i32 height, u32 mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(gfx.physicalDevice, format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		LOG(Error, "GenerateMipmaps() - Linera filtering not supported for the given format.\n");
		QUIT_ABNORMALLY();
	}

	VkCommandBuffer commandBuffer = BeginTransientCommandBuffer(gfx);

	VkImageMemoryBarrier barrier = {};
	barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
	barrier.image = image;
	barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
	barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
	barrier.subresourceRange.baseArrayLayer = 0;
	barrier.subresourceRange.layerCount = 1;
	barrier.subresourceRange.levelCount = 1;

	i32 mipWidth = width;
	i32 mipHeight = height;

	for (u32 i = 1; i < mipLevels; ++i)
	{
		barrier.subresourceRange.baseMipLevel = i - 1;
		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
				0, NULL,
				0, NULL,
				1, &barrier);

		const i32 dstMipWidth = mipWidth > 1 ? mipWidth / 2 : 1;
		const i32 dstMipHeight = mipHeight > 1 ? mipHeight / 2 : 1;

		VkImageBlit blit = {};
		blit.srcOffsets[0] = { 0, 0, 0 };
		blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
		blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.srcSubresource.mipLevel = i - 1;
		blit.srcSubresource.baseArrayLayer = 0;
		blit.srcSubresource.layerCount = 1;
		blit.dstOffsets[0] = { 0, 0, 0 };
		blit.dstOffsets[1] = { dstMipWidth, dstMipHeight, 1 };
		blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
		blit.dstSubresource.mipLevel = i;
		blit.dstSubresource.baseArrayLayer = 0;
		blit.dstSubresource.layerCount = 1;

		vkCmdBlitImage(commandBuffer,
				image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
				image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
				1, &blit,
				VK_FILTER_LINEAR);

		barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
		barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		vkCmdPipelineBarrier(commandBuffer,
				VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
				0, nullptr,
				0, nullptr,
				1, &barrier);

		mipWidth = dstMipWidth;
		mipHeight = dstMipHeight;
	}

	barrier.subresourceRange.baseMipLevel = mipLevels - 1;
	barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
	barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
	barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
	barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

	vkCmdPipelineBarrier(commandBuffer,
			VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
			0, nullptr,
			0, nullptr,
			1, &barrier);

	EndTransientCommandBuffer(gfx, commandBuffer);
}

TextureH CreateTexture(Graphics &gfx, const char *filePath)
{
	int texWidth, texHeight, texChannels;
	FilePath imagePath = MakePath(filePath);
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

	const u32 size = texWidth * texHeight * 4;

	StagedData staged = StageData(gfx, pixels, size);

	if ( originalPixels )
	{
		stbi_image_free(originalPixels);
	}

	const u32 mipLevels = static_cast<uint32_t>(Floor(Log2(Max(texWidth, texHeight)))) + 1;

	Image image = CreateImage(gfx,
			texWidth, texHeight, mipLevels,
			VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_TRANSFER_SRC_BIT | // for mipmap blits
			VK_IMAGE_USAGE_TRANSFER_DST_BIT | // for intitial copy from buffer and blits
			VK_IMAGE_USAGE_SAMPLED_BIT, // to be sampled in shaders
			gfx.heaps[HeapType_General]);

	TransitionImageLayout(gfx, image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);
	CopyBufferToImage(gfx, staged.buffer, staged.offset, image.image, texWidth, texHeight);

	// GenerateMipmaps takes care of this transition after generating the mip levels
	//TransitionImageLayout(gfx, image.image, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

	GenerateMipmaps(gfx, image.image, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	ASSERT( gfx.textureCount < ARRAY_COUNT(gfx.textures) );
	TextureH textureHandle = gfx.textureCount++;
	gfx.textures[textureHandle].image = image;
	gfx.textures[textureHandle].imageView = CreateImageView(gfx, image.image, image.format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	gfx.textures[textureHandle].mipLevels = mipLevels;
	return textureHandle;
}

Texture &GetTexture(Graphics &gfx, TextureH handle)
{
	Texture &texture = gfx.textures[handle];
	return texture;
}

MaterialH CreateMaterial( Graphics &gfx, PipelineH pipelineHandle, TextureH textureHandle, f32 uvScale = 1.0f )
{
	ASSERT(gfx.materialCount < MAX_MATERIALS);
	MaterialH materialHandle = gfx.materialCount++;
	gfx.materials[materialHandle].pipelineH = pipelineHandle;
	gfx.materials[materialHandle].albedoTexture = textureHandle;
	gfx.materials[materialHandle].uvScale = uvScale;
	gfx.materials[materialHandle].bufferOffset = materialHandle * AlignUp(sizeof(SMaterial), gfx.alignment.uniformBufferOffset);
	return materialHandle;
}

Material &GetMaterial( Graphics &gfx, MaterialH materialHandle )
{
	Material &material = gfx.materials[materialHandle];
	return material;
}

void CreateEntity(Graphics &gfx, float3 position, f32 scale, Buffer *vertexBuffer, Buffer *indexBuffer, u32 materialIndex)
{
	ASSERT ( gfx.entityCount < MAX_ENTITIES );
	if ( gfx.entityCount < MAX_ENTITIES )
	{
		const u32 entityIndex = gfx.entityCount++;
		gfx.entities[entityIndex].visible = true;
		gfx.entities[entityIndex].position = position;
		gfx.entities[entityIndex].scale = scale;
		gfx.entities[entityIndex].vertexBuffer = vertexBuffer;
		gfx.entities[entityIndex].indexBuffer = indexBuffer;
		gfx.entities[entityIndex].materialIndex = materialIndex;
	}
	else
	{
		LOG(Warning, "CreateEntity() - MAX_ENTITIES limit reached.\n");
	}
}

VkFormat FindSupportedFormat(const Graphics &gfx, const VkFormat candidates[], u32 candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (u32 i = 0; i < candidateCount; ++i)
	{
		VkFormat format = candidates[i];
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(gfx.physicalDevice, format, &properties);

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

VkFormat FindDepthFormat(const Graphics &gfx)
{
	const VkFormat candidates[] = {VK_FORMAT_D32_SFLOAT, VK_FORMAT_D32_SFLOAT_S8_UINT, VK_FORMAT_D24_UNORM_S8_UINT};
	return FindSupportedFormat(gfx, candidates, ARRAY_COUNT(candidates),
			VK_IMAGE_TILING_OPTIMAL, VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
}

bool CreateSwapchain(const Graphics &gfx, Window &window, const SwapchainInfo &swapchainInfo, Swapchain &swapchain)
{
	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(gfx.physicalDevice, gfx.surface, &surfaceCapabilities);

	// Swapchain extent
	if ( surfaceCapabilities.currentExtent.width != 0xFFFFFFFF )
	{
		swapchain.extent = surfaceCapabilities.currentExtent;

		// This is the size of the window. We get it here just in case it was not set by the window manager yet.
		window.width = swapchain.extent.width;
		window.height = swapchain.extent.height;
		window.flags &= ~WindowFlags_WasResized;
	}
	else
	{
		swapchain.extent.width = Clamp( window.width, surfaceCapabilities.minImageExtent.width, surfaceCapabilities.maxImageExtent.width );
		swapchain.extent.height = Clamp( window.height, surfaceCapabilities.minImageExtent.height, surfaceCapabilities.maxImageExtent.height );
	}


	LOG(Info, "Swapchain:\n");
#if PLATFORM_ANDROID
	const u32 baseWidth = swapchain.extent.width;
	const u32 baseHeight = swapchain.extent.height;
	const u32 reducedWidth = Max(baseWidth/2, surfaceCapabilities.minImageExtent.width);
	const u32 reducedHeight = Max(baseHeight/2, surfaceCapabilities.minImageExtent.height);
	swapchain.extent.width = reducedWidth;
	swapchain.extent.height = reducedHeight;
	LOG(Info, "- min extent (%ux%u)\n", surfaceCapabilities.minImageExtent.width, surfaceCapabilities.minImageExtent.height);
	LOG(Info, "- max extent (%ux%u)\n", surfaceCapabilities.maxImageExtent.width, surfaceCapabilities.maxImageExtent.height);
	LOG(Info, "- base extent (%ux%u)\n", baseWidth, baseHeight);
#endif
	LOG(Info, "- extent (%ux%u)\n", swapchain.extent.width, swapchain.extent.height);


	// Pre transform
	VkSurfaceTransformFlagBitsKHR preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_IDENTITY_BIT_KHR;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_90_BIT_KHR;
		const u32 temp = swapchain.extent.width;
		swapchain.extent.width = swapchain.extent.height;
		swapchain.extent.height = temp;
		swapchain.preRotationDegrees = 90.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_180_BIT_KHR;
		swapchain.preRotationDegrees = 180.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR ) {
		preTransform = VK_SURFACE_TRANSFORM_ROTATE_270_BIT_KHR;
		const u32 temp = swapchain.extent.width;
		swapchain.extent.width = swapchain.extent.height;
		swapchain.extent.height = temp;
		swapchain.preRotationDegrees = 270.0f;
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_BIT_KHR ) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_90_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_180_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_HORIZONTAL_MIRROR_ROTATE_270_BIT_KHR) {
		INVALID_CODE_PATH();
	} else if ( surfaceCapabilities.currentTransform & VK_SURFACE_TRANSFORM_INHERIT_BIT_KHR) {
		INVALID_CODE_PATH();
	} else {
		INVALID_CODE_PATH();
	}
	LOG(Info, "- preRotationDegrees: %f\n", swapchain.preRotationDegrees);


	// Image count
	u32 imageCount = surfaceCapabilities.minImageCount + 1;
	if ( surfaceCapabilities.maxImageCount > 0 )
		imageCount = Min( imageCount, surfaceCapabilities.maxImageCount );


	// Queues
	u32 queueFamilyIndices[] = {
		gfx.graphicsQueueFamilyIndex,
		gfx.presentQueueFamilyIndex
	};

	VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	u32 queueFamilyIndexCount = 0;
	u32 *pQueueFamilyIndices = NULL;
	if ( gfx.graphicsQueueFamilyIndex != gfx.presentQueueFamilyIndex )
	{
		imageSharingMode = VK_SHARING_MODE_CONCURRENT;
		queueFamilyIndexCount = ARRAY_COUNT(queueFamilyIndices);
		pQueueFamilyIndices = queueFamilyIndices;
	}


	// Composite alpha
	VkCompositeAlphaFlagBitsKHR compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_INHERIT_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_PRE_MULTIPLIED_BIT_KHR;
	} else if ( surfaceCapabilities.supportedCompositeAlpha & VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR ) {
		compositeAlpha = VK_COMPOSITE_ALPHA_POST_MULTIPLIED_BIT_KHR;
	} else {
		INVALID_CODE_PATH();
	}


	// Swapchain
	VkSwapchainCreateInfoKHR swapchainCreateInfo = {};
	swapchainCreateInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
	swapchainCreateInfo.surface = gfx.surface;
	swapchainCreateInfo.minImageCount = imageCount;
	swapchainCreateInfo.imageFormat = swapchainInfo.format;
	swapchainCreateInfo.imageColorSpace = swapchainInfo.colorSpace;
	swapchainCreateInfo.imageExtent = swapchain.extent;
	swapchainCreateInfo.imageArrayLayers = 1;
	swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT; // we will render directly on it
	//swapchainCreateInfo.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT; // for typical engines with several render passes before
	swapchainCreateInfo.imageSharingMode = imageSharingMode;
	swapchainCreateInfo.queueFamilyIndexCount = queueFamilyIndexCount;
	swapchainCreateInfo.pQueueFamilyIndices = pQueueFamilyIndices;
	swapchainCreateInfo.compositeAlpha = compositeAlpha;
	swapchainCreateInfo.presentMode = swapchainInfo.presentMode;
	swapchainCreateInfo.clipped = VK_TRUE; // Don't care about pixels obscured by other windows
	swapchainCreateInfo.oldSwapchain = VK_NULL_HANDLE;
	swapchainCreateInfo.preTransform = preTransform;

	VK_CALL( vkCreateSwapchainKHR(gfx.device, &swapchainCreateInfo, VULKAN_ALLOCATORS, &swapchain.handle) );


	// Get the swapchain images
	vkGetSwapchainImagesKHR( gfx.device, swapchain.handle, &swapchain.imageCount, NULL );
	ASSERT( swapchain.imageCount <= ARRAY_COUNT(swapchain.images) );
	vkGetSwapchainImagesKHR( gfx.device, swapchain.handle, &swapchain.imageCount, swapchain.images );


	// Create image views
	for ( u32 i = 0; i < swapchain.imageCount; ++i )
	{
		const VkImage image = swapchain.images[i];
		const VkFormat format = swapchainInfo.format;
		swapchain.imageViews[i] = CreateImageView(gfx, image, format, VK_IMAGE_ASPECT_COLOR_BIT, 1);
	}


	return true;
}

bool CreateRenderPass( const Graphics &gfx, VkRenderPass &renderPass )
{
	// Create render pass
	VkAttachmentDescription colorAttachmentDesc = {};
	colorAttachmentDesc.format = gfx.swapchainInfo.format;
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
	depthAttachmentDesc.format = FindDepthFormat(gfx);
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

	VK_CALL( vkCreateRenderPass( gfx.device, &renderPassCreateInfo, VULKAN_ALLOCATORS, &renderPass ) );

	return true;
}

bool CreateRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	// Depth buffer
	VkFormat depthFormat = FindDepthFormat(gfx);
	renderTargets.depthImage = CreateImage(gfx,
			gfx.swapchain.extent.width, gfx.swapchain.extent.height, 1,
			depthFormat,
			VK_IMAGE_TILING_OPTIMAL,
			VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT,
			gfx.heaps[HeapType_RTs]);
	VkImageView depthImageView = CreateImageView(gfx, renderTargets.depthImage.image, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	TransitionImageLayout(gfx, renderTargets.depthImage.image, depthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	renderTargets.depthImageView = depthImageView;


	// Framebuffer
	for ( u32 i = 0; i < gfx.swapchain.imageCount; ++i )
	{
		VkImageView attachments[] = { gfx.swapchain.imageViews[i], renderTargets.depthImageView };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = gfx.renderPass;
		framebufferCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = gfx.swapchain.extent.width;
		framebufferCreateInfo.height = gfx.swapchain.extent.height;
		framebufferCreateInfo.layers = 1;

		VK_CALL( vkCreateFramebuffer( gfx.device, &framebufferCreateInfo, VULKAN_ALLOCATORS, &renderTargets.framebuffers[i]) );
	}

	return true;
}

bool InitializeGraphicsPre(Arena &arena, Graphics &gfx)
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

	u32 instanceLayerCount;
	VK_CALL( vkEnumerateInstanceLayerProperties( &instanceLayerCount, NULL ) );
	VkLayerProperties *instanceLayers = PushArray(scratch, VkLayerProperties, instanceLayerCount);
	VK_CALL( vkEnumerateInstanceLayerProperties( &instanceLayerCount, instanceLayers ) );

	const char *wantedInstanceLayerNames[] = {
		"VK_LAYER_KHRONOS_validation"
	};
	const char *enabledInstanceLayerNames[ARRAY_COUNT(wantedInstanceLayerNames)];
	u32 enabledInstanceLayerCount = 0;

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

	u32 instanceExtensionCount;
	VK_CALL( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, NULL ) );
	VkExtensionProperties *instanceExtensions = PushArray(scratch, VkExtensionProperties, instanceExtensionCount);
	VK_CALL( vkEnumerateInstanceExtensionProperties( NULL, &instanceExtensionCount, instanceExtensions ) );

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
	u32 enabledInstanceExtensionCount = 0;

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

	VK_CALL ( vkCreateInstance( &instanceCreateInfo, VULKAN_ALLOCATORS, &gfx.instance ) );


	// Load the instance-related Vulkan function pointers
	volkLoadInstanceOnly(gfx.instance);


	// Report callback
	if ( vkCreateDebugReportCallbackEXT )
	{
		gfx.support.debugReportCallbacks = true;

		VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = { VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT };
		//debugReportCallbackCreateInfo.flags = VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT;
		debugReportCallbackCreateInfo.flags |= VK_DEBUG_REPORT_ERROR_BIT_EXT;
		debugReportCallbackCreateInfo.pfnCallback = VulkanDebugReportCallback;
		debugReportCallbackCreateInfo.pUserData = 0;

		VK_CALL( vkCreateDebugReportCallbackEXT( gfx.instance, &debugReportCallbackCreateInfo, VULKAN_ALLOCATORS, &gfx.debugReportCallback) );
	}

	return true;
}

bool InitializeGraphicsSurface(Window &window, Graphics &gfx)
{
#if VK_USE_PLATFORM_XCB_KHR
	VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.connection = window.connection;
	surfaceCreateInfo.window = window.window;
	VK_CALL( vkCreateXcbSurfaceKHR( gfx.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gfx.surface ) );
#elif VK_USE_PLATFORM_ANDROID_KHR
	VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.window = window.nativeWindow;
	VK_CALL( vkCreateAndroidSurfaceKHR( gfx.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gfx.surface ) );
#elif VK_USE_PLATFORM_WIN32_KHR
	VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {};
	surfaceCreateInfo.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR;
	surfaceCreateInfo.hinstance = window.hInstance;
	surfaceCreateInfo.hwnd = window.hWnd;
	VK_CALL( vkCreateWin32SurfaceKHR( gfx.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &gfx.surface ) );
#endif

	return true;
}

bool InitializeGraphicsDevice(Arena &arena, Window &window, Graphics &gfx)
{
	Arena scratch = MakeSubArena(arena);
	VkResult result = VK_RESULT_MAX_ENUM;


	// List of physical devices
	u32 physicalDeviceCount = 0;
	VK_CALL( vkEnumeratePhysicalDevices( gfx.instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CALL( vkEnumeratePhysicalDevices( gfx.instance, &physicalDeviceCount, physicalDevices ) );

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
		u32 queueFamilyCount = 0;
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, NULL);
		VkQueueFamilyProperties *queueFamilies = PushArray( scratch2, VkQueueFamilyProperties, queueFamilyCount );
		vkGetPhysicalDeviceQueueFamilyProperties( physicalDevice, &queueFamilyCount, queueFamilies );

		u32 gfxFamilyIndex = -1;
		u32 presentFamilyIndex = -1;
		for ( u32 i = 0; i < queueFamilyCount; ++i )
		{
			VkBool32 presentSupport = VK_FALSE;
			vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, gfx.surface, &presentSupport );
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
		u32 deviceExtensionCount;
		VK_CALL( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, NULL ) );
		VkExtensionProperties *deviceExtensions = PushArray( scratch2, VkExtensionProperties, deviceExtensionCount );
		VK_CALL( vkEnumerateDeviceExtensionProperties( physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

		u32 foundDeviceExtensionCount = 0;

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
		u32 surfaceFormatCount = 0;
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, gfx.surface, &surfaceFormatCount, NULL);
		if ( surfaceFormatCount == 0 )
			continue;
		VkSurfaceFormatKHR *surfaceFormats = PushArray( scratch2, VkSurfaceFormatKHR, surfaceFormatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, gfx.surface, &surfaceFormatCount, surfaceFormats);

		gfx.swapchainInfo.format = VK_FORMAT_MAX_ENUM;
		for ( u32 i = 0; i < surfaceFormatCount; ++i )
		{
			if ( ( surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB || surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB ) &&
					surfaceFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
			{
				gfx.swapchainInfo.format = surfaceFormats[i].format;
				gfx.swapchainInfo.colorSpace = surfaceFormats[i].colorSpace;
				break;
			}
		}
		if ( gfx.swapchainInfo.format == VK_FORMAT_MAX_ENUM )
		{
			gfx.swapchainInfo.format = surfaceFormats[0].format;
			gfx.swapchainInfo.colorSpace = surfaceFormats[0].colorSpace;
		}

		// Swapchain present mode
		u32 surfacePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, gfx.surface, &surfacePresentModeCount, NULL);
		if ( surfacePresentModeCount == 0 )
			continue;
		VkPresentModeKHR *surfacePresentModes = PushArray( scratch2, VkPresentModeKHR, surfacePresentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, gfx.surface, &surfacePresentModeCount, surfacePresentModes);

#if USE_SWAPCHAIN_MAILBOX_PRESENT_MODE
		gfx.swapchainPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		for ( u32 i = 0; i < surfacePresentModeCount; ++i )
		{
			if ( surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				gfx.swapchainPresentMode = surfacePresentModes[i];
			}
		}
		if ( gfx.swapchainInfo.presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			gfx.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
		gfx.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

		// At this point, we know this device meets all the requirements
		suitableDeviceFound = true;
		gfx.physicalDevice = physicalDevice;
		gfx.graphicsQueueFamilyIndex = gfxFamilyIndex;
		gfx.presentQueueFamilyIndex = presentFamilyIndex;
		break;
	}

	if ( !suitableDeviceFound )
	{
		LOG(Error, "Could not find any suitable GFX device.\n");
		return false;
	}


	// Device creation
	u32 queueCount = 1;
	float queuePriorities[1] = { 1.0f };
	VkDeviceQueueCreateInfo queueCreateInfos[2] = {};
	u32 queueCreateInfoCount = 0;
	u32 queueCreateInfoIndex = queueCreateInfoCount++;
	queueCreateInfos[queueCreateInfoIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
	queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = gfx.graphicsQueueFamilyIndex;
	queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
	queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	if ( gfx.presentQueueFamilyIndex != gfx.graphicsQueueFamilyIndex )
	{
		queueCreateInfoIndex = queueCreateInfoCount++;
		queueCreateInfos[queueCreateInfoIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = gfx.presentQueueFamilyIndex;
		queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
		queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	}

#if 0
	u32 deviceExtensionCount;
	VK_CALL( vkEnumerateDeviceExtensionProperties( gfx.physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CALL( vkEnumerateDeviceExtensionProperties( gfx.physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

	// We don't need this loop anymore unless we want to print this device extensions
	const char *enabledDeviceExtensionNames[ARRAY_COUNT(requiredDeviceExtensionNames)];
	u32 enabledDeviceExtensionCount = 0;

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
	u32 enabledDeviceExtensionCount = ARRAY_COUNT(requiredDeviceExtensionNames);
#endif

	VkPhysicalDeviceFeatures requiredPhysicalDeviceFeatures = {};
	requiredPhysicalDeviceFeatures.samplerAnisotropy = VK_TRUE;

	VkDeviceCreateInfo deviceCreateInfo = { VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO };
	deviceCreateInfo.queueCreateInfoCount = queueCreateInfoCount;
	deviceCreateInfo.pQueueCreateInfos = queueCreateInfos;
	deviceCreateInfo.enabledLayerCount = 0;
	deviceCreateInfo.ppEnabledLayerNames = NULL;
	deviceCreateInfo.enabledExtensionCount = enabledDeviceExtensionCount;
	deviceCreateInfo.ppEnabledExtensionNames = enabledDeviceExtensionNames;
	deviceCreateInfo.pEnabledFeatures = &requiredPhysicalDeviceFeatures;

	result = vkCreateDevice( gfx.physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &gfx.device );
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "vkCreateDevice failed!\n");
		return false;
	}


	// Load all the remaining device-related Vulkan function pointers
	volkLoadDevice(gfx.device);


	// Print physical device info
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(gfx.physicalDevice, &properties);
	LOG(Info, "Physical device info:\n");
	LOG(Info, "- apiVersion: %u\n", properties.apiVersion); // uint32_t
	LOG(Info, "- driverVersion: %u.%u.%u\n",
			VK_VERSION_MAJOR(properties.driverVersion),
			VK_VERSION_MINOR(properties.driverVersion),
			VK_VERSION_PATCH(properties.driverVersion)); // uint32_t
	LOG(Info, "- vendorID: %u\n", properties.vendorID); // uint32_t
	LOG(Info, "- deviceID: %u\n", properties.deviceID); // uint32_t
	LOG(Info, "- deviceType: %s\n", VkPhysicalDeviceTypeToString(properties.deviceType)); // VkPhysicalDeviceType
	LOG(Info, "- deviceName: %s\n", properties.deviceName); // char
	//LOG(Info, "- \n"); // uint8_t							 pipelineCacheUUID[VK_UUID_SIZE];
	//LOG(Info, "- \n"); // VkPhysicalDeviceLimits			  limits;
	//LOG(Info, "- \n"); // VkPhysicalDeviceSparseProperties	sparseProperties;


	// Get alignments
	gfx.alignment.uniformBufferOffset = properties.limits.minUniformBufferOffsetAlignment;
	gfx.alignment.optimalBufferCopyOffset = properties.limits.optimalBufferCopyOffsetAlignment;
	gfx.alignment.optimalBufferCopyRowPitch = properties.limits.optimalBufferCopyRowPitchAlignment;


	// Create heaps
	gfx.heaps[HeapType_General] = CreateHeap(gfx, HeapType_General, MB(16), false);
	gfx.heaps[HeapType_RTs] = CreateHeap(gfx, HeapType_RTs, MB(64), false);
	gfx.heaps[HeapType_Staging] = CreateHeap(gfx, HeapType_Staging, MB(16), true);
	gfx.heaps[HeapType_Dynamic] = CreateHeap(gfx, HeapType_Dynamic, MB(16), true);
	//gfx.heaps[HeapType_Readback] = CreateHeap(gfx, HeapType_Readback, 0);


	// Retrieve queues
	vkGetDeviceQueue(gfx.device, gfx.graphicsQueueFamilyIndex, 0, &gfx.graphicsQueue);
	vkGetDeviceQueue(gfx.device, gfx.presentQueueFamilyIndex, 0, &gfx.presentQueue);


	// Command pools
	for (u32 i = 0; i < ARRAY_COUNT(gfx.commandPools); ++i)
	{
		VkCommandPoolCreateInfo commandPoolCreateInfo = {};
		commandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
		commandPoolCreateInfo.flags = 0; // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows individual resets using vkResetCommandBuffer
		commandPoolCreateInfo.queueFamilyIndex = gfx.graphicsQueueFamilyIndex;
		VK_CALL( vkCreateCommandPool(gfx.device, &commandPoolCreateInfo, VULKAN_ALLOCATORS, &gfx.commandPools[i]) );
	}


	// Command buffers
	for (u32 i = 0; i < ARRAY_COUNT(gfx.commandBuffers); ++i)
	{
		VkCommandBufferAllocateInfo commandBufferAllocInfo = {};
		commandBufferAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
		commandBufferAllocInfo.commandPool = gfx.commandPools[i];
		commandBufferAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
		commandBufferAllocInfo.commandBufferCount = 1;
		VK_CALL( vkAllocateCommandBuffers( gfx.device, &commandBufferAllocInfo, &gfx.commandBuffers[i]) );
	}


	// Transient command pool
	VkCommandPoolCreateInfo transientCommandPoolCreateInfo = {};
	transientCommandPoolCreateInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
	transientCommandPoolCreateInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT;
	transientCommandPoolCreateInfo.queueFamilyIndex = gfx.graphicsQueueFamilyIndex;
	VK_CALL( vkCreateCommandPool(gfx.device, &transientCommandPoolCreateInfo, VULKAN_ALLOCATORS, &gfx.transientCommandPool) );


	// Create swapchain
	CreateSwapchain( gfx, window, gfx.swapchainInfo, gfx.swapchain );


	// Synchronization objects
	VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };
	VkFenceCreateInfo fenceCreateInfo = { VK_STRUCTURE_TYPE_FENCE_CREATE_INFO };
	fenceCreateInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT; // Start signaled

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		VK_CALL( vkCreateSemaphore( gfx.device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gfx.imageAvailableSemaphores[i] ) );
		VK_CALL( vkCreateSemaphore( gfx.device, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &gfx.renderFinishedSemaphores[i] ) );
		VK_CALL( vkCreateFence( gfx.device, &fenceCreateInfo, VULKAN_ALLOCATORS, &gfx.inFlightFences[i] ) );
	}


	// Create pipeline cache
	VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {};
	pipelineCacheCreateInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO;
	pipelineCacheCreateInfo.flags = 0;
	pipelineCacheCreateInfo.initialDataSize = 0;
	pipelineCacheCreateInfo.pInitialData = NULL;
	VK_CALL( vkCreatePipelineCache( gfx.device, &pipelineCacheCreateInfo, VULKAN_ALLOCATORS, &gfx.pipelineCache ) );



	// TODO: All the code that follows shouldn't be part of the device initialization

	// Global render pass
	CreateRenderPass( gfx, gfx.renderPass );

	// Render targets
	CreateRenderTargets( gfx, gfx.renderTargets );

	// Create staging buffer
	gfx.stagingBuffer = CreateStagingBuffer(gfx);

	// Create vertex/index buffers
	gfx.cubeVertices = CreateVertexBuffer(gfx, cubeVertices, sizeof(cubeVertices));
	gfx.cubeIndices = CreateIndexBuffer(gfx, cubeIndices, sizeof(cubeIndices));
	gfx.planeVertices = CreateVertexBuffer(gfx, planeVertices, sizeof(planeVertices));
	gfx.planeIndices = CreateIndexBuffer(gfx, planeIndices, sizeof(cubeIndices));

	// Create globals buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 globalsBufferSize = sizeof(Globals);
		gfx.globalsBuffer[i] = CreateBuffer(
			gfx,
			globalsBufferSize,
			VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
			gfx.heaps[HeapType_Dynamic]);
	}

	// Create entities buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 entityBufferSize = MAX_ENTITIES * AlignUp( sizeof(SEntity), gfx.alignment.uniformBufferOffset );
		gfx.entityBuffer[i] = CreateBuffer(
			gfx,
			entityBufferSize,
			VK_BUFFER_USAGE_STORAGE_BUFFER_BIT,
			gfx.heaps[HeapType_Dynamic]);
	}

	// Create material buffer
	const u32 materialBufferSize = MAX_MATERIALS * AlignUp( sizeof(SMaterial), gfx.alignment.uniformBufferOffset );
	gfx.materialBuffer = CreateBuffer(gfx, materialBufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, gfx.heaps[HeapType_General]);


	// Create Global Descriptor Pool
	{
		VkDescriptorPoolSize descriptorPoolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<u32>(MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS) },
			{ VK_DESCRIPTOR_TYPE_SAMPLER, static_cast<u32>(MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS) },
		};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		//descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCreateInfo.poolSizeCount = ARRAY_COUNT(descriptorPoolSizes);
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
		descriptorPoolCreateInfo.maxSets = static_cast<u32>(MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS);
		VK_CALL( vkCreateDescriptorPool( gfx.device, &descriptorPoolCreateInfo, VULKAN_ALLOCATORS, &gfx.globalDescriptorPool ) );
	}
	// Create Material Descriptor Pool
	{
		VkDescriptorPoolSize descriptorPoolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, static_cast<u32>(MAX_MATERIALS) },
			{ VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, static_cast<u32>(MAX_MATERIALS) }
		};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		//descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCreateInfo.poolSizeCount = ARRAY_COUNT(descriptorPoolSizes);
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
		descriptorPoolCreateInfo.maxSets = static_cast<u32>(MAX_MATERIALS);
		VK_CALL( vkCreateDescriptorPool( gfx.device, &descriptorPoolCreateInfo, VULKAN_ALLOCATORS, &gfx.materialDescriptorPool ) );
	}


#if USE_IMGUI
	// Create Imgui Descriptor Pool
	{
		// The example only requires a single combined image sampler descriptor for the font image and only uses one descriptor set (for that)
		// If you wish to load e.g. additional textures you may need to alter pools sizes.
		VkDescriptorPoolSize descriptorPoolSizes[] = {
			{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 1 },
		};
		VkDescriptorPoolCreateInfo descriptorPoolCreateInfo = {};
		descriptorPoolCreateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
		descriptorPoolCreateInfo.flags = VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT;
		descriptorPoolCreateInfo.poolSizeCount = ARRAY_COUNT(descriptorPoolSizes);
		descriptorPoolCreateInfo.pPoolSizes = descriptorPoolSizes;
		descriptorPoolCreateInfo.maxSets = 1;
		VK_CALL( vkCreateDescriptorPool( gfx.device, &descriptorPoolCreateInfo, VULKAN_ALLOCATORS, &gfx.imGuiDescriptorPool ) );
	}
#endif

	// Create pipeline
	const ShaderSource vertexShaderSource = GetShaderSource(scratch, "shaders/vertex.spv");
	const ShaderSource fragmentShaderSource = GetShaderSource(scratch, "shaders/fragment.spv");
	const ShaderModule vertexShaderModule = CreateShaderModule(gfx, vertexShaderSource);
	const ShaderModule fragmentShaderModule = CreateShaderModule(gfx, fragmentShaderSource);
	ShaderReflectionH shaderReflectionH = CreateShaderReflection(gfx, scratch, vertexShaderSource, fragmentShaderSource);
	const ShaderReflection &shaderReflection = GetShaderReflection(gfx, shaderReflectionH);
	PipelineH pipeline = CreatePipeline(gfx, scratch, vertexShaderModule, fragmentShaderModule, shaderReflection );
	DestroyShaderModule(gfx, vertexShaderModule);
	DestroyShaderModule(gfx, fragmentShaderModule);

	// Create textures
	TextureH textureDiamond = CreateTexture(gfx, "assets/diamond.png");
	TextureH textureDirt = CreateTexture(gfx, "assets/dirt.jpg");
	TextureH textureGrass = CreateTexture(gfx, "assets/grass.jpg");

	// Create materials
	MaterialH materialDiamond = CreateMaterial(gfx, pipeline, textureDiamond);
	MaterialH materialDirt = CreateMaterial(gfx, pipeline, textureDirt);
	//MaterialH materialGrass = CreateMaterial(gfx, pipeline, textureGrass);
	MaterialH materialGrass = CreateMaterial(gfx, pipeline, textureGrass, 11.0f);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));
		CopyBufferToBuffer(gfx, staged.buffer, staged.offset, gfx.materialBuffer.buffer, material.bufferOffset, sizeof(shaderMaterial));
	}

	// DescriptorSets for materials
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		const Pipeline &pipeline = GetPipeline(gfx, material.pipelineH);
		if ( pipeline.globalDescriptorSetLayout )
		{
			const u32 globalDescriptorSetLayoutCount = ARRAY_COUNT(gfx.globalDescriptorSets[i]);
			VkDescriptorSetLayout globalDescriptorSetLayouts[globalDescriptorSetLayoutCount];
			for (u32 i = 0; i < globalDescriptorSetLayoutCount; ++i)
			{
				globalDescriptorSetLayouts[i] = pipeline.globalDescriptorSetLayout;
			}
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = gfx.globalDescriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = globalDescriptorSetLayoutCount;
			descriptorSetAllocateInfo.pSetLayouts = globalDescriptorSetLayouts;
			VK_CALL( vkAllocateDescriptorSets(gfx.device, &descriptorSetAllocateInfo, gfx.globalDescriptorSets[i]) );
		}
		if ( pipeline.materialDescriptorSetLayout )
		{
			VkDescriptorSetAllocateInfo descriptorSetAllocateInfo = {};
			descriptorSetAllocateInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
			descriptorSetAllocateInfo.descriptorPool = gfx.materialDescriptorPool;
			descriptorSetAllocateInfo.descriptorSetCount = 1;
			descriptorSetAllocateInfo.pSetLayouts = &pipeline.materialDescriptorSetLayout;
			VK_CALL( vkAllocateDescriptorSets(gfx.device, &descriptorSetAllocateInfo, &gfx.materialDescriptorSets[i]) );
		}
	}


	// Samplers
	gfx.textureSampler = CreateSampler(gfx);

	gfx.deviceInitialized = true;

	return true;
}

void UpdateDescriptorSets(Graphics &gfx, bool updateGlobalDS, bool updateMaterialDS )
{
	const u32 frameIndex = gfx.currentFrame;

	// Update material descriptors
	VkWriteDescriptorSet descriptorWrites[MAX_MATERIALS * MAX_SHADER_BINDINGS] = {};
	VkDescriptorImageInfo imageInfos[MAX_MATERIALS * MAX_SHADER_BINDINGS] = {};
	VkDescriptorBufferInfo bufferInfos[MAX_MATERIALS * MAX_SHADER_BINDINGS] = {};

	u32 descriptorWriteCount = 0;
	u32 bufferInfoCount = 0;
	u32 imageInfoCount = 0;

	for (u32 materialIndex = 0; materialIndex < gfx.materialCount; ++materialIndex)
	{
		const Material &material = GetMaterial(gfx, materialIndex);
		const Pipeline &pipeline = GetPipeline(gfx, material.pipelineH);
		const ShaderReflection &reflection = GetShaderReflection(gfx, material.shaderReflectionH);

		for ( u32 bindingIndex = 0; bindingIndex < reflection.bindingCount; ++bindingIndex )
		{
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;
			VkDescriptorImageInfo *imageInfo = 0;
			VkDescriptorBufferInfo *bufferInfo = 0;
			VkDescriptorType descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;

			const ShaderBinding &binding = reflection.bindings[bindingIndex];

			if ( binding.set == DESCRIPTOR_SET_GLOBAL && updateGlobalDS )
			{
				descriptorSet = gfx.globalDescriptorSets[materialIndex][frameIndex];

				if ( binding.type == SpvTypeSampler )
				{
					descriptorType = VK_DESCRIPTOR_TYPE_SAMPLER;
					imageInfo = &imageInfos[imageInfoCount++];
					imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo->imageView = VK_NULL_HANDLE;
					imageInfo->sampler = GetSampler(gfx, gfx.textureSampler).sampler;
				}
				else if ( binding.type == SpvTypeUniformBuffer )
				{
					descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					bufferInfo = &bufferInfos[bufferInfoCount++];
					bufferInfo->buffer = gfx.globalsBuffer[frameIndex].buffer;
					bufferInfo->offset = 0;
					bufferInfo->range = sizeof(Globals);
				}
				else if ( binding.type == SpvTypeStorageBuffer )
				{
					descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
					bufferInfo = &bufferInfos[bufferInfoCount++];
					bufferInfo->buffer = gfx.entityBuffer[frameIndex].buffer;
					bufferInfo->offset = 0;
					bufferInfo->range = sizeof(SEntity) * MAX_ENTITIES;
				}
				else
				{
					INVALID_CODE_PATH();
				}
			}
			else if ( binding.set == DESCRIPTOR_SET_MATERIAL && updateMaterialDS )
			{
				descriptorSet = gfx.materialDescriptorSets[materialIndex];

				if ( binding.type == SpvTypeImage )
				{
					descriptorType = VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE;
					imageInfo = &imageInfos[imageInfoCount++];
					imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
					imageInfo->imageView = GetTexture( gfx, material.albedoTexture ).imageView; // TODO: Somehow get the right texture
					imageInfo->sampler = VK_NULL_HANDLE;
				}
				else if ( binding.type == SpvTypeUniformBuffer )
				{
					descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
					bufferInfo = &bufferInfos[bufferInfoCount++];
					bufferInfo->buffer = gfx.materialBuffer.buffer; // TODO: Somehow get the right buffer here
					bufferInfo->offset = material.bufferOffset; // TODO: Somehow get the right buffer offset here
					bufferInfo->range = sizeof(SMaterial);
				}
				else
				{
					INVALID_CODE_PATH();
				}
			}

			if ( descriptorType != VK_DESCRIPTOR_TYPE_MAX_ENUM )
			{
				const u32 index = descriptorWriteCount++;
				descriptorWrites[index].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
				descriptorWrites[index].dstSet = descriptorSet;
				descriptorWrites[index].dstBinding = binding.binding;
				descriptorWrites[index].dstArrayElement = 0;
				descriptorWrites[index].descriptorType = descriptorType;
				descriptorWrites[index].descriptorCount = 1;
				descriptorWrites[index].pBufferInfo = bufferInfo;
				descriptorWrites[index].pImageInfo = imageInfo;
				descriptorWrites[index].pTexelBufferView = NULL;
				descriptorType = VK_DESCRIPTOR_TYPE_MAX_ENUM;
			}
		}
	}

	if ( descriptorWriteCount > 0 )
	{
		vkUpdateDescriptorSets(gfx.device, descriptorWriteCount, descriptorWrites, 0, NULL);
	}
}

void InitializeScene(Graphics &gfx)
{
	if ( gfx.sceneInitialized )
		return;

	// Camera
	gfx.camera.position = {0, 1, 2};
	gfx.camera.orientation = {0, -0.45f};

	const bool updateGlobalDS = false;
	const bool updateMaterialDS = true;
	UpdateDescriptorSets(gfx, updateGlobalDS, updateMaterialDS);

	// Entities
	for (i32 i = 0; i < 4; ++i)
	{
		for (i32 j = 0; j < 4; ++j)
		{
			const f32 x = -3 + i * 2;
			const f32 z = -3 + j * 2;
			CreateEntity(gfx, float3{x, 0, z}, 1.0f, &gfx.cubeVertices, &gfx.cubeIndices, (i+j)%2);
		}
	}
	CreateEntity(gfx, float3{ 0, -0.5, 0}, 11.0f, &gfx.planeVertices, &gfx.planeIndices, 2);

	gfx.sceneInitialized = true;
}

void WaitDeviceIdle(Graphics &gfx)
{
	vkDeviceWaitIdle( gfx.device );

	gfx.stagingBufferOffset = 0;
}

void CleanupSwapchain(const Graphics &gfx, Swapchain &swapchain)
{
	for ( u32 i = 0; i < swapchain.imageCount; ++i )
	{
		vkDestroyImageView(gfx.device, swapchain.imageViews[i], VULKAN_ALLOCATORS);
	}

	vkDestroySwapchainKHR(gfx.device, swapchain.handle, VULKAN_ALLOCATORS);

	swapchain = {};
}

void CleanupRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	vkDestroyImageView(gfx.device, renderTargets.depthImageView, VULKAN_ALLOCATORS);
	vkDestroyImage(gfx.device, renderTargets.depthImage.image, VULKAN_ALLOCATORS);

	// Reset the heap used for render targets
	Heap &rtHeap = gfx.heaps[HeapType_RTs];
	rtHeap.used = 0;

	for ( u32 i = 0; i < gfx.swapchain.imageCount; ++i )
	{
		vkDestroyFramebuffer( gfx.device, renderTargets.framebuffers[i], VULKAN_ALLOCATORS );
	}

	renderTargets = {};
}

void CleanupGraphicsDevice(Graphics &gfx)
{
	gfx.deviceInitialized = false;

	WaitDeviceIdle( gfx );

	vkDestroyDescriptorPool( gfx.device, gfx.globalDescriptorPool, VULKAN_ALLOCATORS );
	vkDestroyDescriptorPool( gfx.device, gfx.materialDescriptorPool, VULKAN_ALLOCATORS );
#if USE_IMGUI
	vkDestroyDescriptorPool( gfx.device, gfx.imGuiDescriptorPool, VULKAN_ALLOCATORS );
#endif

	for (u32 i = 0; i < gfx.samplerCount; ++i)
	{
		vkDestroySampler( gfx.device, gfx.samplers[i].sampler, VULKAN_ALLOCATORS );
	}

	for (u32 i = 0; i < gfx.textureCount; ++i)
	{
		vkDestroyImageView( gfx.device, gfx.textures[i].imageView, VULKAN_ALLOCATORS );
		vkDestroyImage( gfx.device, gfx.textures[i].image.image, VULKAN_ALLOCATORS );
	}

	vkDestroyBuffer( gfx.device, gfx.cubeIndices.buffer, VULKAN_ALLOCATORS );
	vkDestroyBuffer( gfx.device, gfx.cubeVertices.buffer, VULKAN_ALLOCATORS );
	vkDestroyBuffer( gfx.device, gfx.planeIndices.buffer, VULKAN_ALLOCATORS );
	vkDestroyBuffer( gfx.device, gfx.planeVertices.buffer, VULKAN_ALLOCATORS );
	vkDestroyBuffer( gfx.device, gfx.stagingBuffer.buffer, VULKAN_ALLOCATORS );

	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; i++)
	{
		vkDestroyBuffer(gfx.device, gfx.globalsBuffer[i].buffer, VULKAN_ALLOCATORS);
		vkDestroyBuffer(gfx.device, gfx.entityBuffer[i].buffer, VULKAN_ALLOCATORS);
	}

	vkDestroyBuffer( gfx.device, gfx.materialBuffer.buffer, VULKAN_ALLOCATORS );

	for (u32 i = 0; i < HeapType_COUNT; ++i)
	{
		vkFreeMemory(gfx.device, gfx.heaps[i].memory, VULKAN_ALLOCATORS);
	}

	for (u32 i = 0; i < gfx.pipelineCount; ++i )
	{
		const Pipeline &pipeline = GetPipeline(gfx, i);
		vkDestroyPipeline( gfx.device, pipeline.handle, VULKAN_ALLOCATORS );
		vkDestroyPipelineLayout( gfx.device, pipeline.layout, VULKAN_ALLOCATORS );
		vkDestroyDescriptorSetLayout( gfx.device, pipeline.globalDescriptorSetLayout, VULKAN_ALLOCATORS );
		vkDestroyDescriptorSetLayout( gfx.device, pipeline.materialDescriptorSetLayout, VULKAN_ALLOCATORS );
	}

	vkDestroyRenderPass( gfx.device, gfx.renderPass, VULKAN_ALLOCATORS );


	vkDestroyPipelineCache( gfx.device, gfx.pipelineCache, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		vkDestroySemaphore( gfx.device, gfx.imageAvailableSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroySemaphore( gfx.device, gfx.renderFinishedSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroyFence( gfx.device, gfx.inFlightFences[i], VULKAN_ALLOCATORS );
	}

	vkDestroyCommandPool( gfx.device, gfx.transientCommandPool, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < ARRAY_COUNT(gfx.commandPools); ++i )
	{
		vkDestroyCommandPool( gfx.device, gfx.commandPools[i], VULKAN_ALLOCATORS );
	}

	vkDestroyDevice(gfx.device, VULKAN_ALLOCATORS);

	vkDestroySurfaceKHR(gfx.instance, gfx.surface, VULKAN_ALLOCATORS);

	if ( gfx.support.debugReportCallbacks )
	{
		vkDestroyDebugReportCallbackEXT( gfx.instance, gfx.debugReportCallback, VULKAN_ALLOCATORS );
	}

	vkDestroyInstance(gfx.instance, VULKAN_ALLOCATORS);

	ZeroStruct( &gfx );
}

void LoadScene(Graphics &gfx)
{
	// TODO
}

void CleanupScene(Graphics &gfx)
{
	// TODO
	gfx.sceneInitialized = false;
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

#define USE_CAMERA_MOVEMENT 1
#if USE_CAMERA_MOVEMENT

#if PLATFORM_ANDROID
bool GetOrientationTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 > window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}

bool GetMovementTouchId(const Window &window, u32 *touchId)
{
	ASSERT( touchId != 0 );
	for (u32 i = 0; i < ARRAY_COUNT(window.touches); ++i)
	{
		if (window.touches[i].state == TOUCH_STATE_PRESSED &&
			window.touches[i].x0 <= window.width/2 )
		{
			*touchId = i;
			return true;
		}
	}
	return false;
}
#endif

void AnimateCamera(const Window &window, Camera &camera, float deltaSeconds)
{
	// Camera rotation
	f32 deltaYaw = 0.0f;
	f32 deltaPitch = 0.0f;
#if PLATFORM_ANDROID
	u32 touchId;
	if ( GetOrientationTouchId(window, &touchId) )
	{
		deltaYaw = - window.touches[touchId].dx * ToRadians * 0.2f;
		deltaPitch = - window.touches[touchId].dy * ToRadians * 0.2f;
	}
#else
	if (MouseButtonPressed(window.mouse, MOUSE_BUTTON_LEFT)) {
		deltaYaw = - window.mouse.dx * ToRadians * 0.2f;
		deltaPitch = - window.mouse.dy * ToRadians * 0.2f;
	}
#endif
	float2 angles = camera.orientation;
	angles.x = angles.x + deltaYaw;
	angles.y = Clamp(angles.y + deltaPitch, -Pi * 0.49, Pi * 0.49);
	camera.orientation = angles;

	// Movement direction
	float3 dir = { 0, 0, 0 };
#if PLATFORM_ANDROID
	if ( GetMovementTouchId(window, &touchId) )
	{
		const float3 forward = ForwardDirectionFromAngles(angles);
		const float3 right = RightDirectionFromAngles(angles);
		const float scaleForward = -window.touches[touchId].dy;
		const float scaleRight = window.touches[touchId].dx;
		dir = Add(Mul(forward, scaleForward), Mul(right, scaleRight));
	}
#else
	if ( KeyPressed(window.keyboard, KEY_W) ) { dir = Add(dir, ForwardDirectionFromAngles(angles)); }
	if ( KeyPressed(window.keyboard, KEY_S) ) { dir = Add(dir, Negate( ForwardDirectionFromAngles(angles) )); }
	if ( KeyPressed(window.keyboard, KEY_D) ) { dir = Add(dir, RightDirectionFromAngles(angles)); }
	if ( KeyPressed(window.keyboard, KEY_A) ) { dir = Add(dir, Negate( RightDirectionFromAngles(angles) )); }
#endif
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
	camera.position = Add(camera.position, translation);

	// Apply deceleration
	speed = Mul(speed, 0.9);
}
#endif // USE_CAMERA_MOVEMENT

bool RenderGraphics(Graphics &gfx, Window &window, Arena &frameArena, f32 deltaSeconds)
{
	u32 frameIndex = gfx.currentFrame;

	// Swapchain sync
	VK_CALL( vkWaitForFences( gfx.device, 1, &gfx.inFlightFences[frameIndex], VK_TRUE, UINT64_MAX ) );

	u32 imageIndex;
	VkResult acquireResult = vkAcquireNextImageKHR( gfx.device, gfx.swapchain.handle, UINT64_MAX, gfx.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex );

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LOG(Warning, "vkAcquireNextImageKHR - result: VK_ERROR_OUT_OF_DATE_KHR\n");
		gfx.swapchain.outdated = true;
		return false;
	}
	else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		LOG(Error, "vkAcquireNextImageKHR failed.\n");
		return false;
	}

	VK_CALL( vkResetFences( gfx.device, 1, &gfx.inFlightFences[frameIndex] ) );


	// Calculate camera matrices
	const f32 ar = static_cast<f32>(gfx.swapchain.extent.width) / static_cast<f32>(gfx.swapchain.extent.height);
	const float orthox = ar > 1.0f ? ar : 1.0f;
	const float orthoy = ar > 1.0f ? 1.0f : 1.0f/ar;
	const float4x4 viewMatrix = ViewMatrixFromCamera(gfx.camera);
	//const float4x4 = Orthogonal(-orthox, orthox, -orthoy, orthoy, -10, 10);
	const float preRotationDegrees = gfx.swapchain.preRotationDegrees;
	const float4x4 preTransformMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
#if PLATFORM_ANDROID
	const float4x4 perspectiveMatrix = Perspective(90.0f, ar, 0.1f, 1000.0f);
#else
	// TODO: Make this depend on swapchain config, not platform
	const float4x4 perspectiveMatrix = Perspective(60.0f, ar, 0.1f, 1000.0f);
#endif
	const float4x4 projectionMatrix = Mul(perspectiveMatrix, preTransformMatrix);

	// Update globals data
	Globals globals;
	globals.view = viewMatrix;
	globals.proj = projectionMatrix;
	globals.eyePosition = Float4(gfx.camera.position, 1.0f);
	Buffer &globalsBuffer = gfx.globalsBuffer[frameIndex];
	Heap &globalsBufferHeap = gfx.heaps[globalsBuffer.alloc.heap];
	void *ptr = globalsBufferHeap.data + globalsBuffer.alloc.offset;
	MemCopy( ptr, &globals, sizeof(globals) );

	// Update entity data
	Buffer &entityBuffer = gfx.entityBuffer[frameIndex];
	Heap &entityBufferHeap = gfx.heaps[entityBuffer.alloc.heap];
	SEntity *entities = (SEntity*)(entityBufferHeap.data + entityBuffer.alloc.offset);
	for (u32 i = 0; i < gfx.entityCount; ++i)
	{
		const Entity &entity = gfx.entities[i];
		const float4x4 worldMatrix = Mul(Translate(entity.position), Scale(Float3(entity.scale))); // TODO: Apply also rotation
		entities[i].world = worldMatrix;
	}

	// Update global descriptor sets
	const bool updateGlobalDS = true;
	const bool updateMaterialDS = false;
	UpdateDescriptorSets(gfx, updateGlobalDS, updateMaterialDS);

	// Reset commands for this frame
	VkCommandPool commandPool = gfx.commandPools[frameIndex];
	VK_CALL( vkResetCommandPool(gfx.device, commandPool, 0) );

	// Record commands
	VkCommandBuffer commandBuffer = gfx.commandBuffers[frameIndex];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // Optional
	commandBufferBeginInfo.pInheritanceInfo = NULL; // Optional
	VK_CALL( vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) );

	VkClearValue clearValues[2] = {};
	clearValues[0].color = {{0.0f, 0.0f, 0.0f, 1.0f}};
	clearValues[1].depthStencil = {1.0f, 0};

	VkRenderPassBeginInfo renderPassBeginInfo = {};
	renderPassBeginInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
	renderPassBeginInfo.renderPass = gfx.renderPass;
	renderPassBeginInfo.framebuffer = gfx.renderTargets.framebuffers[imageIndex];
	renderPassBeginInfo.renderArea.offset = { 0, 0 };
	renderPassBeginInfo.renderArea.extent = gfx.swapchain.extent;
	renderPassBeginInfo.clearValueCount = ARRAY_COUNT(clearValues);
	renderPassBeginInfo.pClearValues = clearValues;

	vkCmdBeginRenderPass( commandBuffer, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );

	VkViewport viewport = {};
	viewport.x = 0.0f;
	viewport.y = static_cast<float>(gfx.swapchain.extent.height);
	viewport.width = static_cast<float>(gfx.swapchain.extent.width);
	viewport.height = -static_cast<float>(gfx.swapchain.extent.height);
	viewport.minDepth = 0.0f;
	viewport.maxDepth = 1.0f;
	vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

	VkRect2D scissor = {};
	scissor.offset = {0, 0};
	scissor.extent = gfx.swapchain.extent;
	vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

	VkPipeline prevPipeline = VK_NULL_HANDLE;
	VkDescriptorSet prevGlobalSet = VK_NULL_HANDLE;
	VkDescriptorSet prevMaterialSet = VK_NULL_HANDLE;

	for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
	{
		const Entity &entity = gfx.entities[entityIndex];

		if ( !entity.visible ) continue;

		const u32 materialIndex = entity.materialIndex;
		const Material &material = gfx.materials[materialIndex];
		const Pipeline &pipeline = gfx.pipelines[material.pipelineH];

		// Pipeline
		const VkPipeline vkPipeline = pipeline.handle;
		if ( vkPipeline != prevPipeline )
		{
			prevPipeline = vkPipeline;
			vkCmdBindPipeline( commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, vkPipeline );
		}

		// Descriptor sets
		VkDescriptorSet descriptorSets[4] = {};
		u32 descriptorSetCount = 0;
		u32 descriptorSetFirst = UINT32_MAX;

		// firstSet = 0
		const VkDescriptorSet globalSet = gfx.globalDescriptorSets[materialIndex][frameIndex];
		if ( globalSet && globalSet != prevGlobalSet ) {
			prevGlobalSet = globalSet;
			descriptorSetFirst = 0;
			descriptorSets[descriptorSetCount++] = globalSet;
		}

		// firstSet = 1
		const VkDescriptorSet materialSet = gfx.materialDescriptorSets[materialIndex];
		if ( materialSet && materialSet != prevMaterialSet ) {
			prevMaterialSet = materialSet;
			descriptorSetFirst = Min(descriptorSetFirst, 1u);
			descriptorSets[descriptorSetCount++] = materialSet;
		}

		if ( descriptorSetCount > 0 ) {
			vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline.layout, descriptorSetFirst, descriptorSetCount, descriptorSets, 0, NULL);
		}

		Buffer *vertexBuffer = entity.vertexBuffer;
		Buffer *indexBuffer = entity.indexBuffer;

		VkBuffer vertexBuffers[] = { vertexBuffer->buffer };
		VkDeviceSize offsets[] = { 0 };
		vkCmdBindVertexBuffers(commandBuffer, 0, ARRAY_COUNT(vertexBuffers), vertexBuffers, offsets);

		vkCmdBindIndexBuffer(commandBuffer, indexBuffer->buffer, 0, VK_INDEX_TYPE_UINT16);

		vkCmdDrawIndexed(commandBuffer, indexBuffer->alloc.size/2, 1, 0, 0, entityIndex);
	}

#if USE_IMGUI
	// Record dear imgui primitives into command buffer
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplVulkan_RenderDrawData(draw_data, commandBuffer);
#endif

	vkCmdEndRenderPass(commandBuffer);

	VK_CALL( vkEndCommandBuffer( commandBuffer ) );


	// Submit commands
	VkSemaphore waitSemaphores[] = { gfx.imageAvailableSemaphores[frameIndex] };
	VkSemaphore signalSemaphores[] = { gfx.renderFinishedSemaphores[frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.waitSemaphoreCount = ARRAY_COUNT(waitSemaphores);
	submitInfo.pWaitSemaphores = waitSemaphores;
	submitInfo.pWaitDstStageMask = waitStages;
	submitInfo.signalSemaphoreCount = ARRAY_COUNT(signalSemaphores);
	submitInfo.pSignalSemaphores = signalSemaphores;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;

	VK_CALL( vkQueueSubmit( gfx.graphicsQueue, 1, &submitInfo, gfx.inFlightFences[frameIndex] ) );


	// Presentation
	VkPresentInfoKHR presentInfo = {};
	presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;
	presentInfo.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores);
	presentInfo.pWaitSemaphores = signalSemaphores;
	presentInfo.swapchainCount = 1;
	presentInfo.pSwapchains = &gfx.swapchain.handle;
	presentInfo.pImageIndices = &imageIndex;
	presentInfo.pResults = NULL; // Optional

	VkResult presentResult = vkQueuePresentKHR( gfx.presentQueue, &presentInfo );

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
	{
		LOG(Warning, "vkQueuePresentKHR - result: VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR\n");
		gfx.swapchain.outdated = true;
	}
	else if (presentResult != VK_SUCCESS)
	{
		LOG(Error, "vkQueuePresentKHR failed.\n");
		return false;
	}

	gfx.currentFrame = ( gfx.currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;

	return true;
}




#if USE_IMGUI
void InitializeImGuiContext()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.5f;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;	 // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;	  // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();
}

void InitializeImGuiGraphics(Graphics &gfx, const Window &window)
{
	// Setup Platform/Renderer backends
#if USE_WINAPI
	ImGui_ImplWin32_Init(window.hWnd);
#elif USE_XCB
	ImGui_ImplXcb_Init(window.connection, window.window);
#else
#error "Missing codepath"
#endif

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gfx.instance;
	init_info.PhysicalDevice = gfx.physicalDevice;
	init_info.Device = gfx.device;
	init_info.QueueFamily = gfx.graphicsQueueFamilyIndex;
	init_info.Queue = gfx.graphicsQueue;
	init_info.PipelineCache = gfx.pipelineCache;
	init_info.DescriptorPool = gfx.imGuiDescriptorPool;
	init_info.Subpass = 0;
	init_info.MinImageCount = gfx.swapchain.imageCount;
	init_info.ImageCount = gfx.swapchain.imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = VULKAN_ALLOCATORS;
	init_info.CheckVkResultFn = CheckVulkanResultImGui;
	ImGui_ImplVulkan_Init(&init_info, gfx.renderPass);

	// Upload Fonts
	{
		// Use any command buffer
		VkCommandPool commandPool = gfx.commandPools[0];
		VK_CALL( vkResetCommandPool(gfx.device, commandPool, 0) );
		VkCommandBuffer commandBuffer = gfx.commandBuffers[0];

		VkCommandBufferBeginInfo beginInfo = {};
		beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
		beginInfo.flags |= VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;
		VK_CALL( vkBeginCommandBuffer(commandBuffer, &beginInfo) );

		ImGui_ImplVulkan_CreateFontsTexture(commandBuffer);

		VkSubmitInfo endInfo = {};
		endInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
		endInfo.commandBufferCount = 1;
		endInfo.pCommandBuffers = &commandBuffer;
		VK_CALL( vkEndCommandBuffer(commandBuffer) );
		VK_CALL( vkQueueSubmit(gfx.graphicsQueue, 1, &endInfo, VK_NULL_HANDLE) );

		WaitDeviceIdle(gfx);

		ImGui_ImplVulkan_DestroyFontUploadObjects();
	}
}

void UpdateImGui(Graphics &gfx)
{
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
	ImGui::Begin("Hello, world!");


	if ( ImGui::CollapsingHeader("Scene", ImGuiTreeNodeFlags_DefaultOpen) )
	{
		static bool sceneLoaded = false;
		if ( ImGui::Button("Load") && !sceneLoaded )
		{
			LoadScene(gfx);
		}
		if ( ImGui::Button("Cleanup") && sceneLoaded )
		{
			CleanupScene(gfx);
		}
	}

	ImGui::Separator();

	char tmpString[4092] = {};
	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(gfx.physicalDevice, &memoryProperties);


	ImGui::Text("Memory units");
	static int unit = 1;
	static const char *unitSuffix[] = {"B", "KB", "MB"};
	static const u32 unitBytes[] = {1, KB(1), MB(1)};
	if (ImGui::BeginTable("##mem_units", 3)) {
		for (int i = 0; i < ARRAY_COUNT(unitSuffix); i++) {
			ImGui::TableNextColumn();
			if (ImGui::RadioButton(unitSuffix[i], i == unit)) {
				unit = i;
			}
		}
		ImGui::EndTable();
	}

	if ( ImGui::CollapsingHeader("Vulkan memory types", ImGuiTreeNodeFlags_None) )
	{
		for ( u32 i = 0; i < memoryProperties.memoryTypeCount; ++i )
		{
			const VkMemoryType &memoryType = memoryProperties.memoryTypes[i];

			// Some devices expose memory types we cannot use
			if ( memoryType.propertyFlags == 0 ) continue;

			ImGui::Text("- type[%u]", i);
			ImGui::Text("- propertyFlags: %s", VkMemoryPropertyFlagsToString(memoryType.propertyFlags, tmpString));
			ImGui::Text("- heapIndex: %u", memoryType.heapIndex);
			ImGui::Separator();
		}
	}

	if ( ImGui::CollapsingHeader("Vulkan memory heaps", ImGuiTreeNodeFlags_None) )
	{
		for ( u32 i = 0; i < memoryProperties.memoryHeapCount; ++i )
		{
			const VkMemoryHeap &memoryHeap = memoryProperties.memoryHeaps[i];
			ImGui::Text("- heap[%u]", i );
			ImGui::Text("- size: %u %s", memoryHeap.size / unitBytes[unit], unitSuffix[unit]);
			ImGui::Text("- flags: %s", VkMemoryHeapFlagsToString(memoryHeap.flags, tmpString));
			ImGui::Separator();
		}
	}

	if ( ImGui::CollapsingHeader("Application memory heaps", ImGuiTreeNodeFlags_None) )
	{
		for ( u32 i = 0; i < HeapType_COUNT; ++i )
		{
			const Heap &heap = gfx.heaps[i];
			ImGui::Text("- %s", HeapTypeToString((HeapType)i));
			ImGui::Text("  - size: %u %s\n", heap.size / unitBytes[unit], unitSuffix[unit]);
			ImGui::Text("  - used: %u %s\n", heap.used / unitBytes[unit], unitSuffix[unit]);
			ImGui::Text("  - memoryTypeIndex: %u\n", heap.memoryTypeIndex);
			ImGui::Separator();
		}
	}
	if ( ImGui::CollapsingHeader("Entities", ImGuiTreeNodeFlags_None) )
	{
		ImGui::Text("Entity count: %u", gfx.entityCount);
		ImGui::Separator();

		for ( u32 i = 0; i < gfx.entityCount; ++i )
		{
			const Entity &entity = gfx.entities[i];
			const Heap &heap = gfx.heaps[i];
			ImGui::Text("- entity[%u]", i);
			ImGui::Text("  - position: (%f, %f, %f)", entity.position.x, entity.position.y, entity.position.z);
			ImGui::Text("  - materialIndex: %u", entity.materialIndex);
			ImGui::Separator();
		}
	}

	const ImGuiIO& io = ImGui::GetIO();
	ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

	ImGui::End();
	ImGui::Render(); // Generate the draw data
}

void CleanupImGui()
{
	ImGui_ImplVulkan_Shutdown();
#if USE_WINAPI
	ImGui_ImplWin32_Shutdown();
#elif USE_XCB
	ImGui_ImplXcb_Shutdown();
#else
#error "Missing codepath"
#endif
	ImGui::DestroyContext();
}
#endif // USE_IMGUI




Graphics &GetPlatformGraphics(Platform &platform)
{
	Graphics *gfx = (Graphics*)platform.userData;
	ASSERT(gfx != NULL);
	return *gfx;
}

bool EngineInit(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

#if USE_IMGUI
	InitializeImGuiContext();
#endif

	// Initialize graphics
	if ( !InitializeGraphicsPre(platform.globalArena, gfx) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsPre failed!\n");
		return false;
	}

	return true;
}

bool EngineWindowInit(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	if ( !InitializeGraphicsSurface(platform.window, gfx) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsSurface failed!\n");
		return false;
	}

	if ( gfx.deviceInitialized )
	{
		// TODO: Check the current device still supports the new surface
	}
	else
	{
		if ( !InitializeGraphicsDevice(platform.globalArena, platform.window, gfx) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphicsDevice failed!\n");
			return false;
		}
	}

	InitializeScene(gfx);

#if USE_IMGUI
	InitializeImGuiGraphics(gfx, platform.window);
#endif

	return true;
}

void EngineUpdate(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	if ( gfx.swapchain.outdated || platform.window.flags & WindowFlags_WasResized )
	{
		WaitDeviceIdle(gfx);
		CleanupRenderTargets(gfx, gfx.renderTargets);
		CleanupSwapchain(gfx, gfx.swapchain);
		CreateSwapchain(gfx, platform.window, gfx.swapchainInfo, gfx.swapchain);
		CreateRenderTargets(gfx, gfx.renderTargets);
		gfx.swapchain.outdated = false;
	}

	if ( gfx.deviceInitialized && gfx.swapchain.handle != VK_NULL_HANDLE )
	{
#if USE_CAMERA_MOVEMENT
		AnimateCamera(platform.window, gfx.camera, platform.deltaSeconds);
#endif

#if USE_IMGUI
		UpdateImGui(gfx);
#endif

		RenderGraphics(gfx, platform.window, platform.frameArena, platform.deltaSeconds);
	}
}

void EngineWindowCleanup(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	WaitDeviceIdle(gfx);
	CleanupRenderTargets(gfx, gfx.renderTargets);
	CleanupSwapchain(gfx, gfx.swapchain);
}

void EngineCleanup(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	WaitDeviceIdle(gfx);

#if USE_IMGUI
	CleanupImGui();
#endif

	CleanupScene(gfx);

	CleanupRenderTargets(gfx, gfx.renderTargets);
	CleanupSwapchain(gfx, gfx.swapchain);

	CleanupGraphicsDevice(gfx);
}

int main(int argc, char **argv)
{
	Platform platform = {};

	// Memory
	platform.stringMemorySize = KB(16);
	platform.globalMemorySize = MB(64);
	platform.frameMemorySize = MB(16);

	// Callbacks
	platform.InitCallback = EngineInit;
	platform.UpdateCallback = EngineUpdate;
	platform.CleanupCallback = EngineCleanup;
	platform.WindowInitCallback = EngineWindowInit;
	platform.WindowCleanupCallback = EngineWindowCleanup;

	// User data
	Graphics gfx = {};
	platform.userData = &gfx;

	PlatformRun(platform);

	return 1;
}

// TODO:
// - Investigate how to write descriptors in a more elegant manner (avoid hardcoding).
// - Put all the geometry in the same buffer.
// - Avoid using push constants and put transformation matrices in buffers instead.
// - Instead of binding descriptors per entity, group entities by material and perform a multi draw call for each material group.

