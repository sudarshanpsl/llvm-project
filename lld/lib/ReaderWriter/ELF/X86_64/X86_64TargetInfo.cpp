//===- lib/ReaderWriter/ELF/X86_64/X86_64TargetInfo.cpp -------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "X86_64TargetInfo.h"

#include "lld/Core/File.h"
#include "lld/Core/Pass.h"
#include "lld/Core/PassManager.h"
#include "lld/ReaderWriter/Simple.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/ADT/StringSwitch.h"

using namespace lld;

namespace {
using namespace llvm::ELF;

class GOTAtom : public SimpleDefinedAtom {
  static const uint8_t _defaultContent[8];
  StringRef _section;

public:
  GOTAtom(const File &f, StringRef secName)
      : SimpleDefinedAtom(f), _section(secName) {
  }

  virtual Scope scope() const { return scopeTranslationUnit; }

  virtual SectionChoice sectionChoice() const { return sectionCustomRequired; }

  virtual StringRef customSectionName() const { return _section; }

  virtual ContentType contentType() const { return typeGOT; }

  virtual uint64_t size() const { return rawContent().size(); }

  virtual ContentPermissions permissions() const { return permRW_; }

  virtual ArrayRef<uint8_t> rawContent() const {
    return ArrayRef<uint8_t>(_defaultContent, 8);
  }

  virtual Alignment alignment() const {
    // The alignment should be 8 byte aligned
    return Alignment(3);
  }

#ifndef NDEBUG
  virtual StringRef name() const { return _name; }

  std::string _name;
#else
  virtual StringRef name() const { return ""; }
#endif
};

const uint8_t GOTAtom::_defaultContent[8] = { 0 };

class PLTAtom : public SimpleDefinedAtom {
  static const uint8_t _defaultContent[16];
  StringRef _section;

public:
  PLTAtom(const File &f, StringRef secName)
      : SimpleDefinedAtom(f), _section(secName) {
  }

  virtual Scope scope() const { return scopeTranslationUnit; }

  virtual SectionChoice sectionChoice() const { return sectionCustomRequired; }

  virtual StringRef customSectionName() const { return _section; }

  virtual ContentType contentType() const { return typeStub; }

  virtual uint64_t size() const { return rawContent().size(); }

  virtual ContentPermissions permissions() const { return permR_X; }

  virtual ArrayRef<uint8_t> rawContent() const {
    return ArrayRef<uint8_t>(_defaultContent, 16);
  }

  virtual Alignment alignment() const {
    // The alignment should be 4 byte aligned
    return Alignment(2);
  }

#ifndef NDEBUG
  virtual StringRef name() const { return _name; }

  std::string _name;
#else
  virtual StringRef name() const { return ""; }
#endif
};

const uint8_t PLTAtom::_defaultContent[16] = {
  0xff, 0x25, 0x00, 0x00, 0x00, 0x00, // jmpq *gotatom(%rip)
  0x68, 0x00, 0x00, 0x00, 0x00,       // pushq pltentry
  0xe9, 0x00, 0x00, 0x00, 0x00        // jmpq plt[-1]
};

class ELFPassFile : public SimpleFile {
public:
  ELFPassFile(const ELFTargetInfo &eti) : SimpleFile(eti, "ELFPassFile") {}

