/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PIVOT_LONGER_DLG_H
#define PIVOT_LONGER_DLG_H

#include "../../base/reportbuilder.h"
#include "../../data/pivot.h"
#include "../controls/datasetgridtable.h"
#include "variableselectdlg.h"
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Options describing how a longer-pivoted dataset was created.
        @details Stored alongside the dataset for future serialization.*/
    struct PivotLongerOptions
        {
        wxString m_sourceDatasetName;
        wxString m_outputName;
        std::vector<wxString> m_columnsToKeep;
        std::vector<wxString> m_fromColumns;
        std::vector<wxString> m_namesTo;
        wxString m_valuesTo;
        wxString m_namesPattern;
        };

    /** @brief Dialog for pivoting a dataset longer (stacking).
        @details Provides:
            - A dataset selector (from the project's datasets).
            - Columns-to-keep selection (ID and grouping columns).
            - From-columns selection (continuous columns to stack).
            - A "names to" field for the target grouping column name(s).
            - A "values to" field for the target value column name.
            - An optional names pattern (regex) for splitting column names.
            - A live preview grid showing the pivoted result.
            - An output dataset name field.*/
    class PivotLongerDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        PivotLongerDlg(const ReportBuilder* reportBuilder, wxWindow* parent,
                       wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize,
                       long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        PivotLongerDlg(const PivotLongerDlg&) = delete;
        /// @private
        PivotLongerDlg& operator=(const PivotLongerDlg&) = delete;

        /// @returns The selected source dataset, or @c nullptr if none.
        [[nodiscard]]
        std::shared_ptr<Data::Dataset> GetSelectedDataset() const;

        /// @returns The name of the selected source dataset.
        [[nodiscard]]
        wxString GetSelectedDatasetName() const;

        /// @returns The pivoted result dataset, or @c nullptr if not yet generated.
        [[nodiscard]]
        const std::shared_ptr<Data::Dataset>& GetPivotedDataset() const noexcept
            {
            return m_pivotedDataset;
            }

        /// @returns The user-specified output dataset name.
        [[nodiscard]]
        wxString GetOutputName() const;

        /// @returns The pivot options describing how the dataset was created.
        [[nodiscard]]
        PivotLongerOptions GetPivotOptions() const;

      private:
        void CreateControls();
        bool Validate() override;
        void OnDatasetChanged();
        void OnSelectColumns();
        void UpdateColumnLabels();
        void UpdatePreview();
        void SetDefaultOutputName();
        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        /// @returns The default name for the grouping column created by pivot longer.
        [[nodiscard]]
        static wxString DefaultNamesTo()
            {
            // TRANSLATORS: default name for the grouping column created by pivot longer.
            return _(L"variable");
            }

        /// @returns The default name for the value column created by pivot longer.
        [[nodiscard]]
        static wxString DefaultValuesTo()
            {
            // TRANSLATORS: default name for the value column created by pivot longer.
            return _(L"value");
            }

        const ReportBuilder* m_reportBuilder{ nullptr };

        wxChoice* m_datasetChoice{ nullptr };
        wxTextCtrl* m_outputNameCtrl{ nullptr };
        wxGrid* m_previewGrid{ nullptr };

        wxStaticText* m_keepColumnsLabel{ nullptr };
        wxStaticText* m_fromColumnsLabel{ nullptr };
        wxTextCtrl* m_namesToCtrl{ nullptr };
        wxTextCtrl* m_valuesToCtrl{ nullptr };
        wxTextCtrl* m_namesPatternCtrl{ nullptr };

        std::vector<wxString> m_columnsToKeep;
        std::vector<wxString> m_fromColumns;

        std::shared_ptr<Data::Dataset> m_pivotedDataset;
        std::vector<wxString> m_datasetNames;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // PIVOT_LONGER_DLG_H
