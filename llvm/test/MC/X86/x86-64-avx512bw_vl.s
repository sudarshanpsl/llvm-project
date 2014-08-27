// RUN: llvm-mc -triple x86_64-unknown-unknown -mcpu=skx  --show-encoding %s | FileCheck %s

// CHECK: vpcmpeqb %xmm21, %xmm21, %k4
// CHECK:  encoding: [0x62,0xb1,0x55,0x00,0x74,0xe5]
          vpcmpeqb %xmm21, %xmm21, %k4

// CHECK: vpcmpeqb %xmm21, %xmm21, %k4 {%k3}
// CHECK:  encoding: [0x62,0xb1,0x55,0x03,0x74,0xe5]
          vpcmpeqb %xmm21, %xmm21, %k4 {%k3}

// CHECK: vpcmpeqb (%rcx), %xmm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x00,0x74,0x21]
          vpcmpeqb (%rcx), %xmm21, %k4

// CHECK: vpcmpeqb 291(%rax,%r14,8), %xmm21, %k4
// CHECK:  encoding: [0x62,0xb1,0x55,0x00,0x74,0xa4,0xf0,0x23,0x01,0x00,0x00]
          vpcmpeqb 291(%rax,%r14,8), %xmm21, %k4

// CHECK: vpcmpeqb 2032(%rdx), %xmm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x00,0x74,0x62,0x7f]
          vpcmpeqb 2032(%rdx), %xmm21, %k4

// CHECK: vpcmpeqb 2048(%rdx), %xmm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x00,0x74,0xa2,0x00,0x08,0x00,0x00]
          vpcmpeqb 2048(%rdx), %xmm21, %k4

// CHECK: vpcmpeqb -2048(%rdx), %xmm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x00,0x74,0x62,0x80]
          vpcmpeqb -2048(%rdx), %xmm21, %k4

// CHECK: vpcmpeqb -2064(%rdx), %xmm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x00,0x74,0xa2,0xf0,0xf7,0xff,0xff]
          vpcmpeqb -2064(%rdx), %xmm21, %k4

// CHECK: vpcmpeqb %ymm18, %ymm21, %k4
// CHECK:  encoding: [0x62,0xb1,0x55,0x20,0x74,0xe2]
          vpcmpeqb %ymm18, %ymm21, %k4

// CHECK: vpcmpeqb %ymm18, %ymm21, %k4 {%k1}
// CHECK:  encoding: [0x62,0xb1,0x55,0x21,0x74,0xe2]
          vpcmpeqb %ymm18, %ymm21, %k4 {%k1}

// CHECK: vpcmpeqb (%rcx), %ymm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x20,0x74,0x21]
          vpcmpeqb (%rcx), %ymm21, %k4

// CHECK: vpcmpeqb 291(%rax,%r14,8), %ymm21, %k4
// CHECK:  encoding: [0x62,0xb1,0x55,0x20,0x74,0xa4,0xf0,0x23,0x01,0x00,0x00]
          vpcmpeqb 291(%rax,%r14,8), %ymm21, %k4

// CHECK: vpcmpeqb 4064(%rdx), %ymm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x20,0x74,0x62,0x7f]
          vpcmpeqb 4064(%rdx), %ymm21, %k4

// CHECK: vpcmpeqb 4096(%rdx), %ymm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x20,0x74,0xa2,0x00,0x10,0x00,0x00]
          vpcmpeqb 4096(%rdx), %ymm21, %k4

// CHECK: vpcmpeqb -4096(%rdx), %ymm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x20,0x74,0x62,0x80]
          vpcmpeqb -4096(%rdx), %ymm21, %k4

// CHECK: vpcmpeqb -4128(%rdx), %ymm21, %k4
// CHECK:  encoding: [0x62,0xf1,0x55,0x20,0x74,0xa2,0xe0,0xef,0xff,0xff]
          vpcmpeqb -4128(%rdx), %ymm21, %k4

// CHECK: vpcmpeqw %xmm27, %xmm30, %k3
// CHECK:  encoding: [0x62,0x91,0x0d,0x00,0x75,0xdb]
          vpcmpeqw %xmm27, %xmm30, %k3

// CHECK: vpcmpeqw %xmm27, %xmm30, %k3 {%k1}
// CHECK:  encoding: [0x62,0x91,0x0d,0x01,0x75,0xdb]
          vpcmpeqw %xmm27, %xmm30, %k3 {%k1}

// CHECK: vpcmpeqw (%rcx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x75,0x19]
          vpcmpeqw (%rcx), %xmm30, %k3

// CHECK: vpcmpeqw 291(%rax,%r14,8), %xmm30, %k3
// CHECK:  encoding: [0x62,0xb1,0x0d,0x00,0x75,0x9c,0xf0,0x23,0x01,0x00,0x00]
          vpcmpeqw 291(%rax,%r14,8), %xmm30, %k3

// CHECK: vpcmpeqw 2032(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x75,0x5a,0x7f]
          vpcmpeqw 2032(%rdx), %xmm30, %k3

// CHECK: vpcmpeqw 2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x75,0x9a,0x00,0x08,0x00,0x00]
          vpcmpeqw 2048(%rdx), %xmm30, %k3

// CHECK: vpcmpeqw -2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x75,0x5a,0x80]
          vpcmpeqw -2048(%rdx), %xmm30, %k3

// CHECK: vpcmpeqw -2064(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x75,0x9a,0xf0,0xf7,0xff,0xff]
          vpcmpeqw -2064(%rdx), %xmm30, %k3

// CHECK: vpcmpeqw %ymm29, %ymm20, %k2
// CHECK:  encoding: [0x62,0x91,0x5d,0x20,0x75,0xd5]
          vpcmpeqw %ymm29, %ymm20, %k2

// CHECK: vpcmpeqw %ymm29, %ymm20, %k2 {%k5}
// CHECK:  encoding: [0x62,0x91,0x5d,0x25,0x75,0xd5]
          vpcmpeqw %ymm29, %ymm20, %k2 {%k5}

// CHECK: vpcmpeqw (%rcx), %ymm20, %k2
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x75,0x11]
          vpcmpeqw (%rcx), %ymm20, %k2

