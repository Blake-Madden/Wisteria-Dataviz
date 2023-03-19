/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __PRINTERHEADERFOOTER_DLG_H__
#define __PRINTERHEADERFOOTER_DLG_H__

#include <wx/wx.h>
#include <wx/spinctrl.h>
#include <wx/valgen.h>
#include <wx/textdlg.h>
#include <wx/combobox.h>
#include <wx/tooltip.h>
#include <wx/regex.h>
#include <set>
#include "dialogwithhelp.h"
#include "../../i18n-check/src/donttranslate.h"

namespace Wisteria::UI
    {
    /** @brief Dialog for specifying headers and footers for printing.
        @details The headers and footers returned from this dialog may embed the following tags
            that the client should convert in their printing code at runtime:

         - `@TITLE@`: The title of the printed document.
         - `@DATE@`: The date when it was printed.
         - `@TIME@`: The time when it was printed.
         - `@PAGENUM@`: The current page number.
         - `@PAGESCNT@`: The number of printed pages.*/
    class PrinterHeaderFooterDlg final : public DialogWithHelp
        {
    public:
        /** @brief Constructor.
            @param parent The parent window.
            @param leftPrinterHeader The default left printer header.
            @param centerPrinterHeader The default center printer header.
            @param rightPrinterHeader The default right printer header.
            @param leftPrinterFooter The default left printer footer.
            @param centerPrinterFooter The default center printer footer.
            @param rightPrinterFooter The default right printer footer.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        PrinterHeaderFooterDlg(wxWindow* parent,
                          const wxString& leftPrinterHeader, const wxString& centerPrinterHeader,
                          const wxString& rightPrinterHeader, const wxString& leftPrinterFooter,
                          const wxString& centerPrinterFooter, const wxString& rightPrinterFooter,
                          wxWindowID id = wxID_ANY, const wxString& caption = _(L"Printer Headers & Footers"),
                          const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER) :
                          DialogWithHelp(parent, id, caption, pos, size, style),
                          m_leftPrinterHeader(leftPrinterHeader), m_centerPrinterHeader(centerPrinterHeader),
                          m_rightPrinterHeader(rightPrinterHeader), m_leftPrinterFooter(leftPrinterFooter),
                          m_centerPrinterFooter(centerPrinterFooter), m_rightPrinterFooter(rightPrinterFooter)
            {
            SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxDIALOG_EX_METAL);
            CreateControls();

            Bind(wxEVT_BUTTON, &PrinterHeaderFooterDlg::OnOK, this, wxID_OK);

            Centre();
            }
        /// @private
        PrinterHeaderFooterDlg(const PrinterHeaderFooterDlg&) = delete;
        /// @private
        PrinterHeaderFooterDlg(PrinterHeaderFooterDlg&&) = delete;
        /// @private
        PrinterHeaderFooterDlg& operator=(const PrinterHeaderFooterDlg&) = delete;
        /// @private
        PrinterHeaderFooterDlg& operator=(PrinterHeaderFooterDlg&&) = delete;

        /// @returns The left header.
        const wxString& GetLeftPrinterHeader() const noexcept
            { return m_leftPrinterHeader; }
        /// @returns The center header.
        const wxString& GetCenterPrinterHeader() const noexcept
            { return m_centerPrinterHeader; }
        /// @returns The right header.
        const wxString& GetRightPrinterHeader() const noexcept
            { return m_rightPrinterHeader; }
        /// @returns The left footer.
        const wxString& GetLeftPrinterFooter() const noexcept
            { return m_leftPrinterFooter; }
        /// @returns The center footer.
        const wxString& GetCenterPrinterFooter() const noexcept
            { return m_centerPrinterFooter; }
        /// @returns The right footer.
        const wxString& GetRightPrinterFooter() const noexcept
            { return m_rightPrinterFooter; }
    private:
        void CreateControls();
        void OnOK([[maybe_unused]] wxCommandEvent& event);
        [[nodiscard]] bool Validate() final;
        static void UCaseEmbeddedTags(wxString& str);

        // headers
        wxString m_leftPrinterHeader;
        wxString m_centerPrinterHeader;
        wxString m_rightPrinterHeader;
        // footers
        wxString m_leftPrinterFooter;
        wxString m_centerPrinterFooter;
        wxString m_rightPrinterFooter;

        enum ControlIDs
            {
            ID_LEFT_HEADER_COMBOBOX = wxID_HIGHEST,
            ID_CENTER_HEADER_COMBOBOX,
            ID_RIGHT_HEADER_COMBOBOX,
            ID_LEFT_FOOTER_COMBOBOX,
            ID_CENTER_FOOTER_COMBOBOX,
            ID_RIGHT_FOOTER_COMBOBOX
            };

        wxComboBox* leftHeaderPrinterCombo{ nullptr };
        wxComboBox* centerHeaderPrinterCombo{ nullptr };
        wxComboBox* rightHeaderPrinterCombo{ nullptr };
        wxComboBox* leftFooterPrinterCombo{ nullptr };
        wxComboBox* centerFooterPrinterCombo{ nullptr };
        wxComboBox* rightFooterPrinterCombo{ nullptr };
        };
    }

/** @}*/

#endif //__PRINTERHEADERFOOTER_DLG_H__
