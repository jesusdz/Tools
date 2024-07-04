#define TOOLS_WINDOW
#include "tools.h"

// TODO: For now `assets.h` needs to be included before `tools_gfx.h
// as it defines types used by some functions there. We should make
// possible for assets.h to use types defined externally.
#include "assets/assets.h"
#include "clon.h"

// TODO: This StringInterning also needs to be declared before we
// include `tools_gfx.h` as it still makes use of it.
#define InternString(str) MakeStringIntern(gStringInterning, str)
static StringInterning *gStringInterning;

// TODO: Add VertexBinding/stride information to PipelineDesc so
// this struct doesn't need to be before `tools_gfx.h`
struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

#include "tools_gfx.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

// C/HLSL shared types and bindings
#include "shaders/types.hlsl"
#include "shaders/bindings.hlsl"




#define MAX_BUFFERS 64
#define MAX_TEXTURES 4
#define MAX_SAMPLERS 4
#define MAX_PIPELINES 8
#define MAX_COMPUTES 8
#define MAX_RENDERPASSES 4
#define MAX_MATERIALS 4
#define MAX_ENTITIES 32
#define MAX_GLOBAL_BINDINGS 8
#define MAX_MATERIAL_BINDINGS 4
#define MAX_COMPUTE_BINDINGS 4

#define INVALID_HANDLE -1



struct Material
{
	const char *name;
	PipelineH pipelineH;
	TextureH albedoTexture;
	f32 uvScale;
	u32 bufferOffset;
};

typedef u32 MaterialH;

struct RenderTargets
{
	Image depthImage;
	VkImageView depthImageView;
	VkFramebuffer framebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	Image shadowmapImage;
	VkImageView shadowmapImageView;
	VkFramebuffer shadowmapFramebuffer;
};

struct Camera
{
	float3 position;
	float2 orientation; // yaw and pitch
};

struct Entity
{
	const char *name;
	float3 position;
	float scale;
	bool visible;
	BufferChunk vertices;
	BufferChunk indices;
	u16 materialIndex;
};

struct Graphics
{
	GraphicsDevice device;

	RenderTargets renderTargets;

	RenderPass renderPasses[MAX_RENDERPASSES];
	u32 renderPassCount;

	Buffer buffers[MAX_BUFFERS];
	u32 bufferCount;

	BufferView bufferViews[MAX_BUFFERS];
	u32 bufferViewCount;

	BufferH stagingBuffer;
	u32 stagingBufferOffset;

	BufferArena globalVertexArena;
	BufferArena globalIndexArena;

	BufferChunk cubeVertices;
	BufferChunk cubeIndices;
	BufferChunk planeVertices;
	BufferChunk planeIndices;

	BufferH globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH entityBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH materialBuffer;
	BufferH computeBufferH;
	BufferViewH computeBufferViewH;

	SamplerH samplerH;
	SamplerH shadowmapSamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;

	Sampler samplers[MAX_SAMPLERS];
	u32 samplerCount;

	Texture textures[MAX_TEXTURES];
	u32 textureCount;

	Pipeline pipelines[MAX_PIPELINES];
	u32 pipelineCount;

	Material materials[MAX_MATERIALS];
	u32 materialCount;

	BindGroupAllocator globalBindGroupAllocator;
	BindGroupAllocator materialBindGroupAllocator;
	BindGroupAllocator computeBindGroupAllocator[MAX_FRAMES_IN_FLIGHT];
#if USE_IMGUI
	BindGroupAllocator imGuiBindGroupAllocator;
#endif

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	BindGroup globalBindGroups[MAX_MATERIALS][MAX_FRAMES_IN_FLIGHT];
	// Updated once at the beginning for each material
	BindGroup materialBindGroups[MAX_MATERIALS];
	// For computes
	// Compute bind groups are created on-the-fly every frame

	Camera camera;

	Entity entities[MAX_ENTITIES];
	u32 entityCount;

	PipelineH shadowmapPipelineH;

	PipelineH computeClearH;
	PipelineH computeUpdateH;