// CHECK: vpcmpeqw 291(%rax,%r14,8), %ymm20, %k2
// CHECK:  encoding: [0x62,0xb1,0x5d,0x20,0x75,0x94,0xf0,0x23,0x01,0x00,0x00]
          vpcmpeqw 291(%rax,%r14,8), %ymm20, %k2

// CHECK: vpcmpeqw 4064(%rdx), %ymm20, %k2
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x75,0x52,0x7f]
          vpcmpeqw 4064(%rdx), %ymm20, %k2

// CHECK: vpcmpeqw 4096(%rdx), %ymm20, %k2
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x75,0x92,0x00,0x10,0x00,0x00]
          vpcmpeqw 4096(%rdx), %ymm20, %k2

// CHECK: vpcmpeqw -4096(%rdx), %ymm20, %k2
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x75,0x52,0x80]
          vpcmpeqw -4096(%rdx), %ymm20, %k2

// CHECK: vpcmpeqw -4128(%rdx), %ymm20, %k2
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x75,0x92,0xe0,0xef,0xff,0xff]
          vpcmpeqw -4128(%rdx), %ymm20, %k2

// CHECK: vpcmpgtb %xmm17, %xmm30, %k3
// CHECK:  encoding: [0x62,0xb1,0x0d,0x00,0x64,0xd9]
          vpcmpgtb %xmm17, %xmm30, %k3

// CHECK: vpcmpgtb %xmm17, %xmm30, %k3 {%k7}
// CHECK:  encoding: [0x62,0xb1,0x0d,0x07,0x64,0xd9]
          vpcmpgtb %xmm17, %xmm30, %k3 {%k7}

// CHECK: vpcmpgtb (%rcx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x64,0x19]
          vpcmpgtb (%rcx), %xmm30, %k3

// CHECK: vpcmpgtb 291(%rax,%r14,8), %xmm30, %k3
// CHECK:  encoding: [0x62,0xb1,0x0d,0x00,0x64,0x9c,0xf0,0x23,0x01,0x00,0x00]
          vpcmpgtb 291(%rax,%r14,8), %xmm30, %k3

// CHECK: vpcmpgtb 2032(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x64,0x5a,0x7f]
          vpcmpgtb 2032(%rdx), %xmm30, %k3

// CHECK: vpcmpgtb 2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x64,0x9a,0x00,0x08,0x00,0x00]
          vpcmpgtb 2048(%rdx), %xmm30, %k3

// CHECK: vpcmpgtb -2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x64,0x5a,0x80]
          vpcmpgtb -2048(%rdx), %xmm30, %k3

// CHECK: vpcmpgtb -2064(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf1,0x0d,0x00,0x64,0x9a,0xf0,0xf7,0xff,0xff]
          vpcmpgtb -2064(%rdx), %xmm30, %k3

// CHECK: vpcmpgtb %ymm17, %ymm17, %k2
// CHECK:  encoding: [0x62,0xb1,0x75,0x20,0x64,0xd1]
          vpcmpgtb %ymm17, %ymm17, %k2

// CHECK: vpcmpgtb %ymm17, %ymm17, %k2 {%k4}
// CHECK:  encoding: [0x62,0xb1,0x75,0x24,0x64,0xd1]
          vpcmpgtb %ymm17, %ymm17, %k2 {%k4}

// CHECK: vpcmpgtb (%rcx), %ymm17, %k2
// CHECK:  encoding: [0x62,0xf1,0x75,0x20,0x64,0x11]
          vpcmpgtb (%rcx), %ymm17, %k2

// CHECK: vpcmpgtb 291(%rax,%r14,8), %ymm17, %k2
// CHECK:  encoding: [0x62,0xb1,0x75,0x20,0x64,0x94,0xf0,0x23,0x01,0x00,0x00]
          vpcmpgtb 291(%rax,%r14,8), %ymm17, %k2

// CHECK: vpcmpgtb 4064(%rdx), %ymm17, %k2
// CHECK:  encoding: [0x62,0xf1,0x75,0x20,0x64,0x52,0x7f]
          vpcmpgtb 4064(%rdx), %ymm17, %k2

// CHECK: vpcmpgtb 4096(%rdx), %ymm17, %k2
// CHECK:  encoding: [0x62,0xf1,0x75,0x20,0x64,0x92,0x00,0x10,0x00,0x00]
          vpcmpgtb 4096(%rdx), %ymm17, %k2

// CHECK: vpcmpgtb -4096(%rdx), %ymm17, %k2
// CHECK:  encoding: [0x62,0xf1,0x75,0x20,0x64,0x52,0x80]
          vpcmpgtb -4096(%rdx), %ymm17, %k2

// CHECK: vpcmpgtb -4128(%rdx), %ymm17, %k2
// CHECK:  encoding: [0x62,0xf1,0x75,0x20,0x64,0x92,0xe0,0xef,0xff,0xff]
          vpcmpgtb -4128(%rdx), %ymm17, %k2

// CHECK: vpcmpgtw %xmm22, %xmm28, %k2
// CHECK:  encoding: [0x62,0xb1,0x1d,0x00,0x65,0xd6]
          vpcmpgtw %xmm22, %xmm28, %k2

// CHECK: vpcmpgtw %xmm22, %xmm28, %k2 {%k7}
// CHECK:  encoding: [0x62,0xb1,0x1d,0x07,0x65,0xd6]
          vpcmpgtw %xmm22, %xmm28, %k2 {%k7}

// CHECK: vpcmpgtw (%rcx), %xmm28, %k2
// CHECK:  encoding: [0x62,0xf1,0x1d,0x00,0x65,0x11]
          vpcmpgtw (%rcx), %xmm28, %k2

// CHECK: vpcmpgtw 291(%rax,%r14,8), %xmm28, %k2
// CHECK:  encoding: [0x62,0xb1,0x1d,0x00,0x65,0x94,0xf0,0x23,0x01,0x00,0x00]
          vpcmpgtw 291(%rax,%r14,8), %xmm28, %k2

