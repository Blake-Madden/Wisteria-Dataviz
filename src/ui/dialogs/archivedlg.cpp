/////////////////////////////////////////////////////////////////////////////
// Name:        archivedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "archivedlg.h"

using namespace Wisteria::UI;

ArchiveDlg::ArchiveDlg(wxWindow* parent, const wxString& fullFileFilter,
                       wxWindowID id /*= wxID_ANY*/,
                       const wxString& caption /*= _(L"Select Archive File")*/,
                       const wxPoint& pos /*= wxDefaultPosition*/,
                       const wxSize& size /*= wxDefaultSize*/,
                       long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER*/)
    : m_fullFileFilter(fullFileFilter)
    {
    SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS | wxWS_EX_CONTEXTHELP);
    DialogWithHelp::Create(parent, id, caption, pos, size, style);

    CreateControls();
    Centre();

    Bind(wxEVT_BUTTON, &ArchiveDlg::OnOK, this, wxID_OK);
    Bind(wxEVT_BUTTON, &ArchiveDlg::OnFileButtonClick, this, ID_FILE_BROWSE_BUTTON);
    }

//-------------------------------------------------------------
void ArchiveDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();

    if (m_filePath.empty() || !wxFileName::Exists(m_filePath))
        {
        wxMessageBox(_(L"Please select a valid archive file."), _(L"Invalid File"),
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
void ArchiveDlg::OnFileButtonClick([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();
    wxFileDialog dialog(this, _(L"Select Archive File"), wxEmptyString, wxEmptyString,
                        _(L"Archive files (*.zip)|*.zip"),
                        wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_PREVIEW);

    if (dialog.ShowModal() != wxID_OK)
        {
        return;
        }
    m_filePath = dialog.GetPath();
    TransferDataToWindow();
    SetFocus();
    }

//-------------------------------------------------------------
void ArchiveDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxBoxSizer* fileBrowseBoxSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileBrowseBoxSizer, wxSizerFlags{}.Expand().Border());

    wxTextCtrl* filePathEdit =
        new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                       wxTE_RICH2 | wxBORDER_THEME, wxGenericValidator(&m_filePath));
    filePathEdit->AutoCompleteFileNames();
    fileBrowseBoxSizer->Add(filePathEdit, wxSizerFlags{ 1 }.Expand());

    wxBitmapButton* fileBrowseButton = new wxBitmapButton(
        this, ID_FILE_BROWSE_BUTTON, wxArtProvider::GetBitmapBundle(wxART_FILE_OPEN, wxART_BUTTON));
    fileBrowseBoxSizer->Add(fileBrowseButton, wxSizerFlags{}.CenterVertical());

    wxBoxSizer* fileTypeSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(fileTypeSizer, wxSizerFlags{}.Expand());
    fileTypeSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"File types to include:")),
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
    m_fileFilterCombo = new wxChoice(this, wxID_ANY, wxDefaultPosition,
                                     // need to hard code size in case file filter is too wide
                                     wxSize(FromDIP(wxSize{ 150, 150 }).GetWidth(), -1),
                                     choiceStrings, 0, wxGenericValidator(&m_selectedFileFilter));
    fileTypeSizer->Add(m_fileFilterCombo, wxSizerFlags{ 1 }.Expand().Border());

    mainSizer->AddStretchSpacer();

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                   wxSizerFlags{}.Expand().Border());

    SetSizerAndFit(mainSizer);

    filePathEdit->SetFocus();
    }
