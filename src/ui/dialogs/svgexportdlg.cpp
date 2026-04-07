///////////////////////////////////////////////////////////////////////////////
// Name:        svgexportdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgexportdlg.h"
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

        // page size controls
        auto* sizeSizer = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Page Size (pixels)"));
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

        contentSizer->Add(sizeSizer, wxSizerFlags{});
        contentSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

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
        wxPaintDC dc(m_previewPanel);
        dc.SetBackground(wxBrush{ m_previewPanel->GetBackgroundColour() });
        dc.Clear();

        const wxSize panelSize = m_previewPanel->GetClientSize();
        const wxString label = _(L"[Preview]");
        const wxSize textSize = dc.GetTextExtent(label);
        dc.DrawText(label, (panelSize.GetWidth() - textSize.GetWidth()) / 2,
                    (panelSize.GetHeight() - textSize.GetHeight()) / 2);
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
