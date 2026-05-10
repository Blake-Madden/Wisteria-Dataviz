/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PDF_EXPORT_DLG_H
#define WISTERIA_PDF_EXPORT_DLG_H

#include "../../base/pdfreportprintout.h"
#include "dialogwithhelp.h"
#include <wx/printdlg.h>

namespace Wisteria::UI
    {
    /** @brief Options dialog for exporting to PDF.
        @details Prompts the user for PDF-specific options, such as paper size and orientation.*/
    class PdfExportDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param printData The print data to pre-populate the dialog with.
            @param options The PDF-specific options (metadata, etc.).
            @param caption The title of the export dialog.*/
        PdfExportDlg(wxWindow* parent, const wxPrintData& printData,
                     const Wisteria::PdfExportOptions& options,
                     const wxString& caption = _(L"PDF Export Options"));

        /// @private
        PdfExportDlg(const PdfExportDlg&) = delete;
        /// @private
        PdfExportDlg& operator=(const PdfExportDlg&) = delete;

        /// @returns The print data selected by the user.
        [[nodiscard]]
        const wxPrintData& GetPrintData() const noexcept
            {
            return m_printData;
            }

        /// @returns The PDF options selected by the user.
        [[nodiscard]]
        const Wisteria::PdfExportOptions& GetOptions() const noexcept
            {
            return m_options;
            }

      private:
        void CreateControls();
        void UpdateLabels();
        void OnPaintPreview(wxPaintEvent& event);
        void UpdatePreview();

        wxPrintData m_printData;
        Wisteria::PdfExportOptions m_options;

        wxStaticText* m_paperTypeLabel{ nullptr };
        wxStaticText* m_orientationLabel{ nullptr };

        wxPanel* m_previewPanel{ nullptr };
        int m_pageWidth{ 0 };
        int m_pageHeight{ 0 };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // WISTERIA_PDF_EXPORT_DLG_H
