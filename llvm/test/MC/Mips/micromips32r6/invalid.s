# RUN: not llvm-mc %s -triple=mips -show-encoding -mcpu=mips32r6 -mattr=micromips 2>%t1
# RUN: FileCheck %s < %t1

  break 1024               # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
  break 1023, 1024         # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
  ei $32                   # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
  swe $33, 8($4)           # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
  swe $5, 8($34)           # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
  swe $5, 512($4)          # CHECK: :[[@LINE]]:{{[0-9]+}}: error: invalid operand for instruction
