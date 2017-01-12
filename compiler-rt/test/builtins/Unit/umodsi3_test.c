//===-- umodsi3_test.c - Test __umodsi3 -----------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file tests __umodsi3 for the compiler_rt library.
//
//===----------------------------------------------------------------------===//

#include "int_lib.h"
#include <stdio.h>

// Returns: a % b

COMPILER_RT_ABI su_int __umodsi3(su_int a, su_int b);

int test__umodsi3(su_int a, su_int b, su_int expected_r)
{
    su_int r = __umodsi3(a, b);
    if (r != expected_r)
        printf("error in __umodsi3: %X %% %X = %X, expected %X\n",
               a, b, r, expected_r);
    return r != expected_r;
}

su_int tests[][4] =
{
{0x00000000, 0x00000001, 0x00000000},
{0x00000000, 0x00000002, 0x00000000},
{0x00000000, 0x00000003, 0x00000000},
{0x00000000, 0x00000010, 0x00000000},
{0x00000000, 0x078644FA, 0x00000000},
{0x00000000, 0x0747AE14, 0x00000000},
{0x00000000, 0x7FFFFFFF, 0x00000000},
{0x00000000, 0x80000000, 0x00000000},
{0x00000000, 0xFFFFFFFD, 0x00000000},
{0x00000000, 0xFFFFFFFE, 0x00000000},
{0x00000000, 0xFFFFFFFF, 0x00000000},
{0x00000001, 0x00000001, 0x00000000},
{0x00000001, 0x00000002, 0x00000001},
{0x00000001, 0x00000003, 0x00000001},
{0x00000001, 0x00000010, 0x00000001},
{0x00000001, 0x078644FA, 0x00000001},
{0x00000001, 0x0747AE14, 0x00000001},
{0x00000001, 0x7FFFFFFF, 0x00000001},
{0x00000001, 0x80000000, 0x00000001},
{0x00000001, 0xFFFFFFFD, 0x00000001},
{0x00000001, 0xFFFFFFFE, 0x00000001},
{0x00000001, 0xFFFFFFFF, 0x00000001},
{0x00000002, 0x00000001, 0x00000000},
{0x00000002, 0x00000002, 0x00000000},
{0x00000002, 0x00000003, 0x00000002},
{0x00000002, 0x00000010, 0x00000002},
{0x00000002, 0x078644FA, 0x00000002},
{0x00000002, 0x0747AE14, 0x00000002},
{0x00000002, 0x7FFFFFFF, 0x00000002},
{0x00000002, 0x80000000, 0x00000002},
{0x00000002, 0xFFFFFFFD, 0x00000002},
{0x00000002, 0xFFFFFFFE, 0x00000002},
{0x00000002, 0xFFFFFFFF, 0x00000002},
{0x00000003, 0x00000001, 0x00000000},
{0x00000003, 0x00000002, 0x00000001},
{0x00000003, 0x00000003, 0x00000000},
{0x00000003, 0x00000010, 0x00000003},
{0x00000003, 0x078644FA, 0x00000003},
{0x00000003, 0x0747AE14, 0x00000003},
{0x00000003, 0x7FFFFFFF, 0x00000003},
{0x00000003, 0x80000000, 0x00000003},
{0x00000003, 0xFFFFFFFD, 0x00000003},
{0x00000003, 0xFFFFFFFE, 0x00000003},
{0x00000003, 0xFFFFFFFF, 0x00000003},
{0x00000010, 0x00000001, 0x00000000},
{0x00000010, 0x00000002, 0x00000000},
{0x00000010, 0x00000003, 0x00000001},
{0x00000010, 0x00000010, 0x00000000},
{0x00000010, 0x078644FA, 0x00000010},
{0x00000010, 0x0747AE14, 0x00000010},
{0x00000010, 0x7FFFFFFF, 0x00000010},
{0x00000010, 0x80000000, 0x00000010},
{0x00000010, 0xFFFFFFFD, 0x00000010},
{0x00000010, 0xFFFFFFFE, 0x00000010},
{0x00000010, 0xFFFFFFFF, 0x00000010},
{0x078644FA, 0x00000001, 0x00000000},
{0x078644FA, 0x00000002, 0x00000000},
{0x078644FA, 0x00000003, 0x00000000},
{0x078644FA, 0x00000010, 0x0000000A},
{0x078644FA, 0x078644FA, 0x00000000},
{0x078644FA, 0x0747AE14, 0x003E96E6},
{0x078644FA, 0x7FFFFFFF, 0x078644FA},
{0x078644FA, 0x80000000, 0x078644FA},
{0x078644FA, 0xFFFFFFFD, 0x078644FA},
{0x078644FA, 0xFFFFFFFE, 0x078644FA},
{0x078644FA, 0xFFFFFFFF, 0x078644FA},
{0x0747AE14, 0x00000001, 0x00000000},
{0x0747AE14, 0x00000002, 0x00000000},
{0x0747AE14, 0x00000003, 0x00000002},
{0x0747AE14, 0x00000010, 0x00000004},
{0x0747AE14, 0x078644FA, 0x0747AE14},
{0x0747AE14, 0x0747AE14, 0x00000000},
{0x0747AE14, 0x7FFFFFFF, 0x0747AE14},
{0x0747AE14, 0x80000000, 0x0747AE14},
{0x0747AE14, 0xFFFFFFFD, 0x0747AE14},
{0x0747AE14, 0xFFFFFFFE, 0x0747AE14},
{0x0747AE14, 0xFFFFFFFF, 0x0747AE14},
{0x7FFFFFFF, 0x00000001, 0x00000000},
{0x7FFFFFFF, 0x00000002, 0x00000001},
{0x7FFFFFFF, 0x00000003, 0x00000001},
{0x7FFFFFFF, 0x00000010, 0x0000000F},
{0x7FFFFFFF, 0x078644FA, 0x00156B65},
{0x7FFFFFFF, 0x0747AE14, 0x043D70AB},
{0x7FFFFFFF, 0x7FFFFFFF, 0x00000000},
{0x7FFFFFFF, 0x80000000, 0x7FFFFFFF},
{0x7FFFFFFF, 0xFFFFFFFD, 0x7FFFFFFF},
{0x7FFFFFFF, 0xFFFFFFFE, 0x7FFFFFFF},
{0x7FFFFFFF, 0xFFFFFFFF, 0x7FFFFFFF},
{0x80000000, 0x00000001, 0x00000000},
{0x80000000, 0x00000002, 0x00000000},
{0x80000000, 0x00000003, 0x00000002},
{0x80000000, 0x00000010, 0x00000000},
{0x80000000, 0x078644FA, 0x00156B66},
{0x80000000, 0x0747AE14, 0x043D70AC},
{0x80000000, 0x7FFFFFFF, 0x00000001},
{0x80000000, 0x80000000, 0x00000000},
{0x80000000, 0xFFFFFFFD, 0x80000000},
{0x80000000, 0xFFFFFFFE, 0x80000000},
{0x80000000, 0xFFFFFFFF, 0x80000000},
{0xFFFFFFFD, 0x00000001, 0x00000000},
{0xFFFFFFFD, 0x00000002, 0x00000001},
{0xFFFFFFFD, 0x00000003, 0x00000001},
{0xFFFFFFFD, 0x00000010, 0x0000000D},
{0xFFFFFFFD, 0x078644FA, 0x002AD6C9},
{0xFFFFFFFD, 0x0747AE14, 0x01333341},
{0xFFFFFFFD, 0x7FFFFFFF, 0x7FFFFFFE},
{0xFFFFFFFD, 0x80000000, 0x7FFFFFFD},
{0xFFFFFFFD, 0xFFFFFFFD, 0x00000000},
{0xFFFFFFFD, 0xFFFFFFFE, 0xFFFFFFFD},
{0xFFFFFFFD, 0xFFFFFFFF, 0xFFFFFFFD},
{0xFFFFFFFE, 0x00000001, 0x00000000},
{0xFFFFFFFE, 0x00000002, 0x00000000},
{0xFFFFFFFE, 0x00000003, 0x00000002},
{0xFFFFFFFE, 0x00000010, 0x0000000E},
{0xFFFFFFFE, 0x078644FA, 0x002AD6CA},
{0xFFFFFFFE, 0x0747AE14, 0x01333342},
{0xFFFFFFFE, 0x7FFFFFFF, 0x00000000},
{0xFFFFFFFE, 0x80000000, 0x7FFFFFFE},
{0xFFFFFFFE, 0xFFFFFFFD, 0x00000001},
{0xFFFFFFFE, 0xFFFFFFFE, 0x00000000},
{0xFFFFFFFE, 0xFFFFFFFF, 0xFFFFFFFE},
{0xFFFFFFFF, 0x00000001, 0x00000000},
{0xFFFFFFFF, 0x00000002, 0x00000001},
{0xFFFFFFFF, 0x00000003, 0x00000000},
{0xFFFFFFFF, 0x00000010, 0x0000000F},
{0xFFFFFFFF, 0x078644FA, 0x002AD6CB},
{0xFFFFFFFF, 0x0747AE14, 0x01333343},
{0xFFFFFFFF, 0x7FFFFFFF, 0x00000001},
{0xFFFFFFFF, 0x80000000, 0x7FFFFFFF},
{0xFFFFFFFF, 0xFFFFFFFD, 0x00000002},
{0xFFFFFFFF, 0xFFFFFFFE, 0x00000001},
{0xFFFFFFFF, 0xFFFFFFFF, 0x00000000}
};

int main()
{
    const unsigned N = sizeof(tests) / sizeof(tests[0]);
    unsigned i;
    for (i = 0; i < N; ++i)
        if (test__umodsi3(tests[i][0], tests[i][1], tests[i][2]))
            return 1;

    return 0;
}
