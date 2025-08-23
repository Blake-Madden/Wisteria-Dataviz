/////////////////////////////////////////////////////////////////////////////
// Name:        searchpanel.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "searchpanel.h"
#include <wx/artprov.h>
#include <wx/bmpbuttn.h>
#include <wx/clipbrd.h>
#include <wx/fdrepdlg.h>
#include <wx/string.h>
#include <wx/xrc/xmlres.h>

using namespace Wisteria::UI;

//---------------------------------------------------
SearchPanel::SearchPanel(wxWindow* parent, wxWindowID id) : wxWindow(parent, id)
    {
    auto* mainSizer = new wxBoxSizer(wxHORIZONTAL);

    m_search = new wxSearchCtrl(this, ControlIDs::ID_SEARCH_TEXT_ENTRY, wxString{},
                                wxDefaultPosition, FromDIP(wxSize{ 200, -1 }), 0);
    mainSizer->Add(m_search, wxSizerFlags{}.Border(wxTOP | wxBOTTOM));

    auto* nextButton =
        new wxBitmapButton(this, ControlIDs::ID_SEARCH_NEXT,
                           wxArtProvider::GetBitmapBundle(wxART_GO_DOWN, wxART_BUTTON));
    nextButton->SetToolTip(_(L"Find the next occurrence"));
    mainSizer->Add(nextButton, wxSizerFlags{}.CentreVertical());

    auto* previousButton =
        new wxBitmapButton(this, ControlIDs::ID_SEARCH_PREVIOUS,
                           wxArtProvider::GetBitmapBundle(wxART_GO_UP, wxART_BUTTON));
    previousButton->SetToolTip(_(L"Find the previous occurrence"));
    mainSizer->Add(previousButton, wxSizerFlags{}.CentreVertical());

    mainSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);

    // search options menu
    auto* searchOptionsMenu = new wxMenu;

    m_matchCaseItem = searchOptionsMenu->AppendCheckItem(XRCID("ID_MATCH_CASE"), _(L"Match Case"),
                                                         _(L"Match Case"));
    m_matchCaseItem->Check(false);

    m_wholeWordItem = searchOptionsMenu->AppendCheckItem(
        XRCID("ID_MATCH_WHOLE_WORD"), _(L"Match Whole Word"), _(L"Match Whole Word"));
    m_wholeWordItem->Check(false);

    m_search->SetMenu(searchOptionsMenu);

    Bind(wxEVT_SEARCH, &SearchPanel::OnSearch, this, ControlIDs::ID_SEARCH_TEXT_ENTRY);
    Bind(wxEVT_BUTTON, &SearchPanel::OnSearchButton, this, ControlIDs::ID_SEARCH_NEXT);
    Bind(wxEVT_BUTTON, &SearchPanel::OnSearchButton, this, ControlIDs::ID_SEARCH_PREVIOUS);
    Bind(wxEVT_CHAR_HOOK,
         [this](wxKeyEvent& event)
         {
             if (event.ControlDown() && event.GetKeyCode() == L'V')
                 {
                 if (wxTheClipboard->Open())
                     {
                     if (wxTheClipboard->IsSupported(wxDF_TEXT))
                         {
                         wxTextDataObject data;
                         wxTheClipboard->GetData(data);
                         m_search->SetValue(data.GetText());
                         }
                     wxTheClipboard->Close();
                     }
                 m_search->SetFocus();
                 }
             else
                 {
                 event.Skip();
                 }
         });
    }

//---------------------------------------------------
bool SearchPanel::SetBackgroundColour(const wxColour& color)
    {
    auto& controls = GetSizer()->GetChildren();
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
void SearchPanel::OnSearchButton(const wxCommandEvent& event)
    {
    wxCommandEvent cmd(wxEVT_NULL, event.GetId());
    OnSearch(cmd);
    }

//---------------------------------------------------
void SearchPanel::OnSearch(const wxCommandEvent& event)
    {
    if (m_search->GetValue().empty())
        {
        return;
        }
    m_previousSearches.push_back(m_search->GetValue());
    m_search->AutoComplete(m_previousSearches);
    wxFindDialogEvent findEvent;
    findEvent.SetFindString(m_search->GetValue());
    // set up the search flags
    wxUint32 searchFlags = event.GetId() == ID_SEARCH_PREVIOUS ? 0 : wxFR_DOWN;
    if (m_wholeWordItem->IsChecked())
        {
        searchFlags |= wxFR_WHOLEWORD;
        }
    if (m_matchCaseItem->IsChecked())
        {
        searchFlags |= wxFR_MATCHCASE;
        }
    findEvent.SetFlags(searchFlags);

    findEvent.SetEventType(wxEVT_COMMAND_FIND);
    GetParent()->ProcessWindowEvent(findEvent);
    }