	bool deviceInitialized;
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


BufferH CreateBuffer(Graphics &gfx, u32 size, BufferUsageFlags bufferUsageFlags, HeapType heapType)
{
	ASSERT( gfx.bufferCount < ARRAY_COUNT(gfx.buffers) );
	BufferH bufferHandle = gfx.bufferCount++;
	gfx.buffers[bufferHandle] = CreateBuffer(gfx.device, size, bufferUsageFlags, heapType);
	return bufferHandle;
}

Buffer &GetBuffer(Graphics &gfx, BufferH bufferHandle)
{
	Buffer &buffer = gfx.buffers[bufferHandle];
	return buffer;
}

const Buffer &GetBuffer(const Graphics &gfx, BufferH bufferHandle)
{
	const Buffer &buffer = gfx.buffers[bufferHandle];
	return buffer;
}

BufferViewH CreateBufferView(Graphics &gfx, BufferH bufferHandle, Format format, u32 offset = 0, u32 size = 0)
{
	const Buffer &buffer = GetBuffer(gfx, bufferHandle);

	ASSERT( gfx.bufferViewCount < ARRAY_COUNT(gfx.bufferViews) );
	BufferViewH bufferViewHandle = gfx.bufferViewCount++;
	gfx.bufferViews[bufferViewHandle] = CreateBufferView(gfx.device, buffer, format, offset, size);
	return bufferViewHandle;
}

const BufferView &GetBufferView(const Graphics &gfx, BufferViewH bufferViewHandle)
{
	const BufferView &bufferView = gfx.bufferViews[bufferViewHandle];
	return bufferView;
}

void CopyBufferToBuffer(const CommandList &commandList, VkBuffer srcBuffer, u32 srcOffset, VkBuffer dstBuffer, u32 dstOffset, VkDeviceSize size)
{
	VkCommandBuffer commandBuffer = commandList.handle;

	VkBufferCopy copyRegion = {};
	copyRegion.srcOffset = srcOffset;
	copyRegion.dstOffset = dstOffset;
	copyRegion.size = size;
	vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

struct StagedData
{
	VkBuffer buffer;
	u32 offset;
};

StagedData StageData(Graphics &gfx, const void *data, u32 size)
{
	Buffer &stagingBuffer = GetBuffer(gfx, gfx.stagingBuffer);

	StagedData staging = {};
	staging.buffer = stagingBuffer.handle;
	staging.offset = stagingBuffer.alloc.offset + gfx.stagingBufferOffset;

	Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	void* stagingData = stagingHeap.data + staging.offset;
	MemCopy(stagingData, data, (size_t) size);

	gfx.stagingBufferOffset = AlignUp(gfx.stagingBufferOffset + size, gfx.device.alignment.optimalBufferCopyOffset);

	return staging;
}

BufferH CreateStagingBuffer(Graphics &gfx)
{
	const Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	BufferH stagingBufferHandle = CreateBuffer(gfx, stagingHeap.size, BufferUsageTransferSrc, HeapType_Staging);
	return stagingBufferHandle;
}

BufferH CreateVertexBuffer(Graphics &gfx, u32 size)
{
	BufferH vertexBufferHandle = CreateBuffer(
			gfx,
			size,
			BufferUsageVertexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return vertexBufferHandle;
}

BufferH CreateIndexBuffer(Graphics &gfx, u32 size)
{
	BufferH indexBufferHandle = CreateBuffer(
			gfx,
			size,
			BufferUsageIndexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return indexBufferHandle;
}

BufferArena MakeBufferArena(Graphics &gfx, BufferH bufferHandle)
{
	Buffer &buffer = GetBuffer(gfx, bufferHandle);

	BufferArena arena = {};
	arena.buffer = bufferHandle;
	arena.size = buffer.size;
	return arena;
}

BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size)
{
	StagedData staged = StageData(gfx, data, size);

	Buffer &finalBuffer = GetBuffer(gfx, arena.buffer);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(commandList, staged.buffer, staged.offset, finalBuffer.handle, arena.used, size);

	BufferChunk chunk = {};
	chunk.buffer = arena.buffer;
	chunk.offset = arena.used;
	chunk.size = size;

	arena.used += size;

	return chunk;
}

RenderPassH CreateRenderPass( Graphics &gfx, const RenderpassDesc &desc )
{
	ASSERT( gfx.renderPassCount < ARRAY_COUNT( gfx.renderPasses ) );
	RenderPassH renderPassH = gfx.renderPassCount++;
	gfx.renderPasses[renderPassH] = CreateRenderPass(gfx.device, desc);
	return renderPassH;
}

const RenderPass &GetRenderPass(const Graphics &gfx, RenderPassH handle)
{
	const RenderPass &renderPass = gfx.renderPasses[handle];
	return renderPass;
}

RenderPassH RenderPassHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.renderPassCount; ++i) {
		if ( StrEq(gfx.renderPasses[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find render <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return INVALID_HANDLE;
}

PipelineH CreateGraphicsPipeline(Graphics &gfx, Arena &arena, const PipelineDesc &desc)
{
	RenderPassH renderPassH = RenderPassHandle(gfx, desc.renderPass);
	const RenderPass &renderPass = GetRenderPass(gfx, renderPassH);

	ASSERT( gfx.pipelineCount < ARRAY_COUNT(gfx.pipelines) );
	PipelineH pipelineHandle = gfx.pipelineCount++;
	Pipeline &pipeline = gfx.pipelines[pipelineHandle];
	pipeline = CreateGraphicsPipeline(gfx.device, arena, desc, renderPass);
	for (u32 bindGroup = 0; bindGroup < MAX_DESCRIPTOR_SETS; ++bindGroup) {
		pipeline.layout.bindGroupLayouts[bindGroup].bindings = GetBindGroupBindingPointer(pipeline.layout.shaderBindings, bindGroup);
	}
	return pipelineHandle;
}

PipelineH CreateComputePipeline(Graphics &gfx, Arena &arena, const ComputeDesc &desc)
{
	ASSERT( gfx.pipelineCount < ARRAY_COUNT(gfx.pipelines) );
	PipelineH pipelineHandle = gfx.pipelineCount++;
	Pipeline &pipeline = gfx.pipelines[pipelineHandle];
	pipeline = CreateComputePipeline(gfx.device, arena, desc);
	for (u32 bindGroup = 0; bindGroup < MAX_DESCRIPTOR_SETS; ++bindGroup) {
		pipeline.layout.bindGroupLayouts[bindGroup].bindings = GetBindGroupBindingPointer(pipeline.layout.shaderBindings, bindGroup);
	}
	return pipelineHandle;
}

const Pipeline &GetPipeline(const Graphics &gfx, PipelineH handle)
{
	const Pipeline &pipeline = gfx.pipelines[handle];
	return pipeline;
}

PipelineH PipelineHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.pipelineCount; ++i) {
		if ( StrEq(gfx.pipelines[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find pipeline <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return INVALID_HANDLE;
}

SamplerH CreateSampler(Graphics &gfx, const SamplerDesc &desc)
{
	ASSERT( gfx.samplerCount < ARRAY_COUNT( gfx.samplers ) );
	SamplerH samplerHandle = gfx.samplerCount++;
	gfx.samplers[samplerHandle] = CreateSampler(gfx.device, desc);
	return samplerHandle;
}

const Sampler &GetSampler(const Graphics &gfx, SamplerH handle)
{
	const Sampler &sampler = gfx.samplers[handle];
	return sampler;
}

void TransitionImageLayout(const CommandList &commandList, VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, u32 mipLevels)
{
	VkCommandBuffer commandBuffer = commandList.handle;

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

	if (IsDepthFormat(format)) {
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
	} else if (oldLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_SHADER_READ_BIT;
		barrier.dstAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

		sourceStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
		destinationStage = VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
	} else if (oldLayout == VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
		barrier.srcAccessMask = VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;
		barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

		sourceStage = VK_PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT;
		destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
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
}

void CopyBufferToImage(Graphics &gfx, const CommandList &commandList, VkBuffer buffer, u32 bufferOffset, VkImage image, u32 width, u32 height)
{
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

	VkCommandBuffer commandBuffer = commandList.handle;

	vkCmdCopyBufferToImage(
			commandBuffer,
			buffer,
			image,
			VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
			1,
			&region
			);
}

void GenerateMipmaps(Graphics &gfx, const CommandList &commandList, VkImage image, VkFormat format, i32 width, i32 height, u32 mipLevels)
{
	VkFormatProperties formatProperties;
	vkGetPhysicalDeviceFormatProperties(gfx.device.physicalDevice, format, &formatProperties);

	if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
		LOG(Error, "GenerateMipmaps() - Linera filtering not supported for the given format.\n");
		QUIT_ABNORMALLY();
	}

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

	VkCommandBuffer commandBuffer = commandList.handle;

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
}

TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	int texWidth, texHeight, texChannels;
	FilePath imagePath = MakePath(desc.filename);
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

	const Format texFormat = FormatRGB8_SRGB;

	Image image = CreateImage(gfx.device,
			texWidth, texHeight, mipLevels,
			texFormat,
			ImageUsageTransferSrc | // for mipmap blits
			ImageUsageTransferDst | // for intitial copy from buffer and blits
			ImageUsageSampled, // to be sampled in shaders
			HeapType_General);

	CommandList commandList = BeginTransientCommandList(gfx.device);

	TransitionImageLayout(commandList, image.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, mipLevels);

	CopyBufferToImage(gfx, commandList, staged.buffer, staged.offset, image.handle, texWidth, texHeight);

	// GenerateMipmaps takes care of this transition after generating the mip levels
	//TransitionImageLayout(commandList, image.handle, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, mipLevels);

	GenerateMipmaps(gfx, commandList, image.handle, VK_FORMAT_R8G8B8A8_SRGB, texWidth, texHeight, mipLevels);

	EndTransientCommandList(gfx.device, commandList);

	ASSERT( gfx.textureCount < ARRAY_COUNT(gfx.textures) );
	TextureH textureHandle = gfx.textureCount++;
	gfx.textures[textureHandle].name = desc.name;
	gfx.textures[textureHandle].image = image;
	gfx.textures[textureHandle].imageView = CreateImageView(gfx.device, image.handle, image.format, VK_IMAGE_ASPECT_COLOR_BIT, mipLevels);
	gfx.textures[textureHandle].mipLevels = mipLevels;

	return textureHandle;
}

const Texture &GetTexture(const Graphics &gfx, TextureH handle)
{
	const Texture &texture = gfx.textures[handle];
	return texture;
}

TextureH TextureHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.textureCount; ++i) {
		if ( StrEq(gfx.textures[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find texture <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return INVALID_HANDLE;
}

MaterialH CreateMaterial( Graphics &gfx, const MaterialDesc &desc)
{
	TextureH textureHandle = TextureHandle(gfx, desc.textureName);
	PipelineH pipelineHandle = PipelineHandle(gfx, desc.pipelineName);

	ASSERT(gfx.materialCount < MAX_MATERIALS);
	MaterialH materialHandle = gfx.materialCount++;
	gfx.materials[materialHandle].name = desc.name;
	gfx.materials[materialHandle].pipelineH = pipelineHandle;
	gfx.materials[materialHandle].albedoTexture = textureHandle;
	gfx.materials[materialHandle].uvScale = desc.uvScale;
	gfx.materials[materialHandle].bufferOffset = materialHandle * AlignUp(sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset);

	return materialHandle;
}

Material &GetMaterial( Graphics &gfx, MaterialH materialHandle )
{
	Material &material = gfx.materials[materialHandle];
	return material;
}

MaterialH MaterialHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.materialCount; ++i) {
		if ( StrEq(gfx.materials[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find material <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return INVALID_HANDLE;
}

BufferChunk GetVerticesForGeometryType(Graphics &gfx, GeometryType geometryType)
{
	if ( geometryType == GeometryTypeCube) {
		return gfx.cubeVertices;
	} else {
		return gfx.planeVertices;
	}
}

BufferChunk GetIndicesForGeometryType(Graphics &gfx, GeometryType geometryType)
{
	if ( geometryType == GeometryTypeCube) {
		return gfx.cubeIndices;
	} else {
		return gfx.planeIndices;
	}
}

void CreateEntity(Graphics &gfx, const EntityDesc &desc)
{
	ASSERT ( gfx.entityCount < MAX_ENTITIES );
	if ( gfx.entityCount < MAX_ENTITIES )
	{
		BufferChunk vertices = GetVerticesForGeometryType(gfx, desc.geometryType);
		BufferChunk indices = GetIndicesForGeometryType(gfx, desc.geometryType);

		const u32 entityIndex = gfx.entityCount++;
		gfx.entities[entityIndex].name = desc.name;
		gfx.entities[entityIndex].visible = true;
		gfx.entities[entityIndex].position = desc.pos;
		gfx.entities[entityIndex].scale = desc.scale;
		gfx.entities[entityIndex].vertices = vertices;
		gfx.entities[entityIndex].indices = indices;
		gfx.entities[entityIndex].materialIndex = MaterialHandle(gfx, desc.materialName);
	}
	else
	{
		LOG(Warning, "CreateEntity() - MAX_ENTITIES limit reached.\n");
	}
}

RenderTargets CreateRenderTargets(Graphics &gfx)
{
	RenderTargets renderTargets = {};

	CommandList commandList = BeginTransientCommandList(gfx.device);

	const VkFormat vkDepthFormat = FindDepthVkFormat(gfx.device);
	const Format depthFormat = FormatFromVulkan(vkDepthFormat);

	// Depth buffer
	renderTargets.depthImage = CreateImage(gfx.device,
			gfx.device.swapchain.extent.width, gfx.device.swapchain.extent.height, 1,
			depthFormat,
			ImageUsageDepthStencilAttachment,
			HeapType_RTs);
	VkImageView depthImageView = CreateImageView(gfx.device, renderTargets.depthImage.handle, vkDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
	TransitionImageLayout(commandList, renderTargets.depthImage.handle, vkDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
	renderTargets.depthImageView = depthImageView;

	const RenderPass &renderPass = GetRenderPass(gfx, gfx.litRenderPassH);

	// Framebuffer
	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		VkImageView attachments[] = { gfx.device.swapchain.imageViews[i], renderTargets.depthImageView };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass.handle;
		framebufferCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = gfx.device.swapchain.extent.width;
		framebufferCreateInfo.height = gfx.device.swapchain.extent.height;
		framebufferCreateInfo.layers = 1;

		VK_CALL( vkCreateFramebuffer( gfx.device.handle, &framebufferCreateInfo, VULKAN_ALLOCATORS, &renderTargets.framebuffers[i]) );
	}

	// Shadowmap
	{
		renderTargets.shadowmapImage = CreateImage(gfx.device,
				1024, 1024, 1,
				depthFormat,
				ImageUsageDepthStencilAttachment | ImageUsageSampled,
				HeapType_RTs);
		VkImageView shadowmapImageView = CreateImageView(gfx.device, renderTargets.shadowmapImage.handle, vkDepthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);
		TransitionImageLayout(commandList, renderTargets.shadowmapImage.handle, vkDepthFormat, VK_IMAGE_LAYOUT_UNDEFINED, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);
		renderTargets.shadowmapImageView = shadowmapImageView;

		const RenderPass &renderPass = GetRenderPass(gfx, gfx.shadowmapRenderPassH);

		VkImageView attachments[] = { renderTargets.shadowmapImageView };

		VkFramebufferCreateInfo framebufferCreateInfo = {};
		framebufferCreateInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
		framebufferCreateInfo.renderPass = renderPass.handle;
		framebufferCreateInfo.attachmentCount = ARRAY_COUNT(attachments);
		framebufferCreateInfo.pAttachments = attachments;
		framebufferCreateInfo.width = 1024;
		framebufferCreateInfo.height = 1024;
		framebufferCreateInfo.layers = 1;

		VK_CALL( vkCreateFramebuffer( gfx.device.handle, &framebufferCreateInfo, VULKAN_ALLOCATORS, &renderTargets.shadowmapFramebuffer ) );
	}

	EndTransientCommandList(gfx.device, commandList);

	return renderTargets;
}

void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	vkDestroyImageView(gfx.device.handle, renderTargets.depthImageView, VULKAN_ALLOCATORS);
	vkDestroyImage(gfx.device.handle, renderTargets.depthImage.handle, VULKAN_ALLOCATORS);

	// Reset the heap used for render targets
	Heap &rtHeap = gfx.device.heaps[HeapType_RTs];
	rtHeap.used = 0;

	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		vkDestroyFramebuffer( gfx.device.handle, renderTargets.framebuffers[i], VULKAN_ALLOCATORS );
	}

	vkDestroyImageView(gfx.device.handle, renderTargets.shadowmapImageView, VULKAN_ALLOCATORS);
	vkDestroyImage(gfx.device.handle, renderTargets.shadowmapImage.handle, VULKAN_ALLOCATORS);
	vkDestroyFramebuffer( gfx.device.handle, renderTargets.shadowmapFramebuffer, VULKAN_ALLOCATORS );

	renderTargets = {};
}


bool InitializeGraphics(Graphics &gfx, Arena &arena, Window &window)
{
	Arena scratch = MakeSubArena(arena);

	if ( !InitializeGraphicsDevice( gfx.device, scratch, window ) ) {
		return false;
	}

	// Global render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "main_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .loadOp = LoadOpClear, .storeOp = StoreOpStore },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = StoreOpDontCare
			}
		};
		gfx.litRenderPassH = CreateRenderPass( gfx, renderpassDesc );
	}

	// Shadowmap render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "shadowmap_renderpass",
			.colorAttachmentCount = 0,
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = StoreOpStore
			}
		};
		gfx.shadowmapRenderPassH = CreateRenderPass( gfx, renderpassDesc );
	}

	// Render targets
	gfx.renderTargets = CreateRenderTargets(gfx);

	// Create staging buffer
	gfx.stagingBuffer = CreateStagingBuffer(gfx);

	// Create global geometry buffers
	gfx.globalVertexArena = MakeBufferArena( gfx, CreateVertexBuffer(gfx, MB(4)) );
	gfx.globalIndexArena = MakeBufferArena( gfx, CreateIndexBuffer(gfx, MB(4)) );

	CommandList commandList = BeginTransientCommandList(gfx.device);

	// Create vertex/index buffers
	gfx.cubeVertices = PushData(gfx, commandList, gfx.globalVertexArena, cubeVertices, sizeof(cubeVertices));
	gfx.cubeIndices = PushData(gfx, commandList, gfx.globalIndexArena, cubeIndices, sizeof(cubeIndices));
	gfx.planeVertices = PushData(gfx, commandList, gfx.globalVertexArena, planeVertices, sizeof(planeVertices));
	gfx.planeIndices = PushData(gfx, commandList, gfx.globalIndexArena, planeIndices, sizeof(planeIndices));

	EndTransientCommandList(gfx.device, commandList);

	// Create globals buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 globalsBufferSize = sizeof(Globals);
		gfx.globalsBuffer[i] = CreateBuffer(
			gfx,
			globalsBufferSize,
			BufferUsageUniformBuffer,
			HeapType_Dynamic);
	}

	// Create entities buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 entityBufferSize = MAX_ENTITIES * AlignUp( sizeof(SEntity), gfx.device.alignment.uniformBufferOffset );
		gfx.entityBuffer[i] = CreateBuffer(
			gfx,
			entityBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create material buffer
	const u32 materialBufferSize = MAX_MATERIALS * AlignUp( sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset );
	gfx.materialBuffer = CreateBuffer(gfx, materialBufferSize, BufferUsageUniformBuffer | BufferUsageTransferDst, HeapType_General);

	// Create buffer for computes
	const u32 computeBufferSize = sizeof(float);
	gfx.computeBufferH = CreateBuffer(gfx, computeBufferSize, BufferUsageStorageTexelBuffer, HeapType_General);
	gfx.computeBufferViewH = CreateBufferView(gfx, gfx.computeBufferH, FormatFloat);


	// Create Global Descriptor Pool
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS,
			.storageBufferCount = MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS,
			.textureCount = 1000,
			.samplerCount = MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS * 2,
			.groupCount = MAX_FRAMES_IN_FLIGHT * MAX_MATERIALS,
		};
		gfx.globalBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}
	// Create Material Descriptor Pool
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = MAX_MATERIALS,
			.textureCount = MAX_MATERIALS,
			.groupCount = MAX_MATERIALS,
		};
		gfx.materialBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}
	// Create Compute Descriptor Pool
	for (u32 i = 0; i < ARRAY_COUNT(gfx.computeBindGroupAllocator); ++i)
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = 1000,
			.storageBufferCount = 1000,
			.storageTexelBufferCount = 1000,
			.textureCount = 1000,
			.samplerCount = 1000,
			.groupCount = MAX_COMPUTES,
		};
		gfx.computeBindGroupAllocator[i] = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}


