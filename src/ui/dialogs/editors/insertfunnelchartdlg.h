/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_FUNNEL_CHART_DIALOG_H
#define INSERT_FUNNEL_CHART_DIALOG_H

#include "../../graphs/funnelchart.h"
#include "insertgraphdlg.h"
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a funnel chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              stage-label column, the value column, and an optional target column.
            - A funnel style choice (standard or glassy).
            - Checkboxes for showing explanations and conversion labels.
            - A spin control for the target ghost opacity.
            - The shared color scheme controls from InsertGraphDlg.*/
    class InsertFunnelChartDlg final : public InsertGraphDlg
        {
      public:
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
        InsertFunnelChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                             const wxString& caption = _(L"Insert Funnel Chart"),
                             wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER,
                             EditMode editMode = EditMode::Insert);

        /// @private
        InsertFunnelChartDlg(const InsertFunnelChartDlg&) = delete;
        /// @private
        InsertFunnelChartDlg& operator=(const InsertFunnelChartDlg&) = delete;

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

        /// @returns The stage-label variable name (one bar per row, in dataset order).
        [[nodiscard]]
        const wxString& GetStageVariable() const noexcept
            {
            return m_stageVariable;
            }

        /// @returns The value (actual) variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns The target variable name, or empty if targets are not used.
        [[nodiscard]]
        const wxString& GetTargetVariable() const noexcept
            {
            return m_targetVariable;
            }

        /// @returns The selected funnel style.
        [[nodiscard]]
        Graphs::FunnelChart::FunnelStyle GetFunnelStyle() const noexcept
            {
            return (m_funnelStyleIndex == 0) ? Graphs::FunnelChart::FunnelStyle::Glassy :
                                               Graphs::FunnelChart::FunnelStyle::Standard;
            }

        /// @returns Whether explanations are shown.
        [[nodiscard]]
        bool IsShowingExplanations() const noexcept
            {
            return m_showExplanations;
            }

        /// @returns Whether conversion labels are shown.
        [[nodiscard]]
        bool IsShowingConversionLabels() const noexcept
            {
            return m_showConversionLabels;
            }

        /// @returns The ghost opacity used for the target overlay.
        [[nodiscard]]
        uint8_t GetTargetGhostOpacity() const noexcept
            {
            return static_cast<uint8_t>(m_targetGhostOpacity);
            }

        /// @brief Populates all dialog controls from an existing funnel chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a funnel chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed funnel chart.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::FunnelChart>
        BuildFunnelChart(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        static Data::Dataset::ColumnPreviewInfo
        BuildColumnPreviewInfo(const Data::Dataset& dataset);

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_stageVarLabel{ nullptr };
        wxStaticText* m_valueVarLabel{ nullptr };
        wxStaticText* m_targetVarLabel{ nullptr };

        // DDX data members
        wxString m_stageVariable;
        wxString m_valueVariable;
        wxString m_targetVariable;
        int m_funnelStyleIndex{ 0 }; // 0 = Glassy, 1 = Standard
        bool m_showExplanations{ false };
        bool m_showConversionLabels{ true };
        int m_targetGhostOpacity{ Settings::GHOST_OPACITY };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_FUNNEL_CHART_DIALOG_H
