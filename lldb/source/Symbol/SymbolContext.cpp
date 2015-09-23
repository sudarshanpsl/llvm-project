//===-- SymbolContext.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Symbol/SymbolContext.h"

#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Host/Host.h"
#include "lldb/Host/StringConvert.h"
#include "lldb/Symbol/Block.h"
#include "lldb/Symbol/ClangASTContext.h"
#include "lldb/Symbol/CompileUnit.h"
#include "lldb/Symbol/ObjectFile.h"
#include "lldb/Symbol/Symbol.h"
#include "lldb/Symbol/SymbolFile.h"
#include "lldb/Symbol/SymbolVendor.h"
#include "lldb/Symbol/Variable.h"
#include "lldb/Target/Target.h"

using namespace lldb;
using namespace lldb_private;

SymbolContext::SymbolContext() :
    target_sp   (),
    module_sp   (),
    comp_unit   (nullptr),
    function    (nullptr),
    block       (nullptr),
    line_entry  (),
    symbol      (nullptr),
    variable    (nullptr)
{
}

SymbolContext::SymbolContext(const ModuleSP& m, CompileUnit *cu, Function *f, Block *b, LineEntry *le, Symbol *s) :
    target_sp   (),
    module_sp   (m),
    comp_unit   (cu),
    function    (f),
    block       (b),
    line_entry  (),
    symbol      (s),
    variable    (nullptr)
{
    if (le)
        line_entry = *le;
}

SymbolContext::SymbolContext(const TargetSP &t, const ModuleSP& m, CompileUnit *cu, Function *f, Block *b, LineEntry *le, Symbol *s) :
    target_sp   (t),
    module_sp   (m),
    comp_unit   (cu),
    function    (f),
    block       (b),
    line_entry  (),
    symbol      (s),
    variable    (nullptr)
{
    if (le)
        line_entry = *le;
}

SymbolContext::SymbolContext(const SymbolContext& rhs) :
    target_sp   (rhs.target_sp),
    module_sp   (rhs.module_sp),
    comp_unit   (rhs.comp_unit),
    function    (rhs.function),
    block       (rhs.block),
    line_entry  (rhs.line_entry),
    symbol      (rhs.symbol),
    variable    (rhs.variable)
{
}


SymbolContext::SymbolContext (SymbolContextScope *sc_scope) :
    target_sp   (),
    module_sp   (),
    comp_unit   (nullptr),
    function    (nullptr),
    block       (nullptr),
    line_entry  (),
    symbol      (nullptr),
    variable    (nullptr)
{
    sc_scope->CalculateSymbolContext (this);
}

SymbolContext::~SymbolContext ()
{
}

const SymbolContext&
SymbolContext::operator= (const SymbolContext& rhs)
{
    if (this != &rhs)
    {
        target_sp   = rhs.target_sp;
        module_sp   = rhs.module_sp;
        comp_unit   = rhs.comp_unit;
        function    = rhs.function;
        block       = rhs.block;
        line_entry  = rhs.line_entry;
        symbol      = rhs.symbol;
        variable    = rhs.variable;
    }
    return *this;
}

void
SymbolContext::Clear(bool clear_target)
{
    if (clear_target)
        target_sp.reset();
    module_sp.reset();
    comp_unit   = nullptr;
    function    = nullptr;
    block       = nullptr;
    line_entry.Clear();
    symbol      = nullptr;
    variable    = nullptr;
}

