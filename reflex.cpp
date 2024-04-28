#include "tools.h"
#include "reflex.h"

#define USE_CPARSER2 0
#if USE_CPARSER2
#include "cparser2.h"
#else
#define CPARSER_IMPLEMENTATION
#include "cparser.h"
#endif

const ReflexStruct* ReflexGetStruct(ReflexID id)
{
	return NULL;
}

#define StringPrintfArgs(string) string.size, string.str

#if USE_CPARSER2
void GenerateReflex(const Cast *cast)
{
	// TODO
}
#else
void CAssembly_GenerateReflex(const CAssembly &cAsm)
{
	if ( !cAsm.valid )
	{
		return;
	}

	// ReflexID enum
	bool firstValueInEnum = true;
	printf("\n");
	printf("// IDs\n");
	printf("enum\n");
	printf("{\n");
	for (u32 index = 0; index < cAsm.structCount; ++index)
	{
		const CStruct *cStruct = CAssembly_GetStruct(cAsm, index);
		printf("  ReflexID_%.*s", StringPrintfArgs(cStruct->name));
		printf("%s", firstValueInEnum ? " = ReflexID_Struct" : "");
		printf(",\n");
		firstValueInEnum = false;
	}
	printf("};\n");

	// ReflexGetStruct function
	printf("\n");
	printf("const ReflexStruct* ReflexGetStruct(ReflexID id)\n");
	printf("{\n");
	printf("  ASSERT(ReflexIsStruct(id));\n");

	// ReflexMember
	printf("\n");
	printf("  // ReflexMembers\n");
	for (u32 index = 0; index < cAsm.structCount; ++index)
	{
		const CStruct *cStruct = CAssembly_GetStruct(cAsm, index);
		printf("  static const ReflexMember reflex%.*sMembers[] = {\n", StringPrintfArgs(cStruct->name));
		for ( u32 fieldIndex = 0; fieldIndex < cStruct->memberCount; ++fieldIndex)
		{
			const CMember *member = &cStruct->members[fieldIndex];
			printf("    { ");
			printf(".name = \"%.*s\", ", StringPrintfArgs(member->name));
			printf(".isConst = %s, ", member->isConst ? "true" : "false");
			printf(".pointerCount = %u, ", member->pointerCount);
			printf(".isArray = %s, ", member->isArray ? "true" : "false");
			printf(".arrayDim = %u, ", member->arrayDim);
			printf(".reflexId = %s, ", CAssembly_GetTypeName(cAsm, member->reflexId));
			printf(".offset = offsetof(%.*s, %.*s) ", StringPrintfArgs(cStruct->name), StringPrintfArgs(member->name));
			printf(" },\n");
		}
		printf("  };\n");
	}

	// ReflexStruct
	printf("\n");
	printf("  // ReflexStructs\n");
	printf("  static const ReflexStruct reflexStructs[] =\n");
	printf("  {\n");
	for (u32 index = 0; index < cAsm.structCount; ++index)
	{
		const CStruct *cStruct = CAssembly_GetStruct(cAsm, index);
		printf("    {\n");
		printf("      .name = \"%.*s\",\n", StringPrintfArgs(cStruct->name));
		printf("      .members = reflex%.*sMembers,\n", StringPrintfArgs(cStruct->name));
		printf("      .memberCount = ARRAY_COUNT(reflex%.*sMembers),\n", StringPrintfArgs(cStruct->name));
		printf("      .size = sizeof(%.*s),\n", StringPrintfArgs(cStruct->name));
		printf("    },\n");
	}
	printf("  };\n");

	printf("  \n");
	printf("  return &reflexStructs[id - ReflexID_Struct];\n");
	printf("}\n");
}
#endif

int main(int argc, char **argv)
{
	if (argc != 2 )
	{
		LOG(Info, "Usage: %s <c file>\n", argv[0]);
		return -1;
	}

	const char *filename = argv[1];

	u64 fileSize;
	if ( GetFileSize(filename, fileSize) && fileSize > 0 )
	{
		u32 globalArenaSize = MB(4);
		byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);
		Arena globalArena = MakeArena(globalArenaBase, globalArenaSize);

		char* bytes = PushArray(globalArena, char, fileSize + 1);
		if ( ReadEntireFile(filename, bytes, fileSize) )
		{
			bytes[fileSize] = 0;

#if USE_CPARSER2
			Cast *cast = Cast_Create(globalArena, bytes, fileSize);
			if (cast)
			{
				GenerateReflex(cast);
			}
			else
			{
				LOG(Error, "Cast_Create() failed:\n");
				LOG(Error, "- file: %s\n", filename);
				LOG(Error, "- message: %s\n", Cast_GetError());
				return -1;
			}
#else
			CAssembly cAsm;
			if ( CAssembly_Create(cAsm, globalArena, bytes, fileSize) )
			{
				CAssembly_GenerateReflex(cAsm);
			}
			else
			{
				LOG(Error, "CAssembly_Read() for file %s\n", filename);
				return -1;
			}
#endif
		}
		else
		{
			LOG(Error, "ReadEntireFile() failed reading %s\n", filename);
			return -1;
		}
	}
	else
	{
		LOG(Error, "GetFileSize() failed reading %s\n", filename);
		return -1;
	}
	
	return 0;
}

