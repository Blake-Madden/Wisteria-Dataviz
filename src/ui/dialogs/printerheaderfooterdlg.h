/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PRINTERHEADERFOOTER_DLG_H
#define PRINTERHEADERFOOTER_DLG_H

#include "dialogwithhelp.h"
#include <set>
#include <wx/combobox.h>
#include <wx/regex.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for specifying headers and footers for printing.
        @details The headers and footers returned from this dialog may embed the following tags
            that the client should convert in their printing code at runtime:

         - `@TITLE@`: The title of the printed document.
         - `@DATE@`: The date when it was printed.
         - `@TIME@`: The time when it was printed.
         - `@PAGENUM@`: The current page number.
         - `@PAGESCNT@`: The number of printed pages.
         - `@USER@`: The user's name.*/
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
            @param caption The title of the dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        PrinterHeaderFooterDlg(wxWindow* parent, wxString leftPrinterHeader,
                               wxString centerPrinterHeader, wxString rightPrinterHeader,
                               wxString leftPrinterFooter, wxString centerPrinterFooter,
                               wxString rightPrinterFooter, wxWindowID id = wxID_ANY,
                               const wxString& caption = _(L"Printer Headers & Footers"),
                               const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER)
            : DialogWithHelp(parent, id, caption, pos, size, style),
              m_leftPrinterHeader(std::move(leftPrinterHeader)),
              m_centerPrinterHeader(std::move(centerPrinterHeader)),
              m_rightPrinterHeader(std::move(rightPrinterHeader)),
              m_leftPrinterFooter(std::move(leftPrinterFooter)),
              m_centerPrinterFooter(std::move(centerPrinterFooter)),
              m_rightPrinterFooter(std::move(rightPrinterFooter))
            {
            wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS |
                                            wxDIALOG_EX_METAL);
            CreateControls();

            Bind(wxEVT_BUTTON, &PrinterHeaderFooterDlg::OnOK, this, wxID_OK);

            Centre();
            }

        /// @private
        PrinterHeaderFooterDlg(const PrinterHeaderFooterDlg&) = delete;
        /// @private
        PrinterHeaderFooterDlg& operator=(const PrinterHeaderFooterDlg&) = delete;

        /// @returns The left header.
        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept { return m_leftPrinterHeader; }

        /// @returns The center header.
        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept { return m_centerPrinterHeader; }

        /// @returns The right header.
        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept { return m_rightPrinterHeader; }

        /// @returns The left footer.
        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept { return m_leftPrinterFooter; }

        /// @returns The center footer.
        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept { return m_centerPrinterFooter; }

        /// @returns The right footer.
        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept { return m_rightPrinterFooter; }

      private:
        void CreateControls();
        void OnOK([[maybe_unused]] wxCommandEvent& event);
        [[nodiscard]]
        bool Validate() final;
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
    } // namespace Wisteria::UI

/** @}*/

#endif // PRINTERHEADERFOOTER_DLG_H
