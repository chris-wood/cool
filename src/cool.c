#include <stdio.h>

#include "cool.h"

#define FILE_BLOCK_SIZE 128

// builtin function
typedef Value *(*cbuiltin)(Environment *, Value *);

struct cval {
    int type;
    long number;
    double fpnumber;

    char *errorString;
    char *symbolString;
    char *string;

    cbuiltin builtin;
    Environment *env;
    Value *formals;
    Value *body;

    int count;
    struct cval ** cell;
    int error;
};

struct cenv {
    Environment *parent;
    int count;
    char **symbols;
    struct cval **values;
}; 

// Forward declaration prototypes
void Value_print(Value *value);
Value *Value_eval(Environment *env, Value *value);
void Value_delete(Value *value);
Value *Value_error(char *fmt, ...);
Value *Value_copy(Value *in);
char *Value_typeString(int type);
Value *builtin_eval(Environment *env, Value *x);
Value *builtin_list(Environment *env, Value *x);

#define CASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        Value *error = Value_error(fmt, ##__VA_ARGS__); \
        Value_delete(args); \
        return error; \
    }

#define CASSERT_TYPE(func, args, index, expect) \
    CASSERT(args, (args->cell[index]->type & expect) > 0, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, Value_typeString(args->cell[index]->type), Value_typeString(expect));

#define CASSERT_NUM(func, args, num) \
    CASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num);

#define CASSERT_NOT_EMPTY(func, args, index) \
    CASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

Environment *
Environment_new() 
{
    Environment *env = (Environment *) malloc(sizeof(Environment));
    env->parent = NULL;
    env->count = 0;
    env->symbols = (char **) malloc(sizeof(char *));
    env->values = (Value **) malloc(sizeof(Value *));
    return env;
}

void
Environment_delete(Environment *env) 
{
    for (int i = 0; i < env->count; i++) {
        free(env->symbols[i]);
        Value_delete(env->values[i]);
    }
    free(env->symbols);
    free(env->values);
    free(env);
}

Value *
Environment_get(Environment *env, Value *val) 
{
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], val->symbolString) == 0) {
            return Value_copy(env->values[i]);
        }
    }

    if (env->parent) {
        return Environment_get(env->parent, val);
    } else {
        return Value_error("Undefined symbol: %s", val->symbolString);
    }
}

void 
Environment_put(Environment *env, Value* key, Value *val) 
{
    // Replace the value if it exists
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], key->symbolString) == 0) {
            Value_delete(env->values[i]);
            env->values[i] = Value_copy(val);
            return;
        }
    }

    env->count++;
    env->values = realloc(env->values, sizeof(Value *) * env->count);
    env->symbols = realloc(env->symbols, sizeof(char *) * env->count);

    env->values[env->count - 1] = Value_copy(val);
    env->symbols[env->count - 1] = malloc(strlen(key->symbolString) + 1);
    strcpy(env->symbols[env->count - 1], key->symbolString);
}

Environment *
Environment_copy(Environment *env)
{
    Environment *copy = (Environment *) malloc(sizeof(Environment));
    copy->parent = env->parent;
    copy->count = env->count;
    copy->symbols = (char **) malloc(sizeof(char *) * env->count);
    copy->values = (Value **) malloc(sizeof(Value *) * env->count);
    for (int i = 0; i < env->count; i++) {
        copy->symbols[i] = (char *) malloc(strlen(env->symbols[i]) + 1);
        strcpy(copy->symbols[i], env->symbols[i]);
        copy->values[i] = Value_copy(env->values[i]);
    }
    return copy;
}

void
Environment_def(Environment *env, Value *key, Value *value)
{
    while (env->parent != NULL) {
        env = env->parent;
    }
    Environment_put(env, key, value);
}

CoolValue 
Value_getType(Value *value)
{
    return (CoolValue) value->type;
}

Value *
Value_longInteger(long x) 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_LongInteger;
    value->number = x;
    return value;
}

