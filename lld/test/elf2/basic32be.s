# RUN: llvm-mc -filetype=obj -triple=powerpc-unknown-linux %s -o %t
# RUN: ld.lld2 %t -o %t2
# RUN: llvm-readobj -file-headers -sections -program-headers %t2 | FileCheck %s
# REQUIRES: ppc

# exits with return code 42 on linux
.globl _start;
_start:
 li 0,1
 li 3,42
 sc

# CHECK:      ElfHeader {
# CHECK-NEXT:   Ident {
# CHECK-NEXT:     Magic: (7F 45 4C 46)
# CHECK-NEXT:     Class: 32-bit (0x1)
# CHECK-NEXT:     DataEncoding: BigEndian (0x2)
# CHECK-NEXT:     FileVersion: 1
# CHECK-NEXT:     OS/ABI: SystemV (0x0)
# CHECK-NEXT:     ABIVersion: 0
# CHECK-NEXT:     Unused: (00 00 00 00 00 00 00)
# CHECK-NEXT:   }
# CHECK-NEXT:   Type: Executable (0x2)
# CHECK-NEXT:   Machine: EM_PPC (0x14)
# CHECK-NEXT:   Version: 1
# CHECK-NEXT:   Entry: 0x10001000
# CHECK-NEXT:   ProgramHeaderOffset: 0x34
# CHECK-NEXT:   SectionHeaderOffset: 0x1058
# CHECK-NEXT:   Flags [ (0x0)
# CHECK-NEXT:   ]
# CHECK-NEXT:   HeaderSize: 52
# CHECK-NEXT:   ProgramHeaderEntrySize: 32
# CHECK-NEXT:   ProgramHeaderCount: 2
# CHECK-NEXT:   SectionHeaderEntrySize: 40
# CHECK-NEXT:   SectionHeaderCount: 6
# CHECK-NEXT:    StringTableSectionIndex: 5
# CHECK-NEXT: }
# CHECK-NEXT: Sections [
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 0
# CHECK-NEXT:     Name:  (0)
# CHECK-NEXT:     Type: SHT_NULL (0x0)
# CHECK-NEXT:     Flags [ (0x0)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x0
# CHECK-NEXT:     Offset: 0x0
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Link: 0
# CHECK-NEXT:     Info: 0
# CHECK-NEXT:     AddressAlignment: 0
# CHECK-NEXT:     EntrySize: 0
# CHECK-NEXT:   }
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 1
# CHECK-NEXT:     Name: .text
# CHECK-NEXT:     Type: SHT_PROGBITS (0x1)
# CHECK-NEXT:     Flags [ (0x6)
# CHECK-NEXT:       SHF_ALLOC (0x2)
# CHECK-NEXT:       SHF_EXECINSTR (0x4)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x10001000
# CHECK-NEXT:     Offset: 0x1000
# CHECK-NEXT:     Size: 12
# CHECK-NEXT:     Link: 0
# CHECK-NEXT:     Info: 0
# CHECK-NEXT:     AddressAlignment: 4
# CHECK-NEXT:     EntrySize: 0
# CHECK-NEXT:   }
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 2
# CHECK-NEXT:     Name: .data
# CHECK-NEXT:     Type: SHT_PROGBITS (0x1)
# CHECK-NEXT:     Flags [ (0x3)
# CHECK-NEXT:       SHF_ALLOC (0x2)
# CHECK-NEXT:       SHF_WRITE (0x1)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x1000100C
# CHECK-NEXT:     Offset: 0x100C
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Link: 0
# CHECK-NEXT:     Info: 0
# CHECK-NEXT:     AddressAlignment: 4
# CHECK-NEXT:     EntrySize: 0
# CHECK-NEXT:   }
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 3
# CHECK-NEXT:     Name: .bss
# CHECK-NEXT:     Type: SHT_NOBITS (0x8)
# CHECK-NEXT:     Flags [ (0x3)
# CHECK-NEXT:       SHF_ALLOC (0x2)
# CHECK-NEXT:       SHF_WRITE (0x1)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x1000100C
# CHECK-NEXT:     Offset: 0x100C
# CHECK-NEXT:     Size: 0
# CHECK-NEXT:     Link: 0
# CHECK-NEXT:     Info: 0
# CHECK-NEXT:     AddressAlignment: 4
# CHECK-NEXT:     EntrySize: 0
# CHECK-NEXT:   }
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 4
# CHECK-NEXT:     Name: .symtab
# CHECK-NEXT:     Type: SHT_SYMTAB
# CHECK-NEXT:     Flags [
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x0
# CHECK-NEXT:     Offset: 0x100C
# CHECK-NEXT:     Size: 32
# CHECK-NEXT:     Link: 5
# CHECK-NEXT:     Info: 1
# CHECK-NEXT:     AddressAlignment: 4
# CHECK-NEXT:     EntrySize: 16
# CHECK-NEXT:   }
# CHECK-NEXT:   Section {
# CHECK-NEXT:     Index: 5
# CHECK-NEXT:     Name: .strtab
# CHECK-NEXT:     Type: SHT_STRTAB (0x3)
# CHECK-NEXT:     Flags [ (0x0)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Address: 0x0
# CHECK-NEXT:     Offset: 0x102C
# CHECK-NEXT:     Size: 41
# CHECK-NEXT:     Link: 0
# CHECK-NEXT:     Info: 0
# CHECK-NEXT:     AddressAlignment: 1
# CHECK-NEXT:     EntrySize: 0
# CHECK-NEXT:   }
# CHECK-NEXT: ]
# CHECK-NEXT: ProgramHeaders [
# CHECK-NEXT:   ProgramHeader {
# CHECK-NEXT:     Type: PT_LOAD (0x1)
# CHECK-NEXT:     Offset: 0x0
# CHECK-NEXT:     VirtualAddress: 0x10000000
# CHECK-NEXT:     PhysicalAddress: 0x10000000
# CHECK-NEXT:     FileSize: 116
# CHECK-NEXT:     MemSize: 116
# CHECK-NEXT:     Flags [
# CHECK-NEXT:       PF_R
# CHECK-NEXT:     ]
# CHECK-NEXT:     Alignment: 4096
# CHECK-NEXT:   }
# CHECK-NEXT:   ProgramHeader {
# CHECK-NEXT:     Type: PT_LOAD (0x1)
# CHECK-NEXT:     Offset: 0x1000
# CHECK-NEXT:     VirtualAddress: 0x10001000
# CHECK-NEXT:     PhysicalAddress: 0x10001000
# CHECK-NEXT:     FileSize: 12
# CHECK-NEXT:     MemSize: 12
# CHECK-NEXT:     Flags [ (0x5)
# CHECK-NEXT:       PF_R (0x4)
# CHECK-NEXT:       PF_X (0x1)
# CHECK-NEXT:     ]
# CHECK-NEXT:     Alignment: 4096
# CHECK-NEXT:   }
# CHECK-NEXT: ]
