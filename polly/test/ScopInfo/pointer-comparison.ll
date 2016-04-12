; RUN: opt %loadPolly -polly-scops -analyze < %s | FileCheck %s
;
; TODO: FIXME: Investigate why we need a InvalidContext here.
;
;    void f(int *A, int *B) {
;      while (A != B) {
;        *A = *A + 1;
;        A++;
;      }
;    }
;
; CHECK:      Invalid Context:
; CHECK-NEXT:   [A, B] -> { : (4*floor((-A + B)/4) = -A + B and B >= 9223372036854775808 + A) or (4*floor((A - B)/4) < A - B) or (4*floor((-A + B)/4) = -A + B and B <= -4 + A) }
;
; CHECK:      Domain :=
; CHECK-NEXT:   [A, B] -> { Stmt_while_body[i0] : 4*floor((A - B)/4) = A - B and B >= A and i0 >= 0 and 4i0 < -A + B }
;
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"

define void @f(i32* %A, i32* %B) {
entry:
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %A.addr.0 = phi i32* [ %A, %entry ], [ %incdec.ptr, %while.body ]
  %cmp = icmp eq i32* %A.addr.0, %B
  br i1 %cmp, label %while.end, label %while.body

while.body:                                       ; preds = %while.cond
  %tmp = load i32, i32* %A.addr.0, align 4
  %add = add nsw i32 %tmp, 1
  store i32 %add, i32* %A.addr.0, align 4
  %incdec.ptr = getelementptr inbounds i32, i32* %A.addr.0, i64 1
  br label %while.cond

while.end:                                        ; preds = %while.cond
  ret void
}
