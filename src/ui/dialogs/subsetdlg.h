/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef SUBSET_DLG_H
#define SUBSET_DLG_H

#include "../../base/reportbuilder.h"
#include "../../base/settings.h"
#include "../../data/subset.h"
#include "../controls/datasetgridtable.h"
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Options describing how a subset was created from a dataset.
        @details Stored alongside the dataset for future serialization.*/
    struct SubsetOptions
        {
        /// @brief The name of the source dataset that was subsetted.
        wxString m_sourceDatasetName;
        /// @brief The user-specified name for the output dataset.
        wxString m_outputName;

        /// @brief Which filter type is active.
        enum class FilterType
            {
            Single,
            And,
            Or,
            Section
            };

        /// @brief Which filter type is active.
        FilterType m_filterType{ FilterType::Single };

        /// @brief A single filter criterion (column + operator + values).
        struct FilterCriterion
            {
            /// @brief The column to filter on.
            wxString m_column;
            /// @brief The comparison operator (e.g., "=", "!=", "<", ">").
            wxString m_operator{ L"=" };
            /// @brief The filter values (OR'd within a single criterion).
            std::vector<wxString> m_values;
            };

        /// @brief The filter criteria (used for Single, And, and Or filter types).
        std::vector<FilterCriterion> m_filters;

        /// @brief The column to filter on (Section filter type).
        wxString m_sectionColumn;
        /// @brief The label marking the start of the section.
        wxString m_sectionStartLabel;
        /// @brief The label marking the end of the section.
        wxString m_sectionEndLabel;
        /// @brief Whether to include the sentinel (start/end) rows in the subset.
        bool m_sectionIncludeSentinelLabels{ true };
        };

    /** @brief Dialog for creating or editing a dataset subset.
        @details Provides:
            - A dataset selector (from the project's datasets).
            - A filter type selector (single, AND, OR, or section).
            - For filter modes: a dynamic list of filter criteria,
              each with a column selector, comparison operator, and values field.
            - For section mode: column, start label, end label,
              and an include-sentinels checkbox.
            - A live preview grid showing the subsetted result.
            - An output dataset name field.*/
    class SubsetDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        SubsetDlg(const ReportBuilder* reportBuilder, wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);
        /** @brief Constructor that pre-populates the dialog with existing subset settings.
            @param reportBuilder The report builder containing the project's datasets.
            @param subsetOptions The subset options to pre-populate.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        SubsetDlg(const ReportBuilder* reportBuilder, const SubsetOptions& subsetOptions,
                  wxWindow* parent, wxWindowID id = wxID_ANY,
                  const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                  long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        SubsetDlg(const SubsetDlg&) = delete;
        /// @private
        SubsetDlg& operator=(const SubsetDlg&) = delete;

        /// @returns The selected source dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The name of the selected source dataset.
        [[nodiscard]]
        wxString GetSelectedDatasetName() const;

        /// @returns The subsetted result dataset, or @c nullptr if not yet generated.
        [[nodiscard]]
        const std::shared_ptr<Data::Dataset>& GetSubsettedDataset() const noexcept
            {
            return m_subsettedDataset;
            }

        /// @returns The user-specified output dataset name.
        [[nodiscard]]
        wxString GetOutputName() const;

        /// @returns The subset options describing how the dataset was created.
        [[nodiscard]]
        SubsetOptions GetSubsetOptions() const;

      private:
        void CreateControls();
        bool Validate() override;
        void OnDatasetChanged();
        void OnFilterTypeChanged();
        void OnAddFilter();
        void OnRemoveFilter();
        void PopulateColumnChoices();
        void EnableControlsForFilterType();
        void UpdatePreview();
        void ResetPreviewGrid();
        void SetDefaultOutputName();

        /// @returns The currently selected filter type from the radio buttons.
        [[nodiscard]]
        SubsetOptions::FilterType GetSelectedFilterType() const;

        const ReportBuilder* m_reportBuilder{ nullptr };

        wxChoice* m_datasetChoice{ nullptr };
        wxTextCtrl* m_outputNameCtrl{ nullptr };
        wxGrid* m_previewGrid{ nullptr };

        // filter type radio buttons
        wxRadioButton* m_singleRadio{ nullptr };
        wxRadioButton* m_andRadio{ nullptr };
        wxRadioButton* m_orRadio{ nullptr };
        wxRadioButton* m_sectionRadio{ nullptr };

        // filter criteria (Single / AND / OR)
        wxStaticBoxSizer* m_filterBox{ nullptr };
        wxBoxSizer* m_filterRowsSizer{ nullptr };

        /// @brief Controls for a single filter criterion row.
        struct FilterRow
            {
            wxChoice* m_columnChoice{ nullptr };
            wxChoice* m_operatorChoice{ nullptr };
            wxTextCtrl* m_valuesCtrl{ nullptr };
            };

        std::vector<FilterRow> m_filterRows;
        wxButton* m_addFilterBtn{ nullptr };
        wxButton* m_removeFilterBtn{ nullptr };

        // section controls
        wxStaticBoxSizer* m_sectionBox{ nullptr };
        wxStaticText* m_sectionColumnLabel{ nullptr };
        wxChoice* m_sectionColumnChoice{ nullptr };
        wxStaticText* m_startLabelLabel{ nullptr };
        wxTextCtrl* m_startLabelCtrl{ nullptr };
        wxStaticText* m_endLabelLabel{ nullptr };
        wxTextCtrl* m_endLabelCtrl{ nullptr };
        wxCheckBox* m_includeSentinelsCheck{ nullptr };

        std::shared_ptr<Data::Dataset> m_subsettedDataset;
        std::vector<wxString> m_datasetNames;

        enum class Mode
            {
            Insert,
            Edit
            };
        Mode m_mode{ Mode::Insert };
        bool m_suppressPreview{ false };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // SUBSET_DLG_H