#if USE_IMGUI
	// Create Imgui Descriptor Pool
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.combinedImageSamplerCount = 1,
			.groupCount = 1,
			.allowIndividualFrees = true,
		};
		gfx.imGuiBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}
#endif

	return true;
}

BufferBinding Binding(const Buffer &buffer, u32 offset = 0, u32 range = 0)
{
	const BufferBinding binding = {
		.handle = buffer.handle,
		.offset = offset,
		.range = range > 0 ? range : buffer.size,
	};
	return binding;
}

BufferViewBinding Binding(const BufferView &bufferView)
{
	const BufferViewBinding binding = { .handle = bufferView.handle };
	return binding;
}

TextureBinding Binding(const Texture &texture)
{
	const TextureBinding binding = { .handle = texture.imageView };
	return binding;
}

SamplerBinding Binding(const Sampler &sampler)
{
	const SamplerBinding binding = { .handle = sampler.handle };
	return binding;
}

void BindResource(ResourceBinding *bindingTable, u32 binding, const Buffer &buffer, u32 offset = 0, u32 range = 0)
{
	bindingTable[binding].buffer = Binding(buffer, offset, range);
}

void BindResource(ResourceBinding *bindingTable, u32 binding, const BufferView &bufferView)
{
	bindingTable[binding].bufferView = Binding(bufferView);
}

