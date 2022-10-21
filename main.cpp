#include <stdio.h>
#include <assert.h>



////////////////////////////////////////////////////////////////////////////////////////////////////
// Base types

typedef char i8;
typedef short int i16;
typedef int i32;
typedef int i32;
typedef long long int i64;
typedef unsigned char u8;
typedef unsigned short int u16;
typedef unsigned int u32;
typedef unsigned int u32;
typedef unsigned long long int u64;
typedef float f32;
typedef double f64;
typedef	unsigned char byte;

#define KB(x) (1024 * x)
#define MB(x) (1024 * KB(x))
#define TB(x) (1024 * MB(x))

#define ASSERT(expression) assert(expression)
#define ERROR(message) ASSERT( 0 && message )



////////////////////////////////////////////////////////////////////////////////////////////////////
// Strings

void StrCopy(char *dst, const char *src, u32 size)
{
	while (size-- > 0) *dst++ = *src++;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Memory

struct Arena
{
	byte* base;
	u32 used;
	u32 size;
};

Arena MakeArena(byte* base, u32 size)
{
	Arena arena = {};
	arena.base = base;
	arena.size = size;
	arena.used = 0;
	return arena;
}

Arena MakeSubArena(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "MakeSubArena of bounds of the memory arena.");
	Arena subarena = {};
	subarena.base = arena.base + arena.used;
	subarena.size = size;
	subarena.used = 0;
	return subarena;
}

byte* PushSize(Arena &arena, u32 size)
{
	ASSERT(arena.used + size <= arena.size && "PushSize of bounds of the memory arena.");
	byte* head = arena.base + arena.used;
	arena.used += size;
	return head;
}

char* PushCString(Arena &arena, const char* str, u32 size)
{
	char* dest = (char*)PushSize(arena, size + 1);
	StrCopy(dest, str, size);
	dest[size] = '\0';
	return dest;
}

#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

u32 ReadEntireFile(const char *filename, char *bytes, u32 bytesSize)
{
	u32 bytesRead = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		bytesRead = fread(bytes, sizeof(unsigned char), bytesSize, f);
		ASSERT(bytesRead <= bytesSize);
		bytes[bytesRead] = 0;
		fclose(f);
	}
	return bytesRead;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Scanner


#define COMMAND_NAME "jsl"
#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)
#define MAX_LEXEME_SIZE 128


// Enum values
#define ENUM_ENTRY(entryName) entryName,
enum TokenType {
#include "token_list.h"
};
#undef ENUM_ENTRY



// Enum names
#define ENUM_ENTRY(entryName) #entryName,
const char *TokenNames[] = {
#include "token_list.h"
};
#undef ENUM_ENTRY



Arena globalArena;



enum LiteralType
{
	LITERAL_BOOL,
	LITERAL_INT,
	LITERAL_FLOAT,
	LITERAL_STRING,
};

struct Literal
{
	LiteralType type;
	union
	{
		bool b;
		i32 i;
		f32 f;
		char* s;
	};
};

struct Token
{
    TokenType type;
	char* lexeme;
	Literal literal;
	i32 line;
};

struct TokenList
{
	Token tokens[MAX_TOKEN_COUNT];
	i32 count;
};

struct ScanState
{
	i32 start;
	i32 current;
	i32 line;

	const char *script;
	char scriptSize;
	Arena stringArena;
};

const char* TokenName(TokenType type)
{
	return TokenNames[type];
}

bool IsEOL(char character)
{
	return character == '\n';
}

bool IsAtEnd(const ScanState &scanState)
{
	return scanState.current >= scanState.scriptSize;
}

char Advance(ScanState &scanState)
{
	ASSERT(scanState.current < scanState.scriptSize);
	return scanState.script[scanState.current++];
}

char Peek(const ScanState &scanState)
{
	return IsAtEnd(scanState) ? '\0' : scanState.script[ scanState.current ];
}

bool Consume(ScanState &scanState, char expected)
{
	if ( IsAtEnd(scanState) ) return false;
	if ( scanState.script[scanState.current] != expected ) return false;
	scanState.current++;
	return true;
}

