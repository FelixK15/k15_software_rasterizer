#define K15_SOFTWARE_RASTERIZER_IMPLEMENTATION
#include "../k15_software_rasterizer.hpp"

#include <stdio.h>

#define TEST(fn) {#fn, fn}

typedef int(*test_function_t)(void);
struct test_t
{
    const char*     pTestName;
    test_function_t pTestFunction;
};

int test_matrix_multiplications()
{
    matrix4x4f_t identityMatrix = {};
    matrix4x4f_t scaleMatrix = {};
    _k15_set_identity_matrix4x4f(&identityMatrix);
    _k15_set_identity_matrix4x4f(&scaleMatrix);

    scaleMatrix.m00 = 2.0f;
    scaleMatrix.m11 = 2.0f;
    scaleMatrix.m22 = 2.0f;

    matrix4x4f_t mulMatrix = _k15_mul_matrix4x4f(&identityMatrix, &scaleMatrix);

    return _k15_matrix4x4f_is_equal(&scaleMatrix, &mulMatrix);
}

int test_vector_matrix_multiplications()
{
    matrix4x4f_t matrix = {};
    vector4f_t vector = k15_create_vector4f(0.0f, 0.0f, 0.0f, 1.0f);

    _k15_set_identity_matrix4x4f(&matrix);

    matrix.m03 = 10.0f;
    matrix.m13 = 20.0f;
    matrix.m23 = 30.0f;

    vector4f_t newVector = _k15_mul_vector4_matrix44(&vector, &matrix);

    _k15_set_identity_matrix4x4f(&matrix);
    matrix.m00 = 2.0f;
    matrix.m11 = 2.0f;
    matrix.m22 = 2.0f;

    newVector = _k15_mul_vector4_matrix44(&newVector, &matrix);
    return newVector.x == 20.0f && newVector.y == 40.0f && newVector.z == 60.0f;
}

constexpr test_t tests[] = {
    TEST(test_matrix_multiplications),
    TEST(test_vector_matrix_multiplications)
};

constexpr uint32_t testCount = sizeof(tests) / sizeof(test_t);

int main(int argc, char** argv)
{
    int result = 0;
    for(uint32_t testIndex = 0; testIndex < testCount; ++testIndex)
    {
        const int testResult = tests[testIndex].pTestFunction();
        if(testResult != 1)
        {
            printf("Test '%s' FAILED with '%d'\n", tests[testIndex].pTestName, testResult);
            result = 1;
        }
        else
        {
            printf("Test '%s' SUCCEEDED\n", tests[testIndex].pTestName);
        }
    }

    return result;
}