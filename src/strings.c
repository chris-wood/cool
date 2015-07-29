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
    CoolValue_String,
    CoolValue_Symbol,
    CoolValue_Sexpr,
    CoolValue_Qexpr,
    CoolValue_Function,
    CoolValue_Error
} CoolValue;

struct cenv;
struct cval;
typedef struct cenv cenv;
typedef struct cval cval;

// builtin function
typedef cval *(*cbuiltin)(cenv *, cval *);

struct cval {
    int type;
    long number;
    char *errorString;
    char *symbolString;
    char *string;

    cbuiltin builtin;
    cenv *env;
    cval *formals;
    cval *body;

    int count;
    struct cval ** cell; 
    int error;
};

struct cenv {
    cenv *parent;
    int count;
    char **symbols;
    struct cval **values;
}; 

// Forward declaration prototypes
void cval_print(cval *value);
cval *cval_eval(cenv *env, cval *value);
void cval_delete(cval *value);
cval *cval_error(char *fmt, ...);
cval *cval_copy(cval *in);
char *cval_typeString(int type);
cval *builtin_eval(cenv *env, cval *x);
cval *builtin_list(cenv *env, cval *x);

// Parsing grammars
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Cool;

#define CASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        cval *error = cval_error(fmt, ##__VA_ARGS__); \
        cval_delete(args); \
        return error; \
    }

#define CASSERT_TYPE(func, args, index, expect) \
    CASSERT(args, args->cell[index]->type == expect, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, cval_typeString(args->cell[index]->type), cval_typeString(expect));

#define CASSERT_NUM(func, args, num) \
    CASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num);

#define CASSERT_NOT_EMPTY(func, args, index) \
    CASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

cenv *
cenv_new() 
{
    cenv *env = (cenv *) malloc(sizeof(cenv));
    env->parent = NULL;
    env->count = 0;
    env->symbols = (char **) malloc(sizeof(char *));
    env->values = (cval **) malloc(sizeof(cval *));
    return env;
}

void
cenv_delete(cenv *env) 
{
    for (int i = 0; i < env->count; i++) {
        free(env->symbols[i]);
        cval_delete(env->values[i]);
    }
    free(env->symbols);
    free(env->values);
    free(env);
}

cval *
cenv_get(cenv *env, cval *val) 
{
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], val->symbolString) == 0) {
            return cval_copy(env->values[i]);
        }
    }

    if (env->parent) {
        return cenv_get(env->parent, val);
    } else {
        return cval_error("Undefined symbol: %s", val->symbolString);
    }
}

void 
cenv_put(cenv *env, cval* key, cval *val) 
{
    // Replace the value if it exists
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], key->symbolString) == 0) {
            cval_delete(env->values[i]);
            env->values[i] = cval_copy(val);
            return;
        }
    }

    env->count++;
    env->values = realloc(env->values, sizeof(cval *) * env->count);
    env->symbols = realloc(env->symbols, sizeof(char *) * env->count);

    env->values[env->count - 1] = cval_copy(val);
    env->symbols[env->count - 1] = malloc(strlen(key->symbolString) + 1);
    strcpy(env->symbols[env->count - 1], key->symbolString);
}

cenv *
cenv_copy(cenv *env)
{
    cenv *copy = (cenv *) malloc(sizeof(cenv));
    copy->parent = env->parent;
    copy->count = env->count;
    copy->symbols = (char **) malloc(sizeof(char *) * env->count);
    copy->values = (cval **) malloc(sizeof(cval *) * env->count);
    for (int i = 0; i < env->count; i++) {
        copy->symbols[i] = (char *) malloc(strlen(env->symbols[i]) + 1);
        strcpy(copy->symbols[i], env->symbols[i]);
        copy->values[i] = cval_copy(env->values[i]);
    }
    return copy;
}

