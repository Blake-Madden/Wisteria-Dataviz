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
#include "reportexportpage.h"
#include <array>
#include <functional>
#include <vector>

class wxPdfDocument;

namespace Wisteria
    {
    /// @brief Exports canvases and tables as pages into a single multi-page PDF file.
    /// @details Each canvas is rendered into its own page using @c wxPdfDC.
    ///     A list or grid control is exported as a vector table, spanning as many pages
    ///     as it needs.
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

        /** @brief Constructor. Exports all items as pages to a PDF file immediately.
            @param pages The canvases and tables to export.
            @param filePath The output PDF file path.
            @param options The PDF export options (metadata, compression, etc.).*/
        ReportPDFExport(const std::vector<ReportExportPage>& pages, const wxString& filePath,
                        const PdfExportOptions& options = PdfExportOptions{});

      private:
        /// @brief Margin between the sides of the page and a table, in points.
        constexpr static double TABLE_SIDE_MARGIN{ 36.0 };
        /// @brief Margin at the top and bottom of a table's page, in points.
        constexpr static double TABLE_EDGE_MARGIN{ 24.0 };
        /// @brief Space between a table and its title, header, or footer, in points.
        constexpr static double TABLE_STRIP_GAP{ 6.0 };

        /// @brief Determines whether any of a strip's text is set.
        /// @param text The left, center, and right text.
        /// @returns @c true if the strip has text.
        [[nodiscard]]
        static bool HasStripText(const std::array<wxString, 3>& text);

        /// @brief Sets a document's margins for a table, leaving room for its strips.
        /// @param doc The document.
        /// @param page The table's page.
        /// @param stripHeight The height of a strip, in points.
        static void ApplyTableMargins(wxPdfDocument& doc, const ReportExportPage& page,
                                      int stripHeight);

        /// @brief Adds a table to the current page of a document.
        /// @param doc The document.
        /// @param page The table's page.
        /// @param stripHeight The height of a strip, in points.
        /// @param pageBreakHandler The function that starts each continuation page.
        ///     If empty, then the document adds the pages itself.
        static void AddTable(wxPdfDocument& doc, const ReportExportPage& page, int stripHeight,
                             const std::function<void()>& pageBreakHandler);

        /// @brief Counts the pages that a table needs.
        /// @param page The table's page.
        /// @param printData The paper settings.
        /// @param stripHeight The height of a strip, in points.
        /// @returns The number of pages.
        [[nodiscard]]
        static size_t CountTablePages(const ReportExportPage& page, const wxPrintData& printData,
                                      int stripHeight);

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
