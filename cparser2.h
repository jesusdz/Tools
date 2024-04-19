/*
 * cparser.h
 * Author: Jesus Diaz Garcia
 */

#ifndef CPARSER_H
#define CPARSER_H

#define MAX_TOKEN_COUNT KB(10)
#define MAX_TYPE_COUNT 16
#define MAX_STRUCT_FIELD_COUNT 16

enum CTokenId
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
	TOKEN_TYPEDEF,
	TOKEN_EXTERN,
	TOKEN_STATIC,
	TOKEN_REGISTER,
	TOKEN_AUTO,
	TOKEN_THREAD_LOCAL,
	TOKEN_CONST,
	TOKEN_RESTRICT,
	TOKEN_VOLATILE,
	TOKEN_SIGNED,
	TOKEN_UNSIGNED,
	TOKEN_SHORT,
	TOKEN_LONG,
	TOKEN_VOID,
	TOKEN_BOOL,
	TOKEN_CHAR,
	TOKEN_INT,
	TOKEN_FLOAT,
	TOKEN_DOUBLE,
	TOKEN_NULL, // ad-hoc value
	TOKEN_ARR_COUNT, // ad-hoc macro
	TOKEN_EOF,
	TOKEN_COUNT,
};

static const char *CTokenIdNames[] =
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
	"TOKEN_TYPEDEF",
	"TOKEN_EXTERN",
	"TOKEN_STATIC",
	"TOKEN_REGISTER",
	"TOKEN_AUTO",
	"TOKEN_THREAD_LOCAL",
	"TOKEN_CONST",
	"TOKEN_RESTRICT",
	"TOKEN_VOLATILE",
	"TOKEN_SIGNED",
	"TOKEN_UNSIGNED",
	"TOKEN_SHORT",
	"TOKEN_LONG",
	"TOKEN_VOID",
	"TOKEN_BOOL",
	"TOKEN_CHAR",
	"TOKEN_INT",
	"TOKEN_FLOAT",
	"TOKEN_DOUBLE",
	"TOKEN_NULL",
	"TOKEN_ARR_COUNT",
	"TOKEN_EOF",
};

CT_ASSERT(ARRAY_COUNT(CTokenIdNames) == TOKEN_COUNT);

enum ValueType
{
	VALUE_TYPE_BOOL,
	//VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
	VALUE_TYPE_NULL,
};

struct CValue
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

typedef CValue CLiteral;

struct CToken
{
	CTokenId id;
	String lexeme;
	CLiteral literal;
	i32 line;
};

static CToken gNullToken = {
	.id = TOKEN_COUNT,
	.lexeme = { "", 0 },
	.literal = { .type = VALUE_TYPE_NULL, .i = 0 },
	.line = 0,
};

struct CTokenList
{
	CToken *tokens;
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

struct CIdentifierList
{
	String identifier;
	CIdentifierList *next;
};

struct CParser
{
	const CTokenList *tokenList;
	Arena *arena;
	u32 nextToken;
	u32 lastToken;
	bool hasErrors;
	bool hasFinished;

	CIdentifierList *identifiers;
};

#if 0
struct CMember
{
	String name;
	u16 isConst : 1;
	u16 pointerCount : 2;
	u16 isArray: 1;
	u16 arrayDim: 12; // 4096 values
	u16 reflexId;
};

struct CStruct
{
	String name;
	CMember members[MAX_STRUCT_FIELD_COUNT];
	u16 memberCount;
	u16 index;
};

struct CEnum
{
	String name;
	u16 index;
};

struct CAssembly
{
	bool valid;

	CStruct structs[MAX_TYPE_COUNT];
	u16 structCount;

