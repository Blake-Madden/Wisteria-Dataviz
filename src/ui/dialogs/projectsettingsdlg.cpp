///////////////////////////////////////////////////////////////////////////////
// Name:        projectsettingsdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "projectsettingsdlg.h"
#include "../../app/wisteriaapp.h"
#include "printerheaderfooterdlg.h"
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    ProjectSettingsDlg::ProjectSettingsDlg(wxWindow* parent, const wxWindowID id,
                                           const wxString& caption, const wxPoint& pos,
                                           const wxSize& size, const long style)
        : DialogWithHelp(parent, id, caption, pos, size, style)
        {
        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //-------------------------------------------
    void ProjectSettingsDlg::LoadFromProject(const Wisteria::ReportBuilder& reportBuilder)
        {
        m_projectName = reportBuilder.GetName();
        m_subject = reportBuilder.GetSubject();
        m_author = reportBuilder.GetAuthor();
        m_keywords = reportBuilder.GetKeywords();
        m_autoRefresh = reportBuilder.IsAutoRefresh();
        m_watermarkLabel = reportBuilder.GetWatermarkLabel();
        m_leftPrinterHeader = reportBuilder.GetLeftPrinterHeader();
        m_centerPrinterHeader = reportBuilder.GetCenterPrinterHeader();
        m_rightPrinterHeader = reportBuilder.GetRightPrinterHeader();
        m_leftPrinterFooter = reportBuilder.GetLeftPrinterFooter();
        m_centerPrinterFooter = reportBuilder.GetCenterPrinterFooter();
        m_rightPrinterFooter = reportBuilder.GetRightPrinterFooter();
        if (reportBuilder.GetWatermarkColor().IsOk())
            {
            m_watermarkColor = reportBuilder.GetWatermarkColor();
            }
        TransferDataToWindow();
        if (m_watermarkColorPicker != nullptr)
            {
            m_watermarkColorPicker->SetColour(m_watermarkColor);
            }
        }

    //-------------------------------------------
    wxPanel* ProjectSettingsDlg::CreateGeneralPage(wxWindow* parent)
        {
        auto* page = new wxPanel(parent);
        auto* pageSizer = new wxBoxSizer(wxVERTICAL);

        auto* projectBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Project"));
        auto* projectGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        projectGrid->AddGrowableCol(1, 1);

        // project name
        projectGrid->Add(
            new wxStaticText(projectBox->GetStaticBox(), wxID_STATIC, _(L"Project name:")),
            wxSizerFlags{}.CenterVertical());
        projectGrid->Add(new wxTextCtrl(projectBox->GetStaticBox(), wxID_ANY, wxString{},
                                        wxDefaultPosition, FromDIP(wxSize{ 300, -1 }), 0,
                                        wxGenericValidator{ &m_projectName }),
                         wxSizerFlags{}.Expand());

        // subject
        projectGrid->Add(new wxStaticText(projectBox->GetStaticBox(), wxID_STATIC, _(L"Subject:")),
                         wxSizerFlags{}.CenterVertical());
        projectGrid->Add(new wxTextCtrl(projectBox->GetStaticBox(), wxID_ANY, wxString{},
                                        wxDefaultPosition, FromDIP(wxSize{ 300, -1 }), 0,
                                        wxGenericValidator{ &m_subject }),
                         wxSizerFlags{}.Expand());

        // author
        projectGrid->Add(new wxStaticText(projectBox->GetStaticBox(), wxID_STATIC, _(L"Author:")),
                         wxSizerFlags{}.CenterVertical());
        projectGrid->Add(new wxTextCtrl(projectBox->GetStaticBox(), wxID_ANY, wxString{},
                                        wxDefaultPosition, FromDIP(wxSize{ 300, -1 }), 0,
                                        wxGenericValidator{ &m_author }),
                         wxSizerFlags{}.Expand());

        // keywords
        projectGrid->Add(new wxStaticText(projectBox->GetStaticBox(), wxID_STATIC, _(L"Keywords:")),
                         wxSizerFlags{}.CenterVertical());
        projectGrid->Add(new wxTextCtrl(projectBox->GetStaticBox(), wxID_ANY, wxString{},
                                        wxDefaultPosition, FromDIP(wxSize{ 300, -1 }), 0,
                                        wxGenericValidator{ &m_keywords }),
                         wxSizerFlags{}.Expand());

        projectBox->Add(projectGrid, wxSizerFlags{}.Expand().Border());

        auto* autoRefreshCheckBox = new wxCheckBox(
            projectBox->GetStaticBox(), wxID_ANY, _(L"Auto-refresh when data files change"),
            wxDefaultPosition, wxDefaultSize, 0, wxGenericValidator{ &m_autoRefresh });
        projectBox->Add(autoRefreshCheckBox, wxSizerFlags{}.Border(wxLEFT | wxRIGHT | wxBOTTOM));

        pageSizer->Add(projectBox, wxSizerFlags{}.Expand().Border());
        pageSizer->AddStretchSpacer();
        page->SetSizer(pageSizer);
        return page;
        }

    //-------------------------------------------
    wxPanel* ProjectSettingsDlg::CreatePrintingPage(wxWindow* parent)
        {
        auto* page = new wxPanel(parent);
        auto* pageSizer = new wxBoxSizer(wxVERTICAL);

        auto* headerFooterBox = new wxStaticBoxSizer(wxVERTICAL, page, _(L"Headers && Footers"));
        auto* headerFooterButton =
            new wxButton(headerFooterBox->GetStaticBox(), wxID_ANY, _(L"Headers && Footers..."));
        headerFooterButton->Bind(wxEVT_BUTTON, &ProjectSettingsDlg::OnEditHeadersFooters, this);
        headerFooterBox->Add(headerFooterButton, wxSizerFlags{}.Border());

        pageSizer->Add(headerFooterBox, wxSizerFlags{}.Expand().Border());

        // the watermark is drawn when exporting or printing, not on the canvas itself
        auto* watermarkHeaderSizer = new wxBoxSizer(wxHORIZONTAL);
        watermarkHeaderSizer->Add(new wxStaticText(page, wxID_ANY, _(L"Watermark")),
                                  wxSizerFlags{}.CenterVertical());
        watermarkHeaderSizer->Add(new wxStaticLine(page),
                                  wxSizerFlags{ 1 }.CenterVertical().Border(wxLEFT));
        pageSizer->Add(watermarkHeaderSizer,
                       wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxTOP));

        auto* watermarkGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        watermarkGrid->AddGrowableCol(1, 1);

        watermarkGrid->Add(new wxStaticText(page, wxID_STATIC, _(L"Text:")),
                           wxSizerFlags{}.CenterVertical());
        watermarkGrid->Add(new wxTextCtrl(page, wxID_ANY, wxString{}, wxDefaultPosition,
                                          FromDIP(wxSize{ 300, -1 }), 0,
                                          wxGenericValidator{ &m_watermarkLabel }),
                           wxSizerFlags{}.Expand());

        watermarkGrid->Add(new wxStaticText(page, wxID_STATIC, _(L"Color:")),
                           wxSizerFlags{}.CenterVertical());
        m_watermarkColorPicker = new wxColourPickerCtrl(page, wxID_ANY, m_watermarkColor);
        watermarkGrid->Add(m_watermarkColorPicker, wxSizerFlags{}.Expand());

        pageSizer->Add(watermarkGrid, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        pageSizer->AddStretchSpacer();
        page->SetSizer(pageSizer);
        return page;
        }

    //-------------------------------------------
    void ProjectSettingsDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_sideBarBook = new SideBarBook(this, wxID_ANY);
        m_sideBarBook->SetIconSize(wxSize{ 16, 16 });
        m_sideBarBook->GetImageList().push_back(
            wxGetApp().GetResourceManager().GetSVG(L"images/project-settings.svg"));
        m_sideBarBook->GetImageList().push_back(
            wxGetApp().GetResourceManager().GetSVG(L"images/print-setup.svg"));

        m_sideBarBook->AddPage(CreateGeneralPage(m_sideBarBook), _(L"General"), ID_GENERAL_SECTION,
                               true, 0);
        m_sideBarBook->AddPage(CreatePrintingPage(m_sideBarBook), _(L"Export & Printing"),
                               ID_PRINTING_SECTION, false, 1);

        mainSizer->Add(m_sideBarBook, wxSizerFlags{ 1 }.Expand().Border());
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());
        SetSizer(mainSizer);
        }

    //-------------------------------------------
    void ProjectSettingsDlg::OnEditHeadersFooters([[maybe_unused]] wxCommandEvent& event)
        {
        PrinterHeaderFooterDlg dlg(this, m_leftPrinterHeader, m_centerPrinterHeader,
                                   m_rightPrinterHeader, m_leftPrinterFooter, m_centerPrinterFooter,
                                   m_rightPrinterFooter);
        if (dlg.ShowModal() != wxID_OK)
            {
            return;
            }
        m_leftPrinterHeader = dlg.GetLeftPrinterHeader();
        m_centerPrinterHeader = dlg.GetCenterPrinterHeader();
        m_rightPrinterHeader = dlg.GetRightPrinterHeader();
        m_leftPrinterFooter = dlg.GetLeftPrinterFooter();
        m_centerPrinterFooter = dlg.GetCenterPrinterFooter();
        m_rightPrinterFooter = dlg.GetRightPrinterFooter();
        }
    } // namespace Wisteria::UI
