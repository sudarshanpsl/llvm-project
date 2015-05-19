//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// test sized operator delete[] replacement.

// Note that sized delete operator definitions below are simply ignored
// when sized deallocation is not supported, e.g., prior to C++14.

// UNSUPPORTED: sanitizer-new-delete

// NOTE: -fsized-deallocation was only added in clang 3.7
// XFAIL: clang-3.4, clang-3.5, clang-3.6

// NOTE: -fsized-deallocation was only added to GCC in 5.1.
// XFAIL: gcc-4.7, gcc-4.8, gcc-4.9

// RUN: %build -fsized-deallocation
// RUN: %run

#if !defined(__cpp_sized_deallocation)
# error __cpp_sized_deallocation should be defined
#endif

#if !(__cpp_sized_deallocation >= 201309L)
# error expected __cpp_sized_deallocation >= 201309L
#endif

#include <new>
#include <cstddef>
#include <cstdlib>
#include <cassert>

int unsized_delete_called = 0;
int unsized_delete_nothrow_called = 0;
int sized_delete_called = 0;

void operator delete[](void* p) throw()
{
    ++unsized_delete_called;
    std::free(p);
}

void operator delete[](void* p, const std::nothrow_t&) throw()
{
    ++unsized_delete_nothrow_called;
    std::free(p);
}

void operator delete[](void* p, std::size_t) noexcept
{
    ++sized_delete_called;
    std::free(p);
}

// NOTE: Use a class with a non-trivial destructor as the test type in order
// to ensure the correct overload is called.
// C++14 5.3.5 [expr.delete]p10
// - If the type is complete and if, for the second alternative (delete array)
//   only, the operand is a pointer to a class type with a non-trivial
//   destructor or a (possibly multi-dimensional) array thereof, the function
//   with two parameters is selected.
// - Otherwise, it is unspecified which of the two deallocation functions is
//   selected.
struct A { ~A() {} };

int main()
{
    A* x = new A[3];
    assert(0 == unsized_delete_called);
    assert(0 == unsized_delete_nothrow_called);
    assert(0 == sized_delete_called);

    delete [] x;
    assert(0 == unsized_delete_called);
    assert(0 == unsized_delete_nothrow_called);
    assert(1 == sized_delete_called);
}
