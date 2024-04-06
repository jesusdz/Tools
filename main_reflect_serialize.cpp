#include "tools.h"
#include "tools_reflex.h"
#include "assets.h"
#include <cstddef> // offsetof

////////////////////////////////////////////////////////////////////////
// Print utils

#define Printf( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintBeginScope( format, ... ) LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ ); IndentationIncrease();
#define PrintEndScope( format, ... ) IndentationDecrease(); LOG(Info, "%s" format, Indentation(), ##__VA_ARGS__ );
#define PrintNewLine() LOG(Info, "\n"); indent.apply = true;

struct IndentationState
{
	bool apply;
	u32 pos;
	char padding[64];
};

static IndentationState indent = {
	.apply = true,
	.pos = 0,
	.padding = "",
};

static void IndentationIncrease()
{
	indent.padding[indent.pos++] = '\t';
}

static void IndentationDecrease()
{
	ASSERT(indent.pos > 0);
	indent.padding[--indent.pos] = '\0';
}

static const char *Indentation()
{
	const char *padding = indent.apply ? indent.padding : "";
	indent.apply = false;
	return padding;
}


////////////////////////////////////////////////////////////////////////
// Automatic implementation (using reflection)

#define REFLEX_ID(TypeName) ReflexID_ ## TypeName

#include "assets.reflex.h"

void PrintTrivial(const void *data, const ReflexID id, bool isString)
{
	const ReflexTrivial *trivial = ReflexGetTrivial(id);

	ASSERT(!isString || trivial->isChar);

	if (trivial->isBool) {
		const bool bVal = *((bool*)data);
		Printf("%d", bVal ? 1 : 0);
	} else if (isString) {
		const char *sVal = (const char *)data;
		Printf("\"%s\"", sVal);
	} else if (trivial->isChar) {
		const char cVal = *((char*)data);
		Printf("%c", cVal);
	} else if (trivial->isFloat) {
		const float fVal = *((float*)data);
		Printf("%f", fVal);
	} else if (trivial->isUnsigned) {
		const u32 uVal = *((u32*)data);
		Printf("%u", uVal);
	} else {
		const i32 iVal = *((i32*)data);
		Printf("%d", iVal);
	}
}

void Print(const void *data, const ReflexID id)
{
	const ReflexStruct *rstruct = ReflexGetStruct(id);
	const void *structPtr = data;

	PrintBeginScope("{");
	PrintNewLine();

	for (u32 i = 0; i < rstruct->memberCount; ++i)
	{
		const ReflexMember *member = &rstruct->members[i];
		const ReflexID reflexId = member->reflexId;
		const bool isTrivial = ReflexIsTrivial(reflexId);
		const bool isArray = member->isArray;
		const bool isPointer = member->isPointer;
		const bool isDoublePointer = member->isDoublePointer;
		const u16 arrayDim = member->arrayDim;

		Printf("\"%s\" : ", member->name);

		const u32 typeSize = ReflexGetTypeSize(reflexId);
		const u32 elemSize =
			isArray && isPointer ? sizeof(void*) :
			isDoublePointer ? sizeof(void*) :
			typeSize;

		const u32 elemCount =
			isArray ? arrayDim :
			isPointer ? ReflexGetElemCount(structPtr, rstruct, member->name) :
			0;

		if ( elemCount > 0 ) {
			PrintBeginScope("[");
			if (!isTrivial) {
				PrintNewLine();
			}
		}

		const bool isString = isPointer && reflexId == ReflexID_Char;
		const void *memberPtr = ReflexGetMemberPtr(structPtr, member);

		u32 indirections =
			isDoublePointer ? 2 :
			isPointer ? 1 :
			0;

		// If the member is a pointed array (no fixed length), we dereference the pointer
		// to get the base address of the pointed array. For fixed length arrays (isArray)
		// there's no need for dereferencing, memberPtr already points to the base address.
		if ( !isArray && isPointer ) {
			memberPtr = *(void**)memberPtr;
			indirections--;
		}

		for (u32 elemIndex = 0; elemIndex == 0 || elemIndex < elemCount; ++elemIndex)
		{
			const void *elemPtr = (u8*)memberPtr + elemIndex * elemSize;

			// We apply the remaining indirections until getting the value address.
			for (u32 i = 0; i < indirections; ++i) {
				elemPtr = *(void**)elemPtr;
			}

			if (isTrivial) {
				PrintTrivial(elemPtr, reflexId, isString);
			} else {
				Print(elemPtr, reflexId);
			}

			if (elemCount > 0 && elemIndex + 1 < elemCount) {
				Printf(",");
				if (isTrivial) {
					Printf(" ");
				} else {
					PrintNewLine();
				}
			}
		}

		if ( elemCount > 0 ) {
			if (!isTrivial) {
				PrintNewLine();
			}
			PrintEndScope("]");
		}

		if ( i + 1 < rstruct->memberCount ) {
			Printf(",");
		}
		PrintNewLine();

	}

	PrintEndScope("}");
}


////////////////////////////////////////////////////////////////////////
//  Main function

int main(int argc, char **argv)
{
	Print(&gAssets, REFLEX_ID(Assets));

	return 0;
}

