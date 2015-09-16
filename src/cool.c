#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>
#include <pthread.h>
#include <assert.h>

#include "cool.h"

#define FILE_BLOCK_SIZE 128

// builtin function
typedef Value *(*cbuiltin)(Environment *, Value *);

struct cval {
    uint8_t type;
    union {
        double fpnumber;
        uint8_t byte;
        mpz_t bignumber;
    };

    char *errorString;
    char *symbolString;
    char *string;

    cbuiltin builtin;
    Environment *env;
    Value *formals;
    Value *body;

    int count;
    struct cval **cell;
    int error;
};

struct cenv {
    Environment *parent;
    int count;
    char **symbols;
    struct cval **values;
};

// Wrapper for coroutines (from _Run builtin)
typedef struct {
    Environment *env;
    Value *param;
} EvaluateWrapper;

// Forward declaration prototypes
void value_Print(FILE *out, Value *value);
void value_Println(FILE *out, Value *value);
Value *value_Eval(Environment *env, Value *value);
void value_Delete(Value *value);
Value *value_Error(char *fmt, ...);
Value *value_Copy(Value *in);
char *value_TypeString(int type);
Value *builtin_Eval(Environment *env, Value *x);
Value *builtin_List(Environment *env, Value *x);
Value *value_EvaluateExpression(Environment *env, Value *value);

#define CASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        Value *error = value_Error(fmt, ##__VA_ARGS__); \
        value_Delete(args); \
        return error; \
    }

#define CASSERT_TYPE(func, args, index, expect) \
    CASSERT(args, (args->cell[index]->type & expect) > 0, \
        "Function '%s' passed incorrect type for argument %i. Got %s, Expected %s.", \
        func, index, value_TypeString(args->cell[index]->type), value_TypeString(expect));

#define CASSERT_NUM(func, args, num) \
    CASSERT(args, args->count == num, \
        "Function '%s' passed incorrect number of arguments. Got %i, Expected %i.", \
        func, args->count, num);

#define CASSERT_NOT_EMPTY(func, args, index) \
    CASSERT(args, args->cell[index]->count != 0, \
        "Function '%s' passed {} for argument %i.", func, index);

Environment *
environment_Create()
{
    Environment *env = (Environment *) malloc(sizeof(Environment));
    env->parent = NULL;
    env->count = 0;
    env->symbols = (char **) malloc(sizeof(char *));
    env->values = (Value **) malloc(sizeof(Value *));
    return env;
}

void
environment_Delete(Environment *env)
{
    for (int i = 0; i < env->count; i++) {
        free(env->symbols[i]);
        value_Delete(env->values[i]);
    }
    free(env->symbols);
    free(env->values);
    free(env);
}

Value *
environment_Get(Environment *env, Value *val)
{
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], val->symbolString) == 0) {
            return value_Copy(env->values[i]);
        }
    }

    if (env->parent) {
        return environment_Get(env->parent, val);
    } else {
        return value_Error("Undefined symbol: %s", val->symbolString);
    }
}

void
environment_PutKeyValue(Environment *env, Value* key, Value *val)
{
    // Replace the value if it exists
    for (int i = 0; i < env->count; i++) {
        if (strcmp(env->symbols[i], key->symbolString) == 0) {
            value_Delete(env->values[i]);
            env->values[i] = value_Copy(val);
            return;
        }
    }

    env->count++;
    env->values = realloc(env->values, sizeof(Value *) * env->count);
    env->symbols = realloc(env->symbols, sizeof(char *) * env->count);

    env->values[env->count - 1] = value_Copy(val);
    env->symbols[env->count - 1] = malloc(strlen(key->symbolString) + 1);
    strcpy(env->symbols[env->count - 1], key->symbolString);
}

