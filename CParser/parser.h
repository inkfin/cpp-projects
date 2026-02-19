#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <string.h>
#include <assert.h>
#include "tl_def.h"

typedef enum {
    EPARSE_SUCCESS,
    EPARSE_FAILURE,
    EPARSE_INVALID_TOKEN,
    EPARSE_CORRUPT_STATE,
} ParseResult;

typedef struct ParserState {
    struct Token   *tl;
    struct ASTNode *ast_root;
} ParserState;


/** Helpers **/

/**
 * Compare a string with a token string and check if they are the same.
 *
 * @param token: a null-terminated string
 */
static inline b32_t
token_is_same_str(const char *str, size_t str_len, const char *token) {
    size_t token_len = strlen(token);
    if (str_len != token_len) return 0;
    return strncmp(str, token, str_len) == 0;
}

/*** Token definitions ***/

typedef enum TokenType {
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_PUNCTUATION,
    TOKEN_TYPE_INVALID = 99,
} TokenType;

typedef enum TokenIdentifierType {
    TOKEN_ID_VAR,
    TOKEN_ID_NAME = 99,
} TokenIdentifierType;

typedef struct {
    const char *name;
    TokenIdentifierType id;
} TokenIdentifierInfo;

static inline const char *
token_identifier_to_str(const TokenIdentifierInfo *info) {
    switch (info->id) {
        case TOKEN_ID_VAR: return "var";
        case TOKEN_ID_NAME:
            assert(info->name != NULL);
            return info->name;
        default: return "<invalid identifier>";
    }
}

static inline TokenIdentifierInfo
token_identifier_from_str(const char *str, size_t n) {
    TokenIdentifierInfo info = {.name = NULL, .id = TOKEN_ID_NAME};
    if (token_is_same_str(str, n, "var")) info.id = TOKEN_ID_VAR;
    if (info.id == TOKEN_ID_NAME) {
        info.name = strndup(str, n);
        info.id = TOKEN_ID_NAME;
    }
    return info;
}

typedef enum {
    TOKEN_COMMA,
    TOKEN_DOT,
    TOKEN_L_PAREN,
    TOKEN_R_PAREN,
    TOKEN_L_BRACKET,
    TOKEN_R_BRACKET,
    TOKEN_L_BRACE,
    TOKEN_R_BRACE,
    TOKEN_SEMICOLON,
    TOKEN_NEWLINE,
    TOKEN_PUNCTUATION_INVALID = 99,
} TokenPunctuationType;

static inline const char *
token_punctuation_to_str(TokenPunctuationType punct) {
    switch (punct) {
        case TOKEN_COMMA: return ",";
        case TOKEN_DOT: return ".";
        case TOKEN_L_PAREN: return "(";
        case TOKEN_R_PAREN: return ")";
        case TOKEN_L_BRACKET: return "[";
        case TOKEN_R_BRACKET: return "]";
        case TOKEN_L_BRACE: return "{";
        case TOKEN_R_BRACE: return "}";
        case TOKEN_SEMICOLON: return ";";
        case TOKEN_NEWLINE: return "\\n";
        default: return "<invalid punctuation>";
    }
}

static inline TokenPunctuationType
token_punctuation_from_char(char c) {
    switch (c) {
        case ',': return TOKEN_COMMA;
        case '.': return TOKEN_DOT;
        case '(': return TOKEN_L_PAREN;
        case ')': return TOKEN_R_PAREN;
        case '[': return TOKEN_L_BRACKET;
        case ']': return TOKEN_R_BRACKET;
        case '{': return TOKEN_L_BRACE;
        case '}': return TOKEN_R_BRACE;
        case ';': return TOKEN_SEMICOLON;
        case '\n': return TOKEN_NEWLINE;
        default: return TOKEN_PUNCTUATION_INVALID;
    }
}

typedef enum {
    TOKEN_OP_PLUS,
    TOKEN_OP_MINUS,
    TOKEN_OP_MULTIPLY,
    TOKEN_OP_DIVIDE,
    TOKEN_OP_EQUALS,
    TOKEN_OP_ASSIGNMENT,
    TOKEN_OP_INVALID = 99,
} TokenOperatorType;

static inline const char *
token_operator_to_str(TokenOperatorType op) {
    switch (op) {
        case TOKEN_OP_PLUS: return "+";
        case TOKEN_OP_MINUS: return "-";
        case TOKEN_OP_MULTIPLY: return "*";
        case TOKEN_OP_DIVIDE: return "/";
        case TOKEN_OP_EQUALS: return "==";
        case TOKEN_OP_ASSIGNMENT: return "=";
        default: return "<invalid operator>";
    }
}

static inline TokenOperatorType
token_operator_from_str(const char *str, size_t n) {
    if (token_is_same_str(str, n, "+")) return TOKEN_OP_PLUS;
    if (token_is_same_str(str, n, "-")) return TOKEN_OP_MINUS;
    if (token_is_same_str(str, n, "*")) return TOKEN_OP_MULTIPLY;
    if (token_is_same_str(str, n, "/")) return TOKEN_OP_DIVIDE;
    if (token_is_same_str(str, n, "==")) return TOKEN_OP_EQUALS;
    if (token_is_same_str(str, n, "=")) return TOKEN_OP_ASSIGNMENT;
    return TOKEN_OP_INVALID;
}

typedef double TokenNumber;

typedef struct Token {
    TokenType type;
    size_t    beg;
    size_t    end;

    union TokenValue {
        TokenNumber          as_number;
        TokenOperatorType    as_operator;
        TokenIdentifierInfo  as_identifier;
        TokenPunctuationType as_punctuation;
    } val;
} Token;

/** AST definitions **/

typedef enum ASTNodeType {
    AST_NODE_TYPE_NUMBER,
    AST_NODE_TYPE_OPERATOR,
    AST_NODE_TYPE_IDENTIFIER,
    AST_NODE_TYPE_INVALID = 99,
} ASTNodeType;

typedef struct ASTNode {
    int node_type;

    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

/** Lexer functions **/

#define IS_NUMBER_TOKEN(c)     ((c) >= '0' && (c) <= '9')
#define IS_OPERATOR_TOKEN(c)   ((c) == '+' || (c) == '-' || (c) == '*' || (c) == '/' || (c) == '=')
#define IS_WHITESPACE(c)       ((c) == ' ' || (c) == '\t' || (c) == '\r')
#define IS_PUNCTUATION(c)                                                                                            \
    ((c) == ',' || (c) == '.' || (c) == '(' || (c) == ')' || (c) == '[' || (c) == ']' || (c) == '{' || (c) == '}' || \
     (c) == ';' || (c) == '\n')
#define IS_IDENTIFIER_TOKEN(c) ((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z') || (c) == '_'

/** Parser API **/

/**
 * Load the contents of a file into a buffer and prepare it for parsing.
 */
ParseResult
load_parse_file(const char *filename, char *buffer, size_t buffer_size, size_t *out_size);

/**
 * Tokenize the contents of a buffer and return a list of tokens.
 */
ParseResult
tokenize_buffer(ParserState *state, const char *buffer, size_t buffer_size);

/**
 * Parse the contents of a buffer and build an AST.
 */
ParseResult
parse_buffer(ParserState *state, const char *buffer, size_t buffer_size);


/** Debug **/

void
print_debug_token(const ParserState *state);

#endif  // PARSER_H