void BindResource(ResourceBinding *bindingTable, u32 binding, const Texture &texture)
{
	bindingTable[binding].texture = Binding(texture);
}

void BindResource(ResourceBinding *bindingTable, u32 binding, const Sampler &sampler)
{
	bindingTable[binding].sampler = Binding(sampler);;
}

void BindGlobalResources(const Graphics &gfx, ResourceBinding bindingTable[])
{
	const u32 frameIndex = gfx.device.currentFrame;
	const Buffer &globalsBuffer = GetBuffer(gfx, gfx.globalsBuffer[frameIndex]);
	const Buffer &entityBuffer = GetBuffer(gfx, gfx.entityBuffer[frameIndex]);
	const Sampler &sampler = GetSampler(gfx, gfx.samplerH);

	// TODO: Make this better, the shadowmap should also figure as a texture somewhere
	const Texture shadowmap = { .imageView = gfx.renderTargets.shadowmapImageView };
	const Sampler &shadowmapSampler = GetSampler(gfx, gfx.shadowmapSamplerH);

	BindResource(bindingTable, BINDING_GLOBALS, globalsBuffer);
	BindResource(bindingTable, BINDING_ENTITIES, entityBuffer);
	BindResource(bindingTable, BINDING_SAMPLER, sampler);
	BindResource(bindingTable, BINDING_SHADOWMAP, shadowmap);
	BindResource(bindingTable, BINDING_SHADOWMAP_SAMPLER, shadowmapSampler);
}

