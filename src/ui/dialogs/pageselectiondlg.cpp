/////////////////////////////////////////////////////////////////////////////
// Name:        pageselectiondlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pageselectiondlg.h"
#include "../../base/colorbrewer.h"
#include <algorithm>
#include <utility>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //----------------------------------------------------------
    PageSelectionDlg::PageSelectionDlg(wxWindow* parent, const wxString& caption,
                                       wxArrayString pageLabels, std::vector<wxString> pageGuids,
                                       const std::vector<wxString>& excludedGuids,
                                       const wxWindowID id, const wxPoint& pos, const wxSize& size,
                                       const long style)
        : m_pageLabels(std::move(pageLabels)), m_pageGuids(std::move(pageGuids))
        {
        wxASSERT_MSG(m_pageLabels.GetCount() == m_pageGuids.size(),
                     L"Pages don't equal GUID size?!");

        for (size_t idx = 0; idx < m_pageGuids.size(); ++idx)
            {
            const bool isExcluded =
                std::ranges::any_of(excludedGuids, [this, idx](const auto& guid)
                                    { return guid.CmpNoCase(m_pageGuids[idx]) == 0; });
            if (!isExcluded)
                {
                m_checkedItems.Add(static_cast<int>(idx));
                }
            }

        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        DialogWithHelp::Create(parent, id, caption, pos, size, style);

        CreateControls();
        GetSizer()->SetSizeHints(this);
        Centre();
        }

    //----------------------------------------------------------
    void PageSelectionDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* banner = new wxBannerWindow(this, wxTOP);
        banner->SetText(_(L"Pages to Export"), _(L"Select the pages to include in the export."));
        banner->SetGradient(banner->GetBackgroundColour(),
                            Colors::ColorContrast::ShadeOrTint(banner->GetBackgroundColour()));
        mainSizer->Add(banner, wxSizerFlags{}.Expand());

        m_pageList =
            new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize{ 400, 250 }),
                               m_pageLabels, 0, wxGenericValidator{ &m_checkedItems });
        mainSizer->Add(m_pageList, wxSizerFlags{ 1 }.Expand().Border());

        auto* selectSizer = new wxBoxSizer(wxHORIZONTAL);
        auto* selectAllButton = new wxButton(this, wxID_ANY, _(L"Select All"));
        auto* selectNoneButton = new wxButton(this, wxID_ANY, _(L"Select None"));
        selectSizer->Add(selectAllButton, wxSizerFlags{}.Border(wxRIGHT));
        selectSizer->Add(selectNoneButton);
        mainSizer->Add(selectSizer,
                       wxSizerFlags{}.Align(wxALIGN_LEFT).Border(wxLEFT | wxRIGHT | wxBOTTOM));

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        selectAllButton->Bind(wxEVT_BUTTON,
                              [this]([[maybe_unused]] wxCommandEvent&) { CheckAll(true); });
        selectNoneButton->Bind(wxEVT_BUTTON,
                               [this]([[maybe_unused]] wxCommandEvent&) { CheckAll(false); });
        m_pageList->Bind(wxEVT_CHECKLISTBOX,
                         [this]([[maybe_unused]] wxCommandEvent&) { UpdateOkButton(); });

        TransferDataToWindow();
        UpdateOkButton();
        }

    //----------------------------------------------------------
    void PageSelectionDlg::CheckAll(const bool check)
        {
        for (unsigned int idx = 0; idx < m_pageList->GetCount(); ++idx)
            {
            m_pageList->Check(idx, check);
            }
        UpdateOkButton();
        }

    //----------------------------------------------------------
    void PageSelectionDlg::UpdateOkButton()
        {
        bool anyChecked{ false };
        for (unsigned int idx = 0; idx < m_pageList->GetCount(); ++idx)
            {
            if (m_pageList->IsChecked(idx))
                {
                anyChecked = true;
                break;
                }
            }
        if (auto* okButton = FindWindow(wxID_OK); okButton != nullptr)
            {
            okButton->Enable(anyChecked);
            }
        }

    //----------------------------------------------------------
    std::vector<wxString> PageSelectionDlg::GetExcludedGuids() const
        {
        std::vector<wxString> excludedGuids;
        for (size_t idx = 0; idx < m_pageGuids.size(); ++idx)
            {
            if (m_checkedItems.Index(static_cast<int>(idx)) == wxNOT_FOUND)
                {
                excludedGuids.push_back(m_pageGuids[idx]);
                }
            }
        return excludedGuids;
        }
    } // namespace Wisteria::UI
