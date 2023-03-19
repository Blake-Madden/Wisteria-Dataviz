/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __HTML_TABLE_WINDOW_H__
#define __HTML_TABLE_WINDOW_H__

#include <wx/wx.h>
#include <wx/file.h>
#include <wx/html/htmlwin.h>
#include <wx/html/htmlcell.h>
#include <wx/filename.h>
#include <wx/clipbrd.h>
#include <wx/fdrepdlg.h>
#include <wx/menu.h>
#include <wx/print.h>
#include <wx/xrc/xmlres.h>
#include <wx/html/htmprint.h>
#include "../../base/canvas.h"
#include "../../i18n-check/src/donttranslate.h"

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
        explicit HtmlTableWindow(wxWindow *parent, wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition,
            const wxSize& size = wxDefaultSize,
            long style = wxHW_DEFAULT_STYLE|wxBORDER_THEME) :
                wxHtmlWindow(parent, id, pos, size, style, L"HtmlTableWindow")
            {}
        /// @private
        HtmlTableWindow(const HtmlTableWindow&) = delete;
        /// @private
        HtmlTableWindow& operator=(const HtmlTableWindow&) = delete;
        /// @private
        ~HtmlTableWindow()
            { delete m_menu; }

        /** @brief Assigns a context menu.
            @param menu The heap-based menu to use.
            @note This will take ownership of this menu and delete it upon descruction.*/
        void AssignContextMenu(wxMenu* menu)
            {
            delete m_menu;
            m_menu = menu;
            }
        /** @brief Sets the printer settings.
            @param printData A pointer to the printer settings to use.*/
        void SetPrinterSettings(wxPrintData* printData) noexcept
            { m_printData = printData; }
        /// @brief Sets the left header used for printing.
        /// @param header The string to use.
        void SetLeftPrinterHeader(const wxString& header)
            { m_leftPrinterHeader = header; }
        /// @returns The left header used for printing.
        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept
            { return m_leftPrinterHeader; }
        /// @brief Sets the center header used for printing.
        /// @param header The string to use.
        void SetCenterPrinterHeader(const wxString& header)
            { m_centerPrinterHeader = header; }
        /// @returns The center header used for printing.
        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept
            { return m_centerPrinterHeader; }
        /// @brief Sets the right header used for printing.
        /// @param header The string to use.
        void SetRightPrinterHeader(const wxString& header)
            { m_rightPrinterHeader = header; }
        /// @returns The right header used for printing.
        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept
            { return m_rightPrinterHeader; }
        /// @brief Sets the left footer used for printing.
        /// @param footer The string to use.
        void SetLeftPrinterFooter(const wxString& footer)
            { m_leftPrinterFooter = footer; }
        /// @returns The left footer used for printing.
        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept
            { return m_leftPrinterFooter; }
        /// @brief Sets the center footer used for printing.
        /// @param footer The string to use.
        void SetCenterPrinterFooter(const wxString& footer)
            { m_centerPrinterFooter = footer; }
        /// @returns The center footer used for printing.
        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept
            { return m_centerPrinterFooter; }
        /// @brief Sets the right footer used for printing.
        /// @param footer The string to use.
        void SetRightPrinterFooter(const wxString& footer)
            { m_rightPrinterFooter = footer; }
        /// @returns The right footer used for printing.
        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept
            { return m_rightPrinterFooter; }

        /// @brief Sets the watermark for the list when printed.
        /// @param watermark The watermark information.
        void SetWatermark(const Wisteria::Canvas::Watermark& watermark) noexcept
            { m_waterMark = watermark; }
        /// @returns The printer watermark.
        [[nodiscard]]
        const Wisteria::Canvas::Watermark& GetWatermark() const noexcept
            { return m_waterMark; }
        /// @brief Saving the contents of the window to HTML.
        /// @param path The path to save to.
        /// @returns @c true upon success.
        bool Save(const wxFileName& path);

        /// @private
        void OnSelectAll([[maybe_unused]] wxCommandEvent& event );
        /// @private
        void OnCopy([[maybe_unused]] wxCommandEvent& event );
        /// @private
        void OnCopyAll([[maybe_unused]] wxCommandEvent& event );
        /// @private
        void OnFind([[maybe_unused]] wxFindDialogEvent& event);
        /// @private
        void OnRightClick([[maybe_unused]] wxMouseEvent& event);
        /// @private
        void OnSave([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnPreview([[maybe_unused]] wxCommandEvent& event);
        /// @private
        void OnPrint([[maybe_unused]] wxCommandEvent& event);
    private:
        wxMenu* m_menu{ nullptr };
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

        wxDECLARE_EVENT_TABLE();
        wxDECLARE_DYNAMIC_CLASS(HtmlTableWindow);
        HtmlTableWindow() = default;
        };
    }
/** @}*/

#endif //__HTML_TABLE_WINDOW_H__
