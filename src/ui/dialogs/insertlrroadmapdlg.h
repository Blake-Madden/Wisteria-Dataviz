/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_LRROADMAP_DIALOG_H
#define INSERT_LRROADMAP_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Linear Regression Roadmap into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting predictor, coefficient, and optional p-value columns.
            - Labels showing the current variable selections.
            - A text control for the dependent variable name.
            - A choice for which predictors to include (all, positive, negative).
            - A spin control for the p-value threshold.
            - A checkbox for adding the default explanatory caption.
            - Legend placement.*/
    class InsertLRRoadmapDlg final : public InsertGraphDlg
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
        InsertLRRoadmapDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                           const wxString& caption = _(L"Insert Linear Regression Roadmap"),
                           wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           EditMode editMode = EditMode::Insert);

        /// @private
        InsertLRRoadmapDlg(const InsertLRRoadmapDlg&) = delete;
        /// @private
        InsertLRRoadmapDlg& operator=(const InsertLRRoadmapDlg&) = delete;

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

        /// @returns The predictor column name.
        [[nodiscard]]
        const wxString& GetPredictorVariable() const noexcept
            {
            return m_predictorVariable;
            }

        /// @returns The coefficient column name.
        [[nodiscard]]
        const wxString& GetCoefficientVariable() const noexcept
            {
            return m_coefficientVariable;
            }

        /// @returns The p-value column name, or empty if not used.
        [[nodiscard]]
        const wxString& GetPValueVariable() const noexcept
            {
            return m_pValueVariable;
            }

        /// @returns The dependent variable name for the legend/caption.
        [[nodiscard]]
        const wxString& GetDVName() const noexcept
            {
            return m_dvName;
            }

        /// @returns The p-value threshold, or @c std::nullopt if no p-value column is used.
        [[nodiscard]]
        std::optional<double> GetPLevel() const;

        /// @returns Which predictors to include, or @c std::nullopt for all.
        [[nodiscard]]
        std::optional<Influence> GetPredictorsToInclude() const;

        /// @returns Whether to add the default explanatory caption.
        [[nodiscard]]
        bool GetAddDefaultCaption() const noexcept
            {
            return m_addDefaultCaption;
            }

        /// @brief Populates all dialog controls from an existing LR Roadmap.
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
        wxStaticText* m_predictorVarLabel{ nullptr };
        wxStaticText* m_coefficientVarLabel{ nullptr };
        wxStaticText* m_pValueVarLabel{ nullptr };

        // DDX data members
        wxString m_dvName;
        int m_predictorsFilter{ 0 };
        bool m_addDefaultCaption{ true };

        wxString m_predictorVariable;
        wxString m_coefficientVariable;
        wxString m_pValueVariable;

        // controls without DDX support
        wxSpinCtrlDouble* m_pLevelSpin{ nullptr };

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_LRROADMAP_DIALOG_H
