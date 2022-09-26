#include <stdio.h>
#include <string>
#include <string.h>
#include <assert.h>

#define KB(x) (1024 * x)
#define MB(x) (1024 * KB(x))
#define TB(x) (1024 * MB(x))

#define ASSERT(expression) assert(expression)

#define COMMAND_NAME "jsl"
#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)
#define MAX_LEXEME_SIZE 128



#define ENUM_ENTRY(entryName) entryName,
enum TokenType {
#include "token_list.h"
};
#undef ENUM_ENTRY

#define ENUM_ENTRY(entryName) #entryName,
const char *TokenNames[] = {
#include "token_list.h"
};
#undef ENUM_ENTRY

enum LiteralType
{
	LITERAL_BOOL,
	LITERAL_INT,
	LITERAL_FLOAT,
};

struct Literal
{
	LiteralType type;
	union
	{
		bool b;
		int i;
		float f;
	};
};

struct Token
{
    TokenType type;
	std::string lexeme;
	Literal literal;
	int line;
};

struct TokenList
{
	Token tokens[MAX_TOKEN_COUNT];
	int count;
};

struct ScanState
{
	int start;
	int current;
	int line;

	const char *script;
	char scriptSize;
};

unsigned int ReadEntireFile(const char *filename, char *bytes, unsigned int bytesSize)
{
	unsigned int bytesRead = 0;
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

const char* TokenName(TokenType type)
{
	return TokenNames[type];
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

void AddToken(ScanState &scanState, TokenList &tokenList, TokenType tokenType, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenType;

	char lexeme[MAX_LEXEME_SIZE];
	strncpy(lexeme, scanState.script + scanState.start, scanState.current - scanState.start);
	newToken.lexeme = lexeme;
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
	}
}

TokenList ScanTokens(const char *script, unsigned int scriptSize)
{
	TokenList tokenList = {};

	ScanState scanState = {};
	scanState.line = 1;
	scanState.script = script;
	scanState.scriptSize = scriptSize;

	while ( !IsAtEnd(scanState) )
	{
		scanState.start = scanState.current;
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	return tokenList;
}

void Run(const char *script, unsigned int scriptSize)
{
	TokenList tokenList = ScanTokens(script, scriptSize);

	for (unsigned int tokenIdx = 0; tokenIdx < tokenList.count; ++tokenIdx)
	{
		const Token &token = tokenList.tokens[tokenIdx];
		printf("%s ", TokenName(token.type));
	}
	printf("\n");
}

void RunFile(const char* filename)
{
	char *bytes = new char[MB(1)]();
	unsigned int bytesRead = ReadEntireFile(filename, bytes, MB(1));
	Run(bytes, bytesRead);
	delete[] bytes;
}

void RunPrompt()
{
	char line[MAX_LINE_SIZE];
	unsigned int lineSize = 0;
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
    return 0;
}

