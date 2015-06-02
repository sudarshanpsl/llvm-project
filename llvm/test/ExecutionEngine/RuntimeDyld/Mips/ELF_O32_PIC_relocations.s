# RUN: llvm-mc -triple=mipsel-unknown-linux -relocation-model=pic -code-model=small -filetype=obj -o %T/test_ELF_O32.o %s
# RUN: llc -mtriple=mipsel-unknown-linux -relocation-model=pic -filetype=obj -o %T/test_ELF_ExternalFunction_O32.o %S/Inputs/ExternalFunction.ll
# RUN: llvm-rtdyld -triple=mipsel-unknown-linux -verify -map-section test_ELF_O32.o,.text=0x1000 -map-section test_ELF_ExternalFunction_O32.o,.text=0x10000 -check=%s %T/test_ELF_O32.o %T/test_ELF_ExternalFunction_O32.o
# There are endian awareness bugs in the O32 support.
# XFAIL: mips-

        .data
# rtdyld-check: *{4}R_MIPS_32 = foo
R_MIPS_32:
        .word foo
# rtdyld-check: *{4}(R_MIPS_32+4) = foo
        .4byte foo
# rtdyld-check: *{4}(R_MIPS_PC32) = foo - R_MIPS_PC32
R_MIPS_PC32:
        .word foo-.
# rtdyld-check: *{4}(R_MIPS_PC32 + 4) = foo - tmp1
tmp1:
        .4byte foo-tmp1

	.text
	.abicalls
	.nan	legacy
	.text
	.set	nomicromips
	.set	nomips16
	.set	noreorder
	.set	nomacro
	.set	noat

	.align	3
	.globl	bar
	.type	bar,@function
bar:
# rtdyld-check:  decode_operand(R_MIPS_26, 0)[27:0] = stub_addr(test_ELF_O32.o, .text, foo)[27:0]
# rtdyld-check:  decode_operand(R_MIPS_26, 0)[1:0] = 0
R_MIPS_26:
	j   foo
	nop

# rtdyld-check:  decode_operand(R_MIPS_HI16, 1)[15:0] = foo[31:16]
R_MIPS_HI16:
	lui	$1, %hi(foo)

# rtdyld-check:  decode_operand(R_MIPS_LO16, 1)[15:0] = foo[15:0]
R_MIPS_LO16:
	lui	$1, %lo(foo)

	.size	bar, .-bar
