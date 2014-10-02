; Test that coverage instrumentation does not lose debug location.

; RUN: opt < %s -asan -asan-module -asan-coverage=1 -S | FileCheck %s

; C++ source:
; 1: struct A {
; 2:  int f();
; 3:  int x;
; 4: };
; 5:
; 6: int A::f() {
; 7:    return x;
; 8: }
; clang++ ../1.cc -O3 -g -S -emit-llvm  -fno-strict-aliasing
; and add sanitize_address to @_ZN1A1fEv

; Test that __sanitizer_cov call has !dbg pointing to the opening { of A::f().
; CHECK: call void @__sanitizer_cov(), !dbg [[A:!.*]]
; CHECK: [[A]] = metadata !{i32 6, i32 0, metadata !{{.*}}, null}


target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.A = type { i32 }

; Function Attrs: nounwind readonly uwtable
define i32 @_ZN1A1fEv(%struct.A* nocapture readonly %this) #0 align 2 {
entry:
  tail call void @llvm.dbg.value(metadata !{%struct.A* %this}, i64 0, metadata !15, metadata !{metadata !"0x102"}), !dbg !20
  %x = getelementptr inbounds %struct.A* %this, i64 0, i32 0, !dbg !21
  %0 = load i32* %x, align 4, !dbg !21
  ret i32 %0, !dbg !21
}

; Function Attrs: nounwind readnone
declare void @llvm.dbg.value(metadata, i64, metadata, metadata) #1

attributes #0 = { sanitize_address nounwind readonly uwtable "less-precise-fpmad"="false" "no-frame-pointer-elim"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "stack-protector-buffer-size"="8" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!17, !18}
!llvm.ident = !{!19}

!0 = metadata !{metadata !"0x11\004\00clang version 3.5.0 (210251)\001\00\000\00\001", metadata !1, metadata !2, metadata !3, metadata !12, metadata !2, metadata !2} ; [ DW_TAG_compile_unit ] [/code/llvm/build0/../1.cc] [DW_LANG_C_plus_plus]
!1 = metadata !{metadata !"../1.cc", metadata !"/code/llvm/build0"}
!2 = metadata !{}
!3 = metadata !{metadata !4}
!4 = metadata !{metadata !"0x13\00A\001\0032\0032\000\000\000", metadata !1, null, null, metadata !5, null, null, metadata !"_ZTS1A"} ; [ DW_TAG_structure_type ] [A] [line 1, size 32, align 32, offset 0] [def] [from ]
!5 = metadata !{metadata !6, metadata !8}
!6 = metadata !{metadata !"0xd\00x\003\0032\0032\000\000", metadata !1, metadata !"_ZTS1A", metadata !7} ; [ DW_TAG_member ] [x] [line 3, size 32, align 32, offset 0] [from int]
!7 = metadata !{metadata !"0x24\00int\000\0032\0032\000\000\005", null, null} ; [ DW_TAG_base_type ] [int] [line 0, size 32, align 32, offset 0, enc DW_ATE_signed]
!8 = metadata !{metadata !"0x2e\00f\00f\00_ZN1A1fEv\002\000\000\000\006\00256\001\002", metadata !1, metadata !"_ZTS1A", metadata !9, null, null, null, i32 0, null} ; [ DW_TAG_subprogram ] [line 2] [f]
!9 = metadata !{metadata !"0x15\00\000\000\000\000\000\000", i32 0, null, null, metadata !10, null, null, null} ; [ DW_TAG_subroutine_type ] [line 0, size 0, align 0, offset 0] [from ]
!10 = metadata !{metadata !7, metadata !11}
!11 = metadata !{metadata !"0xf\00\000\0064\0064\000\001088", null, null, metadata !"_ZTS1A"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [artificial] [from _ZTS1A]
!12 = metadata !{metadata !13}
!13 = metadata !{metadata !"0x2e\00f\00f\00_ZN1A1fEv\006\000\001\000\006\00256\001\006", metadata !1, metadata !"_ZTS1A", metadata !9, null, i32 (%struct.A*)* @_ZN1A1fEv, null, metadata !8, metadata !14} ; [ DW_TAG_subprogram ] [line 6] [def] [f]
!14 = metadata !{metadata !15}
!15 = metadata !{metadata !"0x101\00this\0016777216\001088", metadata !13, null, metadata !16} ; [ DW_TAG_arg_variable ] [this] [line 0]
!16 = metadata !{metadata !"0xf\00\000\0064\0064\000\000", null, null, metadata !"_ZTS1A"} ; [ DW_TAG_pointer_type ] [line 0, size 64, align 64, offset 0] [from _ZTS1A]
!17 = metadata !{i32 2, metadata !"Dwarf Version", i32 4}
!18 = metadata !{i32 2, metadata !"Debug Info Version", i32 2}
!19 = metadata !{metadata !"clang version 3.5.0 (210251)"}
!20 = metadata !{i32 0, i32 0, metadata !13, null}
!21 = metadata !{i32 7, i32 0, metadata !13, null}
