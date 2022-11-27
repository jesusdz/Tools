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
#define INVALID_CODE_PATH() ERROR("Invalid code path")
#define ARRAY_COUNT(array) (sizeof(array)/sizeof(array[0]))



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

void StrCopy(char *dst, const String& src_string)
{
	u32 size = src_string.size;
	const char *src = src_string.str;
	while (size-- > 0) *dst++ = *src++;
	dst[src_string.size] = '\0';
}

void StrCopy(char *dst, const char *src)
{
	while (*src) *dst++ = *src++;
	*dst = 0;
}

void StrCat(char *dst, const char *src)
{
	while (*dst) ++dst;
	StrCopy(dst, src);
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

bool StrEq(const char *s1, const char *s2)
{
	while ( *s1 == *s2 && *s1 )
	{
		s1++;
		s2++;
	}
	return *s1 == *s2;
}

f32 StrToFloat(const String &s)
{
	char buf[256] = {};
	ASSERT(s.size + 1 < ARRAY_COUNT(buf));
	StrCopy(buf, s);
	f32 number = atof(buf);
	return number;
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

void ResetArena(Arena &arena)
{
	arena.used = 0;
}

void PrintArenaUsage(Arena &arena)
{
	printf("Memory Arena Usage:\n");
	printf("- size: %u B / %u kB\n", arena.size, arena.size/1024);
	printf("- used: %u B / %u kB\n", arena.used, arena.used/1024);
}

#define ZeroStruct( pointer ) MemSet(pointer, sizeof(*pointer) )
#define PushStruct( arena, struct_type ) (struct_type*)PushSize(arena, sizeof(struct_type))
#define PushArray( arena, type, count ) (type*)PushSize(arena, sizeof(type) * count)

#if __APPLE__

#include<sys/mman.h>

void* AllocateVirtualMemory(u32 size)
{
	void* baseAddress = 0;
	int prot = PROT_READ | PROT_WRITE;
	int flags = MAP_ANONYMOUS | MAP_SHARED;
	int fd = -1;
	off_t offset = 0;
	void *allocatedMemory = mmap(baseAddress, size, prot, flags, fd, offset);
	ASSERT( allocatedMemory != MAP_FAILED && "Failed to allocate memory." );
	return allocatedMemory;
}

#else
#	error "Missing platform implementation"
#endif

void MemSet(void *ptr, u32 size)
{
	byte *bytePtr = (byte*)ptr;
	while (size-- > 0) *bytePtr++ = 0;
}



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



enum ValueType
{
	VALUE_TYPE_BOOL,
	//VALUE_TYPE_INT,
	VALUE_TYPE_FLOAT,
	VALUE_TYPE_STRING,
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
	TokenType type;
	String lexeme;
	Literal literal;
	i32 line;
};

struct TokenList
{
	Token *tokens;
	i32 count;
};

struct ScanState
{
	i32 start;
	i32 current;
	i32 line;
	bool hasErrors;

	const char *script;
	u32 scriptSize;
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

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenType tokenType, Literal literal)
{
	Token newToken = {};
	newToken.type = tokenType;
	newToken.lexeme = ScannedString(scanState);
	newToken.literal = literal;
	newToken.line = scanState.line;

	ASSERT( tokenList.count < MAX_TOKEN_COUNT );
	tokenList.tokens[tokenList.count++] = newToken;
}

void AddToken(const ScanState &scanState, TokenList &tokenList, TokenType tokenType)
{
	Literal literal = {};

	if ( tokenType == TOKEN_STRING )
	{
		literal.type = VALUE_TYPE_STRING;
		literal.s = ScannedString(scanState);
	}
	else if ( tokenType == TOKEN_NUMBER )
	{
		literal.type = VALUE_TYPE_FLOAT;
		literal.f = StrToFloat( ScannedString(scanState) );
	}
	else if ( tokenType == TOKEN_TRUE || tokenType == TOKEN_FALSE )
	{
		literal.type = VALUE_TYPE_BOOL;
		literal.b = tokenType == TOKEN_TRUE;
	}

	AddToken(scanState, tokenList, tokenType, literal);
}

void ReportError(ScanState &scanState, const char *message)
{
	printf("ERROR: %d:%d: %s\n", scanState.line, scanState.current, message);
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
				else if ( StrEq( word, "class" ) )  AddToken(scanState, tokenList, TOKEN_CLASS);
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

TokenList Scan(Arena &arena, ScanState &scanState, const char *script, u32 scriptSize)
{
	TokenList tokenList = {};
	tokenList.tokens = PushArray(arena, Token, MAX_TOKEN_COUNT);

	scanState.line = 1;
	scanState.hasErrors = false;
	scanState.script = script;
	scanState.scriptSize = scriptSize;

	while ( !IsAtEnd(scanState) )
	{
		ScanToken(scanState, tokenList);
	}

	AddToken(scanState, tokenList, TOKEN_EOF);

	return tokenList;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Parser

// Grammar:
// program        -> statement* EOF
// statement      -> exprStatement | printStatement
// exprStatement  -> expression ";"
// printStatement -> "print" "(" expression ")" ";"
// expression     -> equality
// equality       -> comparison ( ( "!=" | "==") comparison )
// comparison     -> term ( ( ">" | ">=" | "<" | "<=" ) term )*
// term           -> factor ( ( "-" | "+" ) factor )*
// factor         -> unary ( ( "/" | "*" ) unary )*
// unary          -> ( "!" | "-" ) unary | primary
// primary        -> NUMBER | STRING | "true" | "false" | "nil" | "(" expression ")"

struct ParseState
{
	TokenList* tokenList;
	u32 current;
	bool hasErrors;
};

enum ExprType
{
	EXPR_LITERAL,
	EXPR_UNARY,
	EXPR_BINARY,
};

struct Expr;

struct ExprLiteral
{
	Token *literalToken;
};

struct ExprUnary
{
	Token *operatorToken;
	Expr *expr;
};

struct ExprBinary
{
	Expr *left;
	Token *operatorToken;
	Expr *right;
};

struct Expr
{
	ExprType type;
	union
	{
		ExprLiteral literal;
		ExprUnary unary;
		ExprBinary binary;
	};
};

struct ExprList
{
	Expr exprs[8];
	u32 exprsCount;
	ExprList *next;
};

enum StmtType
{
	STMT_PRINT,
	STMT_EXPR,
};

struct Stmt
{
	StmtType type;
	Expr *expr;
};

struct StmtList
{
	Stmt stmts[8];
	u32 stmtsCount;
	StmtList *next;
};

struct Program
{
	Arena *arena;
	ExprList *expressions;
	StmtList *statements;
};

void ReportError(ParseState &parseState, const char *message)
{
	Token &token = parseState.tokenList->tokens[ parseState.current ];
	u32 line = token.line;
	printf("ERROR: %d: %s\n", line, message);
	parseState.hasErrors = true;
}

bool IsAtEnd(const ParseState &parseState)
{
	const TokenList &tokenList = *parseState.tokenList;
	const Token &currentToken = tokenList.tokens[ parseState.current ];
	return currentToken.type == TOKEN_EOF;
}

bool Consume(ParseState &parseState, TokenType tokenType)
{
	TokenList &tokenList = *parseState.tokenList;
	Token &currentToken = tokenList.tokens[ parseState.current ];
	if ( currentToken.type == tokenType )
	{
		parseState.current++;
		return true;
	}
	else
	{
		return false;
	}
}

void ConsumeForced(ParseState &parseState, TokenType tokenType)
{
	if ( !Consume(parseState, tokenType) )
	{
		char errorMessage[512];
		StrCopy(errorMessage, "Expected token ");
		StrCat(errorMessage, TokenNames[tokenType]);
		StrCat(errorMessage, " not found.");
		ReportError( parseState, errorMessage );
	}
}

Token* Consumed(ParseState &parseState)
{
	ASSERT( parseState.current > 0 );
	TokenList &tokenList = *parseState.tokenList;
	Token &consumedToken = tokenList.tokens[ parseState.current - 1 ];
	return &consumedToken;
}

Expr* AddExpression(Program &program)
{
	// Ensure the first expression list block
	if ( !program.expressions )
	{
		program.expressions = PushStruct(*program.arena, ExprList);
		program.expressions->exprsCount = 0;
		program.expressions->next = 0;
	}

	// Advance to the last expression list block
	ExprList *list = program.expressions;
	while ( list->next ) list = list->next;

	// Ensure we have a list block with space
	if ( list->exprsCount == ARRAY_COUNT(list->exprs) )
	{
		list->next = PushStruct(*program.arena, ExprList);
		list->next->exprsCount = 0;
		list->next->next = 0;
		list = list->next;
	}

	Expr* expr = &list->exprs[list->exprsCount++];
	return expr;
}

Expr* AddExpression(Program &program, Token* literalToken)
{
	Expr* expr = AddExpression(program);
	expr->type = EXPR_LITERAL;
	expr->literal.literalToken = literalToken;
	return expr;
}

Expr* AddExpression(Program &program, Token* operatorToken, Expr* pExpr)
{
	Expr* expr = AddExpression(program);
	expr->type = EXPR_UNARY;
	expr->unary.operatorToken = operatorToken;
	expr->unary.expr = pExpr;
	return expr;
}

Expr* AddExpression(Program &program, Expr* left, Token* operatorToken, Expr* right)
{
	Expr* expr = AddExpression(program);
	expr->type = EXPR_BINARY;
	expr->binary.left = left;
	expr->binary.operatorToken = operatorToken;
	expr->binary.right = right;
	return expr;
}

Stmt* AddStatement(Program &program)
{
	// Ensure first list block
	if ( !program.statements )
	{
		program.statements = PushStruct(*program.arena, StmtList);
		program.statements->stmtsCount = 0;
		program.statements->next = 0;
	}

	// Advance to the last list block
	StmtList *list = program.statements;
	while ( list->next ) list = list->next;

	// Ensure a current list block with space
	if ( list->stmtsCount == ARRAY_COUNT( list->stmts ) )
	{
		list->next = PushStruct(*program.arena, StmtList);
		list->next->stmtsCount = 0;
		list->next->next = 0;
		list = list->next;
	}

	Stmt *stmt = &list->stmts[ list->stmtsCount++ ];
	return stmt;
}

Stmt* AddPrintStatement(Program &program, Expr *expr)
{
	Stmt *statement = AddStatement(program);
	statement->type = STMT_PRINT;
	statement->expr = expr;
	return statement;
}

Stmt* AddExpressionStatement(Program &program, Expr *expr)
{
	Stmt *statement = AddStatement(program);
	statement->type = STMT_EXPR;
	statement->expr = expr;
	return statement;
}

Expr* ParseExpression(ParseState &parseState, Program &program);

Expr* ParsePrimary(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_FALSE) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_TRUE) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_NIL) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_NUMBER) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_STRING) ) return AddExpression(program, Consumed(parseState));
	if ( Consume(parseState, TOKEN_LEFT_PAREN) )
	{
		Expr* expr = ParseExpression(parseState, program);
		ConsumeForced(parseState, TOKEN_RIGHT_PAREN);
		return expr;
	}

	ReportError(parseState, "Could not parse primary expression");
	return 0;
}

