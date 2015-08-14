#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../cool.c"
#include "../mpc.h"

// Environment *Environment_new();
// void Environment_delete(Environment *env);
// Value *Environment_get(Environment *env, Value *val);
// void Environment_put(Environment *env, Value* key, Value *val);
// Environment *Environment_copy(Environment *env);
// void Environment_def(Environment *env, Value *key, Value *value)

static void test_environment_Create(void **state) {
    Environment *env = environment_Create();
    assert_true(env != NULL);
    assert_true(env->parent == NULL);
}

static void test_environment_Delete(void **state) {

}

int 
main(int argc, char **argv) 
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_environment_Create),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
