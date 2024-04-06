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

struct PipelineDesc
{
	const char *name;
	const char *vsFilename;
	const char *fsFilename;
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

	const MaterialDesc *materials;
	unsigned int materialsCount;

	const EntityDesc *entities;
	unsigned int entitiesCount;

	const int numbers[5];

	const char *names[3];

	const char **names2;
	unsigned int names2Count;
};



// Descriptor definitions

static const TextureDesc textures[] =
{
	{ .name = "tex_diamond", .filename = "assets/diamond.png" },
	{ .name = "tex_dirt",    .filename = "assets/dirt.jpg" },
	{ .name = "tex_grass",   .filename = "assets/grass.jpg" },
};

static const PipelineDesc pipelines[] =
{
	{ .name = "pipeline", .vsFilename = "shaders/vertex.spv", .fsFilename = "shaders/fragment.spv" },
};

static const MaterialDesc materials[] =
{
	{ .name = "mat_diamond", .textureName = "tex_diamond", .pipelineName = "pipeline", .uvScale = 1.0f },
	{ .name = "mat_dirt",    .textureName = "tex_dirt",    .pipelineName = "pipeline", .uvScale = 1.0f },
	{ .name = "mat_grass",   .textureName = "tex_grass",   .pipelineName = "pipeline", .uvScale = 11.0f },
};

static const EntityDesc entities[] =
{
	{ .name = "ent_cube0", .materialName = "mat_diamond", .pos = { 1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube1", .materialName = "mat_diamond", .pos = { 1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube2", .materialName = "mat_dirt",    .pos = {-1, 0,  1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_cube3", .materialName = "mat_dirt",    .pos = {-1, 0, -1},   .scale = 1,  .geometryType = GeometryTypeCube},
	{ .name = "ent_plane", .materialName = "mat_grass",   .pos = { 0, -0.5, 0}, .scale = 11, .geometryType = GeometryTypePlane},
};

const char *names2[] = { "Jesus", "Pedro", "Marcos" };

static const Assets gAssets =
{
	.textures = textures,
	.texturesCount = ARRAY_COUNT(textures),

	.pipelines = pipelines,
	.pipelinesCount = ARRAY_COUNT(pipelines),

	.materials = materials,
	.materialsCount = ARRAY_COUNT(materials),

	.entities = entities,
	.entitiesCount = ARRAY_COUNT(entities),

	.numbers = { 1, 2, 3, 4, 5 },

	.names = { "Jesus", "Pedro", "Marcos" },

	.names2 = names2,
	.names2Count = ARRAY_COUNT(names2),
};

#endif // ASSETS_H