  llvm::BumpPtrAllocator _alloc;
};

/// \brief Create GOT and PLT entries for relocations. Handles standard GOT/PLT
/// along with IFUNC and TLS.
template <class Derived> class GOTPLTPass : public Pass {
  /// \brief Handle a specific reference.
  void handleReference(const DefinedAtom &atom, const Reference &ref) {
    const DefinedAtom *da = dyn_cast_or_null<const DefinedAtom>(ref.target());
    switch (ref.kind()) {
    case R_X86_64_PLT32:
      static_cast<Derived *>(this)->handlePLT32(ref);
      break;
    case R_X86_64_PC32:
      static_cast<Derived *>(this)->handleIFUNC(ref, da);
      break;
    case R_X86_64_GOTTPOFF: // GOT Thread Pointer Offset
      static_cast<Derived *>(this)->handleGOTTPOFF(ref, da);
      break;
    case R_X86_64_GOTPCREL:
      static_cast<Derived *>(this)->handleGOTPCREL(ref);
      break;
    }
  }

protected:
  /// \brief get the PLT entry for a given IFUNC Atom.
  ///
  /// If the entry does not exist. Both the GOT and PLT entry is created.
  const PLTAtom *getIFUNCPLTEntry(const DefinedAtom *da) {
    auto plt = _pltMap.find(da);
    if (plt != _pltMap.end())
      return plt->second;
    auto ga = new (_file._alloc) GOTAtom(_file, ".got.plt");
    ga->addReference(R_X86_64_IRELATIVE, 0, da, 0);
    auto pa = new (_file._alloc) PLTAtom(_file, ".plt");
    pa->addReference(R_X86_64_PC32, 2, ga, -4);
#ifndef NDEBUG
    ga->_name = "__got_ifunc_";
    ga->_name += da->name();
    pa->_name = "__plt_ifunc_";
    pa->_name += da->name();
#endif
    _gotMap[da] = ga;
    _pltMap[da] = pa;
    return pa;
  }

  /// \brief Redirect the call to the PLT stub for the target IFUNC.
  ///
  /// This create a PLT and GOT entry for the IFUNC if one does not exist. The
  /// GOT entry and a IRELATIVE relocation to the original target resolver.
  ErrorOr<void> handleIFUNC(const Reference &ref, const DefinedAtom *target) {
    if (target && target->contentType() == DefinedAtom::typeResolver)
      const_cast<Reference &>(ref).setTarget(getIFUNCPLTEntry(target));
    return error_code::success();
  }

  /// \brief Create a GOT entry for the TP offset of a TLS atom.
  const GOTAtom *getGOTTPOFF(const DefinedAtom *atom) {
    auto got = _gotMap.find(atom);
    if (got == _gotMap.end()) {
      auto g = new (_file._alloc) GOTAtom(_file, ".got");
      g->addReference(R_X86_64_TPOFF64, 0, atom, 0);
#ifndef NDEBUG
      g->_name = "__got_tls_";
      g->_name += atom->name();
#endif
      _gotMap[atom] = g;
      return g;
    }
    return got->second;
  }

  /// \brief Create a TPOFF64 GOT entry and change the relocation to a PC32 to
  /// the GOT.
  void handleGOTTPOFF(const Reference &ref, const DefinedAtom *target) {
    const_cast<Reference &>(ref).setTarget(getGOTTPOFF(target));
    const_cast<Reference &>(ref).setKind(R_X86_64_PC32);
  }

  /// \brief Create a GOT entry containing 0.
  const GOTAtom *getNullGOT() {
    if (!_null) {
      _null = new (_file._alloc) GOTAtom(_file, ".got");
#ifndef NDEBUG
      _null->_name = "__got_null";
#endif
    }
    return _null;
  }

  const GOTAtom *getGOT(const DefinedAtom *da) {
    auto got = _gotMap.find(da);
    if (got == _gotMap.end()) {
      auto g = new (_file._alloc) GOTAtom(_file, ".got");
      g->addReference(R_X86_64_64, 0, da, 0);
#ifndef NDEBUG
      g->_name = "__got_";
      g->_name += da->name();
#endif
      _gotMap[da] = g;
      return g;
    }
    return got->second;
  }

  /// \brief Handle a GOTPCREL relocation to an undefined weak atom by using a
  /// null GOT entry.
  void handleGOTPCREL(const Reference &ref) {
    const_cast<Reference &>(ref).setKind(R_X86_64_PC32);
    if (isa<UndefinedAtom>(ref.target()))
      const_cast<Reference &>(ref).setTarget(getNullGOT());
    else if (const DefinedAtom *da = dyn_cast<const DefinedAtom>(ref.target()))
      const_cast<Reference &>(ref).setTarget(getGOT(da));
  }

public:
  GOTPLTPass(const ELFTargetInfo &ti) : _file(ti), _null(nullptr) {}

  /// \brief Do the pass.
  ///
  /// The goal here is to first process each reference individually. Each call
  /// to handleReference may modify the reference itself and/or create new
  /// atoms which must be stored in one of the maps below.
  ///
  /// After all references are handled, the atoms created during that are all
  /// added to mf.
  virtual void perform(MutableFile &mf) {
    // Process all references.
    for (const auto &atom : mf.defined())
      for (const auto &ref : *atom)
        handleReference(*atom, *ref);

    // Add all created atoms to the link.
    if (_null)
      mf.addAtom(*_null);
    for (const auto &got : _gotMap)
      mf.addAtom(*got.second);
    for (const auto &plt : _pltMap)
      mf.addAtom(*plt.second);
  }

protected:
  /// \brief Owner of all the Atoms created by this pass.
  ELFPassFile _file;
  /// \brief Map Atoms to their GOT entries.
  llvm::DenseMap<const Atom *, const GOTAtom *> _gotMap;
  /// \brief Map Atoms to their PLT entries.
  llvm::DenseMap<const Atom *, const PLTAtom *> _pltMap;
  /// \brief GOT entry that is always 0. Used for undefined weaks.
  GOTAtom *_null;
};

/// This implements the static relocation model. Meaning GOT and PLT entries are
/// not created for references that can be directly resolved. These are
/// converted to a direct relocation. For entries that do require a GOT or PLT
/// entry, that entry is statically bound.
///
/// TLS always assumes module 1 and attempts to remove indirection.
class StaticGOTPLTPass LLVM_FINAL : public GOTPLTPass<StaticGOTPLTPass> {
public:
  StaticGOTPLTPass(const elf::X86_64TargetInfo &ti) : GOTPLTPass(ti) {}

