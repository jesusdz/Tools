/*
 * tools_gfx.h
 * Author: Jesus Diaz Garcia
 *
 * Graphics API abstraction made on top of Vulkan.
 *
 * These are the functions abstracted from the low-level API.
 *
 * Device initialization:
 * - (Create/Destroy)Swapchain
 * - (Initialize/Cleanup)GraphicsDriver
 * - (Initialize/Cleanup)GraphicsSurface
 * - (Initialize/Cleanup)GraphicsDevice
 *
 * Resource management:
 * - (Create/Destroy/Reset)BindGroupAllocator
 * - (Create/Destroy)BindGroupLayout
 * - CreateBindGroup
 * - (Create/Destroy)Buffer
 * - (Create/Destroy)BufferView
 * - (Create/Destroy)Image
 * - (Create/Destroy)Sampler
 * - CreateGraphicsPipeline
 * - CreateComputePipeline
 * - DestroyPipeline
 * - (Create/Destroy)RenderPass
 * - (Create/Destroy)Framebuffer
 *
 * Command lists
 * - (Begin/End)CommandList
 * - (Begin/End)TransientCommandList
 *
 * Commands:
 * - CopyBufferToBuffer
 * - CopyBufferToImage
 * - Blit
 * - TransitionImageLayout
 * - (Begin/End)RenderPass
 * - SetViewportAndScissor
 * - SetClear(Color/Depth/Stencil)
 * - SetPipeline
 * - SetBindGroup
 * - SetVertexBuffer
 * - SetIndexBuffer
 * - DrawIndexed
 * - Dispatch
 *
 * Work submission and synchronization:
 * - Submit
 * - Present
 * - (Begin/End)Frame
 * - WaitQueueIdle
 * - WaitDeviceIdle
 */

#ifndef TOOLS_GFX_H
#define TOOLS_GFX_H


////////////////////////////////////////////////////////////////////////
// Includes and definitions
////////////////////////////////////////////////////////////////////////

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

#endif // #if USE_IMGUI

// Undefining VK_NO_PROTOTYPES here to avoid ImGui to retrieve Vulkan functions again.
#undef VK_NO_PROTOTYPES
#include "imgui/imgui_impl_vulkan.cpp"
#endif

#define SPV_ASSERT ASSERT
#define SPV_PRINTF(...) LOG(Info, ##__VA_ARGS__)
#define SPV_IMPLEMENTATION
#define SPV_PRINT_FUNCTIONS
#include "tools_spirv.h"

#define MAX_DESCRIPTOR_SETS 4
#define MAX_SHADER_BINDINGS 16
#if PLATFORM_ANDROID
#define MAX_SWAPCHAIN_IMAGE_COUNT 5
#else
#define MAX_SWAPCHAIN_IMAGE_COUNT 3
#endif
#define MAX_FRAMES_IN_FLIGHT 2
#define MAX_FENCES 128
#define MAX_RENDER_TARGETS 4

#define VULKAN_ALLOCATORS NULL

#define VK_CALL( call ) CheckVulkanResult( call, #call )


////////////////////////////////////////////////////////////////////////
// Types
////////////////////////////////////////////////////////////////////////

enum HeapType
{
	HeapType_General,
	HeapType_RTs,
	HeapType_Staging,
	HeapType_Dynamic,
	HeapType_Readback,
	HeapType_COUNT,
};

enum BorderColor
{
	BorderColorBlackInt,
	BorderColorWhiteInt,
	BorderColorBlackFloat,
	BorderColorWhiteFloat,
	BorderColorCount,
};

enum AddressMode
{
	AddressModeRepeat,
	AddressModeClampToBorder,
	AddressModeClampToEdge,
	AddressModeCount,
};

enum BufferUsageFlagBits
{
	BufferUsageTransferSrc = 1<<0,
	BufferUsageTransferDst = 1<<1,
	BufferUsageUniformBuffer = 1<<2,
	BufferUsageUniformTexelBuffer = 1<<3,
	BufferUsageStorageBuffer = 1<<4,
	BufferUsageStorageTexelBuffer = 1<<5,
	BufferUsageVertexBuffer = 1<<6,
	BufferUsageIndexBuffer = 1<<7,
};

typedef u32 BufferUsageFlags;

enum ImageUsageFlagBits
{
  ImageUsageTransferSrc = 1<<0,
  ImageUsageTransferDst = 1<<1,
  ImageUsageSampled = 1<<2,
  ImageUsageStorage = 1<<3,
  ImageUsageColorAttachment = 1<<4,
  ImageUsageDepthStencilAttachment = 1<<5,
  ImageUsageTransient = 1<<6,
  ImageUsageInputAttachment = 1<<7,
};

typedef u32 ImageUsageFlags;

enum ImageState
{
	ImageStateInitial,
	ImageStateTransferSrc,
	ImageStateTransferDst,
	ImageStateShaderInput,
	ImageStateRenderTarget,
};

struct BlitRegion
{
	i32 x;
	i32 y;
	i32 width;
	i32 height;
	u32 mipLevel;
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

struct ShaderBindings
{
	ShaderBinding bindings[MAX_SHADER_BINDINGS];
	u8 bindingCount;
};

struct BindGroupAllocatorCounts
{
	u32 uniformBufferCount;
	u32 storageBufferCount;
	u32 storageTexelBufferCount;
	u32 textureCount;
	u32 samplerCount;
	u32 combinedImageSamplerCount;
	u32 groupCount;
	bool allowIndividualFrees;
};

struct BindGroupAllocator
{
	BindGroupAllocatorCounts maxCounts;
	BindGroupAllocatorCounts usedCounts;
	VkDescriptorPool handle;
};

struct BindGroupLayout
{
	VkDescriptorSetLayout handle;
	const ShaderBinding *bindings;
	u8 bindingCount;
};

struct PipelineLayout
{
	VkPipelineLayout handle;
	BindGroupLayout bindGroupLayouts[MAX_DESCRIPTOR_SETS];
	ShaderBindings shaderBindings;
};

struct ResourceBuffer
{
	VkBuffer handle;
	u32 offset;
	u32 range;
};

struct ResourceBufferView
{
	VkBufferView handle;
};

struct ResourceTexture
{
	VkImageView handle;
};

struct ResourceSampler
{
	VkSampler handle;
};

union ResourceGeneric
{
	ResourceBuffer buffer;
	ResourceBufferView bufferView;
	ResourceTexture texture;
	ResourceSampler sampler;
};

struct ResourceBinding
{
	u8 index;
	ResourceGeneric resource;
};

struct BindGroupDesc
{
	BindGroupLayout layout;
	ResourceBinding bindings[MAX_SHADER_BINDINGS];
};

struct BindGroup
{
	VkDescriptorSet handle;
};

struct Pipeline
{
	const char *name;
	VkPipeline handle;
	PipelineLayout layout;
	VkPipelineBindPoint bindPoint;
};

typedef u32 PipelineH;

struct Buffer
{
	VkBuffer handle;
	Alloc alloc;
	u32 size;
};

typedef u32 BufferH;

struct BufferArena
{
	BufferH buffer;
	u32 used;
	u32 size;
};

struct BufferChunk
{
	BufferH buffer;
	u32 offset;
	u32 size;
};

struct BufferView
{
	VkBufferView handle;
};

typedef u32 BufferViewH;

struct Image
{
	VkImage handle;
	Format format;
	u32 width;
	u32 height;
	u32 mipLevels;
	Alloc alloc;
};

struct Texture
{
	const char *name;
	Image image;
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

struct RenderPass
{
	const char *name;
	VkRenderPass handle;
};

typedef u32 RenderPassH;

struct FramebufferDesc
{
	RenderPass renderPass;
	VkImageView attachments[4];
	u32 attachmentCount;
	u32 width;
	u32 height;
};

struct Framebuffer
{
	VkFramebuffer handle;
	VkRenderPass renderPassHandle;
	VkExtent2D extent;
	u32 attachmentCount;
};

struct SamplerDesc
{
	AddressMode addressMode;
	BorderColor borderColor;
	CompareOp compareOp;
};

struct Sampler
{
	VkSampler handle;
};

typedef u32 SamplerH;

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
	u32 currentImageIndex;
};

struct Alignment
{
	u32 uniformBufferOffset;
	u32 optimalBufferCopyOffset;
	u32 optimalBufferCopyRowPitch;
	// u32 nonCoherentAtomSize;
	// VkExtent3D minImageTransferGranularity;
};

struct CommandList
{
	VkCommandBuffer handle;

	VkDescriptorSet descriptorSetHandles[MAX_DESCRIPTOR_SETS];
	u8 descriptorSetDirtyMask;

	const Pipeline *pipeline;

	// State
	union
	{
		struct // For gfx pipelines
		{
			VkClearValue clearValues[MAX_RENDER_TARGETS];
			VkBuffer vertexBufferHandle;
			VkBuffer indexBufferHandle;
		};
	};
};

struct SubmitResult
{
	VkSemaphore signalSemaphore;
};

struct GraphicsDevice
{
	VkInstance instance;

	VkPhysicalDevice physicalDevice;
	VkDevice handle;

	Format defaultDepthFormat;

	Alignment alignment;

	u32 graphicsQueueFamilyIndex;
	u32 presentQueueFamilyIndex;
	VkQueue graphicsQueue;
	VkQueue presentQueue;

	VkCommandPool commandPools[MAX_FRAMES_IN_FLIGHT];
	VkCommandBuffer commandBuffers[MAX_FRAMES_IN_FLIGHT];
	VkCommandPool transientCommandPool;

	VkSurfaceKHR surface;
	SwapchainInfo swapchainInfo;
	Swapchain swapchain;

	VkSemaphore imageAvailableSemaphores[MAX_FRAMES_IN_FLIGHT];
	VkSemaphore renderFinishedSemaphores[MAX_FRAMES_IN_FLIGHT];

	VkFence fences[MAX_FENCES];
	u32 firstFenceIndex;
	u32 usedFenceCount;

	struct FrameData
	{
		u32 firstFenceIndex;
		u32 usedFenceCount;
	};

	FrameData frameData[MAX_FRAMES_IN_FLIGHT];

	VkPipelineCache pipelineCache;

	VkDescriptorSetLayout nullDescriptorSetLayout;

	Heap heaps[HeapType_COUNT];

	u32 currentFrame;

	struct
	{
		bool debugReportCallbacks;
	} support;

	VkDebugReportCallbackEXT debugReportCallback;
};


////////////////////////////////////////////////////////////////////////
// Helper functions to map Vulkan types
////////////////////////////////////////////////////////////////////////

static const char *VkPhysicalDeviceTypeToString( VkPhysicalDeviceType type )
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

static const char *VkMemoryPropertyFlagsToString( VkMemoryPropertyFlags flags, char *outString )
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

static const char *VkMemoryHeapFlagsToString( VkMemoryHeapFlags flags, char *outString )
{
	*outString = 0;
	CONCAT_FLAG(VK_MEMORY_HEAP_DEVICE_LOCAL_BIT);
	CONCAT_FLAG(VK_MEMORY_HEAP_MULTI_INSTANCE_BIT);
	return outString;
}

#undef CONCAT_FLAG

static const char *HeapTypeToString(HeapType heapType)
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

static VkMemoryPropertyFlags HeapTypeToVkMemoryPropertyFlags( HeapType heapType )
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

static VkDescriptorType SpvDescriptorTypeToVulkan(SpvType type)
{
	static const VkDescriptorType vkDescriptorTypes[] = {
		VK_DESCRIPTOR_TYPE_MAX_ENUM,
		VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE,
		VK_DESCRIPTOR_TYPE_SAMPLER,
		VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER,
		VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER,
		VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, // Should be DYNAMIC if we use dynamic offsets
		VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER,
	};
	CT_ASSERT(ARRAY_COUNT(vkDescriptorTypes) == SpvTypeCount);
	ASSERT(type != SpvTypeNone);
	ASSERT(type < SpvTypeCount);
	const VkDescriptorType vkDescriptorType = vkDescriptorTypes[type];
	return vkDescriptorType;
}

static VkShaderStageFlags SpvStageFlagsToVulkan(u8 stageFlags)
{
	VkShaderStageFlags vkStageFlags = 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsVertexBit ) ? VK_SHADER_STAGE_VERTEX_BIT : 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsFragmentBit ) ? VK_SHADER_STAGE_FRAGMENT_BIT : 0;
	vkStageFlags |= ( stageFlags & SpvStageFlagsComputeBit ) ? VK_SHADER_STAGE_COMPUTE_BIT : 0;
	return vkStageFlags;
}

