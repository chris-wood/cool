#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../cool.c"
#include "../mpc.h"

static void test_environment_Create(void **state) {
    Environment *env = environment_Create();
    assert_true(env != NULL);
    assert_true(env->parent == NULL);
}

static void test_environment_Delete(void **state) {
	Environment *env = environment_Create();
	assert_true(env != NULL);
	environment_Delete(env);
}

int 
main(int argc, char **argv) 
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_environment_Create),
        cmocka_unit_test(test_environment_Delete)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
