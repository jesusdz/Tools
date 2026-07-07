#ifndef ENGINE_H
#define ENGINE_H

typedef u16 Index;

struct Vertex
{
	float3 pos;
	float3 normal;
	float2 texCoord;
};

struct DebugDrawVertex
{
	float2 pos;
	rgba color;
};

struct Texture
{
	const char *name;
	ImageH image;
	uint2 size;
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
	uint2 sceneSize;
	ImageH depthImage;
	ImageH sceneImage;
	Framebuffer sceneFramebuffer;

	Framebuffer displayFramebuffers[MAX_SWAPCHAIN_IMAGE_COUNT];

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

struct Sprite
{
	const char *name;
	TextureH textureH;
	uint2 pos;
	uint2 size;
	u32 frameCount;
	u32 fps;
	bool loop;
};

typedef Handle SpriteH;

struct SpriteAnimState
{
	f32 elapsedTime;
	u32 currentFrame;
};

struct Entity
{
	const char *name;
	float3 position;
	float scale;
	bool visible;
	bool culled;
	// 3D entity
	GeometryType geometryType;
	BufferChunk vertices;
	BufferChunk indices;
	MaterialH materialH;
	// Sprite entity
	SpriteH spriteH;
	i32 layer;
};

#define MAX_TIME_SAMPLES 32
struct TimeSamples
{
	f32 samples[MAX_TIME_SAMPLES];
	u32 sampleCount;
	f32 average;
};

#define MAX_TEXTURES 4092
#define MAX_MATERIALS 4092
#define MAX_DYNAMIC_BIND_GROUPS 4092

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
	BufferChunk quadVertices;
	BufferChunk quadIndices;
	BufferChunk spriteVertices;
	BufferChunk spriteIndices;
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

	BufferH debugDrawVertexBuffer[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVertices[MAX_FRAMES_IN_FLIGHT];
	DebugDrawVertex *debugDrawVerticesCPU;
	u32 debugDrawVertexCount;

	BufferH spriteDataBuffer[MAX_FRAMES_IN_FLIGHT];

	SamplerH pointSamplerH;
	SamplerH linearSamplerH;
	SamplerH shadowmapSamplerH;
	SamplerH skySamplerH;
	SamplerH screenSamplerH;

	RenderPassH litRenderPassH;
	RenderPassH shadowmapRenderPassH;
	RenderPassH idRenderPassH;
	RenderPassH displayRenderPassH;

	Texture textures[MAX_TEXTURES];
	TextureDesc textureDescs[MAX_TEXTURES];
	HandleManager textureHandles;

	Material materials[MAX_MATERIALS];
	MaterialDesc materialDescs[MAX_MATERIALS];
	HandleManager materialHandles;
	bool shouldUpdateMaterials;

	BindGroupAllocator globalBindGroupAllocator;
	BindGroupAllocator materialBindGroupAllocator;
	BindGroupAllocator dynamicBindGroupAllocator[MAX_FRAMES_IN_FLIGHT];

	BindGroupLayout globalBindGroupLayout;

	// Updated each frame so we need MAX_FRAMES_IN_FLIGHT elements
	BindGroup globalBindGroups[MAX_FRAMES_IN_FLIGHT];
	bool shouldUpdateGlobalBindGroups;

	// Updated once at the beginning for each material
	BindGroup materialBindGroups[MAX_MATERIALS];
	bool shouldUpdateMaterialBindGroups;

	BindGroupDesc dynamicBindGroupDescs[MAX_DYNAMIC_BIND_GROUPS];
	BindGroup dynamicBindGroups[MAX_DYNAMIC_BIND_GROUPS];
	u32 dynamicBindGroupCount;

	TimestampPool timestampPools[MAX_FRAMES_IN_FLIGHT];

	ImageH whiteImageH;
	ImageH pinkImageH;
	ImageH grayImageH;
	ImageH blackImageH;

	TextureH skyTextureH;
	TextureH defaultTexture;

	MaterialH defaultMaterial;

	PipelineH shadowmapPipelineH;
	PipelineH skyPipelineH;
	PipelineH spritePipelineH;
	PipelineH blitPipelineH;
	PipelineH guiPipelineH;
#if USE_EDITOR
	PipelineH grid2dPipelineH;
	PipelineH grid3dPipelineH;
	PipelineH modelIdPipelineH;
	PipelineH spriteIdPipelineH;
#endif
	PipelineH debugDrawPipelineH;

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

	const Camera *activeCamera;
};

//#define TILE_GRID_SIZE_X 20
//#define TILE_GRID_SIZE_Y 15
#define TILE_GRID_SIZE_X 40
#define TILE_GRID_SIZE_Y 30
#define TILE_SIZE_PIXELS 16.0f // size of each grid cell, in pixels (at PIXELS_PER_METER scale)

struct TileGrid
{
	Handle entities[TILE_GRID_SIZE_X][TILE_GRID_SIZE_Y]; // sprite entity per cell, InvalidHandle if empty
	uint2 size;
};

struct Layer
{
	bool initialized;
	const char *name;
	TileGrid grid;
	i32 order; // draw order within the room, lower values drawn first (further back)
	bool visible;
};

#define MAX_LAYERS 4

struct Room
{
	const char *name;
	int2 pos;
	Layer layers[MAX_LAYERS];
	u32 layerCount;
};

#define MAX_ENTITIES 4092
#define MAX_SPRITES 4092
#define MAX_ROOMS 256

constexpr u32 SCENE_WIDTH = 320;
constexpr u32 SCENE_HEIGHT = 180;

struct Scene
{
	Room rooms[MAX_ROOMS];
	HandleManager roomHandles;

	Entity entities[MAX_ENTITIES];
	HandleManager entityHandles;

	Sprite sprites[MAX_SPRITES];
	HandleManager spriteHandles;

	SpriteAnimState spriteAnimStates[MAX_SPRITES];
};

#endif // ENGINE_H