Environment *
environment_Copy(Environment *env)
{
    Environment *copy = (Environment *) malloc(sizeof(Environment));
    copy->parent = env->parent;
    copy->count = env->count;
    copy->symbols = (char **) malloc(sizeof(char *) * env->count);
    copy->values = (Value **) malloc(sizeof(Value *) * env->count);
    for (int i = 0; i < env->count; i++) {
        copy->symbols[i] = (char *) malloc(strlen(env->symbols[i]) + 1);
        strcpy(copy->symbols[i], env->symbols[i]);
        copy->values[i] = value_Copy(env->values[i]);
    }
    return copy;
}

void
environment_DefineKeyValue(Environment *env, Value *key, Value *value)
{
    while (env->parent != NULL) {
        env = env->parent;
    }
    environment_PutKeyValue(env, key, value);
}

CoolValue
value_GetType(Value *value)
{
    return (CoolValue) value->type;
}

Value *
value_Integer(size_t x)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Integer;

    mpz_init(value->bignumber);
    mpz_set_ui(value->bignumber, x);

    return value;
}

Value *
value_Double(double x)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Double;
    value->fpnumber = x;
    return value;
}

Value *
value_Byte(uint8_t x)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Byte;
    value->byte = (long) x;
    return value;
}

Value *
value_String(char *str)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_String;
    value->string = (char *) malloc((strlen(str) + 1) * sizeof(char));
    strcpy(value->string, str);
    return value;
}

Value *
value_Symbol(char *symbol)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Symbol;
    value->symbolString = (char *) malloc((strlen(symbol) + 1) * sizeof(char));
    strcpy(value->symbolString, symbol);
    return value;
}

Value *
value_SExpr()
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Sexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

Value *
value_QExpr()
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Qexpr;
    value->count = 0;
    value->cell = NULL;
    return value;
}

Value *
value_Builtin(cbuiltin function)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Function;
    value->builtin = function;
    return value;
}

Value *
value_Lambda(Value *formals, Value *body)
{
    Value *value = (Value *) malloc(sizeof(Value));
    value->type = CoolValue_Function;
    value->builtin = NULL;
    value->env = environment_Create();
    value->formals = formals;
    value->body = body;
    return value;
}

char *
value_TypeString(int type)
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
        case CoolValue_Integer:
            return "CoolValue_Integer";
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
value_Error(char *fmt, ...)
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
value_ReadContent(char *contentName)
{
    FILE *fp = fopen(contentName, "r");
    if (fp == NULL) {
        // TODO: insert interest issuance here
        return value_Error("Unable to open file %s", contentName);
    } else {
        Value *value = value_SExpr();
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
                value->cell[index] = value_Byte(fileBuffer[i]);
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

Value *
value_WriteContent(char *contentName, Value *data)
{
    FILE *fp = fopen(contentName, "w");
    if (fp == NULL) {
        // TODO: insert interest issuance here
        return value_Error("Unable to open file %s", contentName);
    } else {
        for (int i = 0; i < data->count; i++) {
            fputc(data->cell[i]->byte, fp);
        }
        fclose(fp);
        return value_SExpr();
    }
}

void
value_Delete(Value *value)
{
    switch (value->type) {
        case CoolValue_Integer:
            mpz_clear(value->bignumber);
            break;
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
                value_Delete(value->cell[i]);
            }
            free(value->cell);
            break;
        case CoolValue_Function:
            if (value->builtin == NULL) {
                environment_Delete(value->env);
                value_Delete(value->formals);
                value_Delete(value->body);
            }
            break;
    }

    free(value);
}

void
value_PrintExpr(FILE *out, Value *value, char *open, char *close)
{
    fprintf(out, "%s", open);
    for (int i = 0; i < value->count; i++) {
        value_Print(out, value->cell[i]);
        if (i != (value->count - 1)) {
            fprintf(out, " ");
        }
    }
    fprintf(out, "%s", close);
}

void
value_Print(FILE* out, Value *value)
{
    switch (value->type) {
        case CoolValue_Integer: {
            mpz_out_str(out,10,value->bignumber);
            // char *repr = mpz_get_str(NULL, 10, value->bignumber);
            // fprintf(out, "%s", repr);
            // free(repr);
            break;
        }
        case CoolValue_Double:
            fprintf(out, "%f", value->fpnumber);
            break;
        case CoolValue_Byte:
            fprintf(out, "%x", value->byte);
            break;
        case CoolValue_String:
            fprintf(out, "'%s'", value->string);
            break;
        case CoolValue_Error:
            fprintf(out, "Error: %s", value->errorString);
            break;
        case CoolValue_Symbol:
            fprintf(out, "%s", value->symbolString);
            break;
        case CoolValue_Sexpr:
            value_PrintExpr(out, value, "(", ")");
            break;
        case CoolValue_Qexpr:
            value_PrintExpr(out, value, "{", "}");
            break;
        case CoolValue_Function:
            if (value->builtin != NULL) {
                fprintf(out, "<builtin>");
            } else {
                fprintf(out, "(\\ ");
                value_Print(out, value->formals);
                fprintf(out, " ");
                value_Print(out, value->body);
                fprintf(out, " ");
            }
            break;
    }
}

void
value_Println(FILE *out, Value *value)
{
    value_Print(out, value);
    fprintf(out, "\n");
}

Value *
value_Copy(Value *in)
{
    Value *copy = (Value *) malloc(sizeof(Value));
    copy->type = in->type;

    switch (in->type) {
        case CoolValue_Function:
            if (in->builtin != NULL) {
                copy->builtin = in->builtin;
            } else {
                copy->builtin = NULL;
                copy->env = environment_Copy(in->env);
                copy->formals = value_Copy(in->formals);
                copy->body = value_Copy(in->body);
            }
            break;
        case CoolValue_Integer:
            mpz_init(copy->bignumber);
            mpz_set(copy->bignumber, in->bignumber);
        case CoolValue_Byte:
            copy->byte = in->byte;
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
                copy->cell[i] = value_Copy(in->cell[i]);
            }
            break;
    }

    return copy;
}

