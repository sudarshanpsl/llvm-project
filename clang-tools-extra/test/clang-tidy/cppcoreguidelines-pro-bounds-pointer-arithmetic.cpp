// RUN: %python %S/check_clang_tidy.py %s cppcoreguidelines-pro-bounds-pointer-arithmetic %t

enum E {
  ENUM_LITERAL = 1
};

int i = 4;
int j = 1;
int *p = 0;
int *q = 0;

void fail() {
  q = p + 4;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic [cppcoreguidelines-pro-bounds-pointer-arithmetic]
  p = q + i;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic
  p = q + ENUM_LITERAL;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic

  q = p - 1;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic
  p = q - i;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic
  p = q - ENUM_LITERAL;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic

  p += 4;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic
  p += i;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic
  p += ENUM_LITERAL;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic

  q -= 1;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic
  q -= i;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic
  q -= ENUM_LITERAL;
  // CHECK-MESSAGES: :[[@LINE-1]]:5: warning: do not use pointer arithmetic

  p++;
  // CHECK-MESSAGES: :[[@LINE-1]]:4: warning: do not use pointer arithmetic
  ++p;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: do not use pointer arithmetic

  p--;
  // CHECK-MESSAGES: :[[@LINE-1]]:4: warning: do not use pointer arithmetic
  --p;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: do not use pointer arithmetic

  i = p[1];
  // CHECK-MESSAGES: :[[@LINE-1]]:7: warning: do not use pointer arithmetic
}

struct S {
  operator int() const;
};

void f(S &s) {
  int *i;
  i = i + s;
  // CHECK-MESSAGES: :[[@LINE-1]]:9: warning: do not use pointer arithmetic
}

void f2(int i[]) {
  i[1] = 0;
  // CHECK-MESSAGES: :[[@LINE-1]]:3: warning: do not use pointer arithmetic
}

void okay() {
  int a[3];
  i = a[2]; // OK, access to array

  p = q;
  p = &i;

  i++;
  ++i;
  i--;
  --i;
  i += 1;
  i -= 1;
  i = j + 1;
  i = j - 1;

  auto diff = p - q; // OK, result is arithmetic
}
