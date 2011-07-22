//===-- RegisterContextDarwin_x86_64.cpp ------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//


// C Includes
#include <stdarg.h>

// C++ Includes
// Other libraries and framework includes
#include "lldb/Core/DataBufferHeap.h"
#include "lldb/Core/DataExtractor.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/RegisterValue.h"
#include "lldb/Core/Scalar.h"
#include "lldb/Host/Endian.h"

// Project includes
#include "RegisterContextDarwin_x86_64.h"

using namespace lldb;
using namespace lldb_private;

enum
{
    gpr_rax = 0,
    gpr_rbx,
    gpr_rcx,
    gpr_rdx,
    gpr_rdi,
    gpr_rsi,
    gpr_rbp,
    gpr_rsp,
    gpr_r8,
    gpr_r9,
    gpr_r10,
    gpr_r11,
    gpr_r12,
    gpr_r13,
    gpr_r14,
    gpr_r15,
    gpr_rip,
    gpr_rflags,
    gpr_cs,
    gpr_fs,
    gpr_gs,

    fpu_fcw,
    fpu_fsw,
    fpu_ftw,
    fpu_fop,
    fpu_ip,
    fpu_cs,
    fpu_dp,
    fpu_ds,
    fpu_mxcsr,
    fpu_mxcsrmask,
    fpu_stmm0,
    fpu_stmm1,
    fpu_stmm2,
    fpu_stmm3,
    fpu_stmm4,
    fpu_stmm5,
    fpu_stmm6,
    fpu_stmm7,
    fpu_xmm0,
    fpu_xmm1,
    fpu_xmm2,
    fpu_xmm3,
    fpu_xmm4,
    fpu_xmm5,
    fpu_xmm6,
    fpu_xmm7,
    fpu_xmm8,
    fpu_xmm9,
    fpu_xmm10,
    fpu_xmm11,
    fpu_xmm12,
    fpu_xmm13,
    fpu_xmm14,
    fpu_xmm15,

    exc_trapno,
    exc_err,
    exc_faultvaddr,

    k_num_registers,

    // Aliases
    fpu_fctrl = fpu_fcw,
    fpu_fstat = fpu_fsw,
    fpu_ftag  = fpu_ftw,
    fpu_fiseg = fpu_cs,
    fpu_fioff = fpu_ip,
    fpu_foseg = fpu_ds,
    fpu_fooff = fpu_dp,
};

enum gcc_dwarf_regnums
{
    gcc_dwarf_gpr_rax = 0,
    gcc_dwarf_gpr_rdx,
    gcc_dwarf_gpr_rcx,
    gcc_dwarf_gpr_rbx,
    gcc_dwarf_gpr_rsi,
    gcc_dwarf_gpr_rdi,
    gcc_dwarf_gpr_rbp,
    gcc_dwarf_gpr_rsp,
    gcc_dwarf_gpr_r8,
    gcc_dwarf_gpr_r9,
    gcc_dwarf_gpr_r10,
    gcc_dwarf_gpr_r11,
    gcc_dwarf_gpr_r12,
    gcc_dwarf_gpr_r13,
    gcc_dwarf_gpr_r14,
    gcc_dwarf_gpr_r15,
    gcc_dwarf_gpr_rip,
    gcc_dwarf_fpu_xmm0,
    gcc_dwarf_fpu_xmm1,
    gcc_dwarf_fpu_xmm2,
    gcc_dwarf_fpu_xmm3,
    gcc_dwarf_fpu_xmm4,
    gcc_dwarf_fpu_xmm5,
    gcc_dwarf_fpu_xmm6,
    gcc_dwarf_fpu_xmm7,
    gcc_dwarf_fpu_xmm8,
    gcc_dwarf_fpu_xmm9,
    gcc_dwarf_fpu_xmm10,
    gcc_dwarf_fpu_xmm11,
    gcc_dwarf_fpu_xmm12,
    gcc_dwarf_fpu_xmm13,
    gcc_dwarf_fpu_xmm14,
    gcc_dwarf_fpu_xmm15,
    gcc_dwarf_fpu_stmm0,
    gcc_dwarf_fpu_stmm1,
    gcc_dwarf_fpu_stmm2,
    gcc_dwarf_fpu_stmm3,
    gcc_dwarf_fpu_stmm4,
    gcc_dwarf_fpu_stmm5,
    gcc_dwarf_fpu_stmm6,
    gcc_dwarf_fpu_stmm7,

};

