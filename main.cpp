#include <stdio.h>

#define KB(x) (1024 * x)
#define MB(x) (1024 * KB(x))
#define TB(x) (1024 * MB(x))

#define COMMAND_NAME "jsl"
#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)



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

struct Token
{
    TokenType type;
};

unsigned int readEntireFile(const char *filename, char *bytes, unsigned int bytesSize)
{
	unsigned int bytesRead = 0;
	FILE *f = fopen(filename, "rb");
	if ( f )
	{
		bytesRead = fread(bytes, sizeof(unsigned char), bytesSize, f);
		// TODO: Assert bytesRead <= byesSize
		bytes[bytesRead] = 0;
		fclose(f);
	}
	return bytesRead;
}

const char* TokenName(TokenType type)
{
	return TokenNames[type];
}

unsigned int ScanTokens(Token tokens[MAX_TOKEN_COUNT], const char *script, unsigned int scriptSize)
{
	// TODO
	return 0;
}

void run(const char *script, unsigned int scriptSize)
{
	Token tokens[MAX_TOKEN_COUNT] = {};
	unsigned int tokenCount = ScanTokens(tokens, script, scriptSize);

	for (unsigned int tokenIdx = 0; tokenIdx < tokenCount; ++tokenIdx)
	{
		const Token &token = tokens[tokenIdx];
		printf("%s ", TokenName(token.type));
	}
	printf("\n");
}

void runFile(const char* filename)
{
	char *bytes = new char[MB(1)]();
	unsigned int bytesRead = readEntireFile(filename, bytes, MB(1));
	run(bytes, bytesRead);
	delete[] bytes;
}

void runPrompt()
{
	char line[MAX_LINE_SIZE];
	unsigned int lineSize = 0;
	for (;;)
	{
		printf("> ");
		// TODO: Read line
		// TODO: Break if exit command
		run(line, lineSize);
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
		runFile(argv[1]);
	}
	else
	{
		runPrompt();
	}
    return 0;
}

