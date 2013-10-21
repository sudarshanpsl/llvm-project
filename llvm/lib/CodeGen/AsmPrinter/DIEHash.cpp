//===-- llvm/CodeGen/DIEHash.cpp - Dwarf Hashing Framework ----------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for DWARF4 hashing of DIEs.
//
//===----------------------------------------------------------------------===//

#define DEBUG_TYPE "dwarfdebug"

#include "DIE.h"
#include "DIEHash.h"
#include "DwarfCompileUnit.h"
#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringRef.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/Dwarf.h"
#include "llvm/Support/Endian.h"
#include "llvm/Support/MD5.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

/// \brief Grabs the string in whichever attribute is passed in and returns
/// a reference to it.
static StringRef getDIEStringAttr(DIE *Die, uint16_t Attr) {
  const SmallVectorImpl<DIEValue *> &Values = Die->getValues();
  const DIEAbbrev &Abbrevs = Die->getAbbrev();

  // Iterate through all the attributes until we find the one we're
  // looking for, if we can't find it return an empty string.
  for (size_t i = 0; i < Values.size(); ++i) {
    if (Abbrevs.getData()[i].getAttribute() == Attr) {
      DIEValue *V = Values[i];
      assert(isa<DIEString>(V) && "String requested. Not a string.");
      DIEString *S = cast<DIEString>(V);
      return S->getString();
    }
  }
  return StringRef("");
}

/// \brief Adds the string in \p Str to the hash. This also hashes
/// a trailing NULL with the string.
void DIEHash::addString(StringRef Str) {
  DEBUG(dbgs() << "Adding string " << Str << " to hash.\n");
  Hash.update(Str);
  Hash.update(makeArrayRef((uint8_t)'\0'));
}

// FIXME: The LEB128 routines are copied and only slightly modified out of
// LEB128.h.

/// \brief Adds the unsigned in \p Value to the hash encoded as a ULEB128.
void DIEHash::addULEB128(uint64_t Value) {
  DEBUG(dbgs() << "Adding ULEB128 " << Value << " to hash.\n");
  do {
    uint8_t Byte = Value & 0x7f;
    Value >>= 7;
    if (Value != 0)
      Byte |= 0x80; // Mark this byte to show that more bytes will follow.
    Hash.update(Byte);
  } while (Value != 0);
}

void DIEHash::addSLEB128(int64_t Value) {
  DEBUG(dbgs() << "Adding ULEB128 " << Value << " to hash.\n");
  bool More;
  do {
    uint8_t Byte = Value & 0x7f;
    Value >>= 7;
    More = !((((Value == 0 ) && ((Byte & 0x40) == 0)) ||
              ((Value == -1) && ((Byte & 0x40) != 0))));
    if (More)
      Byte |= 0x80; // Mark this byte to show that more bytes will follow.
    Hash.update(Byte);
  } while (More);
}

/// \brief Including \p Parent adds the context of Parent to the hash..
void DIEHash::addParentContext(DIE *Parent) {

  DEBUG(dbgs() << "Adding parent context to hash...\n");

  // [7.27.2] For each surrounding type or namespace beginning with the
  // outermost such construct...
  SmallVector<DIE *, 1> Parents;
  while (Parent->getTag() != dwarf::DW_TAG_compile_unit) {
    Parents.push_back(Parent);
    Parent = Parent->getParent();
  }

  // Reverse iterate over our list to go from the outermost construct to the
  // innermost.
  for (SmallVectorImpl<DIE *>::reverse_iterator I = Parents.rbegin(),
                                                E = Parents.rend();
       I != E; ++I) {
    DIE *Die = *I;

    // ... Append the letter "C" to the sequence...
    addULEB128('C');

    // ... Followed by the DWARF tag of the construct...
    addULEB128(Die->getTag());

    // ... Then the name, taken from the DW_AT_name attribute.
    StringRef Name = getDIEStringAttr(Die, dwarf::DW_AT_name);
    DEBUG(dbgs() << "... adding context: " << Name << "\n");
    if (!Name.empty())
      addString(Name);
  }
}

