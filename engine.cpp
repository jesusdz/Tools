#define TOOLS_PLATFORM
#include "tools.h"

#include "tools_gfx.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#define USE_EDITOR ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_UI ( PLATFORM_LINUX || PLATFORM_WINDOWS )
#define USE_DATA_BUILD ( PLATFORM_LINUX || PLATFORM_WINDOWS )

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

// Engine API
#include "engine.h"




#define MAX_TEXTURES 8 
#define MAX_MATERIALS 4
#define MAX_ENTITIES 32
#define MAX_GLOBAL_BINDINGS 8
#define MAX_MATERIAL_BINDINGS 4
#define MAX_COMPUTE_BINDINGS 4

#define INVALID_HANDLE -1

#define LOAD_FROM_SOURCE_FILES 0




struct MaterialDesc
{
	const char *name;
	const char *textureName;
	const char *pipelineName;
	float uvScale;
};

enum GeometryType
{
	GeometryTypeCube,
	GeometryTypePlane,
	GeometryTypeScreen,
};

struct EntityDesc
{
	const char *name;
	const char *materialName;
	float3 pos;
	float scale;
	GeometryType geometryType;
};

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

struct TextureH
{
	u16 gen;
	u16 idx;
};

constexpr TextureH InvalidTextureH = {};

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

enum ProjectionType
{
	ProjectionPerspective,
	ProjectionOrthographic,
	ProjectionTypeCount,
};

struct Camera
{
	ProjectionType projectionType;
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

enum EndFrameCommandType
{
	EndFrameCommandReloadGraphicsPipeline,
	EndFrameCommandRemoveTexture,
	EndFrameCommandCount,
};

struct EndFrameCommand
{
	EndFrameCommandType type;
	union
	{
		u32 pipelineIndex;
		TextureH textureH;
	};
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
#if USE_EDITOR
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
	TextureH textureHandles[MAX_TEXTURES];
	u16 textureIndices[MAX_TEXTURES]; // First all used indices, then all free indices
	u16 textureCount;

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
	bool shouldUpdateGlobalBindGroups;

	// Updated once at the beginning for each material
	BindGroup materialBindGroups[MAX_MATERIALS];
	bool shouldUpdateMaterialBindGroups;

	TimestampPool timestampPools[MAX_FRAMES_IN_FLIGHT];

	TextureH skyTextureH;

	ImageH pinkImageH;

	PipelineH shadowmapPipelineH;
	PipelineH skyPipelineH;
	PipelineH guiPipelineH;
#if USE_EDITOR
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

	EndFrameCommand endFrameCommands[128];
	u32 endFrameCommandCount;
};

// Editor API
#if USE_EDITOR
#include "editor.h"
#endif

typedef void (*GameInitAPIFunction)(const EngineAPI &api);
typedef void (*GameStartFunction)();
typedef void (*GameUpdateFunction)();
typedef void (*GameStopFunction)();

enum GameState
{
	GameStateStopped,
	GameStateStarting,
	GameStateRunning,
	GameStateStopping,
	GameStateCount,
};

struct Game
{
	DynamicLibrary library;

	GameInitAPIFunction InitAPI;
	GameStartFunction Start;
	GameUpdateFunction Update;
	GameStopFunction Stop;

	GameState state;
	bool shouldRecompile;
};

struct Scene
{
	Entity entities[MAX_ENTITIES];
	u32 entityCount;
};

enum ShaderType
{
	ShaderTypeVertex,
	ShaderTypeFragment,
	ShaderTypeCompute
};

#pragma pack(push, 1)
struct TextureDesc
{
	const char name[32];
	const char filename[32];
	int mipmap;
};

struct ShaderSourceDesc
{
	ShaderType type;
	const char filename[32];
	const char entryPoint[32];
	const char name[32];
};

struct ShaderAndPipelineDesc
{
	const char *vsName;
	const char *fsName;
	const char *renderPass;
	PipelineDesc desc;
};

struct ShaderAndComputeDesc
{
	const char *csName;
	ComputeDesc desc;
};

struct ShaderHeader
{
	ShaderSourceDesc desc;
	u32 spirvOffset;
	u32 spirvSize;
};

struct ImageHeader
{
	TextureDesc desc;
	u16 width;
	u16 height;
	u8  channels;
	u32 pixelsOffset;
	u32 pixelsSize;
};

struct DataHeader
{
	u32 magicNumber;
	u32 shadersOffset;
	u32 shaderCount;
	u32 imagesOffset;
	u32 imageCount;
};
#pragma pack(pop)

struct LoadedShader
{
	ShaderHeader *header;
	byte *spirv;
};

struct LoadedImage
{
	ImageHeader *header;
	byte *pixels;
};

struct LoadedData
{
	DataHeader *header;
	LoadedShader *shaders;
	LoadedImage *images;
};


struct Engine
{
	Platform platform;
	Graphics gfx;
	Game game;
#if USE_UI
	UI ui;
#endif

	Scene scene;

	Editor editor;

