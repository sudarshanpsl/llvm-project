// RUN: %clang_cc1 -std=c++11 %s -verify

struct string;
namespace std {
  using size_t = decltype(sizeof(int));
}

void operator "" _km(long double); // ok
string operator "" _i18n(const char*, std::size_t); // ok
// FIXME: This should be accepted once we support UCNs
template<char...> int operator "" \u03C0(); // ok, UCN for lowercase pi // expected-error {{expected identifier}}
// FIXME: Accept this as an extension, with a fix-it to add the space
float operator ""E(const char *); // expected-error {{must be '""'}} expected-error {{expected identifier}}
float operator " " B(const char *); // expected-error {{must be '""'}} expected-warning {{hexfloat}}
string operator "" 5X(const char *, std::size_t); // expected-error {{expected identifier}}
double operator "" _miles(double); // expected-error {{parameter}}
template<char...> int operator "" j(const char*); // expected-error {{parameter}}