Value *
Value_double(double x)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Double;
    value->fpnumber = x;
    return value;
}

Value *
Value_byte(char x)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Byte;
    value->number = (long) x;
    return value;
}

Value *
Value_string(char *str)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_String;
    value->string = (char *) malloc((strlen(str) + 1) * sizeof(char));
    strcpy(value->string, str);
    return value;
}

Value *
Value_symbol(char *symbol) 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Symbol;
    value->symbolString = (char *) malloc((strlen(symbol) + 1) * sizeof(char));
    strcpy(value->symbolString, symbol);
    return value;
}

Value * 
Value_sexpr() 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Sexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

Value * 
Value_qexpr() 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Qexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

Value *
Value_builtin(cbuiltin function) 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Function;
    value->builtin = function;
    return value;
}

Value *
Value_lambda(Value *formals, Value *body)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Function;
    value->builtin = NULL;
    value->env = Environment_new();
    value->formals = formals;
    value->body = body;
    return value;
}

char *
Value_typeString(int type)
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
        case CoolValue_LongInteger:
            return "CoolValue_LongInteger";
        case CoolValue_Double:
            return "CoolValue_Double";
        case CoolValue_Byte:
            return "CoolValue_Byte";
        case CoolValue_String:
            return "CoolValue_String";
        case CoolValue_Symbol:
        default:
            return "CoolValue_Symbol";
    }
}

Value * 
Value_error(char *fmt, ...) 
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Error;

    va_list va;
    va_start(va, fmt);

    value->errorString = (char *) malloc(512); // arbitrary for all error strings (more than needed)
    vsnprintf(value->errorString, 511, fmt, va);

    value->errorString = (char *) realloc(value->errorString, strlen(value->errorString) + 1);
    va_end(va);

    return value;
}

Value *
Value_readContent(char *fileName)
{
    FILE *fp = fopen(fileName, "r");
    if (fp == NULL) {
        // TODO: insert interest issuance here
        return Value_error("Unable to open file %s", fileName);
    } else {
        Value *value = Value_sexpr();
        char fileBuffer[FILE_BLOCK_SIZE]; // read in FILE_BLOCK_SIZE-byte blocks
        size_t numBytesRead = 0;
        for (;;) {
            numBytesRead = fread(fileBuffer, 1, FILE_BLOCK_SIZE, fp);

            // Copy fileBuffer bytes to the value list
            int start = value->count;

            value->count += numBytesRead;
            value->cell = realloc(value->cell, sizeof(Value *) * value->count);

            for (int i = 0; i < numBytesRead; i++) {
                int index = start + i;
                value->cell[index] = Value_byte(fileBuffer[i]);
            }

            // Reset the file buffer (to zeros) and check to see if we're done
            memset(fileBuffer, 0, FILE_BLOCK_SIZE); 
            if (numBytesRead != FILE_BLOCK_SIZE) {
                break;
            }
        }
        return value;
    }
}

void 
Value_delete(Value *value)
{
    switch (value->type) {
        case CoolValue_LongInteger:
        case CoolValue_Double:
        case CoolValue_Byte:
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
                Value_delete(value->cell[i]);
            }
            free(value->cell);
            break;
        case CoolValue_Function:
            if (value->builtin == NULL) {
                Environment_delete(value->env);
                Value_delete(value->formals);
                Value_delete(value->body);
            }
            break;
    }

    free(value);
}

void
Value_printExpr(Value *value, char open, char close)
{
    putchar(open);
    for (int i = 0; i < value->count; i++) {
        Value_print(value->cell[i]);
        if (i != (value->count - 1)) {
            putchar(' ');
        }
    }
    putchar(close);
}

