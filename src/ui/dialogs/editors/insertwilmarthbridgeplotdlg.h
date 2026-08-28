/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_WILMARTH_BRIDGE_PLOT_DIALOG_H
#define INSERT_WILMARTH_BRIDGE_PLOT_DIALOG_H

#include "../../graphs/wilmarth_bridge_plot.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Wilmarth bridge plot into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting the label, exit, entry, status, and intermediate event columns.
            - Labels showing the current variable selections.
            - Display options (fade effect, survival statistics, censored markers,
              intermediate event color).*/
    class InsertWilmarthBridgePlotDlg final : public InsertGraphDlg
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
        InsertWilmarthBridgePlotDlg(
            Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
            const wxString& caption = _(L"Insert Wilmarth Bridge Plot"), wxWindowID id = wxID_ANY,
            const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
            long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
            EditMode editMode = EditMode::Insert);

        /// @private
        InsertWilmarthBridgePlotDlg(const InsertWilmarthBridgePlotDlg&) = delete;
        /// @private
        InsertWilmarthBridgePlotDlg& operator=(const InsertWilmarthBridgePlotDlg&) = delete;

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

        /// @returns The label variable name (categorical column).
        [[nodiscard]]
        const wxString& GetLabelVariable() const noexcept
            {
            return m_labelVariable;
            }

        /// @returns The exit variable name (continuous or date column).
        [[nodiscard]]
        const wxString& GetExitVariable() const noexcept
            {
            return m_exitVariable;
            }

        /// @returns The entry variable name, or empty if not selected.
        [[nodiscard]]
        const wxString& GetEntryVariable() const noexcept
            {
            return m_entryVariable;
            }

        /// @returns The status variable name, or empty if not selected.
        [[nodiscard]]
        const wxString& GetStatusVariable() const noexcept
            {
            return m_statusVariable;
            }

        /// @returns The intermediate event variable name, or empty if not selected.
        [[nodiscard]]
        const wxString& GetIntermediateEventVariable() const noexcept
            {
            return m_intermediateEventVariable;
            }

        /// @returns The color used for an observation's cells from its intermediate
        ///     event onward.
        [[nodiscard]]
        wxColour GetIntermediateEventColor() const;

        /// @returns How an observation's label ink weakens over its lifetime.
        [[nodiscard]]
        Graphs::WilmarthBridgePlot::FadeEffect GetFadeEffect() const noexcept;

        /// @returns What survival statistics to display alongside the grid.
        [[nodiscard]]
        Graphs::WilmarthBridgePlot::SurvivalDisplay GetSurvivalDisplay() const noexcept;

        /// @returns Whether censored observations are marked with an arrow.
        [[nodiscard]]
        bool IsShowingCensoredMarkers() const noexcept
            {
            return m_showCensoredMarkers;
            }

        /// @returns The label for the terminal row appended after the last observed
        ///     period, or empty if no terminal row is being shown.
        [[nodiscard]]
        const wxString& GetTerminalRowLabel() const noexcept
            {
            return m_terminalRowLabel;
            }

        /// @brief Populates all dialog controls from an existing Wilmarth bridge plot.
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
        wxStaticText* m_labelVarLabel{ nullptr };
        wxStaticText* m_exitVarLabel{ nullptr };
        wxStaticText* m_entryVarLabel{ nullptr };
        wxStaticText* m_statusVarLabel{ nullptr };
        wxStaticText* m_intermediateEventVarLabel{ nullptr };
        wxColourPickerCtrl* m_intermediateEventColorPicker{ nullptr };

        // DDX data members
        wxString m_labelVariable;
        wxString m_exitVariable;
        wxString m_entryVariable;
        wxString m_statusVariable;
        wxString m_intermediateEventVariable;
        int m_fadeEffectSelection{ 0 }; // 0 = None, 1 = RemainingLifetime, 2 = ElapsedTime
        int m_survivalDisplaySelection{
            0
        }; // 0 = None, 1 = AtRiskCount, 2 = SurvivalPercent, 3 = Both
        bool m_showCensoredMarkers{ true };
        wxString m_terminalRowLabel;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_WILMARTH_BRIDGE_PLOT_DIALOG_H
