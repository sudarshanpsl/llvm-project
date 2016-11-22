; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mcpu=x86-64 -mattr=+avx512vl,+avx512dq | FileCheck %s --check-prefix=CHECK

define <4 x i32> @mask_shuffle_v4i32_1234(<4 x i32> %a, <4 x i32> %b, <4 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v4i32_1234:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} xmm2 {%k1} = xmm0[1,2,3],xmm1[0]
; CHECK-NEXT:    vmovdqa64 %xmm2, %xmm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 1, i32 2, i32 3, i32 4>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i32> %shuffle, <4 x i32> %passthru
  ret <4 x i32> %res
}

define <4 x i32> @maskz_shuffle_v4i32_1234(<4 x i32> %a, <4 x i32> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v4i32_1234:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} xmm0 {%k1} {z} = xmm0[1,2,3],xmm1[0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 1, i32 2, i32 3, i32 4>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i32> %shuffle, <4 x i32> zeroinitializer
  ret <4 x i32> %res
}

define <4 x i32> @mask_shuffle_v4i32_2345(<4 x i32> %a, <4 x i32> %b, <4 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v4i32_2345:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} xmm2 {%k1} = xmm0[2,3],xmm1[0,1]
; CHECK-NEXT:    vmovdqa64 %xmm2, %xmm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 3, i32 4, i32 5>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i32> %shuffle, <4 x i32> %passthru
  ret <4 x i32> %res
}

define <4 x i32> @maskz_shuffle_v4i32_2345(<4 x i32> %a, <4 x i32> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v4i32_2345:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} xmm0 {%k1} {z} = xmm0[2,3],xmm1[0,1]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i32> %a, <4 x i32> %b, <4 x i32> <i32 2, i32 3, i32 4, i32 5>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i32> %shuffle, <4 x i32> zeroinitializer
  ret <4 x i32> %res
}

