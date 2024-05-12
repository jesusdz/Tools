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

void ClonFillTrivial(void *dataPtr, ReflexID reflexId, const CastInitializer *initializer, const Clon *clon)
{
	const CastExpression *expression = CAST_CHILD(initializer, expression);
	if (expression)
	{
		if (expression->type == CAST_EXPR_NUMBER)
		{
			if (reflexId == ReflexID_Bool) {
				*(bool*)dataPtr = Cast_EvaluateBool(expression);
			} else if (reflexId == ReflexID_Char) {
				*(char*)dataPtr = Cast_EvaluateChar(expression);
			} else if (reflexId == ReflexID_UnsignedChar) {
				*(unsigned char*)dataPtr = Cast_EvaluateUnsignedChar(expression);
			} else if (reflexId == ReflexID_Int) {
				*(int*)dataPtr = Cast_EvaluateInt(expression);
			} else if (reflexId == ReflexID_ShortInt) {
				*(short int*)dataPtr = Cast_EvaluateShortInt(expression);
			} else if (reflexId == ReflexID_LongInt) {
				*(long int*)dataPtr = Cast_EvaluateLongInt(expression);
			} else if (reflexId == ReflexID_LongLongInt) {
				*(long long int*)dataPtr = Cast_EvaluateLongLongInt(expression);
			} else if (reflexId == ReflexID_UnsignedInt) {
				*(unsigned int*)dataPtr = Cast_EvaluateUnsignedInt(expression);
			} else if (reflexId == ReflexID_UnsignedShortInt) {
				*(unsigned short int*)dataPtr = Cast_EvaluateUnsignedShortInt(expression);
			} else if (reflexId == ReflexID_UnsignedLongInt) {
				*(unsigned long int*)dataPtr = Cast_EvaluateUnsignedLongInt(expression);
			} else if (reflexId == ReflexID_UnsignedLongLongInt) {
				*(unsigned long long int*)dataPtr = Cast_EvaluateUnsignedLongLongInt(expression);
			} else if (reflexId == ReflexID_Float) {
				*(float*)dataPtr = Cast_EvaluateFloat(expression);
			} else if (reflexId == ReflexID_Double) {
				*(double*)dataPtr = Cast_EvaluateDouble(expression);
			} else {
				LOG(Error, "Void member does not expect any number.\n");
			}
		}
		else if (expression->type == CAST_EXPR_STRING)
		{
			// TODO: Avoid this allocation
			char *str = new char[expression->constant.size+1];
			StrCopyN(str, expression->constant.str, expression->constant.size);
			*(char **)dataPtr = str;
		}
		else if (expression->type == CAST_EXPR_ARRAY_COUNT)
		{
			char globalName[128];
			StrCopyN(globalName, expression->constant.str, expression->constant.size);
			const ClonGlobal *clonGlobal = ClonGetGlobal(clon, globalName);
			if (clonGlobal)
			{
				*(unsigned int*)dataPtr = clonGlobal->elemCount;
			}
			else
			{
				*(unsigned int*)dataPtr = 1;
				LOG(Error,"Could not find global %s for ARRAY_COUNT.\n", globalName);
			}
		}
		else if (expression->type == CAST_EXPR_IDENTIFIER)
		{
			LOG(Debug, "Identifier?\n");
		}
		else
		{
			LOG(Error, "Invalid expression type (%u) to initialize trivial type.\n", expression->type);
		}
	}
	else
	{
		LOG(Error, "Invalid code path: No expression provided... better handle this error in the calling code.\n");
	}
}

void ClonFillStruct(void *structData, const ReflexStruct *rstruct, const CastInitializer *initializer, const Clon *clon)
{
	const CastInitializerList *membersInitializerList = CAST_CHILD(initializer, initializerList);

	for (u32 i = 0; i < rstruct->memberCount && membersInitializerList; ++i)
	{
		const ReflexMember *member = rstruct->members + i;

		const CastDesignator *designator = CAST_CHILD(membersInitializerList, designation, designatorList, designator);

		if (!designator || designator && StrEqN(member->name, designator->identifier.str, designator->identifier.size))
		{
			byte *memberPtr = (byte*)ReflexGetMemberPtr(structData, member);
			const u32 elemSize = member->pointerCount > 0 ? sizeof(void*) : ReflexGetTypeSize(member->reflexId);
			const u32 numElems = member->isArray ? member->arrayDim : 1;
			ASSERT(member->pointerCount < 2);

			const CastInitializer *memberInitializer = CAST_CHILD(membersInitializerList, initializer);

			// NOTE: Used for arrayed or struct elements (initializations within braces { ... })
			const CastInitializerList *elementInitializerList = CAST_CHILD(memberInitializer, initializerList);

			for (u32 elemIndex = 0; elemIndex < numElems; ++elemIndex)
			{
				const CastInitializer *elementInitializer = memberInitializer;
				if (member->isArray) {
					elementInitializer = CAST_CHILD(elementInitializerList, initializer);
					elementInitializerList = elementInitializerList->next;
				}

				const CastExpression *expression = CAST_CHILD(elementInitializer, expression);

				byte *elemPtr = memberPtr + elemIndex * elemSize;

				if (ReflexIsTrivial(member->reflexId))
				{
					ClonFillTrivial(elemPtr, member->reflexId, elementInitializer, clon);
				}
				else if (ReflexIsStruct(member->reflexId))
				{
					const ReflexStruct *rstruct2 = ReflexGetStruct(member->reflexId);

					if (member->pointerCount == 0)
					{
						ClonFillStruct(elemPtr, rstruct2, elementInitializer, clon);
					}
					else if (member->pointerCount == 1)
					{
						if (expression->type == CAST_EXPR_IDENTIFIER)
						{
							// TODO: Avoid this allocation
							char *globalName = new char[expression->constant.size+1];
							StrCopyN(globalName, expression->constant.str, expression->constant.size);
							const ClonGlobal *clonGlobal = ClonGetGlobal(clon, rstruct2->name, globalName);
							*(void**)elemPtr = clonGlobal->data;
						}
						else
						{
							LOG(Error, "Unsupported expression type for member %s of type %s*.\n", member->name, rstruct2->name);
						}
					}
					else
					{
						LOG(Error, "Unsupported more than one level of pointer indirections.\n");
					}
				}
				else
				{
					LOG(Error, "Invalid code path: Unhandled type for member %s.\n", member->name);
				}
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

#endif // #ifndef CLON_H

