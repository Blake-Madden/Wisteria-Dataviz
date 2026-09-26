///////////////////////////////////////////////////////////////////////////////
// Name:        htmldashboarddlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmldashboarddlg.h"
#include <string>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    HtmlDashboardDlg::HtmlDashboardDlg(wxWindow* parent, const wxArrayString& themes,
                                       const Wisteria::HtmlDashboardOptions& options,
                                       const wxString& theme, wxWindowID id /*= wxID_ANY*/,
                                       const wxString& caption /*= _(L"HTML Export Options")*/,
                                       const wxPoint& pos /*= wxDefaultPosition*/,
                                       const wxSize& size /*= wxDefaultSize*/,
                                       long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN*/)
        : DialogWithHelp(parent, id, caption, pos, size, style), m_dashboardTitle(options.m_title),
          m_theme(theme), m_includeColorModeToggle(options.m_includeColorModeToggle),
          m_countUpNumbers(options.m_countUpNumbers), m_pageWidth(options.m_pageSize.GetWidth()),
          m_pageHeight(options.m_pageSize.GetHeight())
        {
        using View = Wisteria::HtmlDashboardOptions::View;
        using ColorMode = Wisteria::HtmlDashboardOptions::ColorMode;

        m_view = (options.m_view == View::Atlas) ? 0 : 1;
        m_colorMode = (options.m_colorMode == ColorMode::Auto)  ? 0 :
                      (options.m_colorMode == ColorMode::Light) ? 1 :
                                                                  2;

        SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_BLOCK_EVENTS);

        CreateControls(themes);

        Bind(wxEVT_BUTTON, &HtmlDashboardDlg::OnOK, this, wxID_OK);

        Centre();
        }

    //------------------------------------------------------
    void HtmlDashboardDlg::CreateControls(const wxArrayString& themes)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        gridSizer->AddGrowableCol(1, 1);

        // title
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Title:")),
                       wxSizerFlags{}.CenterVertical());
        auto* titleCtrl = new wxTextCtrl(this, wxID_ANY, wxString{}, wxDefaultPosition,
                                         FromDIP(wxSize{ 300, -1 }));
        titleCtrl->SetValidator(wxGenericValidator{ &m_dashboardTitle });
        gridSizer->Add(titleCtrl, wxSizerFlags{}.Expand());

        // theme
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Theme:")),
                       wxSizerFlags{}.CenterVertical());
        auto* themeChoice = new wxChoice(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, themes);
        themeChoice->SetValidator(wxGenericValidator{ &m_theme });
        gridSizer->Add(themeChoice, wxSizerFlags{}.Expand());

        // page size
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Page width:")),
                       wxSizerFlags{}.CenterVertical());
        auto* widthCtrl =
            new wxSpinCtrl(this, wxID_ANY, std::to_wstring(m_pageWidth), wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        widthCtrl->SetValidator(wxGenericValidator{ &m_pageWidth });
        gridSizer->Add(widthCtrl, wxSizerFlags{}.Expand());

        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Page height:")),
                       wxSizerFlags{}.CenterVertical());
        auto* heightCtrl =
            new wxSpinCtrl(this, wxID_ANY, std::to_wstring(m_pageHeight), wxDefaultPosition,
                           wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        heightCtrl->SetValidator(wxGenericValidator{ &m_pageHeight });
        gridSizer->Add(heightCtrl, wxSizerFlags{}.Expand());

        mainSizer->Add(gridSizer, wxSizerFlags{}.Expand().Border());

        // initial view
        wxArrayString viewChoices;
        viewChoices.Add(_(L"Atlas"));
        viewChoices.Add(_(L"Story"));
        auto* viewRadio = new wxRadioBox(this, wxID_ANY, _(L"Initial View"), wxDefaultPosition,
                                         wxDefaultSize, viewChoices, 1, wxRA_SPECIFY_ROWS);
        viewRadio->SetValidator(wxGenericValidator{ &m_view });
        mainSizer->Add(viewRadio, wxSizerFlags{}.Expand().Border());

        // color mode toggle, which enables the initial color mode options
        auto* toggleCheck = new wxCheckBox(this, wxID_ANY, _(L"Include color mode toggle"));
        toggleCheck->SetValidator(wxGenericValidator{ &m_includeColorModeToggle });
        mainSizer->Add(toggleCheck, wxSizerFlags{}.Border());

        // initial color mode
        wxArrayString colorChoices;
        colorChoices.Add(_(L"Auto"));
        colorChoices.Add(_(L"Light"));
        colorChoices.Add(_(L"Dark"));
        auto* colorRadio =
            new wxRadioBox(this, wxID_ANY, _(L"Initial Color Mode"), wxDefaultPosition,
                           wxDefaultSize, colorChoices, 1, wxRA_SPECIFY_ROWS);
        colorRadio->SetValidator(wxGenericValidator{ &m_colorMode });
        colorRadio->Enable(m_includeColorModeToggle);
        mainSizer->Add(colorRadio, wxSizerFlags{}.Expand().Border());

        toggleCheck->Bind(wxEVT_CHECKBOX, [toggleCheck, colorRadio](wxCommandEvent&)
                          { colorRadio->Enable(toggleCheck->GetValue()); });

        auto* countUpCheck = new wxCheckBox(this, wxID_ANY, _(L"Count up large numbers"));
        countUpCheck->SetValidator(wxGenericValidator{ &m_countUpNumbers });
        mainSizer->Add(countUpCheck, wxSizerFlags{}.Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);
        }

    //------------------------------------------------------
    Wisteria::HtmlDashboardOptions::View HtmlDashboardDlg::GetInitialView() const noexcept
        {
        using View = Wisteria::HtmlDashboardOptions::View;
        return (m_view == 0) ? View::Atlas : View::Story;
        }

    //------------------------------------------------------
    Wisteria::HtmlDashboardOptions::ColorMode HtmlDashboardDlg::GetInitialColorMode() const noexcept
        {
        using ColorMode = Wisteria::HtmlDashboardOptions::ColorMode;
        return (m_colorMode == 0) ? ColorMode::Auto :
               (m_colorMode == 1) ? ColorMode::Light :
                                    ColorMode::Dark;
        }
    } // namespace Wisteria::UI
