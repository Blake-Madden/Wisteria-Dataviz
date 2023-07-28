/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __EXCEL_PREVIEW_DIALOG_H__
#define __EXCEL_PREVIEW_DIALOG_H__

#include <set>
#include <wx/wx.h>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/valgen.h>
#include "dialogwithhelp.h"
#include "../../import/xlsx_extract_text.h"

/// @brief Data provider for Excel worksheet.
class ExcelTable final : public wxGridTableBase
    {
public:
    /// @private
    ExcelTable() = default;
    /// @brief Constructor.
    /// @param wrk The worksheet to preview.
    /// @param excelFile The excel extractor that @c wrk belongs to.
    ExcelTable(lily_of_the_valley::xlsx_extract_text::worksheet* wrk,
        lily_of_the_valley::xlsx_extract_text* excelFile) :
            m_wrk(wrk), m_excelFile(excelFile)
        {}

    // these are pure virtual in wxGridTableBase
    /// @private
    [[nodiscard]]
    int GetNumberRows() final
        {
        assert(m_wrk && m_excelFile);
        return m_wrk ? static_cast<int>(m_wrk->size()) : 0;
        }
    /// @private
    [[nodiscard]]
    int GetNumberCols() final
        {
        assert(m_wrk && m_excelFile);
        return (m_wrk && m_wrk->size()) ? (*m_wrk)[0].size() : 0;
        }
    /// @private
    [[nodiscard]]
    wxString GetValue(int row, int col) final
        {
        assert(m_wrk && m_excelFile);
        wxCHECK_MSG( (row >= 0 && row < GetNumberRows()) &&
                 (col >= 0 && col < GetNumberCols()),
                 wxString{},
                 L"invalid row or column index in ExcelTable");
        return m_excelFile ?
            wxString((*m_wrk)[row][col].get_value()) :
            wxString();
        }
    /// @private
    void SetValue([[maybe_unused]] int row, [[maybe_unused]] int col,
                  [[maybe_unused]] const wxString& s) final
        { wxASSERT_MSG(0, L"SetValue not supported in ExcelTable class."); }
private:
    lily_of_the_valley::xlsx_extract_text::worksheet* m_wrk{ nullptr };
    lily_of_the_valley::xlsx_extract_text* m_excelFile{ nullptr };
    DECLARE_DYNAMIC_CLASS_NO_COPY(ExcelTable)
    };

/// @brief Import preview dialog for an Excel worksheet.
/// @warning This dialog only currently supports text cells.\n
///     All other cell types (e.g., numbers) are ignored.
class ExcelPreviewDlg final : public Wisteria::UI::DialogWithHelp
    {
public:
    /** @brief Constructor.
        @param parent The parent window.
        @param wrk The worksheet to preview.
        @param excelFile The excel extractor that @c wrk belongs to.
        @param id The dialog's ID.
        @param caption The dialog's caption.
        @param pos The dialog's pos.
        @param size The dialog's size.
        @param style The dialog's style.*/
    ExcelPreviewDlg(wxWindow* parent, lily_of_the_valley::xlsx_extract_text::worksheet* wrk,
             lily_of_the_valley::xlsx_extract_text* excelFile,
             wxWindowID id = wxID_ANY, const wxString& caption = wxString{},
             const wxPoint& pos = wxDefaultPosition,
             const wxSize& size = wxDefaultSize,
             long style = wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER) :
             m_wrk(wrk), m_excelFile(excelFile)
        {
        SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxWS_EX_CONTEXTHELP);
        Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style );

        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }
    /// @private
    ExcelPreviewDlg(const ExcelPreviewDlg& that) = delete;
    /// @private
    ExcelPreviewDlg& operator=(const ExcelPreviewDlg& that) = delete;
    /// @returns @c true if a given cell was inside of the preview grid's selected cells.
    /// @param cell The cell to review.
    [[nodiscard]]
    bool IsCellSelected(const wxGridCellCoords& cell) const
        {
        if (m_selectedRows.find(cell.GetRow()) != m_selectedRows.end())
            { return true; }
        else if (m_selectedColumns.find(cell.GetCol()) != m_selectedColumns.end())
            { return true; }
        else if (std::find(m_selectedCells.begin(), m_selectedCells.end(), cell) !=
            m_selectedCells.end())
            { return true; }
        else
            {
            for (const auto& selBlocks : m_selectedBlocks)
                {
                if (cell.GetRow() >= selBlocks.first.GetRow() &&
                    cell.GetRow() <= selBlocks.second.GetRow() &&
                    cell.GetCol() >= selBlocks.first.GetCol() &&
                    cell.GetCol() <= selBlocks.second.GetCol())
                    { return true; }
                }
            return false;
            }
        }
    /// @returns Whether importing only the selected cells was specified.
    ///     If @c false, then the entire sheet should be imported.
    [[nodiscard]]
    bool IsImportingOnlySelectedCells() const noexcept
        { return (m_importMethod==1); }
private:
    void OnOK([[maybe_unused]] wxCommandEvent& event);
    void OnChangeImportMethod([[maybe_unused]] wxCommandEvent& event);
    // creates the controls and sizers
    void CreateControls();
    lily_of_the_valley::xlsx_extract_text::worksheet* m_wrk{ nullptr };
    lily_of_the_valley::xlsx_extract_text* m_excelFile{ nullptr};
    wxGrid* m_grid{ nullptr };
    int m_importMethod{ 0 };
    // selection info
    std::vector<wxGridCellCoords> m_selectedCells;
    std::set<int> m_selectedRows;
    std::set<int> m_selectedColumns;
    std::vector<std::pair<wxGridCellCoords,wxGridCellCoords>> m_selectedBlocks;

    ExcelPreviewDlg() = default;
    DECLARE_EVENT_TABLE()
    };

/** @}*/

#endif // __EXCEL_PREVIEW_DIALOG_H__
