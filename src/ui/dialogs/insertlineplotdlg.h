/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_LINEPLOT_DIALOG_H
#define INSERT_LINEPLOT_DIALOG_H

#include "../../base/icons.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a line plot into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting Y, X, and an optional grouping column.
            - Labels showing the current variable selections.
            - A checkbox for auto-splining.
            - Legend placement.*/
    class InsertLinePlotDlg final : public InsertGraphDlg
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
        InsertLinePlotDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                          const wxString& caption = _(L"Insert Line Plot"),
                          wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                          const wxSize& size = wxDefaultSize,
                          long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                          EditMode editMode = EditMode::Insert);

        /// @private
        InsertLinePlotDlg(const InsertLinePlotDlg&) = delete;
        /// @private
        InsertLinePlotDlg& operator=(const InsertLinePlotDlg&) = delete;

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

        /// @returns The Y variable name (continuous column).
        [[nodiscard]]
        const wxString& GetYVariable() const noexcept
            {
            return m_yVariable;
            }

        /// @returns The X variable name (continuous, categorical, or date column).
        [[nodiscard]]
        const wxString& GetXVariable() const noexcept
            {
            return m_xVariable;
            }

        /// @returns The grouping variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetGroupVariable() const noexcept
            {
            return m_groupVariable;
            }

        /// @returns Whether to enable auto-splining.
        [[nodiscard]]
        bool GetAutoSpline() const noexcept
            {
            return m_autoSpline;
            }

        /// @returns The selected shape scheme for the line plot points.
        [[nodiscard]]
        std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> GetShapeScheme() const
            {
            switch (m_shapeSchemeIndex)
                {
            case 1:
                return std::make_shared<Wisteria::Icons::Schemes::Semesters>();
            case 0:
                [[fallthrough]];
            default:
                return std::make_shared<Wisteria::Icons::Schemes::StandardShapes>();
                }
            }

        /// @brief Populates all dialog controls from an existing line plot.
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
        wxStaticText* m_xVarLabel{ nullptr };
        wxStaticText* m_yVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        // DDX data members
        bool m_autoSpline{ true };
        int m_shapeSchemeIndex{ 0 };

        wxString m_xVariable;
        wxString m_yVariable;
        wxString m_groupVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_LINEPLOT_DIALOG_H