enum gdb_regnums
{
    gdb_gpr_rax     =   0,
    gdb_gpr_rbx     =   1,
    gdb_gpr_rcx     =   2,
    gdb_gpr_rdx     =   3,
    gdb_gpr_rsi     =   4,
    gdb_gpr_rdi     =   5,
    gdb_gpr_rbp     =   6,
    gdb_gpr_rsp     =   7,
    gdb_gpr_r8      =   8,
    gdb_gpr_r9      =   9,
    gdb_gpr_r10     =  10,
    gdb_gpr_r11     =  11,
    gdb_gpr_r12     =  12,
    gdb_gpr_r13     =  13,
    gdb_gpr_r14     =  14,
    gdb_gpr_r15     =  15,
    gdb_gpr_rip     =  16,
    gdb_gpr_rflags  =  17,
    gdb_gpr_cs      =  18,
    gdb_gpr_ss      =  19,
    gdb_gpr_ds      =  20,
    gdb_gpr_es      =  21,
    gdb_gpr_fs      =  22,
    gdb_gpr_gs      =  23,
    gdb_fpu_stmm0   =  24,
    gdb_fpu_stmm1   =  25,
    gdb_fpu_stmm2   =  26,
    gdb_fpu_stmm3   =  27,
    gdb_fpu_stmm4   =  28,
    gdb_fpu_stmm5   =  29,
    gdb_fpu_stmm6   =  30,
    gdb_fpu_stmm7   =  31,
    gdb_fpu_fctrl   =  32,  gdb_fpu_fcw = gdb_fpu_fctrl,
    gdb_fpu_fstat   =  33,  gdb_fpu_fsw = gdb_fpu_fstat,
    gdb_fpu_ftag    =  34,  gdb_fpu_ftw = gdb_fpu_ftag,
    gdb_fpu_fiseg   =  35,  gdb_fpu_cs  = gdb_fpu_fiseg,
    gdb_fpu_fioff   =  36,  gdb_fpu_ip  = gdb_fpu_fioff,
    gdb_fpu_foseg   =  37,  gdb_fpu_ds  = gdb_fpu_foseg,
    gdb_fpu_fooff   =  38,  gdb_fpu_dp  = gdb_fpu_fooff,
    gdb_fpu_fop     =  39,
    gdb_fpu_xmm0    =  40,
    gdb_fpu_xmm1    =  41,
    gdb_fpu_xmm2    =  42,
    gdb_fpu_xmm3    =  43,
    gdb_fpu_xmm4    =  44,
    gdb_fpu_xmm5    =  45,
    gdb_fpu_xmm6    =  46,
    gdb_fpu_xmm7    =  47,
    gdb_fpu_xmm8    =  48,
    gdb_fpu_xmm9    =  49,
    gdb_fpu_xmm10   =  50,
    gdb_fpu_xmm11   =  51,
    gdb_fpu_xmm12   =  52,
    gdb_fpu_xmm13   =  53,
    gdb_fpu_xmm14   =  54,
    gdb_fpu_xmm15   =  55,
    gdb_fpu_mxcsr   =  56,
};

RegisterContextDarwin_x86_64::RegisterContextDarwin_x86_64 (Thread &thread, uint32_t concrete_frame_idx) :
    RegisterContext (thread, concrete_frame_idx),
    gpr(),
    fpu(),
    exc()
{
    uint32_t i;
    for (i=0; i<kNumErrors; i++)
    {
        gpr_errs[i] = -1;
        fpu_errs[i] = -1;
        exc_errs[i] = -1;
    }
}

RegisterContextDarwin_x86_64::~RegisterContextDarwin_x86_64()
{
}

#define GPR_OFFSET(reg) (offsetof (RegisterContextDarwin_x86_64::GPR, reg))
#define FPU_OFFSET(reg) (offsetof (RegisterContextDarwin_x86_64::FPU, reg) + sizeof (RegisterContextDarwin_x86_64::GPR))
#define EXC_OFFSET(reg) (offsetof (RegisterContextDarwin_x86_64::EXC, reg) + sizeof (RegisterContextDarwin_x86_64::GPR) + sizeof (RegisterContextDarwin_x86_64::FPU))

// These macros will auto define the register name, alt name, register size,
// register offset, encoding, format and native register. This ensures that
// the register state structures are defined correctly and have the correct
// sizes and offsets.
#define DEFINE_GPR(reg, alt)    #reg, alt, sizeof(((RegisterContextDarwin_x86_64::GPR *)NULL)->reg), GPR_OFFSET(reg), eEncodingUint, eFormatHex
#define DEFINE_FPU_UINT(reg)    #reg, NULL, sizeof(((RegisterContextDarwin_x86_64::FPU *)NULL)->reg), FPU_OFFSET(reg), eEncodingUint, eFormatHex
#define DEFINE_FPU_VECT(reg, i) #reg#i, NULL, sizeof(((RegisterContextDarwin_x86_64::FPU *)NULL)->reg[i].bytes), FPU_OFFSET(reg[i]), eEncodingVector, eFormatVectorOfUInt8, { gcc_dwarf_fpu_##reg##i, gcc_dwarf_fpu_##reg##i, LLDB_INVALID_REGNUM, gdb_fpu_##reg##i, fpu_##reg##i }
#define DEFINE_EXC(reg)         #reg, NULL, sizeof(((RegisterContextDarwin_x86_64::EXC *)NULL)->reg), EXC_OFFSET(reg), eEncodingUint, eFormatHex

#define REG_CONTEXT_SIZE (sizeof (RegisterContextDarwin_x86_64::GPR) + sizeof (RegisterContextDarwin_x86_64::FPU) + sizeof (RegisterContextDarwin_x86_64::EXC))

