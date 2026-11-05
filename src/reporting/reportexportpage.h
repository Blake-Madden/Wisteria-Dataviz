/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_REPORT_EXPORT_PAGE_H
#define WISTERIA_REPORT_EXPORT_PAGE_H

#include "../base/canvas.h"
#include <array>
#include <optional>
#include <variant>
#include <vector>
#include <wx/colour.h>
#include <wx/defs.h>
#include <wx/string.h>

class wxGrid;
class wxListCtrl;

namespace Wisteria
    {
    /// @brief Where a table page's watermark goes, for exports that don't draw on a DC.
    /// @details Lengths use the same units as the page size that they were calculated from.
    struct ReportWatermarkLayout
        {
        /// @brief The text, with its placeholders expanded.
        wxString m_label;
        /// @brief The text color.
        wxColour m_color;
        /// @brief The text opacity, from 0 to 1.
        double m_opacity{ 0.5 };
        /// @brief The counterclockwise rotation, in degrees.
        double m_angle{ 0.0 };
        /// @brief The font size.
        double m_fontSize{ 0.0 };
        /// @brief The length of the line that the text is centered on.
        double m_length{ 0.0 };
        };

    /// @brief A column in an exported table.
    struct ReportTableColumn
        {
        /// @brief The column's header.
        wxString m_label;
        /// @brief The horizontal alignment of the column's header.
        wxAlignment m_alignment{ wxALIGN_LEFT };
        /// @brief The width (in pixels) needed by the column's widest text and its padding.
        ///     This includes an allowance for viewers showing a wider font.
        int m_width{ 0 };
        };

    /// @brief The text of a list or grid control.
    struct ReportTableData
        {
        /// @brief The columns.
        std::vector<ReportTableColumn> m_columns;
        /// @brief The cells of each row, in the same order as the columns.
        std::vector<std::vector<wxString>> m_rows;
        /// @brief The horizontal alignment of each cell, laid out the same as @c m_rows.
        std::vector<std::vector<wxAlignment>> m_cellAlignments;
        };

    /// @brief An item to include in an exported report.
    /// @details A canvas uses its own printer headers and footers.
    ///     The other fields only apply to list and grid controls, which are exported as tables.
    struct ReportExportPage
        {
        /// @brief The canvas, list control, or grid control to export.
        std::variant<Canvas*, const wxListCtrl*, const wxGrid*> m_content;
        /// @brief The title above the table.
        wxString m_title;
        /// @brief The stable ID of the table, used to exclude it from an export.
        /// @note A canvas uses its own GUID instead.
        wxString m_guid;
        /// @brief The left, center, and right header of the table's printed pages.
        std::array<wxString, 3> m_headers;
        /// @brief The left, center, and right footer of the table's printed pages.
        std::array<wxString, 3> m_footers;
        /// @brief The watermark drawn across the table's pages.
        /// @note A canvas uses its own watermark instead.
        Canvas::Watermark m_watermark;

        /// @brief Calculates where the watermark goes on a table's page.
        /// @param pageWidth The page width.
        /// @param pageHeight The page height.
        /// @returns The layout, or @c std::nullopt if there is no watermark.
        [[nodiscard]]
        std::optional<ReportWatermarkLayout> GetWatermarkLayout(double pageWidth,
                                                                double pageHeight) const;

        /// @returns @c true if this is a list or grid control with at least one shown
        ///     row and column. Exporters skip tables without any.
        [[nodiscard]]
        bool HasTableData() const;

        /// @brief Reads the text of a list or grid control.
        /// @param fontSize The font size (in pixels) to measure the columns with.
        /// @param cellPadding The space (in pixels) to add to both sides of the columns.
        /// @returns The table. This is empty if HasTableData() is @c false.
        [[nodiscard]]
        ReportTableData GetTableData(int fontSize, int cellPadding) const;

        /// @brief Wraps canvases as export pages.
        /// @param canvases The canvases.
        /// @returns The export pages.
        [[nodiscard]]
        static std::vector<ReportExportPage> FromCanvases(const std::vector<Canvas*>& canvases)
            {
            std::vector<ReportExportPage> pages;
            pages.reserve(canvases.size());
            for (auto* canvas : canvases)
                {
                pages.emplace_back().m_content = canvas;
                }
            return pages;
            }

        /// @brief Gets the canvases from export pages, for exports that don't support tables.
        /// @param pages The export pages.
        /// @returns The canvases.
        [[nodiscard]]
        static std::vector<Canvas*> ToCanvases(const std::vector<ReportExportPage>& pages)
            {
            std::vector<Canvas*> canvases;
            canvases.reserve(pages.size());
            for (const auto& page : pages)
                {
                if (const auto* const canvasHolder = std::get_if<Canvas*>(&page.m_content);
                    canvasHolder != nullptr)
                    {
                    canvases.push_back(*canvasHolder);
                    }
                }
            return canvases;
            }

      private:
        /// @brief Gets a grid cell's text, as its renderer formats it.
        /// @param grid The grid.
        /// @param row The cell's row.
        /// @param col The cell's column.
        /// @returns The text. Custom renderers can't be read, so they return the cell's value.
        [[nodiscard]]
        static wxString GetGridCellText(const wxGrid& grid, int row, int col);

        /// @brief Gets the horizontal alignment that a grid cell is drawn with.
        /// @param grid The grid.
        /// @param row The cell's row.
        /// @param col The cell's column.
        /// @returns The alignment.
        [[nodiscard]]
        static wxAlignment GetGridCellAlignment(const wxGrid& grid, int row, int col);

        /// @brief Gets the horizontal part of a wxWidgets alignment.
        /// @param alignment The alignment flags.
        /// @returns The horizontal alignment.
        [[nodiscard]]
        static wxAlignment ToHorizontalAlignment(int alignment);
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_REPORT_EXPORT_PAGE_H