bool
SymbolContext::DumpStopContext (
    Stream *s,
    ExecutionContextScope *exe_scope,
    const Address &addr,
    bool show_fullpaths,
    bool show_module,
    bool show_inlined_frames,
    bool show_function_arguments,
    bool show_function_name
) const
{
    bool dumped_something = false;
    if (show_module && module_sp)
    {
        if (show_fullpaths)
            *s << module_sp->GetFileSpec();
        else
            *s << module_sp->GetFileSpec().GetFilename();
        s->PutChar('`');
        dumped_something = true;
    }

    if (function != nullptr)
    {
        SymbolContext inline_parent_sc;
        Address inline_parent_addr;
        if (show_function_name == false)
        {
            s->Printf("<");
            dumped_something = true;
        }
        else
        {
            ConstString name;
            if (show_function_arguments == false)
                name = function->GetNameNoArguments();
            if (!name)
                name = function->GetName();
            if (name)
                name.Dump(s);
        }
        
        if (addr.IsValid())
        {
            const addr_t function_offset = addr.GetOffset() - function->GetAddressRange().GetBaseAddress().GetOffset();
            if (show_function_name == false)
            {
                // Print +offset even if offset is 0
                dumped_something = true;
                s->Printf("+%" PRIu64 ">", function_offset);
            }
            else if (function_offset)
            {
                dumped_something = true;
                s->Printf(" + %" PRIu64, function_offset);
            }
        }

        if (GetParentOfInlinedScope (addr, inline_parent_sc, inline_parent_addr))
        {
            dumped_something = true;
            Block *inlined_block = block->GetContainingInlinedBlock();
            const InlineFunctionInfo* inlined_block_info = inlined_block->GetInlinedFunctionInfo();
            s->Printf (" [inlined] %s", inlined_block_info->GetName(function->GetLanguage()).GetCString());
            
            lldb_private::AddressRange block_range;
            if (inlined_block->GetRangeContainingAddress(addr, block_range))
            {
                const addr_t inlined_function_offset = addr.GetOffset() - block_range.GetBaseAddress().GetOffset();
                if (inlined_function_offset)
                {
                    s->Printf(" + %" PRIu64, inlined_function_offset);
                }
            }
            const Declaration &call_site = inlined_block_info->GetCallSite();
            if (call_site.IsValid())
            {
                s->PutCString(" at ");
                call_site.DumpStopContext (s, show_fullpaths);
            }
            if (show_inlined_frames)
            {
                s->EOL();
                s->Indent();
                const bool show_function_name = true;
                return inline_parent_sc.DumpStopContext (s, exe_scope, inline_parent_addr, show_fullpaths, show_module, show_inlined_frames, show_function_arguments, show_function_name);
            }
        }
        else
        {
            if (line_entry.IsValid())
            {
                dumped_something = true;
                s->PutCString(" at ");
                if (line_entry.DumpStopContext(s, show_fullpaths))
                    dumped_something = true;
            }
        }
    }
    else if (symbol != nullptr)
    {
        if (show_function_name == false)
        {
            s->Printf("<");
            dumped_something = true;
        }
        else if (symbol->GetName())
        {
            dumped_something = true;
            if (symbol->GetType() == eSymbolTypeTrampoline)
                s->PutCString("symbol stub for: ");
            symbol->GetName().Dump(s);
        }

        if (addr.IsValid() && symbol->ValueIsAddress())
        {
            const addr_t symbol_offset = addr.GetOffset() - symbol->GetAddressRef().GetOffset();
            if (show_function_name == false)
            {
                // Print +offset even if offset is 0
                dumped_something = true;
                s->Printf("+%" PRIu64 ">", symbol_offset);
            }
            else if (symbol_offset)
            {
                dumped_something = true;
                s->Printf(" + %" PRIu64, symbol_offset);
            }
        }
    }
    else if (addr.IsValid())
    {
        addr.Dump(s, exe_scope, Address::DumpStyleModuleWithFileAddress);
        dumped_something = true;
    }
    return dumped_something;
}

void
SymbolContext::GetDescription(Stream *s, lldb::DescriptionLevel level, Target *target) const
{
    if (module_sp)
    {
        s->Indent("     Module: file = \"");
        module_sp->GetFileSpec().Dump(s);
        *s << '"';
        if (module_sp->GetArchitecture().IsValid())
            s->Printf (", arch = \"%s\"", module_sp->GetArchitecture().GetArchitectureName());
        s->EOL();
    }

    if (comp_unit != nullptr)
    {
        s->Indent("CompileUnit: ");
        comp_unit->GetDescription (s, level);
        s->EOL();
    }

    if (function != nullptr)
    {
        s->Indent("   Function: ");
        function->GetDescription (s, level, target);
        s->EOL();

        Type *func_type = function->GetType();
        if (func_type)
        {
            s->Indent("   FuncType: ");
            func_type->GetDescription (s, level, false);
            s->EOL();
        }
    }

    if (block != nullptr)
    {
        std::vector<Block *> blocks;
        blocks.push_back (block);
        Block *parent_block = block->GetParent();
        
        while (parent_block)
        {
            blocks.push_back (parent_block);
            parent_block = parent_block->GetParent();
        }
        std::vector<Block *>::reverse_iterator pos;        
        std::vector<Block *>::reverse_iterator begin = blocks.rbegin();
        std::vector<Block *>::reverse_iterator end = blocks.rend();
        for (pos = begin; pos != end; ++pos)
        {
            if (pos == begin)
                s->Indent("     Blocks: ");
            else
                s->Indent("             ");
            (*pos)->GetDescription(s, function, level, target);
            s->EOL();
        }
    }

    if (line_entry.IsValid())
    {
        s->Indent("  LineEntry: ");
        line_entry.GetDescription (s, level, comp_unit, target, false);
        s->EOL();
    }

    if (symbol != nullptr)
    {
        s->Indent("     Symbol: ");
        symbol->GetDescription(s, level, target);
        s->EOL();
    }

    if (variable != nullptr)
    {
        s->Indent("   Variable: ");

        s->Printf("id = {0x%8.8" PRIx64 "}, ", variable->GetID());

        switch (variable->GetScope())
        {
            case eValueTypeVariableGlobal:
                s->PutCString("kind = global, ");
                break;

            case eValueTypeVariableStatic:
                s->PutCString("kind = static, ");
                break;

            case eValueTypeVariableArgument:
                s->PutCString("kind = argument, ");
                break;

            case eValueTypeVariableLocal:
                s->PutCString("kind = local, ");
                break;

            default:
                break;
        }

        s->Printf ("name = \"%s\"\n", variable->GetName().GetCString());
    }
}

uint32_t
SymbolContext::GetResolvedMask () const
{
    uint32_t resolved_mask = 0;
    if (target_sp)              resolved_mask |= eSymbolContextTarget;
    if (module_sp)              resolved_mask |= eSymbolContextModule;
    if (comp_unit)              resolved_mask |= eSymbolContextCompUnit;
    if (function)               resolved_mask |= eSymbolContextFunction;
    if (block)                  resolved_mask |= eSymbolContextBlock;
    if (line_entry.IsValid())   resolved_mask |= eSymbolContextLineEntry;
    if (symbol)                 resolved_mask |= eSymbolContextSymbol;
    if (variable)               resolved_mask |= eSymbolContextVariable;
    return resolved_mask;
}

