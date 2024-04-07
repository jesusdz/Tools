/*
 * clon.h
 * Author: Jesus Diaz Garcia
 * CLON stands for C-like object notation.
 * It is a text format designed to store data in a descriptive and similar way to the C language.
 */

#ifndef CLON_H
#define CLON_H

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

struct Clon
{
	u32 structCount;
	ClonStruct *structs;

	u32 arrayCount;
	ClonArray *arrays;
};

#endif // #ifndef CLON_H

