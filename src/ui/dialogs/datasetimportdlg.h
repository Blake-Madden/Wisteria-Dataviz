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

        /// @private
        DatasetImportDlg() = delete;
        /// @private
        DatasetImportDlg(const DatasetImportDlg&) = delete;
        /// @private
        DatasetImportDlg& operator=(const DatasetImportDlg&) = delete;

        /// @returns The finalized ImportInfo based on the dialog's settings.
        [[nodiscard]]
        Data::ImportInfo GetImportInfo() const;

        /// @returns The selected worksheet (1-based index).
        [[nodiscard]]
        std::variant<wxString, size_t> GetWorksheet() const;

        /// @returns The file path being imported.
        [[nodiscard]]
        const wxString& GetFilePath() const noexcept
            {
            return m_filePath;
            }

      private:
        void CreateControls();
        void RefreshPreview();
        void ApplyColumnHeaderIcons(DatasetGridTable* table);
        void AdjustGridColumnsForIcons();
        void OnOptionChanged(wxCommandEvent& event);
        void OnSpinChanged(wxSpinEvent& event);

        wxString m_filePath;
        wxString m_fileExt;

        // controls
        wxStaticText* m_worksheetLabel{ nullptr };
        wxChoice* m_worksheetChoice{ nullptr };
        wxSpinCtrl* m_skipRowsSpin{ nullptr };
        wxSpinCtrl* m_maxDiscreteSpin{ nullptr };
        wxCheckBox* m_leadingZerosCheck{ nullptr };
        wxCheckBox* m_yearsAsTextCheck{ nullptr };
        wxGrid* m_previewGrid{ nullptr };

        // preview data
        std::shared_ptr<Data::Dataset> m_previewDataset;

        // worksheet names (for XLSX/ODS)
        std::vector<std::wstring> m_worksheetNames;

        constexpr static size_t PREVIEW_ROW_COUNT = 100;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // DATASET_IMPORT_DIALOG_H
