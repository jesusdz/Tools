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

struct Assets
{
	TextureDesc textures[3] = {
		{"assets/diamond.png"},
		{"assets/dirt.jpg"},
		{"assets/grass.jpg"},
	};
	uint textureCount = 3;

	MaterialDesc materials[3] = {
		{ 0, 1.0f },
		{ 1, 1.0f },
		{ 2, 11.0f },
	};
	uint materialCount = 3;

	EntityDesc entities[5] = {
		{{ 1, 0,  1}, 1, 0, GeometryTypeCube},
		{{ 1, 0, -1}, 1, 0, GeometryTypeCube},
		{{-1, 0,  1}, 1, 1, GeometryTypeCube},
		{{-1, 0, -1}, 1, 1, GeometryTypeCube},
		{{ 0, -1,  0}, 11, 2, GeometryTypePlane},
	};
	uint entityCount = 5;
};

static Assets gAssets = {};

#endif // ASSETS_H