// General purpose registers for 64 bit
static RegisterInfo g_register_infos[] =
{
//  Macro auto defines most stuff   GCC                    DWARF                GENERIC                    GDB                  LLDB
//  =============================== ====================== ===================  ========================== ==================== =====================
    { DEFINE_GPR (rax   , NULL)     , { gcc_dwarf_gpr_rax  , gcc_dwarf_gpr_rax  , LLDB_INVALID_REGNUM      , gdb_gpr_rax        , gpr_rax       }},
    { DEFINE_GPR (rbx   , NULL)     , { gcc_dwarf_gpr_rbx  , gcc_dwarf_gpr_rbx  , LLDB_INVALID_REGNUM      , gdb_gpr_rbx        , gpr_rbx       }},
    { DEFINE_GPR (rcx   , NULL)     , { gcc_dwarf_gpr_rcx  , gcc_dwarf_gpr_rcx  , LLDB_INVALID_REGNUM      , gdb_gpr_rcx        , gpr_rcx       }},
    { DEFINE_GPR (rdx   , NULL)     , { gcc_dwarf_gpr_rdx  , gcc_dwarf_gpr_rdx  , LLDB_INVALID_REGNUM      , gdb_gpr_rdx        , gpr_rdx       }},
    { DEFINE_GPR (rdi   , NULL)     , { gcc_dwarf_gpr_rdi  , gcc_dwarf_gpr_rdi  , LLDB_INVALID_REGNUM      , gdb_gpr_rdi        , gpr_rdi       }},
    { DEFINE_GPR (rsi   , NULL)     , { gcc_dwarf_gpr_rsi  , gcc_dwarf_gpr_rsi  , LLDB_INVALID_REGNUM      , gdb_gpr_rsi        , gpr_rsi       }},
    { DEFINE_GPR (rbp   , "fp")     , { gcc_dwarf_gpr_rbp  , gcc_dwarf_gpr_rbp  , LLDB_REGNUM_GENERIC_FP   , gdb_gpr_rbp        , gpr_rbp       }},
    { DEFINE_GPR (rsp   , "sp")     , { gcc_dwarf_gpr_rsp  , gcc_dwarf_gpr_rsp  , LLDB_REGNUM_GENERIC_SP   , gdb_gpr_rsp        , gpr_rsp       }},
    { DEFINE_GPR (r8    , NULL)     , { gcc_dwarf_gpr_r8   , gcc_dwarf_gpr_r8   , LLDB_INVALID_REGNUM      , gdb_gpr_r8         , gpr_r8        }},
    { DEFINE_GPR (r9    , NULL)     , { gcc_dwarf_gpr_r9   , gcc_dwarf_gpr_r9   , LLDB_INVALID_REGNUM      , gdb_gpr_r9         , gpr_r9        }},
    { DEFINE_GPR (r10   , NULL)     , { gcc_dwarf_gpr_r10  , gcc_dwarf_gpr_r10  , LLDB_INVALID_REGNUM      , gdb_gpr_r10        , gpr_r10       }},
    { DEFINE_GPR (r11   , NULL)     , { gcc_dwarf_gpr_r11  , gcc_dwarf_gpr_r11  , LLDB_INVALID_REGNUM      , gdb_gpr_r11        , gpr_r11       }},
    { DEFINE_GPR (r12   , NULL)     , { gcc_dwarf_gpr_r12  , gcc_dwarf_gpr_r12  , LLDB_INVALID_REGNUM      , gdb_gpr_r12        , gpr_r12       }},
    { DEFINE_GPR (r13   , NULL)     , { gcc_dwarf_gpr_r13  , gcc_dwarf_gpr_r13  , LLDB_INVALID_REGNUM      , gdb_gpr_r13        , gpr_r13       }},
    { DEFINE_GPR (r14   , NULL)     , { gcc_dwarf_gpr_r14  , gcc_dwarf_gpr_r14  , LLDB_INVALID_REGNUM      , gdb_gpr_r14        , gpr_r14       }},
    { DEFINE_GPR (r15   , NULL)     , { gcc_dwarf_gpr_r15  , gcc_dwarf_gpr_r15  , LLDB_INVALID_REGNUM      , gdb_gpr_r15        , gpr_r15       }},
    { DEFINE_GPR (rip   , "pc")     , { gcc_dwarf_gpr_rip  , gcc_dwarf_gpr_rip  , LLDB_REGNUM_GENERIC_PC   , gdb_gpr_rip        , gpr_rip       }},
    { DEFINE_GPR (rflags, "flags")  , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_REGNUM_GENERIC_FLAGS, gdb_gpr_rflags     , gpr_rflags    }},
    { DEFINE_GPR (cs    , NULL)     , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_gpr_cs         , gpr_cs        }},
    { DEFINE_GPR (fs    , NULL)     , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_gpr_fs         , gpr_fs        }},
    { DEFINE_GPR (gs    , NULL)     , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_gpr_gs         , gpr_gs        }},

    { DEFINE_FPU_UINT(fcw)          , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_fcw        , fpu_fcw       }},
    { DEFINE_FPU_UINT(fsw)          , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_fsw        , fpu_fsw       }},
    { DEFINE_FPU_UINT(ftw)          , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_ftw        , fpu_ftw       }},
    { DEFINE_FPU_UINT(fop)          , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_fop        , fpu_fop       }},
    { DEFINE_FPU_UINT(ip)           , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_ip         , fpu_ip        }},
    { DEFINE_FPU_UINT(cs)           , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_cs         , fpu_cs        }},
    { DEFINE_FPU_UINT(dp)           , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_dp         , fpu_dp        }},
    { DEFINE_FPU_UINT(ds)           , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_ds         , fpu_ds        }},
    { DEFINE_FPU_UINT(mxcsr)        , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , gdb_fpu_mxcsr      , fpu_mxcsr     }},
    { DEFINE_FPU_UINT(mxcsrmask)    , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , LLDB_INVALID_REGNUM, fpu_mxcsrmask }},
    { DEFINE_FPU_VECT(stmm,0)   },
    { DEFINE_FPU_VECT(stmm,1)   },
    { DEFINE_FPU_VECT(stmm,2)   },
    { DEFINE_FPU_VECT(stmm,3)   },
    { DEFINE_FPU_VECT(stmm,4)   },
    { DEFINE_FPU_VECT(stmm,5)   },
    { DEFINE_FPU_VECT(stmm,6)   },
    { DEFINE_FPU_VECT(stmm,7)   },
    { DEFINE_FPU_VECT(xmm,0)    },
    { DEFINE_FPU_VECT(xmm,1)    },
    { DEFINE_FPU_VECT(xmm,2)    },
    { DEFINE_FPU_VECT(xmm,3)    },
    { DEFINE_FPU_VECT(xmm,4)    },
    { DEFINE_FPU_VECT(xmm,5)    },
    { DEFINE_FPU_VECT(xmm,6)    },
    { DEFINE_FPU_VECT(xmm,7)    },
    { DEFINE_FPU_VECT(xmm,8)    },
    { DEFINE_FPU_VECT(xmm,9)    },
    { DEFINE_FPU_VECT(xmm,10)   },
    { DEFINE_FPU_VECT(xmm,11)   },
    { DEFINE_FPU_VECT(xmm,12)   },
    { DEFINE_FPU_VECT(xmm,13)   },
    { DEFINE_FPU_VECT(xmm,14)   },
    { DEFINE_FPU_VECT(xmm,15)   },

    { DEFINE_EXC(trapno)            , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , LLDB_INVALID_REGNUM, exc_trapno     }},
    { DEFINE_EXC(err)               , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , LLDB_INVALID_REGNUM, exc_err        }},
    { DEFINE_EXC(faultvaddr)        , { LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM, LLDB_INVALID_REGNUM      , LLDB_INVALID_REGNUM, exc_faultvaddr }}
};

