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
	int mipmap;
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

struct Assets
{
	const TextureDesc *textures;
	unsigned int texturesCount;

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

static const Assets gAssets =
{
	.textures = textures,
	.texturesCount = ARRAY_COUNT(textures),

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