void BindMaterialResources(const Graphics &gfx, const Material &material, ResourceBinding bindingTable[])
{
	const Texture &albedoTexture = GetTexture(gfx, material.albedoTexture);
	const Buffer &materialBuffer = GetBuffer(gfx, gfx.materialBuffer);

	BindResource(bindingTable, BINDING_MATERIAL, materialBuffer, material.bufferOffset, sizeof(SMaterial));
	BindResource(bindingTable, BINDING_ALBEDO, albedoTexture);
}

void UpdateMaterialBindGroup(Graphics &gfx, u8 bindGroupIndex)
{
	VkDescriptorGenericInfo descriptorInfos[MAX_MATERIALS * MAX_SHADER_BINDINGS] = {};
	VkWriteDescriptorSet descriptorWrites[MAX_MATERIALS * MAX_SHADER_BINDINGS] = {};
	u32 descriptorWriteCount = 0;

	ResourceBinding globalBindingTable[MAX_GLOBAL_BINDINGS];
	ResourceBinding materialBindingTable[MAX_MATERIAL_BINDINGS];
	ResourceBinding *bindingTables[MAX_DESCRIPTOR_SETS];
	bindingTables[BIND_GROUP_GLOBAL] = globalBindingTable;
	bindingTables[BIND_GROUP_MATERIAL] = materialBindingTable;

	BindGlobalResources(gfx, bindingTables[BIND_GROUP_GLOBAL]);

	for (u32 materialIndex = 0; materialIndex < gfx.materialCount; ++materialIndex)
	{
		const Material &material = GetMaterial(gfx, materialIndex);
		const Pipeline &pipeline = GetPipeline(gfx, material.pipelineH);
		const ShaderBindings &shaderBindings = pipeline.layout.shaderBindings;

		BindMaterialResources(gfx, material, bindingTables[BIND_GROUP_MATERIAL]);

		for ( u32 bindingIndex = 0; bindingIndex < shaderBindings.bindingCount; ++bindingIndex )
		{
			const ShaderBinding &binding = shaderBindings.bindings[bindingIndex];

			if ( binding.set != bindGroupIndex ) continue;

			ResourceBinding *bindingTable = 0;
			VkDescriptorSet descriptorSet = VK_NULL_HANDLE;

			if ( binding.set == BIND_GROUP_GLOBAL )
			{
				bindingTable = bindingTables[BIND_GROUP_GLOBAL];
				descriptorSet = gfx.globalBindGroups[materialIndex][gfx.device.currentFrame].handle;
			}
			else if ( binding.set == BIND_GROUP_MATERIAL )
			{
				bindingTable = bindingTables[BIND_GROUP_MATERIAL];
				descriptorSet = gfx.materialBindGroups[materialIndex].handle;
			}
			else
			{
				LOG(Warning, "Unhandled descriptor set (%u) in UpdateMaterialBindGroup.\n", binding.set);
				continue;
			}

			if ( !AddDescriptorWrite(bindingTable, binding, descriptorSet, descriptorInfos, descriptorWrites, descriptorWriteCount) ) {
				LOG(Warning, "Could not add descriptor write for binding %s of material %s.\n", binding.name, material.name);
			}
		}
	}

	UpdateDescriptorSets(gfx.device, descriptorWrites, descriptorWriteCount);
}