static VkFormat FormatToVulkan(Format format)
{
	ASSERT(format < FormatCount);
	static VkFormat vkFormats[] = {
		VK_FORMAT_R32_SFLOAT,
		VK_FORMAT_R32G32_SFLOAT,
		VK_FORMAT_R32G32B32_SFLOAT,
		VK_FORMAT_R8G8B8A8_SRGB,
		VK_FORMAT_B8G8R8A8_SRGB,
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT,
	};
	CT_ASSERT(ARRAY_COUNT(vkFormats) == FormatCount);
	const VkFormat vkFormat = vkFormats[format];
	return vkFormat;
}

static Format FormatFromVulkan(VkFormat format)
{
	switch (format) {
		case VK_FORMAT_R32_SFLOAT: return FormatFloat;
		case VK_FORMAT_R32G32_SFLOAT: return FormatFloat2;
		case VK_FORMAT_R32G32B32_SFLOAT: return FormatFloat3;
		case VK_FORMAT_R8G8B8A8_SRGB: return FormatRGBA8_SRGB;
		case VK_FORMAT_B8G8R8A8_SRGB: return FormatBGRA8_SRGB;
		case VK_FORMAT_D32_SFLOAT: return FormatD32;
		case VK_FORMAT_D32_SFLOAT_S8_UINT: return FormatD32S1;
		case VK_FORMAT_D24_UNORM_S8_UINT: return FormatD24S1;
		default:;
	};
	INVALID_CODE_PATH();
	return FormatCount;
}

static VkBorderColor BorderColorToVulkan(BorderColor color)
{
	static const VkBorderColor vkBorderColors[] = {
		VK_BORDER_COLOR_INT_OPAQUE_BLACK,
		VK_BORDER_COLOR_INT_OPAQUE_WHITE,
		VK_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
		VK_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
	};
	CT_ASSERT(ARRAY_COUNT(vkBorderColors) == BorderColorCount);
	ASSERT(color < BorderColorCount);
	const VkBorderColor vkBorderColor = vkBorderColors[color];
	return vkBorderColor;
};

static VkSamplerAddressMode AddressModeToVulkan(AddressMode mode)
{
	static const VkSamplerAddressMode vkAddressModes[] = {
		VK_SAMPLER_ADDRESS_MODE_REPEAT,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_BORDER,
		VK_SAMPLER_ADDRESS_MODE_CLAMP_TO_EDGE,
	};
	CT_ASSERT(ARRAY_COUNT(vkAddressModes) == AddressModeCount);
	ASSERT(mode < AddressModeCount);
	const VkSamplerAddressMode vkAddressMode = vkAddressModes[mode];
	return vkAddressMode;
}

static VkCompareOp CompareOpToVulkan(CompareOp compareOp)
{
	static const VkCompareOp vkCompareOps[] = {
		VK_COMPARE_OP_NEVER,
		VK_COMPARE_OP_LESS,
		VK_COMPARE_OP_GREATER,
		VK_COMPARE_OP_GREATER_OR_EQUAL,
	};
	CT_ASSERT(ARRAY_COUNT(vkCompareOps) == CompareOpCount);
	ASSERT(compareOp < CompareOpCount);
	const VkCompareOp vkCompareOp = vkCompareOps[compareOp];
	return vkCompareOp;
}

static VkBufferUsageFlags BufferUsageFlagsToVulkan(BufferUsageFlags flags)
{
	VkBufferUsageFlags vkFlags = 0;
	vkFlags |= ( flags & BufferUsageTransferSrc ) ? VK_BUFFER_USAGE_TRANSFER_SRC_BIT : 0;
	vkFlags |= ( flags & BufferUsageTransferDst ) ? VK_BUFFER_USAGE_TRANSFER_DST_BIT : 0;
	vkFlags |= ( flags & BufferUsageUniformBuffer ) ? VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageUniformTexelBuffer ) ? VK_BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageStorageBuffer ) ? VK_BUFFER_USAGE_STORAGE_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageStorageTexelBuffer ) ? VK_BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageVertexBuffer ) ? VK_BUFFER_USAGE_VERTEX_BUFFER_BIT : 0;
	vkFlags |= ( flags & BufferUsageIndexBuffer ) ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT : 0;
	return vkFlags;
}

static VkImageUsageFlags ImageUsageFlagsToVulkan(ImageUsageFlags flags)
{
	VkImageUsageFlags vkFlags = 0;
	vkFlags |= ( flags & ImageUsageTransferSrc ) ? VK_IMAGE_USAGE_TRANSFER_SRC_BIT : 0;
	vkFlags |= ( flags & ImageUsageTransferDst ) ? VK_IMAGE_USAGE_TRANSFER_DST_BIT : 0;
	vkFlags |= ( flags & ImageUsageSampled ) ? VK_IMAGE_USAGE_SAMPLED_BIT : 0;
	vkFlags |= ( flags & ImageUsageStorage ) ? VK_IMAGE_USAGE_STORAGE_BIT : 0;
	vkFlags |= ( flags & ImageUsageColorAttachment ) ? VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageDepthStencilAttachment ) ? VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageTransient ) ? VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT : 0;
	vkFlags |= ( flags & ImageUsageInputAttachment ) ? VK_IMAGE_USAGE_INPUT_ATTACHMENT_BIT : 0;
	return vkFlags;
}

//static bool IsDepthFormat(VkFormat format)
//{
//	const bool isDepthFormat =
//		format == VK_FORMAT_D24_UNORM_S8_UINT ||
//		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
//		format == VK_FORMAT_D32_SFLOAT ||
//		format == VK_FORMAT_D16_UNORM_S8_UINT ||
//		format == VK_FORMAT_D16_UNORM;
//	return isDepthFormat;
//}
static bool IsDepthFormat(Format format)
{
	const bool isDepthFormat =
		format == FormatD32S1 ||
		format == FormatD24S1 ||
		format == FormatD32;
	return isDepthFormat;
}

//static bool HasStencilComponent(VkFormat format)
//{
//	const bool hasStencil =
//		format == VK_FORMAT_D32_SFLOAT_S8_UINT ||
//		format == VK_FORMAT_D24_UNORM_S8_UINT ||
//		format == VK_FORMAT_D16_UNORM_S8_UINT;
//	return hasStencil;
//}
static bool HasStencilComponent(Format format)
{
	const bool hasStencil =
		format == FormatD32S1 ||
		format == FormatD24S1;
	return hasStencil;
}

static VkAttachmentLoadOp LoadOpToVulkan( LoadOp loadOp )
{
	ASSERT(loadOp <= LoadOpDontCare);
	static VkAttachmentLoadOp vkLoadOps[] = {
		VK_ATTACHMENT_LOAD_OP_LOAD,
		VK_ATTACHMENT_LOAD_OP_CLEAR,
		VK_ATTACHMENT_LOAD_OP_DONT_CARE,
	};
	const VkAttachmentLoadOp vkLoadOp = vkLoadOps[loadOp];
	return vkLoadOp;
}

static VkAttachmentStoreOp StoreOpToVulkan( StoreOp storeOp )
{
	ASSERT(storeOp <= StoreOpDontCare);
	static VkAttachmentStoreOp vkStoreOps[] = {
		VK_ATTACHMENT_STORE_OP_STORE,
		VK_ATTACHMENT_STORE_OP_DONT_CARE,
	};
	const VkAttachmentStoreOp vkStoreOp = vkStoreOps[storeOp];
	return vkStoreOp;
}



////////////////////////////////////////////////////////////////////////
// Internal functions
////////////////////////////////////////////////////////////////////////

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
		case VK_ERROR_OUT_OF_POOL_MEMORY: return "VK_ERROR_OUT_OF_POOL_MEMORY";
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

static Heap CreateHeap(const GraphicsDevice &device, HeapType heapType, u32 size, bool mapMemory)
{
	u32 memoryTypeIndex = -1;

	VkMemoryPropertyFlags requiredFlags = HeapTypeToVkMemoryPropertyFlags( heapType );

	VkPhysicalDeviceMemoryProperties memoryProperties;
	vkGetPhysicalDeviceMemoryProperties(device.physicalDevice, &memoryProperties);

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
	VK_CALL( vkAllocateMemory(device.handle, &memoryAllocateInfo, VULKAN_ALLOCATORS, &memory) );

	u8 *data = 0;
	if ( mapMemory )
	{
		VK_CALL( vkMapMemory(device.handle, memory, 0, size, 0, (void**)&data) );
	}

	Heap heap = {};
	heap.type = heapType;
	heap.size = size;
	heap.memoryTypeIndex = memoryTypeIndex;
	heap.memory = memory;
	heap.data = data;
	return heap;
}

static void DestroyHeap(const GraphicsDevice &device, const Heap &heap)
{
	vkFreeMemory(device.handle, heap.memory, VULKAN_ALLOCATORS);
}

static ShaderModule CreateShaderModule(const GraphicsDevice &device, const ShaderSource &source)
{
	const VkShaderModuleCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
		.codeSize = source.dataSize,
		.pCode = reinterpret_cast<const u32*>(source.data),
	};

	VkShaderModule shaderModuleHandle;
	if ( vkCreateShaderModule(device.handle, &createInfo, VULKAN_ALLOCATORS, &shaderModuleHandle) != VK_SUCCESS )
	{
		LOG(Error, "Error in CreateShaderModule.\n");
		shaderModuleHandle = VK_NULL_HANDLE;
	}

	const ShaderModule shaderModule = {
		.handle = shaderModuleHandle,
	};
	return shaderModule;
}

static void DestroyShaderModule(const GraphicsDevice &device, const ShaderModule &shaderModule)
{
	vkDestroyShaderModule(device.handle, shaderModule.handle, VULKAN_ALLOCATORS);
}

static void UpdateDescriptorSets(const GraphicsDevice &device, VkWriteDescriptorSet *descriptorWrites, u32 descriptorWriteCount)
{
	if ( descriptorWriteCount > 0 )
	{
		vkUpdateDescriptorSets(device.handle, descriptorWriteCount, descriptorWrites, 0, NULL);
	}
}

static const ShaderBinding *GetBindGroupBindingPointer(const ShaderBindings &shaderBindings, u8 bindGroupIndex)
{
	for (u32 i = 0; i < shaderBindings.bindingCount; ++i) {
		if ( shaderBindings.bindings[i].set == bindGroupIndex ) {
			return &shaderBindings.bindings[i];
		}
	}
	return NULL;
}

union VkDescriptorGenericInfo
{
	VkDescriptorImageInfo imageInfo;
	VkDescriptorBufferInfo bufferInfo;
	VkBufferView bufferView;
};

static const ResourceBinding &GetResourceBinding(const ResourceBinding resourceBindings[MAX_SHADER_BINDINGS], u8 bindingIndex)
{
	ASSERT(bindingIndex < MAX_SHADER_BINDINGS);
	const ResourceBinding &resourceBinding = resourceBindings[bindingIndex];
	if ( resourceBinding.index == bindingIndex ) {
		return resourceBinding;
	}
	INVALID_CODE_PATH();
	static ResourceBinding nullResourceBinding = {};
	return nullResourceBinding;
}

