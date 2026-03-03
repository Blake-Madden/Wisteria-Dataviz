/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef ODS_PREVIEW_DIALOG_H
#define ODS_PREVIEW_DIALOG_H

#include "../../import/ods_extract_text.h"
#include "dialogwithhelp.h"
#include <set>
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Data provider for ODS worksheet.
    class OdsTable final : public wxGridTableBase
        {
      public:
        /// @private
        OdsTable() = delete;

        /// @brief Constructor.
        /// @param wrk The worksheet to preview.
        /// @param odsFile The ODS extractor that @c wrk belongs to.
        OdsTable(lily_of_the_valley::ods_extract_text::worksheet* wrk,
                 lily_of_the_valley::ods_extract_text* odsFile)
            : m_wrk(wrk), m_odsFile(odsFile)
            {
            }

        // these are pure virtual in wxGridTableBase
        /// @private
        [[nodiscard]]
        int GetNumberRows() final
            {
            wxASSERT(m_wrk && m_odsFile);
            return (m_wrk != nullptr) ? static_cast<int>(m_wrk->size()) : 0;
            }

        /// @private
        [[nodiscard]]
        int GetNumberCols() final
            {
            wxASSERT(m_wrk && m_odsFile);
            return ((m_wrk != nullptr) && !m_wrk->empty()) ? (*m_wrk)[0].size() : 0;
            }

        /// @private
        [[nodiscard]]
        wxString GetValue(int row, int col) final
            {
            wxASSERT(m_wrk && m_odsFile);
            wxCHECK_MSG((row >= 0 && row < GetNumberRows()) && (col >= 0 && col < GetNumberCols()),
                        wxString{}, L"Invalid row or column index in OdsTable");
            return (m_odsFile != nullptr && m_wrk != nullptr) ?
                       wxString((*m_wrk)[row][col].get_value()) :
                       wxString{};
            }

        /// @private
        void SetValue([[maybe_unused]] int row, [[maybe_unused]] int col,
                      [[maybe_unused]] const wxString& s) final
            {
            wxASSERT_MSG(0, L"SetValue not supported in OdsTable class.");
            }

      private:
        lily_of_the_valley::ods_extract_text::worksheet* m_wrk{ nullptr };
        lily_of_the_valley::ods_extract_text* m_odsFile{ nullptr };
        };

    /// @brief Import preview dialog for an ODS worksheet.
    /// @warning This dialog only currently supports text cells.\n
    ///     All other cell types (e.g., numbers) are ignored.
    class OdsPreviewDlg final : public Wisteria::UI::DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param wrk The worksheet to preview.
            @param odsFile The ODS extractor that @c wrk belongs to.
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param pos The dialog's pos.
            @param size The dialog's size.
            @param style The dialog's style.*/
        OdsPreviewDlg(wxWindow* parent, lily_of_the_valley::ods_extract_text::worksheet* wrk,
                      lily_of_the_valley::ods_extract_text* odsFile, wxWindowID id = wxID_ANY,
                      const wxString& caption = wxString{}, const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER)
            : m_wrk(wrk), m_odsFile(odsFile)
            {
            wxWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
            Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

            Bind(wxEVT_BUTTON, &OdsPreviewDlg::OnOK, this, wxID_OK);
            Bind(wxEVT_RADIOBOX, &OdsPreviewDlg::OnChangeImportMethod, this);

            CreateControls();
            GetSizer()->SetSizeHints(this);
            Centre();
            }

        /// @private
        OdsPreviewDlg() = delete;
        /// @private
        OdsPreviewDlg(const OdsPreviewDlg& that) = delete;
        /// @private
        OdsPreviewDlg& operator=(const OdsPreviewDlg& that) = delete;

        /// @returns @c true if a given cell was inside the preview grid's
        ///     selected cells.
        /// @param cell The cell to review.
        [[nodiscard]]
        bool IsCellSelected(const wxGridCellCoords& cell) const
            {
            if (m_selectedRows.contains(cell.GetRow()))
                {
                return true;
                }
            if (m_selectedColumns.contains(cell.GetCol()))
                {
                return true;
                }
            if (std::ranges::find(m_selectedCells, cell) != m_selectedCells.end())
                {
                return true;
                }

            return std::ranges::any_of(m_selectedBlocks,
                                       [&](const auto& selBlocks)
                                       {
                                           return cell.GetRow() >= selBlocks.first.GetRow() &&
                                                  cell.GetRow() <= selBlocks.second.GetRow() &&
                                                  cell.GetCol() >= selBlocks.first.GetCol() &&
                                                  cell.GetCol() <= selBlocks.second.GetCol();
                                       });
            }

        /// @returns Whether importing only the selected cells was specified.
        ///     If @c false, then the entire sheet should be imported.
        [[nodiscard]]
        bool IsImportingOnlySelectedCells() const noexcept
            {
            return (m_importMethod == 1);
            }

      private:
        void OnOK([[maybe_unused]] wxCommandEvent& event);
        void OnChangeImportMethod([[maybe_unused]] wxCommandEvent& event);
        // creates the controls and sizers
        void CreateControls();
        lily_of_the_valley::ods_extract_text::worksheet* m_wrk{ nullptr };
        lily_of_the_valley::ods_extract_text* m_odsFile{ nullptr };
        wxGrid* m_grid{ nullptr };
        int m_importMethod{ 0 };
        // selection info
        std::vector<wxGridCellCoords> m_selectedCells;
        std::set<int> m_selectedRows;
        std::set<int> m_selectedColumns;
        std::vector<std::pair<wxGridCellCoords, wxGridCellCoords>> m_selectedBlocks;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // ODS_PREVIEW_DIALOG_H
