#include "tools.h"

#define TOOLS_CPARSER_IMPLEMENTATION
#include "tools_cparser.h"

#include "assets/assets.h"

//#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)

enum TokenId
{
	// Single character tokens
	TOKEN_LEFT_PAREN,
	TOKEN_RIGHT_PAREN,
	TOKEN_LEFT_BRACE,
	TOKEN_RIGHT_BRACE,
	TOKEN_LEFT_BRACKET,
	TOKEN_RIGHT_BRACKET,
	TOKEN_COMMA,
	TOKEN_DOT,
	TOKEN_MINUS,
	TOKEN_PLUS,
	TOKEN_SEMICOLON,
	TOKEN_SLASH,
	TOKEN_STAR,
	// One or two-character tokens
	TOKEN_NOT,
	TOKEN_NOT_EQUAL,
	TOKEN_EQUAL,
	TOKEN_EQUAL_EQUAL,
	TOKEN_GREATER,
	TOKEN_GREATER_EQUAL,
	TOKEN_LESS,
	TOKEN_LESS_EQUAL,
	TOKEN_AND,
	TOKEN_ANDAND,
	TOKEN_OR,
	TOKEN_OROR,
	// Literals
	TOKEN_IDENTIFIER,
	TOKEN_STRING,
	TOKEN_NUMBER,
	// Keywords
	TOKEN_IF,
	TOKEN_ELSE,
	TOKEN_FOR,
	TOKEN_WHILE,
	TOKEN_STRUCT,
	TOKEN_SUPER,
	TOKEN_THIS,
	TOKEN_FUN,
	TOKEN_RETURN,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_NIL,
	TOKEN_VAR,
	TOKEN_PRINT,
	TOKEN_EOF,
	TOKEN_COUNT,
};

const char *TokenIdNames[] =
{
	// Single character tokens
	"TOKEN_LEFT_PAREN",
	"TOKEN_RIGHT_PAREN",
	"TOKEN_LEFT_BRACE",
	"TOKEN_RIGHT_BRACE",
	"TOKEN_LEFT_BRACKET",
	"TOKEN_RIGHT_BRACKET",
	"TOKEN_COMMA",
	"TOKEN_DOT",
	"TOKEN_MINUS",
	"TOKEN_PLUS",
	"TOKEN_SEMICOLON",
	"TOKEN_SLASH",
	"TOKEN_STAR",
	// One or two-character tokens
	"TOKEN_NOT",
	"TOKEN_NOT_EQUAL",
	"TOKEN_EQUAL",
	"TOKEN_EQUAL_EQUAL",
	"TOKEN_GREATER",
	"TOKEN_GREATER_EQUAL",
	"TOKEN_LESS",
	"TOKEN_LESS_EQUAL",
	"TOKEN_AND",
	"TOKEN_ANDAND",
	"TOKEN_OR",
	"TOKEN_OROR",
	// Literals
	"TOKEN_IDENTIFIER",
	"TOKEN_STRING",
	"TOKEN_NUMBER",
	// Keywords
	"TOKEN_IF",
	"TOKEN_ELSE",
	"TOKEN_FOR",
	"TOKEN_WHILE",
	"TOKEN_STRUCT",
	"TOKEN_SUPER",
	"TOKEN_THIS",
	"TOKEN_FUN",
	"TOKEN_RETURN",
	"TOKEN_TRUE",
	"TOKEN_FALSE",
	"TOKEN_NIL",
	"TOKEN_VAR",
	"TOKEN_PRINT",
	"TOKEN_EOF",
	"TOKEN_COUNT",
};

CT_ASSERT(ARRAY_COUNT(TokenIdNames) == TOKEN_COUNT);

enum ValueType
{
	VALUE_TYPE_BOOL,
	//VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_NIL,
};

struct Value
{
	ValueType type;
	union
	{
		bool b;
		i32 i;
		f32 f;
		String s;
	};
};

typedef Value Literal;

struct Token
{
	TokenId type;
	String lexeme;
	Literal literal;
	i32 line;
};

struct TokenList
{
	Token *tokens;
	i32 count;
	bool valid;
};

struct ScanState
{
	i32 start;
	i32 current;
	i32 currentInLine;
	i32 line;
	bool hasErrors;

	const char *text;
	u32 textSize;
};

struct CType
{
	const char *name;
};