Value *
value_AddCell(Value *value, Value *x)
{
    value->count++;
    value->cell = realloc(value->cell, sizeof(Value *) * value->count);
    value->cell[value->count - 1] = x;
    return value;
}

Value *
value_ReadInteger(mpc_ast_t* t)
{
    Value *val = value_Integer(0);
    mpz_init_set_str(val->bignumber, t->contents, 10); // assumes decimal notation
    return val;
}

Value *
value_ReadDouble(mpc_ast_t* t)
{
    errno = 0;
    double x = atof(t->contents);
    return value_Double(x);
}

Value *
value_ReadNumber(mpc_ast_t* t)
{
    if (strstr(t->contents, ".")) {
        return value_ReadDouble(t);
    } else {
        return value_ReadInteger(t);
    }
}

Value *
value_ReadString(mpc_ast_t *t)
{
    t->contents[strlen(t->contents) - 1] = '\0';
    char *unescaped = (char *) malloc(strlen(t->contents + 1) + 1); // quote at start and end
    strcpy(unescaped, t->contents + 1);
    unescaped = mpcf_unescape(unescaped); // drop quotes
    Value *string = value_String(unescaped);
    free(unescaped);
    return string;
}

Value *
value_Read(mpc_ast_t *t)
{
    if (strstr(t->tag, "number")) {
        return value_ReadNumber(t);
    }
    if (strstr(t->tag, "symbol")) {
        return value_Symbol(t->contents);
    }
    if (strstr(t->tag, "string")) {
        return value_ReadString(t);
    }

    Value *x = NULL;
    if (strcmp(t->tag, ">") == 0) {
        x = value_SExpr();
    }
    if (strstr(t->tag, "sexpr")) {
        x = value_SExpr();
    }
    if (strstr(t->tag, "qexpr")) {
        x = value_QExpr();
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
        x = value_AddCell(x, value_Read(t->children[i]));
    }

    return x;
}

Value *
value_Pop(Value *value, int i)
{
    Value *x = value->cell[i];
    memmove(&value->cell[i], &value->cell[i + 1], sizeof(Value *) * (value->count - 1));
    value->count--;
    value->cell = realloc(value->cell, sizeof(Value *) * value->count);
    return x;
}