// CHECK: vpcmpgtw 2032(%rdx), %xmm28, %k2
// CHECK:  encoding: [0x62,0xf1,0x1d,0x00,0x65,0x52,0x7f]
          vpcmpgtw 2032(%rdx), %xmm28, %k2

// CHECK: vpcmpgtw 2048(%rdx), %xmm28, %k2
// CHECK:  encoding: [0x62,0xf1,0x1d,0x00,0x65,0x92,0x00,0x08,0x00,0x00]
          vpcmpgtw 2048(%rdx), %xmm28, %k2

// CHECK: vpcmpgtw -2048(%rdx), %xmm28, %k2
// CHECK:  encoding: [0x62,0xf1,0x1d,0x00,0x65,0x52,0x80]
          vpcmpgtw -2048(%rdx), %xmm28, %k2

// CHECK: vpcmpgtw -2064(%rdx), %xmm28, %k2
// CHECK:  encoding: [0x62,0xf1,0x1d,0x00,0x65,0x92,0xf0,0xf7,0xff,0xff]
          vpcmpgtw -2064(%rdx), %xmm28, %k2

// CHECK: vpcmpgtw %ymm26, %ymm20, %k5
// CHECK:  encoding: [0x62,0x91,0x5d,0x20,0x65,0xea]
          vpcmpgtw %ymm26, %ymm20, %k5

// CHECK: vpcmpgtw %ymm26, %ymm20, %k5 {%k2}
// CHECK:  encoding: [0x62,0x91,0x5d,0x22,0x65,0xea]
          vpcmpgtw %ymm26, %ymm20, %k5 {%k2}

// CHECK: vpcmpgtw (%rcx), %ymm20, %k5
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x65,0x29]
          vpcmpgtw (%rcx), %ymm20, %k5

// CHECK: vpcmpgtw 291(%rax,%r14,8), %ymm20, %k5
// CHECK:  encoding: [0x62,0xb1,0x5d,0x20,0x65,0xac,0xf0,0x23,0x01,0x00,0x00]
          vpcmpgtw 291(%rax,%r14,8), %ymm20, %k5

// CHECK: vpcmpgtw 4064(%rdx), %ymm20, %k5
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x65,0x6a,0x7f]
          vpcmpgtw 4064(%rdx), %ymm20, %k5

// CHECK: vpcmpgtw 4096(%rdx), %ymm20, %k5
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x65,0xaa,0x00,0x10,0x00,0x00]
          vpcmpgtw 4096(%rdx), %ymm20, %k5

// CHECK: vpcmpgtw -4096(%rdx), %ymm20, %k5
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x65,0x6a,0x80]
          vpcmpgtw -4096(%rdx), %ymm20, %k5

// CHECK: vpcmpgtw -4128(%rdx), %ymm20, %k5
// CHECK:  encoding: [0x62,0xf1,0x5d,0x20,0x65,0xaa,0xe0,0xef,0xff,0xff]
          vpcmpgtw -4128(%rdx), %ymm20, %k5

// CHECK: vpcmpb $171, %xmm17, %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x0d,0x00,0x3f,0xd9,0xab]
          vpcmpb $171, %xmm17, %xmm30, %k3

// CHECK: vpcmpb $171, %xmm17, %xmm30, %k3 {%k1}
// CHECK:  encoding: [0x62,0xb3,0x0d,0x01,0x3f,0xd9,0xab]
          vpcmpb $171, %xmm17, %xmm30, %k3 {%k1}

// CHECK: vpcmpb $123, %xmm17, %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x0d,0x00,0x3f,0xd9,0x7b]
          vpcmpb $123, %xmm17, %xmm30, %k3

// CHECK: vpcmpb $123, (%rcx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x0d,0x00,0x3f,0x19,0x7b]
          vpcmpb $123, (%rcx), %xmm30, %k3

// CHECK: vpcmpb $123, 291(%rax,%r14,8), %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x0d,0x00,0x3f,0x9c,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpb $123, 291(%rax,%r14,8), %xmm30, %k3

// CHECK: vpcmpb $123, 2032(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x0d,0x00,0x3f,0x5a,0x7f,0x7b]
          vpcmpb $123, 2032(%rdx), %xmm30, %k3

// CHECK: vpcmpb $123, 2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x0d,0x00,0x3f,0x9a,0x00,0x08,0x00,0x00,0x7b]
          vpcmpb $123, 2048(%rdx), %xmm30, %k3

// CHECK: vpcmpb $123, -2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x0d,0x00,0x3f,0x5a,0x80,0x7b]
          vpcmpb $123, -2048(%rdx), %xmm30, %k3

// CHECK: vpcmpb $123, -2064(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x0d,0x00,0x3f,0x9a,0xf0,0xf7,0xff,0xff,0x7b]
          vpcmpb $123, -2064(%rdx), %xmm30, %k3

// CHECK: vpcmpb $171, %ymm19, %ymm19, %k5
// CHECK:  encoding: [0x62,0xb3,0x65,0x20,0x3f,0xeb,0xab]
          vpcmpb $171, %ymm19, %ymm19, %k5

// CHECK: vpcmpb $171, %ymm19, %ymm19, %k5 {%k4}
// CHECK:  encoding: [0x62,0xb3,0x65,0x24,0x3f,0xeb,0xab]
          vpcmpb $171, %ymm19, %ymm19, %k5 {%k4}

// CHECK: vpcmpb $123, %ymm19, %ymm19, %k5
// CHECK:  encoding: [0x62,0xb3,0x65,0x20,0x3f,0xeb,0x7b]
          vpcmpb $123, %ymm19, %ymm19, %k5

// CHECK: vpcmpb $123, (%rcx), %ymm19, %k5
// CHECK:  encoding: [0x62,0xf3,0x65,0x20,0x3f,0x29,0x7b]
          vpcmpb $123, (%rcx), %ymm19, %k5

// CHECK: vpcmpb $123, 291(%rax,%r14,8), %ymm19, %k5
// CHECK:  encoding: [0x62,0xb3,0x65,0x20,0x3f,0xac,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpb $123, 291(%rax,%r14,8), %ymm19, %k5

