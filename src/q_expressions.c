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
    CoolValue_Sexpr,
    CoolValue_Qexpr,
    CoolValue_Error
} CoolValue;

typedef struct cval {
    int type;
    long number;
    char *errorString;
    char *symbolString;
    int count;
    struct cval ** cell; // cons cells for this s-expression
    int error;
} cval;

#define CASSERT(args, cond, err) \
    if (!(cond)) { lval_del(args); return lval_err(err); }

void cval_print(cval *value);
cval * cval_eval(cval *value);

cval *
cval_number(long x) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Number;
    value->number = x;
    return value;
}

cval *
cval_symbol(char *symbol) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Symbol;
    value->symbolString = (char *) malloc((strlen(symbol) + 1) * sizeof(char));
    strcpy(value->symbolString, symbol);
    return value;
}

cval * 
cval_sexpr() 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Sexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cval * 
cval_qexpr() 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Qexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

cval * 
cval_error(char *message) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Error;
    value->errorString = (char *) malloc((strlen(message) + 1) * sizeof(char));
    strcpy(value->errorString, message);
    return value;
}

void 
cval_delete(cval *value)
{
    switch (value->type) {
        case CoolValue_Number:
            break;
        case CoolValue_Error:
            free(value->errorString);
            break;
        case CoolValue_Symbol:
            free(value->symbolString);
            break;
        case CoolValue_Sexpr:
        case CoolValue_Qexpr:
            for (int i = 0; i < value->count; i++) {
                cval_delete(value->cell[i]);
            }
            free(value->cell);
            break;
    }

    free(value);
}

void 
cval_printError(CoolValueError errorType) 
{
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

void
cval_printExpr(cval *value, char open, char close)
{
    putchar(open);
    for (int i = 0; i < value->count; i++) {
        cval_print(value->cell[i]);
        if (i != (value->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void 
cval_print(cval *value) 
{
    switch (value->type) {
        case CoolValue_Number: 
            printf("%li", value->number);
            break;
        case CoolValue_Error:
            cval_printError((CoolValueError) value->number);
            break;
        case CoolValue_Symbol:
            printf("%s", value->symbolString);
            break;
        case CoolValue_Sexpr:
            cval_printExpr(value, '(', ')');
            break;
        case CoolValue_Qexpr:
            cval_printExpr(value, '{', '}');
            break;
    }  
}

void 
cval_println(cval *value) 
{
    cval_print(value);
    putchar('\n');
}

cval *
cval_add(cval *value, cval *x) 
{
    value->count++;
    value->cell = realloc(value->cell, sizeof(cval *) * value->count);
    value->cell[value->count - 1] = x;
    return value;
}

cval * 
cval_read_num(mpc_ast_t* t) {
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? cval_number(x) : cval_error("invalid number");
}

cval *
cval_read(mpc_ast_t *t) 
{
    if (strstr(t->tag, "number")) {
        return cval_read_num(t);
    }
    if (strstr(t->tag, "symbol")) {
        return cval_symbol(t->contents);
    }

    cval *x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = cval_sexpr();
    }
    if (strstr(t->tag, "sexpr")) {
        x = cval_sexpr();
    }
    if (strstr(t->tag, "qexpr")) {
        x = cval_qexpr();
    }

    for (int i = 0; i < t->children_num; i++) {
        if (strcmp(t->children[i]->contents, "(") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, ")") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "}") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->contents, "{") == 0) {
            continue;
        }
        if (strcmp(t->children[i]->tag, "regex") == 0) {
            continue;
        }
        x = cval_add(x, cval_read(t->children[i]));
    }

    return x;
}

cval *
cval_pop(cval *value, int i)
{
    cval *x = value->cell[i];
    memmove(&value->cell[i], &value->cell[i + 1], sizeof(cval *) * (value->count - 1));
    value->count--;
    value->cell = realloc(value->cell, sizeof(cval *) * value->count);
    return x;
}

cval *
cval_take(cval *value, int i) 
{
    cval *x = cval_pop(value, i);
    cval_delete(value);
    return x;
}

cval *
cval_join(cval *x, cval *y)
{
    while (y->count > 0) {
        x = cval_add(x, cval_pop(y, 0));
    }
    cval_delete(y);
    return x;
}

cval *
builtin_head(cval *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments");
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'head' passed incorrect type");
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}");

    cval *head = cval_take(x, 0);
    while (head->count > 1) {
        cval_delete(cval_pop(head, 1));
    }

    return head;
}

cval *
builtin_tail(cval *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments");
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'tail' passed incorrect type");
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}");    

    cval *head = cval_take(x, 0);
    cval_delete(cval_pop(head, 0));
    cval *tail = head;

    return tail;
}

cval *
builtin_list(cval *x)
{
    x->type = CoolValue_Qexpr;
    return x;
}

