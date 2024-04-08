/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __HTML_TABLE_WINDOW_H__
#define __HTML_TABLE_WINDOW_H__

#include "../../base/canvas.h"
#include "../../i18n-check/src/donttranslate.h"
#include <wx/clipbrd.h>
#include <wx/fdrepdlg.h>
#include <wx/file.h>
#include <wx/filename.h>
#include <wx/html/htmlcell.h>
#include <wx/html/htmlwin.h>
#include <wx/html/htmprint.h>
#include <wx/menu.h>
#include <wx/print.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>

namespace Wisteria::UI
    {
    /// @brief A `wxHtmlWindow`-derived window, designed for displaying an HTML table.
    /// @details Includes built-in support for printing, exporting, and copying.
    class HtmlTableWindow : public wxHtmlWindow
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param id The window's ID.
            @param pos The window's position.
            @param size The window's size.
            @param style The window's style.*/
        explicit HtmlTableWindow(wxWindow* parent, wxWindowID id = wxID_ANY,
                                 const wxPoint& pos = wxDefaultPosition,
                                 const wxSize& size = wxDefaultSize,
                                 long style = wxHW_DEFAULT_STYLE | wxHW_NO_SELECTION |
                                              wxBORDER_THEME);
        /// @private
        HtmlTableWindow(const HtmlTableWindow&) = delete;
        /// @private
        HtmlTableWindow& operator=(const HtmlTableWindow&) = delete;

        /** @brief Sets the printer settings.
            @param printData A pointer to the printer settings to use.*/
        void SetPrinterSettings(wxPrintData* printData) noexcept { m_printData = printData; }

        /// @brief Sets the left header used for printing.
        /// @param header The string to use.
        void SetLeftPrinterHeader(wxString header) { m_leftPrinterHeader = std::move(header); }

        /// @returns The left header used for printing.
        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept
            {
            return m_leftPrinterHeader;
            }

        /// @brief Sets the center header used for printing.
        /// @param header The string to use.
        void SetCenterPrinterHeader(wxString header) { m_centerPrinterHeader = std::move(header); }

        /// @returns The center header used for printing.
        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept
            {
            return m_centerPrinterHeader;
            }

        /// @brief Sets the right header used for printing.
        /// @param header The string to use.
        void SetRightPrinterHeader(wxString header) { m_rightPrinterHeader = std::move(header); }

        /// @returns The right header used for printing.
        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept
            {
            return m_rightPrinterHeader;
            }

        /// @brief Sets the left footer used for printing.
        /// @param footer The string to use.
        void SetLeftPrinterFooter(wxString footer) { m_leftPrinterFooter = std::move(footer); }

        /// @returns The left footer used for printing.
        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept
            {
            return m_leftPrinterFooter;
            }

        /// @brief Sets the center footer used for printing.
        /// @param footer The string to use.
        void SetCenterPrinterFooter(wxString footer) { m_centerPrinterFooter = std::move(footer); }

        /// @returns The center footer used for printing.
        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept
            {
            return m_centerPrinterFooter;
            }

        /// @brief Sets the right footer used for printing.
        /// @param footer The string to use.
        void SetRightPrinterFooter(wxString footer) { m_rightPrinterFooter = std::move(footer); }

        /// @returns The right footer used for printing.
        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept
            {
            return m_rightPrinterFooter;
            }

        /// @brief Sets the watermark for the list when printed.
        /// @param watermark The watermark information.
        void SetWatermark(Wisteria::Canvas::Watermark watermark)
            {
            m_waterMark = std::move(watermark);
            }

        /// @returns The printer watermark.
        [[nodiscard]]
        const Wisteria::Canvas::Watermark& GetWatermark() const noexcept
            {
            return m_waterMark;
            }

        /// @brief Saving the contents of the window to HTML.
        /// @param path The path to save to.
        /// @returns @c true upon success.
        bool Save(const wxFileName& path);

        /// @brief Copies all the HTML tables to the clipboard.
        void Copy();

      private:
        HtmlTableWindow() = default;

        void OnSelectAll([[maybe_unused]] wxCommandEvent& event);

        void OnCopy([[maybe_unused]] wxCommandEvent& event);

        void OnFind([[maybe_unused]] wxFindDialogEvent& event);

        void OnRightClick([[maybe_unused]] wxMouseEvent& event);

        void OnSave([[maybe_unused]] wxCommandEvent& event);

        void OnPreview([[maybe_unused]] wxCommandEvent& event);

        void OnPrint([[maybe_unused]] wxCommandEvent& event);

        wxMenu m_menu;
        wxPrintData* m_printData{ nullptr };
        // headers
        wxString m_leftPrinterHeader;
        wxString m_centerPrinterHeader;
        wxString m_rightPrinterHeader;
        // footers
        wxString m_leftPrinterFooter;
        wxString m_centerPrinterFooter;
        wxString m_rightPrinterFooter;

        Wisteria::Canvas::Watermark m_waterMark;

        wxDECLARE_DYNAMIC_CLASS(HtmlTableWindow);
        };
    } // namespace Wisteria::UI

/** @}*/

#endif //__HTML_TABLE_WINDOW_H__
