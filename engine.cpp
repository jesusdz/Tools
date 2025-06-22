#define TOOLS_WINDOW
#include "tools.h"

#include "tools_gfx.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define USE_UI 1

#if USE_UI && !USE_IMGUI

#define STB_RECT_PACK_IMPLEMENTATION
#include "stb/stb_rect_pack.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb/stb_truetype.h"

#endif

#if USE_UI
#include "tools_ui.h"
#endif

// C/HLSL shared types and bindings
#include "shaders/types.hlsl"
#include "shaders/bindings.hlsl"

// Assets
#include "assets/assets.h"
#include "clon.h"




#define MAX_TEXTURES 8 
#define MAX_MATERIALS 4
#define MAX_ENTITIES 32
#define MAX_GLOBAL_BINDINGS 8
#define MAX_MATERIAL_BINDINGS 4
#define MAX_COMPUTE_BINDINGS 4

#define INVALID_HANDLE -1

#define USE_ENTITY_SELECTION 1




struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct Texture
{
	const char *name;
	ImageH image;
};

typedef u32 TextureH;

struct Material
{
	const char *name;
	PipelineH pipelineH;
	TextureH albedoTexture;
	f32 uvScale;
	u32 bufferOffset;
	MaterialDesc desc;
};

typedef u32 MaterialH;

struct RenderTargets
{
	ImageH depthImage;
	Framebuffer framebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	ImageH shadowmapImage;
	Framebuffer shadowmapFramebuffer;

	ImageH idImage;
	Framebuffer idFramebuffer;
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
	bool culled;
	BufferChunk vertices;
	BufferChunk indices;
	u16 materialIndex;
};

#define MAX_TIME_SAMPLES 32
struct TimeSamples
{
	f32 samples[MAX_TIME_SAMPLES];
	u32 sampleCount;
	f32 average;
};

struct Graphics
{
	GraphicsDevice device;

	RenderTargets renderTargets;

	BufferH stagingBuffer;
	u32 stagingBufferOffset;

	BufferArena globalVertexArena;
	BufferArena globalIndexArena;

	BufferChunk cubeVertices;
	BufferChunk cubeIndices;
	BufferChunk planeVertices;
	BufferChunk planeIndices;
	BufferChunk screenTriangleVertices;
	BufferChunk screenTriangleIndices;

	BufferH globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH entityBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH materialBuffer;
	BufferH computeBufferH;
	BufferViewH computeBufferViewH;
#if USE_ENTITY_SELECTION
	BufferH selectionBufferH;
	BufferViewH selectionBufferViewH;
#endif

	SamplerH materialSamplerH;
	SamplerH shadowmapSamplerH;
	SamplerH skySamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;
	RenderPassH idRenderPassH;

	Texture textures[MAX_TEXTURES];
	u32 textureCount;

	Material materials[MAX_MATERIALS];
	u32 materialCount;

	BindGroupAllocator globalBindGroupAllocator;
	BindGroupAllocator materialBindGroupAllocator;
	BindGroupAllocator dynamicBindGroupAllocator[MAX_FRAMES_IN_FLIGHT];
#if USE_IMGUI
	BindGroupAllocator imGuiBindGroupAllocator;
#endif

	BindGroupLayout globalBindGroupLayout;

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	BindGroup globalBindGroups[MAX_FRAMES_IN_FLIGHT];

	// Updated once at the beginning for each material
	BindGroup materialBindGroups[MAX_MATERIALS];

	TimestampPool timestampPools[MAX_FRAMES_IN_FLIGHT];

	Camera camera;

	Entity entities[MAX_ENTITIES];
	u32 entityCount;

	bool selectEntity;
	u32 selectedEntity;

	TextureH skyTextureH;

	PipelineH shadowmapPipelineH;
	PipelineH skyPipelineH;
	PipelineH guiPipelineH;
#if USE_ENTITY_SELECTION
	PipelineH idPipelineH;
#endif

#define USE_COMPUTE_TEST 0
#if USE_COMPUTE_TEST
	PipelineH computeClearH;
	PipelineH computeUpdateH;
#endif
	PipelineH computeSelectH;

	bool deviceInitialized;

	TimeSamples cpuFrameTimes;
	TimeSamples gpuFrameTimes;

#if USE_UI
	UI ui;
#endif

	u32 reloadQueue[128];
	u32 reloadQueueSize;
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

static const Vertex screenTriangleVertices[] = {
	{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
	{{-1.0f, -3.0f, 0.0f}, {0.0f, 2.0f}},
	{{ 3.0f,  1.0f, 0.0f}, {2.0f, 0.0f}},
};

static const u16 screenTriangleIndices[] = {
	0, 1, 2,
};

enum ShaderType
{
	ShaderTypeVertex,
	ShaderTypeFragment,
	ShaderTypeCompute
};

struct ShaderSourceDesc
{
	ShaderType type;
	const char *filename;
	const char *entryPoint;
	const char *outputName;
};

static const ShaderSourceDesc shaderSources[] = {
	{ .type = ShaderTypeVertex,   .filename = "shading.hlsl",        .entryPoint = "VSMain",      .outputName = "vs_shading" },
	{ .type = ShaderTypeFragment, .filename = "shading.hlsl",        .entryPoint = "PSMain",      .outputName = "fs_shading" },
	{ .type = ShaderTypeVertex,   .filename = "sky.hlsl",            .entryPoint = "VSMain",      .outputName = "vs_sky" },
	{ .type = ShaderTypeFragment, .filename = "sky.hlsl",            .entryPoint = "PSMain",      .outputName = "fs_sky" },
	{ .type = ShaderTypeVertex,   .filename = "shadowmap.hlsl",      .entryPoint = "VSMain",      .outputName = "vs_shadowmap" },
	{ .type = ShaderTypeFragment, .filename = "shadowmap.hlsl",      .entryPoint = "PSMain",      .outputName = "fs_shadowmap" },
	{ .type = ShaderTypeVertex,   .filename = "ui.hlsl",             .entryPoint = "VSMain",      .outputName = "vs_ui" },
	{ .type = ShaderTypeFragment, .filename = "ui.hlsl",             .entryPoint = "PSMain",      .outputName = "fs_ui" },
	{ .type = ShaderTypeVertex,   .filename = "id.hlsl",             .entryPoint = "VSMain",      .outputName = "vs_id" },
	{ .type = ShaderTypeFragment, .filename = "id.hlsl",             .entryPoint = "PSMain",      .outputName = "fs_id" },
	{ .type = ShaderTypeCompute,  .filename = "compute_select.hlsl", .entryPoint = "CSMain",      .outputName = "compute_select" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_clear",  .outputName = "compute_clear" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_update", .outputName = "compute_update" },
};

static const PipelineDesc pipelineDescs[] =
{
	{
		.name = "pipeline_shading",
		.vsFilename = "shaders/vs_shading.spv",
		.fsFilename = "shaders/fs_shading.spv",
		.vsFunction = "VSMain",
		.fsFunction = "PSMain",
		.renderPass = "main_renderpass",
		.vertexBufferCount = 1,
		.vertexBuffers = { { .stride = 32 }, },
		.vertexAttributeCount = 3,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
		},
		.depthTest = true,
		.depthWrite = true,
		.depthCompareOp = CompareOpGreater,
	},
	{
		.name = "pipeline_shadowmap",
		.vsFilename = "shaders/vs_shadowmap.spv",
		.fsFilename = "shaders/fs_shadowmap.spv",
		.vsFunction = "VSMain",
		.fsFunction = "PSMain",
		.renderPass = "shadowmap_renderpass",
		.vertexBufferCount = 1,
		.vertexBuffers = { { .stride = 32 }, },
		.vertexAttributeCount = 1,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
		},
		.depthTest = true,
		.depthWrite = true,
		.depthCompareOp = CompareOpGreater,
	},
	{
		.name = "pipeline_sky",
		.vsFilename = "shaders/vs_sky.spv",
		.fsFilename = "shaders/fs_sky.spv",
		.vsFunction = "VSMain",
		.fsFunction = "PSMain",
		.renderPass = "main_renderpass",
		.vertexBufferCount = 1,
		.vertexBuffers = { { .stride = 32 }, },
		.vertexAttributeCount = 2,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat2, },
		},
		.depthTest = true,
		.depthWrite = false,
		.depthCompareOp = CompareOpGreaterOrEqual,
	},
	{
		.name = "pipeline_ui",
		.vsFilename = "shaders/vs_ui.spv",
		.fsFilename = "shaders/fs_ui.spv",
		.vsFunction = "VSMain",
		.fsFunction = "PSMain",
		.renderPass = "main_renderpass",
		.vertexBufferCount = 1,
		.vertexBuffers = { { .stride = 20 }, },
		.vertexAttributeCount = 3,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat2, },
			{ .bufferIndex = 0, .location = 1, .offset = 8, .format = FormatFloat2, },
			{ .bufferIndex = 0, .location = 2, .offset = 16, .format = FormatRGBA8, },
		},
		.depthTest = false,
		.blending = true,
	},
