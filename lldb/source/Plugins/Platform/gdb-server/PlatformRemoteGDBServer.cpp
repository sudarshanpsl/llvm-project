//===-- PlatformRemoteGDBServer.cpp -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#include "PlatformRemoteGDBServer.h"
#include "lldb/Host/Config.h"

// C++ Includes
// Other libraries and framework includes
// Project includes
#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/Error.h"
#include "lldb/Core/Log.h"
#include "lldb/Core/Module.h"
#include "lldb/Core/ModuleList.h"
#include "lldb/Core/ModuleSpec.h"
#include "lldb/Core/PluginManager.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Host/ConnectionFileDescriptor.h"
#include "lldb/Host/FileSpec.h"
#include "lldb/Host/Host.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Target.h"

#include "Utility/UriParser.h"

using namespace lldb;
using namespace lldb_private;

static bool g_initialized = false;

static std::string MakeGdbServerUrl (const std::string &platform_hostname, uint16_t port)
{
    const char *override_hostname = getenv("LLDB_PLATFORM_REMOTE_GDB_SERVER_HOSTNAME");
    const char *port_offset_c_str = getenv("LLDB_PLATFORM_REMOTE_GDB_SERVER_PORT_OFFSET");
    int port_offset = port_offset_c_str ? ::atoi(port_offset_c_str) : 0;
    lldb_private::StreamString result;
    result.Printf("connect://%s:%u",
            override_hostname ? override_hostname : platform_hostname.c_str(),
            port + port_offset);
    return result.GetString();
}

void
PlatformRemoteGDBServer::Initialize ()
{
    Platform::Initialize ();

    if (g_initialized == false)
    {
        g_initialized = true;
        PluginManager::RegisterPlugin (PlatformRemoteGDBServer::GetPluginNameStatic(),
                                       PlatformRemoteGDBServer::GetDescriptionStatic(),
                                       PlatformRemoteGDBServer::CreateInstance);
    }
}

void
PlatformRemoteGDBServer::Terminate ()
{
    if (g_initialized)
    {
        g_initialized = false;
        PluginManager::UnregisterPlugin (PlatformRemoteGDBServer::CreateInstance);
    }

    Platform::Terminate ();
}

PlatformSP
PlatformRemoteGDBServer::CreateInstance (bool force, const lldb_private::ArchSpec *arch)
{
    bool create = force;
    if (!create)
    {
        create = !arch->TripleVendorWasSpecified() && !arch->TripleOSWasSpecified();
    }
    if (create)
        return PlatformSP(new PlatformRemoteGDBServer());
    return PlatformSP();
}


lldb_private::ConstString
PlatformRemoteGDBServer::GetPluginNameStatic()
{
    static ConstString g_name("remote-gdb-server");
    return g_name;
}

const char *
PlatformRemoteGDBServer::GetDescriptionStatic()
{
    return "A platform that uses the GDB remote protocol as the communication transport.";
}

const char *
PlatformRemoteGDBServer::GetDescription ()
{
    if (m_platform_description.empty())
    {
        if (IsConnected())
        {
            // Send the get description packet
        }
    }
    
    if (!m_platform_description.empty())
        return m_platform_description.c_str();
    return GetDescriptionStatic();
}

