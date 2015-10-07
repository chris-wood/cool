#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../signal.c"

static void test_signal_Create(void **state) {
    // assert_true(env != NULL);
    // TODO
}

int
main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_signal_Create)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
