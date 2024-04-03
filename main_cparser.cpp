#include "tools.h"
#include "tools_reflex.h"

//#define MAX_LINE_SIZE KB(1)
#define MAX_TOKEN_COUNT KB(10)
#define MAX_TYPE_COUNT 16
#define MAX_STRUCT_FIELD_COUNT 16

const ReflexStruct* ReflexGetStruct(ReflexID id)
{
	return NULL;
}

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
	TOKEN_UNSIGNED,
	TOKEN_SHORT,
	TOKEN_LONG,
	TOKEN_BOOL,
	TOKEN_CHAR,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_NULL, // ad-hoc value
	TOKEN_FLOAT3, // ad-hoc type
	TOKEN_ARR_COUNT, // ad-hoc macro
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
	"TOKEN_UNSIGNED",
	"TOKEN_SHORT",
	"TOKEN_LONG",
	"TOKEN_BOOL",
	"TOKEN_CHAR",
	"TOKEN_INT",
	"TOKEN_FLOAT",
	"TOKEN_NULL",
	"TOKEN_FLOAT3",
	"TOKEN_ARR_COUNT",
	"TOKEN_EOF",
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

struct CParser
{
	const TokenList *tokenList;
	u32 nextToken;
	bool hasErrors;
	bool hasFinished;
};

enum ClonTrivialEnum
{
	ClonTrivial_Bool,
	ClonTrivial_Int,
	ClonTrivial_Uint,
	ClonTrivial_Float,
	ClonTrivial_COUNT,
};

struct ClonTrivial
{
	ClonTrivialEnum type;
};

struct ClonMember
{
	String name;
	u8 isConst : 1;
	u8 isPointer : 1;
	u16 reflexId;
};

struct ClonStruct
{
	String name;
	ClonMember members[MAX_STRUCT_FIELD_COUNT];
	u32 memberCount;
};

struct ClonEnum
{
	String name;
};

enum ClonTypeEnum
{
	ClonType_Trivial,
	ClonType_Struct,
	ClonType_Enum,
	ClonType_COUNT,
};

struct ClonType
{
	ClonTypeEnum type;
	union
	{
		ClonTrivial *cTrivial;
		ClonStruct *cStruct;
		ClonEnum *cEnum;
	};
};

struct Clon
{
	ClonType types[MAX_TYPE_COUNT];
	u32 typeCount;
	bool valid;
};


////////////////////////////////////////////////////////////////////////
// CScanner

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
				if      ( StrEq( word, "struct" ) )   AddToken(scanner, tokenList, TOKEN_STRUCT);
				else if ( StrEq( word, "enum" ) )     AddToken(scanner, tokenList, TOKEN_ENUM);
				else if ( StrEq( word, "true" ) )     AddToken(scanner, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )    AddToken(scanner, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "static" ) )   AddToken(scanner, tokenList, TOKEN_STATIC);
				else if ( StrEq( word, "const" ) )    AddToken(scanner, tokenList, TOKEN_CONST);
				else if ( StrEq( word, "unsigned" ) ) AddToken(scanner, tokenList, TOKEN_UNSIGNED);
				else if ( StrEq( word, "short" ) )    AddToken(scanner, tokenList, TOKEN_SHORT);
				else if ( StrEq( word, "long" ) )     AddToken(scanner, tokenList, TOKEN_LONG);
				else if ( StrEq( word, "bool" ) )     AddToken(scanner, tokenList, TOKEN_BOOL);
				else if ( StrEq( word, "char" ) )     AddToken(scanner, tokenList, TOKEN_CHAR);
				else if ( StrEq( word, "int" ) )      AddToken(scanner, tokenList, TOKEN_INT);
				else if ( StrEq( word, "float" ) )    AddToken(scanner, tokenList, TOKEN_FLOAT);
				else if ( StrEq( word, "NULL" ) )     AddToken(scanner, tokenList, TOKEN_NULL);
				else if ( StrEq( word, "float3" ) )   AddToken(scanner, tokenList, TOKEN_FLOAT3);
				else if ( StrEq( word, "ARRAY_COUNT" ) ) AddToken(scanner, tokenList, TOKEN_ARR_COUNT);
				else                                  AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
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
		const int tokenIdLen = strlen(TokenIdNames[token.id]);
		const int paddingSize = 32 - tokenIdLen;
		printf("%s:%.*s%.*s\n", TokenIdNames[token.id], paddingSize, paddingStr, token.lexeme.size, token.lexeme.str);
	}
}

////////////////////////////////////////////////////////////////////////
// Clon structure

ClonType *Clon_CreateType(Clon &clon)
{
	ASSERT(clon.typeCount < ARRAY_COUNT(clon.types));
	ClonType *clonType = &clon.types[clon.typeCount++];
	return clonType;
}