void
cenv_def(cenv *env, cval *key, cval *value)
{
    while (env->parent != NULL) {
        env = env->parent;
    }
    cenv_put(env, key, value);
}

cval *
cval_number(long x) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Number;
    value->number = x;
    return value;
}

cval *
cval_string(char *str)
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_String;
    value->string = (char *) malloc((strlen(str) + 1) * sizeof(char));
    strcpy(value->string, str);
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
cval_builtin(cbuiltin function) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Function;
    value->builtin = function;
    return value;
}

cval *
cval_lambda(cval *formals, cval *body)
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Function;
    value->builtin = NULL;
    value->env = cenv_new();
    value->formals = formals;
    value->body = body;
    return value;
}

char *
cval_typeString(int type)
{
    switch (type) {
        case CoolValue_Error: 
            return "CoolValue_Error";
        case CoolValue_Function:
            return "CoolValue_Function";
        case CoolValue_Qexpr:
            return "CoolValue_Qexpr";
        case CoolValue_Sexpr:
            return "CoolValue_Sexpr";
        case CoolValue_Number:
            return "CoolValue_Number";
        case CoolValue_String:
            return "CoolValue_String";
        case CoolValue_Symbol:
        default:
            return "CoolValue_Symbol";
    }
}

cval * 
cval_error(char *fmt, ...) 
{
    cval *value = (cval *) malloc(sizeof(cval));
    value->type = CoolValue_Error;

    va_list va;
    va_start(va, fmt);

    value->errorString = (char *) malloc(512); // arbitrary for all error strings (more than needed)
    vsnprintf(value->errorString, 511, fmt, va);

    value->errorString = (char *) realloc(value->errorString, strlen(value->errorString) + 1);
    va_end(va);

    return value;
}

void 
cval_delete(cval *value)
{
    switch (value->type) {
        case CoolValue_Number:
            break;
        case CoolValue_String:
            free(value->string);
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
        case CoolValue_Function:
            if (value->builtin == NULL) {
                cenv_delete(value->env);
                cval_delete(value->formals);
                cval_delete(value->body);
            }
            break;
    }

    free(value);
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
        case CoolValue_String: 
            printf("'%s'", value->string);
            break;
        case CoolValue_Error:
            printf("Error: %s", value->errorString); 
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
        case CoolValue_Function:
            if (value->builtin != NULL) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                cval_print(value->formals);
                printf(" ");
                cval_print(value->body);
                printf(" ");
            }
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
cval_copy(cval *in)
{
    cval *copy = (cval *) malloc(sizeof(cval));
    copy->type = in->type;

    switch (in->type) {
        case CoolValue_Function:
            if (in->builtin != NULL) {
                copy->builtin = in->builtin;
            } else {
                copy->builtin = NULL;
                copy->env = cenv_copy(in->env);
                copy->formals = cval_copy(in->formals);
                copy->body = cval_copy(in->body);
            }
            break;
        case CoolValue_Number:
            copy->number = in->number;
            break;
        case CoolValue_String: 
            copy->string = (char *) malloc((strlen(in->string) + 1) * sizeof(char));
            strcpy(copy->string, in->string);
            break;
        case CoolValue_Error:
            copy->errorString = (char *) malloc((strlen(in->errorString) + 1) * sizeof(char));
            strcpy(copy->errorString, in->errorString);
            break;
        case CoolValue_Symbol:
            copy->symbolString = (char *) malloc((strlen(in->symbolString) + 1) * sizeof(char));
            strcpy(copy->symbolString, in->symbolString);
            break;
        case CoolValue_Sexpr:
        case CoolValue_Qexpr:
            copy->count = in->count;
            copy->cell = (cval **) malloc(sizeof(cval *) * copy->count);
            for (int i = 0; i < copy->count; i++) {
                copy->cell[i] = cval_copy(in->cell[i]);
            }
            break;
    }

    return copy;
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
cval_read_num(mpc_ast_t* t) 
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? cval_number(x) : cval_error("invalid number");
}

