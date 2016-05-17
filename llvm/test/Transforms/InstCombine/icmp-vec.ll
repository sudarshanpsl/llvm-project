; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt < %s -instcombine -S | FileCheck %s

; Canonicalize vector ge/le comparisons with constants to gt/lt.

; Normal types are ConstantDataVectors. Test the constant values adjacent to the
; min/max values that we're not allowed to transform.

define <2 x i1> @sge(<2 x i8> %x) {
; CHECK-LABEL: @sge(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt <2 x i8> %x, <i8 -128, i8 126>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sge <2 x i8> %x, <i8 -127, i8 -129>
  ret <2 x i1> %cmp
}

define <2 x i1> @uge(<2 x i8> %x) {
; CHECK-LABEL: @uge(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt <2 x i8> %x, <i8 -2, i8 0>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp uge <2 x i8> %x, <i8 -1, i8 1>
  ret <2 x i1> %cmp
}

define <2 x i1> @sle(<2 x i8> %x) {
; CHECK-LABEL: @sle(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt <2 x i8> %x, <i8 127, i8 -127>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sle <2 x i8> %x, <i8 126, i8 128>
  ret <2 x i1> %cmp
}

define <2 x i1> @ule(<2 x i8> %x) {
; CHECK-LABEL: @ule(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult <2 x i8> %x, <i8 -1, i8 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp ule <2 x i8> %x, <i8 254, i8 0>
  ret <2 x i1> %cmp
}

; Zeros are special: they're ConstantAggregateZero.

define <2 x i1> @sge_zero(<2 x i8> %x) {
; CHECK-LABEL: @sge_zero(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt <2 x i8> %x, <i8 -1, i8 -1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sge <2 x i8> %x, <i8 0, i8 0>
  ret <2 x i1> %cmp
}

define <2 x i1> @uge_zero(<2 x i8> %x) {
; CHECK-LABEL: @uge_zero(
; CHECK-NEXT:    ret <2 x i1> <i1 true, i1 true>
;
  %cmp = icmp uge <2 x i8> %x, <i8 0, i8 0>
  ret <2 x i1> %cmp
}

define <2 x i1> @sle_zero(<2 x i8> %x) {
; CHECK-LABEL: @sle_zero(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt <2 x i8> %x, <i8 1, i8 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sle <2 x i8> %x, <i8 0, i8 0>
  ret <2 x i1> %cmp
}

define <2 x i1> @ule_zero(<2 x i8> %x) {
; CHECK-LABEL: @ule_zero(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult <2 x i8> %x, <i8 1, i8 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp ule <2 x i8> %x, <i8 0, i8 0>
  ret <2 x i1> %cmp
}

; Weird types are ConstantVectors, not ConstantDataVectors. For an i3 type:
; Signed min = -4
; Unsigned min = 0
; Signed max = 3
; Unsigned max = 7

define <3 x i1> @sge_weird(<3 x i3> %x) {
; CHECK-LABEL: @sge_weird(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sgt <3 x i3> %x, <i3 -4, i3 2, i3 -1>
; CHECK-NEXT:    ret <3 x i1> [[CMP]]
;
  %cmp = icmp sge <3 x i3> %x, <i3 -3, i3 -5, i3 0>
  ret <3 x i1> %cmp
}

define <3 x i1> @uge_weird(<3 x i3> %x) {
; CHECK-LABEL: @uge_weird(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ugt <3 x i3> %x, <i3 -2, i3 0, i3 1>
; CHECK-NEXT:    ret <3 x i1> [[CMP]]
;
  %cmp = icmp uge <3 x i3> %x, <i3 -1, i3 1, i3 2>
  ret <3 x i1> %cmp
}

define <3 x i1> @sle_weird(<3 x i3> %x) {
; CHECK-LABEL: @sle_weird(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt <3 x i3> %x, <i3 3, i3 -3, i3 1>
; CHECK-NEXT:    ret <3 x i1> [[CMP]]
;
  %cmp = icmp sle <3 x i3> %x, <i3 2, i3 4, i3 0>
  ret <3 x i1> %cmp
}

define <3 x i1> @ule_weird(<3 x i3> %x) {
; CHECK-LABEL: @ule_weird(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ult <3 x i3> %x, <i3 -1, i3 1, i3 2>
; CHECK-NEXT:    ret <3 x i1> [[CMP]]
;
  %cmp = icmp ule <3 x i3> %x, <i3 6, i3 0, i3 1>
  ret <3 x i1> %cmp
}

; We can't do the transform if any constants are already at the limits.

define <2 x i1> @sge_min(<2 x i3> %x) {
; CHECK-LABEL: @sge_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sge <2 x i3> %x, <i3 -4, i3 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sge <2 x i3> %x, <i3 -4, i3 1>
  ret <2 x i1> %cmp
}

define <2 x i1> @uge_min(<2 x i3> %x) {
; CHECK-LABEL: @uge_min(
; CHECK-NEXT:    [[CMP:%.*]] = icmp uge <2 x i3> %x, <i3 1, i3 0>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp uge <2 x i3> %x, <i3 1, i3 0>
  ret <2 x i1> %cmp
}

define <2 x i1> @sle_max(<2 x i3> %x) {
; CHECK-LABEL: @sle_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle <2 x i3> %x, <i3 1, i3 3>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sle <2 x i3> %x, <i3 1, i3 3>
  ret <2 x i1> %cmp
}

define <2 x i1> @ule_max(<2 x i3> %x) {
; CHECK-LABEL: @ule_max(
; CHECK-NEXT:    [[CMP:%.*]] = icmp ule <2 x i3> %x, <i3 -1, i3 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp ule <2 x i3> %x, <i3 7, i3 1>
  ret <2 x i1> %cmp
}

; If we can't determine if a constant element is min/max (eg, it's a ConstantExpr), do nothing.

define <2 x i1> @PR27756_1(<2 x i8> %a) {
; CHECK-LABEL: @PR27756_1(
; CHECK-NEXT:    [[CMP:%.*]] = icmp sle <2 x i8> %a, <i8 bitcast (<2 x i4> <i4 1, i4 2> to i8), i8 0>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sle <2 x i8> %a, <i8 bitcast (<2 x i4> <i4 1, i4 2> to i8), i8 0>
  ret <2 x i1> %cmp
}

; Undef elements don't prevent the transform of the comparison.

define <2 x i1> @PR27756_2(<2 x i8> %a) {
; CHECK-LABEL: @PR27756_2(
; CHECK-NEXT:    [[CMP:%.*]] = icmp slt <2 x i8> %a, <i8 undef, i8 1>
; CHECK-NEXT:    ret <2 x i1> [[CMP]]
;
  %cmp = icmp sle <2 x i8> %a, <i8 undef, i8 0>
  ret <2 x i1> %cmp
}

