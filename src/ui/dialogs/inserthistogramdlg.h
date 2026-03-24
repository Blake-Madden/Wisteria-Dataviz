/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_HISTOGRAM_DIALOG_H
#define INSERT_HISTOGRAM_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a histogram into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a continuous column and an optional grouping column.
            - Labels showing the current variable selections.
            - Binning method, rounding method, interval display, and bin label options.
            - Checkboxes for showing the full range and neat intervals.
            - Legend placement.*/
    class InsertHistogramDlg final : public InsertGraphDlg
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
        InsertHistogramDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                           const wxString& caption = _(L"Insert Histogram"),
                           wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           EditMode editMode = EditMode::Insert);

        /// @private
        InsertHistogramDlg(const InsertHistogramDlg&) = delete;
        /// @private
        InsertHistogramDlg& operator=(const InsertHistogramDlg&) = delete;

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

        /// @returns The continuous variable name.
        [[nodiscard]]
        const wxString& GetContinuousVariable() const noexcept
            {
            return m_continuousVariable;
            }

        /// @returns The grouping variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns The binning method selection index.
        [[nodiscard]]
        int GetBinningMethod() const noexcept
            {
            return m_binningMethod;
            }

        /// @returns The rounding method selection index.
        [[nodiscard]]
        int GetRoundingMethod() const noexcept
            {
            return m_roundingMethod;
            }

        /// @returns The interval display selection index.
        [[nodiscard]]
        int GetIntervalDisplay() const noexcept
            {
            return m_intervalDisplay;
            }

        /// @returns The bin label display selection index.
        [[nodiscard]]
        int GetBinLabelDisplay() const noexcept
            {
            return m_binLabelDisplay;
            }

        /// @returns Whether to show the full range of values.
        [[nodiscard]]
        bool GetShowFullRange() const noexcept
            {
            return m_showFullRange;
            }

        /// @returns Whether to use neat (rounded) intervals.
        [[nodiscard]]
        bool GetNeatIntervals() const noexcept
            {
            return m_neatIntervals;
            }

        /// @returns The selected color scheme, or @c nullptr for default.
        [[nodiscard]]
        std::shared_ptr<Colors::Schemes::ColorScheme> GetColorScheme() const
            {
            return ColorSchemeFromIndex(m_colorSchemeIndex);
            }

        /// @brief Populates all dialog controls from an existing histogram.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

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
        wxStaticText* m_continuousVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        // DDX data members
        int m_colorSchemeIndex{ 0 };
        int m_binningMethod{ 2 };   // BinByIntegerRange
        int m_roundingMethod{ 3 };  // NoRounding
        int m_intervalDisplay{ 0 }; // Cutpoints
        int m_binLabelDisplay{ 0 }; // BinValue
        bool m_showFullRange{ true };
        bool m_neatIntervals{ false };

        wxString m_continuousVariable;
        wxString m_groupVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_HISTOGRAM_DIALOG_H
