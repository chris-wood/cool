#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../list.c"

static void test_list_Create(void **state) {
    // assert_true(env != NULL);
}

int
main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_list_Create)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
