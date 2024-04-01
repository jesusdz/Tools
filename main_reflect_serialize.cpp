#include "tools.h"
#include "assets.h"
#include <cstddef> // offsetof

#define USE_REFLECTION 1

////////////////////////////////////////////////////////////////////////
// Print utils

#define Printf( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );
#define PrintfSameLine( format, ... ) LOG(Info, "%s" format, indent, ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ ); IncreaseIndentation();
#define PrintEndScope( format, ... ) DecreaseIndentation(); LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );

static u32 indentPos = 0;
static char indent[64] = {};

static void IncreaseIndentation()
{
	indent[indentPos++] = '\t';
}

static void DecreaseIndentation()
{
	ASSERT(indentPos > 0);
	indent[--indentPos] = '\0';
}


////////////////////////////////////////////////////////////////////////
// Automatic implementation (using reflection)

#if USE_REFLECTION

enum ReflexType
{
	ReflexType_Trivial,
	ReflexType_Struct,
	ReflexType_Enum,
	ReflexType_COUNT,
};

struct ReflexTrivial
{
	u8 isBool : 1;
	u8 isFloat : 1;
	u8 isUnsigned : 1; // !bool and !float
	u8 isString : 1;
    u8 byteCount : 5;
	u8 elemCount;
};

struct ReflexMember
{
	const char *name;
	u8 isConst : 1;
	u8 isPointer : 1;
	u16 reflexId;
	u32 offset;
};

struct ReflexStruct
{
	const char *name;
	const ReflexMember *members;
	const u32 memberCount;
};

struct Reflex
{
	ReflexType type;
	union
	{
		const ReflexTrivial *trivial;
		const ReflexStruct *rstruct;
	};
};

enum ReflexID
{
	ReflexID_TextureDesc,
	ReflexID_PipelineDesc,
	ReflexID_MaterialDesc,
	ReflexID_EntityDesc,
	ReflexID_Assets,
	ReflexID_Int,
	ReflexID_UInt,
	ReflexID_Float,
	ReflexID_Float3,
	ReflexID_String,
};

ReflexID GetReflexID(const TextureDesc *) { return ReflexID_TextureDesc; }
ReflexID GetReflexID(const PipelineDesc *) { return ReflexID_PipelineDesc; }
ReflexID GetReflexID(const MaterialDesc *) { return ReflexID_MaterialDesc; }
ReflexID GetReflexID(const EntityDesc *) { return ReflexID_EntityDesc; }
ReflexID GetReflexID(const Assets *) { return ReflexID_Assets; }