const ClonType *Clon_GetType(const Clon &clon, u32 index)
{
	ASSERT(index < clon.typeCount);
	const ClonType *clonType = &clon.types[index];
	return clonType;
}

ClonTrivial *Clon_CreateTrivial(Clon &clon, Arena &arena)
{
	ClonTrivial *clonSubtype = PushZeroStruct(arena, ClonTrivial);
	ClonType *clonType = Clon_CreateType(clon);
	clonType->type = ClonType_Trivial;
	clonType->cTrivial = clonSubtype;
	return clonSubtype;
}

const ClonTrivial *Clon_GetTrivial(const Clon &clon, u32 index)
{
	const ClonType *clonType = Clon_GetType(clon, index);
	ASSERT(clonType->type == ClonType_Trivial);
	const ClonTrivial *clonSubtype = clonType->cTrivial;
	return clonSubtype;
}

ClonStruct *Clon_CreateStruct(Clon &clon, Arena &arena)
{
	ClonStruct *clonSubtype = PushZeroStruct(arena, ClonStruct);
	ClonType *clonType = Clon_CreateType(clon);
	clonType->type = ClonType_Struct;
	clonType->cStruct = clonSubtype;
	return clonSubtype;
}

const ClonStruct *Clon_GetStruct(const Clon &clon, u32 index)
{
	const ClonType *clonType = Clon_GetType(clon, index);
	ASSERT(clonType->type == ClonType_Struct);
	const ClonStruct *clonSubtype = clonType->cStruct;
	return clonSubtype;
}

ClonEnum *Clon_CreateEnum(Clon &clon, Arena &arena)
{
	ClonEnum *clonSubtype = PushZeroStruct(arena, ClonEnum);
	ClonType *clonType = Clon_CreateType(clon);
	clonType->type = ClonType_Enum;
	clonType->cEnum = clonSubtype;
	return clonSubtype;
}

const ClonEnum *Clon_GetEnum(const Clon &clon, u32 index)
{
	const ClonType *clonType = Clon_GetType(clon, index);
	ASSERT(clonType->type == ClonType_Enum);
	const ClonEnum *clonSubtype = clonType->cEnum;
	return clonSubtype;
}

ClonMember *Clon_AddMember(ClonStruct *clonStruct)
{
	ASSERT(clonStruct->memberCount < MAX_STRUCT_FIELD_COUNT);
	ClonMember *member = &clonStruct->members[clonStruct->memberCount++];
	return member;
}

////////////////////////////////////////////////////////////////////////
// CParser

u32 CParser_RemainingTokens(const CParser &parser)
{
	return parser.tokenList->count - parser.nextToken;
}

bool CParser_HasFinished(const CParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = parser.nextToken >= parser.tokenList->count;
	return hasErrors || hasFinished;
}

bool CParser_IsNextToken( const CParser &parser, TokenId tokenId )
{
	if ( CParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		return tokenId == parser.tokenList->tokens[parser.nextToken].id;
	}
}

bool CParser_AreNextTokens( const CParser &parser, TokenId tokenId0, TokenId tokenId1 )
{
	if ( CParser_HasFinished( parser ) || CParser_RemainingTokens(parser) < 2 ) {
		return false;
	} else {
		return
			tokenId0 == parser.tokenList->tokens[parser.nextToken].id &&
			tokenId1 == parser.tokenList->tokens[parser.nextToken+1].id;
	}
}

bool CParser_AreNextTokens( const CParser &parser, TokenId tokenId0, TokenId tokenId1, TokenId tokenId2 )
{
	if ( CParser_HasFinished( parser ) || CParser_RemainingTokens(parser) < 3 ) {
		return false;
	} else {
		return
			tokenId0 == parser.tokenList->tokens[parser.nextToken].id &&
			tokenId1 == parser.tokenList->tokens[parser.nextToken+1].id &&
			tokenId2 == parser.tokenList->tokens[parser.nextToken+2].id;
	}
}

const Token &CParser_GetNextToken( const CParser &parser )
{
	return parser.tokenList->tokens[parser.nextToken];
}

const Token &CParser_GetPreviousToken( const CParser &parser )
{
	ASSERT(parser.nextToken > 0);
	return parser.tokenList->tokens[parser.nextToken-1];
}

const Token &CParser_Consume( CParser &parser )
{
	if ( !CParser_HasFinished( parser ) ) {
		parser.nextToken++;
	} else {
		parser.hasErrors = true;
	}
	return CParser_GetPreviousToken(parser);
}

