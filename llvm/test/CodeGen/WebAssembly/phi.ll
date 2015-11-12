; RUN: llc < %s -asm-verbose=false | FileCheck %s

; Test that phis are lowered.

target datalayout = "e-p:32:32-i64:64-n32:64-S128"
target triple = "wasm32-unknown-unknown"

; Basic phi triangle.

; CHECK-LABEL: test0:
; CHECK: div_s $push, (get_local 0), (get_local 3){{$}}
; CHECK: set_local 0, $pop
; CHECK: return (get_local 0)
define i32 @test0(i32 %p) {
entry:
  %t = icmp slt i32 %p, 0
  br i1 %t, label %true, label %done
true:
  %a = sdiv i32 %p, 3
  br label %done
done:
  %s = phi i32 [ %a, %true ], [ %p, %entry ]
  ret i32 %s
}

; Swap phis.

; CHECK-LABEL: test1:
; CHECK: BB1_1:
; CHECK: set_local [[REG0:.*]], (get_local [[REG1:.*]])
; CHECK: set_local [[REG1]], (get_local [[REG2:.*]])
; CHECK: set_local [[REG2]], (get_local [[REG0]])
define i32 @test1(i32 %n) {
entry:
  br label %loop

loop:
  %a = phi i32 [ 0, %entry ], [ %b, %loop ]
  %b = phi i32 [ 1, %entry ], [ %a, %loop ]
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]

  %i.next = add i32 %i, 1
  %t = icmp slt i32 %i.next, %n
  br i1 %t, label %loop, label %exit

exit:
  ret i32 %a
}
