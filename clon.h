/*
 * clon.h
 * Author: Jesus Diaz Garcia
 * CLON stands for C-like object notation.
 * It is a text format designed to store data in a descriptive and similar way to the C language.
 */

#ifndef CLON_H
#define CLON_H

#include "tools.h"

#define CAST_IMPLEMENTATION
#include "cast.h"

#include "reflex.h"

#define CLON_GLOBALS_LIST_SIZE 64

struct ClonGlobal
{
	const char *typeName;
	const char *name;
	void *data;

	// TODO: remove
	u32 count;
	u32 size;
};

struct ClonGlobalsList
{
	ClonGlobal globals[CLON_GLOBALS_LIST_SIZE];
	u32 globalsCount;
	ClonGlobalsList *next;
};

struct Clon
{
	ClonGlobalsList *globalsList;
};

void ClonAddGlobal(Arena *arena, Clon *clon, const char *typeName, const char *name, void *data, u32 size, u32 count)
{
	ClonGlobalsList *globalsList = clon->globalsList;
	if (!globalsList || globalsList->globalsCount == CLON_GLOBALS_LIST_SIZE)
	{
		ClonGlobalsList *prevGlobalsList = globalsList;
		globalsList = PushZeroStruct(*arena, ClonGlobalsList);
		globalsList->next = prevGlobalsList;
		clon->globalsList = globalsList;
	}

	ClonGlobal *global = &globalsList->globals[globalsList->globalsCount++];
	global->typeName = typeName;
	global->name = name;
	global->data = data;

	// TODO: Remove
	global->size = size;
	global->count = count;

// TODO: Remove
//	const char *globalName = global->name;
//	const char *globalTypeName = global->typeName;
//	const u32 typeSize = global->size;
//	const u32 elemCount = global->count;
//	printf("Global %s of type %s\n", globalName, globalTypeName);
//	printf(" - typeSize: %u\n", typeSize);
//	printf(" - elemCount: %u\n", elemCount);
}

bool ClonParse(Arena *arena, const char *data, u32 dataSize, Clon *clon)
{
	const Cast *cast = Cast_Create(*arena, data, dataSize);
	if (cast)
	{
		const CastTranslationUnit *translationUnit = CAST_CHILD(cast, translationUnit);

		while (translationUnit)
		{
			const CastDeclaration *declaration = CAST_CHILD(translationUnit, externalDeclaration, declaration);
			const CastInitDeclarator *initDeclarator = CAST_CHILD(declaration, initDeclaratorList, initDeclarator);
			const CastDeclarator *declarator = CAST_CHILD(initDeclarator, declarator);
			const CastDirectDeclarator *directDeclarator = CAST_CHILD(declarator, directDeclarator);
			const CastInitializer *initializer = CAST_CHILD(initDeclarator, initializer);

			if (directDeclarator && initializer)
			{
				const CastDeclarationSpecifiers *specifiers = CAST_CHILD(declaration, declarationSpecifiers);
				CastTypeSpecifier *typeSpecifier = 0;
				while (specifiers && !typeSpecifier)
				{
					typeSpecifier = specifiers->typeSpecifier;
					specifiers = specifiers->next;
				}

				if (typeSpecifier && typeSpecifier->type == CAST_IDENTIFIER)
				{
					char *globalTypeName = PushArray(*arena, char, typeSpecifier->identifier.size+1);
					StrCopy(globalTypeName, typeSpecifier->identifier);

					const ReflexStruct *rstruct = ReflexGetStructFromName(globalTypeName);

					if (rstruct)
					{
						const u32 typeSize = rstruct->size;
						const u32 elemCount = !directDeclarator->isArray ? 1 :
							directDeclarator->expression ? Cast_EvaluateInt(directDeclarator->expression):
							initializer->initializerCount;
						const u32 globalSize = typeSize * elemCount;

						char *globalName = PushArray(*arena, char, directDeclarator->name.size+1);
						StrCopy(globalName, directDeclarator->name);

						// TODO: Remove
						//printf("Global %s of type %s\n", globalName, globalTypeName);
						//printf(" - typeSize: %u\n", typeSize);
						//printf(" - elemCount: %u\n", elemCount);

						void *globalData = PushSize(*arena, globalSize);
						ClonAddGlobal(arena, clon, globalTypeName, globalName, globalData, typeSize, elemCount);
					}
				}
			}

			translationUnit = translationUnit->next;
		}
	}

// TODO: Remove
//	const ClonGlobalsList *list = clon->globalsList;
//	while (list)
//	{
//		for (u32 i = 0; i < list->globalsCount; ++i)
//		{
//			const ClonGlobal *global = &list->globals[i];
//			const char *globalName = global->name;
//			const char *globalTypeName = global->typeName;
//			const u32 typeSize = global->size;
//			const u32 elemCount = global->count;
//			printf("Global %s of type %s\n", globalName, globalTypeName);
//			printf(" - typeSize: %u\n", typeSize);
//			printf(" - elemCount: %u\n", elemCount);
//		}
//
//		list = list->next;
//	}

	return cast != NULL;
}

