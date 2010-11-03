//===-- ThreadPlanCallFunction.cpp ------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/Target/ThreadPlanCallFunction.h"

// C Includes
// C++ Includes
// Other libraries and framework includes
#include "llvm/Support/MachO.h"
// Project includes
#include "lldb/lldb-private-log.h"
#include "lldb/Breakpoint/Breakpoint.h"
#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Core/Address.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Stream.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/RegisterContext.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "lldb/Target/Thread.h"
#include "lldb/Target/ThreadPlanRunToAddress.h"

using namespace lldb;
using namespace lldb_private;

//----------------------------------------------------------------------
// ThreadPlanCallFunction: Plan to call a single function
//----------------------------------------------------------------------

ThreadPlanCallFunction::ThreadPlanCallFunction (Thread &thread,
                                                Address &function,
                                                lldb::addr_t arg,
                                                bool stop_other_threads,
                                                bool discard_on_error,
                                                lldb::addr_t *this_arg) :
    ThreadPlan (ThreadPlan::eKindCallFunction, "Call function plan", thread, eVoteNoOpinion, eVoteNoOpinion),
    m_valid (false),
    m_stop_other_threads (stop_other_threads),
    m_arg_addr (arg),
    m_args (NULL),
    m_process (thread.GetProcess()),
    m_thread (thread)
{
    SetOkayToDiscard (discard_on_error);

    Process& process = thread.GetProcess();
    Target& target = process.GetTarget();
    const ABI *abi = process.GetABI();
    
    if (!abi)
        return;
    
    SetBreakpoints();
    
    lldb::addr_t spBelowRedZone = thread.GetRegisterContext()->GetSP() - abi->GetRedZoneSize();
    
    SymbolContextList contexts;
    SymbolContext context;
    ModuleSP executableModuleSP (target.GetExecutableModule());

    if (!executableModuleSP ||
        !executableModuleSP->FindSymbolsWithNameAndType(ConstString ("start"), eSymbolTypeCode, contexts))
        return;
    
    contexts.GetContextAtIndex(0, context);
    
    m_start_addr = context.symbol->GetValue();
    lldb::addr_t StartLoadAddr = m_start_addr.GetLoadAddress(&target);

    if (!thread.SaveFrameZeroState(m_register_backup))
        return;

    m_function_addr = function;
    lldb::addr_t FunctionLoadAddr = m_function_addr.GetLoadAddress(&target);
        
    if (!abi->PrepareTrivialCall(thread, 
                                 spBelowRedZone, 
                                 FunctionLoadAddr, 
                                 StartLoadAddr, 
                                 m_arg_addr,
                                 this_arg))
        return;
    
    m_valid = true;    
}

ThreadPlanCallFunction::ThreadPlanCallFunction (Thread &thread,
                                                Address &function,
                                                ValueList &args,
                                                bool stop_other_threads,
                                                bool discard_on_error) :
    ThreadPlan (ThreadPlan::eKindCallFunction, "Call function plan", thread, eVoteNoOpinion, eVoteNoOpinion),
    m_valid (false),
    m_stop_other_threads (stop_other_threads),
    m_arg_addr (0),
    m_args (&args),
    m_process (thread.GetProcess()),
    m_thread (thread)
{
    
    SetOkayToDiscard (discard_on_error);
    
    Process& process = thread.GetProcess();
    Target& target = process.GetTarget();
    const ABI *abi = process.GetABI();
    
    if(!abi)
        return;
    
    SetBreakpoints();
    
    lldb::addr_t spBelowRedZone = thread.GetRegisterContext()->GetSP() - abi->GetRedZoneSize();
    
    SymbolContextList contexts;
    SymbolContext context;
    ModuleSP executableModuleSP (target.GetExecutableModule());
    
    if (!executableModuleSP ||
        !executableModuleSP->FindSymbolsWithNameAndType(ConstString ("start"), eSymbolTypeCode, contexts))
        return;
    
    contexts.GetContextAtIndex(0, context);
    
    m_start_addr = context.symbol->GetValue();
    lldb::addr_t StartLoadAddr = m_start_addr.GetLoadAddress(&target);
    
    if(!thread.SaveFrameZeroState(m_register_backup))
        return;
    
    m_function_addr = function;
    lldb::addr_t FunctionLoadAddr = m_function_addr.GetLoadAddress(&target);
    
    if (!abi->PrepareNormalCall(thread, 
                                spBelowRedZone, 
                                FunctionLoadAddr, 
                                StartLoadAddr, 
                                *m_args))
        return;
    
    m_valid = true;    
}