#if USE_ENTITY_SELECTION
	{
		.name = "pipeline_id",
		.vsFilename = "shaders/vs_id.spv",
		.fsFilename = "shaders/fs_id.spv",
		.vsFunction = "VSMain",
		.fsFunction = "PSMain",
		.renderPass = "id_renderpass",
		.vertexBufferCount = 1,
		.vertexBuffers = { { .stride = 32 }, },
		.vertexAttributeCount = 3,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
		},
		.depthTest = true,
		.depthWrite = false,
		.depthCompareOp = CompareOpEqual,
	},
#endif // USE_ENTITY_SELECTION
};

static PipelineH pipelineHandles[ARRAY_COUNT(pipelineDescs)] = {};

static const ComputeDesc computeDescs[] =
{
	//{ .name = "compute_clear", .filename = "shaders/compute_clear.spv", .function = "main_clear" },
	//{ .name = "compute_update", .filename = "shaders/compute_update.spv", .function = "main_update" },
	{ .name = "compute_select", .filename = "shaders/compute_select.spv", .function = "CSMain" },
};

static PipelineH computeHandles[ARRAY_COUNT(computeDescs)] = {};

static StringInterning *gStringInterning;

Graphics &GetPlatformGraphics(Platform &platform)
{
	Graphics *gfx = (Graphics*)platform.userData;
	ASSERT(gfx != NULL);
	return *gfx;
}

void AddTimeSample(TimeSamples &timeSamples, f32 sample)
{
	timeSamples.samples[timeSamples.sampleCount] = sample;
	timeSamples.sampleCount = ( timeSamples.sampleCount + 1 ) % ARRAY_COUNT(timeSamples.samples);
	f32 sum = 0.0f;
	for (u32 i = 0; i < ARRAY_COUNT(timeSamples.samples); ++i)
	{
		sum += timeSamples.samples[i];
	}
	timeSamples.average = sum / ARRAY_COUNT(timeSamples.samples);
}

RenderPassH FindRenderPassHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.device.renderPassCount; ++i) {
		if ( StrEq(gfx.device.renderPasses[i].name, name) ) {
			return { .index = i };
		}
	}
	LOG(Warning, "Could not find render <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return { .index = (u32)INVALID_HANDLE };
}

PipelineH FindPipelineHandle(const Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i) {
		if ( StrEq(pipelineDescs[i].name, name) ) {
			return pipelineHandles[i];
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i) {
		if ( StrEq(computeDescs[i].name, name) ) {
			return computeHandles[i];
		}
	}
	LOG(Warning, "Could not find pipeline <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return { .index = (u32)INVALID_HANDLE };
}



struct StagedData
{
	BufferH buffer;
	u32 offset;
};

StagedData StageData(Graphics &gfx, const void *data, u32 size, u32 alignment = 0)
{
	const Buffer &stagingBuffer = GetBuffer(gfx.device, gfx.stagingBuffer);

	const u32 finalAlignment = Max(alignment, gfx.device.alignment.optimalBufferCopyOffset);
	const u32 unalignedOffset = stagingBuffer.alloc.offset + gfx.stagingBufferOffset;
	const u32 alignedOffset = AlignUp(unalignedOffset, finalAlignment);

	StagedData staging = {};
	staging.buffer = gfx.stagingBuffer;
	staging.offset = alignedOffset;

	Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	void* stagingData = stagingHeap.data + staging.offset;
	MemCopy(stagingData, data, (size_t) size);

	gfx.stagingBufferOffset = staging.offset + size;

	return staging;
}

BufferH CreateStagingBuffer(Graphics &gfx)
{
	const Heap &stagingHeap = gfx.device.heaps[HeapType_Staging];
	BufferH stagingBufferHandle = CreateBuffer(gfx.device, stagingHeap.size, BufferUsageTransferSrc, HeapType_Staging);
	return stagingBufferHandle;
}

BufferH CreateVertexBuffer(Graphics &gfx, u32 size)
{
	BufferH vertexBufferHandle = CreateBuffer(
			gfx.device,
			size,
			BufferUsageVertexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return vertexBufferHandle;
}

BufferH CreateIndexBuffer(Graphics &gfx, u32 size)
{
	BufferH indexBufferHandle = CreateBuffer(
			gfx.device,
			size,
			BufferUsageIndexBuffer | BufferUsageTransferDst,
			HeapType_General);

	return indexBufferHandle;
}

BufferArena MakeBufferArena(Graphics &gfx, BufferH bufferHandle)
{
	const BufferArena arena = {
		.buffer = bufferHandle,
		.used = 0,
	};
	return arena;
}

BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment = 0)
{
	StagedData staged = StageData(gfx, data, size, alignment);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(commandList, staged.buffer, staged.offset, arena.buffer, arena.used, size);

	BufferChunk chunk = {};
	chunk.buffer = arena.buffer;
	chunk.offset = arena.used;
	chunk.size = size;

	arena.used += size;

	return chunk;
}

void GenerateMipmaps(const GraphicsDevice &device, const CommandList &commandList, ImageH imageH)
{
	const Image &image = GetImage(device, imageH);

	if (!device.formatSupport[image.format].linearFilteredSampling) {
		LOG(Error, "GenerateMipmaps() - Linear filtering not supported for format %s.\n", FormatName(image.format));
		QUIT_ABNORMALLY();
	}

	for (u32 i = 1; i < image.mipLevels; ++i)
	{
		TransitionImageLayout(commandList, imageH, ImageStateTransferDst, ImageStateTransferSrc, i - 1, 1);

		const i32 srcWidth = image.width > 1 ? image.width >> (i-1): 1;
		const i32 srcHeight = image.height > 1 ? image.height >> (i-1) : 1;
		const i32 dstWidth = image.width > 1 ? image.width >> i : 1;
		const i32 dstHeight = image.height > 1 ? image.height >> i : 1;
		const BlitRegion srcRegion = { .x = 0, .y = 0, .width = srcWidth, .height = srcHeight, .mipLevel = i - 1 };
		const BlitRegion dstRegion = { .x = 0, .y = 0, .width = dstWidth, .height = dstHeight, .mipLevel = i };

		Blit(commandList, image, srcRegion, image, dstRegion);

		TransitionImageLayout(commandList, imageH, ImageStateTransferSrc, ImageStateShaderInput, i - 1, 1);
	}

	TransitionImageLayout(commandList, imageH, ImageStateTransferDst, ImageStateShaderInput, image.mipLevels - 1, 1);
}

ImageH CreateImage(Graphics &gfx, const char *name, int width, int height, int channels, bool mipmap, const byte *pixels)
{
	const u32 pixelSize = channels * sizeof(byte);
	const u32 size = width * height * pixelSize;
	const u32 alignment = channels == 1 ? 1 : 4;

	StagedData staged = StageData(gfx, pixels, size, alignment);

	const u32 mipLevels = mipmap ?
		static_cast<uint32_t>(Floor(Log2(Max(width, height)))) + 1 :
		1;

	ASSERT(channels >= 1 && channels <= 4);
	const Format texFormat =
		channels == 4 ? FormatRGBA8_SRGB :
		channels == 3 ? FormatRGB8_SRGB :
		channels == 2 ? FormatRG8_SRGB :
		FormatR8;

	ImageH image = CreateImage(gfx.device,
			width, height, mipLevels,
			texFormat,
			ImageUsageTransferSrc | // for mipmap blits
			ImageUsageTransferDst | // for intitial copy from buffer and blits
			ImageUsageSampled, // to be sampled in shaders
			HeapType_General);

	CommandList commandList = BeginTransientCommandList(gfx.device);

	TransitionImageLayout(commandList, image, ImageStateInitial, ImageStateTransferDst, 0, mipLevels);

	CopyBufferToImage(commandList, staged.buffer, staged.offset, image);

	if ( mipLevels > 1 )
	{
		// GenerateMipmaps takes care of transitions after creating the image
		GenerateMipmaps(gfx.device, commandList, image);
	}
	else
	{
		TransitionImageLayout(commandList, image, ImageStateTransferDst, ImageStateShaderInput, 0, 1);
	}

	EndTransientCommandList(gfx.device, commandList);

	return image;
}

TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	int texWidth, texHeight, texChannels;
	FilePath imagePath = MakePath(desc.filename);
	stbi_uc* originalPixels = stbi_load(imagePath.str, &texWidth, &texHeight, &texChannels, STBI_rgb_alpha);
	texChannels = 4; // Because we use STBI_rgb_alpha
	stbi_uc* pixels = originalPixels;
	if ( !pixels )
	{
		LOG(Error, "stbi_load failed to load %s\n", imagePath.str);
		static stbi_uc constPixels[] = {255, 0, 255, 255};
		pixels = constPixels;
		texWidth = texHeight = 1;
		texChannels = 4;
	}

	const ImageH imageHandle = CreateImage(gfx, desc.name, texWidth, texHeight, texChannels, desc.mipmap, pixels);

	ASSERT( gfx.textureCount < ARRAY_COUNT(gfx.textures) );
	const TextureH textureHandle = gfx.textureCount++;
	gfx.textures[textureHandle].name = desc.name;
	gfx.textures[textureHandle].image = imageHandle;

	if ( originalPixels )
	{
		stbi_image_free(originalPixels);
	}

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
	PipelineH pipelineHandle = FindPipelineHandle(gfx, desc.pipelineName);

	ASSERT(gfx.materialCount < MAX_MATERIALS);
	MaterialH materialHandle = gfx.materialCount++;
	gfx.materials[materialHandle].name = desc.name;
	gfx.materials[materialHandle].pipelineH = pipelineHandle;
	gfx.materials[materialHandle].albedoTexture = textureHandle;
	gfx.materials[materialHandle].uvScale = desc.uvScale;
	gfx.materials[materialHandle].bufferOffset = materialHandle * AlignUp(sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset);
	gfx.materials[materialHandle].desc = desc;

	return materialHandle;
}

