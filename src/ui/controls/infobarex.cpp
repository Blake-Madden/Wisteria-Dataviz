///////////////////////////////////////////////////////////////////////////////
// Name:        infobarex.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "infobarex.h"

using namespace Wisteria::UI;

bool InfoBarEx::Create(wxWindow* parent, wxWindowID winid)
    {
    // calling Hide() before Create() ensures that we're created initially hidden
    Hide();
    if (!wxWindow::Create(parent, winid))
        {
        return false;
        }

    // use special, easy to notice, colours
    SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOBK));

    // create the controls: icon, text and the button to dismiss the message.

    // the icon is not shown unless it's assigned a valid bitmap
    m_icon = new wxStaticBitmap(this, wxID_ANY, wxNullBitmap);

    m_text = new wxStaticText(this, wxID_ANY, wxEmptyString);
    m_text->SetForegroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));

    wxBitmapButton* closeButton = wxBitmapButton::NewCloseButton(this, wxID_CLOSE);
    closeButton->Connect(wxEVT_BUTTON, wxCommandEventHandler(InfoBarEx::OnButton), nullptr, this);
    closeButton->SetToolTip(_(L"Hide this notification message."));

    m_dontShowAgainCheckbox =
        new wxCheckBox(this, wxID_ANY, _(L"Do not show this again."), wxDefaultPosition,
                       wxDefaultSize, 0, wxGenericValidator(&m_dontShowAgain));
    m_dontShowAgainCheckbox->SetForegroundColour(
        wxSystemSettings::GetColour(wxSYS_COLOUR_INFOTEXT));

    // center the text inside the sizer with an icon to the left of it and a
    // button at the very right
    wxBoxSizer* const sizer = new wxBoxSizer(wxVERTICAL);
    wxBoxSizer* const firstRowSizer = new wxBoxSizer(wxHORIZONTAL);
    firstRowSizer->Add(
        m_icon, wxSizerFlags().CentreVertical().Border(wxALL, wxSizerFlags::GetDefaultBorder()));
    firstRowSizer->Add(
        m_text,
        wxSizerFlags().Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()).Proportion(1));
    firstRowSizer->AddStretchSpacer();
    firstRowSizer->Add(closeButton, wxSizerFlags().CentreVertical().Border(
                                        wxALL, wxSizerFlags::GetDefaultBorder()));

    wxBoxSizer* const secondRowSizer = new wxBoxSizer(wxHORIZONTAL);
    secondRowSizer->Add(m_dontShowAgainCheckbox, wxSizerFlags().CentreVertical().Border(
                                                     wxALL, wxSizerFlags::GetDefaultBorder()));

    sizer->Add(firstRowSizer, wxSizerFlags().Proportion(1).Expand());
    sizer->Add(secondRowSizer);
    sizer->Show(m_dontShowAgainCheckbox, m_includeDontShowAgain, true);

    SetSizer(sizer);

    return true;
    }

bool InfoBarEx::SetForegroundColour(const wxColour& colour)
    {
    if (m_text)
        {
        return m_text->SetForegroundColour(colour);
        }

    return true;
    }

InfoBarEx::BarPlacement InfoBarEx::GetBarPlacement() const
    {
    wxSizer* const sizer = GetContainingSizer();
    if (!sizer)
        {
        return BarPlacement::BarPlacement_Unknown;
        }

    const wxSizerItemList& siblings = sizer->GetChildren();
    if (siblings.GetFirst()->GetData()->GetWindow() == this)
        {
        return BarPlacement::BarPlacement_Top;
        }
    else if (siblings.GetLast()->GetData()->GetWindow() == this)
        {
        return BarPlacement::BarPlacement_Bottom;
        }
    else
        {
        return BarPlacement::BarPlacement_Unknown;
        }
    }

wxShowEffect InfoBarEx::GetShowEffect() const
    {
    if (m_showEffect != wxSHOW_EFFECT_MAX)
        {
        return m_showEffect;
        }

    switch (GetBarPlacement())
        {
    case BarPlacement::BarPlacement_Top:
        return wxSHOW_EFFECT_SLIDE_TO_BOTTOM;
    case BarPlacement::BarPlacement_Bottom:
        return wxSHOW_EFFECT_SLIDE_TO_TOP;
    case BarPlacement::BarPlacement_Unknown:
        [[fallthrough]];
    default:
        return wxSHOW_EFFECT_NONE;
        }
    }

wxShowEffect InfoBarEx::GetHideEffect() const
    {
    if (m_hideEffect != wxSHOW_EFFECT_MAX)
        {
        return m_hideEffect;
        }

    switch (GetBarPlacement())
        {
    case BarPlacement::BarPlacement_Top:
        return wxSHOW_EFFECT_SLIDE_TO_TOP;
    case BarPlacement::BarPlacement_Bottom:
        return wxSHOW_EFFECT_SLIDE_TO_BOTTOM;
    case BarPlacement::BarPlacement_Unknown:
        [[fallthrough]];
    default:
        return wxSHOW_EFFECT_NONE;
        }
    }

void InfoBarEx::UpdateParent()
    {
    wxWindow* const parent = GetParent();
    assert(parent);
    if (parent)
        {
        parent->Layout();
        }
    }

void InfoBarEx::DoHide()
    {
    HideWithEffect(GetHideEffect(), GetEffectDuration());
    UpdateParent();
    }

void InfoBarEx::DoShow()
    {
    // re-layout the parent first so that the window expands into an already
    // unoccupied by the other controls area: for this we need to change our
    // internal visibility flag to force Layout() to take us into account (an
    // alternative solution to this hack would be to temporarily set
    // wxRESERVE_SPACE_EVEN_IF_HIDDEN flag but it's not really better)

    // just change the internal flag indicating that the window is visible,
    // without really showing it
    wxWindowBase::Show();

    // adjust the parent layout to account for us
    UpdateParent();

    // reset the flag back before really showing the window or it wouldn't be
    // shown at all because it would believe itself already visible
    wxWindowBase::Show(false);

    // finally do really show the window.
    ShowWithEffect(GetShowEffect(), GetEffectDuration());
    }

void InfoBarEx::ShowMessage(const wxString& msg, int flags)
    {
    // first update the controls
    const int icon = flags & wxICON_MASK;
    if (!icon || (icon == wxICON_NONE))
        {
        m_icon->Hide();
        }
    else // do show an icon
        {
        m_icon->SetBitmap(wxArtProvider::GetBitmapBundle(wxArtProvider::GetMessageBoxIconId(flags),
                                                         wxART_BUTTON));
        m_icon->Show();
        }

    m_dontShowAgain = false;

    m_text->SetFont(GetFont());
    m_text->SetLabel(wxControl::EscapeMnemonics(msg));

    TransferDataToWindow();

    // then show this entire window if not done yet
    if (!IsShown())
        {
        DoShow();
        }
    else // we're already shown
        {
        // just update the layout to correspond to the new message
        Layout();
        }
    }

void InfoBarEx::Dismiss() { DoHide(); }

void InfoBarEx::OnButton(wxCommandEvent& event)
    {
    TransferDataFromWindow();
    DoHide();
    event.Skip();
    }
