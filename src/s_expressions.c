#include <stdio.h>
#include <stdlib.h>
#include <editline/readline.h>

#include "mpc.h"

typedef enum {
    CoolValueError_DivideByZero,
    CoolValueError_BadOperator,
    CoolValueError_BadNumber
} CoolValueError;

typedef enum {
    CoolValue_Number,
    CoolValue_Symbol,

    CoolValue_Error
} CoolValue;

typedef struct {
    int type;
    long number;
    char *errorString;
    char *symbolString;
    int count;
    struct cval** cell; // cons cells for this s-expression
    int error;
} cval;

cval* cval_number(long x) {
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Number;
    value->number = x;
    return value;
}

cval* cval_error(int x) {
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Error;
    
    value->number = x;
    return value;
}

void cval_printError(CoolValueError errorType) {
    switch (errorType) {
        case CoolValueError_BadNumber:
            printf("Error: Invalid number.");
            break;
        case CoolValueError_BadOperator:
            printf("Error: Invalid operator.");
            break;
        case CoolValueError_DivideByZero:
            printf("Error: Division by zero is not allowed.");
            break;
    }
}

void cval_print(cval value) {
    switch (value.type) {
        case CoolValue_Number: 
            printf("%li", value.number);
            break;
        case CoolValue_Error:
            cval_printError((CoolValueError) value.number);
            break;
    }  
}

void cval_println(cval value) {
    cval_print(value);
    putchar('\n');
}

cval 
eval_op(cval x, char* op, cval y) {

    if (x.type == CoolValue_Error) {
        return x;
    }
    if (y.type == CoolValue_Error) {
        return y;
    }

    if (strcmp(op, "+") == 0) { 
        return cval_number(x.number + y.number); 
    } 
    if (strcmp(op, "-") == 0) { 
        return cval_number(x.number - y.number); 
    }
    if (strcmp(op, "*") == 0) { 
        return cval_number(x.number * y.number); 
    }
    if (strcmp(op, "/") == 0) { 
        return y.number == 0 ? 
            cval_error(CoolValueError_DivideByZero) : 
            cval_number(x.number / y.number); 
    }

    return cval_error(CoolValueError_BadOperator);
}

cval 
eval(mpc_ast_t* t) 
{
    if (strstr(t->tag, "number")) {
        // TODO: handle bad numbers here
        return cval_number(atoi(t->contents));
    }

    char* op = t->children[1]->contents;

    cval x = eval(t->children[2]);

    int i = 3;
    while (strstr(t->children[i]->tag, "expr")) {
        x = eval_op(x, op, eval(t->children[i]));
        i++;
    }

    return x;  
}

int main(int argc, char** argv) {
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Cool = mpc_new("cool");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                            \
            number : /-?[0-9]+/ ;                    \
            symbol : '+' | '-' | '*' | '/' ;         \
            sexpr  : '(' <expr>* ')' ;               \
            expr   : <number> | <symbol> | <sexpr> ; \
            cool   : /^/ <expr>* /$/ ;               \
        ",
        Number, Operator, Expr, Cool);

    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");

    for (;;) {
        char* input = readline("COOL> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Cool, &r)) {
            cval result = eval(r.output);
            cval_println(result);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    
    mpc_cleanup(5, Number, Symbol, Sexpr, Expr, Cool);

    return 0;
}