static bool AddDescriptorWrite(const ResourceBinding resourceBindings[MAX_SHADER_BINDINGS], const ShaderBinding &binding, VkDescriptorSet descriptorSet, VkDescriptorGenericInfo *descriptorInfos, VkWriteDescriptorSet *descriptorWrites, u32 &descriptorWriteCount)
{
	const ResourceBinding &resourceBinding = GetResourceBinding(resourceBindings, binding.binding);

	VkDescriptorImageInfo *imageInfo = 0;
	VkDescriptorBufferInfo *bufferInfo = 0;
	VkBufferView *bufferView = 0;

	if ( binding.type == SpvTypeSampler )
	{
		imageInfo = &descriptorInfos[descriptorWriteCount].imageInfo;
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = VK_NULL_HANDLE;
		imageInfo->sampler = resourceBinding.resource.sampler.handle;
	}
	else if ( binding.type == SpvTypeImage )
	{
		imageInfo = &descriptorInfos[descriptorWriteCount].imageInfo;
		imageInfo->imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
		imageInfo->imageView = resourceBinding.resource.texture.handle;
		imageInfo->sampler = VK_NULL_HANDLE;
	}
	else if ( binding.type == SpvTypeUniformBuffer || binding.type == SpvTypeStorageBuffer )
	{
		bufferInfo = &descriptorInfos[descriptorWriteCount].bufferInfo;
		bufferInfo->buffer = resourceBinding.resource.buffer.handle;
		bufferInfo->offset = resourceBinding.resource.buffer.offset;
		bufferInfo->range = resourceBinding.resource.buffer.range;
	}
	else if ( binding.type == SpvTypeStorageTexelBuffer )
	{
		bufferView = &descriptorInfos[descriptorWriteCount].bufferView;
		*bufferView = resourceBinding.resource.bufferView.handle;
	}
	else
	{
		LOG(Warning, "Unhandled descriptor type (%u) for binding %s.\n", binding.type, binding.name);
		return false;
	}

	VkWriteDescriptorSet *descriptorWrite = &descriptorWrites[descriptorWriteCount++];
	descriptorWrite->sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
	descriptorWrite->dstSet = descriptorSet;
	descriptorWrite->dstBinding = binding.binding;
	descriptorWrite->dstArrayElement = 0;
	descriptorWrite->descriptorType = SpvDescriptorTypeToVulkan( binding.type );
	descriptorWrite->descriptorCount = 1;
	descriptorWrite->pBufferInfo = bufferInfo;
	descriptorWrite->pImageInfo = imageInfo;
	descriptorWrite->pTexelBufferView = bufferView;
	return true;
}

static ShaderSource GetShaderSource(Arena &arena, const char *filename)
{
	FilePath shaderPath = MakePath(filename);
	DataChunk *chunk = PushFile( arena, shaderPath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open shader file %s.\n", shaderPath.str );
		QUIT_ABNORMALLY();
	}
	ShaderSource shaderSource = { chunk->bytes, chunk->size };
	return shaderSource;
}

static VkFormat FindSupportedFormat(const GraphicsDevice &device, const VkFormat candidates[], u32 candidateCount, VkImageTiling tiling, VkFormatFeatureFlags features)
{
	for (u32 i = 0; i < candidateCount; ++i)
	{
		VkFormat format = candidates[i];
		VkFormatProperties properties;
		vkGetPhysicalDeviceFormatProperties(device.physicalDevice, format, &properties);

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

static ShaderBindings ReflectShaderBindings( Arena scratch, byte* microcodeData[], const u64 microcodeSize[], u32 microcodeCount)
{
	void *tempMem = scratch.base + scratch.used;
	const u32 tempMemSize = scratch.size - scratch.used;

	SpvDescriptorSetList spvDescriptorList = {};
	for (u32 i = 0; i < microcodeCount; ++i)
	{
		SpvParser parser = SpvParserInit( microcodeData[i], microcodeSize[i] );
		SpvParseDescriptors( &parser, &spvDescriptorList, tempMem, tempMemSize );
	}

	ShaderBindings shaderBindings = {};

	for (u32 setIndex = 0; setIndex < SPV_MAX_DESCRIPTOR_SETS; ++setIndex)
	{
		for (u32 bindingIndex = 0; bindingIndex < SPV_MAX_DESCRIPTORS_PER_SET; ++bindingIndex)
		{
			SpvDescriptor &descriptor = spvDescriptorList.sets[setIndex].bindings[bindingIndex];

			if ( descriptor.type != SpvTypeNone )
			{
				ASSERT( shaderBindings.bindingCount < ARRAY_COUNT(shaderBindings.bindings) );
				ShaderBinding &shaderBinding = shaderBindings.bindings[shaderBindings.bindingCount++];
				shaderBinding.binding = descriptor.binding;
				shaderBinding.set = setIndex;
				shaderBinding.type = (SpvType)descriptor.type;
				shaderBinding.stageFlags = descriptor.stageFlags;
				shaderBinding.name = InternString( descriptor.name );
				//LOG(Info, "Descriptor name: %s\n", descriptor.name);
			}
		}
	}

	return shaderBindings;
}

static ShaderBindings ReflectShaderBindings( Arena scratch, const ShaderSource &source )
{
	byte* microcodeData[] = { source.data };
	const u64 microcodeSize[] = { source.dataSize };
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, microcodeData, microcodeSize, ARRAY_COUNT(microcodeData));
	return shaderBindings;
}

static ShaderBindings ReflectShaderBindings( Arena scratch, const ShaderSource &vertexSource, const ShaderSource &fragmentSource )
{
	byte* microcodeData[] = { vertexSource.data, fragmentSource.data };
	const u64 microcodeSize[] = { vertexSource.dataSize, fragmentSource.dataSize };
	ShaderBindings shaderBindings = ReflectShaderBindings(scratch, microcodeData, microcodeSize, ARRAY_COUNT(microcodeData));
	return shaderBindings;
}

static void BindDescriptorSets(CommandList &commandList)
{
	while ( commandList.descriptorSetDirtyMask )
	{
		const u32 descriptorSetFirst = CTZ(commandList.descriptorSetDirtyMask);

		u32 descriptorSetCount = 0;
		VkDescriptorSet descriptorSets[MAX_DESCRIPTOR_SETS] = {};

		for (u32 i = descriptorSetFirst; i < MAX_DESCRIPTOR_SETS; ++i )
		{
			const VkDescriptorSet descriptorSet = commandList.descriptorSetHandles[i];

			if ( commandList.descriptorSetDirtyMask & (1 << i) )
			{
				commandList.descriptorSetDirtyMask &= ~(1 << i);

				if ( descriptorSet )
				{
					descriptorSets[descriptorSetCount++] = commandList.descriptorSetHandles[i];
				}
			}
			else
			{
				break;
			}
		}

		if ( descriptorSetCount > 0 )
		{
			VkPipelineBindPoint bindPoint = commandList.pipeline->bindPoint;
			VkPipelineLayout pipelineLayout = commandList.pipeline->layout.handle;
			vkCmdBindDescriptorSets(commandList.handle, bindPoint, pipelineLayout, descriptorSetFirst, descriptorSetCount, descriptorSets, 0, NULL);
		}
	}
}

static VkImageAspectFlags FormatToVulkanAspect(Format format)
{
	VkImageAspectFlags aspectMask = 0;
	aspectMask |= IsDepthFormat(format) ? VK_IMAGE_ASPECT_DEPTH_BIT : 0;
	aspectMask |= HasStencilComponent(format) ? VK_IMAGE_ASPECT_STENCIL_BIT : 0;
	aspectMask = aspectMask ? aspectMask : VK_IMAGE_ASPECT_COLOR_BIT;
	return aspectMask;
}

static VkImageView CreateImageView(const GraphicsDevice &device, const Image &image)
{
	const VkImageViewCreateInfo viewInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
		.image = image.handle,
		.viewType = VK_IMAGE_VIEW_TYPE_2D,
		.format = FormatToVulkan(image.format),
		.components = {
			.r = VK_COMPONENT_SWIZZLE_IDENTITY,
			.g = VK_COMPONENT_SWIZZLE_IDENTITY,
			.b = VK_COMPONENT_SWIZZLE_IDENTITY,
			.a = VK_COMPONENT_SWIZZLE_IDENTITY,
		},
		.subresourceRange = {
			.aspectMask = FormatToVulkanAspect(image.format),
			.baseMipLevel = 0,
			.levelCount = image.mipLevels,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	VkImageView imageView;
	VK_CALL( vkCreateImageView(device.handle, &viewInfo, VULKAN_ALLOCATORS, &imageView) );
	return imageView;
}

static void DestroyImageView(const GraphicsDevice &device, VkImageView imageView)
{
	vkDestroyImageView(device.handle, imageView, VULKAN_ALLOCATORS);
}







////////////////////////////////////////////////////////////////////////
// Public API
////////////////////////////////////////////////////////////////////////

//////////////////////////////
// WaitIdle
//////////////////////////////

void WaitQueueIdle(const GraphicsDevice &device)
{
	vkQueueWaitIdle(device.graphicsQueue);
}

void WaitDeviceIdle(const GraphicsDevice &device)
{
	vkDeviceWaitIdle( device.handle );
}


//////////////////////////////
// Device
//////////////////////////////

Swapchain CreateSwapchain(const GraphicsDevice &device, Window &window, const SwapchainInfo &swapchainInfo)
{
	Swapchain swapchain = {};

	VkSurfaceCapabilitiesKHR surfaceCapabilities;
	vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device.physicalDevice, device.surface, &surfaceCapabilities);

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
		device.graphicsQueueFamilyIndex,
		device.presentQueueFamilyIndex
	};

	VkSharingMode imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
	u32 queueFamilyIndexCount = 0;
	u32 *pQueueFamilyIndices = NULL;
	if ( device.graphicsQueueFamilyIndex != device.presentQueueFamilyIndex )
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
	const VkSwapchainCreateInfoKHR swapchainCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
		.surface = device.surface,
		.minImageCount = imageCount,
		.imageFormat = swapchainInfo.format,
		.imageColorSpace = swapchainInfo.colorSpace,
		.imageExtent = swapchain.extent,
		.imageArrayLayers = 1,
		.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT, // we will render directly on it
		//.imageUsage = VK_IMAGE_USAGE_TRANSFER_DST_BIT, // for typical engines with several render passes before
		.imageSharingMode = imageSharingMode,
		.queueFamilyIndexCount = queueFamilyIndexCount,
		.pQueueFamilyIndices = pQueueFamilyIndices,
		.preTransform = preTransform,
		.compositeAlpha = compositeAlpha,
		.presentMode = swapchainInfo.presentMode,
		.clipped = VK_TRUE, // Don't care about pixels obscured by other windows
		.oldSwapchain = VK_NULL_HANDLE,
	};

	VK_CALL( vkCreateSwapchainKHR(device.handle, &swapchainCreateInfo, VULKAN_ALLOCATORS, &swapchain.handle) );


	// Get the swapchain images
	vkGetSwapchainImagesKHR( device.handle, swapchain.handle, &swapchain.imageCount, NULL );
	ASSERT( swapchain.imageCount <= ARRAY_COUNT(swapchain.images) );
	vkGetSwapchainImagesKHR( device.handle, swapchain.handle, &swapchain.imageCount, swapchain.images );


	// Create image views
	for ( u32 i = 0; i < swapchain.imageCount; ++i )
	{
		const Image image = {
			.handle = swapchain.images[i],
			.format = FormatFromVulkan(swapchainInfo.format),
			.mipLevels = 1,
		};
		swapchain.imageViews[i] = CreateImageView(device, image);
	}


	return swapchain;
}

void DestroySwapchain(const GraphicsDevice &device, Swapchain &swapchain)
{
	for ( u32 i = 0; i < swapchain.imageCount; ++i )
	{
		vkDestroyImageView(device.handle, swapchain.imageViews[i], VULKAN_ALLOCATORS);
	}

	vkDestroySwapchainKHR(device.handle, swapchain.handle, VULKAN_ALLOCATORS);

	swapchain = {};
}

bool InitializeGraphicsDriver(GraphicsDevice &device, Arena scratch)
{
	// Initialize Volk -- load basic Vulkan function pointers
	VkResult result = volkInitialize();
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "The Vulkan loader was not found in the system.\n");
		return false;
	}


	// Instance creation
	const VkApplicationInfo applicationInfo = {
		.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
		.pApplicationName = "Vulkan application",
		.applicationVersion = VK_MAKE_VERSION(1, 0, 0),
		.pEngineName = "Vulkan engine",
		.engineVersion = VK_MAKE_VERSION(1, 0, 0),
		.apiVersion = VK_API_VERSION_1_1,
	};

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

	const VkInstanceCreateInfo instanceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
		.pApplicationInfo = &applicationInfo,
#if USE_VK_EXT_PORTABILITY_ENUMERATION
		.flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR,
