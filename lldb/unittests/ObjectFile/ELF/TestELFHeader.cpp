//===-- TestELFHeader.cpp ---------------------------------------*- C++ -*-===//
//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Plugins/ObjectFile/ELF/ELFHeader.h"
#include "lldb/Core/DataExtractor.h"
#include "gtest/gtest.h"

using namespace lldb;
using namespace lldb_private;


TEST(ELFHeader, ParseHeaderExtension) {
  uint8_t data[] = {
      // e_ident
      0x7f, 0x45, 0x4c, 0x46, 0x02, 0x01, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      // e_type, e_machine, e_version, e_entry
      0x03, 0x00, 0x3e, 0x00, 0x01, 0x00, 0x00, 0x00, 0x90, 0x48, 0x40, 0x00,
      0x00, 0x00, 0x00, 0x00,

      // e_phoff, e_shoff
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      // e_flags, e_ehsize, e_phentsize, e_phnum, e_shentsize, e_shnum,
      // e_shstrndx
      0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x38, 0x00, 0xff, 0xff, 0x40, 0x00,
      0x00, 0x00, 0xff, 0xff,

      // sh_name, sh_type, sh_flags
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      // sh_addr, sh_offset
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,

      // sh_size, sh_link, sh_info
      0x23, 0x45, 0x67, 0x00, 0x00, 0x00, 0x00, 0x00, 0x34, 0x56, 0x78, 0x00,
      0x12, 0x34, 0x56, 0x00,

      // sh_addralign, sh_entsize
      0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
      0x00, 0x00, 0x00, 0x00,
  };

  DataExtractor extractor(data, sizeof data, eByteOrderLittle, 8);
  elf::ELFHeader header;
  offset_t offset = 0;
  ASSERT_TRUE(header.Parse(extractor, &offset));
  EXPECT_EQ(0x563412u, header.e_phnum);
  EXPECT_EQ(0x785634u, header.e_shstrndx);
  EXPECT_EQ(0x674523u, header.e_shnum);
}