Material &GetMaterial( Graphics &gfx, MaterialH materialHandle )
{
	Material &material = gfx.materials[materialHandle];
	return material;
}

MaterialH FindMaterialHandle(const Graphics &gfx, const char *name)
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
	} else if ( geometryType == GeometryTypePlane ) {
		return gfx.planeVertices;
	} else {
		return gfx.screenTriangleVertices;
	}
}

BufferChunk GetIndicesForGeometryType(Graphics &gfx, GeometryType geometryType)
{
	if ( geometryType == GeometryTypeCube) {
		return gfx.cubeIndices;
	} else if ( geometryType == GeometryTypePlane ) {
		return gfx.planeIndices;
	} else {
		return gfx.screenTriangleIndices;
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
		gfx.entities[entityIndex].materialIndex = FindMaterialHandle(gfx, desc.materialName);
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

	const Format depthFormat = gfx.device.defaultDepthFormat;
	const u32 swapchainWidth = gfx.device.swapchain.extent.width;
	const u32 swapchainHeight = gfx.device.swapchain.extent.height;

	// Depth buffer
	renderTargets.depthImage = CreateImage(gfx.device,
			swapchainWidth, swapchainHeight, 1,
			depthFormat,
			ImageUsageDepthStencilAttachment,
			HeapType_RTs);
	TransitionImageLayout(commandList, renderTargets.depthImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);

	// Framebuffer
	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		const FramebufferDesc desc = {
			.renderPass = gfx.litRenderPassH,
			.attachments = {
				gfx.device.swapchain.images[i],
				renderTargets.depthImage,
			},
			.attachmentCount = 2,
		};

		renderTargets.framebuffers[i] = CreateFramebuffer(gfx.device, desc);
	}

	// Shadowmap
	{
		renderTargets.shadowmapImage = CreateImage(gfx.device,
				1024, 1024, 1,
				depthFormat,
				ImageUsageDepthStencilAttachment | ImageUsageSampled,
				HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.shadowmapImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);

		const FramebufferDesc desc = {
			.renderPass = gfx.shadowmapRenderPassH,
			.attachments = { renderTargets.shadowmapImage },
			.attachmentCount = 1,
		};

		renderTargets.shadowmapFramebuffer = CreateFramebuffer( gfx.device, desc );
	}

	// ID buffer
	{
		renderTargets.idImage = CreateImage(gfx.device,
			 swapchainWidth, swapchainHeight, 1,
			 FormatUInt,
			 ImageUsageColorAttachment | ImageUsageSampled,
			 HeapType_RTs);
		TransitionImageLayout(commandList, renderTargets.idImage, ImageStateInitial, ImageStateRenderTarget, 0, 1);

		const FramebufferDesc desc = {
			.renderPass = gfx.idRenderPassH,
			.attachments = { renderTargets.idImage, renderTargets.depthImage },
			.attachmentCount = 2,
		};

		renderTargets.idFramebuffer = CreateFramebuffer( gfx.device, desc );
	}

	EndTransientCommandList(gfx.device, commandList);

	return renderTargets;
}

void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	DestroyImageH(gfx.device, renderTargets.depthImage);

	// Reset the heap used for render targets
	Heap &rtHeap = gfx.device.heaps[HeapType_RTs];
	rtHeap.used = 0;

	for ( u32 i = 0; i < gfx.device.swapchain.imageCount; ++i )
	{
		DestroyFramebuffer( gfx.device, renderTargets.framebuffers[i] );
	}

	DestroyImageH(gfx.device, renderTargets.shadowmapImage);
	DestroyFramebuffer( gfx.device, renderTargets.shadowmapFramebuffer );

	DestroyImageH(gfx.device, renderTargets.idImage);
	DestroyFramebuffer( gfx.device, renderTargets.idFramebuffer );

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
		const StoreOp storeOp = USE_ENTITY_SELECTION ? StoreOpStore : StoreOpDontCare;

		const RenderpassDesc renderpassDesc = {
			.name = "main_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = FormatBGRA8_SRGB, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = true },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpClear, .storeOp = storeOp
			}
		};
		gfx.litRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
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
		gfx.shadowmapRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
	}

	// ID render pass
	{
		const RenderpassDesc renderpassDesc = {
			.name = "id_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = FormatUInt, .loadOp = LoadOpClear, .storeOp = StoreOpStore },
			},
			.hasDepthAttachment = true,
			.depthAttachment = {
				.loadOp = LoadOpLoad, .storeOp = StoreOpStore
			}
		};
		gfx.idRenderPassH = CreateRenderPass( gfx.device, renderpassDesc );
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
	gfx.screenTriangleVertices = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices, sizeof(screenTriangleVertices));
	gfx.screenTriangleIndices = PushData(gfx, commandList, gfx.globalIndexArena, screenTriangleIndices, sizeof(screenTriangleIndices));

	EndTransientCommandList(gfx.device, commandList);

	// Create globals buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 globalsBufferSize = sizeof(Globals);
		gfx.globalsBuffer[i] = CreateBuffer(
			gfx.device,
			globalsBufferSize,
			BufferUsageUniformBuffer,
			HeapType_Dynamic);
	}

	// Create entities buffer
	for (u32 i = 0; i < MAX_FRAMES_IN_FLIGHT; ++i)
	{
		const u32 entityBufferSize = MAX_ENTITIES * AlignUp( sizeof(SEntity), gfx.device.alignment.uniformBufferOffset );
		gfx.entityBuffer[i] = CreateBuffer(
			gfx.device,
			entityBufferSize,
			BufferUsageStorageBuffer,
			HeapType_Dynamic);
	}

	// Create material buffer
	const u32 materialBufferSize = MAX_MATERIALS * AlignUp( sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset );
	gfx.materialBuffer = CreateBuffer(gfx.device, materialBufferSize, BufferUsageUniformBuffer | BufferUsageTransferDst, HeapType_General);

	// Create buffer for computes
	const u32 computeBufferSize = sizeof(float);
	gfx.computeBufferH = CreateBuffer(gfx.device, computeBufferSize, BufferUsageStorageTexelBuffer, HeapType_General);
	gfx.computeBufferViewH = CreateBufferView(gfx.device, gfx.computeBufferH, FormatFloat);