const void *ClonGetGlobal(const Clon *clon, const char *type_name, const char *global_name)
{
	const ClonGlobalsList *list = clon->globalsList;

	while (list)
	{
		for (u32 i = 0; i < list->globalsCount; ++i)
		{
			const ClonGlobal *global = &list->globals[i];

			if (StrEq(global->typeName, type_name) && StrEq(global->name, global_name) )
			{
				const void* globalData = global->data;
				return globalData;
			}
		}

		list = list->next;
	}

	return NULL;
}


#if 0
////////////////////////////////////////////////////////////////////////
// Clon types

enum ClonType
{
	ClonType_Value,
	ClonType_Array,
	ClonType_Struct,
	ClonType_COUNT,
};

struct ClonArray;
struct ClonStruct;

struct ClonArray
{
	u32 elemCount;
	ClonType elemType;
	union
	{
		void *values;
		ClonArray *arrays;
		ClonStruct *structs;
	};
};

struct ClonMember
{
	String name;
	ClonType type;
	union
	{
		void *valuePtr;
		ClonArray *arrayPtr;
		ClonStruct *structPtr;
	};
};

struct ClonStruct
{
	u32 memberCount;
	ClonMember *members;
};

typedef ClonMember ClonGlobal;

struct Clon
{
	u32 globalCount;
	ClonGlobal *globals;
};


////////////////////////////////////////////////////////////////////////
// Tokens

enum ClonTokenId
{
	// Single character tokens
	CLON_LEFT_PAREN,
	CLON_RIGHT_PAREN,
	CLON_LEFT_BRACE,
	CLON_RIGHT_BRACE,
	CLON_LEFT_BRACKET,
	CLON_RIGHT_BRACKET,
	CLON_COMMA,
	CLON_DOT,
	CLON_MINUS,
	CLON_PLUS,
	CLON_SEMICOLON,
	CLON_SLASH,
	CLON_STAR,
	// One or two-character tokens
	CLON_NOT,
	CLON_NOT_EQUAL,
	CLON_EQUAL,
	CLON_EQUAL_EQUAL,
	CLON_GREATER,
	CLON_GREATER_EQUAL,
	CLON_LESS,
	CLON_LESS_EQUAL,
	CLON_AND,
	CLON_ANDAND,
	CLON_OR,
	CLON_OROR,
	// Literals
	CLON_IDENTIFIER,
	CLON_STRING,
	CLON_NUMBER,
	// Keywords
	CLON_STRUCT,
	CLON_ENUM,
	CLON_TRUE,
	CLON_FALSE,
	CLON_STATIC,
	CLON_CONST,
	CLON_UNSIGNED,
	CLON_SHORT,
	CLON_LONG,
	CLON_BOOL,
	CLON_CHAR,
	CLON_INT,
	CLON_FLOAT,
	CLON_NULL, // ad-hoc value
	CLON_ARR_COUNT, // ad-hoc macro
	CLON_EOF,
	CLON_TOKEN_ID_COUNT,
};

static const char *CTokenIdNames[] =
{
	// Single character tokens
	"CLON_LEFT_PAREN",
	"CLON_RIGHT_PAREN",
	"CLON_LEFT_BRACE",
	"CLON_RIGHT_BRACE",
	"CLON_LEFT_BRACKET",
	"CLON_RIGHT_BRACKET",
	"CLON_COMMA",
	"CLON_DOT",
	"CLON_MINUS",
	"CLON_PLUS",
	"CLON_SEMICOLON",
	"CLON_SLASH",
	"CLON_STAR",
	// One or two-character tokens
	"CLON_NOT",
	"CLON_NOT_EQUAL",
	"CLON_EQUAL",
	"CLON_EQUAL_EQUAL",
	"CLON_GREATER",
	"CLON_GREATER_EQUAL",
	"CLON_LESS",
	"CLON_LESS_EQUAL",
	"CLON_AND",
	"CLON_ANDAND",
	"CLON_OR",
	"CLON_OROR",
	// Literals
	"CLON_IDENTIFIER",
	"CLON_STRING",
	"CLON_NUMBER",
	// Keywords
	"CLON_STRUCT",
	"CLON_ENUM",
	"CLON_TRUE",
	"CLON_FALSE",
	"CLON_STATIC",
	"CLON_CONST",
	"CLON_UNSIGNED",
	"CLON_SHORT",
	"CLON_LONG",
	"CLON_BOOL",
	"CLON_CHAR",
	"CLON_INT",
	"CLON_FLOAT",
	"CLON_NULL",
	"CLON_ARR_COUNT",
	"CLON_EOF",
};

CT_ASSERT(ARRAY_COUNT(CTokenIdNames) == CLON_TOKEN_ID_COUNT);
#endif

#endif // #ifndef CLON_H