static size_t k_num_register_infos = (sizeof(g_register_infos)/sizeof(RegisterInfo));


void
RegisterContextDarwin_x86_64::InvalidateAllRegisters ()
{
    InvalidateAllRegisterStates();
}


size_t
RegisterContextDarwin_x86_64::GetRegisterCount ()
{
    assert(k_num_register_infos == k_num_registers);
    return k_num_registers;
}


const RegisterInfo *
RegisterContextDarwin_x86_64::GetRegisterInfoAtIndex (uint32_t reg)
{
    assert(k_num_register_infos == k_num_registers);
    if (reg < k_num_registers)
        return &g_register_infos[reg];
    return NULL;
}


size_t
RegisterContextDarwin_x86_64::GetRegisterInfosCount ()
{
    return k_num_register_infos;
}

const lldb_private::RegisterInfo *
RegisterContextDarwin_x86_64::GetRegisterInfos ()
{
    return g_register_infos;
}



static uint32_t g_gpr_regnums[] =
{
    gpr_rax,
    gpr_rbx,
    gpr_rcx,
    gpr_rdx,
    gpr_rdi,
    gpr_rsi,
    gpr_rbp,
    gpr_rsp,
    gpr_r8,
    gpr_r9,
    gpr_r10,
    gpr_r11,
    gpr_r12,
    gpr_r13,
    gpr_r14,
    gpr_r15,
    gpr_rip,
    gpr_rflags,
    gpr_cs,
    gpr_fs,
    gpr_gs
};

static uint32_t g_fpu_regnums[] =
{
    fpu_fcw,
    fpu_fsw,
    fpu_ftw,
    fpu_fop,
    fpu_ip,
    fpu_cs,
    fpu_dp,
    fpu_ds,
    fpu_mxcsr,
    fpu_mxcsrmask,
    fpu_stmm0,
    fpu_stmm1,
    fpu_stmm2,
    fpu_stmm3,
    fpu_stmm4,
    fpu_stmm5,
    fpu_stmm6,
    fpu_stmm7,
    fpu_xmm0,
    fpu_xmm1,
    fpu_xmm2,
    fpu_xmm3,
    fpu_xmm4,
    fpu_xmm5,
    fpu_xmm6,
    fpu_xmm7,
    fpu_xmm8,
    fpu_xmm9,
    fpu_xmm10,
    fpu_xmm11,
    fpu_xmm12,
    fpu_xmm13,
    fpu_xmm14,
    fpu_xmm15
};

static uint32_t
g_exc_regnums[] =
{
    exc_trapno,
    exc_err,
    exc_faultvaddr
};

// Number of registers in each register set
const size_t k_num_gpr_registers = sizeof(g_gpr_regnums) / sizeof(uint32_t);
const size_t k_num_fpu_registers = sizeof(g_fpu_regnums) / sizeof(uint32_t);
const size_t k_num_exc_registers = sizeof(g_exc_regnums) / sizeof(uint32_t);

