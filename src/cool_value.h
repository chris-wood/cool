#ifndef COOL_VALUE_H
#define COOL_VALUE_H

#include "cool.h"

struct cval;
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

CoolValue value_GetType(Value *value);
Value *value_AddCell(Value *value, Value *x);
Value *value_String(char *str);
Value *value_SExpr();
void value_Println(Value *value);
void value_Delete(Value *value);
Value *value_Eval(Environment *env, Value *value);
Value *value_Read(mpc_ast_t *t);

#endif // COOL_VALUE_H