Expr* ParseUnary(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_NOT) ||
		 Consume(parseState, TOKEN_MINUS) )
	{
		Token* op = Consumed(parseState);
		Expr* expr = ParseUnary(parseState, program);
		return AddExpression(program, op, expr);
	}
	else
	{
		return ParsePrimary(parseState, program);
	}
}

Expr* ParseFactor(ParseState &parseState, Program &program)
{
	Expr *expr = ParseUnary(parseState, program);

	while ( Consume(parseState, TOKEN_STAR) ||
			Consume(parseState, TOKEN_SLASH) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseUnary(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseTerm(ParseState &parseState, Program &program)
{
	Expr* expr = ParseFactor(parseState, program);

	while ( Consume(parseState, TOKEN_MINUS) ||
			Consume(parseState, TOKEN_PLUS) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseFactor(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseComparison(ParseState &parseState, Program &program)
{
	Expr* expr = ParseTerm(parseState, program);

	while ( Consume(parseState, TOKEN_GREATER) ||
			Consume(parseState, TOKEN_GREATER_EQUAL) ||
			Consume(parseState, TOKEN_LESS) ||
			Consume(parseState, TOKEN_LESS_EQUAL) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseTerm(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseEquality(ParseState &parseState, Program &program)
{
	Expr* expr = ParseComparison(parseState, program);

	if ( Consume(parseState, TOKEN_EQUAL_EQUAL) ||
		 Consume(parseState, TOKEN_NOT_EQUAL) )
	{
		Token* op = Consumed(parseState);
		Expr* right = ParseComparison(parseState, program);
		expr = AddExpression(program, expr, op, right);
	}

	return expr;
}

Expr* ParseExpression(ParseState &parseState, Program &program)
{
	return ParseEquality(parseState, program);
}

Stmt* ParseExpressionStatement(ParseState &parseState, Program &program)
{
	Expr *expr = ParseExpression(parseState, program);
	ConsumeForced(parseState, TOKEN_SEMICOLON);
	Stmt *stmt = AddPrintStatement(program, expr);
	return stmt;
}

Stmt* ParsePrintStatement(ParseState &parseState, Program &program)
{
	ConsumeForced(parseState, TOKEN_LEFT_PAREN);
	Expr *expr = ParseExpression(parseState, program);
	ConsumeForced(parseState, TOKEN_RIGHT_PAREN);
	ConsumeForced(parseState, TOKEN_SEMICOLON);
	Stmt *stmt = AddPrintStatement(program, expr);
	return stmt;
}

Stmt* ParseStatement(ParseState &parseState, Program &program)
{
	if ( Consume(parseState, TOKEN_PRINT) )
	{
		return ParsePrintStatement(parseState, program);
	}
	else
	{
		return ParseExpressionStatement(parseState, program);
	}
}

#define MAX_LEXEME_SIZE 128

#if 0
void PrintExpr(Expr* expr, u32 level = 0)
{
	for (u32 i = 0; i < level; ++i) printf("  ");
	u32 space = 16 - level*2;

	char lexeme[MAX_LEXEME_SIZE] = {};
	if (expr->type == EXPR_LITERAL)
	{
		StrCopy(lexeme, expr->literal.literalToken->lexeme);
		printf("%s%*s(%s)\n", lexeme, space, "", TokenNames[expr->literal.literalToken->type] );
	}
	else if (expr->type == EXPR_UNARY)
	{
		StrCopy(lexeme, expr->unary.operatorToken->lexeme);
		printf("%s%*s(%s)\n", lexeme, space, "", TokenNames[expr->unary.operatorToken->type] );
		PrintExpr(expr->unary.expr, level+1);
	}
	else if (expr->type == EXPR_BINARY)
	{
		StrCopy(lexeme, expr->binary.operatorToken->lexeme);
		printf("%s%*s(%s)\n", lexeme, space, "", TokenNames[expr->binary.operatorToken->type] );
		PrintExpr(expr->binary.left, level+1);
		PrintExpr(expr->binary.right, level+1);
	}
}
#endif

Program Parse(Arena &arena, ParseState &parseState, TokenList &tokens)
{
	Program program = {};
	program.arena = &arena;

	parseState.tokenList = &tokens;
	parseState.current = 0;
	parseState.hasErrors = false;

	while (!IsAtEnd(parseState) && !parseState.hasErrors)
	{
		ParseStatement(parseState, program);
		//program.exprCount = 0; // Reset AST expression list
		//Expr *expr = ParseExpression(parseState, program);
#if 0
		PrintExpr(expr);
#endif
	}

	return program;
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Evaluator

Value Evaluate(Expr *expr)
{
	Value value = {};

	switch (expr->type)
	{
		case EXPR_LITERAL:
		{
			value = expr->literal.literalToken->literal;
			break;
		}
		case EXPR_UNARY:
		{
			value = Evaluate( expr->unary.expr );
			switch ( expr->unary.operatorToken->type )
			{
				case TOKEN_MINUS:
					ASSERT( value.type == VALUE_TYPE_FLOAT );
					value.f = -value.f;
					break;
				case TOKEN_NOT:
					assert( value.type == VALUE_TYPE_BOOL );
					value.b = !value.b;
					break;
				default:
					INVALID_CODE_PATH();
			}
			break;
		}
		case EXPR_BINARY:
		{
			Value left = Evaluate( expr->binary.left );
			Value right = Evaluate( expr->binary.right );
			switch ( expr->binary.operatorToken->type )
			{
				case TOKEN_MINUS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f - right.f;
					break;
				case TOKEN_PLUS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f + right.f;
					break;
				case TOKEN_STAR:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f * right.f;
					break;
				case TOKEN_SLASH:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_FLOAT;
					value.f = left.f / right.f;
					break;
				case TOKEN_LESS:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f < right.f;
					break;
				case TOKEN_LESS_EQUAL:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f <= right.f;
					break;
				case TOKEN_GREATER:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f > right.f;
					break;
				case TOKEN_GREATER_EQUAL:
					ASSERT( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT );
					value.type = VALUE_TYPE_BOOL;
					value.b = left.f >= right.f;
					break;
				case TOKEN_NOT_EQUAL:
					value.type = VALUE_TYPE_BOOL;
					value.b = false;
					if ( left.type == VALUE_TYPE_BOOL && right.type == VALUE_TYPE_BOOL ) {
						value.b = left.b != right.b;
					} else if ( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT ) {
						value.b = left.f != right.f;
					}
					break;
				case TOKEN_EQUAL_EQUAL:
					value.type = VALUE_TYPE_BOOL;
					value.b = false;
					if ( left.type == VALUE_TYPE_BOOL && right.type == VALUE_TYPE_BOOL ) {
						value.b = left.b == right.b;
					} else if ( left.type == VALUE_TYPE_FLOAT && right.type == VALUE_TYPE_FLOAT ) {
						value.b = left.f == right.f;
					}
					break;
				default:
					INVALID_CODE_PATH();
			}
			break;
		}
		default:
			INVALID_CODE_PATH();
	}

	return value;
}

void Execute(Stmt &stmt)
{
	switch ( stmt.type )
	{
		case STMT_PRINT:
		case STMT_EXPR:
			{
				Value val = Evaluate(stmt.expr);

				printf("Evaluated value: ");
				switch ( val.type )
				{
					case VALUE_TYPE_FLOAT:
						printf("%f", val.f);
						break;
					case VALUE_TYPE_BOOL:
						printf("%s", val.b ? "true" : "false" );
						break;
					case VALUE_TYPE_STRING:
						char cstring[512];
						StrCopy(cstring, val.s);
						printf("%s", cstring);
						break;
					default:
						INVALID_CODE_PATH();
				}
				printf("\n");
			}
			break;
		default:
			INVALID_CODE_PATH();
	}
}

void Execute(Program &program)
{
	for (StmtList *list = program.statements; list; list = list->next)
	{
		for (u32 i = 0; i < list->stmtsCount; ++i)
		{
			Execute( list->stmts[i] );
		}
	}
}



////////////////////////////////////////////////////////////////////////////////////////////////////
// Program

#define COMMAND_NAME "jsl"

#if 0
void PrintTokenList(const TokenList &tokenList)
{
	printf("List of tokens:\n");
	char lexeme[MAX_LEXEME_SIZE] = {};
	for (u32 i = 0; i < tokenList.count; ++i)
	{
		const Token& token = tokenList.tokens[i];
		ASSERT(token.lexeme.size < MAX_LEXEME_SIZE);
		StrCopy(lexeme, token.lexeme);
		printf("%s\t: %s\n", TokenNames[token.type], lexeme);
	}
}
#endif

void Run(Arena &arena, const char *script, u32 scriptSize)
{
	ScanState scanState = {};
	TokenList tokenList = Scan(arena, scanState, script, scriptSize);

	if ( !scanState.hasErrors )
	{
#if 0
		PrintTokenList(tokenList);
#endif

		ParseState parseState = {};
		Program program = Parse(arena, parseState, tokenList);

		if ( !parseState.hasErrors )
		{
			Execute(program);
		}
	}
}

void RunFile(Arena &arena, const char* filename)
{
	u32 fileSize = GetFileSize(filename);
	char* bytes = PushArray(arena, char, fileSize);
	u32 bytesRead = ReadEntireFile(filename, bytes, fileSize);
	ASSERT(bytesRead == fileSize);
	Run(arena, bytes, bytesRead);
}

void RunPrompt(Arena &arena)
{
	char* line = NULL;
	size_t lineLen = 0; // used buffer size

	for (;;)
	{
		printf("> ");

		ssize_t lineSize; // number of characters, includes \n
		lineSize = getline(&line, &lineLen, stdin);

		line[--lineSize] = 0; // remove trailing \n

		if ( StrEq( "", line ) )
		{
		    continue;
		}
		else if ( StrEq( "exit", line ) ||
			 StrEq( "quit", line ) ||
			 StrEq( "q", line ) )
		{
			break;
		}
		else
		{
			ResetArena(arena);
			Run(arena, line, lineSize);
		}
	}

	free(line);
}

int main(int argc, char **argv)
{
	u32 globalArenaSize = MB(2);
	byte *globalArenaBase = (byte*)AllocateVirtualMemory(globalArenaSize);

	Arena globalArena = MakeArena(globalArenaBase, globalArenaSize);

	if ( argc > 2 )
	{
		printf("Usage: %s [script]\n", COMMAND_NAME);
		return -1;
	}
	else if ( argc == 2 )
	{
		RunFile(globalArena, argv[1]);
	}
	else
	{
		RunPrompt(globalArena);
	}

	PrintArenaUsage(globalArena);

	return 0;
}

