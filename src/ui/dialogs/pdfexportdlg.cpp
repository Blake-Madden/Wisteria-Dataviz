///////////////////////////////////////////////////////////////////////////////
// Name:        pdfexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdfexportdlg.h"
#include "../../app/wisteriaapp.h"
#include "../../base/settings.h"
#include <algorithm>
#include <array>
#include <wx/dcgraph.h>
#include <wx/paper.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    PdfExportDlg::PdfExportDlg(wxWindow* parent, const wxPrintData& printData,
                               const PdfExportOptions& options, const wxString& caption)
        : DialogWithHelp(parent, wxID_ANY, caption), m_printData(printData), m_options(options)
        {
        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //------------------------------------------------------
    void PdfExportDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* topSizer = new wxBoxSizer(wxHORIZONTAL);
        mainSizer->Add(topSizer, wxSizerFlags{}.Expand().Border());

        // left column: page settings
        auto* pageSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Page"));

        auto* pageSetupBtn = new wxButton(pageSizer->GetStaticBox(), wxID_ANY, _(L"Page Setup..."));
        pageSetupBtn->Bind(wxEVT_BUTTON,
                           [this]([[maybe_unused]] wxCommandEvent&)
                           {
                               wxPageSetupDialogData pageSetupData;
                               pageSetupData.SetPrintData(m_printData);
                               wxPageSetupDialog dlg(this, &pageSetupData);
                               if (dlg.ShowModal() == wxID_OK)
                                   {
                                   m_printData = dlg.GetPageSetupData().GetPrintData();
                                   UpdateLabels();
                                   }
                           });
        pageSizer->Add(pageSetupBtn, wxSizerFlags{}.Border(wxBOTTOM));

        auto* gridSizer = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });

        // paper type
        gridSizer->Add(new wxStaticText(pageSizer->GetStaticBox(), wxID_STATIC, _(L"Paper type:")),
                       wxSizerFlags{}.CenterVertical());
        m_paperTypeLabel = new wxStaticText(pageSizer->GetStaticBox(), wxID_ANY, wxEmptyString);
        m_paperTypeLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        gridSizer->Add(m_paperTypeLabel, wxSizerFlags{}.CenterVertical());

        // orientation
        gridSizer->Add(new wxStaticText(pageSizer->GetStaticBox(), wxID_STATIC, _(L"Orientation:")),
                       wxSizerFlags{}.CenterVertical());
        m_orientationLabel = new wxStaticText(pageSizer->GetStaticBox(), wxID_ANY, wxEmptyString);
        m_orientationLabel->SetForegroundColour(Wisteria::Settings::GetHighlightedLabelColor());
        gridSizer->Add(m_orientationLabel, wxSizerFlags{}.CenterVertical());

        pageSizer->Add(gridSizer, wxSizerFlags{}.Expand());
        topSizer->Add(pageSizer, wxSizerFlags{}.Expand());
        topSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        // right column: preview
        auto* previewSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Preview"));
        const wxSize previewBounds = FromDIP(wxSize{ 150, 150 });
        m_previewPanel =
            new wxPanel(previewSizer->GetStaticBox(), wxID_ANY, wxDefaultPosition, previewBounds);
        m_previewPanel->SetBackgroundColour(*wxLIGHT_GREY);
        previewSizer->Add(m_previewPanel, wxSizerFlags{}.Center());
        previewSizer->SetMinSize(previewBounds);
        topSizer->Add(previewSizer, wxSizerFlags{});

        // document info section
        auto* docInfoBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Document Information"));
        auto* docInfoGrid = new wxFlexGridSizer(
            2, wxSize{ wxSizerFlags::GetDefaultBorder() * 2, wxSizerFlags::GetDefaultBorder() });
        docInfoGrid->AddGrowableCol(1, 1);

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Title:")),
                         wxSizerFlags{}.CenterVertical());
        auto* titleCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           FromDIP(wxSize{ 300, -1 }), 0, wxGenericValidator{ &m_options.m_title });
        docInfoGrid->Add(titleCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Author:")),
                         wxSizerFlags{}.CenterVertical());
        auto* authorCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_author });
        docInfoGrid->Add(authorCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Subject:")),
                         wxSizerFlags{}.CenterVertical());
        auto* subjectCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_subject });
        docInfoGrid->Add(subjectCtrl, wxSizerFlags{}.Expand());

        docInfoGrid->Add(new wxStaticText(docInfoBox->GetStaticBox(), wxID_STATIC, _(L"Keywords:")),
                         wxSizerFlags{}.CenterVertical());
        auto* keywordsCtrl =
            new wxTextCtrl(docInfoBox->GetStaticBox(), wxID_ANY, wxString{}, wxDefaultPosition,
                           wxDefaultSize, 0, wxGenericValidator{ &m_options.m_keywords });
        docInfoGrid->Add(keywordsCtrl, wxSizerFlags{}.Expand());

        docInfoBox->Add(docInfoGrid, wxSizerFlags{}.Expand().Border());

        auto* compressCheck = new wxCheckBox(
            docInfoBox->GetStaticBox(), wxID_ANY, _(L"Compress content"), wxDefaultPosition,
            wxDefaultSize, 0, wxGenericValidator{ &m_options.m_compress });
        docInfoBox->Add(compressCheck, wxSizerFlags{}.Border());

        mainSizer->Add(docInfoBox, wxSizerFlags{}.Expand().Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        m_previewPanel->Bind(wxEVT_PAINT, &PdfExportDlg::OnPaintPreview, this);

        UpdateLabels();
        }

    //------------------------------------------------------
    void PdfExportDlg::UpdateLabels()
        {
        const wxPrintPaperType* paperType =
            wxThePrintPaperDatabase->FindPaperType(m_printData.GetPaperId());
        m_paperTypeLabel->SetLabel((paperType != nullptr) ? paperType->GetName() : _(L"Custom"));
        m_orientationLabel->SetLabel(
            (m_printData.GetOrientation() == wxLANDSCAPE) ? _(L"Landscape") : _(L"Portrait"));

        // update preview dimensions
        constexpr double tenthsMmPerInch = 254.0;
        constexpr double dipsPerInch = 96.0;
        if (paperType != nullptr)
            {
            const wxSize sizeMM = paperType->GetSize();
            m_pageWidth =
                wxRound(safe_divide<double>(sizeMM.GetWidth(), tenthsMmPerInch) * dipsPerInch);
            m_pageHeight =
                wxRound(safe_divide<double>(sizeMM.GetHeight(), tenthsMmPerInch) * dipsPerInch);
            }
        else
            {
            // fallback: US Letter at 96 DPI
            m_pageWidth = static_cast<int>(8.5 * dipsPerInch);
            m_pageHeight = static_cast<int>(11.0 * dipsPerInch);
            }
        if (m_printData.GetOrientation() == wxLANDSCAPE)
            {
            std::swap(m_pageWidth, m_pageHeight);
            }

        UpdatePreview();
        }

    //------------------------------------------------------
    void PdfExportDlg::UpdatePreview()
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
        m_previewPanel->Refresh();
        }

    //------------------------------------------------------
    void PdfExportDlg::OnPaintPreview([[maybe_unused]] wxPaintEvent& event)
        {
        wxPaintDC pdc(m_previewPanel);
        wxGCDC dc(pdc);

        const wxColour bgColour = m_previewPanel->GetBackgroundColour();
        dc.SetBackground(wxBrush{ bgColour });
        dc.Clear();

        const wxSize panelSize = m_previewPanel->GetClientSize();
        constexpr int margin = 15;
        const int availWidth = panelSize.GetWidth() - (margin * 2);
        const int availHeight = panelSize.GetHeight() - (margin * 2);

        if (availWidth <= 0 || availHeight <= 0)
            {
            return;
            }

        // draw a stylized "report" page
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.SetBrush(*wxWHITE_BRUSH);
        dc.DrawRectangle(0, 0, panelSize.GetWidth(), panelSize.GetHeight());

        // pie chart occupies roughly half the available height
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

        // text line widths
        const std::array<int, 6> widthFracs = { 10, 9, 10, 7, 9, 6 };
        const wxColour textLineColor{ 100, 100, 100 };
        dc.SetBrush(wxBrush{ textLineColor });

        // 4 lines above the pie
        for (int i = 0; i < 4; ++i)
            {
            const int lineWidth = availWidth * widthFracs[i] / 10;
            const int lineY = margin + i * slotAbove + (slotAbove - lineH) / 2;
            dc.DrawRoundedRectangle(margin, lineY, lineWidth, lineH, 1);
            }

        // pie chart mockup
        const wxPoint center(pieX + pieDiameter / 2, pieY + pieDiameter / 2);
        const int radius = pieDiameter / 2;

        dc.SetPen(*wxBLACK_PEN);
        dc.SetBrush(*wxBLACK_BRUSH);
        dc.DrawEllipse(pieX, pieY, pieDiameter, pieDiameter);

        wxPen slicePen(*wxWHITE, 2);
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
    } // namespace Wisteria::UI