struct CParseState
{
	const TokenList *tokenList;
	u32 currentToken;
	bool hasErrors;
	bool hasFinished;
};

struct CData
{
	u32 typeCount;
	CType types[128];
};

const char* TokenIdName(TokenId type)
{
	return TokenIdNames[type];
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
	return scanState.current >= scanState.textSize;
}

char Advance(ScanState &scanState)
{
	ASSERT(scanState.current < scanState.textSize);
	char currentChar = scanState.text[scanState.current];
	scanState.current++;
	scanState.currentInLine++;
	return currentChar;
}

bool Consume(ScanState &scanState, char expected)
{
	if ( IsAtEnd(scanState) ) return false;
	if ( scanState.text[scanState.current] != expected ) return false;
	scanState.current++;
	scanState.currentInLine++;
	return true;
}

char Peek(const ScanState &scanState)
{
	return IsAtEnd(scanState) ? '\0' : scanState.text[ scanState.current ];
}

char PeekNext(const ScanState &scanState)
{
	if (scanState.current + 1 >= scanState.textSize) return '\0';
	return scanState.text[ scanState.current + 1 ];
}

String ScannedString(const ScanState &scanState)
{
	const char *lexemeStart = scanState.text + scanState.start;
	u32 lexemeSize = scanState.current - scanState.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenId tokenId, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenId;
	newToken.lexeme = ScannedString(scanState);
	newToken.literal = literal;
	newToken.line = scanState.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenId tokenId)
{
	Literal literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = ScannedString(scanState);
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( ScannedString(scanState) );
	}
	else if ( tokenId == TOKEN_TRUE || tokenId == TOKEN_FALSE )
	{
		literal.type = VALUE_TYPE_BOOL;
		literal.b = tokenId == TOKEN_TRUE;
	}
	else if ( tokenId == TOKEN_NIL )
	{
		literal.type = VALUE_TYPE_NIL;
	}

	AddToken(scanState, tokenList, tokenId, literal);
}

void ReportError(ScanState &scanState, const char *message)
{
	printf("ERROR: %d:%d: %s\n", scanState.line, scanState.currentInLine, message);
	scanState.hasErrors = true;
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
		case '[': AddToken(scanState, tokenList, TOKEN_LEFT_BRACKET); break;
		case ']': AddToken(scanState, tokenList, TOKEN_RIGHT_BRACKET); break;
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
					if ( IsEOL( Peek(scanState) ) ) {
						scanState.line++;
						scanState.currentInLine = 0;
					}
					Advance(scanState);
				}
			}
			else
			{
				AddToken(scanState, tokenList, TOKEN_SLASH);
			}
			break;

		case '#':
			// Discard all chars until the end of line is reached
			while ( !IsEOL( Peek(scanState) ) && !IsAtEnd(scanState) ) Advance(scanState);
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanState.line++;
			scanState.currentInLine = 0;
			break;

		case '\"':
			while ( Peek(scanState) != '\"' && !IsAtEnd(scanState) )
			{
				if ( IsEOL( Peek(scanState) ) ) {
					scanState.line++;
					scanState.currentInLine = 0;
				}
				Advance(scanState);
			}

			if ( IsAtEnd(scanState) )
			{
				ReportError( scanState, "Unterminated string." );
				return;
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
					while ( IsDigit(Peek(scanState)) ) Advance(scanState);
				}

				AddToken(scanState, tokenList, TOKEN_NUMBER);
			}
			else if ( IsAlpha(c) )
			{
				while ( IsAlphaNumeric( Peek(scanState) ) ) Advance(scanState);

				String word = ScannedString(scanState);

				// Keywords // TODO: This keyword search could be much more efficient
				if      ( StrEq( word, "if" ) )     AddToken(scanState, tokenList, TOKEN_IF);
				else if ( StrEq( word, "else" ) )   AddToken(scanState, tokenList, TOKEN_ELSE);
				else if ( StrEq( word, "for" ) )    AddToken(scanState, tokenList, TOKEN_FOR);
				else if ( StrEq( word, "while" ) )  AddToken(scanState, tokenList, TOKEN_WHILE);
				else if ( StrEq( word, "struct" ) ) AddToken(scanState, tokenList, TOKEN_STRUCT);
				else if ( StrEq( word, "super" ) )  AddToken(scanState, tokenList, TOKEN_SUPER);
				else if ( StrEq( word, "this" ) )   AddToken(scanState, tokenList, TOKEN_THIS);
				else if ( StrEq( word, "fun" ) )    AddToken(scanState, tokenList, TOKEN_FUN);
				else if ( StrEq( word, "return" ) ) AddToken(scanState, tokenList, TOKEN_RETURN);
				else if ( StrEq( word, "true" ) )   AddToken(scanState, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )  AddToken(scanState, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "nil" ) )    AddToken(scanState, tokenList, TOKEN_NIL);
				else if ( StrEq( word, "var" ) )    AddToken(scanState, tokenList, TOKEN_VAR);
				else if ( StrEq( word, "print" ) )  AddToken(scanState, tokenList, TOKEN_PRINT);
				else if ( StrEq( word, "eof" ) )    AddToken(scanState, tokenList, TOKEN_EOF);
				else                                AddToken(scanState, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				ReportError( scanState, "Unexpected character." );
			}
	}
}