void AddToken(ScanState &scanState, TokenList &tokenList, TokenType tokenType, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenType;

	const char *lexemeStart = scanState.script + scanState.start;
	u32 lexemeSize = scanState.current - scanState.start;
	newToken.lexeme = PushCString(scanState.stringArena, lexemeStart, lexemeSize);
	newToken.literal = literal;
	newToken.line = scanState.line;

	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(ScanState &scanState, TokenList &tokenList, TokenType tokenType)
{
	Literal nullLiteral = {};
	AddToken(scanState, tokenList, tokenType, nullLiteral);
}

void ScanToken(ScanState &scanState, TokenList &tokenList)
{
	char c = Advance(scanState);

	switch (c)
	{
		case '(': AddToken(scanState, tokenList, TOKEN_LEFT_PAREN); break;
		case ')': AddToken(scanState, tokenList, TOKEN_RIGHT_PAREN); break;
		case '{': AddToken(scanState, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': AddToken(scanState, tokenList, TOKEN_RIGHT_BRACE); break;
		case ',': AddToken(scanState, tokenList, TOKEN_COMMA); break;
		case '.': AddToken(scanState, tokenList, TOKEN_DOT); break;
		case '-': AddToken(scanState, tokenList, TOKEN_MINUS); break;
		case '+': AddToken(scanState, tokenList, TOKEN_PLUS); break;
		case '*': AddToken(scanState, tokenList, TOKEN_STAR); break;
		case ';': AddToken(scanState, tokenList, TOKEN_SEMICOLON); break;
		case '!': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT); break;
		case '=': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
		case '<': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
		case '>': AddToken(scanState, tokenList, Consume(scanState, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
		case '/':
			if ( Consume(scanState, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !IsEOL( Peek(scanState) ) && !IsAtEnd(scanState) ) Advance(scanState);
			}
			else
			{
				AddToken(scanState, tokenList, TOKEN_SLASH);
			}
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanState.line++;
			break;

		case '\"':
			while ( Peek(scanState) != '\"' && !IsAtEnd(scanState) )
			{
				if ( IsEOL( Peek(scanState) ) ) scanState.line++;
				Advance(scanState);
			}

			if ( IsAtEnd(scanState) )
			{
				ERROR( "Unterminated string.");
			}

			Advance(scanState);

			AddToken(scanState, tokenList, TOKEN_STRING);
			break;

		default:;
			// TODO(jesus): Substitute this by a proper error call
			//ERROR( "Unexpected character");
	}
}

TokenList ScanTokens(const char *script, u32 scriptSize)
{
	TokenList tokenList = {};

	ScanState scanState = {};
	scanState.line = 1;
	scanState.script = script;
	scanState.scriptSize = scriptSize;

	u32 stringArenaSize = KB(128);
	scanState.stringArena = MakeSubArena(globalArena, stringArenaSize);

	while ( !IsAtEnd(scanState) )
	{
		scanState.start = scanState.current;
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	return tokenList;
}

void PrintTokenList(const TokenList &tokenList)
{
	printf("List of tokens:\n");
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		TokenType tokenId = tokenList.tokens[i].type;
		printf("%s\t: %s\n", TokenNames[tokenId], tokenList.tokens[i].lexeme);
	}
	printf("\n");
}

void Run(const char *script, u32 scriptSize)
{
	TokenList tokenList = ScanTokens(script, scriptSize);

	PrintTokenList(tokenList);

	for (u32 tokenIdx = 0; tokenIdx < tokenList.count; ++tokenIdx)
	{
		const Token &token = tokenList.tokens[tokenIdx];
		printf("%s ", TokenName(token.type));
	}
	printf("\n");
}

void RunFile(const char* filename)
{
	u32 bufferSize = KB(1);
	char *bytes = new char[bufferSize]();
	u32 bytesRead = ReadEntireFile(filename, bytes, bufferSize);
	Run(bytes, bytesRead);
	delete[] bytes;
}

void RunPrompt()
{
	char line[MAX_LINE_SIZE];
	u32 lineSize = 0;
	for (;;)
	{
		printf("> ");
		// TODO: Read line
		// TODO: Break if exit command
		Run(line, lineSize);
	}
}

int main(int argc, char **argv)
{
	u32 globalArenaSize = MB(1);
	byte* globalArenaBase = new byte[globalArenaSize]();
	globalArena = MakeArena(globalArenaBase, globalArenaSize);

	if ( argc > 2 )
	{
		printf("Usage: %s [script]\n", COMMAND_NAME);
		return -1;
	}
	else if ( argc == 2 )
	{
		RunFile(argv[1]);
	}
	else
	{
		RunPrompt();
	}

	delete[] globalArenaBase;

    return 0;
}