Value *
value_Take(Value *value, int i)
{
    Value *x = value_Pop(value, i);
    value_Delete(value);
    return x;
}

Value *
value_Join(Value *x, Value *y)
{
    while (y->count > 0) {
        x = value_AddCell(x, value_Pop(y, 0));
    }
    value_Delete(y);
    return x;
}

Value *
value_Call(Environment *env, Value *function, Value *x)
{
    if (function->builtin) {
        Value *val = function->builtin(env, x);
        return val;
    }

    int given = x->count;
    int total = function->formals->count;

    while (x->count > 0) {
        if (function->formals->count == 0) {
            value_Delete(x);
            return value_Error("Function was given too many arguments. Got %d, expected %d", given, total);
        }

        Value *symbol = value_Pop(function->formals, 0);

        if (strcmp(symbol->symbolString, "&") == 0) {
            if (function->formals->count != 1) {
                value_Delete(x);
                return value_Error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
            }

            Value* nsym = value_Pop(function->formals, 0);
            environment_PutKeyValue(function->env, nsym, builtin_List(env, x));
            value_Delete(symbol);
            value_Delete(nsym);
            break;
        }

        Value *value = value_Pop(x, 0);

        environment_PutKeyValue(function->env, symbol, value);

        value_Delete(symbol);
        value_Delete(value);
    }

    value_Delete(x);

    if (function->formals->count > 0 && strcmp(function->formals->cell[0]->symbolString, "&") == 0) {
        if (function->formals->count != 2) {
            return value_Error("Function format invalid. ", "Symbol '&' not followed by single symbol.");
        }

        value_Delete(value_Pop(function->formals, 0));

        Value* sym = value_Pop(function->formals, 0);
        Value* val = value_QExpr();

        environment_PutKeyValue(function->env, sym, val);
        value_Delete(sym);
        value_Delete(val);
    }

    if (function->formals->count == 0) {
        function->env->parent = env;
        Value *newFunctionBody = value_AddCell(value_SExpr(), value_Copy(function->body));
        return builtin_Eval(function->env, newFunctionBody);
    } else {
        return value_Copy(function);
    }
}

Value *
builtin_Lambda(Environment *env, Value *x)
{
    CASSERT_NUM("\\", x, 2);
    CASSERT_TYPE("\\", x, 0, CoolValue_Qexpr);
    CASSERT_TYPE("\\", x, 1, CoolValue_Qexpr);

    for (int i = 0; i < x->cell[0]->count; i++) {
        CASSERT(x, (x->cell[0]->cell[i]->type == CoolValue_Symbol), "Cannot define non-symbol. Got %s, Expected %s",
            value_TypeString(x->cell[0]->cell[i]->type), value_TypeString(CoolValue_Symbol));
    }

    Value *formals = value_Pop(x, 0);
    Value *body = value_Pop(x, 0);
    value_Delete(x);

    Value *lambda = value_Lambda(formals, body);

    return lambda;
}

Value *
builtin_Var(Environment *env, Value *val, char *function)
{
    CASSERT(val, val->count > 0, "Function 'def' expected non-empty variable declaration, got %d", val->count);
    CASSERT(val, val->cell[0]->type == CoolValue_Qexpr, "Function 'def' passed incorrect type, got %s", value_TypeString(val->cell[0]->type));

    Value *symbols = val->cell[0];

    for (int i = 0; i < symbols->count; i++) {
        CASSERT(val, symbols->cell[i]->type == CoolValue_Symbol, "Function 'def' cannot define a non-symbol, got %s", value_TypeString(symbols->cell[i]->type));
    }

    CASSERT(val, symbols->count == val->count - 1, "Function 'def' cannot define incorrect number of values to symbols, got %d", symbols->count);

    for (int i = 0; i < symbols->count; i++) {
        if (strcmp(function, "def") == 0) {
            environment_DefineKeyValue(env, symbols->cell[i], val->cell[i + 1]);
        }

        if (strcmp(function, "=") == 0) {
            environment_PutKeyValue(env, symbols->cell[i], val->cell[i + 1]);
        }
    }

    value_Delete(val);

    return value_SExpr();
}