void 
Value_print(Value *value) 
{
    switch (value->type) {
        case CoolValue_LongInteger: 
            printf("%li", value->number);
            break;
        case CoolValue_Double: 
            printf("%f", value->fpnumber);
            break;
        case CoolValue_Byte:
            printf("%x", (uint8_t) value->number);
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
            Value_printExpr(value, '(', ')');
            break;
        case CoolValue_Qexpr:
            Value_printExpr(value, '{', '}');
            break;
        case CoolValue_Function:
            if (value->builtin != NULL) {
                printf("<builtin>");
            } else {
                printf("(\\ ");
                Value_print(value->formals);
                printf(" ");
                Value_print(value->body);
                printf(" ");
            }
            break;
    }  
}

void 
Value_println(Value *value) 
{
    Value_print(value);
    putchar('\n');
}

Value *
Value_copy(Value *in)
{
    Value *copy = (Value *) malloc(sizeof(Value));
    copy->type = in->type;

    switch (in->type) {
        case CoolValue_Function:
            if (in->builtin != NULL) {
                copy->builtin = in->builtin;
            } else {
                copy->builtin = NULL;
                copy->env = Environment_copy(in->env);
                copy->formals = Value_copy(in->formals);
                copy->body = Value_copy(in->body);
            }
            break;
        case CoolValue_LongInteger:
        case CoolValue_Byte:
            copy->number = in->number;
            break;
        case CoolValue_Double:
            copy->fpnumber = in->fpnumber;
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
            copy->cell = (Value **) malloc(sizeof(Value *) * copy->count);
            for (int i = 0; i < copy->count; i++) {
                copy->cell[i] = Value_copy(in->cell[i]);
            }
            break;
    }

    return copy;
}

Value *
Value_add(Value *value, Value *x) 
{
    value->count++;
    value->cell = realloc(value->cell, sizeof(Value *) * value->count);
    value->cell[value->count - 1] = x;
    return value;
}

Value * 
Value_read_num(mpc_ast_t* t) 
{
    errno = 0;
    long x = strtol(t->contents, NULL, 10);
    return errno != ERANGE ? Value_longInteger(x) : Value_error("invalid number");
}

Value *
Value_read_fpnumber(mpc_ast_t* t)
{
    errno = 0;
    double x = atof(t->contents);
    return Value_double(x);
}

Value *
Value_read_number(mpc_ast_t* t)
{
    if (strstr(t->contents, ".")) {
        return Value_read_fpnumber(t);
    } else {
        return Value_read_num(t);
    }
}

Value *
Value_readString(mpc_ast_t *t)
{
    t->contents[strlen(t->contents) - 1] = '\0';
    char *unescaped = (char *) malloc(strlen(t->contents + 1) + 1); // quote at start and end
    strcpy(unescaped, t->contents + 1);
    unescaped = mpcf_unescape(unescaped); // drop quotes
    Value *string = Value_string(unescaped);
    free(unescaped);
    return string;
}

Value *
Value_read(mpc_ast_t *t) 
{
    if (strstr(t->tag, "number")) {
        return Value_read_number(t);
    }
    if (strstr(t->tag, "symbol")) {
        return Value_symbol(t->contents);
    }
    if (strstr(t->tag, "string")) {
        return Value_readString(t);
    }

    Value *x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = Value_sexpr();
    }
    if (strstr(t->tag, "sexpr")) {
        x = Value_sexpr();
    }
    if (strstr(t->tag, "qexpr")) {
        x = Value_qexpr();
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
        x = Value_add(x, Value_read(t->children[i]));
    }

    return x;
}

Value *
Value_pop(Value *value, int i)
{
    Value *x = value->cell[i];
    memmove(&value->cell[i], &value->cell[i + 1], sizeof(Value *) * (value->count - 1));
    value->count--;
    value->cell = realloc(value->cell, sizeof(Value *) * value->count);
    return x;
}

Value *
Value_take(Value *value, int i) 
{
    Value *x = Value_pop(value, i);
    Value_delete(value);
    return x;
}

Value *
Value_join(Value *x, Value *y)
{
    while (y->count > 0) {
        x = Value_add(x, Value_pop(y, 0));
    }
    Value_delete(y);
    return x;
}

