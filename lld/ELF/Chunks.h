//===- Chunks.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_CHUNKS_H
#define LLD_ELF_CHUNKS_H

#include "lld/Core/LLVM.h"
#include "llvm/Object/ELF.h"

namespace lld {
namespace elf2 {

template <class ELFT> class ObjectFile;
template <class ELFT> class OutputSection;

// A chunk corresponding a section of an input file.
template <class ELFT> class SectionChunk {
  typedef llvm::object::Elf_Shdr_Impl<ELFT> Elf_Shdr;
  typedef llvm::object::Elf_Rel_Impl<ELFT, true> Elf_Rela;
  typedef llvm::object::Elf_Rel_Impl<ELFT, false> Elf_Rel;
  typedef typename llvm::object::ELFFile<ELFT>::uintX_t uintX_t;

public:
  SectionChunk(llvm::object::ELFFile<ELFT> *Obj, const Elf_Shdr *Header);

  // Returns the size of this chunk (even if this is a common or BSS.)
  size_t getSize() const { return Header->sh_size; }

  // Write this chunk to a mmap'ed file, assuming Buf is pointing to
  // beginning of the output section.
  void writeTo(uint8_t *Buf);

  StringRef getSectionName() const;
  const Elf_Shdr *getSectionHdr() const { return Header; }

  // The writer sets and uses the addresses.
  uintX_t getOutputSectionOff() const { return OutputSectionOff; }
  uintX_t getAlign() { return Header->sh_addralign; }
  void setOutputSectionOff(uint64_t V) { OutputSectionOff = V; }

  void setOutputSection(OutputSection<ELFT> *O) { Out = O; }
  OutputSection<ELFT> *getOutputSection() const { return Out; }

private:
  // The offset from beginning of the output sections this chunk was assigned
  // to. The writer sets a value.
  uint64_t OutputSectionOff = 0;

  // A file this chunk was created from.
  llvm::object::ELFFile<ELFT> *Obj;

  OutputSection<ELFT> *Out = nullptr;

  const Elf_Shdr *Header;
};

} // namespace elf2
} // namespace lld

#endif
