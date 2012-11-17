// RUN: %clang_cc1 -fsyntax-only %s -verify
#ifndef __has_warning
#error Should have __has_warning
#endif

#if __has_warning("not valid") // expected-warning {{__has_warning expected option name}}
#endif

// expected-warning@+2 {{Should have -Wparentheses}}
#if __has_warning("-Wparentheses")
#warning Should have -Wparentheses
#endif

// expected-error@+2 {{builtin warning check macro requires a parenthesized string}}
// expected-error@+1 {{expected value in expression}}
#if __has_warning(-Wfoo)
#endif

// expected-warning@+3 {{Not a valid warning flag}}
#if __has_warning("-Wnot-a-valid-warning-flag-at-all")
#else
#warning Not a valid warning flag
#endif
