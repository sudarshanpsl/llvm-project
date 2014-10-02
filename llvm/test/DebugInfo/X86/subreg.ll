; RUN: llc %s -mtriple=x86_64-pc-linux-gnu -O0 -o - | FileCheck %s

; We are testing that a value in a 16 bit register gets reported as
; being in its superregister.

; CHECK: .byte   80                      # super-register
; CHECK-NEXT:                            # DW_OP_reg0
; CHECK-NEXT: .byte   147                # DW_OP_piece
; CHECK-NEXT: .byte   2                  # 2

define i16 @f(i16 signext %zzz) nounwind {
entry:
  call void @llvm.dbg.value(metadata !{i16 %zzz}, i64 0, metadata !0, metadata !{metadata !"0x102"})
  %conv = sext i16 %zzz to i32, !dbg !7
  %conv1 = trunc i32 %conv to i16
  ret i16 %conv1
}

declare void @llvm.dbg.value(metadata, i64, metadata, metadata) nounwind readnone

!llvm.dbg.cu = !{!3}
!llvm.module.flags = !{!11}
!9 = metadata !{metadata !1}

!0 = metadata !{metadata !"0x101\00zzz\0016777219\000", metadata !1, metadata !2, metadata !6} ; [ DW_TAG_arg_variable ]
!1 = metadata !{metadata !"0x2e\00f\00f\00\003\000\001\000\006\00256\000\003", metadata !10, metadata !2, metadata !4, null, i16 (i16)* @f, null, null, null} ; [ DW_TAG_subprogram ] [line 3] [def] [f]
!2 = metadata !{metadata !"0x29", metadata !10} ; [ DW_TAG_file_type ]
!3 = metadata !{metadata !"0x11\0012\00clang version 3.0 ()\000\00\000\00\001", metadata !10, metadata !5, metadata !5, metadata !9, null,  null} ; [ DW_TAG_compile_unit ]
!4 = metadata !{metadata !"0x15\00\000\000\000\000\000\000", metadata !10, metadata !2, null, metadata !5, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!5 = metadata !{null}
!6 = metadata !{metadata !"0x24\00short\000\0016\0016\000\000\005", null, metadata !3} ; [ DW_TAG_base_type ]
!7 = metadata !{i32 4, i32 22, metadata !8, null}
!8 = metadata !{metadata !"0xb\003\0019\000", metadata !10, metadata !1} ; [ DW_TAG_lexical_block ]
!10 = metadata !{metadata !"/home/espindola/llvm/test.c", metadata !"/home/espindola/tmpfs/build"}
!11 = metadata !{i32 1, metadata !"Debug Info Version", i32 2}