void InitializeScene(Arena scratch, Graphics &gfx)
{
	FilePath assetsPath = MakePath("assets/assets.h");
	DataChunk *chunk = PushFile( scratch, assetsPath.str );
	if (!chunk)
	{
		LOG(Error, "Error reading file: %s\n", assetsPath.str);
		return;
	}

	Clon clon = {};
	const bool clonOk = ClonParse(&clon, &scratch, chunk->chars, chunk->size);
	if (!clonOk)
	{
		LOG(Error, "Error in ClonParse file: %s\n", assetsPath.str);
		return;
	}

	const ClonGlobal *clonGlobal = ClonGetGlobal(&clon, "Assets", "gAssets");

	if (clonGlobal)
	{
		const Assets *gAssets = (const Assets *)clonGlobal->data;

		for (u32 i = 0; i < gAssets->pipelinesCount; ++i)
		{
			CreateGraphicsPipeline(gfx, scratch, gAssets->pipelines[i]);
		}

		for (u32 i = 0; i < gAssets->computesCount; ++i)
		{
			CreateComputePipeline(gfx, scratch, gAssets->computes[i]);
		}

		for (u32 i = 0; i < gAssets->texturesCount; ++i)
		{
			CreateTexture(gfx, gAssets->textures[i]);
		}

		for (u32 i = 0; i < gAssets->materialsCount; ++i)
		{
			CreateMaterial(gfx, gAssets->materials[i]);
		}

		for (u32 i = 0; i < gAssets->entitiesCount; ++i)
		{
			CreateEntity(gfx, gAssets->entities[i]);
		}
	}

	// Pipelines
	gfx.shadowmapPipelineH = PipelineHandle(gfx, "pipeline_shadowmap");

	// Computes
	gfx.computeClearH = PipelineHandle(gfx, "compute_clear");
	gfx.computeUpdateH = PipelineHandle(gfx, "compute_update");

	// Samplers
	const SamplerDesc samplerDesc = {
		.addressMode = AddressModeRepeat,
	};
	gfx.samplerH = CreateSampler(gfx, samplerDesc);
	const SamplerDesc shadowmapSamplerDesc = {
		.addressMode = AddressModeClampToBorder,
		.borderColor = USE_REVERSE_Z ? BorderColorBlackFloat : BorderColorWhiteFloat,
		.compareOp = USE_REVERSE_Z ? CompareOpGreater : CompareOpLess,
	};
	gfx.shadowmapSamplerH = CreateSampler(gfx, shadowmapSamplerDesc);

	const Buffer &materialBuffer = GetBuffer(gfx, gfx.materialBuffer);

	CommandList commandList = BeginTransientCommandList(gfx.device);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));

		CopyBufferToBuffer(commandList, staged.buffer, staged.offset, materialBuffer.handle, material.bufferOffset, sizeof(shaderMaterial));
	}

	EndTransientCommandList(gfx.device, commandList);

	// DescriptorSets for materials
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		const Pipeline &pipeline = GetPipeline(gfx, material.pipelineH);
		const u32 globalBindGroupCount = ARRAY_COUNT(gfx.globalBindGroups[i]);
		for (u32 j = 0; j < globalBindGroupCount; ++j)
		{
			gfx.globalBindGroups[i][j] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[0], gfx.globalBindGroupAllocator);
		}
		gfx.materialBindGroups[i] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[1], gfx.materialBindGroupAllocator);
	}

	// Update material descriptor sets
	UpdateMaterialBindGroup(gfx, BIND_GROUP_MATERIAL);

	// Camera
	gfx.camera.position = {0, 1, 2};
	gfx.camera.orientation = {0, -0.45f};

	gfx.deviceInitialized = true;
}

void WaitDeviceIdle(Graphics &gfx)
{
	WaitDeviceIdle(gfx.device);

	gfx.stagingBufferOffset = 0;
}

void CleanupGraphics(Graphics &gfx)
{
	WaitDeviceIdle( gfx );

	DestroyBindGroupAllocator( gfx.device, gfx.globalBindGroupAllocator );
	DestroyBindGroupAllocator( gfx.device, gfx.materialBindGroupAllocator );
	for (u32 i = 0; i < ARRAY_COUNT(gfx.computeBindGroupAllocator); ++i)
	{
		DestroyBindGroupAllocator( gfx.device, gfx.computeBindGroupAllocator[i] );
	}
#if USE_IMGUI
	DestroyBindGroupAllocator( gfx.device, gfx.imGuiBindGroupAllocator );
#endif

	for (u32 i = 0; i < gfx.samplerCount; ++i)
	{
		DestroySampler( gfx.device, gfx.samplers[i] );
	}

	for (u32 i = 0; i < gfx.textureCount; ++i)
	{
		vkDestroyImageView( gfx.device.handle, gfx.textures[i].imageView, VULKAN_ALLOCATORS );
		DestroyImage( gfx.device, gfx.textures[i].image );
	}

	for (u32 i = 0; i < gfx.bufferCount; ++i)
	{
		DestroyBuffer( gfx.device, gfx.buffers[i] );
	}

	for (u32 i = 0; i < gfx.bufferViewCount; ++i)
	{
		DestroyBufferView( gfx.device, gfx.bufferViews[i] );
	}

	for (u32 i = 0; i < gfx.pipelineCount; ++i )
	{
		const Pipeline &pipeline = GetPipeline(gfx, i);
		DestroyPipeline( gfx.device, pipeline );
	}

	for (u32 i = 0; i < gfx.renderPassCount; ++i)
	{
		DestroyRenderPass(gfx.device, gfx.renderPasses[i]);
	}

	CleanupGraphicsDevice( gfx.device );

	CleanupGraphicsDriver( gfx.device );

	ZeroStruct( &gfx ); // deviceInitialized = false;
}

void LoadScene(Graphics &gfx)
{
	// TODO
}

