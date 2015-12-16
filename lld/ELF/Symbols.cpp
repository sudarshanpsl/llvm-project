//===- Symbols.cpp --------------------------------------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Symbols.h"
#include "InputSection.h"
#include "Error.h"
#include "InputFiles.h"

#include "llvm/ADT/STLExtras.h"

using namespace llvm;
using namespace llvm::object;
using namespace llvm::ELF;

using namespace lld;
using namespace lld::elf2;

static uint8_t getMinVisibility(uint8_t VA, uint8_t VB) {
  if (VA == STV_DEFAULT)
    return VB;
  if (VB == STV_DEFAULT)
    return VA;
  return std::min(VA, VB);
}

// Returns 1, 0 or -1 if this symbol should take precedence
// over the Other, tie or lose, respectively.
template <class ELFT> int SymbolBody::compare(SymbolBody *Other) {
  typedef typename ELFFile<ELFT>::uintX_t uintX_t;
  assert(!isLazy() && !Other->isLazy());
  std::pair<bool, bool> L(isDefined(), !isWeak());
  std::pair<bool, bool> R(Other->isDefined(), !Other->isWeak());

  // Normalize
  if (L > R)
    return -Other->compare<ELFT>(this);

  Visibility = Other->Visibility =
      getMinVisibility(Visibility, Other->Visibility);

  if (IsUsedInRegularObj || Other->IsUsedInRegularObj)
    IsUsedInRegularObj = Other->IsUsedInRegularObj = true;

  if (L != R)
    return -1;
  if (!L.first || !L.second)
    return 1;
  if (isShared())
    return -1;
  if (Other->isShared())
    return 1;
  if (isCommon()) {
    if (!Other->isCommon())
      return -1;
    auto *ThisC = cast<DefinedCommon<ELFT>>(this);
    auto *OtherC = cast<DefinedCommon<ELFT>>(Other);
    uintX_t Align = std::max(ThisC->MaxAlignment, OtherC->MaxAlignment);
    if (ThisC->Sym.st_size >= OtherC->Sym.st_size) {
      ThisC->MaxAlignment = Align;
      return 1;
    }
    OtherC->MaxAlignment = Align;
    return -1;
  }
  if (Other->isCommon())
    return 1;
  return 0;
}

std::unique_ptr<InputFile> Lazy::getMember() {
  MemoryBufferRef MBRef = File->getMember(&Sym);

  // getMember returns an empty buffer if the member was already
  // read from the library.
  if (MBRef.getBuffer().empty())
    return std::unique_ptr<InputFile>(nullptr);

  return createELFFile<ObjectFile>(MBRef);
}

template <class ELFT> static void doInitSymbols() {
  DefinedAbsolute<ELFT>::End.setBinding(STB_GLOBAL);
  DefinedAbsolute<ELFT>::IgnoreUndef.setBinding(STB_WEAK);
  DefinedAbsolute<ELFT>::IgnoreUndef.setVisibility(STV_HIDDEN);
  Undefined<ELFT>::Optional.setVisibility(STV_HIDDEN);
}

void lld::elf2::initSymbols() {
  doInitSymbols<ELF32LE>();
  doInitSymbols<ELF32BE>();
  doInitSymbols<ELF64LE>();
  doInitSymbols<ELF64BE>();
}

template int SymbolBody::compare<ELF32LE>(SymbolBody *Other);
template int SymbolBody::compare<ELF32BE>(SymbolBody *Other);
template int SymbolBody::compare<ELF64LE>(SymbolBody *Other);
template int SymbolBody::compare<ELF64BE>(SymbolBody *Other);
