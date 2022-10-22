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

struct String
{
	const char* str;
	u32 size;
};

String MakeString(const char *str, u32 size)
{
	String string = { str, size };
	return string;
}

void StrCopy(char *dst, const char *src, u32 size)
{
	while (size-- > 0) *dst++ = *src++;
}

void StrCopy(char *dst, const String& src)
{
	StrCopy(dst, src.str, src.size);
	dst[src.size] = '\0';
}

bool StrEq(const String &s11, const char *s2)
{
	const char *s1 = s11.str;
	u32 count = s11.size;
	while ( count > 0 && *s1 == *s2 )
	{
		s1++;
		s2++;
		count--;
	}
	return count == 0 && *s2 == 0;
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

void ResetArena(Arena &arena)
{
	arena.used = 0;
}

#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * count)



////////////////////////////////////////////////////////////////////////////////////////////////////
// Files

u32 GetFileSize(const char *filename)
{
	u32 size = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fclose(f);
	}
	return size;
}

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
	String lexeme;
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
	u32 scriptSize;
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

bool IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

bool IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

bool IsAlphaNumeric(char character)
{
	return IsAlpha(character) || IsDigit(character);
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

bool Consume(ScanState &scanState, char expected)
{
	if ( IsAtEnd(scanState) ) return false;
	if ( scanState.script[scanState.current] != expected ) return false;
	scanState.current++;
	return true;
}

char Peek(const ScanState &scanState)
{
	return IsAtEnd(scanState) ? '\0' : scanState.script[ scanState.current ];
}

char PeekNext(const ScanState &scanState)
{
	if (scanState.current + 1 >= scanState.scriptSize) return '\0';
	return scanState.script[ scanState.current + 1 ];
}

String ScannedString(const ScanState &scanState)
{
	const char *lexemeStart = scanState.script + scanState.start;
	u32 lexemeSize = scanState.current - scanState.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

void AddToken(ScanState &scanState, TokenList &tokenList, TokenType tokenType, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenType;
	newToken.lexeme = ScannedString(scanState);
	newToken.literal = literal;
	newToken.line = scanState.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(ScanState &scanState, TokenList &tokenList, TokenType tokenType)
{
	Literal nullLiteral = {};
	AddToken(scanState, tokenList, tokenType, nullLiteral);
}

void ScanToken(ScanState &scanState, TokenList &tokenList)
{
	scanState.start = scanState.current;

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
			else if ( Consume(scanState, '*') )
			{
				while( Peek(scanState) != '*' && PeekNext(scanState) != '/' &&  !IsAtEnd(scanState) )
				{
					if ( IsEOL( Peek(scanState) ) ) scanState.line++;
					Advance(scanState);
				}
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

		default:
			if ( IsDigit(c) )
			{
				while ( IsDigit( Peek(scanState) ) ) Advance(scanState);

				if ( Peek(scanState) == '.' && IsDigit( PeekNext(scanState) ) )
				{
					Advance(scanState);
					//while ( IsDigit(Peek(scanState)) ) Advance(scanState);
				}

				Literal literal;
				// TODO: Convert literal to number
				AddToken(scanState, tokenList, TOKEN_NUMBER, literal);
			}
			else if ( IsAlpha(c) )
			{
				while ( IsAlphaNumeric( Peek(scanState) ) ) Advance(scanState);

				String word = ScannedString(scanState);

				// Keywords
				TokenType tokenType;
				// TODO: This keyword search could be much more efficient
				if ( StrEq( word, "if" ) ) tokenType = TOKEN_IF;
				else if ( StrEq( word, "else" ) ) tokenType = TOKEN_ELSE;
				else if ( StrEq( word, "for" ) ) tokenType = TOKEN_FOR;
				else if ( StrEq( word, "while" ) ) tokenType = TOKEN_WHILE;
				else if ( StrEq( word, "class" ) ) tokenType = TOKEN_CLASS;
				else if ( StrEq( word, "super" ) ) tokenType = TOKEN_SUPER;
				else if ( StrEq( word, "this" ) ) tokenType = TOKEN_THIS;
				else if ( StrEq( word, "fun" ) ) tokenType = TOKEN_FUN;
				else if ( StrEq( word, "return" ) ) tokenType = TOKEN_RETURN;
				else if ( StrEq( word, "true" ) ) tokenType = TOKEN_TRUE;
				else if ( StrEq( word, "false" ) ) tokenType = TOKEN_FALSE;
				else if ( StrEq( word, "nil" ) ) tokenType = TOKEN_NIL;
				else if ( StrEq( word, "var" ) ) tokenType = TOKEN_VAR;
				else if ( StrEq( word, "print" ) ) tokenType = TOKEN_PRINT;
				else if ( StrEq( word, "eof" ) ) tokenType = TOKEN_EOF;
				// Identifier
				else tokenType = TOKEN_IDENTIFIER;

				AddToken(scanState, tokenList, tokenType);
			}
			else
			{
				// TODO(jesus): Substitute this by a proper error call
				ERROR( "Unexpected character");
			}
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
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	return tokenList;
}

void PrintTokenList(const TokenList &tokenList)
{
	printf("List of tokens:\n");
	#define MAX_LEXEME_SIZE 128
	char lexeme[MAX_LEXEME_SIZE] = {};
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		const Token& token = tokenList.tokens[i];
		ASSERT(token.lexeme.size < MAX_LEXEME_SIZE);
		StrCopy(lexeme, token.lexeme);
		printf("%s\t: %s\n", TokenNames[token.type], lexeme);
	}
}

void Run(const char *script, u32 scriptSize)
{
	TokenList tokenList = ScanTokens(script, scriptSize);

	PrintTokenList(tokenList);
}

void RunFile(const char* filename)
{
	u32 fileSize = GetFileSize(filename);
	char* bytes = PushArray(globalArena, char, fileSize);
	u32 bytesRead = ReadEntireFile(filename, bytes, fileSize);
	ASSERT(bytesRead == fileSize);
	Run(bytes, bytesRead);
}

void RunPrompt()
{
	char line[MAX_LINE_SIZE];
	u32 lineSize = 0;
	for (;;)
	{
		ResetArena(globalArena);
		printf("> ");
		// TODO: Read line
		// TODO: Break if exit command
		Run(line, lineSize);
	}
}

int main(int argc, char **argv)
{
	u32 globalArenaSize = KB(512);
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

