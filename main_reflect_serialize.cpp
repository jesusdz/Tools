#include "tools.h"
#include "tools_reflex.h"
#include "assets.h"
#include <cstddef> // offsetof

////////////////////////////////////////////////////////////////////////
// Print utils

#define Printf( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); IncreaseIndentation();
#define PrintEndScope( format, ... ) DecreaseIndentation(); LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintNewLine() LOG(Info, "\n"); newLine = true;

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

#define REFLEX_ID(TypeName) ReflexID_ ## TypeName

enum
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

u32 ReflexGetElemCount( const void *data, const ReflexStruct *rstruct, const char *memberName )
{
	for (uint i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const bool isPointer = member->isPointer;
		const uint reflexId = member->reflexId;

		if ( !isPointer && reflexId == ReflexID_UInt )
		{
			const char *cursor = member->name; // current member name

			// NOTE: This solution is quite ad-hoc. We are searching for a member that's
			// called memberNameCount (e.g. for "textures" we look for "texturesCount").
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
			const u32 elemSize = ReflexGetTypeSize(member->reflexId);

			Printf("\"%s\" : ", member->name);

			const u32 elemCount = ReflexGetElemCount(data, rstruct, member->name);
			if ( elemCount > 0 ) {
				PrintBeginScope("[");
				PrintNewLine();
			}

			for (u32 elemIndex = 0; elemIndex == 0 || elemIndex < elemCount; ++elemIndex)
			{
				const void *elemPtr = (u8*)basePtr + elemIndex * elemSize;
				Print(elemPtr, member->reflexId);

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


////////////////////////////////////////////////////////////////////////
//  Main function

int main(int argc, char **argv)
{
	Print(&gAssets, REFLEX_ID(Assets));

	return 0;
}