// CHECK: vpcmpb $123, 4064(%rdx), %ymm19, %k5
// CHECK:  encoding: [0x62,0xf3,0x65,0x20,0x3f,0x6a,0x7f,0x7b]
          vpcmpb $123, 4064(%rdx), %ymm19, %k5

// CHECK: vpcmpb $123, 4096(%rdx), %ymm19, %k5
// CHECK:  encoding: [0x62,0xf3,0x65,0x20,0x3f,0xaa,0x00,0x10,0x00,0x00,0x7b]
          vpcmpb $123, 4096(%rdx), %ymm19, %k5

// CHECK: vpcmpb $123, -4096(%rdx), %ymm19, %k5
// CHECK:  encoding: [0x62,0xf3,0x65,0x20,0x3f,0x6a,0x80,0x7b]
          vpcmpb $123, -4096(%rdx), %ymm19, %k5

// CHECK: vpcmpb $123, -4128(%rdx), %ymm19, %k5
// CHECK:  encoding: [0x62,0xf3,0x65,0x20,0x3f,0xaa,0xe0,0xef,0xff,0xff,0x7b]
          vpcmpb $123, -4128(%rdx), %ymm19, %k5

// CHECK: vpcmpw $171, %xmm22, %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x8d,0x00,0x3f,0xde,0xab]
          vpcmpw $171, %xmm22, %xmm30, %k3

// CHECK: vpcmpw $171, %xmm22, %xmm30, %k3 {%k6}
// CHECK:  encoding: [0x62,0xb3,0x8d,0x06,0x3f,0xde,0xab]
          vpcmpw $171, %xmm22, %xmm30, %k3 {%k6}

// CHECK: vpcmpw $123, %xmm22, %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x8d,0x00,0x3f,0xde,0x7b]
          vpcmpw $123, %xmm22, %xmm30, %k3

// CHECK: vpcmpw $123, (%rcx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x8d,0x00,0x3f,0x19,0x7b]
          vpcmpw $123, (%rcx), %xmm30, %k3

// CHECK: vpcmpw $123, 291(%rax,%r14,8), %xmm30, %k3
// CHECK:  encoding: [0x62,0xb3,0x8d,0x00,0x3f,0x9c,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpw $123, 291(%rax,%r14,8), %xmm30, %k3

// CHECK: vpcmpw $123, 2032(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x8d,0x00,0x3f,0x5a,0x7f,0x7b]
          vpcmpw $123, 2032(%rdx), %xmm30, %k3

// CHECK: vpcmpw $123, 2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x8d,0x00,0x3f,0x9a,0x00,0x08,0x00,0x00,0x7b]
          vpcmpw $123, 2048(%rdx), %xmm30, %k3

// CHECK: vpcmpw $123, -2048(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x8d,0x00,0x3f,0x5a,0x80,0x7b]
          vpcmpw $123, -2048(%rdx), %xmm30, %k3

// CHECK: vpcmpw $123, -2064(%rdx), %xmm30, %k3
// CHECK:  encoding: [0x62,0xf3,0x8d,0x00,0x3f,0x9a,0xf0,0xf7,0xff,0xff,0x7b]
          vpcmpw $123, -2064(%rdx), %xmm30, %k3

// CHECK: vpcmpw $171, %ymm18, %ymm26, %k3
// CHECK:  encoding: [0x62,0xb3,0xad,0x20,0x3f,0xda,0xab]
          vpcmpw $171, %ymm18, %ymm26, %k3

// CHECK: vpcmpw $171, %ymm18, %ymm26, %k3 {%k3}
// CHECK:  encoding: [0x62,0xb3,0xad,0x23,0x3f,0xda,0xab]
          vpcmpw $171, %ymm18, %ymm26, %k3 {%k3}

// CHECK: vpcmpw $123, %ymm18, %ymm26, %k3
// CHECK:  encoding: [0x62,0xb3,0xad,0x20,0x3f,0xda,0x7b]
          vpcmpw $123, %ymm18, %ymm26, %k3

// CHECK: vpcmpw $123, (%rcx), %ymm26, %k3
// CHECK:  encoding: [0x62,0xf3,0xad,0x20,0x3f,0x19,0x7b]
          vpcmpw $123, (%rcx), %ymm26, %k3

// CHECK: vpcmpw $123, 291(%rax,%r14,8), %ymm26, %k3
// CHECK:  encoding: [0x62,0xb3,0xad,0x20,0x3f,0x9c,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpw $123, 291(%rax,%r14,8), %ymm26, %k3

// CHECK: vpcmpw $123, 4064(%rdx), %ymm26, %k3
// CHECK:  encoding: [0x62,0xf3,0xad,0x20,0x3f,0x5a,0x7f,0x7b]
          vpcmpw $123, 4064(%rdx), %ymm26, %k3

// CHECK: vpcmpw $123, 4096(%rdx), %ymm26, %k3
// CHECK:  encoding: [0x62,0xf3,0xad,0x20,0x3f,0x9a,0x00,0x10,0x00,0x00,0x7b]
          vpcmpw $123, 4096(%rdx), %ymm26, %k3

// CHECK: vpcmpw $123, -4096(%rdx), %ymm26, %k3
// CHECK:  encoding: [0x62,0xf3,0xad,0x20,0x3f,0x5a,0x80,0x7b]
          vpcmpw $123, -4096(%rdx), %ymm26, %k3

// CHECK: vpcmpw $123, -4128(%rdx), %ymm26, %k3
// CHECK:  encoding: [0x62,0xf3,0xad,0x20,0x3f,0x9a,0xe0,0xef,0xff,0xff,0x7b]
          vpcmpw $123, -4128(%rdx), %ymm26, %k3

// CHECK: vpcmpub $171, %xmm21, %xmm22, %k5
// CHECK:  encoding: [0x62,0xb3,0x4d,0x00,0x3e,0xed,0xab]
          vpcmpub $171, %xmm21, %xmm22, %k5

// CHECK: vpcmpub $171, %xmm21, %xmm22, %k5 {%k3}
// CHECK:  encoding: [0x62,0xb3,0x4d,0x03,0x3e,0xed,0xab]
          vpcmpub $171, %xmm21, %xmm22, %k5 {%k3}

