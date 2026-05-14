///////////////////////////////////////////////////////////////////////////////
// Name:        svgexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgexportdlg.h"
#include "../../base/settings.h"
#include <utility>
#include <wx/clrpicker.h>
#include <wx/dcgraph.h>
#include <wx/graphics.h>
#include <wx/paper.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    SvgExportDlg::SvgExportDlg(wxWindow* parent, const wxSize& defaultSize,
                               const wxPrintData& printData,
                               const Wisteria::SVGReportOptions* savedOptions /*= nullptr*/,
                               wxWindowID id /*= wxID_ANY*/,
                               const wxString& caption /*= _(L"SVG Export Options")*/,
                               const wxPoint& pos /*= wxDefaultPosition*/,
                               const wxSize& size /*= wxDefaultSize*/,
                               long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN*/)
        : m_pageWidth(defaultSize.GetWidth()), m_pageHeight(defaultSize.GetHeight())
        {
        const wxPrintPaperType* paperType =
            wxThePrintPaperDatabase->FindPaperType(printData.GetPaperId());
        m_paperType = (paperType != nullptr) ? paperType->GetName() : _(L"Custom");
        m_orientation =
            (printData.GetOrientation() == wxLANDSCAPE) ? _(L"Landscape") : _(L"Portrait");

        // paper size is in tenths of a millimeter; convert to DIPs (96 DPI)
        constexpr double tenthsMmPerInch = 254.0;
        constexpr double dipsPerInch = 96.0;
        if (paperType != nullptr)
            {
            const wxSize sizeMM = paperType->GetSize();
            m_globalPageWidth =
                wxRound(safe_divide<double>(sizeMM.GetWidth(), tenthsMmPerInch) * dipsPerInch);
            m_globalPageHeight =
                wxRound(safe_divide<double>(sizeMM.GetHeight(), tenthsMmPerInch) * dipsPerInch);
            }
        else
            {
            // fallback: US Letter at 96 DPI
            m_globalPageWidth = static_cast<int>(8.5 * dipsPerInch);
            m_globalPageHeight = static_cast<int>(11.0 * dipsPerInch);
            }
        if (printData.GetOrientation() == wxLANDSCAPE)
            {
            std::swap(m_globalPageWidth, m_globalPageHeight);
            }

        if (savedOptions != nullptr)
            {
            m_useGlobalPrintSettings = savedOptions->m_useGlobalPrintSettings;
            m_includeTransitions = savedOptions->m_includeTransitions;
            m_includeHighlighting = savedOptions->m_includeHighlighting;
            m_includeLayoutOptions = savedOptions->m_includeLayoutOptions;
            m_includeDarkModeToggle = savedOptions->m_includeDarkModeToggle;
            m_includeSlideshow = savedOptions->m_includeSlideshow;
            m_includePageShadow = savedOptions->m_includePageShadow;
            m_themeColor = savedOptions->m_themeColor;
            m_layout = savedOptions->m_layout;
            }

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

        m_useGlobalPrintSettingsCheckbox =
            new wxCheckBox(sizeSizer->GetStaticBox(), USE_GLOBAL_PRINT_SETTINGS_ID,
                           _(L"Use global print settings"));
        m_useGlobalPrintSettingsCheckbox->SetValidator(
            wxGenericValidator{ &m_useGlobalPrintSettings });
        sizeSizer->Add(m_useGlobalPrintSettingsCheckbox, wxSizerFlags{}.Border());

        auto* globalPrintSettingsSizer = new wxBoxSizer(wxVERTICAL);
        auto* globalPrintSettingsIndentSizer = new wxBoxSizer(wxHORIZONTAL);
        globalPrintSettingsIndentSizer->AddSpacer(wxSizerFlags::GetDefaultBorder() * 2);

        auto* globalPrintLabelsSizer = new wxBoxSizer(wxVERTICAL);
        m_paperTypeLabel = new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, m_paperType);
        m_paperTypeLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        globalPrintLabelsSizer->Add(m_paperTypeLabel);

        m_orientationLabel =
            new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, m_orientation);
        m_orientationLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        globalPrintLabelsSizer->Add(m_orientationLabel);

        globalPrintSettingsIndentSizer->Add(globalPrintLabelsSizer);
        globalPrintSettingsSizer->Add(globalPrintSettingsIndentSizer);
        sizeSizer->Add(globalPrintSettingsSizer, wxSizerFlags{}.Expand().Border(wxBOTTOM));

        auto* sizeGridSizer = new wxFlexGridSizer(2, 2, wxSizerFlags::GetDefaultBorder(),
                                                  wxSizerFlags::GetDefaultBorder());
        sizeGridSizer->AddGrowableCol(1, 1);
        sizeSizer->Add(sizeGridSizer, wxSizerFlags{}.Expand().Border());

        m_widthLabel = new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, _(L"Width:"));
        sizeGridSizer->Add(m_widthLabel, wxSizerFlags{}.CenterVertical());
        m_widthCtrl =
            new wxSpinCtrl(sizeSizer->GetStaticBox(), PAGE_WIDTH_ID, std::to_wstring(m_pageWidth),
                           wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        m_widthCtrl->SetValidator(wxGenericValidator{ &m_pageWidth });
        sizeGridSizer->Add(m_widthCtrl, wxSizerFlags{}.Expand());

        m_heightLabel = new wxStaticText(sizeSizer->GetStaticBox(), wxID_STATIC, _(L"Height:"));
        sizeGridSizer->Add(m_heightLabel, wxSizerFlags{}.CenterVertical());
        m_heightCtrl =
            new wxSpinCtrl(sizeSizer->GetStaticBox(), PAGE_HEIGHT_ID, std::to_wstring(m_pageHeight),
                           wxDefaultPosition, wxDefaultSize, wxSP_ARROW_KEYS, 128, 10'000);
        m_heightCtrl->SetValidator(wxGenericValidator{ &m_pageHeight });
        sizeGridSizer->Add(m_heightCtrl, wxSizerFlags{}.Expand());

        leftColumnSizer->Add(sizeSizer, wxSizerFlags{}.Expand());
        leftColumnSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        // toggle custom size controls based on global print settings checkbox
        const auto toggleSizeControls = [this](const bool useGlobal)
        {
            m_widthLabel->Enable(!useGlobal);
            m_widthLabel->Refresh();
            m_widthCtrl->Enable(!useGlobal);
            m_heightLabel->Enable(!useGlobal);
            m_heightLabel->Refresh();
            m_heightCtrl->Enable(!useGlobal);
            m_paperTypeLabel->Enable(useGlobal);
            m_paperTypeLabel->Refresh();
            m_orientationLabel->Enable(useGlobal);
            m_orientationLabel->Refresh();
        };
        m_useGlobalPrintSettingsCheckbox->Bind(wxEVT_CHECKBOX,
                                               [this, toggleSizeControls](wxCommandEvent& event)
                                               {
                                                   m_useGlobalPrintSettings = event.IsChecked();
                                                   toggleSizeControls(m_useGlobalPrintSettings);
                                                   UpdatePreview();
                                               });
        toggleSizeControls(m_useGlobalPrintSettings);

        // interactive features
        auto* featuresSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Interactive Features"));

        auto* layoutToggleCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Layout options"));
        layoutToggleCheck->SetValidator(wxGenericValidator{ &m_includeLayoutOptions });
        featuresSizer->Add(layoutToggleCheck, wxSizerFlags{}.Border());

        wxArrayString layoutChoices;
        layoutChoices.Add(_(L"Stacked"));
        layoutChoices.Add(_(L"Duplex"));
        auto* layoutRadio =
            new wxRadioBox(featuresSizer->GetStaticBox(), LAYOUT_RADIO_ID, _(L"Page Layout"),
                           wxDefaultPosition, wxDefaultSize, layoutChoices, 1, wxRA_SPECIFY_COLS);
        // map enum to int
        layoutRadio->SetSelection(m_layout == Wisteria::SVGReportOptions::PageLayout::Stacked ? 0 :
                                                                                                1);
        featuresSizer->Add(layoutRadio, wxSizerFlags{}.Expand().Border());
        layoutRadio->Bind(wxEVT_RADIOBOX,
                          [this, layoutRadio](wxCommandEvent&)
                          {
                              m_layout = (layoutRadio->GetSelection() == 0) ?
                                             Wisteria::SVGReportOptions::PageLayout::Stacked :
                                             Wisteria::SVGReportOptions::PageLayout::Duplex;
                          });

        // enable/disable layout options
        layoutToggleCheck->Bind(wxEVT_CHECKBOX, [layoutRadio](wxCommandEvent& event)
                                { layoutRadio->Enable(event.IsChecked()); });
        // initial state
        layoutRadio->Enable(m_includeLayoutOptions);

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

        auto* shadowCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Page shadow"));
        shadowCheck->SetValidator(wxGenericValidator{ &m_includePageShadow });
        featuresSizer->Add(shadowCheck, wxSizerFlags{}.Border());

        auto* highlightingCheck =
            new wxCheckBox(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Hover highlighting"));
        highlightingCheck->SetValidator(wxGenericValidator{ &m_includeHighlighting });
        featuresSizer->Add(highlightingCheck, wxSizerFlags{}.Border());

        auto* colorSizer = new wxBoxSizer(wxHORIZONTAL);
        colorSizer->Add(
            new wxStaticText(featuresSizer->GetStaticBox(), wxID_ANY, _(L"Theme Color:")),
            wxSizerFlags{}.CenterVertical());
        auto* colorPicker =
            new wxColourPickerCtrl(featuresSizer->GetStaticBox(), THEME_COLOR_ID, m_themeColor);
        colorPicker->SetValidator(wxGenericValidator{ &m_themeColor });
        colorSizer->Add(colorPicker, wxSizerFlags{}.Border());
        featuresSizer->Add(colorSizer, wxSizerFlags{}.Expand().Border());

        leftColumnSizer->Add(featuresSizer, wxSizerFlags{}.Expand());

        // preview panel inside a fixed-size bounding box
        auto* previewSizer = new wxBoxSizer(wxVERTICAL);
        const wxSize previewBounds = FromDIP(wxSize{ 150, 150 });
        m_previewPanel = new wxPanel(this, wxID_ANY, wxDefaultPosition, previewBounds);
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

        const wxRect drawArea{ wxPoint{ 0, 0 }, dc.GetSize() };
        wxRect pageArea{ drawArea };
        pageArea.Deflate(FromDIP(5));
        pageArea.SetTopLeft(drawArea.GetTopLeft());
        wxRect shadowArea{ pageArea };
        shadowArea.Offset(drawArea.GetRight() - pageArea.GetRight(),
                          drawArea.GetBottom() - pageArea.GetBottom());
        dc.SetBrush(Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::DimGray));
        dc.DrawRectangle(shadowArea);

        const wxColour bgColour =
            Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::AntiqueWhite);
        dc.SetBrush(wxBrush{ bgColour });
        dc.SetPen(Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::DarkGray));
        dc.DrawRectangle(pageArea);

        constexpr int margin{ 15 };
        const int availWidth = pageArea.GetSize().GetWidth() - (margin * 2);
        const int availHeight = pageArea.GetSize().GetHeight() - (margin * 2);

        if (availWidth <= 0 || availHeight <= 0)
            {
            return;
            }

        // pie occupies roughly half the available height
        const int pieDiameter = std::min(availWidth, availHeight / 2);
        if (pieDiameter <= 0)
            {
            return;
            }

        // distribute remaining vertical space: 4 text lines above, 2 below
        const int remaining = availHeight - pieDiameter;
        const int aboveHeight = remaining * 4 / 6;
        const int belowHeight = remaining - aboveHeight;
        const int slotAbove = std::max(1, aboveHeight / 4);
        const int slotBelow = std::max(1, belowHeight / 2);
        const int lineH = std::max(2, slotAbove / 3);

        const int pieY = margin + 4 * slotAbove;
        const int pieX = margin + (availWidth - pieDiameter) / 2;

        // text line widths (varying to look like natural text)
        const std::array<int, 6> widthFracs = { 10, 9, 10, 7, 9, 6 };

        const wxColour textLineColor{ 100, 100, 100 };
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush{ textLineColor });

        // 4 lines above the pie
        for (int i = 0; i < 4; ++i)
            {
            const int lineWidth = availWidth * widthFracs[i] / 10;
            const int lineY = margin + i * slotAbove + (slotAbove - lineH) / 2;
            dc.DrawRoundedRectangle(margin, lineY, lineWidth, lineH, 1);
            }

        // pie chart
        const wxPoint center(pieX + pieDiameter / 2, pieY + pieDiameter / 2);
        const int radius = pieDiameter / 2;

        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawEllipse(pieX, pieY, pieDiameter, pieDiameter);

        wxPen slicePen(bgColour, 2);
        dc.SetPen(slicePen);
        dc.DrawLine(center, wxPoint(center.x, center.y - radius));
        dc.DrawLine(center, wxPoint(center.x + (radius * 0.86), center.y + (radius * 0.5)));
        dc.DrawLine(center, wxPoint(center.x - (radius * 0.5), center.y + (radius * 0.86)));

        // 2 lines below the pie
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(wxBrush{ textLineColor });
        for (int i = 0; i < 2; ++i)
            {
            const int lineWidth = availWidth * widthFracs[i + 4] / 10;
            const int lineY = pieY + pieDiameter + i * slotBelow + (slotBelow - lineH) / 2;
            dc.DrawRoundedRectangle(margin, lineY, lineWidth, lineH, 1);
            }
        }

    //------------------------------------------------------
    void SvgExportDlg::UpdatePreview()
        {
        if (m_previewPanel == nullptr)
            {
            return;
            }

        const int useWidth = m_useGlobalPrintSettings ? m_globalPageWidth : m_pageWidth;
        const int useHeight = m_useGlobalPrintSettings ? m_globalPageHeight : m_pageHeight;

        if (useWidth <= 0 || useHeight <= 0)
            {
            return;
            }

        // fit the page aspect ratio into the fixed preview bounding box
        const wxSize bounds = FromDIP(wxSize{ 150, 150 });

        const double scaleX = safe_divide<double>(bounds.GetWidth(), useWidth);
        const double scaleY = safe_divide<double>(bounds.GetHeight(), useHeight);
        const double scale = std::min(scaleX, scaleY);

        const int previewWidth = std::max(1, static_cast<int>(useWidth * scale));
        const int previewHeight = std::max(1, static_cast<int>(useHeight * scale));

        m_previewPanel->SetMinSize(wxSize{ previewWidth, previewHeight });
        m_previewPanel->SetSize(wxSize{ previewWidth, previewHeight });

        GetSizer()->Layout();
        }
    } // namespace Wisteria::UI