Value *
Value_call(Environment *env, Value *function, Value *x)
{
    if (function->builtin) {
        Value *val = function->builtin(env, x);
        return val;
    }

    int given = x->count;
    int total = function->formals->count;

    while (x->count > 0) {
        if (function->formals->count == 0) {
            Value_delete(x);
            return Value_error("Function was given too many arguments. Got %d, expected %d", given, total);
        }

        Value *symbol = Value_pop(function->formals, 0);

        if (strcmp(symbol->symbolString, "&") == 0) {
            if (function->formals->count != 1) {
                Value_delete(x);
                return Value_error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
            }

            Value* nsym = Value_pop(function->formals, 0);
            Environment_put(function->env, nsym, builtin_list(env, x));
            Value_delete(symbol); 
            Value_delete(nsym);
            break;
        }

        Value *value = Value_pop(x, 0);
        
        Environment_put(function->env, symbol, value);

        Value_delete(symbol);
        Value_delete(value);
    }

    Value_delete(x);

    if (function->formals->count > 0 && strcmp(function->formals->cell[0]->symbolString, "&") == 0) {
        if (function->formals->count != 2) {
            return Value_error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
        }

        Value_delete(Value_pop(function->formals, 0));

        Value* sym = Value_pop(function->formals, 0);
        Value* val = Value_qexpr();

        Environment_put(function->env, sym, val);
        Value_delete(sym); 
        Value_delete(val);
    }

    if (function->formals->count == 0) {
        function->env->parent = env;
        Value *newFunctionBody = Value_add(Value_sexpr(), Value_copy(function->body));
        return builtin_eval(function->env, newFunctionBody);
    } else {
        return Value_copy(function);
    }
}

Value *
builtin_lambda(Environment *env, Value *x) 
{
    CASSERT_NUM("\\", x, 2);
    CASSERT_TYPE("\\", x, 0, CoolValue_Qexpr);
    CASSERT_TYPE("\\", x, 1, CoolValue_Qexpr);

    for (int i = 0; i < x->cell[0]->count; i++) {
        CASSERT(x, (x->cell[0]->cell[i]->type == CoolValue_Symbol), "Cannot define non-symbol. Got %s, Expected %s", 
            Value_typeString(x->cell[0]->cell[i]->type), Value_typeString(CoolValue_Symbol));
    }

    Value *formals = Value_pop(x, 0);
    Value *body = Value_pop(x, 0);
    Value_delete(x);

    Value *lambda = Value_lambda(formals, body);

    return lambda;
}

Value *
builtin_var(Environment *env, Value *val, char *function) 
{
    CASSERT(val, val->count > 0, "Function 'def' expected non-empty variable declaration, got %d", val->count);
    CASSERT(val, val->cell[0]->type == CoolValue_Qexpr, "Function 'def' passed incorrect type, got %s", Value_typeString(val->cell[0]->type));

    Value *symbols = val->cell[0];

    // Value_println(val);
    // Value_println(symbols);

    for (int i = 0; i < symbols->count; i++) {
        CASSERT(val, symbols->cell[i]->type == CoolValue_Symbol, "Function 'def' cannot define a non-symbol, got %s", Value_typeString(symbols->cell[i]->type));
    }

    CASSERT(val, symbols->count == val->count - 1, "Function 'def' cannot define incorrect number of values to symbols, got %d", symbols->count);

    for (int i = 0; i < symbols->count; i++) {
        if (strcmp(function, "def") == 0) {
            Environment_def(env, symbols->cell[i], val->cell[i + 1]);
        } 

        if (strcmp(function, "=") == 0) {
            Environment_put(env, symbols->cell[i], val->cell[i + 1]);
        }
    }

    Value_delete(val);

    return Value_sexpr();
}

Value *
builtin_def(Environment *env, Value *val)
{
    return builtin_var(env, val, "def");
}

Value *
builtin_put(Environment *env, Value *val)
{
    return builtin_var(env, val, "put");
}