TokenList Scan(Arena &arena, const char *text, u32 textSize)
{
	TokenList tokenList = {};
	tokenList.tokens = PushArray(arena, Token, MAX_TOKEN_COUNT);

	ScanState scanState = {};
	scanState.line = 1;
	scanState.hasErrors = false;
	scanState.text = text;
	scanState.textSize = textSize;

	while ( !IsAtEnd(scanState) )
	{
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	tokenList.valid = !scanState.hasErrors;

	return tokenList;
}

void PrintTokenList(const TokenList &tokenList)
{
	static const char *paddingStr =
		"                                                  "
		"                                                  ";
	printf("List of tokens:\n");
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		const Token& token = tokenList.tokens[i];
		const size_t tokenIdLen = strlen(TokenIdNames[token.type]);
		const size_t paddingSize = 32 - tokenIdLen;
		printf("%s:%.*s%.*s\n", TokenIdNames[token.type], paddingSize, paddingStr, token.lexeme.size, token.lexeme.str);
	}
}

bool CParseState_HasFinished(const CParseState &parseState)
{
	const bool hasErrors = parseState.hasErrors;
	const bool hasFinished = parseState.currentToken >= parseState.tokenList->count;
	return hasErrors || hasFinished;
}

void CParseState_ParseDeclaration(Arena &arena, CParseState &parseState)
{
	// TODO
}

CData Parse(Arena &arena, const TokenList &tokenList)
{
	CData data = {};

	CParseState parseState = {};
	parseState.tokenList = &tokenList;

	while ( !CParseState_HasFinished(parseState) )
	{
		CParseState_ParseDeclaration(arena, parseState);
	}

	return data;
}

const char *GetTypeName(const CData &data, u32 typeIndex)
{
	ASSERT(typeIndex < data.typeCount);
	const CType &type = data.types[typeIndex];
	return type.name;
}

void ParseFile(Arena arena, const char *text, u64 textSize)
{
	LOG(Info, "ParseFile()\n");

	TokenList tokenList = Scan(arena, text, textSize);

	if ( tokenList.valid )
	{
		PrintTokenList(tokenList);

		// TODO
#if 0
		CData data = Parse(arena, tokenList);

		printf("Types:\n");
		for (u32 typeIndex = 0; typeIndex < data.typeCount; ++typeIndex)
		{
			const char *typeName = GetTypeName(data, typeIndex);
			printf("- %s\n", typeName);
		}
#endif
	}
}

int main(int argc, char **argv)
{
	if (argc != 2 )
	{
		LOG(Info, "Usage: %s <c file>\n", argv[0]);
		return -1;
	}

	const char *filename = argv[1];

	u64 fileSize;
	if ( GetFileSize(filename, fileSize) && fileSize > 0 )
	{
		u32 globalArenaSize = MB(4);
		byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);
		Arena globalArena = MakeArena(globalArenaBase, globalArenaSize);

		char* bytes = PushArray(globalArena, char, fileSize + 1);
		if ( ReadEntireFile(filename, bytes, fileSize) )
		{
			bytes[fileSize] = 0;
			ParseFile(globalArena, bytes, fileSize);
		}
		else
		{
			LOG(Error, "ReadEntireFile() failed reading %s\n", filename);
			return -1;
		}
	}
	else
	{
		LOG(Error, "GetFileSize() failed reading %s\n", filename);
		return -1;
	}
	
	return 0;
}

