#include "tools.h"
#include "reflex.h"

#include "cast.h"

const ReflexStruct* ReflexGetStruct(ReflexID id)
{
	return NULL;
}

#define StringPrintfArgs(string) string.size, string.str

void GenerateReflex(const Cast *cast)
{
	const CastStructSpecifier *structs[128];
	u32 structCount = 0;

	const CastStructDeclaration *structDeclarations[128]; // members
	u32 structDeclarationCount = 0;

	const CastEnumSpecifier *enums[128];
	u32 enumCount = 0;

	// Get all the global struct and enum specifiers from the AST
	const CastTranslationUnit *translationUnit = cast->translationUnit;
	while (translationUnit)
	{
		if (translationUnit->externalDeclaration &&
			translationUnit->externalDeclaration->declaration &&
			translationUnit->externalDeclaration->declaration->declarationSpecifiers &&
			translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier)
		{
			const CastTypeSpecifier *typeSpecifier =
				translationUnit->externalDeclaration->declaration->declarationSpecifiers->typeSpecifier;

			if (typeSpecifier->type == CAST_STRUCT && typeSpecifier->structSpecifier) {
				structs[structCount++] = typeSpecifier->structSpecifier;
			} else if (typeSpecifier->type == CAST_ENUM && typeSpecifier->enumSpecifier) {
				enums[enumCount++] = typeSpecifier->enumSpecifier;
			}
		}
		translationUnit = translationUnit->next;
	}

	// ReflexID enum
	bool firstValueInEnum = true;
	printf("\n");
	printf("// IDs\n");
	printf("enum\n");
	printf("{\n");
	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructSpecifier *cstruct = structs[index];
		printf("  ReflexID_%.*s", StringPrintfArgs(cstruct->name));
		printf("%s", firstValueInEnum ? " = ReflexID_StructFirst" : "");
		printf(",\n");
		firstValueInEnum = false;
	}
	firstValueInEnum = true;
	for (u32 index = 0; index < enumCount; ++index)
	{
		const CastEnumSpecifier *cenum = enums[index];
		printf("  ReflexID_%.*s", StringPrintfArgs(cenum->name));
		printf("%s", firstValueInEnum ? " = ReflexID_EnumFirst" : "");
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
	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructSpecifier *cstruct = structs[index];
		printf("  static const ReflexMember reflex%.*sMembers[] = {\n", StringPrintfArgs(cstruct->name));

		structDeclarationCount = 0;
		const CastStructDeclarationList *structDeclarationList = cstruct->structDeclarationList;
		while (structDeclarationList) {
			if (structDeclarationList->structDeclaration) {
				structDeclarations[structDeclarationCount++] = structDeclarationList->structDeclaration;
			}
			structDeclarationList = structDeclarationList->next;
		}

		for ( u32 memberIndex = 0; memberIndex < structDeclarationCount; ++memberIndex)
		{
			const CastStructDeclaration *structDeclaration = structDeclarations[memberIndex];

			// Type qualifiers

			const CastSpecifierQualifierList *specifierQualifierList = NULL;
			if (structDeclaration && structDeclaration->specifierQualifierList) {
				specifierQualifierList = structDeclaration->specifierQualifierList;
			}

			const CastTypeQualifier *typeQualifier = NULL;
			if (specifierQualifierList && specifierQualifierList->typeQualifier) {
				typeQualifier = specifierQualifierList->typeQualifier;
			}

			bool isConst = false;
			if (typeQualifier) {
				isConst = typeQualifier->type == CAST_CONST;
			}

			// Type specifiers

			bool isVoid = false;
			bool isBool = false;
			bool isChar = false;
			bool isInt = false;
			bool isFloat = false;
			bool isDouble = false;
			bool isShort = false;
			bool isLong = false;
			bool isLongLong = false;
			bool isUnsigned = false;
			bool isIdentifier = false;
			String identifier = MakeString("");
			char typeNameBuffer[MAX_PATH_LENGTH];
			String typeName = MakeString("<none>");
			//bool isStruct = false;
			//bool isEnum = false;

			const CastTypeSpecifier *typeSpecifier = NULL;
			if (specifierQualifierList)
			{
				const CastSpecifierQualifierList *specifierList = specifierQualifierList;
				while (specifierList)
				{
					typeSpecifier = specifierList->typeSpecifier;
					if (typeSpecifier) {
						if (typeSpecifier->type == CAST_VOID) {
							isVoid = true; break;
						} else if (typeSpecifier->type == CAST_BOOL) {
							isBool = true; break;
						} else if (typeSpecifier->type == CAST_CHAR) {
							isChar = true; break;
						} else if (typeSpecifier->type == CAST_FLOAT) {
							isFloat = true; break;
						} else if (typeSpecifier->type == CAST_DOUBLE) {
							isDouble = true; break;
						} else if (typeSpecifier->type == CAST_IDENTIFIER) {
							isIdentifier = true; identifier = typeSpecifier->identifier; break;
						} else if (typeSpecifier->type == CAST_INT) {
							isInt = true;
						} else if (typeSpecifier->type == CAST_UNSIGNED) {
							isUnsigned = true;
						} else if (typeSpecifier->type == CAST_SHORT) {
							isShort = true;
						} else if (typeSpecifier->type == CAST_LONG) {
							isLongLong = isLong; isLong = true;
						} else {
							typeName = MakeString("<error>"); break;
						}
					}
					specifierList = specifierList->next;
				}
			}

			if (isVoid) {
				typeName = MakeString("ReflexID_Void");
			} else if (isBool) {
				typeName = MakeString("ReflexID_Bool");
			} else if (isChar) {
				if (isUnsigned) typeName = MakeString("ReflexID_UnsignedChar");
				else typeName = MakeString("ReflexID_Char");
			} else if (isInt) {
				if (isUnsigned) {
					if (isLongLong) typeName = MakeString("ReflexID_UnsignedLongLongInt");
					else if (isLong) typeName = MakeString("ReflexID_UnsignedLongInt");
					else if (isShort) typeName = MakeString("ReflexID_UnsignedShortInt");
					else typeName = MakeString("ReflexID_UnsignedInt");
				} else {
					if (isLongLong) typeName = MakeString("ReflexID_LongLongInt");
					else if (isLong) typeName = MakeString("ReflexID_LongInt");
					else if (isShort) typeName = MakeString("ReflexID_ShortInt");
					else typeName = MakeString("ReflexID_Int");
				}
			} else if (isFloat) {
				typeName = MakeString("ReflexID_Float");
			} else if (isDouble) {
				typeName = MakeString("ReflexID_Double");
			} else if (isIdentifier) {
				StrCopy(typeNameBuffer, "ReflexID_");
				StrCat(typeNameBuffer, identifier);
				typeName = MakeString(typeNameBuffer);
			}

			// Declarator

			const CastDeclarator *declarator = NULL;
			if (structDeclaration->structDeclaratorList &&
				structDeclaration->structDeclaratorList->structDeclarator) {
				declarator = structDeclaration->structDeclaratorList->structDeclarator;
			}

			String memberName = MakeString("<none>");
			if (declarator &&
				declarator->directDeclarator) {
				memberName = declarator->directDeclarator->name;
			}

			uint pointerCount = 0;
			CastPointer *pointer = declarator->pointer;
			while (pointer) {
				pointerCount++;
				pointer = pointer->next;
			}

			bool isArray = false;
			u32 arrayDim = 0;
			if (declarator && declarator->directDeclarator) {
				isArray = declarator->directDeclarator->isArray;
				CastExpression *expression = declarator->directDeclarator->expression;
				arrayDim = expression ? Cast_EvaluateInt(expression) : 0;
			}

			printf("    { ");
			printf(".name = \"%.*s\", ", StringPrintfArgs(memberName));
			printf(".isConst = %s, ", isConst ? "true" : "false");
			printf(".pointerCount = %u, ", pointerCount);
			printf(".isArray = %s, ", isArray ? "true" : "false");
			printf(".arrayDim = %u, ", arrayDim);
			printf(".reflexId = %.*s, ", StringPrintfArgs(typeName));
			printf(".offset = offsetof(%.*s, %.*s) ", StringPrintfArgs(cstruct->name), StringPrintfArgs(memberName));
			printf("},\n");
		}
		printf("  };\n");
	}

	// ReflexStruct
	printf("\n");
	printf("  // ReflexStructs\n");
	printf("  static const ReflexStruct reflexStructs[] =\n");
	printf("  {\n");
	for (u32 index = 0; index < structCount; ++index)
	{
		const CastStructSpecifier *cstruct = structs[index];
		printf("    {\n");
		printf("      .name = \"%.*s\",\n", StringPrintfArgs(cstruct->name));
		printf("      .members = reflex%.*sMembers,\n", StringPrintfArgs(cstruct->name));
		printf("      .memberCount = ARRAY_COUNT(reflex%.*sMembers),\n", StringPrintfArgs(cstruct->name));
		printf("      .size = sizeof(%.*s),\n", StringPrintfArgs(cstruct->name));
		printf("    },\n");
	}
	printf("  };\n");

	printf("  \n");
	printf("  return &reflexStructs[id - ReflexID_StructFirst];\n");
	printf("}\n");
}

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

