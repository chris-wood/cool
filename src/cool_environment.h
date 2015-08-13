#ifndef COOL_ENVIRONMENT_H_
#define COOL_ENVIRONMENT_H_

#include "cool.h"

struct cenv;
typedef struct cenv Environment;

Environment *environment_Create();
void environment_AddBuiltinFunctions(Environment *env);
void environment_Delete(Environment *env);

#endif // COOL_ENVIRONMENT_H_