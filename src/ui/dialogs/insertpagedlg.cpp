///////////////////////////////////////////////////////////////////////////////
// Name:        insertpagedlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "insertpagedlg.h"
#include <wx/dcbuffer.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    InsertPageDlg::InsertPageDlg(wxWindow* parent, const wxWindowID id, const wxString& caption,
                                 const wxPoint& pos, const wxSize& size, const long style)
        : DialogWithHelp(parent, id, caption, pos, size, style)
        {
        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //-------------------------------------------
    void InsertPageDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* contentSizer = new wxBoxSizer(wxHORIZONTAL);

        auto* gridSizer = new wxFlexGridSizer(2, wxSize{ FromDIP(8), FromDIP(4) });
        gridSizer->AddGrowableCol(1, 1);

        // rows
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Rows:")),
                       wxSizerFlags{}.CenterVertical());
        m_rowsSpin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize,
                                    wxSP_ARROW_KEYS, 1, 100, 1);
        gridSizer->Add(m_rowsSpin, wxSizerFlags{}.Expand());

        // columns
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Columns:")),
                       wxSizerFlags{}.CenterVertical());
        m_columnsSpin = new wxSpinCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition,
                                       wxDefaultSize, wxSP_ARROW_KEYS, 1, 100, 1);
        gridSizer->Add(m_columnsSpin, wxSizerFlags{}.Expand());

        // name
        gridSizer->Add(new wxStaticText(this, wxID_STATIC, _(L"Name:")),
                       wxSizerFlags{}.CenterVertical());
        m_nameCtrl = new wxTextCtrl(this, wxID_ANY);
        gridSizer->Add(m_nameCtrl, wxSizerFlags{}.Expand());

        contentSizer->Add(gridSizer, wxSizerFlags{ 1 }.Expand().Border());

        // preview panel
        const int previewHeight = FromDIP(200);
        const int previewWidth = static_cast<int>(previewHeight * math_constants::golden_ratio);
        m_previewPanel =
            new wxPanel(this, wxID_ANY, wxDefaultPosition, wxSize(previewWidth, previewHeight),
                        wxTAB_TRAVERSAL | wxWANTS_CHARS);
        m_previewPanel->SetBackgroundStyle(wxBG_STYLE_PAINT);
        m_previewPanel->SetMinSize(wxSize(previewWidth, previewHeight));
        contentSizer->Add(m_previewPanel, wxSizerFlags{ 1 }.Expand().Border());

        mainSizer->Add(contentSizer, wxSizerFlags{ 1 }.Expand());

        // OK/Cancel buttons
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        const auto paintPreview = [this]([[maybe_unused]]
                                         wxPaintEvent&)
        {
            wxAutoBufferedPaintDC dc(m_previewPanel);
            dc.SetBackground(*wxWHITE_BRUSH);
            dc.Clear();

            const auto clientRect = m_previewPanel->GetClientRect();
            if (clientRect.IsEmpty())
                {
                return;
                }

            const auto rows = static_cast<size_t>(m_rowsSpin->GetValue());
            const auto columns = static_cast<size_t>(m_columnsSpin->GetValue());

            const auto cellWidth = safe_divide<double>(clientRect.GetWidth(), columns);
            const auto cellHeight = safe_divide<double>(clientRect.GetHeight(), rows);

            for (size_t row = 0; row < rows; ++row)
                {
                for (size_t col = 0; col < columns; ++col)
                    {
                    const auto cellLeft = static_cast<int>(col * cellWidth);
                    const auto cellTop = static_cast<int>(row * cellHeight);
                    const auto cellW = static_cast<int>((col + 1) * cellWidth) - cellLeft;
                    const auto cellH = static_cast<int>((row + 1) * cellHeight) - cellTop;

                    dc.SetPen(*wxBLACK_PEN);
                    dc.SetBrush(*wxWHITE_BRUSH);
                    dc.DrawRectangle(clientRect.x + cellLeft, clientRect.y + cellTop, cellW, cellH);
                    }
                }
        };

        m_previewPanel->Bind(wxEVT_PAINT, paintPreview);
        m_previewPanel->Bind(wxEVT_SIZE,
                             [this](wxSizeEvent& evt)
                             {
                                 m_previewPanel->Refresh();
                                 evt.Skip();
                             });

        m_rowsSpin->Bind(wxEVT_SPINCTRL,
                         [this]([[maybe_unused]] wxSpinEvent&) { m_previewPanel->Refresh(); });
        m_columnsSpin->Bind(wxEVT_SPINCTRL,
                            [this]([[maybe_unused]] wxSpinEvent&) { m_previewPanel->Refresh(); });
        }
    } // namespace Wisteria::UI