//----------------------------------------------------------------------
// Register set definitions. The first definitions at register set index
// of zero is for all registers, followed by other registers sets. The
// register information for the all register set need not be filled in.
//----------------------------------------------------------------------
static const RegisterSet g_reg_sets[] =
{
    { "General Purpose Registers",  "gpr",  k_num_gpr_registers,    g_gpr_regnums,      },
    { "Floating Point Registers",   "fpu",  k_num_fpu_registers,    g_fpu_regnums       },
    { "Exception State Registers",  "exc",  k_num_exc_registers,    g_exc_regnums       }
};

const size_t k_num_regsets = sizeof(g_reg_sets) / sizeof(RegisterSet);


size_t
RegisterContextDarwin_x86_64::GetRegisterSetCount ()
{
    return k_num_regsets;
}

const RegisterSet *
RegisterContextDarwin_x86_64::GetRegisterSet (uint32_t reg_set)
{
    if (reg_set < k_num_regsets)
        return &g_reg_sets[reg_set];
    return NULL;
}

int
RegisterContextDarwin_x86_64::GetSetForNativeRegNum (int reg_num)
{
    if (reg_num < fpu_fcw)
        return GPRRegSet;
    else if (reg_num < exc_trapno)
        return FPURegSet;
    else if (reg_num < k_num_registers)
        return EXCRegSet;
    return -1;
}

void
RegisterContextDarwin_x86_64::LogGPR(Log *log, const char *format, ...)
{
    if (log)
    {
        if (format)
        {
            va_list args;
            va_start (args, format);
            log->VAPrintf (format, args);
            va_end (args);
        }
        for (uint32_t i=0; i<k_num_gpr_registers; i++)
        {
            uint32_t reg = gpr_rax + i;
            log->Printf("%12s = 0x%16.16llx", g_register_infos[reg].name, (&gpr.rax)[reg]);
        }
    }
}

int
RegisterContextDarwin_x86_64::ReadGPR (bool force)
{
    int set = GPRRegSet;
    if (force || !RegisterSetIsCached(set))
    {
        SetError(set, Read, DoReadGPR(GetThreadID(), set, gpr));
    }
    return GetError(GPRRegSet, Read);
}

int
RegisterContextDarwin_x86_64::ReadFPU (bool force)
{
    int set = FPURegSet;
    if (force || !RegisterSetIsCached(set))
    {
        SetError(set, Read, DoReadFPU(GetThreadID(), set, fpu));
    }
    return GetError(FPURegSet, Read);
}

int
RegisterContextDarwin_x86_64::ReadEXC (bool force)
{
    int set = EXCRegSet;
    if (force || !RegisterSetIsCached(set))
    {
        SetError(set, Read, DoReadEXC(GetThreadID(), set, exc));
    }
    return GetError(EXCRegSet, Read);
}

int
RegisterContextDarwin_x86_64::WriteGPR ()
{
    int set = GPRRegSet;
    if (!RegisterSetIsCached(set))
    {
        SetError (set, Write, -1);
        return -1;
    }
    SetError (set, Write, DoWriteGPR(GetThreadID(), set, gpr));
    SetError (set, Read, -1);
    return GetError (set, Write);
}

int
RegisterContextDarwin_x86_64::WriteFPU ()
{
    int set = FPURegSet;
    if (!RegisterSetIsCached(set))
    {
        SetError (set, Write, -1);
        return -1;
    }
    SetError (set, Write, DoWriteFPU(GetThreadID(), set, fpu));
    SetError (set, Read, -1);
    return GetError (set, Write);
}

int
RegisterContextDarwin_x86_64::WriteEXC ()
{
    int set = EXCRegSet;
    if (!RegisterSetIsCached(set))
    {
        SetError (set, Write, -1);
        return -1;
    }
    SetError (set, Write, DoWriteEXC(GetThreadID(), set, exc));
    SetError (set, Read, -1);
    return GetError (set, Write);
}

int
RegisterContextDarwin_x86_64::ReadRegisterSet(uint32_t set, bool force)
{
    switch (set)
    {
    case GPRRegSet:    return ReadGPR (force);
    case FPURegSet:    return ReadFPU (force);
    case EXCRegSet:    return ReadEXC (force);
    default: break;
    }
    return -1;
}

int
RegisterContextDarwin_x86_64::WriteRegisterSet(uint32_t set)
{
    // Make sure we have a valid context to set.
    switch (set)
    {
    case GPRRegSet:    return WriteGPR ();
    case FPURegSet:    return WriteFPU ();
    case EXCRegSet:    return WriteEXC ();
    default: break;
    }
    return -1;
}


