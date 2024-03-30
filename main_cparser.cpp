#include "tools.h"

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
	TOKEN_STRUCT,
	TOKEN_ENUM,
	TOKEN_TRUE,
	TOKEN_FALSE,
	TOKEN_STATIC,
	TOKEN_CONST,
	TOKEN_CHAR,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_NULL, // ad-hoc value
	TOKEN_UINT, // ad-hoc type
	TOKEN_FLOAT3, // ad-hoc type
	TOKEN_ARR_COUNT, // ad-hoc macro
	TOKEN_INVALID, // part of C but not included in this subset
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
	"TOKEN_STRUCT",
	"TOKEN_ENUM",
	"TOKEN_TRUE",
	"TOKEN_FALSE",
	"TOKEN_STATIC",
	"TOKEN_CONST",
	"TOKEN_CHAR",
	"TOKEN_INT",
	"TOKEN_FLOAT",
	"TOKEN_NULL",
	"TOKEN_UINT",
	"TOKEN_FLOAT3",
	"TOKEN_ARR_COUNT",
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
	VALUE_TYPE_NULL,
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
	TokenId id;
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

struct CScanner
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

struct CParser
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

const char* TokenIdName(TokenId id)
{
	return TokenIdNames[id];
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

bool IsAtEnd(const CScanner &scanner)
{
	return scanner.current >= scanner.textSize;
}

char Advance(CScanner &scanner)
{
	ASSERT(scanner.current < scanner.textSize);
	char currentChar = scanner.text[scanner.current];
	scanner.current++;
	scanner.currentInLine++;
	return currentChar;
}

bool Consume(CScanner &scanner, char expected)
{
	if ( IsAtEnd(scanner) ) return false;
	if ( scanner.text[scanner.current] != expected ) return false;
	scanner.current++;
	scanner.currentInLine++;
	return true;
}

char Peek(const CScanner &scanner)
{
	return IsAtEnd(scanner) ? '\0' : scanner.text[ scanner.current ];
}

char PeekNext(const CScanner &scanner)
{
	if (scanner.current + 1 >= scanner.textSize) return '\0';
	return scanner.text[ scanner.current + 1 ];
}

String ScannedString(const CScanner &scanner)
{
	const char *lexemeStart = scanner.text + scanner.start;
	u32 lexemeSize = scanner.current - scanner.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

void AddToken(const CScanner &scanner, TokenList &tokenList, TokenId tokenId, Literal literal)
{
	Token newToken = {};
	newToken.id = tokenId;
	newToken.lexeme = ScannedString(scanner);
	newToken.literal = literal;
	newToken.line = scanner.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(const CScanner &scanner, TokenList &tokenList, TokenId tokenId)
{
	Literal literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = ScannedString(scanner);
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( ScannedString(scanner) );
	}
	else if ( tokenId == TOKEN_TRUE || tokenId == TOKEN_FALSE )
	{
		literal.type = VALUE_TYPE_BOOL;
		literal.b = tokenId == TOKEN_TRUE;
	}
	else if ( tokenId == TOKEN_NULL )
	{
		literal.type = VALUE_TYPE_NULL;
	}

	AddToken(scanner, tokenList, tokenId, literal);
}

void ReportError(CScanner &scanner, const char *message)
{
	printf("ERROR: %d:%d: %s\n", scanner.line, scanner.currentInLine, message);
	scanner.hasErrors = true;
}

void ScanToken(CScanner &scanner, TokenList &tokenList)
{
	scanner.start = scanner.current;

	char c = Advance(scanner);

	switch (c)
	{
		case '(': AddToken(scanner, tokenList, TOKEN_LEFT_PAREN); break;
		case ')': AddToken(scanner, tokenList, TOKEN_RIGHT_PAREN); break;
		case '{': AddToken(scanner, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': AddToken(scanner, tokenList, TOKEN_RIGHT_BRACE); break;
		case '[': AddToken(scanner, tokenList, TOKEN_LEFT_BRACKET); break;
		case ']': AddToken(scanner, tokenList, TOKEN_RIGHT_BRACKET); break;
		case ',': AddToken(scanner, tokenList, TOKEN_COMMA); break;
		case '.': AddToken(scanner, tokenList, TOKEN_DOT); break;
		case '-': AddToken(scanner, tokenList, TOKEN_MINUS); break;
		case '+': AddToken(scanner, tokenList, TOKEN_PLUS); break;
		case '*': AddToken(scanner, tokenList, TOKEN_STAR); break;
		case ';': AddToken(scanner, tokenList, TOKEN_SEMICOLON); break;
		case '!': AddToken(scanner, tokenList, Consume(scanner, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT); break;
		case '=': AddToken(scanner, tokenList, Consume(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
		case '<': AddToken(scanner, tokenList, Consume(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
		case '>': AddToken(scanner, tokenList, Consume(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
		case '/':
			if ( Consume(scanner, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !IsEOL( Peek(scanner) ) && !IsAtEnd(scanner) ) Advance(scanner);
			}
			else if ( Consume(scanner, '*') )
			{
				while( Peek(scanner) != '*' && PeekNext(scanner) != '/' &&  !IsAtEnd(scanner) )
				{
					if ( IsEOL( Peek(scanner) ) ) {
						scanner.line++;
						scanner.currentInLine = 0;
					}
					Advance(scanner);
				}
			}
			else
			{
				AddToken(scanner, tokenList, TOKEN_SLASH);
			}
			break;

		case '#':
			// Discard all chars until the end of line is reached
			while ( !IsEOL( Peek(scanner) ) && !IsAtEnd(scanner) ) Advance(scanner);
			break;

		// Skip whitespaces
		case ' ':
		case '\r':
		case '\t':
			break;

		// End of line counter
		case '\n':
			scanner.line++;
			scanner.currentInLine = 0;
			break;

		case '\"':
			while ( Peek(scanner) != '\"' && !IsAtEnd(scanner) )
			{
				if ( IsEOL( Peek(scanner) ) ) {
					scanner.line++;
					scanner.currentInLine = 0;
				}
				Advance(scanner);
			}

			if ( IsAtEnd(scanner) )
			{
				ReportError( scanner, "Unterminated string." );
				return;
			}

			Advance(scanner);

			AddToken(scanner, tokenList, TOKEN_STRING);
			break;

		default:
			if ( IsDigit(c) )
			{
				while ( IsDigit( Peek(scanner) ) ) Advance(scanner);

				if ( Peek(scanner) == '.' && IsDigit( PeekNext(scanner) ) )
				{
					Advance(scanner);
					while ( IsDigit(Peek(scanner)) ) Advance(scanner);
					Consume(scanner, 'f');
				}

				AddToken(scanner, tokenList, TOKEN_NUMBER);
			}
			else if ( IsAlpha(c) )
			{
				while ( IsAlphaNumeric( Peek(scanner) ) ) Advance(scanner);

				String word = ScannedString(scanner);

				// Keywords // TODO: This keyword search could be much more efficient
				if      ( StrEq( word, "struct" ) ) AddToken(scanner, tokenList, TOKEN_STRUCT);
				else if ( StrEq( word, "enum" ) )   AddToken(scanner, tokenList, TOKEN_ENUM);
				else if ( StrEq( word, "true" ) )   AddToken(scanner, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )  AddToken(scanner, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "static" ) ) AddToken(scanner, tokenList, TOKEN_STATIC);
				else if ( StrEq( word, "const" ) )  AddToken(scanner, tokenList, TOKEN_CONST);
				else if ( StrEq( word, "char" ) )   AddToken(scanner, tokenList, TOKEN_CHAR);
				else if ( StrEq( word, "int" ) )    AddToken(scanner, tokenList, TOKEN_INT);
				else if ( StrEq( word, "float" ) )  AddToken(scanner, tokenList, TOKEN_FLOAT);
				else if ( StrEq( word, "NULL" ) )   AddToken(scanner, tokenList, TOKEN_NULL);
				else if ( StrEq( word, "uint" ) )   AddToken(scanner, tokenList, TOKEN_UINT);
				else if ( StrEq( word, "float3" ) ) AddToken(scanner, tokenList, TOKEN_FLOAT3);
				else if ( StrEq( word, "ARRAY_COUNT" ) ) AddToken(scanner, tokenList, TOKEN_ARR_COUNT);
				else                                AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				ReportError( scanner, "Unexpected character." );
			}
	}
}

TokenList Scan(Arena &arena, const char *text, u32 textSize)
{
	TokenList tokenList = {};
	tokenList.tokens = PushArray(arena, Token, MAX_TOKEN_COUNT);

	CScanner scanner = {};
	scanner.line = 1;
	scanner.hasErrors = false;
	scanner.text = text;
	scanner.textSize = textSize;

	while ( !IsAtEnd(scanner) )
	{
		ScanToken(scanner, tokenList);
	}

	AddToken(scanner, tokenList, TOKEN_EOF);

	tokenList.valid = !scanner.hasErrors;

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
		const size_t tokenIdLen = strlen(TokenIdNames[token.id]);
		const size_t paddingSize = 32 - tokenIdLen;
		printf("%s:%.*s%.*s\n", TokenIdNames[token.id], paddingSize, paddingStr, token.lexeme.size, token.lexeme.str);
	}
}

bool CParser_HasFinished(const CParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = parser.currentToken >= parser.tokenList->count;
	return hasErrors || hasFinished;
}

bool CParser_IsCurrentToken( const CParser &parser, TokenId tokenId )
{
	if ( CParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		return tokenId == parser.tokenList->tokens[parser.currentToken].id;
	}
}

void CParser_Consume( CParser &parser )
{
	if ( !CParser_HasFinished( parser ) ) {
		parser.currentToken++;
	} else {
		parser.hasErrors = true;
	}
}

void CParser_Consume( CParser &parser, TokenId tokenId )
{
	if ( CParser_IsCurrentToken( parser, tokenId ) ) {
		CParser_Consume( parser );
	} else {
		parser.hasErrors = true;
		parser.hasFinished = true;
	}
}

bool CParser_TryConsume( CParser &parser, TokenId tokenId )
{
	if ( CParser_IsCurrentToken( parser, tokenId ) ) {
		CParser_Consume( parser );
		return true;
	}
	return false;
}

void CParser_ParseStruct(CParser &parser, CData &cdata, Arena &arena)
{
	CParser_Consume( parser ); // Identifier
	CParser_Consume( parser, TOKEN_LEFT_BRACE );
	while ( !CParser_TryConsume( parser, TOKEN_RIGHT_BRACE ) && !CParser_HasFinished( parser ) )
	{
		CParser_Consume( parser ); // Inside of the struct
	}
	CParser_Consume( parser, TOKEN_SEMICOLON );
}

void CParser_ParseDeclaration(CParser &parser, CData &cdata, Arena &arena)
{
	if ( CParser_TryConsume( parser, TOKEN_STRUCT ) )
	{
		CParser_ParseStruct( parser, cdata, arena );
	}
	else
	{
		CParser_Consume( parser );
	}
}

CData Parse(Arena &arena, const TokenList &tokenList)
{
	CData cdata = {};

	CParser parser = {};
	parser.tokenList = &tokenList;

	while ( !CParser_HasFinished(parser) )
	{
		CParser_ParseDeclaration(parser, cdata, arena);
	}

	if ( parser.hasErrors )
		printf("Parse finished with errors.\n");
	else
		printf("Parse finished successfully.\n");

	return cdata;
}

const char *GetTypeName(const CData &cdata, u32 typeIndex)
{
	ASSERT(typeIndex < cdata.typeCount);
	const CType &type = cdata.types[typeIndex];
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
		CData cdata = Parse(arena, tokenList);

		printf("Types:\n");
		for (u32 typeIndex = 0; typeIndex < cdata.typeCount; ++typeIndex)
		{
			const char *typeName = GetTypeName(cdata, typeIndex);
			printf("- %s\n", typeName);
		}
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
			//Clon clon = Clon_Parse(globalArena, bytes, fileSize);
			//Clon_Print(data);
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

