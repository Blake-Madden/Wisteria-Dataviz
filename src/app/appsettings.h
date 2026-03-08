///////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_APPSETTINGS_H
#define WISTERIA_APPSETTINGS_H

#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <wx/stdpaths.h>
#include <wx/string.h>

/// @brief Manages application settings persistence.
class AppSettings
    {
  public:
    /// @brief Loads settings from the given XML file.
    /// @param filePath The full path to the settings XML file.
    /// @returns @c true if the file was loaded successfully.
    bool LoadSettingsFile(const wxString& filePath);

    /// @brief Saves settings to the previously loaded file path.
    /// @returns @c true if the file was saved successfully.
    /// @note LoadSettingsFile() or SetSettingsFilePath() must be called first.
    bool SaveSettingsFile();

    /// @brief Saves settings to the given file path.
    /// @param filePath The full path to save the settings to.
    /// @returns @c true if the file was saved successfully.
    bool SaveSettingsFile(const wxString& filePath);

    /// @brief Sets the file path for subsequent calls to SaveSettingsFile().
    /// @param filePath The full path to the settings file.
    void SetSettingsFilePath(const wxString& filePath) { m_settingsFilePath = filePath; }

    /// @returns The file path used for settings persistence.
    [[nodiscard]]
    const wxString& GetSettingsFilePath() const noexcept
        {
        return m_settingsFilePath;
        }

    /// @returns @c true if the application window was maximized.
    [[nodiscard]]
    bool IsAppWindowMaximized() const noexcept
        {
        return m_appWindowMaximized;
        }

    /// @brief Sets whether the application window was maximized.
    /// @param maximized @c true if the window was maximized.
    void SetAppWindowMaximized(const bool maximized) noexcept { m_appWindowMaximized = maximized; }

    /// @returns The application window width.
    [[nodiscard]]
    int GetAppWindowWidth() const noexcept
        {
        return m_appWindowWidth;
        }

    /// @brief Sets the application window width.
    /// @param width The window width.
    void SetAppWindowWidth(const int width) noexcept { m_appWindowWidth = width; }

    /// @returns The application window height.
    [[nodiscard]]
    int GetAppWindowHeight() const noexcept
        {
        return m_appWindowHeight;
        }

    /// @brief Sets the application window height.
    /// @param height The window height.
    void SetAppWindowHeight(const int height) noexcept { m_appWindowHeight = height; }

    /// @returns The stored window size.
    [[nodiscard]]
    wxSize GetAppWindowSize() const noexcept
        {
        return { m_appWindowWidth, m_appWindowHeight };
        }

  private:
    wxString m_settingsFilePath;
    bool m_appWindowMaximized{ true };
    int m_appWindowWidth{ 800 };
    int m_appWindowHeight{ 700 };
    };

#endif // WISTERIA_APPSETTINGS_H
