/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_WATERFALL_CHART_DIALOG_H
#define INSERT_WATERFALL_CHART_DIALOG_H

#include "../../graphs/waterfallchart.h"
#include "insertgraphdlg.h"
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a waterfall chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg for selecting the
              step-label column, the change-amount column, and an optional total-flag
              column (@c 0 = change, @c 1 = total).
            - A bar orientation choice (vertical or horizontal).
            - A value-display format choice and bar/block value visibility checkboxes.
            - Color pickers for the increase, decrease, and total bars.*/
    class InsertWaterfallChartDlg final : public InsertGraphDlg
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
        InsertWaterfallChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                wxWindow* parent,
                                const wxString& caption = _(L"Insert Waterfall Chart"),
                                wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                const wxSize& size = wxDefaultSize,
                                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                             wxRESIZE_BORDER,
                                EditMode editMode = EditMode::Insert);

        /// @private
        InsertWaterfallChartDlg(const InsertWaterfallChartDlg&) = delete;
        /// @private
        InsertWaterfallChartDlg& operator=(const InsertWaterfallChartDlg&) = delete;

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

        /// @returns The step-label variable name (one bar per row, in dataset order).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The change-amount variable name.
        [[nodiscard]]
        const wxString& GetValueVariable() const noexcept
            {
            return m_valueVariable;
            }

        /// @returns The total-flag variable name, or empty if totals are not used.
        [[nodiscard]]
        const wxString& GetTotalFlagVariable() const noexcept
            {
            return m_totalFlagVariable;
            }

        /// @returns The bar orientation to use.
        [[nodiscard]]
        Wisteria::Orientation GetOrientation() const noexcept
            {
            return (m_orientationIndex == 1) ? Wisteria::Orientation::Horizontal :
                                               Wisteria::Orientation::Vertical;
            }

        /// @returns The selected increase-bar color.
        [[nodiscard]]
        wxColour GetIncreaseColor() const;

        /// @returns The selected decrease-bar color.
        [[nodiscard]]
        wxColour GetDecreaseColor() const;

        /// @returns The selected total-bar color.
        [[nodiscard]]
        wxColour GetTotalColor() const;

        /// @returns How the values on the bars and blocks are displayed.
        [[nodiscard]]
        NumberDisplay GetValueDisplay() const noexcept
            {
            switch (m_valueFormatIndex)
                {
            case 1:
                return NumberDisplay::Currency;
            case 2:
                return NumberDisplay::Percentage;
            case 3:
                return NumberDisplay::ValueSimple;
            case 0:
                [[fallthrough]];
            default:
                return NumberDisplay::Value;
                }
            }

        /// @returns Whether the values at the ends of the bars are shown.
        [[nodiscard]]
        bool IsShowingBarValues() const noexcept
            {
            return m_showBarValues;
            }

        /// @returns Whether the values across the bars are shown.
        [[nodiscard]]
        bool IsShowingBlockValues() const noexcept
            {
            return m_showBlockValues;
            }

        /// @brief Populates all dialog controls from an existing waterfall chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a waterfall chart from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed waterfall chart.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::WaterfallChart>
        BuildWaterfallChart(const Graphs::Graph2D* oldGraph = nullptr);

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
        wxStaticText* m_totalFlagVarLabel{ nullptr };

        wxColourPickerCtrl* m_increaseColorPicker{ nullptr };
        wxColourPickerCtrl* m_decreaseColorPicker{ nullptr };
        wxColourPickerCtrl* m_totalColorPicker{ nullptr };

        // DDX data members
        wxString m_labelVariable;
        wxString m_valueVariable;
        wxString m_totalFlagVariable;
        int m_orientationIndex{ 0 }; // 0 = Vertical, 1 = Horizontal
        int m_valueFormatIndex{ 0 }; // 0 = Value, 1 = Currency, 2 = Percentage, 3 = ValueSimple
        bool m_showBarValues{ true };
        bool m_showBlockValues{ false };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_WATERFALL_CHART_DIALOG_H
