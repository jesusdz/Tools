
enum TokenType
{
    // Single character tokens
    LEFT_PAREN,
    RIGHT_PAREN,
    LEFT_BRACE,
    RIGHT_BRACE,
    COMMA,
    DOT,
    MINUS,
    PLUS,
    SEMICOLON,
    SLASH,
    STAR,
    // One or two-character tokens
    NOT,
    NOT_EQUAL,
    EQUAL,
    EQUAL_EQUAL,
    GREATER,
    GREATER_EQUAL,
    LESS,
    LESS_EQUAL,
    AND,
    ANDAND,
    OR,
    OROR,
    // Literals
    IDENTIFIER,
    STRING,
    NUMBER,
    // Keywords
    IF,
    ELSE,
    FOR,
    WHILE,
    CLASS,
    SUPER,
    THIS,
    FUN,
    RETURN,
    TRUE,
    FALSE,
    NIL,
    VAR,
    PRINT,
};

struct Token
{
    TokenType type;
};

int main(int argc, char **argv)
{
    return 0;
}

