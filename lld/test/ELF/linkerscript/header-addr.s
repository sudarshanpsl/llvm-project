# REQUIRES: x86
# RUN: llvm-mc -filetype=obj -triple=x86_64-pc-linux %s -o %t.o
# RUN: echo "PHDRS {all PT_LOAD PHDRS;} \
# RUN:       SECTIONS { \
# RUN:       . = 0x2000; \
# RUN:       .text : {*(.text)} :all \
# RUN:     }" > %t.script
# RUN: ld.lld -o %t.so --script %t.script %t.o -shared
# RUN: llvm-readobj -program-headers %t.so | FileCheck %s

# CHECK:      ProgramHeaders [
# CHECK-NEXT:   ProgramHeader {
# CHECK-NEXT:     Type: PT_LOAD
# CHECK-NEXT:     Offset: 0x40
# CHECK-NEXT:     VirtualAddress: 0x1040
# CHECK-NEXT:     PhysicalAddress: 0x1040
# CHECK-NEXT:     FileSize: 4176
# CHECK-NEXT:     MemSize: 4176
# CHECK-NEXT:     Flags [
# CHECK-NEXT:       PF_R (0x4)
# CHECK-NEXT:       PF_W (0x2)
# CHECK-NEXT:       PF_X (0x1)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Alignment: 4096
# CHECK-NEXT:   }
# CHECK-NEXT: ]