void CleanupScene(Graphics &gfx)
{
	// TODO
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

uint2 GetFramebufferSize(const Framebuffer &framebuffer)
{
	const uint2 size = { framebuffer.extent.width, framebuffer.extent.height };
	return size;
}

Framebuffer GetDisplayFramebuffer(const Graphics &gfx)
{
	const u32 imageIndex = gfx.device.swapchain.currentImageIndex;
	const RenderPass &renderPass = GetRenderPass(gfx, gfx.litRenderPassH);

	const Framebuffer framebuffer = {
		.handle = gfx.renderTargets.framebuffers[imageIndex],
		.renderPassHandle = renderPass.handle,
		.extent = gfx.device.swapchain.extent,
		.isDisplay = true,
	};
	return framebuffer;
}

Framebuffer GetShadowmapFramebuffer(const Graphics &gfx)
{
	const RenderPass &renderPass = GetRenderPass(gfx, gfx.shadowmapRenderPassH);

	const Framebuffer framebuffer = {
		.handle = gfx.renderTargets.shadowmapFramebuffer,
		.renderPassHandle = renderPass.handle,
		.extent = { 1024, 1024 },
		.isShadowmap = true,
	};
	return framebuffer;
}


bool RenderGraphics(Graphics &gfx, Window &window, Arena &frameArena, f32 deltaSeconds)
{
	u32 frameIndex = gfx.device.currentFrame;

	if ( !BeginFrame(gfx.device) )
	{
		return false;
	}

	// Calculate camera matrices
	const f32 ar = static_cast<f32>(gfx.device.swapchain.extent.width) / static_cast<f32>(gfx.device.swapchain.extent.height);
	const float orthox = ar > 1.0f ? ar : 1.0f;
	const float orthoy = ar > 1.0f ? 1.0f : 1.0f/ar;
	const float4x4 viewMatrix = ViewMatrixFromCamera(gfx.camera);
	//const float4x4 = Orthogonal(-orthox, orthox, -orthoy, orthoy, -10, 10);
	const float preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
	const float fovy = (preRotationDegrees == 0 || preRotationDegrees == 180) ? 60.0f : 90.0f;
	const float4x4 preTransformMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
	const float4x4 perspectiveMatrix = Perspective(fovy, ar, 0.1f, 1000.0f);
	const float4x4 projectionMatrix = Mul(perspectiveMatrix, preTransformMatrix);

	// Sun matrices
	const float3 sunPos = float3{0.0f, 0.0f, 0.0f};
	const float3 sunVrp = float3{-2.0f, -6.0f, -4.0f};
	const float3 sunUp = float3{0.0f, 1.0f, 0.0f};
	const float4x4 sunViewMatrix = LookAt(sunVrp, sunPos, sunUp);
	const float4x4 sunProjMatrix = Orthogonal(-5.0f, 5.0f, -5.0f, 5.0f, -5.0f, 5.0f);

	// Update globals struct
	Globals globals;
	globals.cameraView = viewMatrix;
	globals.cameraProj = projectionMatrix;
	globals.sunView = sunViewMatrix;
	globals.sunProj = sunProjMatrix;
	globals.sunDir = Float4(Normalize(FromTo(sunVrp, sunPos)), 0.0f);
	globals.eyePosition = Float4(gfx.camera.position, 1.0f);
	globals.shadowmapDepthBias = USE_REVERSE_Z ? 0.005 : -0.005;

	// Update globals buffer
	Buffer &globalsBuffer = GetBuffer(gfx, gfx.globalsBuffer[frameIndex]);
	Heap &globalsBufferHeap = gfx.device.heaps[globalsBuffer.alloc.heap];
	void *ptr = globalsBufferHeap.data + globalsBuffer.alloc.offset;
	MemCopy( ptr, &globals, sizeof(globals) );

	// Update entity data
	Buffer &entityBuffer = GetBuffer(gfx, gfx.entityBuffer[frameIndex]);
	Heap &entityBufferHeap = gfx.device.heaps[entityBuffer.alloc.heap];
	SEntity *entities = (SEntity*)(entityBufferHeap.data + entityBuffer.alloc.offset);
	for (u32 i = 0; i < gfx.entityCount; ++i)
	{
		const Entity &entity = gfx.entities[i];
		const float4x4 worldMatrix = Mul(Translate(entity.position), Scale(Float3(entity.scale))); // TODO: Apply also rotation
		entities[i].world = worldMatrix;
	}

	// Reset descriptor pools for this frame
	ResetBindGroupAllocator( gfx.device, gfx.computeBindGroupAllocator[frameIndex] );

	// Update global descriptor sets
	UpdateMaterialBindGroup(gfx, BIND_GROUP_GLOBAL);

	// Record commands
	CommandList commandList = BeginCommandList(gfx.device);

	#define COMPUTE_TEST 0
	#if COMPUTE_TEST
	{
		const Pipeline &pipeline = GetPipeline(gfx, gfx.computeClearH);

		SetPipeline(commandList, pipeline);

		const BufferView &computeBufferView = GetBufferView(gfx, gfx.computeBufferViewH);

		const BindGroupDesc bindGroupDesc = {
			.layout = pipeline.layout.bindGroupLayouts[0],
			.bindings = {
				{ .bufferView = Binding(computeBufferView) },
			},
		};
		const BindGroup bindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.computeBindGroupAllocator[frameIndex]);

		SetBindGroup(commandList, 0, bindGroup);

		Dispatch(commandList, 1, 1, 1);
	}
	#endif // COMPUTE_TEST

	// Shadow map
	{
		const Framebuffer shadowmapFramebuffer = GetShadowmapFramebuffer(gfx);
		BeginRenderPass(commandList, shadowmapFramebuffer);

		const uint2 shadowmapSize = GetFramebufferSize(shadowmapFramebuffer);
		SetViewportAndScissor(commandList, shadowmapSize);

		// Pipeline
		const Pipeline &pipeline = gfx.pipelines[gfx.shadowmapPipelineH];
		SetPipeline(commandList, pipeline);

		const Sampler &sampler = GetSampler(gfx, gfx.samplerH);

		const BindGroupDesc bindGroupDesc = {
			.layout = pipeline.layout.bindGroupLayouts[0],
			.bindings = {
				{ .buffer = Binding(globalsBuffer) },
				{ .sampler = Binding(sampler) },
				{ .buffer = Binding(entityBuffer) },
			},
		};
		const BindGroup bindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.computeBindGroupAllocator[frameIndex]);
		SetBindGroup(commandList, 0, bindGroup);

		for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
		{
			const Entity &entity = gfx.entities[entityIndex];

			if ( !entity.visible ) continue;

			// Vertex buffer
			const Buffer &vertexBuffer = GetBuffer(gfx, gfx.globalVertexArena.buffer);
			SetVertexBuffer(commandList, vertexBuffer);

			// Index buffer
			const Buffer &indexBuffer = GetBuffer(gfx, gfx.globalIndexArena.buffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
			const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
		}

		EndRenderPass(commandList);
	}

	VkFormat depthFormat = FindDepthVkFormat(gfx.device);
	VkImage shadowmapImage = gfx.renderTargets.shadowmapImage.handle;
	TransitionImageLayout(commandList, shadowmapImage, depthFormat, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, 1);

	// Scene and UI
	{
		const Framebuffer displayFramebuffer = GetDisplayFramebuffer(gfx);
		BeginRenderPass(commandList, displayFramebuffer);

		const uint2 displaySize = GetFramebufferSize(displayFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
		{
			const Entity &entity = gfx.entities[entityIndex];

			if ( !entity.visible ) continue;

			const u32 materialIndex = entity.materialIndex;
			const Material &material = gfx.materials[materialIndex];
			const Pipeline &pipeline = gfx.pipelines[material.pipelineH];

			// Pipeline
			SetPipeline(commandList, pipeline);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[materialIndex][frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[materialIndex]);

			// Vertex buffer
			const Buffer &vertexBuffer = GetBuffer(gfx, gfx.globalVertexArena.buffer);
			SetVertexBuffer(commandList, vertexBuffer);

			// Index buffer
			const Buffer &indexBuffer = GetBuffer(gfx, gfx.globalIndexArena.buffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
			const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
		}

#if USE_IMGUI
		// Record dear imgui primitives into command buffer
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, commandList.handle);
#endif

		EndRenderPass(commandList);
	}

	TransitionImageLayout(commandList, shadowmapImage, depthFormat, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL, 1);

	EndCommandList(commandList);

	SubmitResult submitRes = Submit(gfx.device, commandList);

	if ( !Present(gfx.device, submitRes) ) {
		return false;
	}

	EndFrame(gfx.device);

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

	const RenderPass &renderPass = GetRenderPass(gfx, gfx.litRenderPassH);

	ImGui_ImplVulkan_InitInfo init_info = {};
	init_info.Instance = gfx.device.instance;
	init_info.PhysicalDevice = gfx.device.physicalDevice;
	init_info.Device = gfx.device.handle;
	init_info.QueueFamily = gfx.device.graphicsQueueFamilyIndex;
	init_info.Queue = gfx.device.graphicsQueue;
	init_info.PipelineCache = gfx.device.pipelineCache;
	init_info.DescriptorPool = gfx.imGuiBindGroupAllocator.handle;
	init_info.Subpass = 0;
	init_info.MinImageCount = gfx.device.swapchain.imageCount;
	init_info.ImageCount = gfx.device.swapchain.imageCount;
	init_info.MSAASamples = VK_SAMPLE_COUNT_1_BIT;
	init_info.Allocator = VULKAN_ALLOCATORS;
	init_info.CheckVkResultFn = CheckVulkanResultImGui;
	ImGui_ImplVulkan_Init(&init_info, renderPass.handle);

	// Upload Fonts
	{
		CommandList commandList = BeginTransientCommandList(gfx.device);

		ImGui_ImplVulkan_CreateFontsTexture(commandList.handle);

		EndTransientCommandList(gfx.device, commandList);

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
	vkGetPhysicalDeviceMemoryProperties(gfx.device.physicalDevice, &memoryProperties);


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
			const Heap &heap = gfx.device.heaps[i];
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
			const Heap &heap = gfx.device.heaps[i];
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
	gStringInterning = &platform.stringInterning;

	Graphics &gfx = GetPlatformGraphics(platform);

#if USE_IMGUI
	InitializeImGuiContext();
#endif

	// Initialize graphics
	if ( !InitializeGraphicsDriver(gfx.device, platform.globalArena) )
	{
		// TODO: Actually we could throw a system error and exit...
		LOG(Error, "InitializeGraphicsDriver failed!\n");
		return false;
	}

	return true;
}

bool EngineWindowInit(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	if ( !InitializeGraphicsSurface(gfx.device, platform.window) )
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
		if ( !InitializeGraphics(gfx, platform.globalArena, platform.window) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphics failed!\n");
			return false;
		}

		InitializeScene(platform.globalArena, gfx);
	}

#if USE_IMGUI
	InitializeImGuiGraphics(gfx, platform.window);
#endif

	return true;
}

void EngineUpdate(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	if ( gfx.device.swapchain.outdated || platform.window.flags & WindowFlags_WasResized )
	{
		WaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		DestroySwapchain(gfx.device, gfx.device.swapchain);
		gfx.device.swapchain = CreateSwapchain(gfx.device, platform.window, gfx.device.swapchainInfo);
		gfx.renderTargets = CreateRenderTargets(gfx);
		gfx.device.swapchain.outdated = false;
	}

	if ( gfx.deviceInitialized && gfx.device.swapchain.handle != VK_NULL_HANDLE )
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
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device, gfx.device.swapchain);
	CleanupGraphicsSurface(gfx.device);
}