#if USE_ENTITY_SELECTION
	const u32 selectionBufferSize = AlignUp( sizeof(u32), gfx.device.alignment.storageBufferOffset );
	gfx.selectionBufferH = CreateBuffer(gfx.device, selectionBufferSize, BufferUsageStorageTexelBuffer, HeapType_Readback);
	gfx.selectionBufferViewH = CreateBufferView(gfx.device, gfx.selectionBufferH, FormatUInt);
#endif // USE_ENTITY_SELECTION


	// Create Global BindGroup allocator
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = MAX_FRAMES_IN_FLIGHT,
			.storageBufferCount = MAX_FRAMES_IN_FLIGHT,
			.textureCount = 1000,
			.samplerCount = MAX_FRAMES_IN_FLIGHT * 2,
			.groupCount = MAX_FRAMES_IN_FLIGHT,
		};
		gfx.globalBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}

	// Create Material BindGroup allocator
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = MAX_MATERIALS,
			.textureCount = MAX_MATERIALS,
			.groupCount = MAX_MATERIALS,
		};
		gfx.materialBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}

	// Create dynamic per-frame BindGroup allocator
	for (u32 i = 0; i < ARRAY_COUNT(gfx.dynamicBindGroupAllocator); ++i)
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.uniformBufferCount = 1000,
			.storageBufferCount = 1000,
			.storageTexelBufferCount = 1000,
			.textureCount = 1000,
			.samplerCount = 1000,
			.groupCount = 100,
		};
		gfx.dynamicBindGroupAllocator[i] = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}

#if USE_IMGUI
	// Create Imgui BindGroup allocator
	{
		const BindGroupAllocatorCounts allocatorCounts = {
			.combinedImageSamplerCount = 1,
			.groupCount = 1,
			.allowIndividualFrees = true,
		};
		gfx.imGuiBindGroupAllocator = CreateBindGroupAllocator(gfx.device, allocatorCounts);
	}
#endif

	// Create global BindGroup layout
	const ShaderBinding globalShaderBindings[] = {
		{ .set = 0, .binding = BINDING_GLOBALS, .type = SpvTypeUniformBuffer, .stageFlags = SpvStageFlagsVertexBit | SpvStageFlagsFragmentBit | SpvStageFlagsComputeBit },
		{ .set = 0, .binding = BINDING_SAMPLER, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_ENTITIES, .type = SpvTypeStorageBuffer, .stageFlags = SpvStageFlagsVertexBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP, .type = SpvTypeImage, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP_SAMPLER, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
	};
	gfx.globalBindGroupLayout = CreateBindGroupLayout(gfx.device, globalShaderBindings, ARRAY_COUNT(globalShaderBindings));

	// Graphics pipelines
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		const RenderPassH renderPassH = FindRenderPassHandle(gfx, pipelineDescs[i].renderPass);
		LOG(Info, "Creating Graphics Pipeline: %s\n", pipelineDescs[i].name);
		const PipelineH pipelineH = CreateGraphicsPipeline(gfx.device, scratch, pipelineDescs[i], renderPassH, gfx.globalBindGroupLayout);
		pipelineHandles[i] = pipelineH;
		SetObjectName(gfx.device, pipelineH, pipelineDescs[i].name);
	}

	// Compute pipelines
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i)
	{
		LOG(Info, "Creating Compute Pipeline: %s\n", computeDescs[i].name);
		const PipelineH pipelineH = CreateComputePipeline(gfx.device, scratch, computeDescs[i], gfx.globalBindGroupLayout);
		computeHandles[i] = pipelineH;
		SetObjectName(gfx.device, pipelineH, computeDescs[i].name);
	}

#if USE_UI
	UI_Initialize(gfx.ui, gfx, gfx.device, scratch);
#endif

	return true;
}

BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameIndex)
{
	const BindGroupDesc bindGroupDesc = {
		.layout = gfx.globalBindGroupLayout,
		.bindings = {
			{ .index = BINDING_GLOBALS, .buffer = gfx.globalsBuffer[frameIndex] },
			{ .index = BINDING_SAMPLER, .sampler = gfx.materialSamplerH },
			{ .index = BINDING_ENTITIES, .buffer = gfx.entityBuffer[frameIndex] },
			{ .index = BINDING_SHADOWMAP, .image = gfx.renderTargets.shadowmapImage },
			{ .index = BINDING_SHADOWMAP_SAMPLER, .sampler = gfx.shadowmapSamplerH },
		},
	};
	return bindGroupDesc;
}

void UpdateGlobalBindGroups(Graphics &gfx)
{
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		const BindGroupDesc globalBindGroupDesc = GlobalBindGroupDesc(gfx, i);
		UpdateBindGroup(gfx.device, globalBindGroupDesc, gfx.globalBindGroups[i]);
	}
}

BindGroupDesc MaterialBindGroupDesc(const Graphics &gfx, const Material &material)
{
	const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
	const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[BIND_GROUP_MATERIAL];
	const Texture &albedoTexture = GetTexture(gfx, material.albedoTexture);

	const BindGroupDesc bindGroupDesc = {
		.layout = bindGroupLayout,
		.bindings = {
			{ .index = BINDING_MATERIAL, .buffer = gfx.materialBuffer, .offset = material.bufferOffset, .range = sizeof(SMaterial) },
			{ .index = BINDING_ALBEDO, .image = albedoTexture.image },
		},
	};
	return bindGroupDesc;
}

void UpdateMaterialBindGroups(Graphics &gfx)
{
	for (u32 materialIndex = 0; materialIndex < gfx.materialCount; ++materialIndex)
	{
		const Material &material = GetMaterial(gfx, materialIndex);
		const BindGroupDesc materialBindGroupDesc = MaterialBindGroupDesc(gfx, material);
		UpdateBindGroup(gfx.device, materialBindGroupDesc, gfx.materialBindGroups[materialIndex]);
	}
}

void LinkPipelineHandles(Graphics &gfx)
{
	// Graphics pipelines
	gfx.shadowmapPipelineH = FindPipelineHandle(gfx, "pipeline_shadowmap");
	gfx.skyPipelineH = FindPipelineHandle(gfx, "pipeline_sky");
	gfx.guiPipelineH = FindPipelineHandle(gfx, "pipeline_ui");
#if USE_ENTITY_SELECTION
	gfx.idPipelineH = FindPipelineHandle(gfx, "pipeline_id");
#endif // USE_ENTITY_SELECTION

	// Compute pipelines
#if USE_COMPUTE_TEST
	gfx.computeClearH = FindPipelineHandle(gfx, "compute_clear");
	gfx.computeUpdateH = FindPipelineHandle(gfx, "compute_update");
#endif // USE_COMPUTE_TEST
	gfx.computeSelectH = FindPipelineHandle(gfx, "compute_select");

	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		Material &material = gfx.materials[i];
		material.pipelineH = FindPipelineHandle(gfx, material.desc.pipelineName);
	}
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
	else
	{
		LOG(Error, "Could not get the global 'gAssets' from Clon object.\n");
	}

	// Textures
	gfx.skyTextureH = TextureHandle(gfx, "tex_sky");

	// Pipelines
	LinkPipelineHandles(gfx);

	// Samplers
	const SamplerDesc materialSamplerDesc = {
		.addressMode = AddressModeRepeat,
	};
	gfx.materialSamplerH = CreateSampler(gfx.device, materialSamplerDesc);
	const SamplerDesc shadowmapSamplerDesc = {
		.addressMode = AddressModeClampToBorder,
		.borderColor = BorderColorBlackFloat,
		.compareOp = CompareOpGreater,
	};
	gfx.shadowmapSamplerH = CreateSampler(gfx.device, shadowmapSamplerDesc);
	const SamplerDesc skySamplerDesc = {
		.addressMode = AddressModeClampToEdge,
	};
	gfx.skySamplerH = CreateSampler(gfx.device, skySamplerDesc);

	CommandList commandList = BeginTransientCommandList(gfx.device);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));

		CopyBufferToBuffer(commandList, staged.buffer, staged.offset, gfx.materialBuffer, material.bufferOffset, sizeof(shaderMaterial));
	}

	EndTransientCommandList(gfx.device, commandList);

	// BindGroups for globals
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		gfx.globalBindGroups[i] = CreateBindGroup(gfx.device, gfx.globalBindGroupLayout, gfx.globalBindGroupAllocator);
	}

	// BindGroups for materials
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
		gfx.materialBindGroups[i] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[1], gfx.materialBindGroupAllocator);
	}

	// Update bind groups
	UpdateGlobalBindGroups(gfx);
	UpdateMaterialBindGroups(gfx);

	// Timestamp queries
	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		gfx.timestampPools[i] = CreateTimestampPool(gfx.device, 128);
		//ResetTimestampPool(gfx.device, gfx.timestampPools[i]); // Vulkan 1.2
	}

	// Camera
	gfx.camera.position = {0, 1, 2};
	gfx.camera.orientation = {0, -0.45f};

	gfx.deviceInitialized = true;

	gfx.selectedEntity = U32_MAX;
}