Error
PlatformRemoteGDBServer::ResolveExecutable (const ModuleSpec &module_spec,
                                            lldb::ModuleSP &exe_module_sp,
                                            const FileSpecList *module_search_paths_ptr)
{
    // copied from PlatformRemoteiOS

    Error error;
    // Nothing special to do here, just use the actual file and architecture

    ModuleSpec resolved_module_spec(module_spec);

    // Resolve any executable within an apk on Android?
    //Host::ResolveExecutableInBundle (resolved_module_spec.GetFileSpec());

    if (resolved_module_spec.GetFileSpec().Exists())
    {
        if (resolved_module_spec.GetArchitecture().IsValid() || resolved_module_spec.GetUUID().IsValid())
        {
            error = ModuleList::GetSharedModule (resolved_module_spec,
                                                 exe_module_sp,
                                                 NULL,
                                                 NULL,
                                                 NULL);

            if (exe_module_sp && exe_module_sp->GetObjectFile())
                return error;
            exe_module_sp.reset();
        }
        // No valid architecture was specified or the exact arch wasn't
        // found so ask the platform for the architectures that we should be
        // using (in the correct order) and see if we can find a match that way
        StreamString arch_names;
        for (uint32_t idx = 0; GetSupportedArchitectureAtIndex (idx, resolved_module_spec.GetArchitecture()); ++idx)
        {
            error = ModuleList::GetSharedModule (resolved_module_spec,
                                                 exe_module_sp,
                                                 NULL,
                                                 NULL,
                                                 NULL);
            // Did we find an executable using one of the
            if (error.Success())
            {
                if (exe_module_sp && exe_module_sp->GetObjectFile())
                    break;
                else
                    error.SetErrorToGenericError();
            }

            if (idx > 0)
                arch_names.PutCString (", ");
            arch_names.PutCString (resolved_module_spec.GetArchitecture().GetArchitectureName());
        }

        if (error.Fail() || !exe_module_sp)
        {
            if (resolved_module_spec.GetFileSpec().Readable())
            {
                error.SetErrorStringWithFormat ("'%s' doesn't contain any '%s' platform architectures: %s",
                                                resolved_module_spec.GetFileSpec().GetPath().c_str(),
                                                GetPluginName().GetCString(),
                                                arch_names.GetString().c_str());
            }
            else
            {
                error.SetErrorStringWithFormat("'%s' is not readable", resolved_module_spec.GetFileSpec().GetPath().c_str());
            }
        }
    }
    else
    {
        error.SetErrorStringWithFormat ("'%s' does not exist",
                                        resolved_module_spec.GetFileSpec().GetPath().c_str());
    }

    return error;
}

Error
PlatformRemoteGDBServer::GetFileWithUUID (const FileSpec &platform_file, 
                                          const UUID *uuid_ptr,
                                          FileSpec &local_file)
{
    // Default to the local case
    local_file = platform_file;
    return Error();
}

//------------------------------------------------------------------
/// Default Constructor
//------------------------------------------------------------------
PlatformRemoteGDBServer::PlatformRemoteGDBServer () :
    Platform (false), // This is a remote platform
    m_gdb_client ()
{
}

//------------------------------------------------------------------
/// Destructor.
///
/// The destructor is virtual since this class is designed to be
/// inherited from by the plug-in instance.
//------------------------------------------------------------------
PlatformRemoteGDBServer::~PlatformRemoteGDBServer()
{
}

bool
PlatformRemoteGDBServer::GetSupportedArchitectureAtIndex (uint32_t idx, ArchSpec &arch)
{
    ArchSpec remote_arch = m_gdb_client.GetSystemArchitecture();

    // TODO: 64 bit systems should also advertize support for 32 bit arch
    // unknown CPU, we just support the one arch
    if (idx == 0)
    {
        arch = remote_arch;
        return true;
    }
    return false;
}

size_t
PlatformRemoteGDBServer::GetSoftwareBreakpointTrapOpcode (Target &target, BreakpointSite *bp_site)
{
    // This isn't needed if the z/Z packets are supported in the GDB remote
    // server. But we might need a packet to detect this.
    return 0;
}

bool
PlatformRemoteGDBServer::GetRemoteOSVersion ()
{
    uint32_t major, minor, update;
    if (m_gdb_client.GetOSVersion (major, minor, update))
    {
        m_major_os_version = major;
        m_minor_os_version = minor;
        m_update_os_version = update;
        return true;
    }
    return false;
}

bool
PlatformRemoteGDBServer::GetRemoteOSBuildString (std::string &s)
{
    return m_gdb_client.GetOSBuildString (s);
}

bool
PlatformRemoteGDBServer::GetRemoteOSKernelDescription (std::string &s)
{
    return m_gdb_client.GetOSKernelDescription (s);
}

// Remote Platform subclasses need to override this function
ArchSpec
PlatformRemoteGDBServer::GetRemoteSystemArchitecture ()
{
    return m_gdb_client.GetSystemArchitecture();
}

lldb_private::ConstString
PlatformRemoteGDBServer::GetRemoteWorkingDirectory()
{
    if (IsConnected())
    {
        Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
        std::string cwd;
        if (m_gdb_client.GetWorkingDir(cwd))
        {
            ConstString working_dir(cwd.c_str());
            if (log)
                log->Printf("PlatformRemoteGDBServer::GetRemoteWorkingDirectory() -> '%s'", working_dir.GetCString());
            return working_dir;
        }
        else
        {
            return ConstString();
        }
    }
    else
    {
        return Platform::GetRemoteWorkingDirectory();
    }
}

