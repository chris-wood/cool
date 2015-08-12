#ifndef COOL_H_
#define COOL_H_

#include "mpc.h"

struct cenv;
struct cval;
typedef struct cenv Environment;
typedef struct cval Value;

typedef enum {
    CoolValueError_DivideByZero,
    CoolValueError_BadOperator,
    CoolValueError_BadNumber
} CoolValueError;

// Primitive types
typedef enum {
    CoolValue_LongInteger = 1,
    CoolValue_Double,
    CoolValue_Byte,
    CoolValue_String,
    CoolValue_Symbol,
    CoolValue_Sexpr,
    CoolValue_Qexpr,
    CoolValue_Function,
    CoolValue_Error
} CoolValue;

// Parsing grammars
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Cool;

CoolValue Value_getType(Value *value);
void Environment_addBuiltinFunctions(Environment *env);
Environment *Environment_new();
Value *Value_add(Value *value, Value *x);
Value *Value_string(char *str);
Value *Value_sexpr();
Value *builtin_load(Environment *env, Value *x);
void Value_println(Value *value);
void Value_delete(Value *value);
void Environment_delete(Environment *env);
Value *Value_eval(Environment *env, Value *value);
Value *Value_read(mpc_ast_t *t);

#endif 