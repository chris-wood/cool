// TODO: only expose the headers necessary
#include "cool.c"

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
    cenv *env = cenv_new();
    cenv_addBuiltinFunctions(env);

    // Handle command line arguments
    if (argc > 1) {
        for (int i = 1; i < argc; i++) {
            cval *args = cval_add(cval_sexpr(), cval_string(argv[i]));
            cval *x = builtin_load(env, args);
            if (x->type == CoolValue_Error) {
                cval_println(x);
            }
            cval_delete(x);
        }
    }

    for (;;) {
        char* input = readline("COOL> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Cool, &r)) {
            cval *input = cval_read(r.output);
            cval_println(input);
            cval *x = cval_eval(env, input);
            cval_println(x);
            cval_delete(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    
    cenv_delete(env);

    mpc_cleanup(8, Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Cool);

    return 0;
}
