//////////////////////////////////////////////////////////////////////////////
// Name:        htmltablepanel.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmltablepanel.h"
#include "../../util/parentblocker.h"

using namespace Wisteria::UI;

//-------------------------------------------------------
HtmlTablePanel::HtmlTablePanel(wxWindow *parent, wxWindowID id, const wxColour bkColor) :
      wxWindow(parent, id)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    SetBackgroundColour(bkColor);
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* toolbarSizer = new wxBoxSizer(wxHORIZONTAL);

    wxBitmapButton* copyButton = new wxBitmapButton(this, wxID_COPY,
            wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON, FromDIP(wxSize(16,16))));
    copyButton->SetToolTip(_(L"Copy selected text"));
    toolbarSizer->Add(copyButton, 0, wxALIGN_CENTER_VERTICAL);

    wxBitmapButton* saveButton = new wxBitmapButton(this, wxID_SAVE,
            wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON, FromDIP(wxSize(16,16))));
    saveButton->SetToolTip(_(L"Save"));
    toolbarSizer->Add(saveButton, 0, wxALIGN_CENTER_VERTICAL);

    mainSizer->Add(toolbarSizer);
    m_htmlWindow = new HtmlTableWindow(this, wxID_ANY,
                           wxDefaultPosition, wxSize(FromDIP(wxSize(300,300)).GetWidth(),-1),
                           wxHW_DEFAULT_STYLE|wxBORDER_THEME);
    mainSizer->Add(m_htmlWindow, 1);

    SetSizerAndFit(mainSizer);

    Bind(wxEVT_BUTTON, &HtmlTablePanel::OnButtonClick, this, wxID_COPY);
    Bind(wxEVT_BUTTON, &HtmlTablePanel::OnButtonClick, this, wxID_SAVE);
    }

//-------------------------------------------------------
void HtmlTablePanel::OnButtonClick(wxCommandEvent& event)
    {
    ParentEventBlocker blocker(GetHtmlWindow());
    GetHtmlWindow()->ProcessWindowEvent(event);
    }
