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

void Print(const void *data, const ReflexID id, bool isPointer, bool isDoublePointer, bool isArray, u16 arrayDim)
{
	if ( ReflexIsTrivial(id) )
	{
		const ReflexTrivial *trivial = ReflexGetTrivial(id);

		if (isArray) {
			Printf("[");
		}

		const u32 elemCount = isArray ? arrayDim : 1;

		for (u32 i = 0; i < elemCount; ++i)
		{
			if (i > 0) {
				Printf(", ");
			}

			if (trivial->isBool) {
				const bool bVal = *((bool*)data + i);
				Printf("%d", bVal ? 1 : 0);
			} else if (trivial->isChar) {
				if (isPointer) {
					const char *sVal = (const char *)data;
					Printf("\"%s\"", sVal);
				} else {
					const char cVal = *((char*)data + i);
					Printf("%c", cVal);
				}
			} else if (trivial->isFloat) {
				const float fVal = *((float*)data + i);
				Printf("%f", fVal);
			} else if (trivial->isUnsigned) {
				const u32 uVal = *((u32*)data + i);
				Printf("%u", uVal);
			} else {
				const i32 iVal = *((i32*)data + i);
				Printf("%d", iVal);
			}

		}

		if (isArray) {
			Printf("]");
		}
	}
	else
	{
		const ReflexStruct *rstruct = ReflexGetStruct(id);

		PrintBeginScope("{");
		PrintNewLine();

		for (u32 i = 0; i < rstruct->memberCount; ++i)
		{
			const ReflexMember *member = &rstruct->members[i];
			const void *structPtr = data;
			const void *memberPtr = ReflexGetMemberPtr(structPtr, member);

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

				if ( ReflexIsStruct(member->reflexId) ) {
					Print(elemPtr, member->reflexId, false, false, false, 1);
				} else {
					Print(elemPtr, member->reflexId, member->isPointer, member->isDoublePointer,  member->isArray, member->arrayDim);
				}

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
	Print(&gAssets, REFLEX_ID(Assets), false, false, false, 1);

	return 0;
}

