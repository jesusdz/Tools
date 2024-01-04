#ifndef ASSET_TYPES
#define ASSET_TYPES

struct Material
{
	const char *vs;
	const char *fs;
	const char *cs;
};

Material material = {
	.vs = "shaders/vertex.spv",
	.fs = "shaders/fragment.spv",
};

#endif // #ifndef ASSET_TYPES

