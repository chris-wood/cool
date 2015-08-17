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

    value_Delete(value);
}

static void test_value_Double(void **state) {
    double input = 1337.0;
    Value *value = value_Double(input);

    assert_true(value->type == CoolValue_Double);
    assert_true(value->fpnumber == input);
    assert_true(value->cell == NULL);

    value_Delete(value);
}

static void test_value_Byte(void **state) {
    uint8_t input = (uint8_t) 0xAB;
    Value *value = value_Byte(input);

    assert_true(value->type == CoolValue_Byte);
    assert_true(value->byte == input);
    assert_true(value->cell == NULL);

    value_Delete(value);
}

static void test_value_String(void **state) {
    char *input = "COOL";
    Value *value = value_String(input);

    assert_true(value->type == CoolValue_String);
    assert_true(memcmp(input, value->string, strlen(input)) == 0);
    assert_true(value->cell == NULL);

    value_Delete(value);
}

static void test_value_Print(void **state) {
    long expected = 10L;
    Value *value = value_Integer(expected);
    FILE *fhandle = fopen("test.tmp", "w");
    value_Print(fhandle, value);
    fclose(fhandle);

    fhandle = fopen("test.tmp", "r");
    char *numberString = (char *) malloc(sizeof(long) + 1);
    fgets(numberString, sizeof(long) + 1, fhandle);
    long actual = atol(numberString);

    free(numberString);

    assert_true(actual == expected);

    value_Delete(value);
}

static void test_value_Error(void **state) {
    char *expected = "Testing error messages";
    Value *error = value_Error(expected);
    assert_true(strcmp(expected, error->errorString) == 0);
    value_Delete(error);
}

static void test_value_builtin_add(void **state) {
    long x = 5;
    Value *xv = value_Integer(x);
    long y = 10;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("+");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_add(env, op);
    
    assert_true(zv->number == 15);

    value_Delete(zv);
}

static void test_value_builtin_sub(void **state) {
    long x = 5;
    Value *xv = value_Integer(x);
    long y = 10;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("-");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_sub(env, op);
    
    assert_true(zv->number == -5);

    value_Delete(zv);
}

static void test_value_builtin_mul(void **state) {
    long x = 5;
    Value *xv = value_Integer(x);
    long y = 10;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("*");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_mul(env, op);
    
    assert_true(zv->number == 50);

    value_Delete(zv);
}

static void test_value_builtin_div(void **state) {
    long x = 10;
    Value *xv = value_Integer(x);
    long y = 5;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("/");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_div(env, op);
    
    assert_true(zv->number == 2);

    value_Delete(zv);
}

static void test_value_builtin_xor(void **state) {
    long x = 0xFFFF1111;
    Value *xv = value_Integer(x);
    long y = 0xFFFF1111;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("^");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_xor(env, op);
    
    assert_true(zv->number == 0);

    value_Delete(zv);
}

static void test_value_builtin_exp(void **state) {
    long x = 2;
    Value *xv = value_Integer(x);
    long y = 6;
    Value *yv = value_Integer(y);
    
    Value *op = value_Symbol("**");
    value_AddCell(op, xv);
    value_AddCell(op, yv);

    Environment *env = environment_Create();
    Value *zv = builtin_exp(env, op);
    
    assert_true(zv->number == 64);

    value_Delete(zv);
}

int
main(int argc, char **argv)
{
    const struct CMUnitTest tests[] = {
        cmocka_unit_test(test_value_Integer),
        cmocka_unit_test(test_value_Double),
        cmocka_unit_test(test_value_Byte),
        cmocka_unit_test(test_value_String),
        cmocka_unit_test(test_value_Print),
        cmocka_unit_test(test_value_Error),
        cmocka_unit_test(test_value_builtin_add),
        cmocka_unit_test(test_value_builtin_sub),
        cmocka_unit_test(test_value_builtin_mul),
        cmocka_unit_test(test_value_builtin_div),
        cmocka_unit_test(test_value_builtin_xor),
        cmocka_unit_test(test_value_builtin_exp)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
