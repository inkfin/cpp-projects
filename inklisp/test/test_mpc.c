#include <string.h>
#include "../mpc.h"

#define KILOBYTE (1024UL)

size_t read_next_line(char* line, size_t linecap, FILE* fp) {
    size_t nread = 0;
    while (fgets(line, linecap, fp)) {
        nread += strlen(line);
        if (line[strlen(line) - 1] == '\n') {
            return nread;
        }
    }
    return nread;
}

int main(int argc, char** argv) {
    /* Create Some Parsers */
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Lispy = mpc_new("lispy");

    /* Define them with the following Language */
    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                       \
            number   : /-?[0-9]+/ ;                             \
            operator : '+' | '-' | '*' | '/' ;                  \
            expr     : <number> | '(' <operator> <expr>+ ')' ;  \
            lispy    : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Number, Operator, Expr, Lispy);

    char* buf = calloc(1, 64 * KILOBYTE);
    FILE *fp = fopen("a.test", "r");
    if (!fp) {
        perror("fopen");
        goto cleanup;
    }
    size_t filelen;
    if ((filelen = fread(buf, 1, sizeof buf, fp)) == 0) {
        fprintf(stderr, "input file is empty");
        fclose(fp);
        goto cleanup;
    }
    fclose(fp);
    const char* input = buf;

    mpc_result_t r;

    if (mpc_parse("a.test", input, Lispy, &r)) {
        mpc_ast_print(r.output);
        mpc_ast_delete(r.output);
    } else {
        mpc_err_print(r.error);
        mpc_err_delete(r.error);
    }

cleanup: /* cleanup correctly before exit */
    free(buf);
    mpc_cleanup(4, Number, Operator, Expr, Lispy);

    return 0;
}
