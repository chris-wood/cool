#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

// For coverage
#include "../cool.c"
#include "../mpc.h"

static void test_cval_longInteger(void **state) {
	long input = 10;
    cval *value = cval_longInteger(input);
    assert_true(value->type == CoolValue_LongInteger);
    assert_true(value->number == input);
    assert_true(value->cell == NULL);
}

int 
main(int argc, char **argv) 
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_cval_longInteger),
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
