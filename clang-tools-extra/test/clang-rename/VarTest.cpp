namespace A {
int foo;  // CHECK: int hector;
}
int foo;  // CHECK: int foo;
int bar = foo; // CHECK: bar = foo;
int baz = A::foo; // CHECK: baz = A::hector;
void fun1() {
  struct {
    int foo; // CHECK: int foo;
  } b = { 100 };
  int foo = 100; // CHECK: int foo
  baz = foo; // CHECK: baz = foo;
  {
    extern int foo; // CHECK: int foo;
    baz = foo; // CHECK: baz = foo;
    foo = A::foo + baz; // CHECK: foo = A::hector + baz;
    A::foo = b.foo; // CHECK: A::hector = b.foo;
  }
  foo = b.foo; // CHECK: foo = b.foo;
}
// REQUIRES: shell
// RUN: cat %s > %t.cpp
// RUN: clang-rename -offset=$(grep -FUbo 'foo;' %t.cpp | head -1 | cut -d: -f1) -new-name=hector %t.cpp -i --
// RUN: sed 's,//.*,,' %t.cpp | FileCheck %s
