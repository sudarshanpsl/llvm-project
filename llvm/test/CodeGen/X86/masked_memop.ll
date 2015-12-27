; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc -mtriple=x86_64-apple-darwin  -mcpu=knl < %s | FileCheck %s --check-prefix=AVX512
; RUN: llc -mtriple=x86_64-apple-darwin  -mcpu=core-avx2 < %s | FileCheck %s --check-prefix=AVX2
; RUN: opt -mtriple=x86_64-apple-darwin -codegenprepare -mcpu=corei7-avx -S < %s | FileCheck %s --check-prefix=AVX_SCALAR
; RUN: llc -mtriple=x86_64-apple-darwin  -mcpu=skx < %s | FileCheck %s --check-prefix=SKX

; AVX512-LABEL: test1
; AVX512: vmovdqu32       (%rdi), %zmm0 {%k1} {z}

; AVX2-LABEL: test1
; AVX2: vpmaskmovd      {{.*}}(%rdi)
; AVX2: vpmaskmovd      {{.*}}(%rdi)
; AVX2-NOT: blend

; AVX_SCALAR-LABEL: test1
; AVX_SCALAR-NOT: masked
; AVX_SCALAR: extractelement
; AVX_SCALAR: insertelement
; AVX_SCALAR: extractelement
; AVX_SCALAR: insertelement
define <16 x i32> @test1(<16 x i32> %trigger, <16 x i32>* %addr) {
  %mask = icmp eq <16 x i32> %trigger, zeroinitializer
  %res = call <16 x i32> @llvm.masked.load.v16i32(<16 x i32>* %addr, i32 4, <16 x i1>%mask, <16 x i32>undef)
  ret <16 x i32> %res
}

; AVX512-LABEL: test2
; AVX512: vmovdqu32       (%rdi), %zmm0 {%k1} {z}

; AVX2-LABEL: test2
; AVX2: vpmaskmovd      {{.*}}(%rdi)
; AVX2: vpmaskmovd      {{.*}}(%rdi)
; AVX2-NOT: blend
define <16 x i32> @test2(<16 x i32> %trigger, <16 x i32>* %addr) {
  %mask = icmp eq <16 x i32> %trigger, zeroinitializer
  %res = call <16 x i32> @llvm.masked.load.v16i32(<16 x i32>* %addr, i32 4, <16 x i1>%mask, <16 x i32>zeroinitializer)
  ret <16 x i32> %res
}

; AVX512-LABEL: test3
; AVX512: vmovdqu32       %zmm1, (%rdi) {%k1}

; AVX_SCALAR-LABEL: test3
; AVX_SCALAR-NOT: masked
; AVX_SCALAR: extractelement
; AVX_SCALAR: store
; AVX_SCALAR: extractelement
; AVX_SCALAR: store
; AVX_SCALAR: extractelement
; AVX_SCALAR: store
define void @test3(<16 x i32> %trigger, <16 x i32>* %addr, <16 x i32> %val) {
  %mask = icmp eq <16 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v16i32(<16 x i32>%val, <16 x i32>* %addr, i32 4, <16 x i1>%mask)
  ret void
}

; AVX512-LABEL: test4
; AVX512: vmovups       (%rdi), %zmm{{.*{%k[1-7]}}}

; AVX2-LABEL: test4
; AVX2: vmaskmovps      {{.*}}(%rdi)
; AVX2: vmaskmovps      {{.*}}(%rdi)
; AVX2: blend
define <16 x float> @test4(<16 x i32> %trigger, <16 x float>* %addr, <16 x float> %dst) {
  %mask = icmp eq <16 x i32> %trigger, zeroinitializer
  %res = call <16 x float> @llvm.masked.load.v16f32(<16 x float>* %addr, i32 4, <16 x i1>%mask, <16 x float> %dst)
  ret <16 x float> %res
}

; AVX512-LABEL: test5
; AVX512: vmovupd (%rdi), %zmm1 {%k1}

; AVX2-LABEL: test5
; AVX2: vmaskmovpd
; AVX2: vblendvpd
; AVX2: vmaskmovpd
; AVX2: vblendvpd
define <8 x double> @test5(<8 x i32> %trigger, <8 x double>* %addr, <8 x double> %dst) {
  %mask = icmp eq <8 x i32> %trigger, zeroinitializer
  %res = call <8 x double> @llvm.masked.load.v8f64(<8 x double>* %addr, i32 4, <8 x i1>%mask, <8 x double>%dst)
  ret <8 x double> %res
}

; AVX2-LABEL: test6
; AVX2: vmaskmovpd
; AVX2: vblendvpd

; SKX-LABEL: test6
; SKX: vmovupd {{.*}}{%k1}
define <2 x double> @test6(<2 x i64> %trigger, <2 x double>* %addr, <2 x double> %dst) {
  %mask = icmp eq <2 x i64> %trigger, zeroinitializer
  %res = call <2 x double> @llvm.masked.load.v2f64(<2 x double>* %addr, i32 4, <2 x i1>%mask, <2 x double>%dst)
  ret <2 x double> %res
}

; AVX2-LABEL: test7
; AVX2: vmaskmovps      {{.*}}(%rdi)
; AVX2: blend

