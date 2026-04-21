/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_HEATMAP_DIALOG_H
#define INSERT_HEATMAP_DIALOG_H

#include "../../graphs/heatmap.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a heat map into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a continuous column and an optional grouping column.
            - Labels showing the current variable selections.
            - A group column count spin control (1-5).
            - A checkbox for showing group headers.
            - A text field for the group header prefix.
            - A color scheme selector.
            - Legend placement.*/
    class InsertHeatMapDlg final : public InsertGraphDlg
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
        InsertHeatMapDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                         const wxString& caption = _(L"Insert Heat Map"), wxWindowID id = wxID_ANY,
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                         EditMode editMode = EditMode::Insert);

        /// @private
        InsertHeatMapDlg(const InsertHeatMapDlg&) = delete;
        /// @private
        InsertHeatMapDlg& operator=(const InsertHeatMapDlg&) = delete;

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

        /// @returns The continuous variable name (with any constant placeholders expanded).
        [[nodiscard]]
        wxString GetContinuousVariable() const
            {
            return ExpandVariable(m_continuousVariable);
            }

        /// @returns The grouping variable name (with any constant placeholders
        ///     expanded), or empty if none.
        [[nodiscard]]
        wxString GetGroupVariable() const
            {
            return ExpandVariable(m_groupVariable);
            }

        /// @returns The group column count (1-5).
        [[nodiscard]]
        int GetGroupColumnCount() const noexcept
            {
            return m_groupColumnCount;
            }

        /// @returns Whether to show group headers.
        [[nodiscard]]
        bool GetShowGroupHeaders() const noexcept
            {
            return m_showGroupHeaders;
            }

        /// @returns The group header prefix.
        [[nodiscard]]
        const wxString& GetGroupHeaderPrefix() const noexcept
            {
            return m_groupHeaderPrefix;
            }

        /// @brief Populates all dialog controls from an existing heat map.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void UpdateVariableLabels();
        void UpdateGroupControlStates();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_continuousVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        wxStaticText* m_groupColumnCountLabel{ nullptr };
        wxSpinCtrl* m_groupColumnCountSpin{ nullptr };
        wxCheckBox* m_showGroupHeadersCheck{ nullptr };
        wxStaticText* m_groupHeaderPrefixLabel{ nullptr };
        wxTextCtrl* m_groupHeaderPrefixText{ nullptr };

        // DDX data members
        int m_groupColumnCount{ 1 };
        bool m_showGroupHeaders{ true };
        wxString m_groupHeaderPrefix{ _(L"Groups") };

        wxString m_continuousVariable;
        wxString m_groupVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_HEATMAP_DIALOG_H
