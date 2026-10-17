/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_ISHIKAWA_DIAGRAM_DIALOG_H
#define INSERT_ISHIKAWA_DIAGRAM_DIALOG_H

#include "../../graphs/ishikawadiagram.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/scrolwin.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting an Ishikawa (cause-and-effect) diagram into a canvas cell.
        @details Extends InsertGraphDlg with an "Ishikawa Diagram" page containing:
            - A dataset selector and the effect text.
            - A grid of branches (six by default). Each branch has a label,
              the column containing its causes, and a "Sub-causes..." button for assigning
              columns of sub-causes to its causes.
            - Display options.*/
    class InsertIshikawaDiagramDlg final : public InsertGraphDlg
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
        InsertIshikawaDiagramDlg(Canvas* canvas, const ReportBuilder* reportBuilder,
                                 wxWindow* parent,
                                 const wxString& caption = _(L"Insert Ishikawa Diagram"),
                                 wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                                 const wxSize& size = wxDefaultSize,
                                 long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                              wxRESIZE_BORDER,
                                 EditMode editMode = EditMode::Insert);

        /// @private
        InsertIshikawaDiagramDlg(const InsertIshikawaDiagramDlg&) = delete;
        /// @private
        InsertIshikawaDiagramDlg& operator=(const InsertIshikawaDiagramDlg&) = delete;

        /// @returns The effect (problem) text.
        [[nodiscard]]
        const wxString& GetEffect() const noexcept
            {
            return m_effect;
            }

        /// @returns The branches.
        [[nodiscard]]
        const std::vector<Graphs::IshikawaDiagram::Branch>& GetBranches() const noexcept
            {
            return m_branches;
            }

        /// @brief Populates all dialog controls from an existing diagram.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

        /** @brief Constructs a diagram from the dialog's current settings.
            @param oldGraph The previous graph being edited, or @c nullptr if inserting new.
            @returns The newly constructed diagram.*/
        [[nodiscard]]
        std::shared_ptr<Graphs::IshikawaDiagram>
        BuildIshikawaDiagram(const Graphs::Graph2D* oldGraph = nullptr);

      protected:
        void CreateControls() override;
        [[nodiscard]]
        DataGuide GetDataGuide() const final;

      private:
        bool Validate() override;
        void OnDatasetChanged();
        /// @brief Removes sub-causes that name a column that isn't in the selected dataset,
        ///     or that belong to a cause that its branch no longer has.
        void RemoveStaleSubCauses();

        // branch grid helpers
        void RebuildBranchRows();
        void OnAddBranch();
        void OnRemoveBranch(size_t index);
        void OnBranchColumnChanged(size_t index, int selection);
        void OnEditSubCauses(size_t index);

        /// @returns The names of the selected dataset's categorical columns.
        [[nodiscard]]
        wxArrayString GetColumnNames() const;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };

        wxScrolledWindow* m_branchPanel{ nullptr };
        wxFlexGridSizer* m_branchGrid{ nullptr };

        // DDX data members
        wxString m_effect;
        bool m_showEmptyBranches{ true };
        bool m_showBranchLabels{ true };
        bool m_showCauseCounts{ false };
        bool m_taperedLines{ true };
        bool m_monochrome{ false };

        // the branches that the grid shows
        std::vector<Graphs::IshikawaDiagram::Branch> m_branches;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_ISHIKAWA_DIAGRAM_DIALOG_H
