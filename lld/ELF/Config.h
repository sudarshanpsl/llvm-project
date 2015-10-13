//===- Config.h -------------------------------------------------*- C++ -*-===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef LLD_ELF_CONFIG_H
#define LLD_ELF_CONFIG_H

#include "llvm/ADT/StringRef.h"
#include "llvm/Support/ELF.h"

#include <vector>

namespace lld {
namespace elf2 {

class InputFile;
class SymbolBody;

enum ELFKind {
  ELFNoneKind,
  ELF32LEKind,
  ELF32BEKind,
  ELF64LEKind,
  ELF64BEKind
};

struct Configuration {
  SymbolBody *EntrySym = nullptr;
  InputFile *FirstElf = nullptr;
  llvm::StringRef DynamicLinker;
  llvm::StringRef Entry;
  llvm::StringRef Emulation;
  llvm::StringRef Fini;
  llvm::StringRef Init;
  llvm::StringRef OutputFile;
  llvm::StringRef SoName;
  llvm::StringRef Sysroot;
  std::string RPath;
  std::vector<llvm::StringRef> SearchPaths;
  bool AllowMultipleDefinition;
  bool AsNeeded = false;
  bool DiscardAll;
  bool DiscardLocals;
  bool DiscardNone;
  bool EnableNewDtags;
  bool ExportDynamic;
  bool NoInhibitExec;
  bool NoUndefined;
  bool Shared;
  bool Static = false;
  bool Verbose;
  bool ZNow = false;
  ELFKind EKind = ELFNoneKind;
  uint16_t EMachine = llvm::ELF::EM_NONE;
};

extern Configuration *Config;

} // namespace elf2
} // namespace lld

#endif
