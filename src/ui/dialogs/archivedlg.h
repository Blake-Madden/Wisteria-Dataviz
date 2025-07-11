/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef ARCHIVE_DLG_H
#define ARCHIVE_DLG_H

#include "dialogwithhelp.h"
#include <wx/combobox.h>
#include <wx/filename.h>
#include <wx/statline.h>
#include <wx/string.h>
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /** @brief Prompt for selecting an archive file and a file filter for files to select from it.*/
    class ArchiveDlg final : public DialogWithHelp
        {
      public:
        /** @brief Constructor.
            @param parent The parent window.
            @param fullFileFilter The file filter.
            @param id The window ID.
            @param caption The title of the export dialog.
            @param pos The screen position of the window.
            @param size The window size.
            @param style The window style (i.e., decorations and flags).*/
        ArchiveDlg(wxWindow* parent, const wxString& fullFileFilter, wxWindowID id = wxID_ANY,
                   const wxString& caption = _(L"Select Archive File"),
                   const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                   long style = wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER);
        /// @private
        ArchiveDlg(const ArchiveDlg&) = delete;
        /// @private
        ArchiveDlg& operator=(const ArchiveDlg&) = delete;

        /// @returns The path of the selected archive file.
        [[nodiscard]]
        const wxString& GetPath() const noexcept
            {
            return m_filePath;
            }

        /** @brief Sets the path of the default selected file.
            @param path The default file path to select.*/
        void SetPath(const wxString& path) { m_filePath = path; }

        /** @brief Sets the selected file filter.
            @param filter The file filter to display.*/
        void SetSelectedFileFilter(const wxString& filter)
            {
            const auto pos = m_fileFilterCombo->FindString(filter);
            if (pos != wxNOT_FOUND)
                {
                m_selectedFileFilter = pos;
                TransferDataToWindow();
                }
            }

        /// @returns The selected document filter.
        [[nodiscard]]
        wxString GetSelectedFileFilter()
            {
            TransferDataFromWindow();
            return m_fileFilterCombo->GetString(m_selectedFileFilter);
            }

      private:
        void CreateControls();
        void OnFileButtonClick([[maybe_unused]] wxCommandEvent& event);
        void OnOK([[maybe_unused]] wxCommandEvent& event);

        constexpr static int ID_FILE_BROWSE_BUTTON = wxID_HIGHEST;
        wxString m_filePath;
        wxString m_fullFileFilter;
        int m_selectedFileFilter{ 0 };
        wxChoice* m_fileFilterCombo{ nullptr };
        };
    } // namespace Wisteria::UI

/** @}*/

#endif // ARCHIVE_DLG_H
