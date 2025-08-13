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
	ShaderSourceDesc *shaderDescs;
	u32 shaderDescCount;

	TextureDesc *textureDescs;
	u32 textureDescCount;

	MaterialDesc *materialDescs;
	u32 materialDescCount;

	EntityDesc *entityDescs;
	u32 entityDescCount;

	AudioClipDesc *audioClipDescs;
	u32 audioClipDescCount;
};

// Functions

#if USE_DATA_BUILD
void CompileShaders();
void SaveAssetDescriptors(const char *path, const AssetDescriptors &assetDescriptors);
AssetDescriptors ParseDescriptors(const char *filepath, Arena &arena);
#endif // USE_DATA_BUILD



////////////////////////////////////////////////////////////////////////
// Binary data

// Types

#pragma pack(push, 1)

struct BinLocation
{
	u32 offset;
	u32 size;
};

struct BinShaderDesc
{
	char name[32];
	char entryPoint[32];
	ShaderType type;
	BinLocation location;
};

struct BinImageDesc
{
	char name[32];
	u16 width;
	u16 height;
	u8  channels;
	u8  mipmap;
	u16 unused;
	BinLocation location;
};

struct BinAudioClipDesc
{
	u32 sampleCount;
	u32 samplingRate;
	u16 sampleSize;
	u16 channelCount;
	BinLocation location;
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
	File file;

	BinAssetsHeader header;

	BinShader *shaders;
	BinImage *images;
	BinAudioClip *audioClips;
	BinMaterialDesc *materialDescs;
	BinEntityDesc *entityDescs;
};

// Functions

#if USE_DATA_BUILD
void BuildAssets(const AssetDescriptors &assetDescriptors, const char *filepath, Arena tempArena);
#endif // USE_DATA_BUILD

BinAssets OpenAssets(Arena &dataArena);
void CloseAssets(BinAssets &assets);

#endif // DATA_H
