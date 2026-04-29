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

#ifdef INCLUDE_PDF

    #include "canvas.h"
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
        /// @brief Constructor. Exports all canvases as pages to a PDF file immediately.
        /// @param canvases The canvases (pages) to export.
        /// @param filePath The output PDF file path.
        /// @param title The document title embedded in the PDF.
        ReportPDFExport(const std::vector<Canvas*>& canvases, const wxString& filePath,
                        const wxString& title = wxString{});
        };
    } // namespace Wisteria

#endif // INCLUDE_PDF

/** @}*/

#endif // WISTERIA_REPORT_PDF_EXPORT_H
