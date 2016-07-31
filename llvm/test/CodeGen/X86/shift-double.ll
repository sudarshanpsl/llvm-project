; NOTE: Assertions have been autogenerated by utils/update_llc_test_checks.py
; RUN: llc < %s -mtriple=i686-unknown | FileCheck %s

; Shift i64 integers on 32-bit target

define i64 @test1(i64 %X, i8 %C) nounwind {
; CHECK-LABEL: test1:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl %esi, %eax
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    shldl %cl, %esi, %edx
; CHECK-NEXT:    testb $32, %cl
; CHECK-NEXT:    je .LBB0_2
; CHECK-NEXT:  # BB#1:
; CHECK-NEXT:    movl %eax, %edx
; CHECK-NEXT:    xorl %eax, %eax
; CHECK-NEXT:  .LBB0_2:
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
        %shift.upgrd.1 = zext i8 %C to i64              ; <i64> [#uses=1]
        %Y = shl i64 %X, %shift.upgrd.1         ; <i64> [#uses=1]
        ret i64 %Y
}

define i64 @test2(i64 %X, i8 %C) nounwind {
; CHECK-LABEL: test2:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl %esi, %edx
; CHECK-NEXT:    sarl %cl, %edx
; CHECK-NEXT:    shrdl %cl, %esi, %eax
; CHECK-NEXT:    testb $32, %cl
; CHECK-NEXT:    je .LBB1_2
; CHECK-NEXT:  # BB#1:
; CHECK-NEXT:    sarl $31, %esi
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    movl %esi, %edx
; CHECK-NEXT:  .LBB1_2:
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
        %shift.upgrd.2 = zext i8 %C to i64              ; <i64> [#uses=1]
        %Y = ashr i64 %X, %shift.upgrd.2                ; <i64> [#uses=1]
        ret i64 %Y
}

define i64 @test3(i64 %X, i8 %C) nounwind {
; CHECK-LABEL: test3:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl %esi, %edx
; CHECK-NEXT:    shrl %cl, %edx
; CHECK-NEXT:    shrdl %cl, %esi, %eax
; CHECK-NEXT:    testb $32, %cl
; CHECK-NEXT:    je .LBB2_2
; CHECK-NEXT:  # BB#1:
; CHECK-NEXT:    movl %edx, %eax
; CHECK-NEXT:    xorl %edx, %edx
; CHECK-NEXT:  .LBB2_2:
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
        %shift.upgrd.3 = zext i8 %C to i64              ; <i64> [#uses=1]
        %Y = lshr i64 %X, %shift.upgrd.3                ; <i64> [#uses=1]
        ret i64 %Y
}

; Combine 2xi32/2xi16 shifts into SHLD

define i32 @test4(i32 %A, i32 %B, i8 %C) nounwind {
; CHECK-LABEL: test4:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shldl %cl, %edx, %eax
; CHECK-NEXT:    retl
        %shift.upgrd.4 = zext i8 %C to i32              ; <i32> [#uses=1]
        %X = shl i32 %A, %shift.upgrd.4         ; <i32> [#uses=1]
        %Cv = sub i8 32, %C             ; <i8> [#uses=1]
        %shift.upgrd.5 = zext i8 %Cv to i32             ; <i32> [#uses=1]
        %Y = lshr i32 %B, %shift.upgrd.5                ; <i32> [#uses=1]
        %Z = or i32 %Y, %X              ; <i32> [#uses=1]
        ret i32 %Z
}

define i16 @test5(i16 %A, i16 %B, i8 %C) nounwind {
; CHECK-LABEL: test5:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movzwl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shldw %cl, %dx, %ax
; CHECK-NEXT:    retl
        %shift.upgrd.6 = zext i8 %C to i16              ; <i16> [#uses=1]
        %X = shl i16 %A, %shift.upgrd.6         ; <i16> [#uses=1]
        %Cv = sub i8 16, %C             ; <i8> [#uses=1]
        %shift.upgrd.7 = zext i8 %Cv to i16             ; <i16> [#uses=1]
        %Y = lshr i16 %B, %shift.upgrd.7                ; <i16> [#uses=1]
        %Z = or i16 %Y, %X              ; <i16> [#uses=1]
        ret i16 %Z
}

; Combine 2xi32/2xi16 shifts into SHRD

define i32 @test6(i32 %A, i32 %B, i8 %C) nounwind {
; CHECK-LABEL: test6:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shrdl %cl, %edx, %eax
; CHECK-NEXT:    retl
        %shift.upgrd.4 = zext i8 %C to i32              ; <i32> [#uses=1]
        %X = lshr i32 %A, %shift.upgrd.4         ; <i32> [#uses=1]
        %Cv = sub i8 32, %C             ; <i8> [#uses=1]
        %shift.upgrd.5 = zext i8 %Cv to i32             ; <i32> [#uses=1]
        %Y = shl i32 %B, %shift.upgrd.5                ; <i32> [#uses=1]
        %Z = or i32 %Y, %X              ; <i32> [#uses=1]
        ret i32 %Z
}

define i16 @test7(i16 %A, i16 %B, i8 %C) nounwind {
; CHECK-LABEL: test7:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movzwl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movzwl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shrdw %cl, %dx, %ax
; CHECK-NEXT:    retl
        %shift.upgrd.6 = zext i8 %C to i16              ; <i16> [#uses=1]
        %X = lshr i16 %A, %shift.upgrd.6         ; <i16> [#uses=1]
        %Cv = sub i8 16, %C             ; <i8> [#uses=1]
        %shift.upgrd.7 = zext i8 %Cv to i16             ; <i16> [#uses=1]
        %Y = shl i16 %B, %shift.upgrd.7                ; <i16> [#uses=1]
        %Z = or i16 %Y, %X              ; <i16> [#uses=1]
        ret i16 %Z
}

; Shift i64 integers on 32-bit target by shift value less then 32 (PR14593)

define i64 @test8(i64 %val, i32 %bits) nounwind {
; CHECK-LABEL: test8:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl %esi, %eax
; CHECK-NEXT:    shll %cl, %eax
; CHECK-NEXT:    andb $31, %cl
; CHECK-NEXT:    shldl %cl, %esi, %edx
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %bits, 31
  %sh_prom = zext i32 %and to i64
  %shl = shl i64 %val, %sh_prom
  ret i64 %shl
}

define i64 @test9(i64 %val, i32 %bits) nounwind {
; CHECK-LABEL: test9:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl %esi, %edx
; CHECK-NEXT:    sarl %cl, %edx
; CHECK-NEXT:    andb $31, %cl
; CHECK-NEXT:    shrdl %cl, %esi, %eax
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %bits, 31
  %sh_prom = zext i32 %and to i64
  %ashr = ashr i64 %val, %sh_prom
  ret i64 %ashr
}

define i64 @test10(i64 %val, i32 %bits) nounwind {
; CHECK-LABEL: test10:
; CHECK:       # BB#0:
; CHECK-NEXT:    pushl %esi
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %esi
; CHECK-NEXT:    movl %esi, %edx
; CHECK-NEXT:    shrl %cl, %edx
; CHECK-NEXT:    andb $31, %cl
; CHECK-NEXT:    shrdl %cl, %esi, %eax
; CHECK-NEXT:    popl %esi
; CHECK-NEXT:    retl
  %and = and i32 %bits, 31
  %sh_prom = zext i32 %and to i64
  %lshr = lshr i64 %val, %sh_prom
  ret i64 %lshr
}

; SHLD/SHRD manual shifts

define i32 @test11(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test11:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    andl $31, %ecx
; CHECK-NEXT:    # kill: %CL<def> %CL<kill> %ECX<kill>
; CHECK-NEXT:    shldl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %and = and i32 %bits, 31
  %and32 = sub i32 32, %and
  %sh_lo = lshr i32 %lo, %and32
  %sh_hi = shl i32 %hi, %and
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test12(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test12:
; CHECK:       # BB#0:
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %ecx
; CHECK-NEXT:    andl $31, %ecx
; CHECK-NEXT:    # kill: %CL<def> %CL<kill> %ECX<kill>
; CHECK-NEXT:    shrdl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %and = and i32 %bits, 31
  %and32 = sub i32 32, %and
  %sh_lo = shl i32 %hi, %and32
  %sh_hi = lshr i32 %lo, %and
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test13(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test13:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shldl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %bits32 = sub i32 32, %bits
  %sh_lo = lshr i32 %lo, %bits32
  %sh_hi = shl i32 %hi, %bits
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test14(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test14:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shrdl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %bits32 = sub i32 32, %bits
  %sh_lo = shl i32 %hi, %bits32
  %sh_hi = lshr i32 %lo, %bits
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test15(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test15:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shldl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %bits32 = xor i32 %bits, 31
  %lo2 = lshr i32 %lo, 1
  %sh_lo = lshr i32 %lo2, %bits32
  %sh_hi = shl i32 %hi, %bits
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test16(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test16:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shrdl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %bits32 = xor i32 %bits, 31
  %lo2 = shl i32 %lo, 1
  %sh_lo = shl i32 %lo2, %bits32
  %sh_hi = lshr i32 %hi, %bits
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}

define i32 @test17(i32 %hi, i32 %lo, i32 %bits) nounwind {
; CHECK-LABEL: test17:
; CHECK:       # BB#0:
; CHECK-NEXT:    movb {{[0-9]+}}(%esp), %cl
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %edx
; CHECK-NEXT:    movl {{[0-9]+}}(%esp), %eax
; CHECK-NEXT:    shrdl %cl, %edx, %eax
; CHECK-NEXT:    retl
  %bits32 = xor i32 %bits, 31
  %lo2 = add i32 %lo, %lo
  %sh_lo = shl i32 %lo2, %bits32
  %sh_hi = lshr i32 %hi, %bits
  %sh = or i32 %sh_lo, %sh_hi
  ret i32 %sh
}
