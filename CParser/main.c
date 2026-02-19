#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#define TL_LOG_IMPLEMENTATION
#include "tl_log.h"

#define MAXIMUM_BUFFER_SIZE (1u << 20)  // 1 MB

int main(int argc, char* argv[]) {
    int return_code = EXIT_SUCCESS;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <source-file>\n", argv[0]);
        return 1;
    }

    if (!tl_log_init(&(TL_LogConfig){
            .level = TL_LOG_LEVEL_DEBUG,
        })) {
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
        return 1;
    }

    ParserState parser_state = {0};

    if (tokenize_buffer(&parser_state, buffer, buffer_size) != EPARSE_SUCCESS) {
        fprintf(stderr, "Parsing failed for file: %s\n", source_file);
        return_code = EXIT_FAILURE;
        goto cleanup;
    }

    print_debug_token(&parser_state);

cleanup:
    free(buffer);
    tl_log_deinit();
    return return_code;
}
