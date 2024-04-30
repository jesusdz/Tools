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

struct CParser
{
	const CTokenList *tokenList;
	u32 nextToken;
	bool hasErrors;
	bool hasFinished;
};

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
				if      ( StrEq( word, "struct" ) )   CScanner_AddToken(scanner, tokenList, TOKEN_STRUCT);
				else if ( StrEq( word, "enum" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_ENUM);
				else if ( StrEq( word, "true" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_TRUE);
				else if ( StrEq( word, "false" ) )    CScanner_AddToken(scanner, tokenList, TOKEN_FALSE);
				else if ( StrEq( word, "static" ) )   CScanner_AddToken(scanner, tokenList, TOKEN_STATIC);
				else if ( StrEq( word, "const" ) )    CScanner_AddToken(scanner, tokenList, TOKEN_CONST);
				else if ( StrEq( word, "unsigned" ) ) CScanner_AddToken(scanner, tokenList, TOKEN_UNSIGNED);
				else if ( StrEq( word, "short" ) )    CScanner_AddToken(scanner, tokenList, TOKEN_SHORT);
				else if ( StrEq( word, "long" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_LONG);
				else if ( StrEq( word, "bool" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_BOOL);
				else if ( StrEq( word, "char" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_CHAR);
				else if ( StrEq( word, "int" ) )      CScanner_AddToken(scanner, tokenList, TOKEN_INT);
				else if ( StrEq( word, "float" ) )    CScanner_AddToken(scanner, tokenList, TOKEN_FLOAT);
				else if ( StrEq( word, "NULL" ) )     CScanner_AddToken(scanner, tokenList, TOKEN_NULL);
				else if ( StrEq( word, "ARRAY_COUNT" ) ) CScanner_AddToken(scanner, tokenList, TOKEN_ARR_COUNT);
				else                                  CScanner_AddToken(scanner, tokenList, TOKEN_IDENTIFIER);
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
		case ReflexID_UnsignedChar: return "ReflexID_UnsignedChar";
		case ReflexID_Int: return "ReflexID_Int";
		case ReflexID_ShortInt: return "ReflexID_ShortInt";
		case ReflexID_LongInt: return "ReflexID_LongInt";
		case ReflexID_LongLongInt: return "ReflexID_LongLongInt";
		case ReflexID_UnsignedInt: return "ReflexID_UnsignedInt";
		case ReflexID_UnsignedShortInt: return "ReflexID_UnsignedShortInt";
		case ReflexID_UnsignedLongInt: return "ReflexID_UnsignedLongInt";
		case ReflexID_UnsignedLongLongInt: return "ReflexID_UnsignedLongLongInt";
		case ReflexID_Float: return "ReflexID_Float";
		case ReflexID_Double: return "ReflexID_Double";
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
	} else {
		CParser_SetError(parser, "Reached end of file");
	}
	return CParser_GetPreviousToken(parser);
}

static const CToken &CParser_Consume( CParser &parser, CTokenId tokenId )
{
	if ( CParser_IsNextToken( parser, tokenId ) ) {
		CParser_Consume( parser );
	} else {
		static char errorMessage[128];
		StrCopy(errorMessage, "Could not consume token ");
		StrCat(errorMessage, CTokenIdNames[tokenId]);
		CParser_SetError(parser, errorMessage);
		parser.hasFinished = true;
	}
	return CParser_GetPreviousToken(parser);
}

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
	else if ( isChar ) member->reflexId = isUnsigned ? ReflexID_UnsignedChar : ReflexID_Char;
	else if ( isInt ) member->reflexId = isUnsigned ? ReflexID_UnsignedInt : ReflexID_Int;
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

#endif // #ifdef CPARSER_IMPLEMENTATION

#endif // #ifndef CPARSER_H

