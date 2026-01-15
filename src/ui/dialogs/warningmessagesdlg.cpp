///////////////////////////////////////////////////////////////////////////////
// Name:        warningmessagesdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "warningmessagesdlg.h"
#include "../../util/warningmanager.h"
#include <map>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //---------------------------------------------------
    void WarningMessagesDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        auto* checksBoxSizer = new wxStaticBoxSizer(
            new wxStaticBox(this, wxID_ANY, _(L"Display the following prompts:")), wxVERTICAL);

        // sort the warnings by display label
        std::map<wxString, std::vector<WarningMessage>::iterator> warningsSortedByLabel;
        for (auto warningIter = WarningManager::GetWarnings().begin();
             warningIter != WarningManager::GetWarnings().end(); ++warningIter)
            {
            warningsSortedByLabel.insert(
                std::make_pair(warningIter->GetDescription(), warningIter));
            }
        for (const auto& warningLabel : warningsSortedByLabel)
            {
            auto* checkBox = new wxCheckBox(
                checksBoxSizer->GetStaticBox(), wxID_ANY, warningLabel.second->GetDescription(),
                wxDefaultPosition, wxDefaultSize, 0,
                wxGenericValidator(&warningLabel.second->ShouldBeShown()));
            checksBoxSizer->Add(checkBox, wxSizerFlags{}.Border());
            }

        mainSizer->Add(checksBoxSizer, wxSizerFlags{ 1 }.Expand().Border());

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                       wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);
        mainSizer->Fit(this);

        TransferDataToWindow();
        }
    } // namespace Wisteria::UI
