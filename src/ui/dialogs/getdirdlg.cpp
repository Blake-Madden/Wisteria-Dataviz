///////////////////////////////////////////////////////////////////////////////
// Name:        getdirdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "getdirdlg.h"
#include <wx/artprov.h>
#include <wx/dirdlg.h>
#include <wx/tokenzr.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------------------------
    GetDirFilterDialog::GetDirFilterDialog(
        wxWindow* parent, const wxString& fullFileFilter, wxWindowID id /*= wxID_ANY*/,
        const wxString& caption /*= _(L"Select Directory")*/,
        const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
        long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER*/)
        : m_fullFileFilter(fullFileFilter)
        {
        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS |
                                        wxWS_EX_CONTEXTHELP);
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

        if (m_filePath.empty() || !wxFileName::DirExists(m_filePath))
            {
            wxMessageBox(_(L"Please select a valid folder."), _(L"Invalid Folder"),
                         wxICON_EXCLAMATION | wxOK, this);
            return;
            }

        if (IsModal())
            {
            EndModal(wxID_OK);
            }
        else
            {
            Show(false);
            }
        }

    //-------------------------------------------------------------
    void GetDirFilterDialog::OnFolderButtonClick([[maybe_unused]] wxCommandEvent& event)
        {
        TransferDataFromWindow();
        wxDirDialog dirDlg(this);
        dirDlg.SetPath(m_filePath);
        if (dirDlg.ShowModal() != wxID_OK)
            {
            return;
            }
        m_filePath = dirDlg.GetPath();
        TransferDataToWindow();
        SetFocus();
        }

    //-------------------------------------------------------------
    void GetDirFilterDialog::CreateControls()
        {
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        wxBoxSizer* fileBrowseBoxSizer = new wxBoxSizer(wxHORIZONTAL);
        mainSizer->Add(fileBrowseBoxSizer, wxSizerFlags{}.Expand().Border());

        wxTextCtrl* filePathEdit =
            new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                           wxSize(FromDIP(wxSize{ 600, 600 }).GetWidth(), -1),
                           wxTE_RICH2 | wxBORDER_THEME, wxGenericValidator(&m_filePath));
        filePathEdit->AutoCompleteFileNames();
        fileBrowseBoxSizer->Add(filePathEdit, wxSizerFlags{ 1 }.Expand());

        wxBitmapButton* fileBrowseButton =
            new wxBitmapButton(this, ID_FOLDER_BROWSE_BUTTON,
                               wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON));
        fileBrowseBoxSizer->Add(fileBrowseButton, wxSizerFlags{}.CenterVertical());

        wxBoxSizer* fileTypeSizer = new wxBoxSizer(wxHORIZONTAL);
        mainSizer->Add(fileTypeSizer, wxSizerFlags{}.Expand());
        fileTypeSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"File types to include:"),
                                            wxDefaultPosition, wxDefaultSize, 0),
                           wxSizerFlags{}.CenterVertical().Border(wxLEFT));
        wxArrayString choiceStrings;
        wxStringTokenizer tkz(m_fullFileFilter, L"|", wxTOKEN_STRTOK);
        while (tkz.HasMoreTokens())
            {
            wxString currentFilter = tkz.GetNextToken();
            if (currentFilter.length() && currentFilter[0] != L'*')
                {
                choiceStrings.Add(currentFilter);
                }
            }
        m_fileFilterCombo = new wxChoice(
            this, wxID_ANY, wxDefaultPosition, wxSize(FromDIP(wxSize{ 100, 100 }).GetWidth(), -1),
            choiceStrings, 0, wxGenericValidator(&m_selectedFileFilter));
        fileTypeSizer->Add(m_fileFilterCombo, wxSizerFlags{ 1 }.Expand().Border());

        wxCheckBox* recurseDirsCheckBox =
            new wxCheckBox(this, wxID_ANY, _(L"&Search directories recursively"), wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator(&m_isRecursive));
        mainSizer->Add(recurseDirsCheckBox, 0, wxALIGN_LEFT | wxALL,
                       wxSizerFlags::GetDefaultBorder());
        mainSizer->AddStretchSpacer(1);

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        filePathEdit->SetFocus();
        }
    } // namespace Wisteria::UI
