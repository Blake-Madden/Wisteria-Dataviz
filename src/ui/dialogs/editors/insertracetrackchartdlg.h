/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_RACETRACK_CHART_DIALOG_H
#define INSERT_RACETRACK_CHART_DIALOG_H

#include "../../graphs/racetrackchart.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a race track chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting the value and label columns.
            - Labels showing the current variable selections.
            - Track layout options (track count, angles, and proportions).*/
    class InsertRaceTrackChartDlg final : public InsertGraphDlg
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
        InsertRaceTrackChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                wxWindow* parent,
                                const wxString& caption = _(L"Insert Race Track Chart"),
                                wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize,
                                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                             wxRESIZE_BORDER,
                                EditMode editMode = EditMode::Insert);

        /// @private
        InsertRaceTrackChartDlg(const InsertRaceTrackChartDlg&) = delete;
        /// @private
        InsertRaceTrackChartDlg& operator=(const InsertRaceTrackChartDlg&) = delete;

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

        /// @returns The value variable name (continuous column).
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns The label variable name (categorical column).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns How many concentric tracks to spread the track lanes across.
        [[nodiscard]]
        Graphs::RaceTrackChart::TrackCount GetTrackCount() const noexcept;

        /// @returns The proportion of the plot area used for the track.
        [[nodiscard]]
        double GetTrackProportion() const;

        /// @returns The angle (in degrees) where the track lanes start.
        [[nodiscard]]
        double GetStartAngle() const;

        /// @returns Whether labels are shown at the start position of the track lanes.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @brief Populates all dialog controls from an existing race track chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

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
        wxStaticText* m_valueVarLabel{ nullptr };
        wxStaticText* m_labelVarLabel{ nullptr };
        // validators do not work with wxSpinCtrlDouble, so these are read directly
        wxSpinCtrlDouble* m_startAngleSpin{ nullptr };
        wxSpinCtrlDouble* m_trackProportionSpin{ nullptr };

        // DDX data members
        wxString m_valueVariable;
        wxString m_labelVariable;
        int m_trackCountSelection{ 0 }; // 0 = Auto, 1 = One, 2 = Two
        bool m_showLabels{ true };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_RACETRACK_CHART_DIALOG_H
