//===----------------------------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is dual licensed under the MIT and the University of Illinois Open
// Source Licenses. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// <regex>

// class regex_token_iterator<BidirectionalIterator, charT, traits>

// regex_token_iterator& operator++(int);

#include <regex>
#include <cassert>

int main()
{
    {
        std::regex phone_numbers("\\d{3}-\\d{4}");
        const char phone_book[] = "start 555-1234, 555-2345, 555-3456 end";
        std::cregex_token_iterator i(std::begin(phone_book), std::end(phone_book),
                                     phone_numbers, -1);
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "start ");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == ", ");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == ", ");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == " end");
        i++;
        assert(i == std::cregex_token_iterator());
    }
    {
        std::regex phone_numbers("\\d{3}-\\d{4}");
        const char phone_book[] = "start 555-1234, 555-2345, 555-3456 end";
        std::cregex_token_iterator i(std::begin(phone_book), std::end(phone_book),
                                     phone_numbers);
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "555-1234");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "555-2345");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "555-3456");
        i++;
        assert(i == std::cregex_token_iterator());
    }
    {
        std::regex phone_numbers("\\d{3}-(\\d{4})");
        const char phone_book[] = "start 555-1234, 555-2345, 555-3456 end";
        std::cregex_token_iterator i(std::begin(phone_book), std::end(phone_book),
                                     phone_numbers, 1);
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "1234");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "2345");
        i++;
        assert(i != std::cregex_token_iterator());
        assert(i->str() == "3456");
        i++;
        assert(i == std::cregex_token_iterator());
    }
}
