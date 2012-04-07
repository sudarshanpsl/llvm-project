//===-- AppleThreadPlanStepThroughObjCTrampoline.cpp --------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// C Includes
// C++ Includes
// Other libraries and framework includes
// Project includes
#include "AppleThreadPlanStepThroughObjCTrampoline.h"
#include "AppleObjCTrampolineHandler.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Thread.h"
#include "lldb/Expression/ClangExpression.h"
#include "lldb/Expression/ClangFunction.h"
#include "lldb/Target/ExecutionContext.h"
#include "lldb/Target/ObjCLanguageRuntime.h"
#include "lldb/Target/ThreadPlanRunToAddress.h"
#include "lldb/Target/ThreadPlanStepOut.h"
#include "lldb/Core/Log.h"


using namespace lldb;
using namespace lldb_private;

//----------------------------------------------------------------------
// ThreadPlanStepThroughObjCTrampoline constructor
//----------------------------------------------------------------------
AppleThreadPlanStepThroughObjCTrampoline::AppleThreadPlanStepThroughObjCTrampoline
(
    Thread &thread, 
    AppleObjCTrampolineHandler *trampoline_handler,
    ValueList &input_values,
    lldb::addr_t isa_addr,
    lldb::addr_t sel_addr,
    bool stop_others
) :
    ThreadPlan (ThreadPlan::eKindGeneric, 
                "MacOSX Step through ObjC Trampoline", 
                thread, 
                eVoteNoOpinion, 
                eVoteNoOpinion),
    m_trampoline_handler (trampoline_handler),
    m_args_addr (LLDB_INVALID_ADDRESS),
    m_input_values (input_values),
    m_isa_addr(isa_addr),
    m_sel_addr(sel_addr),
    m_impl_function (NULL),
    m_stop_others (stop_others)
{
    
}

//----------------------------------------------------------------------
// Destructor
//----------------------------------------------------------------------
AppleThreadPlanStepThroughObjCTrampoline::~AppleThreadPlanStepThroughObjCTrampoline()
{
}

void
AppleThreadPlanStepThroughObjCTrampoline::DidPush ()
{
//    StreamString errors;
//    ExecutionContext exc_ctx;
//    m_thread.CalculateExecutionContext(exc_ctx);
//    m_func_sp.reset(m_impl_function->GetThreadPlanToCallFunction (exc_ctx, m_args_addr, errors, m_stop_others));
//    m_func_sp->SetPrivate(true);
//    m_thread.QueueThreadPlan (m_func_sp, false);
    m_thread.GetProcess()->AddPreResumeAction (PreResumeInitializeClangFunction, (void *) this);
}

bool
AppleThreadPlanStepThroughObjCTrampoline::InitializeClangFunction ()
{
    if (!m_func_sp)
    {
        StreamString errors;
        m_args_addr = m_trampoline_handler->SetupDispatchFunction(m_thread, m_input_values);
        
        if (m_args_addr == LLDB_INVALID_ADDRESS)
        {
            return false;
        }
        m_impl_function = m_trampoline_handler->GetLookupImplementationWrapperFunction();
        ExecutionContext exc_ctx;
        m_thread.CalculateExecutionContext(exc_ctx);
        m_func_sp.reset(m_impl_function->GetThreadPlanToCallFunction (exc_ctx, m_args_addr, errors, m_stop_others));
        m_func_sp->SetPrivate(true);
        m_thread.QueueThreadPlan (m_func_sp, false);
    }
    return true;
}

bool
AppleThreadPlanStepThroughObjCTrampoline::PreResumeInitializeClangFunction(void *void_myself)
{
    AppleThreadPlanStepThroughObjCTrampoline *myself = static_cast<AppleThreadPlanStepThroughObjCTrampoline *>(void_myself);
    return myself->InitializeClangFunction();
}

