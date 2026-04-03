/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_SCALE_CHART_DIALOG_H
#define INSERT_SCALE_CHART_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/listctrl.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a scale chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a score (continuous) column and an optional grouping column.
            - Main scale values, precision, and column headers.
            - A "Scales" page for defining color-block scales with blocks.
            - A checkbox for showcasing the score.
            - Legend placement.*/
    class InsertScaleChartDlg final : public InsertGraphDlg
        {
      public:
        /// @brief Describes a single block within a scale.
        struct BlockInfo
            {
            wxString m_label;
            double m_length{ 10 };
            wxColour m_color{ *wxGREEN };
            };

        /// @brief Describes a complete scale (a stack of colored blocks).
        struct ScaleInfo
            {
            wxString m_header;
            std::optional<double> m_startPosition;
            std::vector<BlockInfo> m_blocks;
            };

        /** @brief Constructor.
            @param canvas The canvas whose grid layout is displayed.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param caption The dialog title.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.
            @param editMode Whether the item is being inserted or edited.*/
        InsertScaleChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                            const wxString& caption = _(L"Insert Scale Chart"),
                            wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                            EditMode editMode = EditMode::Insert);

        /// @private
        InsertScaleChartDlg(const InsertScaleChartDlg&) = delete;
        /// @private
        InsertScaleChartDlg& operator=(const InsertScaleChartDlg&) = delete;

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

        /// @returns The score variable name.
        [[nodiscard]]
        const wxString& GetScoreVariable() const noexcept
            {
            return m_scoreVariable;
            }

        /// @returns The grouping variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns Whether to showcase the score.
        [[nodiscard]]
        bool GetShowcaseScore() const noexcept
            {
            return m_showcaseScore;
            }

        /// @returns The main scale values as a comma-separated string.
        [[nodiscard]]
        const wxString& GetMainScaleValuesStr() const noexcept
            {
            return m_mainScaleValues;
            }

        /// @returns The parsed main scale values.
        [[nodiscard]]
        std::vector<double> GetMainScaleValues() const;

        /// @returns The main scale precision.
        [[nodiscard]]
        int GetMainScalePrecision() const noexcept
            {
            return m_mainScalePrecision;
            }

        /// @returns The main scale column header.
        [[nodiscard]]
        const wxString& GetMainScaleHeader() const noexcept
            {
            return m_mainScaleHeader;
            }

        /// @returns The data column header.
        [[nodiscard]]
        const wxString& GetDataColumnHeader() const noexcept
            {
            return m_dataColumnHeader;
            }

        /// @returns The defined scales.
        [[nodiscard]]
        const std::vector<ScaleInfo>& GetScales() const noexcept
            {
            return m_scales;
            }

        /// @brief Populates all dialog controls from an existing scale chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // scales page helpers
        void CreateScalesPage();
        void RefreshScalesList();
        void RefreshBlocksList();
        void OnAddScale();
        void OnRemoveScale();
        void OnAddBlock();
        void OnEditBlock();
        void OnRemoveBlock();
        void OnMoveBlockUp();
        void OnMoveBlockDown();
        void OnScaleSelected();

        [[nodiscard]]
        long GetSelectedScaleIndex() const;

        [[nodiscard]]
        long GetSelectedBlockIndex() const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_SCALES_SECTION{ wxID_HIGHEST + 5 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_scoreVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        // scales page controls
        wxListView* m_scalesList{ nullptr };
        wxListView* m_blocksList{ nullptr };
        wxTextCtrl* m_scaleHeaderCtrl{ nullptr };

        // DDX data members
        bool m_showcaseScore{ false };
        wxString m_mainScaleValues;
        int m_mainScalePrecision{ 0 };
        wxString m_mainScaleHeader;
        wxString m_dataColumnHeader;

        wxString m_scoreVariable;
        wxString m_groupVariable;

        std::vector<wxString> m_datasetNames;
        std::vector<ScaleInfo> m_scales;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_SCALE_CHART_DIALOG_H
