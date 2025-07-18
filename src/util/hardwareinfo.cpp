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
    #include <sysinfoapi.h>
    #include <windows.h>
#elif defined(__APPLE__)
    #include <sys/sysctl.h>
#elif defined(__UNIX__)
    #include <sys/resource.h>
    #include <sys/sysinfo.h>
#endif

namespace wxSystemHardwareInfo
    {
    wxMemorySize GetMemory()
        {
        wxMemorySize physicalMemory = -1;

#ifdef __WXMSW__
        MEMORYSTATUSEX status{};
        status.dwLength = sizeof(status);
        if (::GlobalMemoryStatusEx(&status))
            {
            physicalMemory = status.ullTotalPhys;
            }
#elif defined(__APPLE__)
        int64_t memSize = 0;
        size_t sizeOfRetVal = sizeof(memSize);
        if (sysctlbyname("hw.memsize", &memSize, &sizeOfRetVal, nullptr, 0) != -1)
            {
            physicalMemory = memSize;
            }
#elif defined(__UNIX__)
        struct sysinfo status{};
        if (sysinfo(&status) == 0)
            {
            physicalMemory = status.totalram;
            }
#endif

        return physicalMemory;
        }
    } // namespace wxSystemHardwareInfo
