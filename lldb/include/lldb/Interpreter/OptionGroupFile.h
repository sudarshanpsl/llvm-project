//===-- OptionGroupFile.h -------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#ifndef liblldb_OptionGroupFile_h_
#define liblldb_OptionGroupFile_h_

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Interpreter/Options.h"
#include "lldb/Interpreter/NamedOptionValue.h"

namespace lldb_private {
//-------------------------------------------------------------------------
// OptionGroupFile
//-------------------------------------------------------------------------

class OptionGroupFile : public OptionGroup
{
public:
    
    OptionGroupFile (uint32_t usage_mask,
                     bool required,
                     const char *long_option, 
                     char short_option,
                     uint32_t completion_type,
                     lldb::CommandArgumentType argument_type,
                     const char *usage_text);
    
    virtual
    ~OptionGroupFile ();

    
    virtual uint32_t
    GetNumDefinitions ()
    {
        return 1;
    }
    
    virtual const OptionDefinition*
    GetDefinitions ()
    {
        return &m_option_definition;
    }
    
    virtual Error
    SetOptionValue (CommandInterpreter &interpreter,
                    uint32_t option_idx,
                    const char *option_value);
    
    virtual void
    OptionParsingStarting (CommandInterpreter &interpreter);
    
    OptionValueFileSpec &
    GetOptionValue ()
    {
        return m_file;
    }

    const OptionValueFileSpec &
    GetOptionValue () const
    {
        return m_file;
    }

protected:
    OptionValueFileSpec m_file;
    OptionDefinition m_option_definition;
    
};

} // namespace lldb_private

#endif  // liblldb_OptionGroupFile_h_
