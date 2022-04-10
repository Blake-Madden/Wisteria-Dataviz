///////////////////////////////////////////////////////////////////////////////
// Name:        variableselectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "variableselectdlg.h"

using namespace Wisteria;
using namespace Wisteria::UI;

VariableSelectDlg::VariableSelectDlg(wxWindow* parent, const Data::Dataset::ColumnPreviewInfo& columnInfo,
                       VariableSelections varTypes,
                       SingleSelectionTypes singleSelTypes /*= SingleSelectionTypes::NoSingleSelection*/,
                       wxWindowID id /*= wxID_ANY*/,
                       const wxString& caption /*= _("Set Opacity")*/,
                       const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
                       long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/) :
                       m_columnInfo(columnInfo)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls(varTypes, singleSelTypes);

    // when items are selected in any list, enable the buttons
    Bind(wxEVT_LIST_ITEM_SELECTED,
        [this](wxCommandEvent&)
            {
            UpdateButtonStates();
            },
        wxID_ANY);
    Bind(wxEVT_LIST_ITEM_DESELECTED,
        [this](wxCommandEvent&)
        {
            UpdateButtonStates();
        },
        wxID_ANY);

    // Add and Remove buttons
    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_varList, m_xVarList);
            UpdateButtonStates();
            },
        ID_X_VAR_ADD);
    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_xVarList, m_varList);
            UpdateButtonStates();
            },
        ID_X_VAR_REMOVE);

    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_varList, m_yVarList);
            UpdateButtonStates();
            },
        ID_Y_VAR_ADD);
    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_yVarList, m_varList);
            UpdateButtonStates();
            },
        ID_Y_VAR_REMOVE);

    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_varList, m_categoricalVarList);
            UpdateButtonStates();
            },
        ID_CAT_VARS_ADD);
    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_categoricalVarList, m_varList);
            UpdateButtonStates();
            },
        ID_CAT_VARS_REMOVE);

    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_varList, m_groupVarList);
            UpdateButtonStates();
            },
        ID_GROUP_VAR_ADD);
    Bind(wxEVT_BUTTON,
        [this](wxCommandEvent&)
            {
            MoveSelectedVariables(m_groupVarList, m_varList);
            UpdateButtonStates();
            },
        ID_GROUP_VAR_REMOVE);

    Centre();
    }

//-------------------------------------------------------------
void VariableSelectDlg::UpdateButtonStates()
    {
    // X buttons
    auto* button = FindWindowById(ID_X_VAR_REMOVE);
    if (button)
        { button->Enable(m_xVarList->GetSelectedItemCount()); }
    button = FindWindowById(ID_X_VAR_ADD);
    if (button)
        { button->Enable(m_varList->GetSelectedItemCount()); }

    // Y buttons
    button = FindWindowById(ID_Y_VAR_REMOVE);
    if (button)
        { button->Enable(m_yVarList->GetSelectedItemCount()); }
    button = FindWindowById(ID_Y_VAR_ADD);
    if (button)
        { button->Enable(m_varList->GetSelectedItemCount()); }

    // categorical buttons
    button = FindWindowById(ID_CAT_VARS_REMOVE);
    if (button)
        { button->Enable(m_categoricalVarList->GetSelectedItemCount()); }
    button = FindWindowById(ID_CAT_VARS_ADD);
    if (button)
        { button->Enable(m_varList->GetSelectedItemCount()); }

    // group buttons
    button = FindWindowById(ID_GROUP_VAR_REMOVE);
    if (button)
        { button->Enable(m_groupVarList->GetSelectedItemCount()); }
    button = FindWindowById(ID_GROUP_VAR_ADD);
    if (button)
        { button->Enable(m_varList->GetSelectedItemCount()); }
    }

//-------------------------------------------------------------
void VariableSelectDlg::MoveSelectedVariables(wxListView* list, wxListView* otherList)
    {
    wxASSERT_MSG(list, "Invalid list control!");
    wxASSERT_MSG(otherList, "Invalid list control!");
    if (list == nullptr || otherList == nullptr)
        { return; }
    // if target list only supports having one variable, then ensure that it won't
    // have more than one after moving
    if (otherList->HasFlag(wxLC_SINGLE_SEL))
        {
        if (otherList->GetItemCount() ||
            list->GetSelectedItemCount() > 1)
            {
            wxMessageBox(_(L"Only one variable is allowed in this list."),
                         _("Invalid Variable Selection"),
                         wxOK|wxICON_WARNING|wxCENTRE);
            return;
            }
        }
    const auto selStrings = GetSelectedVariables(list);
    for (const auto& str : selStrings)
        { otherList->InsertItem(otherList->GetItemCount(), str); }
    otherList->SetColumnWidth(0, wxLIST_AUTOSIZE);
    RemoveSelectedVariables(list);
    }

//-------------------------------------------------------------
std::vector<wxString> VariableSelectDlg::GetSelectedVariables(wxListView* list)
    {
    std::vector<wxString> selStrings;
    long item{ wxNOT_FOUND };
    for (;;)
        {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == wxNOT_FOUND)
            { break; }
        selStrings.emplace_back(list->GetItemText(item));
        }
    return selStrings;
    };