void
SymbolContext::Dump(Stream *s, Target *target) const
{
    *s << this << ": ";
    s->Indent();
    s->PutCString("SymbolContext");
    s->IndentMore();
    s->EOL();
    s->IndentMore();
    s->Indent();
    *s << "Module       = " << module_sp.get() << ' ';
    if (module_sp)
        module_sp->GetFileSpec().Dump(s);
    s->EOL();
    s->Indent();
    *s << "CompileUnit  = " << comp_unit;
    if (comp_unit != nullptr)
        *s << " {0x" << comp_unit->GetID() << "} " << *(static_cast<FileSpec*> (comp_unit));
    s->EOL();
    s->Indent();
    *s << "Function     = " << function;
    if (function != nullptr)
    {
        *s << " {0x" << function->GetID() << "} " << function->GetType()->GetName() << ", address-range = ";
        function->GetAddressRange().Dump(s, target, Address::DumpStyleLoadAddress, Address::DumpStyleModuleWithFileAddress);
        s->EOL();
        s->Indent();
        Type* func_type = function->GetType();
        if (func_type)
        {
            *s << "        Type = ";
            func_type->Dump (s, false);
        }
    }
    s->EOL();
    s->Indent();
    *s << "Block        = " << block;
    if (block != nullptr)
        *s << " {0x" << block->GetID() << '}';
    // Dump the block and pass it a negative depth to we print all the parent blocks
    //if (block != NULL)
    //  block->Dump(s, function->GetFileAddress(), INT_MIN);
    s->EOL();
    s->Indent();
    *s << "LineEntry    = ";
    line_entry.Dump (s, target, true, Address::DumpStyleLoadAddress, Address::DumpStyleModuleWithFileAddress, true);
    s->EOL();
    s->Indent();
    *s << "Symbol       = " << symbol;
    if (symbol != nullptr && symbol->GetMangled())
        *s << ' ' << symbol->GetName().AsCString();
    s->EOL();
    *s << "Variable     = " << variable;
    if (variable != nullptr)
    {
        *s << " {0x" << variable->GetID() << "} " << variable->GetType()->GetName();
        s->EOL();
    }
    s->IndentLess();
    s->IndentLess();
}

bool
lldb_private::operator== (const SymbolContext& lhs, const SymbolContext& rhs)
{
    return  lhs.function == rhs.function
            && lhs.symbol == rhs.symbol 
            && lhs.module_sp.get() == rhs.module_sp.get()
            && lhs.comp_unit == rhs.comp_unit
            && lhs.target_sp.get() == rhs.target_sp.get() 
            && LineEntry::Compare(lhs.line_entry, rhs.line_entry) == 0
            && lhs.variable == rhs.variable;
}

bool
lldb_private::operator!= (const SymbolContext& lhs, const SymbolContext& rhs)
{
    return  lhs.function != rhs.function
            || lhs.symbol != rhs.symbol 
            || lhs.module_sp.get() != rhs.module_sp.get()
            || lhs.comp_unit != rhs.comp_unit
            || lhs.target_sp.get() != rhs.target_sp.get() 
            || LineEntry::Compare(lhs.line_entry, rhs.line_entry) != 0
            || lhs.variable != rhs.variable;
}

bool
SymbolContext::GetAddressRange (uint32_t scope, 
                                uint32_t range_idx, 
                                bool use_inline_block_range,
                                AddressRange &range) const
{
    if ((scope & eSymbolContextLineEntry) && line_entry.IsValid())
    {
        range = line_entry.range;
        return true;
    }
    
    if ((scope & eSymbolContextBlock) && (block != nullptr))
    {
        if (use_inline_block_range)
        {
            Block *inline_block = block->GetContainingInlinedBlock();
            if (inline_block)
                return inline_block->GetRangeAtIndex (range_idx, range);
        }
        else
        {
            return block->GetRangeAtIndex (range_idx, range);
        }
    }

    if ((scope & eSymbolContextFunction) && (function != nullptr))
    {
        if (range_idx == 0)
        {
            range = function->GetAddressRange();
            return true;
        }            
    } 
    
    if ((scope & eSymbolContextSymbol) && (symbol != nullptr))
    {
        if (range_idx == 0)
        {
            if (symbol->ValueIsAddress())
            {
                range.GetBaseAddress() = symbol->GetAddressRef();
                range.SetByteSize (symbol->GetByteSize());
                return true;
            }
        }
    }
    range.Clear();
    return false;
}

