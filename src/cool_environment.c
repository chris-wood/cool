#include "cool_environment.h"
#include "cool_value.h"

struct cenv {
    Environment *parent;
    int count;
    char **symbols;
    struct cval **values;
}; 

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
Environment_delete(Environment *env) 
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
    // environment_AddBuiltin(env, "write", builtin_write);

    environment_AddBuiltin(env, "\\", builtin_lambda);
    environment_AddBuiltin(env, "def", builtin_def);
    environment_AddBuiltin(env, "=", builtin_put);

    environment_AddBuiltin(env, "if", builtin_if);
    environment_AddBuiltin(env, "==", builtin_equal);
    environment_AddBuiltin(env, "!=", builtin_notequal);
    environment_AddBuiltin(env, ">", builtin_gt);
    environment_AddBuiltin(env, "<", builtin_lt);
    environment_AddBuiltin(env, ">=", builtin_gte);
    environment_AddBuiltin(env, "<=", builtin_lte);

    environment_AddBuiltin(env, "list", builtin_list);
    environment_AddBuiltin(env, "eval", builtin_eval);
    environment_AddBuiltin(env, "join", builtin_join);
    environment_AddBuiltin(env, "head", builtin_head);
    environment_AddBuiltin(env, "tail", builtin_tail);

    environment_AddBuiltin(env, "+", builtin_add);
    environment_AddBuiltin(env, "-", builtin_sub);
    environment_AddBuiltin(env, "*", builtin_mul);
    environment_AddBuiltin(env, "/", builtin_div);
    
    // caw: xor, exponentation, etc
}