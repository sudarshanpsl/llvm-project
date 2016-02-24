; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -march=x86 -mattr=+mmx,+sse4.2 -mtriple=i686-apple-darwin9 | FileCheck %s

; MMX insertelement is not available; these are promoted to XMM.
; (Without SSE they are split to two ints, and the code is much better.)

define x86_mmx @mmx_movzl(x86_mmx %x) nounwind {
; CHECK-LABEL: mmx_movzl:
; CHECK:       ## BB#0:
; CHECK-NEXT:    subl $20, %esp
; CHECK-NEXT:    movq %mm0, {{[0-9]+}}(%esp)
; CHECK-NEXT:    pmovzxdq {{.*#+}} xmm0 = mem[0],zero,mem[1],zero
; CHECK-NEXT:    movl $32, %eax
; CHECK-NEXT:    pinsrd $0, %eax, %xmm0
; CHECK-NEXT:    pxor %xmm1, %xmm1
; CHECK-NEXT:    pblendw {{.*#+}} xmm1 = xmm0[0,1],xmm1[2,3,4,5,6,7]
; CHECK-NEXT:    movq %xmm1, (%esp)
; CHECK-NEXT:    movq (%esp), %mm0
; CHECK-NEXT:    addl $20, %esp
; CHECK-NEXT:    retl
  %tmp = bitcast x86_mmx %x to <2 x i32>
  %tmp3 = insertelement <2 x i32> %tmp, i32 32, i32 0		; <<2 x i32>> [#uses=1]
  %tmp8 = insertelement <2 x i32> %tmp3, i32 0, i32 1		; <<2 x i32>> [#uses=1]
  %tmp9 = bitcast <2 x i32> %tmp8 to x86_mmx
  ret x86_mmx %tmp9
}