bool
PlatformRemoteGDBServer::SetRemoteWorkingDirectory(const lldb_private::ConstString &path)
{
    if (IsConnected())
    {
        // Clear the working directory it case it doesn't get set correctly. This will
        // for use to re-read it
        Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
        if (log)
            log->Printf("PlatformRemoteGDBServer::SetRemoteWorkingDirectory('%s')", path.GetCString());
        return m_gdb_client.SetWorkingDir(path.GetCString()) == 0;
    }
    else
        return Platform::SetRemoteWorkingDirectory(path);
}

bool
PlatformRemoteGDBServer::IsConnected () const
{
    return m_gdb_client.IsConnected();
}        

Error
PlatformRemoteGDBServer::ConnectRemote (Args& args)
{
    Error error;
    if (IsConnected())
    {
        error.SetErrorStringWithFormat ("the platform is already connected to '%s', execute 'platform disconnect' to close the current connection", 
                                        GetHostname());
    }
    else
    {
        if (args.GetArgumentCount() == 1)
        {
            const char *url = args.GetArgumentAtIndex(0);
            m_gdb_client.SetConnection (new ConnectionFileDescriptor());

            // we're going to reuse the hostname when we connect to the debugserver
            std::string scheme;
            int port;
            std::string path;
            if ( !UriParser::Parse(url, scheme, m_platform_hostname, port, path) )
            {
                error.SetErrorString("invalid uri");
                return error;
            }

            const ConnectionStatus status = m_gdb_client.Connect(url, &error);
            if (status == eConnectionStatusSuccess)
            {
                if (m_gdb_client.HandshakeWithServer(&error))
                {
                    m_gdb_client.GetHostInfo();
                    // If a working directory was set prior to connecting, send it down now
                    if (m_working_dir)
                        m_gdb_client.SetWorkingDir(m_working_dir.GetCString());
                }
                else
                {
                    m_gdb_client.Disconnect();
                    if (error.Success())
                        error.SetErrorString("handshake failed");
                }
            }
        }
        else
        {
            error.SetErrorString ("\"platform connect\" takes a single argument: <connect-url>");
        }
    }

    return error;
}

Error
PlatformRemoteGDBServer::DisconnectRemote ()
{
    Error error;
    m_gdb_client.Disconnect(&error);
    return error;
}

const char *
PlatformRemoteGDBServer::GetHostname ()
{
    m_gdb_client.GetHostname (m_name);
    if (m_name.empty())
        return NULL;
    return m_name.c_str();
}

const char *
PlatformRemoteGDBServer::GetUserName (uint32_t uid)
{
    // Try and get a cache user name first
    const char *cached_user_name = Platform::GetUserName(uid);
    if (cached_user_name)
        return cached_user_name;
    std::string name;
    if (m_gdb_client.GetUserName(uid, name))
        return SetCachedUserName(uid, name.c_str(), name.size());

    SetUserNameNotFound(uid); // Negative cache so we don't keep sending packets
    return NULL;
}

const char *
PlatformRemoteGDBServer::GetGroupName (uint32_t gid)
{
    const char *cached_group_name = Platform::GetGroupName(gid);
    if (cached_group_name)
        return cached_group_name;
    std::string name;
    if (m_gdb_client.GetGroupName(gid, name))
        return SetCachedGroupName(gid, name.c_str(), name.size());

    SetGroupNameNotFound(gid); // Negative cache so we don't keep sending packets
    return NULL;
}

uint32_t
PlatformRemoteGDBServer::FindProcesses (const ProcessInstanceInfoMatch &match_info,
                                        ProcessInstanceInfoList &process_infos)
{
    return m_gdb_client.FindProcesses (match_info, process_infos);
}

bool
PlatformRemoteGDBServer::GetProcessInfo (lldb::pid_t pid, ProcessInstanceInfo &process_info)
{
    return m_gdb_client.GetProcessInfo (pid, process_info);
}


