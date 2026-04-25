#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#define TL_LOG_IMPLEMENTATION
#include "tinylib/logging.h"

#define MAXIMUM_BUFFER_SIZE (1u << 20)  // 1 MB

static const TL_LogConfig g_log_cfg = {
    .level = TL_LOG_LEVEL_INFO,
    .output = TL_LOG_OUTPUT_STDOUT,
    .error_output = TL_LOG_OUTPUT_STDERR,
    .filename = NULL,
    .append = true,
    .auto_flush = true,
    .show_time = true,
    .show_level = true,
    .show_file = true,
    .show_line = true,
    .show_func = true,
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    if (!tl_log_init(&g_log_cfg)) {
        return EXIT_FAILURE;
    }

    // global buffer allocation
    size_t buffer_size = MAXIMUM_BUFFER_SIZE;
    char  *buffer = (char *)malloc(buffer_size);

    const char* source_file = argv[1];
    size_t file_len = 0;
    if (load_parse_file(source_file, buffer, buffer_size, &file_len) !=
        EPARSE_SUCCESS) {
        fprintf(stderr, "Failed to load file: %s\n", source_file);
        return EXIT_FAILURE;
    }

    int return_code = EXIT_SUCCESS;
    ParserState parser_state = {0};

    if (tokenize_buffer(&parser_state, buffer, buffer_size) != EPARSE_SUCCESS) {
        fprintf(stderr, "Parsing failed for file: %s\n", source_file);
        return_code = EXIT_FAILURE;
        goto cleanup;
    }

    print_debug_token(&parser_state);

cleanup:
    free_parser_state(&parser_state);
    free(buffer);
    tl_log_shutdown();
    return return_code;
}
