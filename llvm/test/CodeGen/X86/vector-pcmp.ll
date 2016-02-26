; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse2 | FileCheck %s --check-prefix=SSE --check-prefix=SSE2
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+sse4.2 | FileCheck %s --check-prefix=SSE --check-prefix=SSE42
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx | FileCheck %s --check-prefix=AVX --check-prefix=AVX1
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+avx2 | FileCheck %s --check-prefix=AVX --check-prefix=AVX2

; Lower common integer comparisons such as 'isPositive' efficiently:
; https://llvm.org/bugs/show_bug.cgi?id=26701

define <16 x i8> @test_pcmpgtb(<16 x i8> %x) {
; SSE-LABEL: test_pcmpgtb:
; SSE:       # BB#0:
; SSE-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE-NEXT:    pcmpgtb %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: test_pcmpgtb:
; AVX:       # BB#0:
; AVX-NEXT:    vpcmpeqd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpcmpgtb %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %sign = ashr <16 x i8> %x, <i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7, i8 7>
  %not = xor <16 x i8> %sign, <i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1, i8 -1>
  ret <16 x i8> %not
}

define <8 x i16> @test_pcmpgtw(<8 x i16> %x) {
; SSE-LABEL: test_pcmpgtw:
; SSE:       # BB#0:
; SSE-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE-NEXT:    pcmpgtw %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: test_pcmpgtw:
; AVX:       # BB#0:
; AVX-NEXT:    vpcmpeqd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpcmpgtw %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %sign = ashr <8 x i16> %x, <i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15, i16 15>
  %not = xor <8 x i16> %sign, <i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1, i16 -1>
  ret <8 x i16> %not
}

define <4 x i32> @test_pcmpgtd(<4 x i32> %x) {
; SSE-LABEL: test_pcmpgtd:
; SSE:       # BB#0:
; SSE-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE-NEXT:    pcmpgtd %xmm1, %xmm0
; SSE-NEXT:    retq
;
; AVX-LABEL: test_pcmpgtd:
; AVX:       # BB#0:
; AVX-NEXT:    vpcmpeqd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpcmpgtd %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %sign = ashr <4 x i32> %x, <i32 31, i32 31, i32 31, i32 31>
  %not = xor <4 x i32> %sign, <i32 -1, i32 -1, i32 -1, i32 -1>
  ret <4 x i32> %not
}

define <2 x i64> @test_pcmpgtq(<2 x i64> %x) {
; SSE2-LABEL: test_pcmpgtq:
; SSE2:       # BB#0:
; SSE2-NEXT:    psrad $31, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm1 = xmm0[1,1,3,3]
; SSE2-NEXT:    pcmpeqd %xmm0, %xmm0
; SSE2-NEXT:    pxor %xmm1, %xmm0
; SSE2-NEXT:    retq
;
; SSE42-LABEL: test_pcmpgtq:
; SSE42:       # BB#0:
; SSE42-NEXT:    pcmpeqd %xmm1, %xmm1
; SSE42-NEXT:    pcmpgtq %xmm1, %xmm0
; SSE42-NEXT:    retq
;
; AVX-LABEL: test_pcmpgtq:
; AVX:       # BB#0:
; AVX-NEXT:    vpcmpeqd %xmm1, %xmm1, %xmm1
; AVX-NEXT:    vpcmpgtq %xmm1, %xmm0, %xmm0
; AVX-NEXT:    retq
  %sign = ashr <2 x i64> %x, <i64 63, i64 63>
  %not = xor <2 x i64> %sign, <i64 -1, i64 -1>
  ret <2 x i64> %not
}

define <1 x i128> @test_strange_type(<1 x i128> %x) {
; SSE2-LABEL: test_strange_type:
; SSE2:       # BB#0:
; SSE2-NEXT:    sarq $63, %rsi
; SSE2-NEXT:    notq %rsi
; SSE2-NEXT:    movd %rsi, %xmm0
; SSE2-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,0,1]
; SSE2-NEXT:    movd %xmm0, %rax
; SSE2-NEXT:    movq %rsi, %rdx
; SSE2-NEXT:    retq
;
; SSE42-LABEL: test_strange_type:
; SSE42:       # BB#0:
; SSE42-NEXT:    sarq $63, %rsi
; SSE42-NEXT:    notq %rsi
; SSE42-NEXT:    movd %rsi, %xmm0
; SSE42-NEXT:    pshufd {{.*#+}} xmm0 = xmm0[0,1,0,1]
; SSE42-NEXT:    movd %xmm0, %rax
; SSE42-NEXT:    pextrq $1, %xmm0, %rdx
; SSE42-NEXT:    retq
;
; AVX1-LABEL: test_strange_type:
; AVX1:       # BB#0:
; AVX1-NEXT:    sarq $63, %rsi
; AVX1-NEXT:    notq %rsi
; AVX1-NEXT:    vmovq %rsi, %xmm0
; AVX1-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[0,1,0,1]
; AVX1-NEXT:    vmovq %xmm0, %rax
; AVX1-NEXT:    vpextrq $1, %xmm0, %rdx
; AVX1-NEXT:    retq
;
; AVX2-LABEL: test_strange_type:
; AVX2:       # BB#0:
; AVX2-NEXT:    sarq $63, %rsi
; AVX2-NEXT:    notq %rsi
; AVX2-NEXT:    vmovq %rsi, %xmm0
; AVX2-NEXT:    vpbroadcastq %xmm0, %xmm0
; AVX2-NEXT:    vmovq %xmm0, %rax
; AVX2-NEXT:    vpextrq $1, %xmm0, %rdx
; AVX2-NEXT:    retq
  %sign = ashr <1 x i128> %x, <i128 127>
  %not = xor <1 x i128> %sign, <i128 -1>
  ret <1 x i128> %not
}

