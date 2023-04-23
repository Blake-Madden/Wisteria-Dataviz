///////////////////////////////////////////////////////////////////////////////
// Name:        mainframe.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "getdirdlg.h"

using namespace Wisteria::UI;

//-------------------------------------------------------------
GetDirFilterDialog::GetDirFilterDialog(wxWindow* parent, const wxString& fullFileFilter,
                    wxWindowID id /*= wxID_ANY*/,
                    const wxString& caption /*= _("Select Directory")*/,
                    const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
                    long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER*/) :
                m_fullFileFilter(fullFileFilter)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS|wxWS_EX_CONTEXTHELP);
    DialogWithHelp::Create(parent, id, caption, pos, size, style);

    CreateControls();
    Centre();

    // bind events
    Bind(wxEVT_BUTTON, &GetDirFilterDialog::OnFolderButtonClick, this,
         GetDirFilterDialog::ID_FOLDER_BROWSE_BUTTON);
    Bind(wxEVT_BUTTON, &GetDirFilterDialog::OnOK, this, wxID_OK);
    }

//-------------------------------------------------------------
void GetDirFilterDialog::OnOK([[maybe_unused]] wxCommandEvent& event)
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
void GetDirFilterDialog::OnFolderButtonClick([[maybe_unused]] wxCommandEvent& event)
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
void GetDirFilterDialog::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* fileBrowseBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileBrowseBoxSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxTextCtrl* filePathEdit = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
        wxSize(FromDIP(wxSize(600, 600)).GetWidth(), -1), wxTE_RICH2|wxBORDER_THEME,
        wxGenericValidator(&m_filePath) );
    filePathEdit->AutoCompleteFileNames();
    fileBrowseBoxSizer->Add(filePathEdit, 1, wxEXPAND);

    wxBitmapButton* fileBrowseButton = new wxBitmapButton(this, ID_FOLDER_BROWSE_BUTTON,
        wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON));
    fileBrowseBoxSizer->Add(fileBrowseButton, 0, wxALIGN_CENTER_VERTICAL);

    wxBoxSizer* fileTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileTypeSizer, 0, wxEXPAND);
    fileTypeSizer->Add(new wxStaticText(this, wxID_STATIC, _("File types to include:"),
                                        wxDefaultPosition, wxDefaultSize, 0), 0,
                                        wxALIGN_CENTER_VERTICAL|wxLEFT,
                                        wxSizerFlags::GetDefaultBorder());
    wxArrayString choiceStrings;
    wxStringTokenizer tkz(m_fullFileFilter, L"|", wxTOKEN_STRTOK);
    while (tkz.HasMoreTokens() )
        {
        wxString currentFilter = tkz.GetNextToken();
        if (currentFilter.length() && currentFilter[0] != L'*')
            { choiceStrings.Add(currentFilter); }
        }
    m_fileFilterCombo = new wxChoice(this, wxID_ANY, wxDefaultPosition,
                                     wxSize(FromDIP(wxSize(100, 100)).GetWidth(),-1),
                                     choiceStrings, 0, wxGenericValidator(&m_selectedFileFilter));
    fileTypeSizer->Add(m_fileFilterCombo, 1, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxCheckBox* recurseDirsCheckBox = new wxCheckBox(this, wxID_ANY,
        _("&Search directories recursively"), wxDefaultPosition, wxDefaultSize, 0,
        wxGenericValidator(&m_isRecursive) );
    mainSizer->Add(recurseDirsCheckBox, 0, wxALIGN_LEFT|wxALL, wxSizerFlags::GetDefaultBorder());
    mainSizer->AddStretchSpacer(1);

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL|wxHELP), 0,
        wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);

    filePathEdit->SetFocus();
    }
