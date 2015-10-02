; RUN: opt < %s -loop-unroll -indvars -disable-output

@b = external global i32, align 4

; Test that LoopUnroll does not break LCSSA form.
;
; In this function we have a following CFG:
;            ( entry )
;                |
;                v
;         ( outer.header ) <--
;                |             \
;                v              |
;     --> ( inner.header )      |
;   /       /          \        |
;   \      /            \       |
;    \    v              v     /
;  ( inner.latch )   ( outer.latch )
;         |
;         v
;     ( exit )
;
; When the inner loop is unrolled, we inner.latch block has only one
; predecessor and one successor, so it can be merged with exit block.
; During the merge, however, we remove an LCSSA definition for
; %storemerge1.lcssa, breaking LCSSA form for the outer loop.

; Function Attrs: nounwind uwtable
define void @fn1() #0 {
entry:
  br label %outer.header

outer.header:                                     ; preds = %outer.latch, %entry
  %storemerge1 = phi i32 [ 0, %entry ], [ %inc9, %outer.latch ]
  br label %inner.header

inner.header:                                     ; preds = %inner.latch, %outer.header
  %storemerge = phi i32 [ %add, %inner.latch ], [ 0, %outer.header ]
  %cmp = icmp slt i32 %storemerge, 1
  br i1 %cmp, label %inner.latch, label %outer.latch

inner.latch:                                      ; preds = %inner.header
  %tobool4 = icmp eq i32 %storemerge, 0
  %add = add nsw i32 %storemerge, 1
  br i1 %tobool4, label %inner.header, label %exit

exit:                                             ; preds = %inner.latch
  %storemerge1.lcssa = phi i32 [ %storemerge1, %inner.latch ]
  store i32 %storemerge1.lcssa, i32* @b, align 4
  ret void

outer.latch:                                      ; preds = %inner.header
  %inc9 = add nsw i32 %storemerge1, 1
  br label %outer.header
}
