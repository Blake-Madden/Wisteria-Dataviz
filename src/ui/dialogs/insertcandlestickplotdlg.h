/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_CANDLESTICKPLOT_DIALOG_H
#define INSERT_CANDLESTICKPLOT_DIALOG_H

#include "../../graphs/candlestickplot.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a candlestick plot into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting date, open, high, low, and close columns.
            - Labels showing the current variable selections.
            - A plot type choice (Candlestick or OHLC).*/
    class InsertCandlestickPlotDlg final : public InsertGraphDlg
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
        InsertCandlestickPlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                 wxWindow* parent,
                                 const wxString& caption = _(L"Insert Candlestick Plot"),
                                 wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                 const wxSize& size = wxDefaultSize,
                                 long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                              wxRESIZE_BORDER,
                                 EditMode editMode = EditMode::Insert);

        /// @private
        InsertCandlestickPlotDlg(const InsertCandlestickPlotDlg&) = delete;
        /// @private
        InsertCandlestickPlotDlg& operator=(const InsertCandlestickPlotDlg&) = delete;

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

        /// @returns The date variable name.
        [[nodiscard]]
        const wxString& GetDateVariable() const noexcept
            {
            return m_dateVariable;
            }

        /// @returns The open variable name.
        [[nodiscard]]
        const wxString& GetOpenVariable() const noexcept
            {
            return m_openVariable;
            }

        /// @returns The high variable name.
        [[nodiscard]]
        const wxString& GetHighVariable() const noexcept
            {
            return m_highVariable;
            }

        /// @returns The low variable name.
        [[nodiscard]]
        const wxString& GetLowVariable() const noexcept
            {
            return m_lowVariable;
            }

        /// @returns The close variable name.
        [[nodiscard]]
        const wxString& GetCloseVariable() const noexcept
            {
            return m_closeVariable;
            }

        /// @returns The selected plot type (Candlestick or OHLC).
        [[nodiscard]]
        Graphs::CandlestickPlot::PlotType GetPlotType() const noexcept
            {
            return (m_plotTypeIndex == 1) ? Graphs::CandlestickPlot::PlotType::Ohlc :
                                            Graphs::CandlestickPlot::PlotType::Candlestick;
            }

        /// @brief Populates all dialog controls from an existing candlestick plot.
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
        wxStaticText* m_dateVarLabel{ nullptr };
        wxStaticText* m_openVarLabel{ nullptr };
        wxStaticText* m_highVarLabel{ nullptr };
        wxStaticText* m_lowVarLabel{ nullptr };
        wxStaticText* m_closeVarLabel{ nullptr };

        // DDX data members
        int m_plotTypeIndex{ 0 };

        wxString m_dateVariable;
        wxString m_openVariable;
        wxString m_highVariable;
        wxString m_lowVariable;
        wxString m_closeVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_CANDLESTICKPLOT_DIALOG_H
