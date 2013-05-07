//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <unordered_set>

// template <class Value, class Hash = hash<Value>, class Pred = equal_to<Value>,
//           class Alloc = allocator<Value>>
// class unordered_multiset

// void rehash(size_type n);

#include <unordered_set>
#include <cassert>

void test(const std::unordered_multiset<int>& c)
{
    assert(c.size() == 6);
    assert(c.count(1) == 2);
    assert(c.count(2) == 2);
    assert(c.count(3) == 1);
    assert(c.count(4) == 1);
}

int main()
{
    {
        typedef std::unordered_multiset<int> C;
        typedef int P;
        P a[] =
        {
            P(1),
            P(2),
            P(3),
            P(4),
            P(1),
            P(2)
        };
        C c(a, a + sizeof(a)/sizeof(a[0]));
        test(c);
        assert(c.bucket_count() >= 7);
        c.rehash(3);
        assert(c.bucket_count() == 7);
        test(c);
        c.max_load_factor(2);
        c.rehash(3);
        assert(c.bucket_count() == 3);
        test(c);
        c.rehash(31);
        assert(c.bucket_count() == 31);
        test(c);
    }
}
