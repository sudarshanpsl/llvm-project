//===-- Symbol.cpp ----------------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Symbol/Symbol.h"

#include "lldb/Core/Module.h"
#include "lldb/Core/Section.h"
#include "lldb/Core/Stream.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"

using namespace lldb;
using namespace lldb_private;


Symbol::Symbol() :
    SymbolContextScope (),
    m_uid (UINT32_MAX),
    m_mangled (),
    m_type_data (0),
    m_type_data_resolved (false),
    m_is_synthetic (false),
    m_is_debug (false),
    m_is_external (false),
    m_size_is_sibling (false),
    m_size_is_synthesized (false),
    m_calculated_size (false),
    m_type (eSymbolTypeInvalid),
    m_flags (),
    m_addr_range ()
{
}

Symbol::Symbol
(
    uint32_t symID,
    const char *name,
    bool name_is_mangled,
    SymbolType type,
    bool external,
    bool is_debug,
    bool is_trampoline,
    bool is_artificial,
    const lldb::SectionSP &section_sp,
    addr_t offset,
    uint32_t size,
    uint32_t flags
) :
    SymbolContextScope (),
    m_uid (symID),
    m_mangled (name, name_is_mangled),
    m_type_data (0),
    m_type_data_resolved (false),
    m_is_synthetic (is_artificial),
    m_is_debug (is_debug),
    m_is_external (external),
    m_size_is_sibling (false),
    m_size_is_synthesized (false),
    m_calculated_size (size > 0),
    m_type (type),
    m_flags (flags),
    m_addr_range (section_sp, offset, size)
{
}

Symbol::Symbol
(
    uint32_t symID,
    const char *name,
    bool name_is_mangled,
    SymbolType type,
    bool external,
    bool is_debug,
    bool is_trampoline,
    bool is_artificial,
    const AddressRange &range,
    uint32_t flags
) :
    SymbolContextScope (),
    m_uid (symID),
    m_mangled (name, name_is_mangled),
    m_type_data (0),
    m_type_data_resolved (false),
    m_is_synthetic (is_artificial),
    m_is_debug (is_debug),
    m_is_external (external),
    m_size_is_sibling (false),
    m_size_is_synthesized (false),
    m_calculated_size (range.GetByteSize() > 0),
    m_type (type),
    m_flags (flags),
    m_addr_range (range)
{
}

Symbol::Symbol(const Symbol& rhs):
    SymbolContextScope (rhs),
    m_uid (rhs.m_uid),
    m_mangled (rhs.m_mangled),
    m_type_data (rhs.m_type_data),
    m_type_data_resolved (rhs.m_type_data_resolved),
    m_is_synthetic (rhs.m_is_synthetic),
    m_is_debug (rhs.m_is_debug),
    m_is_external (rhs.m_is_external),
    m_size_is_sibling (rhs.m_size_is_sibling),
    m_size_is_synthesized (false),
    m_calculated_size (rhs.m_calculated_size),
    m_type (rhs.m_type),
    m_flags (rhs.m_flags),
    m_addr_range (rhs.m_addr_range)
{
}

const Symbol&
Symbol::operator= (const Symbol& rhs)
{
    if (this != &rhs)
    {
        SymbolContextScope::operator= (rhs);
        m_uid = rhs.m_uid;
        m_mangled = rhs.m_mangled;
        m_type_data = rhs.m_type_data;
        m_type_data_resolved = rhs.m_type_data_resolved;
        m_is_synthetic = rhs.m_is_synthetic;
        m_is_debug = rhs.m_is_debug;
        m_is_external = rhs.m_is_external;
        m_size_is_sibling = rhs.m_size_is_sibling;
        m_size_is_synthesized = rhs.m_size_is_sibling;
        m_calculated_size = rhs.m_calculated_size;
        m_type = rhs.m_type;
        m_flags = rhs.m_flags;
        m_addr_range = rhs.m_addr_range;
    }
    return *this;
}

void
Symbol::Clear()
{
    m_uid = UINT32_MAX;
    m_mangled.Clear();
    m_type_data = 0;
    m_type_data_resolved = false;
    m_is_synthetic = false;
    m_is_debug = false;
    m_is_external = false;
    m_size_is_sibling = false;
    m_size_is_synthesized = false;
    m_calculated_size = false;
    m_type = eSymbolTypeInvalid;
    m_flags = 0;
    m_addr_range.Clear();
}

