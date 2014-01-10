// Test for disabling LSan at link-time.
// RUN: LSAN_BASE="use_stacks=0:use_registers=0"
// RUN: %clangxx_lsan %s -o %t
// RUN: LSAN_OPTIONS=$LSAN_BASE %t
// RUN: LSAN_OPTIONS=$LSAN_BASE not %t foo 2>&1 | FileCheck %s

#include <sanitizer/lsan_interface.h>

int argc_copy;

extern "C" {
int LeakSanitizerIsTurnedOffForTheCurrentProcess() {
  return (argc_copy == 1);
}
}

int main(int argc, char *argv[]) {
  volatile int *x = new int;
  *x = 42;
  argc_copy = argc;
  return 0;
}

// CHECK: SUMMARY: {{(Leak|Address)}}Sanitizer: 4 byte(s) leaked in 1 allocation(s)
