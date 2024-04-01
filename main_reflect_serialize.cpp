#include "tools.h"
#include "assets.h"
#include <cstddef> // offsetof

#define USE_REFLECTION 1

////////////////////////////////////////////////////////////////////////
// Print utils

#if USE_REFLECTION
#define Printf( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); IncreaseIndentation();
#define PrintEndScope( format, ... ) DecreaseIndentation(); LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintNewLine() LOG(Info, "\n"); newLine = true;
#else
#define Printf( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ ); IncreaseIndentation();
#define PrintEndScope( format, ... ) DecreaseIndentation(); LOG(Info, "%s" format "\n", indent, ##__VA_ARGS__ );
#endif

static bool newLine = false;
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

static const char *Indentation()
{
	const char *indentation = newLine ? indent : "";
	newLine = false;
	return indentation;
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
	u16 offset;
};

struct ReflexStruct
{
	const char *name;
	const ReflexMember *members;
	u16 memberCount;
	u16 size;
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

const Reflex* GetReflex(ReflexID id);

u32 ReflexGetTypeSize(ReflexID id)
{
	const Reflex* reflex = GetReflex(id);
	if ( reflex->type == ReflexType_Trivial )
	{
		const ReflexTrivial *trivial = reflex->trivial;
		const u32 size = trivial->elemCount * trivial->byteCount;
		return size;
	}
	else if ( reflex->type == ReflexType_Struct )
	{
		const ReflexStruct *rstruct = reflex->rstruct;
		const u32 size = rstruct->size;
		return size;
	}
	else
	{
		INVALID_CODE_PATH();
	}
	return 0;
}

u32 ReflexGetElemCount( const void *data, const ReflexStruct *rstruct, const ReflexMember *member )
{
	const char *memberName = member->name;

	for (uint i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const bool isPointer = member->isPointer;
		const uint reflexId = member->reflexId;

		if ( !isPointer && reflexId == ReflexID_UInt )
		{
			const char *cursor = member->name; // current member name

			if ( ( cursor = StrConsume( cursor, memberName ) ) &&
					( cursor = StrConsume( cursor, "Count" ) ) && *cursor == 0 )
			{
				const void *memberPtr = (u8*)data + member->offset;
				const u32 count = *(u32*)memberPtr;
				return count;
			}
		}
	}
	return 0;
}

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
		{ .name = "texturesCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, texturesCount) },
		{ .name = "pipelines", true, true, .reflexId = ReflexID_PipelineDesc, .offset = offsetof(Assets, pipelines) },
		{ .name = "pipelinesCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, pipelinesCount) },
		{ .name = "materials", true, true, .reflexId = ReflexID_MaterialDesc, .offset = offsetof(Assets, materials) },
		{ .name = "materialsCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, materialsCount) },
		{ .name = "entities", true, true, .reflexId = ReflexID_EntityDesc, .offset = offsetof(Assets, entities) },
		{ .name = "entitiesCount", false, false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, entitiesCount) },
	};
	static const ReflexStruct reflexTextureDesc = {
		.name = "TextureDesc",
		.members = reflexTextureDescMembers,
		.memberCount = ARRAY_COUNT(reflexTextureDescMembers),
		.size = sizeof(TextureDesc),
	};
	static const ReflexStruct reflexPipelineDesc = {
		.name = "PipelineDesc",
		.members = reflexPipelineDescMembers,
		.memberCount = ARRAY_COUNT(reflexPipelineDescMembers),
		.size = sizeof(PipelineDesc),
	};
	static const ReflexStruct reflexMaterialDesc = {
		.name = "MaterialDesc",
		.members = reflexMaterialDescMembers,
		.memberCount = ARRAY_COUNT(reflexMaterialDescMembers),
		.size = sizeof(MaterialDesc),
	};
	static const ReflexStruct reflexEntityDesc = {
		.name = "EntityDesc",
		.members = reflexEntityDescMembers,
		.memberCount = ARRAY_COUNT(reflexEntityDescMembers),
		.size = sizeof(EntityDesc),
	};
	static const ReflexStruct reflexAssets = {
		.name = "Assets",
		.members = reflexAssetsMembers,
		.memberCount = ARRAY_COUNT(reflexAssetsMembers),
		.size = sizeof(Assets),
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

void Print(const void *data, const ReflexID id)
{
	const Reflex *reflex = GetReflex(id);

	if ( reflex->type == ReflexType_Trivial )
	{
		const ReflexTrivial *trivial = reflex->trivial;

		if (trivial->elemCount > 1) {
			Printf("[");
		}

		for (uint i = 0; i < trivial->elemCount; ++i)
		{
			if (i > 0) {
				Printf(", ");
			}

			const char *sVal = (const char *)data;
			const bool bVal = *(bool*)data;
			const i32 iVal = *(i32*)data;
			const u32 uVal = *(u32*)data;
			const float fVal = *(float*)data;

			if ( trivial->isString  ) {
				Printf("\"%s\"", sVal);
			} else if ( trivial->isBool ) {
				Printf("%d", bVal ? 1 : 0);
			} else if ( trivial->isFloat ) {
				Printf("%f", fVal);
			} else if ( trivial->isUnsigned ) {
				Printf("%u", uVal);
			} else {
				Printf("%d", iVal);
			}
		}

		if (trivial->elemCount > 1) {
			Printf("]");
		}
	}
	else if ( reflex->type == ReflexType_Struct )
	{
		const ReflexStruct *rstruct = reflex->rstruct;

		PrintBeginScope("{");
		PrintNewLine();

		for (uint i = 0; i < rstruct->memberCount; ++i)
		{
			const ReflexMember *member = &rstruct->members[i];
			const bool isConst = member->isConst;
			const bool isPointer = member->isPointer;
			const void *memberPtr = (u8*)data + member->offset;
			const void *basePtr = isPointer ? (void*)(*((u8**)memberPtr)) : memberPtr;
			const u32 elemSize = ReflexGetTypeSize((ReflexID)member->reflexId);

			Printf("\"%s\" : ", member->name);

			const u32 elemCount = ReflexGetElemCount(data, rstruct, member);
			if ( elemCount > 0 ) {
				PrintBeginScope("[");
				PrintNewLine();
			}

			for (u32 elemIndex = 0; elemIndex == 0 || elemIndex < elemCount; ++elemIndex)
			{
				const void *elemPtr = (u8*)basePtr + elemIndex * elemSize;
				Print(elemPtr, (ReflexID)member->reflexId);

				if (elemCount > 0 && elemIndex + 1 < elemCount) {
					Printf(",");
					PrintNewLine();
				}
			}

			if ( elemCount > 0 ) {
				PrintNewLine();
				PrintEndScope("]");
			}

			if ( i + 1 < rstruct->memberCount ) {
				Printf(",");
			}
			PrintNewLine();

		}

		PrintEndScope("}");
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
	for (int i = 0; i < assets->texturesCount; ++i)
	{
		Print(&assets->textures[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"pipelines\" : [");
	for (int i = 0; i < assets->pipelinesCount; ++i)
	{
		Print(&assets->pipelines[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"materials\" : [");
	for (int i = 0; i < assets->materialsCount; ++i)
	{
		Print(&assets->materials[i]);
	}
	PrintEndScope("],");

	PrintBeginScope("\"entities\" : [");
	for (int i = 0; i < assets->entitiesCount; ++i)
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

