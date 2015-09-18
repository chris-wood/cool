#ifndef libcool_types_h_
#define libcool_types_h_

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
    CoolValue_Actor,
    CoolValue_Error
} CoolValue;

#endif // libcool_types_h_
