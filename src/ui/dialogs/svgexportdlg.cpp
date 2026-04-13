///////////////////////////////////////////////////////////////////////////////
// Name:        svgexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgexportdlg.h"
#include <wx/clrpicker.h>
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    SvgExportDlg::SvgExportDlg(wxWindow* parent, const wxSize& defaultSize,
                               wxWindowID id /*= wxID_ANY*/,
                               const wxString& caption /*= _(L"SVG Export Options")*/,
                               const wxPoint& pos /*= wxDefaultPosition*/,
                               const wxSize& size /*= wxDefaultSize*/,
                               long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN*/)
        : m_pageWidth(defaultSize.GetWidth()), m_pageHeight(defaultSize.GetHeight())
        {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_VALIDATE_RECURSIVELY | wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style);

        CreateControls();

        Bind(wxEVT_BUTTON, &SvgExportDlg::OnOK, this, wxID_OK);
        Bind(wxEVT_SPINCTRL, &SvgExportDlg::OnSizeChanged, this);
        m_previewPanel->Bind(wxEVT_PAINT, &SvgExportDlg::OnPaintPreview, this);

        Centre();
        }

    //------------------------------------------------------
    void SvgExportDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);
        mainSizer->Add(contentSizer, wxSizerFlags{}.Expand().Border());

        auto* leftColumnSizer = new wxBoxSizer(wxVERTICAL);
        contentSizer->Add(leftColumnSizer, wxSizerFlags{});
        contentSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        // page size controls
        auto* sizeSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Page Size"));
        auto* sizeGridSizer = new wxFlexGridSizer(2, 2, wxSizerFlags::GetDefaultBorder(),
                                                  wxSizerFlags::GetDefaultBorder());
        sizeGridSizer->AddGrowableCol(1, 1);
        sizeSizer->Add(sizeGridSizer, wxSizerFlags{}.Expand());

        sizeGridSizer->Add(new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, _(L"Width:")),
                           wxSizerFlags{}.CenterVertical());
        auto* widthCtrl =
            new wxSpinCtrl(sizeSizer->GetStaticBox(), PAGE_WIDTH_ID, std::to_wstring(m_pageWidth),
                           wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        widthCtrl->SetValidator(wxGenericValidator{ &m_pageWidth });
        sizeGridSizer->Add(widthCtrl, wxSizerFlags{}.Expand());

        sizeGridSizer->Add(new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, _(L"Height:")),
                           wxSizerFlags{}.CenterVertical());
        auto* heightCtrl =
            new wxSpinCtrl(sizeSizer->GetStaticBox(), PAGE_HEIGHT_ID, std::to_wstring(m_pageHeight),
                           wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        heightCtrl->SetValidator(wxGenericValidator{ &m_pageHeight });
        sizeGridSizer->Add(heightCtrl, wxSizerFlags{}.Expand());

        leftColumnSizer->Add(sizeSizer, wxSizerFlags{}.Expand());
        leftColumnSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        // horizontal gap
        auto* gapSizer = new wxBoxSizer(wxHORIZONTAL);
        gapSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Horizontal page gap:")),
                      wxSizerFlags{}.CenterVertical().Border(wxRIGHT));
        auto* gapCtrl = new wxSpinCtrl(this, HORIZONTAL_GAP_ID, std::to_wstring(m_horizontalGap),
                                       wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 0, 500);
        gapCtrl->SetValidator(wxGenericValidator{ &m_horizontalGap });
        gapSizer->Add(gapCtrl, wxSizerFlags{}.CenterVertical());
        leftColumnSizer->Add(gapSizer, wxSizerFlags{}.Expand().Border());
        leftColumnSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        // interactive features
        auto* featuresSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Interactive Features"));

        auto* layoutToggleCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Layout toggle"));
        layoutToggleCheck->SetValidator(wxGenericValidator{ &m_includeLayoutToggle });
        featuresSizer->Add(layoutToggleCheck, wxSizerFlags{}.Border());

        auto* darkModeToggleCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Dark mode toggle"));
        darkModeToggleCheck->SetValidator(wxGenericValidator{ &m_includeDarkModeToggle });
        featuresSizer->Add(darkModeToggleCheck, wxSizerFlags{}.Border());

        auto* slideshowCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Slideshow navigation"));
        slideshowCheck->SetValidator(wxGenericValidator{ &m_includeSlideshow });
        featuresSizer->Add(slideshowCheck, wxSizerFlags{}.Border());

        auto* transitionsCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Transitions"));
        transitionsCheck->SetValidator(wxGenericValidator{ &m_includeTransitions });
        featuresSizer->Add(transitionsCheck, wxSizerFlags{}.Border());

        auto* highlightingCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Hover highlighting"));
        highlightingCheck->SetValidator(wxGenericValidator{ &m_includeHighlighting });
        featuresSizer->Add(highlightingCheck, wxSizerFlags{}.Border());

        auto* colorSizer = new wxBoxSizer(wxHORIZONTAL);
        colorSizer->Add(
            new wxStaticText(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Button Color:")),
            wxSizerFlags{}.CenterVertical());
        auto* colorPicker =
            new wxColourPickerCtrl(featuresSizer->GetStaticBox(), BUTTON_COLOR_ID, m_buttonColor);
        colorPicker->SetValidator(wxGenericValidator{ &m_buttonColor });
        colorSizer->Add(colorPicker, wxSizerFlags{}.Border());
        featuresSizer->Add(colorSizer, wxSizerFlags{}.Expand().Border());

        leftColumnSizer->Add(featuresSizer, wxSizerFlags{}.Expand());

        // preview panel inside a fixed-size bounding box
        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        const wxSize previewBounds = FromDIP(wxSize{ 150, 150 });
        m_previewPanel =
            new wxPanel(previewSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, previewBounds);
        m_previewPanel->SetBackgroundColour(*wxLIGHT_GREY);
        previewSizer->Add(m_previewPanel, wxSizerFlags{}.Center());
        previewSizer->SetMinSize(previewBounds);
        contentSizer->Add(previewSizer, wxSizerFlags{});

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        UpdatePreview();
        }

    //------------------------------------------------------
    void SvgExportDlg::OnSizeChanged([[maybe_unused]] wxSpinEvent& event)
        {
        TransferDataFromWindow();
        UpdatePreview();
        }

    //------------------------------------------------------
    void SvgExportDlg::OnPaintPreview([[maybe_unused]] wxPaintEvent& event)
        {
        wxPaintDC pdc(m_previewPanel);
        wxGCDC dc(pdc);

        const wxColour bgColour = m_previewPanel->GetBackgroundColour();
        dc.SetBackground(wxBrush{ bgColour });
        dc.Clear();

        const wxSize panelSize = m_previewPanel->GetClientSize();
        constexpr int margin = 15;
        const int diameter = std::min(panelSize.GetWidth(), panelSize.GetHeight()) - (margin * 2);

        if (diameter <= 0)
            {
            return;
            }

        const int x = (panelSize.GetWidth() - diameter) / 2;
        const int y = (panelSize.GetHeight() - diameter) / 2;
        const wxPoint center(x + diameter / 2, y + diameter / 2);
        const int radius = diameter / 2;

        // draw the main black circle
        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawEllipse(x, y, diameter, diameter);

        // draw the "Pie Slice" lines
        wxPen slicePen(bgColour, 2);
        dc.SetPen(slicePen);

        // line 1: straight up (12 o'clock)
        dc.DrawLine(center, wxPoint(center.x, center.y - radius));

        // line 2: to the right-middle (roughly 4 o'clock)
        dc.DrawLine(center, wxPoint(center.x + (radius * 0.86), center.y + (radius * 0.5)));

        // line 3: to the left-bottom (roughly 8 o'clock)
        dc.DrawLine(center, wxPoint(center.x - (radius * 0.5), center.y + (radius * 0.86)));
        }

    //------------------------------------------------------
    void SvgExportDlg::UpdatePreview()
        {
        if (m_previewPanel == nullptr || m_pageWidth <= 0 || m_pageHeight <= 0)
            {
            return;
            }

        // fit the page aspect ratio into the fixed preview bounding box
        const wxSize bounds = FromDIP(wxSize{ 150, 150 });

        const double scaleX = safe_divide<double>(bounds.GetWidth(), m_pageWidth);
        const double scaleY = safe_divide<double>(bounds.GetHeight(), m_pageHeight);
        const double scale = std::min(scaleX, scaleY);

        const int previewWidth = std::max(1, static_cast<int>(m_pageWidth * scale));
        const int previewHeight = std::max(1, static_cast<int>(m_pageHeight * scale));

        m_previewPanel->SetMinSize(wxSize{ previewWidth, previewHeight });
        m_previewPanel->SetSize(wxSize{ previewWidth, previewHeight });

        GetSizer()->Layout();
        }
    } // namespace Wisteria::UI
