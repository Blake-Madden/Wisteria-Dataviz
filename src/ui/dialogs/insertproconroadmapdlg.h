/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_PROCONROADMAP_DIALOG_H
#define INSERT_PROCONROADMAP_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Pro & Con Roadmap into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting positive, negative, and optional value columns.
            - Labels showing the current variable selections.
            - A spin control for the minimum count threshold.
            - Text controls for positive and negative legend labels.
            - A checkbox for adding the default explanatory caption.
            - Legend placement.*/
    class InsertProConRoadmapDlg final : public InsertGraphDlg
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
        InsertProConRoadmapDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                               const wxString& caption = _(L"Insert Pro && Con Roadmap"),
                               wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                               const wxSize& size = wxDefaultSize,
                               long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN |
                                            wxRESIZE_BORDER,
                               EditMode editMode = EditMode::Insert);

        /// @private
        InsertProConRoadmapDlg(const InsertProConRoadmapDlg&) = delete;
        /// @private
        InsertProConRoadmapDlg& operator=(const InsertProConRoadmapDlg&) = delete;

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

        /// @returns The positive (pros) column name.
        [[nodiscard]]
        const wxString& GetPositiveVariable() const noexcept
            {
            return m_positiveVariable;
            }

        /// @returns The positive value column name, or empty if using frequency counts.
        [[nodiscard]]
        const wxString& GetPositiveValueVariable() const noexcept
            {
            return m_positiveValueVariable;
            }

        /// @returns The negative (cons) column name.
        [[nodiscard]]
        const wxString& GetNegativeVariable() const noexcept
            {
            return m_negativeVariable;
            }

        /// @returns The negative value column name, or empty if using frequency counts.
        [[nodiscard]]
        const wxString& GetNegativeValueVariable() const noexcept
            {
            return m_negativeValueVariable;
            }

        /// @returns The minimum count for inclusion, or @c std::nullopt if not filtering.
        [[nodiscard]]
        std::optional<size_t> GetMinimumCount() const;

        /// @returns The positive legend label.
        [[nodiscard]]
        const wxString& GetPositiveLabel() const noexcept
            {
            return m_positiveLabel;
            }

        /// @returns The negative legend label.
        [[nodiscard]]
        const wxString& GetNegativeLabel() const noexcept
            {
            return m_negativeLabel;
            }

        /// @returns Whether to add the default explanatory caption.
        [[nodiscard]]
        bool GetAddDefaultCaption() const noexcept
            {
            return m_addDefaultCaption;
            }

        /// @brief Populates all dialog controls from an existing Pro & Con Roadmap.
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

        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_positiveVarLabel{ nullptr };
        wxStaticText* m_positiveValueVarLabel{ nullptr };
        wxStaticText* m_negativeVarLabel{ nullptr };
        wxStaticText* m_negativeValueVarLabel{ nullptr };

        // DDX data members
        wxString m_positiveLabel{ _(L"Pro") };
        wxString m_negativeLabel{ _(L"Con") };
        bool m_addDefaultCaption{ true };

        wxString m_positiveVariable;
        wxString m_positiveValueVariable;
        wxString m_negativeVariable;
        wxString m_negativeValueVariable;

        // controls without DDX support
        wxSpinCtrl* m_minCountSpin{ nullptr };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_PROCONROADMAP_DIALOG_H