#endif
		.enabledLayerCount = enabledInstanceLayerCount,
		.ppEnabledLayerNames = enabledInstanceLayerNames,
		.enabledExtensionCount = enabledInstanceExtensionCount,
		.ppEnabledExtensionNames = enabledInstanceExtensionNames,
	};

	VK_CALL ( vkCreateInstance( &instanceCreateInfo, VULKAN_ALLOCATORS, &device.instance ) );


	// Load the instance-related Vulkan function pointers
	volkLoadInstanceOnly(device.instance);


	// Report callback
	if ( vkCreateDebugReportCallbackEXT )
	{
		device.support.debugReportCallbacks = true;

		const VkDebugReportCallbackCreateInfoEXT debugReportCallbackCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DEBUG_REPORT_CALLBACK_CREATE_INFO_EXT,
			.flags = 0
		//		| VK_DEBUG_REPORT_INFORMATION_BIT_EXT;
				| VK_DEBUG_REPORT_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_PERFORMANCE_WARNING_BIT_EXT
				| VK_DEBUG_REPORT_ERROR_BIT_EXT,
			.pfnCallback = VulkanDebugReportCallback,
			.pUserData = 0,
		};

		VK_CALL( vkCreateDebugReportCallbackEXT( device.instance, &debugReportCallbackCreateInfo, VULKAN_ALLOCATORS, &device.debugReportCallback) );
	}

	return true;
}

void CleanupGraphicsDriver(GraphicsDevice &device)
{
	if ( device.support.debugReportCallbacks )
	{
		vkDestroyDebugReportCallbackEXT( device.instance, device.debugReportCallback, VULKAN_ALLOCATORS );
	}

	vkDestroyInstance(device.instance, VULKAN_ALLOCATORS);
}

bool InitializeGraphicsSurface(GraphicsDevice &device, const Window &window)
{
#if VK_USE_PLATFORM_XCB_KHR
	const VkXcbSurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_XCB_SURFACE_CREATE_INFO_KHR,
		.connection = window.connection,
		.window = window.window,
	};
	VK_CALL( vkCreateXcbSurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#elif VK_USE_PLATFORM_ANDROID_KHR
	const VkAndroidSurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_ANDROID_SURFACE_CREATE_INFO_KHR,
		.window = window.nativeWindow,
	};
	VK_CALL( vkCreateAndroidSurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#elif VK_USE_PLATFORM_WIN32_KHR
	const VkWin32SurfaceCreateInfoKHR surfaceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR,
		.hinstance = window.hInstance,
		.hwnd = window.hWnd,
	};
	VK_CALL( vkCreateWin32SurfaceKHR( device.instance, &surfaceCreateInfo, VULKAN_ALLOCATORS, &device.surface ) );
#endif

	return true;
}

void CleanupGraphicsSurface(const GraphicsDevice &device)
{
	vkDestroySurfaceKHR(device.instance, device.surface, VULKAN_ALLOCATORS);
}

bool InitializeGraphicsDevice(GraphicsDevice &device, Arena scratch, Window &window)
{
	VkResult result = VK_RESULT_MAX_ENUM;


	// List of physical devices
	u32 physicalDeviceCount = 0;
	VK_CALL( vkEnumeratePhysicalDevices( device.instance, &physicalDeviceCount, NULL ) );
	VkPhysicalDevice *physicalDevices = PushArray( scratch, VkPhysicalDevice, physicalDeviceCount );
	VK_CALL( vkEnumeratePhysicalDevices( device.instance, &physicalDeviceCount, physicalDevices ) );

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
			vkGetPhysicalDeviceSurfaceSupportKHR( physicalDevice, i, device.surface, &presentSupport );
			if ( presentSupport )
			{
				presentFamilyIndex = i;
			}

			// We want the gfx queue to support both gfx and compute tasks
			if ((queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT) &&
				(queueFamilies[i].queueFlags & VK_QUEUE_COMPUTE_BIT))
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
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.surface, &surfaceFormatCount, NULL);
		if ( surfaceFormatCount == 0 )
			continue;
		VkSurfaceFormatKHR *surfaceFormats = PushArray( scratch2, VkSurfaceFormatKHR, surfaceFormatCount );
		vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, device.surface, &surfaceFormatCount, surfaceFormats);

		device.swapchainInfo.format = VK_FORMAT_MAX_ENUM;
		for ( u32 i = 0; i < surfaceFormatCount; ++i )
		{
			if ( ( surfaceFormats[i].format == VK_FORMAT_R8G8B8A8_SRGB || surfaceFormats[i].format == VK_FORMAT_B8G8R8A8_SRGB ) &&
					surfaceFormats[i].colorSpace == VK_COLORSPACE_SRGB_NONLINEAR_KHR )
			{
				device.swapchainInfo.format = surfaceFormats[i].format;
				device.swapchainInfo.colorSpace = surfaceFormats[i].colorSpace;
				break;
			}
		}
		if ( device.swapchainInfo.format == VK_FORMAT_MAX_ENUM )
		{
			device.swapchainInfo.format = surfaceFormats[0].format;
			device.swapchainInfo.colorSpace = surfaceFormats[0].colorSpace;
		}

		// Swapchain present mode
		u32 surfacePresentModeCount = 0;
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, device.surface, &surfacePresentModeCount, NULL);
		if ( surfacePresentModeCount == 0 )
			continue;
		VkPresentModeKHR *surfacePresentModes = PushArray( scratch2, VkPresentModeKHR, surfacePresentModeCount );
		vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, device.surface, &surfacePresentModeCount, surfacePresentModes);

#if USE_SWAPCHAIN_MAILBOX_PRESENT_MODE
		device.swapchainPresentMode = VK_PRESENT_MODE_MAX_ENUM_KHR;
		for ( u32 i = 0; i < surfacePresentModeCount; ++i )
		{
			if ( surfacePresentModes[i] == VK_PRESENT_MODE_MAILBOX_KHR )
			{
				device.swapchainPresentMode = surfacePresentModes[i];
			}
		}
		if ( device.swapchainInfo.presentMode == VK_PRESENT_MODE_MAILBOX_KHR )
			device.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#else
		device.swapchainInfo.presentMode = VK_PRESENT_MODE_FIFO_KHR;
#endif

		// At this point, we know this device meets all the requirements
		suitableDeviceFound = true;
		device.physicalDevice = physicalDevice;
		device.graphicsQueueFamilyIndex = gfxFamilyIndex;
		device.presentQueueFamilyIndex = presentFamilyIndex;
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
	queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = device.graphicsQueueFamilyIndex;
	queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
	queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	if ( device.presentQueueFamilyIndex != device.graphicsQueueFamilyIndex )
	{
		queueCreateInfoIndex = queueCreateInfoCount++;
		queueCreateInfos[queueCreateInfoIndex].sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
		queueCreateInfos[queueCreateInfoIndex].queueFamilyIndex = device.presentQueueFamilyIndex;
		queueCreateInfos[queueCreateInfoIndex].queueCount = queueCount;
		queueCreateInfos[queueCreateInfoIndex].pQueuePriorities = queuePriorities;
	}

#if 0
	u32 deviceExtensionCount;
	VK_CALL( vkEnumerateDeviceExtensionProperties( device.physicalDevice, NULL, &deviceExtensionCount, NULL ) );
	VkExtensionProperties *deviceExtensions = PushArray(scratch, VkExtensionProperties, deviceExtensionCount);
	VK_CALL( vkEnumerateDeviceExtensionProperties( device.physicalDevice, NULL, &deviceExtensionCount, deviceExtensions ) );

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

	const VkDeviceCreateInfo deviceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
		.queueCreateInfoCount = queueCreateInfoCount,
		.pQueueCreateInfos = queueCreateInfos,
		.enabledLayerCount = 0,
		.ppEnabledLayerNames = NULL,
		.enabledExtensionCount = enabledDeviceExtensionCount,
		.ppEnabledExtensionNames = enabledDeviceExtensionNames,
		.pEnabledFeatures = &requiredPhysicalDeviceFeatures,
	};

	result = vkCreateDevice( device.physicalDevice, &deviceCreateInfo, VULKAN_ALLOCATORS, &device.handle );
	if ( result != VK_SUCCESS )
	{
		LOG(Error, "vkCreateDevice failed!\n");
		return false;
	}


	// Load all the remaining device-related Vulkan function pointers
	volkLoadDevice(device.handle);


	// Print physical device info
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device.physicalDevice, &properties);
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


	// Format support
	const VkFormat depthFormatCandidates[] = {
		VK_FORMAT_D32_SFLOAT,
		VK_FORMAT_D32_SFLOAT_S8_UINT,
		VK_FORMAT_D24_UNORM_S8_UINT
	};
	const VkFormat defaultVkDepthFormat = FindSupportedFormat(device,
			depthFormatCandidates, ARRAY_COUNT(depthFormatCandidates),
			VK_IMAGE_TILING_OPTIMAL,
			VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT);
	device.defaultDepthFormat = FormatFromVulkan(defaultVkDepthFormat);

	// Get alignments
	device.alignment.uniformBufferOffset = properties.limits.minUniformBufferOffsetAlignment;
	device.alignment.optimalBufferCopyOffset = properties.limits.optimalBufferCopyOffsetAlignment;
	device.alignment.optimalBufferCopyRowPitch = properties.limits.optimalBufferCopyRowPitchAlignment;


	// Create heaps
	device.heaps[HeapType_General] = CreateHeap(device, HeapType_General, MB(16), false);
	device.heaps[HeapType_RTs] = CreateHeap(device, HeapType_RTs, MB(64), false);
	device.heaps[HeapType_Staging] = CreateHeap(device, HeapType_Staging, MB(16), true);
	device.heaps[HeapType_Dynamic] = CreateHeap(device, HeapType_Dynamic, MB(16), true);
	//device.heaps[HeapType_Readback] = CreateHeap(gfx, HeapType_Readback, 0);


	// Retrieve queues
	vkGetDeviceQueue(device.handle, device.graphicsQueueFamilyIndex, 0, &device.graphicsQueue);
	vkGetDeviceQueue(device.handle, device.presentQueueFamilyIndex, 0, &device.presentQueue);


	// Command pools
	for (u32 i = 0; i < ARRAY_COUNT(device.commandPools); ++i)
	{
		const VkCommandPoolCreateInfo commandPoolCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
			.flags = 0, // VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT allows individual resets using vkResetCommandBuffer
			.queueFamilyIndex = device.graphicsQueueFamilyIndex,
		};
		VK_CALL( vkCreateCommandPool(device.handle, &commandPoolCreateInfo, VULKAN_ALLOCATORS, &device.commandPools[i]) );
	}


	// Command buffers
	for (u32 i = 0; i < ARRAY_COUNT(device.commandBuffers); ++i)
	{
		const VkCommandBufferAllocateInfo commandBufferAllocInfo = {
			.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
			.commandPool = device.commandPools[i],
			.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
			.commandBufferCount = 1,
		};
		VK_CALL( vkAllocateCommandBuffers( device.handle, &commandBufferAllocInfo, &device.commandBuffers[i]) );
	}


	// Transient command pool
	const VkCommandPoolCreateInfo transientCommandPoolCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
		.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT | VK_COMMAND_POOL_CREATE_TRANSIENT_BIT,
		.queueFamilyIndex = device.graphicsQueueFamilyIndex,
	};
	VK_CALL( vkCreateCommandPool(device.handle, &transientCommandPoolCreateInfo, VULKAN_ALLOCATORS, &device.transientCommandPool) );


	// Create swapchain
	device.swapchain = CreateSwapchain( device, window, device.swapchainInfo );


	// Synchronization objects
	const VkSemaphoreCreateInfo semaphoreCreateInfo = { VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO };

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		VK_CALL( vkCreateSemaphore( device.handle, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &device.imageAvailableSemaphores[i] ) );
		VK_CALL( vkCreateSemaphore( device.handle, &semaphoreCreateInfo, VULKAN_ALLOCATORS, &device.renderFinishedSemaphores[i] ) );
	}

	const VkFenceCreateInfo fenceCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
		//.flags = VK_FENCE_CREATE_SIGNALED_BIT, // Start signaled
	};
	for ( u32 i = 0; i < MAX_FENCES; ++i )
	{
		VK_CALL( vkCreateFence( device.handle, &fenceCreateInfo, VULKAN_ALLOCATORS, &device.fences[i] ) );
	}


	// Create pipeline cache
	const VkPipelineCacheCreateInfo pipelineCacheCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO,
		.flags = 0,
		.initialDataSize = 0,
		.pInitialData = NULL,
	};
	VK_CALL( vkCreatePipelineCache( device.handle, &pipelineCacheCreateInfo, VULKAN_ALLOCATORS, &device.pipelineCache ) );

	// Null descriptor set layout
	const VkDescriptorSetLayoutCreateInfo nullDescriptorSetLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
		.bindingCount = 0,
		.pBindings =  NULL,
	};

	VK_CALL( vkCreateDescriptorSetLayout(device.handle, &nullDescriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &device.nullDescriptorSetLayout) );

	return true;
}

