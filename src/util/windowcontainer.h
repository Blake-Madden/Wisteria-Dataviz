/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WINDOW_CONTAINER_H__
#define __WINDOW_CONTAINER_H__

#include <wx/wx.h>
#include <vector>

/// @brief Container class that keeps track of a list of windows.
/// @details This includes helper functions for accessing windows based
///     on ID, name, class type, or even combinations of the three.
class WindowContainer
    {
public:
    /// @brief Adds a window to the container.
    /// @param window The window to add.
    void AddWindow(wxWindow* window)
        {
        // don't add any duplicate or null windows
        if (!window)
            { return; }
        for (auto pos = m_windows.cbegin();
            pos != m_windows.cend();
            ++pos)
            {
            if (*pos == window)
                { return; }
            }
        m_windows.push_back(window);
        }
    /// @brief Inserts a window into the container at a give position.
    /// @param position The position to insert the window.
    /// @param window The window to add.
    void InsertWindow(const int position, wxWindow* window)
        {
        // don't add any duplicate or null windows
        if (!window)
            { return; }
        for (auto pos = m_windows.cbegin();
            pos != m_windows.cend();
            ++pos)
            {
            if (*pos == window)
                { return; }
            }
        wxASSERT_MSG(static_cast<size_t>(position) <= GetWindowCount(),
            wxString(__WXFUNCTION__) + L": position is larger than container.");
        // shouldn't happen, but work around a bad insert
        if (static_cast<size_t>(position) > GetWindowCount())
            { AddWindow(window); }
        else
            { m_windows.insert(m_windows.begin()+position, window); }
        }
    /// @brief Removes the first window with the given ID.
    /// @param id The id to search for.
    /// @returns @c true if successfully removed.
    bool RemoveWindowById(const wxWindowID id)
        {
        for (auto pos = m_windows.cbegin();
             pos != m_windows.cend();
             ++pos)
            {
            if (*pos && (*pos)->GetId() == id)
                {
                m_windows.erase(pos);
                return true;
                }
            }
        return false;
        }
    /// @brief Removes the first window with the given ID and label.
    /// @param id The id to search for.
    /// @param label The label to search for.
    /// @returns @c true if successfully removed.
    bool RemoveWindowByIdAndLabel(const wxWindowID id, const wxString& label)
        {
        for (auto pos = m_windows.cbegin();
             pos != m_windows.cend();
             ++pos)
            {
            if (*pos && (*pos)->GetId() == id && (*pos)->GetName().CmpNoCase(label) == 0)
                {
                m_windows.erase(pos);
                return true;
                }
            }
        return false;
        }
    /// @brief Removes all windows from the container.
    void Clear() noexcept
        { m_windows.clear(); }
    /// @returns The window at the given index, or null if out of range.
    /// @param position The position of the window to get.
    [[nodiscard]]
    wxWindow* GetWindow(const int position)
        {
        return (static_cast<size_t>(position) >= GetWindowCount()) ?
                nullptr : m_windows[position];
        }
    /// @returns The window with @c id, or null if not found.
    /// @param id The window ID to search for.
    [[nodiscard]]
    wxWindow* FindWindowById(const wxWindowID id)
        {
        for (auto pos = m_windows.begin();
             pos != m_windows.end();
             ++pos)
            {
            if (*pos && (*pos)->GetId() == id)
                { return *pos; }
            }
        return nullptr;
        }
    /// @returns The window with @c id that is also the same class type as
    ///     @c classInfo, or null if not found.
    /// @param id The window ID to search for.
    /// @param classInfo The class information that the window should match.
    [[nodiscard]]
    wxWindow* FindWindowById(const wxWindowID id, const wxClassInfo* classInfo)
        {
        wxASSERT_LEVEL_2(classInfo);
        for (auto pos = m_windows.begin();
             pos != m_windows.end();
             ++pos)
            {
            if (*pos && ((*pos)->GetId() == id) && (*pos)->IsKindOf(classInfo))
                { return *pos; }
            }
        return nullptr;
        }
    /// @returns The window that matches a window ID and label, or null if not found.
    /// @param id The id to search for.
    /// @param label The label to search for.
    [[nodiscard]]
    wxWindow* FindWindowByIdAndLabel(const wxWindowID id, const wxString& label)
        {
        for (auto pos = m_windows.begin();
             pos != m_windows.end();
             ++pos)
            {
            if (*pos && ((*pos)->GetId() == id) && (*pos)->GetName().CmpNoCase(label) == 0)
                { return *pos; }
            }
        return nullptr;
        }
    /// @returns The first window with the same class type as @c classInfo, or null if not found.
    /// @param classInfo The class information that the window should match.
    [[nodiscard]]
    wxWindow* FindWindowByType(const wxClassInfo* classInfo)
        {
        wxASSERT_LEVEL_2(classInfo);
        for (auto win : m_windows)
            {
            wxASSERT_LEVEL_2_MSG(win, "NULL window in window container!");
            if (win && win->IsKindOf(classInfo))
                { return win; }
            }
        return nullptr;
        }
    /// @returns The last window with the same class type as @c classInfo, or null if not found.
    /// @param classInfo The class information that the window should match.
    [[nodiscard]]
    wxWindow* RFindWindowByType(const wxClassInfo* classInfo)
        {
        wxASSERT_LEVEL_2(classInfo);
        for (auto win = m_windows.crbegin(); win != m_windows.crend(); ++win)
            {
            wxASSERT_LEVEL_2_MSG(*win, "NULL window in window container!");
            if (*win && (*win)->IsKindOf(classInfo))
                { return *win; }
            }
        return nullptr;
        }
    /// @returns The position of the window with @c id, or @c wxNOT_FOUND if not found.
    /// @param id The id to search for.
    [[nodiscard]]
    int FindWindowPositionById(const wxWindowID id)
        {
        for (auto pos = m_windows.begin();
             pos != m_windows.end();
             ++pos)
            {
            if (*pos && (*pos)->GetId() == id)
                { return (pos-m_windows.begin()); }
            }
        return wxNOT_FOUND;
        }
    /// @returns The position of the window with @c id that is also the same class type as
    ///     @c classInfo, or @c wxNOT_FOUND if not found.
    /// @param id The window ID to search for.
    /// @param classInfo The class information that the window should match.
    [[nodiscard]]
    int FindWindowPositionById(const wxWindowID id, const wxClassInfo* classInfo)
        {
        wxASSERT_LEVEL_2(classInfo);
        for (auto pos = m_windows.begin();
             pos != m_windows.end();
             ++pos)
            {
            if (*pos && ((*pos)->GetId() == id) && (*pos)->IsKindOf(classInfo))
                { return (pos-m_windows.begin()); }
            }
        return wxNOT_FOUND;
        }
    /// @returns The number of windows in the container.
    [[nodiscard]]
    size_t GetWindowCount() const noexcept
        { return m_windows.size(); }
private:
    std::vector<wxWindow*> m_windows;
    };

/** @}*/

#endif //__WINDOW_CONTAINER_H__