Error
PlatformRemoteGDBServer::LaunchProcess (ProcessLaunchInfo &launch_info)
{
    Log *log(lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_PLATFORM));
    Error error;

    if (log)
        log->Printf ("PlatformRemoteGDBServer::%s() called", __FUNCTION__);

    auto num_file_actions = launch_info.GetNumFileActions ();
    for (decltype(num_file_actions) i = 0; i < num_file_actions; ++i)
    {
        const auto file_action = launch_info.GetFileActionAtIndex (i);
        if (file_action->GetAction () != FileAction::eFileActionOpen)
            continue;
        switch(file_action->GetFD())
        {
        case STDIN_FILENO:
            m_gdb_client.SetSTDIN (file_action->GetPath());
            break;
        case STDOUT_FILENO:
            m_gdb_client.SetSTDOUT (file_action->GetPath());
            break;
        case STDERR_FILENO:
            m_gdb_client.SetSTDERR (file_action->GetPath());
            break;
        }
    }

    m_gdb_client.SetDisableASLR (launch_info.GetFlags().Test (eLaunchFlagDisableASLR));
    m_gdb_client.SetDetachOnError (launch_info.GetFlags().Test (eLaunchFlagDetachOnError));
    
    const char *working_dir = launch_info.GetWorkingDirectory();
    if (working_dir && working_dir[0])
    {
        m_gdb_client.SetWorkingDir (working_dir);
    }
    
    // Send the environment and the program + arguments after we connect
    const char **envp = launch_info.GetEnvironmentEntries().GetConstArgumentVector();

    if (envp)
    {
        const char *env_entry;
        for (int i=0; (env_entry = envp[i]); ++i)
        {
            if (m_gdb_client.SendEnvironmentPacket(env_entry) != 0)
                break;
        }
    }
    
    ArchSpec arch_spec = launch_info.GetArchitecture();
    const char *arch_triple = arch_spec.GetTriple().str().c_str();
    
    m_gdb_client.SendLaunchArchPacket(arch_triple);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::%s() set launch architecture triple to '%s'", __FUNCTION__, arch_triple ? arch_triple : "<NULL>");

    int arg_packet_err;
    {
        // Scope for the scoped timeout object
        GDBRemoteCommunication::ScopedTimeout timeout(m_gdb_client, 5);
        arg_packet_err = m_gdb_client.SendArgumentsPacket (launch_info);
    }

    if (arg_packet_err == 0)
    {
        std::string error_str;
        if (m_gdb_client.GetLaunchSuccess (error_str))
        {
            const auto pid = m_gdb_client.GetCurrentProcessID (false);
            if (pid != LLDB_INVALID_PROCESS_ID)
            {
                launch_info.SetProcessID (pid);
                if (log)
                    log->Printf ("PlatformRemoteGDBServer::%s() pid %" PRIu64 " launched successfully", __FUNCTION__, pid);
            }
            else
            {
                if (log)
                    log->Printf ("PlatformRemoteGDBServer::%s() launch succeeded but we didn't get a valid process id back!", __FUNCTION__);
                error.SetErrorString ("failed to get PID");
            }
        }
        else
        {
            error.SetErrorString (error_str.c_str());
            if (log)
                log->Printf ("PlatformRemoteGDBServer::%s() launch failed: %s", __FUNCTION__, error.AsCString ());
        }
    }
    else
    {
        error.SetErrorStringWithFormat("'A' packet returned an error: %i", arg_packet_err);
    }
    return error;
}

Error
PlatformRemoteGDBServer::KillProcess (const lldb::pid_t pid)
{
    if (!KillSpawnedProcess(pid))
        return Error("failed to kill remote spawned process");
    return Error();
}

