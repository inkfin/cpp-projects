#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#define TLDS_ABBR
#define TLDS_DEBUG
#define TLDS_DEBUG_PRINT
#include "tl_ds.h"
#include "tl_log.h"

ParseResult
load_parse_file(const char *filename,
                char *buffer,
                size_t buffer_size,
                size_t *out_size) {
    ParseResult result = EPARSE_SUCCESS;
    *out_size = 0;

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open file");
        return EPARSE_FAILURE;
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
        *out_size = total_read;
    }
    buffer[total_read] = '\0';  // Null-terminate the buffer

cleanup:
    fclose(file);
    return result;
}

ParseResult
tokenize_buffer(ParserState *state,
                const char *buffer,
                size_t buffer_size) {
    if (buffer_size == 0) {
        return EPARSE_FAILURE;
    }

    if (state->tl != NULL) arr_free(state->tl);
    if (!arr_init_reserve(state->tl, 100)) {
        LOG_ERROR("Failed to initialize token list\n");
        return EPARSE_FAILURE;
    }

    char token_buf[64];
    size_t beg = 0;
    size_t end = 0;
    TokenType token_type = TOKEN_TYPE_INVALID;
    for (size_t i = 0; i < buffer_size && buffer[i] != '\0'; ++i) {
        char c = buffer[i];
        if (IS_WHITESPACE(c) || IS_SEPARATOR(c)) {
            LOG_DEBUG("<break> at %zu", i);
            end = i;
            LOG_DEBUG("beg = %zu, end = %zu", beg, end);
            if (end > beg) {
                if (token_type == TOKEN_TYPE_IDENTIFIER) {
                    Token token = (Token){
                        .type = TOKEN_TYPE_IDENTIFIER,
                        .val = {.as_identifier = token_identifier_from_str(buffer + beg, end - beg)},
                        .beg = beg,
                        .end = end,
                    };

                    size_t token_len = sizeof(token_buf) < end - beg ? sizeof(token_buf) : end - beg;
                    stpncpy(token_buf, buffer + beg, token_len);
                    token_buf[token_len] = '\0';

                    LOG_DEBUG("%s<identifier>: '%s'", token_identifier_to_str(token.val.as_identifier), token_buf);
                    arr_pushp(state->tl, &token);
                } else if (token_type == TOKEN_TYPE_NUMBER) {
                    /* handle in tokenize stage */
                    assert(0);
                } else if (token_type == TOKEN_TYPE_OPERATOR) {
                    Token token = (Token){
                        .type = TOKEN_TYPE_OPERATOR,
                        .val = {.as_operator = token_operator_from_str(buffer + beg, end - beg)},
                        .beg = beg,
                        .end = end,
                    };

                    size_t token_len = sizeof(token_buf) < end - beg ? sizeof(token_buf) : end - beg;
                    stpncpy(token_buf, buffer + beg, token_len);
                    token_buf[token_len] = '\0';

                    LOG_DEBUG("%s<operator>: '%s'", token_operator_to_str(token.val.as_operator), token_buf);
                    arr_pushp(state->tl, &token);
                } else {
                    LOG_ERROR("%u<invalid token>: '%c' at %zu", c, c, i);
                    return EPARSE_INVALID_TOKEN;
                }
            }

            beg = i + 1;
            token_type = TOKEN_TYPE_INVALID;
        } else if (IS_IDENTIFIER_TOKEN(c) || token_type == TOKEN_TYPE_IDENTIFIER) {
            token_type = TOKEN_TYPE_IDENTIFIER;
            /* token begins with character must be an identifier, continue to break */
        } else if (IS_NUMBER_TOKEN(c)
                   || ((buffer[beg] == '-' || buffer[beg] == '+') && IS_NUMBER_TOKEN(buffer[beg + 1]))
        ) {
            token_type = TOKEN_TYPE_NUMBER;
            char  *endptr = NULL;
            double num = strtod(buffer + beg, &endptr);
            assert(endptr);
            end = (size_t)(endptr - buffer) / sizeof(byte_t);
            Token token = (Token){
                .type = TOKEN_TYPE_NUMBER,
                .val = {.as_number = num},
                .beg = beg,
                .end = end,
            };

            size_t token_len = sizeof(token_buf) > end - beg ? end - beg : sizeof(token_buf);
            stpncpy(token_buf, buffer + beg, token_len);
            token_buf[token_len] = '\0';

            LOG_DEBUG("%.2f<number>: '%s'", token.val.as_number, token_buf);
            arr_pushp(state->tl, &token);

            i = end - 1;  // skip the rest
            beg = end;
            token_type = TOKEN_TYPE_INVALID;
            continue;
        } else if (IS_OPERATOR_TOKEN(c)) {
            token_type = TOKEN_TYPE_OPERATOR;
            /* token begins with '-/+' will check the second token to see if its number */
        }
    }

    return EPARSE_SUCCESS;
}

ParseResult
parse_buffer(ParserState *state,
             const char *buffer,
             size_t buffer_size) {
    (void)state;
    (void)buffer;
    (void)buffer_size;
    return EPARSE_SUCCESS;
}

static inline void
print_one_debug_token(const Token *token) {
    if (token == NULL) {
        printf("<null token pointer>\n");
        return;
    }
    printf("[DEBUG] Token: type=%d, beg=%zu, end=%zu, val=", token->type, token->beg, token->end);
    switch (token->type) {
        case TOKEN_TYPE_NUMBER:
            printf("%f", token->val.as_number);
            break;
        case TOKEN_TYPE_OPERATOR:
            printf("%s", token_operator_to_str(token->val.as_operator));
            break;
        default:
            printf("<invalid token type>");
            break;
    }
    printf("\n");
}

void
print_debug_token(const ParserState *state) {
    for (size_t i = 0; i < ARR_SIZE(state->tl); ++i) {
        print_one_debug_token(&state->tl[i]);
    }
}

