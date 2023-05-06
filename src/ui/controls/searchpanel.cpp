/////////////////////////////////////////////////////////////////////////////
// Name:        searchpanel.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "searchpanel.h"

using namespace Wisteria::UI;

//---------------------------------------------------
SearchPanel::SearchPanel(wxWindow *parent, wxWindowID id) :
      wxWindow(parent, id)
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    m_search = new wxSearchCtrl(this, ControlIDs::ID_SEARCH_TEXT_ENTRY, wxEmptyString,
        wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
    mainSizer->Add(m_search, 0, wxTOP|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxBitmapButton* nextButton = new wxBitmapButton(this, ControlIDs::ID_SEARCH_NEXT,
            wxArtProvider::GetBitmapBundle(wxART_GO_DOWN, wxART_BUTTON));
    nextButton->SetToolTip(_(L"Find the next occurrence"));
    mainSizer->Add(nextButton, 0, wxALIGN_CENTER_VERTICAL);

    wxBitmapButton* peviousButton = new wxBitmapButton(this, ControlIDs::ID_SEARCH_PREVIOUS,
            wxArtProvider::GetBitmapBundle(wxART_GO_UP, wxART_BUTTON));
    peviousButton->SetToolTip(_(L"Find the previous occurrence"));
    mainSizer->Add(peviousButton, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->AddSpacer(3);

    SetSizerAndFit(mainSizer);

    // search options menu
    auto searchOptionsMenu = new wxMenu;

    m_matchCaseItem = searchOptionsMenu->AppendCheckItem(XRCID("ID_MATCH_CASE"),
        _(L"Match Case"), _(L"Match Case"));
    m_matchCaseItem->Check(false);

    m_wholeWordItem = searchOptionsMenu->AppendCheckItem(XRCID("ID_MATCH_WHOLE_WORD"),
        _(L"Match Whole Word"), _(L"Match Whole Word"));
    m_wholeWordItem->Check(false);

    m_search->SetMenu(searchOptionsMenu);

    Bind(wxEVT_TEXT_ENTER, &SearchPanel::OnSearch, this, ControlIDs::ID_SEARCH_TEXT_ENTRY);
    Bind(wxEVT_BUTTON, &SearchPanel::OnSearchButton, this, ControlIDs::ID_SEARCH_NEXT);
    Bind(wxEVT_BUTTON, &SearchPanel::OnSearchButton, this, ControlIDs::ID_SEARCH_PREVIOUS);
    }

//---------------------------------------------------
bool SearchPanel::SetBackgroundColour(const wxColour& color)
    {
    auto controls = GetSizer()->GetChildren();
    for (size_t i = 0; i < controls.size(); ++i)
        {
        if (controls.Item(i)->GetData()->IsWindow() &&
            // leave the search entry like the system;
            // otherwise, you may not be able to read the text
            !controls.Item(i)->GetData()->GetWindow()->IsKindOf(CLASSINFO(wxSearchCtrl)))
            {
            controls.Item(i)->GetData()->GetWindow()->SetBackgroundColour(color);
            }
        }

    return wxWindow::SetBackgroundColour(color);
    }

//---------------------------------------------------
void SearchPanel::Activate()
    {
    m_search->SelectAll();
    m_search->SetFocus();
    }

//---------------------------------------------------
void SearchPanel::OnSearchButton(wxCommandEvent& event)
    {
    wxCommandEvent cmd(wxEVT_NULL, event.GetId());
    OnSearch(cmd);
    }

//---------------------------------------------------
void SearchPanel::OnSearch(wxCommandEvent& event)
    {
    if (m_search->GetValue().empty())
        {
        wxMessageBox(_(L"Please enter an item to search for."),
            _(L"Search"), wxOK|wxICON_INFORMATION, nullptr);
        return;
        }
    m_previousSearches.push_back(m_search->GetValue());
    m_search->AutoComplete(m_previousSearches);
    wxFindDialogEvent findEvent;
    findEvent.SetFindString(m_search->GetValue());
    // set up the search flags
    wxUint32 searchFlags = event.GetId() == ID_SEARCH_PREVIOUS ? 0 : wxFR_DOWN;
    if (m_wholeWordItem->IsChecked())
        { searchFlags |= wxFR_WHOLEWORD; }
    if (m_matchCaseItem->IsChecked())
        { searchFlags |= wxFR_MATCHCASE; }
    findEvent.SetFlags(searchFlags);

    findEvent.SetEventType(wxEVT_COMMAND_FIND);
    GetParent()->ProcessWindowEvent(findEvent);
    }