	LoadedData data;
};

static const TextureDesc textures[] =
{
	{ .name = "tex_diamond", .filename = "assets/diamond.png", .mipmap = 1 },
	{ .name = "tex_dirt",    .filename = "assets/dirt.jpg",    .mipmap = 1 },
	{ .name = "tex_grass",   .filename = "assets/grass.jpg",   .mipmap = 1 },
	{ .name = "tex_sky",     .filename = "assets/sky01.png" },
};

static const MaterialDesc materials[] =
{
	{ .name = "mat_diamond", .textureName = "tex_diamond", .pipelineName = "pipeline_shading", .uvScale = 1.0f },
	{ .name = "mat_dirt",    .textureName = "tex_dirt",    .pipelineName = "pipeline_shading", .uvScale = 1.0f },
	{ .name = "mat_grass",   .textureName = "tex_grass",   .pipelineName = "pipeline_shading", .uvScale = 11.0f },
};

static const EntityDesc entities[] =
{
	{ .name = "ent_cube0", .materialName = "mat_diamond", .pos = { 1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube1", .materialName = "mat_diamond", .pos = { 1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube2", .materialName = "mat_dirt",    .pos = {-1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube3", .materialName = "mat_dirt",    .pos = {-1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_plane", .materialName = "mat_grass",   .pos = { 0, -0.5, 0}, .scale = 11, .geometryType = GeometryTypePlane},
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

static const ShaderSourceDesc shaderSources[] = {
	{ .type = ShaderTypeVertex,   .filename = "shading.hlsl",        .entryPoint = "VSMain",      .name = "vs_shading" },
	{ .type = ShaderTypeFragment, .filename = "shading.hlsl",        .entryPoint = "PSMain",      .name = "fs_shading" },
	{ .type = ShaderTypeVertex,   .filename = "sky.hlsl",            .entryPoint = "VSMain",      .name = "vs_sky" },
	{ .type = ShaderTypeFragment, .filename = "sky.hlsl",            .entryPoint = "PSMain",      .name = "fs_sky" },
	{ .type = ShaderTypeVertex,   .filename = "shadowmap.hlsl",      .entryPoint = "VSMain",      .name = "vs_shadowmap" },
	{ .type = ShaderTypeFragment, .filename = "shadowmap.hlsl",      .entryPoint = "PSMain",      .name = "fs_shadowmap" },
	{ .type = ShaderTypeVertex,   .filename = "ui.hlsl",             .entryPoint = "VSMain",      .name = "vs_ui" },
	{ .type = ShaderTypeFragment, .filename = "ui.hlsl",             .entryPoint = "PSMain",      .name = "fs_ui" },
	{ .type = ShaderTypeVertex,   .filename = "id.hlsl",             .entryPoint = "VSMain",      .name = "vs_id" },
	{ .type = ShaderTypeFragment, .filename = "id.hlsl",             .entryPoint = "PSMain",      .name = "fs_id" },
	{ .type = ShaderTypeCompute,  .filename = "compute_select.hlsl", .entryPoint = "CSMain",      .name = "compute_select" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_clear",  .name = "compute_clear" },
	{ .type = ShaderTypeCompute,  .filename = "compute.hlsl",        .entryPoint = "main_update", .name = "compute_update" },
};

static const ShaderAndPipelineDesc pipelineDescs[] =
{
	{
		.vsName = "vs_shading",
		.fsName = "fs_shading",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_shading",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
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
		}
	},
	{
		.vsName = "vs_shadowmap",
		.fsName = "fs_shadowmap",
		.renderPass = "shadowmap_renderpass",
		.desc = {
			.name = "pipeline_shadowmap",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
			.vertexBufferCount = 1,
			.vertexBuffers = { { .stride = 32 }, },
			.vertexAttributeCount = 1,
			.vertexAttributes = {
				{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			},
			.depthTest = true,
			.depthWrite = true,
			.depthCompareOp = CompareOpGreater,
		}
	},
	{
		.vsName = "vs_sky",
		.fsName = "fs_sky",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_sky",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
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
		}
	},
	{
		.vsName = "vs_ui",
		.fsName = "fs_ui",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_ui",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
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
		}
	},
#if USE_EDITOR
	{
		.vsName = "vs_id",
		.fsName = "fs_id",
		.renderPass = "id_renderpass",
		.desc = {
			.name = "pipeline_id",
			.vsFunction = "VSMain",
			.fsFunction = "PSMain",
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
		}
	},
#endif // USE_EDITOR
};

static PipelineH pipelineHandles[ARRAY_COUNT(pipelineDescs)] = {};

static const ShaderAndComputeDesc computeDescs[] =
{
	//{ .name = "compute_clear", .filename = "shaders/compute_clear.spv", .function = "main_clear" },
	//{ .name = "compute_update", .filename = "shaders/compute_update.spv", .function = "main_update" },
	{
		.csName = "compute_select",
		.desc = {
			.name = "compute_select",
			.function = "CSMain"
		},
	},
};

static PipelineH computeHandles[ARRAY_COUNT(computeDescs)] = {};

static StringInterning *gStringInterning;

u32 U32FromChars(char a, char b, char c, char d)
{
	const u32 res =
		(u32)a << 0 |
		(u32)b << 8 |
		(u32)c << 16 |
		(u32)d << 24 ;
	return res;
}

Engine &GetEngine(Platform &platform)
{
	Engine *engine = (Engine*)platform.userData;
	ASSERT(engine != NULL);
	return *engine;
}

Window &GetWindow(Engine &engine)
{
	return engine.platform.window;
}

#if USE_UI
UI &GetUI(Engine &engine)
{
	return engine.ui;
}
#endif

void CompileShaders()
{
	char text[MAX_PATH_LENGTH];

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
		const char *output = desc.name;
		const char *filename = desc.filename;
		SPrintf(text,
			"%s/%s "
			"%s -T %s -E %s "
			"-Fo %s/shaders/%s.spv -Fc %s/shaders/%s.dis "
			"%s/shaders/%s",
			ProjectDir, dxc,
			flags, target, entry,
			DataDir, output, DataDir, output,
			ProjectDir, filename );
		LOG(Debug, "%s\n", text);
		ExecuteProcess(text);
	}
}

void CopyTextures()
{
	char text[MAX_PATH_LENGTH];

	for (u32 i = 0; i < ARRAY_COUNT(textures); ++i)
	{
		const FilePath srcPath = MakePath(ProjectDir, textures[i].filename);
		const FilePath dstPath = MakePath(DataDir, textures[i].filename);
		LOG(Debug, "CopyFile %s to %s\n", srcPath.str, dstPath.str);
		CopyFile(srcPath.str, dstPath.str);
	}
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
		if ( StrEq(pipelineDescs[i].desc.name, name) ) {
			return pipelineHandles[i];
		}
	}
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i) {
		if ( StrEq(computeDescs[i].desc.name, name) ) {
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

inline bool operator==(TextureH a, TextureH b)
{
	const bool equal = ( a.idx == b.idx ) && ( a.gen == b.gen );
	return equal;
}

bool IsValidHandle(Graphics &gfx, TextureH handle)
{
	TextureH storedHandle = gfx.textureHandles[handle.idx];
	const bool valid = handle == storedHandle;
	return valid;
}

TextureH NewTextureHandle(Graphics &gfx)
{
	ASSERT(gfx.textureCount < MAX_TEXTURES);
	u16 index = gfx.textureIndices[gfx.textureCount++];
	TextureH &handle = gfx.textureHandles[index];
	ASSERT(handle.idx == index);
	return handle;
}

void FreeTextureHandle(Graphics &gfx, TextureH handle)
{
	ASSERT(IsValidHandle(gfx, handle));
	gfx.textureHandles[handle.idx].gen++;

	ASSERT(gfx.textureCount > 0);
	gfx.textureCount--;
	bool found = false;
	for (u16 i = 0; i < gfx.textureCount; ++i) {
		found = found || handle.idx == gfx.textureIndices[i];
		if (found) { // compact indices from found index onwards
			gfx.textureIndices[i] = gfx.textureIndices[i+1];
		}
	}
	// Insert the freed index at the end
	gfx.textureIndices[gfx.textureCount] = handle.idx;

}

#if LOAD_FROM_SOURCE_FILES
TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	int texWidth, texHeight, texChannels;
	FilePath imagePath = MakePath(ProjectDir, desc.filename);
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

	const TextureH textureHandle = NewTextureHandle(gfx);
	gfx.textures[textureHandle].name = desc.name;
	gfx.textures[textureHandle].image = imageHandle;

	if ( originalPixels )
	{
		stbi_image_free(originalPixels);
	}

	return textureHandle;
}
#endif

Texture &GetTexture(Graphics &gfx, TextureH handle)
{
	ASSERT( IsValidHandle(gfx, handle) );
	Texture &texture = gfx.textures[handle.idx];
	return texture;
}

TextureH CreateTexture(Graphics &gfx, const LoadedImage &image)
{
	const TextureDesc &desc = image.header->desc;
	const u32 width = image.header->width;
	const u32 height = image.header->height;
	const u32 channels = image.header->channels;
	const u8 *pixels = image.pixels;

	const ImageH imageHandle = CreateImage(gfx, desc.name, width, height, channels, desc.mipmap, pixels);

	const TextureH textureHandle = NewTextureHandle(gfx);
	Texture &texture = GetTexture(gfx, textureHandle);
	texture.name = desc.name;
	texture.image = imageHandle;

	return textureHandle;
}

ImageH GetTextureImage(Graphics &gfx, TextureH textureH, ImageH imageH)
{
	ImageH res = imageH;

	if ( IsValidHandle(gfx, textureH) ) {
		const Texture &texture = GetTexture(gfx, textureH);
		res = texture.image;
	}

	return res;
}

TextureH FindTextureHandle(Graphics &gfx, const char *name)
{
	for (u32 i = 0; i < gfx.textureCount; ++i)
	{
		u16 index = gfx.textureIndices[i];
		TextureH handle = gfx.textureHandles[index];
		const Texture &texture = GetTexture(gfx, handle);
		if ( StrEq(texture.name, name) ) {
			return handle;
		}
	}
	LOG(Warning, "Could not find texture <%s> handle.\n", name);
	INVALID_CODE_PATH();
	return InvalidTextureH;
}

void RemoveTexture(Graphics &gfx, TextureH textureH)
{
	ASSERT(IsValidHandle(gfx, textureH));

	Texture &texture = GetTexture(gfx, textureH);
	DestroyImageH(gfx.device, texture.image);
	texture = {};

	FreeTextureHandle(gfx, textureH);

	gfx.shouldUpdateMaterialBindGroups = true;
}

MaterialH CreateMaterial( Graphics &gfx, const MaterialDesc &desc)
{
	TextureH textureHandle = FindTextureHandle(gfx, desc.textureName);
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

void CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Scene &scene = engine.scene;

	ASSERT ( scene.entityCount < MAX_ENTITIES );
	if ( scene.entityCount < MAX_ENTITIES )
	{
		BufferChunk vertices = GetVerticesForGeometryType(engine.gfx, desc.geometryType);
		BufferChunk indices = GetIndicesForGeometryType(engine.gfx, desc.geometryType);

		const u32 entityIndex = scene.entityCount++;
		scene.entities[entityIndex].name = desc.name;
		scene.entities[entityIndex].visible = true;
		scene.entities[entityIndex].position = desc.pos;
		scene.entities[entityIndex].scale = desc.scale;
		scene.entities[entityIndex].vertices = vertices;
		scene.entities[entityIndex].indices = indices;
		scene.entities[entityIndex].materialIndex = FindMaterialHandle(engine.gfx, desc.materialName);
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

#if LOAD_FROM_SOURCE_FILES
static ShaderSource GetShaderSource(Arena &arena, const char *shaderName)
{
	char filename[MAX_PATH_LENGTH];
	SPrintf(filename, "shaders/%s.spv", shaderName);
	FilePath shaderPath = MakePath(DataDir, filename);
	DataChunk *chunk = PushFile( arena, shaderPath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open shader file %s\n", shaderPath.str );
		QUIT_ABNORMALLY();
	}
	ShaderSource shaderSource = { chunk->bytes, chunk->size };
	return shaderSource;
}
#else
static ShaderSource GetShaderSource(LoadedData &data, const char *shaderName)
{
	byte *bytes = 0;
	u32 size = 0;
	for (u32 i = 0; i < data.header->shaderCount; ++i)
	{
		LoadedShader &loadedShader = data.shaders[i];
		if ( StrEq( loadedShader.header->desc.name, shaderName) )
		{
			bytes = loadedShader.spirv;
			size = loadedShader.header->spirvSize;
		}
	}
	if ( !bytes ) {
		LOG( Error, "Could not find shader in data: %s\n", shaderName );
		LOG( Error, "Shaders in data:\n");
		for (u32 i = 0; i < data.header->shaderCount; ++i)
		{
			LoadedShader &loadedShader = data.shaders[i];
			LOG( Error, "- %s\n", loadedShader.header->desc.name);
		}
		QUIT_ABNORMALLY();
	}
	const ShaderSource shaderSource = { .data = bytes, .dataSize = size };
	return shaderSource;
}
#endif

static void CompileGraphicsPipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	const RenderPassH renderPassH = FindRenderPassHandle(gfx, pipelineDescs[pipelineIndex].renderPass);

	PipelineDesc desc = pipelineDescs[pipelineIndex].desc;
	desc.renderPass = GetRenderPass(gfx.device, renderPassH);
#if LOAD_FROM_SOURCE_FILES
	desc.vertexShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].vsName);
	desc.fragmentShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].fsName);
