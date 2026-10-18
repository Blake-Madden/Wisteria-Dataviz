///////////////////////////////////////////////////////////////////////////////
// Name:        optionsdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "optionsdlg.h"
#include "../../app/wisteriaapp.h"
#include <algorithm>
#include <iterator>
#include <utility>
#include <wx/paper.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    OptionsDlg::OptionsDlg(wxWindow* parent, const wxWindowID id, const wxString& caption,
                           const wxPoint& pos, const wxSize& size, const long style)
        : DialogWithHelp(parent, id, caption, pos, size, style)
        {
        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //-------------------------------------------
    void OptionsDlg::LoadFromSettings(const AppSettings& settings)
        {
        m_portrait = (settings.GetPrintOrientation() == wxPORTRAIT);
        m_landscape = !m_portrait;
        m_collate = settings.IsPrintCollated();
        m_color = settings.IsPrintColor();
        m_logAutoRefresh = settings.IsLogAutoRefresh();
        m_logVerbose = settings.IsLogVerbose();

        if (m_paperChoice != nullptr)
            {
            if (const auto paperPos =
                    std::find(m_paperIds.cbegin(), m_paperIds.cend(), settings.GetPaperId());
                paperPos != m_paperIds.cend())
                {
                m_paperChoice->SetSelection(
                    static_cast<int>(std::distance(m_paperIds.cbegin(), paperPos)));
                }
            }
        if (m_duplexChoice != nullptr)
            {
            m_duplexChoice->SetSelection(static_cast<int>(settings.GetPrintDuplex()));
            }
        TransferDataToWindow();
        }

    //-------------------------------------------
    void OptionsDlg::ApplyToSettings(AppSettings& settings)
        {
        TransferDataFromWindow();
        settings.SetPrintOrientation(m_portrait ? wxPORTRAIT : wxLANDSCAPE);
        settings.SetPrintCollated(m_collate);
        settings.SetPrintColor(m_color);
        settings.SetLogAutoRefresh(m_logAutoRefresh);
        settings.SetLogVerbose(m_logVerbose);

        if (m_paperChoice != nullptr && m_paperChoice->GetSelection() != wxNOT_FOUND &&
            std::cmp_less(m_paperChoice->GetSelection(), m_paperIds.size()))
            {
            settings.SetPaperId(m_paperIds.at(m_paperChoice->GetSelection()));
            }
        if (m_duplexChoice != nullptr && m_duplexChoice->GetSelection() != wxNOT_FOUND)
            {
            settings.SetPrintDuplex(static_cast<wxDuplexMode>(m_duplexChoice->GetSelection()));
            }
        }

    //-------------------------------------------
    wxPanel* OptionsDlg::CreatePageSetupPage(wxWindow* parent)
        {
        auto* page = new wxPanel(parent);
        auto* pageSizer = new wxBoxSizer(wxVERTICAL);

        // paper
        auto* paperBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Paper"));
        auto* paperGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        paperGrid->AddGrowableCol(1, 1);

        paperGrid->Add(new wxStaticText(paperBox->GetStaticBox(), wxID_STATIC, _(L"Paper size:")),
                       wxSizerFlags{}.CenterVertical());
        m_paperChoice = new wxChoice(paperBox->GetStaticBox(), wxID_ANY);
        if (wxThePrintPaperDatabase != nullptr)
            {
            for (size_t i = 0; i < wxThePrintPaperDatabase->GetCount(); ++i)
                {
                const auto* paperType = wxThePrintPaperDatabase->Item(i);
                m_paperChoice->Append(paperType->GetName());
                m_paperIds.push_back(paperType->GetId());
                }
            }
        paperGrid->Add(m_paperChoice, wxSizerFlags{}.Expand());
        paperBox->Add(paperGrid, wxSizerFlags{}.Expand().Border());

        // orientation
        auto* orientationBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Orientation"));
        auto* portraitRadio =
            new wxRadioButton(orientationBox->GetStaticBox(), wxID_ANY, _(L"Portrait"),
                              wxDefaultPosition, wxDefaultSize, wxRB_GROUP);
        portraitRadio->SetValidator(wxGenericValidator{ &m_portrait });
        auto* landscapeRadio =
            new wxRadioButton(orientationBox->GetStaticBox(), wxID_ANY, _(L"Landscape"));
        landscapeRadio->SetValidator(wxGenericValidator{ &m_landscape });
        orientationBox->Add(portraitRadio, wxSizerFlags{}.Border());
        orientationBox->Add(landscapeRadio, wxSizerFlags{}.Border());

        // printing
        auto* printBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Printing"));
        auto* printGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        printGrid->AddGrowableCol(1, 1);
        printGrid->Add(new wxStaticText(printBox->GetStaticBox(), wxID_STATIC, _(L"Duplex:")),
                       wxSizerFlags{}.CenterVertical());
        m_duplexChoice = new wxChoice(printBox->GetStaticBox(), wxID_ANY);
        m_duplexChoice->Append(_(L"None"));
        m_duplexChoice->Append(_(L"Flip on long edge"));
        m_duplexChoice->Append(_(L"Flip on short edge"));
        printGrid->Add(m_duplexChoice, wxSizerFlags{}.Expand());
        printBox->Add(printGrid, wxSizerFlags{}.Expand().Border());
        printBox->Add(new wxCheckBox(printBox->GetStaticBox(), wxID_ANY, _(L"Collate copies"),
                                     wxDefaultPosition, wxDefaultSize, 0,
                                     wxGenericValidator{ &m_collate }),
                      wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));
        printBox->Add(new wxCheckBox(printBox->GetStaticBox(), wxID_ANY, _(L"Print in color"),
                                     wxDefaultPosition, wxDefaultSize, 0,
                                     wxGenericValidator{ &m_color }),
                      wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        pageSizer->Add(paperBox, wxSizerFlags{}.Expand().Border());
        pageSizer->Add(orientationBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT));
        pageSizer->Add(printBox, wxSizerFlags{}.Expand().Border());
        page->SetSizer(pageSizer);
        return page;
        }

    //-------------------------------------------
    wxPanel* OptionsDlg::CreateGeneralPage(wxWindow* parent)
        {
        auto* page = new wxPanel(parent);
        auto* pageSizer = new wxBoxSizer(wxVERTICAL);

        auto* logBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Log"));
        logBox->Add(new wxCheckBox(logBox->GetStaticBox(), wxID_ANY, _(L"Auto-refresh log"),
                                   wxDefaultPosition, wxDefaultSize, 0,
                                   wxGenericValidator{ &m_logAutoRefresh }),
                    wxSizerFlags{}.Border());
        logBox->Add(new wxCheckBox(logBox->GetStaticBox(), wxID_ANY, _(L"Verbose logging"),
                                   wxDefaultPosition, wxDefaultSize, 0,
                                   wxGenericValidator{ &m_logVerbose }),
                    wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        pageSizer->Add(logBox, wxSizerFlags{}.Expand().Border());
        page->SetSizer(pageSizer);
        return page;
        }

    //-------------------------------------------
    void OptionsDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_sideBarBook = new SideBarBook(this, wxID_ANY);
        m_sideBarBook->SetIconSize(wxSize{ 32, 32 });
        m_sideBarBook->GetImageList().push_back(
            wxGetApp().GetResourceManager().GetSVG(L"images/print-setup.svg"));
        m_sideBarBook->GetImageList().push_back(
            wxGetApp().GetResourceManager().GetSVG(L"images/project-settings.svg"));

        m_sideBarBook->AddPage(CreateGeneralPage(m_sideBarBook), _(L"General"), ID_GENERAL_SECTION,
                               true, 1);
        m_sideBarBook->AddPage(CreatePageSetupPage(m_sideBarBook), _(L"Page Setup"),
                               ID_PAGE_SETUP_SECTION, false, 0);

        mainSizer->Add(m_sideBarBook, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        SetSizer(mainSizer);
        }
    } // namespace Wisteria::UI