AddressRange *
Symbol::GetAddressRangePtr()
{
    if (m_addr_range.GetBaseAddress().GetSection())
    {
        if (!m_calculated_size)
            GetByteSize();
        return &m_addr_range;
    }
    return NULL;
}

const AddressRange *
Symbol::GetAddressRangePtr() const
{
    if (m_addr_range.GetBaseAddress().GetSection())
    {
        if (!m_calculated_size)
            GetByteSize();
        return &m_addr_range;
    }
    return NULL;
}

uint32_t
Symbol::GetSiblingIndex() const
{
    return m_size_is_sibling ? m_addr_range.GetByteSize() : 0;
}

bool
Symbol::IsTrampoline () const
{
    return m_type == eSymbolTypeTrampoline;
}

void
Symbol::GetDescription (Stream *s, lldb::DescriptionLevel level, Target *target) const
{
    s->Printf("uid={%6u}", m_uid);
    
    if (m_addr_range.GetBaseAddress().GetSection())
    {
        if (m_addr_range.GetBaseAddress().IsSectionOffset())
        {
            if (m_addr_range.GetByteSize() > 0)
            {
                s->PutCString (", range = ");
                m_addr_range.Dump(s, target, Address::DumpStyleLoadAddress, Address::DumpStyleFileAddress);
            }
            else 
            {
                s->PutCString (", address = ");
                m_addr_range.GetBaseAddress().Dump(s, target, Address::DumpStyleLoadAddress, Address::DumpStyleFileAddress);
            }
        }
        else
            s->Printf (", value = 0x%16.16llx", m_addr_range.GetBaseAddress().GetOffset());
    }
    else
    {
        if (m_size_is_sibling)                
            s->Printf (", sibling = %5llu", m_addr_range.GetBaseAddress().GetOffset());
        else
            s->Printf (", value = 0x%16.16llx", m_addr_range.GetBaseAddress().GetOffset());
    }
    if (m_mangled.GetDemangledName())
        s->Printf(", name=\"%s\"", m_mangled.GetDemangledName().AsCString());
    if (m_mangled.GetMangledName())
        s->Printf(", mangled=\"%s\"", m_mangled.GetMangledName().AsCString());

}

void
Symbol::Dump(Stream *s, Target *target, uint32_t index) const
{
//  s->Printf("%.*p: ", (int)sizeof(void*) * 2, this);
//  s->Indent();
//  s->Printf("Symbol[%5u] %6u %c%c %-12s ",
    s->Printf("[%5u] %6u %c%c%c %-12s ",
              index,
              GetID(),
              m_is_debug ? 'D' : ' ',
              m_is_synthetic ? 'S' : ' ',
              m_is_external ? 'X' : ' ',
              GetTypeAsString());

    SectionSP section_sp (m_addr_range.GetBaseAddress().GetSection());
    if (section_sp)
    {
        if (!m_addr_range.GetBaseAddress().Dump(s, NULL, Address::DumpStyleFileAddress))
            s->Printf("%*s", 18, "");

        s->PutChar(' ');

        if (!m_addr_range.GetBaseAddress().Dump(s, target, Address::DumpStyleLoadAddress))
            s->Printf("%*s", 18, "");

        const char *format = m_size_is_sibling ?
                            " Sibling -> [%5llu] 0x%8.8x %s\n":
                            " 0x%16.16llx 0x%8.8x %s\n";
        s->Printf(  format,
                    m_addr_range.GetByteSize(),
                    m_flags,
                    m_mangled.GetName().AsCString(""));
    }
    else
    {
        const char *format = m_size_is_sibling ?
                            "0x%16.16llx                    Sibling -> [%5llu] 0x%8.8x %s\n":
                            "0x%16.16llx                    0x%16.16llx 0x%8.8x %s\n";
        s->Printf(  format,
                    m_addr_range.GetBaseAddress().GetOffset(),
                    m_addr_range.GetByteSize(),
                    m_flags,
                    m_mangled.GetName().AsCString(""));
    }
}

uint32_t
Symbol::GetPrologueByteSize ()
{
    if (m_type == eSymbolTypeCode)
    {
        if (!m_type_data_resolved)
        {
            m_type_data_resolved = true;
            ModuleSP module_sp (m_addr_range.GetBaseAddress().GetModule());
            SymbolContext sc;
            if (module_sp && module_sp->ResolveSymbolContextForAddress (m_addr_range.GetBaseAddress(),
                                                                        eSymbolContextLineEntry,
                                                                        sc))
            {
                m_type_data = sc.line_entry.range.GetByteSize();
            }
            else
            {
                // TODO: expose something in Process to figure out the
                // size of a function prologue.
            }
        }
        return m_type_data;
    }
    return 0;
}