	CEnum enums[MAX_TYPE_COUNT];
	u16 enumCount;
};


// Public functions
bool CAssembly_Create(CAssembly &cAsm, Arena &arena, const char *text, u64 textSize);
const CStruct *CAssembly_GetStruct(const CAssembly &cAsm, u32 index);
const CEnum *CAssembly_GetEnum(const CAssembly &cAsm, u32 index);
const char *CAssembly_GetTypeName(const CAssembly &cAsm, ReflexID id);
#endif


#ifdef CPARSER_IMPLEMENTATION

////////////////////////////////////////////////////////////////////////
// CScanner

static bool Char_IsEOL(char character)
{
	return character == '\n';
}

static bool Char_IsAlpha(char character)
{
	return
		( character >= 'a' && character <= 'z' ) ||
		( character >= 'A' && character <= 'Z' ) ||
		( character == '_' );
}

static bool Char_IsDigit(char character)
{
	return character >= '0' && character <= '9';
}

static bool Char_IsAlphaNumeric(char character)
{
	return Char_IsAlpha(character) || Char_IsDigit(character);
}

static bool CScanner_IsAtEnd(const CScanner &scanner)
{
	return scanner.current >= scanner.textSize;
}

static char CScanner_Advance(CScanner &scanner)
{
	ASSERT(scanner.current < scanner.textSize);
	char currentChar = scanner.text[scanner.current];
	scanner.current++;
	scanner.currentInLine++;
	return currentChar;
}

static bool CScanner_Consume(CScanner &scanner, char expected)
{
	if ( CScanner_IsAtEnd(scanner) ) return false;
	if ( scanner.text[scanner.current] != expected ) return false;
	scanner.current++;
	scanner.currentInLine++;
	return true;
}

static char CScanner_Peek(const CScanner &scanner)
{
	return CScanner_IsAtEnd(scanner) ? '\0' : scanner.text[ scanner.current ];
}

static char CScanner_PeekNext(const CScanner &scanner)
{
	if (scanner.current + 1 >= scanner.textSize) return '\0';
	return scanner.text[ scanner.current + 1 ];
}

static String CScanner_ScannedString(const CScanner &scanner)
{
	const char *lexemeStart = scanner.text + scanner.start;
	u32 lexemeSize = scanner.current - scanner.start;
	String scannedString = { lexemeStart, lexemeSize };
	return scannedString;
}

static void CScanner_AddToken(const CScanner &scanner, CTokenList &tokenList, CTokenId tokenId, CLiteral literal)
{
	CToken newToken = {};
	newToken.id = tokenId;
	newToken.lexeme = CScanner_ScannedString(scanner);
	newToken.literal = literal;
	newToken.line = scanner.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

static void CScanner_AddToken(const CScanner &scanner, CTokenList &tokenList, CTokenId tokenId)
{
	CLiteral literal = {};

	if ( tokenId == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = CScanner_ScannedString(scanner);
	}
	else if ( tokenId == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( CScanner_ScannedString(scanner) );
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

	CScanner_AddToken(scanner, tokenList, tokenId, literal);
}

static void CScanner_SetError(CScanner &scanner, const char *message)
{
	printf("ERROR: %d:%d: %s\n", scanner.line, scanner.currentInLine, message);
	scanner.hasErrors = true;
}

static void CScanner_ScanToken(CScanner &scanner, CTokenList &tokenList)
{
	scanner.start = scanner.current;

	char c = CScanner_Advance(scanner);

	switch (c)
	{
		case '(': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_PAREN); break;
		case ')': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_PAREN); break;
		case '{': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACE); break;
		case '}': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACE); break;
		case '[': CScanner_AddToken(scanner, tokenList, TOKEN_LEFT_BRACKET); break;
		case ']': CScanner_AddToken(scanner, tokenList, TOKEN_RIGHT_BRACKET); break;
		case ',': CScanner_AddToken(scanner, tokenList, TOKEN_COMMA); break;
		case '.': CScanner_AddToken(scanner, tokenList, TOKEN_DOT); break;
		case '-': CScanner_AddToken(scanner, tokenList, TOKEN_MINUS); break;
		case '+': CScanner_AddToken(scanner, tokenList, TOKEN_PLUS); break;
		case '*': CScanner_AddToken(scanner, tokenList, TOKEN_STAR); break;
		case ';': CScanner_AddToken(scanner, tokenList, TOKEN_SEMICOLON); break;
		case '!': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_NOT_EQUAL : TOKEN_NOT); break;
		case '=': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_EQUAL_EQUAL : TOKEN_EQUAL); break;
		case '<': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_LESS_EQUAL : TOKEN_LESS); break;
		case '>': CScanner_AddToken(scanner, tokenList, CScanner_Consume(scanner, '=') ? TOKEN_GREATER_EQUAL : TOKEN_GREATER); break;
		case '/':
			if ( CScanner_Consume(scanner, '/') )
			{
				// Discard all chars until the end of line is reached
				while ( !Char_IsEOL( CScanner_Peek(scanner) ) && !CScanner_IsAtEnd(scanner) )
				{
					CScanner_Advance(scanner);
				}
			}
			else if ( CScanner_Consume(scanner, '*') )
			{
				while( CScanner_Peek(scanner) != '*' && CScanner_PeekNext(scanner) != '/' &&  !CScanner_IsAtEnd(scanner) )
				{
					if ( Char_IsEOL( CScanner_Peek(scanner) ) ) {
						scanner.line++;
						scanner.currentInLine = 0;
					}
					CScanner_Advance(scanner);
				}
			}
			else
			{
				CScanner_AddToken(scanner, tokenList, TOKEN_SLASH);
			}
			break;

		case '#':
			// Discard all chars until the end of line is reached
			while ( !Char_IsEOL( CScanner_Peek(scanner) ) && !CScanner_IsAtEnd(scanner) ) CScanner_Advance(scanner);
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
			while ( CScanner_Peek(scanner) != '\"' && !CScanner_IsAtEnd(scanner) )
			{
				if ( Char_IsEOL( CScanner_Peek(scanner) ) ) {
					scanner.line++;
					scanner.currentInLine = 0;
				}
				CScanner_Advance(scanner);
			}

			if ( CScanner_IsAtEnd(scanner) )
			{
				CScanner_SetError( scanner, "Unterminated string." );
				return;
			}

			CScanner_Advance(scanner);

			CScanner_AddToken(scanner, tokenList, TOKEN_STRING);
			break;

		default:
			if ( Char_IsDigit(c) )
			{
				while ( Char_IsDigit( CScanner_Peek(scanner) ) ) CScanner_Advance(scanner);

				if ( CScanner_Peek(scanner) == '.' && Char_IsDigit( CScanner_PeekNext(scanner) ) )
				{
					CScanner_Advance(scanner);
					while ( Char_IsDigit(CScanner_Peek(scanner)) ) CScanner_Advance(scanner);
					CScanner_Consume(scanner, 'f');
				}

				CScanner_AddToken(scanner, tokenList, TOKEN_NUMBER);
			}
			else if ( Char_IsAlpha(c) )
			{
				while ( Char_IsAlphaNumeric( CScanner_Peek(scanner) ) ) CScanner_Advance(scanner);

				String word = CScanner_ScannedString(scanner);

				// Keywords // TODO: This keyword search could be much more efficient
				if      ( StrEq( word, "struct" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_STRUCT);
				else if ( StrEq( word, "enum" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_ENUM);
				else if ( StrEq( word, "true" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "typedef" ) )      CScanner_AddToken(scanner, tokenList, TOKEN_TYPEDEF);
				else if ( StrEq( word, "extern" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_EXTERN);
				else if ( StrEq( word, "static" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_STATIC);
				else if ( StrEq( word, "register" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_REGISTER);
				else if ( StrEq( word, "auto" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_AUTO);
				else if ( StrEq( word, "thread_local" ) ) CScanner_AddToken(scanner, tokenList, TOKEN_THREAD_LOCAL);
				else if ( StrEq( word, "const" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_CONST);
				else if ( StrEq( word, "restrict" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_RESTRICT);
				else if ( StrEq( word, "volatile" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_VOLATILE);
				else if ( StrEq( word, "signed" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_SIGNED);
				else if ( StrEq( word, "unsigned" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_UNSIGNED);
				else if ( StrEq( word, "short" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_SHORT);
				else if ( StrEq( word, "long" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_LONG);
				else if ( StrEq( word, "void" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_VOID);
				else if ( StrEq( word, "bool" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_BOOL);
				else if ( StrEq( word, "char" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_CHAR);
				else if ( StrEq( word, "int" ) )          CScanner_AddToken(scanner, tokenList, TOKEN_INT);
				else if ( StrEq( word, "float" ) )        CScanner_AddToken(scanner, tokenList, TOKEN_FLOAT);
				else if ( StrEq( word, "double" ) )       CScanner_AddToken(scanner, tokenList, TOKEN_DOUBLE);
				else if ( StrEq( word, "NULL" ) )         CScanner_AddToken(scanner, tokenList, TOKEN_NULL);
				else if ( StrEq( word, "ARRAY_COUNT" ) )  CScanner_AddToken(scanner, tokenList, TOKEN_ARR_COUNT);
				else                                      CScanner_AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
			}
			else
			{
				CScanner_SetError( scanner, "Unexpected character." );
			}
	}
}

static CTokenList CScan(Arena &arena, const char *text, u32 textSize)
{
	CTokenList tokenList = {};
	tokenList.tokens = PushArray(arena, CToken, MAX_TOKEN_COUNT);

	CScanner scanner = {};
	scanner.line = 1;
	scanner.hasErrors = false;
	scanner.text = text;
	scanner.textSize = textSize;

	while ( !CScanner_IsAtEnd(scanner) )
	{
		CScanner_ScanToken(scanner, tokenList);
	}

	CScanner_AddToken(scanner, tokenList, TOKEN_EOF);

	tokenList.valid = !scanner.hasErrors;

	return tokenList;
}

#if 0
static void PrintTokenList(const CTokenList &tokenList)
{
	static const char *paddingStr =
		"                                                  "
		"                                                  ";
	printf("List of tokens:\n");
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		const CToken& token = tokenList.tokens[i];
		const int tokenIdLen = strlen(CTokenIdNames[token.id]);
		const int paddingSize = 32 - tokenIdLen;
		printf("%s:%.*s%.*s\n", CTokenIdNames[token.id], paddingSize, paddingStr, token.lexeme.size, token.lexeme.str);
	}
}
#endif


////////////////////////////////////////////////////////////////////////
// CAssembly

#if 0
static CStruct *CAssembly_CreateStruct(CAssembly &cAsm)
{
	ASSERT(cAsm.structCount < ARRAY_COUNT(cAsm.structs));
	CStruct *cStruct = &cAsm.structs[cAsm.structCount];
	cStruct->index = cAsm.structCount++;
	return cStruct;
}

const CStruct *CAssembly_GetStruct(const CAssembly &cAsm, u32 index)
{
	ASSERT(index < cAsm.structCount);
	const CStruct *cStruct = &cAsm.structs[index];
	return cStruct;
}

static const CStruct *CAssembly_FindStructWithName(const CAssembly &cAsm, String name)
{
	for (u32 i = 0; i < cAsm.structCount; ++i) {
		const CStruct *cStruct = &cAsm.structs[i];
		if ( StrEq(cStruct->name, name) ) {
			return cStruct;
		}
	}
	return 0;
}

static CEnum *CAssembly_CreateEnum(CAssembly &cAsm, Arena &arena)
{
	ASSERT(cAsm.enumCount < ARRAY_COUNT(cAsm.enums));
	CEnum *cEnum = &cAsm.enums[cAsm.enumCount];
	cEnum->index = cAsm.enumCount++;
	return cEnum;
}

const CEnum *CAssembly_GetEnum(const CAssembly &cAsm, u32 index)
{
	ASSERT(index < cAsm.enumCount);
	const CEnum *cEnum = &cAsm.enums[index];
	return cEnum;
}

static const CEnum *CAssembly_FindEnumWithName(const CAssembly &cAsm, String name)
{
	for (u32 i = 0; i < cAsm.enumCount; ++i) {
		const CEnum *cEnum = &cAsm.enums[i];
		if ( StrEq(cEnum->name, name) ) {
			return cEnum;
		}
	}
	return 0;
}

static CMember *CAssembly_AddMember(CStruct *cStruct)
{
	ASSERT(cStruct->memberCount < MAX_STRUCT_FIELD_COUNT);
	CMember *member = &cStruct->members[cStruct->memberCount++];
	return member;
}

const char *CAssembly_GetTypeName(const CAssembly &cAsm, ReflexID id)
{
	switch (id)
	{
		case ReflexID_Bool: return "ReflexID_Bool";
		case ReflexID_Char: return "ReflexID_Char";
		case ReflexID_Int: return "ReflexID_Int";
		case ReflexID_UInt: return "ReflexID_UInt";
		case ReflexID_Float: return "ReflexID_Float";
		default:
		{
			static char typeName[128];
			const u16 index = id - ReflexID_Struct;
			const CStruct *cStruct = CAssembly_GetStruct(cAsm, index);
			StrCopy(typeName, "ReflexID_");
			StrCat(typeName, cStruct->name);
			return typeName;
		}
	}
}
#endif


////////////////////////////////////////////////////////////////////////
// CParser

static u32 CParser_RemainingTokens(const CParser &parser)
{
	return parser.tokenList->count - parser.nextToken;
}

static bool CParser_HasFinished(const CParser &parser)
{
	const bool hasErrors = parser.hasErrors;
	const bool hasFinished = parser.nextToken >= parser.tokenList->count;
	return hasErrors || hasFinished;
}

static bool CParser_IsNextToken( const CParser &parser, CTokenId tokenId )
{
	if ( CParser_HasFinished( parser ) ) {
		return tokenId == TOKEN_EOF;
	} else {
		return tokenId == parser.tokenList->tokens[parser.nextToken].id;
	}
}

static bool CParser_AreNextTokens( const CParser &parser, CTokenId tokenId0, CTokenId tokenId1 )
{
	if ( CParser_HasFinished( parser ) || CParser_RemainingTokens(parser) < 2 ) {
		return false;
	} else {
		return
			tokenId0 == parser.tokenList->tokens[parser.nextToken].id &&
			tokenId1 == parser.tokenList->tokens[parser.nextToken+1].id;
	}
}

static bool CParser_AreNextTokens( const CParser &parser, CTokenId tokenId0, CTokenId tokenId1, CTokenId tokenId2 )
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

static const CToken &CParser_GetPreviousToken( const CParser &parser )
{
	ASSERT(parser.nextToken > 0);
	return parser.tokenList->tokens[parser.nextToken-1];
}

static const CToken &CParser_GetNextToken( const CParser &parser )
{
	return parser.tokenList->tokens[parser.nextToken];
}

static const CToken &CParser_GetLastToken( const CParser &parser )
{
	return parser.tokenList->tokens[parser.lastToken];
}

static void CParser_SetError(CParser &parser, const char *message)
{
	CToken token = CParser_GetNextToken(parser);
	printf("ERROR: %d:%.*s %s\n", token.line, token.lexeme.size, token.lexeme.str, message);
	parser.hasErrors = true;
}

static const CToken &CParser_Consume( CParser &parser )
{
	if ( !CParser_HasFinished( parser ) ) {
		parser.nextToken++;
		parser.lastToken = parser.nextToken > parser.lastToken ? parser.nextToken : parser.lastToken;
		return CParser_GetPreviousToken(parser);
	} else {
		CParser_SetError(parser, "Reached end of file");
		return gNullToken;
	}
}

//static const CToken &CParser_Consume( CParser &parser, CTokenId tokenId )
//{
//	if ( CParser_IsNextToken( parser, tokenId ) ) {
//		CParser_Consume( parser );
//	} else {
//		static char errorMessage[128];
//		StrCopy(errorMessage, "Could not consume token ");
//		StrCat(errorMessage, CTokenIdNames[tokenId]);
//		CParser_SetError(parser, errorMessage);
//		parser.hasFinished = true;
//	}
//	return CParser_GetPreviousToken(parser);
//}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId )
{
	if ( CParser_IsNextToken( parser, tokenId ) ) {
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId0, CTokenId tokenId1 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static bool CParser_TryConsume( CParser &parser, CTokenId tokenId0, CTokenId tokenId1, CTokenId tokenId2 )
{
	if ( CParser_AreNextTokens( parser, tokenId0, tokenId1, tokenId2 ) ) {
		CParser_Consume( parser );
		CParser_Consume( parser );
		CParser_Consume( parser );
		return true;
	}
	return false;
}

static void CParser_AddIdentifier( CParser &parser, String identifier )
{
	CIdentifierList *previousFirst = parser.identifiers;
	parser.identifiers = PushZeroStruct( *parser.arena, CIdentifierList );
	parser.identifiers->identifier = identifier;
	parser.identifiers->next = previousFirst;
}

static bool CParser_FindIdentifier( const CParser &parser, String identifier)
{
	CIdentifierList *node = parser.identifiers;
	while (node)
	{
		if (StrEq(node->identifier, identifier))
		{
			return true;
		}
		node = node->next;
	}
	return false;
}

//static bool CParser_ConsumeUntil( CParser &parser, CTokenId tokenId )
//{
//	while ( !CParser_HasFinished && !CParser_IsNextToken( parser, tokenId ) ) {
//		CParser_Consume( parser );
//	}
//	return ( CParser_GetPreviousToken( parser ).id == tokenId );
//}

#if 0
static void CParser_ParseMember(CParser &parser, CAssembly &cAsm, CStruct *cStruct, Arena &arena)
{
	// Parse type
	const bool isConst = CParser_TryConsume(parser, TOKEN_CONST);
	const bool isUnsigned = CParser_TryConsume(parser, TOKEN_UNSIGNED);
	const bool isShort = CParser_TryConsume(parser, TOKEN_SHORT);
	const bool isLong = CParser_TryConsume(parser, TOKEN_LONG);
	const bool isLongLong = CParser_TryConsume(parser, TOKEN_LONG);

	const CToken &type = CParser_Consume(parser);
	const bool isBool = type.id == TOKEN_BOOL;
	const bool isChar = type.id == TOKEN_CHAR;
	const bool isInt = type.id == TOKEN_INT;
	const bool isFloat = type.id == TOKEN_FLOAT;
	const bool isIdent = type.id == TOKEN_IDENTIFIER;
	ASSERT( !isUnsigned || isInt );

	ASSERT( isIdent || isBool || isChar || isInt || isFloat );

	u32 pointerCount = 0;
	for (u32 i = 0; i < 3; ++i) {
		if (CParser_TryConsume(parser, TOKEN_STAR)) {
			pointerCount++;
		}
	}

	const CToken &identifier = CParser_Consume(parser, TOKEN_IDENTIFIER);

	i32 arrayDim = 0;
	const bool isArray = CParser_TryConsume(parser, TOKEN_LEFT_BRACKET);
	if (isArray){
		const CToken &arrayDimToken = CParser_Consume(parser, TOKEN_NUMBER);
		arrayDim = StrToInt(arrayDimToken.lexeme);
		CParser_Consume(parser, TOKEN_RIGHT_BRACKET);
	}

	CParser_Consume(parser, TOKEN_SEMICOLON);

	CMember *member = CAssembly_AddMember(cStruct);
	member->name = identifier.lexeme;
	member->isConst = isConst;
	member->pointerCount = pointerCount;
	member->isArray = isArray;
	member->arrayDim = arrayDim;
	member->isArray = isArray;
	member->arrayDim = arrayDim;

	if ( isBool ) member->reflexId = ReflexID_Bool;
	else if ( isChar ) member->reflexId = ReflexID_Char;
	else if ( isInt && !isUnsigned ) member->reflexId = ReflexID_Int;
	else if ( isInt && isUnsigned ) member->reflexId = ReflexID_UInt;
	else if ( isFloat ) member->reflexId = ReflexID_Float;
	else // isIdent
	{
		ASSERT( type.id == TOKEN_IDENTIFIER );
		const CStruct *cStruct = CAssembly_FindStructWithName(cAsm, type.lexeme);
		if (cStruct) {
			member->reflexId = ReflexID_Struct + cStruct->index;
		} else {
			const CEnum *cEnum = CAssembly_FindEnumWithName(cAsm, type.lexeme);
			if (cEnum) {
				member->reflexId = ReflexID_Int; // TODO: Enums need to have more information than this
			} else {
				INVALID_CODE_PATH();
			}
		}
	}
}

static bool CParser_ParseStruct(CParser &parser, CAssembly &cAsm, Arena &arena)
{
	if ( CParser_TryConsume( parser, TOKEN_STRUCT ) )
	{
		CStruct *cStruct = CAssembly_CreateStruct(cAsm);
		cStruct->name = CParser_GetNextToken( parser ).lexeme;

		CParser_Consume( parser, TOKEN_IDENTIFIER );
		CParser_Consume( parser, TOKEN_LEFT_BRACE );
		while ( !CParser_TryConsume( parser, TOKEN_RIGHT_BRACE ) && !CParser_HasFinished( parser ) )
		{
			CParser_ParseMember(parser, cAsm, cStruct, arena);
		}
		CParser_Consume( parser, TOKEN_SEMICOLON );
		return true;
	}
	return false;
}

static bool CParser_ParseEnum(CParser &parser, CAssembly &cAsm, Arena &arena)
{
	if ( CParser_TryConsume( parser, TOKEN_ENUM ) )
	{
		CEnum *cEnum = CAssembly_CreateEnum(cAsm, arena);
		cEnum->name = CParser_GetNextToken(parser).lexeme;

		CParser_Consume( parser, TOKEN_IDENTIFIER );
		CParser_Consume( parser, TOKEN_LEFT_BRACE );
		while ( !CParser_TryConsume( parser, TOKEN_RIGHT_BRACE ) && !CParser_HasFinished( parser ) )
		{
			CParser_Consume(parser);
		}
		CParser_Consume( parser, TOKEN_SEMICOLON );
		return true;
	}
	return false;
}

static bool CParser_ParseDeclaration(CParser &parser, CAssembly &cAsm, Arena &arena)
{
	u32 nextTokenBackup = parser.nextToken;
	bool ok = CParser_ParseStruct( parser, cAsm, arena );
	ok = ok || CParser_ParseEnum( parser, cAsm, arena );
	if ( !ok ) { parser.nextToken = nextTokenBackup; }
	return ok;
}

static bool CParser_ParseFunctionDefinition(CParser &parser, CAssembly &cAsm, Arena &arena)
{
	CParser_Consume(parser);
	return true;
}

static bool CParser_ParseExternalDeclaration(CParser &parser, CAssembly &cAsm, Arena &arena)
{
	u32 nextTokenBackup = parser.nextToken;
	bool ok = CParser_ParseDeclaration( parser, cAsm, arena );
	ok = ok || CParser_ParseFunctionDefinition( parser, cAsm, arena );
	if ( !ok ) { parser.nextToken = nextTokenBackup; }
	return ok;
}

static CAssembly CParse(Arena &arena, const CTokenList &tokenList)
{
	CAssembly cAsm = {};

	CParser parser = {};
	parser.tokenList = &tokenList;

	while ( !CParser_HasFinished(parser) )
	{
		if ( !CParser_ParseExternalDeclaration(parser, cAsm, arena) )
		{
			break;
		}
	}

	cAsm.valid = !parser.hasErrors;

	if ( parser.hasErrors )
	{
		printf("Parse finished with errors.\n");
	}

	return cAsm;
}


////////////////////////////////////////////////////////////////////////
// CAssembly creation function

bool CAssembly_Create(CAssembly &cAsm, Arena &arena, const char *text, u64 textSize)
{
	CTokenList tokenList = CScan(arena, text, textSize);

	if ( tokenList.valid )
	{
#if 0
		PrintTokenList(tokenList);
#endif
		cAsm = CParse(arena, tokenList);

		return cAsm.valid;
	}

	return false;
}
#endif // #if 0


////////////////////////////////////////////////////////////////////////
// C AST helpers

#include <stdarg.h>

static char gCastError[4096] = {};

const char *Cast_GetError()
{
	return gCastError;
}

const void Cast_SetError(const char *format, ...)
{
	va_list vaList;
	va_start(vaList, format);
	vsnprintf(gCastError, ARRAY_COUNT(gCastError), format, vaList);
	va_end(vaList);
}


////////////////////////////////////////////////////////////////////////
// C AST structs

enum CastStorageClassSpecifierType
{
	CAST_TYPEDEF,
	CAST_EXTERN,
	CAST_STATIC,
	CAST_THREAD_LOCAL,
	CAST_AUTO,
	CAST_REGISTER,
};

struct CastStorageClassSpecifier
{
	CastStorageClassSpecifierType type;
};

enum CastTypeSpecifierType
{
	CAST_VOID,
	CAST_BOOL,
	CAST_CHAR,
	CAST_INT,
	CAST_FLOAT,
	CAST_DOUBLE,
	CAST_SHORT,
	CAST_LONG,
	CAST_SIGNED,
	CAST_UNSIGNED,
	CAST_IDENTIFIER,
	CAST_STRUCT,
	CAST_ENUM,
};

enum CastTypeQualifierType
{
	CAST_CONST,
	CAST_RESTRICT,
	CAST_VOLATILE,
};

struct CastTypeSpecifier;
struct CastTypeQualifier;

struct CastSpecifierQualifierList
{
	CastTypeSpecifier *typeSpecifier;
	CastTypeQualifier *typeQualifier;
	CastSpecifierQualifierList *next;
};

struct CastDeclarator;

struct CastStructDeclaratorList
{
	CastDeclarator *structDeclarator; // pass through struct-declarator in the grammar
	CastStructDeclaratorList *next;
};

struct CastStructDeclaration
{
	CastSpecifierQualifierList *specifierQualifierList;
	CastStructDeclaratorList *structDeclaratorList;
};

struct CastStructDeclarationList
{
	CastStructDeclaration *structDeclaration;
	CastStructDeclarationList *next;
};

struct CastStructSpecifier
{
	String name;
	CastStructDeclarationList *structDeclarationList;
};

struct CastEnumerator
{
	String name;
	// TODO: Could have a constant expression
};

struct CastEnumeratorList
{
	CastEnumerator *enumerator;
	CastEnumeratorList *next;
};

struct CastEnumSpecifier
{
	String name;
	CastEnumeratorList *enumeratorList;
};

struct CastTypeSpecifier
{
	CastTypeSpecifierType type;
	union
	{
		String identifier;
		CastStructSpecifier *structSpecifier;
		CastEnumSpecifier *enumSpecifier;
	};
};

struct CastTypeQualifier
{
	CastTypeQualifierType type;
};

struct CastTypeQualifierList
{
	CastTypeQualifier *typeQualifierList;
	CastTypeQualifierList *next;
};

struct CastFunctionSpecifier
{
// : INLINE
// | NORETURN
};

struct CastDeclarationSpecifiers
{
	CastStorageClassSpecifier *storageClassSpecifier;
	CastTypeSpecifier *typeSpecifier;
	CastTypeQualifier *typeQualifier;
	CastFunctionSpecifier *functionSpecifier;
	//CastAlignmentSpecifier *alignmentSpecifier;
	CastDeclarationSpecifiers *next;
};

struct CastExpression
{
	String constant; // TODO: Expressions can be much more complex
};

struct CastDirectDeclarator
{
	String name;
	CastExpression *expression;
	bool isArray : 1;
	// TODO could be a function as well
};

struct CastPointer
{
	CastTypeQualifierList *typeQualifierList;
	CastPointer *next;
};

struct CastDeclarator
{
	CastPointer *pointer;
	CastDirectDeclarator *directDeclarator;
};

struct CastDesignator
{
	String identifier;
	// TODO: Add missing array element designator: [ constant-expression ]
};

struct CastDesignatorList
{
	CastDesignator *designator;
	CastDesignatorList *next;
};

struct CastDesignation
{
	CastDesignatorList *designatorList;
};

struct CastInitializerList;

struct CastInitializer
{
	String value; // TODO: This should reference an assignment-expression
	CastInitializerList *initializerList;
};

struct CastInitializerList
{
	CastDesignation *designation; // optional
	CastInitializer *initializer;
	CastInitializerList *next;
};

struct CastInitDeclarator
{
	CastDeclarator *declarator;
	CastInitializer *initializer;
};

struct CastInitDeclaratorList
{
	CastInitDeclarator *initDeclarator;
	CastInitDeclaratorList *next;
};

struct CastDeclaration
{
	CastDeclarationSpecifiers *declarationSpecifiers;
	CastInitDeclaratorList *initDeclaratorList;
	//CastStaticAssertDeclaration *staticAssertDeclaration;
};

struct CastFunctionDefinition
{
	// TODO
};

union CastExternalDeclaration
{
	CastDeclaration *declaration;
	CastFunctionDefinition *functionDefinition;
};

struct CastTranslationUnit
{
	CastExternalDeclaration *externalDeclaration;
	CastTranslationUnit *next;
};

struct Cast
{
	CastTranslationUnit *translationUnit;
};

#define CAST_BACKUP() \
	Arena backupArena = *parser.arena; \
	u32 nextTokenBackup = parser.nextToken;

#define CAST_RESTORE() \
	*parser.arena = backupArena; \
	parser.nextToken = nextTokenBackup;

#define CAST_NODE( TypeName ) \
	PushZeroStruct( *parser.arena, TypeName )

CastStorageClassSpecifier *Cast_ParseStorageClassSpecifier( CParser &parser, CTokenList &tokenList )
{
	bool match = true;
	CastStorageClassSpecifierType type = CAST_TYPEDEF;
	if ( CParser_TryConsume( parser, TOKEN_TYPEDEF ) ) type = CAST_TYPEDEF;
	else if ( CParser_TryConsume( parser, TOKEN_EXTERN ) ) type = CAST_EXTERN;
	else if ( CParser_TryConsume( parser, TOKEN_STATIC ) ) type = CAST_STATIC;
	else if ( CParser_TryConsume( parser, TOKEN_THREAD_LOCAL ) ) type = CAST_THREAD_LOCAL;
	else if ( CParser_TryConsume( parser, TOKEN_AUTO ) ) type = CAST_AUTO;
	else if ( CParser_TryConsume( parser, TOKEN_REGISTER ) ) type = CAST_REGISTER;
	else { match = false; }

	CastStorageClassSpecifier *castStorageSpecifier = NULL;
	if (match) {
		castStorageSpecifier = CAST_NODE( CastStorageClassSpecifier );
		castStorageSpecifier->type = type;
	}
	return castStorageSpecifier;
}

CastTypeSpecifier *Cast_ParseTypeSpecifier( CParser &parser, CTokenList &tokenList );
CastTypeQualifier *Cast_ParseTypeQualifier( CParser &parser, CTokenList &tokenList );

CastSpecifierQualifierList *Cast_ParseSpecifierQualifierList( CParser &parser, CTokenList &tokenList )
{
	CastTypeSpecifier *typeSpecifier = NULL;
	CastTypeQualifier *typeQualifier = NULL;
	CastSpecifierQualifierList *firstSpecifierQualifierList = NULL;
	CastSpecifierQualifierList *prevSpecifierQualifierList = NULL;
	do
	{
		typeSpecifier = Cast_ParseTypeSpecifier(parser, tokenList);

		typeQualifier = Cast_ParseTypeQualifier(parser, tokenList);

		if ( typeSpecifier || typeQualifier )
		{
			CastSpecifierQualifierList *specifierQualifierList = CAST_NODE( CastSpecifierQualifierList );
			specifierQualifierList->typeSpecifier = typeSpecifier;
			specifierQualifierList->typeQualifier = typeQualifier;
			if (!firstSpecifierQualifierList) {
				firstSpecifierQualifierList = specifierQualifierList;
			}
			if (prevSpecifierQualifierList) {
				prevSpecifierQualifierList->next = specifierQualifierList;
			}
			prevSpecifierQualifierList = specifierQualifierList;
		}
	}
	while ( typeSpecifier || typeQualifier );
	return firstSpecifierQualifierList;
}

CastTypeQualifierList *Cast_ParseTypeQualifierList( CParser &parser, CTokenList &tokenList );

CastPointer *Cast_ParsePointer( CParser &parser, CTokenList &tokenList )
{
	CastPointer *pointer = NULL;
	if (CParser_TryConsume(parser, TOKEN_STAR))
	{
		CastTypeQualifierList *typeQualifierList = Cast_ParseTypeQualifierList(parser, tokenList);

		pointer = CAST_NODE( CastPointer );
		pointer->typeQualifierList = typeQualifierList;
	}
	return pointer;
}

CastExpression *Cast_ParseExpression( CParser &parser, CTokenList &tokenList )
{
	// TODO: make checks
	const CToken &token = CParser_Consume(parser);
	CastExpression *expression = CAST_NODE( CastExpression );
	expression->constant = token.lexeme;
	return expression;
}

CastDirectDeclarator *Cast_ParseDirectDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if (!CParser_TryConsume(parser, TOKEN_IDENTIFIER)) {
		return NULL;
	}
	const CToken &token = CParser_GetPreviousToken(parser);

	bool isArray = false;
	CastExpression *expression = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACKET) )
	{
		isArray = true;
		expression = Cast_ParseExpression(parser, tokenList);
		if (!CParser_TryConsume(parser, TOKEN_RIGHT_BRACKET)) {
			CAST_RESTORE();
			return NULL;
		}
	}

	CastDirectDeclarator *directDeclarator = CAST_NODE( CastDirectDeclarator );
	directDeclarator->name = token.lexeme;
	directDeclarator->expression = expression;
	directDeclarator->isArray = isArray;
	return directDeclarator;
}

CastDeclarator *Cast_ParseDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDeclarator *declarator =  NULL;
	CastPointer *pointer = Cast_ParsePointer(parser, tokenList);
	CastDirectDeclarator *directDeclarator = Cast_ParseDirectDeclarator(parser, tokenList);
	if (directDeclarator) {
		declarator = CAST_NODE( CastDeclarator );
		declarator->pointer = pointer;
		declarator->directDeclarator = directDeclarator;
	}
	if (!declarator) {
		CAST_RESTORE();
	}
	return declarator;
}

CastStructDeclaratorList *Cast_ParseStructDeclaratorList( CParser &parser, CTokenList &tokenList )
{
	CastDeclarator *declarator = NULL;
	CastStructDeclaratorList *firstStructDeclaratorList = NULL;
	CastStructDeclaratorList *prevStructDeclaratorList = NULL;
	do
	{
		declarator = Cast_ParseDeclarator(parser, tokenList);
		if (declarator)
		{
			CastStructDeclaratorList *structDeclaratorList = CAST_NODE( CastStructDeclaratorList );
			structDeclaratorList->structDeclarator = declarator;
			if (!firstStructDeclaratorList) {
				firstStructDeclaratorList = structDeclaratorList;
			}
			if (prevStructDeclaratorList) {
				prevStructDeclaratorList->next = structDeclaratorList;
			}
			prevStructDeclaratorList = structDeclaratorList;
		}
	}
	while (declarator);
	
	return firstStructDeclaratorList;
}

CastStructDeclaration *Cast_ParseStructDeclaration( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastStructDeclaration *structDeclaration = NULL;
	CastSpecifierQualifierList *specifierQualifierList = Cast_ParseSpecifierQualifierList(parser, tokenList);
	if ( specifierQualifierList )
	{
		CastStructDeclaratorList *structDeclaratorList = Cast_ParseStructDeclaratorList(parser, tokenList);
		if ( structDeclaratorList ) {
			if (CParser_TryConsume(parser, TOKEN_SEMICOLON)) {
				structDeclaration = CAST_NODE( CastStructDeclaration );
				structDeclaration->specifierQualifierList = specifierQualifierList;
				structDeclaration->structDeclaratorList = structDeclaratorList;
			}
		}
	}
	if ( !structDeclaration ) {
		CAST_RESTORE();
	}
	return structDeclaration;
}

CastStructDeclarationList *Cast_ParseStructDeclarationList( CParser &parser, CTokenList &tokenList )
{
	CastStructDeclaration *structDeclaration = NULL;
	CastStructDeclarationList *firstDeclarationList = NULL;
	CastStructDeclarationList *previousDeclarationList = NULL;
	do
	{
		structDeclaration = Cast_ParseStructDeclaration(parser, tokenList);
		if ( structDeclaration )
		{
			CastStructDeclarationList *structDeclarationList = CAST_NODE( CastStructDeclarationList );
			structDeclarationList->structDeclaration = structDeclaration;
			if (!firstDeclarationList) {
				firstDeclarationList = structDeclarationList;
			}
			if (previousDeclarationList) {
				previousDeclarationList->next = structDeclarationList;
			}
			previousDeclarationList = structDeclarationList;
		}
	}
	while ( structDeclaration );
	return firstDeclarationList;
}

CastStructSpecifier *Cast_ParseStructSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if ( !CParser_TryConsume(parser, TOKEN_STRUCT) ) {
		CAST_RESTORE();
		return NULL;
	}
	if ( !CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		CAST_RESTORE();
		return NULL;
	}
	const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
	String identifier = tokenIdentifier.lexeme;
	CParser_AddIdentifier(parser, identifier);

	CastStructDeclarationList* structDeclarationList = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACE) )
	{
		structDeclarationList = Cast_ParseStructDeclarationList(parser, tokenList);

		if ( !CParser_TryConsume(parser, TOKEN_RIGHT_BRACE) )
		{
			CAST_RESTORE();
			return NULL;
		}
	}
	CastStructSpecifier *structSpecifier = CAST_NODE( CastStructSpecifier );
	structSpecifier->name = identifier;
	structSpecifier->structDeclarationList = structDeclarationList;
	return structSpecifier;
}

CastEnumerator *Cast_ParseEnumerator( CParser &parser, CTokenList &tokenList )
{
	CastEnumerator *enumerator = NULL;
	if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		const CToken &identifier = CParser_GetPreviousToken(parser);
		enumerator = CAST_NODE( CastEnumerator );
		enumerator->name = identifier.lexeme;
	}
	return enumerator;
}

CastEnumeratorList *Cast_ParseEnumeratorList( CParser &parser, CTokenList &tokenList )
{
	CastEnumerator *enumerator = NULL;
	CastEnumeratorList *firstEnumeratorList = NULL;
	CastEnumeratorList *prevEnumeratorList = NULL;
	do
	{
		enumerator = Cast_ParseEnumerator(parser, tokenList);
		if (enumerator)
		{
			CParser_TryConsume(parser, TOKEN_COMMA);

			CastEnumeratorList *enumeratorList = CAST_NODE( CastEnumeratorList );
			enumeratorList->enumerator = enumerator;
			if (!firstEnumeratorList) {
				firstEnumeratorList = enumeratorList;
			}
			if (prevEnumeratorList) {
				prevEnumeratorList->next = enumeratorList;
			}
			prevEnumeratorList = enumeratorList;
		}
	} while (enumerator);
	return firstEnumeratorList;
}

CastEnumSpecifier *Cast_ParseEnumSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	if ( !CParser_TryConsume(parser, TOKEN_ENUM) ) {
		return NULL;
	}

	String name = {};
	if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
		const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
		String identifier = tokenIdentifier.lexeme;
		CParser_AddIdentifier(parser, identifier);
		name = identifier;
	}

	CastEnumeratorList *enumeratorList = NULL;
	if ( CParser_TryConsume(parser, TOKEN_LEFT_BRACE) )
	{
		Cast_ParseEnumeratorList(parser, tokenList);
		CParser_TryConsume(parser, TOKEN_COMMA);
		if (!CParser_TryConsume(parser, TOKEN_RIGHT_BRACE))
		{
			CAST_RESTORE();
			return NULL;
		}
	}

	CastEnumSpecifier *enumSpecifier = CAST_NODE( CastEnumSpecifier );
	enumSpecifier->name = name;
	enumSpecifier->enumeratorList = enumeratorList;
	return enumSpecifier;
}

CastTypeSpecifier *Cast_ParseTypeSpecifier( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();

	bool match = true;
	CastTypeSpecifierType type = CAST_VOID;
	String identifier = {};
	CastStructSpecifier *structSpecifier = NULL;
	CastEnumSpecifier *enumSpecifier = NULL;
	if ( CParser_TryConsume(parser, TOKEN_VOID) ) type = CAST_VOID;
	else if ( CParser_TryConsume(parser, TOKEN_BOOL) ) type = CAST_BOOL;
	else if ( CParser_TryConsume(parser, TOKEN_CHAR) ) type = CAST_CHAR;
	else if ( CParser_TryConsume(parser, TOKEN_INT) ) type = CAST_INT;
	else if ( CParser_TryConsume(parser, TOKEN_FLOAT) ) type = CAST_FLOAT;
	else if ( CParser_TryConsume(parser, TOKEN_DOUBLE) ) type = CAST_DOUBLE;
	else if ( CParser_TryConsume(parser, TOKEN_SHORT) ) type = CAST_SHORT;
	else if ( CParser_TryConsume(parser, TOKEN_LONG) ) type = CAST_LONG;
	else if ( CParser_TryConsume(parser, TOKEN_SIGNED) ) type = CAST_SIGNED;
	else if ( CParser_TryConsume(parser, TOKEN_UNSIGNED) ) type = CAST_UNSIGNED;
	else if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) )
	{
		const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
		identifier = tokenIdentifier.lexeme;
		if ( CParser_FindIdentifier(parser, identifier) ) {
			type = CAST_IDENTIFIER;
		} else {
			CAST_RESTORE();
			match = false;
		}
	}
	else
	{
		type = CAST_STRUCT;
		structSpecifier = Cast_ParseStructSpecifier(parser, tokenList);
		if ( !structSpecifier )
		{
			type = CAST_ENUM;
			enumSpecifier = Cast_ParseEnumSpecifier(parser, tokenList);
			if ( !enumSpecifier )
			{
				match = false;
			}
		}
	}

	CastTypeSpecifier *castTypeSpecifier = NULL;
	if (match)
	{
		castTypeSpecifier = CAST_NODE( CastTypeSpecifier );
		castTypeSpecifier->type = type;
		switch (type) {
			case CAST_IDENTIFIER: castTypeSpecifier->identifier = identifier; break;
			case CAST_STRUCT: castTypeSpecifier->structSpecifier = structSpecifier; break;
			case CAST_ENUM: castTypeSpecifier->enumSpecifier = enumSpecifier; break;
			default:;
		};
	}
	return castTypeSpecifier;
}

CastTypeQualifier *Cast_ParseTypeQualifier( CParser &parser, CTokenList &tokenList )
{
	bool match = true;
	CastTypeQualifierType type = CAST_CONST;
	if ( CParser_TryConsume( parser, TOKEN_CONST) ) type = CAST_CONST;
	else if ( CParser_TryConsume( parser, TOKEN_RESTRICT) ) type = CAST_RESTRICT;
	else if ( CParser_TryConsume( parser, TOKEN_VOLATILE) ) type = CAST_VOLATILE;
	else { match = false; }

	CastTypeQualifier *typeQualifier = NULL;
	if (match) {
		typeQualifier = CAST_NODE( CastTypeQualifier );
		typeQualifier->type = type;
	}
	return typeQualifier;
}

CastTypeQualifierList *Cast_ParseTypeQualifierList( CParser &parser, CTokenList &tokenList )
{
	return NULL;
}

CastFunctionSpecifier *Cast_ParseFunctionSpecifier( CParser &parser, CTokenList &tokenList )
{
	return NULL;
}

CastDeclarationSpecifiers *Cast_ParseDeclarationSpecifiers( CParser &parser, CTokenList &tokenList )
{
	CastStorageClassSpecifier *storageClassSpecifier = Cast_ParseStorageClassSpecifier(parser, tokenList);
	CastTypeSpecifier *typeSpecifier = Cast_ParseTypeSpecifier(parser, tokenList);
	CastTypeQualifier *typeQualifier = Cast_ParseTypeQualifier(parser, tokenList);
	CastFunctionSpecifier *functionSpecifier = Cast_ParseFunctionSpecifier(parser, tokenList);

	CastDeclarationSpecifiers *declarationSpecifiers =  NULL;
	if (storageClassSpecifier || typeSpecifier || typeQualifier || functionSpecifier) {
		declarationSpecifiers = CAST_NODE( CastDeclarationSpecifiers );
		declarationSpecifiers->storageClassSpecifier = storageClassSpecifier;
		declarationSpecifiers->typeSpecifier = typeSpecifier;
		declarationSpecifiers->typeQualifier = typeQualifier;
		declarationSpecifiers->functionSpecifier = functionSpecifier;
		declarationSpecifiers->next = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	}
	return declarationSpecifiers;
}

CastDesignator *Cast_ParseDesignator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDesignator *designator = NULL;
	if ( CParser_TryConsume(parser, TOKEN_DOT) ) {
		if ( CParser_TryConsume(parser, TOKEN_IDENTIFIER) ) {
			const CToken &tokenIdentifier = CParser_GetPreviousToken(parser);
			designator = CAST_NODE( CastDesignator );
			designator->identifier = tokenIdentifier.lexeme;
		}
	}
	// TODO: Parse array element designator: [ constant-expression ]
	if (!designator) {
		CAST_RESTORE();
	}
	return NULL;
}

CastDesignatorList *Cast_ParseDesignatorList( CParser &parser, CTokenList &tokenList )
{
	CastDesignatorList *firstDesignatorList = NULL;
	CastDesignatorList *prevDesignatorList = NULL;
	CastDesignator *designator = NULL;
	do
	{
		designator = Cast_ParseDesignator(parser, tokenList);
		if (designator)
		{
			CastDesignatorList *designatorList = CAST_NODE( CastDesignatorList );
			designatorList->designator = designator;
			if (!firstDesignatorList) {
				firstDesignatorList = designatorList;
			}
			if (prevDesignatorList) {
				prevDesignatorList->next = designatorList;
			}
			prevDesignatorList = designatorList;
		}
	} while (designator);
	return firstDesignatorList;
}

CastDesignation *Cast_ParseDesignation( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastDesignation *designation = NULL;
	CastDesignatorList *designatorList = Cast_ParseDesignatorList(parser, tokenList);
	if ( designatorList && CParser_TryConsume(parser, TOKEN_EQUAL) )
	{
		designation = CAST_NODE( CastDesignation );
		designation->designatorList = designatorList;
	}
	if (!designation) {
		CAST_RESTORE();
	}
	return designation;
}

CastInitializerList *Cast_ParseInitializerList ( CParser &parser, CTokenList &tokenList );

CastInitializer *Cast_ParseInitializer( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastInitializer *castInitializer = NULL;
	if (CParser_TryConsume(parser, TOKEN_LEFT_BRACE))
	{
		CastInitializerList *initializerList = Cast_ParseInitializerList(parser, tokenList);
		if (initializerList)
		{
			castInitializer = CAST_NODE( CastInitializer );
			castInitializer->initializerList = initializerList;
			CParser_TryConsume(parser, TOKEN_RIGHT_BRACE);
		}
	}
	else
	{
		// TODO: Parse this better. It could be any kind of expression
		const CToken &tokenExpression = CParser_Consume(parser);
		castInitializer = CAST_NODE( CastInitializer );
		castInitializer->value = tokenExpression.lexeme;
	}
	if (!castInitializer) {
		CAST_RESTORE();
	}
	return castInitializer;
}

CastInitializerList *Cast_ParseInitializerList ( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	bool comma = false;
	CastInitializer *initializer = NULL;
	CastInitializerList *firstInitalizerList = NULL;
	CastInitializerList *prevInitalizerList = NULL;
	do
	{
		CastDesignation *designation = Cast_ParseDesignation(parser, tokenList);
		initializer = Cast_ParseInitializer(parser, tokenList);
		if (initializer)
		{
			CastInitializerList *initializerList = CAST_NODE( CastInitializerList );
			initializerList->designation = designation;
			initializerList->initializer = initializer;
			if ( !firstInitalizerList ) {
				firstInitalizerList = initializerList;
			}
			if ( prevInitalizerList ) {
				prevInitalizerList->next = initializerList;
			}
			prevInitalizerList = initializerList;
			comma = CParser_TryConsume(parser, TOKEN_COMMA);
		}
		else if ( designation || comma )
		{
			CAST_RESTORE();
			firstInitalizerList = NULL;
		}
	} while (initializer && comma);
	return firstInitalizerList;
}

CastInitDeclarator *Cast_ParseInitDeclarator( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	CastInitDeclarator *initDeclarator = NULL;
	CastInitializer *initializer = NULL;
	CastDeclarator *declarator = Cast_ParseDeclarator(parser, tokenList);
	if ( declarator )
	{
		if ( CParser_TryConsume(parser, TOKEN_EQUAL) )
		{
			initializer = Cast_ParseInitializer(parser, tokenList);
			if (!initializer) {
				CAST_RESTORE();
				return NULL;
			}
		}

		initDeclarator = CAST_NODE( CastInitDeclarator );
		initDeclarator->declarator = declarator;
		initDeclarator->initializer = initializer;
	}
	return initDeclarator;
}

CastInitDeclaratorList *Cast_ParseInitDeclaratorList( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();
	bool comma = false;
	CastInitDeclarator *initDeclarator = NULL;
	CastInitDeclaratorList *firstInitDeclaratorList = NULL;
	CastInitDeclaratorList *prevInitDeclaratorList = NULL;
	do
	{
		initDeclarator = Cast_ParseInitDeclarator(parser, tokenList);
		if (initDeclarator)
		{
			CastInitDeclaratorList *initDeclaratorList = CAST_NODE( CastInitDeclaratorList );
			initDeclaratorList->initDeclarator = initDeclarator;
			if ( !firstInitDeclaratorList ) {
				firstInitDeclaratorList = initDeclaratorList;
			}
			if ( prevInitDeclaratorList ) {
				prevInitDeclaratorList->next = initDeclaratorList;
			}
			prevInitDeclaratorList = initDeclaratorList;
			comma = CParser_TryConsume(parser, TOKEN_COMMA);
		}
		else if ( comma )
		{
			CAST_RESTORE();
			firstInitDeclaratorList = NULL;
		}
	} while ( initDeclarator );
	return firstInitDeclaratorList;
}

CastDeclaration *Cast_ParseDeclaration( CParser &parser, CTokenList &tokenList )
{
	CAST_BACKUP();

	CastDeclarationSpecifiers *declarationSpecifiers = Cast_ParseDeclarationSpecifiers(parser, tokenList);
	if (!declarationSpecifiers)
	{
		return NULL;
	}

	CastInitDeclaratorList *initDeclaratorList = Cast_ParseInitDeclaratorList(parser, tokenList);

	CastDeclaration *declaration = CAST_NODE( CastDeclaration );
	if ( CParser_TryConsume(parser, TOKEN_SEMICOLON) )
	{
		declaration->declarationSpecifiers = declarationSpecifiers;
		declaration->initDeclaratorList = initDeclaratorList;
	}

	if (!declaration) {
		CAST_RESTORE();
	}
	return declaration;
}

CastFunctionDefinition *Cast_ParseFunctionDefinition( CParser &parser, CTokenList &tokenList )
{
	return NULL;
}

CastExternalDeclaration *Cast_ParseExternalDeclaration( CParser &parser, CTokenList &tokenList )
{
	CastExternalDeclaration *externalDeclaration = NULL;
	CastDeclaration *declaration = Cast_ParseDeclaration(parser, tokenList);
	if (declaration)
	{
		externalDeclaration = CAST_NODE( CastExternalDeclaration );
		externalDeclaration->declaration = declaration;
	}
	return externalDeclaration;
}

CastTranslationUnit *Cast_ParseTranslationUnit( CParser &parser, CTokenList &tokenList )
{
	CastTranslationUnit *firstTranslationUnit = NULL;
	CastTranslationUnit *previousTranslationUnit = NULL;
	while ( !CParser_HasFinished(parser) )
	{
		CastExternalDeclaration *externalDeclaration = Cast_ParseExternalDeclaration(parser, tokenList);
		if ( externalDeclaration )
		{
			CastTranslationUnit *translationUnit = CAST_NODE( CastTranslationUnit );
			translationUnit->externalDeclaration = externalDeclaration;
			if (!firstTranslationUnit) {
				firstTranslationUnit = translationUnit;
			}
			if (previousTranslationUnit) {
				previousTranslationUnit->next = translationUnit;
			}
			previousTranslationUnit = translationUnit;
		}
		else
		{
			CToken token = CParser_GetNextToken(parser);
			CToken lastToken = CParser_GetLastToken(parser);
			Cast_SetError("Invalid grammar between lines: %u-%u\n", token.line, lastToken.line);
			firstTranslationUnit = NULL;
			break;
		}
	}
	return firstTranslationUnit;
}


Cast *Cast_Create( CParser &parser, CTokenList &tokenList )
{
	Cast *cast = NULL;
	CastTranslationUnit *translationUnit = Cast_ParseTranslationUnit(parser, tokenList);
	if ( translationUnit ) {
		cast = CAST_NODE( Cast );
		cast->translationUnit = translationUnit;
	}
	return cast;
}

Cast *Cast_Create( Arena &arena, const char *text, u64 textSize)
{
	CTokenList tokenList = CScan(arena, text, textSize);

	if ( tokenList.valid )
	{
		CParser parser = {};
		parser.tokenList = &tokenList;
		parser.arena = &arena;
		Cast *cast = Cast_Create(parser, tokenList);
		return cast;
	}

	return NULL;
}

void Cast_Print( Cast *cast )
{
}


#endif // #ifdef CPARSER_IMPLEMENTATION

#endif // #ifndef CPARSER_H