// Collect all of the attributes for a particular DIE in single structure.
void DIEHash::collectAttributes(DIE *Die, DIEAttrs &Attrs) {
  const SmallVectorImpl<DIEValue *> &Values = Die->getValues();
  const DIEAbbrev &Abbrevs = Die->getAbbrev();

#define COLLECT_ATTR(NAME)                                                     \
  case dwarf::NAME:                                                            \
    Attrs.NAME.Val = Values[i];                                                \
    Attrs.NAME.Desc = &Abbrevs.getData()[i];                                   \
    break

  for (size_t i = 0, e = Values.size(); i != e; ++i) {
    DEBUG(dbgs() << "Attribute: "
                 << dwarf::AttributeString(Abbrevs.getData()[i].getAttribute())
                 << " added.\n");
    switch (Abbrevs.getData()[i].getAttribute()) {
    COLLECT_ATTR(DW_AT_name);
    COLLECT_ATTR(DW_AT_accessibility);
    COLLECT_ATTR(DW_AT_address_class);
    COLLECT_ATTR(DW_AT_allocated);
    COLLECT_ATTR(DW_AT_artificial);
    COLLECT_ATTR(DW_AT_associated);
    COLLECT_ATTR(DW_AT_binary_scale);
    COLLECT_ATTR(DW_AT_bit_offset);
    COLLECT_ATTR(DW_AT_bit_size);
    COLLECT_ATTR(DW_AT_bit_stride);
    COLLECT_ATTR(DW_AT_byte_size);
    COLLECT_ATTR(DW_AT_byte_stride);
    COLLECT_ATTR(DW_AT_const_expr);
    COLLECT_ATTR(DW_AT_const_value);
    COLLECT_ATTR(DW_AT_containing_type);
    COLLECT_ATTR(DW_AT_count);
    COLLECT_ATTR(DW_AT_data_bit_offset);
    COLLECT_ATTR(DW_AT_data_location);
    COLLECT_ATTR(DW_AT_data_member_location);
    COLLECT_ATTR(DW_AT_decimal_scale);
    COLLECT_ATTR(DW_AT_decimal_sign);
    COLLECT_ATTR(DW_AT_default_value);
    COLLECT_ATTR(DW_AT_digit_count);
    COLLECT_ATTR(DW_AT_discr);
    COLLECT_ATTR(DW_AT_discr_list);
    COLLECT_ATTR(DW_AT_discr_value);
    COLLECT_ATTR(DW_AT_encoding);
    COLLECT_ATTR(DW_AT_enum_class);
    COLLECT_ATTR(DW_AT_endianity);
    COLLECT_ATTR(DW_AT_explicit);
    COLLECT_ATTR(DW_AT_is_optional);
    COLLECT_ATTR(DW_AT_location);
    COLLECT_ATTR(DW_AT_lower_bound);
    COLLECT_ATTR(DW_AT_mutable);
    COLLECT_ATTR(DW_AT_ordering);
    COLLECT_ATTR(DW_AT_picture_string);
    COLLECT_ATTR(DW_AT_prototyped);
    COLLECT_ATTR(DW_AT_small);
    COLLECT_ATTR(DW_AT_segment);
    COLLECT_ATTR(DW_AT_string_length);
    COLLECT_ATTR(DW_AT_threads_scaled);
    COLLECT_ATTR(DW_AT_upper_bound);
    COLLECT_ATTR(DW_AT_use_location);
    COLLECT_ATTR(DW_AT_use_UTF8);
    COLLECT_ATTR(DW_AT_variable_parameter);
    COLLECT_ATTR(DW_AT_virtuality);
    COLLECT_ATTR(DW_AT_visibility);
    COLLECT_ATTR(DW_AT_vtable_elem_location);
    COLLECT_ATTR(DW_AT_type);
    default:
      break;
    }
  }
}

