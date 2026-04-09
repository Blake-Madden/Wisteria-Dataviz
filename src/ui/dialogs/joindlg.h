/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef JOIN_DLG_H
#define JOIN_DLG_H

#include "../../base/reportbuilder.h"
#include "../../base/settings.h"
#include "../../data/join_inner.h"
#include "../../data/join_left.h"
#include "../controls/datasetgridtable.h"
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Options describing how a joined dataset was created.
        @details Stored alongside the dataset for future serialization.*/
    struct JoinOptions
        {
        /// @brief The available join types.
        enum class JoinType
            {
            LeftJoinUniqueLast,
            LeftJoinUniqueFirst,
            LeftJoin,
            InnerJoin
            };

        /// @brief The name of the left (primary) dataset.
        wxString m_sourceDatasetName;
        /// @brief The name of the right (secondary) dataset.
        wxString m_otherDatasetName;
        /// @brief The user-specified name for the output dataset.
        wxString m_outputName;
        /// @brief The join type.
        JoinType m_type{ JoinType::LeftJoinUniqueLast };
        /// @brief Left-column / right-column pairs used for matching rows.
        std::vector<std::pair<wxString, wxString>> m_byColumns;
        /// @brief Suffix appended to disambiguate duplicate column names.
        wxString m_suffix{ L".x" };
        };

    /** @brief Dialog for joining two datasets.
        @details Provides:
            - Left and right dataset selectors (from the project's datasets).
            - A join type selector (left join unique last/first, left join, inner join).
            - Dynamic by-column pair rows for specifying join keys.
            - A suffix option for duplicate column names.
            - A live preview grid showing the joined result.
            - An output dataset name field.*/
    class JoinDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        JoinDlg(const ReportBuilder* reportBuilder, wxWindow* parent, wxWindowID id = wxID_ANY,
                const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);
        /** @brief Constructor that pre-populates the dialog with existing join settings.
            @param reportBuilder The report builder containing the project's datasets.
            @param joinOptions The join options to pre-populate.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        JoinDlg(const ReportBuilder* reportBuilder, const JoinOptions& joinOptions,
                wxWindow* parent, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                const wxSize& size = wxDefaultSize,
                long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        JoinDlg(const JoinDlg&) = delete;
        /// @private
        JoinDlg& operator=(const JoinDlg&) = delete;

        /// @returns The joined result dataset, or @c nullptr if not yet generated.
        [[nodiscard]]
        const std::shared_ptr<Data::Dataset>& GetJoinedDataset() const noexcept
            {
            return m_joinedDataset;
            }

        /// @returns The user-specified output dataset name.
        [[nodiscard]]
        wxString GetOutputName() const;

        /// @returns The join options describing how the dataset was created.
        [[nodiscard]]
        JoinOptions GetJoinOptions() const;

      private:
        void CreateControls();
        bool Validate() override;
        void OnLeftDatasetChanged();
        void OnRightDatasetChanged();
        void OnAddByColumn();
        void OnRemoveByColumn();
        void PopulateColumnChoices();
        void UpdatePreview();
        void ResetPreviewGrid();
        void SetDefaultOutputName();

        /// @returns The selected left dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedLeftDataset() const;

        /// @returns The name of the selected left dataset.
        [[nodiscard]]
        wxString GetSelectedLeftDatasetName() const;

        /// @returns The selected right dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedRightDataset() const;

        /// @returns The name of the selected right dataset.
        [[nodiscard]]
        wxString GetSelectedRightDatasetName() const;

        const ReportBuilder* m_reportBuilder{ nullptr };

        wxChoice* m_leftDatasetChoice{ nullptr };
        wxChoice* m_rightDatasetChoice{ nullptr };
        wxChoice* m_joinTypeChoice{ nullptr };
        wxTextCtrl* m_suffixCtrl{ nullptr };
        wxTextCtrl* m_outputNameCtrl{ nullptr };
        wxGrid* m_previewGrid{ nullptr };

        /// @brief Controls for a single by-column pair row.
        struct ByColumnRow
            {
            wxChoice* m_leftColumnChoice{ nullptr };
            wxChoice* m_rightColumnChoice{ nullptr };
            };

        wxStaticBoxSizer* m_byColumnsBox{ nullptr };
        wxBoxSizer* m_byColumnRowsSizer{ nullptr };
        std::vector<ByColumnRow> m_byColumnRows;
        wxButton* m_addByColumnBtn{ nullptr };
        wxButton* m_removeByColumnBtn{ nullptr };

        std::shared_ptr<Data::Dataset> m_joinedDataset;
        std::vector<wxString> m_datasetNames;

        enum class Mode
            {
            Insert,
            Edit
            };
        Mode m_mode{ Mode::Insert };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // JOIN_DLG_H
