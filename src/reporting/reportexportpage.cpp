///////////////////////////////////////////////////////////////////////////////
// Name:        reportexportpage.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportexportpage.h"
#include "../math/safe_math.h"
#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>
#include <wx/bitmap.h>
#include <wx/dcmemory.h>
#include <wx/font.h>
#include <wx/generic/gridctrl.h>
#include <wx/grid.h>
#include <wx/listctrl.h>
#include <wx/log.h>
#include <wx/settings.h>

//------------------------------------------------------
std::optional<Wisteria::ReportWatermarkLayout>
Wisteria::ReportExportPage::GetWatermarkLayout(const double pageWidth,
                                               const double pageHeight) const
    {
    if (m_watermark.m_label.empty())
        {
        return std::nullopt;
        }

    // the text can't be measured without a DC, so its width is estimated
    constexpr double AVERAGE_CHAR_WIDTH{ 0.6 };
    constexpr double PAGE_AREA{ 0.8 };
    constexpr double MAX_ALPHA{ wxALPHA_OPAQUE / 2.0 };

    const double areaWidth{ pageWidth * PAGE_AREA };
    const double areaHeight{ pageHeight * PAGE_AREA };
    const bool isDiagonal{ m_watermark.m_direction == Canvas::WatermarkDirection::Diagonal };

    ReportWatermarkLayout layout;
    layout.m_label = Canvas::ExpandWatermark(m_watermark.m_label);
    layout.m_color = m_watermark.m_color;
    layout.m_opacity = std::min<double>(m_watermark.m_color.Alpha(), MAX_ALPHA) / wxALPHA_OPAQUE;
    layout.m_angle =
        isDiagonal ? std::atan(safe_divide(areaHeight, areaWidth)) * (180 / std::numbers::pi) : 0.0;
    layout.m_length = isDiagonal ? std::hypot(areaWidth, areaHeight) : areaWidth;
    layout.m_fontSize =
        std::min(safe_divide(layout.m_length,
                             static_cast<double>(layout.m_label.length()) * AVERAGE_CHAR_WIDTH),
                 areaHeight);
    return layout;
    }

//------------------------------------------------------
bool Wisteria::ReportExportPage::HasTableData() const
    {
    if (const auto* const listHolder = std::get_if<const wxListCtrl*>(&m_content);
        listHolder != nullptr && *listHolder != nullptr)
        {
        return (*listHolder)->GetColumnCount() > 0 && (*listHolder)->GetItemCount() > 0;
        }
    if (const auto* const gridHolder = std::get_if<const wxGrid*>(&m_content);
        gridHolder != nullptr && *gridHolder != nullptr)
        {
        const wxGrid* const grid{ *gridHolder };
        bool hasShownCol{ false };
        for (int col = 0; col < grid->GetNumberCols() && !hasShownCol; ++col)
            {
            hasShownCol = grid->IsColShown(col);
            }
        bool hasShownRow{ false };
        for (int row = 0; row < grid->GetNumberRows() && !hasShownRow; ++row)
            {
            hasShownRow = grid->IsRowShown(row);
            }
        return hasShownCol && hasShownRow;
        }
    return false;
    }

