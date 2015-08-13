#include <editline/readline.h>
#include <stdlib.h>
#include "mpc.h"
#include "cool.h"

int 
main(int argc, char** argv) 
{
    Number = mpc_new("number");
    Symbol = mpc_new("symbol");
    String = mpc_new("string");
    Comment = mpc_new("comment");
    Sexpr = mpc_new("sexpr");
    Qexpr = mpc_new("qexpr");
    Expr = mpc_new("expr");
    Cool = mpc_new("cool");

    mpca_lang(MPCA_LANG_DEFAULT,
        "   \
            number  : /[+-]?[0-9]+[.]?[0-9]*/ ; \
            symbol  : /[a-zA-Z_+\\-*\\/\\\\=<>!&][a-zA-Z0-9_+\\-*\\/\\\\=<>!&]*/ ; \
            string  : /\"(\\\\.|[^\"])*\"/ ;                    \
            comment : /;[^\\r\\n]*/ ;                           \
            sexpr   : '(' <expr>* ')' ;                         \
            qexpr   : '{' <expr>* '}' ;                         \
            expr    : <number> | <symbol> | <sexpr> | <qexpr>   \
                    | <string> | <comment>  ;          \
            cool    : /^/ <expr>* /$/ ;                         \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Cool);
    
    // Old number regex
    // number  : /-?[0-9]+/ ;
    // ^[-+]?[0-9]+\.[0-9]+$
    // float   : /<number>('.'[0-9]+)?/ ; 

    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");

    // Setup the environment
    Environment *env = Environment_new();
    Environment_addBuiltinFunctions(env);

    // Handle command line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            Value *args = Value_add(Value_sexpr(), Value_string(argv[i]));
            Value *x = builtin_load(env, args);
            if (Value_getType(x) == CoolValue_Error) {
                Value_println(x);
            }
            Value_delete(x);
        }
    }

    for (;;) {
        char* input = readline("COOL> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Cool, &r)) {
            Value *input = Value_read(r.output);
            Value_println(input);
            Value *x = Value_eval(env, input);
            Value_println(x);
            Value_delete(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    
    Environment_delete(env);

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Cool);

    return 0;
}