const Reflex* GetReflex(ReflexID id)
{
	static const ReflexMember reflexTextureDescMembers[] = {
		{ .name = "name", true, true, .reflexId = ReflexID_String, .offset = offsetof(TextureDesc, name) },
		{ .name = "filename", true, true, .reflexId = ReflexID_String, .offset = offsetof(TextureDesc, filename) },
	};
	static const ReflexMember reflexPipelineDescMembers[] = {
		{ .name = "name", true, true, .reflexId = ReflexID_String, .offset = offsetof(PipelineDesc, name) },
		{ .name = "vsFilename", true, true, .reflexId = ReflexID_String, .offset = offsetof(PipelineDesc, vsFilename) },
		{ .name = "fsFilename", true, true, .reflexId = ReflexID_String, .offset = offsetof(PipelineDesc, fsFilename) },
	};
	static const ReflexMember reflexMaterialDescMembers[] = {
		{ .name = "name", true, true, .reflexId = ReflexID_String, .offset = offsetof(MaterialDesc, name) },
		{ .name = "textureName", true, true, .reflexId = ReflexID_String, .offset = offsetof(MaterialDesc, textureName) },
		{ .name = "pipelineName", true, true, .reflexId = ReflexID_String, .offset = offsetof(MaterialDesc, pipelineName) },
		{ .name = "uvScale", false, false, .reflexId = ReflexID_Float, .offset = offsetof(MaterialDesc, uvScale) },
	};
	static const ReflexMember reflexEntityDescMembers[] = {
		{ .name = "name", true, true, .reflexId = ReflexID_String, .offset = offsetof(EntityDesc, name) },
		{ .name = "materialName", true, true, .reflexId = ReflexID_String, .offset = offsetof(EntityDesc, materialName) },
		{ .name = "pos", false, false, .reflexId = ReflexID_Float3, .offset = offsetof(EntityDesc, pos) },
		{ .name = "scale", false, false, .reflexId = ReflexID_Float, .offset = offsetof(EntityDesc, scale) },
		{ .name = "geometryType", false, false, .reflexId = ReflexID_Int, .offset = offsetof(EntityDesc, geometryType) },
	};
	static const ReflexMember reflexAssetsMembers[] = {
		{ .name = "textures", true, true, .reflexId = ReflexID_TextureDesc, .offset = offsetof(Assets, textures) },
		{ .name = "textureCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, textureCount) },
		{ .name = "pipelines", true, true, .reflexId = ReflexID_PipelineDesc, .offset = offsetof(Assets, pipelines) },
		{ .name = "pipelineCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, pipelineCount) },
		{ .name = "materials", true, true, .reflexId = ReflexID_MaterialDesc, .offset = offsetof(Assets, materials) },
		{ .name = "materialCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, materialCount) },
		{ .name = "entities", true, true, .reflexId = ReflexID_EntityDesc, .offset = offsetof(Assets, entities) },
		{ .name = "entityCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, entityCount) },
	};
	static const ReflexStruct reflexTextureDesc = {
		.name = "TextureDesc",
		.members = reflexTextureDescMembers,
		.memberCount = ARRAY_COUNT(reflexTextureDescMembers),
	};
	static const ReflexStruct reflexPipelineDesc = {
		.name = "PipelineDesc",
		.members = reflexPipelineDescMembers,
		.memberCount = ARRAY_COUNT(reflexPipelineDescMembers),
	};
	static const ReflexStruct reflexMaterialDesc = {
		.name = "MaterialDesc",
		.members = reflexMaterialDescMembers,
		.memberCount = ARRAY_COUNT(reflexMaterialDescMembers),
	};
	static const ReflexStruct reflexEntityDesc = {
		.name = "EntityDesc",
		.members = reflexEntityDescMembers,
		.memberCount = ARRAY_COUNT(reflexEntityDescMembers),
	};
	static const ReflexStruct reflexAssets = {
		.name = "Assets",
		.members = reflexAssetsMembers,
		.memberCount = ARRAY_COUNT(reflexAssetsMembers),
	};
	static const ReflexTrivial reflexInt =
	{ .isBool = 0, .isFloat = 0, .isUnsigned = 0, .isString = 0, .byteCount = 4, .elemCount = 1 };
	static const ReflexTrivial reflexUInt =
	{ .isBool = 0, .isFloat = 0, .isUnsigned = 1, .isString = 0, .byteCount = 4, .elemCount = 1 };
	static const ReflexTrivial reflexFloat =
	{ .isBool = 0, .isFloat = 1, .isUnsigned = 0, .isString = 0, .byteCount = 4, .elemCount = 1 };
	static const ReflexTrivial reflexFloat3 =
	{ .isBool = 0, .isFloat = 1, .isUnsigned = 0, .isString = 0, .byteCount = 4, .elemCount = 3 };
	static const ReflexTrivial reflexString =
	{ .isBool = 0, .isFloat = 0, .isUnsigned = 0, .isString = 1, .byteCount = 4, .elemCount = 1 };

	static const Reflex reflections[] =
	{
		{ .type = ReflexType_Struct, .rstruct = &reflexTextureDesc },
		{ .type = ReflexType_Struct, .rstruct = &reflexPipelineDesc },
		{ .type = ReflexType_Struct, .rstruct = &reflexMaterialDesc },
		{ .type = ReflexType_Struct, .rstruct = &reflexEntityDesc },
		{ .type = ReflexType_Struct, .rstruct = &reflexAssets },
		{ .type = ReflexType_Trivial, .trivial = &reflexInt },
		{ .type = ReflexType_Trivial, .trivial = &reflexUInt },
		{ .type = ReflexType_Trivial, .trivial = &reflexFloat },
		{ .type = ReflexType_Trivial, .trivial = &reflexFloat3 },
		{ .type = ReflexType_Trivial, .trivial = &reflexString },
	};

	return &reflections[id];
}

// TODO
void Print(const void *data, const ReflexID id)
{
	const Reflex *reflex = GetReflex(id);

	if ( reflex->type == ReflexType_Trivial )
	{
		Printf("");
		const ReflexTrivial *trivial = reflex->trivial;
	}
	else if ( reflex->type == ReflexType_Struct )
	{
		const ReflexStruct *rstruct = reflex->rstruct;

		PrintBeginScope("{");

		for (uint i = 0; i < rstruct->memberCount; ++i)
		{
			const ReflexMember *member = &rstruct->members[i];
			const bool isConst = member->isConst;
			const bool isPointer = member->isPointer;
			const void *memberPtr = (u8*)data + member->offset;
			const void *dataPtr = isPointer ? (void*)(*((u8**)memberPtr)) : memberPtr;
			PrintfSameLine("\"%s\" : ", member->name);
			Print(dataPtr, (ReflexID)member->reflexId);
		}

		PrintEndScope("},");
	}
}

#endif // USE_REFLECTION


////////////////////////////////////////////////////////////////////////
// Manual implementation

#if !USE_REFLECTION

void Print(const TextureDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"filename\" : \"%s\",", desc->filename);
	PrintEndScope("},");
}

void Print(const PipelineDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"vsFilename\" : \"%s\",", desc->vsFilename);
	Printf("\"fsFilename\" : \"%s\",", desc->fsFilename);
	PrintEndScope("},");
}

void Print(const MaterialDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"textureName\" : \"%s\",", desc->textureName);
	Printf("\"pipelineName\" : \"%s\",", desc->pipelineName);
	Printf("\"uvScale\" : \"%f\",", desc->uvScale);
	PrintEndScope("},");
}

void Print(const EntityDesc *desc)
{
	PrintBeginScope("{");
	Printf("\"name\" : \"%s\",", desc->name);
	Printf("\"materialName\" : \"%s\",", desc->materialName);
	Printf("\"pos\" : [%f, %f, %f],", desc->pos.x, desc->pos.y, desc->pos.z);
	Printf("\"scale\" : %f,", desc->scale);
	Printf("\"geometryType\" : \"%d\",", desc->geometryType);
	PrintEndScope("},");
}

void Print(const Assets *assets)
{
	PrintBeginScope("{");

	PrintBeginScope("\"textures\" : [");
	for (int i = 0; i < assets->textureCount; ++i)
	{
		Print(&assets->textures[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"pipelines\" : [");
	for (int i = 0; i < assets->pipelineCount; ++i)
	{
		Print(&assets->pipelines[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"materials\" : [");
	for (int i = 0; i < assets->materialCount; ++i)
	{
		Print(&assets->materials[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"entities\" : [");
	for (int i = 0; i < assets->entityCount; ++i)
	{
		Print(&assets->entities[i]);
	}
	PrintEndScope("],");

	PrintEndScope("};");
}

#endif // !USE_REFLECTION


////////////////////////////////////////////////////////////////////////
//  Main function

int main(int argc, char **argv)
{
#if USE_REFLECTION
	Print(&gAssets, GetReflexID(&gAssets));
#else
	Print(&gAssets);
#endif

	return 0;
}