//-------------------------------------------------------------
void VariableSelectDlg::RemoveSelectedVariables(wxListView* list)
    {
    long item{ wxNOT_FOUND };
    for (;;)
        {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == wxNOT_FOUND)
            { break; }
        list->DeleteItem(item--);
        }
    }

//-------------------------------------------------------------
std::vector<wxString> VariableSelectDlg::GetCategoricalVariables() const
    {
    std::vector<wxString> strings;
    strings.reserve(m_categoricalVarList->GetItemCount());
    for (auto i = 0; i < m_categoricalVarList->GetItemCount(); ++i)
        { strings.emplace_back(m_categoricalVarList->GetItemText(i)); }
    return strings;
    };

//-------------------------------------------------------------
void VariableSelectDlg::CreateControls(VariableSelections varTypes, SingleSelectionTypes singleSelTypes)
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxGridBagSizer* varsSizer = new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(),
                                                   wxSizerFlags::GetDefaultBorder());
    mainSizer->Add(varsSizer,
        wxSizerFlags(1).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    // fill the main list of variables
    varsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Variables")),
        wxGBPosition(0, 0), wxGBSpan(1, 1), wxEXPAND|wxALL);
    m_varList = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                               wxLC_REPORT|wxLC_NO_HEADER);
    m_varList->InsertColumn(0, wxEmptyString);
    for (const auto& [name, type] : m_columnInfo)
        { m_varList->InsertItem(m_varList->GetItemCount(), name); }
    m_varList->SetColumnWidth(0, wxLIST_AUTOSIZE);
    varsSizer->Add(m_varList, wxGBPosition(1, 0), wxGBSpan(3, 1), wxEXPAND|wxALL);

    // set up the variable groups on the right side
    int currentButtonRow{ 1 };
    int currentLabelRow{ 0 };
    int currentListRow{ 1 };
    const auto addVarControls = [&](const auto labelId, const auto addId, const auto removeId,
                                    const wxString& label, const long listStyle)
        {
        auto buttonSz = new wxBoxSizer(wxVERTICAL);
        auto varButtonAdd = new wxButton(this, addId);
        varButtonAdd->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_FORWARD));
        buttonSz->Add(varButtonAdd);
        auto varButtonRemove = new wxButton(this, removeId);
        varButtonRemove->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_BACK));
        buttonSz->Add(varButtonRemove);
        varsSizer->Add(buttonSz,
            wxGBPosition(currentButtonRow, 1), wxGBSpan(1, 1), wxALIGN_CENTER_VERTICAL);
        if (!(listStyle & wxLC_SINGLE_SEL))
            { varsSizer->AddGrowableRow(currentButtonRow); }
        currentButtonRow += 2;

        varsSizer->Add(new wxStaticText(this, labelId, label),
            wxGBPosition(currentLabelRow, 2), wxGBSpan(1, 1), wxEXPAND|wxALL);
        currentLabelRow += 2;

        auto list = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, listStyle);
        list->InsertColumn(0, wxEmptyString);
        varsSizer->Add(list, wxGBPosition(currentListRow, 2), wxGBSpan(1, 1), wxEXPAND|wxALL);
        currentListRow += 2;

        return list;
        };

    // X
    if (varTypes & XVariable)
        {
        m_xVarList =
            addVarControls(ID_X_VAR_LABEL, ID_X_VAR_ADD, ID_X_VAR_REMOVE, _(L"X Variable:"),
                wxLC_REPORT|wxLC_NO_HEADER|wxLC_SINGLE_SEL);
        }
    // Y
    if (varTypes & YVariable)
        {
        m_yVarList =
            addVarControls(ID_Y_VAR_LABEL, ID_Y_VAR_ADD, ID_Y_VAR_REMOVE, _(L"Y Variable:"),
                wxLC_REPORT|wxLC_NO_HEADER|wxLC_SINGLE_SEL);
        }
    // categoricals
    if (varTypes & CategoricalVariables)
        {
        const long style = (singleSelTypes & SingleSelectionTypes::Categorical) ?
            (wxLC_REPORT|wxLC_NO_HEADER|wxLC_SINGLE_SEL) : (wxLC_REPORT|wxLC_NO_HEADER);
        m_categoricalVarList =
            addVarControls(ID_CAT_VARS_LABEL, ID_CAT_VARS_ADD, ID_CAT_VARS_REMOVE,
                _(L"Categorical Variables:"), style);
        }
    // groupings
    if (varTypes & GroupingVariables)
        {
        const long style = (singleSelTypes & SingleSelectionTypes::Grouping) ?
            (wxLC_REPORT|wxLC_NO_HEADER|wxLC_SINGLE_SEL) : (wxLC_REPORT|wxLC_NO_HEADER);
        m_groupVarList =
            addVarControls(ID_GROUP_VAR_LABEL, ID_GROUP_VAR_ADD, ID_GROUP_VAR_REMOVE,
                _("Grouping Variables:"), style);
        }

    // make list columns growable, but not button columns
    varsSizer->AddGrowableCol(0);
    varsSizer->AddGrowableCol(2);

    UpdateButtonStates();

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK|wxCANCEL),
        wxSizerFlags(0).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    SetSizerAndFit(mainSizer);
    }
