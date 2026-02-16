#ifndef PARSER_H
#define PARSER_H

#include <stdint.h>
#include <stdlib.h>
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

/*** Token definitions ***/

typedef enum {
    TOKEN_TYPE_EOF,
    TOKEN_TYPE_IDENTIFIER,
    TOKEN_TYPE_NUMBER,
    TOKEN_TYPE_OPERATOR,
    TOKEN_TYPE_PUNCTUATION,
} TokenType;

typedef enum {
    TOKEN_OP_PLUS,
    TOKEN_OP_MINUS,
    TOKEN_OP_MULTIPLY,
    TOKEN_OP_DIVIDE,
    TOKEN_OP_ASSIGNMENT,
} TokenOperatorType;

typedef float TokenNumber;

typedef struct Token {
    TokenType type;
    size_t    position;

    union {
        TokenNumber       as_number;
        TokenOperatorType as_operator;
    } val;
} Token;

static inline TokenNumber
token_number(const Token *token) {
    return token->val.as_number;
}

static inline TokenOperatorType
token_operator(const Token *token) {
    return token->val.as_operator;
}

/** AST definitions **/

typedef enum ASTNodeType {
    AST_NODE_TYPE_NUMBER,
    AST_NODE_TYPE_OPERATOR,
    AST_NODE_TYPE_IDENTIFIER,
} ASTNodeType;

typedef struct ASTNode {
    int node_type;

    struct ASTNode *left;
    struct ASTNode *right;
} ASTNode;

/** Lexer functions **/

#define IS_NUMBER_TOKEN(c) ((c) >= '0' && (c) <= '9')
#define IS_OPERATOR_TOKEN(c) ((c) == '+' || (c) == '-' || (c) == '*' || (c) == '/' || (c) == '=')
#define IS_WHITESPACE(c) ((c) == ' ' || (c) == '\r' || (c) == '\t')
#define IS_EOL(c) ((c) == '\n' || (c) == '\0')

/** Parser API **/

/**
 * Load the contents of a file into a buffer and prepare it for parsing.
 */
ParseResult
load_parse_file(const char *filename, byte_t *buffer, size_t buffer_size, size_t *out_size);

/**
 * Tokenize the contents of a buffer and return a list of tokens.
 */
ParseResult
tokenize_buffer(ParserState *state, const byte_t *buffer, size_t buffer_size);

/**
 * Parse the contents of a buffer and build an AST.
 */
ParseResult
parse_buffer(ParserState *state, const byte_t *buffer, size_t buffer_size);

#endif  // PARSER_H