  ErrorOr<void> handlePLT32(const Reference &ref) {
    // __tls_get_addr is handled elsewhere.
    if (ref.target() && ref.target()->name() == "__tls_get_addr") {
      const_cast<Reference &>(ref).setKind(R_X86_64_NONE);
      return error_code::success();
    } else
      // Static code doesn't need PLTs.
      const_cast<Reference &>(ref).setKind(R_X86_64_PC32);
    // Handle IFUNC.
    if (const DefinedAtom *da = dyn_cast_or_null<const DefinedAtom>(ref.target()))
      if (da->contentType() == DefinedAtom::typeResolver)
        return handleIFUNC(ref, da);
    return error_code::success();
  }
};

class DynamicGOTPLTPass LLVM_FINAL : public GOTPLTPass<DynamicGOTPLTPass> {
public:
  DynamicGOTPLTPass(const elf::X86_64TargetInfo &ti) : GOTPLTPass(ti) {}

  const PLTAtom *getPLTEntry(const Atom *a) {
    auto plt = _pltMap.find(a);
    if (plt != _pltMap.end())
      return plt->second;
    auto ga = new (_file._alloc) GOTAtom(_file, ".got.plt");
    ga->addReference(R_X86_64_RELATIVE, 0, a, 0);
    auto pa = new (_file._alloc) PLTAtom(_file, ".plt");
    pa->addReference(R_X86_64_PC32, 2, ga, -4);
#ifndef NDEBUG
    ga->_name = "__got_";
    ga->_name += a->name();
    pa->_name = "__plt_";
    pa->_name += a->name();
#endif
    _gotMap[a] = ga;
    _pltMap[a] = pa;
    return pa;
  }

  ErrorOr<void> handlePLT32(const Reference &ref) {
    // Turn this into a PC32 to the PLT entry.
    const_cast<Reference &>(ref).setKind(R_X86_64_PC32);
    // Handle IFUNC.
    if (const DefinedAtom *da = dyn_cast_or_null<const DefinedAtom>(ref.target()))
      if (da->contentType() == DefinedAtom::typeResolver)
        return handleIFUNC(ref, da);
    const_cast<Reference &>(ref).setTarget(getPLTEntry(ref.target()));
    return error_code::success();
  }
};
} // end anon namespace

void elf::X86_64TargetInfo::addPasses(PassManager &pm) const {
  if (_options._outputKind == OutputKind::StaticExecutable)
    pm.add(std::unique_ptr<Pass>(new StaticGOTPLTPass(*this)));
  else if (_options._outputKind == OutputKind::DynamicExecutable ||
           _options._outputKind == OutputKind::Shared)
    pm.add(std::unique_ptr<Pass>(new DynamicGOTPLTPass(*this)));
}

#define LLD_CASE(name) .Case(#name, llvm::ELF::name)

