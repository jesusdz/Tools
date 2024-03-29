/*
 * tools_clon.h
 * Author: Jesus Diaz Garcia
 * CLON stands for C-like object notation.
 * It is a text format designed to store data in a descriptive and similar way to the C language.
 */

#ifndef TOOLS_CLON_H
#define TOOLS_CLON_H

struct ClonArena
{
};

struct ClonNode
{
};

enum ClonType
{
	ClonType_Struct,
	ClonType_Array,
	ClonType_COUNT,
};

struct ClonStruct
{
};

struct ClonArray
{
};

struct Clon
{
	ClonType type;
	union
	{
		ClonStruct struc;
		ClonArray array;
	};
};

Clon Clon_Parse(ClonArena &arena, const char *text, unsigned int textLen);
void Clon_Print(const Clon &clon);

#if defined(TOOLS_CLON_IMPLEMENTATION)

// TODO

#endif

#endif // #ifndef TOOLS_CLON_H