Value *
builtin_head(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'head' passed incorrect type, got %s", Value_typeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    Value *head = Value_take(x, 0);
    while (head->count > 1) {
        Value_delete(Value_pop(head, 1));
    }

    return head;
}

Value *
builtin_tail(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'tail' passed incorrect type, got %s", Value_typeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    Value *head = Value_take(x, 0);
    Value_delete(Value_pop(head, 0));
    Value *tail = head;

    return tail;
}

Value *
builtin_list(Environment *env, Value *x)
{
    x->type = CoolValue_Qexpr;
    return x;
}

Value *
builtin_eval(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'eval' passed too many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'eval' passed incorrect type, got %s", Value_typeString(x->cell[0]->type));

    // pop the head and evaluate it
    Value *head = Value_take(x, 0);
    head->type = CoolValue_Sexpr;
    Value *evalResult = Value_eval(env, head);

    return evalResult;
}

Value *
builtin_join(Environment *env, Value *x)
{
    for (int i = 0; i < x->count; i++) {
        CASSERT(x, x->cell[i]->type == CoolValue_Qexpr, "Function 'join' passed incorrect type, got %s", x->cell[i]->type);
    }

    Value *y = Value_pop(x, 0);
    while (x->count > 0) {
        Value *z = Value_pop(x, 0);
        y = Value_join(y, z);
    }

    Value_delete(x);

    return y;
}

int 
Value_equal(Value *x, Value *y)
{
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case CoolValue_LongInteger:
        case CoolValue_Byte:
            return x->number == y->number;
        case CoolValue_Double:
            return x->fpnumber == y->fpnumber;
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
                return Value_equal(x->formals, y->formals) && Value_equal(x->body, y->body);
            }
        case CoolValue_Sexpr:
        case CoolValue_Qexpr:
            if (x->count != y->count) {
                return 0;
            }
            for (int i = 0; i < x->count; i++) {
                int valuesEqual = Value_equal(x->cell[i], y->cell[i]);
                if (valuesEqual == 0) { // values not equal
                    return 0;
                }
            }
            return 1; // must have been true, we didn't short circuit
    }

    return 0; // false by default
}

Value *
builtin_double_order(Environment *env, Value *x, char *operator) 
{
    int ret = 0;
    if (strcmp(operator, ">") == 0) {
        ret = (x->cell[0]->fpnumber > x->cell[1]->fpnumber);
    } else if (strcmp(operator, "<") == 0) {
        ret = (x->cell[0]->fpnumber < x->cell[1]->fpnumber);
    } else if (strcmp(operator, ">=") == 0) {
        ret = (x->cell[0]->fpnumber >= x->cell[1]->fpnumber);
    } else if (strcmp(operator, "<=") == 0) {
        ret = (x->cell[0]->fpnumber <= x->cell[1]->fpnumber);
    } else {
        Value_delete(x);
        return Value_error("Error: unexpected operator in 'builtin_order': %s", operator);
    }

    Value_delete(x);
    return Value_longInteger(ret);
}

Value *
builtin_long_order(Environment *env, Value *x, char *operator) 
{
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
        Value_delete(x);
        return Value_error("Error: unexpected operator in 'builtin_order': %s", operator);
    }

    Value_delete(x);
    return Value_longInteger(ret);
}

Value *
builtin_order(Environment *env, Value *x, char *operator) 
{
    CASSERT_NUM(operator, x, 2);
    if (x->cell[1]->type == CoolValue_LongInteger || x->cell[1]->type == CoolValue_Byte) {
        CASSERT_TYPE(operator, x, 0, CoolValue_LongInteger | CoolValue_Byte);
        CASSERT_TYPE(operator, x, 1, CoolValue_LongInteger | CoolValue_Byte);
        return builtin_long_order(env, x, operator);
    } else {
        CASSERT_TYPE(operator, x, 0, CoolValue_Double);
        CASSERT_TYPE(operator, x, 1, CoolValue_Double);
        return builtin_double_order(env, x, operator);
    }
}

Value *
builtin_gt(Environment *env, Value *x)
{
    return builtin_order(env, x, ">");
}