ErrorOr<int32_t> elf::X86_64TargetInfo::relocKindFromString(
    StringRef str) const {
  int32_t ret = llvm::StringSwitch<int32_t>(str)
    LLD_CASE(R_X86_64_NONE)
    LLD_CASE(R_X86_64_64)
    LLD_CASE(R_X86_64_PC32)
    LLD_CASE(R_X86_64_GOT32)
    LLD_CASE(R_X86_64_PLT32)
    LLD_CASE(R_X86_64_COPY)
    LLD_CASE(R_X86_64_GLOB_DAT)
    LLD_CASE(R_X86_64_JUMP_SLOT)
    LLD_CASE(R_X86_64_RELATIVE)
    LLD_CASE(R_X86_64_GOTPCREL)
    LLD_CASE(R_X86_64_32)
    LLD_CASE(R_X86_64_32S)
    LLD_CASE(R_X86_64_16)
    LLD_CASE(R_X86_64_PC16)
    LLD_CASE(R_X86_64_8)
    LLD_CASE(R_X86_64_PC8)
    LLD_CASE(R_X86_64_DTPMOD64)
    LLD_CASE(R_X86_64_DTPOFF64)
    LLD_CASE(R_X86_64_TPOFF64)
    LLD_CASE(R_X86_64_TLSGD)
    LLD_CASE(R_X86_64_TLSLD)
    LLD_CASE(R_X86_64_DTPOFF32)
    LLD_CASE(R_X86_64_GOTTPOFF)
    LLD_CASE(R_X86_64_TPOFF32)
    LLD_CASE(R_X86_64_PC64)
    LLD_CASE(R_X86_64_GOTOFF64)
    LLD_CASE(R_X86_64_GOTPC32)
    LLD_CASE(R_X86_64_GOT64)
    LLD_CASE(R_X86_64_GOTPCREL64)
    LLD_CASE(R_X86_64_GOTPC64)
    LLD_CASE(R_X86_64_GOTPLT64)
    LLD_CASE(R_X86_64_PLTOFF64)
    LLD_CASE(R_X86_64_SIZE32)
    LLD_CASE(R_X86_64_SIZE64)
    LLD_CASE(R_X86_64_GOTPC32_TLSDESC)
    LLD_CASE(R_X86_64_TLSDESC_CALL)
    LLD_CASE(R_X86_64_TLSDESC)
    LLD_CASE(R_X86_64_IRELATIVE)
    .Default(-1);

  if (ret == -1)
    return make_error_code(yaml_reader_error::illegal_value);
  return ret;
}

#undef LLD_CASE

#define LLD_CASE(name) case llvm::ELF::name: return std::string(#name);

ErrorOr<std::string> elf::X86_64TargetInfo::stringFromRelocKind(
    int32_t kind) const {
  switch (kind) {
  LLD_CASE(R_X86_64_NONE)
  LLD_CASE(R_X86_64_64)
  LLD_CASE(R_X86_64_PC32)
  LLD_CASE(R_X86_64_GOT32)
  LLD_CASE(R_X86_64_PLT32)
  LLD_CASE(R_X86_64_COPY)
  LLD_CASE(R_X86_64_GLOB_DAT)
  LLD_CASE(R_X86_64_JUMP_SLOT)
  LLD_CASE(R_X86_64_RELATIVE)
  LLD_CASE(R_X86_64_GOTPCREL)
  LLD_CASE(R_X86_64_32)
  LLD_CASE(R_X86_64_32S)
  LLD_CASE(R_X86_64_16)
  LLD_CASE(R_X86_64_PC16)
  LLD_CASE(R_X86_64_8)
  LLD_CASE(R_X86_64_PC8)
  LLD_CASE(R_X86_64_DTPMOD64)
  LLD_CASE(R_X86_64_DTPOFF64)
  LLD_CASE(R_X86_64_TPOFF64)
  LLD_CASE(R_X86_64_TLSGD)
  LLD_CASE(R_X86_64_TLSLD)
  LLD_CASE(R_X86_64_DTPOFF32)
  LLD_CASE(R_X86_64_GOTTPOFF)
  LLD_CASE(R_X86_64_TPOFF32)
  LLD_CASE(R_X86_64_PC64)
  LLD_CASE(R_X86_64_GOTOFF64)
  LLD_CASE(R_X86_64_GOTPC32)
  LLD_CASE(R_X86_64_GOT64)
  LLD_CASE(R_X86_64_GOTPCREL64)
  LLD_CASE(R_X86_64_GOTPC64)
  LLD_CASE(R_X86_64_GOTPLT64)
  LLD_CASE(R_X86_64_PLTOFF64)
  LLD_CASE(R_X86_64_SIZE32)
  LLD_CASE(R_X86_64_SIZE64)
  LLD_CASE(R_X86_64_GOTPC32_TLSDESC)
  LLD_CASE(R_X86_64_TLSDESC_CALL)
  LLD_CASE(R_X86_64_TLSDESC)
  LLD_CASE(R_X86_64_IRELATIVE)
  }

  return make_error_code(yaml_reader_error::illegal_value);
}
