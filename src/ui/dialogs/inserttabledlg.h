/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_TABLE_DIALOG_H
#define INSERT_TABLE_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/editlbox.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a table into a canvas cell.
        @details Extends InsertGraphDlg with a "Table" page containing:
            - A dataset selector (from the project's datasets).
            - A variable selection mode (all columns, pattern match,
              pattern exclusion, or custom selection via VariableSelectDlg).
            - Alternate row color options.
            - Alternate row color options.
            - Minimum width/height proportion controls.*/
    class InsertTableDlg final : public InsertGraphDlg
        {
      public:
        /// @brief Variable selection mode.
        enum class VarMode
            {
            Everything,       ///< All columns.
            Matches,          ///< Columns matching a regex pattern.
            EverythingExcept, ///< All columns except those matching a pattern.
            Custom            ///< Manual column selection.
            };

        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder containing the
                project's datasets.
            @param parent The parent window.
            @param caption The dialog caption.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.*/
        InsertTableDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                       const wxString& caption = _(L"Insert Table"), wxWindowID id = wxID_ANY,
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                       EditMode editMode = EditMode::Insert);

        /// @private
        InsertTableDlg(const InsertTableDlg&) = delete;
        /// @private
        InsertTableDlg& operator=(const InsertTableDlg&) = delete;

        /// @returns The selected dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The name of the selected dataset, or empty if none.
        [[nodiscard]]
        wxString GetSelectedDatasetName() const
            {
            const int sel = m_datasetChoice->GetSelection();
            return (sel != wxNOT_FOUND && std::cmp_less(sel, m_datasetNames.size())) ?
                       m_datasetNames[sel] :
                       wxString{};
            }

        /// @returns The variable selection mode.
        [[nodiscard]]
        VarMode GetVarMode() const noexcept
            {
            return static_cast<VarMode>(m_varModeIndex);
            }

        /// @returns The variable formula string
        ///     (e.g., `{{Everything()}}` or `{{Matches(\`pattern\`)}}`)
        ///     if a formula mode is selected, or an empty string
        ///     for custom mode.
        [[nodiscard]]
        wxString GetVariableFormula() const;

        /// @returns The manually selected variable names
        ///     (only meaningful when GetVarMode() returns VarMode::Custom).
        [[nodiscard]]
        const std::vector<wxString>& GetSelectedVariables() const noexcept
            {
            return m_variableNames;
            }

        /// @returns Whether to transpose the table.
        [[nodiscard]]
        bool GetTranspose() const noexcept
            {
            return m_transpose;
            }

        /// @returns Whether to bold the header row.
        [[nodiscard]]
        bool GetBoldHeaderRow() const noexcept
            {
            return m_boldHeaderRow;
            }

        /// @returns Whether to center the header row.
        [[nodiscard]]
        bool GetCenterHeaderRow() const noexcept
            {
            return m_centerHeaderRow;
            }

        /// @returns Whether to bold the first column.
        [[nodiscard]]
        bool GetBoldFirstColumn() const noexcept
            {
            return m_boldFirstColumn;
            }

        /// @returns Whether to clear trailing row formatting.
        [[nodiscard]]
        bool GetClearTrailingRowFormatting() const noexcept
            {
            return m_clearTrailingRowFormatting;
            }

        /// @returns Whether to apply alternate row colors.
        [[nodiscard]]
        bool GetAlternateRowColors() const noexcept
            {
            return m_alternateRowColors;
            }

        /// @returns The alternate row color.
        [[nodiscard]]
        wxColour GetAlternateRowColor() const
            {
            return (m_altRowColorPicker != nullptr) ? m_altRowColorPicker->GetColour() : *wxWHITE;
            }

        /// @returns The minimum width proportion (0.0–1.0),
        ///     or @c std::nullopt for auto-fit.
        [[nodiscard]]
        std::optional<double> GetMinWidthProportion() const noexcept
            {
            return m_useMinWidth ? std::optional<double>(m_minWidthPct / 100.0) : std::nullopt;
            }

        /// @returns The minimum height proportion (0.0–1.0),
        ///     or @c std::nullopt for auto-fit.
        [[nodiscard]]
        std::optional<double> GetMinHeightProportion() const noexcept
            {
            return m_useMinHeight ? std::optional<double>(m_minHeightPct / 100.0) : std::nullopt;
            }

        /// @returns The footnotes as (value, footnote) pairs.
        [[nodiscard]]
        const std::vector<std::pair<wxString, wxString>>& GetFootnotes() const noexcept
            {
            return m_footnotes;
            }

        /// @brief Populates all dialog controls from an existing table.
        /// @param graph The table graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void OnVarModeChanged();
        void UpdateVariableLabels();
        void OnAddFootnote();
        void OnEditFootnote();
        void OnRemoveFootnote();
        void RefreshFootnoteList();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // starts at +2 to avoid collision with
        // InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_VAR_MODE_RADIO{ wxID_HIGHEST + 5 };

        wxChoice* m_datasetChoice{ nullptr };
        wxRadioBox* m_varModeRadio{ nullptr };
        wxTextCtrl* m_varPatternCtrl{ nullptr };
        wxButton* m_varButton{ nullptr };
        wxStaticText* m_varsLabelCaption{ nullptr };
        wxStaticText* m_varsLabel{ nullptr };
        wxColourPickerCtrl* m_altRowColorPicker{ nullptr };
        wxEditableListBox* m_footnotesListBox{ nullptr };

        // DDX data members
        int m_varModeIndex{ 0 };
        wxString m_varPattern;
        bool m_transpose{ false };
        bool m_boldHeaderRow{ true };
        bool m_centerHeaderRow{ true };
        bool m_boldFirstColumn{ false };
        bool m_clearTrailingRowFormatting{ false };
        bool m_alternateRowColors{ false };
        bool m_useMinWidth{ false };
        bool m_useMinHeight{ false };
        // stored as 0–100 percentage; converted to 0.0–1.0
        // in the getter
        int m_minWidthPct{ 100 };
        int m_minHeightPct{ 100 };

        std::vector<wxString> m_variableNames;
        std::vector<wxString> m_datasetNames;
        std::vector<std::pair<wxString, wxString>> m_footnotes;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_TABLE_DIALOG_H
