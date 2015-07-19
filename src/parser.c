#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Operator = mpc_new("operator");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Cool = mpc_new("cool");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                     \
        number   : /-?[0-9]+/ ;                             \
        operator : '+' | '-' | '*' | '/' ;                  \
        expr     : <number> | '(' <operator> <expr>+ ')' ;  \
        cool    : /^/ <operator> <expr>+ /$/ ;             \
        ",
        Number, Operator, Expr, Cool);

    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");

    for (;;) {
        char* input = readline("COOL> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Cool, &r)) {
            mpc_ast_print(r.output);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    
    mpc_cleanup(4, Number, Operator, Expr, Cool);

    return 0;
}