ThreadPlanCallFunction::~ThreadPlanCallFunction ()
{
}

void
ThreadPlanCallFunction::GetDescription (Stream *s, lldb::DescriptionLevel level)
{
    if (level == lldb::eDescriptionLevelBrief)
    {
        s->Printf("Function call thread plan");
    }
    else
    {
        if (m_args)
            s->Printf("Thread plan to call 0x%llx with parsed arguments", m_function_addr.GetLoadAddress(&m_process.GetTarget()), m_arg_addr);
        else
            s->Printf("Thread plan to call 0x%llx void * argument at: 0x%llx", m_function_addr.GetLoadAddress(&m_process.GetTarget()), m_arg_addr);
    }
}

bool
ThreadPlanCallFunction::ValidatePlan (Stream *error)
{
    if (!m_valid)
        return false;

    return true;
}

bool
ThreadPlanCallFunction::PlanExplainsStop ()
{    
    // If our subplan knows why we stopped, even if it's done (which would forward the question to us)
    // we answer yes.
    if(m_subplan_sp.get() != NULL && m_subplan_sp->PlanExplainsStop())
        return true;
    
    // Check if the breakpoint is one of ours.
    
    if (BreakpointsExplainStop())
        return true;
    
    // If we don't want to discard this plan, than any stop we don't understand should be propagated up the stack.
    if (!OkayToDiscard())
        return false;
            
    // Otherwise, check the case where we stopped for an internal breakpoint, in that case, continue on.
    // If it is not an internal breakpoint, consult OkayToDiscard.
    lldb::StopInfoSP stop_info_sp = GetPrivateStopReason();
    
    if (stop_info_sp && stop_info_sp->GetStopReason() == eStopReasonBreakpoint)
    {
        uint64_t break_site_id = stop_info_sp->GetValue();
        lldb::BreakpointSiteSP bp_site_sp = m_thread.GetProcess().GetBreakpointSiteList().FindByID(break_site_id);
        if (bp_site_sp)
        {
            uint32_t num_owners = bp_site_sp->GetNumberOfOwners();
            bool is_internal = true;
            for (uint32_t i = 0; i < num_owners; i++)
            {
                Breakpoint &bp = bp_site_sp->GetOwnerAtIndex(i)->GetBreakpoint();
                
                if (!bp.IsInternal())
                {
                    is_internal = false;
                    break;
                }
            }
            if (is_internal)
                return false;
        }
        
        return OkayToDiscard();
    }
    else
    {
        // If the subplan is running, any crashes are attributable to us.
        return (m_subplan_sp.get() != NULL);
    }
}

bool
ThreadPlanCallFunction::ShouldStop (Event *event_ptr)
{
    if (PlanExplainsStop())
    {
        Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_STEP);
        
        if (log)
        {
            RegisterContext *reg_ctx = m_thread.GetRegisterContext();

            log->PutCString("Function completed.  Register state was:");

            for (uint32_t register_index = 0, num_registers = reg_ctx->GetRegisterCount();
                 register_index < num_registers;
                 ++register_index)
            {
                const char *register_name = reg_ctx->GetRegisterName(register_index);
                uint64_t register_value = reg_ctx->ReadRegisterAsUnsigned(register_index, LLDB_INVALID_ADDRESS);
                
                log->Printf("  %s = 0x%llx", register_name, register_value);
            }
        }
        
        m_thread.RestoreSaveFrameZero(m_register_backup);
        m_thread.ClearStackFrames();
        SetPlanComplete();
        
        ClearBreakpoints();
        return true;
    }
    else
    {
        return false;
    }
}

bool
ThreadPlanCallFunction::StopOthers ()
{
    return m_stop_other_threads;
}

void
ThreadPlanCallFunction::SetStopOthers (bool new_value)
{
    if (m_subplan_sp)
    {
        ThreadPlanRunToAddress *address_plan = static_cast<ThreadPlanRunToAddress *>(m_subplan_sp.get());
        address_plan->SetStopOthers(new_value);
    }
    m_stop_other_threads = new_value;
}

StateType
ThreadPlanCallFunction::RunState ()
{
    return eStateRunning;
}

void
ThreadPlanCallFunction::DidPush ()
{
//#define SINGLE_STEP_EXPRESSIONS
    
#ifndef SINGLE_STEP_EXPRESSIONS
    m_subplan_sp.reset(new ThreadPlanRunToAddress(m_thread, m_start_addr, m_stop_other_threads));
    
    m_thread.QueueThreadPlan(m_subplan_sp, false);
#endif
}

