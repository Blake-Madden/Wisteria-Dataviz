/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_PIECHART_DIALOG_H
#define INSERT_PIECHART_DIALOG_H

#include "../../base/settings.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/editlbox.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a pie chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a grouping column, an optional weight column,
              and an optional secondary grouping column (inner ring).
            - Labels showing the current variable selections.
            - A checkbox for including a donut hole.
            - Outer midpoint label display.
            - Legend placement.*/
    class InsertPieChartDlg final : public InsertGraphDlg
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
        InsertPieChartDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                          const wxString& caption = _(L"Insert Pie Chart"),
                          wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                          EditMode editMode = EditMode::Insert);

        /// @private
        InsertPieChartDlg(const InsertPieChartDlg&) = delete;
        /// @private
        InsertPieChartDlg& operator=(const InsertPieChartDlg&) = delete;

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

        /// @returns The primary grouping variable name (outer ring).
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns The weight variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetWeightVariable() const noexcept
            {
            return m_weightVariable;
            }

        /// @returns The secondary grouping variable name (inner ring), or empty if none.
        [[nodiscard]]
        const wxString& GetGroup2Variable() const noexcept
            {
            return m_group2Variable;
            }

        /// @returns Whether to include a donut hole.
        [[nodiscard]]
        bool GetIncludeDonutHole() const noexcept
            {
            return m_includeDonutHole;
            }

        /// @returns The outer midpoint label display selection index.
        [[nodiscard]]
        int GetOuterMidPointLabelDisplay() const noexcept
            {
            return m_outerMidPointDisplay;
            }

        /// @returns The outer label display selection index.
        [[nodiscard]]
        int GetOuterLabelDisplay() const noexcept
            {
            return m_outerLabelDisplay;
            }

        /// @returns The inner midpoint label display selection index.
        [[nodiscard]]
        int GetInnerMidPointLabelDisplay() const noexcept
            {
            return m_innerMidPointDisplay;
            }

        /// @returns The label placement selection index.
        [[nodiscard]]
        int GetLabelPlacement() const noexcept
            {
            return m_labelPlacement;
            }

        /// @returns The pie style selection index.
        [[nodiscard]]
        int GetPieStyle() const noexcept
            {
            return m_pieStyle;
            }

        /// @returns Whether to show outer pie labels.
        [[nodiscard]]
        bool GetShowOuterPieLabels() const noexcept
            {
            return m_showOuterPieLabels;
            }

        /// @returns Whether to show inner pie labels.
        [[nodiscard]]
        bool GetShowInnerPieLabels() const noexcept
            {
            return m_showInnerPieLabels;
            }

        /// @returns Whether to use color-matched labels.
        [[nodiscard]]
        bool GetUseColorLabels() const noexcept
            {
            return m_useColorLabels;
            }

        /// @returns The donut hole label (text, font, color, header, etc.).
        [[nodiscard]]
        const Wisteria::GraphItems::Label& GetDonutHoleLabel() const noexcept
            {
            return m_donutHoleLabel;
            }

        /// @returns The donut hole color.
        [[nodiscard]]
        const wxColour& GetDonutHoleColor() const noexcept
            {
            return m_donutHoleColor;
            }

        /// @returns The donut hole proportion (0.0 - 1.0).
        [[nodiscard]]
        double GetDonutHoleProportion() const noexcept
            {
            return safe_divide<double>(m_donutHoleProportion, 100);
            }

        /// @returns The showcase mode selection index (maps to @c PieChart::ShowcaseMode).
        [[nodiscard]]
        int GetShowcaseMode() const noexcept
            {
            return m_showcaseMode;
            }

        /// @returns The showcased ring labels selection index (maps to @c Perimeter).
        [[nodiscard]]
        int GetShowcasedRingLabels() const noexcept
            {
            return m_showcasedRingLabels;
            }

        /// @returns Whether showcased inner slices are grouped by their outer slice.
        [[nodiscard]]
        bool IsShowcaseByGroup() const noexcept
            {
            return m_showcaseByGroup;
            }

        /// @returns Whether outer pie midpoint labels are shown during inner showcasing.
        [[nodiscard]]
        bool IsShowcaseShowingOuterPieMidPointLabels() const noexcept
            {
            return m_showcaseShowOuterPieMidPointLabels;
            }

        /// @returns The explicit list of outer slice labels to showcase.
        [[nodiscard]]
        const std::vector<wxString>& GetShowcaseSlices() const noexcept
            {
            return m_showcaseSlices;
            }

        /// @returns The ghost opacity applied to non-showcased slices (0-255).
        [[nodiscard]]
        int GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /// @brief Populates all dialog controls from an existing pie chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      private:
        void CreateControls() final;
        bool Validate() final;
        void OnSelectVariables();
        void OnDatasetChanged();
        void OnEditDonutHoleLabel();
        void OnShowcaseModeChanged();
        void UpdateVariableLabels();
        void RefreshShowcaseListBox();
        wxArrayString GetOuterSliceChoices() const;
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };
        wxStaticText* m_weightVarLabel{ nullptr };
        wxStaticText* m_group2VarLabel{ nullptr };
        wxButton* m_editDonutLabelButton{ nullptr };
        wxColourPickerCtrl* m_donutColorPicker{ nullptr };
        wxStaticText* m_donutColorLabel{ nullptr };
        wxSpinCtrl* m_donutProportionSpin{ nullptr };
        wxStaticText* m_donutProportionLabel{ nullptr };
        wxStaticText* m_donutProportionPercentLabel{ nullptr };
        wxChoice* m_showcaseModeChoice{ nullptr };
        wxChoice* m_showcasedRingChoice{ nullptr };
        wxCheckBox* m_showcaseByGroupCheck{ nullptr };
        wxCheckBox* m_showcaseShowOuterMidPtsCheck{ nullptr };
        wxEditableListBox* m_showcaseListBox{ nullptr };

        // DDX data members
        bool m_includeDonutHole{ false };
        bool m_showOuterPieLabels{ true };
        bool m_showInnerPieLabels{ true };
        bool m_useColorLabels{ false };
        int m_outerMidPointDisplay{ 1 }; // BinPercentage
        int m_outerLabelDisplay{ 4 };    // BinName
        int m_innerMidPointDisplay{ 1 }; // BinPercentage
        int m_labelPlacement{ 1 };       // Flush
        int m_pieStyle{ 0 };             // None
        int m_showcaseMode{ 0 };         // ShowcaseMode::None
        int m_showcasedRingLabels{ 1 };  // Perimeter::Outer
        bool m_showcaseByGroup{ false };
        bool m_showcaseShowOuterPieMidPointLabels{ false };
        int m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };
        int m_donutHoleProportion{ 50 }; // stored as 0-100, divided by 100 for API

        wxString m_groupVariable;
        wxString m_weightVariable;
        wxString m_group2Variable;

        std::vector<wxString> m_datasetNames;
        std::vector<wxString> m_showcaseSlices;

        Wisteria::GraphItems::Label m_donutHoleLabel;
        wxColour m_donutHoleColor{ *wxWHITE };
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PIECHART_DIALOG_H
