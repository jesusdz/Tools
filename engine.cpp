#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "tools.h"

#include "tools_platform.h"

#include "tools_gfx.h"

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

#include "handle_manager.h"
#include "engine.h"
#include "data.h"
#include "audio.h"

#define MAX_TEXTURES 8 
#define MAX_MATERIALS 4
#define MAX_ENTITIES 32
#define MAX_GLOBAL_BINDINGS 8
#define MAX_MATERIAL_BINDINGS 4
#define MAX_COMPUTE_BINDINGS 4

#define INVALID_HANDLE -1



#if USE_DATA_BUILD
static constexpr bool sLoadShadersFromText = true;
#else
static constexpr bool sLoadShadersFromText = false;
#endif


struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

typedef u16 Index;

struct Texture
{
	const char *name;
	ImageH image;
	TextureDesc desc;
	u64 ts;
};

typedef Handle TextureH;

struct Material
{
	const char *name;
	const char *pipelineName;
	PipelineH pipelineH;
	TextureH albedoTexture;
	f32 uvScale;
	u32 bufferOffset;
	//MaterialDesc desc;
};

typedef Handle MaterialH;

struct RenderTargets
{
	ImageH depthImage;
	Framebuffer framebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

	ImageH shadowmapImage;
	Framebuffer shadowmapFramebuffer;

	ImageH idImage;
	Framebuffer idFramebuffer;

	bool initialized;
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
	f32 height; // orthographic only
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
	MaterialH materialH;
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
	bool inUploadContext;

	BufferArena globalVertexArena;
	BufferArena globalIndexArena;

	BufferChunk cubeVertices;
	BufferChunk cubeIndices;
	BufferChunk planeVertices;
	BufferChunk planeIndices;
	BufferChunk screenTriangleVertices;
	BufferChunk screenTriangleIndices;
	BufferChunk tileGridVertices;
	BufferChunk tileGridIndices;

	BufferH globalsBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH entityBuffer[MAX_FRAMES_IN_FLIGHT];
	BufferH materialBuffer;
	BufferH computeBufferH;
	BufferViewH computeBufferViewH;
#if USE_EDITOR
	BufferH selectionBufferH;
	BufferViewH selectionBufferViewH;
#endif

	SamplerH pointSamplerH;
	SamplerH linearSamplerH;
	SamplerH shadowmapSamplerH;
	SamplerH skySamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;
	RenderPassH idRenderPassH;

	Texture textures[MAX_TEXTURES];
	TextureDesc textureDescs[MAX_TEXTURES];
	HandleManager textureHandles;

	Material materials[MAX_MATERIALS];
	MaterialDesc materialDescs[MAX_MATERIALS];
	HandleManager materialHandles;

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
	ImageH grayImageH;
	ImageH blackImageH;

	PipelineH shadowmapPipelineH;
	PipelineH skyPipelineH;
	PipelineH guiPipelineH;
#if USE_EDITOR
	PipelineH grid2dPipelineH;
	PipelineH grid3dPipelineH;
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

	f32 deltaSeconds;
};

struct TileAtlasDesc
{
	const char *imagePath;
	const char *name;
	f32 tileSize;
};

struct TileAtlas
{
	TextureH textureH;
	MaterialH materialH;
	f32 size;
	f32 tileSize;
};

union Tile
{
	u32 value;
	struct
	{
		u32 used : 1;
		u32 atlasId : 8;
		u32 tileId : 23;
	};
};

#define TILE_GRID_SIZE_X 20
#define TILE_GRID_SIZE_Y 15

struct TileGrid
{
	Tile tiles[TILE_GRID_SIZE_X][TILE_GRID_SIZE_Y];
	BufferChunk vertices;
	BufferChunk indices;
	u32 indexCount;
	bool needsUpdate;
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
	EntityDesc entityDescs[MAX_ENTITIES];
	HandleManager entityHandles;

	TileAtlas tileAtlas;
	TileGrid tileGrid;
	u32 tileGridCount;
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

enum EngineMode
{
	EngineModeEditor2D,
	EngineModeEditor3D,
	EngineModeGame2D,
	EngineModeGame3D,
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
	Audio audio;

#if USE_EDITOR
	Editor editor;
#endif

	BinAssets shaderAssets;
	BinAssets assets;

	Arena dataArenaStates[1];
	u32 dataArenaStateCount;

