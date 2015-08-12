#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../cool.c"
#include "../mpc.h"

static void test_Value_longInteger(void **state) {
    long input = 10;
    Value *value = Value_longInteger(input);
    assert_true(value->type == CoolValue_LongInteger);
    assert_true(value->number == input);
    assert_true(value->cell == NULL);
}

static void test_Value_double(void **state) {
    double input = 1337.0;
    Value *value = Value_double(input);
    assert_true(value->type == CoolValue_Double);
    assert_true(value->fpnumber == input);
    assert_true(value->cell == NULL);
}

static void test_Value_byte(void **state) {
    uint8_t input = (uint8_t) 0xBEEF;
    Value *value = Value_byte(input);
    assert_true(value->type == CoolValue_Byte);
    assert_true(value->number == input);
    assert_true(value->cell == NULL);
}

static void test_Value_string(void **state) {
    char *input = "COOL";
    Value *value = Value_string(input);
    assert_true(value->type == CoolValue_String);
    assert_true(memcmp(input, value->string, strlen(input)) == 0);
    assert_true(value->cell == NULL);
}

int 
main(int argc, char **argv) 
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_Value_longInteger),
        cmocka_unit_test(test_Value_double),
        cmocka_unit_test(test_Value_byte),
        cmocka_unit_test(test_Value_string)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
