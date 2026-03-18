/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_WORDCLOUD_DIALOG_H
#define INSERT_WORDCLOUD_DIALOG_H

#include "insertgraphdlg.h"
#include <vector>
#include <wx/spinctrl.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a word cloud into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting a word column and an optional weight column.
            - Labels showing the current variable selections.
            - Frequency filtering options (min, max, max words).*/
    class InsertWordCloudDlg final : public InsertGraphDlg
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
        InsertWordCloudDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                           const wxString& caption = _(L"Insert Word Cloud"),
                           wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                           const wxSize& size = wxDefaultSize,
                           long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                           EditMode editMode = EditMode::Insert);

        /// @private
        InsertWordCloudDlg(const InsertWordCloudDlg&) = delete;
        /// @private
        InsertWordCloudDlg& operator=(const InsertWordCloudDlg&) = delete;

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

        /// @returns The word variable name (categorical column).
        [[nodiscard]]
        const wxString& GetWordVariable() const noexcept
            {
            return m_wordVariable;
            }

        /// @returns The weight variable name, or empty if none.
        [[nodiscard]]
        const wxString& GetWeightVariable() const noexcept
            {
            return m_weightVariable;
            }

        /// @returns The minimum frequency threshold.
        [[nodiscard]]
        size_t GetMinFrequency() const noexcept;

        /// @returns The maximum frequency, or @c std::nullopt if zero (unlimited).
        [[nodiscard]]
        std::optional<size_t> GetMaxFrequency() const noexcept;

        /// @returns The maximum number of words, or @c std::nullopt if zero (unlimited).
        [[nodiscard]]
        std::optional<size_t> GetMaxWords() const noexcept;

        /// @brief Populates all dialog controls from an existing word cloud.
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
        wxStaticText* m_wordVarLabel{ nullptr };
        wxStaticText* m_weightVarLabel{ nullptr };
        wxSpinCtrl* m_minFreqSpin{ nullptr };
        wxSpinCtrl* m_maxFreqSpin{ nullptr };
        wxSpinCtrl* m_maxWordsSpin{ nullptr };

        wxString m_wordVariable;
        wxString m_weightVariable;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_WORDCLOUD_DIALOG_H