bool
ThreadPlanCallFunction::WillStop ()
{
    return true;
}

bool
ThreadPlanCallFunction::MischiefManaged ()
{
    if (IsPlanComplete())
    {
        Log *log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_STEP);

        if (log)
            log->Printf("Completed call function plan.");

        ThreadPlan::MischiefManaged ();
        return true;
    }
    else
    {
        return false;
    }
}

void
ThreadPlanCallFunction::SetBreakpoints ()
{
    Target& target = m_process.GetTarget();
    
    ArchSpec arch_spec = target.GetArchitecture();
    
    switch (arch_spec.GetCPUType())
    {
    default:
        break;
    case llvm::MachO::CPUTypeI386:
        m_cxx_exception_bp_sp = target.CreateBreakpoint (NULL,
                                                       "__cxa_throw",
                                                       eFunctionNameTypeBase, 
                                                       true);
        m_cxx_exception_alloc_bp_sp = target.CreateBreakpoint (NULL,
                                                             "__cxa_allocate",
                                                             eFunctionNameTypeBase,
                                                             true);
        m_objc_exception_bp_sp = target.CreateBreakpoint (NULL,
                                                        "objc_exception_throw",
                                                        eFunctionNameTypeBase,
                                                        true);
        break;
    case llvm::MachO::CPUTypeX86_64:
        m_cxx_exception_bp_sp = target.CreateBreakpoint (NULL,
                                                       "__cxa_throw",
                                                       eFunctionNameTypeBase, 
                                                       true);
        m_cxx_exception_alloc_bp_sp = target.CreateBreakpoint (NULL,
                                                             "__cxa_allocate",
                                                             eFunctionNameTypeBase,
                                                             true);
        break;
    }
}

void
ThreadPlanCallFunction::ClearBreakpoints ()
{
    Target& target = m_process.GetTarget();
    
    if (m_cxx_exception_bp_sp.get())
    {
        target.RemoveBreakpointByID(m_cxx_exception_bp_sp->GetID());
        m_cxx_exception_bp_sp.reset();
    }
    
    if (m_cxx_exception_alloc_bp_sp.get())
    {
        target.RemoveBreakpointByID(m_cxx_exception_alloc_bp_sp->GetID());
        m_cxx_exception_bp_sp.reset();
    }
    
    if (m_objc_exception_bp_sp.get())
    {
        target.RemoveBreakpointByID(m_objc_exception_bp_sp->GetID());
        m_cxx_exception_bp_sp.reset();
    }
}

bool
ThreadPlanCallFunction::BreakpointsExplainStop()
{
    // A temporary fix to set breakpoints at points where exceptions are being
    // thrown.  This functionality will migrate into the Target.
    
    lldb::StopInfoSP stop_info_sp = GetPrivateStopReason();
    
    if (!stop_info_sp || 
        stop_info_sp->GetStopReason() != eStopReasonBreakpoint)
        return false;
    
    uint64_t break_site_id = stop_info_sp->GetValue();
    lldb::BreakpointSiteSP bp_site_sp = m_thread.GetProcess().GetBreakpointSiteList().FindByID(break_site_id);
    
    if (!bp_site_sp)
        return false;
    
    uint32_t num_owners = bp_site_sp->GetNumberOfOwners();
    
    bool        check_cxx_exception = false;
    break_id_t  cxx_exception_bid;
    
    bool        check_cxx_exception_alloc = false;
    break_id_t  cxx_exception_alloc_bid;
    
    bool        check_objc_exception = false;
    break_id_t  objc_exception_bid;
    
    if (m_cxx_exception_bp_sp.get())
    {
        check_cxx_exception = true;
        cxx_exception_bid = m_cxx_exception_bp_sp->GetID();
    }
    
    if (m_cxx_exception_bp_sp.get())
    {
        check_cxx_exception_alloc = true;
        cxx_exception_alloc_bid = m_cxx_exception_alloc_bp_sp->GetID();
    }
    
    if (m_cxx_exception_bp_sp.get())
    {
        check_objc_exception = true;
        objc_exception_bid = m_objc_exception_bp_sp->GetID();
    }
        

    for (uint32_t i = 0; i < num_owners; i++)
    {
        break_id_t bid = bp_site_sp->GetOwnerAtIndex(i)->GetBreakpoint().GetID();
        
        if ((check_cxx_exception        && (bid == cxx_exception_bid)) ||
            (check_cxx_exception_alloc  && (bid == cxx_exception_alloc_bid)) ||
            (check_objc_exception       && (bid == objc_exception_bid)))
            return true;
    }
    
    return false;
}