	EngineMode mode;
};

inline bool IsEngineModeEditor(EngineMode engineMode)
{
	const bool res = engineMode == EngineModeEditor2D || engineMode == EngineModeEditor3D;
	return res;
}

inline bool IsEngineModeGame(EngineMode engineMode)
{
	const bool res = !IsEngineModeEditor(engineMode);
	return res;
}

inline bool IsEngineMode2D(EngineMode engineMode)
{
	const bool res = engineMode == EngineModeEditor2D || engineMode == EngineModeGame2D;
	return res;
}

inline bool IsEngineMode3D(EngineMode engineMode)
{
	const bool res = !IsEngineMode2D(engineMode);
	return res;
}

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

static const Index cubeIndices[] = {
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

static const Index planeIndices[] = {
	0, 1, 2, 2, 3, 0,
};

static const Vertex screenTriangleVertices[] = {
	{{-1.0f,  1.0f, 0.0f}, {0.0f, 0.0f}},
	{{-1.0f, -3.0f, 0.0f}, {0.0f, 2.0f}},
	{{ 3.0f,  1.0f, 0.0f}, {2.0f, 0.0f}},
};

static const Index screenTriangleIndices[] = {
	0, 1, 2,
};

static ShaderSourceDesc shaderSourceDescs[] = {
	{ .type = ShaderTypeVertex,   .filename = "shading.hlsl",        .entryPoint = "VSMain",      .name = "vs_shading" },
	{ .type = ShaderTypeFragment, .filename = "shading.hlsl",        .entryPoint = "PSMain",      .name = "fs_shading" },
	{ .type = ShaderTypeVertex,   .filename = "shading_2d.hlsl",     .entryPoint = "VSMain",      .name = "vs_shading_2d" },
	{ .type = ShaderTypeFragment, .filename = "shading_2d.hlsl",     .entryPoint = "PSMain",      .name = "fs_shading_2d" },
	{ .type = ShaderTypeVertex,   .filename = "sky.hlsl",            .entryPoint = "VSMain",      .name = "vs_sky" },
	{ .type = ShaderTypeFragment, .filename = "sky.hlsl",            .entryPoint = "PSMain",      .name = "fs_sky" },
	{ .type = ShaderTypeVertex,   .filename = "shadowmap.hlsl",      .entryPoint = "VSMain",      .name = "vs_shadowmap" },
	{ .type = ShaderTypeFragment, .filename = "shadowmap.hlsl",      .entryPoint = "PSMain",      .name = "fs_shadowmap" },
	{ .type = ShaderTypeVertex,   .filename = "grid_2d.hlsl",        .entryPoint = "VSMain",      .name = "vs_grid_2d" },
	{ .type = ShaderTypeFragment, .filename = "grid_2d.hlsl",        .entryPoint = "PSMain",      .name = "fs_grid_2d" },
	{ .type = ShaderTypeVertex,   .filename = "grid_3d.hlsl",        .entryPoint = "VSMain",      .name = "vs_grid_3d" },
	{ .type = ShaderTypeFragment, .filename = "grid_3d.hlsl",        .entryPoint = "PSMain",      .name = "fs_grid_3d" },
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
		.vsName = "vs_shading_2d",
		.fsName = "fs_shading_2d",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_shading_2d",
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
			.blending = true,
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
		.vsName = "vs_grid_2d",
		.fsName = "fs_grid_2d",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_grid_2d",
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
			.blending = true,
		}
	},
	{
		.vsName = "vs_grid_3d",
		.fsName = "fs_grid_3d",
		.renderPass = "main_renderpass",
		.desc = {
			.name = "pipeline_grid_3d",
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
			.blending = true,
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

void OnPlatformPreRenderAudio(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	PreRenderAudio(engine);
}

void OnPlatformRenderAudio(Platform &platform, SoundBuffer &soundBuffer)
{
	Engine &engine = GetEngine(platform);
	RenderAudio(engine, soundBuffer);
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

const u32 FindShaderSourceDescIndex(const char *name)
{
	for (u32 i = 0; i < ARRAY_COUNT(shaderSourceDescs); ++i) {
		if ( StrEq(shaderSourceDescs[i].name, name) ) {
			return i;
		}
	}
	LOG(Warning, "Could not find ShaderSourceDesc <%s>.\n", name);
	INVALID_CODE_PATH();
	return U32_MAX;
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

static CommandList BeginUploadCommandList(Graphics &gfx)
{
	ASSERT(!gfx.inUploadContext && "Cannot nest calls to BeginUploadCommandList");
	gfx.inUploadContext = true;

	gfx.stagingBufferOffset = 0;

	CommandList commandList = BeginTransientCommandList(gfx.device);
	return commandList;
}

static void EndUploadCommandList(Graphics &gfx, CommandList commandList)
{
	EndTransientCommandList(gfx.device, commandList);

	ASSERT(gfx.inUploadContext && "BeginUploadCommandList must have been called first");
	gfx.inUploadContext = false;
}

static StagedData StageData(Graphics &gfx, const void *data, u32 size, u32 alignment = 0)
{
	ASSERT(gfx.inUploadContext && "StageData must be called between calls to Begin/EndUploadCommandList");

	const Buffer &stagingBuffer = GetBuffer(gfx.device, gfx.stagingBuffer);

	const u32 finalAlignment = Max(alignment, gfx.device.alignment.optimalBufferCopyOffset);
	const u32 unalignedOffset = stagingBuffer.alloc.offset + gfx.stagingBufferOffset;
	const u32 alignedOffset = AlignUp(unalignedOffset, finalAlignment);

	ASSERT(alignedOffset + size <= stagingBuffer.size);

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

void UploadData(Graphics &gfx, const CommandList &commandList, const void *data, u32 size, BufferH destBuffer, u32 destOffset, u32 alignment = 0)
{
	StagedData staged = StageData(gfx, data, size, alignment);

	// Copy contents from the staging to the final buffer
	CopyBufferToBuffer(commandList, staged.buffer, staged.offset, destBuffer, destOffset, size);
}

BufferChunk PushData(Graphics &gfx, const CommandList &commandList, BufferArena &arena, const void *data, u32 size, u32 alignment = 0)
{
	if (data)
	{
		UploadData(gfx, commandList, data, size, arena.buffer, arena.used, alignment);
	}

	BufferChunk chunk = {};
	chunk.buffer = arena.buffer;
	chunk.offset = arena.used;
	chunk.size = size;

	arena.used += size;

	return chunk;
}


////////////////////////////////////////////////////////////////////////
// Image management

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

	CommandList commandList = BeginUploadCommandList(gfx);

	StagedData staged = StageData(gfx, pixels, size, alignment);

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

	EndUploadCommandList(gfx, commandList);

	return image;
}

ImageH CreateImage(Graphics &gfx, const ImagePixels &img, const char *name, bool createMipmaps)
{
	const ImageH imageHandle = CreateImage(gfx, name, img.width, img.height, img.channelCount, createMipmaps, img.pixels);
	return imageHandle;
}

////////////////////////////////////////////////////////////////////////
// Texture management

Texture &GetTexture(Graphics &gfx, TextureH handle)
{
	ASSERT( IsValidHandle(gfx.textureHandles, handle) );
	Texture &texture = gfx.textures[handle.idx];
	return texture;
}

TextureDesc &GetTextureDesc(Graphics &gfx, TextureH handle)
{
	ASSERT( IsValidHandle(gfx.textureHandles, handle) );
	TextureDesc &textureDesc = gfx.textureDescs[handle.idx];
	return textureDesc;
}

Texture &GetTextureAt(Graphics &gfx, u32 index)
{
	const u16 handleIndex = gfx.textureHandles.indices[index];
	const TextureH handle = gfx.textureHandles.handles[handleIndex];
	Texture &texture = GetTexture(gfx, handle);
	return texture;
}

TextureH CreateTexture(Graphics &gfx)
{
	const TextureH textureHandle = NewHandle(gfx.textureHandles);
	return textureHandle;
}

TextureH CreateTexture(Graphics &gfx, const TextureDesc &desc)
{
	TextureH textureHandle = InvalidHandle;

	const FilePath imagePath = MakePath(AssetDir, desc.filename);

	ImagePixels img;
	if ( ReadImagePixels(imagePath.str, img) )
	{

		const ImageH imageHandle = CreateImage(gfx, img, desc.name, desc.mipmap);

		textureHandle = CreateTexture(gfx);

		gfx.textureDescs[textureHandle.idx] = desc;

		Texture &texture = GetTexture(gfx, textureHandle);
		texture.name = desc.name;
		texture.image = imageHandle;
		texture.desc = desc;

		GetFileLastWriteTimestamp(imagePath.str, texture.ts);

		FreeImagePixels(img);
	}

	return textureHandle;
}

TextureH CreateTexture(Graphics &gfx, const BinImage &binImage)
{
	const BinImageDesc &desc = *binImage.desc;
	const char *name = desc.name;
	const u32 width = desc.width;
	const u32 height = desc.height;
	const u32 channels = desc.channels;
	const u32 mipmap = desc.mipmap;
	const u8 *pixels = binImage.pixels;

	const ImageH imageHandle = CreateImage(gfx, name, width, height, channels, mipmap, pixels);

	const TextureH textureHandle = CreateTexture(gfx);

	Texture &texture = GetTexture(gfx, textureHandle);
	texture.name = desc.name;
	texture.image = imageHandle;

	return textureHandle;
}

ImageH GetTextureImage(Graphics &gfx, TextureH textureH, ImageH imageH)
{
	ImageH res = imageH;

	if ( IsValidHandle(gfx.textureHandles, textureH) ) {
		const Texture &texture = GetTexture(gfx, textureH);
		res = texture.image;
	}

	return res;
}

static bool IsTextureName( Handle handle, const char *name, void *data )
{
	Graphics &gfx = *(Graphics*)data;
	Texture &texture = GetTexture(gfx, handle);
	const bool equals = StrEq(texture.name, name);
	return equals;
}

TextureH FindTextureHandle(Graphics &gfx, const char *name)
{
	const HandleFinder finder = {
		.checkHandle = IsTextureName,
		.name = name,
		.data = &gfx,
	};
	const TextureH handle = FindHandle(gfx.textureHandles, finder);
	return handle;
}

void RemoveTexture(Graphics &gfx, TextureH textureH, bool freeHandle = true)
{
	if (IsValidHandle(gfx.textureHandles, textureH))
	{
		Texture &texture = GetTexture(gfx, textureH);
		DestroyImageH(gfx.device, texture.image);
		texture = {};

		if (freeHandle) {
			FreeHandle(gfx.textureHandles, textureH);
		}

		gfx.shouldUpdateMaterialBindGroups = true;
	}
}

static void RecreateTextureIfModifed(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	Graphics &gfx = engine.gfx;

	Texture &texture = GetTexture(gfx, handle);
	const TextureDesc &desc = texture.desc;

	const FilePath imagePath = MakePath(AssetDir, desc.filename);

	u64 ts;
	GetFileLastWriteTimestamp(imagePath.str, ts);

	if ( ts > texture.ts )
	{
		ImagePixels img;

		if ( ReadImagePixels(imagePath.str, img) )
		{
			WaitDeviceIdle(gfx.device);

			texture.ts = ts;

			DestroyImageH(gfx.device, texture.image);

			texture.image = CreateImage(gfx, img, desc.name, desc.mipmap);

			GetFileLastWriteTimestamp(imagePath.str, texture.ts);

			FreeImagePixels(img);

			gfx.shouldUpdateMaterialBindGroups = true;
		}
	}
}

void RecreateModifiedTextures(Engine &engine)
{
	ForAllHandles(engine.gfx.textureHandles, RecreateTextureIfModifed, &engine);
}


////////////////////////////////////////////////////////////////////////
// Material management

Material &GetMaterial(Graphics &gfx, MaterialH handle)
{
	ASSERT( IsValidHandle(gfx.materialHandles, handle) );
	Material &material = gfx.materials[handle.idx];
	return material;
}

MaterialDesc &GetMaterialDesc(Graphics &gfx, MaterialH handle)
{
	ASSERT( IsValidHandle(gfx.materialHandles, handle) );
	MaterialDesc &materialDesc = gfx.materialDescs[handle.idx];
	return materialDesc;
}

void CreateMaterialBindGroup(Graphics &gfx, MaterialH materialH);

MaterialH CreateMaterial(Graphics &gfx, const MaterialDesc &desc)
{
	TextureH textureHandle = FindTextureHandle(gfx, desc.textureName);
	PipelineH pipelineHandle = FindPipelineHandle(gfx, desc.pipelineName);

	MaterialH materialHandle = NewHandle(gfx.materialHandles);
	Material& material = GetMaterial(gfx, materialHandle);
	material.name = desc.name;
	material.pipelineName = desc.pipelineName;
	material.pipelineH = pipelineHandle;
	material.albedoTexture = textureHandle;
	material.uvScale = desc.uvScale;
	material.bufferOffset = materialHandle.idx * AlignUp(sizeof(SMaterial), gfx.device.alignment.uniformBufferOffset);

	gfx.materialDescs[materialHandle.idx] = desc;

	CreateMaterialBindGroup(gfx, materialHandle);

	return materialHandle;
}

MaterialH CreateMaterial( Graphics &gfx, const BinMaterialDesc &desc)
{
	const MaterialDesc materialDesc = {
		.name = desc.name,
		.textureName = desc.textureName,
		.pipelineName = desc.pipelineName,
		.uvScale = desc.uvScale,
	};
	MaterialH materialHandle = CreateMaterial(gfx, materialDesc);
	return materialHandle;
}

static bool IsMaterialName( Handle handle, const char *name, void *data )
{
	Graphics &gfx = *(Graphics*)data;
	Material &material = GetMaterial(gfx, handle);
	const bool equals = StrEq(material.name, name);
	return equals;
}

MaterialH FindMaterialHandle(Graphics &gfx, const char *name)
{
	const HandleFinder finder = {
		.checkHandle = IsMaterialName,
		.name = name,
		.data = &gfx,
	};
	const MaterialH handle = FindHandle(gfx.materialHandles, finder);
	return handle;
}

void RemoveMaterial(Graphics &gfx, MaterialH materialH, bool freeHandle = true)
{
	Material &material = GetMaterial(gfx, materialH);
	material = {};

	if (freeHandle) {
		FreeHandle(gfx.materialHandles, materialH);
	}

	// TODO: Do we need this here? I think we don't as we are removing, not modifying
	//gfx.shouldUpdateMaterialBindGroups = true;
}


////////////////////////////////////////////////////////////////////////
// TileAtlas management

void CreateTileAtlas(Engine &engine, const TileAtlasDesc &desc)
{
	Graphics &gfx = engine.gfx;
	TileAtlas &tileAtlas = engine.scene.tileAtlas;

	if ( !IsValidHandle(gfx.textureHandles, tileAtlas.textureH))
	{
		const TextureDesc textureDesc = {
			.name = desc.name,
			.filename = desc.imagePath,
			.mipmap = false,
		};

		const TextureH textureHandle = CreateTexture(gfx, textureDesc);
		if ( IsValidHandle(gfx.textureHandles, textureHandle) )
		{
			tileAtlas.textureH = textureHandle;
			const Texture &texture = GetTexture(gfx, textureHandle);
			const Image &image = GetImage(gfx.device, texture.image);

			const MaterialDesc materialDesc = {
				.name = desc.name,
				.textureName = desc.name,
				.pipelineName = "pipeline_shading_2d",
				.uvScale = 1.0f,
			};
			const MaterialH materialHandle = CreateMaterial(engine.gfx, materialDesc);
			tileAtlas.materialH = materialHandle;
			tileAtlas.size = image.width;
			tileAtlas.tileSize = desc.tileSize;

			// TODO(jesus): Put this in a better place
			engine.scene.tileGrid.vertices = engine.gfx.tileGridVertices;
			engine.scene.tileGrid.indices = engine.gfx.tileGridIndices;
			engine.scene.tileGridCount = 1;
		}
	}
	else
	{
		LOG(Warning, "Cannot create more than one tile atlas.\n");
	}
}

void DestroyTileAtlas(Engine &engine)
{
	Graphics &gfx = engine.gfx;
	TileAtlas &tileAtlas = engine.scene.tileAtlas;

	if ( IsValidHandle(gfx.textureHandles, tileAtlas.textureH) )
	{
		RemoveMaterial(engine.gfx, tileAtlas.materialH);
		tileAtlas.materialH = {};

		RemoveTexture(engine.gfx, tileAtlas.textureH);
		tileAtlas.textureH = {};

		engine.scene.tileGridCount = 0;
	}
}

bool IsTileAtlasValid(const Engine &engine)
{
	const Graphics &gfx = engine.gfx;
	const TileAtlas &tileAtlas = engine.scene.tileAtlas;
	const bool res = IsValidHandle(gfx.textureHandles, tileAtlas.textureH);
	return res;
}

const TileAtlas &GetTileAtlas(const Engine &engine)
{
	const TileAtlas &tileAtlas = engine.scene.tileAtlas;
	return tileAtlas;
}

int2 GetGridTileCoord(const Engine &engine, const Camera &camera, int2 pixelCoord)
{
	const uint2 windowSize = {engine.platform.window.width, engine.platform.window.height };
	const float2 uvCoords = {(f32)pixelCoord.x/windowSize.x, 1.0f - (f32)pixelCoord.y/windowSize.y};
	const float2 ndcCoords = 2.0f * uvCoords - float2{1.0f, 1.0f};
	const f32 aspect = (f32)windowSize.x / (f32)windowSize.y;
	const float2 scale = {camera.height * aspect, camera.height};
	const float2 worldCoords = scale * ndcCoords + float2{camera.position.x, camera.position.y};
	const int2 res = {(i32)Floor(worldCoords.x), (i32)Floor(worldCoords.y)};
	//LOG(Debug, "Tile coord: (%f, %f)\n", uvCoords.x, uvCoords.y);
	//LOG(Debug, "Tile coord: (%d, %d)\n", res.x, res.y);
	return res;
}

void SetGridTileAtCoord(Engine &engine, Tile tile, int2 coord)
{
	//LOG(Debug, "Tile %u %u %u\n", (u32)tile.used, tile.atlasId, tile.tileId);
	Graphics &gfx = engine.gfx;
	TileAtlas &tileAtlas = engine.scene.tileAtlas;

	const bool tileAtlasValid = IsValidHandle(gfx.textureHandles, tileAtlas.textureH);
	const bool tileGridValid = engine.scene.tileGridCount > 0;

	if (tileAtlasValid && tileGridValid)
	{
		if (coord.x >= 0 && coord.x < TILE_GRID_SIZE_X &&
				coord.y >= 0 && coord.y < TILE_GRID_SIZE_Y )
		{
			TileGrid &tileGrid = engine.scene.tileGrid;
			if ( tileGrid.tiles[coord.x][coord.y].value != tile.value ) {
				tileGrid.tiles[coord.x][coord.y] = tile;
				tileGrid.needsUpdate = true;
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
// Builtin geometry

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


////////////////////////////////////////////////////////////////////////
// Entity management

Entity &GetEntity(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	Entity &entity = scene.entities[handle.idx];
	return entity;
}

EntityDesc &GetEntityDesc(Scene &scene, Handle handle)
{
	ASSERT( IsValidHandle(scene.entityHandles, handle) );
	EntityDesc &entityDesc = scene.entityDescs[handle.idx];
	return entityDesc;
}

Entity &GetEntityAt(Scene &scene, u32 index)
{
	const u16 handleIndex = scene.entityHandles.indices[index];
	const Handle handle = scene.entityHandles.handles[handleIndex];
	Entity &entity = GetEntity(scene, handle);
	return entity;
}

void CreateEntity(Engine &engine, const EntityDesc &desc)
{
	Scene &scene = engine.scene;

	BufferChunk vertices = GetVerticesForGeometryType(engine.gfx, desc.geometryType);
	BufferChunk indices = GetIndicesForGeometryType(engine.gfx, desc.geometryType);

	Handle handle = NewHandle(scene.entityHandles);
	Entity &entity = GetEntity(scene, handle);
	entity.name = desc.name;
	entity.visible = true;
	entity.position = desc.pos;
	entity.scale = desc.scale;
	entity.vertices = vertices;
	entity.indices = indices;
	entity.materialH = FindMaterialHandle(engine.gfx, desc.materialName);

	scene.entityDescs[handle.idx] = desc;
}

void CreateEntity(Engine &engine, const BinEntityDesc &desc)
{
	const EntityDesc entityDesc = {
		.name = desc.name,
		.materialName = desc.materialName,
		.pos = desc.pos,
		.scale = desc.scale,
		.geometryType = desc.geometryType,
	};
	CreateEntity(engine, entityDesc);
}

void RemoveEntity(Engine &engine, Handle handle, bool freeHandle = true)
{
	Entity &entity = GetEntity(engine.scene, handle);
	entity = {};

	if (freeHandle) {
		FreeHandle(engine.scene.entityHandles, handle);
	}
}


////////////////////////////////////////////////////////////////////////
// Render targets

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

	renderTargets.initialized = true;

	return renderTargets;
}

void DestroyRenderTargets(Graphics &gfx, RenderTargets &renderTargets)
{
	if ( !renderTargets.initialized )
	{
		return;
	}

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


////////////////////////////////////////////////////////////////////////
// Shaders and pipelines

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

static ShaderSource GetShaderSource(BinAssets &assets, const char *shaderName)
{
	byte *bytes = 0;
	u32 size = 0;
	for (u32 i = 0; i < assets.header.shaderCount; ++i)
	{
		BinShader &loadedShader = assets.shaders[i];
		if ( StrEq( loadedShader.desc->name, shaderName) )
		{
			bytes = loadedShader.spirv;
			size = loadedShader.desc->location.size;
		}
	}
	if ( !bytes ) {
		LOG( Error, "Could not find shader in assets: %s\n", shaderName );
		LOG( Error, "Shaders in assets:\n");
		for (u32 i = 0; i < assets.header.shaderCount; ++i)
		{
			BinShader &loadedShader = assets.shaders[i];
			LOG( Error, "- %s\n", loadedShader.desc->name);
		}
		QUIT_ABNORMALLY();
	}
	const ShaderSource shaderSource = { .data = bytes, .dataSize = size };
	return shaderSource;
}

static void CompileGraphicsPipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	const RenderPassH renderPassH = FindRenderPassHandle(gfx, pipelineDescs[pipelineIndex].renderPass);

	PipelineDesc desc = pipelineDescs[pipelineIndex].desc;
	desc.renderPass = GetRenderPass(gfx.device, renderPassH);

	if ( sLoadShadersFromText )
	{
		desc.vertexShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].vsName);
		desc.fragmentShaderSource = GetShaderSource(scratch, pipelineDescs[pipelineIndex].fsName);
	}
	else
	{
		desc.vertexShaderSource = GetShaderSource(engine.shaderAssets, pipelineDescs[pipelineIndex].vsName);
		desc.fragmentShaderSource = GetShaderSource(engine.shaderAssets, pipelineDescs[pipelineIndex].fsName);
	}

	LOG(Info, "Creating Graphics Pipeline: %s\n", desc.name);
	PipelineH pipelineH = pipelineHandles[pipelineIndex];
	if ( IsValid(pipelineH) ) {
		DestroyPipeline( gfx.device, pipelineH );
	}
	pipelineH = CreateGraphicsPipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectName(gfx.device, pipelineH, desc.name);
	pipelineHandles[pipelineIndex] = pipelineH;
}

static void CompileComputePipeline(Engine &engine, Arena scratch, u32 pipelineIndex)
{
	Graphics &gfx = engine.gfx;

	ComputeDesc desc = computeDescs[pipelineIndex].desc;

	if ( sLoadShadersFromText )
	{
		desc.computeShaderSource = GetShaderSource(scratch, computeDescs[pipelineIndex].csName);
	}
	else
	{
		desc.computeShaderSource = GetShaderSource(engine.shaderAssets, computeDescs[pipelineIndex].csName);
	}

	LOG(Info, "Creating Compute Pipeline: %s\n", desc.name);
	PipelineH pipelineH = computeHandles[pipelineIndex];
	if ( IsValid(pipelineH) ) {
		DestroyPipeline( gfx.device, pipelineH );
	}
	pipelineH = CreateComputePipeline(gfx.device, scratch, desc, gfx.globalBindGroupLayout);
	SetObjectName(gfx.device, pipelineH, desc.name);
	computeHandles[pipelineIndex] = pipelineH;
}

void LinkHandles(Graphics &gfx)
{
	// Textures
	gfx.skyTextureH = FindTextureHandle(gfx, "tex_sky");

	// Graphics pipelines
	gfx.shadowmapPipelineH = FindPipelineHandle(gfx, "pipeline_shadowmap");
	gfx.skyPipelineH = FindPipelineHandle(gfx, "pipeline_sky");
	gfx.guiPipelineH = FindPipelineHandle(gfx, "pipeline_ui");
#if USE_EDITOR
	gfx.grid2dPipelineH = FindPipelineHandle(gfx, "pipeline_grid_2d");
	gfx.grid3dPipelineH = FindPipelineHandle(gfx, "pipeline_grid_3d");
	gfx.idPipelineH = FindPipelineHandle(gfx, "pipeline_id");
#endif // USE_EDITOR

	// Compute pipelines
#if USE_COMPUTE_TEST
	gfx.computeClearH = FindPipelineHandle(gfx, "compute_clear");
	gfx.computeUpdateH = FindPipelineHandle(gfx, "compute_update");
#endif // USE_COMPUTE_TEST
	gfx.computeSelectH = FindPipelineHandle(gfx, "compute_select");

	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		Material &material = GetMaterial(gfx, handle);
		material.pipelineH = FindPipelineHandle(gfx, material.pipelineName);
	}
}

static void RecompilePipelines(Engine &engine, Arena scratch)
{
	for (u32 i = 0; i < ARRAY_COUNT(pipelineDescs); ++i)
	{
		CompileGraphicsPipeline(engine, scratch, i);
	}

	// Compute pipelines
	for (u32 i = 0; i < ARRAY_COUNT(computeDescs); ++i)
	{
		CompileComputePipeline(engine, scratch, i);
	}
}

bool InitializeGraphics(Engine &engine, Arena &globalArena, Arena scratch)
{
	Graphics &gfx = engine.gfx;

	if ( !InitializeGraphicsDevice( gfx.device, scratch ) ) {
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

	// Create staging buffer
	gfx.stagingBuffer = CreateStagingBuffer(gfx);

	// Create global geometry buffers
	gfx.globalVertexArena = MakeBufferArena( gfx, CreateVertexBuffer(gfx, MB(4)) );
	gfx.globalIndexArena = MakeBufferArena( gfx, CreateIndexBuffer(gfx, MB(4)) );

	CommandList commandList = BeginUploadCommandList(gfx);

	// Create vertex/index buffers
	gfx.cubeVertices = PushData(gfx, commandList, gfx.globalVertexArena, cubeVertices, sizeof(cubeVertices));
	gfx.cubeIndices = PushData(gfx, commandList, gfx.globalIndexArena, cubeIndices, sizeof(cubeIndices));
	gfx.planeVertices = PushData(gfx, commandList, gfx.globalVertexArena, planeVertices, sizeof(planeVertices));
	gfx.planeIndices = PushData(gfx, commandList, gfx.globalIndexArena, planeIndices, sizeof(planeIndices));
	gfx.screenTriangleVertices = PushData(gfx, commandList, gfx.globalVertexArena, screenTriangleVertices, sizeof(screenTriangleVertices));
	gfx.screenTriangleIndices = PushData(gfx, commandList, gfx.globalIndexArena, screenTriangleIndices, sizeof(screenTriangleIndices));

	const u32 numTileVertices = TILE_GRID_SIZE_X * TILE_GRID_SIZE_Y * 4;
	const u32 numTileIndices = TILE_GRID_SIZE_X * TILE_GRID_SIZE_Y * 2 * 3;
	const u32 tileVertexSize = numTileVertices * sizeof(Vertex);
	const u32 tileIndexSize = numTileIndices * sizeof(Index);
	gfx.tileGridVertices = PushData(gfx, commandList, gfx.globalVertexArena, nullptr, tileVertexSize);
	gfx.tileGridIndices = PushData(gfx, commandList, gfx.globalIndexArena, nullptr, tileIndexSize);

	EndUploadCommandList(gfx, commandList);

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
			.samplerCount = MAX_FRAMES_IN_FLIGHT * 4,
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
		{ .set = 0, .binding = BINDING_SAMPLER_POINT, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SAMPLER_LINEAR, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_ENTITIES, .type = SpvTypeStorageBuffer, .stageFlags = SpvStageFlagsVertexBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP, .type = SpvTypeImage, .stageFlags = SpvStageFlagsFragmentBit },
		{ .set = 0, .binding = BINDING_SHADOWMAP_SAMPLER, .type = SpvTypeSampler, .stageFlags = SpvStageFlagsFragmentBit },
	};
	gfx.globalBindGroupLayout = CreateBindGroupLayout(gfx.device, globalShaderBindings, ARRAY_COUNT(globalShaderBindings));

	// Handle managers
	Initialize(gfx.textureHandles, globalArena, MAX_TEXTURES);
	Initialize(gfx.materialHandles, globalArena, MAX_MATERIALS);

	// Graphics pipelines
	RecompilePipelines(engine, scratch);

	// Builtin images
	const byte pinkImagePixels[] = { 255, 0, 255, 255 };
	gfx.pinkImageH = CreateImage(gfx, "pinkImage", 1, 1, 4, false, pinkImagePixels);
	const byte grayImagePixels[] = { 127, 127, 127, 255 };
	gfx.grayImageH = CreateImage(gfx, "grayImage", 1, 1, 4, false, grayImagePixels);
	const byte blackImagePixels[] = { 0, 0, 0, 0 };
	gfx.blackImageH = CreateImage(gfx, "blackImage", 1, 1, 4, false, blackImagePixels);

	// Samplers
	const SamplerDesc pointSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterNearest,
	};
	gfx.pointSamplerH = CreateSampler(gfx.device, pointSamplerDesc);
	const SamplerDesc materialSamplerDesc = {
		.addressMode = AddressModeRepeat,
		.filter = FilterLinear,
	};
	gfx.linearSamplerH = CreateSampler(gfx.device, materialSamplerDesc);
	const SamplerDesc shadowmapSamplerDesc = {
		.addressMode = AddressModeClampToBorder,
		.filter = FilterNearest,
		.borderColor = BorderColorBlackFloat,
		.compareOp = CompareOpGreater,
	};
	gfx.shadowmapSamplerH = CreateSampler(gfx.device, shadowmapSamplerDesc);
	const SamplerDesc skySamplerDesc = {
		.addressMode = AddressModeClampToEdge,
		.filter = FilterLinear,
	};
	gfx.skySamplerH = CreateSampler(gfx.device, skySamplerDesc);

	// BindGroups for globals
	for (u32 i = 0; i < ARRAY_COUNT(gfx.globalBindGroups); ++i)
	{
		gfx.globalBindGroups[i] = CreateBindGroup(gfx.device, gfx.globalBindGroupLayout, gfx.globalBindGroupAllocator);
	}

	gfx.shouldUpdateGlobalBindGroups = true;

	// Timestamp queries
	for (u32 i = 0; i < ARRAY_COUNT(gfx.timestampPools); ++i)
	{
		gfx.timestampPools[i] = CreateTimestampPool(gfx.device, 128);
		//ResetTimestampPool(gfx.device, gfx.timestampPools[i]); // Vulkan 1.2
	}

	LinkHandles(gfx);

#if USE_UI
	UIIcon *icons = nullptr;
	u32 iconCount = 0;
#if USE_EDITOR
	const char *iconFilenames[] = { "editor/open.png", "editor/save.png", "editor/reload.png" };
	iconCount = ARRAY_COUNT(iconFilenames);
	icons = PushArray(globalArena, UIIcon, iconCount);
	for (u32 i = 0; i < iconCount; ++i) {
		FilePath filepath = MakePath(ProjectDir, iconFilenames[i]);
		ReadImagePixels(filepath.str, icons[i].image);
	}
#endif
	UI_Initialize(engine.ui, gfx, gfx.device, globalArena, icons, iconCount);
#endif

	gfx.deviceInitialized = true;

	return true;
}

BindGroupDesc GlobalBindGroupDesc(const Graphics &gfx, u32 frameIndex)
{
	const BindGroupDesc bindGroupDesc = {
		.layout = gfx.globalBindGroupLayout,
		.bindings = {
			{ .index = BINDING_GLOBALS, .buffer = gfx.globalsBuffer[frameIndex] },
			{ .index = BINDING_SAMPLER_POINT, .sampler = gfx.pointSamplerH },
			{ .index = BINDING_SAMPLER_LINEAR, .sampler = gfx.linearSamplerH },
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
	for (u32 materialIndex = 0; materialIndex < gfx.materialHandles.handleCount; ++materialIndex)
	{
		const MaterialH handle = GetHandleAt(gfx.materialHandles, materialIndex);
		const Material &material = GetMaterial(gfx, handle);
		const BindGroupDesc materialBindGroupDesc = MaterialBindGroupDesc(gfx, material);
		UpdateBindGroup(gfx.device, materialBindGroupDesc, gfx.materialBindGroups[handle.idx]);
	}
}

void UploadMaterialData(Graphics &gfx)
{
	CommandList commandList = BeginUploadCommandList(gfx);

	// Copy material info to buffer
	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		const Material &material = GetMaterial(gfx, handle);
		SMaterial shaderMaterial = { material.uvScale };
		StagedData staged = StageData(gfx, &shaderMaterial, sizeof(shaderMaterial));

		CopyBufferToBuffer(commandList, staged.buffer, staged.offset, gfx.materialBuffer, material.bufferOffset, sizeof(shaderMaterial));
	}

	EndUploadCommandList(gfx, commandList);
}

void CreateMaterialBindGroup(Graphics &gfx, MaterialH handle)
{
	const Material &material = GetMaterial(gfx, handle);
	const Pipeline &pipeline = GetPipeline(gfx.device, material.pipelineH);
	gfx.materialBindGroups[handle.idx] = CreateBindGroup(gfx.device, pipeline.layout.bindGroupLayouts[1], gfx.materialBindGroupAllocator);
	gfx.shouldUpdateMaterialBindGroups = true;
}

void CreateMaterialBindGroups(Graphics &gfx)
{
	// BindGroups for materials
	for (u32 i = 0; i < gfx.materialHandles.handleCount; ++i)
	{
		MaterialH handle = GetHandleAt(gfx.materialHandles, i);
		CreateMaterialBindGroup(gfx, handle);
	}
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

AssetDescriptors GetAssetDescriptors(Engine &engine)
{
	static TextureDesc textureDescs[MAX_TEXTURES];
	const u32 textureCount = engine.gfx.textureHandles.handleCount;
	for ( u32 i = 0; i < textureCount; ++i) {
		Handle handle = engine.gfx.textureHandles.handles[i];
		textureDescs[i] = GetTextureDesc(engine.gfx, handle);
	}

	static MaterialDesc materialDescs[MAX_MATERIALS];
	const u32 materialCount = engine.gfx.materialHandles.handleCount;
	for ( u32 i = 0; i < materialCount; ++i) {
		Handle handle = engine.gfx.materialHandles.handles[i];
		materialDescs[i] = GetMaterialDesc(engine.gfx, handle);
	}

	static EntityDesc entityDescs[MAX_ENTITIES];
	const u32 entityCount = engine.scene.entityHandles.handleCount;
	for ( u32 i = 0; i < entityCount; ++i) {
		Handle handle = engine.scene.entityHandles.handles[i];
		entityDescs[i] = GetEntityDesc(engine.scene, handle);
	}

	static AudioClipDesc audioClipDescs[MAX_AUDIO_CLIPS];
	const u32 audioClipCount = engine.audio.clipHandles.handleCount;
	for ( u32 i = 0; i < audioClipCount; ++i) {
		Handle handle = engine.audio.clipHandles.handles[i];
		audioClipDescs[i] = GetAudioClipDesc(engine.audio, handle);
	}

	const AssetDescriptors assetDescs = {
		.shaderDescs = nullptr, // shaderSourceDescs,
		.shaderDescCount = 0, //ARRAY_COUNT(shaderSourceDescs),
		.textureDescs = textureDescs,
		.textureDescCount = textureCount,
		.materialDescs = materialDescs,
		.materialDescCount = materialCount,
		.entityDescs = entityDescs,
		.entityDescCount = entityCount,
		.audioClipDescs = audioClipDescs,
		.audioClipDescCount = audioClipCount,
	};

	return assetDescs;
}

static bool PushDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount < ARRAY_COUNT(engine.dataArenaStates);
	if (ok)
	{
		engine.dataArenaStates[engine.dataArenaStateCount++] = engine.platform.dataArena;
	}
	return ok;
}

static bool PopDataArenaState(Engine &engine)
{
	const bool ok = engine.dataArenaStateCount > 0;
	if (ok)
	{
		engine.platform.dataArena = engine.dataArenaStates[--engine.dataArenaStateCount];
	}
	return ok;
}

#if USE_DATA_BUILD
void LoadSceneFromTxt(Engine &engine)
{
	if ( PushDataArenaState(engine) )
	{
		Arena &dataArena = engine.platform.dataArena;
		const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
		AssetDescriptors assetDescriptors = ParseDescriptors(descriptorsFilepath.str, dataArena);

		// Textures
		for (u32 i = 0; i < assetDescriptors.textureDescCount; ++i)
		{
			CreateTexture(engine.gfx, assetDescriptors.textureDescs[i]);
		}

		// Materials
		for (u32 i = 0; i < assetDescriptors.materialDescCount; ++i)
		{
			CreateMaterial(engine.gfx, assetDescriptors.materialDescs[i]);
		}

		// Entities
		for (u32 i = 0; i < assetDescriptors.entityDescCount; ++i)
		{
			CreateEntity(engine, assetDescriptors.entityDescs[i]);
		}

		// Audio clips
		for (u32 i = 0; i < assetDescriptors.audioClipDescCount; ++i)
		{
			CreateAudioClip(engine, assetDescriptors.audioClipDescs[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
	}
}

void SaveSceneToTxt(Engine &engine)
{
	const AssetDescriptors assetDescs = GetAssetDescriptors(engine);

	FilePath path = MakePath(AssetDir, "assets_new.txt");
	SaveAssetDescriptors(path.str, assetDescs);
}
#endif // USE_DATA_BUILD

void LoadShadersFromBin(Engine &engine)
{
	const FilePath filepath = MakePath(DataDir, "shaders.dat");
	engine.shaderAssets = OpenAssets(engine.platform.dataArena, filepath.str);
}

void LoadSceneFromBin(Engine &engine)
{
	if (PushDataArenaState(engine))
	{
		const FilePath filepath = MakePath(DataDir, "assets.dat");
		engine.assets = OpenAssets(engine.platform.dataArena, filepath.str);

		// Textures
		for (u32 i = 0; i < engine.assets.header.imageCount; ++i)
		{
			CreateTexture(engine.gfx, engine.assets.images[i]);
		}

		// Materials
		for (u32 i = 0; i < engine.assets.header.materialCount; ++i)
		{
			CreateMaterial(engine.gfx, engine.assets.materialDescs[i]);
		}

		// Entities
		for (u32 i = 0; i < engine.assets.header.entityCount; ++i)
		{
			CreateEntity(engine, engine.assets.entityDescs[i]);
		}

		// Audio clips
		for (u32 i = 0; i < engine.assets.header.audioClipCount; ++i)
		{
			CreateAudioClip(engine, engine.assets.audioClips[i]);
		}

		UploadMaterialData(engine.gfx);
		LinkHandles(engine.gfx);
	}
}

void CleanTexture(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveTexture(engine.gfx, handle, false);
}

void CleanMaterial(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveMaterial(engine.gfx, handle, false);
}

void CleanEntity(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveEntity(engine, handle, false);
}

void CleanAudioClip(Handle handle, void* data)
{
	Engine &engine = *(Engine*)data;
	RemoveAudioClip(engine, handle, false);
}

void CleanScene(Engine &engine)
{
	WaitDeviceIdle(engine.gfx.device);

	if (PopDataArenaState(engine))
	{
		ForAllHandles(engine.gfx.textureHandles, CleanTexture, &engine);
		FreeAllHandles(engine.gfx.textureHandles);
		ForAllHandles(engine.gfx.materialHandles, CleanMaterial, &engine);
		FreeAllHandles(engine.gfx.materialHandles);
		ForAllHandles(engine.scene.entityHandles, CleanEntity, &engine);
		FreeAllHandles(engine.scene.entityHandles);
		ForAllHandles(engine.audio.clipHandles, CleanAudioClip, &engine);
		FreeAllHandles(engine.audio.clipHandles);
		CloseAssets(engine.assets);
		ResetBindGroupAllocator( engine.gfx.device, engine.gfx.materialBindGroupAllocator );
	}
}

#if USE_DATA_BUILD
void BuildShaders(Engine &engine, const char *outBinFilepath)
{
	CompileShaders();

	Arena scratch = MakeSubArena(engine.platform.dataArena, "Scratch - BuildShaders");
	AssetDescriptors assetDescriptors = {};
	assetDescriptors.shaderDescs = shaderSourceDescs;
	assetDescriptors.shaderDescCount = ARRAY_COUNT(shaderSourceDescs);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}

void BuildAssetsFromTxt(Engine &engine, const char *inTxtFilepath, const char *outBinFilepath)
{
	Arena scratch = MakeSubArena(engine.platform.dataArena, "Scratch - BuildAssetsFromTxt");
	AssetDescriptors assetDescriptors = ParseDescriptors(inTxtFilepath, scratch);
	BuildAssets(assetDescriptors, outBinFilepath, scratch);
}
#endif // USE_DATA_BUILD

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


bool RenderGraphics(Engine &engine)
{
	Scene &scene = engine.scene;
	Graphics &gfx = engine.gfx;
	Window &window = engine.platform.window;
#if USE_EDITOR
	Editor &editor = engine.editor;
#endif

	static f32 totalSeconds = 0.0f;
	totalSeconds += gfx.deltaSeconds;

	u32 frameIndex = gfx.device.frameIndex;

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
	float4x4 inverseViewMatrix = Eye();
	float4x4 viewportRotationMatrix = Eye();
	float4x4 projectionMatrix = Eye();
	float4 frustumTopLeft = {};
	float4 frustumBottomRight = {};
	float3 cameraPosition = {};

	Camera camera = {};
#if USE_EDITOR
	if (engine.mode == EngineModeEditor3D) {
		camera = engine.editor.camera[ProjectionPerspective];
	} else if (engine.mode == EngineModeEditor2D) {
		camera = engine.editor.camera[ProjectionOrthographic];
	}
#endif
	if (engine.mode == EngineModeGame3D) {
		camera = {
			.projectionType = ProjectionPerspective,
			.position = {0, 1, 2},
			.orientation = {0, -0.45f},
		};
	} else if (engine.mode == EngineModeGame2D) {
		// TODO: Set more appropriate values
		camera = {
			.projectionType = ProjectionPerspective,
			.position = {0, 1, 2},
			.orientation = {0, -0.45f},
		};
	}

	if (camera.projectionType == ProjectionPerspective)
	{
		// Calculate camera matrices
		const float preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
		ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
		const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
		const f32 ar = isLandscapeRotation ?  displayWidth / displayHeight : displayHeight / displayWidth;
		viewMatrix = ViewMatrixFromCamera(camera);
		inverseViewMatrix = Float4x4(Transpose(Float3x3(viewMatrix)));
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
		for (u32 i = 0; i < scene.entityHandles.handleCount; ++i)
		{
			Entity &entity = GetEntityAt(scene, i);
			entity.culled = !EntityIsInFrustum(entity, frustumPlanes);
		}
	}
	else
	{
		const f32 height = camera.height;
		// Calculate camera matrices
		const f32 preRotationDegrees = gfx.device.swapchain.preRotationDegrees;
		ASSERT(preRotationDegrees == 0 || preRotationDegrees == 90 || preRotationDegrees == 180 || preRotationDegrees == 270);
		const bool isLandscapeRotation = preRotationDegrees == 0 || preRotationDegrees == 180;
		const f32 ar = isLandscapeRotation ?  displayWidth / displayHeight : displayHeight / displayWidth;
		viewMatrix = ViewMatrixFromCamera(camera);
		//inverseViewMatrix = Inverse2D(viewMatrix);
		viewportRotationMatrix = Rotate(float3{0.0, 0.0, 1.0}, preRotationDegrees);
		//const float4x4 preTransformMatrixInv = Float4x4(Transpose(Float3x3(viewportRotationMatrix)));
		const float4x4 orthographicMatrix = Orthogonal(-height*ar, height*ar, -height, height, -10.0, 10.0);
		projectionMatrix = Mul(viewportRotationMatrix, orthographicMatrix);

		// Frustum vectors
		frustumTopLeft = Float4( Float3(-height*ar, height, 0), 0.0f );
		frustumBottomRight = Float4( Float3(height*ar, -height, 0), 0.0f );

		cameraPosition = camera.position;

		// CPU Frustum culling
		for (u32 i = 0; i < scene.entityHandles.handleCount; ++i)
		{
			Entity &entity = GetEntityAt(scene, i);
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

	// Camera UI 2D
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
		.cameraViewInv = inverseViewMatrix,
		.cameraProj = projectionMatrix,
		.camera2dProj = camera2dProjection, // UI
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
#if USE_EDITOR
		.selectedEntity = editor.selectedEntity,
#endif
	};

	// Update globals buffer
	Globals *globalsBufferPtr = (Globals*)GetBufferPtr(gfx.device, gfx.globalsBuffer[frameIndex]);
	*globalsBufferPtr = globals;

	// Update tile grids
	if (scene.tileGrid.needsUpdate)
	{
		CommandList commandList = BeginUploadCommandList(gfx);

		const TileGrid &grid = scene.tileGrid;

		Arena scratch = engine.platform.frameArena;

		// Count vertices and indices
		u32 vertexCount = 0;
		u32 indexCount = 0;
		for (u32 x = 0; x < TILE_GRID_SIZE_X; ++x) {
			for (u32 y = 0; y < TILE_GRID_SIZE_Y; ++y) {
				if (grid.tiles[x][y].value != 0) {
					vertexCount += 4;
					indexCount += 6;
				}
			}
		}

		scene.tileGrid.indexCount = indexCount;

		const f32 atlasSize = scene.tileAtlas.size;
		const f32 tileSize = scene.tileAtlas.tileSize;
		const f32 du = tileSize / atlasSize;
		const f32 dv = tileSize / atlasSize;
		const u32 atlasCellCountX = (u32)atlasSize / (u32)tileSize;

		Vertex *tileVertices = PushArray(scratch, Vertex, vertexCount);
		Index *tileIndices = PushArray(scratch, Index, indexCount);
		const u32 tileVertexSize = vertexCount * sizeof(Vertex);
		const u32 tileIndexSize = indexCount * sizeof(Index);

		Vertex *vertex = tileVertices;
		Index *index = tileIndices;
		u32 indexOffset = 0;
		for (u32 x = 0; x < TILE_GRID_SIZE_X; ++x) {
			for (u32 y = 0; y < TILE_GRID_SIZE_Y; ++y) {
				if (grid.tiles[x][y].value != 0) {
					const f32 atlasX = grid.tiles[x][y].tileId % atlasCellCountX;
					const f32 atlasY = grid.tiles[x][y].tileId / atlasCellCountX;
					const f32 u0 = atlasX * du;
					const f32 u1 = u0 + du;
					const f32 v0 = atlasY * dv;
					const f32 v1 = v0 + dv;
					*vertex++ = Vertex{{(f32)(x + 0), (f32)(y + 0), 0}, {0,0,0}, {u0,v1}};
					*vertex++ = Vertex{{(f32)(x + 1), (f32)(y + 0), 0}, {0,0,0}, {u1,v1}};
					*vertex++ = Vertex{{(f32)(x + 1), (f32)(y + 1), 0}, {0,0,0}, {u1,v0}};
					*vertex++ = Vertex{{(f32)(x + 0), (f32)(y + 1), 0}, {0,0,0}, {u0,v0}};
					*index++ = indexOffset + 0;
					*index++ = indexOffset + 1;
					*index++ = indexOffset + 2;
					*index++ = indexOffset + 0;
					*index++ = indexOffset + 2;
					*index++ = indexOffset + 3;
					indexOffset += 4;
				}
			}
		}

		WaitDeviceIdle(gfx.device);
		UploadData(gfx, commandList, tileVertices, tileVertexSize, scene.tileGrid.vertices.buffer, scene.tileGrid.vertices.offset);
		UploadData(gfx, commandList, tileIndices, tileIndexSize, scene.tileGrid.indices.buffer, scene.tileGrid.indices.offset);
		scene.tileGrid.needsUpdate = false;

		EndUploadCommandList(gfx, commandList);
	}

	// Update entity data
	SEntity *entities = (SEntity*)GetBufferPtr(gfx.device, gfx.entityBuffer[frameIndex]);
	for (u32 i = 0; i < scene.entityHandles.handleCount; ++i)
	{
		const Handle handle = GetHandleAt(scene.entityHandles, i);
		const Entity &entity = GetEntity(scene, handle);
		const float4x4 worldMatrix = Mul(Translate(entity.position), Scale(Float3(entity.scale))); // TODO: Apply also rotation
		entities[handle.idx].world = worldMatrix;
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

	// Shadow map
	if (IsEngineMode3D(engine.mode))
	{
		BeginDebugGroup(commandList, "Shadow map");

		SetClearDepth(commandList, 0, 0.0f);

		const Framebuffer shadowmapFramebuffer = GetShadowmapFramebuffer(gfx);
		BeginRenderPass(commandList, shadowmapFramebuffer);

		const uint2 shadowmapSize = GetFramebufferSize(shadowmapFramebuffer);
		SetViewportAndScissor(commandList, shadowmapSize);

		SetPipeline(commandList, gfx.shadowmapPipelineH);

		SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);

		for (u32 entityIndex = 0; entityIndex < scene.entityHandles.handleCount; ++entityIndex)
		{
			const Handle handle = GetHandleAt(scene.entityHandles, entityIndex);
			const Entity &entity = GetEntity(scene, handle);

			if ( !entity.visible ) continue;

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, handle.idx);
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

		// TileGrid
		for (u32 i = 0; i < scene.tileGridCount; ++i)
		{
			const TileGrid &tileGrid = scene.tileGrid;
			const TileAtlas &tileAtlas = scene.tileAtlas;
			const MaterialH materialH = tileAtlas.materialH;
			const Material &material = GetMaterial(gfx, materialH);

			BeginDebugGroup(commandList, "TileGrid");

			// Pipeline
			SetPipeline(commandList, material.pipelineH);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[materialH.idx]);

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = tileGrid.indexCount;
			const uint32_t firstIndex = tileGrid.indices.offset/sizeof(Index);
			const int32_t firstVertex = tileGrid.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

			EndDebugGroup(commandList);
		}

		// Entities
		for (u32 entityIndex = 0; entityIndex < scene.entityHandles.handleCount; ++entityIndex)
		{
			const Handle handle = GetHandleAt(scene.entityHandles, entityIndex);
			const Entity &entity = GetEntityAt(scene, entityIndex);

			if ( !entity.visible || entity.culled ) continue;

			const MaterialH materialH = entity.materialH;
			const Material &material = GetMaterial(gfx, materialH);

			BeginDebugGroup(commandList, material.name);

			// Pipeline
			SetPipeline(commandList, material.pipelineH);

			// Bind groups
			SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
			SetBindGroup(commandList, 1, gfx.materialBindGroups[materialH.idx]);

			// Geometry
			SetVertexBuffer(commandList, vertexBuffer);
			SetIndexBuffer(commandList, indexBuffer);

			// Draw!!!
			const uint32_t indexCount = entity.indices.size/sizeof(Index);
			const uint32_t firstIndex = entity.indices.offset/sizeof(Index);
			const int32_t firstVertex = entity.vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same
			DrawIndexed(commandList, indexCount, firstIndex, firstVertex, handle.idx);

			EndDebugGroup(commandList);
		}

		// Sky
		if (IsEngineMode3D(engine.mode))
		{
			const ImageH &skyImage = GetTextureImage(gfx, gfx.skyTextureH, gfx.grayImageH);
			const Pipeline &pipeline = GetPipeline(gfx.device, gfx.skyPipelineH);
			const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
			const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
			const uint32_t indexCount = indices.size/sizeof(Index);
			const uint32_t firstIndex = indices.offset/sizeof(Index);
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

#if USE_EDITOR
		// TileGrid
		if (editor.showGrid)
		{
			if (IsEngineMode3D(engine.mode))
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_3d");

				SetPipeline(commandList, gfx.grid3dPipelineH);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
			else // if (IsEngineMode2D(engine.mode))
			{
				const BufferChunk indices = GetIndicesForGeometryType(gfx, GeometryTypeScreen);
				const BufferChunk vertices = GetVerticesForGeometryType(gfx, GeometryTypeScreen);
				const uint32_t indexCount = indices.size/sizeof(Index);
				const uint32_t firstIndex = indices.offset/sizeof(Index);
				const int32_t firstVertex = vertices.offset/sizeof(Vertex); // assuming all vertices in the buffer are the same

				BeginDebugGroup(commandList, "grid_2d");

				SetPipeline(commandList, gfx.grid2dPipelineH);
				SetBindGroup(commandList, 0, gfx.globalBindGroups[frameIndex]);
				SetVertexBuffer(commandList, vertexBuffer);
				SetIndexBuffer(commandList, indexBuffer);
				DrawIndexed(commandList, indexCount, firstIndex, firstVertex, 0);

				EndDebugGroup(commandList);
			}
		}
#endif

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
	EditorRender(engine, commandList);
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

	const TimestampPool &timestampPool = gfx.timestampPools[gfx.device.frameIndex];
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

#if USE_EDITOR
	CompileModifiedShaders();
#endif

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

	// Initialize sound system
	if ( !InitializeAudio(engine.audio, platform.globalArena) )
	{
		LOG(Error, "InitializeAudio failed!\n");
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
		if (!sLoadShadersFromText) {
			LoadShadersFromBin(engine);
		}

		if ( !InitializeGraphics(engine, platform.globalArena, platform.frameArena) )
		{
			// TODO: Actually we could throw a system error and exit...
			LOG(Error, "InitializeGraphics failed!\n");
			return false;
		}

		Initialize(engine.scene.entityHandles, platform.globalArena, MAX_ENTITIES);

#if USE_EDITOR
		EditorInitialize(engine);
#else
		engine.mode = EngineModeGame3D;
		LoadSceneFromBin(engine);
#endif
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

	const Clock begin = GetClock();

#if 0
	// TODO(jesus): This is test code
	static bool firstUpdate = true;
	if ( firstUpdate )
	{
		firstUpdate = false;
		LoadSceneFromBin(engine);
		PlayAudioClip(engine, 1);
	}
#endif

#if USE_IMGUI
	UpdateImGui(gfx);
#endif

#if USE_UI
	UIBeginFrameRecording(engine);
#endif

#if USE_EDITOR
	static Clock lastClock = GetClock();
	const Clock currentClock = GetClock();
	const f32 secondsSinceLastCheck  = GetSecondsElapsed(lastClock, currentClock);

	if ( secondsSinceLastCheck > 0.2 )
	{
		lastClock = currentClock;

		if ( CompileModifiedShaders() )
		{
			// NOTE(jesus): Recompiling all pipelines here even if likely only a shader was recompiled :-S
			WaitDeviceIdle(gfx.device);
			RecompilePipelines(engine, platform.frameArena);
		}

		RecreateModifiedTextures(engine);
	}

	EditorUpdate(engine);
#endif

	GameUpdate(engine);

#if USE_UI
	UIEndFrameRecording(engine);
#endif

	const Clock end = GetClock();
	const f32 updateMillis = GetSecondsElapsed(begin, end) * 1000.0f;
	AddTimeSample( gfx.cpuFrameTimes, updateMillis );
}

void OnPlatformRenderGraphics(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	static Clock lastClock = GetClock();
	Clock currentClock = GetClock();
	gfx.deltaSeconds = GetSecondsElapsed(lastClock, currentClock);
	lastClock = currentClock;

	if ( !gfx.deviceInitialized )
	{
		return;
	}

	if ( !IsValidSwapchain(gfx.device) || platform.window.flags & WindowFlags_WasResized )
	{
		WaitDeviceIdle(gfx);
		DestroyRenderTargets(gfx, gfx.renderTargets);
		DestroySwapchain(gfx.device);
		if ( platform.window.width != 0 && platform.window.height != 0 )
		{
			CreateSwapchain(gfx.device, platform.window);
			gfx.renderTargets = CreateRenderTargets(gfx);
			gfx.shouldUpdateGlobalBindGroups = true;
		}
	}

	if ( IsValidSwapchain(gfx.device) )
	{
		RenderGraphics(engine);

#if USE_EDITOR
		EditorPostRender(engine);
#endif
	}
}

void OnPlatformWindowCleanup(Platform &platform)
{
	Engine &engine = GetEngine(platform);
	Graphics &gfx = engine.gfx;

	WaitDeviceIdle(gfx);
	DestroyRenderTargets(gfx, gfx.renderTargets);
	DestroySwapchain(gfx.device);
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

	// TODO(jesus): Remove, already handled OnPlatformWindowCleanup
	//DestroyRenderTargets(gfx, gfx.renderTargets);
	//DestroySwapchain(gfx.device);
	//CleanupGraphicsSurface(gfx.device);

	CleanupGraphics(gfx);
}


void EngineMain( int argc, char **argv,  void *userData )
{
	Engine engine = {};

	// Memory
	engine.platform.stringMemorySize = KB(16);
	engine.platform.dataMemorySize = MB(64);
	engine.platform.globalMemorySize = MB(64);
	engine.platform.frameMemorySize = MB(16);

	// Callbacks
	engine.platform.InitCallback = OnPlatformInit;
	engine.platform.UpdateCallback = OnPlatformUpdate;
	engine.platform.RenderGraphicsCallback = OnPlatformRenderGraphics;
	engine.platform.PreRenderAudioCallback = OnPlatformPreRenderAudio;
	engine.platform.RenderAudioCallback = OnPlatformRenderAudio;
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
	bool buildAssets = false;
	bool exitAfterBuild = false;
	for ( u32 i = 0; i < argc; ++i ) {
		if ( StrEq(argv[i], "--build-assets") ) {
			buildAssets = true;
			exitAfterBuild = true;
		}
	}

	const FilePath assetsFilepath = MakePath(DataDir, "assets.dat");
	if ( !ExistsFile(assetsFilepath.str) ) {
		buildAssets = true;
	}

	if ( buildAssets ) {
		const FilePath shadersFilepath = MakePath(DataDir, "shaders.dat");
		BuildShaders(engine, shadersFilepath.str);
		const FilePath descriptorsFilepath = MakePath(AssetDir, "assets.txt");
		BuildAssetsFromTxt(engine, descriptorsFilepath.str, assetsFilepath.str);
		if (exitAfterBuild) {
			return;
		}
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
	return 0;
}
#endif

// Implementations

#include "data.cpp"
#include "audio.cpp"

#if USE_EDITOR
#include "editor.cpp"
#endif
#include "ibxm/ibxm.c"

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

