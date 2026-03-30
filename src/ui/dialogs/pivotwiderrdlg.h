/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PIVOT_WIDER_DLG_H
#define PIVOT_WIDER_DLG_H

#include "../../base/reportbuilder.h"
#include "../../base/settings.h"
#include "../../data/pivot.h"
#include "../controls/datasetgridtable.h"
#include "variableselectdlg.h"
#include <wx/dialog.h>
#include <wx/grid.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Options describing how a wider-pivoted dataset was created.
        @details Stored alongside the dataset for future serialization.*/
    struct PivotWiderOptions
        {
        /// @brief The name of the source dataset that was pivoted.
        wxString m_sourceDatasetName;
        /// @brief The user-specified name for the output dataset.
        wxString m_outputName;
        /// @brief Columns used as row identifiers in the wide layout.
        std::vector<wxString> m_idColumns;
        /// @brief The categorical column whose values become new column names.
        wxString m_namesFromColumn;
        /// @brief Columns whose values fill the new wide-format cells.
        std::vector<wxString> m_valuesFromColumns;
        /// @brief Separator inserted between combined column name parts.
        wxString m_namesSep{ L"_" };
        /// @brief Prefix prepended to generated column names.
        wxString m_namesPrefix;
        /// @brief Value used to fill cells with no corresponding data.
        double m_fillValue{ std::numeric_limits<double>::quiet_NaN() };
        };

    /** @brief Dialog for pivoting a dataset wider (unstacking).
        @details Provides:
            - A dataset selector (from the project's datasets).
            - ID column(s) selection.
            - A "names from" categorical column selection.
            - Value column(s) selection.
            - Separator, prefix, and fill value options.
            - A live preview grid showing the pivoted result.
            - An output dataset name field.*/
    class PivotWiderDlg final : public wxDialog
        {
      public:
        /** @brief Constructor.
            @param reportBuilder The report builder containing the project's datasets.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        PivotWiderDlg(const ReportBuilder* reportBuilder, wxWindow* parent,
                      wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition,
                      const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);
        /** @brief Constructor that pre-populates the dialog with existing pivot settings.
            @param reportBuilder The report builder containing the project's datasets.
            @param pivotOptions The pivot options to pre-populate.
            @param parent The parent window.
            @param id The window ID.
            @param pos The screen position.
            @param size The window size.
            @param style The window style.*/
        PivotWiderDlg(const ReportBuilder* reportBuilder, const PivotWiderOptions& pivotOptions,
                      wxWindow* parent, wxWindowID id = wxID_ANY,
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);

        /// @private
        PivotWiderDlg(const PivotWiderDlg&) = delete;
        /// @private
        PivotWiderDlg& operator=(const PivotWiderDlg&) = delete;

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
        PivotWiderOptions GetPivotOptions() const;

      private:
        void CreateControls();
        bool Validate() override;
        void OnDatasetChanged();
        void OnSelectColumns();
        void UpdateColumnLabels();
        void UpdatePreview();
        void SetDefaultOutputName();

        [[nodiscard]]
        static wxString NaNLabel()
            {
            return L"NaN";
            }

        Data::Dataset::ColumnPreviewInfo BuildColumnPreviewInfo(const Data::Dataset& dataset) const;

        const ReportBuilder* m_reportBuilder{ nullptr };

        wxChoice* m_datasetChoice{ nullptr };
        wxTextCtrl* m_outputNameCtrl{ nullptr };
        wxGrid* m_previewGrid{ nullptr };

        wxStaticText* m_idColumnsLabel{ nullptr };
        wxStaticText* m_namesFromLabel{ nullptr };
        wxStaticText* m_valuesFromLabel{ nullptr };
        wxTextCtrl* m_namesSepCtrl{ nullptr };
        wxTextCtrl* m_namesPrefixCtrl{ nullptr };
        wxTextCtrl* m_fillValueCtrl{ nullptr };

        std::vector<wxString> m_idColumns;
        wxString m_namesFromColumn;
        std::vector<wxString> m_valuesFromColumns;

        std::shared_ptr<Data::Dataset> m_pivotedDataset;
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

#endif // PIVOT_WIDER_DLG_H