bool
SymbolContext::GetParentOfInlinedScope (const Address &curr_frame_pc, 
                                        SymbolContext &next_frame_sc, 
                                        Address &next_frame_pc) const
{
    next_frame_sc.Clear(false);
    next_frame_pc.Clear();

    if (block)
    {
        //const addr_t curr_frame_file_addr = curr_frame_pc.GetFileAddress();
        
        // In order to get the parent of an inlined function we first need to
        // see if we are in an inlined block as "this->block" could be an 
        // inlined block, or a parent of "block" could be. So lets check if
        // this block or one of this blocks parents is an inlined function.
        Block *curr_inlined_block = block->GetContainingInlinedBlock();
        if (curr_inlined_block)
        {
            // "this->block" is contained in an inline function block, so to
            // get the scope above the inlined block, we get the parent of the
            // inlined block itself
            Block *next_frame_block = curr_inlined_block->GetParent();
            // Now calculate the symbol context of the containing block
            next_frame_block->CalculateSymbolContext (&next_frame_sc);
            
            // If we get here we weren't able to find the return line entry using the nesting of the blocks and
            // the line table.  So just use the call site info from our inlined block.
            
            AddressRange range;
            if (curr_inlined_block->GetRangeContainingAddress (curr_frame_pc, range))
            {
                // To see there this new frame block it, we need to look at the
                // call site information from 
                const InlineFunctionInfo* curr_inlined_block_inlined_info = curr_inlined_block->GetInlinedFunctionInfo();
                next_frame_pc = range.GetBaseAddress();
                next_frame_sc.line_entry.range.GetBaseAddress() = next_frame_pc;
                next_frame_sc.line_entry.file = curr_inlined_block_inlined_info->GetCallSite().GetFile();
                next_frame_sc.line_entry.line = curr_inlined_block_inlined_info->GetCallSite().GetLine();
                next_frame_sc.line_entry.column = curr_inlined_block_inlined_info->GetCallSite().GetColumn();
                return true;
            }
            else
            {
                Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_SYMBOLS));

                if (log)
                {
                    log->Printf ("warning: inlined block 0x%8.8" PRIx64 " doesn't have a range that contains file address 0x%" PRIx64,
                                 curr_inlined_block->GetID(), curr_frame_pc.GetFileAddress());
                }
#ifdef LLDB_CONFIGURATION_DEBUG
                else
                {
                    ObjectFile *objfile = NULL;
                    if (module_sp)
                    {
                        SymbolVendor *symbol_vendor = module_sp->GetSymbolVendor();
                        if (symbol_vendor)
                        {
                            SymbolFile *symbol_file = symbol_vendor->GetSymbolFile();
                            if (symbol_file)
                                objfile = symbol_file->GetObjectFile();
                        }
                    }
                    if (objfile)
                    {
                        Host::SystemLog (Host::eSystemLogWarning, 
                                         "warning: inlined block 0x%8.8" PRIx64 " doesn't have a range that contains file address 0x%" PRIx64 " in %s\n",
                                         curr_inlined_block->GetID(), 
                                         curr_frame_pc.GetFileAddress(),
                                         objfile->GetFileSpec().GetPath().c_str());
                    }
                    else
                    {
                        Host::SystemLog (Host::eSystemLogWarning, 
                                         "warning: inlined block 0x%8.8" PRIx64 " doesn't have a range that contains file address 0x%" PRIx64 "\n",
                                         curr_inlined_block->GetID(), 
                                         curr_frame_pc.GetFileAddress());
                    }
                }
#endif
            }
        }
    }
    
    return false;
}

Block *
SymbolContext::GetFunctionBlock ()
{
    if (function)
    {
        if (block)
        {
            // If this symbol context has a block, check to see if this block
            // is itself, or is contained within a block with inlined function
            // information. If so, then the inlined block is the block that
            // defines the function.
            Block *inlined_block = block->GetContainingInlinedBlock();
            if (inlined_block)
                return inlined_block;

            // The block in this symbol context is not inside an inlined
            // block, so the block that defines the function is the function's
            // top level block, which is returned below.
        }

        // There is no block information in this symbol context, so we must
        // assume that the block that is desired is the top level block of
        // the function itself.
        return &function->GetBlock(true);
    }
    return nullptr;
}

bool
SymbolContext::GetFunctionMethodInfo (lldb::LanguageType &language,
                                      bool &is_instance_method,
                                      ConstString &language_object_name)


{
    Block *function_block = GetFunctionBlock();
    if (function_block)
    {
        CompilerDeclContext decl_ctx = function_block->GetDeclContext();
        if (decl_ctx)
            return decl_ctx.IsClassMethod(&language, &is_instance_method, &language_object_name);
    }
    return false;
}

class TypeMoveMatchingBlock
{
public:
	TypeMoveMatchingBlock(Block * block, TypeMap &typem, TypeList &typel) :
		curr_block(block),type_map(typem),type_list(typel)
    {
    }

    bool
    operator() (const lldb::TypeSP& type)
    {
    	if (type && type->GetSymbolContextScope() != nullptr && curr_block == type->GetSymbolContextScope()->CalculateSymbolContextBlock())
    	{
    	   	type_list.Insert(type);
    		type_map.RemoveTypeWithUID(type->GetID());
    		return false;
    	}
        return true;
    }

private:
    const Block * const curr_block;
    TypeMap &type_map;
    TypeList &type_list;
};

class TypeMoveMatchingFunction
{
public:
	TypeMoveMatchingFunction(Function * block, TypeMap &typem, TypeList &typel) :
		func(block),type_map(typem),type_list(typel)
    {
    }

