#ifndef ASSETS_H
#define ASSETS_H

// Descriptor type definitions

#if 0 // Types to be parsed for C reflection
struct float3 { float x; float y; float z; };
#endif

struct TextureDesc
{
	const char *name;
	const char *filename;
};

enum LoadOp
{
	LoadOpLoad,
	LoadOpClear,
	LoadOpDontCare,
};

enum StoreOp
{
	StoreOpStore,
	StoreOpDontCare,
};

struct AttachmentDesc
{
	LoadOp loadOp;
	StoreOp storeOp;
};

struct RenderpassDesc
{
	const char *name;
	unsigned char colorAttachmentCount;
	AttachmentDesc colorAttachments[4];
	bool hasDepthAttachment;
	AttachmentDesc depthAttachment;
};

enum Format
{
	FormatFloat2,
	FormatFloat3,
	FormatCount,
};

struct VertexAttributeDesc
{
	unsigned int bufferIndex;
	unsigned int location;
	unsigned int offset;
	Format format;
};

struct PipelineDesc
{
	const char *name;
	const char *vsFilename;
	const char *fsFilename;
	const char *vsFunction;
	const char *fsFunction;
	const char *renderPass;
	unsigned int vertexAttributeCount;
	VertexAttributeDesc vertexAttributes[4];
};

struct ComputeDesc
{
	const char *name;
	const char *filename;
	const char *function;
};

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
};

struct EntityDesc
{
	const char *name;
	const char *materialName;
	float3 pos;
	float scale;
	GeometryType geometryType;
};

struct Assets
{
	const TextureDesc *textures;
	unsigned int texturesCount;

	const PipelineDesc *pipelines;
	unsigned int pipelinesCount;

	const ComputeDesc *computes;
	unsigned int computesCount;

	const MaterialDesc *materials;
	unsigned int materialsCount;

	const EntityDesc *entities;
	unsigned int entitiesCount;

	int arrayTest[2];
};


#ifdef ASSETS_IMPLEMENTATION

// Descriptor definitions

static const TextureDesc textures[] =
{
	{ .name = "tex_diamond", .filename = "assets/diamond.png" },
	{ .name = "tex_dirt",    .filename = "assets/dirt.jpg" },
	{ .name = "tex_grass",   .filename = "assets/grass.jpg" },
};

static const PipelineDesc pipelines[] =
{
	{
		.name = "pipeline_lit",
		.vsFilename = "shaders/vertex.spv",
		.fsFilename = "shaders/fragment.spv",
		.renderPass = "main_renderpass",
		.vertexAttributeCount = 3,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 1, .offset = 12, .format = FormatFloat3, },
			{ .bufferIndex = 0, .location = 2, .offset = 24, .format = FormatFloat2, },
		}
	},
	{
		.name = "pipeline_shadowmap",
		.vsFilename = "shaders/vs_shadowmap.spv",
		.fsFilename = "shaders/fs_shadowmap.spv",
		.renderPass = "shadowmap_renderpass",
		.vertexAttributeCount = 1,
		.vertexAttributes = {
			{ .bufferIndex = 0, .location = 0, .offset = 0, .format = FormatFloat3, },
		}
	},
};

static const ComputeDesc computes[] =
{
	{ .name = "compute_clear", .filename = "shaders/compute_clear.spv", .function = "main_clear" },
	{ .name = "compute_update", .filename = "shaders/compute_update.spv", .function = "main_update" },
};

static const MaterialDesc materials[] =
{
	{ .name = "mat_diamond", .textureName = "tex_diamond", .pipelineName = "pipeline_lit", .uvScale = 1.0f },
	{ .name = "mat_dirt",    .textureName = "tex_dirt",    .pipelineName = "pipeline_lit", .uvScale = 1.0f },
	{ .name = "mat_grass",   .textureName = "tex_grass",   .pipelineName = "pipeline_lit", .uvScale = 11.0f },
};

static const EntityDesc entities[] =
{
	{ .name = "ent_cube0", .materialName = "mat_diamond", .pos = { 1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube1", .materialName = "mat_diamond", .pos = { 1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube2", .materialName = "mat_dirt",    .pos = {-1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube3", .materialName = "mat_dirt",    .pos = {-1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_plane", .materialName = "mat_grass",   .pos = { 0, -0.5, 0}, .scale = 11, .geometryType = GeometryTypePlane},
};

static const Assets gAssets =
{
	.textures = textures,
	.texturesCount = ARRAY_COUNT(textures),

	.pipelines = pipelines,
	.pipelinesCount = ARRAY_COUNT(pipelines),

	.computes = computes,
	.computesCount = ARRAY_COUNT(computes),

	.materials = materials,
	.materialsCount = ARRAY_COUNT(materials),

	.entities = entities,
	.entitiesCount = ARRAY_COUNT(entities),

	.arrayTest = {4, 5},
};

#endif // ASSETS_IMPLEMENTATION

#include "../reflex.h"
#include "../assets.reflex.h"

#endif // ASSETS_H
