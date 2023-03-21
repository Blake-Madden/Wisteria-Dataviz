/////////////////////////////////////////////////////////////////////////////
// Name:        opacitydlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "opacitydlg.h"
#include "../../math/mathematics.h"

using namespace Wisteria::UI;

//-------------------------------------------------------------
OpacityDlg::OpacityDlg(wxWindow* parent, const uint8_t opacity,
                       const wxBitmap& image, wxWindowID id /*= wxID_ANY*/,
                       const wxString& caption /*= _(L"Set Opacity")*/,
                       const wxPoint& pos /*= wxDefaultPosition*/,
                       const wxSize& size /*= wxDefaultSize*/,
                       long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/) :
                       m_opacity(opacity), m_image(image)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls();

    Bind(wxEVT_SCROLL_CHANGED, &OpacityDlg::OnChangeOpacity, this);

    Centre();
    }

//-------------------------------------------------------------
void OpacityDlg::OnChangeOpacity(wxScrollEvent& event)
    {
    if (m_thumb)
        { m_thumb->SetOpacity(static_cast<uint8_t>(event.GetPosition())); }
    }

//-------------------------------------------------------------
void OpacityDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxASSERT(m_image.IsOk());
    if (m_image.IsOk())
        {
        const wxSize scaledSize = FromDIP(wxSize(300, 300));
        const std::pair<double,double> thumbSize =
            geometry::downscaled_size(std::make_pair(m_image.GetWidth(), m_image.GetHeight()),
            std::make_pair(scaledSize.GetWidth(),scaledSize.GetHeight()));

        m_thumb = new Thumbnail(this, m_image, Thumbnail::ClickMode::FullSizeViewable,
                                false, wxID_ANY, wxDefaultPosition,
                                wxSize(thumbSize.first, thumbSize.second));
        m_thumb->SetOpacity(static_cast<uint8_t>(GetOpacity()));
        m_thumb->SetMinSize(wxSize(thumbSize.first, thumbSize.second));

        mainSizer->Add(m_thumb, wxSizerFlags(1).Expand());
        }

    mainSizer->Add(new wxSlider(this, wxID_ANY,
            m_opacity, 0, 255, wxDefaultPosition, wxDefaultSize,
            wxSL_HORIZONTAL|wxSL_LABELS|wxSL_AUTOTICKS, wxGenericValidator(&m_opacity)),
            wxSizerFlags(0).Expand().Border(wxALL,wxSizerFlags::GetDefaultBorder()));

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL), 0, wxEXPAND|wxALL,
                                              wxSizerFlags::GetDefaultBorder());

    SetSizerAndFit(mainSizer);
    }
