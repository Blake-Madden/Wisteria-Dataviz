/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_SANKEYDIAGRAM_DIALOG_H
#define INSERT_SANKEYDIAGRAM_DIALOG_H

#include "../../graphs/sankeydiagram.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Sankey diagram into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting from, to, optional weight, and optional sort columns.
            - Labels showing the current variable selections.
            - A flow shape choice (Curvy or Jagged).
            - A group label display choice.
            - A column header display choice.*/
    class InsertSankeyDiagramDlg final : public InsertGraphDlg
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
        InsertSankeyDiagramDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                               const wxString& caption = _(L"Insert Sankey Diagram"),
                               wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER,
                               EditMode editMode = EditMode::Insert);

        /// @private
        InsertSankeyDiagramDlg(const InsertSankeyDiagramDlg&) = delete;
        /// @private
        InsertSankeyDiagramDlg& operator=(const InsertSankeyDiagramDlg&) = delete;

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

        /// @returns The "from" variable name.
        [[nodiscard]]
        const wxString& GetFromVariable() const noexcept
            {
            return m_fromVariable;
            }

        /// @returns The "to" variable name.
        [[nodiscard]]
        const wxString& GetToVariable() const noexcept
            {
            return m_toVariable;
            }

        /// @returns The "from weight" variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetFromWeightVariable() const noexcept
            {
            return m_fromWeightVariable;
            }

        /// @returns The "to weight" variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetToWeightVariable() const noexcept
            {
            return m_toWeightVariable;
            }

        /// @returns The "from sort/group" variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetFromGroupVariable() const noexcept
            {
            return m_fromGroupVariable;
            }

        /// @returns The selected flow shape.
        [[nodiscard]]
        FlowShape GetFlowShape() const noexcept
            {
            return (m_flowShapeIndex == 1) ? FlowShape::Jagged : FlowShape::Curvy;
            }

        /// @returns The selected group label display.
        [[nodiscard]]
        BinLabelDisplay GetGroupLabelDisplay() const noexcept
            {
            switch (m_groupLabelDisplayIndex)
                {
            case 1:
                return BinLabelDisplay::BinValue;
            case 2:
                return BinLabelDisplay::BinPercentage;
            case 3:
                return BinLabelDisplay::BinNameAndValue;
            case 4:
                return BinLabelDisplay::BinNameAndPercentage;
            case 5:
                return BinLabelDisplay::BinValueAndPercentage;
            case 6:
                return BinLabelDisplay::NoDisplay;
            case 0:
                [[fallthrough]];
            default:
                return BinLabelDisplay::BinName;
                }
            }

        /// @returns The selected column header display.
        [[nodiscard]]
        GraphColumnHeader GetColumnHeaderDisplay() const noexcept
            {
            switch (m_columnHeaderDisplayIndex)
                {
            case 1:
                return GraphColumnHeader::AsHeader;
            case 2:
                return GraphColumnHeader::AsFooter;
            case 0:
                [[fallthrough]];
            default:
                return GraphColumnHeader::NoDisplay;
                }
            }

        /// @brief Populates all dialog controls from an existing Sankey diagram.
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
        wxStaticText* m_fromVarLabel{ nullptr };
        wxStaticText* m_toVarLabel{ nullptr };
        wxStaticText* m_fromWeightVarLabel{ nullptr };
        wxStaticText* m_toWeightVarLabel{ nullptr };
        wxStaticText* m_fromGroupVarLabel{ nullptr };

        // DDX data members
        int m_flowShapeIndex{ 0 };
        int m_groupLabelDisplayIndex{ 0 };
        int m_columnHeaderDisplayIndex{ 0 };

        wxString m_fromVariable;
        wxString m_toVariable;
        wxString m_fromWeightVariable;
        wxString m_toWeightVariable;
        wxString m_fromGroupVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_SANKEYDIAGRAM_DIALOG_H
