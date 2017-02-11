; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown-unknown -mattr=+mmx,+3dnow | FileCheck %s --check-prefix=X32
; RUN: llc < %s -mtriple=x86_64-unknown-unknown -mattr=+mmx,+3dnow | FileCheck %s --check-prefix=X64

define void @commute_m_pfadd(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pfadd:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    pfadd (%eax), %mm0
; X32-NEXT:    pfadd (%ecx), %mm0
; X32-NEXT:    movq %mm0, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pfadd:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    pfadd (%rsi), %mm0
; X64-NEXT:    pfadd (%rdx), %mm0
; X64-NEXT:    movq %mm0, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pfadd(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pfadd(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pfadd(x86_mmx, x86_mmx)

; FIXME - missed PFSUB commutation.
define void @commute_m_pfsub(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pfsub:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    movq (%ecx), %mm1
; X32-NEXT:    pfsub (%eax), %mm0
; X32-NEXT:    pfsub %mm0, %mm1
; X32-NEXT:    movq %mm1, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pfsub:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    movq (%rdx), %mm1
; X64-NEXT:    pfsub (%rsi), %mm0
; X64-NEXT:    pfsub %mm0, %mm1
; X64-NEXT:    movq %mm1, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pfsub(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pfsub(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pfsub(x86_mmx, x86_mmx)

; FIXME - missed PFSUBR commutation.
define void @commute_m_pfsubr(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pfsubr:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    movq (%ecx), %mm1
; X32-NEXT:    pfsubr (%eax), %mm0
; X32-NEXT:    pfsubr %mm0, %mm1
; X32-NEXT:    movq %mm1, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pfsubr:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    movq (%rdx), %mm1
; X64-NEXT:    pfsubr (%rsi), %mm0
; X64-NEXT:    pfsubr %mm0, %mm1
; X64-NEXT:    movq %mm1, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pfsubr(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pfsubr(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pfsubr(x86_mmx, x86_mmx)

define void @commute_m_pfmul(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pfmul:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    pfmul (%eax), %mm0
; X32-NEXT:    pfmul (%ecx), %mm0
; X32-NEXT:    movq %mm0, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pfmul:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    pfmul (%rsi), %mm0
; X64-NEXT:    pfmul (%rdx), %mm0
; X64-NEXT:    movq %mm0, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pfmul(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pfmul(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pfmul(x86_mmx, x86_mmx)

define void @commute_m_pfcmpeq(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pfcmpeq:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    pfcmpeq (%eax), %mm0
; X32-NEXT:    pfcmpeq (%ecx), %mm0
; X32-NEXT:    movq %mm0, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pfcmpeq:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    pfcmpeq (%rsi), %mm0
; X64-NEXT:    pfcmpeq (%rdx), %mm0
; X64-NEXT:    movq %mm0, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pfcmpeq(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pfcmpeq(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pfcmpeq(x86_mmx, x86_mmx)

define void @commute_m_pavgusb(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pavgusb:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    pavgusb (%eax), %mm0
; X32-NEXT:    pavgusb (%ecx), %mm0
; X32-NEXT:    movq %mm0, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pavgusb:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    pavgusb (%rsi), %mm0
; X64-NEXT:    pavgusb (%rdx), %mm0
; X64-NEXT:    movq %mm0, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pavgusb(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pavgusb(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pavgusb(x86_mmx, x86_mmx)

define void @commute_m_pmulhrw(x86_mmx *%a0, x86_mmx *%a1, x86_mmx *%a2) nounwind {
; X32-LABEL: commute_m_pmulhrw:
; X32:       # BB#0:
; X32-NEXT:    movl {{[0-9]+}}(%esp), %eax
; X32-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; X32-NEXT:    movl {{[0-9]+}}(%esp), %edx
; X32-NEXT:    movq (%edx), %mm0
; X32-NEXT:    pmulhrw (%eax), %mm0
; X32-NEXT:    pmulhrw (%ecx), %mm0
; X32-NEXT:    movq %mm0, (%ecx)
; X32-NEXT:    retl
;
; X64-LABEL: commute_m_pmulhrw:
; X64:       # BB#0:
; X64-NEXT:    movq (%rdi), %mm0
; X64-NEXT:    pmulhrw (%rsi), %mm0
; X64-NEXT:    pmulhrw (%rdx), %mm0
; X64-NEXT:    movq %mm0, (%rdx)
; X64-NEXT:    retq
  %1 = load x86_mmx, x86_mmx* %a0
  %2 = load x86_mmx, x86_mmx* %a1
  %3 = load x86_mmx, x86_mmx* %a2
  %4 = tail call x86_mmx @llvm.x86.3dnow.pmulhrw(x86_mmx %1, x86_mmx %2)
  %5 = tail call x86_mmx @llvm.x86.3dnow.pmulhrw(x86_mmx %3, x86_mmx %4)
  store x86_mmx %5, x86_mmx* %a2
  ret void
}
declare x86_mmx @llvm.x86.3dnow.pmulhrw(x86_mmx, x86_mmx)