// CHECK: vpcmpub $123, %xmm21, %xmm22, %k5
// CHECK:  encoding: [0x62,0xb3,0x4d,0x00,0x3e,0xed,0x7b]
          vpcmpub $123, %xmm21, %xmm22, %k5

// CHECK: vpcmpub $123, (%rcx), %xmm22, %k5
// CHECK:  encoding: [0x62,0xf3,0x4d,0x00,0x3e,0x29,0x7b]
          vpcmpub $123, (%rcx), %xmm22, %k5

// CHECK: vpcmpub $123, 291(%rax,%r14,8), %xmm22, %k5
// CHECK:  encoding: [0x62,0xb3,0x4d,0x00,0x3e,0xac,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpub $123, 291(%rax,%r14,8), %xmm22, %k5

// CHECK: vpcmpub $123, 2032(%rdx), %xmm22, %k5
// CHECK:  encoding: [0x62,0xf3,0x4d,0x00,0x3e,0x6a,0x7f,0x7b]
          vpcmpub $123, 2032(%rdx), %xmm22, %k5

// CHECK: vpcmpub $123, 2048(%rdx), %xmm22, %k5
// CHECK:  encoding: [0x62,0xf3,0x4d,0x00,0x3e,0xaa,0x00,0x08,0x00,0x00,0x7b]
          vpcmpub $123, 2048(%rdx), %xmm22, %k5

// CHECK: vpcmpub $123, -2048(%rdx), %xmm22, %k5
// CHECK:  encoding: [0x62,0xf3,0x4d,0x00,0x3e,0x6a,0x80,0x7b]
          vpcmpub $123, -2048(%rdx), %xmm22, %k5

// CHECK: vpcmpub $123, -2064(%rdx), %xmm22, %k5
// CHECK:  encoding: [0x62,0xf3,0x4d,0x00,0x3e,0xaa,0xf0,0xf7,0xff,0xff,0x7b]
          vpcmpub $123, -2064(%rdx), %xmm22, %k5

// CHECK: vpcmpub $171, %ymm21, %ymm23, %k2
// CHECK:  encoding: [0x62,0xb3,0x45,0x20,0x3e,0xd5,0xab]
          vpcmpub $171, %ymm21, %ymm23, %k2

// CHECK: vpcmpub $171, %ymm21, %ymm23, %k2 {%k2}
// CHECK:  encoding: [0x62,0xb3,0x45,0x22,0x3e,0xd5,0xab]
          vpcmpub $171, %ymm21, %ymm23, %k2 {%k2}

// CHECK: vpcmpub $123, %ymm21, %ymm23, %k2
// CHECK:  encoding: [0x62,0xb3,0x45,0x20,0x3e,0xd5,0x7b]
          vpcmpub $123, %ymm21, %ymm23, %k2

// CHECK: vpcmpub $123, (%rcx), %ymm23, %k2
// CHECK:  encoding: [0x62,0xf3,0x45,0x20,0x3e,0x11,0x7b]
          vpcmpub $123, (%rcx), %ymm23, %k2

// CHECK: vpcmpub $123, 291(%rax,%r14,8), %ymm23, %k2
// CHECK:  encoding: [0x62,0xb3,0x45,0x20,0x3e,0x94,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpub $123, 291(%rax,%r14,8), %ymm23, %k2

// CHECK: vpcmpub $123, 4064(%rdx), %ymm23, %k2
// CHECK:  encoding: [0x62,0xf3,0x45,0x20,0x3e,0x52,0x7f,0x7b]
          vpcmpub $123, 4064(%rdx), %ymm23, %k2

// CHECK: vpcmpub $123, 4096(%rdx), %ymm23, %k2
// CHECK:  encoding: [0x62,0xf3,0x45,0x20,0x3e,0x92,0x00,0x10,0x00,0x00,0x7b]
          vpcmpub $123, 4096(%rdx), %ymm23, %k2

// CHECK: vpcmpub $123, -4096(%rdx), %ymm23, %k2
// CHECK:  encoding: [0x62,0xf3,0x45,0x20,0x3e,0x52,0x80,0x7b]
          vpcmpub $123, -4096(%rdx), %ymm23, %k2

// CHECK: vpcmpub $123, -4128(%rdx), %ymm23, %k2
// CHECK:  encoding: [0x62,0xf3,0x45,0x20,0x3e,0x92,0xe0,0xef,0xff,0xff,0x7b]
          vpcmpub $123, -4128(%rdx), %ymm23, %k2

// CHECK: vpcmpuw $171, %xmm17, %xmm28, %k5
// CHECK:  encoding: [0x62,0xb3,0x9d,0x00,0x3e,0xe9,0xab]
          vpcmpuw $171, %xmm17, %xmm28, %k5

// CHECK: vpcmpuw $171, %xmm17, %xmm28, %k5 {%k4}
// CHECK:  encoding: [0x62,0xb3,0x9d,0x04,0x3e,0xe9,0xab]
          vpcmpuw $171, %xmm17, %xmm28, %k5 {%k4}

// CHECK: vpcmpuw $123, %xmm17, %xmm28, %k5
// CHECK:  encoding: [0x62,0xb3,0x9d,0x00,0x3e,0xe9,0x7b]
          vpcmpuw $123, %xmm17, %xmm28, %k5

// CHECK: vpcmpuw $123, (%rcx), %xmm28, %k5
// CHECK:  encoding: [0x62,0xf3,0x9d,0x00,0x3e,0x29,0x7b]
          vpcmpuw $123, (%rcx), %xmm28, %k5

// CHECK: vpcmpuw $123, 291(%rax,%r14,8), %xmm28, %k5
// CHECK:  encoding: [0x62,0xb3,0x9d,0x00,0x3e,0xac,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpuw $123, 291(%rax,%r14,8), %xmm28, %k5

// CHECK: vpcmpuw $123, 2032(%rdx), %xmm28, %k5
// CHECK:  encoding: [0x62,0xf3,0x9d,0x00,0x3e,0x6a,0x7f,0x7b]
          vpcmpuw $123, 2032(%rdx), %xmm28, %k5

