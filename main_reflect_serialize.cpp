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
	ReflexID_float3 = ReflexID_Struct,
	ReflexID_TextureDesc,
	ReflexID_PipelineDesc,
	ReflexID_MaterialDesc,
	ReflexID_EntityDesc,
	ReflexID_Assets,
};

const ReflexStruct* ReflexGetStruct(ReflexID id)
{
	ASSERT(ReflexIsStruct(id));

	static const ReflexMember reflexfloat3Members[] = {
		{ .name = "x", .isConst = false, .isPointer = false, .reflexId = ReflexID_Float, .offset = offsetof(float3, x) },
		{ .name = "y", .isConst = false, .isPointer = false, .reflexId = ReflexID_Float, .offset = offsetof(float3, y) },
		{ .name = "z", .isConst = false, .isPointer = false, .reflexId = ReflexID_Float, .offset = offsetof(float3, z) },
	};
	static const ReflexMember reflexTextureDescMembers[] = {
		{ .name = "name", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(TextureDesc, name) },
		{ .name = "filename", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(TextureDesc, filename) },
	};
	static const ReflexMember reflexPipelineDescMembers[] = {
		{ .name = "name", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(PipelineDesc, name) },
		{ .name = "vsFilename", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(PipelineDesc, vsFilename) },
		{ .name = "fsFilename", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(PipelineDesc, fsFilename) },
	};
	static const ReflexMember reflexMaterialDescMembers[] = {
		{ .name = "name", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(MaterialDesc, name) },
		{ .name = "textureName", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(MaterialDesc, textureName) },
		{ .name = "pipelineName", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(MaterialDesc, pipelineName) },
		{ .name = "uvScale", .isConst = false, .isPointer = false, .reflexId = ReflexID_Float, .offset = offsetof(MaterialDesc, uvScale) },
	};
	static const ReflexMember reflexEntityDescMembers[] = {
		{ .name = "name", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(EntityDesc, name) },
		{ .name = "materialName", .isConst = true, .isPointer = true, .reflexId = ReflexID_Char, .offset = offsetof(EntityDesc, materialName) },
		{ .name = "pos", .isConst = false, .isPointer = false, .reflexId = ReflexID_float3, .offset = offsetof(EntityDesc, pos) },
		{ .name = "scale", .isConst = false, .isPointer = false, .reflexId = ReflexID_Float, .offset = offsetof(EntityDesc, scale) },
		{ .name = "geometryType", .isConst = false, .isPointer = false, .reflexId = ReflexID_Int, .offset = offsetof(EntityDesc, geometryType) },
	};
	static const ReflexMember reflexAssetsMembers[] = {
		{ .name = "textures", .isConst = true, .isPointer = true, .reflexId = ReflexID_TextureDesc, .offset = offsetof(Assets, textures) },
		{ .name = "texturesCount", .isConst = false, .isPointer = false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, texturesCount) },
		{ .name = "pipelines", .isConst = true, .isPointer = true, .reflexId = ReflexID_PipelineDesc, .offset = offsetof(Assets, pipelines) },
		{ .name = "pipelinesCount", .isConst = false, .isPointer = false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, pipelinesCount) },
		{ .name = "materials", .isConst = true, .isPointer = true, .reflexId = ReflexID_MaterialDesc, .offset = offsetof(Assets, materials) },
		{ .name = "materialsCount", .isConst = false, .isPointer = false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, materialsCount) },
		{ .name = "entities", .isConst = true, .isPointer = true, .reflexId = ReflexID_EntityDesc, .offset = offsetof(Assets, entities) },
		{ .name = "entitiesCount", .isConst = false, .isPointer = false, .reflexId = ReflexID_UInt, .offset = offsetof(Assets, entitiesCount) },
	};
	static const ReflexStruct reflexfloat3 = {
		.name = "float3",
		.members = reflexfloat3Members,
		.memberCount = ARRAY_COUNT(reflexfloat3Members),
		.size = sizeof(float3),
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

	static const ReflexStruct *reflections[] =
	{
		&reflexfloat3,
		&reflexTextureDesc,
		&reflexPipelineDesc,
		&reflexMaterialDesc,
		&reflexEntityDesc,
		&reflexAssets,
	};

	return reflections[id - ReflexID_Struct];
}

void Print(const void *data, const ReflexID id, bool isPointer)
{
	if (ReflexIsTrivial(id))
	{
		const ReflexTrivial *trivial = ReflexGetTrivial(id);
		//const u32 elemCount = trivial->elemCount;

		//if (elemCount > 1) {
		//	Printf("[");
		//}

		//for (u32 i = 0; i < elemCount; ++i)
		//{
		//	if (i > 0) {
		//		Printf(", ");
		//	}
			const u32 i = 0;

			const char *sVal = (const char *)data;
			const bool bVal = *((bool*)data + i);
			const char cVal = *((char*)data + i);
			const i32 iVal = *((i32*)data + i);
			const u32 uVal = *((u32*)data + i);
			const float fVal = *((float*)data + i);

			if (trivial->isBool) {
				Printf("%d", bVal ? 1 : 0);
			} else if (trivial->isChar) {
				if ( isPointer )
				{
					Printf("\"%s\"", sVal);
				}
				else
				{
					Printf("%c", cVal);
				}
			} else if (trivial->isFloat) {
				Printf("%f", fVal);
			} else if (trivial->isUnsigned) {
				Printf("%u", uVal);
			} else {
				Printf("%d", iVal);
			}
		//}

		//if (elemCount > 1) {
		//	Printf("]");
		//}
	}
	else if (ReflexIsStruct(id))
	{
		const ReflexStruct *rstruct = ReflexGetStruct(id);

		PrintBeginScope("{");
		PrintNewLine();

		for (u32 i = 0; i < rstruct->memberCount; ++i)
		{
			const ReflexMember *member = &rstruct->members[i];
			const void *structPtr = data;
			const void *memberPtr = ReflexMemberPtr(structPtr, member);

			Printf("\"%s\" : ", member->name);

			const u32 elemCount = ReflexGetElemCount(structPtr, rstruct, member->name);
			if ( elemCount > 0 ) {
				PrintBeginScope("[");
				PrintNewLine();
			}

			const u32 elemSize = ReflexGetTypeSize(member->reflexId);
			for (u32 elemIndex = 0; elemIndex == 0 || elemIndex < elemCount; ++elemIndex)
			{
				const void *elemPtr = (u8*)memberPtr + elemIndex * elemSize;
				Print(elemPtr, member->reflexId, member->isPointer);

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
	Print(&gAssets, REFLEX_ID(Assets), false);

	return 0;
}