// Hash an individual attribute \param Attr based on the type of attribute and
// the form.
void DIEHash::hashAttribute(AttrEntry Attr, dwarf::Tag Tag) {
  const DIEValue *Value = Attr.Val;
  const DIEAbbrevData *Desc = Attr.Desc;

  // 7.27 Step 3
  // ... An attribute that refers to another type entry T is processed as
  // follows:
  if (const DIEEntry *EntryAttr = dyn_cast<DIEEntry>(Value)) {
    DIE *Entry = EntryAttr->getEntry();

    // Step 5
    // If the tag in Step 3 is one of ...
    if (Tag == dwarf::DW_TAG_pointer_type) {
      // ... and the referenced type (via the DW_AT_type or DW_AT_friend
      // attribute) ...
      assert(Desc->getAttribute() == dwarf::DW_AT_type ||
             Desc->getAttribute() == dwarf::DW_AT_friend);
      // [FIXME] ... has a DW_AT_name attribute,
      // append the letter 'N'
      addULEB128('N');

      // the DWARF attribute code (DW_AT_type or DW_AT_friend),
      addULEB128(Desc->getAttribute());

      // the context of the tag,
      if (DIE *Parent = Entry->getParent())
        addParentContext(Parent);

      // the letter 'E',
      addULEB128('E');

      // and the name of the type.
      addString(getDIEStringAttr(Entry, dwarf::DW_AT_name));

      // FIXME:
      // For DW_TAG_friend, if the referenced entry is the DW_TAG_subprogram,
      // the context is omitted and the name to be used is the ABI-specific name
      // of the subprogram (e.g., the mangled linker name).
      return;
    }

    unsigned &DieNumber = Numbering[Entry];
    if (DieNumber) {
      // a) If T is in the list of [previously hashed types], use the letter
      // 'R' as the marker
      addULEB128('R');

      addULEB128(Desc->getAttribute());

      // and use the unsigned LEB128 encoding of [the index of T in the
      // list] as the attribute value;
      addULEB128(DieNumber);
      return;
    }

    // otherwise, b) use the letter 'T' as a the marker, ...
    addULEB128('T');

    addULEB128(Desc->getAttribute());

    // ... process the type T recursively by performing Steps 2 through 7, and
    // use the result as the attribute value.
    DieNumber = Numbering.size();
    computeHash(Entry);
    return;
  }

  // Other attribute values use the letter 'A' as the marker, ...
  addULEB128('A');

  addULEB128(Desc->getAttribute());

  // ... and the value consists of the form code (encoded as an unsigned LEB128
  // value) followed by the encoding of the value according to the form code. To
  // ensure reproducibility of the signature, the set of forms used in the
  // signature computation is limited to the following: DW_FORM_sdata,
  // DW_FORM_flag, DW_FORM_string, and DW_FORM_block.
  switch (Desc->getForm()) {
  case dwarf::DW_FORM_string:
    llvm_unreachable(
        "Add support for DW_FORM_string if we ever start emitting them again");
  case dwarf::DW_FORM_GNU_str_index:
  case dwarf::DW_FORM_strp:
    addULEB128(dwarf::DW_FORM_string);
    addString(cast<DIEString>(Value)->getString());
    break;
  case dwarf::DW_FORM_data1:
  case dwarf::DW_FORM_data2:
  case dwarf::DW_FORM_data4:
  case dwarf::DW_FORM_data8:
  case dwarf::DW_FORM_udata:
    addULEB128(dwarf::DW_FORM_sdata);
    addSLEB128((int64_t)cast<DIEInteger>(Value)->getValue());
    break;
  default:
    llvm_unreachable("Add support for additional forms");
  }
}

// Go through the attributes from \param Attrs in the order specified in 7.27.4
// and hash them.
void DIEHash::hashAttributes(const DIEAttrs &Attrs, dwarf::Tag Tag) {
#define ADD_ATTR(ATTR)                                                         \
  {                                                                            \
    if (ATTR.Val != 0)                                                         \
      hashAttribute(ATTR, Tag);                                                \
  }

  ADD_ATTR(Attrs.DW_AT_name);
  ADD_ATTR(Attrs.DW_AT_accessibility);
  ADD_ATTR(Attrs.DW_AT_address_class);
  ADD_ATTR(Attrs.DW_AT_allocated);
  ADD_ATTR(Attrs.DW_AT_artificial);
  ADD_ATTR(Attrs.DW_AT_associated);
  ADD_ATTR(Attrs.DW_AT_binary_scale);
  ADD_ATTR(Attrs.DW_AT_bit_offset);
  ADD_ATTR(Attrs.DW_AT_bit_size);
  ADD_ATTR(Attrs.DW_AT_bit_stride);
  ADD_ATTR(Attrs.DW_AT_byte_size);
  ADD_ATTR(Attrs.DW_AT_byte_stride);
  ADD_ATTR(Attrs.DW_AT_const_expr);
  ADD_ATTR(Attrs.DW_AT_const_value);
  ADD_ATTR(Attrs.DW_AT_containing_type);
  ADD_ATTR(Attrs.DW_AT_count);
  ADD_ATTR(Attrs.DW_AT_data_bit_offset);
  ADD_ATTR(Attrs.DW_AT_data_location);
  ADD_ATTR(Attrs.DW_AT_data_member_location);
  ADD_ATTR(Attrs.DW_AT_decimal_scale);
  ADD_ATTR(Attrs.DW_AT_decimal_sign);
  ADD_ATTR(Attrs.DW_AT_default_value);
  ADD_ATTR(Attrs.DW_AT_digit_count);
  ADD_ATTR(Attrs.DW_AT_discr);
  ADD_ATTR(Attrs.DW_AT_discr_list);
  ADD_ATTR(Attrs.DW_AT_discr_value);
  ADD_ATTR(Attrs.DW_AT_encoding);
  ADD_ATTR(Attrs.DW_AT_enum_class);
  ADD_ATTR(Attrs.DW_AT_endianity);
  ADD_ATTR(Attrs.DW_AT_explicit);
  ADD_ATTR(Attrs.DW_AT_is_optional);
  ADD_ATTR(Attrs.DW_AT_location);
  ADD_ATTR(Attrs.DW_AT_lower_bound);
  ADD_ATTR(Attrs.DW_AT_mutable);
  ADD_ATTR(Attrs.DW_AT_ordering);
  ADD_ATTR(Attrs.DW_AT_picture_string);
  ADD_ATTR(Attrs.DW_AT_prototyped);
  ADD_ATTR(Attrs.DW_AT_small);
  ADD_ATTR(Attrs.DW_AT_segment);
  ADD_ATTR(Attrs.DW_AT_string_length);
  ADD_ATTR(Attrs.DW_AT_threads_scaled);
  ADD_ATTR(Attrs.DW_AT_upper_bound);
  ADD_ATTR(Attrs.DW_AT_use_location);
  ADD_ATTR(Attrs.DW_AT_use_UTF8);
  ADD_ATTR(Attrs.DW_AT_variable_parameter);
  ADD_ATTR(Attrs.DW_AT_virtuality);
  ADD_ATTR(Attrs.DW_AT_visibility);
  ADD_ATTR(Attrs.DW_AT_vtable_elem_location);
  ADD_ATTR(Attrs.DW_AT_type);

  // FIXME: Add the extended attributes.
}

