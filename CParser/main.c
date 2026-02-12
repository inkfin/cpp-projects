#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#define MAXIMUM_BUFFER_SIZE (1u << 20)  // 1 MB

typedef enum ParseResult {
    EPARSE_SUCCESS,
    EPARSE_FAILURE,
    EPARSE_EOF,
} ParseResult;

ParseResult load_parse_file(const char *filename, char *buffer, size_t buffer_size) {
    ParseResult result = EPARSE_SUCCESS;

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    size_t total_read = 0;
    while (total_read < buffer_size - 1) {
        size_t bytes_read = fread(buffer + total_read, 1, buffer_size - 1 - total_read, file);
        if (bytes_read == 0) {
            if (feof(file)) {
                break;
            } else {
                perror("Error reading file");
                result = EPARSE_FAILURE;
                goto cleanup;
            }
        }
        total_read += bytes_read;
    }
    buffer[total_read] = '\0';  // Null-terminate the buffer

cleanup:
    fclose(file);
    return result;
}


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
} TokenOperatorType;

typedef float TokenNumber;

typedef struct {
    TokenType type;
    size_t position;

    union {
        TokenNumber number;
        TokenOperatorType operator;
    } as;
} Token;

static inline TokenNumber token_number (const Token *token) {
    return token->as.number;
}

static inline TokenOperatorType token_operator (const Token *token) {
    return token->as.operator;
}


typedef struct ASTNode ASTNode;
struct ASTNode {
    int node_type;
    ASTNode *left;
    ASTNode *right;
};

ParseResult parse_file(ASTNode *root, const char *buffer, size_t buffer_size) {
    if (buffer_size == 0) {
        return EPARSE_FAILURE;
    }

    return EPARSE_SUCCESS;
}


int main(int argc, char *argv[]) {
    int return_code = EXIT_SUCCESS;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    // global buffer allocation
    size_t buffer_size = MAXIMUM_BUFFER_SIZE;
    char *buffer = (char *)malloc(buffer_size);

    const char *source_file = argv[1];
    if (load_parse_file(source_file, buffer, buffer_size) != EPARSE_SUCCESS) {
        fprintf(stderr, "Failed to load file: %s\n", source_file);
        return 1;
    }

    ASTNode root = {0};

    if (parse_file(&root, buffer, buffer_size) != EPARSE_SUCCESS) {
        fprintf(stderr, "Parsing failed for file: %s\n", source_file);
        free(buffer);
        return_code = EXIT_FAILURE;
        goto cleanup;
    }

cleanup:
    free(buffer);
    return return_code;
}
