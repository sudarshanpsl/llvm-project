//===-- POSIXThread.cpp -----------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

// C Includes
#include <errno.h>

// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Core/Debugger.h"
#include "lldb/Host/Host.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/StopInfo.h"
#include "lldb/Target/Target.h"
#include "POSIXStopInfo.h"
#include "POSIXThread.h"
#include "ProcessPOSIX.h"
#include "ProcessPOSIXLog.h"
#include "ProcessMonitor.h"
#include "RegisterContext_i386.h"
#include "RegisterContext_x86_64.h"
#include "RegisterContextPOSIX.h"
#include "RegisterContextLinux_x86_64.h"
#include "RegisterContextFreeBSD_x86_64.h"

#include "UnwindLLDB.h"

using namespace lldb;
using namespace lldb_private;


POSIXThread::POSIXThread(Process &process, lldb::tid_t tid)
    : Thread(process, tid),
      m_frame_ap()
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("POSIXThread::%s (tid = %" PRIi64 ")", __FUNCTION__, tid);
}

POSIXThread::~POSIXThread()
{
    DestroyThread();
}

ProcessMonitor &
POSIXThread::GetMonitor()
{
    ProcessSP base = GetProcess();
    ProcessPOSIX &process = static_cast<ProcessPOSIX&>(*base);
    return process.GetMonitor();
}

void
POSIXThread::RefreshStateAfterStop()
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("POSIXThread::%s ()", __FUNCTION__);

    // Let all threads recover from stopping and do any clean up based
    // on the previous thread state (if any).
    ProcessSP base = GetProcess();
    ProcessPOSIX &process = static_cast<ProcessPOSIX&>(*base);
    process.GetThreadList().RefreshStateAfterStop();
}

const char *
POSIXThread::GetInfo()
{
    return NULL;
}

lldb::RegisterContextSP
POSIXThread::GetRegisterContext()
{
    if (!m_reg_context_sp)
    {
        ArchSpec arch = Host::GetArchitecture();

        switch (arch.GetCore())
        {
        default:
            assert(false && "CPU type not supported!");
            break;

        case ArchSpec::eCore_x86_32_i386:
        case ArchSpec::eCore_x86_32_i486:
        case ArchSpec::eCore_x86_32_i486sx:
            m_reg_context_sp.reset(new RegisterContext_i386(*this, 0));
            break;

        case ArchSpec::eCore_x86_64_x86_64:
// TODO: Use target OS/architecture detection rather than ifdefs so that
// lldb built on FreeBSD can debug on Linux and vice-versa.
#ifdef __linux__
            m_reg_context_sp.reset(new RegisterContextLinux_x86_64(*this, 0));
#endif
#ifdef __FreeBSD__
            m_reg_context_sp.reset(new RegisterContextFreeBSD_x86_64(*this, 0));
#endif
            break;
        }
    }
    return m_reg_context_sp;
}

lldb::RegisterContextSP
POSIXThread::CreateRegisterContextForFrame(lldb_private::StackFrame *frame)
{
    lldb::RegisterContextSP reg_ctx_sp;
    uint32_t concrete_frame_idx = 0;

    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("POSIXThread::%s ()", __FUNCTION__);

    if (frame)
        concrete_frame_idx = frame->GetConcreteFrameIndex();

    if (concrete_frame_idx == 0)
        reg_ctx_sp = GetRegisterContext();
    else
    {
        assert(GetUnwinder());
        reg_ctx_sp = GetUnwinder()->CreateRegisterContextForFrame(frame);
    }

    return reg_ctx_sp;
}

lldb::StopInfoSP
POSIXThread::GetPrivateStopReason()
{
    return m_actual_stop_info_sp;
}

Unwind *
POSIXThread::GetUnwinder()
{
    if (m_unwinder_ap.get() == NULL)
        m_unwinder_ap.reset(new UnwindLLDB(*this));

    return m_unwinder_ap.get();
}

void
POSIXThread::WillResume(lldb::StateType resume_state)
{
	// TODO: the line below shouldn't really be done, but
    // the POSIXThread might rely on this so I will leave this in for now
    SetResumeState(resume_state);
}

bool
POSIXThread::Resume()
{
    lldb::StateType resume_state = GetResumeState();
    ProcessMonitor &monitor = GetMonitor();
    bool status;

    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log && log->GetMask().Test(POSIX_LOG_VERBOSE))
        log->Printf ("POSIXThread::%s ()", __FUNCTION__);

    switch (resume_state)
    {
    default:
        assert(false && "Unexpected state for resume!");
        status = false;
        break;

    case lldb::eStateRunning:
        SetState(resume_state);
        status = monitor.Resume(GetID(), GetResumeSignal());
        break;

    case lldb::eStateStepping:
        SetState(resume_state);
        status = monitor.SingleStep(GetID(), GetResumeSignal());
        break;
    case lldb::eStateStopped:
    case lldb::eStateSuspended:
        status = true;
        break;
    }

    return status;
}

