/** @addtogroup UI
    @brief User interface classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __GET_DIR_DLG_H__
#define __GET_DIR_DLG_H__

#include <wx/wx.h>
#include <wx/string.h>
#include <wx/statline.h>
#include <wx/valgen.h>
#include <wx/dirdlg.h>
#include <wx/combobox.h>
#include <wx/valtext.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/artprov.h>
#include "dialogwithhelp.h"

/** @brief Prompt for selecting a folder and a file filter for files to select from it.*/
class wxGetDirDlg final : public Wisteria::UI::DialogWithHelp
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
    wxGetDirDlg(wxWindow* parent, const wxString& fullFileFilter, wxWindowID id = wxID_ANY, const wxString& caption = _("Select Directory"),
                      const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                      long style = wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER) :
            m_fullFileFilter(fullFileFilter), m_isRecursive(true), m_selectedFileFilter(0), m_fileFilterCombo(nullptr)
        {
        SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxWS_EX_CONTEXTHELP);
        Wisteria::UI::DialogWithHelp::Create(parent, id, caption, pos, size, style);

        CreateControls();
        Centre();
        }

    /// @returns The path of the selected folder.
    [[nodiscard]] const wxString& GetPath() const noexcept
        { return m_filePath; }
    /** Sets the path of the default selected folder.
        @param path The default folder path to select.*/
    void SetPath(const wxString& path)
        { m_filePath = path; }
    /** Sets the selected file filter.
        @param filter The file filter to display.*/
    void SetSelectedFileFilter(const wxString& filter)
        {
        const int pos = m_fileFilterCombo->FindString(filter);
        if (pos != wxNOT_FOUND)
            {
            m_selectedFileFilter = pos;
            TransferDataToWindow();
            }
        }
    /// @returns The selected document filter.
    [[nodiscard]] wxString GetSelectedFileFilter()
        {
        TransferDataFromWindow();
        return m_fileFilterCombo->GetString(m_selectedFileFilter);
        }
    /// @returns True if "recursive" checkbox is checked.
    [[nodiscard]] bool IsRecursive() const noexcept
        { return m_isRecursive; }
private:
    wxGetDirDlg() noexcept : m_isRecursive(true), m_fileFilterCombo(nullptr), m_selectedFileFilter(0)
      {}
    void CreateControls();
    void OnFolderButtonClick([[maybe_unused]] wxCommandEvent& event);
    void OnOK([[maybe_unused]] wxCommandEvent& event);

    static constexpr int ID_FOLDER_BROWSE_BUTTON = 10001;
    wxString m_filePath;
    wxString m_fullFileFilter;
    int m_selectedFileFilter{ 0 };
    bool m_isRecursive{ true };
    wxChoice* m_fileFilterCombo{ nullptr };

    wxDECLARE_NO_COPY_CLASS(wxGetDirDlg);
    wxDECLARE_EVENT_TABLE();
    };

/** @}*/

#endif //__GET_DIR_DLG_H__