; SKX-LABEL: test7
; SKX: vmovups (%rdi){{.*}}{%k1}
define <4 x float> @test7(<4 x i32> %trigger, <4 x float>* %addr, <4 x float> %dst) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  %res = call <4 x float> @llvm.masked.load.v4f32(<4 x float>* %addr, i32 4, <4 x i1>%mask, <4 x float>%dst)
  ret <4 x float> %res
}

; AVX2-LABEL: test8
; AVX2: vpmaskmovd      {{.*}}(%rdi)
; AVX2: blend

; SKX-LABEL: test8
; SKX: vmovdqu32 (%rdi){{.*}}{%k1}
define <4 x i32> @test8(<4 x i32> %trigger, <4 x i32>* %addr, <4 x i32> %dst) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  %res = call <4 x i32> @llvm.masked.load.v4i32(<4 x i32>* %addr, i32 4, <4 x i1>%mask, <4 x i32>%dst)
  ret <4 x i32> %res
}

; AVX2-LABEL: test9
; AVX2: vpmaskmovd %xmm

; SKX-LABEL: test9
; SKX: vmovdqu32 %xmm{{.*}}{%k1}
define void @test9(<4 x i32> %trigger, <4 x i32>* %addr, <4 x i32> %val) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v4i32(<4 x i32>%val, <4 x i32>* %addr, i32 4, <4 x i1>%mask)
  ret void
}

; AVX2-LABEL: test10
; AVX2: vmaskmovpd    (%rdi), %ymm
; AVX2: blend

; SKX-LABEL: test10
; SKX: vmovapd {{.*}}{%k1}
define <4 x double> @test10(<4 x i32> %trigger, <4 x double>* %addr, <4 x double> %dst) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  %res = call <4 x double> @llvm.masked.load.v4f64(<4 x double>* %addr, i32 32, <4 x i1>%mask, <4 x double>%dst)
  ret <4 x double> %res
}

; AVX2-LABEL: test11a
; AVX2: vmaskmovps
; AVX2: vblendvps

; SKX-LABEL: test11a
; SKX: vmovaps (%rdi), %ymm1 {%k1}
; AVX512-LABEL: test11a
; AVX512: kshiftlw $8
; AVX512: kshiftrw $8
; AVX512: vmovups (%rdi), %zmm1 {%k1}
define <8 x float> @test11a(<8 x i32> %trigger, <8 x float>* %addr, <8 x float> %dst) {
  %mask = icmp eq <8 x i32> %trigger, zeroinitializer
  %res = call <8 x float> @llvm.masked.load.v8f32(<8 x float>* %addr, i32 32, <8 x i1>%mask, <8 x float>%dst)
  ret <8 x float> %res
}

; SKX-LABEL: test11b
; SKX: vmovdqu32 (%rdi), %ymm1 {%k1}
; AVX512-LABEL: test11b
; AVX512: kshiftlw        $8
; AVX512: kshiftrw        $8
; AVX512: vmovdqu32 (%rdi), %zmm1 {%k1}
define <8 x i32> @test11b(<8 x i1> %mask, <8 x i32>* %addr, <8 x i32> %dst) {
  %res = call <8 x i32> @llvm.masked.load.v8i32(<8 x i32>* %addr, i32 4, <8 x i1>%mask, <8 x i32>%dst)
  ret <8 x i32> %res
}

; SKX-LABEL: test11c
; SKX: vmovaps (%rdi), %ymm0 {%k1} {z}
; AVX512-LABEL: test11c
; AVX512: kshiftlw  $8
; AVX512: kshiftrw  $8
; AVX512: vmovups (%rdi), %zmm0 {%k1} {z}
define <8 x float> @test11c(<8 x i1> %mask, <8 x float>* %addr) {
  %res = call <8 x float> @llvm.masked.load.v8f32(<8 x float>* %addr, i32 32, <8 x i1> %mask, <8 x float> zeroinitializer)
  ret <8 x float> %res
}

; SKX-LABEL: test11d
; SKX: vmovdqu32 (%rdi), %ymm0 {%k1} {z}
; AVX512-LABEL: test11d
; AVX512: kshiftlw  $8
; AVX512: kshiftrw  $8
; AVX512: vmovdqu32 (%rdi), %zmm0 {%k1} {z}
define <8 x i32> @test11d(<8 x i1> %mask, <8 x i32>* %addr) {
  %res = call <8 x i32> @llvm.masked.load.v8i32(<8 x i32>* %addr, i32 4, <8 x i1> %mask, <8 x i32> zeroinitializer)
  ret <8 x i32> %res
}

; AVX2-LABEL: test12
; AVX2: vpmaskmovd %ymm

; SKX-LABEL: test12
; SKX: vmovdqu32 {{.*}}{%k1}
define void @test12(<8 x i32> %trigger, <8 x i32>* %addr, <8 x i32> %val) {
  %mask = icmp eq <8 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v8i32(<8 x i32>%val, <8 x i32>* %addr, i32 4, <8 x i1>%mask)
  ret void
}

; AVX512-LABEL: test13
; AVX512: vmovups       %zmm1, (%rdi) {%k1}