    bool
    operator() (const lldb::TypeSP& type)
    {
    	if (type && type->GetSymbolContextScope() != nullptr && func == type->GetSymbolContextScope()->CalculateSymbolContextFunction())
    	{
    	    type_list.Insert(type);
    	    type_map.RemoveTypeWithUID(type->GetID());
    	    return false;
    	}
        return true;
    }

private:
    const Function * const func;
    TypeMap &type_map;
    TypeList &type_list;
};

class TypeMoveMatchingCompileUnit
{
public:
	TypeMoveMatchingCompileUnit(CompileUnit * cunit, TypeMap &typem, TypeList &typel) :
		comp_unit(cunit),type_map(typem),type_list(typel)
    {
    }

    bool
    operator() (const lldb::TypeSP& type)
    {
    	if (type && type->GetSymbolContextScope() != nullptr && comp_unit == type->GetSymbolContextScope()->CalculateSymbolContextCompileUnit())
    	{
    	    type_list.Insert(type);
    	    type_map.RemoveTypeWithUID(type->GetID());
    	    return false;
    	}
    	return true;
    }

private:
    const CompileUnit * const comp_unit;
    TypeMap &type_map;
    TypeList &type_list;
};

class TypeMoveMatchingModule
{
public:
	TypeMoveMatchingModule(lldb::ModuleSP modsp, TypeMap &typem, TypeList &typel) :
		modulesp(modsp),type_map(typem),type_list(typel)
    {
    }

    bool
    operator() (const lldb::TypeSP& type)
    {
    	if (type && type->GetSymbolContextScope() != nullptr && modulesp.get() == type->GetSymbolContextScope()->CalculateSymbolContextModule().get())
    	{
    	    type_list.Insert(type);
    	    type_map.RemoveTypeWithUID(type->GetID());
    	    return false;
    	}
        return true;
    }

private:
    lldb::ModuleSP modulesp;
    TypeMap &type_map;
    TypeList &type_list;
};

class TypeMaptoList
{
public:
	TypeMaptoList(TypeMap &typem, TypeList &typel) :
		type_map(typem),type_list(typel)
    {
    }

    bool
    operator() (const lldb::TypeSP& type)
    {
    	if(type)
    	{
            type_list.Insert(type);
            type_map.RemoveTypeWithUID(type->GetID());
    	}
        return true;
    }

private:
    TypeMap &type_map;
    TypeList &type_list;
};

void
SymbolContext::SortTypeList(TypeMap &type_map, TypeList &type_list ) const
{
	Block * curr_block = block;
    bool isInlinedblock = false;
    if(curr_block != nullptr && curr_block->GetContainingInlinedBlock() != nullptr)
    	isInlinedblock = true;

	while (curr_block != nullptr && !isInlinedblock)
    {
		TypeMoveMatchingBlock callbackBlock (curr_block, type_map, type_list);
		type_map.ForEach(callbackBlock);
	    curr_block = curr_block->GetParent();
	}
    if(function != nullptr && type_map.GetSize() > 0)
    {
    	TypeMoveMatchingFunction callbackFunction (function, type_map, type_list);
    	type_map.ForEach(callbackFunction);
    }
    if(comp_unit != nullptr && type_map.GetSize() > 0)
    {
    	TypeMoveMatchingCompileUnit callbackCompileUnit (comp_unit, type_map, type_list);
    	type_map.ForEach(callbackCompileUnit);
    }
    if(module_sp && type_map.GetSize() > 0)
    {
    	TypeMoveMatchingModule callbackModule (module_sp, type_map, type_list);
    	type_map.ForEach(callbackModule);
    }
    if(type_map.GetSize() > 0)
    {
    	TypeMaptoList callbackM2L (type_map, type_list);
    	type_map.ForEach(callbackM2L);
    }
	return ;
}

ConstString
SymbolContext::GetFunctionName (Mangled::NamePreference preference) const
{
    if (function)
    {
        if (block)
        {
            Block *inlined_block = block->GetContainingInlinedBlock();
            
            if (inlined_block)
            {
                const InlineFunctionInfo *inline_info = inlined_block->GetInlinedFunctionInfo();
                if (inline_info)
                    return inline_info->GetName(function->GetLanguage());
            }
        }
        return function->GetMangled().GetName(function->GetLanguage(), preference);
    }
    else if (symbol && symbol->ValueIsAddress())
    {
        return symbol->GetMangled().GetName(symbol->GetLanguage(), preference);
    }
    else
    {
        // No function, return an empty string.
        return ConstString();
    }
}

LineEntry
SymbolContext::GetFunctionStartLineEntry () const
{
    LineEntry line_entry;
    Address start_addr;
    if (block)
    {
        Block *inlined_block = block->GetContainingInlinedBlock();
        if (inlined_block)
        {
            if (inlined_block->GetStartAddress (start_addr))
            {
                if (start_addr.CalculateSymbolContextLineEntry (line_entry))
                    return line_entry;
            }
            return LineEntry();
        }
    }
    
    if (function)
    {
        if (function->GetAddressRange().GetBaseAddress().CalculateSymbolContextLineEntry(line_entry))
            return line_entry;
    }
    return LineEntry();
}

//----------------------------------------------------------------------
//
//  SymbolContextSpecifier
//
//----------------------------------------------------------------------

