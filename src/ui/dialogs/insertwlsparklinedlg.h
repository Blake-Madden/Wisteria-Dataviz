/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_WL_SPARKLINE_DIALOG_H
#define INSERT_WL_SPARKLINE_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a win/loss sparkline into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting the season, won, shutout, home game,
              and an optional postseason column.
            - Labels showing the current variable selections.
            - A checkbox for highlighting best records.
            - Legend placement.*/
    class InsertWLSparklineDlg final : public InsertGraphDlg
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
        InsertWLSparklineDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                             const wxString& caption = _(L"Insert Win/Loss Sparkline"),
                             wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                             const wxSize& size = wxDefaultSize,
                             long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                          wxRESIZE_BORDER,
                             EditMode editMode = EditMode::Insert);

        /// @private
        InsertWLSparklineDlg(const InsertWLSparklineDlg&) = delete;
        /// @private
        InsertWLSparklineDlg& operator=(const InsertWLSparklineDlg&) = delete;

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

        /// @returns The season variable name.
        [[nodiscard]]
        const wxString& GetSeasonVariable() const noexcept
            {
            return m_seasonVariable;
            }

        /// @returns The won/lost variable name.
        [[nodiscard]]
        const wxString& GetWonVariable() const noexcept
            {
            return m_wonVariable;
            }

        /// @returns The shutout variable name.
        [[nodiscard]]
        const wxString& GetShutoutVariable() const noexcept
            {
            return m_shutoutVariable;
            }

        /// @returns The home game variable name.
        [[nodiscard]]
        const wxString& GetHomeGameVariable() const noexcept
            {
            return m_homeGameVariable;
            }

        /// @returns The postseason variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetPostseasonVariable() const noexcept
            {
            return m_postseasonVariable;
            }

        /// @returns Whether to highlight best records.
        [[nodiscard]]
        bool GetHighlightBestRecords() const noexcept
            {
            return m_highlightBestRecords;
            }

        /// @brief Populates all dialog controls from an existing win/loss sparkline.
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
        wxStaticText* m_seasonVarLabel{ nullptr };
        wxStaticText* m_wonVarLabel{ nullptr };
        wxStaticText* m_shutoutVarLabel{ nullptr };
        wxStaticText* m_homeGameVarLabel{ nullptr };
        wxStaticText* m_postseasonVarLabel{ nullptr };

        // DDX data members
        bool m_highlightBestRecords{ true };

        wxString m_seasonVariable;
        wxString m_wonVariable;
        wxString m_shutoutVariable;
        wxString m_homeGameVariable;
        wxString m_postseasonVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_WL_SPARKLINE_DIALOG_H
