///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlitemviewdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlitemviewdlg.h"
#include <wx/utils.h>
#include "../../import/html_encode.h"

using namespace Wisteria::UI;

//------------------------------------------------
void ListCtrlItemViewDlg::OnButtonClick(wxCommandEvent& event)
    {
    if (event.GetId() == wxID_CLOSE)
        { Close(); }
    else if (event.GetId() == wxID_PRINT)
        { m_htmlWindow->OnPrint(event); }
    else if (event.GetId() == wxID_SAVE)
        { m_htmlWindow->OnSave(event); }
    else if (event.GetId() == wxID_COPY)
        {
        if (m_htmlWindow->SelectionToText().length())
            { m_htmlWindow->OnCopy(event); }
        else
            { m_htmlWindow->OnCopyAll(event); }
        }
    else
        { event.Skip(); }
    }

//------------------------------------------------
void ListCtrlItemViewDlg::OnHyperlinkClicked(wxHtmlLinkEvent& event)
    { wxLaunchDefaultApplication(event.GetLinkInfo().GetHref()); }

//------------------------------------------------
void ListCtrlItemViewDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    SetSizer(mainSizer);

    std::wstring htmlText =
        L"<body style=\"background:#E9F2FA\"><table border=\"1\" width=\"100%\">";
    for (const auto& value : m_values)
        {
        htmlText += L"<tr>";
        if (value.m_column.length() > 0)
            { htmlText += L"<td>" + value.m_column + L"</td>"; }

        lily_of_the_valley::html_encode_text encode;
        if (value.m_isUrl)
            {
            htmlText += wxString::Format(L"<td><a href=\"%s\">", value.m_value) +
                encode({ value.m_value.wc_str(), value.m_value.length() }, true).c_str() +
                wxString(L"</a></td></tr>");
            }
        else
            {
            htmlText += wxString(L"<td>") +
                encode({ value.m_value.wc_str(), value.m_value.length() }, true).c_str() +
                wxString(L"</td></tr>");
            }
        }
    htmlText += L"</table></body>";

    m_htmlWindow = new HtmlTableWindow(this, wxID_ANY, wxDefaultPosition,
                                       wxDefaultSize, wxHW_SCROLLBAR_NEVER);
    m_htmlWindow->SetLabel(_(L"Item"));
    lily_of_the_valley::html_format::set_title(htmlText, m_htmlWindow->GetLabel().ToStdWstring());
    lily_of_the_valley::html_format::set_encoding(htmlText);
    m_htmlWindow->SetPage(htmlText);
    m_htmlWindow->GetInternalRepresentation()->SetIndent(0, wxHTML_INDENT_ALL);
    m_htmlWindow->GetInternalRepresentation()->Layout(FromDIP(wxSize(700,700)).GetWidth());

    const int screenWidth = wxSystemSettings::GetMetric(wxSYS_SCREEN_X);
    const int screenHeight = wxSystemSettings::GetMetric(wxSYS_SCREEN_Y);
    if ((screenWidth * .8 < m_htmlWindow->GetInternalRepresentation()->GetWidth()) ||
        (screenHeight * .8 < m_htmlWindow->GetInternalRepresentation()->GetHeight()))
        {
        // if the table is too big for the screen then set its min size
        // to the screen and add scrollbars
        m_htmlWindow->SetMinSize(
            wxSize(wxMin(m_htmlWindow->GetInternalRepresentation()->GetWidth(), screenWidth * .8),
                   wxMin(m_htmlWindow->GetInternalRepresentation()->GetHeight(), screenHeight * .8)));
        m_htmlWindow->SetWindowStyle(wxHW_SCROLLBAR_AUTO);
        }
    else
        {
        // otherwise, fit the table to fit the window
        m_htmlWindow->SetMinSize(
            wxSize(m_htmlWindow->GetInternalRepresentation()->GetWidth(),
                   m_htmlWindow->GetInternalRepresentation()->GetHeight()));
        }
    mainSizer->Add(m_htmlWindow, 1, wxALL|wxEXPAND, wxSizerFlags::GetDefaultBorder());

    mainSizer->Add(new wxStaticLine(this),
                   wxSizerFlags().Expand().Border(wxRIGHT|wxLEFT,wxSizerFlags::GetDefaultBorder()));
    wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);

    {
    wxButton* button = new wxButton(this, wxID_PRINT);
    button->SetBitmap(wxArtProvider::GetBitmap(wxART_PRINT, wxART_BUTTON, FromDIP(wxSize(16, 16))));
    buttonSizer->Add(button, 0, wxRIGHT, wxSizerFlags::GetDefaultBorder());
    }

    {
    wxButton* button = new wxButton(this, wxID_COPY);
    button->SetBitmap(wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON, FromDIP(wxSize(16, 16))));
    buttonSizer->Add(button, 0, wxRIGHT, wxSizerFlags::GetDefaultBorder());
    }

    {
    wxButton* button = new wxButton(this, wxID_SAVE);
    button->SetBitmap(wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON, FromDIP(wxSize(16, 16))));
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
