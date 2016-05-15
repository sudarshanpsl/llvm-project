; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S -simplifycfg < %s | FileCheck %s

define i32 @test1(i32 %x) nounwind {
; CHECK-LABEL: @test1(
; CHECK-NEXT:  a:
; CHECK-NEXT:    [[I:%.*]] = shl i32 %x, 1
; CHECK-NEXT:    [[COND:%.*]] = icmp eq i32 [[I]], 24
; CHECK-NEXT:    [[DOT:%.*]] = select i1 [[COND]], i32 5, i32 0
; CHECK-NEXT:    ret i32 [[DOT]]
;
  %i = shl i32 %x, 1
  switch i32 %i, label %a [
  i32 21, label %b
  i32 24, label %c
  ]

a:
  ret i32 0
b:
  ret i32 3
c:
  ret i32 5
}


define i32 @test2(i32 %x) nounwind {
; CHECK-LABEL: @test2(
; CHECK-NEXT:  a:
; CHECK-NEXT:    ret i32 0
;
  %i = shl i32 %x, 1
  switch i32 %i, label %a [
  i32 21, label %b
  i32 23, label %c
  ]

a:
  ret i32 0
b:
  ret i32 3
c:
  ret i32 5
}
