/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_GANTTCHART_DIALOG_H
#define INSERT_GANTTCHART_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Gantt chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting task, start date, end date, and optional
              resource, description, completion, and grouping columns.
            - Labels showing the current variable selections.
            - Date interval and fiscal year type choices.
            - Task label display choice.
            - Legend placement.*/
    class InsertGanttChartDlg final : public InsertGraphDlg
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
        InsertGanttChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                            const wxString& caption = _(L"Insert Gantt Chart"),
                            wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                            const wxSize& size = wxDefaultSize,
                            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                            EditMode editMode = EditMode::Insert);

        /// @private
        InsertGanttChartDlg(const InsertGanttChartDlg&) = delete;
        /// @private
        InsertGanttChartDlg& operator=(const InsertGanttChartDlg&) = delete;

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

        /// @returns The task column name.
        [[nodiscard]]
        const wxString& GetTaskVariable() const noexcept
            {
            return m_taskVariable;
            }

        /// @returns The start date column name.
        [[nodiscard]]
        const wxString& GetStartDateVariable() const noexcept
            {
            return m_startDateVariable;
            }

        /// @returns The end date column name.
        [[nodiscard]]
        const wxString& GetEndDateVariable() const noexcept
            {
            return m_endDateVariable;
            }

        /// @returns The resource column name, or empty if none.
        [[nodiscard]]
        const wxString& GetResourceVariable() const noexcept
            {
            return m_resourceVariable;
            }

        /// @returns The description column name, or empty if none.
        [[nodiscard]]
        const wxString& GetDescriptionVariable() const noexcept
            {
            return m_descriptionVariable;
            }

        /// @returns The completion column name, or empty if none.
        [[nodiscard]]
        const wxString& GetCompletionVariable() const noexcept
            {
            return m_completionVariable;
            }

        /// @returns The grouping variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns The selected date interval.
        [[nodiscard]]
        DateInterval GetDateInterval() const noexcept;

        /// @returns The selected fiscal year type.
        [[nodiscard]]
        FiscalYear GetFiscalYearType() const noexcept;

        /// @returns The selected task label display mode.
        [[nodiscard]]
        Graphs::GanttChart::TaskLabelDisplay GetTaskLabelDisplay() const noexcept;

        /// @returns The selected color scheme, or @c nullptr for default.
        [[nodiscard]]
        std::shared_ptr<Colors::Schemes::ColorScheme> GetColorScheme() const
            {
            return ColorSchemeFromIndex(m_colorSchemeIndex);
            }

        /// @brief Populates all dialog controls from an existing Gantt chart.
        /// @param graph The graph to read settings from.
        /// @param canvas The canvas the graph belongs to.
        void LoadFromGraph(const Graphs::Graph2D& graph, Canvas* canvas);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_taskVarLabel{ nullptr };
        wxStaticText* m_startDateVarLabel{ nullptr };
        wxStaticText* m_endDateVarLabel{ nullptr };
        wxStaticText* m_resourceVarLabel{ nullptr };
        wxStaticText* m_descriptionVarLabel{ nullptr };
        wxStaticText* m_completionVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        // DDX data members
        int m_colorSchemeIndex{ 0 };
        int m_dateInterval{ 1 };     // default: FiscalQuarterly
        int m_fyType{ 1 };           // default: USBusiness
        int m_taskLabelDisplay{ 3 }; // default: Days

        wxString m_taskVariable;
        wxString m_startDateVariable;
        wxString m_endDateVariable;
        wxString m_resourceVariable;
        wxString m_descriptionVariable;
        wxString m_completionVariable;
        wxString m_groupVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_GANTTCHART_DIALOG_H
