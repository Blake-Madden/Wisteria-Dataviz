/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_BULLET_CHART_DIALOG_H
#define INSERT_BULLET_CHART_DIALOG_H

#include "../../graphs/bulletchart.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/listctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a bullet chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              label column, the actual-value column, and an optional target-value column.
            - A value-display format choice and callout/range-label visibility checkboxes.
            - Range color scheme controls (two-tone gradient vs. met/did-not-meet-goal
              shading) with their respective color pickers.
            - A "Ranges" page for defining the shared qualitative performance bands.*/
    class InsertBulletChartDlg final : public InsertGraphDlg
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
        InsertBulletChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                             const wxString& caption = _(L"Insert Bullet Chart"),
                             wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER,
                             EditMode editMode = EditMode::Insert);

        /// @private
        InsertBulletChartDlg(const InsertBulletChartDlg&) = delete;
        /// @private
        InsertBulletChartDlg& operator=(const InsertBulletChartDlg&) = delete;

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

        /// @returns The label variable name (one KPI per row).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The actual-value variable name.
        [[nodiscard]]
        const wxString& GetActualVariable() const noexcept
            {
            return m_actualVariable;
            }

        /// @returns The target-value variable name.
        [[nodiscard]]
        const wxString& GetTargetVariable() const noexcept
            {
            return m_targetVariable;
            }

        /// @returns The shared qualitative performance bands.
        [[nodiscard]]
        const std::vector<Graphs::BulletChart::Range>& GetRanges() const noexcept
            {
            return m_ranges;
            }

        /// @returns How the qualitative performance bands are colored.
        [[nodiscard]]
        Graphs::BulletChartRangeColorScheme GetRangeColorScheme() const noexcept
            {
            return (m_rangeColorSchemeIndex == 1) ? Graphs::BulletChartRangeColorScheme::TwoTone :
                                                    Graphs::BulletChartRangeColorScheme::GoalStatus;
            }

        /// @returns The selected range-start color (for the two-tone scheme).
        [[nodiscard]]
        wxColour GetRangeStartColor() const;

        /// @returns The selected range-end color (for the two-tone scheme).
        [[nodiscard]]
        wxColour GetRangeEndColor() const;

        /// @returns The selected met-goal color (for the goal-status scheme).
        [[nodiscard]]
        wxColour GetMetGoalColor() const;

        /// @returns The selected did-not-meet-goal color (for the goal-status scheme).
        [[nodiscard]]
        wxColour GetDidNotMeetGoalColor() const;

        /// @returns How the actual/target values are displayed in the value callouts.
        [[nodiscard]]
        Graphs::BulletChartValueFormat GetValueDisplayFormat() const noexcept
            {
            return (m_valueFormatIndex == 1) ? Graphs::BulletChartValueFormat::Percentage :
                                               Graphs::BulletChartValueFormat::Value;
            }

        /// @returns Whether the "Actual"/"Target" value callouts are shown.
        [[nodiscard]]
        bool IsShowingValueCallouts() const noexcept
            {
            return m_showValueCallouts;
            }

        /// @returns Whether the qualitative bands' axis bracket labels are shown.
        [[nodiscard]]
        bool IsShowingRangeLabels() const noexcept
            {
            return m_showRangeLabels;
            }

        /// @brief Populates all dialog controls from an existing bullet chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a bullet chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed bullet chart.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::BulletChart>
        BuildBulletChart(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        static Data::Dataset::ColumnPreviewInfo
        BuildColumnPreviewInfo(const Data::Dataset& dataset);

        // ranges page helpers
        void CreateRangesPage();
        void RefreshRangesList();
        void OnAddRange();
        void OnEditRange();
        void OnRemoveRange();
        void OnMoveRangeUp();
        void OnMoveRangeDown();

        [[nodiscard]]
        long GetSelectedRangeIndex() const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };
        constexpr static wxWindowID ID_RANGES_SECTION{ wxID_HIGHEST + 5 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_labelVarLabel{ nullptr };
        wxStaticText* m_actualVarLabel{ nullptr };
        wxStaticText* m_targetVarLabel{ nullptr };

        wxColourPickerCtrl* m_rangeStartColorPicker{ nullptr };
        wxColourPickerCtrl* m_rangeEndColorPicker{ nullptr };
        wxColourPickerCtrl* m_metGoalColorPicker{ nullptr };
        wxColourPickerCtrl* m_didNotMeetGoalColorPicker{ nullptr };

        wxListView* m_rangesList{ nullptr };

        // DDX data members
        wxString m_labelVariable;
        wxString m_actualVariable;
        wxString m_targetVariable;
        int m_rangeColorSchemeIndex{ 0 }; // 0 = GoalStatus, 1 = TwoTone
        int m_valueFormatIndex{ 0 };      // 0 = Value, 1 = Percentage
        bool m_showValueCallouts{ true };
        bool m_showRangeLabels{ true };

        std::vector<wxString> m_datasetNames;
        std::vector<Graphs::BulletChart::Range> m_ranges;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_BULLET_CHART_DIALOG_H
