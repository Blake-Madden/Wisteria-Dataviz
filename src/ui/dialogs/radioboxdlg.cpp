/////////////////////////////////////////////////////////////////////////////
// Name:        radioboxdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "radioboxdlg.h"
#include "../../base/colorbrewer.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //----------------------------------------------------------
    void RadioBoxDlg::CreateControls(const bool showHelpButton)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto banner = new wxBannerWindow(this, wxTOP);
        banner->SetText(m_bannerLabel, m_bannerDescription);
        banner->SetGradient(banner->GetBackgroundColour(),
                            Colors::ColorContrast::ShadeOrTint(banner->GetBackgroundColour()));

        mainSizer->Add(banner, wxSizerFlags{}.Expand());

        auto* radioBox =
            new wxRadioBox(this, wxID_ANY, m_optionsLabel, wxDefaultPosition, wxDefaultSize,
                           m_choices, 0, wxRA_SPECIFY_ROWS, wxGenericValidator{ &m_selected });

        mainSizer->Add(radioBox, 0, wxALIGN_LEFT | wxALL, wxSizerFlags::GetDefaultBorder());
        mainSizer->AddStretchSpacer();
        if (m_descriptions.GetCount() > 0)
            {
            m_descriptionLabel =
                new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, wxSize{ -1, FromDIP(125) },
                                 wxHW_SCROLLBAR_AUTO | wxBORDER_THEME | wxHW_NO_SELECTION);
            if (m_selected < static_cast<int>(m_descriptions.GetCount()))
                {
                m_descriptionLabel->SetPage(
                    wxString::Format(L"<html><body bgcolor=%s text=%s link=%s>",
                                     wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW)
                                         .GetAsString(wxC2S_HTML_SYNTAX),
                                     wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                                         .GetAsString(wxC2S_HTML_SYNTAX),
                                     wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT)
                                         .GetAsString(wxC2S_HTML_SYNTAX)) +
                    m_descriptions[m_selected] + L"</body></html>");
                }
            mainSizer->Add(m_descriptionLabel,
                           wxSizerFlags{}.Expand().Border().Align(wxALIGN_LEFT));
            }
        mainSizer->AddStretchSpacer(wxSizerFlags::GetDefaultBorder());
        mainSizer->Add(CreateSeparatedButtonSizer(showHelpButton ? (wxOK | wxCANCEL | wxHELP) :
                                                                   (wxOK | wxCANCEL)),
                       wxSizerFlags{}.Expand().Border());

        mainSizer->SetMinSize(FromDIP(wxSize{ 500, 250 }));
        SetSizerAndFit(mainSizer);

        TransferDataToWindow();
        }

    //----------------------------------------------------------
    void RadioBoxDlg::OnRadioBoxChange([[maybe_unused]] wxCommandEvent& event)
        {
        TransferDataFromWindow();
        if (m_descriptionLabel && (m_selected < static_cast<int>(m_descriptions.GetCount())))
            {
            m_descriptionLabel->SetPage(
                wxString::Format(
                    L"<html><body bgcolor=%s text=%s link=%s>",
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetAsString(wxC2S_HTML_SYNTAX),
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                        .GetAsString(wxC2S_HTML_SYNTAX),
                    wxSystemSettings::GetColour(wxSYS_COLOUR_HOTLIGHT)
                        .GetAsString(wxC2S_HTML_SYNTAX)) +
                m_descriptions[m_selected] + L"</body></html>");
            }
        }
    } // namespace Wisteria::UI
