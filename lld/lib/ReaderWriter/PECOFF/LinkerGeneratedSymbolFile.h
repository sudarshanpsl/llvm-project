//===- lib/ReaderWriter/PECOFF/LinkerGeneratedSymbolFile.cpp --------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "Atoms.h"

#include "lld/Core/ArchiveLibraryFile.h"
#include "lld/ReaderWriter/PECOFFLinkingContext.h"
#include "lld/ReaderWriter/Simple.h"
#include "llvm/Support/Allocator.h"

namespace lld {
namespace pecoff {

namespace {

/// The defined atom for dllexported symbols with __imp_ prefix.
class ImpPointerAtom : public COFFLinkerInternalAtom {
public:
  ImpPointerAtom(const File &file, StringRef symbolName, uint64_t ordinal)
      : COFFLinkerInternalAtom(file, /*oridnal*/ 0, std::vector<uint8_t>(4),
                               symbolName),
        _ordinal(ordinal) {}

  uint64_t ordinal() const override { return _ordinal; }
  Scope scope() const override { return scopeGlobal; }
  ContentType contentType() const override { return typeData; }
  Alignment alignment() const override { return Alignment(4); }
  ContentPermissions permissions() const override { return permR__; }

private:
  uint64_t _ordinal;
};

class ImpSymbolFile : public SimpleFile {
public:
  ImpSymbolFile(StringRef defsym, StringRef undefsym, uint64_t ordinal)
      : SimpleFile(defsym), _undefined(*this, undefsym),
        _defined(*this, defsym, ordinal) {
    _defined.addReference(std::unique_ptr<COFFReference>(
        new COFFReference(&_undefined, 0, llvm::COFF::IMAGE_REL_I386_DIR32)));
    addAtom(_defined);
    addAtom(_undefined);
  };

private:
  SimpleUndefinedAtom _undefined;
  ImpPointerAtom _defined;
};

} // anonymous namespace

// A virtual file containing absolute symbol __ImageBase. __ImageBase (or
// ___ImageBase on x86) is a linker-generated symbol whose address is the same
// as the image base address.
class LinkerGeneratedSymbolFile : public SimpleFile {
public:
  LinkerGeneratedSymbolFile(const PECOFFLinkingContext &ctx)
      : SimpleFile("<linker-internal-file>"),
        _imageBaseAtom(*this, ctx.decorateSymbol("__ImageBase"),
                       Atom::scopeGlobal, ctx.getBaseAddress()) {
    addAtom(_imageBaseAtom);
  };

private:
  COFFAbsoluteAtom _imageBaseAtom;
};

// A LocallyImporteSymbolFile is an archive file containing _imp_
// symbols for local use.
//
// For each defined symbol, linker creates an implicit defined symbol
// by appending "_imp_" prefix to the original name. The content of
// the implicit symbol is a pointer to the original symbol
// content. This feature allows one to compile and link the following
// code without error, although _imp__hello is not defined in the
// code.
//
//   void hello() { printf("Hello\n"); }
//   extern void (*_imp__hello)();
//   int main() {
//      _imp__hello();
//      return 0;
//   }
//
// This odd feature is for the compatibility with MSVC link.exe.
class LocallyImportedSymbolFile : public ArchiveLibraryFile {
public:
  LocallyImportedSymbolFile(const PECOFFLinkingContext &ctx)
      : ArchiveLibraryFile("__imp_"), _prefix(ctx.decorateSymbol("_imp_")),
        _ordinal(0) {}

  const File *find(StringRef sym, bool dataSymbolOnly) const override {
    if (!sym.startswith(_prefix))
      return nullptr;
    StringRef undef = sym.substr(_prefix.size());
    return new (_alloc) ImpSymbolFile(sym, undef, _ordinal++);
  }

  const atom_collection<DefinedAtom> &defined() const override {
    return _definedAtoms;
  }

  const atom_collection<UndefinedAtom> &undefined() const override {
    return _undefinedAtoms;
  }

  const atom_collection<SharedLibraryAtom> &sharedLibrary() const override {
    return _sharedLibraryAtoms;
  }

  const atom_collection<AbsoluteAtom> &absolute() const override {
    return _absoluteAtoms;
  }

  error_code
  parseAllMembers(std::vector<std::unique_ptr<File>> &result) const override {
    return error_code::success();
  }

private:
  std::string _prefix;
  mutable uint64_t _ordinal;
  mutable llvm::BumpPtrAllocator _alloc;

  atom_collection_vector<DefinedAtom> _definedAtoms;
  atom_collection_vector<UndefinedAtom> _undefinedAtoms;
  atom_collection_vector<SharedLibraryAtom> _sharedLibraryAtoms;
  atom_collection_vector<AbsoluteAtom> _absoluteAtoms;
};

} // end namespace pecoff
} // end namespace lld