SymbolContextSpecifier::SymbolContextSpecifier (const TargetSP &target_sp) :
    m_target_sp (target_sp),
    m_module_spec (),
    m_module_sp (),
    m_file_spec_ap (),
    m_start_line (0),
    m_end_line (0),
    m_function_spec (),
    m_class_name (),
    m_address_range_ap (),
    m_type (eNothingSpecified)
{
}   

SymbolContextSpecifier::~SymbolContextSpecifier()
{
}

bool
SymbolContextSpecifier::AddLineSpecification (uint32_t line_no, SpecificationType type)
{
    bool return_value = true;
    switch (type)
    {
    case eNothingSpecified:
        Clear();
        break;
    case eLineStartSpecified:
        m_start_line = line_no;
        m_type |= eLineStartSpecified;
        break;
    case eLineEndSpecified:
        m_end_line = line_no;
        m_type |= eLineEndSpecified;
        break;
    default:
        return_value = false;
        break;
    }
    return return_value;
}

bool
SymbolContextSpecifier::AddSpecification (const char *spec_string, SpecificationType type)
{
    bool return_value = true;
    switch (type)
    {
    case eNothingSpecified:
        Clear();
        break;
    case eModuleSpecified:
        {
            // See if we can find the Module, if so stick it in the SymbolContext.
            FileSpec module_file_spec(spec_string, false);
            ModuleSpec module_spec (module_file_spec);
            lldb::ModuleSP module_sp (m_target_sp->GetImages().FindFirstModule (module_spec));
            m_type |= eModuleSpecified;
            if (module_sp)
                m_module_sp = module_sp;
            else
                m_module_spec.assign (spec_string);
        }
        break;
    case eFileSpecified:
        // CompUnits can't necessarily be resolved here, since an inlined function might show up in 
        // a number of CompUnits.  Instead we just convert to a FileSpec and store it away.
        m_file_spec_ap.reset (new FileSpec (spec_string, false));
        m_type |= eFileSpecified;
        break;
    case eLineStartSpecified:
        m_start_line = StringConvert::ToSInt32(spec_string, 0, 0, &return_value);
        if (return_value)
            m_type |= eLineStartSpecified;
        break;
    case eLineEndSpecified:
        m_end_line = StringConvert::ToSInt32(spec_string, 0, 0, &return_value);
        if (return_value)
            m_type |= eLineEndSpecified;
        break;
    case eFunctionSpecified:
        m_function_spec.assign(spec_string);
        m_type |= eFunctionSpecified;
        break;
    case eClassOrNamespaceSpecified:
        Clear();
        m_class_name.assign (spec_string);
        m_type = eClassOrNamespaceSpecified;
        break;
    case eAddressRangeSpecified:
        // Not specified yet...
        break;
    }
    
    return return_value;
}

void
SymbolContextSpecifier::Clear()
{
    m_module_spec.clear();
    m_file_spec_ap.reset();
    m_function_spec.clear();
    m_class_name.clear();
    m_start_line = 0;
    m_end_line = 0;
    m_address_range_ap.reset();
    
    m_type = eNothingSpecified;
}

bool
SymbolContextSpecifier::SymbolContextMatches(SymbolContext &sc)
{
    if (m_type == eNothingSpecified)
        return true;
        
    if (m_target_sp.get() != sc.target_sp.get())
        return false;
        
    if (m_type & eModuleSpecified)
    {
        if (sc.module_sp)
        {
            if (m_module_sp.get() != nullptr)
            { 
                if (m_module_sp.get() != sc.module_sp.get())
                    return false;
            }
            else
            {
                FileSpec module_file_spec (m_module_spec.c_str(), false);
                if (!FileSpec::Equal (module_file_spec, sc.module_sp->GetFileSpec(), false))
                    return false;
            }
        }
    }
    if (m_type & eFileSpecified)
    {
        if (m_file_spec_ap.get())
        {
            // If we don't have a block or a comp_unit, then we aren't going to match a source file.
            if (sc.block == nullptr && sc.comp_unit == nullptr)
                return false;
                
            // Check if the block is present, and if so is it inlined:
            bool was_inlined = false;
            if (sc.block != nullptr)
            {
                const InlineFunctionInfo *inline_info = sc.block->GetInlinedFunctionInfo();
                if (inline_info != nullptr)
                {
                    was_inlined = true;
                    if (!FileSpec::Equal (inline_info->GetDeclaration().GetFile(), *(m_file_spec_ap.get()), false))
                        return false;
                }
            }
            
            // Next check the comp unit, but only if the SymbolContext was not inlined.
            if (!was_inlined && sc.comp_unit != nullptr)
            {
                if (!FileSpec::Equal (*(sc.comp_unit), *(m_file_spec_ap.get()), false))
                    return false;
            }
        }
    }
    if (m_type & eLineStartSpecified 
        || m_type & eLineEndSpecified)
    {
        if (sc.line_entry.line < m_start_line || sc.line_entry.line > m_end_line)
            return false;
    }
    
    if (m_type & eFunctionSpecified)
    {
        // First check the current block, and if it is inlined, get the inlined function name:
        bool was_inlined = false;
        ConstString func_name(m_function_spec.c_str());
        
        if (sc.block != nullptr)
        {
            const InlineFunctionInfo *inline_info = sc.block->GetInlinedFunctionInfo();
            if (inline_info != nullptr)
            {
                was_inlined = true;
                const Mangled &name = inline_info->GetMangled();
                if (!name.NameMatches (func_name, sc.function->GetLanguage()))
                    return false;
            }
        }
        //  If it wasn't inlined, check the name in the function or symbol:
        if (!was_inlined)
        {
            if (sc.function != nullptr)
            {
                if (!sc.function->GetMangled().NameMatches(func_name, sc.function->GetLanguage()))
                    return false;
            }
            else if (sc.symbol != nullptr)
            {
                if (!sc.symbol->GetMangled().NameMatches(func_name, sc.function->GetLanguage()))
                    return false;
            }
        }
        
            
    }
    
    return true;
}