bool
RegisterContextDarwin_x86_64::ReadRegister (const RegisterInfo *reg_info,
                                          RegisterValue &value)
{
    const uint32_t reg = reg_info->kinds[eRegisterKindLLDB];
    int set = RegisterContextDarwin_x86_64::GetSetForNativeRegNum (reg);
    if (set == -1)
        return false;

    if (ReadRegisterSet(set, false) != 0)
        return false;

    switch (reg)
    {
    case gpr_rax:
    case gpr_rbx:
    case gpr_rcx:
    case gpr_rdx:
    case gpr_rdi:
    case gpr_rsi:
    case gpr_rbp:
    case gpr_rsp:
    case gpr_r8:
    case gpr_r9:
    case gpr_r10:
    case gpr_r11:
    case gpr_r12:
    case gpr_r13:
    case gpr_r14:
    case gpr_r15:
    case gpr_rip:
    case gpr_rflags:
    case gpr_cs:
    case gpr_fs:
    case gpr_gs:
        value = (&gpr.rax)[reg - gpr_rax];
        break;

    case fpu_fcw:
        value = fpu.fcw;
        break;

    case fpu_fsw:
        value = fpu.fsw;
        break;

    case fpu_ftw:
        value = fpu.ftw;
        break;

    case fpu_fop:
        value = fpu.fop;
        break;

    case fpu_ip:
        value = fpu.ip;
        break;

    case fpu_cs:
        value = fpu.cs;
        break;

    case fpu_dp:
        value = fpu.dp;
        break;

    case fpu_ds:
        value = fpu.ds;
        break;

    case fpu_mxcsr:
        value = fpu.mxcsr;
        break;

    case fpu_mxcsrmask:
        value = fpu.mxcsrmask;
        break;

    case fpu_stmm0:
    case fpu_stmm1:
    case fpu_stmm2:
    case fpu_stmm3:
    case fpu_stmm4:
    case fpu_stmm5:
    case fpu_stmm6:
    case fpu_stmm7:
        value.SetBytes(fpu.stmm[reg - fpu_stmm0].bytes, reg_info->byte_size, lldb::endian::InlHostByteOrder());
        break;

    case fpu_xmm0:
    case fpu_xmm1:
    case fpu_xmm2:
    case fpu_xmm3:
    case fpu_xmm4:
    case fpu_xmm5:
    case fpu_xmm6:
    case fpu_xmm7:
    case fpu_xmm8:
    case fpu_xmm9:
    case fpu_xmm10:
    case fpu_xmm11:
    case fpu_xmm12:
    case fpu_xmm13:
    case fpu_xmm14:
    case fpu_xmm15:
        value.SetBytes(fpu.xmm[reg - fpu_xmm0].bytes, reg_info->byte_size, lldb::endian::InlHostByteOrder());
        break;

    case exc_trapno:
        value = exc.trapno;
        break;

    case exc_err:
        value = exc.err;
        break;

    case exc_faultvaddr:
        value = exc.faultvaddr;
        break;

    default:
        return false;
    }
    return true;
}


bool
RegisterContextDarwin_x86_64::WriteRegister (const RegisterInfo *reg_info,
                                           const RegisterValue &value)
{
    const uint32_t reg = reg_info->kinds[eRegisterKindLLDB];
    int set = RegisterContextDarwin_x86_64::GetSetForNativeRegNum (reg);

    if (set == -1)
        return false;

    if (ReadRegisterSet(set, false) != 0)
        return false;

    switch (reg)
    {
    case gpr_rax:
    case gpr_rbx:
    case gpr_rcx:
    case gpr_rdx:
    case gpr_rdi:
    case gpr_rsi:
    case gpr_rbp:
    case gpr_rsp:
    case gpr_r8:
    case gpr_r9:
    case gpr_r10:
    case gpr_r11:
    case gpr_r12:
    case gpr_r13:
    case gpr_r14:
    case gpr_r15:
    case gpr_rip:
    case gpr_rflags:
    case gpr_cs:
    case gpr_fs:
    case gpr_gs:
        (&gpr.rax)[reg - gpr_rax] = value.GetAsUInt64();
        break;

    case fpu_fcw:
        fpu.fcw = value.GetAsUInt16();
        break;

    case fpu_fsw:
        fpu.fsw = value.GetAsUInt16();
        break;

    case fpu_ftw:
        fpu.ftw = value.GetAsUInt8();
        break;

    case fpu_fop:
        fpu.fop = value.GetAsUInt16();
        break;

    case fpu_ip:
        fpu.ip = value.GetAsUInt32();
        break;

    case fpu_cs:
        fpu.cs = value.GetAsUInt16();
        break;

    case fpu_dp:
        fpu.dp = value.GetAsUInt32();
        break;

    case fpu_ds:
        fpu.ds = value.GetAsUInt16();
        break;

    case fpu_mxcsr:
        fpu.mxcsr = value.GetAsUInt32();
        break;

    case fpu_mxcsrmask:
        fpu.mxcsrmask = value.GetAsUInt32();
        break;

    case fpu_stmm0:
    case fpu_stmm1:
    case fpu_stmm2:
    case fpu_stmm3:
    case fpu_stmm4:
    case fpu_stmm5:
    case fpu_stmm6:
    case fpu_stmm7:
        ::memcpy (fpu.stmm[reg - fpu_stmm0].bytes, value.GetBytes(), value.GetByteSize());
        break;

    case fpu_xmm0:
    case fpu_xmm1:
    case fpu_xmm2:
    case fpu_xmm3:
    case fpu_xmm4:
    case fpu_xmm5:
    case fpu_xmm6:
    case fpu_xmm7:
    case fpu_xmm8:
    case fpu_xmm9:
    case fpu_xmm10:
    case fpu_xmm11:
    case fpu_xmm12:
    case fpu_xmm13:
    case fpu_xmm14:
    case fpu_xmm15:
        ::memcpy (fpu.xmm[reg - fpu_xmm0].bytes, value.GetBytes(), value.GetByteSize());
        return false;

    case exc_trapno:
        exc.trapno = value.GetAsUInt32();
        break;

    case exc_err:
        exc.err = value.GetAsUInt32();
        break;

    case exc_faultvaddr:
        exc.faultvaddr = value.GetAsUInt64();
        break;

    default:
        return false;
    }
    return WriteRegisterSet(set) == 0;
}