#else
	desc.vertexShaderSource = GetShaderSource(engine.data, pipelineDescs[pipelineIndex].vsName);
	desc.fragmentShaderSource = GetShaderSource(engine.data, pipelineDescs[pipelineIndex].fsName);
#endif

	LOG(Info, "Creating Graphics Pipeline: %s\n", desc.name);
	PipelineH pipelineH = pipelineHandles[pipelineIndex];
	if ( IsValid(pipelineH) ) {
		DestroyPipeline( gfx.device, pipelineH );
	}
	pipelineH = CreateGraphicsPipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectName(gfx.device, pipelineH, desc.name);
	pipelineHandles[pipelineIndex] = pipelineH;
}

static void LoadData(Engine &engine);

static void CompileComputePipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	ComputeDesc desc = computeDescs[pipelineIndex].desc;
#if LOAD_FROM_SOURCE_FILES
	desc.computeShaderSource = GetShaderSource(scratch, computeDescs[pipelineIndex].csName);
#else
	desc.computeShaderSource = GetShaderSource(engine.data, computeDescs[pipelineIndex].csName);
#endif

	LOG(Info, "Creating Compute Pipeline: %s\n", desc.name);
	PipelineH pipelineH = computeHandles[pipelineIndex];
	if ( IsValid(pipelineH) ) {
		DestroyPipeline( gfx.device, pipelineH );
	}
	pipelineH = CreateComputePipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectName(gfx.device, pipelineH, desc.name);
	computeHandles[pipelineIndex] = pipelineH;
}