void WaitDeviceIdle(Graphics &gfx)
{
	WaitDeviceIdle(gfx.device);

	gfx.stagingBufferOffset = 0;
}

void CleanupGraphics(Graphics &gfx)
{
	WaitDeviceIdle( gfx );

	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		DestroyTimestampPool(gfx.device, gfx.timestampPools[i]);
	}

	DestroyBindGroupAllocator( gfx.device, gfx.globalBindGroupAllocator );
	DestroyBindGroupAllocator( gfx.device, gfx.materialBindGroupAllocator );
	for (u32 i = 0; i < ARRAY_COUNT(gfx.dynamicBindGroupAllocator); ++i)
	{
		DestroyBindGroupAllocator( gfx.device, gfx.dynamicBindGroupAllocator[i] );
	}
#if USE_IMGUI
	DestroyBindGroupAllocator( gfx.device, gfx.imGuiBindGroupAllocator );
#endif

	CleanupGraphicsDevice( gfx.device );

	CleanupGraphicsDriver( gfx.device );

#if USE_UI
	UI_Cleanup(gfx.ui);
#endif

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

void AnimateCamera(const Window &window, Camera &camera, float deltaSeconds, bool handleInput)
{
	float3 dir = { 0, 0, 0 };

	if ( handleInput )
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
	}

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

#if USE_ENTITY_SELECTION
void BeginEntitySelection(Graphics &gfx, const Mouse &mouse, bool handleInput)
{
	static bool mouseMoved = false;
	if ( handleInput )
	{
		if (MouseButtonPress(mouse, MOUSE_BUTTON_LEFT)) {
			mouseMoved = false;
		}
		if (mouse.dx != 0 || mouse.dx != 0) {
			mouseMoved = true;
		}
		if (!mouseMoved && MouseButtonRelease(mouse, MOUSE_BUTTON_LEFT)) {
			gfx.selectEntity = true;
		}
	}
}

void EndEntitySelection(Graphics &gfx)
{
	if ( gfx.selectEntity )
	{
		WaitDeviceIdle(gfx.device);
		gfx.selectedEntity = *(u32*)GetBufferPtr(gfx.device, gfx.selectionBufferH);
		gfx.selectEntity = false;
	}
}
#endif // USE_ENTITY_SELECTION

uint2 GetFramebufferSize(const Framebuffer &framebuffer)
{
	const uint2 size = { framebuffer.extent.width, framebuffer.extent.height };
	return size;
}

Framebuffer GetDisplayFramebuffer(const Graphics &gfx)
{
	const u32 imageIndex = gfx.device.swapchain.currentImageIndex;
	const Framebuffer framebuffer = gfx.renderTargets.framebuffers[imageIndex];
	return framebuffer;
}

Framebuffer GetShadowmapFramebuffer(const Graphics &gfx)
{
	const Framebuffer &framebuffer = gfx.renderTargets.shadowmapFramebuffer;
	return framebuffer;
}


struct Plane
{
	float3 normal; // Orientation of the plane
	float3 point; // Point in the plane
	//float distance; // Distance from the origin to the nearest point in the plane
};

struct FrustumPlanes
{
	Plane planes[6];
};

FrustumPlanes FrustumPlanesFromCamera(float3 cameraPosition, float3 cameraForward, float zNear, float zFar, float fovy, float aspect)
{
	const float3 up = Float3(0.0f, 1.0f, 0.0f);
	const float3 cameraRight = Normalize(Cross(cameraForward, up));
	const float3 cameraUp = Normalize(Cross(cameraRight, cameraForward));

    const float farHalfY = zFar * Tan(0.5f * fovy * ToRadians);
    const float farHalfX = farHalfY * aspect;
	const float3 farRight = Mul(cameraRight, farHalfX);
	const float3 farLeft = Negate(farRight);
	const float3 farTop = Mul(cameraUp, farHalfY);
	const float3 farBot = Negate(farTop);
	const float3 farForward = Mul(cameraForward, zFar);

	const float3 topLeft = Add(Add(farForward, farLeft), farTop);
	const float3 topRight = Add(Add(farForward, farRight), farTop);
	const float3 botLeft = Add(Add(farForward, farLeft), farBot);
	const float3 botRight = Add(Add(farForward, farRight), farBot);

	const float3 topFaceNormal = Normalize(Cross(topLeft, topRight));
	const float3 botFaceNormal = Normalize(Cross(botRight, botLeft));
	const float3 rightFaceNormal = Normalize(Cross(topRight, botRight));
	const float3 leftFaceNormal = Normalize(Cross(botLeft, topLeft));
	const float3 farFaceNormal = Negate(cameraForward);
	const float3 nearFaceNormal = cameraForward;

	const float3 topFacePoint = cameraPosition;
	const float3 botFacePoint = cameraPosition;
	const float3 rightFacePoint = cameraPosition;
	const float3 leftFacePoint = cameraPosition;
	const float3 farFacePoint = Add(cameraPosition, Mul(cameraForward, zFar));
	const float3 nearFacePoint = Add(cameraPosition, Mul(cameraForward, zNear));

	const FrustumPlanes frustumPlanes = {
		.planes = {
			{ .normal = rightFaceNormal, .point = rightFacePoint },
			{ .normal = leftFaceNormal, .point = leftFacePoint },
			{ .normal = topFaceNormal, .point = topFacePoint },
			{ .normal = botFaceNormal, .point = botFacePoint },
			{ .normal = nearFaceNormal, .point = nearFacePoint },
			{ .normal = farFaceNormal, .point = farFacePoint },
		},
	};
    return frustumPlanes;
}


bool PointIsInFrontOfPlane(float3 point, const Plane &plane)
{
	const float3 dir = FromTo(plane.point, point);
	const float dotResult = Dot(plane.normal, dir);
	return dotResult >= 0.0f;
}

bool EntityIsInFrustum(const Entity &entity, const FrustumPlanes &frustum)
{
	// TODO: Calculate entity size properly
	const float x = 0.5f * entity.scale;
	const float y = 0.5f * entity.scale;
	const float z = 0.5f * entity.scale;

	const float3 bounds[] = {
		Add(entity.position, Float3(-x,-y,-z)),
		Add(entity.position, Float3( x,-y,-z)),
		Add(entity.position, Float3(-x, y,-z)),
		Add(entity.position, Float3( x, y,-z)),
		Add(entity.position, Float3(-x,-y, z)),
		Add(entity.position, Float3( x,-y, z)),
		Add(entity.position, Float3(-x, y, z)),
		Add(entity.position, Float3( x, y, z)),
	};
	bool entityIsBehindPlane = false;
	for (u32 j = 0; j < ARRAY_COUNT(frustum.planes); ++j)
	{
		entityIsBehindPlane = true;
		for (u32 i = 0; i < ARRAY_COUNT(bounds); ++i)
		{
			if (PointIsInFrontOfPlane(bounds[i], frustum.planes[j]))
			{
				entityIsBehindPlane = false;
				break;
			}
		}
		if (entityIsBehindPlane)
		{
			break;
		}
	}
	return !entityIsBehindPlane;
}


