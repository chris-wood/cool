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
    CoolValue_Integer = 1,
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

CoolValue value_GetType(Value *value);
void environment_AddBuiltinFunctions(Environment *env);
Environment *environment_Create();
Value *value_AddCell(Value *value, Value *x);
Value *value_String(char *str);
Value *value_SExpr();
Value *builtin_Load(Environment *env, Value *x);
void value_Println(Value *value);
void value_Delete(Value *value);
void environment_Delete(Environment *env);
Value *value_Eval(Environment *env, Value *value);
Value *value_Read(mpc_ast_t *t);

#endif