define void @test13(<16 x i32> %trigger, <16 x float>* %addr, <16 x float> %val) {
  %mask = icmp eq <16 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v16f32(<16 x float>%val, <16 x float>* %addr, i32 4, <16 x i1>%mask)
  ret void
}

; AVX2-LABEL: test14
; AVX2: vpshufd
; AVX2: vmovq
; AVX2: vmaskmovps

; SKX-LABEL: test14
; SKX: kshiftl
; SKX: kshiftr
; SKX: vmovups {{.*}}{%k1}

define void @test14(<2 x i32> %trigger, <2 x float>* %addr, <2 x float> %val) {
  %mask = icmp eq <2 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v2f32(<2 x float>%val, <2 x float>* %addr, i32 4, <2 x i1>%mask)
  ret void
}

; AVX2-LABEL: test15
; AVX2: vpmaskmovd

; SKX-LABEL: test15:
; SKX:       ## BB#0:
; SKX-NEXT:    vpxor %xmm2, %xmm2, %xmm2
; SKX-NEXT:    vpblendd {{.*#+}} xmm0 = xmm0[0],xmm2[1],xmm0[2],xmm2[3]
; SKX-NEXT:    vpcmpeqq %xmm2, %xmm0, %k1
; SKX-NEXT:    vpmovqd %xmm1, (%rdi) {%k1}
; SKX-NEXT:    retq
define void @test15(<2 x i32> %trigger, <2 x i32>* %addr, <2 x i32> %val) {
  %mask = icmp eq <2 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v2i32(<2 x i32>%val, <2 x i32>* %addr, i32 4, <2 x i1>%mask)
  ret void
}

; AVX2-LABEL: test16
; AVX2: vmaskmovps
; AVX2: vblendvps

; SKX-LABEL: test16
; SKX: kshiftl
; SKX: kshiftr
; SKX: vmovups {{.*}}{%k1}
define <2 x float> @test16(<2 x i32> %trigger, <2 x float>* %addr, <2 x float> %dst) {
  %mask = icmp eq <2 x i32> %trigger, zeroinitializer
  %res = call <2 x float> @llvm.masked.load.v2f32(<2 x float>* %addr, i32 4, <2 x i1>%mask, <2 x float>%dst)
  ret <2 x float> %res
}

; AVX2-LABEL: test17
; AVX2: vpmaskmovd
; AVX2: vblendvps
; AVX2: vpmovsxdq

; SKX-LABEL: test17
; SKX: kshiftl
; SKX: kshiftr
; SKX: vmovdqu32 {{.*}}{%k1}
define <2 x i32> @test17(<2 x i32> %trigger, <2 x i32>* %addr, <2 x i32> %dst) {
  %mask = icmp eq <2 x i32> %trigger, zeroinitializer
  %res = call <2 x i32> @llvm.masked.load.v2i32(<2 x i32>* %addr, i32 4, <2 x i1>%mask, <2 x i32>%dst)
  ret <2 x i32> %res
}

; AVX2-LABEL: test18
; AVX2: vmaskmovps
; AVX2-NOT: blend
; AVX2: ret
define <2 x float> @test18(<2 x i32> %trigger, <2 x float>* %addr) {
; SKX-LABEL: test18:
; SKX:       ## BB#0:
; SKX-NEXT:    vpxor %xmm1, %xmm1, %xmm1
; SKX-NEXT:    vpblendd {{.*#+}} xmm0 = xmm0[0],xmm1[1],xmm0[2],xmm1[3]
; SKX-NEXT:    vpcmpeqq %xmm1, %xmm0, %k0
; SKX-NEXT:    kshiftlw $2, %k0, %k0
; SKX-NEXT:    kshiftrw $2, %k0, %k1
; SKX-NEXT:    vmovups (%rdi), %xmm0 {%k1} {z}
; SKX-NEXT:    retq
  %mask = icmp eq <2 x i32> %trigger, zeroinitializer
  %res = call <2 x float> @llvm.masked.load.v2f32(<2 x float>* %addr, i32 4, <2 x i1>%mask, <2 x float>undef)
  ret <2 x float> %res
}

; AVX_SCALAR-LABEL: test19
; AVX_SCALAR: load <4 x float>, <4 x float>* %addr, align 4

define <4 x float> @test19(<4 x i32> %trigger, <4 x float>* %addr) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  %res = call <4 x float> @llvm.masked.load.v4f32(<4 x float>* %addr, i32 4, <4 x i1><i1 true, i1 true, i1 true, i1 true>, <4 x float>undef)
  ret <4 x float> %res
}

; AVX_SCALAR-LABEL: test20
; AVX_SCALAR: load float, {{.*}}, align 4
; AVX_SCALAR: insertelement <4 x float> undef, float
; AVX_SCALAR: select <4 x i1> <i1 true, i1 false, i1 true, i1 true>

define <4 x float> @test20(<4 x i32> %trigger, <4 x float>* %addr, <4 x float> %src0) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  %res = call <4 x float> @llvm.masked.load.v4f32(<4 x float>* %addr, i32 16, <4 x i1><i1 true, i1 false, i1 true, i1 true>, <4 x float> %src0)
  ret <4 x float> %res
}

; AVX_SCALAR-LABEL: test21
; AVX_SCALAR: store <4 x i32> %val
define void @test21(<4 x i32> %trigger, <4 x i32>* %addr, <4 x i32> %val) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v4i32(<4 x i32>%val, <4 x i32>* %addr, i32 4, <4 x i1><i1 true, i1 true, i1 true, i1 true>)
  ret void
}

; AVX_SCALAR-LABEL: test22
; AVX_SCALAR: extractelement <4 x i32> %val, i32 0
; AVX_SCALAR:  store i32
define void @test22(<4 x i32> %trigger, <4 x i32>* %addr, <4 x i32> %val) {
  %mask = icmp eq <4 x i32> %trigger, zeroinitializer
  call void @llvm.masked.store.v4i32(<4 x i32>%val, <4 x i32>* %addr, i32 4, <4 x i1><i1 true, i1 false, i1 false, i1 false>)
  ret void
}

declare <16 x i32> @llvm.masked.load.v16i32(<16 x i32>*, i32, <16 x i1>, <16 x i32>)
declare <4 x i32> @llvm.masked.load.v4i32(<4 x i32>*, i32, <4 x i1>, <4 x i32>)
declare <2 x i32> @llvm.masked.load.v2i32(<2 x i32>*, i32, <2 x i1>, <2 x i32>)
declare void @llvm.masked.store.v16i32(<16 x i32>, <16 x i32>*, i32, <16 x i1>)
declare void @llvm.masked.store.v8i32(<8 x i32>, <8 x i32>*, i32, <8 x i1>)
declare void @llvm.masked.store.v4i32(<4 x i32>, <4 x i32>*, i32, <4 x i1>)
declare void @llvm.masked.store.v2f32(<2 x float>, <2 x float>*, i32, <2 x i1>)
declare void @llvm.masked.store.v2i32(<2 x i32>, <2 x i32>*, i32, <2 x i1>)
declare void @llvm.masked.store.v16f32(<16 x float>, <16 x float>*, i32, <16 x i1>)
declare void @llvm.masked.store.v16f32p(<16 x float>*, <16 x float>**, i32, <16 x i1>)
declare <16 x float> @llvm.masked.load.v16f32(<16 x float>*, i32, <16 x i1>, <16 x float>)
declare <8 x float> @llvm.masked.load.v8f32(<8 x float>*, i32, <8 x i1>, <8 x float>)
declare <8 x i32> @llvm.masked.load.v8i32(<8 x i32>*, i32, <8 x i1>, <8 x i32>)
declare <4 x float> @llvm.masked.load.v4f32(<4 x float>*, i32, <4 x i1>, <4 x float>)
declare <2 x float> @llvm.masked.load.v2f32(<2 x float>*, i32, <2 x i1>, <2 x float>)
declare <8 x double> @llvm.masked.load.v8f64(<8 x double>*, i32, <8 x i1>, <8 x double>)
declare <4 x double> @llvm.masked.load.v4f64(<4 x double>*, i32, <4 x i1>, <4 x double>)
declare <2 x double> @llvm.masked.load.v2f64(<2 x double>*, i32, <2 x i1>, <2 x double>)
declare void @llvm.masked.store.v8f64(<8 x double>, <8 x double>*, i32, <8 x i1>)
declare void @llvm.masked.store.v2f64(<2 x double>, <2 x double>*, i32, <2 x i1>)
declare void @llvm.masked.store.v2i64(<2 x i64>, <2 x i64>*, i32, <2 x i1>)

declare <16 x i32*> @llvm.masked.load.v16p0i32(<16 x i32*>*, i32, <16 x i1>, <16 x i32*>)

; AVX512-LABEL: test23
; AVX512: vmovdqu64       64(%rdi), %zmm1 {%k2} {z}
; AVX512: vmovdqu64       (%rdi), %zmm0 {%k1} {z}

define <16 x i32*> @test23(<16 x i32*> %trigger, <16 x i32*>* %addr) {
  %mask = icmp eq <16 x i32*> %trigger, zeroinitializer
  %res = call <16 x i32*> @llvm.masked.load.v16p0i32(<16 x i32*>* %addr, i32 4, <16 x i1>%mask, <16 x i32*>zeroinitializer)
  ret <16 x i32*> %res
}

%mystruct = type { i16, i16, [1 x i8*] }

declare <16 x %mystruct*> @llvm.masked.load.v16p0mystruct(<16 x %mystruct*>*, i32, <16 x i1>, <16 x %mystruct*>)

define <16 x %mystruct*> @test24(<16 x i1> %mask, <16 x %mystruct*>* %addr) {
; AVX512-LABEL: test24:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k1
; AVX512-NEXT:    vmovdqu64 (%rdi), %zmm0 {%k1} {z}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovdqu64 64(%rdi), %zmm1 {%k1} {z}
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test24:
; AVX2:       ## BB#0:
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vpmaskmovq (%rdi), %ymm1, %ymm4
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vpmaskmovq 96(%rdi), %ymm1, %ymm3
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vpmaskmovq 64(%rdi), %ymm1, %ymm2
; AVX2-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vpmaskmovq 32(%rdi), %ymm0, %ymm1
; AVX2-NEXT:    vmovdqa %ymm4, %ymm0
; AVX2-NEXT:    retq
;
; SKX-LABEL: test24:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %xmm0, %xmm0
; SKX-NEXT:    vpmovb2m %xmm0, %k1
; SKX-NEXT:    vmovdqu64 (%rdi), %zmm0 {%k1} {z}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovdqu64 64(%rdi), %zmm1 {%k1} {z}
; SKX-NEXT:    retq
  %res = call <16 x %mystruct*> @llvm.masked.load.v16p0mystruct(<16 x %mystruct*>* %addr, i32 4, <16 x i1>%mask, <16 x %mystruct*>zeroinitializer)
  ret <16 x %mystruct*> %res
}

define void @test_store_16i64(<16 x i64>* %ptrs, <16 x i1> %mask, <16 x i64> %src0)  {
; AVX512-LABEL: test_store_16i64:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k1
; AVX512-NEXT:    vmovdqu64 %zmm1, (%rdi) {%k1}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovdqu64 %zmm2, 64(%rdi) {%k1}
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test_store_16i64:
; AVX2:       ## BB#0:
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm5 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm5, %xmm5
; AVX2-NEXT:    vpsrad $31, %xmm5, %xmm5
; AVX2-NEXT:    vpmovsxdq %xmm5, %ymm5
; AVX2-NEXT:    vpmaskmovq %ymm1, %ymm5, (%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vpmaskmovq %ymm4, %ymm1, 96(%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vpmaskmovq %ymm3, %ymm1, 64(%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vpmaskmovq %ymm2, %ymm0, 32(%rdi)
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; SKX-LABEL: test_store_16i64:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %xmm0, %xmm0
; SKX-NEXT:    vpmovb2m %xmm0, %k1
; SKX-NEXT:    vmovdqu64 %zmm1, (%rdi) {%k1}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovdqu64 %zmm2, 64(%rdi) {%k1}
; SKX-NEXT:    retq
  call void @llvm.masked.store.v16i64(<16 x i64> %src0, <16 x i64>* %ptrs, i32 4, <16 x i1> %mask)
  ret void
}
declare void @llvm.masked.store.v16i64(<16 x i64> %src0, <16 x i64>* %ptrs, i32, <16 x i1> %mask)
define void @test_store_16f64(<16 x double>* %ptrs, <16 x i1> %mask, <16 x double> %src0)  {
; AVX512-LABEL: test_store_16f64:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k1
; AVX512-NEXT:    vmovupd %zmm1, (%rdi) {%k1}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovupd %zmm2, 64(%rdi) {%k1}
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test_store_16f64:
; AVX2:       ## BB#0:
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm5 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm5, %xmm5
; AVX2-NEXT:    vpsrad $31, %xmm5, %xmm5
; AVX2-NEXT:    vpmovsxdq %xmm5, %ymm5
; AVX2-NEXT:    vmaskmovpd %ymm1, %ymm5, (%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vmaskmovpd %ymm4, %ymm1, 96(%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm1 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm1[0],zero,zero,zero,xmm1[1],zero,zero,zero,xmm1[2],zero,zero,zero,xmm1[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vmaskmovpd %ymm3, %ymm1, 64(%rdi)
; AVX2-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vmaskmovpd %ymm2, %ymm0, 32(%rdi)
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; SKX-LABEL: test_store_16f64:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %xmm0, %xmm0
; SKX-NEXT:    vpmovb2m %xmm0, %k1
; SKX-NEXT:    vmovupd %zmm1, (%rdi) {%k1}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovupd %zmm2, 64(%rdi) {%k1}
; SKX-NEXT:    retq
  call void @llvm.masked.store.v16f64(<16 x double> %src0, <16 x double>* %ptrs, i32 4, <16 x i1> %mask)
  ret void
}
declare void @llvm.masked.store.v16f64(<16 x double> %src0, <16 x double>* %ptrs, i32, <16 x i1> %mask)
define <16 x i64> @test_load_16i64(<16 x i64>* %ptrs, <16 x i1> %mask, <16 x i64> %src0)  {
; AVX512-LABEL: test_load_16i64:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k1
; AVX512-NEXT:    vmovdqu64 (%rdi), %zmm1 {%k1}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovdqu64 64(%rdi), %zmm2 {%k1}
; AVX512-NEXT:    vmovaps %zmm1, %zmm0
; AVX512-NEXT:    vmovaps %zmm2, %zmm1
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test_load_16i64:
; AVX2:       ## BB#0:
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm5 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm5, %xmm5
; AVX2-NEXT:    vpsrad $31, %xmm5, %xmm5
; AVX2-NEXT:    vpmovsxdq %xmm5, %ymm5
; AVX2-NEXT:    vpmaskmovq (%rdi), %ymm5, %ymm9
; AVX2-NEXT:    vpshufd {{.*#+}} xmm7 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm7 = xmm7[0],zero,zero,zero,xmm7[1],zero,zero,zero,xmm7[2],zero,zero,zero,xmm7[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm7, %xmm7
; AVX2-NEXT:    vpsrad $31, %xmm7, %xmm7
; AVX2-NEXT:    vpmovsxdq %xmm7, %ymm7
; AVX2-NEXT:    vpmaskmovq 32(%rdi), %ymm7, %ymm8
; AVX2-NEXT:    vpshufd {{.*#+}} xmm6 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm6 = xmm6[0],zero,zero,zero,xmm6[1],zero,zero,zero,xmm6[2],zero,zero,zero,xmm6[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm6, %xmm6
; AVX2-NEXT:    vpsrad $31, %xmm6, %xmm6
; AVX2-NEXT:    vpmovsxdq %xmm6, %ymm6
; AVX2-NEXT:    vpmaskmovq 64(%rdi), %ymm6, %ymm10
; AVX2-NEXT:    vblendvpd %ymm5, %ymm9, %ymm1, %ymm5
; AVX2-NEXT:    vblendvpd %ymm7, %ymm8, %ymm2, %ymm1
; AVX2-NEXT:    vblendvpd %ymm6, %ymm10, %ymm3, %ymm2
; AVX2-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vpmaskmovq 96(%rdi), %ymm0, %ymm3
; AVX2-NEXT:    vblendvpd %ymm0, %ymm3, %ymm4, %ymm3
; AVX2-NEXT:    vmovapd %ymm5, %ymm0
; AVX2-NEXT:    retq
;
; SKX-LABEL: test_load_16i64:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %xmm0, %xmm0
; SKX-NEXT:    vpmovb2m %xmm0, %k1
; SKX-NEXT:    vmovdqu64 (%rdi), %zmm1 {%k1}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovdqu64 64(%rdi), %zmm2 {%k1}
; SKX-NEXT:    vmovaps %zmm1, %zmm0
; SKX-NEXT:    vmovaps %zmm2, %zmm1
; SKX-NEXT:    retq
  %res = call <16 x i64> @llvm.masked.load.v16i64(<16 x i64>* %ptrs, i32 4, <16 x i1> %mask, <16 x i64> %src0)
  ret <16 x i64> %res
}
declare <16 x i64> @llvm.masked.load.v16i64(<16 x i64>* %ptrs, i32, <16 x i1> %mask, <16 x i64> %src0)
define <16 x double> @test_load_16f64(<16 x double>* %ptrs, <16 x i1> %mask, <16 x double> %src0)  {
; AVX512-LABEL: test_load_16f64:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k1
; AVX512-NEXT:    vmovupd (%rdi), %zmm1 {%k1}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovupd 64(%rdi), %zmm2 {%k1}
; AVX512-NEXT:    vmovaps %zmm1, %zmm0
; AVX512-NEXT:    vmovaps %zmm2, %zmm1
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test_load_16f64:
; AVX2:       ## BB#0:
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm5 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm5, %xmm5
; AVX2-NEXT:    vpsrad $31, %xmm5, %xmm5
; AVX2-NEXT:    vpmovsxdq %xmm5, %ymm5
; AVX2-NEXT:    vmaskmovpd (%rdi), %ymm5, %ymm9
; AVX2-NEXT:    vpshufd {{.*#+}} xmm7 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm7 = xmm7[0],zero,zero,zero,xmm7[1],zero,zero,zero,xmm7[2],zero,zero,zero,xmm7[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm7, %xmm7
; AVX2-NEXT:    vpsrad $31, %xmm7, %xmm7
; AVX2-NEXT:    vpmovsxdq %xmm7, %ymm7
; AVX2-NEXT:    vmaskmovpd 32(%rdi), %ymm7, %ymm8
; AVX2-NEXT:    vpshufd {{.*#+}} xmm6 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm6 = xmm6[0],zero,zero,zero,xmm6[1],zero,zero,zero,xmm6[2],zero,zero,zero,xmm6[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm6, %xmm6
; AVX2-NEXT:    vpsrad $31, %xmm6, %xmm6
; AVX2-NEXT:    vpmovsxdq %xmm6, %ymm6
; AVX2-NEXT:    vmaskmovpd 64(%rdi), %ymm6, %ymm10
; AVX2-NEXT:    vblendvpd %ymm5, %ymm9, %ymm1, %ymm5
; AVX2-NEXT:    vblendvpd %ymm7, %ymm8, %ymm2, %ymm1
; AVX2-NEXT:    vblendvpd %ymm6, %ymm10, %ymm3, %ymm2
; AVX2-NEXT:    vpshufd {{.*#+}} xmm0 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vmaskmovpd 96(%rdi), %ymm0, %ymm3
; AVX2-NEXT:    vblendvpd %ymm0, %ymm3, %ymm4, %ymm3
; AVX2-NEXT:    vmovapd %ymm5, %ymm0
; AVX2-NEXT:    retq
;
; SKX-LABEL: test_load_16f64:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %xmm0, %xmm0
; SKX-NEXT:    vpmovb2m %xmm0, %k1
; SKX-NEXT:    vmovupd (%rdi), %zmm1 {%k1}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovupd 64(%rdi), %zmm2 {%k1}
; SKX-NEXT:    vmovaps %zmm1, %zmm0
; SKX-NEXT:    vmovaps %zmm2, %zmm1
; SKX-NEXT:    retq
  %res = call <16 x double> @llvm.masked.load.v16f64(<16 x double>* %ptrs, i32 4, <16 x i1> %mask, <16 x double> %src0)
  ret <16 x double> %res
}
declare <16 x double> @llvm.masked.load.v16f64(<16 x double>* %ptrs, i32, <16 x i1> %mask, <16 x double> %src0)

define <32 x double> @test_load_32f64(<32 x double>* %ptrs, <32 x i1> %mask, <32 x double> %src0)  {
; AVX512-LABEL: test_load_32f64:
; AVX512:       ## BB#0:
; AVX512-NEXT:    vextractf128 $1, %ymm0, %xmm5
; AVX512-NEXT:    vpmovsxbd %xmm5, %zmm5
; AVX512-NEXT:    vpslld $31, %zmm5, %zmm5
; AVX512-NEXT:    vptestmd %zmm5, %zmm5, %k1
; AVX512-NEXT:    vmovupd 128(%rdi), %zmm3 {%k1}
; AVX512-NEXT:    vpmovsxbd %xmm0, %zmm0
; AVX512-NEXT:    vpslld $31, %zmm0, %zmm0
; AVX512-NEXT:    vptestmd %zmm0, %zmm0, %k2
; AVX512-NEXT:    vmovupd (%rdi), %zmm1 {%k2}
; AVX512-NEXT:    kshiftrw $8, %k1, %k1
; AVX512-NEXT:    vmovupd 192(%rdi), %zmm4 {%k1}
; AVX512-NEXT:    kshiftrw $8, %k2, %k1
; AVX512-NEXT:    vmovupd 64(%rdi), %zmm2 {%k1}
; AVX512-NEXT:    vmovaps %zmm1, %zmm0
; AVX512-NEXT:    vmovaps %zmm2, %zmm1
; AVX512-NEXT:    vmovaps %zmm3, %zmm2
; AVX512-NEXT:    vmovaps %zmm4, %zmm3
; AVX512-NEXT:    retq
;
; AVX2-LABEL: test_load_32f64:
; AVX2:       ## BB#0:
; AVX2-NEXT:    pushq %rbp
; AVX2-NEXT:  Ltmp0:
; AVX2-NEXT:    .cfi_def_cfa_offset 16
; AVX2-NEXT:  Ltmp1:
; AVX2-NEXT:    .cfi_offset %rbp, -16
; AVX2-NEXT:    movq %rsp, %rbp
; AVX2-NEXT:  Ltmp2:
; AVX2-NEXT:    .cfi_def_cfa_register %rbp
; AVX2-NEXT:    andq $-32, %rsp
; AVX2-NEXT:    subq $32, %rsp
; AVX2-NEXT:    vpshufd {{.*#+}} xmm8 = xmm0[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm8 = xmm8[0],zero,zero,zero,xmm8[1],zero,zero,zero,xmm8[2],zero,zero,zero,xmm8[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm8, %xmm8
; AVX2-NEXT:    vpsrad $31, %xmm8, %xmm8
; AVX2-NEXT:    vpmovsxdq %xmm8, %ymm8
; AVX2-NEXT:    vmaskmovpd 32(%rsi), %ymm8, %ymm9
; AVX2-NEXT:    vpshufd {{.*#+}} xmm10 = xmm0[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm10 = xmm10[0],zero,zero,zero,xmm10[1],zero,zero,zero,xmm10[2],zero,zero,zero,xmm10[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm10, %xmm10
; AVX2-NEXT:    vpsrad $31, %xmm10, %xmm10
; AVX2-NEXT:    vpmovsxdq %xmm10, %ymm10
; AVX2-NEXT:    vmaskmovpd 64(%rsi), %ymm10, %ymm11
; AVX2-NEXT:    vpshufd {{.*#+}} xmm12 = xmm0[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm12 = xmm12[0],zero,zero,zero,xmm12[1],zero,zero,zero,xmm12[2],zero,zero,zero,xmm12[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm12, %xmm12
; AVX2-NEXT:    vpsrad $31, %xmm12, %xmm12
; AVX2-NEXT:    vpmovsxdq %xmm12, %ymm12
; AVX2-NEXT:    vmaskmovpd 96(%rsi), %ymm12, %ymm13
; AVX2-NEXT:    vblendvpd %ymm8, %ymm9, %ymm2, %ymm8
; AVX2-NEXT:    vblendvpd %ymm10, %ymm11, %ymm3, %ymm9
; AVX2-NEXT:    vblendvpd %ymm12, %ymm13, %ymm4, %ymm11
; AVX2-NEXT:    vextracti128 $1, %ymm0, %xmm2
; AVX2-NEXT:    vpshufd {{.*#+}} xmm3 = xmm2[1,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm3 = xmm3[0],zero,zero,zero,xmm3[1],zero,zero,zero,xmm3[2],zero,zero,zero,xmm3[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm3, %xmm3
; AVX2-NEXT:    vpsrad $31, %xmm3, %xmm3
; AVX2-NEXT:    vpmovsxdq %xmm3, %ymm3
; AVX2-NEXT:    vmaskmovpd 160(%rsi), %ymm3, %ymm10
; AVX2-NEXT:    vpshufd {{.*#+}} xmm4 = xmm2[2,3,0,1]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm4 = xmm4[0],zero,zero,zero,xmm4[1],zero,zero,zero,xmm4[2],zero,zero,zero,xmm4[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm4, %xmm4
; AVX2-NEXT:    vpsrad $31, %xmm4, %xmm4
; AVX2-NEXT:    vpmovsxdq %xmm4, %ymm4
; AVX2-NEXT:    vmaskmovpd 192(%rsi), %ymm4, %ymm12
; AVX2-NEXT:    vblendvpd %ymm3, %ymm10, %ymm6, %ymm3
; AVX2-NEXT:    vmovapd 16(%rbp), %ymm6
; AVX2-NEXT:    vblendvpd %ymm4, %ymm12, %ymm7, %ymm4
; AVX2-NEXT:    vpshufd {{.*#+}} xmm7 = xmm2[3,1,2,3]
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm7 = xmm7[0],zero,zero,zero,xmm7[1],zero,zero,zero,xmm7[2],zero,zero,zero,xmm7[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm7, %xmm7
; AVX2-NEXT:    vpsrad $31, %xmm7, %xmm7
; AVX2-NEXT:    vpmovsxdq %xmm7, %ymm7
; AVX2-NEXT:    vmaskmovpd 224(%rsi), %ymm7, %ymm10
; AVX2-NEXT:    vblendvpd %ymm7, %ymm10, %ymm6, %ymm6
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm0 = xmm0[0],zero,zero,zero,xmm0[1],zero,zero,zero,xmm0[2],zero,zero,zero,xmm0[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm0, %xmm0
; AVX2-NEXT:    vpsrad $31, %xmm0, %xmm0
; AVX2-NEXT:    vpmovsxdq %xmm0, %ymm0
; AVX2-NEXT:    vmaskmovpd (%rsi), %ymm0, %ymm7
; AVX2-NEXT:    vblendvpd %ymm0, %ymm7, %ymm1, %ymm0
; AVX2-NEXT:    vpmovzxbd {{.*#+}} xmm1 = xmm2[0],zero,zero,zero,xmm2[1],zero,zero,zero,xmm2[2],zero,zero,zero,xmm2[3],zero,zero,zero
; AVX2-NEXT:    vpslld $31, %xmm1, %xmm1
; AVX2-NEXT:    vpsrad $31, %xmm1, %xmm1
; AVX2-NEXT:    vpmovsxdq %xmm1, %ymm1
; AVX2-NEXT:    vmaskmovpd 128(%rsi), %ymm1, %ymm2
; AVX2-NEXT:    vblendvpd %ymm1, %ymm2, %ymm5, %ymm1
; AVX2-NEXT:    vmovapd %ymm1, 128(%rdi)
; AVX2-NEXT:    vmovapd %ymm0, (%rdi)
; AVX2-NEXT:    vmovapd %ymm6, 224(%rdi)
; AVX2-NEXT:    vmovapd %ymm4, 192(%rdi)
; AVX2-NEXT:    vmovapd %ymm3, 160(%rdi)
; AVX2-NEXT:    vmovapd %ymm11, 96(%rdi)
; AVX2-NEXT:    vmovapd %ymm9, 64(%rdi)
; AVX2-NEXT:    vmovapd %ymm8, 32(%rdi)
; AVX2-NEXT:    movq %rdi, %rax
; AVX2-NEXT:    movq %rbp, %rsp
; AVX2-NEXT:    popq %rbp
; AVX2-NEXT:    vzeroupper
; AVX2-NEXT:    retq
;
; SKX-LABEL: test_load_32f64:
; SKX:       ## BB#0:
; SKX-NEXT:    vpsllw $7, %ymm0, %ymm0
; SKX-NEXT:    vpmovb2m %ymm0, %k1
; SKX-NEXT:    vmovupd (%rdi), %zmm1 {%k1}
; SKX-NEXT:    kshiftrd $16, %k1, %k2
; SKX-NEXT:    vmovupd 128(%rdi), %zmm3 {%k2}
; SKX-NEXT:    kshiftrw $8, %k1, %k1
; SKX-NEXT:    vmovupd 64(%rdi), %zmm2 {%k1}
; SKX-NEXT:    kshiftrw $8, %k2, %k1
; SKX-NEXT:    vmovupd 192(%rdi), %zmm4 {%k1}
; SKX-NEXT:    vmovaps %zmm1, %zmm0
; SKX-NEXT:    vmovaps %zmm2, %zmm1
; SKX-NEXT:    vmovaps %zmm3, %zmm2
; SKX-NEXT:    vmovaps %zmm4, %zmm3
; SKX-NEXT:    retq
  %res = call <32 x double> @llvm.masked.load.v32f64(<32 x double>* %ptrs, i32 4, <32 x i1> %mask, <32 x double> %src0)
  ret <32 x double> %res
}
declare <32 x double> @llvm.masked.load.v32f64(<32 x double>* %ptrs, i32, <32 x i1> %mask, <32 x double> %src0)