Value *
builtin_Def(Environment *env, Value *val)
{
    return builtin_Var(env, val, "def");
}

Value *
builtin_Put(Environment *env, Value *val)
{
    return builtin_Var(env, val, "put");
}

Value *
builtin_Head(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'head' passed incorrect type, got %s", value_TypeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    Value *head = value_Take(x, 0); // pull out the list argument
    while (head->count > 1) {
        value_Delete(value_Pop(head, 1)); // delete all elements after the head
    }

    return head;
}

Value *
builtin_Tail(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'head' passed to many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'tail' passed incorrect type, got %s", value_TypeString(x->cell[0]->type));
    CASSERT(x, x->cell[0]->count > 0, "Function 'head' passed {}, got %d", x->cell[0]->count);

    Value *head = value_Take(x, 0);
    value_Delete(value_Pop(head, 0));
    Value *tail = head;

    return tail;
}

Value *
builtin_List(Environment *env, Value *x)
{
    x->type = CoolValue_Qexpr;
    return x;
}

Value *
builtin_Eval(Environment *env, Value *x)
{
    CASSERT(x, x->count == 1, "Function 'eval' passed too many arguments, got %d", x->count);
    CASSERT(x, x->cell[0]->type == CoolValue_Qexpr, "Function 'eval' passed incorrect type, got %s", value_TypeString(x->cell[0]->type));

    // pop the head and evaluate it
    Value *head = value_Take(x, 0);
    head->type = CoolValue_Sexpr;
    Value *evalResult = value_Eval(env, head);

    return evalResult;
}

Value *
builtin_Join(Environment *env, Value *x)
{
    for (int i = 0; i < x->count; i++) {
        CASSERT(x, x->cell[i]->type == CoolValue_Qexpr, "Function 'join' passed incorrect type, got %s", x->cell[i]->type);
    }

    Value *y = value_Pop(x, 0);
    while (x->count > 0) {
        Value *z = value_Pop(x, 0);
        y = value_Join(y, z);
    }

    value_Delete(x);

    return y;
}

int
value_Equal(Value *x, Value *y)
{
    if (x->type != y->type) {
        return 0;
    }

    switch (x->type) {
        case CoolValue_Integer:
            return mpz_cmp(x->bignumber, y->bignumber) == 0;
        case CoolValue_Byte:
            return x->byte == y->byte;
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
                return value_Equal(x->formals, y->formals) && value_Equal(x->body, y->body);
            }
        case CoolValue_Sexpr:
        case CoolValue_Qexpr:
            if (x->count != y->count) {
                return 0;
            }
            for (int i = 0; i < x->count; i++) {
                int valuesEqual = value_Equal(x->cell[i], y->cell[i]);
                if (valuesEqual == 0) { // values not equal
                    return 0;
                }
            }
            return 1; // must have been true, we didn't short circuit
    }

    return 0; // false by default
}

Value *
builtin_DoubleOrder(Environment *env, Value *x, char *operator)
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
        value_Delete(x);
        return value_Error("Error: unexpected operator in 'builtin_Order': %s", operator);
    }

    value_Delete(x);
    return value_Integer(ret);
}

Value *
builtin_IntegerOrder(Environment *env, Value *x, char *operator)
{
    int ret = 0;

    if (strcmp(operator, ">") == 0) {
        ret = mpz_cmp(x->cell[0]->bignumber, x->cell[1]->bignumber) > 0 ? 1 : -1;
    } else if (strcmp(operator, "<") == 0) {
        ret = mpz_cmp(x->cell[0]->bignumber, x->cell[1]->bignumber) < 0 ? 1 : -1;
    } else if (strcmp(operator, ">=") == 0) {
        ret = mpz_cmp(x->cell[0]->bignumber, x->cell[1]->bignumber) >= 0 ? 1 : -1;
    } else if (strcmp(operator, "<=") == 0) {
        ret = mpz_cmp(x->cell[0]->bignumber, x->cell[1]->bignumber) <= 0 ? 1 : -1;
    } else {
        value_Delete(x);
        return value_Error("Error: unexpected operator in 'builtin_Order': %s", operator);
    }

    value_Delete(x);
    return value_Integer(ret);
}

