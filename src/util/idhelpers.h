/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __ID_HELPERS_H__
#define __ID_HELPERS_H__

#include <wx/wx.h>

/// @brief Enables/disables all items with the specified ID, not just the first one that is found.
/// @param menu The menu to edit.
/// @param id The menu ID.
/// @param enable @c true to enable, @c false to disable.
void MenuEnableItem(wxMenu* menu, const wxWindowID id, const bool enable);

/// @brief Enables/disables all items with the specified ID, not just the first one that is found.
/// @param menuBar The menubar to edit.
/// @param id The menu ID.
/// @param enable @c true to enable, @c false to disable.
void MenuBarEnableAll(wxMenuBar* menuBar, const wxWindowID id, const bool enable);

/// @brief Locks a range of IDs that won't be assigned to anything else.
/// @details This is useful for menu IDs that persist throughout the program's lifetime.
///     The IDs will begin at @c wxID_HIGHEST, and each subsequent object's range will
///     be incremented beyond that.
class IdRangeLock
    {
public:
    /** @brief Constructor.
        @param idCount The number of IDs to use in the range.*/
    explicit IdRangeLock(const size_t idCount)
        {
        m_rangeBegin = m_firstId = m_startingId;
        wxWindowID i = m_rangeBegin;
        for (/*already defined*/; static_cast<size_t>(i) < m_rangeBegin+idCount; ++i)
            { wxRegisterId(i); }
        m_rangeEnd = i-1;
        m_startingId = i; // where the next menu id range lock will begin
        }
    /// @returns The first available ID.
    [[nodiscard]] wxWindowID GetFirstId() const noexcept
        { return m_firstId; }
    /// @returns The last available ID.
    [[nodiscard]] wxWindowID GetLastId() const noexcept
        { return m_rangeEnd; }
    /// @returns The next available ID.\n
    ///     Will return @c wxNOT_FOUND if no more IDs are available.
    [[nodiscard]] wxWindowID GetNextId() noexcept
        {
        return (m_rangeBegin > m_rangeEnd) ?
            wxNOT_FOUND : m_rangeBegin++;
        }
private:
    static wxWindowID m_startingId;
    wxWindowID m_firstId{ 0 };
    wxWindowID m_rangeBegin{ 0 };
    wxWindowID m_rangeEnd{ 0 };
    };

/// @brief Keeps track of a range IDs, but does not lock them.
class IdRange
    {
public:
    /** @brief Constructor.
        @param startId The start of the ID range.
        @param idCount The number of IDs to use in the range.*/
    IdRange(const wxWindowID startId, const size_t idCount) noexcept :
        m_firstId(startId), m_rangeBegin(startId), m_rangeEnd(startId+idCount)
        {}
    /// @returns The first available ID.
    [[nodiscard]] wxWindowID GetFirstId() const noexcept
        { return m_firstId; }
    /// @returns The last available ID.
    [[nodiscard]] wxWindowID GetLastId() const noexcept
        { return m_rangeEnd; }
    /// @returns The next available ID.\n
    ///     Will return @c wxNOT_FOUND if no more IDs are available.
    [[nodiscard]] wxWindowID GetNextId() noexcept
        {
        return (m_rangeBegin > m_rangeEnd) ?
            wxNOT_FOUND : m_rangeBegin++;
        }
private:
    wxWindowID m_firstId{ 0 };
    wxWindowID m_rangeBegin{ 0 };
    wxWindowID m_rangeEnd{ 0 };
    };

/** @}*/

#endif //__ID_HELPERS_H__
