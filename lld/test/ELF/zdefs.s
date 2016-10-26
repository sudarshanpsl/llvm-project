# RUN: llvm-mc -filetype=obj -triple=x86_64-unknown-linux %s -o %t.o
# RUN: ld.lld -shared %t.o -o %t1.so

# RUN: not ld.lld -z defs -shared %t.o -o %t1.so 2>&1 | FileCheck -check-prefix=ERR %s
# ERR: error: {{.*}} (.text+0x1): undefined symbol 'foo'

callq foo@PLT