cval *
cval_readString(mpc_ast_t *t)
{
    t->contents[strlen(t->contents) - 1] = '\0';
    char *unescaped = (char *) malloc(strlen(t->contents + 1) + 1); // quote at start and end
    strcpy(unescaped, t->contents + 1);
    unescaped = mpcf_unescape(unescaped); // drop quotes
    cval *string = cval_string(unescaped);
    free(unescaped);
    return string;
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
    if (strstr(t->tag, "string")) {
        return cval_readString(t);
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
        if (strcmp(t->children[i]->tag, "comment") == 0) {
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
cval_call(cenv *env, cval *function, cval *x)
{
    if (function->builtin) {
        return function->builtin(env, x);
    }

    int given = x->count;
    int total = function->formals->count;

    while (x->count > 0) {
        if (function->formals->count == 0) {
            cval_delete(x);
            return cval_error("Function was given too many arguments. Got %d, expected %d", given, total);
        }

        cval *symbol = cval_pop(function->formals, 0);

        if (strcmp(symbol->symbolString, "&") == 0) {
            if (function->formals->count != 1) {
                cval_delete(x);
                return cval_error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
            }

            cval* nsym = cval_pop(function->formals, 0);
            cenv_put(function->env, nsym, builtin_list(env, x));
            cval_delete(symbol); 
            cval_delete(nsym);
            break;
        }

        cval *value = cval_pop(x, 0);
        
        cenv_put(function->env, symbol, value);

        cval_delete(symbol);
        cval_delete(value);
    }

    cval_delete(x);

    if (function->formals->count > 0 && strcmp(function->formals->cell[0]->symbolString, "&") == 0) {
        if (function->formals->count != 2) {
            return cval_error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
        }

        cval_delete(cval_pop(function->formals, 0));

        cval* sym = cval_pop(function->formals, 0);
        cval* val = cval_qexpr();

        cenv_put(function->env, sym, val);
        cval_delete(sym); 
        cval_delete(val);
    }

    if (function->formals->count == 0) {
        function->env->parent = env;
        cval *newFunctionBody = cval_add(cval_sexpr(), cval_copy(function->body));
        return builtin_eval(function->env, newFunctionBody);
    } else {
        return cval_copy(function);
    }
}

cval *
builtin_lambda(cenv *env, cval *x) 
{
    CASSERT_NUM("\\", x, 2);
    CASSERT_TYPE("\\", x, 0, CoolValue_Qexpr);
    CASSERT_TYPE("\\", x, 1, CoolValue_Qexpr);

    for (int i = 0; i < x->cell[0]->count; i++) {
        CASSERT(x, (x->cell[0]->cell[i]->type == CoolValue_Symbol), "Cannot define non-symbol. Got %s, Expected %s", 
            cval_typeString(x->cell[0]->cell[i]->type), cval_typeString(CoolValue_Symbol));
    }

    cval *formals = cval_pop(x, 0);
    cval *body = cval_pop(x, 0);
    cval_delete(x);

    cval *lambda = cval_lambda(formals, body);

    return lambda;
}

cval *
builtin_var(cenv *env, cval *val, char *function) 
{
    CASSERT(val, val->count > 0, "Function 'def' expected non-empty variable declaration, got %d", val->count);
    CASSERT(val, val->cell[0]->type == CoolValue_Qexpr, "Function 'def' passed incorrect type, got %s", cval_typeString(val->cell[0]->type));

    cval *symbols = val->cell[0];

    // cval_println(val);
    // cval_println(symbols);

    for (int i = 0; i < symbols->count; i++) {
        CASSERT(val, symbols->cell[i]->type == CoolValue_Symbol, "Function 'def' cannot define a non-symbol, got %s", cval_typeString(symbols->cell[i]->type));
    }

    CASSERT(val, symbols->count == val->count - 1, "Function 'def' cannot define incorrect number of values to symbols, got %d", symbols->count);

    for (int i = 0; i < symbols->count; i++) {
        if (strcmp(function, "def") == 0) {
            cenv_def(env, symbols->cell[i], val->cell[i + 1]);
        } 

        if (strcmp(function, "=") == 0) {
            cenv_put(env, symbols->cell[i], val->cell[i + 1]);
        }
    }

    cval_delete(val);

    return cval_sexpr();
}

cval *
builtin_def(cenv *env, cval *val)
{
    return builtin_var(env, val, "def");
}

cval *
builtin_put(cenv *env, cval *val)
{
    return builtin_var(env, val, "put");
}

cval *
builtin_head(cenv *env, cval *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'head' passed incorrect type, got %s", cval_typeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    cval *head = cval_take(x, 0);
    while (head->count > 1) {
        cval_delete(cval_pop(head, 1));
    }

    return head;
}

cval *
builtin_tail(cenv *env, cval *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'tail' passed incorrect type, got %s", cval_typeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    cval *head = cval_take(x, 0);
    cval_delete(cval_pop(head, 0));
    cval *tail = head;

    return tail;
}

cval *
builtin_list(cenv *env, cval *x)
{
    x->type = CoolValue_Qexpr;
    return x;
}

cval *
builtin_eval(cenv *env, cval *x)
{
    CASSERT(x, x->count == 1, "Function 'eval' passed too many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'eval' passed incorrect type, got %s", cval_typeString(x->cell[0]->type));

    // pop the head and evaluate it
    cval *head = cval_take(x, 0);
    head->type = CoolValue_Sexpr;
    cval *evalResult = cval_eval(env, head);

    return evalResult;
}

cval *
builtin_join(cenv *env, cval *x)
{
    for (int i = 0; i < x->count; i++) {
        CASSERT(x, x->cell[i]->type == CoolValue_Qexpr, "Function 'join' passed incorrect type, got %s", x->cell[i]->type);
    }

    cval *y = cval_pop(x, 0);
    while (x->count > 0) {
        cval *z = cval_pop(x, 0);
        y = cval_join(y, z);
    }

    cval_delete(x);

    return y;
}

int 
cval_equal(cval *x, cval *y)
{
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case CoolValue_Number:
            return x->number == y->number;
        case CoolValue_String:
            return (strcmp(x->string, y->string) == 0);
        case CoolValue_Symbol:
            return (strcmp(x->symbolString, y->symbolString) == 0);
        case CoolValue_Error:
            return (strcmp(x->errorString, y->errorString) == 0);
        case CoolValue_Function:
            if (x->builtin || y->builtin) {
                return x->builtin == y->builtin;
            } else {
                return cval_equal(x->formals, y->formals) && cval_equal(x->body, y->body);
            }
        case CoolValue_Sexpr:
        case CoolValue_Qexpr:
            if (x->count != y->count) {
                return 0;
            }
            for (int i = 0; i < x->count; i++) {
                int valuesEqual = cval_equal(x->cell[i], y->cell[i]);
                if (valuesEqual == 0) { // values not equal
                    return 0;
                }
            }
            return 1; // must have been true, we didn't short circuit
    }

    return 0; // false by default
}

cval *
builtin_order(cenv *env, cval *x, char *operator) 
{
    CASSERT_NUM(operator, x, 2);
    CASSERT_TYPE(operator, x, 0, CoolValue_Number);
    CASSERT_TYPE(operator, x, 1, CoolValue_Number);

    int ret = 0;
    if (strcmp(operator, ">") == 0) {
        ret = (x->cell[0]->number > x->cell[1]->number);
    } else if (strcmp(operator, "<") == 0) {
        ret = (x->cell[0]->number < x->cell[1]->number);
    } else if (strcmp(operator, ">=") == 0) {
        ret = (x->cell[0]->number >= x->cell[1]->number);
    } else if (strcmp(operator, "<=") == 0) {
        ret = (x->cell[0]->number <= x->cell[1]->number);
    } else {
        cval_delete(x);
        return cval_error("Error: unexpected operator in 'builtin_order': %s", operator);
    }

    cval_delete(x);
    return cval_number(ret);
}

cval *
builtin_gt(cenv *env, cval *x)
{
    return builtin_order(env, x, ">");
}

cval *
builtin_lt(cenv *env, cval *x)
{
    return builtin_order(env, x, "<");
}

cval *
builtin_gte(cenv *env, cval *x)
{
    return builtin_order(env, x, ">=");
}

cval *
builtin_lte(cenv *env, cval *x)
{
    return builtin_order(env, x, "<=");
}

cval *
builtin_compare(cenv *env, cval *x, char *operator) {
    CASSERT_NUM(operator, x, 2);
    int ret = 0;

    if (strcmp(operator, "==") == 0) {
        ret = cval_equal(x->cell[0], x->cell[1]);
    } else if (strcmp(operator, "!=") == 0) {
        ret = !cval_equal(x->cell[0], x->cell[1]);
    }

    cval_delete(x);
    return cval_number(ret);
}

cval *
builtin_equal(cenv *env, cval *x)
{
    return builtin_compare(env, x, "==");
}

cval *
builtin_notequal(cenv *env, cval *x)
{
    return builtin_compare(env, x, "!=");
}

cval *
builtin_if(cenv *env, cval *x)
{
    CASSERT_NUM("if", x, 3);
    CASSERT_TYPE("if", x, 0, CoolValue_Number);
    CASSERT_TYPE("if", x, 1, CoolValue_Qexpr);
    CASSERT_TYPE("if", x, 2, CoolValue_Qexpr);

    cval *y;
    x->cell[1]->type = CoolValue_Sexpr;
    x->cell[2]->type = CoolValue_Sexpr;

    if (x->cell[0]->number > 0) {
        y = cval_eval(env, cval_pop(x, 1));
    } else {
        y = cval_eval(env, cval_pop(x, 2));
    }

    cval_delete(x);
    return y;
}

cval *
builtin_op(cenv *env, cval *expr, char* op) 
{
    for (int i = 0; i < expr->count; i++) {
        if (expr->cell[i]->type != CoolValue_Number) {
            cval_delete(expr);
            return cval_error("Cannot operate on a non-number, got type %s", cval_typeString(expr->cell[i]->type));
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
builtin_add(cenv *env, cval *val)
{
    return builtin_op(env, val, "+");
}

cval *
builtin_sub(cenv *env, cval *val)
{
    return builtin_op(env, val, "-");
}

cval *
builtin_mul(cenv *env, cval *val)
{
    return builtin_op(env, val, "*");
}

cval *
builtin_div(cenv *env, cval *val)
{
    return builtin_op(env, val, "/");
}

cval *
builtin_load(cenv *env, cval *x)
{
    CASSERT_NUM("load", x, 1);
    CASSERT_TYPE("load", x, 0, CoolValue_String);

    mpc_result_t result;
    if (mpc_parse_contents(x->cell[0]->string, Cool, &result)) {
        
        cval *expr = cval_read(result.output);
        mpc_ast_delete(result.output);

        while (expr->count > 0) {
            cval *y = cval_eval(env, cval_pop(expr, 0));
            if (y->type == CoolValue_Error) {
                cval_println(y);
            } 
            cval_delete(y);
        }

        cval_delete(expr);
        cval_delete(x);

        return cval_sexpr();
    } else { // parsing error
        char *errorMessage = mpc_err_string(result.error);
        mpc_err_delete(result.error);

        cval *error = cval_error("Could not load library %s", errorMessage);
        free(errorMessage);
        cval_delete(x);

        return error;
    }
}

cval * 
builtin_print(cenv *env, cval *x)
{
    for (int i = 0; i < x->count; i++) {
        cval_print(x->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    cval_delete(x);

    return cval_sexpr();
}

cval *
builtin_error(cenv *env, cval *x)
{
    CASSERT_NUM("error", x, 1);
    CASSERT_TYPE("error", x, 0, CoolValue_String);
    cval *error = cval_error(x->cell[0]->string);
    cval_delete(x);
    return error;
}

void
cenv_addBuiltin(cenv *env, char *name, cbuiltin function)
{
    cval *key = cval_symbol(name);
    cval *value = cval_builtin(function);
    cenv_put(env, key, value);
    cval_delete(key);
    cval_delete(value);
}

void 
cenv_addBuiltinFunctions(cenv *env) 
{
    cenv_addBuiltin(env, "load", builtin_load);
    cenv_addBuiltin(env, "print", builtin_print);
    cenv_addBuiltin(env, "error", builtin_error);

    cenv_addBuiltin(env, "\\", builtin_lambda);
    cenv_addBuiltin(env, "def", builtin_def);
    cenv_addBuiltin(env, "=", builtin_put);

    cenv_addBuiltin(env, "if", builtin_if);
    cenv_addBuiltin(env, "==", builtin_equal);
    cenv_addBuiltin(env, "!=", builtin_notequal);
    cenv_addBuiltin(env, ">", builtin_gt);
    cenv_addBuiltin(env, "<", builtin_lt);
    cenv_addBuiltin(env, ">=", builtin_gte);
    cenv_addBuiltin(env, "<=", builtin_lte);

    cenv_addBuiltin(env, "list", builtin_list);
    cenv_addBuiltin(env, "eval", builtin_eval);
    cenv_addBuiltin(env, "join", builtin_join);
    cenv_addBuiltin(env, "head", builtin_head);
    cenv_addBuiltin(env, "tail", builtin_tail);

    cenv_addBuiltin(env, "+", builtin_add);
    cenv_addBuiltin(env, "-", builtin_sub);
    cenv_addBuiltin(env, "*", builtin_mul);
    cenv_addBuiltin(env, "/", builtin_div);    
}

cval *
cval_evaluateExpression(cenv *env, cval *value)
{
    // Evaluate each expression
    for (int i = 0; i < value->count; i++) {
        value->cell[i] = cval_eval(env, value->cell[i]);
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

    cval *funcSymbol = cval_pop(value, 0);
    if (funcSymbol->type != CoolValue_Function) {
        cval_delete(funcSymbol);
        cval_delete(value);
        return cval_error("S-expression does not start with a function, got type %s", cval_typeString(funcSymbol->type));
    }

    cval *result = cval_call(env, funcSymbol, value);
    cval_delete(funcSymbol);

    return result;
}

cval *
cval_eval(cenv* env, cval *value) 
{
    if (value->type == CoolValue_Symbol) {
        cval *result = cenv_get(env, value);
        cval_delete(value);
        return result;
    } else if (value->type == CoolValue_Sexpr) {
        return cval_evaluateExpression(env, value);
    }
    return value;
}

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
        "                                                       \
            number  : /-?[0-9]+/ ;                              \
            symbol  : /[a-zA-Z0-9_+\\-*\\/\\\\=<>!&]+/ ;        \
            string  : /\"(\\\\.|[^\"])*\"/ ;                    \
            comment : /;[^\\r\\n]*/ ;                           \
            sexpr   : '(' <expr>* ')' ;                         \
            qexpr   : '{' <expr>* '}' ;                         \
            expr    : <number> | <symbol> | <sexpr> |           \
                      <qexpr> | <string> | <comment> ;          \
            cool    : /^/ <expr>* /$/ ;                         \
        ",
        Number, Symbol, String, Comment, Sexpr, Qexpr, Expr, Cool);

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
            cval * x = cval_eval(env, cval_read(r.output));
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