bool
SymbolContextSpecifier::AddressMatches(lldb::addr_t addr)
{
    if (m_type & eAddressRangeSpecified)
    {
    
    }
    else
    {
        Address match_address (addr, nullptr);
        SymbolContext sc;
        m_target_sp->GetImages().ResolveSymbolContextForAddress(match_address, eSymbolContextEverything, sc);
        return SymbolContextMatches(sc);
    }
    return true;
}

void
SymbolContextSpecifier::GetDescription (Stream *s, lldb::DescriptionLevel level) const
{
    char path_str[PATH_MAX + 1];

    if (m_type == eNothingSpecified)
    {
        s->Printf ("Nothing specified.\n");
    }
    
    if (m_type == eModuleSpecified)
    {
        s->Indent();
        if (m_module_sp)
        {
            m_module_sp->GetFileSpec().GetPath (path_str, PATH_MAX);
            s->Printf ("Module: %s\n", path_str);
        }
        else
            s->Printf ("Module: %s\n", m_module_spec.c_str());
    }
    
    if (m_type == eFileSpecified  && m_file_spec_ap.get() != nullptr)
    {
        m_file_spec_ap->GetPath (path_str, PATH_MAX);
        s->Indent();
        s->Printf ("File: %s", path_str);
        if (m_type == eLineStartSpecified)
        {
            s->Printf (" from line %" PRIu64 "", (uint64_t)m_start_line);
            if (m_type == eLineEndSpecified)
                s->Printf ("to line %" PRIu64 "", (uint64_t)m_end_line);
            else
                s->Printf ("to end");
        }
        else if (m_type == eLineEndSpecified)
        {
            s->Printf (" from start to line %" PRIu64 "", (uint64_t)m_end_line);
        }
        s->Printf (".\n");
    }
    
    if (m_type == eLineStartSpecified)
    {
        s->Indent();
        s->Printf ("From line %" PRIu64 "", (uint64_t)m_start_line);
        if (m_type == eLineEndSpecified)
            s->Printf ("to line %" PRIu64 "", (uint64_t)m_end_line);
        else
            s->Printf ("to end");
        s->Printf (".\n");
    }
    else if (m_type == eLineEndSpecified)
    {
        s->Printf ("From start to line %" PRIu64 ".\n", (uint64_t)m_end_line);
    }
    
    if (m_type == eFunctionSpecified)
    {
        s->Indent();
        s->Printf ("Function: %s.\n", m_function_spec.c_str());
    }
    
    if (m_type == eClassOrNamespaceSpecified)
    {
        s->Indent();
        s->Printf ("Class name: %s.\n", m_class_name.c_str());
    }
    
    if (m_type == eAddressRangeSpecified && m_address_range_ap.get() != nullptr)
    {
        s->Indent();
        s->PutCString ("Address range: ");
        m_address_range_ap->Dump (s, m_target_sp.get(), Address::DumpStyleLoadAddress, Address::DumpStyleFileAddress);
        s->PutCString ("\n");
    }
}

//----------------------------------------------------------------------
//
//  SymbolContextList
//
//----------------------------------------------------------------------


SymbolContextList::SymbolContextList() :
    m_symbol_contexts()
{
}

SymbolContextList::~SymbolContextList()
{
}

void
SymbolContextList::Append(const SymbolContext& sc)
{
    m_symbol_contexts.push_back(sc);
}

void
SymbolContextList::Append (const SymbolContextList& sc_list)
{
    collection::const_iterator pos, end = sc_list.m_symbol_contexts.end();
    for (pos = sc_list.m_symbol_contexts.begin(); pos != end; ++pos)
        m_symbol_contexts.push_back (*pos);
}


uint32_t
SymbolContextList::AppendIfUnique (const SymbolContextList& sc_list, bool merge_symbol_into_function)
{
    uint32_t unique_sc_add_count = 0;
    collection::const_iterator pos, end = sc_list.m_symbol_contexts.end();
    for (pos = sc_list.m_symbol_contexts.begin(); pos != end; ++pos)
    {
        if (AppendIfUnique (*pos, merge_symbol_into_function))
            ++unique_sc_add_count;
    }
    return unique_sc_add_count;
}

