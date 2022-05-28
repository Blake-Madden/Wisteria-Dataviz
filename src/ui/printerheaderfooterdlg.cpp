/////////////////////////////////////////////////////////////////////////////
// Name:        printerheaderfooterdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "printerheaderfooterdlg.h"

using namespace Wisteria::UI;

//-------------------------------------------------------------
void PrinterHeaderFooterDlg::OnOK([[maybe_unused]] wxCommandEvent& event)
    {
    TransferDataFromWindow();

    UCaseEmbeddedTags(m_leftPrinterHeader);
    UCaseEmbeddedTags(m_centerPrinterHeader);
    UCaseEmbeddedTags(m_rightPrinterHeader);
    UCaseEmbeddedTags(m_leftPrinterFooter);
    UCaseEmbeddedTags(m_centerPrinterFooter);
    UCaseEmbeddedTags(m_rightPrinterFooter);

    TransferDataToWindow();

    if (!Validate())
        { return; }

    if (IsModal())
        { EndModal(wxID_OK); }
    else
        { Show(false); }
    }

//-------------------------------------------------------------
void PrinterHeaderFooterDlg::UCaseEmbeddedTags(wxString& str)
    {
    std::vector<wxString> embeddedTags;

    wxRegEx re(L"(@[[:alpha:]]+@)");
    size_t start{ 0 }, len{ 0 };
    wxString processText = str;
    while (re.Matches(processText) )
        {
        // Find the size of the first match
        re.GetMatch(&start, &len, 0);
        embeddedTags.emplace_back(re.GetMatch(processText, 0));

        // Process the remainder of the text if there is any.
        processText = processText.Mid(start + len);
        }
    for (const auto& tag : embeddedTags)
        { str.Replace(tag, tag.Upper()); }
    }

//-------------------------------------------------------------
bool PrinterHeaderFooterDlg::Validate()
    {
    wxRegEx re(L"(@[[:alpha:]]+@)");
    const std::set<wxString> supportedTags =
        { L"@TITLE@", L"@DATE@",
          L"@TIME@", L"@PAGENUM@",
          L"@PAGESCNT@" };

    // make sure the embedded "@@" tags are recognized
    const auto validateString = [&](wxString processText)
        {
        size_t start{ 0 }, len{ 0 };
        wxString embeddedTag;
        while (re.Matches(processText) )
            {
            // Find the size of the first match
            re.GetMatch(&start, &len, 0);
            embeddedTag = re.GetMatch(processText, 0).MakeUpper();

            if (supportedTags.find(embeddedTag) == supportedTags.cend())
                {
                wxMessageBox(wxString::Format(_(L"Invalid tag: %s"), embeddedTag),
                             _(L"Syntax Error"), wxICON_WARNING);
                return false;
                }

            // Process the remainder of the text if there is any
            processText = processText.Mid(start + len);
            }
        return true;
        };

    return (validateString(m_leftPrinterHeader) && validateString(m_centerPrinterHeader) &&
            validateString(m_rightPrinterHeader) && validateString(m_leftPrinterFooter) &&
            validateString(m_centerPrinterFooter) && validateString(m_rightPrinterFooter));
    }

//-------------------------------------------------------------
void PrinterHeaderFooterDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBox* headerFrame = new wxStaticBox(this, wxID_ANY, _(L"Headers"));
    wxStaticBoxSizer* headerFrameSizer = new wxStaticBoxSizer(headerFrame, wxHORIZONTAL);
    mainSizer->Add(headerFrameSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxBoxSizer* headerLeftSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerLeftSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* leftHeaderTextLabel = new wxStaticText(this, wxID_STATIC, _(L"Left:"), wxDefaultPosition, wxDefaultSize, 0);
    headerLeftSizer->Add(leftHeaderTextLabel, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxArrayString choiceStrings;
    choiceStrings.Add(L"");
    choiceStrings.Add(DONTTRANSLATE(L"@TITLE@", DTExplanation::InternalKeyword));
    choiceStrings.Add(DONTTRANSLATE(L"@PAGENUM@"));
    choiceStrings.Add(_(L"Page @PAGENUM@ of @PAGESCNT@"));
    choiceStrings.Add(DONTTRANSLATE(L"@DATE@"));
    choiceStrings.Add(DONTTRANSLATE(L"@TIME@"));
    leftHeaderPrinterCombo = new wxComboBox(this, ID_LEFT_HEADER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_leftPrinterHeader));
    headerLeftSizer->Add(leftHeaderPrinterCombo, 1, wxEXPAND);

    wxBoxSizer* headerCenterSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerCenterSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* centerHeaderTextLabel = new wxStaticText(this, wxID_STATIC, _(L"Center:"), wxDefaultPosition, wxDefaultSize, 0);
    headerCenterSizer->Add(centerHeaderTextLabel, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    centerHeaderPrinterCombo = new wxComboBox(this, ID_CENTER_HEADER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_centerPrinterHeader));
    headerCenterSizer->Add(centerHeaderPrinterCombo, 1, wxEXPAND);

    wxBoxSizer* headerRightSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerRightSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* rightHeaderTextLabel = new wxStaticText(this, wxID_STATIC, _(L"Right:"), wxDefaultPosition, wxDefaultSize, 0);
    headerRightSizer->Add(rightHeaderTextLabel, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    rightHeaderPrinterCombo = new wxComboBox(this, ID_RIGHT_HEADER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_rightPrinterHeader));
    headerRightSizer->Add(rightHeaderPrinterCombo, 1, wxEXPAND);

    wxStaticBox* footersTextBox = new wxStaticBox(this, wxID_ANY, _(L"Footers"));
    wxStaticBoxSizer* footerSizer = new wxStaticBoxSizer(footersTextBox, wxHORIZONTAL);
    mainSizer->Add(footerSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    wxBoxSizer* footerLeftSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerLeftSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* leftFooterPrinterText = new wxStaticText(this, wxID_STATIC, _(L"Left:"), wxDefaultPosition, wxDefaultSize, 0);
    footerLeftSizer->Add(leftFooterPrinterText, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    leftFooterPrinterCombo = new wxComboBox(this, ID_LEFT_FOOTER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_leftPrinterFooter));
    footerLeftSizer->Add(leftFooterPrinterCombo, 1, wxEXPAND);

    wxBoxSizer* footerCenterSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerCenterSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* centerFooterPrinterText = new wxStaticText(this, wxID_STATIC, _(L"Center:"), wxDefaultPosition, wxDefaultSize, 0);
    footerCenterSizer->Add(centerFooterPrinterText, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    centerFooterPrinterCombo = new wxComboBox(this, ID_CENTER_FOOTER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_centerPrinterFooter));
    footerCenterSizer->Add(centerFooterPrinterCombo, 1, wxEXPAND);

    wxBoxSizer* footerRightSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerRightSizer, 1, wxEXPAND|wxLEFT|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    wxStaticText* rightFooterPrinterText = new wxStaticText(this, wxID_STATIC, _(L"Right:"), wxDefaultPosition, wxDefaultSize, 0);
    footerRightSizer->Add(rightFooterPrinterText, 0, wxEXPAND|wxBOTTOM, wxSizerFlags::GetDefaultBorder());

    rightFooterPrinterCombo = new wxComboBox(this, ID_RIGHT_FOOTER_COMBOBOX, wxEmptyString, wxDefaultPosition, wxDefaultSize,
        choiceStrings, wxCB_DROPDOWN, wxGenericValidator(&m_rightPrinterFooter));
    footerRightSizer->Add(rightFooterPrinterCombo, 1, wxEXPAND);

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL|wxHELP), 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);
    }
