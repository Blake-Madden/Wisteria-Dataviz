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
            - A variable selection mode (all columns, or a custom list
              mixing plain column names with regex formulas).
            - Alternate row color options.
            - Alternate row color options.
            - Minimum width/height proportion controls.*/
    class InsertTableDlg final : public InsertGraphDlg
        {
      public:
        /// @brief Variable selection mode.
        enum class VarMode
            {
            Everything, ///< All columns.
            Custom      ///< Custom list of plain names and/or regex formulas.
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

        /// @returns The variable formula string (`{{Everything()}}`)
        ///     if "all columns" is selected, or an empty string for
        ///     custom mode.
        [[nodiscard]]
        wxString GetVariableFormula() const;

        /// @returns The custom list of column entries (each entry may be
        ///     a plain column name or a formula like `{{Matches(\`pat\`)}}`).
        ///     Only meaningful when GetVarMode() returns VarMode::Custom.
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

        /// @returns The full @c alternate-row-color JSON template (for round-tripping).
        /// @details Preserves any advanced sub-properties (e.g., @c start and @c stops)
        ///     that were loaded from an existing table, only overwriting the user-editable
        ///     color when it has actually been changed in the picker.
        [[nodiscard]]
        wxString GetAlternateRowColorTemplate() const;

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

        /// @brief Information about an aggregate column or row.
        struct AggregateEntry
            {
            /// @brief The name of the aggregate.
            wxString m_name;
            /// @brief The type of aggregate ("column" or "row").
            wxString m_type{ L"column" };
            /// @brief How to aggregate.
            AggregateType m_aggregateType{ AggregateType::Total };
            /// @brief The start column/row.
            wxString m_start;
            /// @brief The start position's dimension ("row"/"column"), empty if none.
            wxString m_startDimension;
            /// @brief Offset applied to the start position (0 = no offset).
            int m_startOffset{ 0 };
            /// @brief The end column/row.
            wxString m_end;
            /// @brief The end position's dimension ("row"/"column"), empty if none.
            wxString m_endDimension;
            /// @brief Offset applied to the end position (0 = no offset).
            int m_endOffset{ 0 };
            /// @brief The insertion position.
            std::optional<size_t> m_position;
            /// @brief Whether to use adjacent cell color.
            bool m_useAdjacentColor{ false };
            /// @brief The background color.
            wxColour m_bkColor;
            /// @brief The raw background string (named/"{{constant}}"), empty if none.
            wxString m_bkColorStr;
            };

        /// @returns The aggregates to add to the table.
        [[nodiscard]]
        const std::vector<AggregateEntry>& GetAggregates() const noexcept
            {
            return m_aggregates;
            }

        /// @brief A cell annotation entry.
        struct AnnotationEntry
            {
            /// @brief How cells are selected for annotation.
            enum class CellMode
                {
                Outliers, ///< Outliers in a column.
                TopN,     ///< Top-N values in a column.
                Range     ///< A named range of cells.
                };
            /// @brief The annotation text.
            wxString m_value;
            /// @brief Whether the annotation appears on the right side (false = left).
            bool m_sideRight{ true };
            /// @brief The background color (invalid = none).
            wxColour m_bgColor;
            /// @brief How cells are selected.
            CellMode m_cellMode{ CellMode::Outliers };
            /// @brief Column name for Outliers and TopN modes.
            wxString m_columnName;
            /// @brief Number of top values for TopN mode.
            int m_topN{ 1 };
            /// @brief Range start cell name for Range mode.
            wxString m_rangeStart;
            /// @brief Range end cell name for Range mode.
            wxString m_rangeEnd;
            };

        /// @returns The cell annotation entries.
        [[nodiscard]]
        const std::vector<AnnotationEntry>& GetAnnotationEntries() const noexcept
            {
            return m_annotationEntries;
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
        void RefreshVariablesList();
        void OnAddFootnote();
        void OnEditFootnote();
        void OnRemoveFootnote();
        void RefreshFootnoteList();
        void OnAddAggregate();
        void OnEditAggregate();
        void OnRemoveAggregate();
        void RefreshAggregateList();
        void OnAddAnnotation();
        void OnEditAnnotation();
        void OnRemoveAnnotation();
        void RefreshAnnotationList();
        [[nodiscard]]
        wxArrayString GetColumnNames() const;
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // static helpers shared across the formatting page
        static bool ShowAnnotationDlg(wxWindow* parent, const wxString& title,
                                      AnnotationEntry& entry, const wxArrayString& columnNames);

        // starts at +2 to avoid collision with
        // InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_VAR_MODE_RADIO{ wxID_HIGHEST + 5 };
        constexpr static wxWindowID ID_ANNOTATIONS_SECTION{ wxID_HIGHEST + 6 };

        wxChoice* m_datasetChoice{ nullptr };
        wxRadioBox* m_varModeRadio{ nullptr };
        wxEditableListBox* m_variablesListBox{ nullptr };
        wxButton* m_varButton{ nullptr };
        wxColourPickerCtrl* m_altRowColorPicker{ nullptr };
        wxEditableListBox* m_footnotesListBox{ nullptr };
        wxEditableListBox* m_aggregatesListBox{ nullptr };
        wxEditableListBox* m_annotationsListBox{ nullptr };

        // DDX data members
        int m_varModeIndex{ 0 };
        bool m_transpose{ false };
        bool m_boldHeaderRow{ true };
        bool m_centerHeaderRow{ true };
        bool m_boldFirstColumn{ false };
        bool m_clearTrailingRowFormatting{ false };
        bool m_alternateRowColors{ false };
        // cached alternate-row-color template (preserves start/stops on edit)
        wxString m_alternateRowColorTemplate;
        bool m_useMinWidth{ false };
        bool m_useMinHeight{ false };
        // stored as 0–100 percentage; converted to 0.0–1.0
        // in the getter
        int m_minWidthPct{ 100 };
        int m_minHeightPct{ 100 };

        std::vector<wxString> m_variableNames;
        std::vector<wxString> m_datasetNames;
        std::vector<std::pair<wxString, wxString>> m_footnotes;
        std::vector<AggregateEntry> m_aggregates;
        std::vector<AnnotationEntry> m_annotationEntries;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_TABLE_DIALOG_H
