/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_RUN_CHART_DIALOG_H
#define INSERT_RUN_CHART_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::Graphs
    {
    class RunChart;
    } // namespace Wisteria::Graphs

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a run chart into a canvas cell.
        @details Extends InsertGraphDlg with a "Run Chart" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting the value, label, and sample columns.
            - Sample options (none, by sample variable, or a fixed size) and
              the statistic plotted for each sample.
            - A checkbox for showing the median line, and the number of points at the start
              of the chart (the baseline) used to compute it.
            - Rules for circling shifts, trends, alternating points, and too few or
              too many runs.*/
    class InsertRunChartDlg final : public InsertGraphDlg
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
        InsertRunChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                          const wxString& caption = _(L"Insert Run Chart"),
                          wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                          EditMode editMode = EditMode::Insert);

        /// @private
        InsertRunChartDlg(const InsertRunChartDlg&) = delete;
        /// @private
        InsertRunChartDlg& operator=(const InsertRunChartDlg&) = delete;

        /// @returns The value (measurement) variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns The label variable name (empty if not used).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The sample variable name (empty if not used).
        [[nodiscard]]
        const wxString& GetSampleVariable() const noexcept
            {
            return m_sampleVariable;
            }

        /// @brief Populates all dialog controls from an existing run chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a run chart from the dialog's current settings.
            @param oldGraph The previous graph being replaced (if editing), used to
                carry forward property templates; @c nullptr if inserting a new chart.
            @returns The newly constructed chart.*/
        std::shared_ptr<Graphs::RunChart> BuildRunChart(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;
        [[nodiscard]]
        DataGuide GetDataGuide() const final;

      private:
        /// @brief How observations are collected into plotted points.
        enum class SampleMode
            {
            None,
            ByVariable,
            FixedSize
            };

        /// @returns The variables of the chart, in the order of the variable selection dialog.
        [[nodiscard]]
        static std::vector<DataGuideParameter> GetParameters();

        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        void UpdateSampleControls();
        void SyncSampleModeToVariable();
        void UpdateRuleControls();
        void UpdateBaselineControls();

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_SAMPLE_MODE{ wxID_HIGHEST + 5 };

        wxStaticText* m_valueVarLabel{ nullptr };
        wxStaticText* m_labelVarLabel{ nullptr };
        wxStaticText* m_sampleVarLabel{ nullptr };
        wxSpinCtrl* m_sampleSizeSpin{ nullptr };
        wxChoice* m_statisticChoice{ nullptr };
        wxCheckBox* m_individualPointsCheck{ nullptr };
        wxSpinCtrl* m_individualOpacitySpin{ nullptr };
        wxSpinCtrl* m_shiftLengthSpin{ nullptr };
        wxSpinCtrl* m_trendLengthSpin{ nullptr };
        wxSpinCtrl* m_alternatingLengthSpin{ nullptr };
        wxCheckBox* m_baselineCheck{ nullptr };
        wxSpinCtrl* m_baselineLengthSpin{ nullptr };

        wxString m_valueVariable;
        wxString m_labelVariable;
        wxString m_sampleVariable;

        // indexes into the sample mode and statistic choices
        int m_sampleMode{ 0 };
        int m_sampleSize{ 5 };
        int m_sampleStatistic{ 0 };
        bool m_showMedianLine{ true };
        bool m_useBaseline{ false };
        int m_baselineLength{ 20 };
        bool m_showIndividualPoints{ false };
        int m_individualPointOpacity{ 90 };

        // new charts start with the run chart rules on, unlike RunChart, which starts with none
        bool m_useShiftRule{ true };
        int m_shiftLength{ 6 };
        bool m_useTrendRule{ true };
        int m_trendLength{ 5 };
        bool m_useAlternatingRule{ false };
        int m_alternatingLength{ 14 };
        bool m_useRunsRule{ true };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_RUN_CHART_DIALOG_H