bool RenderGraphics(Graphics &gfx, Window &window, Arena &frameArena, f32 deltaSeconds)
{
	static f32 totalSeconds = 0.0f;
	totalSeconds += deltaSeconds;

	u32 frameIndex = gfx.device.currentFrame;

	BeginFrame(gfx.device);

	// We can query for previous (MAX_FRAMES_IN_FLIGHT before) frame fences
	static u32 frameCount = 0;
	if ( frameCount++ >= MAX_FRAMES_IN_FLIGHT )
	{
		const TimestampPool &timestampPool = gfx.timestampPools[frameIndex];
		const Timestamp t0 = ReadTimestamp(timestampPool, 0);
		const Timestamp t1 = ReadTimestamp(timestampPool, 1);
		ASSERT(t1.millis >= t0.millis);
		AddTimeSample(gfx.gpuFrameTimes, t1.millis - t0.millis);
	}

#if USE_UI
	UI_UploadVerticesToGPU(gfx.ui);
#endif

	// Display size
	const f32 displayWidth = static_cast<f32>(gfx.device.swapchain.extent.width);
	const f32 displayHeight = static_cast<f32>(gfx.device.swapchain.extent.height);

	// Calculate camera matrices
	const float preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
	ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
	const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
	const f32 ar = isLandscapeRotation ?  displayWidth / displayHeight : displayHeight / displayWidth;
	const float4x4 viewMatrix = ViewMatrixFromCamera(gfx.camera);
	const float fovy = 60.0f;
	const float znear = 0.1f;
	const float zfar = 1000.0f;
	const float4x4 viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
	//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
	const float4x4 perspectiveMatrix = Perspective(fovy, ar, znear, zfar);
	const float4x4 projectionMatrix = Mul(viewportRotationMatrix, perspectiveMatrix);

	// Frustum vectors
	const float hypotenuse  = znear / Cos( 0.5f * fovy * ToRadians );
	const float top = hypotenuse * Sin( 0.5f * fovy * ToRadians );
	const float bottom = -top;
	const float right = top * ar;
	const float left = -right;
	const float4 frustumTopLeft = Float4( Float3(left, top, -znear), 0.0f );
	const float4 frustumBottomRight = Float4( Float3(right, bottom, -znear), 0.0f );

	// CPU Frustum culling
	const float3 cameraPosition = gfx.camera.position;
	const float3 cameraForward = ForwardDirectionFromAngles(gfx.camera.orientation);
	const FrustumPlanes frustumPlanes = FrustumPlanesFromCamera(cameraPosition, cameraForward, znear, zfar, fovy, ar);
	for (u32 i = 0; i < gfx.entityCount; ++i)
	{
		Entity &entity = gfx.entities[i];
		entity.culled = !EntityIsInFrustum(entity, frustumPlanes);
	}

	// Sun matrices
	const float3 sunDirUnnormalized = Float3(-2.0f, 2.0f, 0.0f);
	const float4x4 sunRotationMatrix = Rotate(float3{0.0, 1.0, 0.0}, 180.0f);
	const float3 sunDir = Normalize(MulVector(sunRotationMatrix, sunDirUnnormalized));
	const float3 sunPos = Float3(0.0f, 0.0f, 0.0f);
	const float3 sunVrp = Sub(sunPos, sunDir);
	const float3 sunUp = Float3(0.0f, 1.0f, 0.0f);
	const float4x4 sunViewMatrix = LookAt(sunVrp, sunPos, sunUp);
	const float4x4 sunProjMatrix = Orthogonal(-5.0f, 5.0f, -10.0f, 5.0f, -5.0f, 10.0f);

	// Camera 2D
	const f32 l = 0.0f;
	const f32 r = displayWidth;
	const f32 t = 0.0f;
	const f32 b = displayHeight;
	const f32 n = 0.0f;
	const f32 f = 1.0f;
	const float4x4 camera2dProjection = Orthogonal(l, r, b, t, n, f);

	// Update globals struct
	const Globals globals = {
		.cameraView = viewMatrix,
		.cameraViewInv = Float4x4(Transpose(Float3x3(viewMatrix))),
		.cameraProj = projectionMatrix,
		.camera2dProj = camera2dProjection,
		.viewportRotationMatrix = viewportRotationMatrix,
		.cameraFrustumTopLeft = frustumTopLeft,
		.cameraFrustumBottomRight = frustumBottomRight,
		.sunView = sunViewMatrix,
		.sunProj = sunProjMatrix,
		.sunDir = Float4(sunDir, 0.0f),
		.eyePosition = Float4(gfx.camera.position, 1.0f),
		.shadowmapDepthBias = 0.005,
		.time = totalSeconds,
		.mousePosition = int2{window.mouse.x, window.mouse.y},
		.selectedEntity = gfx.selectedEntity,
	};

	// Update globals buffer
	Globals *globalsBufferPtr = (Globals*)GetBufferPtr(gfx.device, gfx.globalsBuffer[frameIndex]);
	*globalsBufferPtr = globals;

	// Update entity data
	SEntity *entities = (SEntity*)GetBufferPtr(gfx.device, gfx.entityBuffer[frameIndex]);
	for (u32 i = 0; i < gfx.entityCount; ++i)
	{
		const Entity &entity = gfx.entities[i];
		const float4x4 worldMatrix = Mul(Translate(entity.position), Scale(Float3(entity.scale))); // TODO: Apply also rotation
		entities[i].world = worldMatrix;
	}

	// Reset per-frame bind group allocators
	ResetBindGroupAllocator( gfx.device, gfx.dynamicBindGroupAllocator[frameIndex] );

	// Record commands
	CommandList commandList = BeginCommandList(gfx.device);

	// Timestamp
	ResetTimestampPool(commandList, gfx.timestampPools[frameIndex]);
	WriteTimestamp(commandList, gfx.timestampPools[frameIndex], PipelineStageTop);

	#if USE_COMPUTE_TEST
	{
		const Pipeline &pipeline = GetPipeline(gfx.device, gfx.computeClearH);

		SetPipeline(commandList, gfx.computeClearH);

		const BindGroupDesc bindGroupDesc = {
			.layout = pipeline.layout.bindGroupLayouts[0],
			.bindings = {
				{ .index = 0, .bufferView = gfx.computeBufferViewH },
			},
		};
		const BindGroup bindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

		SetBindGroup(commandList, 0, bindGroup);

		Dispatch(commandList, 1, 1, 1);
	}
	#endif // USE_COMPUTE_TEST

	const BufferH globalsBuffer = gfx.globalsBuffer[frameIndex];
	const BufferH entityBuffer = gfx.entityBuffer[frameIndex];
	const BufferH vertexBuffer = gfx.globalVertexArena.buffer;
	const BufferH indexBuffer = gfx.globalIndexArena.buffer;
	const SamplerH sampler = gfx.materialSamplerH;

	// Shadow map
	{
		BeginDebugGroup(commandList, "Shadow map");

		SetClearDepth(commandList, 0, 0.0f);

		const Framebuffer shadowmapFramebuffer = GetShadowmapFramebuffer(gfx);
		BeginRenderPass(commandList, shadowmapFramebuffer);

		const uint2 shadowmapSize = GetFramebufferSize(shadowmapFramebuffer);
		SetViewportAndScissor(commandList, shadowmapSize);

		SetPipeline(commandList, gfx.shadowmapPipelineH);

		SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

		for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
		{
			const Entity &entity = gfx.entities[entityIndex];

			if ( !entity.visible ) continue;

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
			const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
		}

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

	const Format depthFormat = gfx.device.defaultDepthFormat;
	const ImageH shadowmapImage = gfx.renderTargets.shadowmapImage;
	TransitionImageLayout(commandList, shadowmapImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

	// Scene and UI
	{
		BeginDebugGroup(commandList, "Scene and UI");

		SetClearColor(commandList, 0, { 0.0f, 0.0f, 0.0f, 0.0f } );
		SetClearDepth(commandList, 1, 0.0f);

		const Framebuffer displayFramebuffer = GetDisplayFramebuffer(gfx);
		BeginRenderPass(commandList, displayFramebuffer);

		const uint2 displaySize = GetFramebufferSize(displayFramebuffer);
		SetViewportAndScissor(commandList, displaySize);

		for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
		{
			const Entity &entity = gfx.entities[entityIndex];

			if ( !entity.visible || entity.culled ) continue;

			const u32 materialIndex = entity.materialIndex;
			const Material &material = gfx.materials[materialIndex];

			BeginDebugGroup(commandList, material.name);

			// Pipeline
			SetPipeline(commandList, material.pipelineH);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[materialIndex]);

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
			const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);

			EndDebugGroup(commandList);
		}

		{ // Sky
			const Texture &texture = GetTexture(gfx, gfx.skyTextureH);
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.skyPipelineH);
			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/2; // div 2 (2 bytes per index)
			const uint32_t firstIndex = indices.offset/2; // div 2 (2 bytes per index)
			const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .sampler = gfx.skySamplerH },
					{ .index = 1, .image = texture.image },
				},
			};
			const BindGroup bindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			BeginDebugGroup(commandList, "sky");

			SetPipeline(commandList, gfx.skyPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, bindGroup);
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			EndDebugGroup(commandList);
		}