void
AppleThreadPlanStepThroughObjCTrampoline::GetDescription (Stream *s,
                                                          lldb::DescriptionLevel level)
{
    if (level == lldb::eDescriptionLevelBrief)
        s->Printf("Step through ObjC trampoline");
    else
    {
        s->Printf ("Stepping to implementation of ObjC method - obj: 0x%llx, isa: 0x%llx, sel: 0x%llx",
        m_input_values.GetValueAtIndex(0)->GetScalar().ULongLong(), m_isa_addr, m_sel_addr);
    }
}
                
bool
AppleThreadPlanStepThroughObjCTrampoline::ValidatePlan (Stream *error)
{
    return true;
}

bool
AppleThreadPlanStepThroughObjCTrampoline::PlanExplainsStop ()
{
    // This plan should actually never stop when it is on the top of the plan
    // stack, since it does all it's running in client plans.
    return false;
}

lldb::StateType
AppleThreadPlanStepThroughObjCTrampoline::GetPlanRunState ()
{
    return eStateRunning;
}

bool
AppleThreadPlanStepThroughObjCTrampoline::ShouldStop (Event *event_ptr)
{
    if (m_func_sp.get() == NULL || m_thread.IsThreadPlanDone(m_func_sp.get()))
    {
        m_func_sp.reset();
        if (!m_run_to_sp) 
        {
            Value target_addr_value;
            ExecutionContext exc_ctx;
            m_thread.CalculateExecutionContext(exc_ctx);
            m_impl_function->FetchFunctionResults (exc_ctx, m_args_addr, target_addr_value);
            m_impl_function->DeallocateFunctionResults(exc_ctx, m_args_addr);
            lldb::addr_t target_addr = target_addr_value.GetScalar().ULongLong();
            Address target_so_addr;
            target_so_addr.SetOpcodeLoadAddress(target_addr, exc_ctx.GetTargetPtr());
            LogSP log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_STEP));
            if (target_addr == 0)
            {
                if (log)
                    log->Printf("Got target implementation of 0x0, stopping.");
                SetPlanComplete();
                return true;
            }
            if (m_trampoline_handler->AddrIsMsgForward(target_addr))
            {
                if (log)
                    log->Printf ("Implementation lookup returned msgForward function: 0x%llx, stopping.", target_addr);

                SymbolContext sc = m_thread.GetStackFrameAtIndex(0)->GetSymbolContext(eSymbolContextEverything);
                m_run_to_sp.reset(new ThreadPlanStepOut (m_thread, 
                                                         &sc, 
                                                         true, 
                                                         m_stop_others, 
                                                         eVoteNoOpinion, 
                                                         eVoteNoOpinion,
                                                         0));
                m_thread.QueueThreadPlan(m_run_to_sp, false);
                m_run_to_sp->SetPrivate(true);
                return false;
            }
            
            if (log)
                log->Printf("Running to ObjC method implementation: 0x%llx", target_addr);
            
            ObjCLanguageRuntime *objc_runtime = GetThread().GetProcess()->GetObjCLanguageRuntime();
            assert (objc_runtime != NULL);
            objc_runtime->AddToMethodCache (m_isa_addr, m_sel_addr, target_addr);
            if (log)
                log->Printf("Adding {isa-addr=0x%llx, sel-addr=0x%llx} = addr=0x%llx to cache.", m_isa_addr, m_sel_addr, target_addr);

            // Extract the target address from the value:
            
            m_run_to_sp.reset(new ThreadPlanRunToAddress(m_thread, target_so_addr, m_stop_others));
            m_thread.QueueThreadPlan(m_run_to_sp, false);
            m_run_to_sp->SetPrivate(true);
            return false;
        }
        else if (m_thread.IsThreadPlanDone(m_run_to_sp.get()))
        {
            SetPlanComplete();
            return true;
        }
    }
    return false;
}

// The base class MischiefManaged does some cleanup - so you have to call it
// in your MischiefManaged derived class.
bool
AppleThreadPlanStepThroughObjCTrampoline::MischiefManaged ()
{
    if (IsPlanComplete())
        return true;
    else
        return false;
}

bool
AppleThreadPlanStepThroughObjCTrampoline::WillStop()
{
    return true;
}
