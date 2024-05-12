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
	u32 elemCount; // Number of elements in the array
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

void ClonAddGlobal(Arena *arena, Clon *clon, const char *typeName, const char *name, void *data, u32 elemCount)
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
	global->elemCount = elemCount;
}

const ClonGlobal *ClonGetGlobal(const Clon *clon, const char *type_name, const char *global_name)
{
	const ClonGlobalsList *list = clon->globalsList;

	while (list)
	{
		for (u32 i = 0; i < list->globalsCount; ++i)
		{
			const ClonGlobal *global = &list->globals[i];

			if (StrEq(global->typeName, type_name) && StrEq(global->name, global_name))
			{
				return global;
			}
		}

		list = list->next;
	}

	return NULL;
}

const ClonGlobal *ClonGetGlobal(const Clon *clon, const char *global_name)
{
	const ClonGlobalsList *list = clon->globalsList;

	while (list)
	{
		for (u32 i = 0; i < list->globalsCount; ++i)
		{
			const ClonGlobal *global = &list->globals[i];

			if (StrEq(global->name, global_name))
			{
				return global;
			}
		}

		list = list->next;
	}

	return NULL;
}

void ClonFillStruct(void *globalData, const ReflexStruct *rstruct, const CastInitializer *initializer, const Clon *clon)
{
	const CastInitializerList *membersInitializerList = CAST_CHILD(initializer, initializerList);
	//printf("%s\n", rstruct->name);

	for (u32 i = 0; i < rstruct->memberCount && membersInitializerList; ++i)
	{
		const ReflexMember *member = rstruct->members + i;
		//printf("- %s\n", member->name);

		const CastDesignator *designator = CAST_CHILD(membersInitializerList, designation, designatorList, designator);
		if (designator && StrEqN(member->name, designator->identifier.str, designator->identifier.size))
		{
			//LOG(Info, "  - OK\n");
			const CastInitializer *memberInitializer = CAST_CHILD(membersInitializerList, initializer);
			const CastExpression *expression = CAST_CHILD(memberInitializer, expression);

			if (expression) // TODO: expression cannot be here to initialize members that are arrays
			{
				byte *memberPtr = (byte*)ReflexGetMemberPtr(globalData, member);
				const u32 elemSize = member->pointerCount > 0 ? sizeof(void*) : ReflexGetTypeSize(member->reflexId);
				const u32 numElems = member->isArray ? member->arrayDim : 1;
				ASSERT(member->pointerCount < 2);

				for (u32 elemIndex = 0; elemIndex < numElems; ++elemIndex)
				{
					byte *elemPtr = memberPtr + elemIndex * elemSize;

					if (ReflexIsTrivial(member->reflexId))
					{
						if (expression->type == CAST_EXPR_NUMBER)
						{
							if (member->reflexId == ReflexID_Bool) {
								*(bool*)elemPtr = Cast_EvaluateBool(expression);
							} else if (member->reflexId == ReflexID_Char) {
								*(char*)elemPtr = Cast_EvaluateChar(expression);
							} else if (member->reflexId == ReflexID_UnsignedChar) {
								*(unsigned char*)elemPtr = Cast_EvaluateUnsignedChar(expression);
							} else if (member->reflexId == ReflexID_Int) {
								*(int*)elemPtr = Cast_EvaluateInt(expression);
							} else if (member->reflexId == ReflexID_ShortInt) {
								*(short int*)elemPtr = Cast_EvaluateShortInt(expression);
							} else if (member->reflexId == ReflexID_LongInt) {
								*(long int*)elemPtr = Cast_EvaluateLongInt(expression);
							} else if (member->reflexId == ReflexID_LongLongInt) {
								*(long long int*)elemPtr = Cast_EvaluateLongLongInt(expression);
							} else if (member->reflexId == ReflexID_UnsignedInt) {
								*(unsigned int*)elemPtr = Cast_EvaluateUnsignedInt(expression);
							} else if (member->reflexId == ReflexID_UnsignedShortInt) {
								*(unsigned short int*)elemPtr = Cast_EvaluateUnsignedShortInt(expression);
							} else if (member->reflexId == ReflexID_UnsignedLongInt) {
								*(unsigned long int*)elemPtr = Cast_EvaluateUnsignedLongInt(expression);
							} else if (member->reflexId == ReflexID_UnsignedLongLongInt) {
								*(unsigned long long int*)elemPtr = Cast_EvaluateUnsignedLongLongInt(expression);
							} else if (member->reflexId == ReflexID_Float) {
								*(float*)elemPtr = Cast_EvaluateFloat(expression);
							} else if (member->reflexId == ReflexID_Double) {
								*(double*)elemPtr = Cast_EvaluateDouble(expression);
							} else {
								LOG(Error, "Void member does not expect any number.\n");
							}
						}
						else if (expression->type == CAST_EXPR_STRING)
						{
							// TODO: Avoid this allocation
							char *str = new char[expression->constant.size+1];
							StrCopyN(str, expression->constant.str, expression->constant.size);
							*(char **)elemPtr = str;
						}
						else if (expression->type == CAST_EXPR_ARRAY_COUNT)
						{
							char globalName[128];
							StrCopyN(globalName, expression->constant.str, expression->constant.size);
							const ClonGlobal *clonGlobal = ClonGetGlobal(clon, globalName);
							if (clonGlobal)
							{
								*(unsigned int*)elemPtr = clonGlobal->elemCount;
							}
							else
							{
								*(unsigned int*)elemPtr = 1;
								LOG(Error,"Could not find global %s for ARRAY_COUNT.\n", globalName);
							}
						}
						else if (expression->type == CAST_EXPR_IDENTIFIER)
						{
							LOG(Debug, "Identifier?\n");
						}
						else
						{
							LOG(Error, "Invalid expression type (%u) to initialize member %s.\n", expression->type, member->name);
						}
					}
					else if (ReflexIsStruct(member->reflexId))
					{
						if (expression->type == CAST_EXPR_IDENTIFIER)
						{
							const ReflexStruct *rstruct2 = ReflexGetStruct(member->reflexId);

							if (member->pointerCount == 0)
							{
								LOG(Info, "WTF %s\n", member->name);
								const CastInitializerList *initializerList2 = memberInitializer->initializerList;
								ClonFillStruct(elemPtr, rstruct2, memberInitializer, clon);
							}
							else if (member->pointerCount == 1)
							{
								// TODO: Avoid this allocation
								char *globalName = new char[expression->constant.size+1];
								StrCopyN(globalName, expression->constant.str, expression->constant.size);
								const ClonGlobal *clonGlobal = ClonGetGlobal(clon, rstruct2->name, globalName);
								*(void**)elemPtr = clonGlobal->data;
							}
							else
							{
								LOG(Error, "Unsupported more than one level of pointer indirections.\n");
							}
						}
						else
						{
							LOG(Error, "Invalid code path: Unhandled expression type for member %s.\n", member->name);
						}
					}
					else
					{
						LOG(Error, "Invalid code path: Unhandled type for member %s.\n", member->name);
					}
				}
			}
			else
			{
				LOG(Error, "Invalid code path: No expression for designator .%s.\n", member->name );
			}

			membersInitializerList = membersInitializerList->next;
		}
		else
		{
			LOG(Errpr, "Invalid code path: No-designated initializers are not supported.\n");
		}
	}
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

				// TODO: Consider that global vars can also be pointers
				u32 pointerCount = 0;
				const CastPointer *pointer = declarator->pointer;
				while (pointer)
				{
					pointerCount++;
					pointer = pointer->next;
				}

				if (typeSpecifier && typeSpecifier->type == CAST_IDENTIFIER)
				{
					// TODO: Unify string allocations
					char *globalTypeName = PushArray(*arena, char, typeSpecifier->identifier.size+1);
					StrCopy(globalTypeName, typeSpecifier->identifier);

					const ReflexStruct *rstruct = ReflexGetStructFromName(globalTypeName);

					if (rstruct)
					{
						const bool isArray = directDeclarator->isArray;
						const u32 typeSize = rstruct->size;
						const u32 elemCount = !isArray ? 1 :
							directDeclarator->expression ? Cast_EvaluateInt(directDeclarator->expression):
							initializer->initializerCount;
						const u32 globalSize = typeSize * elemCount;

						// TODO: Unify string allocations
						char *globalName = PushArray(*arena, char, directDeclarator->name.size+1);
						StrCopy(globalName, directDeclarator->name);

						void *globalData = PushSize(*arena, globalSize);
						ClonAddGlobal(arena, clon, globalTypeName, globalName, globalData, elemCount);

						if ( isArray )
						{
							const CastInitializerList *initializerList = CAST_CHILD(initializer, initializerList);
							u32 elemIndex = 0;

							while (initializerList)
							{
								const CastInitializer *structInitializer = CAST_CHILD(initializerList, initializer);
								ClonFillStruct((byte*)globalData + elemIndex * typeSize, rstruct, structInitializer, clon);
								initializerList = initializerList->next;
								elemIndex++;
							}
						}
						else
						{
							const CastInitializer *structInitializer = initializer;
							ClonFillStruct(globalData, rstruct, initializer, clon);
						}
					}
				}
			}

			translationUnit = translationUnit->next;
		}
	}

	return cast != NULL;
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

