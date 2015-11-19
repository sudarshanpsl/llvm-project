; RUN: llc < %s -mcpu=x86-64 -mattr=+avx | FileCheck %s --check-prefix=ALL --check-prefix=AVX1
; RUN: llc < %s -mcpu=x86-64 -mattr=+avx2 | FileCheck %s --check-prefix=ALL --check-prefix=AVX2
; RUN: llc < %s -mcpu=knl -mattr=+avx512vl | FileCheck %s --check-prefix=ALL --check-prefix=AVX512VL

target triple = "x86_64-unknown-unknown"

define <4 x double> @shuffle_v4f64_0000(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0000:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0000:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0000:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vbroadcastsd %xmm0, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0001(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0001:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm1 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0001:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,0,0,1]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0001:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,0,0,1]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0020(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0020:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0020:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,0,2,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0020:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,0,2,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0300(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0300:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm0[2,3,0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm1 = ymm1[0,1,2,2]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0300:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,3,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0300:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,3,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 3, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1000(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_1000:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_1000:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[1,0,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_1000:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[1,0,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_2200(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_2200:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3,0,1]
; AVX1-NEXT:    vmovddup {{.*#+}} ymm0 = ymm0[0,0,2,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_2200:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[2,2,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_2200:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[2,2,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 2, i32 2, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_3330(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_3330:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm0[2,3,0,1]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm1[0,1,2],ymm0[3]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,1,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_3330:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,3,3,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_3330:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,3,3,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_3210(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_3210:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3,0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_3210:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,2,1,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_3210:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,2,1,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0023(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0023:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[0,0,2,3]
; ALL-NEXT:    retq

  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0022(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0022:
; ALL:       # BB#0:
; ALL-NEXT:    vmovddup {{.*#+}} ymm0 = ymm0[0,0,2,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64mem_0022(<4 x double>* %ptr, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64mem_0022:
; ALL:       # BB#0:
; ALL-NEXT:    vmovddup {{.*#+}} ymm0 = mem[0,0,2,2]
; ALL-NEXT:    retq
  %a = load  <4 x double>,  <4 x double>* %ptr
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1032(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1032:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 3, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1133(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1133:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,1,3,3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 1, i32 3, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1023(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1023:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,2,3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 2, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1022(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1022:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,2,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 2, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0423(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0423:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} ymm1 = ymm1[0,0,2,2]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0423:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd %xmm1, %ymm1
; AVX2-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0423:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vbroadcastsd %xmm1, %ymm1
; AVX512VL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2,3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 4, i32 2, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0462(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0462:
; ALL:       # BB#0:
; ALL-NEXT:    vmovddup {{.*#+}} ymm1 = ymm1[0,0,2,2]
; ALL-NEXT:    vmovddup {{.*#+}} ymm0 = ymm0[0,0,2,2]
; ALL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2],ymm0[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 4, i32 6, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0426(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0426:
; ALL:       # BB#0:
; ALL-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 4, i32 2, i32 6>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1537(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1537:
; ALL:       # BB#0:
; ALL-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 5, i32 3, i32 7>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_4062(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_4062:
; ALL:       # BB#0:
; ALL-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 4, i32 0, i32 6, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_5173(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_5173:
; ALL:       # BB#0:
; ALL-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm1[1],ymm0[1],ymm1[3],ymm0[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 5, i32 1, i32 7, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_5163(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_5163:
; ALL:       # BB#0:
; ALL-NEXT:    vshufpd {{.*#+}} ymm0 = ymm1[1],ymm0[1],ymm1[2],ymm0[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 5, i32 1, i32 6, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0527(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0527:
; ALL:       # BB#0:
; ALL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2],ymm1[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 5, i32 2, i32 7>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_4163(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_4163:
; ALL:       # BB#0:
; ALL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm1[0],ymm0[1],ymm1[2],ymm0[3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 4, i32 1, i32 6, i32 3>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0145(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0145:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0145:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0145:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinsertf32x4 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 5>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_4501(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_4501:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_4501:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_4501:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinsertf32x4 $1, %xmm0, %ymm1, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 4, i32 5, i32 0, i32 1>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0167(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_0167:
; ALL:       # BB#0:
; ALL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 1, i32 6, i32 7>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1054(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_1054:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_1054:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_1054:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinsertf32x4 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 5, i32 4>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_3254(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_3254:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_3254:
; AVX2:       # BB#0:
; AVX2-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX2-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_3254:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX512VL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 3, i32 2, i32 5, i32 4>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_3276(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_3276:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_3276:
; AVX2:       # BB#0:
; AVX2-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX2-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_3276:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX512VL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 3, i32 2, i32 7, i32 6>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_1076(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_1076:
; ALL:       # BB#0:
; ALL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3]
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 0, i32 7, i32 6>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_0415(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_0415:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpckhpd {{.*#+}} xmm2 = xmm0[1],xmm1[1]
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_0415:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm1 = ymm1[0,0,2,1]
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,1,1,3]
; AVX2-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2],ymm1[3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_0415:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm1 = ymm1[0,0,2,1]
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[0,1,1,3]
; AVX512VL-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2],ymm1[3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 0, i32 4, i32 1, i32 5>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_u062(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_u062:
; ALL:       # BB#0:
; ALL-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 undef, i32 0, i32 6, i32 2>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_11uu(<4 x double> %a, <4 x double> %b) {
; ALL-LABEL: shuffle_v4f64_11uu:
; ALL:       # BB#0:
; ALL-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,1,2,2]
; ALL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 1, i32 1, i32 undef, i32 undef>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_22uu(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_22uu:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_22uu:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[2,2,2,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_22uu:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[2,2,2,3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 2, i32 2, i32 undef, i32 undef>
  ret <4 x double> %shuffle
}

define <4 x double> @shuffle_v4f64_3333(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: shuffle_v4f64_3333:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vmovhlps {{.*#+}} xmm0 = xmm0[1,1]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4f64_3333:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,3,3,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4f64_3333:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermpd {{.*#+}} ymm0 = ymm0[3,3,3,3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 3>
  ret <4 x double> %shuffle
}

define <4 x i64> @shuffle_v4i64_0000(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0000:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0000:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0000:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpbroadcastq %xmm0, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0001(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0001:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm1 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0001:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,0,1]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0001:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,0,1]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 0, i32 0, i32 1>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0020(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0020:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm1 = xmm1[0],xmm0[0]
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0020:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,2,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0020:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,2,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 0, i32 2, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0112(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0112:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm1
; AVX1-NEXT:    vshufpd {{.*#+}} xmm1 = xmm0[1],xmm1[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0112:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0112:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,2]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 1, i32 1, i32 2>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0300(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0300:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm0[2,3,0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm1 = ymm1[0,1,2,2]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0300:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,3,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0300:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,3,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 3, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_1000(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_1000:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm0[1,0]
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_1000:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,0,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_1000:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,0,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 0, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_2200(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_2200:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3,0,1]
; AVX1-NEXT:    vmovddup {{.*#+}} ymm0 = ymm0[0,0,2,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_2200:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,2,0,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_2200:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,2,0,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 2, i32 2, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_3330(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_3330:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm1 = ymm0[2,3,0,1]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm1[0,1,2],ymm0[3]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,1,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_3330:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,3,3,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_3330:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,3,3,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_3210(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_3210:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3,0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_3210:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,2,1,0]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_3210:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,2,1,0]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 3, i32 2, i32 1, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0124(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0124:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm1 = xmm1[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm1
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1,2],ymm1[3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0124:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpbroadcastq %xmm1, %ymm1
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3,4,5],ymm1[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0124:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpbroadcastq %xmm1, %ymm1
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3,4,5],ymm1[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 1, i32 2, i32 4>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0142(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0142:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm1, %ymm1
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[0,1,2,2]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2],ymm0[3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0142:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinserti128 $1, %xmm1, %ymm1, %ymm1
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,2,2]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5],ymm0[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0142:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm1, %ymm1, %ymm1
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,2,2]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5],ymm0[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 2>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0412(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0412:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vshufpd {{.*#+}} xmm2 = xmm0[1],xmm2[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    vmovddup {{.*#+}} ymm1 = ymm1[0,0,2,2]
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1],ymm0[2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0412:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,2]
; AVX2-NEXT:    vpbroadcastq %xmm1, %ymm1
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5,6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0412:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,2]
; AVX512VL-NEXT:    vpbroadcastq %xmm1, %ymm1
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5,6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 4, i32 1, i32 2>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_4012(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_4012:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm2
; AVX1-NEXT:    vshufpd {{.*#+}} xmm2 = xmm0[1],xmm2[0]
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm1[0],ymm0[1,2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_4012:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,1,2]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm1[0,1],ymm0[2,3,4,5,6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_4012:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,1,2]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm1[0,1],ymm0[2,3,4,5,6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 4, i32 0, i32 1, i32 2>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0145(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0145:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0145:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0145:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 1, i32 4, i32 5>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0451(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0451:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpckhpd {{.*#+}} xmm2 = xmm1[1],xmm0[1]
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0451:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinserti128 $1, %xmm0, %ymm0, %ymm0
; AVX2-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,0,1,3]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3,4,5],ymm0[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0451:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm0, %ymm0, %ymm0
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,0,1,3]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3,4,5],ymm0[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 4, i32 5, i32 1>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_4501(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_4501:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_4501:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm0, %ymm1, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_4501:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm0, %ymm1, %ymm0
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 4, i32 5, i32 0, i32 1>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_4015(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_4015:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpckhpd {{.*#+}} xmm2 = xmm0[1],xmm1[1]
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm1[0],xmm0[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_4015:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinserti128 $1, %xmm1, %ymm1, %ymm1
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,1,3]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm1[0,1],ymm0[2,3,4,5],ymm1[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_4015:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm1, %ymm1, %ymm1
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,0,1,3]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm1[0,1],ymm0[2,3,4,5],ymm1[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 4, i32 0, i32 1, i32 5>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_2u35(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_2u35:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vunpckhpd {{.*#+}} xmm1 = xmm0[1],xmm1[1]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_2u35:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5,6,7]
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,1,3,1]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_2u35:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5,6,7]
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,1,3,1]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 2, i32 undef, i32 3, i32 5>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_1251(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_1251:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm2 = ymm0[2,3,0,1]
; AVX1-NEXT:    vshufpd {{.*#+}} ymm0 = ymm0[1],ymm2[0],ymm0[2],ymm2[3]
; AVX1-NEXT:    vpermilpd {{.*#+}} xmm1 = xmm1[1,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm1
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2],ymm0[3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_1251:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,1,1,3]
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,2,2,1]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5],ymm0[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_1251:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,1,1,3]
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,2,2,1]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5],ymm0[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 2, i32 5, i32 1>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_1054(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_1054:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_1054:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinserti128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_1054:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 0, i32 5, i32 4>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_3254(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_3254:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_3254:
; AVX2:       # BB#0:
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX2-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_3254:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[0,1]
; AVX512VL-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 3, i32 2, i32 5, i32 4>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_3276(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_3276:
; AVX1:       # BB#0:
; AVX1-NEXT:    vperm2f128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_3276:
; AVX2:       # BB#0:
; AVX2-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX2-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_3276:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vperm2i128 {{.*#+}} ymm0 = ymm0[2,3],ymm1[2,3]
; AVX512VL-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 3, i32 2, i32 7, i32 6>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_1076(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_1076:
; AVX1:       # BB#0:
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3]
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,0,3,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_1076:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5,6,7]
; AVX2-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_1076:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5,6,7]
; AVX512VL-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,0,1,6,7,4,5]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 0, i32 7, i32 6>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_0415(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_0415:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpckhpd {{.*#+}} xmm2 = xmm0[1],xmm1[1]
; AVX1-NEXT:    vunpcklpd {{.*#+}} xmm0 = xmm0[0],xmm1[0]
; AVX1-NEXT:    vinsertf128 $1, %xmm2, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_0415:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,0,2,1]
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,3]
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5],ymm1[6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_0415:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm1 = ymm1[0,0,2,1]
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[0,1,1,3]
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3],ymm0[4,5],ymm1[6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 0, i32 4, i32 1, i32 5>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_z4z6(<4 x i64> %a) {
; AVX1-LABEL: shuffle_v4i64_z4z6:
; AVX1:       # BB#0:
; AVX1-NEXT:    vxorpd %ymm1, %ymm1, %ymm1
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_z4z6:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpslldq {{.*#+}} ymm0 = zero,zero,zero,zero,zero,zero,zero,zero,ymm0[0,1,2,3,4,5,6,7],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[16,17,18,19,20,21,22,23]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_z4z6:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpslldq {{.*#+}} ymm0 = zero,zero,zero,zero,zero,zero,zero,zero,ymm0[0,1,2,3,4,5,6,7],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[16,17,18,19,20,21,22,23]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> zeroinitializer, <4 x i64> %a, <4 x i32> <i32 0, i32 4, i32 0, i32 6>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_5zuz(<4 x i64> %a) {
; AVX1-LABEL: shuffle_v4i64_5zuz:
; AVX1:       # BB#0:
; AVX1-NEXT:    vxorpd %ymm1, %ymm1, %ymm1
; AVX1-NEXT:    vunpckhpd {{.*#+}} ymm0 = ymm0[1],ymm1[1],ymm0[3],ymm1[3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_5zuz:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpsrldq {{.*#+}} ymm0 = ymm0[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[24,25,26,27,28,29,30,31],zero,zero,zero,zero,zero,zero,zero,zero
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_5zuz:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpsrldq {{.*#+}} ymm0 = ymm0[8,9,10,11,12,13,14,15],zero,zero,zero,zero,zero,zero,zero,zero,ymm0[24,25,26,27,28,29,30,31],zero,zero,zero,zero,zero,zero,zero,zero
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> zeroinitializer, <4 x i64> %a, <4 x i32> <i32 5, i32 0, i32 undef, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_40u2(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_40u2:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_40u2:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_40u2:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpunpcklqdq {{.*#+}} ymm0 = ymm1[0],ymm0[0],ymm1[2],ymm0[2]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 4, i32 0, i32 undef, i32 2>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_11uu(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_11uu:
; AVX1:       # BB#0:
; AVX1-NEXT:    vpermilpd {{.*#+}} ymm0 = ymm0[1,1,2,2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_11uu:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,2,3,6,7,6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_11uu:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpshufd {{.*#+}} ymm0 = ymm0[2,3,2,3,6,7,6,7]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 1, i32 undef, i32 undef>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_22uu(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_22uu:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_22uu:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,2,2,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_22uu:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[2,2,2,3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 2, i32 2, i32 undef, i32 undef>
  ret <4 x i64> %shuffle
}

define <4 x i64> @shuffle_v4i64_3333(<4 x i64> %a, <4 x i64> %b) {
; AVX1-LABEL: shuffle_v4i64_3333:
; AVX1:       # BB#0:
; AVX1-NEXT:    vextractf128 $1, %ymm0, %xmm0
; AVX1-NEXT:    vpermilpd {{.*#+}} xmm0 = xmm0[1,1]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: shuffle_v4i64_3333:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,3,3,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: shuffle_v4i64_3333:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[3,3,3,3]
; AVX512VL-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 3, i32 3, i32 3, i32 3>
  ret <4 x i64> %shuffle
}

define <4 x i64> @stress_test1(<4 x i64> %a, <4 x i64> %b) {
; ALL-LABEL: stress_test1:
; ALL:         retq
  %c = shufflevector <4 x i64> %b, <4 x i64> undef, <4 x i32> <i32 3, i32 1, i32 1, i32 0>
  %d = shufflevector <4 x i64> %c, <4 x i64> undef, <4 x i32> <i32 3, i32 undef, i32 2, i32 undef>
  %e = shufflevector <4 x i64> %b, <4 x i64> undef, <4 x i32> <i32 3, i32 3, i32 1, i32 undef>
  %f = shufflevector <4 x i64> %d, <4 x i64> %e, <4 x i32> <i32 5, i32 1, i32 1, i32 0>

  ret <4 x i64> %f
}

define <4 x i64> @insert_reg_and_zero_v4i64(i64 %a) {
; ALL-LABEL: insert_reg_and_zero_v4i64:
; ALL:       # BB#0:
; ALL-NEXT:    vmovq %rdi, %xmm0
; ALL-NEXT:    retq
  %v = insertelement <4 x i64> undef, i64 %a, i64 0
  %shuffle = shufflevector <4 x i64> %v, <4 x i64> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x i64> %shuffle
}

define <4 x i64> @insert_mem_and_zero_v4i64(i64* %ptr) {
; AVX1-LABEL: insert_mem_and_zero_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovq {{.*#+}} xmm0 = mem[0],zero
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_mem_and_zero_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovq {{.*#+}} xmm0 = mem[0],zero
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: insert_mem_and_zero_v4i64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vmovq (%rdi), %xmm0
; AVX512VL-NEXT:    retq
  %a = load i64, i64* %ptr
  %v = insertelement <4 x i64> undef, i64 %a, i64 0
  %shuffle = shufflevector <4 x i64> %v, <4 x i64> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x i64> %shuffle
}

define <4 x double> @insert_reg_and_zero_v4f64(double %a) {
; AVX1-LABEL: insert_reg_and_zero_v4f64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vxorpd %ymm1, %ymm1, %ymm1
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_reg_and_zero_v4f64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vxorpd %ymm1, %ymm1, %ymm1
; AVX2-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0],ymm1[1,2,3]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: insert_reg_and_zero_v4f64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vxorpd %xmm1, %xmm1, %xmm1
; AVX512VL-NEXT:    vmovsd %xmm0, %xmm1, %xmm0
; AVX512VL-NEXT:    retq
  %v = insertelement <4 x double> undef, double %a, i32 0
  %shuffle = shufflevector <4 x double> %v, <4 x double> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x double> %shuffle
}

define <4 x double> @insert_mem_and_zero_v4f64(double* %ptr) {
; AVX1-LABEL: insert_mem_and_zero_v4f64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovsd {{.*#+}} xmm0 = mem[0],zero
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_mem_and_zero_v4f64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovsd {{.*#+}} xmm0 = mem[0],zero
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: insert_mem_and_zero_v4f64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vmovsd (%rdi), %xmm0
; AVX512VL-NEXT:    retq
  %a = load double, double* %ptr
  %v = insertelement <4 x double> undef, double %a, i32 0
  %shuffle = shufflevector <4 x double> %v, <4 x double> zeroinitializer, <4 x i32> <i32 0, i32 5, i32 6, i32 7>
  ret <4 x double> %shuffle
}

define <4 x double> @splat_mem_v4f64(double* %ptr) {
; ALL-LABEL: splat_mem_v4f64:
; ALL:       # BB#0:
; ALL-NEXT:    vbroadcastsd (%rdi), %ymm0
; ALL-NEXT:    retq
  %a = load double, double* %ptr
  %v = insertelement <4 x double> undef, double %a, i32 0
  %shuffle = shufflevector <4 x double> %v, <4 x double> undef, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x i64> @splat_mem_v4i64(i64* %ptr) {
; AVX1-LABEL: splat_mem_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat_mem_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat_mem_v4i64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpbroadcastq (%rdi), %ymm0
; AVX512VL-NEXT:    retq
  %a = load i64, i64* %ptr
  %v = insertelement <4 x i64> undef, i64 %a, i64 0
  %shuffle = shufflevector <4 x i64> %v, <4 x i64> undef, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x double> @splat_mem_v4f64_2(double* %p) {
; ALL-LABEL: splat_mem_v4f64_2:
; ALL:       # BB#0:
; ALL-NEXT:    vbroadcastsd (%rdi), %ymm0
; ALL-NEXT:    retq
  %1 = load double, double* %p
  %2 = insertelement <2 x double> undef, double %1, i32 0
  %3 = shufflevector <2 x double> %2, <2 x double> undef, <4 x i32> zeroinitializer
  ret <4 x double> %3
}

define <4 x double> @splat_v4f64(<2 x double> %r) {
; AVX1-LABEL: splat_v4f64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = xmm0[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat_v4f64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd %xmm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat_v4f64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vbroadcastsd %xmm0, %ymm0
; AVX512VL-NEXT:    retq
  %1 = shufflevector <2 x double> %r, <2 x double> undef, <4 x i32> zeroinitializer
  ret <4 x double> %1
}

define <4 x i64> @splat_mem_v4i64_from_v2i64(<2 x i64>* %ptr) {
; AVX1-LABEL: splat_mem_v4i64_from_v2i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = mem[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat_mem_v4i64_from_v2i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat_mem_v4i64_from_v2i64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vmovdqa64 (%rdi), %xmm0
; AVX512VL-NEXT:    vpbroadcastq %xmm0, %ymm0
; AVX512VL-NEXT:    retq
  %v = load <2 x i64>, <2 x i64>* %ptr
  %shuffle = shufflevector <2 x i64> %v, <2 x i64> undef, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x i64> %shuffle
}

define <4 x double> @splat_mem_v4f64_from_v2f64(<2 x double>* %ptr) {
; AVX1-LABEL: splat_mem_v4f64_from_v2f64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovddup {{.*#+}} xmm0 = mem[0,0]
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat_mem_v4f64_from_v2f64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat_mem_v4f64_from_v2f64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX512VL-NEXT:    retq
  %v = load <2 x double>, <2 x double>* %ptr
  %shuffle = shufflevector <2 x double> %v, <2 x double> undef, <4 x i32> <i32 0, i32 0, i32 0, i32 0>
  ret <4 x double> %shuffle
}

define <4 x i64> @splat128_mem_v4i64_from_v2i64(<2 x i64>* %ptr) {
; AVX1-LABEL: splat128_mem_v4i64_from_v2i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovaps (%rdi), %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat128_mem_v4i64_from_v2i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovaps (%rdi), %xmm0
; AVX2-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat128_mem_v4i64_from_v2i64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vmovdqa64 (%rdi), %xmm0
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm0, %ymm0, %ymm0
; AVX512VL-NEXT:    retq
  %v = load <2 x i64>, <2 x i64>* %ptr
  %shuffle = shufflevector <2 x i64> %v, <2 x i64> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 1>
  ret <4 x i64> %shuffle
}

define <4 x double> @splat128_mem_v4f64_from_v2f64(<2 x double>* %ptr) {
; AVX1-LABEL: splat128_mem_v4f64_from_v2f64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vmovaps (%rdi), %xmm0
; AVX1-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: splat128_mem_v4f64_from_v2f64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vmovaps (%rdi), %xmm0
; AVX2-NEXT:    vinsertf128 $1, %xmm0, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: splat128_mem_v4f64_from_v2f64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vmovapd (%rdi), %xmm0
; AVX512VL-NEXT:    vinsertf32x4 $1, %xmm0, %ymm0, %ymm0
; AVX512VL-NEXT:    retq
  %v = load <2 x double>, <2 x double>* %ptr
  %shuffle = shufflevector <2 x double> %v, <2 x double> undef, <4 x i32> <i32 0, i32 1, i32 0, i32 1>
  ret <4 x double> %shuffle
}

define <4 x double> @bitcast_v4f64_0426(<4 x double> %a, <4 x double> %b) {
; AVX1-LABEL: bitcast_v4f64_0426:
; AVX1:       # BB#0:
; AVX1-NEXT:    vunpcklpd {{.*#+}} ymm0 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: bitcast_v4f64_0426:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpunpcklqdq {{.*#+}} ymm0 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: bitcast_v4f64_0426:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpunpcklqdq {{.*#+}} ymm0 = ymm0[0],ymm1[0],ymm0[2],ymm1[2]
; AVX512VL-NEXT:    retq
  %shuffle64 = shufflevector <4 x double> %a, <4 x double> %b, <4 x i32> <i32 4, i32 0, i32 6, i32 2>
  %bitcast32 = bitcast <4 x double> %shuffle64 to <8 x float>
  %shuffle32 = shufflevector <8 x float> %bitcast32, <8 x float> undef, <8 x i32> <i32 3, i32 2, i32 1, i32 0, i32 7, i32 6, i32 5, i32 4>
  %bitcast16 = bitcast <8 x float> %shuffle32 to <16 x i16>
  %shuffle16 = shufflevector <16 x i16> %bitcast16, <16 x i16> undef, <16 x i32> <i32 2, i32 3, i32 0, i32 1, i32 6, i32 7, i32 4, i32 5, i32 10, i32 11, i32 8, i32 9, i32 14, i32 15, i32 12, i32 13>
  %bitcast64 = bitcast <16 x i16> %shuffle16 to <4 x double>
  ret <4 x double> %bitcast64
}

define <4 x i64> @concat_v4i64_0167(<4 x i64> %a0, <4 x i64> %a1) {
; AVX1-LABEL: concat_v4i64_0167:
; AVX1:       # BB#0:
; AVX1-NEXT:    vblendpd {{.*#+}} ymm0 = ymm0[0,1],ymm1[2,3]
; AVX1-NEXT:    retq
;
; AVX2-LABEL: concat_v4i64_0167:
; AVX2:       # BB#0:
; AVX2-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5,6,7]
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: concat_v4i64_0167:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpblendd {{.*#+}} ymm0 = ymm0[0,1,2,3],ymm1[4,5,6,7]
; AVX512VL-NEXT:    retq
  %a0lo = shufflevector <4 x i64> %a0, <4 x i64> %a1, <2 x i32> <i32 0, i32 1>
  %a1hi = shufflevector <4 x i64> %a0, <4 x i64> %a1, <2 x i32> <i32 6, i32 7>
  %shuffle64 = shufflevector <2 x i64> %a0lo, <2 x i64> %a1hi, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  ret <4 x i64> %shuffle64
}

define <4 x i64> @concat_v4i64_0145_bc(<4 x i64> %a0, <4 x i64> %a1) {
; AVX1-LABEL: concat_v4i64_0145_bc:
; AVX1:       # BB#0:
; AVX1-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: concat_v4i64_0145_bc:
; AVX2:       # BB#0:
; AVX2-NEXT:    vinsertf128 $1, %xmm1, %ymm0, %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: concat_v4i64_0145_bc:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vinserti32x4 $1, %xmm1, %ymm0, %ymm0
; AVX512VL-NEXT:    retq
  %a0lo = shufflevector <4 x i64> %a0, <4 x i64> %a1, <2 x i32> <i32 0, i32 1>
  %a1lo = shufflevector <4 x i64> %a0, <4 x i64> %a1, <2 x i32> <i32 4, i32 5>
  %bc0lo = bitcast <2 x i64> %a0lo to <4 x i32>
  %bc1lo = bitcast <2 x i64> %a1lo to <4 x i32>
  %shuffle32 = shufflevector <4 x i32> %bc0lo, <4 x i32> %bc1lo, <8 x i32> <i32 0, i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7>
  %shuffle64 = bitcast <8 x i32> %shuffle32 to <4 x i64>
  ret <4 x i64> %shuffle64
}

define <4 x i64> @insert_dup_mem_v4i64(i64* %ptr) {
; AVX1-LABEL: insert_dup_mem_v4i64:
; AVX1:       # BB#0:
; AVX1-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX1-NEXT:    retq
;
; AVX2-LABEL: insert_dup_mem_v4i64:
; AVX2:       # BB#0:
; AVX2-NEXT:    vbroadcastsd (%rdi), %ymm0
; AVX2-NEXT:    retq
;
; AVX512VL-LABEL: insert_dup_mem_v4i64:
; AVX512VL:       # BB#0:
; AVX512VL-NEXT:    vpbroadcastq (%rdi), %ymm0
; AVX512VL-NEXT:    retq
  %tmp = load i64, i64* %ptr, align 1
  %tmp1 = insertelement <2 x i64> undef, i64 %tmp, i32 0
  %tmp2 = shufflevector <2 x i64> %tmp1, <2 x i64> undef, <4 x i32> zeroinitializer
  ret <4 x i64> %tmp2
}