//------------------------------------------------------
wxString Wisteria::ReportExportPage::GetGridCellText(const wxGrid& grid, const int row,
                                                     const int col)
    {
    // the renderers' GetString() functions are protected, and a pointer to one can only be
    // taken through a derived class
    struct FloatText : wxGridCellFloatRenderer
        {
        static auto GetFunction() { return &FloatText::GetString; }
        };

    struct NumberText : wxGridCellNumberRenderer
        {
        static auto GetFunction() { return &NumberText::GetString; }
        };

    struct DateText : wxGridCellDateRenderer
        {
        static auto GetFunction() { return &DateText::GetString; }
        };

    struct EnumText : wxGridCellEnumRenderer
        {
        static auto GetFunction() { return &EnumText::GetString; }
        };

    const wxGridCellRendererPtr renderer{ grid.GetCellRenderer(row, col) };
    if (auto* const floatRenderer = dynamic_cast<wxGridCellFloatRenderer*>(renderer.get());
        floatRenderer != nullptr)
        {
        // a width pads the text, which only lines up in the grid's own column
        wxString text{ (floatRenderer->*FloatText::GetFunction())(grid, row, col) };
        return text.Trim(false);
        }
    if (auto* const numberRenderer = dynamic_cast<wxGridCellNumberRenderer*>(renderer.get());
        numberRenderer != nullptr)
        {
        return (numberRenderer->*NumberText::GetFunction())(grid, row, col);
        }
    if (auto* const dateRenderer = dynamic_cast<wxGridCellDateRenderer*>(renderer.get());
        dateRenderer != nullptr)
        {
        return (dateRenderer->*DateText::GetFunction())(grid, row, col);
        }
    if (auto* const enumRenderer = dynamic_cast<wxGridCellEnumRenderer*>(renderer.get());
        enumRenderer != nullptr)
        {
        return (enumRenderer->*EnumText::GetFunction())(grid, row, col);
        }
    return grid.GetCellValue(row, col);
    }

//------------------------------------------------------
wxAlignment Wisteria::ReportExportPage::ToHorizontalAlignment(const int alignment)
    {
    return ((alignment & wxALIGN_RIGHT) != 0)             ? wxALIGN_RIGHT :
           ((alignment & wxALIGN_CENTER_HORIZONTAL) != 0) ? wxALIGN_CENTRE :
                                                            wxALIGN_LEFT;
    }

//------------------------------------------------------
wxAlignment Wisteria::ReportExportPage::GetGridCellAlignment(const wxGrid& grid, const int row,
                                                             const int col)
    {
    const wxGridCellRendererPtr renderer{ grid.GetCellRenderer(row, col) };
    const bool isRightByDefault{
        dynamic_cast<wxGridCellFloatRenderer*>(renderer.get()) != nullptr ||
        dynamic_cast<wxGridCellNumberRenderer*>(renderer.get()) != nullptr ||
        dynamic_cast<wxGridCellDateRenderer*>(renderer.get()) != nullptr ||
        dynamic_cast<wxGridCellEnumRenderer*>(renderer.get()) != nullptr
    };
    int horizAlign{ wxALIGN_LEFT };
    if (isRightByDefault)
        {
        // these renderers ignore the grid's default alignment, but not a cell's own alignment
        horizAlign = wxALIGN_RIGHT;
        const wxGridCellAttrPtr attr{ grid.GetTable()->GetAttrPtr(row, col, wxGridCellAttr::Any) };
        if (attr)
            {
            attr->GetNonDefaultAlignment(&horizAlign, nullptr);
            }
        }
    else
        {
        grid.GetCellAlignment(row, col, &horizAlign, nullptr);
        }
    return ToHorizontalAlignment(horizAlign);
    }

