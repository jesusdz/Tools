#ifndef DATA_H
#define DATA_H

////////////////////////////////////////////////////////////////////////
// Text data

// Types

enum ShaderType
{
	ShaderTypeVertex,
	ShaderTypeFragment,
	ShaderTypeCompute
};

struct ShaderSourceDesc
{
	ShaderType type;
	const char *filename;
	const char *entryPoint;
	const char *name;
};

struct TextureDesc
{
	const char *name;
	const char *filename;
	u8 mipmap;
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

struct AudioClipDesc
{
	const char *name;
	const char *filename;
};

struct AssetDescriptors
{
	const ShaderSourceDesc *shaderDescs;
	u32 shaderDescCount;

	const TextureDesc *textureDescs;
	u32 textureDescCount;

	const MaterialDesc *materialDescs;
	u32 materialDescCount;

	const EntityDesc *entityDescs;
	u32 entityDescCount;

	const AudioClipDesc *audioClipDescs;
	u32 audioClipDescCount;
};

// Functions

void SaveAssetDescriptors(const char *path, const AssetDescriptors &assetDescriptors);



////////////////////////////////////////////////////////////////////////
// Binary data

// Types

#pragma pack(push, 1)
struct BinShaderDesc
{
	char name[32];
	char entryPoint[32];
	ShaderType type;
	u32 dataOffset;
	u32 dataSize;
};

struct BinImageDesc
{
	char name[32];
	u16 width;
	u16 height;
	u8  channels;
	u8  mipmap;
	u16 unused;
	u32 dataOffset;
	u32 dataSize;
};

struct BinAudioClipDesc
{
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	u32 dataOffset;
	u32 dataSize;
};

struct BinMaterialDesc
{
	char name[32];
	char textureName[32];
	char pipelineName[32];
	float uvScale;
};

struct BinEntityDesc
{
	char name[32];
	char materialName[32];
	float3 pos;
	float scale;
	GeometryType geometryType;
};

struct BinAssetsHeader
{
	u32 magicNumber;
	u32 shadersOffset;
	u32 shaderCount;
	u32 imagesOffset;
	u32 imageCount;
	u32 audioClipsOffset;
	u32 audioClipCount;
	u32 materialsOffset;
	u32 materialCount;
	u32 entitiesOffset;
	u32 entityCount;
};
#pragma pack(pop)

struct BinShader
{
	BinShaderDesc *desc;
	byte *spirv;
};

struct BinImage
{
	BinImageDesc *desc;
	byte *pixels;
};

struct BinAudioClip
{
	BinAudioClipDesc *desc;
	void *samples;
};

struct BinAssets
{
	BinAssetsHeader header;

	BinShader *shaders;
	BinImage *images;
	BinAudioClip *audioClips;
	BinMaterialDesc *materialDescs;
	BinEntityDesc *entityDescs;
};

// Functions

struct Engine;

#if USE_DATA_BUILD
void BuildAssets(const AssetDescriptors &assetDescriptors, const char *filepath, Arena tempArena);
#endif // USE_DATA_BUILD

BinAssets LoadAssets(Arena &dataArena);

#endif // DATA_H
