/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef INSERT_LIKERT_DIALOG_H
#define INSERT_LIKERT_DIALOG_H

#include "../../graphs/likertchart.h"
#include "insertgraphdlg.h"
#include <vector>
#include <wx/clrpicker.h>
#include <wx/editlbox.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Dialog for inserting a Likert chart into a canvas cell.
        @details Extends InsertGraphDlg with an "Options" page containing:
            - A dataset selector (from the project's datasets).
            - A "Variables..." button that opens a VariableSelectDlg
              for selecting question columns and an optional grouping column.
            - Labels showing the current variable selections.
            - A survey format choice.
            - Checkboxes for show response counts, show percentages,
              show section headers, and adjust bar widths.
            - Color pickers for negative, positive, neutral, and no-response colors.
            - Text fields for positive, negative, and no-response header labels.*/
    class InsertLikertDlg final : public InsertGraphDlg
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
        InsertLikertDlg(Canvas* canvas, const ReportBuilder* reportBuilder, wxWindow* parent,
                        const wxString& caption = _(L"Insert Likert Chart"),
                        wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                        const wxSize& size = wxDefaultSize,
                        long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER,
                        EditMode editMode = EditMode::Insert);

        /// @private
        InsertLikertDlg(const InsertLikertDlg&) = delete;
        /// @private
        InsertLikertDlg& operator=(const InsertLikertDlg&) = delete;

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

        /// @returns The selected question variable names (with any constant
        ///     placeholders expanded).
        [[nodiscard]]
        std::vector<wxString> GetQuestionVariables() const
            {
            std::vector<wxString> expanded;
            expanded.reserve(m_questionVariables.size());
            for (const auto& var : m_questionVariables)
                {
                expanded.push_back(ExpandVariable(var));
                }
            return expanded;
            }

        /// @returns The grouping variable name (with any constant placeholders
        ///     expanded), or empty if none.
        [[nodiscard]]
        wxString GetGroupVariable() const
            {
            return ExpandVariable(m_groupVariable);
            }

        /// @returns Whether to show response counts.
        [[nodiscard]]
        bool GetShowResponseCounts() const noexcept
            {
            return m_showResponseCounts;
            }

        /// @returns Whether to show percentages on bars.
        [[nodiscard]]
        bool GetShowPercentages() const noexcept
            {
            return m_showPercentages;
            }

        /// @returns Whether to show section headers.
        [[nodiscard]]
        bool GetShowSectionHeaders() const noexcept
            {
            return m_showSectionHeaders;
            }

        /// @returns Whether to adjust bar widths to respondent size.
        [[nodiscard]]
        bool GetAdjustBarWidths() const noexcept
            {
            return m_adjustBarWidths;
            }

        /// @returns The negative color.
        [[nodiscard]]
        wxColour GetNegativeColor() const;

        /// @returns The positive color.
        [[nodiscard]]
        wxColour GetPositiveColor() const;

        /// @returns The neutral color.
        [[nodiscard]]
        wxColour GetNeutralColor() const;

        /// @returns The no-response color.
        [[nodiscard]]
        wxColour GetNoResponseColor() const;

        /// @returns The positive header label.
        [[nodiscard]]
        const wxString& GetPositiveLabel() const noexcept
            {
            return m_positiveLabel;
            }

        /// @returns The negative header label.
        [[nodiscard]]
        const wxString& GetNegativeLabel() const noexcept
            {
            return m_negativeLabel;
            }

        /// @returns The no-response header label.
        [[nodiscard]]
        const wxString& GetNoResponseLabel() const noexcept
            {
            return m_noResponseLabel;
            }

        /// @returns The question brackets.
        [[nodiscard]]
        const std::vector<Graphs::LikertChart::QuestionsBracket>&
        GetQuestionsBrackets() const noexcept
            {
            return m_questionBrackets;
            }

        /// @brief Populates all dialog controls from an existing Likert chart.
        /// @param graph The graph to read settings from.
        void LoadFromGraph(const Graphs::Graph2D& graph);

      protected:
        void CreateControls() override;

      private:
        bool Validate() override;
        void OnSelectVariables();
        void OnDatasetChanged();
        void OnEditQuestionsBracket([[maybe_unused]] wxCommandEvent& event);
        void OnRemoveQuestionsBracket([[maybe_unused]] wxCommandEvent& event);
        void UpdateVariableLabels();
        void SyncBracketsToList();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        [[nodiscard]]
        static int
        SurveyFormatToIndex(Graphs::LikertChart::LikertSurveyQuestionFormat format) noexcept;

        // starts at +2 to avoid collision with InsertItemDlg::ID_PAGE_SECTION (+1)
        constexpr static wxWindowID ID_OPTIONS_SECTION{ wxID_HIGHEST + 2 };
        constexpr static wxWindowID ID_DATASET_CHOICE{ wxID_HIGHEST + 3 };
        constexpr static wxWindowID ID_SELECT_VARS_BUTTON{ wxID_HIGHEST + 4 };

        wxChoice* m_datasetChoice{ nullptr };
        wxStaticText* m_questionsVarLabel{ nullptr };
        wxStaticText* m_groupVarLabel{ nullptr };

        wxColourPickerCtrl* m_negativeColorPicker{ nullptr };
        wxColourPickerCtrl* m_positiveColorPicker{ nullptr };
        wxColourPickerCtrl* m_neutralColorPicker{ nullptr };
        wxColourPickerCtrl* m_noResponseColorPicker{ nullptr };

        wxEditableListBox* m_bracketListBox{ nullptr };

        // DDX data members
        bool m_showResponseCounts{ false };
        bool m_showPercentages{ true };
        bool m_showSectionHeaders{ true };
        bool m_adjustBarWidths{ false };

        wxString m_positiveLabel{ _(L"Positive") };
        wxString m_negativeLabel{ _(L"Negative") };
        wxString m_noResponseLabel{ _(L"No Response") };

        std::vector<wxString> m_questionVariables;
        wxString m_groupVariable;

        std::vector<Graphs::LikertChart::QuestionsBracket> m_questionBrackets;

        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/// @}

#endif // INSERT_LIKERT_DIALOG_H
