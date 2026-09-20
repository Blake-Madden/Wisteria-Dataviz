/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_DUBOIS_SPIRAL_CHART_DIALOG_H
#define INSERT_DUBOIS_SPIRAL_CHART_DIALOG_H

#include "../../graphs/duboisspiralchart.h"
#include "insertgraphdlg.h"
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Du Bois spiral chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              label column and the value column.
            - A choice for how values are displayed in the labels.
            - A checkbox for showing labels.
            - Spin controls for the zigzag angle, spiral radius, and line thickness.
            - The shared color scheme controls from InsertGraphDlg.*/
    class InsertDuBoisSpiralChartDlg final : public InsertGraphDlg
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
        InsertDuBoisSpiralChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                   wxWindow* parent,
                                   const wxString& caption = _(L"Insert Du Bois Spiral Chart"),
                                   wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                   const wxSize& size = wxDefaultSize,
                                   long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                                wxRESIZE_BORDER,
                                   EditMode editMode = EditMode::Insert);

        /// @private
        InsertDuBoisSpiralChartDlg(const InsertDuBoisSpiralChartDlg&) = delete;
        /// @private
        InsertDuBoisSpiralChartDlg& operator=(const InsertDuBoisSpiralChartDlg&) = delete;

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

        /// @returns The label variable name (one segment per row, in dataset order).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The value variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns How values are formatted in the labels.
        [[nodiscard]]
        NumberDisplay GetValueFormat() const noexcept;

        /// @returns Whether labels are shown.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @returns The zigzag angle (in degrees below horizontal).
        [[nodiscard]]
        double GetZigZagAngle() const noexcept
            {
            return m_zigZagAngle;
            }

        /// @returns The outer radius of the spiral, as a proportion of the plot size.
        [[nodiscard]]
        double GetOuterRadiusProportion() const;

        /// @returns The line thickness, as a proportion of the plot size.
        [[nodiscard]]
        double GetLineThicknessProportion() const;

        /// @brief Populates all dialog controls from an existing Du Bois spiral chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a Du Bois spiral chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed chart.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::DuBoisSpiralChart>
        BuildDuBoisSpiralChart(const Graphs::Graph2D* oldGraph = nullptr);

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
        wxStaticText* m_labelVarLabel{ nullptr };
        wxStaticText* m_valueVarLabel{ nullptr };

        // validators do not work with wxSpinCtrlDouble, so these are read directly
        wxSpinCtrlDouble* m_outerRadiusSpin{ nullptr };
        wxSpinCtrlDouble* m_lineThicknessSpin{ nullptr };

        // DDX data members
        wxString m_labelVariable;
        wxString m_valueVariable;
        int m_valueFormatIndex{ 0 }; // matches the order of the choices (see GetValueFormat())
        bool m_showLabels{ true };
        int m_zigZagAngle{ static_cast<int>(Graphs::DuBoisSpiralChart::DEFAULT_ZIGZAG_ANGLE) };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_DUBOIS_SPIRAL_CHART_DIALOG_H