Value *
builtin_lt(Environment *env, Value *x)
{
    return builtin_order(env, x, "<");
}

Value *
builtin_gte(Environment *env, Value *x)
{
    return builtin_order(env, x, ">=");
}

Value *
builtin_lte(Environment *env, Value *x)
{
    return builtin_order(env, x, "<=");
}

Value *
builtin_compare(Environment *env, Value *x, char *operator) {
    CASSERT_NUM(operator, x, 2);
    int ret = 0;

    if (strcmp(operator, "==") == 0) {
        ret = Value_equal(x->cell[0], x->cell[1]);
    } else if (strcmp(operator, "!=") == 0) {
        ret = !Value_equal(x->cell[0], x->cell[1]);
    }

    Value_delete(x);
    Value *result = Value_longInteger(ret);

    return result;
}

Value *
builtin_equal(Environment *env, Value *x)
{
    return builtin_compare(env, x, "==");
}

Value *
builtin_notequal(Environment *env, Value *x)
{
    return builtin_compare(env, x, "!=");
}

Value *
builtin_if(Environment *env, Value *x)
{
    CASSERT_NUM("if", x, 3);
    CASSERT_TYPE("if", x, 0, CoolValue_LongInteger);
    CASSERT_TYPE("if", x, 1, CoolValue_Qexpr);
    CASSERT_TYPE("if", x, 2, CoolValue_Qexpr);

    Value *y;
    x->cell[1]->type = CoolValue_Sexpr;
    x->cell[2]->type = CoolValue_Sexpr;

    if (x->cell[0]->number > 0) {
        y = Value_eval(env, Value_pop(x, 1));
    } else {
        y = Value_eval(env, Value_pop(x, 2));
    }

    Value_delete(x);
    return y;
}

Value *
builtin_op(Environment *env, Value *expr, char* op) 
{
    for (int i = 0; i < expr->count; i++) {
        if (expr->cell[i]->type != CoolValue_LongInteger && expr->cell[i]->type != CoolValue_Double) {
            Value_delete(expr);
            return Value_error("Cannot operate on a non-number, got type %s", Value_typeString(expr->cell[i]->type));
        }
    }

    Value *x = Value_pop(expr, 0);

    if ((strcmp(op, "-") == 0) && expr->count == 0) {
        x->number = -x->number;
    }

    while (expr->count > 0) {
        Value *y = Value_pop(expr, 0);
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
                Value_delete(x);
                Value_delete(y);
                x = Value_error("Division by zero.");
                break;
            }
            x->number /= y->number;
        }
        Value_delete(y);
    }

    Value_delete(expr);
    return x;
}

Value *
builtin_add(Environment *env, Value *val)
{
    return builtin_op(env, val, "+");
}

Value *
builtin_sub(Environment *env, Value *val)
{
    return builtin_op(env, val, "-");
}

Value *
builtin_mul(Environment *env, Value *val)
{
    return builtin_op(env, val, "*");
}

Value *
builtin_div(Environment *env, Value *val)
{
    return builtin_op(env, val, "/");
}

Value *
builtin_load(Environment *env, Value *x)
{
    CASSERT_NUM("load", x, 1);
    CASSERT_TYPE("load", x, 0, CoolValue_String);

    mpc_result_t result;
    if (mpc_parse_contents(x->cell[0]->string, Cool, &result)) {
        
        Value *expr = Value_read(result.output);
        mpc_ast_delete(result.output);

        while (expr->count > 0) {
            Value *top = Value_pop(expr, 0);
            Value *y = Value_eval(env, top);
            if (y->type == CoolValue_Error) {
                Value_println(y);
            } 
            Value_delete(y);
        }

        Value_delete(expr);
        Value_delete(x);

        return Value_sexpr();
    } else { // parsing error
        char *errorMessage = mpc_err_string(result.error);
        mpc_err_delete(result.error);

        Value *error = Value_error("Could not load library %s", errorMessage);
        free(errorMessage);
        Value_delete(x);

        return error;
    }
}