bool
RegisterContextDarwin_x86_64::ReadAllRegisterValues (lldb::DataBufferSP &data_sp)
{
    data_sp.reset (new DataBufferHeap (REG_CONTEXT_SIZE, 0));
    if (data_sp &&
        ReadGPR (false) == 0 &&
        ReadFPU (false) == 0 &&
        ReadEXC (false) == 0)
    {
        uint8_t *dst = data_sp->GetBytes();
        ::memcpy (dst, &gpr, sizeof(gpr));
        dst += sizeof(gpr);

        ::memcpy (dst, &fpu, sizeof(fpu));
        dst += sizeof(gpr);

        ::memcpy (dst, &exc, sizeof(exc));
        return true;
    }
    return false;
}

bool
RegisterContextDarwin_x86_64::WriteAllRegisterValues (const lldb::DataBufferSP &data_sp)
{
    if (data_sp && data_sp->GetByteSize() == REG_CONTEXT_SIZE)
    {
        const uint8_t *src = data_sp->GetBytes();
        ::memcpy (&gpr, src, sizeof(gpr));
        src += sizeof(gpr);

        ::memcpy (&fpu, src, sizeof(fpu));
        src += sizeof(gpr);

        ::memcpy (&exc, src, sizeof(exc));
        uint32_t success_count = 0;
        if (WriteGPR() == 0)
            ++success_count;
        if (WriteFPU() == 0)
            ++success_count;
        if (WriteEXC() == 0)
            ++success_count;
        return success_count == 3;
    }
    return false;
}


