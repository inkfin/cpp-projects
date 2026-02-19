#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <string.h>
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
    TOKEN_ID_ASSIGN,
    TOKEN_ID_INVALID = 99,
} TokenIdentifierType;

static inline const char *
token_identifier_to_str(TokenIdentifierType id) {
    switch (id) {
        case TOKEN_ID_VAR: return "var";
        case TOKEN_ID_ASSIGN: return "=";
        default: return "<invalid identifier>";
    }
}

static inline TokenIdentifierType
token_identifier_from_str(const char *str, size_t n) {
    if (token_is_same_str(str, n, "var")) return TOKEN_ID_VAR;
    if (token_is_same_str(str, n, "=")) return TOKEN_ID_ASSIGN;
    return TOKEN_ID_INVALID;
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
        TokenNumber         as_number;
        TokenOperatorType   as_operator;
        TokenIdentifierType as_identifier;
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
#define IS_SEPARATOR(c)        ((c) == ';' || (c) == '\n')
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
