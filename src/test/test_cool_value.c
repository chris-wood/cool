#include <stdarg.h>
#include <stddef.h>
#include <setjmp.h>
#include <cmocka.h>

#include "../cool.c"
#include "../mpc.h"
#include <gmp.h>

static void test_value_Integer(void **state) {
    long input = 10;
    Value *value = value_Integer(input);

    mpz_t inputVal;
    mpz_init(inputVal);
    mpz_set_ui(inputVal, input);

    assert_true(value->type == CoolValue_Integer);
    assert_true(mpz_cmp(value->bignumber, inputVal) == 0);
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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, 15);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, -5);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, 50);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, 2);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, 0);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

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

    mpz_t result;
    mpz_init(result);
    mpz_set_si(result, 64);

    assert_true(mpz_cmp(zv->bignumber, result) == 0);

    value_Delete(zv);
}

static void test_value_builtin_Head(void **state) {
    Value *list = value_QExpr();
    Value *func = value_Symbol("head");

    long input = 10;
    Value *l1 = value_Integer(input);
    Value *l2 = value_Integer(input + 1);
    Value *l3 = value_Integer(input + 2);

    value_AddCell(list, l1);
    value_AddCell(list, l2);
    value_AddCell(list, l3);

    value_AddCell(func, list);

    Environment *env = environment_Create();
    Value *head = builtin_Head(env, func);

    Value *expectedValue = value_Integer(input);
    Value *expected = value_QExpr();
    value_AddCell(expected, expectedValue);

    assert_true(value_Equal(expected, head));
}

static void test_value_builtin_Tail(void **state) {
    Value *list = value_QExpr();
    Value *func = value_Symbol("head");

    long input = 10;
    Value *l1 = value_Integer(input);
    Value *l2 = value_Integer(input + 1);
    Value *l3 = value_Integer(input + 2);

    value_AddCell(list, l1);
    value_AddCell(list, l2);
    value_AddCell(list, l3);

    value_AddCell(func, list);

    Environment *env = environment_Create();
    Value *tail = builtin_Tail(env, func);

    Value *expectedValue1 = value_Integer(input + 1);
    Value *expectedValue2 = value_Integer(input + 2);
    Value *expected = value_QExpr();
    value_AddCell(expected, expectedValue1);
    value_AddCell(expected, expectedValue2);

    assert_true(value_Equal(expected, tail));
}

static void test_value_builtin_List(void **state) {

}

static void test_value_builtin_Compare(void **state) {
    char *symbol = "==";
    Value *func = value_Symbol(symbol);

    long input = 10;
    Value *l1 = value_Integer(input);
    Value *l2 = value_Integer(input + 1);

    value_AddCell(func, l1);
    value_AddCell(func, l2);

    Environment *env = environment_Create();

    Value *result = builtin_Compare(env, func, symbol);
    // values are not equal, so the result will be 0

    assert_true(result->type == CoolValue_Integer);
    assert_true(mpz_cmp_ui(result->bignumber, 0) == 0);
}

static void test_value_builtin_Equal(void **state) {

}

static void test_value_builtin_NotEqual(void **state) {

}

static void test_value_builtin_If(void **state) {

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
        cmocka_unit_test(test_value_builtin_exp),
        cmocka_unit_test(test_value_builtin_Head),
        cmocka_unit_test(test_value_builtin_Tail),
        cmocka_unit_test(test_value_builtin_List),
        cmocka_unit_test(test_value_builtin_Compare),
        cmocka_unit_test(test_value_builtin_Equal),
        cmocka_unit_test(test_value_builtin_NotEqual),
        cmocka_unit_test(test_value_builtin_If)
    };

    return cmocka_run_group_tests(tests, NULL, NULL);
}