void CleanupGraphicsDevice(const GraphicsDevice &device)
{
	vkDestroyDescriptorSetLayout( device.handle, device.nullDescriptorSetLayout, VULKAN_ALLOCATORS );

	vkDestroyPipelineCache( device.handle, device.pipelineCache, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i )
	{
		vkDestroySemaphore( device.handle, device.imageAvailableSemaphores[i], VULKAN_ALLOCATORS );
		vkDestroySemaphore( device.handle, device.renderFinishedSemaphores[i], VULKAN_ALLOCATORS );
	}

	for ( u32 i = 0; i < MAX_FENCES; ++i )
	{
		vkDestroyFence( device.handle, device.fences[i], VULKAN_ALLOCATORS );
	}

	vkDestroyCommandPool( device.handle, device.transientCommandPool, VULKAN_ALLOCATORS );

	for ( u32 i = 0; i < ARRAY_COUNT(device.commandPools); ++i )
	{
		vkDestroyCommandPool( device.handle, device.commandPools[i], VULKAN_ALLOCATORS );
	}

	for (u32 i = 0; i < HeapType_COUNT; ++i)
	{
		DestroyHeap(device, device.heaps[i]);
	}

	vkDestroyDevice(device.handle, VULKAN_ALLOCATORS);
}


//////////////////////////////
// BindGroupAllocator
//////////////////////////////

BindGroupAllocator CreateBindGroupAllocator(const GraphicsDevice &device, const BindGroupAllocatorCounts &counts)
{
	u32 poolSizeCount = 0;
	VkDescriptorPoolSize poolSizes[8] = {};

	if (counts.uniformBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, counts.uniformBufferCount };
	}
	if (counts.storageBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, counts.storageBufferCount };
	}
	if (counts.storageTexelBufferCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER, counts.storageTexelBufferCount };
	}
	if (counts.textureCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE, counts.textureCount };
	}
	if (counts.samplerCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_SAMPLER, counts.samplerCount };
	}
	if (counts.combinedImageSamplerCount > 0) {
		poolSizes[poolSizeCount++] = { VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, counts.combinedImageSamplerCount };
	}

	ASSERT(poolSizeCount <= ARRAY_COUNT(poolSizes));

	const VkDescriptorPoolCreateInfo createInfo = {
		.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO,
		.flags = counts.allowIndividualFrees ? VK_DESCRIPTOR_POOL_CREATE_FREE_DESCRIPTOR_SET_BIT : 0U,
		.maxSets = counts.groupCount,
		.poolSizeCount = poolSizeCount,
		.pPoolSizes = poolSizes,
	};

	VkDescriptorPool descriptorPool;
	VK_CALL( vkCreateDescriptorPool( device.handle, &createInfo, VULKAN_ALLOCATORS, &descriptorPool) );

	const BindGroupAllocator allocator = {
		.maxCounts = counts,
		.handle = descriptorPool,
	};
	return allocator;
}

void DestroyBindGroupAllocator( const GraphicsDevice &device, const BindGroupAllocator &bindGroupAllocator )
{
	vkDestroyDescriptorPool( device.handle, bindGroupAllocator.handle, VULKAN_ALLOCATORS );
}

void ResetBindGroupAllocator(const GraphicsDevice &device, BindGroupAllocator &bindGroupAllocator)
{
	const VkDescriptorPool descriptorPool = bindGroupAllocator.handle;
	VK_CALL( vkResetDescriptorPool(device.handle, descriptorPool, 0) );

	bindGroupAllocator.usedCounts = {};
}


//////////////////////////////
// BindGroup
//////////////////////////////

BindGroupLayout CreateBindGroupLayout(const GraphicsDevice &device, const ShaderBindings &shaderBindings, u32 bindGroupIndex)
{
	BindGroupLayout layout = {};

	VkDescriptorSetLayoutBinding vkBindings[SPV_MAX_DESCRIPTORS_PER_SET] = {};
	u32 bindingCount = 0;

	for (u32 bindingIndex = 0; bindingIndex < shaderBindings.bindingCount; ++bindingIndex)
	{
		const ShaderBinding &shaderBinding = shaderBindings.bindings[bindingIndex];

		if ( shaderBinding.set == bindGroupIndex )
		{
			VkDescriptorSetLayoutBinding &binding = vkBindings[bindingCount++];
			binding.binding = shaderBinding.binding;
			binding.descriptorType = SpvDescriptorTypeToVulkan((SpvType)shaderBinding.type);
			binding.descriptorCount = 1;
			binding.stageFlags = SpvStageFlagsToVulkan(shaderBinding.stageFlags);
			binding.pImmutableSamplers = NULL;
		}
	}

	if (bindingCount > 0)
	{
		const VkDescriptorSetLayoutCreateInfo descriptorSetLayoutCreateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO,
			.bindingCount = bindingCount,
			.pBindings = vkBindings,
		};

		VkDescriptorSetLayout layoutHandle;
		VK_CALL( vkCreateDescriptorSetLayout(device.handle, &descriptorSetLayoutCreateInfo, VULKAN_ALLOCATORS, &layoutHandle) );

		layout.handle = layoutHandle;
		layout.bindingCount = bindingCount;
		layout.bindings = GetBindGroupBindingPointer(shaderBindings, bindGroupIndex);
	}
	else
	{
		layout.handle = device.nullDescriptorSetLayout;
		layout.bindingCount = 0;
		layout.bindings = NULL;
	}

	return layout;
}

void DestroyBindGroupLayout(const GraphicsDevice &device, const BindGroupLayout &bindGroupLayout)
{
	if ( bindGroupLayout.bindingCount > 0 )
	{
		vkDestroyDescriptorSetLayout( device.handle, bindGroupLayout.handle, VULKAN_ALLOCATORS );
	}
	// Otherwise, if bindingCount == 0, we are using the shared device.nullDescriptorSetLayout
	// which will be destroyed only once in CleanupGraphicsDevice().
}

BindGroup CreateBindGroup(const GraphicsDevice &device, const BindGroupLayout &layout, BindGroupAllocator &allocator)
{
	BindGroup bindGroup = {};

	if (layout.handle)
	{
		const VkDescriptorSetAllocateInfo allocateInfo = {
			.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO,
			.descriptorPool = allocator.handle,
			.descriptorSetCount = 1,
			.pSetLayouts = &layout.handle,
		};
		VK_CALL( vkAllocateDescriptorSets(device.handle, &allocateInfo, &bindGroup.handle) );

		// Update the used descriptor counters in the allocator
		for (u32 i = 0; i < layout.bindingCount; ++i)
		{
			const ShaderBinding &binding = layout.bindings[i];
			switch (binding.type) {
				case SpvTypeImage: allocator.usedCounts.textureCount++; break;
				case SpvTypeSampler: allocator.usedCounts.samplerCount++; break;
				case SpvTypeSampledImage: allocator.usedCounts.combinedImageSamplerCount++; break;
				case SpvTypeUniformBuffer: allocator.usedCounts.uniformBufferCount++; break;
				case SpvTypeStorageBuffer: allocator.usedCounts.storageBufferCount++; break;
				case SpvTypeStorageTexelBuffer: allocator.usedCounts.storageTexelBufferCount++; break;
				default: INVALID_CODE_PATH();
			}
		}
	}

	return bindGroup;
}

BindGroup CreateBindGroup(const GraphicsDevice &device, const BindGroupDesc &desc, BindGroupAllocator &allocator)
{
	BindGroup bindGroup = CreateBindGroup(device, desc.layout, allocator);

	const BindGroupLayout &layout = desc.layout;

	VkDescriptorGenericInfo descriptorInfos[MAX_SHADER_BINDINGS] = {};
	VkWriteDescriptorSet descriptorWrites[MAX_SHADER_BINDINGS] = {};
	u32 descriptorWriteCount = 0;

	for (u32 i = 0; i < layout.bindingCount; ++i)
	{
		const ShaderBinding &binding = layout.bindings[i];
		if ( !AddDescriptorWrite(desc.bindings, binding, bindGroup.handle, descriptorInfos, descriptorWrites, descriptorWriteCount) )
		{
			LOG(Warning, "Could not add descriptor write for binding %s of pipeline %s.\n", binding.name, "<pipeline>");
		}
	}

	UpdateDescriptorSets(device, descriptorWrites, descriptorWriteCount);

	return bindGroup;
}


//////////////////////////////
// Buffer
//////////////////////////////

Buffer CreateBuffer(GraphicsDevice &device, u32 size, BufferUsageFlags bufferUsageFlags, HeapType heapType)
{
	// Buffer
	const VkBufferCreateInfo bufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
		.size = size,
		.usage = BufferUsageFlagsToVulkan(bufferUsageFlags),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
	};

	VkBuffer bufferHandle;
	VK_CALL( vkCreateBuffer(device.handle, &bufferCreateInfo, VULKAN_ALLOCATORS, &bufferHandle) );

	// Memory
	Heap &memoryHeap = device.heaps[heapType];
	VkDeviceMemory memory = memoryHeap.memory;
	VkMemoryRequirements memoryRequirements = {};
	vkGetBufferMemoryRequirements(device.handle, bufferHandle, &memoryRequirements);
	VkDeviceSize offset = AlignUp( memoryHeap.used, memoryRequirements.alignment );
	ASSERT( offset + memoryRequirements.size <= memoryHeap.size );
	memoryHeap.used = offset + memoryRequirements.size;

	VK_CALL( vkBindBufferMemory(device.handle, bufferHandle, memory, offset) );

	Alloc alloc = {
		heapType,
		offset,
		memoryRequirements.size,
	};

	const Buffer buffer = {
		.handle = bufferHandle,
		.alloc = alloc,
		.size = size,
	};
	return buffer;
}

void DestroyBuffer(const GraphicsDevice &device, const Buffer &buffer)
{
	vkDestroyBuffer( device.handle, buffer.handle, VULKAN_ALLOCATORS );

	// TODO: We should somehow deallocate memory when destroying buffers at runtime
}


//////////////////////////////
// BufferView
//////////////////////////////

BufferView CreateBufferView(const GraphicsDevice &device, const Buffer &buffer, Format format, u32 offset = 0, u32 size = 0)
{
	const VkBufferViewCreateInfo bufferViewCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_BUFFER_VIEW_CREATE_INFO,
		.buffer = buffer.handle,
		.format = FormatToVulkan(format),
		.offset = offset,
		.range = size > 0 ? size : buffer.size,
	};

	VkBufferView bufferViewHandle;
	VK_CALL( vkCreateBufferView(device.handle, &bufferViewCreateInfo, VULKAN_ALLOCATORS, &bufferViewHandle) );

	const BufferView bufferView = {
		.handle = bufferViewHandle
	};
	return bufferView;
}

void DestroyBufferView(const GraphicsDevice &device, const BufferView &bufferView)
{
	vkDestroyBufferView( device.handle, bufferView.handle, VULKAN_ALLOCATORS );
}


//////////////////////////////
// Image
//////////////////////////////

