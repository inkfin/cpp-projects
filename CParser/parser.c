#include "parser.h"
#include "tl_log.h"
#include <stdio.h>

ParseResult
load_parse_file(const char *filename,
                byte_t *buffer,
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
                const byte_t *buffer,
                size_t buffer_size) {
    if (buffer_size == 0) {
        return EPARSE_FAILURE;
    }

    if (state->tl != NULL) { free(state->tl); }
    ARR_INIT(state->tl);

    for(size_t i = 0; i < buffer_size && buffer[i] != '\0'; ++i) {
        char c = buffer[i];
        if (IS_NUMBER_TOKEN(c)) {
            LOG_INFO("Found number: %c\n", c);
            /* ARR_PUSH(state->tl, c); */
        } else if (IS_OPERATOR_TOKEN(c)) {
            LOG_INFO("Found operator: %c\n", c);
        } else if (IS_EOL(c)) {
            LOG_INFO("Found end of line\n");
        } else if (IS_WHITESPACE(c)) {
            continue;
        } else {
            LOG_ERROR("Unexpected character <%u>'%c' at position %zu\n", c, c, i);
            return EPARSE_INVALID_TOKEN;
        }
    }

    return EPARSE_SUCCESS;
}

ParseResult
parse_buffer(ParserState *state,
             const byte_t* buffer,
             size_t buffer_size) {
    return EPARSE_SUCCESS;
}