void EngineCleanup(Platform &platform)
{
	Graphics &gfx = GetPlatformGraphics(platform);

	WaitDeviceIdle(gfx);

#if USE_IMGUI
	CleanupImGui();
#endif

	CleanupScene(gfx);

	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device, gfx.device.swapchain);
	CleanupGraphicsSurface(gfx.device);

	CleanupGraphics(gfx);
}

void EngineMain( void *userData )
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

#if PLATFORM_ANDROID
	platform.androidApp = (android_app*)userData;
#endif

	// User data
	Graphics gfx = {};
	platform.userData = &gfx;

	PlatformRun(platform);
}

#if PLATFORM_ANDROID
void android_main(struct android_app* app)
{
	EngineMain(app);
}
#else
int main(int argc, char **argv)
{
	EngineMain(NULL);
	return 1;
}
#endif

// TODO:
// - [ ] Instead of binding descriptors per entity, group entities by material and perform a multi draw call for each material group.
// - [ ] GPU culling: As a first step, perform frustum culling in the CPU.
// - [ ] GPU culling: Modify the compute to perform frustum culling and save the result in the buffer.
// - [ ] Avoid duplicated global descriptor sets.
//
// DONE:
// - [X] Avoid using push constants and put transformation matrices in buffers instead.
// - [X] Investigate how to write descriptors in a more elegant manner (avoid hardcoding).
// - [X] Put all the geometry in the same buffer.
// - [X] GPU culling: Add a "hello world" compute shader that writes some numbers into a buffer.

