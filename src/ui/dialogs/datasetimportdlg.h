/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef DATASET_IMPORT_DIALOG_H
#define DATASET_IMPORT_DIALOG_H

#include "../../base/settings.h"
#include "../../data/dataset.h"
#include "../../data/excelreader.h"
#include "../../data/odsreader.h"
#include "../controls/datasetgridtable.h"
#include "dialogwithhelp.h"
#include <memory>
#include <variant>
#include <wx/checkbox.h>
#include <wx/choice.h>
#include <wx/dialog.h>
#include <wx/filename.h>
#include <wx/grid.h>
#include <wx/spinctrl.h>
#include <wx/stattext.h>
#include <wx/textctrl.h>
#include <wx/tokenzr.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @brief Dialog for importing a dataset with a reactive preview grid.
    class DatasetImportDlg final : public Wisteria::UI::DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param filePath The path to the data file to import.
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's style.*/
        DatasetImportDlg(wxWindow* parent, const wxString& filePath, wxWindowID id = wxID_ANY,
                         const wxString& caption = _(L"Import Dataset"),
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);
        /** @brief Constructor that pre-populates the dialog with existing import settings.
            @param parent The parent window.
            @param filePath The path to the data file to import.
            @param importInfo The import settings to pre-populate.
            @param columnInfo The full column preview information (including excluded
                columns and user type overrides).
            @param worksheet The worksheet selection (1-based index or name),
                as returned by GetWorksheet().
            @param id The dialog's ID.
            @param caption The dialog's caption.
            @param pos The dialog's position.
            @param size The dialog's size.
            @param style The dialog's style.*/
        DatasetImportDlg(wxWindow* parent, const wxString& filePath,
                         const Data::ImportInfo& importInfo,
                         const Data::Dataset::ColumnPreviewInfo& columnInfo,
                         const std::variant<wxString, size_t>& worksheet, wxWindowID id = wxID_ANY,
                         const wxString& caption = _(L"Edit Import Settings"),
                         const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                         long style = wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER);

        /// @private
        DatasetImportDlg() = delete;
        /// @private
        DatasetImportDlg(const DatasetImportDlg&) = delete;
        /// @private
        DatasetImportDlg& operator=(const DatasetImportDlg&) = delete;

        /// @returns The finalized ImportInfo based on the dialog's settings.
        [[nodiscard]]
        Data::ImportInfo GetImportInfo();

        /// @returns The column preview information for non-excluded columns only.
        [[nodiscard]]
        Data::Dataset::ColumnPreviewInfo GetColumnPreviewInfo() const;

        /// @returns The full column preview information, including excluded columns
        ///     and any user type overrides.
        [[nodiscard]]
        const Data::Dataset::ColumnPreviewInfo& GetFullColumnPreviewInfo() const noexcept
            {
            return m_columnInfo;
            }

        /// @returns The selected worksheet (1-based index).
        [[nodiscard]]
        std::variant<wxString, size_t> GetWorksheet();

        /// @returns The file path being imported.
        [[nodiscard]]
        const wxString& GetFilePath() const noexcept
            {
            return m_filePath;
            }

      private:
        void CreateControls();
        void RefreshPreview();
        void RefreshPreviewFromColumnInfo();
        void UpdateGrid();
        void ApplyColumnHeaderIcons(DatasetGridTable* table);
        void ApplyExcludedColumnStyling();
        void AdjustGridColumnsForIcons();
        void OnOptionChanged(wxCommandEvent& event);
        void OnSpinChanged(wxSpinEvent& event);
        void OnColumnHeaderClick(wxGridEvent& event);
        void OnColumnSelected(wxGridEvent& event);
        void OnColumnTypeChanged(wxCommandEvent& event);
        void UpdateColumnTypeControls();

        // controls
        wxChoice* m_idColumnChoice{ nullptr };
        wxChoice* m_columnTypeChoice{ nullptr };

        wxGrid* m_previewGrid{ nullptr };

        wxString m_filePath;
        wxString m_fileExt;
        wxString m_mdValues;
        int m_worksheet{ 0 };
        int m_skipRows{ 0 };
        int m_maxDiscrete{ 7 };
        bool m_leadingZeros{ false };
        bool m_yearsAsText{ false };
        bool m_columnNamesSort{ false };

        // preview data
        std::shared_ptr<Data::Dataset> m_previewDataset;

        // column info with user overrides (exclude, type changes, etc.)
        Data::Dataset::ColumnPreviewInfo m_columnInfo;

        // worksheet names (for XLSX/ODS)
        std::vector<std::wstring> m_worksheetNames;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // DATASET_IMPORT_DIALOG_H