void
Symbol::SetValue(addr_t value)
{
    m_addr_range.GetBaseAddress().SetRawAddress(value);
}


bool
Symbol::Compare(const ConstString& name, SymbolType type) const
{
    if (type == eSymbolTypeAny || m_type == type)
        return m_mangled.GetMangledName() == name || m_mangled.GetDemangledName() == name;
    return false;
}

#define ENUM_TO_CSTRING(x)  case eSymbolType##x: return #x;

const char *
Symbol::GetTypeAsString() const
{
    switch (m_type)
    {
    ENUM_TO_CSTRING(Invalid);
    ENUM_TO_CSTRING(Absolute);
    ENUM_TO_CSTRING(Code);
    ENUM_TO_CSTRING(Data);
    ENUM_TO_CSTRING(Trampoline);
    ENUM_TO_CSTRING(Runtime);
    ENUM_TO_CSTRING(Exception);
    ENUM_TO_CSTRING(SourceFile);
    ENUM_TO_CSTRING(HeaderFile);
    ENUM_TO_CSTRING(ObjectFile);
    ENUM_TO_CSTRING(CommonBlock);
    ENUM_TO_CSTRING(Block);
    ENUM_TO_CSTRING(Local);
    ENUM_TO_CSTRING(Param);
    ENUM_TO_CSTRING(Variable);
    ENUM_TO_CSTRING(VariableType);
    ENUM_TO_CSTRING(LineEntry);
    ENUM_TO_CSTRING(LineHeader);
    ENUM_TO_CSTRING(ScopeBegin);
    ENUM_TO_CSTRING(ScopeEnd);
    ENUM_TO_CSTRING(Additional);
    ENUM_TO_CSTRING(Compiler);
    ENUM_TO_CSTRING(Instrumentation);
    ENUM_TO_CSTRING(Undefined);
    ENUM_TO_CSTRING(ObjCClass);
    ENUM_TO_CSTRING(ObjCMetaClass);
    ENUM_TO_CSTRING(ObjCIVar);
    default:
        break;
    }
    return "<unknown SymbolType>";
}


void
Symbol::CalculateSymbolContext (SymbolContext *sc)
{
    // Symbols can reconstruct the symbol and the module in the symbol context
    sc->symbol = this;
    const AddressRange *range = GetAddressRangePtr();
    if (range)
        sc->module_sp = range->GetBaseAddress().GetModule();
    else
        sc->module_sp.reset();
}

ModuleSP
Symbol::CalculateSymbolContextModule ()
{
    const AddressRange *range = GetAddressRangePtr();
    if (range)
        return range->GetBaseAddress().GetModule();
    return ModuleSP();
}

Symbol *
Symbol::CalculateSymbolContextSymbol ()
{
    return this;
}


void
Symbol::DumpSymbolContext (Stream *s)
{
    bool dumped_module = false;
    const AddressRange *range = GetAddressRangePtr();
    if (range)
    {   
        ModuleSP module_sp (range->GetBaseAddress().GetModule());
        if (module_sp)
        {
            dumped_module = true;
            module_sp->DumpSymbolContext(s);
        }
    }
    if (dumped_module)
        s->PutCString(", ");
    
    s->Printf("Symbol{0x%8.8x}", GetID());
}


lldb::addr_t
Symbol::GetByteSize () const
{
    addr_t byte_size = m_addr_range.GetByteSize();
    if (byte_size == 0 && !m_calculated_size)
    {
        const_cast<Symbol*>(this)->m_calculated_size = true;
        const AddressRange *range = GetAddressRangePtr();
        if (range)
        {
            ModuleSP module_sp (range->GetBaseAddress().GetModule());
            if (module_sp)
            {
                ObjectFile *objfile = module_sp->GetObjectFile();
                if (objfile)
                {
                    Symtab *symtab = objfile->GetSymtab();
                    if (symtab)
                    {
                        const_cast<AddressRange &>(m_addr_range).SetByteSize(symtab->CalculateSymbolSize (const_cast<Symbol *>(this)));
                        byte_size = m_addr_range.GetByteSize();
                    }
                }
            }
        }
    }
    return byte_size;
}

