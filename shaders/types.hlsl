
struct Globals
{
	float4x4 cameraView;
	float4x4 cameraProj;
	float4x4 sunView;
	float4x4 sunProj;
	float4 sunDir;
	float4 eyePosition;
	float shadowmapDepthBias;
	float3 unused;
};

struct SEntity
{
	float4x4 world;
};

struct SMaterial
{
	float uvScale;
};