#if USE_UI
		{ // GUI
			const UI &ui = gfx.ui;

			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.guiPipelineH);
			const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[3];

			BeginDebugGroup(commandList, "GUI");

			SetPipeline(commandList, gfx.guiPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetVertexBuffer(commandList, UI_GetVertexBuffer(ui));

			for (u32 i = 0; i < UI_DrawListCount(ui); ++i)
			{
				const UIDrawList &drawList = UI_GetDrawListAt(ui, i);
				SetScissor(commandList, drawList.scissorRect);

				const BindGroupDesc bindGroupDesc = {
					.layout = bindGroupLayout,
					.bindings = {
						{ .index = 0, .sampler = gfx.skySamplerH },
						{ .index = 1, .image = drawList.imageHandle },
					},
				};
				const BindGroup textureBindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);
				SetBindGroup(commandList, 3, textureBindGroup);

				for (u32 i = 0; i < drawList.vertexRangeCount; ++i)
				{
					const UIVertexRange &range = drawList.vertexRanges[i];
					Draw(commandList, range.count, range.index);
				}
			}

			EndDebugGroup(commandList);
		}
#endif

#if USE_IMGUI
		BeginDebugGroup(commandList, "ImGui");

		// Record dear imgui primitives into command buffer
		ImDrawData* draw_data = ImGui::GetDrawData();
		ImGui_ImplVulkan_RenderDrawData(draw_data, commandList.handle);

		EndDebugGroup(commandList);
#endif

		EndRenderPass(commandList);

		EndDebugGroup(commandList);
	}

#if USE_ENTITY_SELECTION
	if ( gfx.selectEntity )
	{ // Selection buffer
		BeginDebugGroup(commandList, "Entity selection");

		{
			SetClearColor(commandList, 0, U32_MAX);

			BeginRenderPass(commandList, gfx.renderTargets.idFramebuffer );

			const uint2 framebufferSize = GetFramebufferSize(gfx.renderTargets.idFramebuffer);
			SetViewportAndScissor(commandList, framebufferSize);

			SetPipeline(commandList, gfx.idPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 entityIndex = 0; entityIndex < gfx.entityCount; ++entityIndex)
			{
				const Entity &entity = gfx.entities[entityIndex];

				if ( !entity.visible || entity.culled ) continue;

				// Draw!!!
				const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
				const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
				const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
			}

			EndRenderPass(commandList);
		}

		{
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.computeSelectH);

			SetPipeline(commandList, gfx.computeSelectH);

			const BindGroupDesc bindGroupDesc = {
				.layout = pipeline.layout.bindGroupLayouts[3],
				.bindings = {
					{ .index = 0, .bufferView = gfx.selectionBufferViewH },
					{ .index = 1, .image = gfx.renderTargets.idImage },
				},
			};
			const BindGroup dynamicBindGroup = CreateBindGroup(gfx.device, bindGroupDesc, gfx.dynamicBindGroupAllocator[frameIndex]);

			SetBindGroup(commandList, 0, dynamicBindGroup);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 3, dynamicBindGroup);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateRenderTarget, ImageStateShaderInput, 0, 1);

			Dispatch(commandList, 1, 1, 1);

			TransitionImageLayout(commandList, gfx.renderTargets.idImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);
		}

		EndDebugGroup(commandList);
	}
#endif // USE_ENTITY_SELECTION


	TransitionImageLayout(commandList, shadowmapImage, ImageStateShaderInput, ImageStateRenderTarget, 0, 1);

	WriteTimestamp(commandList, gfx.timestampPools[frameIndex], PipelineStageBottom);

	EndCommandList(commandList);

	SubmitResult submitRes = Submit(gfx.device, commandList);

	if ( !Present(gfx.device, submitRes) ) {
		return false;
	}

	// TODO: Check if this should be executed even if Present failed...
	EndFrame(gfx.device);

	return true;
}

void HotReloadAssets(Graphics &gfx, Arena scratch)
{
	if ( gfx.reloadQueueSize > 0 )
	{
		WaitDeviceIdle(gfx.device);

		for (u32 i = 0; i < gfx.reloadQueueSize; ++i)
		{
			const u32 pipelineIndex = gfx.reloadQueue[i];
			const PipelineH pipelineH = pipelineHandles[pipelineIndex];
			const PipelineDesc &pipelineDesc = pipelineDescs[pipelineIndex];

			DestroyPipeline(gfx.device, pipelineH);

			const RenderPassH renderPassH = FindRenderPassHandle(gfx, pipelineDesc.renderPass);
			pipelineHandles[pipelineIndex] = CreateGraphicsPipeline(gfx.device, scratch, pipelineDesc, renderPassH, gfx.globalBindGroupLayout);
		}

		gfx.reloadQueueSize = 0;

		LinkPipelineHandles(gfx);
	}
}



#if USE_IMGUI
void InitializeImGuiContext()
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.FontGlobalScale = 1.0f;
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

	const RenderPass &renderPass = GetRenderPass(gfx.device, gfx.litRenderPassH);

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

	static bool open = true;
	ImGui::ShowDemoWindow(&open);

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

	const TimestampPool &timestampPool = gfx.timestampPools[gfx.device.currentFrame];
	const Timestamp t0 = ReadTimestamp(timestampPool, 0);
	const Timestamp t1 = ReadTimestamp(timestampPool, 1);
	ImGui::Text("Application graphics frame time (ms): %f", t1.millis - t0.millis);

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

#if USE_UI