//------------------------------------------------------
Wisteria::ReportTableData Wisteria::ReportExportPage::GetTableData(const int fontSize,
                                                                   const int cellPadding) const
    {
    ReportTableData table;
    if (!HasTableData())
        {
        return table;
        }

    if (const auto* const listHolder = std::get_if<const wxListCtrl*>(&m_content);
        listHolder != nullptr && *listHolder != nullptr)
        {
        const wxListCtrl* const list{ *listHolder };
        for (int col = 0; col < list->GetColumnCount(); ++col)
            {
            wxListItem item;
            item.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_FORMAT);
            list->GetColumn(col, item);
            table.m_columns.push_back(ReportTableColumn{
                .m_label = item.GetText(),
                .m_alignment = (item.GetAlign() == wxLIST_FORMAT_RIGHT)  ? wxALIGN_RIGHT :
                               (item.GetAlign() == wxLIST_FORMAT_CENTRE) ? wxALIGN_CENTRE :
                                                                           wxALIGN_LEFT });
            }
        std::vector<wxAlignment> columnAlignments;
        columnAlignments.reserve(table.m_columns.size());
        for (const auto& column : table.m_columns)
            {
            columnAlignments.push_back(column.m_alignment);
            }
        for (long row = 0; row < list->GetItemCount(); ++row)
            {
            std::vector<wxString> cells;
            cells.reserve(table.m_columns.size());
            for (int col = 0; col < list->GetColumnCount(); ++col)
                {
                cells.push_back(list->GetItemText(row, col));
                }
            table.m_rows.push_back(std::move(cells));
            table.m_cellAlignments.push_back(columnAlignments);
            }
        }
    else if (const auto* const gridHolder = std::get_if<const wxGrid*>(&m_content);
             gridHolder != nullptr && *gridHolder != nullptr)
        {
        const wxGrid* const grid{ *gridHolder };
        std::vector<int> shownCols;
        for (int position = 0; position < grid->GetNumberCols(); ++position)
            {
            const int col{ grid->GetColAt(position) };
            if (grid->IsColShown(col))
                {
                shownCols.push_back(col);
                }
            }
        std::vector<int> shownRows;
        for (int row = 0; row < grid->GetNumberRows(); ++row)
            {
            if (grid->IsRowShown(row))
                {
                shownRows.push_back(row);
                }
            }
        int labelHorizAlign{ wxALIGN_CENTRE };
        int labelVertAlign{ wxALIGN_CENTRE };
        grid->GetColLabelAlignment(&labelHorizAlign, &labelVertAlign);
        for (const int col : shownCols)
            {
            table.m_columns.push_back(
                ReportTableColumn{ .m_label = grid->GetColLabelValue(col),
                                   .m_alignment = ToHorizontalAlignment(labelHorizAlign) });
            }
        for (const int row : shownRows)
            {
            std::vector<wxString> cells;
            cells.reserve(shownCols.size());
            std::vector<wxAlignment> alignments;
            alignments.reserve(shownCols.size());
            for (const int col : shownCols)
                {
                cells.push_back(GetGridCellText(*grid, row, col));
                alignments.push_back(GetGridCellAlignment(*grid, row, col));
                }
            table.m_rows.push_back(std::move(cells));
            table.m_cellAlignments.push_back(std::move(alignments));
            }
        }

    // size the columns to their widest text
    wxBitmap measureBitmap{ 1, 1 };
    wxMemoryDC measureDC{ measureBitmap };
    wxFont bodyFont{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
    bodyFont.SetPixelSize(wxSize{ 0, fontSize });
    measureDC.SetFont(bodyFont.Bold());
    for (auto& column : table.m_columns)
        {
        column.m_width = measureDC.GetTextExtent(column.m_label).GetWidth();
        }
    measureDC.SetFont(bodyFont);
    // past this many rows, only text with more characters than the column has seen is measured
    constexpr size_t MAX_FULLY_MEASURED_ROWS{ 300 };
    if (table.m_rows.size() > MAX_FULLY_MEASURED_ROWS)
        {
        wxLogWarning(L"Table '%s' has %zu rows, which is a lot of data for a report. "
                     "Consider summarizing it.",
                     m_title, table.m_rows.size());
        }
    std::vector<size_t> longestCells(table.m_columns.size(), 0);
    for (size_t rowIndex = 0; rowIndex < table.m_rows.size(); ++rowIndex)
        {
        const auto& row{ table.m_rows[rowIndex] };
        for (size_t col = 0; col < row.size(); ++col)
            {
            if (rowIndex < MAX_FULLY_MEASURED_ROWS || row[col].length() > longestCells[col])
                {
                longestCells[col] = std::max(longestCells[col], row[col].length());
                table.m_columns[col].m_width = std::max(
                    table.m_columns[col].m_width, measureDC.GetTextExtent(row[col]).GetWidth());
                }
            }
        }
    // the viewer's font can be wider than the font measured here
    constexpr double FONT_WIDTH_ALLOWANCE{ 1.1 };
    for (auto& column : table.m_columns)
        {
        column.m_width =
            static_cast<int>(std::ceil(column.m_width * FONT_WIDTH_ALLOWANCE)) + (cellPadding * 2);
        }
    return table;
    }
