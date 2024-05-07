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

bool ClonParse(Arena *arena, const char *data, u32 dataSize, Clon *clon)
{
	const Cast *cast = Cast_Create(*arena, data, dataSize);
	if (cast)
	{
		const CastTranslationUnit *translationUnit = CAST_CHILD(cast, translationUnit);

		while (translationUnit)
		{
			const CastDeclaration *declaration = CAST_CHILD(translationUnit, externalDeclaration, declaration);
			const CastDeclarator *declarator = CAST_CHILD(declaration, initDeclaratorList, initDeclarator, declarator);
			const CastDirectDeclarator *directDeclarator = CAST_CHILD(declarator, directDeclarator);

			if (directDeclarator)
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
					char typeName[MAX_PATH_LENGTH] = {};
					StrCopy(typeName, typeSpecifier->identifier);

					CastString globalName = directDeclarator->name;

					const ReflexStruct *rstruct = ReflexGetStructFromName(typeName);

					if (rstruct)
					{
						const u32 typeSize = rstruct->size;
						const u32 elemCountFromInitializerList = 1; // TODO get this from initializer list
						const u32 elemCount = !directDeclarator->isArray ? 1 :
							directDeclarator->expression ? Cast_EvaluateInt(directDeclarator->expression):
							elemCountFromInitializerList;
						const u32 globalSize = typeSize * elemCount;
						printf("Global %.*s of type %s\n", globalName.size, globalName.str, typeName);
						printf(" - typeSize: %u\n", typeSize);
						printf(" - elemCount: %u\n", elemCount);
						//printf("Global %.*s\n", globalsName.size, globalsName.str);
					}
				}
			}

			translationUnit = translationUnit->next;
		}
	}
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