Value *
builtin_Order(Environment *env, Value *x, char *operator)
{
    CASSERT_NUM(operator, x, 2);
    if (x->cell[1]->type == CoolValue_Integer || x->cell[1]->type == CoolValue_Byte) {
        CASSERT_TYPE(operator, x, 0, CoolValue_Integer | CoolValue_Byte);
        CASSERT_TYPE(operator, x, 1, CoolValue_Integer | CoolValue_Byte);
        return builtin_IntegerOrder(env, x, operator);
    } else {
        CASSERT_TYPE(operator, x, 0, CoolValue_Double);
        CASSERT_TYPE(operator, x, 1, CoolValue_Double);
        return builtin_DoubleOrder(env, x, operator);
    }
}

Value *
builtin_gt(Environment *env, Value *x)
{
    return builtin_Order(env, x, ">");
}

Value *
builtin_lt(Environment *env, Value *x)
{
    return builtin_Order(env, x, "<");
}

Value *
builtin_gte(Environment *env, Value *x)
{
    return builtin_Order(env, x, ">=");
}

Value *
builtin_lte(Environment *env, Value *x)
{
    return builtin_Order(env, x, "<=");
}

Value *
builtin_Compare(Environment *env, Value *x, char *operator) {
    CASSERT_NUM(operator, x, 2);
    int ret = 0;

    if (strcmp(operator, "==") == 0) {
        ret = value_Equal(x->cell[0], x->cell[1]);
    } else if (strcmp(operator, "!=") == 0) {
        ret = !value_Equal(x->cell[0], x->cell[1]);
    } else {
        // TODO: trap.
    }

    value_Delete(x);
    Value *result = value_Integer(ret);

    return result;
}

Value *
builtin_Equal(Environment *env, Value *x)
{
    return builtin_Compare(env, x, "==");
}

Value *
builtin_NotEqual(Environment *env, Value *x)
{
    return builtin_Compare(env, x, "!=");
}

Value *
builtin_If(Environment *env, Value *x)
{
    CASSERT_NUM("if", x, 3);
    CASSERT_TYPE("if", x, 0, CoolValue_Integer);
    CASSERT_TYPE("if", x, 1, CoolValue_Qexpr);
    CASSERT_TYPE("if", x, 2, CoolValue_Qexpr);

    Value *y;
    x->cell[1]->type = CoolValue_Sexpr;
    x->cell[2]->type = CoolValue_Sexpr;

    if (mpz_sgn(x->cell[0]->bignumber) > 0) {
        y = value_Eval(env, value_Pop(x, 1));
    } else {
        y = value_Eval(env, value_Pop(x, 2));
    }

    value_Delete(x);
    return y;
}