bool
SymbolContextList::AppendIfUnique (const SymbolContext& sc, bool merge_symbol_into_function)
{
    collection::iterator pos, end = m_symbol_contexts.end();
    for (pos = m_symbol_contexts.begin(); pos != end; ++pos)
    {
        if (*pos == sc)
            return false;
    }
    if (merge_symbol_into_function 
        && sc.symbol    != nullptr
        && sc.comp_unit == nullptr
        && sc.function  == nullptr
        && sc.block     == nullptr
        && sc.line_entry.IsValid() == false)
    {
        if (sc.symbol->ValueIsAddress())
        {
            for (pos = m_symbol_contexts.begin(); pos != end; ++pos)
            {
                // Don't merge symbols into inlined function symbol contexts
                if (pos->block && pos->block->GetContainingInlinedBlock())
                    continue;

                if (pos->function)
                {
                    if (pos->function->GetAddressRange().GetBaseAddress() == sc.symbol->GetAddressRef())
                    {
                        // Do we already have a function with this symbol?
                        if (pos->symbol == sc.symbol)
                            return false;
                        if (pos->symbol == nullptr)
                        {
                            pos->symbol = sc.symbol;
                            return false;
                        }
                    }
                }
            }
        }
    }
    m_symbol_contexts.push_back(sc);
    return true;
}

bool
SymbolContextList::MergeSymbolContextIntoFunctionContext (const SymbolContext& symbol_sc,
                                                          uint32_t start_idx,
                                                          uint32_t stop_idx)
{
    if (symbol_sc.symbol    != nullptr
        && symbol_sc.comp_unit == nullptr
        && symbol_sc.function  == nullptr
        && symbol_sc.block     == nullptr
        && symbol_sc.line_entry.IsValid() == false)
    {
        if (symbol_sc.symbol->ValueIsAddress())
        {
            const size_t end = std::min<size_t>(m_symbol_contexts.size(), stop_idx);
            for (size_t i=start_idx; i<end; ++i)
            {
                const SymbolContext &function_sc = m_symbol_contexts[i];
                // Don't merge symbols into inlined function symbol contexts
                if (function_sc.block && function_sc.block->GetContainingInlinedBlock())
                    continue;
                
                if (function_sc.function)
                {
                    if (function_sc.function->GetAddressRange().GetBaseAddress() == symbol_sc.symbol->GetAddressRef())
                    {
                        // Do we already have a function with this symbol?
                        if (function_sc.symbol == symbol_sc.symbol)
                            return true; // Already have a symbol context with this symbol, return true

                        if (function_sc.symbol == nullptr)
                        {
                            // We successfully merged this symbol into an existing symbol context
                            m_symbol_contexts[i].symbol = symbol_sc.symbol;
                            return true;
                        }
                    }
                }
            }
        }
    }
    return false;
}

void
SymbolContextList::Clear()
{
    m_symbol_contexts.clear();
}

void
SymbolContextList::Dump(Stream *s, Target *target) const
{

    *s << this << ": ";
    s->Indent();
    s->PutCString("SymbolContextList");
    s->EOL();
    s->IndentMore();

    collection::const_iterator pos, end = m_symbol_contexts.end();
    for (pos = m_symbol_contexts.begin(); pos != end; ++pos)
    {
        //pos->Dump(s, target);
        pos->GetDescription(s, eDescriptionLevelVerbose, target);
    }
    s->IndentLess();
}

bool
SymbolContextList::GetContextAtIndex(size_t idx, SymbolContext& sc) const
{
    if (idx < m_symbol_contexts.size())
    {
        sc = m_symbol_contexts[idx];
        return true;
    }
    return false;
}

bool
SymbolContextList::GetLastContext(SymbolContext& sc) const
{
    if (!m_symbol_contexts.empty())
    {
        sc = m_symbol_contexts.back();
        return true;
    }
    return false;
}

bool
SymbolContextList::RemoveContextAtIndex (size_t idx)
{
    if (idx < m_symbol_contexts.size())
    {
        m_symbol_contexts.erase(m_symbol_contexts.begin() + idx);
        return true;
    }
    return false;
}

uint32_t
SymbolContextList::GetSize() const
{
    return m_symbol_contexts.size();
}

uint32_t
SymbolContextList::NumLineEntriesWithLine (uint32_t line) const
{
    uint32_t match_count = 0;
    const size_t size = m_symbol_contexts.size();
    for (size_t idx = 0; idx<size; ++idx)
    {
        if (m_symbol_contexts[idx].line_entry.line == line)
            ++match_count;
    }
    return match_count;
}

void
SymbolContextList::GetDescription(Stream *s, 
                                  lldb::DescriptionLevel level, 
                                  Target *target) const
{
    const size_t size = m_symbol_contexts.size();
    for (size_t idx = 0; idx<size; ++idx)
        m_symbol_contexts[idx].GetDescription (s, level, target);
}

bool
lldb_private::operator== (const SymbolContextList& lhs, const SymbolContextList& rhs)
{
    const uint32_t size = lhs.GetSize();
    if (size != rhs.GetSize())
        return false;
    
    SymbolContext lhs_sc;
    SymbolContext rhs_sc;
    for (uint32_t i=0; i<size; ++i)
    {
        lhs.GetContextAtIndex(i, lhs_sc);
        rhs.GetContextAtIndex(i, rhs_sc);
        if (lhs_sc != rhs_sc)
            return false;
    }
    return true;
}

bool
lldb_private::operator!= (const SymbolContextList& lhs, const SymbolContextList& rhs)
{
    return !(lhs == rhs);
}

