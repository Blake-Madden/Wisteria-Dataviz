///////////////////////////////////////////////////////////////////////////////
// Name:        warningmessagesdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "warningmessagesdlg.h"
#include "../../util/warningmanager.h"
#include <map>

using namespace Wisteria::UI;

//---------------------------------------------------
void WarningMessagesDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

    wxStaticBoxSizer* checksBoxSizer = new wxStaticBoxSizer(
        new wxStaticBox(this, wxID_ANY, _(L"Display the following prompts:")), wxVERTICAL);

    // sort the warnings by display label
    std::map<wxString, std::vector<WarningMessage>::iterator> warningsSortedByLabel;
    for (auto warningIter = WarningManager::GetWarnings().begin();
         warningIter != WarningManager::GetWarnings().end(); ++warningIter)
        {
        warningsSortedByLabel.insert(std::make_pair(warningIter->GetDescription(), warningIter));
        }
    for (auto& warningLabel : warningsSortedByLabel)
        {
        wxCheckBox* checkBox =
            new wxCheckBox(checksBoxSizer->GetStaticBox(), wxID_ANY,
                           warningLabel.second->GetDescription(), wxDefaultPosition, wxDefaultSize,
                           0, wxGenericValidator(&warningLabel.second->ShouldBeShown()));
        checksBoxSizer->Add(checkBox, 0, wxALL, wxSizerFlags::GetDefaultBorder());
        }

    mainSizer->Add(checksBoxSizer, 1, wxEXPAND | wxALL, wxSizerFlags::GetDefaultBorder());

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP), 0, wxEXPAND | wxALL,
                   wxSizerFlags::GetDefaultBorder());

    SetSizer(mainSizer);
    mainSizer->Fit(this);
    }
