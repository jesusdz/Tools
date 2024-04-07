/*
 * clon.h
 * Author: Jesus Diaz Garcia
 * CLON stands for C-like object notation.
 * It is a text format designed to store data in a descriptive and similar way to the C language.
 */

#ifndef CLON_H
#define CLON_H

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

#endif // #ifndef CLON_H