bool InitializeGraphics(Engine &engine, Arena &arena)
{
	Graphics &gfx = engine.gfx;
	Window &window = engine.platform.window;
	Arena scratch = MakeSubArena(arena);

	if ( !InitializeGraphicsDevice( gfx.device, scratch, window ) ) {
		return false;
	}

	// Global render pass
	{
		const Format format = FormatFromVulkan(gfx.device.swapchainInfo.format);
#if USE_EDITOR
		const StoreOp storeOp = StoreOpStore;
#else
		const StoreOp storeOp = StoreOpDontCare;
#endif

		const RenderpassDesc renderpassDesc = {
			.name = "main_renderpass",
			.colorAttachmentCount = 1,
			.colorAttachments = {
				{ .format = format, .loadOp = LoadOpClear, .storeOp = StoreOpStore, .isSwapchain = true },
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

#if USE_EDITOR
	const u32 selectionBufferSize = AlignUp( sizeof(u32), gfx.device.alignment.storageBufferOffset );
	gfx.selectionBufferH = CreateBuffer(gfx.device, selectionBufferSize, BufferUsageStorageTexelBuffer, HeapType_Readback);
	gfx.selectionBufferViewH = CreateBufferView(gfx.device, gfx.selectionBufferH, FormatUInt);
#endif // USE_EDITOR


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

	// Texture handles
	for (u32 i = 0; i < MAX_TEXTURES; ++i) {
		gfx.textureIndices[i] = i;
		gfx.textureHandles[i].idx = i;
	}

	LoadData(engine);

	// Graphics pipelines
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		CompileGraphicsPipeline(engine, scratch, i);
	}

	// Compute pipelines
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i)
	{
		CompileComputePipeline(engine, scratch, i);
	}

#if USE_UI
	UI_Initialize(engine.ui, gfx, gfx.device, scratch);
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

BindGroupDesc MaterialBindGroupDesc(Graphics &gfx, const Material &material)
{
	const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
	const BindGroupLayout &bindGroupLayout = pipeline.layout.bindGroupLayouts[BIND_GROUP_MATERIAL];
	const ImageH &albedoImage = GetTextureImage(gfx, material.albedoTexture, gfx.pinkImageH );

	const BindGroupDesc bindGroupDesc = {
		.layout = bindGroupLayout,
		.bindings = {
			{ .index = BINDING_MATERIAL, .buffer = gfx.materialBuffer, .offset = material.bufferOffset, .range = sizeof(SMaterial) },
			{ .index = BINDING_ALBEDO, .image = albedoImage },
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
#if USE_EDITOR
	gfx.idPipelineH = FindPipelineHandle(gfx, "pipeline_id");
#endif // USE_EDITOR

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

void InitializeScene(Engine &engine, Arena scratch)
{
	Graphics &gfx = engine.gfx;

#if LOAD_FROM_SOURCE_FILES
	for (u32 i = 0; i < ARRAY_COUNT(textures); ++i)
	{
		CreateTexture(gfx, textures[i]);
	}
#endif

	for (u32 i = 0; i < ARRAY_COUNT(materials); ++i)
	{
		CreateMaterial(gfx, materials[i]);
	}

	for (u32 i = 0; i < ARRAY_COUNT(entities); ++i)
	{
		CreateEntity(engine, entities[i]);
	}

	// Textures
	gfx.skyTextureH = FindTextureHandle(gfx, "tex_sky");

	// Images
	const byte pinkImagePixels[] = { 255, 0, 255, 255 };
	gfx.pinkImageH = CreateImage(gfx, "pinkImage", 1, 1, 4, false, pinkImagePixels);

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

	gfx.shouldUpdateGlobalBindGroups = true;

	// BindGroups for materials
	for (u32 i = 0; i < gfx.materialCount; ++i)
	{
		const Material &material = GetMaterial(gfx, i);
		const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
		gfx.materialBindGroups[i] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[1], gfx.materialBindGroupAllocator);
	}

	gfx.shouldUpdateMaterialBindGroups = true;

	// Timestamp queries
	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		gfx.timestampPools[i] = CreateTimestampPool(gfx.device, 128);
		//ResetTimestampPool(gfx.device, gfx.timestampPools[i]); // Vulkan 1.2
	}

	gfx.deviceInitialized = true;

}

void InitializeEditor(Engine &engine)
{
	Editor &editor = engine.editor;

	engine.editor.camera[ProjectionPerspective].projectionType = ProjectionPerspective;
	engine.editor.camera[ProjectionPerspective].position = {0, 1, 2};
	engine.editor.camera[ProjectionPerspective].orientation = {0, -0.45f};

	engine.editor.camera[ProjectionOrthographic].projectionType = ProjectionPerspective;
	engine.editor.camera[ProjectionOrthographic].position = {0, 0, -5};
	engine.editor.camera[ProjectionOrthographic].orientation = {};

	engine.editor.cameraType = ProjectionPerspective;

	engine.editor.selectedEntity = U32_MAX;
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

float3 UpDirectionFromAngles(const float2 &angles)
{
	const f32 yaw = angles.x;
	const f32 pitch = angles.y;
	const float3 up = { Sin(yaw)*Sin(pitch), Cos(pitch), Cos(yaw)*Sin(pitch) };
	return up;
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

void Log(LogChannel channel, const char *format, ...)
{
	int finalChannel = 0;
	switch (channel)
	{
		case LogChannelDebug: finalChannel = Debug; break;
		case LogChannelInfo: finalChannel = Info; break;
		case LogChannelWarning: finalChannel = Warning; break;
		case LogChannelError: finalChannel = Error; break;
		default: ASSERT(0 && "Invalid channel");
	}

	char buffer[1024];
	va_list vaList;
	va_start(vaList, format);
	SPrintf(buffer, format, vaList);
	LOG( finalChannel, "%s", buffer );
	va_end(vaList);
}

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


bool RenderGraphics(Engine &engine, f32 deltaSeconds)
{
	Editor &editor = engine.editor;
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Window &window = engine.platform.window;

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
	UI_UploadVerticesToGPU(engine.ui);
#endif

	// Display size
	const f32 displayWidth = static_cast<f32>(gfx.device.swapchain.extent.width);
	const f32 displayHeight = static_cast<f32>(gfx.device.swapchain.extent.height);

	// Camera setup
	float4x4 viewMatrix = Eye();
	float4x4 viewportRotationMatrix = Eye();
	float4x4 projectionMatrix = Eye();
	float4 frustumTopLeft = {};
	float4 frustumBottomRight = {};
	float3 cameraPosition = {};

	if (editor.cameraType == ProjectionPerspective)
	{
		Camera &camera = engine.editor.camera[ProjectionPerspective];

		// Calculate camera matrices
		const float preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
		ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
		const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
		const f32 ar = isLandscapeRotation ?  displayWidth / displayHeight : displayHeight / displayWidth;
		viewMatrix = ViewMatrixFromCamera(camera);
		const float fovy = 60.0f;
		const float znear = 0.1f;
		const float zfar = 1000.0f;
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 perspectiveMatrix = Perspective(fovy, ar, znear, zfar);
		projectionMatrix = Mul(viewportRotationMatrix, perspectiveMatrix);

		// Frustum vectors
		const float hypotenuse  = znear / Cos( 0.5f * fovy * ToRadians );
		const float top = hypotenuse * Sin( 0.5f * fovy * ToRadians );
		const float bottom = -top;
		const float right = top * ar;
		const float left = -right;
		frustumTopLeft = Float4( Float3(left, top, -znear), 0.0f );
		frustumBottomRight = Float4( Float3(right, bottom, -znear), 0.0f );

		cameraPosition = camera.position;

		// CPU Frustum culling
		const float3 cameraPosition = camera.position;
		const float3 cameraForward = ForwardDirectionFromAngles(camera.orientation);
		const FrustumPlanes frustumPlanes = FrustumPlanesFromCamera(cameraPosition, cameraForward, znear, zfar, fovy, ar);
		for (u32 i = 0; i < scene.entityCount; ++i)
		{
			Entity &entity = scene.entities[i];
			entity.culled = !EntityIsInFrustum(entity, frustumPlanes);
		}
	}
	else
	{
		Camera &camera = engine.editor.camera[ProjectionOrthographic];

		// Calculate camera matrices
		const float preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
		ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
		const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
		const f32 ar = isLandscapeRotation ?  displayWidth / displayHeight : displayHeight / displayWidth;
		viewMatrix = ViewMatrixFromCamera(camera);
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 orthographicMatrix = Orthogonal(-8*ar, 8*ar, -8, 8, -10.0, 10.0);
		projectionMatrix = Mul(viewportRotationMatrix, orthographicMatrix);

		// Frustum vectors
		frustumTopLeft = Float4( Float3(-10*ar, 10, -10), 0.0f );
		frustumBottomRight = Float4( Float3(10*ar, -10, 10), 0.0f );

		cameraPosition = camera.position;

		// CPU Frustum culling
		for (u32 i = 0; i < scene.entityCount; ++i)
		{
			Entity &entity = scene.entities[i];
			entity.culled = false; // TODO: Frustum culling with a 2D camera
		}
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
		.eyePosition = Float4(cameraPosition, 1.0f),
		.shadowmapDepthBias = 0.005,
		.time = totalSeconds,
		.mousePosition = int2{window.mouse.x, window.mouse.y},
		.selectedEntity = editor.selectedEntity,
	};

	// Update globals buffer
	Globals *globalsBufferPtr = (Globals*)GetBufferPtr(gfx.device, gfx.globalsBuffer[frameIndex]);
	*globalsBufferPtr = globals;

	// Update entity data
	SEntity *entities = (SEntity*)GetBufferPtr(gfx.device, gfx.entityBuffer[frameIndex]);
	for (u32 i = 0; i < scene.entityCount; ++i)
	{
		const Entity &entity = scene.entities[i];
		const float4x4 worldMatrix = Mul(Translate(entity.position), Scale(Float3(entity.scale))); // TODO: Apply also rotation
		entities[i].world = worldMatrix;
	}

	// Update bind groups
	if (gfx.shouldUpdateGlobalBindGroups)
	{
		gfx.shouldUpdateGlobalBindGroups = false;
		UpdateGlobalBindGroups(gfx);
	}

	if (gfx.shouldUpdateMaterialBindGroups)
	{
		gfx.shouldUpdateMaterialBindGroups = false;
		UpdateMaterialBindGroups(gfx);
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

		for (u32 entityIndex = 0; entityIndex < scene.entityCount; ++entityIndex)
		{
			const Entity &entity = scene.entities[entityIndex];

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

		for (u32 entityIndex = 0; entityIndex < scene.entityCount; ++entityIndex)
		{
			const Entity &entity = scene.entities[entityIndex];

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
			const ImageH &skyImage = GetTextureImage(gfx, gfx.skyTextureH, gfx.pinkImageH);
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
					{ .index = 1, .image = skyImage },
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
			const UI &ui = engine.ui;

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

#if USE_EDITOR
	if ( editor.selectEntity )
	{ // Selection buffer
		BeginDebugGroup(commandList, "Entity selection");

		{ // Draw entity IDs
			SetClearColor(commandList, 0, U32_MAX);

			BeginRenderPass(commandList, gfx.renderTargets.idFramebuffer );

			const uint2 framebufferSize = GetFramebufferSize(gfx.renderTargets.idFramebuffer);
			SetViewportAndScissor(commandList, framebufferSize);

			SetPipeline(commandList, gfx.idPipelineH);
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			for (u32 entityIndex = 0; entityIndex < scene.entityCount; ++entityIndex)
			{
				const Entity &entity = scene.entities[entityIndex];

				if ( !entity.visible || entity.culled ) continue;

				// Draw!!!
				const uint32_t indexCount = entity.indices.size/2; // div 2 (2 bytes per index)
				const uint32_t firstIndex = entity.indices.offset/2; // div 2 (2 bytes per index)
				const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, entityIndex);
			}

			EndRenderPass(commandList);
		}

		{ // Write entity ID under mouse cursor into selection buffer
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
#endif // USE_EDITOR


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

void AddEndFrameCommand(Graphics &gfx, const EndFrameCommand &command)
{
	ASSERT(gfx.endFrameCommandCount < ARRAY_COUNT(gfx.endFrameCommands));
	gfx.endFrameCommands[gfx.endFrameCommandCount++] = command;
}

void ProcessEndFrameCommands(Engine &engine, Arena scratch)
{
	Graphics &gfx = engine.gfx;

	if ( gfx.endFrameCommandCount > 0 )
	{
		WaitDeviceIdle(gfx.device);

		for (u32 i = 0; i < gfx.endFrameCommandCount; ++i)
		{
			const EndFrameCommand &command = gfx.endFrameCommands[i];

			switch (command.type)
			{
				case EndFrameCommandReloadGraphicsPipeline:
				{
					const u32 pipelineIndex = command.pipelineIndex;
					CompileGraphicsPipeline(engine, scratch, pipelineIndex);
					break;
				}
				case EndFrameCommandRemoveTexture:
				{
					RemoveTexture(engine.gfx, command.textureH);
					break;
				}

				default:;
			}
		}

		gfx.endFrameCommandCount = 0;

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
		ImGui::Text("Entity count: %u", scene.entityCount);
		ImGui::Separator();

		for ( u32 i = 0; i < scene.entityCount; ++i )
		{
			const Entity &entity = scene.entities[i];
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

void UIBeginFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
	const Window &window = GetWindow(engine);
	Graphics &gfx = engine.gfx;

	UI_SetInputState(ui, window.keyboard, window.mouse, window.chars);
	UI_SetViewportSize(ui, uint2{window.width, window.height});

	UI_BeginFrame(ui);
}

void UIEndFrameRecording(Engine &engine)
{
	UI &ui = GetUI(engine);
	UI_EndFrame(ui);
}

#endif


void CleanupGameLibrary(Game &game)
{
	if (game.library)
	{
		CloseLibrary(game.library);
		game.library = nullptr;
		game.InitAPI = nullptr;
		game.Start = nullptr;
		game.Update = nullptr;
		game.Stop = nullptr;
	}
}

void LoadGameLibrary(Game &game)
{
#if PLATFORM_LINUX || PLATFORM_ANDROID
	constexpr const char * dynamicLibName = "game.so";
#elif PLATFORM_WINDOWS
	constexpr const char * dynamicLibName = "game.dll";
#else
#error "Missing implementation"
#endif

	FilePath dynamicLibPath = MakePath(BinDir, dynamicLibName);
	LOG(Info, "Loading dynamic library: %s\n", dynamicLibPath.str);

	game.library = OpenLibrary(dynamicLibPath.str);

	if (game.library)
	{
		game.InitAPI = (GameInitAPIFunction) LoadSymbol(game.library, "GameInitAPI");
		game.Start = (GameStartFunction) LoadSymbol(game.library, "GameStart");
		game.Update = (GameUpdateFunction) LoadSymbol(game.library, "GameUpdate");
		game.Stop = (GameStopFunction) LoadSymbol(game.library, "GameStop");

		if (game.InitAPI) {
			LOG(Info, "- GameInitAPI loaded successfully.\n");
		} else {
			LOG(Error, "- GameInitAPI not loaded :-(\n");
		}
		if (game.Start) {
			LOG(Info, "- GameStart loaded successfully.\n");
		} else {
			LOG(Error, "- GameStart not loaded :-(\n");
		}
		if (game.Update) {
			LOG(Info, "- GameUpdate loaded successfully.\n");
		} else {
			LOG(Error, "- GameUpdate not loaded :-(\n");
		}
		if (game.Stop) {
			LOG(Info, "- GameStop loaded successfully.\n");
		} else {
			LOG(Error, "- GameStop not loaded :-(\n");
		}

		if (game.Start == nullptr || game.Update == nullptr || game.Stop == nullptr)
		{
			LOG(Error, "- Could not load all functions in %s\n", dynamicLibName);
			CleanupGameLibrary(game);
		}
	}
	else
	{
		LOG(Error, "- Error opening %s\n", dynamicLibName);
	}

	if (game.InitAPI)
	{
		EngineAPI api = {};
		api.Log = Log;
		game.InitAPI(api);
	}
}

void CompileGameLibrary(Game &game)
{
	char text[MAX_PATH_LENGTH];

#if PLATFORM_WINDOWS
	constexpr const char *build = "build.bat";
#elif PLATFORM_LINUX
	constexpr const char *build = "make";
#else
	constexpr const char *build = "<none>";
#endif
	SPrintf(text, "%s game", build);

	if ( !ExecuteProcess(text) )
	{
		LOG(Warning,
			"Could not compile game library. "
			"Have you tried to launch the app from the project directory?");
	}
}

void GameUpdate(Engine &engine)
{
	Game &game = engine.game;

	if ( game.shouldRecompile )
	{
		const bool wasLoaded = game.library;

		if ( wasLoaded ) {
			CleanupGameLibrary(game);
		}

		CompileGameLibrary(game);

		if ( wasLoaded ) {
			LoadGameLibrary(game);
		}

		game.shouldRecompile = false;
	}

	if (game.state == GameStateStarting)
	{
		LoadGameLibrary(game);

		if ( game.Start )
		{
			game.Start();
		}

		game.state = GameStateRunning;
	}

	if (game.state == GameStateRunning)
	{
		if ( game.Update )
		{
			game.Update();
		}
	}

	if (game.state == GameStateStopping)
	{
		if (game.Stop)
		{
			game.Stop();
		}

		CleanupGameLibrary(game);

		game.state = GameStateStopped;
	}
}




bool OnPlatformInit(Platform &platform)
{
	gStringInterning = &platform.stringInterning;

	SetGraphicsStringInterning(gStringInterning);

	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

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

bool OnPlatformWindowInit(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

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
		if ( !InitializeGraphics(engine, platform.globalArena) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphics failed!\n");
			return false;
		}

		InitializeScene(engine, platform.globalArena);

		InitializeEditor(engine);
	}

#if USE_IMGUI
	InitializeImGuiGraphics(gfx, platform.window);
#endif

	return true;
}

void OnPlatformUpdate(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

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
			gfx.shouldUpdateGlobalBindGroups = true;
		}
		gfx.device.swapchain.outdated = false;
	}

	if ( gfx.deviceInitialized && gfx.device.swapchain.handle != VK_NULL_HANDLE )
	{
#if USE_IMGUI
		UpdateImGui(gfx);
#endif

#if USE_UI
		UIBeginFrameRecording(engine);
#endif

#if USE_EDITOR
		EditorUpdate(engine);
#endif

		GameUpdate(engine);

#if USE_UI
		UIEndFrameRecording(engine);
#endif

		RenderGraphics(engine, platform.deltaSeconds);

#if USE_EDITOR
		EditorUpdatePostRender(engine);
#endif

		ProcessEndFrameCommands(engine, platform.frameArena);
	}
}

void OnPlatformWindowCleanup(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	WaitDeviceIdle(gfx);
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device, gfx.device.swapchain);
	CleanupGraphicsSurface(gfx.device);
}

void OnPlatformCleanup(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;
	Game &game = engine.game;

	CleanupGameLibrary(game);

	WaitDeviceIdle(gfx);

#if USE_IMGUI
	CleanupImGui();
#endif
#if USE_UI
	UI_Cleanup(engine.ui);
#endif

	CleanupScene(gfx);

	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device, gfx.device.swapchain);
	CleanupGraphicsSurface(gfx.device);

	CleanupGraphics(gfx);
}

#if USE_DATA_BUILD
static void BuildData(Arena frameArena)
{
	LOG(Info, "Building data\n");

	FilePath dir;
	CreateDirectory( MakePath(ProjectDir, "build").str );
	CreateDirectory( MakePath(ProjectDir, "build/shaders").str );

	CompileShaders();

	FilePath filepath =  MakePath(DataDir, "assets.dat");
	FILE *file = fopen(filepath.str, "wb");
	if ( file )
	{
		const u32 shaderCount = ARRAY_COUNT(shaderSources);
		const u32 shadersOffset = sizeof( DataHeader );
		const u32 shadersSize = shaderCount * sizeof(ShaderHeader);
		const u32 imageCount = ARRAY_COUNT(textures);
		const u32 imagesOffset = shadersOffset + shadersSize;
		const u32 imagesSize = imageCount * sizeof(ImageHeader);
		const u32 basePayloadOffset = imagesOffset + imagesSize;

		// Write file header
		const DataHeader dataHeader = {
			.magicNumber = U32FromChars('I', 'R', 'I', 'S'),
			.shadersOffset = shadersOffset,
			.shaderCount = shaderCount,
			.imagesOffset = imagesOffset,
			.imageCount = imageCount,
		};
		fwrite(&dataHeader, sizeof(dataHeader), 1, file);

		u32 payloadOffset = basePayloadOffset;

		// Write asset headers

		// Shaders
		for (u32 i = 0; i < shaderCount; ++i)
		{
			const ShaderSourceDesc &shaderSourceDesc = shaderSources[i];

			char filepath[MAX_PATH_LENGTH];
			SPrintf(filepath, "%s/shaders/%s.spv", DataDir, shaderSourceDesc.name);

			u64 payloadSize = 0;
			GetFileSize(filepath, payloadSize);

			const ShaderHeader shaderHeader = {
				.desc = shaderSources[i],
				.spirvOffset = payloadOffset,
				.spirvSize = U64ToU32(payloadSize),
			};
			fwrite(&shaderHeader, sizeof(shaderHeader), 1, file);

			payloadOffset += payloadSize;
		}

		// Images
		for (u32 i = 0; i < imageCount; ++i)
		{
			const TextureDesc &texture = textures[i];

			const FilePath imagePath = MakePath(ProjectDir, texture.filename);

			int texWidth, texHeight, texChannels;
			int ok = stbi_info(imagePath.str, &texWidth, &texHeight, &texChannels);
			texChannels = 4; // Because we use STBI_rgb_alpha
			if ( !ok )
			{
				LOG(Error, "stbi_info failed to load %s\n", imagePath.str);
				texWidth = texHeight = 1;
				texChannels = 4;
			}

			const u64 payloadSize = texWidth * texHeight * texChannels;

			const ImageHeader imageHeader = {
				.desc = texture,
				.width = I32ToU16(texWidth),
				.height = I32ToU16(texHeight),
				.channels = I32ToU8(texChannels),
				.pixelsOffset = payloadOffset,
				.pixelsSize = U64ToU32(payloadSize),
			};
			fwrite(&imageHeader, sizeof(imageHeader), 1, file);

			payloadOffset += payloadSize;
		}

		// Write assets payload

		// Shaders
		for (u32 i = 0; i < ARRAY_COUNT(shaderSources); ++i)
		{
			const ShaderSourceDesc &shaderSourceDesc = shaderSources[i];

			char filepath[MAX_PATH_LENGTH];
			SPrintf(filepath, "%s/shaders/%s.spv", DataDir, shaderSourceDesc.name);

			u64 payloadSize = 0;
			GetFileSize(filepath, payloadSize);

			Arena scratch = MakeSubArena(frameArena);
			void *shaderPayload = PushSize(scratch, payloadSize);
			ReadEntireFile(filepath, shaderPayload, payloadSize);

			fwrite(shaderPayload, payloadSize, 1, file);
		}

		// Images
		for (u32 i = 0; i < imageCount; ++i)
		{
			const TextureDesc &texture = textures[i];

			const FilePath imagePath = MakePath(ProjectDir, texture.filename);

			int texWidth, texHeight, texChannels;
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

			const u64 payloadSize = texWidth * texHeight * texChannels;

			fwrite(pixels, payloadSize, 1, file);

			if ( originalPixels ) {
				stbi_image_free(originalPixels);
			}

			payloadOffset += payloadSize;
		}

		fclose(file);
	}
}
#endif // USE_DATA_BUILD

static void LoadData(Engine &engine)
{
	Arena &dataArena = engine.platform.dataArena;
	ResetArena(dataArena);

	const FilePath filepath = MakePath(DataDir, "assets.dat");

	DataChunk *chunk = PushFile( dataArena, filepath.str );
	if ( !chunk ) {
		LOG( Error, "Could not open file %s\n", filepath.str );
		QUIT_ABNORMALLY();
	}

	byte *bytes =  chunk->bytes;
	DataHeader *dataHeader = (DataHeader*)bytes;

	if (dataHeader->magicNumber != U32FromChars('I', 'R', 'I', 'S'))
	{
		LOG( Error, "Wrong magic number in file %s\n", filepath.str );
		QUIT_ABNORMALLY();
	}

	engine.data.header = dataHeader;
	engine.data.shaders = PushArray(dataArena, LoadedShader, dataHeader->shaderCount);
	engine.data.images = PushArray(dataArena, LoadedImage, dataHeader->imageCount);

	ShaderHeader *shaderHeaders = (ShaderHeader*)(bytes + dataHeader->shadersOffset);

	for (u32 i = 0; i < dataHeader->shaderCount; ++i)
	{
		LoadedShader &shader = engine.data.shaders[i];
		shader.header = shaderHeaders + i;
		shader.spirv = bytes + shader.header->spirvOffset;
	}

	ImageHeader *imageHeaders = (ImageHeader*)(bytes + dataHeader->imagesOffset);

	for (u32 i = 0; i < dataHeader->imageCount; ++i)
	{
		LoadedImage &image = engine.data.images[i];
		image.header = imageHeaders + i;
		image.pixels = bytes + image.header->pixelsOffset;

		CreateTexture(engine.gfx, image);
	}
}

void EngineMain( int argc, char **argv,  void *userData )
{
	Engine engine = {};

	// Memory
	engine.platform.stringMemorySize = KB(16);
	engine.platform.dataMemorySize = MB(16);
	engine.platform.globalMemorySize = MB(64);
	engine.platform.frameMemorySize = MB(16);

	// Callbacks
	engine.platform.InitCallback = OnPlatformInit;
	engine.platform.UpdateCallback = OnPlatformUpdate;
	engine.platform.CleanupCallback = OnPlatformCleanup;
	engine.platform.WindowInitCallback = OnPlatformWindowInit;
	engine.platform.WindowCleanupCallback = OnPlatformWindowCleanup;

#if PLATFORM_ANDROID
	engine.platform.androidApp = (android_app*)userData;
#endif

	// User data
	engine.platform.userData = &engine;

	PlatformInitialize(engine.platform, argc, argv);

#if USE_DATA_BUILD
	bool buildData = false;
	for ( u32 i = 0; i < argc; ++i ) {
		if ( StrEq(argv[i], "--build-data") ) {
			buildData = true;
		}
	}

	FilePath dataFilepath =  MakePath(DataDir, "assets.dat");
	if ( !ExistsFile(dataFilepath.str) ) {
		buildData = true;
	}

	if ( buildData ) {
		BuildData(engine.platform.frameArena);
	}
#endif // USE_DATA_BUILD

	PlatformRun(engine.platform);
}

#if PLATFORM_ANDROID
void android_main(struct android_app* app)
{
	EngineMain(0, nullptr, app);
}
#else
int main(int argc, char **argv)
{
	EngineMain(argc, argv, NULL);
	return 1;
}
#endif

// Editor implementation
#if USE_EDITOR
#include "editor.cpp"
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

