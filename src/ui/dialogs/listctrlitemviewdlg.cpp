///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlitemviewdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlitemviewdlg.h"

//------------------------------------------------
void ListCtrlItemViewDlg::OnButtonClick(wxCommandEvent& event)
    {
    if (event.GetId() == wxID_CLOSE)
        {
        Close();
        }
    else if (event.GetId() == wxID_COPY)
        {
        if (wxCHECK_VERSION(3, 3, 0))
            {
            m_grid->CopySelection();
            }
        }
    else
        {
        event.Skip();
        }
    }

//------------------------------------------------
void ListCtrlItemViewDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);

    auto columnsNameAttr{ new wxGridCellAttr };
    columnsNameAttr->SetFont(wxFont{}.Bold());
    columnsNameAttr->SetReadOnly(true);

    m_grid = new wxGrid(this, wxID_ANY);
    m_grid->SetTable(new ListRowTable(m_values), true);
    m_grid->SetDefaultCellOverflow(false);
    m_grid->SetLabel(_(L"Item"));
    m_grid->SetColLabelValue(0, _(L"Column Name"));
    m_grid->SetColLabelValue(1, _(L"Value"));
    m_grid->SetColAttr(0, columnsNameAttr);
    m_grid->AutoSizeColumns(false);
    m_grid->SetRowLabelSize(0);

    mainSizer->Add(m_grid, 1, wxALL | wxEXPAND, wxSizerFlags::GetDefaultBorder());

    mainSizer->Add(new wxStaticLine(this),
                   wxSizerFlags().Expand().Border(wxRIGHT|wxLEFT,wxSizerFlags::GetDefaultBorder()));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    if (wxCHECK_VERSION(3, 3, 0))
        {
        wxButton* button = new wxButton(this, wxID_COPY);
        button->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON, FromDIP(wxSize(16, 16))));
        buttonSizer->Add(button, 0, wxRIGHT, wxSizerFlags::GetDefaultBorder());
        }

    {
    wxButton* button = new wxButton(this, wxID_CLOSE);
    button->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_BUTTON, FromDIP(wxSize(16, 16))));
    button->SetDefault();
    buttonSizer->Add(button, 0, wxRIGHT, wxSizerFlags::GetDefaultBorder());
    }

    mainSizer->Add(buttonSizer, 0, wxALIGN_RIGHT|wxALL, wxSizerFlags::GetDefaultBorder());
    }