void UpdateUI(UI &ui, Platform &platform)
{
	const Window &window = platform.window;
	Graphics &gfx = GetPlatformGraphics(platform);

	UI_SetInputState(ui, window.keyboard, window.mouse, window.chars);
	UI_SetViewportSize(ui, uint2{window.width, window.height});

	UI_BeginFrame(ui);

	char text[MAX_PATH_LENGTH];
	SPrintf(text, "Debug UI");

	UI_BeginWindow(ui, text);

	if ( UI_Section(ui, "Profiling" ) )
	{
		constexpr f32 maxExpectedMillis = 1000.0f / 60.0f; // Like expecting to reach 60 fps

		const u32 sceneWidth = gfx.device.swapchain.extent.width;
		const u32 sceneHeight = gfx.device.swapchain.extent.height;
		SPrintf(text, "Resolution: %ux%u px", sceneWidth, sceneHeight);
		UI_Label(ui, text);

		const TimeSamples &gpuTimes = gfx.gpuFrameTimes;
		SPrintf(text, "GPU %.02f ms / %.00f fps ", gpuTimes.average, 1000.0f / gpuTimes.average);
		UI_Label(ui, text);
		UI_Histogram(ui, gpuTimes.samples, ARRAY_COUNT(gpuTimes.samples), maxExpectedMillis);

		const TimeSamples &cpuTimes = gfx.cpuFrameTimes;
		SPrintf(text, "CPU %.02f ms / %.00f fps ", cpuTimes.average, 1000.0f / cpuTimes.average);
		UI_Label(ui, text);
		UI_Histogram(ui, cpuTimes.samples, ARRAY_COUNT(cpuTimes.samples), maxExpectedMillis + 1.0f);
	}

	if ( UI_Section(ui, "Entities") )
	{
		if ( gfx.selectedEntity == U32_MAX ) {
			SPrintf(text, "Selected entity: <none>");
		} else {
			SPrintf(text, "Selected entity: %u", gfx.selectedEntity);
		}
		UI_Label(ui, text);
	}

	if ( UI_Section(ui, "Pipelines") )
	{
		if ( UI_Button(ui, "Recompile shaders") )
		{
#if PLATFORM_WINDOWS
			constexpr const char *dxc = "dxc/windows/bin/x64/dxc.exe";
#elif PLATFORM_LINUX
			constexpr const char *dxc = "dxc/linux/bin/dxc";
#else
			constexpr const char *dxc = "<none>";
#endif
			constexpr const char *flags = "-spirv -O3";

			for (u32 i = 0; i < ARRAY_COUNT(shaderSources); ++i)
			{
				const ShaderSourceDesc &desc = shaderSources[i];
				const char *target =
					desc.type == ShaderTypeVertex ? "vs_6_7" :
					desc.type == ShaderTypeFragment ? "ps_6_7" :
					desc.type == ShaderTypeCompute ? "cs_6_7" :
					"unknown";
				const char *entry = desc.entryPoint;
				const char *output = desc.outputName;
				const char *filename = desc.filename;
				SPrintf(text, "%s %s -T %s -E %s -Fo shaders/%s.spv -Fc shaders/%s.dis shaders/%s", dxc, flags, target, entry, output, output, filename );

				ExecuteProcess(text);
			}
		}

		for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
		{
			const char *pipelineName = pipelineDescs[i].name;

			UI_BeginLayout(ui, UiLayoutHorizontal);
			if ( UI_Button(ui, "Reload") )
			{
				ASSERT(gfx.reloadQueueSize < ARRAY_COUNT(gfx.reloadQueue));
				gfx.reloadQueue[gfx.reloadQueueSize++] = i;
			}
			UI_Label(ui, pipelineName);
			UI_EndLayout(ui);
		}
	}

	if ( UI_Section(ui, "Buttons") )
	{
		static u32 labelIndex = 0;
		const char *labels[] = {"Label 1", "Label 2"};
		const char *label = labels[labelIndex];

		UI_Label(ui, label);

		UI_Separator(ui);

		UI_BeginLayout(ui, UiLayoutHorizontal);

		if ( UI_Button(ui, "Hola") )
		{
			labelIndex = (labelIndex + 1) % ARRAY_COUNT(labels);
		}

		UI_Button(ui, "Adios");

		UI_Button(ui, "Memory");

		UI_EndLayout(ui);

	}

	if ( UI_Section(ui, "Radio/Check") )
	{
		const char *radioOptions[] = { "Option 1", "Option 2", "Option 3" };
		static int selectedOption = 0;

		for (u32 i = 0; i < ARRAY_COUNT(radioOptions); ++i)
		{
			if ( UI_Radio(ui, radioOptions[i], selectedOption == i) )
			{
				selectedOption = i;
			}
		}

		UI_Separator(ui);

		static const char *checkOptions[] = { "Check 1", "Check 2", "Check 3" };
		static bool checkSelections[ARRAY_COUNT(checkOptions)] = {};

		for (u32 i = 0; i < ARRAY_COUNT(checkOptions); ++i)
		{
			UI_Checkbox(ui, checkOptions[i], &checkSelections[i]);
		}

		UI_Separator(ui);

		static const char *comboOptions[] = { "Combo 1", "Combo 2", "Combo 3" };
		static u32 comboIndex = 0;

		UI_Combo(ui, "Select type", comboOptions, ARRAY_COUNT(comboOptions), &comboIndex);
	}

	if ( UI_Section(ui, "Images") )
	{
		UI_BeginLayout(ui, UiLayoutHorizontal);
		for (u32 i = 0; i < gfx.textureCount; ++i)
		{
			UI_Image(ui, gfx.textures[i].image);
		}
		UI_EndLayout(ui);
	}

	if ( UI_Section(ui, "Inputs") )
	{
		static char text[128] = "name";
		static i32 intNumber = 0;
		static f32 floatNumber = 0.0f;

		UI_InputText(ui, "Input text", text, ARRAY_COUNT(text));
		UI_InputInt(ui, "Input int", &intNumber);
		UI_InputFloat(ui, "Input float", &floatNumber);
	}

	UI_EndWindow(ui);

	UI_EndFrame(ui);
}
#endif


#define USE_DYNAMIC_LIB 1

#if USE_DYNAMIC_LIB

static DynamicLibrary dynamicLib = {};

typedef void (*PFinit)();
typedef void (*PFupdate)();
typedef void (*PFfinalize)();

PFinit initialize;
PFupdate update;
PFfinalize finalize;

#endif // USE_DYNAMIC_LIB




bool EngineInit(Platform &platform)
{
	gStringInterning = &platform.stringInterning;

	SetGraphicsStringInterning(gStringInterning);

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

#if USE_DYNAMIC_LIB

#if PLATFORM_LINUX
	constexpr const char * dynamicLibName = "./game.so";
#elif PLATFORM_WINDOWS
	constexpr const char *dynamicLibName = "./build/game.dll";
#endif

	dynamicLib = OpenLibrary(dynamicLibName);
	if (dynamicLib)
	{
		initialize = (PFinit)LoadSymbol(dynamicLib, "initialize");
		update = (PFinit)LoadSymbol(dynamicLib, "update");
		finalize = (PFfinalize)LoadSymbol(dynamicLib, "finalize");

		if (initialize)
		{
			initialize();
		}
		else
		{
			LOG(Warning, "initialize not loaded :-(\n");
		}
		if (update)
		{
			update();
		}
		else
		{
			LOG(Warning, "update not loaded :-(\n");
		}
		if (finalize)
		{
			finalize();
		}
		else
		{
			LOG(Warning, "finalize not loaded :-(\n");
		}

		if (initialize == nullptr && update == nullptr && finalize == nullptr)
		{
			CloseLibrary(dynamicLib);
		}
	}
	else
	{
		LOG(Warning, "Error opening %s\n", dynamicLibName);
	}

#endif // USE_DYNAMIC_LIB

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

	const f32 cpuDeltaMillis = platform.deltaSeconds * 1000.0f;
	AddTimeSample( gfx.cpuFrameTimes, cpuDeltaMillis );

	if ( gfx.device.swapchain.outdated || platform.window.flags & WindowFlags_WasResized )
	{
		WaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		DestroySwapchain(gfx.device, gfx.device.swapchain);
		if ( platform.window.width != 0 && platform.window.height != 0 )
		{
			gfx.device.swapchain = CreateSwapchain(gfx.device, platform.window, gfx.device.swapchainInfo);
			gfx.renderTargets = CreateRenderTargets(gfx);
			UpdateGlobalBindGroups(gfx);
		}
		gfx.device.swapchain.outdated = false;
	}

	if ( gfx.deviceInitialized && gfx.device.swapchain.handle != VK_NULL_HANDLE )
	{
#if USE_IMGUI
		UpdateImGui(gfx);
#endif

#if USE_UI
		UpdateUI(gfx.ui, platform);
		const bool handleInput = !gfx.ui.wantsInput;
#else
		constexpr bool handleInput = true;
#endif

#if USE_CAMERA_MOVEMENT
		AnimateCamera(platform.window, gfx.camera, platform.deltaSeconds, handleInput);
#endif
#if USE_ENTITY_SELECTION
		BeginEntitySelection(gfx, platform.window.mouse, handleInput);
#endif

		RenderGraphics(gfx, platform.window, platform.frameArena, platform.deltaSeconds);

#if USE_ENTITY_SELECTION
		EndEntitySelection(gfx);
#endif

		HotReloadAssets(gfx, platform.frameArena);
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
#if USE_DYNAMIC_LIB
	if (dynamicLib)
	{
		if (finalize)
		{
			finalize();
		}

		CloseLibrary(dynamicLib);
		dynamicLib = nullptr;
		initialize = nullptr;
		update = nullptr;
		finalize = nullptr;
	}
#endif // USE_DYNAMIC_LIB

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
// - [ ] GPU culling: Modify the compute to perform frustum culling and save the result in the buffer.
// - [ ] Text rendering
// - [ ] Include directive in the C AST parsing code.
//
// DONE:
// - [X] Avoid using push constants and put transformation matrices in buffers instead.
// - [X] Investigate how to write descriptors in a more elegant manner (avoid hardcoding).
// - [X] Put all the geometry in the same buffer.
// - [X] GPU culling: Add a "hello world" compute shader that writes some numbers into a buffer.
// - [X] GPU time queries
// - [X] GPU culling: As a first step, perform frustum culling in the CPU.
// - [X] Avoid duplicated global descriptor sets.
// - [X] Have a single descripor set for global info that only changes once per frame

