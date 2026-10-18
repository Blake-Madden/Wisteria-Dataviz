/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_PDF_EXPORT_H
#define WISTERIA_REPORT_PDF_EXPORT_H

#include "../base/canvas.h"
#include <array>
#include <vector>

namespace Wisteria
    {
    /// @brief Exports a collection of canvases as pages into a single multi-page PDF file.
    /// @details Each canvas is rendered into its own page using @c wxPdfDC.
    ///     Page dimensions are derived from the paper size stored in each canvas's
    ///     printer settings.
    class ReportPDFExport
        {
      public:
        /** @brief Constructor. Exports all canvases as pages to a PDF file immediately.
            @param canvases The canvases (pages) to export.
            @param filePath The output PDF file path.
            @param options The PDF export options (metadata, compression, etc.).*/
        ReportPDFExport(const std::vector<Canvas*>& canvases, const wxString& filePath,
                        const PdfExportOptions& options = PdfExportOptions{});

      private:
        /// @brief Draws a left/center/right header or footer strip.
        /// @param dc The DC to draw on.
        /// @param stripRect The area of the strip.
        /// @param scaling The canvas scaling.
        /// @param text The left, center, and right text (already expanded).
        /// @param vertAlign How the text is aligned vertically within the strip.
        static void DrawPrintStrip(wxDC& dc, const wxRect& stripRect, double scaling,
                                   const std::array<wxString, 3>& text,
                                   PageVerticalAlignment vertAlign);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_PDF_EXPORT_H