void
POSIXThread::Notify(const ProcessMessage &message)
{
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log)
        log->Printf ("POSIXThread::%s () message kind = '%s'", __FUNCTION__, message.PrintKind());

    switch (message.GetKind())
    {
    default:
        assert(false && "Unexpected message kind!");
        break;

    case ProcessMessage::eLimboMessage:
        LimboNotify(message);
        break;
        
    case ProcessMessage::eSignalMessage:
        SignalNotify(message);
        break;

    case ProcessMessage::eSignalDeliveredMessage:
        SignalDeliveredNotify(message);
        break;

    case ProcessMessage::eTraceMessage:
        TraceNotify(message);
        break;

    case ProcessMessage::eBreakpointMessage:
        BreakNotify(message);
        break;

    case ProcessMessage::eCrashMessage:
        CrashNotify(message);
        break;

    case ProcessMessage::eNewThreadMessage:
        ThreadNotify(message);
        break;
    }
}

void
POSIXThread::BreakNotify(const ProcessMessage &message)
{
    bool status;
    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));

    assert(GetRegisterContext());
    status = GetRegisterContextPOSIX()->UpdateAfterBreakpoint();
    assert(status && "Breakpoint update failed!");

    // With our register state restored, resolve the breakpoint object
    // corresponding to our current PC.
    assert(GetRegisterContext());
    lldb::addr_t pc = GetRegisterContext()->GetPC();
    if (log)
        log->Printf ("POSIXThread::%s () PC=0x%8.8" PRIx64, __FUNCTION__, pc);
    lldb::BreakpointSiteSP bp_site(GetProcess()->GetBreakpointSiteList().FindByAddress(pc));
    assert(bp_site);
    lldb::break_id_t bp_id = bp_site->GetID();
    assert(bp_site && bp_site->ValidForThisThread(this));

    
    m_breakpoint = bp_site;
    SetStopInfo (StopInfo::CreateStopReasonWithBreakpointSiteID(*this, bp_id));
}

void
POSIXThread::TraceNotify(const ProcessMessage &message)
{
    SetStopInfo (StopInfo::CreateStopReasonToTrace(*this));
}

void
POSIXThread::LimboNotify(const ProcessMessage &message)
{
    SetStopInfo (lldb::StopInfoSP(new POSIXLimboStopInfo(*this)));
}

void
POSIXThread::SignalNotify(const ProcessMessage &message)
{
    int signo = message.GetSignal();

    SetStopInfo (StopInfo::CreateStopReasonWithSignal(*this, signo));
    SetResumeSignal(signo);
}

void
POSIXThread::SignalDeliveredNotify(const ProcessMessage &message)
{
    int signo = message.GetSignal();

    SetStopInfo (StopInfo::CreateStopReasonWithSignal(*this, signo));
    SetResumeSignal(signo);
}

void
POSIXThread::CrashNotify(const ProcessMessage &message)
{
    int signo = message.GetSignal();

    assert(message.GetKind() == ProcessMessage::eCrashMessage);

    Log *log (ProcessPOSIXLog::GetLogIfAllCategoriesSet (POSIX_LOG_THREAD));
    if (log)
        log->Printf ("POSIXThread::%s () signo = %i, reason = '%s'", __FUNCTION__, signo, message.PrintCrashReason());

    SetStopInfo (lldb::StopInfoSP(new POSIXCrashStopInfo(*this, signo, message.GetCrashReason())));
    SetResumeSignal(signo);
}

void
POSIXThread::ThreadNotify(const ProcessMessage &message)
{
    SetStopInfo (lldb::StopInfoSP(new POSIXNewThreadStopInfo(*this)));
}

unsigned
POSIXThread::GetRegisterIndexFromOffset(unsigned offset)
{
    unsigned reg;
    ArchSpec arch = Host::GetArchitecture();

    switch (arch.GetCore())
    {
    default:
        assert(false && "CPU type not supported!");
        break;

    case ArchSpec::eCore_x86_32_i386:
    case ArchSpec::eCore_x86_32_i486:
    case ArchSpec::eCore_x86_32_i486sx:
        reg = RegisterContext_i386::GetRegisterIndexFromOffset(offset);
        break;

    case ArchSpec::eCore_x86_64_x86_64:
        reg = RegisterContext_x86_64::GetRegisterIndexFromOffset(offset);
        break;
    }
    return reg;
}

const char *
POSIXThread::GetRegisterName(unsigned reg)
{
    const char * name;
    ArchSpec arch = Host::GetArchitecture();

    switch (arch.GetCore())
    {
    default:
        assert(false && "CPU type not supported!");
        break;

    case ArchSpec::eCore_x86_32_i386:
    case ArchSpec::eCore_x86_32_i486:
    case ArchSpec::eCore_x86_32_i486sx:
        name = RegisterContext_i386::GetRegisterName(reg);
        break;

    case ArchSpec::eCore_x86_64_x86_64:
        name = RegisterContext_x86_64::GetRegisterName(reg);
        break;
    }
    return name;
}

const char *
POSIXThread::GetRegisterNameFromOffset(unsigned offset)
{
    return GetRegisterName(GetRegisterIndexFromOffset(offset));
}

