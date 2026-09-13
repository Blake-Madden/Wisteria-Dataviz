/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_NIGHTINGALE_ROSE_CHART_DIALOG_H
#define INSERT_NIGHTINGALE_ROSE_CHART_DIALOG_H

#include "../../graphs/nightingale_rose_chart.h"
#include "insertgraphdlg.h"
#include <cstdint>
#include <utility>
#include <vector>
#include <wx/editlbox.h>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Nightingale rose chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              category column, an optional aggregate column, and an optional series
              (grouping) column.
            - Labels showing the current variable selections.
            - Radial scaling and series display options, a start angle, and a
              show-labels checkbox.
            - A Legend page (used when a series column is selected).*/
    class InsertNightingaleRoseChartDlg final : public InsertGraphDlg
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
        InsertNightingaleRoseChartDlg(
            Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
            const wxString& caption = _(L"Insert Nightingale Rose Chart"), wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            EditMode editMode = EditMode::Insert);

        /// @private
        InsertNightingaleRoseChartDlg(const InsertNightingaleRoseChartDlg&) = delete;
        /// @private
        InsertNightingaleRoseChartDlg& operator=(const InsertNightingaleRoseChartDlg&) = delete;

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

        /// @returns The category variable name (the angular slices).
        [[nodiscard]]
        const wxString& GetCategoryVariable() const noexcept
            {
            return m_categoryVariable;
            }

        /// @returns The aggregate variable name, or empty for frequency counts.
        [[nodiscard]]
        const wxString& GetAggregateVariable() const noexcept
            {
            return m_aggregateVariable;
            }

        /// @returns The series (grouping) variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns How a wedge's value maps to its radius.
        [[nodiscard]]
        Graphs::NightingaleRoseChart::RadialScaling GetRadialScaling() const noexcept;

        /// @returns How the series within an angular slice are arranged.
        [[nodiscard]]
        Graphs::NightingaleRoseChart::SeriesDisplay GetSeriesDisplay() const noexcept;

        /// @returns The angle (in degrees) where the first slice starts.
        [[nodiscard]]
        double GetStartAngle() const;

        /// @returns Whether category labels are shown around the perimeter.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @returns The opacity that ghosted wedges are drawn with.
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return static_cast<uint8_t>(m_ghostOpacity);
            }

        /// @returns The wedges to ghost, as {group label, category label} pairs.
        ///     An empty category label means every slice.
        [[nodiscard]]
        const std::vector<std::pair<wxString, wxString>>& GetGhostedWedges() const noexcept
            {
            return m_ghostedWedges;
            }

        /// @brief Populates all dialog controls from an existing Nightingale rose chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a Nightingale rose chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed Nightingale rose chart.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::NightingaleRoseChart>
        BuildNightingaleRoseChart(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        void RefreshGhostedWedgesList();
        /// @brief Prompts for a {group, category} ghost entry.
        /// @param[in,out] groupLabel The series label (required on OK).
        /// @param[in,out] categoryLabel The slice label (empty for every slice).
        /// @returns @c true if the user accepted the entry.
        bool EditGhostOptions(wxString& groupLabel, wxString& categoryLabel);
        static Data::Dataset::ColumnPreviewInfo
        BuildColumnPreviewInfo(const Data::Dataset& dataset);

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_categoryVarLabel{ nullptr };
        wxStaticText* m_aggregateVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };
        // validators do not work with wxSpinCtrlDouble, so this is read directly
        wxSpinCtrlDouble* m_startAngleSpin{ nullptr };
        wxEditableListBox* m_ghostedWedgesList{ nullptr };

        // DDX data members
        wxString m_categoryVariable;
        wxString m_aggregateVariable;
        wxString m_groupVariable;
        int m_radialScalingSelection{ 0 }; // 0 = Area, 1 = Radius
        int m_seriesDisplaySelection{ 0 }; // 0 = Overlaid, 1 = Stacked
        bool m_showLabels{ true };
        int m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };
        std::vector<std::pair<wxString, wxString>> m_ghostedWedges;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_NIGHTINGALE_ROSE_CHART_DIALOG_H
