// RUN: %clangxx_asan -O0 %s -Fe%t
// RUN: %run %t

// Just make sure we can parse <windows.h>
#include <windows.h>

int main() {}