const Token &CParser_Consume( CParser &parser, TokenId tokenId )
{
	if ( CParser_IsNextToken( parser, tokenId ) ) {
		CParser_Consume( parser );
	} else {
		parser.hasErrors = true;
		parser.hasFinished = true;
	}
	return CParser_GetPreviousToken(parser);
}

bool CParser_TryConsume( CParser &parser, TokenId tokenId )
{
	if ( CParser_IsNextToken( parser, tokenId ) ) {
		CParser_Consume( parser );
		return true;
	}
	return false;
}

bool CParser_TryConsume( CParser &parser, TokenId tokenId0, TokenId tokenId1 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

bool CParser_TryConsume( CParser &parser, TokenId tokenId0, TokenId tokenId1, TokenId tokenId2 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1, tokenId2 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

void CParser_ParseMember(CParser &parser, Clon &clon, ClonStruct *clonStruct, Arena &arena)
{
	// Parse type
	const bool isConst = CParser_TryConsume(parser, TOKEN_CONST);
	const bool isUnsigned = CParser_TryConsume(parser, TOKEN_UNSIGNED);
	const bool isShort = CParser_TryConsume(parser, TOKEN_SHORT);
	const bool isLong = CParser_TryConsume(parser, TOKEN_LONG);
	const bool isLongLong = CParser_TryConsume(parser, TOKEN_LONG);

	const Token &type = CParser_Consume(parser);
	const bool isBool = type.id == TOKEN_BOOL;
	const bool isChar = type.id == TOKEN_CHAR;
	const bool isInt = type.id == TOKEN_INT;
	ASSERT( isInt || !isUnsigned );
	const bool isFloat = type.id == TOKEN_FLOAT;
	const bool isFloat3 = type.id == TOKEN_FLOAT3;

	// TODO: Check it is trivial or identifier
	ASSERT( type.id == TOKEN_IDENTIFIER || isBool || isChar || isInt || isFloat || isFloat3 );

	const bool isPointer = CParser_TryConsume(parser, TOKEN_STAR);

	const Token &identifier = CParser_Consume(parser, TOKEN_IDENTIFIER);
	CParser_Consume(parser, TOKEN_SEMICOLON);

	ClonMember *member = Clon_AddMember(clonStruct);
	member->name = identifier.lexeme;
	member->isConst = isConst;
	member->isPointer = isPointer;

	if ( isBool ) member->reflexId = ReflexID_Bool;
	else if ( isChar ) member->reflexId = ReflexID_Char;
	else if ( isInt && !isUnsigned ) member->reflexId = ReflexID_Int;
	else if ( isInt && isUnsigned ) member->reflexId = ReflexID_UInt;
	else if ( isFloat ) member->reflexId = ReflexID_Float;
	else if ( isFloat3 ) member->reflexId = ReflexID_Float3;
	else
	{
		ASSERT( type.id == TOKEN_IDENTIFIER );
		member->reflexId = ReflexID_Struct + 0; // TODO
	}
}

void CParser_ParseStruct(CParser &parser, Clon &clon, Arena &arena)
{
	ClonStruct *clonStruct = Clon_CreateStruct(clon, arena);
	clonStruct->name = CParser_GetNextToken( parser ).lexeme;

	CParser_Consume( parser, TOKEN_IDENTIFIER );
	CParser_Consume( parser, TOKEN_LEFT_BRACE );
	while ( !CParser_TryConsume( parser, TOKEN_RIGHT_BRACE ) && !CParser_HasFinished( parser ) )
	{
		CParser_ParseMember(parser, clon, clonStruct, arena);
	}
	CParser_Consume( parser, TOKEN_SEMICOLON );
}

void CParser_ParseEnum(CParser &parser, Clon &clon, Arena &arena)
{
	ClonEnum *clonEnum = Clon_CreateEnum(clon, arena);
	clonEnum->name = CParser_GetNextToken(parser).lexeme;

	CParser_Consume( parser, TOKEN_IDENTIFIER );
	CParser_Consume( parser, TOKEN_LEFT_BRACE );
	while ( !CParser_TryConsume( parser, TOKEN_RIGHT_BRACE ) && !CParser_HasFinished( parser ) )
	{
		CParser_Consume(parser);
	}
	CParser_Consume( parser, TOKEN_SEMICOLON );
}

void CParser_ParseDeclaration(CParser &parser, Clon &clon, Arena &arena)
{
	if ( CParser_TryConsume( parser, TOKEN_STRUCT ) )
	{
		CParser_ParseStruct( parser, clon, arena );
	}
	else if ( CParser_TryConsume( parser, TOKEN_ENUM ) )
	{
		CParser_ParseEnum( parser, clon, arena );
	}
	else
	{
		CParser_Consume( parser );
	}
}

Clon Parse(Arena &arena, const TokenList &tokenList)
{
	Clon clon = {};

	CParser parser = {};
	parser.tokenList = &tokenList;

	while ( !CParser_HasFinished(parser) )
	{
		CParser_ParseDeclaration(parser, clon, arena);
	}

	clon.valid = !parser.hasErrors;

	if ( parser.hasErrors )
	{
		printf("Parse finished with errors.\n");
	}

	return clon;
}

Clon Clon_Read(Arena &arena, const char *text, u64 textSize)
{
	Clon clon = {};

	TokenList tokenList = Scan(arena, text, textSize);

	if ( tokenList.valid )
	{
#if 0
		PrintTokenList(tokenList);
#endif

		clon = Parse(arena, tokenList);
	}

	return clon;
}

void Clon_Print(const Clon &clon)
{
	if ( !clon.valid )
	{
		return;
	}

#if 0
	printf("Trivial types:\n");
	for (u32 index = 0; index < clon.typeCount; ++index)
	{
		const ClonType *clonType = Clon_GetType(clon, index);
		if ( clonType->type == ClonType_Trivial )
		{
			printf("- %s\n", "TODO");
		}
	}
#endif

	// ReflexMember
	bool firstValueInEnum = true;
	printf("\n");
	printf("// IDs\n");
	printf("enum\n");
	printf("{\n");
	for (u32 index = 0; index < clon.typeCount; ++index)
	{
		const ClonType *clonType = Clon_GetType(clon, index);
		if ( clonType->type == ClonType_Struct )
		{
			ClonStruct *clonStruct = clonType->cStruct;
			char structName[128];
			StrCopy(structName, clonStruct->name);
			printf("  ReflexID_%s", structName);
			printf("%s", firstValueInEnum ? " = ReflexID_Struct" : "");
			printf(",\n");
			firstValueInEnum = false;
		}
	}
	printf("}\n");

	// ReflexGetStruct function
	printf("\n");
	printf("const ReflexStruct* ReflexGetStruct(ReflexID id)\n");
	printf("{\n");
	printf("  ASSERT(ReflexIsStruct(id);)\n");

	// ReflexMember
	printf("\n");
	printf("  // ReflexMembers\n");
	for (u32 index = 0; index < clon.typeCount; ++index)
	{
		const ClonType *clonType = Clon_GetType(clon, index);
		if ( clonType->type == ClonType_Struct )
		{
			ClonStruct *clonStruct = clonType->cStruct;
			char structName[128];
			char memberName[128];
			StrCopy(structName, clonStruct->name);
			printf("  static const ReflexMember reflex%sMembers[] = {\n", structName);
			for ( u32 fieldIndex = 0; fieldIndex < clonStruct->memberCount; ++fieldIndex)
			{
				ClonMember *member = &clonStruct->members[fieldIndex];
				StrCopy(memberName, member->name);
				printf("    { ");
				printf(".name = \"%s\", ", memberName);
				printf(".isConst = %s, ", member->isConst ? "true" : "false");
				printf(".isPointer = %s, ", member->isPointer ? "true" : "false");
				printf(".reflexId = %s, ", "ReflexID_XXX"); // TODO: type?
				printf(".offset = offsetof(%s, %s) ", structName, memberName);
				printf(" },\n");
			}
			printf("  };\n");
		}
	}

	// ReflexStruct
	printf("\n");
	printf("  // ReflexStructs\n");
	printf("  static const ReflexStruct reflexStructs[] =\n");
	printf("  {\n");
	for (u32 index = 0; index < clon.typeCount; ++index)
	{
		const ClonType *clonType = Clon_GetType(clon, index);
		if ( clonType->type == ClonType_Struct )
		{
			ClonStruct *clonStruct = clonType->cStruct;
			char structName[128];
			StrCopy(structName, clonStruct->name);
			printf("    {\n");
			printf("      .name = \"%s\"\n", structName);
			printf("      .members = reflex%sMembers,\n", structName);
			printf("      .memberCount = ARRAY_COUNT(reflex%sMembers),\n", structName);
			printf("      .size = sizeof(%s),\n", structName);
			printf("    },\n");
		}
	}
	printf("  };\n");

#if 0
	printf("Enums:\n");
	for (u32 index = 0; index < clon.typeCount; ++index)
	{
		const ClonType *clonType = Clon_GetType(clon, index);
		if ( clonType->type == ClonType_Enum )
		{
			char nameStr[128];
			StrCopy(nameStr, clonType->cEnum->name);
			printf("- %s\n", nameStr);
		}
	}
#endif

	printf("  \n");
	printf("  return &reflexStructs[id - ReflexID_Struct];\n");
	printf("}\n");
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

			const Clon clon = Clon_Read(globalArena, bytes, fileSize);

			Clon_Print(clon);
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