Image CreateImage(GraphicsDevice &device, u32 width, u32 height, u32 mipLevels, Format format, ImageUsageFlags usage, HeapType heapType)
{
	const VkFormat vkFormat = FormatToVulkan(format);
	// Image
	const VkImageCreateInfo imageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO,
		.flags = 0,
		.imageType = VK_IMAGE_TYPE_2D,
		.format = vkFormat,
		.extent = { .width = width, .height = height, .depth = 1, },
		.mipLevels = mipLevels,
		.arrayLayers = 1,
		.samples = VK_SAMPLE_COUNT_1_BIT,
		.tiling = VK_IMAGE_TILING_OPTIMAL,
		.usage = ImageUsageFlagsToVulkan(usage),
		.sharingMode = VK_SHARING_MODE_EXCLUSIVE,
		.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED,
	};

	VkImage imageHandle;
	VK_CALL( vkCreateImage(device.handle, &imageCreateInfo, VULKAN_ALLOCATORS, &imageHandle) );

	// Memory
	Heap &memoryHeap = device.heaps[heapType];
	VkMemoryRequirements memoryRequirements = {};
	vkGetImageMemoryRequirements(device.handle, imageHandle, &memoryRequirements);
	VkDeviceMemory memory = memoryHeap.memory;
	VkDeviceSize offset = AlignUp( memoryHeap.used, memoryRequirements.alignment );
	ASSERT( offset + memoryRequirements.size < memoryHeap.size );
	memoryHeap.used = offset + memoryRequirements.size;

	VK_CALL( vkBindImageMemory(device.handle, imageHandle, memory, offset) );

	const Image image = {
		.handle = imageHandle,
		.format = format,
		.width = width,
		.height = height,
		.mipLevels = mipLevels,
		.alloc = {
			.heap = memoryHeap.type,
			.offset = offset,
			.size = memoryRequirements.size,
		},
	};
	return image;
}

void DestroyImage(const GraphicsDevice &device, const Image &image)
{
	vkDestroyImage( device.handle, image.handle, VULKAN_ALLOCATORS );

	// TODO: We should somehow deallocate memory when destroying buffers at runtime
}


//////////////////////////////
// Sampler
//////////////////////////////

Sampler CreateSampler(const GraphicsDevice &device, const SamplerDesc &desc)
{
	VkPhysicalDeviceProperties properties;
	vkGetPhysicalDeviceProperties(device.physicalDevice, &properties);

	const VkSamplerCreateInfo samplerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO,
		.magFilter = VK_FILTER_LINEAR,
		.minFilter = VK_FILTER_LINEAR,
		.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR,
		.addressModeU = AddressModeToVulkan(desc.addressMode),
		.addressModeV = AddressModeToVulkan(desc.addressMode),
		.addressModeW = AddressModeToVulkan(desc.addressMode),
		.mipLodBias = 0.0f,
		.anisotropyEnable = VK_TRUE,
		.maxAnisotropy = properties.limits.maxSamplerAnisotropy,
		.compareEnable = desc.compareOp != CompareOpNone, // For PCF shadows for instance
		.compareOp = CompareOpToVulkan(desc.compareOp),
		.minLod = 0.0f,
		.maxLod = VK_LOD_CLAMP_NONE,
		.borderColor = BorderColorToVulkan(desc.borderColor),
		.unnormalizedCoordinates = VK_FALSE,
	};

	VkSampler vkSampler;
	VK_CALL( vkCreateSampler(device.handle, &samplerCreateInfo, VULKAN_ALLOCATORS, &vkSampler) );

	const Sampler sampler = {
		.handle = vkSampler
	};
	return sampler;
}

void DestroySampler(const GraphicsDevice &device, const Sampler &sampler)
{
	vkDestroySampler( device.handle, sampler.handle, VULKAN_ALLOCATORS );
}


//////////////////////////////
// Pipelines
//////////////////////////////

// TODO: Avoid having to pass renderPass as a parameter instead of within the PipelineDesc
Pipeline CreateGraphicsPipeline(const GraphicsDevice &device, Arena &arena, const PipelineDesc &desc, const RenderPass &renderPass)
{
	Arena scratch = MakeSubArena(arena);

	// Create pipeline
	const ShaderSource vertexShaderSource = GetShaderSource(scratch, desc.vsFilename);
	const ShaderSource fragmentShaderSource = GetShaderSource(scratch, desc.fsFilename);
	const ShaderModule vertexShaderModule = CreateShaderModule(device, vertexShaderSource);
	const ShaderModule fragmentShaderModule = CreateShaderModule(device, fragmentShaderSource);
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, vertexShaderSource, fragmentShaderSource);

	const VkPipelineShaderStageCreateInfo vertexShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_VERTEX_BIT,
		.module = vertexShaderModule.handle,
		.pName = desc.vsFunction ? desc.vsFunction : "main",
	};

	const VkPipelineShaderStageCreateInfo fragmentShaderStageCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_FRAGMENT_BIT,
		.module = fragmentShaderModule.handle,
		.pName = desc.fsFunction ? desc.fsFunction : "main",
	};

	VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageCreateInfo, fragmentShaderStageCreateInfo};

	VkVertexInputBindingDescription bindingDescriptions[1] = {};
	bindingDescriptions[0].binding = 0;
	bindingDescriptions[0].stride = sizeof(Vertex);
	bindingDescriptions[0].inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

	VkVertexInputAttributeDescription attributeDescriptions[4] = {};
	for (u32 i = 0; i < desc.vertexAttributeCount; ++i)
	{
		attributeDescriptions[i].binding = desc.vertexAttributes[i].bufferIndex;
		attributeDescriptions[i].location = desc.vertexAttributes[i].location;
		attributeDescriptions[i].format = FormatToVulkan(desc.vertexAttributes[i].format);
		attributeDescriptions[i].offset = desc.vertexAttributes[i].offset;
	}

	const VkPipelineVertexInputStateCreateInfo vertexInputCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO,
		.vertexBindingDescriptionCount = 1,
		.pVertexBindingDescriptions = bindingDescriptions, // Optional
		.vertexAttributeDescriptionCount = desc.vertexAttributeCount,
		.pVertexAttributeDescriptions = attributeDescriptions, // Optional
	};

	const VkPipelineInputAssemblyStateCreateInfo inputAssemblyCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO,
		.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST,
		.primitiveRestartEnable = VK_FALSE,
	};

	const VkDynamicState dynamicStates[] = {
		VK_DYNAMIC_STATE_VIEWPORT,
		VK_DYNAMIC_STATE_SCISSOR
	};
	const VkPipelineDynamicStateCreateInfo dynamicStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO,
		.dynamicStateCount = ARRAY_COUNT(dynamicStates),
		.pDynamicStates = dynamicStates,
	};

	const VkPipelineViewportStateCreateInfo viewportStateCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO,
		.viewportCount = 1,
		.scissorCount = 1,
	};
	// NOTE: We don't set values for the viewport and scissor
	// rect because they will be set dynamically using commands

	const VkPipelineRasterizationStateCreateInfo rasterizerCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO,
		.depthClampEnable = VK_FALSE,
		.rasterizerDiscardEnable = VK_FALSE,
		.polygonMode = VK_POLYGON_MODE_FILL,
		.cullMode = VK_CULL_MODE_BACK_BIT,
		.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE,
		.depthBiasEnable = VK_FALSE,
		.depthBiasConstantFactor = 0.0f, // Optional
		.depthBiasClamp = 0.0f, // Optional
		.depthBiasSlopeFactor = 0.0f, // Optional
		.lineWidth = 1.0f,
	};

	const VkPipelineMultisampleStateCreateInfo multisamplingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO,
		.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT,
		.sampleShadingEnable = VK_FALSE,
		.minSampleShading = 1.0f, // Optional
		.pSampleMask = nullptr, // Optional
		.alphaToCoverageEnable = VK_FALSE, // Optional
		.alphaToOneEnable = VK_FALSE, // Optional
	};

	const VkPipelineDepthStencilStateCreateInfo depthStencilCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO,
		.depthTestEnable = VK_TRUE,
		.depthWriteEnable = VK_TRUE,
		.depthCompareOp = CompareOpToVulkan(desc.depthCompareOp),
		.depthBoundsTestEnable = VK_FALSE,
		.stencilTestEnable = VK_FALSE,
		.front = {}, // Optional
		.back = {}, // Optional
		.minDepthBounds = 0.0f,
		.maxDepthBounds = 1.0f,
	};

	const VkPipelineColorBlendAttachmentState colorBlendAttachmentState = {
	// Color replace
		.blendEnable = VK_FALSE,
		.srcColorBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.colorBlendOp = VK_BLEND_OP_ADD, // Optional
		.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE, // Optional
		.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO, // Optional
		.alphaBlendOp = VK_BLEND_OP_ADD, // Optional
	// Alpha blending
	//	.blendEnable = VK_TRUE,
	//	.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA,
	//	.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
	//	.colorBlendOp = VK_BLEND_OP_ADD,
	//	.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE,
	//	.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO,
	//	.alphaBlendOp = VK_BLEND_OP_ADD,
		.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT,
	};

	const VkPipelineColorBlendStateCreateInfo colorBlendingCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO,
		.logicOpEnable = VK_FALSE,
		.logicOp = VK_LOGIC_OP_COPY, // Optional
		.attachmentCount = 1,
		.pAttachments = &colorBlendAttachmentState,
		.blendConstants = { 0.0f, 0.0f, 0.0f, 0.0f }, // Optional
	};

	// BindGroup layouts
	const BindGroupLayout bindGroupLayouts[MAX_DESCRIPTOR_SETS] = {
		CreateBindGroupLayout(device, shaderBindings, 0),
		CreateBindGroupLayout(device, shaderBindings, 1),
		CreateBindGroupLayout(device, shaderBindings, 2),
		CreateBindGroupLayout(device, shaderBindings, 3),
	};

	// Descriptor set layouts (same as BindGroup, but Vulkan handles)
	const VkDescriptorSetLayout descriptorSetLayouts[MAX_DESCRIPTOR_SETS] = {
		bindGroupLayouts[0].handle,
		bindGroupLayouts[1].handle,
		bindGroupLayouts[2].handle,
		bindGroupLayouts[3].handle,
	};

	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARRAY_COUNT(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
	};

	VkPipelineLayout pipelineLayout;
	VK_CALL( vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	const VkGraphicsPipelineCreateInfo graphicsPipelineCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO,
		.stageCount = ARRAY_COUNT(shaderStages),
		.pStages = shaderStages,
		.pVertexInputState = &vertexInputCreateInfo,
		.pInputAssemblyState = &inputAssemblyCreateInfo,
		.pViewportState = &viewportStateCreateInfo,
		.pRasterizationState = &rasterizerCreateInfo,
		.pMultisampleState = &multisamplingCreateInfo,
		.pDepthStencilState = &depthStencilCreateInfo,
		.pColorBlendState = &colorBlendingCreateInfo,
		.pDynamicState = &dynamicStateCreateInfo,
		.layout = pipelineLayout,
		.renderPass = renderPass.handle,
		.subpass = 0,
		.basePipelineHandle = VK_NULL_HANDLE, // Optional
		.basePipelineIndex = -1, // Optional
	};

	VkPipeline vkPipelineHandle;
	VK_CALL( vkCreateGraphicsPipelines( device.handle, device.pipelineCache, 1, &graphicsPipelineCreateInfo, VULKAN_ALLOCATORS, &vkPipelineHandle ) );

	DestroyShaderModule(device, vertexShaderModule);
	DestroyShaderModule(device, fragmentShaderModule);

	Pipeline pipeline = {
		.name = desc.name,
		.handle = vkPipelineHandle,
		.layout = {
			.handle = pipelineLayout,
			.shaderBindings = shaderBindings,
		},
		.bindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
	};

	for (u32 bindGroup = 0; bindGroup < MAX_DESCRIPTOR_SETS; ++bindGroup) {
		pipeline.layout.bindGroupLayouts[bindGroup] = bindGroupLayouts[bindGroup];
		pipeline.layout.bindGroupLayouts[bindGroup].bindings = GetBindGroupBindingPointer(pipeline.layout.shaderBindings, bindGroup);
	}

	return pipeline;
}

