//===- Core/SymbolTable.cpp - Main Symbol Table ---------------------------===//
//
//                             The LLVM Linker
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lld/Core/SymbolTable.h"
#include "lld/Core/Atom.h"
#include "lld/Core/AbsoluteAtom.h"
#include "lld/Core/DefinedAtom.h"
#include "lld/Core/File.h"
#include "lld/Core/InputFiles.h"
#include "lld/Core/Platform.h"
#include "lld/Core/Resolver.h"
#include "lld/Core/SharedLibraryAtom.h"
#include "lld/Core/UndefinedAtom.h"

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/DenseMapInfo.h"
#include "llvm/Support/Casting.h"
#include "llvm/Support/ErrorHandling.h"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <vector>

namespace lld {

SymbolTable::SymbolTable(Platform& plat)
  : _platform(plat) {
}

void SymbolTable::add(const UndefinedAtom &atom) {
  this->addByName(atom);
}

void SymbolTable::add(const SharedLibraryAtom &atom) {
  this->addByName(atom);
}
  
void SymbolTable::add(const AbsoluteAtom &atom) {
  this->addByName(atom);
}

void SymbolTable::add(const DefinedAtom &atom) {
  assert(atom.scope() != DefinedAtom::scopeTranslationUnit);
  if ( !atom.name().empty() ) {
    this->addByName(atom);
  }
  else {
    this->addByContent(atom);
  }
}

enum NameCollisionResolution {
  NCR_First,
  NCR_Second,
  NCR_DupDef,
  NCR_DupUndef,
  NCR_DupShLib,
  NCR_Error
};

static NameCollisionResolution cases[4][4] = {
  //regular     absolute    undef      sharedLib
  {
    // first is regular
    NCR_DupDef, NCR_Error,   NCR_First, NCR_First
  },
  {
    // first is absolute
    NCR_Error,  NCR_Error,  NCR_First, NCR_First
  },
  {
    // first is undef
    NCR_Second, NCR_Second, NCR_DupUndef, NCR_Second
  },
  {
    // first is sharedLib
    NCR_Second, NCR_Second, NCR_First, NCR_DupShLib
  }
};

static NameCollisionResolution collide(Atom::Definition first,
                                       Atom::Definition second) {
  return cases[first][second];
}


enum MergeResolution {
  MCR_First,
  MCR_Second,
  MCR_Largest,
  MCR_Error
};

static MergeResolution mergeCases[4][4] = {
  // no        tentative     weak       weakAddressUsed
  {
    // first is no
    MCR_Error,  MCR_First,   MCR_First, MCR_First
  },
  {
    // first is tentative
    MCR_Second, MCR_Largest, MCR_Second, MCR_Second
  },
  {
    // first is weak
    MCR_Second, MCR_First,   MCR_First, MCR_Second
  },
  {
    // first is weakAddressUsed
    MCR_Second, MCR_First,   MCR_First, MCR_First
  }
};

static MergeResolution mergeSelect(DefinedAtom::Merge first, 
                                   DefinedAtom::Merge second) {
  return mergeCases[first][second];
}


void SymbolTable::addByName(const Atom & newAtom) {
  llvm::StringRef name = newAtom.name();
  const Atom *existing = this->findByName(name);
  if (existing == nullptr) {
    // Name is not in symbol table yet, add it associate with this atom.
    _nameTable[name] = &newAtom;
  } 
  else {
    // Name is already in symbol table and associated with another atom.
    bool useNew = true;
    switch (collide(existing->definition(), newAtom.definition())) {
      case NCR_First:
        useNew = false;
        break;
      case NCR_Second:
        useNew = true;
        break;
      case NCR_DupDef:
        assert(existing->definition() == Atom::definitionRegular);
        assert(newAtom.definition() == Atom::definitionRegular);
        switch ( mergeSelect(((DefinedAtom*)existing)->merge(), 
                            ((DefinedAtom*)(&newAtom))->merge()) ) {
          case MCR_First:
            useNew = false;
            break;
          case MCR_Second:
            useNew = true;
            break;
          case MCR_Largest:
            useNew = true;
            break;
          case MCR_Error:
            llvm::report_fatal_error("duplicate symbol error");
            break;
        }
        break;
      case NCR_DupUndef: {
          const UndefinedAtom* existingUndef =
            llvm::dyn_cast<UndefinedAtom>(existing);
          const UndefinedAtom* newUndef =
            llvm::dyn_cast<UndefinedAtom>(&newAtom);
          assert(existingUndef != nullptr);
          assert(newUndef != nullptr);
          if ( existingUndef->canBeNull() == newUndef->canBeNull() ) {
            useNew = false;
          }
          else {
            useNew = (newUndef->canBeNull() < existingUndef->canBeNull());
            // give platform a change to override which to use
            _platform.undefineCanBeNullMismatch(*existingUndef, 
                                                 *newUndef, useNew);
          }
        }
        break;
      case NCR_DupShLib: {
          const SharedLibraryAtom* existingShLib =
            llvm::dyn_cast<SharedLibraryAtom>(existing);
          const SharedLibraryAtom* newShLib =
            llvm::dyn_cast<SharedLibraryAtom>(&newAtom);
          assert(existingShLib != nullptr);
          assert(newShLib != nullptr);
          if ( (existingShLib->canBeNullAtRuntime() 
                  == newShLib->canBeNullAtRuntime()) &&
               existingShLib->loadName().equals(newShLib->loadName()) ) {
            useNew = false;
          }
          else {
            useNew = false; // use existing shared library by default
            // give platform a change to override which to use
            _platform.sharedLibrarylMismatch(*existingShLib, *newShLib, useNew);
          }
        }
        break;
      default:
        llvm::report_fatal_error("SymbolTable::addByName(): unhandled switch clause");
    }
    if ( useNew ) {
      // Update name table to use new atom.
      _nameTable[name] = &newAtom;
      // Add existing atom to replacement table.
      _replacedAtoms[existing] = &newAtom;
    }
    else {
      // New atom is not being used.  Add it to replacement table.
      _replacedAtoms[&newAtom] = existing;
    }
  }
}


unsigned SymbolTable::AtomMappingInfo::getHashValue(const DefinedAtom * const atom) {
  unsigned hash = atom->size();
  if ( atom->contentType() != DefinedAtom::typeZeroFill ) {
    llvm::ArrayRef<uint8_t> content = atom->rawContent();
    for (unsigned int i=0; i < content.size(); ++i) {
      hash = hash * 33 + content[i];
    }
  }
  hash &= 0x00FFFFFF;
  hash |= ((unsigned)atom->contentType()) << 24;
  //fprintf(stderr, "atom=%p, hash=0x%08X\n", atom, hash);
  return hash;
}


bool SymbolTable::AtomMappingInfo::isEqual(const DefinedAtom * const l, 
                                         const DefinedAtom * const r) {
  if ( l == r )
    return true;
  if ( l == getEmptyKey() )
    return false;
  if ( r == getEmptyKey() )
    return false;
  if ( l == getTombstoneKey() )
    return false;
  if ( r == getTombstoneKey() )
    return false;
    
  if ( l->contentType() != r->contentType() )
    return false;
  if ( l->size() != r->size() )
    return false;
  llvm::ArrayRef<uint8_t> lc = l->rawContent();
  llvm::ArrayRef<uint8_t> rc = r->rawContent();
  return lc.equals(rc);
}


void SymbolTable::addByContent(const DefinedAtom & newAtom) {
  AtomContentSet::iterator pos = _contentTable.find(&newAtom);
  if ( pos == _contentTable.end() ) {
    _contentTable.insert(&newAtom);
    return;
  }
  const Atom* existing = *pos;
    // New atom is not being used.  Add it to replacement table.
    _replacedAtoms[&newAtom] = existing;
}



const Atom *SymbolTable::findByName(llvm::StringRef sym) {
  NameToAtom::iterator pos = _nameTable.find(sym);
  if (pos == _nameTable.end())
    return nullptr;
  return pos->second;
}

bool SymbolTable::isDefined(llvm::StringRef sym) {
  const Atom *atom = this->findByName(sym);
  if (atom == nullptr)
    return false;
  if (atom->definition() == Atom::definitionUndefined)
    return false;
  return true;
}

const Atom *SymbolTable::replacement(const Atom *atom) {
  AtomToAtom::iterator pos = _replacedAtoms.find(atom);
  if (pos == _replacedAtoms.end())
    return atom;
  // might be chain, recurse to end
  return this->replacement(pos->second);
}

unsigned int SymbolTable::size() {
  return _nameTable.size();
}

void SymbolTable::undefines(std::vector<const Atom *> &undefs) {
  for (NameToAtom::iterator it = _nameTable.begin(),
       end = _nameTable.end(); it != end; ++it) {
    const Atom *atom = it->second;
    assert(atom != nullptr);
    if (atom->definition() == Atom::definitionUndefined)
      undefs.push_back(atom);
  }
}

} // namespace lld