// Add all of the attributes for \param Die to the hash.
void DIEHash::addAttributes(DIE *Die) {
  DIEAttrs Attrs = {};
  collectAttributes(Die, Attrs);
  hashAttributes(Attrs, Die->getTag());
}

// Compute the hash of a DIE. This is based on the type signature computation
// given in section 7.27 of the DWARF4 standard. It is the md5 hash of a
// flattened description of the DIE.
void DIEHash::computeHash(DIE *Die) {
  // Append the letter 'D', followed by the DWARF tag of the DIE.
  addULEB128('D');
  addULEB128(Die->getTag());

  // Add each of the attributes of the DIE.
  addAttributes(Die);

  // Then hash each of the children of the DIE.
  for (std::vector<DIE *>::const_iterator I = Die->getChildren().begin(),
                                          E = Die->getChildren().end();
       I != E; ++I)
    computeHash(*I);

  // Following the last (or if there are no children), append a zero byte.
  Hash.update(makeArrayRef((uint8_t)'\0'));
}

/// This is based on the type signature computation given in section 7.27 of the
/// DWARF4 standard. It is the md5 hash of a flattened description of the DIE
/// with the exception that we are hashing only the context and the name of the
/// type.
uint64_t DIEHash::computeDIEODRSignature(DIE *Die) {

  // Add the contexts to the hash. We won't be computing the ODR hash for
  // function local types so it's safe to use the generic context hashing
  // algorithm here.
  // FIXME: If we figure out how to account for linkage in some way we could
  // actually do this with a slight modification to the parent hash algorithm.
  DIE *Parent = Die->getParent();
  if (Parent)
    addParentContext(Parent);

  // Add the current DIE information.

  // Add the DWARF tag of the DIE.
  addULEB128(Die->getTag());

  // Add the name of the type to the hash.
  addString(getDIEStringAttr(Die, dwarf::DW_AT_name));

  // Now get the result.
  MD5::MD5Result Result;
  Hash.final(Result);

  // ... take the least significant 8 bytes and return those. Our MD5
  // implementation always returns its results in little endian, swap bytes
  // appropriately.
  return *reinterpret_cast<support::ulittle64_t *>(Result + 8);
}

/// This is based on the type signature computation given in section 7.27 of the
/// DWARF4 standard. It is an md5 hash of the flattened description of the DIE
/// with the inclusion of the full CU and all top level CU entities.
// TODO: Initialize the type chain at 0 instead of 1 for CU signatures.
uint64_t DIEHash::computeCUSignature(DIE *Die) {
  Numbering.clear();
  Numbering[Die] = 1;

  // Hash the DIE.
  computeHash(Die);

  // Now return the result.
  MD5::MD5Result Result;
  Hash.final(Result);

  // ... take the least significant 8 bytes and return those. Our MD5
  // implementation always returns its results in little endian, swap bytes
  // appropriately.
  return *reinterpret_cast<support::ulittle64_t *>(Result + 8);
}

/// This is based on the type signature computation given in section 7.27 of the
/// DWARF4 standard. It is an md5 hash of the flattened description of the DIE
/// with the inclusion of additional forms not specifically called out in the
/// standard.
uint64_t DIEHash::computeTypeSignature(DIE *Die) {
  Numbering.clear();
  Numbering[Die] = 1;

  if (DIE *Parent = Die->getParent())
    addParentContext(Parent);

  // Hash the DIE.
  computeHash(Die);

  // Now return the result.
  MD5::MD5Result Result;
  Hash.final(Result);

  // ... take the least significant 8 bytes and return those. Our MD5
  // implementation always returns its results in little endian, swap bytes
  // appropriately.
  return *reinterpret_cast<support::ulittle64_t *>(Result + 8);
}
