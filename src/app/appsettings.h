///////////////////////////////////////////////////////////////////////////////
// Name:        appsettings.h
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#ifndef WISTERIA_APPSETTINGS_H
#define WISTERIA_APPSETTINGS_H

#include "../base/svgreportprintout.h"
#include <wx/filename.h>
#include <wx/gdicmn.h>
#include <wx/print.h>
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
    /// @param width The window width in pixels.
    void SetAppWindowWidth(const int width) noexcept { m_appWindowWidth = width; }

    /// @returns The application window height.
    [[nodiscard]]
    int GetAppWindowHeight() const noexcept
        {
        return m_appWindowHeight;
        }

    /// @brief Sets the application window height.
    /// @param height The window height in pixels.
    void SetAppWindowHeight(const int height) noexcept { m_appWindowHeight = height; }

    /// @returns The stored window size.
    [[nodiscard]]
    wxSize GetAppWindowSize() const noexcept
        {
        return { m_appWindowWidth, m_appWindowHeight };
        }

    /// @brief Accesses print orientation (wxPORTRAIT or wxLANDSCAPE).
    /// @returns The print orientation.
    [[nodiscard]]
    int GetPrintOrientation() const noexcept
        {
        return m_printOrientation;
        }

    /// @brief Sets the print orientation.
    /// @param orientation The print orientation (wxPORTRAIT or wxLANDSCAPE).
    void SetPrintOrientation(const int orientation) noexcept { m_printOrientation = orientation; }

    /// @brief Accesses paper ID (wxPaperSize).
    /// @returns The paper ID.
    [[nodiscard]]
    wxPaperSize GetPaperId() const noexcept
        {
        return m_paperId;
        }

    /// @brief Sets the paper ID.
    /// @param paperId The paper ID (wxPaperSize).
    void SetPaperId(const wxPaperSize paperId) noexcept { m_paperId = paperId; }

    // SVG export options
    //-------------------

    /// @returns The SVG export options.
    [[nodiscard]]
    Wisteria::SVGReportOptions& GetSvgExportOptions() noexcept
        {
        return m_svgExportOptions;
        }

    /// @private
    [[nodiscard]]
    const Wisteria::SVGReportOptions& GetSvgExportOptions() const noexcept
        {
        return m_svgExportOptions;
        }

  private:
    wxString m_settingsFilePath;
    bool m_appWindowMaximized{ true };
    int m_appWindowWidth{ 800 };
    int m_appWindowHeight{ 700 };
    int m_printOrientation{ wxPORTRAIT };
    wxPaperSize m_paperId{ wxPAPER_LETTER };

    Wisteria::SVGReportOptions m_svgExportOptions{ wxString{} };
    };

#endif // WISTERIA_APPSETTINGS_H
