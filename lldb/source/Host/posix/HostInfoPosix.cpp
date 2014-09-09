//===-- HostInfoPosix.cpp ---------------------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

#include "lldb/lldb-python.h"

#include "lldb/Core/Log.h"
#include "lldb/Host/posix/HostInfoPosix.h"

#include "llvm/ADT/SmallString.h"
#include "llvm/Support/raw_ostream.h"

#include <grp.h>
#include <limits.h>
#include <netdb.h>
#include <pwd.h>
#include <sys/types.h>
#include <unistd.h>

using namespace lldb_private;

size_t
HostInfoPosix::GetPageSize()
{
    return ::getpagesize();
}

bool
HostInfoPosix::GetHostname(std::string &s)
{
    char hostname[PATH_MAX];
    hostname[sizeof(hostname) - 1] = '\0';
    if (::gethostname(hostname, sizeof(hostname) - 1) == 0)
    {
        struct hostent *h = ::gethostbyname(hostname);
        if (h)
            s.assign(h->h_name);
        else
            s.assign(hostname);
        return true;
    }
    return false;
}

const char *
HostInfoPosix::LookupUserName(uint32_t uid, std::string &user_name)
{
    struct passwd user_info;
    struct passwd *user_info_ptr = &user_info;
    char user_buffer[PATH_MAX];
    size_t user_buffer_size = sizeof(user_buffer);
    if (::getpwuid_r(uid, &user_info, user_buffer, user_buffer_size, &user_info_ptr) == 0)
    {
        if (user_info_ptr)
        {
            user_name.assign(user_info_ptr->pw_name);
            return user_name.c_str();
        }
    }
    user_name.clear();
    return NULL;
}

const char *
HostInfoPosix::LookupGroupName(uint32_t gid, std::string &group_name)
{
    char group_buffer[PATH_MAX];
    size_t group_buffer_size = sizeof(group_buffer);
    struct group group_info;
    struct group *group_info_ptr = &group_info;
    // Try the threadsafe version first
    if (::getgrgid_r(gid, &group_info, group_buffer, group_buffer_size, &group_info_ptr) == 0)
    {
        if (group_info_ptr)
        {
            group_name.assign(group_info_ptr->gr_name);
            return group_name.c_str();
        }
    }
    else
    {
        // The threadsafe version isn't currently working for me on darwin, but the non-threadsafe version
        // is, so I am calling it below.
        group_info_ptr = ::getgrgid(gid);
        if (group_info_ptr)
        {
            group_name.assign(group_info_ptr->gr_name);
            return group_name.c_str();
        }
    }
    group_name.clear();
    return NULL;
}

uint32_t
HostInfoPosix::GetUserID()
{
    return getuid();
}

uint32_t
HostInfoPosix::GetGroupID()
{
    return getgid();
}

uint32_t
HostInfoPosix::GetEffectiveUserID()
{
    return geteuid();
}

uint32_t
HostInfoPosix::GetEffectiveGroupID()
{
    return getegid();
}

bool
HostInfoPosix::ComputeSupportExeDirectory(FileSpec &file_spec)
{
    Log *log = lldb_private::GetLogIfAllCategoriesSet(LIBLLDB_LOG_HOST);

    FileSpec lldb_file_spec;
    if (!GetLLDBPath(lldb::ePathTypeLLDBShlibDir, lldb_file_spec))
        return false;

    char raw_path[PATH_MAX];
    lldb_file_spec.GetPath(raw_path, sizeof(raw_path));

    // Most Posix systems (e.g. Linux/*BSD) will attempt to replace a */lib with */bin as the base
    // directory for helper exe programs.  This will fail if the /lib and /bin directories are
    // rooted in entirely different trees.
    if (log)
        log->Printf("HostInfoPosix::ComputeSupportExeDirectory() attempting to derive the bin path (ePathTypeSupportExecutableDir) from "
                    "this path: %s",
                    raw_path);
    char *lib_pos = ::strstr(raw_path, "/lib");
    if (lib_pos != nullptr)
    {
        // First terminate the raw path at the start of lib.
        *lib_pos = '\0';

        // Now write in bin in place of lib.
        ::strncpy(lib_pos, "/bin", PATH_MAX - (lib_pos - raw_path));

        if (log)
            log->Printf("Host::%s() derived the bin path as: %s", __FUNCTION__, raw_path);
    }
    else
    {
        if (log)
            log->Printf("Host::%s() failed to find /lib/liblldb within the shared lib path, bailing on bin path construction",
                        __FUNCTION__);
    }
    file_spec.GetDirectory().SetCString(raw_path);
    return (bool)file_spec.GetDirectory();
}

bool
HostInfoPosix::ComputeHeaderDirectory(FileSpec &file_spec)
{
    FileSpec temp_file("/opt/local/include/lldb", false);
    file_spec.GetDirectory().SetCString(temp_file.GetPath().c_str());
    return true;
}

bool
HostInfoPosix::ComputePythonDirectory(FileSpec &file_spec)
{
#ifndef LLDB_DISABLE_PYTHON
    FileSpec lldb_file_spec;
    if (!GetLLDBPath(lldb::ePathTypeLLDBShlibDir, lldb_file_spec))
        return false;

    char raw_path[PATH_MAX];
    lldb_file_spec.GetPath(raw_path, sizeof(raw_path));

    llvm::SmallString<256> python_version_dir;
    llvm::raw_svector_ostream os(python_version_dir);
    os << "/python" << PY_MAJOR_VERSION << '.' << PY_MINOR_VERSION << "/site-packages";
    os.flush();

    // We may get our string truncated. Should we protect this with an assert?
    ::strncat(raw_path, python_version_dir.c_str(), sizeof(raw_path) - strlen(raw_path) - 1);

    file_spec.GetDirectory().SetCString(raw_path);
    return true;
#else
    return false;
#endif
}
