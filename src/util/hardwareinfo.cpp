///////////////////////////////////////////////////////////////////////////////
// Name:        hardwareinfo.h
// Purpose:     declaration of the wxSystemHardwareInfo class
// Author:      Blake Madden
// Created:     2024-07-09
// Copyright:   (c) 2024 Blake Madden
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

// ============================================================================
// declarations
// ============================================================================

// ----------------------------------------------------------------------------
// headers
// ----------------------------------------------------------------------------

// for compilers that support precompilation, includes "wx.h".
#include "hardwareinfo.h"
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/thread.h"
#endif // WX_PRECOMP

#ifdef __WXMSW__
    #include <psapi.h>
    #include <sysinfoapi.h>
    #include <windows.h>
#elif defined(__APPLE__)
    #include <mach/mach_init.h>
    #include <mach/message.h>
    #include <mach/task.h>
    #include <mach/task_info.h>
    #include <sys/sysctl.h>
#elif defined(__UNIX__)
    #include <sys/resource.h>
    #include <sys/sysinfo.h>
#endif

namespace wxSystemHardwareInfo
    {
    wxMemorySize GetPeakUsedMemory()
        {
#ifdef __WXMSW__
        // https://docs.microsoft.com/en-us/windows/win32/psapi/collecting-memory-usage-information-for-a-process?redirectedfrom=MSDN
        PROCESS_MEMORY_COUNTERS memCounter;
        ::ZeroMemory(&memCounter, sizeof(PROCESS_MEMORY_COUNTERS));
        if (::GetProcessMemoryInfo(::GetCurrentProcess(), &memCounter, sizeof(memCounter)))
            {
            // PeakWorkingSetSize is in bytes
            return memCounter.PeakWorkingSetSize;
            }
#elif defined(__APPLE__)
        struct task_basic_info info{};
        mach_msg_type_number_t count = TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), TASK_BASIC_INFO, reinterpret_cast<task_info_t>(&info),
                      &count) == KERN_SUCCESS)
            {
            return info.resident_size;
            }
#elif defined(__UNIX__)
        rusage usage{};
        memset(&usage, 0, sizeof(rusage));
        if (getrusage(RUSAGE_SELF, &usage) == 0)
            {
            // ru_maxrss is in kilobytes
            return usage.ru_maxrss * 1024;
            }
#endif
        return -1;
        }

    wxMemorySize GetMemory()
        {
#ifdef __WXMSW__
        MEMORYSTATUSEX status{};
        status.dwLength = sizeof(status);
        if (::GlobalMemoryStatusEx(&status))
            {
            return status.ullTotalPhys;
            }
#elif defined(__APPLE__)
        int64_t memSize = 0;
        size_t sizeOfRetVal = sizeof(memSize);
        if (sysctlbyname("hw.memsize", &memSize, &sizeOfRetVal, nullptr, 0) != -1)
            {
            return memSize;
            }
#elif defined(__UNIX__)
        struct sysinfo status{};
        if (sysinfo(&status) == 0)
            {
            return static_cast<wxMemorySize>(status.totalram) * status.mem_unit;
            }
#endif

        return -1;
        }
    } // namespace wxSystemHardwareInfo
