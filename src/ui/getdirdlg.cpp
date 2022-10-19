///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////


#include "getdirdlg.h"

wxBEGIN_EVENT_TABLE(wxGetDirDlg, DialogWithHelp)
    EVT_BUTTON(wxGetDirDlg::ID_FOLDER_BROWSE_BUTTON, wxGetDirDlg::OnFolderButtonClick)
    EVT_BUTTON(wxID_OK, wxGetDirDlg::OnOK)
wxEND_EVENT_TABLE()

//-------------------------------------------------------------
void wxGetDirDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();

    if (m_filePath.IsEmpty() || !wxFileName::DirExists(m_filePath))
        {
        wxMessageBox(_("Please select a valid folder."), _("Invalid Folder"),
                     wxICON_EXCLAMATION|wxOK, this);
        return;
        }

    if (IsModal())
        { EndModal(wxID_OK); }
    else
        { Show(false); }
    }

//-------------------------------------------------------------
void wxGetDirDlg::OnFolderButtonClick([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();
    wxDirDialog dirDlg(this);
    dirDlg.SetPath(m_filePath);
    if (dirDlg.ShowModal() != wxID_OK)
        { return; }
    m_filePath = dirDlg.GetPath();
    TransferDataToWindow();
    SetFocus();
    }

//-------------------------------------------------------------
void wxGetDirDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* fileBrowseBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileBrowseBoxSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxTextCtrl* filePathEdit = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxSize(FromDIP(wxSize(600,600)).GetWidth(),-1), wxTE_RICH2|wxBORDER_THEME, wxGenericValidator(&m_filePath) );
    filePathEdit->AutoCompleteFileNames();
    fileBrowseBoxSizer->Add(filePathEdit, 1, wxEXPAND);

    wxBitmapButton* fileBrowseButton = new wxBitmapButton(this, ID_FOLDER_BROWSE_BUTTON, wxArtProvider::GetBitmap(wxART_FILE_OPEN, wxART_BUTTON, FromDIP(wxSize(16,16))));
    fileBrowseBoxSizer->Add(fileBrowseButton, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* fileTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileTypeSizer, 0, wxEXPAND);
    fileTypeSizer->Add(new wxStaticText(this, wxID_STATIC, _("File types to include:"), wxDefaultPosition, wxDefaultSize, 0), 0, wxALIGN_CENTER_VERTICAL|wxLEFT, wxSizerFlags::GetDefaultBorder());
    wxArrayString choiceStrings;
    wxStringTokenizer tkz(m_fullFileFilter, wxT("|"), wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens() )
        {
        wxString currentFilter = tkz.GetNextToken();
        if (currentFilter.length() && currentFilter[0] != wxT('*'))
            { choiceStrings.Add(currentFilter); }
        }
    m_fileFilterCombo = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(wxSize(100,100)).GetWidth(),-1), choiceStrings,
        0, wxGenericValidator(&m_selectedFileFilter));
    fileTypeSizer->Add(m_fileFilterCombo, 1, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxCheckBox* recurseDirsCheckBox = new wxCheckBox(this, wxID_ANY, _("&Search directories recursively"), wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator(&m_isRecursive) );
    mainSizer->Add(recurseDirsCheckBox, 0, wxALIGN_LEFT|wxALL, wxSizerFlags::GetDefaultBorder());
    mainSizer->AddStretchSpacer(1);

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL|wxHELP), 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);

    filePathEdit->SetFocus();
    }