Value *
builtin_Operator(Environment *env, Value *expr, char* op)
{
    for (int i = 0; i < expr->count; i++) {
        if (expr->cell[i]->type != CoolValue_Integer && expr->cell[i]->type != CoolValue_Double) {
            value_Delete(expr);
            return value_Error("Cannot operate on a non-number, got type %s", value_TypeString(expr->cell[i]->type));
        }
    }

    Value *x = value_Pop(expr, 0);
    if ((strcmp(op, "-") == 0) && expr->count == 0) {
        mpz_t zero;
        mpz_init(zero);
        mpz_add_ui(zero, zero, 0);
        mpz_sub(x->bignumber, zero, x->bignumber);
    }

    while (expr->count > 0) {
        Value *y = value_Pop(expr, 0);
        if (strcmp(op, "+") == 0) {
            mpz_add(x->bignumber, x->bignumber, y->bignumber);
        }
        if (strcmp(op, "-") == 0) {
            mpz_sub(x->bignumber, x->bignumber, y->bignumber);
        }
        if (strcmp(op, "*") == 0) {
            mpz_mul(x->bignumber, x->bignumber, y->bignumber);
        }
        if (strcmp(op, "/") == 0) {
            if (mpz_sgn(y->bignumber) == 0) {
                value_Delete(x);
                value_Delete(y);
                x = value_Error("Division by zero.");
                break;
            }
            mpz_div(x->bignumber, x->bignumber, y->bignumber);
        }

        // TODO: AND mpz_and
        // TODO: SETBIT mpz_setbit
        // TODO: CLRBIT mpz_clrbit
        // TODO: TSTBIT mpz_tstbit
        // TODO: FLIPBIT mpz_combit

        if (strcmp(op, "^") == 0) {
            mpz_xor(x->bignumber, x->bignumber, y->bignumber);
        }
        if (strcmp(op, "**") == 0) {
            mpz_pow_ui(x->bignumber, x->bignumber, mpz_get_ui(y->bignumber));
        }
        value_Delete(y);
    }

    value_Delete(expr);
    return x;
}

Value *
builtin_add(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "+");
}

Value *
builtin_sub(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "-");
}

Value *
builtin_mul(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "*");
}

Value *
builtin_div(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "/");
}

Value *
builtin_xor(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "^");
}

Value *
builtin_exp(Environment *env, Value *val)
{
    return builtin_Operator(env, val, "**");
}

Value *
builtin_Load(Environment *env, Value *x)
{
    CASSERT_NUM("load", x, 1);
    CASSERT_TYPE("load", x, 0, CoolValue_String);

    mpc_result_t result;
    if (mpc_parse_contents(x->cell[0]->string, Cool, &result)) {

        Value *expr = value_Read(result.output);
        mpc_ast_delete(result.output);

        while (expr->count > 0) {
            Value *top = value_Pop(expr, 0);
            Value *y = value_Eval(env, top);
            if (y->type == CoolValue_Error) {
                value_Println(stdout, y);
            }
            value_Delete(y);
        }

        value_Delete(expr);
        value_Delete(x);

        return value_SExpr();
    } else { // parsing error
        char *errorMessage = mpc_err_string(result.error);
        mpc_err_delete(result.error);

        Value *error = value_Error("Could not load library %s", errorMessage);
        free(errorMessage);
        value_Delete(x);

        return error;
    }
}

Value *
builtin_Print(Environment *env, Value *x)
{
    for (int i = 0; i < x->count; i++) {
        value_Print(stdout, x->cell[i]);
        fprintf(stdout, " ");
    }
    fprintf(stdout, "\n");
    value_Delete(x);

    return value_SExpr();
}

Value *
builtin_Error(Environment *env, Value *x)
{
    CASSERT_NUM("error", x, 1);
    CASSERT_TYPE("error", x, 0, CoolValue_String);
    Value *error = value_Error(x->cell[0]->string);
    value_Delete(x);
    return error;
}

Value *
builtin_Read(Environment *env, Value *x)
{
    CASSERT_NUM("read", x, 1);
    CASSERT_TYPE("error", x, 0, CoolValue_String);
    Value *byteList = value_ReadContent(x->cell[0]->string);
    value_Delete(x);
    return byteList;
}

Value *
builtin_Write(Environment *env, Value *x)
{
    CASSERT_NUM("write", x, 2);
    CASSERT_TYPE("write", x, 0, CoolValue_String);
    CASSERT_TYPE("write", x, 1, CoolValue_Sexpr);
    Value *bytesWritten = value_WriteContent(x->cell[0]->string, x->cell[1]);
    value_Delete(x);
    return bytesWritten;
}

Value *
value_EvaluateExpressionWrapper(EvaluateWrapper *wrapper)
{
    Value *result = value_EvaluateExpression(wrapper->env, wrapper->param);
    value_Println(stdout, result);
    return result;
}