cval *
builtin_eval(cval *x)
{
    CASSERT(x, x->count == 1, "Function 'eval' passed too many arguments");
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'eval' passed incorrect type");

    // pop the head and evaluate it
    cval *head = cval_take(head, 0);
    head->type = CoolValue_Sexpr;
    cval *evalResult = cval_eval(head);

    return evalResult;
}

cval *
builtin_join(cval *x)
{
    for (int i = 0; i < x->count; i++) {
        CASSERT(x, x->cell[i]->type == CoolValue_Qexpr, "Function 'join' passed incorrect type");
    }

    cval *y = cval_pop(x, 0);
    while (y->count > 0) {
        y = cval_join(y, cval_pop(x, 0));
    }

    cval_delete(x);

    return y;
}

cval *
builtin_op(cval *expr, char* op) 
{
    for (int i = 0; i < expr->count; i++) {
        if (expr->cell[i]->type != CoolValue_Number) {
            cval_delete(expr);
            return cval_error("Cannot operate on a non-number");
        }
    }

    cval *x = cval_pop(expr, 0);

    if ((strcmp(op, "-") == 0) && expr->count == 0) {
        x->number = -x->number;
    }

    while (expr->count > 0) {
        cval *y = cval_pop(expr, 0);
        if (strcmp(op, "+") == 0) { 
            x->number += y->number;
        } 
        if (strcmp(op, "-") == 0) { 
            x->number -= y->number; 
        }
        if (strcmp(op, "*") == 0) { 
            x->number *= y->number; 
        }
        if (strcmp(op, "/") == 0) { 
            if (y->number == 0) {
                cval_delete(x);
                cval_delete(y);
                x = cval_error("Division by zero.");
                break;
            }
            x->number /= y->number;
        }
        cval_delete(y);
    }

    cval_delete(expr);
    return x;
}

cval * 
builtin(cval* a, char* func) 
{
    if (strcmp("list", func) == 0) { 
        return builtin_list(a); 
    }
    if (strcmp("head", func) == 0) { 
        return builtin_head(a); 
    }
    if (strcmp("tail", func) == 0) { 
        return builtin_tail(a); 
    }
    if (strcmp("join", func) == 0) { 
        return builtin_join(a); 
    }
    if (strcmp("eval", func) == 0) { 
        return builtin_eval(a); 
    }
    if (strstr("+-/*", func)) { 
        return builtin_op(a, func); 
    }

    lval_del(a);
    return lval_err("Unknown function");
}

cval *
cval_evaluateExpression(cval *value)
{
    // Evaluate each expression
    for (int i = 0; i < value->count; i++) {
        value->cell[i] = cval_eval(value->cell[i]);
    }

    // If one is an error, take it and return it
    for (int i = 0; i < value->count; i++) {
        if (value->cell[i]->type == CoolValue_Error) {
            return cval_take(value, i);
        }
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return cval_take(value, 0);
    }

    cval *first = cval_pop(value, 0);
    if (first->type != CoolValue_Symbol) {
        cval_delete(first);
        cval_delete(value);
        return cval_error("S-expression does not start with a symbol");
    }

    cval *result = builtin(value, first->symbolString);
    cval_delete(first);
    return result;
}

cval *
cval_eval(cval *value) 
{
    if (value->type == CoolValue_Sexpr) {
        return cval_evaluateExpression(value);
    }
    return value;
}

int 
main(int argc, char** argv) 
{
    mpc_parser_t* Number = mpc_new("number");
    mpc_parser_t* Symbol = mpc_new("symbol");
    mpc_parser_t* Sexpr = mpc_new("sexpr");
    mpc_parser_t* Qexpr = mpc_new("qexpr");
    mpc_parser_t* Expr = mpc_new("expr");
    mpc_parser_t* Cool = mpc_new("cool");

    mpca_lang(MPCA_LANG_DEFAULT,
        "                                                      \
            number : /-?[0-9]+/ ;                              \
            symbol : \"list\" | \"head\" | \"tail\"            \
                   | \"join\" | \"eval\" | '+' | '-'           \
                   | '*' | '/' ;                               \
            sexpr  : '(' <expr>* ')' ;                         \
            qexpr  : '{' <expr>* '}' ;                         \
            expr   : <number> | <symbol> | <sexpr> | <qexpr> ; \
            cool   : /^/ <expr>* /$/ ;                         \
        ",
        Number, Symbol, Sexpr, Qexpr, Expr, Cool);

    printf("COOL version 0.0.0.1\n");
    printf("Press ctrl+c to exit\n");

    for (;;) {
        char* input = readline("COOL> ");
        add_history(input);

        mpc_result_t r;
        if (mpc_parse("<stdin>", input, Cool, &r)) {
            cval * x = cval_eval(cval_read(r.output));
            cval_println(x);
            cval_delete(x);
            mpc_ast_delete(r.output);
        } else {
            mpc_err_print(r.error);
            mpc_err_delete(r.error);
        }

        free(input);
    }
    
    mpc_cleanup(6, Number, Symbol, Sexpr, Qexpr, Expr, Cool);

    return 0;
}
