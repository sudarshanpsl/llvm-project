@ RUN: llvm-mc -triple=thumbv6-apple-darwin -show-encoding < %s | FileCheck %s
  .syntax unified
  .globl _func

@ Check that the assembler can handle the documented syntax from the ARM ARM.
@ For complex constructs like shifter operands, check more thoroughly for them
@ once then spot check that following instructions accept the form generally.
@ This gives us good coverage while keeping the overall size of the test
@ more reasonable.

_func:
@ CHECK: _func

@------------------------------------------------------------------------------
@ ADC (register)
@------------------------------------------------------------------------------
        adcs r4, r6

@ CHECK: adcs	r4, r6                  @ encoding: [0x74,0x41]


@------------------------------------------------------------------------------
@ ADD (immediate)
@------------------------------------------------------------------------------
        adds r1, r2, #3
        adds r2, #3
        adds r2, #8

@ CHECK: adds	r1, r2, #3              @ encoding: [0xd1,0x1c]
@ CHECK: adds	r2, r2, #3              @ encoding: [0xd2,0x1c]
@ CHECK: adds	r2, #8                  @ encoding: [0x08,0x32]


@------------------------------------------------------------------------------
@ ADD (register)
@------------------------------------------------------------------------------
        adds r1, r2, r3
        add r2, r8

@ CHECK: adds	r1, r2, r3              @ encoding: [0xd1,0x18]
@ CHECK: add	r2, r8                  @ encoding: [0x42,0x44]


@------------------------------------------------------------------------------
@ FIXME: ADD (SP plus immediate)
@------------------------------------------------------------------------------
@------------------------------------------------------------------------------
@ FIXME: ADD (SP plus register)
@------------------------------------------------------------------------------


@------------------------------------------------------------------------------
@ ADR
@------------------------------------------------------------------------------
        adr r2, _baz

@ CHECK: adr	r2, _baz                @ encoding: [A,0xa2]
            @   fixup A - offset: 0, value: _baz, kind: fixup_thumb_adr_pcrel_10


@------------------------------------------------------------------------------
@ ASR (immediate)
@------------------------------------------------------------------------------
        asrs r2, r3, #32
        asrs r2, r3, #5
        asrs r2, r3, #1

@ CHECK: asrs	r2, r3, #32             @ encoding: [0x1a,0x10]
@ CHECK: asrs	r2, r3, #5              @ encoding: [0x5a,0x11]
@ CHECK: asrs	r2, r3, #1              @ encoding: [0x5a,0x10]


@------------------------------------------------------------------------------
@ ASR (register)
@------------------------------------------------------------------------------
        asrs r5, r2

@ CHECK: asrs	r5, r2                  @ encoding: [0x15,0x41]


@------------------------------------------------------------------------------
@ B
@------------------------------------------------------------------------------
        b _baz
        beq _bar

@ CHECK: b	_baz                    @ encoding: [A,0xe0'A']
             @   fixup A - offset: 0, value: _baz, kind: fixup_arm_thumb_br
@ CHECK: beq	_bar                    @ encoding: [A,0xd0]
             @   fixup A - offset: 0, value: _bar, kind: fixup_arm_thumb_bcc


@------------------------------------------------------------------------------
@ BICS
@------------------------------------------------------------------------------
        bics r1, r6

@ CHECK: bics	r1, r6                  @ encoding: [0xb1,0x43]


@------------------------------------------------------------------------------
@ BKPT
@------------------------------------------------------------------------------
        bkpt #0
        bkpt #255

@ CHECK: bkpt	#0                      @ encoding: [0x00,0xbe]
@ CHECK: bkpt	#255                    @ encoding: [0xff,0xbe]


@------------------------------------------------------------------------------
@ BL/BLX (immediate)
@------------------------------------------------------------------------------
        bl _bar
        blx _baz

@ CHECK: bl	_bar                    @ encoding: [A,0xf0'A',A,0xf8'A']
             @   fixup A - offset: 0, value: _bar, kind: fixup_arm_thumb_bl
@ CHECK: blx	_baz                    @ encoding: [A,0xf0'A',A,0xe8'A']
             @   fixup A - offset: 0, value: _baz, kind: fixup_arm_thumb_blx


@------------------------------------------------------------------------------
@ BLX (register)
@------------------------------------------------------------------------------
        blx r4

@ CHECK: blx	r4                      @ encoding: [0xa0,0x47]


@------------------------------------------------------------------------------
@ BX
@------------------------------------------------------------------------------
        bx r2

@ CHECK: bx	r2                      @ encoding: [0x10,0x47]


@------------------------------------------------------------------------------
@ CMN
@------------------------------------------------------------------------------

        cmn r5, r1

@ CHECK: cmn	r5, r1                  @ encoding: [0xcd,0x42]


@------------------------------------------------------------------------------
@ CMP
@------------------------------------------------------------------------------
        cmp r6, #32
        cmp r3, r4
        cmp r8, r1

@ CHECK: cmp	r6, #32                 @ encoding: [0x20,0x2e]
@ CHECK: cmp	r3, r4                  @ encoding: [0xa3,0x42]
@ CHECK: cmp	r8, r1                  @ encoding: [0x88,0x45]

@------------------------------------------------------------------------------
@ EOR
@------------------------------------------------------------------------------
        eors r4, r5

@ CHECK: eors	r4, r5                  @ encoding: [0x6c,0x40]


@------------------------------------------------------------------------------
@ LDM
@------------------------------------------------------------------------------
        ldm r3, {r0, r1, r2, r3, r4, r5, r6, r7}
        ldm r2!, {r1, r3, r4, r5, r7}
        ldm r1, {r1}

@ CHECK: ldm	r3, {r0, r1, r2, r3, r4, r5, r6, r7} @ encoding: [0xff,0xcb]
@ CHECK: ldm	r2!, {r1, r3, r4, r5, r7} @ encoding: [0xba,0xca]
@ CHECK: ldm	r1, {r1}                @ encoding: [0x02,0xc9]


@------------------------------------------------------------------------------
@ LDR (immediate)
@------------------------------------------------------------------------------
        ldr r1, [r5]
        ldr r2, [r6, #32]
        ldr r3, [r7, #124]
        ldr r1, [sp]
        ldr r2, [sp, #24]
        ldr r3, [sp, #1020]


@ CHECK: ldr	r1, [r5]                @ encoding: [0x29,0x68]
@ CHECK: ldr	r2, [r6, #32]           @ encoding: [0x32,0x6a]
@ CHECK: ldr	r3, [r7, #124]          @ encoding: [0xfb,0x6f]
@ CHECK: ldr	r1, [sp]                @ encoding: [0x00,0x99]
@ CHECK: ldr	r2, [sp, #24]           @ encoding: [0x06,0x9a]
@ CHECK: ldr	r3, [sp, #1020]         @ encoding: [0xff,0x9b]


@------------------------------------------------------------------------------
@ LDR (literal)
@------------------------------------------------------------------------------
        ldr r1, _foo

@ CHECK: ldr	r1, _foo                @ encoding: [A,0x49]
             @   fixup A - offset: 0, value: _foo, kind: fixup_arm_thumb_cp
