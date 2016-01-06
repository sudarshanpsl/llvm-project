// RUN: %clang_cc1 -verify %s

struct X {
  int a; // expected-note {{previous}}
  void b(); // expected-note {{previous}}
  struct c; // expected-note {{previous}}
  typedef int d; // expected-note {{previous}}

  union {
    int a; // expected-error {{member of anonymous union redeclares}}
    int b; // expected-error {{member of anonymous union redeclares}}
    int c; // expected-error {{member of anonymous union redeclares}}
    int d; // expected-error {{member of anonymous union redeclares}}
    int e; // expected-note {{previous}}
    int f; // expected-note {{previous}}
    int g;
    int h; // expected-note {{previous}}
  };

  int e; // expected-error {{duplicate member}}
  void f(); // expected-error {{redefinition}}
  // FIXME: This is ill-formed, even though one name is a tag and the other is
  // an anonymous union member. Reject this.
  struct g;
  typedef int h; // expected-error {{redefinition}}
};