uint32_t
RegisterContextDarwin_x86_64::ConvertRegisterKindToRegisterNumber (uint32_t kind, uint32_t reg)
{
    if (kind == eRegisterKindGeneric)
    {
        switch (reg)
        {
        case LLDB_REGNUM_GENERIC_PC:    return gpr_rip;
        case LLDB_REGNUM_GENERIC_SP:    return gpr_rsp;
        case LLDB_REGNUM_GENERIC_FP:    return gpr_rbp;
        case LLDB_REGNUM_GENERIC_FLAGS: return gpr_rflags;
        case LLDB_REGNUM_GENERIC_RA:
        default:
            break;
        }
    }
    else if (kind == eRegisterKindGCC || kind == eRegisterKindDWARF)
    {
        switch (reg)
        {
        case gcc_dwarf_gpr_rax:  return gpr_rax;
        case gcc_dwarf_gpr_rdx:  return gpr_rdx;
        case gcc_dwarf_gpr_rcx:  return gpr_rcx;
        case gcc_dwarf_gpr_rbx:  return gpr_rbx;
        case gcc_dwarf_gpr_rsi:  return gpr_rsi;
        case gcc_dwarf_gpr_rdi:  return gpr_rdi;
        case gcc_dwarf_gpr_rbp:  return gpr_rbp;
        case gcc_dwarf_gpr_rsp:  return gpr_rsp;
        case gcc_dwarf_gpr_r8:   return gpr_r8;
        case gcc_dwarf_gpr_r9:   return gpr_r9;
        case gcc_dwarf_gpr_r10:  return gpr_r10;
        case gcc_dwarf_gpr_r11:  return gpr_r11;
        case gcc_dwarf_gpr_r12:  return gpr_r12;
        case gcc_dwarf_gpr_r13:  return gpr_r13;
        case gcc_dwarf_gpr_r14:  return gpr_r14;
        case gcc_dwarf_gpr_r15:  return gpr_r15;
        case gcc_dwarf_gpr_rip:  return gpr_rip;
        case gcc_dwarf_fpu_xmm0: return fpu_xmm0;
        case gcc_dwarf_fpu_xmm1: return fpu_xmm1;
        case gcc_dwarf_fpu_xmm2: return fpu_xmm2;
        case gcc_dwarf_fpu_xmm3: return fpu_xmm3;
        case gcc_dwarf_fpu_xmm4: return fpu_xmm4;
        case gcc_dwarf_fpu_xmm5: return fpu_xmm5;
        case gcc_dwarf_fpu_xmm6: return fpu_xmm6;
        case gcc_dwarf_fpu_xmm7: return fpu_xmm7;
        case gcc_dwarf_fpu_xmm8: return fpu_xmm8;
        case gcc_dwarf_fpu_xmm9: return fpu_xmm9;
        case gcc_dwarf_fpu_xmm10: return fpu_xmm10;
        case gcc_dwarf_fpu_xmm11: return fpu_xmm11;
        case gcc_dwarf_fpu_xmm12: return fpu_xmm12;
        case gcc_dwarf_fpu_xmm13: return fpu_xmm13;
        case gcc_dwarf_fpu_xmm14: return fpu_xmm14;
        case gcc_dwarf_fpu_xmm15: return fpu_xmm15;
        case gcc_dwarf_fpu_stmm0: return fpu_stmm0;
        case gcc_dwarf_fpu_stmm1: return fpu_stmm1;
        case gcc_dwarf_fpu_stmm2: return fpu_stmm2;
        case gcc_dwarf_fpu_stmm3: return fpu_stmm3;
        case gcc_dwarf_fpu_stmm4: return fpu_stmm4;
        case gcc_dwarf_fpu_stmm5: return fpu_stmm5;
        case gcc_dwarf_fpu_stmm6: return fpu_stmm6;
        case gcc_dwarf_fpu_stmm7: return fpu_stmm7;
        default:
            break;
        }
    }
    else if (kind == eRegisterKindGDB)
    {
        switch (reg)
        {
        case gdb_gpr_rax     : return gpr_rax;
        case gdb_gpr_rbx     : return gpr_rbx;
        case gdb_gpr_rcx     : return gpr_rcx;
        case gdb_gpr_rdx     : return gpr_rdx;
        case gdb_gpr_rsi     : return gpr_rsi;
        case gdb_gpr_rdi     : return gpr_rdi;
        case gdb_gpr_rbp     : return gpr_rbp;
        case gdb_gpr_rsp     : return gpr_rsp;
        case gdb_gpr_r8      : return gpr_r8;
        case gdb_gpr_r9      : return gpr_r9;
        case gdb_gpr_r10     : return gpr_r10;
        case gdb_gpr_r11     : return gpr_r11;
        case gdb_gpr_r12     : return gpr_r12;
        case gdb_gpr_r13     : return gpr_r13;
        case gdb_gpr_r14     : return gpr_r14;
        case gdb_gpr_r15     : return gpr_r15;
        case gdb_gpr_rip     : return gpr_rip;
        case gdb_gpr_rflags  : return gpr_rflags;
        case gdb_gpr_cs      : return gpr_cs;
        case gdb_gpr_ss      : return gpr_gs;   // HACK: For now for "ss", just copy what is in "gs"
        case gdb_gpr_ds      : return gpr_gs;   // HACK: For now for "ds", just copy what is in "gs"
        case gdb_gpr_es      : return gpr_gs;   // HACK: For now for "es", just copy what is in "gs"
        case gdb_gpr_fs      : return gpr_fs;
        case gdb_gpr_gs      : return gpr_gs;
        case gdb_fpu_stmm0   : return fpu_stmm0;
        case gdb_fpu_stmm1   : return fpu_stmm1;
        case gdb_fpu_stmm2   : return fpu_stmm2;
        case gdb_fpu_stmm3   : return fpu_stmm3;
        case gdb_fpu_stmm4   : return fpu_stmm4;
        case gdb_fpu_stmm5   : return fpu_stmm5;
        case gdb_fpu_stmm6   : return fpu_stmm6;
        case gdb_fpu_stmm7   : return fpu_stmm7;
        case gdb_fpu_fctrl   : return fpu_fctrl;
        case gdb_fpu_fstat   : return fpu_fstat;
        case gdb_fpu_ftag    : return fpu_ftag;
        case gdb_fpu_fiseg   : return fpu_fiseg;
        case gdb_fpu_fioff   : return fpu_fioff;
        case gdb_fpu_foseg   : return fpu_foseg;
        case gdb_fpu_fooff   : return fpu_fooff;
        case gdb_fpu_fop     : return fpu_fop;
        case gdb_fpu_xmm0    : return fpu_xmm0;
        case gdb_fpu_xmm1    : return fpu_xmm1;
        case gdb_fpu_xmm2    : return fpu_xmm2;
        case gdb_fpu_xmm3    : return fpu_xmm3;
        case gdb_fpu_xmm4    : return fpu_xmm4;
        case gdb_fpu_xmm5    : return fpu_xmm5;
        case gdb_fpu_xmm6    : return fpu_xmm6;
        case gdb_fpu_xmm7    : return fpu_xmm7;
        case gdb_fpu_xmm8    : return fpu_xmm8;
        case gdb_fpu_xmm9    : return fpu_xmm9;
        case gdb_fpu_xmm10   : return fpu_xmm10;
        case gdb_fpu_xmm11   : return fpu_xmm11;
        case gdb_fpu_xmm12   : return fpu_xmm12;
        case gdb_fpu_xmm13   : return fpu_xmm13;
        case gdb_fpu_xmm14   : return fpu_xmm14;
        case gdb_fpu_xmm15   : return fpu_xmm15;
        case gdb_fpu_mxcsr   : return fpu_mxcsr;
        default:
            break;
        }
    }
    else if (kind == eRegisterKindLLDB)
    {
        return reg;
    }
    return LLDB_INVALID_REGNUM;
}

bool
RegisterContextDarwin_x86_64::HardwareSingleStep (bool enable)
{
    if (ReadGPR(true) != 0)
        return false;

    const uint64_t trace_bit = 0x100ull;
    if (enable)
    {

        if (gpr.rflags & trace_bit)
            return true;    // trace bit is already set, there is nothing to do
        else
            gpr.rflags |= trace_bit;
    }
    else
    {
        if (gpr.rflags & trace_bit)
            gpr.rflags &= ~trace_bit;
        else
            return true;    // trace bit is clear, there is nothing to do
    }

    return WriteGPR() == 0;
}

