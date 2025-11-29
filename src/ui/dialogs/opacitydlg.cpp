/////////////////////////////////////////////////////////////////////////////
// Name:        opacitydlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "opacitydlg.h"
#include "../../math/mathematics.h"
#include <utility>
#include <wx/slider.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //-------------------------------------------------------------
    OpacityDlg::OpacityDlg(
        wxWindow* parent, const uint8_t opacity, wxBitmap image, const wxWindowID id /*= wxID_ANY*/,
        const wxString& caption /*= _(L"Set Opacity")*/, const wxPoint& pos /*= wxDefaultPosition*/,
        const wxSize& size /*= wxDefaultSize*/,
        const long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN|wxRESIZE_BORDER*/)
        : m_opacity(opacity), m_image(std::move(image))
        {
        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style);

        CreateControls();

        Bind(wxEVT_SCROLL_CHANGED, &OpacityDlg::OnChangeOpacity, this);

        Centre();
        }

    //-------------------------------------------------------------
    void OpacityDlg::OnChangeOpacity(const wxScrollEvent& event)
        {
        if (m_thumb != nullptr)
            {
            m_thumb->SetOpacity(static_cast<uint8_t>(event.GetPosition()));
            }
        }

    //-------------------------------------------------------------
    void OpacityDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        assert(m_image.IsOk());
        if (m_image.IsOk())
            {
            const wxSize scaledSize = FromDIP(wxSize{ 300, 300 });
            const std::pair<double, double> thumbSize = geometry::downscaled_size(
                std::make_pair(m_image.GetWidth(), m_image.GetHeight()),
                std::make_pair(scaledSize.GetWidth(), scaledSize.GetHeight()));

            m_thumb = new Thumbnail(this, m_image, ClickMode::FullSizeViewable, false, wxID_ANY,
                                    wxDefaultPosition, wxSize(thumbSize.first, thumbSize.second));
            m_thumb->SetOpacity(static_cast<uint8_t>(GetOpacity()));
            m_thumb->SetMinSize(wxSize(thumbSize.first, thumbSize.second));

            mainSizer->Add(m_thumb, wxSizerFlags{ 1 }.Expand());
            }

        mainSizer->Add(new wxSlider(this, wxID_ANY, m_opacity, wxALPHA_TRANSPARENT, wxALPHA_OPAQUE,
                                    wxDefaultPosition, wxDefaultSize,
                                    wxSL_HORIZONTAL | wxSL_LABELS | wxSL_AUTOTICKS,
                                    wxGenericValidator(&m_opacity)),
                       wxSizerFlags{}.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);
        }
    } // namespace Wisteria::UI