lldb::ProcessSP
PlatformRemoteGDBServer::DebugProcess (lldb_private::ProcessLaunchInfo &launch_info,
                                       lldb_private::Debugger &debugger,
                                       lldb_private::Target *target,       // Can be NULL, if NULL create a new target, else use existing one
                                       lldb_private::Error &error)
{
    lldb::ProcessSP process_sp;
    if (IsRemote())
    {
        if (IsConnected())
        {
            lldb::pid_t debugserver_pid = LLDB_INVALID_PROCESS_ID;
            uint16_t port = LaunchGDBserverAndGetPort(debugserver_pid);

            if (port == 0)
            {
                error.SetErrorStringWithFormat ("unable to launch a GDB server on '%s'", GetHostname ());
            }
            else
            {
                if (target == NULL)
                {
                    TargetSP new_target_sp;
                    
                    error = debugger.GetTargetList().CreateTarget (debugger,
                                                                   NULL,
                                                                   NULL,
                                                                   false,
                                                                   NULL,
                                                                   new_target_sp);
                    target = new_target_sp.get();
                }
                else
                    error.Clear();
                
                if (target && error.Success())
                {
                    debugger.GetTargetList().SetSelectedTarget(target);
                    
                    // The darwin always currently uses the GDB remote debugger plug-in
                    // so even when debugging locally we are debugging remotely!
                    process_sp = target->CreateProcess (launch_info.GetListenerForProcess(debugger), "gdb-remote", NULL);
                    
                    if (process_sp)
                    {
                        std::string connect_url = MakeGdbServerUrl(m_platform_hostname, port);
                        error = process_sp->ConnectRemote (nullptr, connect_url.c_str());
                        // Retry the connect remote one time...
                        if (error.Fail())
                            error = process_sp->ConnectRemote (nullptr, connect_url.c_str());
                        if (error.Success())
                            error = process_sp->Launch(launch_info);
                        else if (debugserver_pid != LLDB_INVALID_PROCESS_ID)
                        {
                            printf ("error: connect remote failed (%s)\n", error.AsCString());
                            KillSpawnedProcess(debugserver_pid);
                        }
                    }
                }
            }
        }
        else
        {
            error.SetErrorString("not connected to remote gdb server");
        }
    }
    return process_sp;

}

uint16_t
PlatformRemoteGDBServer::LaunchGDBserverAndGetPort (lldb::pid_t &pid)
{
    ArchSpec remote_arch = GetRemoteSystemArchitecture ();
    llvm::Triple &remote_triple = remote_arch.GetTriple ();
    if (remote_triple.getVendor () == llvm::Triple::Apple && remote_triple.getOS () == llvm::Triple::IOS)
    {
        // When remote debugging to iOS, we use a USB mux that always talks
        // to localhost, so we will need the remote debugserver to accept connections
        // only from localhost, no matter what our current hostname is
        return m_gdb_client.LaunchGDBserverAndGetPort (pid, "127.0.0.1");
    }
    else
    {
        // All other hosts should use their actual hostname
        return m_gdb_client.LaunchGDBserverAndGetPort (pid, NULL);
    }
}

bool
PlatformRemoteGDBServer::KillSpawnedProcess (lldb::pid_t pid)
{
    return m_gdb_client.KillSpawnedProcess (pid);
}

lldb::ProcessSP
PlatformRemoteGDBServer::Attach (lldb_private::ProcessAttachInfo &attach_info,
                                 Debugger &debugger,
                                 Target *target,       // Can be NULL, if NULL create a new target, else use existing one
                                 Error &error)
{
    lldb::ProcessSP process_sp;
    if (IsRemote())
    {
        if (IsConnected())
        {
            lldb::pid_t debugserver_pid = LLDB_INVALID_PROCESS_ID;
            uint16_t port = LaunchGDBserverAndGetPort(debugserver_pid);

            if (port == 0)
            {
                error.SetErrorStringWithFormat ("unable to launch a GDB server on '%s'", GetHostname ());
            }
            else
            {
                if (target == NULL)
                {
                    TargetSP new_target_sp;
                    
                    error = debugger.GetTargetList().CreateTarget (debugger,
                                                                   NULL,
                                                                   NULL, 
                                                                   false,
                                                                   NULL,
                                                                   new_target_sp);
                    target = new_target_sp.get();
                }
                else
                    error.Clear();
                
                if (target && error.Success())
                {
                    debugger.GetTargetList().SetSelectedTarget(target);
                    
                    // The darwin always currently uses the GDB remote debugger plug-in
                    // so even when debugging locally we are debugging remotely!
                    process_sp = target->CreateProcess (attach_info.GetListenerForProcess(debugger), "gdb-remote", NULL);
                    
                    if (process_sp)
                    {
                        std::string connect_url = MakeGdbServerUrl(m_platform_hostname, port);
                        error = process_sp->ConnectRemote(nullptr, connect_url.c_str());
                        if (error.Success())
                        {
                            auto listener = attach_info.GetHijackListener();
                            if (listener != nullptr)
                                process_sp->HijackProcessEvents(listener.get());
                            error = process_sp->Attach(attach_info);
                        }

                        if (error.Fail() && debugserver_pid != LLDB_INVALID_PROCESS_ID)
                        {
                            KillSpawnedProcess(debugserver_pid);
                        }
                    }
                }
            }
        }
        else
        {
            error.SetErrorString("not connected to remote gdb server");
        }
    }
    return process_sp;
}

