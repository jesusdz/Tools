#ifndef ASSETS_H
#define ASSETS_H

// Descriptor type definitions

struct TextureDesc
{
	const char *name;
	const char *filename;
};

struct MaterialDesc
{
	const char *name;
	const char *textureName;
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
	uint textureCount;

	const MaterialDesc *materials;
	uint materialCount;

	const EntityDesc *entities;
	uint entityCount;
};



// Descriptor definitions

static const TextureDesc textures[] =
{
	{"tex_diamond", "assets/diamond.png"},
	{"tex_dirt", "assets/dirt.jpg"},
	{"tex_grass", "assets/grass.jpg"},
};

static const MaterialDesc materials[] =
{
	{ "mat_diamond", "tex_diamond", 1.0f },
	{ "mat_dirt", "tex_dirt", 1.0f },
	{ "mat_grass", "tex_grass", 11.0f },
};

static const EntityDesc entities[] =
{
	{"ent_cube0", "mat_diamond", { 1, 0,  1}, 1, GeometryTypeCube},
	{"ent_cube1", "mat_diamond", { 1, 0, -1}, 1, GeometryTypeCube},
	{"ent_cube2", "mat_dirt", {-1, 0,  1}, 1, GeometryTypeCube},
	{"ent_cube3", "mat_dirt", {-1, 0, -1}, 1, GeometryTypeCube},
	{"ent_plane", "mat_grass", { 0, -0.5,  0}, 11, GeometryTypePlane},
};

static const Assets gAssets =
{
	.textures = textures,
	.textureCount = ARRAY_COUNT(textures),

	.materials = materials,
	.materialCount = ARRAY_COUNT(materials),

	.entities = entities,
	.entityCount = ARRAY_COUNT(entities),
};

#endif // ASSETS_H
