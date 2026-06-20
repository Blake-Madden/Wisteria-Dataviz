///////////////////////////////////////////////////////////////////////////////
// Name:        wx/hardwareinfo.h
// Purpose:     declaration of the wxSystemHardwareInfo class
// Author:      Blake Madden
// Created:     2024-07-09
// Copyright:   (c) 2024 Blake Madden
// Licence:     wxWindows licence
///////////////////////////////////////////////////////////////////////////////

#ifndef WX_SYSTEMHWINFO_
#define WX_SYSTEMHWINFO_

#include "wx/power.h"
#include "wx/string.h"
#include "wx/utils.h"

// ----------------------------------------------------------------------------
// wxSystemHardwareInfo
// ----------------------------------------------------------------------------

/// @brief Information about the system's hardware.
namespace wxSystemHardwareInfo
    {
    /// @returns The number of CPU cores/threads available.
    inline int GetCPUCount() { return wxThread::GetCPUCount(); }

    /// @returns The CPU architecture name (as reported by the OS emulation layer).
    inline wxString GetCPUArchitectureName() { return wxGetCpuArchitectureName(); }

    /// @returns The native CPU architecture name of the machine.
    inline wxString GetNativeCPUArchitectureName() { return wxGetNativeCpuArchitectureName(); }

    /// @returns @c true if the process is running under the native CPU architecture
    ///     (i.e., not emulated).
    inline bool IsRunningNatively()
        {
        return wxGetCpuArchitectureName() == wxGetNativeCpuArchitectureName();
        }

    /// @brief Returns the total physical memory size.
    /// @returns The total physical memory size.
    wxMemorySize GetMemory();

    /// @brief Returns the peak memory used so far by the process.
    /// @returns The peak memory usage.
    wxMemorySize GetPeakUsedMemory();

    /// @returns The amount of free memory available.
    inline wxMemorySize GetFreeMemory() { return wxGetFreeMemory(); }

    /// @returns The primary GPU description (name/model), or an empty string if unavailable.
    /// @note Windows only.
    wxString GetGPUDescription();

    /// @returns The dedicated VRAM of the primary GPU in bytes, or -1 if unavailable.
    /// @note Windows only.
    wxMemorySize GetGPUDedicatedVRAM();
    } // namespace wxSystemHardwareInfo

#endif // WX_SYSTEMHWINFO_
