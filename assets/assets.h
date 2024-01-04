#ifndef ASSET_TYPES
#define ASSET_TYPES

struct Material
{
	const char *vs;
	const char *fs;
	const char *cs;
};

struct Geometry
{
	const float *vertices;
	const uint *indices;
};

const Material material = {
	.vs = "shaders/vertex.spv",
	.fs = "shaders/fragment.spv",
};

const float geometryVertices[] = {
	-1.0, -1.0, 0.0,
	1.0, -1.0, 0.0,
	0.0,  1.0, 0.0,
};
const uint geometryIndices[] = { 0, 1, 2 };

const Geometry geometry = {
	.vertices = geometryVertices,
	.indices = geometryIndices,
};

#endif // #ifndef ASSET_TYPES