// CHECK: vpcmpuw $123, 2048(%rdx), %xmm28, %k5
// CHECK:  encoding: [0x62,0xf3,0x9d,0x00,0x3e,0xaa,0x00,0x08,0x00,0x00,0x7b]
          vpcmpuw $123, 2048(%rdx), %xmm28, %k5

// CHECK: vpcmpuw $123, -2048(%rdx), %xmm28, %k5
// CHECK:  encoding: [0x62,0xf3,0x9d,0x00,0x3e,0x6a,0x80,0x7b]
          vpcmpuw $123, -2048(%rdx), %xmm28, %k5

// CHECK: vpcmpuw $123, -2064(%rdx), %xmm28, %k5
// CHECK:  encoding: [0x62,0xf3,0x9d,0x00,0x3e,0xaa,0xf0,0xf7,0xff,0xff,0x7b]
          vpcmpuw $123, -2064(%rdx), %xmm28, %k5

// CHECK: vpcmpuw $171, %ymm28, %ymm27, %k4
// CHECK:  encoding: [0x62,0x93,0xa5,0x20,0x3e,0xe4,0xab]
          vpcmpuw $171, %ymm28, %ymm27, %k4

// CHECK: vpcmpuw $171, %ymm28, %ymm27, %k4 {%k2}
// CHECK:  encoding: [0x62,0x93,0xa5,0x22,0x3e,0xe4,0xab]
          vpcmpuw $171, %ymm28, %ymm27, %k4 {%k2}

// CHECK: vpcmpuw $123, %ymm28, %ymm27, %k4
// CHECK:  encoding: [0x62,0x93,0xa5,0x20,0x3e,0xe4,0x7b]
          vpcmpuw $123, %ymm28, %ymm27, %k4

// CHECK: vpcmpuw $123, (%rcx), %ymm27, %k4
// CHECK:  encoding: [0x62,0xf3,0xa5,0x20,0x3e,0x21,0x7b]
          vpcmpuw $123, (%rcx), %ymm27, %k4

// CHECK: vpcmpuw $123, 291(%rax,%r14,8), %ymm27, %k4
// CHECK:  encoding: [0x62,0xb3,0xa5,0x20,0x3e,0xa4,0xf0,0x23,0x01,0x00,0x00,0x7b]
          vpcmpuw $123, 291(%rax,%r14,8), %ymm27, %k4

// CHECK: vpcmpuw $123, 4064(%rdx), %ymm27, %k4
// CHECK:  encoding: [0x62,0xf3,0xa5,0x20,0x3e,0x62,0x7f,0x7b]
          vpcmpuw $123, 4064(%rdx), %ymm27, %k4

// CHECK: vpcmpuw $123, 4096(%rdx), %ymm27, %k4
// CHECK:  encoding: [0x62,0xf3,0xa5,0x20,0x3e,0xa2,0x00,0x10,0x00,0x00,0x7b]
          vpcmpuw $123, 4096(%rdx), %ymm27, %k4

// CHECK: vpcmpuw $123, -4096(%rdx), %ymm27, %k4
// CHECK:  encoding: [0x62,0xf3,0xa5,0x20,0x3e,0x62,0x80,0x7b]
          vpcmpuw $123, -4096(%rdx), %ymm27, %k4

// CHECK: vpcmpuw $123, -4128(%rdx), %ymm27, %k4
// CHECK:  encoding: [0x62,0xf3,0xa5,0x20,0x3e,0xa2,0xe0,0xef,0xff,0xff,0x7b]
          vpcmpuw $123, -4128(%rdx), %ymm27, %k4

// CHECK: vmovdqu8 %xmm23, %xmm26
// CHECK:  encoding: [0x62,0x21,0x7f,0x08,0x6f,0xd7]
          vmovdqu8 %xmm23, %xmm26

// CHECK: vmovdqu8 %xmm23, %xmm26 {%k2}
// CHECK:  encoding: [0x62,0x21,0x7f,0x0a,0x6f,0xd7]
          vmovdqu8 %xmm23, %xmm26 {%k2}

// CHECK: vmovdqu8 %xmm23, %xmm26 {%k2} {z}
// CHECK:  encoding: [0x62,0x21,0x7f,0x8a,0x6f,0xd7]
          vmovdqu8 %xmm23, %xmm26 {%k2} {z}

// CHECK: vmovdqu8 (%rcx), %xmm26
// CHECK:  encoding: [0x62,0x61,0x7f,0x08,0x6f,0x11]
          vmovdqu8 (%rcx), %xmm26

// CHECK: vmovdqu8 291(%rax,%r14,8), %xmm26
// CHECK:  encoding: [0x62,0x21,0x7f,0x08,0x6f,0x94,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu8 291(%rax,%r14,8), %xmm26

// CHECK: vmovdqu8 2032(%rdx), %xmm26
// CHECK:  encoding: [0x62,0x61,0x7f,0x08,0x6f,0x52,0x7f]
          vmovdqu8 2032(%rdx), %xmm26

// CHECK: vmovdqu8 2048(%rdx), %xmm26
// CHECK:  encoding: [0x62,0x61,0x7f,0x08,0x6f,0x92,0x00,0x08,0x00,0x00]
          vmovdqu8 2048(%rdx), %xmm26

// CHECK: vmovdqu8 -2048(%rdx), %xmm26
// CHECK:  encoding: [0x62,0x61,0x7f,0x08,0x6f,0x52,0x80]
          vmovdqu8 -2048(%rdx), %xmm26

// CHECK: vmovdqu8 -2064(%rdx), %xmm26
// CHECK:  encoding: [0x62,0x61,0x7f,0x08,0x6f,0x92,0xf0,0xf7,0xff,0xff]
          vmovdqu8 -2064(%rdx), %xmm26

// CHECK: vmovdqu8 %ymm29, %ymm18
// CHECK:  encoding: [0x62,0x81,0x7f,0x28,0x6f,0xd5]
          vmovdqu8 %ymm29, %ymm18

// CHECK: vmovdqu8 %ymm29, %ymm18 {%k7}
// CHECK:  encoding: [0x62,0x81,0x7f,0x2f,0x6f,0xd5]
          vmovdqu8 %ymm29, %ymm18 {%k7}

