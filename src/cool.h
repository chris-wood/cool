#ifndef COOL_H_
#define COOL_H_

#include "mpc.h"

// Parsing grammars
mpc_parser_t* Number;
mpc_parser_t* Symbol;
mpc_parser_t* String;
mpc_parser_t* Comment;
mpc_parser_t* Sexpr;
mpc_parser_t* Qexpr;
mpc_parser_t* Expr;
mpc_parser_t* Cool;

// TODO: figure out where to put this
Value *builtin_Load(Environment *env, Value *x);

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

#define CASSERT(args, cond, fmt, ...) \
    if (!(cond)) { \
        Value *error = value_Error(fmt, ##__VA_ARGS__); \
        value_Delete(args); \
        return error; \
    }

#endif 