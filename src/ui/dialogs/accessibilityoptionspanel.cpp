///////////////////////////////////////////////////////////////////////////////
// Name:        accessibilityoptionspanel.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "accessibilityoptionspanel.h"
#include <wx/sizer.h>
#include <wx/stattext.h>

namespace Wisteria::UI
    {
    //-------------------------------------------
    AccessibilityOptionsPanel::AccessibilityOptionsPanel(wxWindow* parent) : wxPanel(parent)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_autoAccessibilityCheck =
            new wxCheckBox(this, wxID_ANY, _(L"Automatically manage accessibility"));
        m_autoAccessibilityCheck->Bind(wxEVT_CHECKBOX, &AccessibilityOptionsPanel::OnAutoCheck,
                                       this);
        mainSizer->Add(m_autoAccessibilityCheck, wxSizerFlags{}.Border());

        auto* staticBox = new wxStaticBoxSizer(wxVERTICAL, this, _(L"Manual Settings"));
        auto* gridSizer = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                              wxSizerFlags::GetDefaultBorder());
        gridSizer->AddGrowableCol(1, 1);

        m_ariaLabelLabel = new wxStaticText(staticBox->GetStaticBox(), wxID_ANY, _(L"ARIA Label:"));
        gridSizer->Add(m_ariaLabelLabel, wxSizerFlags{}.CenterVertical());
        m_ariaLabelCtrl = new wxTextCtrl(staticBox->GetStaticBox(), wxID_ANY);
        gridSizer->Add(m_ariaLabelCtrl, wxSizerFlags{ 1 }.Expand());

        m_roleLabel = new wxStaticText(staticBox->GetStaticBox(), wxID_ANY, _(L"Role:"));
        gridSizer->Add(m_roleLabel, wxSizerFlags{}.CenterVertical());
        m_roleChoice = new wxChoice(staticBox->GetStaticBox(), wxID_ANY);
        m_roleChoice->Append(_(L"(default)"));
        m_roleChoice->Append(_DT(L"figure"));
        m_roleChoice->Append(_DT(L"img"));
        m_roleChoice->Append(_DT(L"presentation"));
        m_roleChoice->SetSelection(0);
        gridSizer->Add(m_roleChoice, wxSizerFlags{ 1 }.Expand());

        m_ariaHiddenCheck = new wxCheckBox(staticBox->GetStaticBox(), wxID_ANY, _(L"ARIA Hidden"));
        gridSizer->Add(m_ariaHiddenCheck, wxSizerFlags{}.Border(wxTOP));

        staticBox->Add(gridSizer, wxSizerFlags{}.Expand().Border());
        mainSizer->Add(staticBox, wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);
        }

    //-------------------------------------------
    void AccessibilityOptionsPanel::LoadFromItem(const GraphItems::GraphItemBase& item)
        {
        const auto& info = item.GetAccessibilityInfo();
        m_autoAccessibilityCheck->SetValue(info.m_auto);

        m_ariaLabelCtrl->SetValue(info.m_attributes.GetAriaLabel());

        const wxString role = info.m_attributes.GetRole();
        if (role.CmpNoCase(_DT(L"figure")) == 0)
            {
            m_roleChoice->SetSelection(1);
            }
        else if (role.CmpNoCase(_DT(L"img")) == 0)
            {
            m_roleChoice->SetSelection(2);
            }
        else if (role.CmpNoCase(_DT(L"presentation")) == 0)
            {
            m_roleChoice->SetSelection(3);
            }
        else
            {
            m_roleChoice->SetSelection(0);
            }

        m_ariaHiddenCheck->SetValue(info.m_attributes.IsAriaHidden());

        wxCommandEvent dummy;
        OnAutoCheck(dummy);
        }

    //-------------------------------------------
    void AccessibilityOptionsPanel::ApplyToItem(GraphItems::GraphItemBase& item) const
        {
        AccessibilityInfo info;
        info.m_auto = m_autoAccessibilityCheck->GetValue();

        if (!info.m_auto)
            {
            if (!m_ariaLabelCtrl->GetValue().empty())
                {
                info.m_attributes.AriaLabel(m_ariaLabelCtrl->GetValue());
                }
            const int roleSel = m_roleChoice->GetSelection();
            if (roleSel == 1)
                {
                info.m_attributes.Role(_DT(L"figure"));
                }
            else if (roleSel == 2)
                {
                info.m_attributes.Role(_DT(L"img"));
                }
            else if (roleSel == 3)
                {
                info.m_attributes.Role(_DT(L"presentation"));
                }

            if (m_ariaHiddenCheck->GetValue())
                {
                info.m_attributes.AriaHidden();
                }
            }

        item.SetAccessibilityInfo(info);

        // Also update templates for round-tripping
        item.SetPropertyTemplate(L"accessibility.aria-label", m_ariaLabelCtrl->GetValue());
        const int roleSelTemplate = m_roleChoice->GetSelection();
        wxString roleTemplate;
        if (roleSelTemplate == 1)
            {
            roleTemplate = _DT(L"figure");
            }
        else if (roleSelTemplate == 2)
            {
            roleTemplate = _DT(L"img");
            }
        else if (roleSelTemplate == 3)
            {
            roleTemplate = _DT(L"presentation");
            }
        item.SetPropertyTemplate(L"accessibility.role", roleTemplate);
        item.SetPropertyTemplate(L"accessibility.aria-hidden",
                                 m_ariaHiddenCheck->GetValue() ? L"true" : L"false");
        item.SetPropertyTemplate(L"accessibility.auto", info.m_auto ? L"true" : L"false");
        }

    //-------------------------------------------
    void AccessibilityOptionsPanel::OnAutoCheck(wxCommandEvent&)
        {
        const bool isAuto = m_autoAccessibilityCheck->GetValue();
        const bool enable = !isAuto;

        m_ariaLabelLabel->Enable(enable);
        m_ariaLabelCtrl->Enable(enable);
        m_roleLabel->Enable(enable);
        m_roleChoice->Enable(enable);
        m_ariaHiddenCheck->Enable(enable);

        m_ariaLabelLabel->Refresh();
        m_ariaLabelCtrl->Refresh();
        m_roleLabel->Refresh();
        m_roleChoice->Refresh();
        m_ariaHiddenCheck->Refresh();
        }
    } // namespace Wisteria::UI