define <2 x i64> @mask_shuffle_v2i64_12(<2 x i64> %a, <2 x i64> %b, <2 x i64> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v2i64_12:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignq {{.*#+}} xmm2 {%k1} = xmm0[1],xmm1[0]
; CHECK-NEXT:    vmovdqa64 %xmm2, %xmm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <2 x i64> %a, <2 x i64> %b, <2 x i32> <i32 1, i32 2>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <2 x i32> <i32 0, i32 1>
  %res = select <2 x i1> %mask.extract, <2 x i64> %shuffle, <2 x i64> %passthru
  ret <2 x i64> %res
}

define <2 x i64> @maskz_shuffle_v2i64_12(<2 x i64> %a, <2 x i64> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v2i64_12:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignq {{.*#+}} xmm0 {%k1} {z} = xmm0[1],xmm1[0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <2 x i64> %a, <2 x i64> %b, <2 x i32> <i32 1, i32 2>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <2 x i32> <i32 0, i32 1>
  %res = select <2 x i1> %mask.extract, <2 x i64> %shuffle, <2 x i64> zeroinitializer
  ret <2 x i64> %res
}

define <4 x i64> @mask_shuffle_v4i64_1234(<4 x i64> %a, <4 x i64> %b, <4 x i64> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v4i64_1234:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignq {{.*#+}} ymm2 {%k1} = ymm0[1,2,3],ymm1[0]
; CHECK-NEXT:    vmovdqa64 %ymm2, %ymm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 2, i32 3, i32 4>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i64> %shuffle, <4 x i64> %passthru
  ret <4 x i64> %res
}

define <4 x i64> @maskz_shuffle_v4i64_1234(<4 x i64> %a, <4 x i64> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v4i64_1234:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignq {{.*#+}} ymm0 {%k1} {z} = ymm0[1,2,3],ymm1[0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> %b, <4 x i32> <i32 1, i32 2, i32 3, i32 4>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i64> %shuffle, <4 x i64> zeroinitializer
  ret <4 x i64> %res
}

define <4 x i64> @mask_shuffle_v4i64_1230(<4 x i64> %a, <4 x i64> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v4i64_1230:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vpermq {{.*#+}} ymm1 {%k1} = ymm0[1,2,3,0]
; CHECK-NEXT:    vmovdqa64 %ymm1, %ymm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> undef, <4 x i32> <i32 1, i32 2, i32 3, i32 0>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i64> %shuffle, <4 x i64> %passthru
  ret <4 x i64> %res
}

define <4 x i64> @maskz_shuffle_v4i64_1230(<4 x i64> %a, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v4i64_1230:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vpermq {{.*#+}} ymm0 {%k1} {z} = ymm0[1,2,3,0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <4 x i64> %a, <4 x i64> undef, <4 x i32> <i32 1, i32 2, i32 3, i32 0>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %mask.extract = shufflevector <8 x i1> %mask.cast, <8 x i1> undef, <4 x i32> <i32 0, i32 1, i32 2, i32 3>
  %res = select <4 x i1> %mask.extract, <4 x i64> %shuffle, <4 x i64> zeroinitializer
  ret <4 x i64> %res
}

define <8 x i32> @mask_shuffle_v8i32_12345678(<8 x i32> %a, <8 x i32> %b, <8 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v8i32_12345678:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm2 {%k1} = ymm0[1,2,3,4,5,6,7],ymm1[0]
; CHECK-NEXT:    vmovdqa64 %ymm2, %ymm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> %b, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> %passthru
  ret <8 x i32> %res
}

define <8 x i32> @maskz_shuffle_v8i32_12345678(<8 x i32> %a, <8 x i32> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v8i32_12345678:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm0 {%k1} {z} = ymm0[1,2,3,4,5,6,7],ymm1[0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> %b, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> zeroinitializer
  ret <8 x i32> %res
}

define <8 x i32> @mask_shuffle_v8i32_23456789(<8 x i32> %a, <8 x i32> %b, <8 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v8i32_23456789:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm2 {%k1} = ymm0[2,3,4,5,6,7],ymm1[0,1]
; CHECK-NEXT:    vmovdqa64 %ymm2, %ymm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> %b, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> %passthru
  ret <8 x i32> %res
}

define <8 x i32> @maskz_shuffle_v8i32_23456789(<8 x i32> %a, <8 x i32> %b, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v8i32_23456789:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm0 {%k1} {z} = ymm0[2,3,4,5,6,7],ymm1[0,1]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> %b, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 8, i32 9>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> zeroinitializer
  ret <8 x i32> %res
}

define <8 x i32> @mask_shuffle_v8i32_12345670(<8 x i32> %a, <8 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v8i32_12345670:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm1 {%k1} = ymm0[1,2,3,4,5,6,7,0]
; CHECK-NEXT:    vmovdqa64 %ymm1, %ymm0
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> undef, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 0>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> %passthru
  ret <8 x i32> %res
}

define <8 x i32> @maskz_shuffle_v8i32_12345670(<8 x i32> %a, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v8i32_12345670:
; CHECK:       # BB#0:
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    valignd {{.*#+}} ymm0 {%k1} {z} = ymm0[1,2,3,4,5,6,7,0]
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> undef, <8 x i32> <i32 1, i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 0>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> zeroinitializer
  ret <8 x i32> %res
}

define <8 x i32> @mask_shuffle_v8i32_23456701(<8 x i32> %a, <8 x i32> %passthru, i8 %mask) {
; CHECK-LABEL: mask_shuffle_v8i32_23456701:
; CHECK:       # BB#0:
; CHECK-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,2,3,0]
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vpblendmd %ymm0, %ymm1, %ymm0 {%k1}
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> undef, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> %passthru
  ret <8 x i32> %res
}

define <8 x i32> @maskz_shuffle_v8i32_23456701(<8 x i32> %a, i8 %mask) {
; CHECK-LABEL: maskz_shuffle_v8i32_23456701:
; CHECK:       # BB#0:
; CHECK-NEXT:    vpermq {{.*#+}} ymm0 = ymm0[1,2,3,0]
; CHECK-NEXT:    kmovb %edi, %k1
; CHECK-NEXT:    vmovdqa32 %ymm0, %ymm0 {%k1} {z}
; CHECK-NEXT:    retq
  %shuffle = shufflevector <8 x i32> %a, <8 x i32> undef, <8 x i32> <i32 2, i32 3, i32 4, i32 5, i32 6, i32 7, i32 0, i32 1>
  %mask.cast = bitcast i8 %mask to <8 x i1>
  %res = select <8 x i1> %mask.cast, <8 x i32> %shuffle, <8 x i32> zeroinitializer
  ret <8 x i32> %res
}
