/////////////////////////////////////////////////////////////////////////////
// Name:        printerheaderfooterdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "printerheaderfooterdlg.h"
#include "../../data/dataset.h"
#include "../../util/donttranslate.h"
#include <wx/spinctrl.h>
#include <wx/valgen.h>

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
        {
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
void PrinterHeaderFooterDlg::UCaseEmbeddedTags(wxString& str)
    {
    std::vector<wxString> embeddedTags;

    wxRegEx re(L"(@[[:alpha:]]+@)");
    size_t start{ 0 }, len{ 0 };
    wxString processText = str;
    while (re.Matches(processText))
        {
        // Find the size of the first match
        if (re.GetMatch(&start, &len, 0))
            {
            embeddedTags.push_back(re.GetMatch(processText, 0));

            // Move past the current tag, preparing to find the next one
            processText = processText.substr(start + len);
            }
        else
            {
            break;
            }
        }
    for (const auto& tag : embeddedTags)
        {
        str.Replace(tag, tag.Upper());
        }
    }

//-------------------------------------------------------------
bool PrinterHeaderFooterDlg::Validate()
    {
    wxRegEx re(L"(@[[:alpha:]]+@)");
    const std::set<wxString, Wisteria::Data::wxStringLessNoCase> supportedTags = {
        L"@TITLE@", L"@DATE@", L"@TIME@", L"@PAGENUM@", L"@PAGESCNT@", L"@USER@"
    };

    // make sure the embedded "@@" tags are recognized
    const auto validateString = [&](wxString processText)
    {
        size_t start{ 0 }, len{ 0 };
        while (re.Matches(processText))
            {
            // Find the size of the first match
            if (re.GetMatch(&start, &len, 0))
                {

                if (const wxString embeddedTag = re.GetMatch(processText, 0).MakeUpper();
                    !supportedTags.contains(embeddedTag))
                    {
                    wxMessageBox(wxString::Format(_(L"Invalid tag: %s"), embeddedTag),
                                 _(L"Syntax Error"), wxICON_WARNING);
                    return false;
                    }

                // Move past the current tag, preparing to find the next one
                processText = processText.substr(start + len);
                }
            else
                {
                break;
                }
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
    mainSizer->Add(headerFrameSizer, wxSizerFlags{}.Expand().Border());

    wxBoxSizer* headerLeftSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerLeftSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* leftHeaderTextLabel =
        new wxStaticText(headerFrameSizer->GetStaticBox(), wxID_STATIC, _(L"Left:"),
                         wxDefaultPosition, wxDefaultSize, 0);
    headerLeftSizer->Add(leftHeaderTextLabel, wxSizerFlags{}.Expand().Border(wxBOTTOM));

    wxArrayString choiceStrings;
    choiceStrings.Add(wxString{});
    choiceStrings.Add(DONTTRANSLATE(L"@TITLE@", DTExplanation::InternalKeyword));
    choiceStrings.Add(DONTTRANSLATE(L"@PAGENUM@"));
    choiceStrings.Add(_(L"Page @PAGENUM@ of @PAGESCNT@"));
    choiceStrings.Add(DONTTRANSLATE(L"@DATE@"));
    choiceStrings.Add(DONTTRANSLATE(L"@TIME@"));
    choiceStrings.Add(DONTTRANSLATE(L"@USER@"));
    leftHeaderPrinterCombo =
        new wxComboBox(headerFrameSizer->GetStaticBox(), ControlIDs::ID_LEFT_HEADER_COMBOBOX,
                       wxString{}, wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_leftPrinterHeader));
    headerLeftSizer->Add(leftHeaderPrinterCombo, wxSizerFlags{ 1 }.Expand());

    wxBoxSizer* headerCenterSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerCenterSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* centerHeaderTextLabel =
        new wxStaticText(headerFrameSizer->GetStaticBox(), wxID_STATIC, _(L"Center:"),
                         wxDefaultPosition, wxDefaultSize, 0);
    headerCenterSizer->Add(centerHeaderTextLabel, wxSizerFlags{}.Expand().Border(wxBOTTOM));

    centerHeaderPrinterCombo =
        new wxComboBox(headerFrameSizer->GetStaticBox(), ControlIDs::ID_CENTER_HEADER_COMBOBOX,
                       wxString{}, wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_centerPrinterHeader));
    headerCenterSizer->Add(centerHeaderPrinterCombo, wxSizerFlags{ 1 }.Expand());

    wxBoxSizer* headerRightSizer = new wxBoxSizer(wxVERTICAL);
    headerFrameSizer->Add(headerRightSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* rightHeaderTextLabel =
        new wxStaticText(headerFrameSizer->GetStaticBox(), wxID_STATIC, _(L"Right:"),
                         wxDefaultPosition, wxDefaultSize, 0);
    headerRightSizer->Add(rightHeaderTextLabel, wxSizerFlags{}.Expand().Border(wxBOTTOM));

    rightHeaderPrinterCombo =
        new wxComboBox(headerFrameSizer->GetStaticBox(), ControlIDs::ID_RIGHT_HEADER_COMBOBOX,
                       wxString{}, wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_rightPrinterHeader));
    headerRightSizer->Add(rightHeaderPrinterCombo, wxSizerFlags{ 1 }.Expand());

    wxStaticBox* footersTextBox = new wxStaticBox(this, wxID_ANY, _(L"Footers"));
    wxStaticBoxSizer* footerSizer = new wxStaticBoxSizer(footersTextBox, wxHORIZONTAL);
    mainSizer->Add(footerSizer, wxSizerFlags{}.Expand().Border());

    wxBoxSizer* footerLeftSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerLeftSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* leftFooterPrinterText = new wxStaticText(
        footerSizer->GetStaticBox(), wxID_STATIC, _(L"Left:"), wxDefaultPosition, wxDefaultSize, 0);
    footerLeftSizer->Add(leftFooterPrinterText, wxSizerFlags{}.Expand().Border());

    leftFooterPrinterCombo =
        new wxComboBox(footerSizer->GetStaticBox(), ControlIDs::ID_LEFT_FOOTER_COMBOBOX, wxString{},
                       wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_leftPrinterFooter));
    footerLeftSizer->Add(leftFooterPrinterCombo, wxSizerFlags{ 1 }.Expand());

    wxBoxSizer* footerCenterSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerCenterSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* centerFooterPrinterText =
        new wxStaticText(footerSizer->GetStaticBox(), wxID_STATIC, _(L"Center:"), wxDefaultPosition,
                         wxDefaultSize, 0);
    footerCenterSizer->Add(centerFooterPrinterText, wxSizerFlags{}.Expand().Border(wxBOTTOM));

    centerFooterPrinterCombo =
        new wxComboBox(footerSizer->GetStaticBox(), ControlIDs::ID_CENTER_FOOTER_COMBOBOX,
                       wxString{}, wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_centerPrinterFooter));
    footerCenterSizer->Add(centerFooterPrinterCombo, wxSizerFlags{ 1 }.Expand());

    wxBoxSizer* footerRightSizer = new wxBoxSizer(wxVERTICAL);
    footerSizer->Add(footerRightSizer, wxSizerFlags{ 1 }.Expand().Border(wxLEFT | wxBOTTOM));

    wxStaticText* rightFooterPrinterText =
        new wxStaticText(footerSizer->GetStaticBox(), wxID_STATIC, _(L"Right:"), wxDefaultPosition,
                         wxDefaultSize, 0);
    footerRightSizer->Add(rightFooterPrinterText, wxSizerFlags{}.Expand().Border(wxBOTTOM));

    rightFooterPrinterCombo =
        new wxComboBox(footerSizer->GetStaticBox(), ControlIDs::ID_RIGHT_FOOTER_COMBOBOX,
                       wxString{}, wxDefaultPosition, wxDefaultSize, choiceStrings, wxCB_DROPDOWN,
                       wxGenericValidator(&m_rightPrinterFooter));
    footerRightSizer->Add(rightFooterPrinterCombo, wxSizerFlags{ 1 }.Expand());

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                   wxSizerFlags{}.Expand().Border());

    SetSizerAndFit(mainSizer);
    }