// CHECK: vmovdqu8 %ymm29, %ymm18 {%k7} {z}
// CHECK:  encoding: [0x62,0x81,0x7f,0xaf,0x6f,0xd5]
          vmovdqu8 %ymm29, %ymm18 {%k7} {z}

// CHECK: vmovdqu8 (%rcx), %ymm18
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x6f,0x11]
          vmovdqu8 (%rcx), %ymm18

// CHECK: vmovdqu8 291(%rax,%r14,8), %ymm18
// CHECK:  encoding: [0x62,0xa1,0x7f,0x28,0x6f,0x94,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu8 291(%rax,%r14,8), %ymm18

// CHECK: vmovdqu8 4064(%rdx), %ymm18
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x6f,0x52,0x7f]
          vmovdqu8 4064(%rdx), %ymm18

// CHECK: vmovdqu8 4096(%rdx), %ymm18
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x6f,0x92,0x00,0x10,0x00,0x00]
          vmovdqu8 4096(%rdx), %ymm18

// CHECK: vmovdqu8 -4096(%rdx), %ymm18
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x6f,0x52,0x80]
          vmovdqu8 -4096(%rdx), %ymm18

// CHECK: vmovdqu8 -4128(%rdx), %ymm18
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x6f,0x92,0xe0,0xef,0xff,0xff]
          vmovdqu8 -4128(%rdx), %ymm18

// CHECK: vmovdqu16 %xmm24, %xmm29
// CHECK:  encoding: [0x62,0x01,0xff,0x08,0x6f,0xe8]
          vmovdqu16 %xmm24, %xmm29

// CHECK: vmovdqu16 %xmm24, %xmm29 {%k6}
// CHECK:  encoding: [0x62,0x01,0xff,0x0e,0x6f,0xe8]
          vmovdqu16 %xmm24, %xmm29 {%k6}

// CHECK: vmovdqu16 %xmm24, %xmm29 {%k6} {z}
// CHECK:  encoding: [0x62,0x01,0xff,0x8e,0x6f,0xe8]
          vmovdqu16 %xmm24, %xmm29 {%k6} {z}

// CHECK: vmovdqu16 (%rcx), %xmm29
// CHECK:  encoding: [0x62,0x61,0xff,0x08,0x6f,0x29]
          vmovdqu16 (%rcx), %xmm29

// CHECK: vmovdqu16 291(%rax,%r14,8), %xmm29
// CHECK:  encoding: [0x62,0x21,0xff,0x08,0x6f,0xac,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu16 291(%rax,%r14,8), %xmm29

// CHECK: vmovdqu16 2032(%rdx), %xmm29
// CHECK:  encoding: [0x62,0x61,0xff,0x08,0x6f,0x6a,0x7f]
          vmovdqu16 2032(%rdx), %xmm29

// CHECK: vmovdqu16 2048(%rdx), %xmm29
// CHECK:  encoding: [0x62,0x61,0xff,0x08,0x6f,0xaa,0x00,0x08,0x00,0x00]
          vmovdqu16 2048(%rdx), %xmm29

// CHECK: vmovdqu16 -2048(%rdx), %xmm29
// CHECK:  encoding: [0x62,0x61,0xff,0x08,0x6f,0x6a,0x80]
          vmovdqu16 -2048(%rdx), %xmm29

// CHECK: vmovdqu16 -2064(%rdx), %xmm29
// CHECK:  encoding: [0x62,0x61,0xff,0x08,0x6f,0xaa,0xf0,0xf7,0xff,0xff]
          vmovdqu16 -2064(%rdx), %xmm29

// CHECK: vmovdqu16 %ymm24, %ymm23
// CHECK:  encoding: [0x62,0x81,0xff,0x28,0x6f,0xf8]
          vmovdqu16 %ymm24, %ymm23

// CHECK: vmovdqu16 %ymm24, %ymm23 {%k3}
// CHECK:  encoding: [0x62,0x81,0xff,0x2b,0x6f,0xf8]
          vmovdqu16 %ymm24, %ymm23 {%k3}

// CHECK: vmovdqu16 %ymm24, %ymm23 {%k3} {z}
// CHECK:  encoding: [0x62,0x81,0xff,0xab,0x6f,0xf8]
          vmovdqu16 %ymm24, %ymm23 {%k3} {z}

// CHECK: vmovdqu16 (%rcx), %ymm23
// CHECK:  encoding: [0x62,0xe1,0xff,0x28,0x6f,0x39]
          vmovdqu16 (%rcx), %ymm23

// CHECK: vmovdqu16 291(%rax,%r14,8), %ymm23
// CHECK:  encoding: [0x62,0xa1,0xff,0x28,0x6f,0xbc,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu16 291(%rax,%r14,8), %ymm23

// CHECK: vmovdqu16 4064(%rdx), %ymm23
// CHECK:  encoding: [0x62,0xe1,0xff,0x28,0x6f,0x7a,0x7f]
          vmovdqu16 4064(%rdx), %ymm23

// CHECK: vmovdqu16 4096(%rdx), %ymm23
// CHECK:  encoding: [0x62,0xe1,0xff,0x28,0x6f,0xba,0x00,0x10,0x00,0x00]
          vmovdqu16 4096(%rdx), %ymm23

// CHECK: vmovdqu16 -4096(%rdx), %ymm23
// CHECK:  encoding: [0x62,0xe1,0xff,0x28,0x6f,0x7a,0x80]
          vmovdqu16 -4096(%rdx), %ymm23

// CHECK: vmovdqu16 -4128(%rdx), %ymm23
// CHECK:  encoding: [0x62,0xe1,0xff,0x28,0x6f,0xba,0xe0,0xef,0xff,0xff]
          vmovdqu16 -4128(%rdx), %ymm23

// CHECK: vmovdqu8 %xmm17, (%rcx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x08,0x7f,0x09]
          vmovdqu8 %xmm17, (%rcx)

// CHECK: vmovdqu8 %xmm17, (%rcx) {%k4}
// CHECK:  encoding: [0x62,0xe1,0x7f,0x0c,0x7f,0x09]
          vmovdqu8 %xmm17, (%rcx) {%k4}