Value * 
builtin_print(Environment *env, Value *x)
{
    for (int i = 0; i < x->count; i++) {
        Value_print(x->cell[i]);
        putchar(' ');
    }
    putchar('\n');
    Value_delete(x);

    return Value_sexpr();
}

Value *
builtin_error(Environment *env, Value *x)
{
    CASSERT_NUM("error", x, 1);
    CASSERT_TYPE("error", x, 0, CoolValue_String);
    Value *error = Value_error(x->cell[0]->string);
    Value_delete(x);
    return error;
}

Value *
builtin_read(Environment *env, Value *x)
{
    CASSERT_NUM("read", x, 1);
    CASSERT_TYPE("error", x, 0, CoolValue_String);
    Value *byteList = Value_readContent(x->cell[0]->string);
    Value_delete(x);
    return byteList;
}

void
Environment_addBuiltin(Environment *env, char *name, cbuiltin function)
{
    Value *key = Value_symbol(name);
    Value *value = Value_builtin(function);
    Environment_put(env, key, value);
    Value_delete(key);
    Value_delete(value);
}

void 
Environment_addBuiltinFunctions(Environment *env)
{
    Environment_addBuiltin(env, "load", builtin_load);
    Environment_addBuiltin(env, "print", builtin_print);
    Environment_addBuiltin(env, "error", builtin_error);

    Environment_addBuiltin(env, "read", builtin_read);
    // Environment_addBuiltin(env, "write", builtin_write);

    Environment_addBuiltin(env, "\\", builtin_lambda);
    Environment_addBuiltin(env, "def", builtin_def);
    Environment_addBuiltin(env, "=", builtin_put);

    Environment_addBuiltin(env, "if", builtin_if);
    Environment_addBuiltin(env, "==", builtin_equal);
    Environment_addBuiltin(env, "!=", builtin_notequal);
    Environment_addBuiltin(env, ">", builtin_gt);
    Environment_addBuiltin(env, "<", builtin_lt);
    Environment_addBuiltin(env, ">=", builtin_gte);
    Environment_addBuiltin(env, "<=", builtin_lte);

    Environment_addBuiltin(env, "list", builtin_list);
    Environment_addBuiltin(env, "eval", builtin_eval);
    Environment_addBuiltin(env, "join", builtin_join);
    Environment_addBuiltin(env, "head", builtin_head);
    Environment_addBuiltin(env, "tail", builtin_tail);

    Environment_addBuiltin(env, "+", builtin_add);
    Environment_addBuiltin(env, "-", builtin_sub);
    Environment_addBuiltin(env, "*", builtin_mul);
    Environment_addBuiltin(env, "/", builtin_div);
    
    // caw: xor, exponentation, etc
}

Value *
Value_evaluateExpression(Environment *env, Value *value)
{
    // Evaluate each expression
    for (int i = 0; i < value->count; i++) {
        value->cell[i] = Value_eval(env, value->cell[i]);
    }

    // If one is an error, take it and return it
    for (int i = 0; i < value->count; i++) {
        if (value->cell[i]->type == CoolValue_Error) {
            return Value_take(value, i);
        }
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return Value_take(value, 0);
    }

    Value *funcSymbol = Value_pop(value, 0);
    if (funcSymbol->type != CoolValue_Function) {
        Value_delete(funcSymbol);
        Value_delete(value);
        return Value_error("S-expression does not start with a function, got type %s", Value_typeString(funcSymbol->type));
    }

    Value *result = Value_call(env, funcSymbol, value);
    Value_delete(funcSymbol);

    return result;
}

Value *
Value_eval(Environment* env, Value *value) 
{
    if (value->type == CoolValue_Symbol) {
        Value *result = Environment_get(env, value);
        Value_delete(value);
        return result;
    } else if (value->type == CoolValue_Sexpr) {
        return Value_evaluateExpression(env, value);
    }
    return value;
}

