#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../cool.c"
#include "../mpc.h"

static void test_value_Integer(void **state) {
    long input = 10;
    Value *value = value_Integer(input);
    assert_true(value->type == CoolValue_Integer);
    assert_true(value->number == input);
    assert_true(value->cell == NULL);
}

static void test_value_Double(void **state) {
    double input = 1337.0;
    Value *value = value_Double(input);
    assert_true(value->type == CoolValue_Double);
    assert_true(value->fpnumber == input);
    assert_true(value->cell == NULL);
}

static void test_value_Byte(void **state) {
    uint8_t input = (uint8_t) 0xAB;
    Value *value = value_Byte(input);
    assert_true(value->type == CoolValue_Byte);
    assert_true(value->byte == input);
    assert_true(value->cell == NULL);
}

static void test_value_String(void **state) {
    char *input = "COOL";
    Value *value = value_String(input);
    assert_true(value->type == CoolValue_String);
    assert_true(memcmp(input, value->string, strlen(input)) == 0);
    assert_true(value->cell == NULL);
}

int 
main(int argc, char **argv) 
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_value_Integer),
        cmocka_unit_test(test_value_Double),
        cmocka_unit_test(test_value_Byte),
        cmocka_unit_test(test_value_String)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
