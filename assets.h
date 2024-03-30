#ifndef ASSETS_H
#define ASSETS_H

struct TextureDesc
{
	const char *filename;
	TextureH handle; // output
};

struct MaterialDesc
{
	const uint textureIndex;
	const float uvScale;
	MaterialH handle; // output
};

enum GeometryType
{
	GeometryTypeCube,
	GeometryTypePlane,
};

struct EntityDesc
{
	float3 pos;
	float scale;
	const uint materialIndex;
	const GeometryType geometryType;
};

TextureDesc textures[] = {
	{"assets/diamond.png"},
	{"assets/dirt.jpg"},
	{"assets/grass.jpg"},
};

MaterialDesc materials[] = {
	{ 0, 1.0f },
	{ 1, 1.0f },
	{ 2, 11.0f },
};

const EntityDesc entities[] = {
	{{ 1, 0,  1}, 1, 0, GeometryTypeCube},
	{{ 1, 0, -1}, 1, 0, GeometryTypeCube},
	{{-1, 0,  1}, 1, 1, GeometryTypeCube},
	{{-1, 0, -1}, 1, 1, GeometryTypeCube},
	{{ 0, -0.5,  0}, 11, 2, GeometryTypePlane},
};

struct Assets
{
	TextureDesc *textures = ::textures;
	uint textureCount = ARRAY_COUNT(::textures);

	MaterialDesc *materials = ::materials;
	uint materialCount = ARRAY_COUNT(::materials);

	const EntityDesc *entities = ::entities;
	uint entityCount = ARRAY_COUNT(::entities);
};

static Assets gAssets = {};

#endif // ASSETS_H