Pipeline CreateComputePipeline(const GraphicsDevice &device, Arena &arena, const ComputeDesc &desc)
{
	Arena scratch = arena;

	const ShaderSource shaderSource = GetShaderSource(scratch, desc.filename);
	const ShaderModule shaderModule = CreateShaderModule(device, shaderSource);
	const ShaderBindings shaderBindings = ReflectShaderBindings(scratch, shaderSource);

	const VkPipelineShaderStageCreateInfo computeShaderStageInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO,
		.stage = VK_SHADER_STAGE_COMPUTE_BIT,
		.module = shaderModule.handle,
		.pName = desc.function ? desc.function : "main",
	};

	// BindGroup layouts
	const BindGroupLayout bindGroupLayouts[MAX_DESCRIPTOR_SETS] = {
		CreateBindGroupLayout(device, shaderBindings, 0),
		CreateBindGroupLayout(device, shaderBindings, 1),
		CreateBindGroupLayout(device, shaderBindings, 2),
		CreateBindGroupLayout(device, shaderBindings, 3),
	};

	// Descriptor set layouts (same as BindGroup, but Vulkan handles)
	const VkDescriptorSetLayout descriptorSetLayouts[MAX_DESCRIPTOR_SETS] = {
		bindGroupLayouts[0].handle,
		bindGroupLayouts[1].handle,
		bindGroupLayouts[2].handle,
		bindGroupLayouts[3].handle,
	};

	// Pipeline layout
	const VkPipelineLayoutCreateInfo pipelineLayoutCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO,
		.setLayoutCount = ARRAY_COUNT(descriptorSetLayouts),
		.pSetLayouts = descriptorSetLayouts,
	};

	VkPipelineLayout pipelineLayout;
	VK_CALL( vkCreatePipelineLayout(device.handle, &pipelineLayoutCreateInfo, VULKAN_ALLOCATORS, &pipelineLayout) );

	const VkComputePipelineCreateInfo pipelineInfo = {
		.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO,
		.stage = computeShaderStageInfo,
		.layout = pipelineLayout,
	};

	VkPipeline computePipeline;
	VK_CALL( vkCreateComputePipelines(device.handle, device.pipelineCache, 1, &pipelineInfo, VULKAN_ALLOCATORS, &computePipeline) );

	DestroyShaderModule(device, shaderModule);

	Pipeline pipeline = {
		.name = desc.name,
		.handle = computePipeline,
		.layout = {
			.handle = pipelineLayout,
			.shaderBindings = shaderBindings,
		},
		.bindPoint = VK_PIPELINE_BIND_POINT_COMPUTE,
	};

	for (u32 bindGroup = 0; bindGroup < MAX_DESCRIPTOR_SETS; ++bindGroup) {
		pipeline.layout.bindGroupLayouts[bindGroup] = bindGroupLayouts[bindGroup];
		pipeline.layout.bindGroupLayouts[bindGroup].bindings = GetBindGroupBindingPointer(pipeline.layout.shaderBindings, bindGroup);
	}

	return pipeline;
}

void DestroyPipeline(const GraphicsDevice &device, const Pipeline &pipeline)
{
	vkDestroyPipeline( device.handle, pipeline.handle, VULKAN_ALLOCATORS );
	vkDestroyPipelineLayout( device.handle, pipeline.layout.handle, VULKAN_ALLOCATORS );
	for (u32 i = 0; i < ARRAY_COUNT(pipeline.layout.bindGroupLayouts); ++i) {
		DestroyBindGroupLayout(device, pipeline.layout.bindGroupLayouts[i]);
	}
}


//////////////////////////////
// RenderPass
//////////////////////////////

RenderPass CreateRenderPass( const GraphicsDevice &device, const RenderpassDesc &desc )
{
	const u8 MAX_COLOR_ATTACHMENTS = 3;
	const u8 MAX_DEPTH_ATTACHMENTS = 1;
	CT_ASSERT(MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS == MAX_RENDER_TARGETS);
	ASSERT(desc.colorAttachmentCount <= MAX_COLOR_ATTACHMENTS);

	VkAttachmentDescription attachmentDescs[MAX_COLOR_ATTACHMENTS + MAX_DEPTH_ATTACHMENTS] = {};
	VkAttachmentReference colorAttachmentRefs[MAX_COLOR_ATTACHMENTS] = {};
	VkAttachmentReference depthAttachmentRef = {};

	for (u8 i = 0; i < desc.colorAttachmentCount; ++i)
	{
		attachmentDescs[i].format = device.swapchainInfo.format;
		attachmentDescs[i].samples = VK_SAMPLE_COUNT_1_BIT;
		attachmentDescs[i].loadOp = LoadOpToVulkan( desc.colorAttachments[i].loadOp );
		attachmentDescs[i].storeOp = StoreOpToVulkan( desc.colorAttachments[i].storeOp );
		attachmentDescs[i].stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		attachmentDescs[i].stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		attachmentDescs[i].initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		attachmentDescs[i].finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

		colorAttachmentRefs[i].attachment = i;
		colorAttachmentRefs[i].layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
	}

	if ( desc.hasDepthAttachment )
	{
		const u8 depthAttachmentIndex = desc.colorAttachmentCount;
		VkAttachmentDescription &depthAttachmentDesc = attachmentDescs[depthAttachmentIndex];
		depthAttachmentDesc.format = FormatToVulkan(device.defaultDepthFormat);
		depthAttachmentDesc.samples = VK_SAMPLE_COUNT_1_BIT;
		depthAttachmentDesc.loadOp = LoadOpToVulkan(desc.depthAttachment.loadOp);
		depthAttachmentDesc.storeOp = StoreOpToVulkan(desc.depthAttachment.storeOp);
		depthAttachmentDesc.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
		depthAttachmentDesc.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
		depthAttachmentDesc.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
		depthAttachmentDesc.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

		depthAttachmentRef.attachment = depthAttachmentIndex;;
		depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;
	}

	const VkSubpassDescription subpassDesc = {
		.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS,
		.colorAttachmentCount = desc.colorAttachmentCount,
		.pColorAttachments = desc.colorAttachmentCount ? colorAttachmentRefs : NULL,
		.pDepthStencilAttachment = desc.hasDepthAttachment ? &depthAttachmentRef : NULL,
	};

	const VkSubpassDependency subpassDependency = {
		.srcSubpass = VK_SUBPASS_EXTERNAL,
		.dstSubpass = 0,
		.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT,
		.srcAccessMask = 0,
		.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT,
		.dependencyFlags = 0,
	};

	const VkRenderPassCreateInfo renderPassCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO,
		.attachmentCount = desc.colorAttachmentCount + (desc.hasDepthAttachment ? 1U : 0U),
		.pAttachments = attachmentDescs,
		.subpassCount = 1,
		.pSubpasses = &subpassDesc,
		.dependencyCount = 1,
		.pDependencies = &subpassDependency,
	};

	VkRenderPass vkRenderPass;
	VK_CALL( vkCreateRenderPass( device.handle, &renderPassCreateInfo, VULKAN_ALLOCATORS, &vkRenderPass ) );

	const RenderPass renderPass = {
		.name = InternString(desc.name),
		.handle = vkRenderPass
	};

	return renderPass;
}

void DestroyRenderPass(const GraphicsDevice &device, const RenderPass &renderPass)
{
	vkDestroyRenderPass( device.handle, renderPass.handle, VULKAN_ALLOCATORS );
}


//////////////////////////////
// Framebuffer
//////////////////////////////

Framebuffer CreateFramebuffer(const GraphicsDevice &device, const FramebufferDesc &desc)
{
	const VkFramebufferCreateInfo framebufferCreateInfo = {
		.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
		.renderPass = desc.renderPass.handle,
		.attachmentCount = desc.attachmentCount,
		.pAttachments = desc.attachments,
		.width = desc.width,
		.height = desc.height,
		.layers = 1,
	};

	VkFramebuffer handle;
	VK_CALL( vkCreateFramebuffer( device.handle, &framebufferCreateInfo, VULKAN_ALLOCATORS, &handle) );

	const Framebuffer framebuffer = {
		.handle = handle,
		.renderPassHandle = desc.renderPass.handle,
		.extent = { desc.width, desc.height },
		.attachmentCount = desc.attachmentCount,
	};
	return framebuffer;
}

void DestroyFramebuffer( const GraphicsDevice &device, const Framebuffer &framebuffer )
{
	vkDestroyFramebuffer( device.handle, framebuffer.handle, VULKAN_ALLOCATORS );
}


//////////////////////////////
// CommandList
//////////////////////////////

CommandList BeginCommandList(const GraphicsDevice &device)
{
	VkCommandBuffer commandBuffer = device.commandBuffers[device.currentFrame];

	VkCommandBufferBeginInfo commandBufferBeginInfo = {};
	commandBufferBeginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	commandBufferBeginInfo.flags = 0; // Optional
	commandBufferBeginInfo.pInheritanceInfo = NULL; // Optional
	VK_CALL( vkBeginCommandBuffer(commandBuffer, &commandBufferBeginInfo) );

	CommandList commandList = {
		.handle = commandBuffer,
	};
	return commandList;
}

void EndCommandList(const CommandList &commandList)
{
	VK_CALL( vkEndCommandBuffer( commandList.handle ) );
}

CommandList BeginTransientCommandList(const GraphicsDevice &device)
{
	VkCommandBufferAllocateInfo allocInfo = {};
	allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
	allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
	allocInfo.commandPool = device.transientCommandPool;
	allocInfo.commandBufferCount = 1;

	VkCommandBuffer commandBuffer;
	vkAllocateCommandBuffers(device.handle, &allocInfo, &commandBuffer);

	VkCommandBufferBeginInfo beginInfo = {};
	beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
	beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

	vkBeginCommandBuffer(commandBuffer, &beginInfo);

	const CommandList commandList {
		.handle = commandBuffer,
	};
	return commandList;
}

void EndTransientCommandList(GraphicsDevice &device, const CommandList &commandList)
{
	VkCommandBuffer commandBuffer = commandList.handle;

	vkEndCommandBuffer(commandBuffer);

	VkSubmitInfo submitInfo = {};
	submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
	submitInfo.commandBufferCount = 1;
	submitInfo.pCommandBuffers = &commandBuffer;
	vkQueueSubmit(device.graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);

	WaitQueueIdle(device);

	vkFreeCommandBuffers(device.handle, device.transientCommandPool, 1, &commandBuffer);
}



//////////////////////////////
// Commands
//////////////////////////////

void CopyBufferToBuffer(const CommandList &commandBuffer, const Buffer &srcBuffer, u32 srcOffset, const Buffer &dstBuffer, u32 dstOffset, u64 size)
{
	const VkBufferCopy copyRegion = {
		srcOffset = srcOffset,
		dstOffset = dstOffset,
		size = size,
	};
	vkCmdCopyBuffer(commandBuffer.handle, srcBuffer.handle, dstBuffer.handle, 1, &copyRegion);
}

