/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_DUELING_PIE_CHART_DIALOG_H
#define INSERT_DUELING_PIE_CHART_DIALOG_H

#include "../../graphs/duelingpiechart.h"
#include "insertgraphdlg.h"
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a dueling pie chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              category column, the group column (which must have two values),
              and the value column.
            - A choice for what the labels inside the slices display.
            - The shared color scheme controls from InsertGraphDlg.*/
    class InsertDuelingPieChartDlg final : public InsertGraphDlg
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
        InsertDuelingPieChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                 wxWindow* parent,
                                 const wxString& caption = _(L"Insert Dueling Pie Chart"),
                                 wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                 const wxSize& size = wxDefaultSize,
                                 long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                              wxRESIZE_BORDER,
                                 EditMode editMode = EditMode::Insert);

        /// @private
        InsertDuelingPieChartDlg(const InsertDuelingPieChartDlg&) = delete;
        /// @private
        InsertDuelingPieChartDlg& operator=(const InsertDuelingPieChartDlg&) = delete;

        /// @returns The category variable name (one slice per category).
        [[nodiscard]]
        const wxString& GetCategoryVariable() const noexcept
            {
            return m_categoryVariable;
            }

        /// @returns The group variable name (which must have exactly two values).
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns The value variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns What the labels inside the slices display.
        [[nodiscard]]
        BinLabelDisplay GetMidPointLabelDisplay() const noexcept;

        /// @brief Populates all dialog controls from an existing dueling pie chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a dueling pie chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed chart.
            @throws std::runtime_error If the group variable does not have exactly two values.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::DuelingPieChart>
        BuildDuelingPieChart(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxStaticText* m_categoryVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };
        wxStaticText* m_valueVarLabel{ nullptr };

        // DDX data members
        wxString m_categoryVariable;
        wxString m_groupVariable;
        wxString m_valueVariable;
        // matches the order of the choices
        int m_midPointLabelIndex{ 0 };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_DUELING_PIE_CHART_DIALOG_H
