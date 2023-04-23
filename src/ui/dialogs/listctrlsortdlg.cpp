///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlsortdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlsortdlg.h"

//------------------------------------------------
void ListCtrlSortDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->SetMinSize(FromDIP(wxSize(500, 300)));

    wxBoxSizer* optionsSizer = new wxBoxSizer(wxHORIZONTAL);
    mainSizer->Add(optionsSizer, 1, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    // construct this first so that we can measure the row height
    m_columnList = new ListCtrlEx(this, wxID_ANY, wxDefaultPosition,
                                  wxDefaultSize,
                                  wxLC_VIRTUAL|wxLC_REPORT|wxLC_ALIGN_LEFT|wxBORDER_THEME);
    m_columnList->SetSortable(false);
    m_columnList->EnableItemAdd(false);
    m_columnList->EnableGridLines();
    m_columnList->EnableItemDeletion();
    m_columnList->EnableItemAdd();
    m_columnList->InsertColumn(0, _("Column"));
    m_columnList->SetColumnEditable(0);
    m_columnList->SetColumnTextSelectionsReadOnly(0, m_columnChoices);
    m_columnList->InsertColumn(1, _("Order"));
    wxArrayString orderOptions;
    orderOptions.Add(GetAscendingLabel());
    orderOptions.Add(GetDescendingLabel());
    m_columnList->SetColumnEditable(1);
    m_columnList->SetColumnTextSelectionsReadOnly(1, orderOptions);
    m_columnList->SetVirtualDataProvider(m_data);
    m_columnList->SetVirtualDataSize(m_columnChoices.Count(), 2);
    m_columnList->DistributeColumns();

    wxBoxSizer* labelsSizer = new wxBoxSizer(wxVERTICAL);
    // align the "sort by" label next to the first row
    wxRect rect;
    m_columnList->GetItemRect(0,rect);
    labelsSizer->AddSpacer(rect.GetHeight() + 3);
    wxStaticText* label = new wxStaticText(this, wxID_STATIC, _("Sort by:"),
                                           wxDefaultPosition, wxDefaultSize, 0);
    labelsSizer->Add(label, 0);
    if (m_columnChoices.size() > 1)
        {
        label = new wxStaticText(this, wxID_STATIC, _("...then by:"),
                                 wxDefaultPosition, wxDefaultSize, 0);
        labelsSizer->Add(label, 0, wxTOP, wxSizerFlags::GetDefaultBorder());
        }
    optionsSizer->Add(labelsSizer, 0, wxALL, wxSizerFlags::GetDefaultBorder());

    optionsSizer->Add(m_columnList, 1, wxEXPAND);

    wxStaticText* infoText = new wxStaticText(this, wxID_ANY,
        _("Double click a field to add or edit a sort criterion."));
    infoText->Wrap(GetSize().GetWidth());
    mainSizer->Add(infoText, 0, wxALL|wxEXPAND, wxSizerFlags::GetDefaultBorder());

    mainSizer->Add(CreateButtonSizer(wxOK|wxCANCEL|wxHELP), 0, wxEXPAND|wxALL,
                                     wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);
    }

//------------------------------------------------
void ListCtrlSortDlg::FillSortCriteria(
    const std::vector<std::pair<size_t,Wisteria::SortDirection>>& sortColumns)
    {
    wxASSERT(sortColumns.size() <= m_columnChoices.size());
    m_columnList->SetVirtualDataSize(m_columnChoices.Count(), 2);
    // if no sort columns, then just use the first column in ascending order, it looks
    // odd not having any sort criteria when this dialog is shown.
    if (sortColumns.size() == 0 && m_columnChoices.size() > 0)
        {
        m_columnList->SetItemText(0,0,m_columnChoices[0]);
        m_columnList->SetItemText(0,1,GetAscendingLabel());
        }
    else
        {
        for (size_t i = 0; i < sortColumns.size(); ++i)
            {
            if (sortColumns[i].first < m_columnChoices.size())
                {
                m_columnList->SetItemText(i,0,m_columnChoices[sortColumns[i].first]);
                m_columnList->SetItemText(i,1,
                    (sortColumns[i].second == Wisteria::SortDirection::SortAscending) ?
                        GetAscendingLabel() : GetDescendingLabel());
                }
            }
        }
    }

//------------------------------------------------
std::vector<std::pair<wxString,Wisteria::SortDirection>> ListCtrlSortDlg::GetColumnsInfo() const
    {
    std::vector<std::pair<wxString,Wisteria::SortDirection>> columns;
    if (m_data)
        {
        for (size_t i = 0; i < m_data->GetItemCount(); ++i)
            {
            const wxString columnName = m_data->GetItemText(i,0);
            if (columnName.IsEmpty())
                { continue; }
            const Wisteria::SortDirection direction =
                (m_data->GetItemText(i,1).CmpNoCase(GetAscendingLabel())==0) ?
                Wisteria::SortDirection::SortAscending :
                Wisteria::SortDirection::SortDescending;
            columns.push_back(std::pair<wxString,Wisteria::SortDirection>(columnName, direction));
            }
        }
    return columns;
    }