void CopyBufferToImage(const CommandList &commandBuffer, const Buffer &buffer, u32 bufferOffset, const Image &image)
{
	const VkBufferImageCopy region = {
		.bufferOffset = bufferOffset,
		.bufferRowLength = 0,
		.bufferImageHeight = 0,
		.imageSubresource = {
			.aspectMask = FormatToVulkanAspect(image.format),
			.mipLevel = 0,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.imageOffset = {0, 0, 0},
		.imageExtent = { image.width, image.height, 1 },
	};

	vkCmdCopyBufferToImage(
			commandBuffer.handle,
			buffer.handle,
			image.handle,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assuming the right layout
			1,
			&region
			);
}

void Blit(const CommandList &commandBuffer, const Image &srcImage, const BlitRegion &srcRegion, const Image &dstImage, const BlitRegion &dstRegion)
{
	const VkImageAspectFlags aspectMask = FormatToVulkanAspect(srcImage.format);

	const VkImageBlit blit = {
		.srcSubresource = {
			.aspectMask = aspectMask,
			.mipLevel = srcRegion.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.srcOffsets = {
			{ srcRegion.x, srcRegion.y, 0u }, // lower bound
			{ srcRegion.x + srcRegion.width, srcRegion.y + srcRegion.height, 1u }, // upper bound
		},
		.dstSubresource = {
			.aspectMask = aspectMask,
			.mipLevel = dstRegion.mipLevel,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
		.dstOffsets = {
			{ dstRegion.x, dstRegion.y, 0 }, // lower bound
			{ dstRegion.x + dstRegion.width, dstRegion.y + dstRegion.height, 1 }, // upper bound
		},
	};

	vkCmdBlitImage(commandBuffer.handle,
			srcImage.handle, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL, // Assuming the proper layout
			dstImage.handle, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, // Assuming the proper layout
			1, &blit,
			VK_FILTER_LINEAR); // Assuming linear filtering
}

void TransitionImageLayout(const CommandList &commandBuffer, const Image &image, ImageState oldState, ImageState newState, u32 baseMipLevel, u32 levelCount)
{
	VkAccessFlags srcAccess = 0;
	VkAccessFlags dstAccess = 0;
	VkPipelineStageFlags srcStage = 0;
	VkPipelineStageFlags dstStage = 0;
	VkImageLayout oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
	VkImageLayout newLayout = VK_IMAGE_LAYOUT_UNDEFINED;

	switch (oldState)
	{
		case ImageStateInitial: oldLayout = VK_IMAGE_LAYOUT_UNDEFINED; break;
		case ImageStateTransferSrc: oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
		case ImageStateTransferDst: oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case ImageStateShaderInput: oldLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case ImageStateRenderTarget: oldLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
		default: INVALID_CODE_PATH();
	}

	switch (newState)
	{
		case ImageStateInitial: newLayout = VK_IMAGE_LAYOUT_UNDEFINED; break;
		case ImageStateTransferSrc: newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL; break;
		case ImageStateTransferDst: newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL; break;
		case ImageStateShaderInput: newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL; break;
		case ImageStateRenderTarget: newLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL; break;
		default: INVALID_CODE_PATH();
	}

	switch (oldLayout)
	{
		case VK_IMAGE_LAYOUT_UNDEFINED:
			srcAccess = 0;
			srcStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			srcAccess = VK_ACCESS_TRANSFER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			srcAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			srcAccess = VK_ACCESS_SHADER_READ_BIT;
			srcStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			srcAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			srcStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
			break;
		default:
			INVALID_CODE_PATH();
	}

	switch (newLayout)
	{
		case VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL:
			dstAccess = VK_ACCESS_TRANSFER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL:
			dstAccess = VK_ACCESS_TRANSFER_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
			break;
		case VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL:
			dstAccess = VK_ACCESS_SHADER_READ_BIT;
			dstStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
			break;
		case VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL:
			dstAccess = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
			dstStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
			break;
		default:
			INVALID_CODE_PATH();
	}

	const VkImageMemoryBarrier barrier = {
		.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER,
		.srcAccessMask = srcAccess,
		.dstAccessMask = dstAccess,
		.oldLayout = oldLayout,
		.newLayout = newLayout,
		.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED,
		.image = image.handle,
		.subresourceRange = {
			.aspectMask = FormatToVulkanAspect(image.format),
			.baseMipLevel = baseMipLevel,
			.levelCount = levelCount,
			.baseArrayLayer = 0,
			.layerCount = 1,
		},
	};

	vkCmdPipelineBarrier(commandBuffer.handle,
		srcStage,
		dstStage,
		0,          // 0 or VK_DEPENDENCY_BY_REGION_BIT
		0, NULL,    // Memory barriers
		0, NULL,    // Buffer barriers
		1, &barrier // Image barriers
		);
}

void BeginRenderPass(const CommandList &commandList, const Framebuffer &framebuffer)
{
	const VkRenderPassBeginInfo renderPassBeginInfo = {
		.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO,
		.renderPass = framebuffer.renderPassHandle,
		.framebuffer = framebuffer.handle,
		.renderArea = {
			.offset = { 0, 0 },
			.extent = framebuffer.extent
		},
		.clearValueCount = framebuffer.attachmentCount,
		.pClearValues = commandList.clearValues,
	};

	vkCmdBeginRenderPass( commandList.handle, &renderPassBeginInfo, VK_SUBPASS_CONTENTS_INLINE );
}

void SetViewportAndScissor(const CommandList &commandList, uint2 size)
{
	const VkViewport viewport = {
		.x = 0.0f,
		.y = static_cast<float>(size.y),
		.width = static_cast<float>(size.x),
		.height = -static_cast<float>(size.y),
		.minDepth = 0.0f,
		.maxDepth = 1.0f,
	};
	vkCmdSetViewport(commandList.handle, 0, 1, &viewport);

	const VkRect2D scissor = {
		.offset = {0, 0},
		.extent = {size.x, size.y},
	};
	vkCmdSetScissor(commandList.handle, 0, 1, &scissor);
}

void SetClearColor(CommandList &commandList, u32 renderTargetIndex, float4 color)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	commandList.clearValues[renderTargetIndex].color.float32[0] = color.r;
	commandList.clearValues[renderTargetIndex].color.float32[1] = color.g;
	commandList.clearValues[renderTargetIndex].color.float32[2] = color.b;
	commandList.clearValues[renderTargetIndex].color.float32[3] = color.a;
}

void SetClearDepth(CommandList &commandList, u32 renderTargetIndex, float depth)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	commandList.clearValues[renderTargetIndex].depthStencil.depth = depth;
}

void SetClearStencil(CommandList &commandList, u32 renderTargetIndex, u32 stencil)
{
	ASSERT(renderTargetIndex < MAX_RENDER_TARGETS);
	commandList.clearValues[renderTargetIndex].depthStencil.stencil = stencil;
}

void SetPipeline(CommandList &commandList, const Pipeline &pipeline)
{
	if ( commandList.pipeline != &pipeline )
	{
		commandList.pipeline = &pipeline;
		vkCmdBindPipeline( commandList.handle, pipeline.bindPoint, pipeline.handle );
	}
}

void SetBindGroup(CommandList &commandList, u32 bindGroupIndex, const BindGroup &bindGroup)
{
	if ( commandList.descriptorSetHandles[bindGroupIndex] != bindGroup.handle )
	{
		commandList.descriptorSetDirtyMask |= 1 << bindGroupIndex;
		commandList.descriptorSetHandles[bindGroupIndex] = bindGroup.handle;
	}
}

void SetVertexBuffer(CommandList &commandList, const Buffer &buffer)
{
	if ( buffer.handle && buffer.handle != commandList.vertexBufferHandle )
	{
		commandList.vertexBufferHandle = buffer.handle;
		VkBuffer vertexBuffers[] = { buffer.handle };
		VkDeviceSize vertexBufferOffsets[] = { 0 };
		vkCmdBindVertexBuffers(commandList.handle, 0, ARRAY_COUNT(vertexBuffers), vertexBuffers, vertexBufferOffsets);
	}
}

void SetIndexBuffer(CommandList &commandList, const Buffer &buffer)
{
	if ( buffer.handle && buffer.handle != commandList.indexBufferHandle )
	{
		commandList.indexBufferHandle = buffer.handle;
		VkDeviceSize indexBufferOffset = 0;
		vkCmdBindIndexBuffer(commandList.handle, buffer.handle, indexBufferOffset, VK_INDEX_TYPE_UINT16);
	}
}

void DrawIndexed(CommandList &commandList, u32 indexCount, u32 firstIndex, u32 firstVertex, u32 instanceIndex)
{
	BindDescriptorSets(commandList);

	vkCmdDrawIndexed(commandList.handle, indexCount, 1, firstIndex, firstVertex, instanceIndex);
}

void Dispatch(CommandList &commandList, u32 x, u32 y, u32 z)
{
	BindDescriptorSets(commandList);

	vkCmdDispatch(commandList.handle, x, y, z);
}

void EndRenderPass(const CommandList &commandList)
{
	vkCmdEndRenderPass(commandList.handle);
}


//////////////////////////////
// Submission and Presentation
//////////////////////////////

SubmitResult Submit(GraphicsDevice &device, const CommandList &commandList)
{
	const u32 frameIndex = device.currentFrame;

	VkSemaphore waitSemaphores[] = { device.imageAvailableSemaphores[frameIndex] };
	VkSemaphore signalSemaphores[] = { device.renderFinishedSemaphores[frameIndex] };
	VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };

	const VkSubmitInfo submitInfo = {
		.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
		.waitSemaphoreCount = ARRAY_COUNT(waitSemaphores),
		.pWaitSemaphores = waitSemaphores,
		.pWaitDstStageMask = waitStages,
		.commandBufferCount = 1,
		.pCommandBuffers = &commandList.handle,
		.signalSemaphoreCount = ARRAY_COUNT(signalSemaphores),
		.pSignalSemaphores = signalSemaphores,
	};

	ASSERT(device.usedFenceCount < MAX_FENCES);
	device.usedFenceCount++;

	GraphicsDevice::FrameData &frameData = device.frameData[frameIndex];
	const u32 fenceIndex = ( frameData.firstFenceIndex + frameData.usedFenceCount ) % MAX_FENCES;
	frameData.usedFenceCount++;

	VK_CALL( vkQueueSubmit( device.graphicsQueue, 1, &submitInfo, device.fences[fenceIndex] ) );

	SubmitResult res = {
		.signalSemaphore = signalSemaphores[0],
	};
	return res;
}

bool Present(GraphicsDevice &device, SubmitResult submitResult)
{
	const VkSemaphore signalSemaphores[] = { submitResult.signalSemaphore };

	const VkPresentInfoKHR presentInfo = {
		.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR,
		.waitSemaphoreCount = ARRAY_COUNT(signalSemaphores),
		.pWaitSemaphores = signalSemaphores,
		.swapchainCount = 1,
		.pSwapchains = &device.swapchain.handle,
		.pImageIndices = &device.swapchain.currentImageIndex,
		.pResults = NULL, // Optional
	};

	VkResult presentResult = vkQueuePresentKHR( device.presentQueue, &presentInfo );

	if (presentResult == VK_ERROR_OUT_OF_DATE_KHR || presentResult == VK_SUBOPTIMAL_KHR)
	{
		LOG(Warning, "vkQueuePresentKHR - result: VK_ERROR_OUT_OF_DATE_KHR || VK_SUBOPTIMAL_KHR\n");
		device.swapchain.outdated = true;
	}
	else if (presentResult != VK_SUCCESS)
	{
		LOG(Error, "vkQueuePresentKHR failed.\n");
		return false;
	}

	return true;
}



//////////////////////////////
// Frame
//////////////////////////////

bool BeginFrame(GraphicsDevice &device)
{
	const u32 frameIndex = device.currentFrame;

	// Catch-up frame fences if needed
	VkFence fences[MAX_FENCES] = {};
	GraphicsDevice::FrameData &frameData = device.frameData[frameIndex];

	if ( frameData.usedFenceCount > 0 )
	{
		for (u32 i = 0; i < frameData.usedFenceCount; ++i)
		{
			const u32 fenceIndex = ( frameData.firstFenceIndex + i ) % MAX_FENCES;
			fences[i] = device.fences[fenceIndex];
		}

		// Swapchain sync
		VK_CALL( vkWaitForFences( device.handle, frameData.usedFenceCount, fences, VK_TRUE, UINT64_MAX ) );
	}

	u32 imageIndex;
	VkResult acquireResult = vkAcquireNextImageKHR( device.handle, device.swapchain.handle, UINT64_MAX, device.imageAvailableSemaphores[frameIndex], VK_NULL_HANDLE, &imageIndex );

	if (acquireResult == VK_ERROR_OUT_OF_DATE_KHR)
	{
		LOG(Warning, "vkAcquireNextImageKHR - result: VK_ERROR_OUT_OF_DATE_KHR\n");
		device.swapchain.outdated = true;
		return false;
	}
	else if (acquireResult != VK_SUCCESS && acquireResult != VK_SUBOPTIMAL_KHR)
	{
		LOG(Error, "vkAcquireNextImageKHR failed.\n");
		return false;
	}

	if ( frameData.usedFenceCount > 0 )
	{
		// Reset this frame fences
		VK_CALL( vkResetFences( device.handle, frameData.usedFenceCount, fences ) );

		// Update global fence ring status
		device.firstFenceIndex = ( device.firstFenceIndex + frameData.usedFenceCount ) % MAX_FENCES;
		device.usedFenceCount -= frameData.usedFenceCount;
	}

	// Reset this frame fence ring status
	frameData.firstFenceIndex = ( device.firstFenceIndex + device.usedFenceCount ) % MAX_FENCES;
	frameData.usedFenceCount = 0;

	device.swapchain.currentImageIndex = imageIndex;

	// Reset commands for this frame
	VkCommandPool commandPool = device.commandPools[frameIndex];
	VK_CALL( vkResetCommandPool(device.handle, commandPool, 0) );

	return true;
}

void EndFrame(GraphicsDevice &device)
{
	device.currentFrame = ( device.currentFrame + 1 ) % MAX_FRAMES_IN_FLIGHT;
}

#endif // #ifndef TOOLS_GFX_H

