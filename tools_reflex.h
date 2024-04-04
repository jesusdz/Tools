/*
 * tools_reflex.h
 * Author: Jesus Diaz Garcia
 */

#ifndef TOOLS_REFLEX_H
#define TOOLS_REFLEX_H

typedef u16 ReflexID;

enum // ReflexID
{
	ReflexID_Bool,
	ReflexID_Char,
	ReflexID_Int,
	ReflexID_UInt,
	ReflexID_Float,
	ReflexID_Float3,
	ReflexID_String,
	ReflexID_Struct,
	ReflexID_COUNT,
	ReflexID_TrivialFirst = ReflexID_Bool,
	ReflexID_TrivialLast = ReflexID_String,
	ReflexID_TrivialCount = ReflexID_TrivialLast - ReflexID_TrivialFirst + 1,
};

struct ReflexTrivial
{
	u8 isBool : 1;
	u8 isChar : 1;
	u8 isFloat : 1;
	u8 isUnsigned : 1; // !bool and !float
	u8 size : 5;
	u8 elemCount;
};

struct ReflexEnumerator
{
	const char *name;
	u16 value;
};

struct ReflexEnum
{
	const char *name;
	const ReflexEnumerator *enumerators;
	u16 enumeratorCount;
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


// NOTE: Function generated by the reflection tool.
const ReflexStruct* ReflexGetStruct(ReflexID id);

inline bool ReflexIsTrivial(ReflexID id)
{
	const bool isTrivial = id >= ReflexID_TrivialFirst && id <= ReflexID_TrivialLast;
	return isTrivial;
}

inline bool ReflexIsStruct(ReflexID id)
{
	const bool isStruct = id >= ReflexID_Struct;
	return isStruct;
}

inline const void *ReflexMemberPtr(const void *structBase, const ReflexMember *member)
{
	const void *memberPtr = (u8*)structBase + member->offset;
	if ( member->isPointer ) {
		// Extra indirection to get the pointed data address
		const void *pointedMemberPtr = *((void**)memberPtr);
		return pointedMemberPtr;
	} else {
		return memberPtr;
	}
}

const ReflexTrivial* ReflexGetTrivial(ReflexID id)
{
	ASSERT(ReflexIsTrivial(id));
	static const ReflexTrivial trivials[] = {
		{ .isBool = 1, .isChar = 0, .isFloat = 0, .isUnsigned = 0, .size = sizeof(bool), .elemCount = 1 },
		{ .isBool = 0, .isChar = 1, .isFloat = 0, .isUnsigned = 0, .size = sizeof(char), .elemCount = 1 },
		{ .isBool = 0, .isChar = 0, .isFloat = 0, .isUnsigned = 0, .size = sizeof(int), .elemCount = 1 },
		{ .isBool = 0, .isChar = 0, .isFloat = 0, .isUnsigned = 1, .size = sizeof(unsigned int), .elemCount = 1 },
		{ .isBool = 0, .isChar = 0, .isFloat = 1, .isUnsigned = 0, .size = sizeof(float), .elemCount = 1 },
		{ .isBool = 0, .isChar = 0, .isFloat = 1, .isUnsigned = 0, .size = sizeof(float), .elemCount = 3 },
		{ .isBool = 0, .isChar = 0, .isFloat = 0, .isUnsigned = 0, .size = 4, .elemCount = 1 },
	};
	CT_ASSERT(ARRAY_COUNT(trivials) == ReflexID_TrivialCount);
	return &trivials[id];
}

u32 ReflexGetTypeSize(ReflexID id)
{
	if (ReflexIsTrivial(id))
	{
		const ReflexTrivial *trivial = ReflexGetTrivial(id);
		const u32 size = trivial->size * trivial->elemCount;
		return size;
	}
	else if (ReflexIsStruct(id))
	{
		const ReflexStruct* rstruct = ReflexGetStruct(id);
		const u32 size = rstruct->size;
		return size;
	}
	else
	{
		INVALID_CODE_PATH();
	}
	return 0;
}

u32 ReflexGetElemCount( const void *data, const ReflexStruct *rstruct, const char *memberName )
{
	for (u32 i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const bool isPointer = member->isPointer;
		const u32 reflexId = member->reflexId;

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

#endif // #ifndef TOOLS_REFLEX_H