// CHECK: vmovdqu8 %xmm17, 291(%rax,%r14,8)
// CHECK:  encoding: [0x62,0xa1,0x7f,0x08,0x7f,0x8c,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu8 %xmm17, 291(%rax,%r14,8)

// CHECK: vmovdqu8 %xmm17, 2032(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x08,0x7f,0x4a,0x7f]
          vmovdqu8 %xmm17, 2032(%rdx)

// CHECK: vmovdqu8 %xmm17, 2048(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x08,0x7f,0x8a,0x00,0x08,0x00,0x00]
          vmovdqu8 %xmm17, 2048(%rdx)

// CHECK: vmovdqu8 %xmm17, -2048(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x08,0x7f,0x4a,0x80]
          vmovdqu8 %xmm17, -2048(%rdx)

// CHECK: vmovdqu8 %xmm17, -2064(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x08,0x7f,0x8a,0xf0,0xf7,0xff,0xff]
          vmovdqu8 %xmm17, -2064(%rdx)

// CHECK: vmovdqu8 %ymm21, (%rcx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x7f,0x29]
          vmovdqu8 %ymm21, (%rcx)

// CHECK: vmovdqu8 %ymm21, (%rcx) {%k1}
// CHECK:  encoding: [0x62,0xe1,0x7f,0x29,0x7f,0x29]
          vmovdqu8 %ymm21, (%rcx) {%k1}

// CHECK: vmovdqu8 %ymm21, 291(%rax,%r14,8)
// CHECK:  encoding: [0x62,0xa1,0x7f,0x28,0x7f,0xac,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu8 %ymm21, 291(%rax,%r14,8)

// CHECK: vmovdqu8 %ymm21, 4064(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x7f,0x6a,0x7f]
          vmovdqu8 %ymm21, 4064(%rdx)

// CHECK: vmovdqu8 %ymm21, 4096(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x7f,0xaa,0x00,0x10,0x00,0x00]
          vmovdqu8 %ymm21, 4096(%rdx)

// CHECK: vmovdqu8 %ymm21, -4096(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x7f,0x6a,0x80]
          vmovdqu8 %ymm21, -4096(%rdx)

// CHECK: vmovdqu8 %ymm21, -4128(%rdx)
// CHECK:  encoding: [0x62,0xe1,0x7f,0x28,0x7f,0xaa,0xe0,0xef,0xff,0xff]
          vmovdqu8 %ymm21, -4128(%rdx)

// CHECK: vmovdqu16 %xmm23, (%rcx)
// CHECK:  encoding: [0x62,0xe1,0xff,0x08,0x7f,0x39]
          vmovdqu16 %xmm23, (%rcx)

// CHECK: vmovdqu16 %xmm23, (%rcx) {%k7}
// CHECK:  encoding: [0x62,0xe1,0xff,0x0f,0x7f,0x39]
          vmovdqu16 %xmm23, (%rcx) {%k7}

// CHECK: vmovdqu16 %xmm23, 291(%rax,%r14,8)
// CHECK:  encoding: [0x62,0xa1,0xff,0x08,0x7f,0xbc,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu16 %xmm23, 291(%rax,%r14,8)

// CHECK: vmovdqu16 %xmm23, 2032(%rdx)
// CHECK:  encoding: [0x62,0xe1,0xff,0x08,0x7f,0x7a,0x7f]
          vmovdqu16 %xmm23, 2032(%rdx)

// CHECK: vmovdqu16 %xmm23, 2048(%rdx)
// CHECK:  encoding: [0x62,0xe1,0xff,0x08,0x7f,0xba,0x00,0x08,0x00,0x00]
          vmovdqu16 %xmm23, 2048(%rdx)

// CHECK: vmovdqu16 %xmm23, -2048(%rdx)
// CHECK:  encoding: [0x62,0xe1,0xff,0x08,0x7f,0x7a,0x80]
          vmovdqu16 %xmm23, -2048(%rdx)

// CHECK: vmovdqu16 %xmm23, -2064(%rdx)
// CHECK:  encoding: [0x62,0xe1,0xff,0x08,0x7f,0xba,0xf0,0xf7,0xff,0xff]
          vmovdqu16 %xmm23, -2064(%rdx)

// CHECK: vmovdqu16 %ymm29, (%rcx)
// CHECK:  encoding: [0x62,0x61,0xff,0x28,0x7f,0x29]
          vmovdqu16 %ymm29, (%rcx)

// CHECK: vmovdqu16 %ymm29, (%rcx) {%k6}
// CHECK:  encoding: [0x62,0x61,0xff,0x2e,0x7f,0x29]
          vmovdqu16 %ymm29, (%rcx) {%k6}

// CHECK: vmovdqu16 %ymm29, 291(%rax,%r14,8)
// CHECK:  encoding: [0x62,0x21,0xff,0x28,0x7f,0xac,0xf0,0x23,0x01,0x00,0x00]
          vmovdqu16 %ymm29, 291(%rax,%r14,8)

// CHECK: vmovdqu16 %ymm29, 4064(%rdx)
// CHECK:  encoding: [0x62,0x61,0xff,0x28,0x7f,0x6a,0x7f]
          vmovdqu16 %ymm29, 4064(%rdx)

// CHECK: vmovdqu16 %ymm29, 4096(%rdx)
// CHECK:  encoding: [0x62,0x61,0xff,0x28,0x7f,0xaa,0x00,0x10,0x00,0x00]
          vmovdqu16 %ymm29, 4096(%rdx)

// CHECK: vmovdqu16 %ymm29, -4096(%rdx)
// CHECK:  encoding: [0x62,0x61,0xff,0x28,0x7f,0x6a,0x80]
          vmovdqu16 %ymm29, -4096(%rdx)

// CHECK: vmovdqu16 %ymm29, -4128(%rdx)
// CHECK:  encoding: [0x62,0x61,0xff,0x28,0x7f,0xaa,0xe0,0xef,0xff,0xff]
          vmovdqu16 %ymm29, -4128(%rdx)
