///////////////////////////////////////////////////////////////////////////////
// Name:        projectsettingsdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "projectsettingsdlg.h"
#include "../../app/wisteriaapp.h"
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
        m_keywords = reportBuilder.GetKeywords();
        m_watermarkLabel = reportBuilder.GetWatermarkLabel();
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
    void ProjectSettingsDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        // project name
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Project name:")),
                       wxSizerFlags{}.CenterVertical());
        gridSizer->Add(new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                      FromDIP(wxSize{ 300, -1 }), 0,
                                      wxGenericValidator{ &m_projectName }),
                       wxSizerFlags{}.Expand());

        // subject
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Subject:")),
                       wxSizerFlags{}.CenterVertical());
        gridSizer->Add(new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                      FromDIP(wxSize{ 300, -1 }), 0,
                                      wxGenericValidator{ &m_subject }),
                       wxSizerFlags{}.Expand());

        // keywords
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Keywords:")),
                       wxSizerFlags{}.CenterVertical());
        gridSizer->Add(new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                      FromDIP(wxSize{ 300, -1 }), 0,
                                      wxGenericValidator{ &m_keywords }),
                       wxSizerFlags{}.Expand());

        mainSizer->Add(gridSizer, wxSizerFlags{}.Expand().Border());

        // watermark section
        auto* wmBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Watermark"));
        auto* wmGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        wmGrid->AddGrowableCol(1, 1);

        wmGrid->Add(new wxStaticText(wmBox->GetStaticBox(), wxID_STATIC, _(L"Text:")),
                    wxSizerFlags{}.CenterVertical());
        auto* wmTextCtrl =
            new wxTextCtrl(wmBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_watermarkLabel });
        wmGrid->Add(wmTextCtrl, wxSizerFlags{}.Expand());

        wmGrid->Add(new wxStaticText(wmBox->GetStaticBox(), wxID_STATIC, _(L"Color:")),
                    wxSizerFlags{}.CenterVertical());
        m_watermarkColorPicker =
            new wxColourPickerCtrl(wmBox->GetStaticBox(), wxID_ANY, m_watermarkColor);
        wmGrid->Add(m_watermarkColorPicker, wxSizerFlags{}.Expand());

        wmBox->Add(wmGrid, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(wmBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);
        }
    } // namespace Wisteria::UI