Value *
builtin_Run(Environment *env, Value *x)
{
    pthread_t runner;
    EvaluateWrapper *wrapper = (EvaluateWrapper *) malloc(sizeof(EvaluateWrapper));
    wrapper->env = environment_Copy(env);
    wrapper->param = value_Copy(x);
    if (pthread_create(&runner, NULL, (void *(*)(void *)) value_EvaluateExpressionWrapper, wrapper)) {
        return value_Error("Error creating thread");
    }
    return value_SExpr();
}

Value *
builtin_Spawn(Environment *env, Value *x)
{
    return NULL;
}

void
environment_AddBuiltin(Environment *env, char *name, cbuiltin function)
{
    Value *key = value_Symbol(name);
    Value *value = value_Builtin(function);
    environment_PutKeyValue(env, key, value);
    value_Delete(key);
    value_Delete(value);
}

void
environment_AddBuiltinFunctions(Environment *env)
{
    environment_AddBuiltin(env, "load", builtin_Load);
    environment_AddBuiltin(env, "print", builtin_Print);
    environment_AddBuiltin(env, "error", builtin_Error);

    environment_AddBuiltin(env, "read", builtin_Read);
    environment_AddBuiltin(env, "write", builtin_Write);
    environment_AddBuiltin(env, "run", builtin_Run);
    environment_AddBuiltin(env, "spawn", builtin_Spawn);

    environment_AddBuiltin(env, "\\", builtin_Lambda);
    environment_AddBuiltin(env, "def", builtin_Def);
    environment_AddBuiltin(env, "=", builtin_Put);

    environment_AddBuiltin(env, "if", builtin_If);
    environment_AddBuiltin(env, "==", builtin_Equal);
    environment_AddBuiltin(env, "!=", builtin_NotEqual);
    environment_AddBuiltin(env, ">", builtin_gt);
    environment_AddBuiltin(env, "<", builtin_lt);
    environment_AddBuiltin(env, ">=", builtin_gte);
    environment_AddBuiltin(env, "<=", builtin_lte);

    environment_AddBuiltin(env, "list", builtin_List);
    environment_AddBuiltin(env, "eval", builtin_Eval);
    environment_AddBuiltin(env, "join", builtin_Join);
    environment_AddBuiltin(env, "head", builtin_Head);
    environment_AddBuiltin(env, "tail", builtin_Tail);

    environment_AddBuiltin(env, "+", builtin_add);
    environment_AddBuiltin(env, "-", builtin_sub);
    environment_AddBuiltin(env, "*", builtin_mul);
    environment_AddBuiltin(env, "/", builtin_div);
    environment_AddBuiltin(env, "^", builtin_xor);
    environment_AddBuiltin(env, "**", builtin_exp);
}

Value *
value_EvaluateExpression(Environment *env, Value *value)
{
    // Evaluate each expression
    for (int i = 0; i < value->count; i++) {
        value->cell[i] = value_Eval(env, value->cell[i]);
    }

    // If one is an error, take it and return it
    for (int i = 0; i < value->count; i++) {
        if (value->cell[i]->type == CoolValue_Error) {
            return value_Take(value, i);
        }
    }

    if (value->count == 0) {
        return value;
    }

    if (value->count == 1) {
        return value_Take(value, 0);
    }

    Value *funcSymbol = value_Pop(value, 0);
    if (funcSymbol->type != CoolValue_Function) {
        value_Delete(funcSymbol);
        value_Delete(value);
        return value_Error("S-expression does not start with a function, got type %s", value_TypeString(funcSymbol->type));
    }

    Value *result = value_Call(env, funcSymbol, value);
    value_Delete(funcSymbol);

    return result;
}

Value *
value_Eval(Environment* env, Value *value)
{
    if (value->type == CoolValue_Symbol) {
        Value *result = environment_Get(env, value);
        value_Delete(value);
        return result;
    } else if (value->type == CoolValue_Sexpr) {
        return value_EvaluateExpression(env, value);
    }
    return value;
}