Error
PlatformRemoteGDBServer::MakeDirectory (const char *path, uint32_t mode)
{
    Error error = m_gdb_client.MakeDirectory(path,mode);
    Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::MakeDirectory(path='%s', mode=%o) error = %u (%s)", path, mode, error.GetError(), error.AsCString());
    return error;
}


Error
PlatformRemoteGDBServer::GetFilePermissions (const char *path, uint32_t &file_permissions)
{
    Error error = m_gdb_client.GetFilePermissions(path, file_permissions);
    Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::GetFilePermissions(path='%s', file_permissions=%o) error = %u (%s)", path, file_permissions, error.GetError(), error.AsCString());
    return error;
}

Error
PlatformRemoteGDBServer::SetFilePermissions (const char *path, uint32_t file_permissions)
{
    Error error = m_gdb_client.SetFilePermissions(path, file_permissions);
    Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::SetFilePermissions(path='%s', file_permissions=%o) error = %u (%s)", path, file_permissions, error.GetError(), error.AsCString());
    return error;
}


lldb::user_id_t
PlatformRemoteGDBServer::OpenFile (const lldb_private::FileSpec& file_spec,
                                   uint32_t flags,
                                   uint32_t mode,
                                   Error &error)
{
    return m_gdb_client.OpenFile (file_spec, flags, mode, error);
}

bool
PlatformRemoteGDBServer::CloseFile (lldb::user_id_t fd, Error &error)
{
    return m_gdb_client.CloseFile (fd, error);
}

lldb::user_id_t
PlatformRemoteGDBServer::GetFileSize (const lldb_private::FileSpec& file_spec)
{
    return m_gdb_client.GetFileSize(file_spec);
}

uint64_t
PlatformRemoteGDBServer::ReadFile (lldb::user_id_t fd,
                                   uint64_t offset,
                                   void *dst,
                                   uint64_t dst_len,
                                   Error &error)
{
    return m_gdb_client.ReadFile (fd, offset, dst, dst_len, error);
}

uint64_t
PlatformRemoteGDBServer::WriteFile (lldb::user_id_t fd,
                                    uint64_t offset,
                                    const void* src,
                                    uint64_t src_len,
                                    Error &error)
{
    return m_gdb_client.WriteFile (fd, offset, src, src_len, error);
}

lldb_private::Error
PlatformRemoteGDBServer::PutFile (const lldb_private::FileSpec& source,
         const lldb_private::FileSpec& destination,
         uint32_t uid,
         uint32_t gid)
{
    return Platform::PutFile(source,destination,uid,gid);
}

Error
PlatformRemoteGDBServer::CreateSymlink (const char *src,    // The name of the link is in src
                                        const char *dst)    // The symlink points to dst
{
    Error error = m_gdb_client.CreateSymlink (src, dst);
    Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::CreateSymlink(src='%s', dst='%s') error = %u (%s)", src, dst, error.GetError(), error.AsCString());
    return error;
}

Error
PlatformRemoteGDBServer::Unlink (const char *path)
{
    Error error = m_gdb_client.Unlink (path);
    Log *log = GetLogIfAnyCategoriesSet(LIBLLDB_LOG_PLATFORM);
    if (log)
        log->Printf ("PlatformRemoteGDBServer::Unlink(path='%s') error = %u (%s)", path, error.GetError(), error.AsCString());
    return error;
}

bool
PlatformRemoteGDBServer::GetFileExists (const lldb_private::FileSpec& file_spec)
{
    return m_gdb_client.GetFileExists (file_spec);
}

lldb_private::Error
PlatformRemoteGDBServer::RunShellCommand (const char *command,           // Shouldn't be NULL
                                          const char *working_dir,       // Pass NULL to use the current working directory
                                          int *status_ptr,               // Pass NULL if you don't want the process exit status
                                          int *signo_ptr,                // Pass NULL if you don't want the signal that caused the process to exit
                                          std::string *command_output,   // Pass NULL if you don't want the command output
                                          uint32_t timeout_sec)          // Timeout in seconds to wait for shell program to finish
{
    return m_gdb_client.RunShellCommand (command, working_dir, status_ptr, signo_ptr, command_output, timeout_sec);
}

void
PlatformRemoteGDBServer::CalculateTrapHandlerSymbolNames ()
{   
    m_trap_handlers.push_back (ConstString ("_sigtramp"));
}   
