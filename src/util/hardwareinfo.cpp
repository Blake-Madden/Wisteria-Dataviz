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
#include "wx/dir.h"
#include "wx/wxprec.h"

#ifndef WX_PRECOMP
    #include "wx/thread.h"
#endif // WX_PRECOMP

#ifdef __WXMSW__
    #include <dxgi.h>
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
    #include <wx/dir.h>
    #include <wx/file.h>
    #include <wx/textfile.h>
#endif

namespace wxSystemHardwareInfo
    {
    //----------------------------------------------------------
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
            // NOLINTNEXTLINE(cppcoreguidelines-pro-type-union-access)
            return usage.ru_maxrss * 1024;
            }
#endif
        return -1;
        }

    //----------------------------------------------------------
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

    //----------------------------------------------------------
    std::vector<wxString> GetDRMDevicePaths()
        {
        std::vector<wxString> paths;
        const wxString drmDir(L"/sys/class/drm");
        wxDir dir(drmDir);
        if (!dir.IsOpened())
            {
            return paths;
            }

        wxString filename;
        bool cont = dir.GetFirst(&filename, L"card*", wxDIR_DIRS);
        while (cont)
            {
            wxString devicePath = drmDir + L"/" + filename + L"/device";
            if (wxDirExists(devicePath))
                {
                paths.push_back(devicePath);
                }
            cont = dir.GetNext(&filename);
            }
        return paths;
        }

    //----------------------------------------------------------
    wxString GetGPUDescription()
        {
#ifdef __WXMSW__
        IDXGIFactory* factory{ nullptr };
        if (SUCCEEDED(
                CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
            {
            IDXGIAdapter* adapter{ nullptr };
            if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
                {
                DXGI_ADAPTER_DESC desc{};
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                    {
                    adapter->Release();
                    factory->Release();
                    return wxString(desc.Description);
                    }
                adapter->Release();
                }
            factory->Release();
            }
#elif defined(__UNIX__) && !defined(__APPLE__)
        // try standard DRM sysfs first...
        const std::vector<wxString> devicePaths = GetDRMDevicePaths();
        wxString fallbackVendor, fallbackDevice;
        for (const auto& base : devicePaths)
            {
            wxString vendorId, deviceId;

            const wxString vendorPath = base + L"/vendor";
            const wxString devicePath = base + L"/device";
            if (wxFileExists(vendorPath) && wxFileExists(devicePath))
                {
                if (wxFile vendorFile(vendorPath); vendorFile.IsOpened())
                    {
                    vendorFile.ReadAll(&vendorId);
                    }
                if (wxFile deviceFile(devicePath); deviceFile.IsOpened())
                    {
                    deviceFile.ReadAll(&deviceId);
                    }
                }

            vendorId.Trim();
            deviceId.Trim();

            if (!vendorId.empty() && !deviceId.empty())
                {
                if (vendorId == L"0x10de")
                    {
                    return L"NVIDIA (DRM)";
                    }
                if (vendorId == L"0x1002")
                    {
                    return L"AMD";
                    }
                if (vendorId == L"0x8086")
                    {
                    return L"Intel";
                    }
                // save first unrecognized vendor as fallback
                if (fallbackVendor.empty())
                    {
                    fallbackVendor = vendorId;
                    fallbackDevice = deviceId;
                    }
                }
            }

        if (!fallbackVendor.empty())
            {
            return wxString::Format(L"%s:%s", fallbackVendor, fallbackDevice);
            }

        // ...fallback for NVIDIA Proprietary Driver
        if (wxDirExists(L"/proc/driver/nvidia"))
            {
            return L"NVIDIA";
            }
#endif
        return {};
        }

    //----------------------------------------------------------
    wxMemorySize GetGPUDedicatedVRAM()
        {
#ifdef __WXMSW__
        IDXGIFactory* factory{ nullptr };
        if (SUCCEEDED(
                CreateDXGIFactory(__uuidof(IDXGIFactory), reinterpret_cast<void**>(&factory))))
            {
            IDXGIAdapter* adapter{ nullptr };
            if (SUCCEEDED(factory->EnumAdapters(0, &adapter)))
                {
                DXGI_ADAPTER_DESC desc{};
                if (SUCCEEDED(adapter->GetDesc(&desc)))
                    {
                    adapter->Release();
                    factory->Release();
                    return static_cast<wxMemorySize>(desc.DedicatedVideoMemory);
                    }
                adapter->Release();
                }
            factory->Release();
            }
#elif defined(__UNIX__) && !defined(__APPLE__)
        const std::vector<wxString> devicePaths = GetDRMDevicePaths();
        for (const auto& base : devicePaths)
            {
            wxString vendorId, deviceId;

            const wxString vendorPath = base + L"/vendor";
            const wxString devicePath = base + L"/device";
            if (wxFileExists(vendorPath) && wxFileExists(devicePath))
                {
                if (wxFile vendorFile(vendorPath); vendorFile.IsOpened())
                    {
                    vendorFile.ReadAll(&vendorId);
                    }
                if (wxFile deviceFile(devicePath); deviceFile.IsOpened())
                    {
                    deviceFile.ReadAll(&deviceId);
                    }
                }

            vendorId.Trim();
            deviceId.Trim();

            if (vendorId.empty() || deviceId.empty())
                {
                continue;
                }

            // try VRAM
            const wxString vramPath = base + L"/mem_info_vram_total";
            if (wxFileExists(vramPath))
                {
                wxFile vramFile(vramPath);
                if (vramFile.IsOpened())
                    {
                    wxString content;
                    if (vramFile.ReadAll(&content))
                        {
                        unsigned long long vram = 0;
                        if (content.Trim().ToULongLong(&vram) && vram > 0)
                            {
                            return static_cast<wxMemorySize>(vram);
                            }
                        }
                    }
                }

            // some drivers expose memory differently
            const wxString memInfoPath = base + L"/mem_info_vis_vram_total";
            if (wxFileExists(memInfoPath))
                {
                wxFile memInfo(memInfoPath);
                if (memInfo.IsOpened())
                    {
                    wxString content;
                    if (memInfo.ReadAll(&content))
                        {
                        unsigned long long vram = 0;
                        if (content.Trim().ToULongLong(&vram) && vram > 0)
                            {
                            return static_cast<wxMemorySize>(vram);
                            }
                        }
                    }
                }
            }
#endif
        return -1;
        }
    } // namespace wxSystemHardwareInfo
