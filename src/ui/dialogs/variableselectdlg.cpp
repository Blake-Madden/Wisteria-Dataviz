///////////////////////////////////////////////////////////////////////////////
// Name:        variableselectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "variableselectdlg.h"

using namespace Wisteria;
using namespace Wisteria::UI;

VariableSelectDlg::VariableSelectDlg(
    wxWindow* parent, const Data::Dataset::ColumnPreviewInfo& columnInfo,
    const std::vector<VariableListInfo>& varInfo, wxWindowID id /*= wxID_ANY*/,
    const wxString& caption /*= _(L"Set Opacity")*/, const wxPoint& pos /*= wxDefaultPosition*/,
    const wxSize& size /*= wxDefaultSize*/, long style /*= wxDEFAULT_DIALOG_STYLE|wxCLIP_CHILDREN*/)
    : m_columnInfo(columnInfo)
    {
    SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);

    CreateControls(varInfo);

    Centre();
    }

//-------------------------------------------------------------
void VariableSelectDlg::UpdateButtonStates()
    {
    // enable/disable the Add/Remove
    for (const auto& varList : m_varLists)
        {
        assert(m_mainVarlist && L"Main variable list not created!");
        auto* button = FindWindowById(varList.m_removeId);
        if (button && m_mainVarlist)
            {
            button->Enable(varList.m_list->GetSelectedItemCount());
            }
        button = FindWindowById(varList.m_addId);
        if (button && m_mainVarlist)
            {
            button->Enable(m_mainVarlist->GetSelectedItemCount());
            }
        }
    }

//-------------------------------------------------------------
void VariableSelectDlg::MoveSelectedVariablesBetweenLists(wxListView* list, wxListView* otherList)
    {
    assert(list && "Invalid list control!");
    assert(otherList && "Invalid list control!");
    if (list == nullptr || otherList == nullptr)
        {
        return;
        }
    // if target list only supports having one variable, then ensure that it won't
    // have more than one after moving
    if (otherList->HasFlag(wxLC_SINGLE_SEL))
        {
        if (otherList->GetItemCount() || list->GetSelectedItemCount() > 1)
            {
            wxMessageBox(_(L"Only one variable is allowed in this list."),
                         _(L"Invalid Variable Selection"), wxOK | wxICON_WARNING | wxCENTRE);
            return;
            }
        }
    const auto selStrings = GetSelectedVariablesInList(list);
    wxWindowUpdateLocker noUpdates(otherList);
    for (const auto& str : selStrings)
        {
        otherList->InsertItem(otherList->GetItemCount(), str);
        }
    otherList->SetColumnWidth(0, wxLIST_AUTOSIZE);
    RemoveSelectedVariablesFromList(list);
    }

//-------------------------------------------------------------
std::vector<wxString> VariableSelectDlg::GetSelectedVariablesInList(wxListView* list)
    {
    std::vector<wxString> selStrings;
    long item{ wxNOT_FOUND };
    for (;;)
        {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == wxNOT_FOUND)
            {
            break;
            }
        selStrings.emplace_back(list->GetItemText(item));
        }
    return selStrings;
    }

//-------------------------------------------------------------
void VariableSelectDlg::RemoveSelectedVariablesFromList(wxListView* list)
    {
    wxWindowUpdateLocker noUpdates(list);
    long item{ wxNOT_FOUND };
    for (;;)
        {
        item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        if (item == wxNOT_FOUND)
            {
            break;
            }
        list->DeleteItem(item--);
        }
    }

//-------------------------------------------------------------
std::vector<wxString> VariableSelectDlg::GetSelectedVariables(const size_t listIndex) const
    {
    if (listIndex >= m_varLists.size())
        {
        wxFAIL_MSG(L"Invalid index specified for variable list!");
        return std::vector<wxString>();
        }
    const auto& varList = m_varLists[listIndex];
    std::vector<wxString> strings;
    strings.reserve(varList.m_list->GetItemCount());
    for (auto i = 0; i < varList.m_list->GetItemCount(); ++i)
        {
        strings.emplace_back(varList.m_list->GetItemText(i));
        }
    return strings;
    }

//-------------------------------------------------------------
void VariableSelectDlg::CreateControls(const std::vector<VariableListInfo>& varInfo)
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    wxGridBagSizer* varsSizer =
        new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder());
    mainSizer->Add(varsSizer,
                   wxSizerFlags(1).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    // fill the main list of variables
    varsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Variables")), wxGBPosition(0, 0),
                   wxGBSpan(1, 1), wxEXPAND | wxALL);
    m_mainVarlist = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                   wxLC_REPORT | wxLC_NO_HEADER);
    m_mainVarlist->InsertColumn(0, wxEmptyString);
    for (const auto& [name, type] : m_columnInfo)
        {
        m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name);
        }
    m_mainVarlist->SetColumnWidth(0, wxLIST_AUTOSIZE);
    varsSizer->Add(m_mainVarlist, wxGBPosition(1, 0), wxGBSpan(3, 1), wxEXPAND | wxALL);

    // set up the variable groups on the right side
    int currentButtonRow{ 1 };
    int currentLabelRow{ 0 };
    int currentListRow{ 1 };
    const auto addVarControls =
        [&](const auto addId, const auto removeId, const wxString& label, const long listStyle)
    {
        auto buttonSz = new wxBoxSizer(wxVERTICAL);
        auto varButtonAdd = new wxButton(this, addId);
        varButtonAdd->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_FORWARD));
        buttonSz->Add(varButtonAdd);
        auto varButtonRemove = new wxButton(this, removeId);
        varButtonRemove->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_BACK));
        buttonSz->Add(varButtonRemove);
        varsSizer->Add(buttonSz, wxGBPosition(currentButtonRow, 1), wxGBSpan(1, 1),
                       wxALIGN_CENTER_VERTICAL);
        if (!(listStyle & wxLC_SINGLE_SEL))
            {
            varsSizer->AddGrowableRow(currentButtonRow);
            }
        currentButtonRow += 2;

        varsSizer->Add(new wxStaticText(this, wxID_STATIC, label), wxGBPosition(currentLabelRow, 2),
                       wxGBSpan(1, 1), wxEXPAND | wxALL);
        currentLabelRow += 2;

        auto list = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, listStyle);
        list->InsertColumn(0, wxEmptyString);
        varsSizer->Add(list, wxGBPosition(currentListRow, 2), wxGBSpan(1, 1), wxEXPAND | wxALL);
        currentListRow += 2;

        return list;
    };

    int currentId{ wxID_HIGHEST };

    // create the user-defined variable lists on the right
    for (const auto& var : varInfo)
        {
        VariableList currentList;
        currentList.m_label = var.m_label;
        currentList.m_addId = ++currentId;
        currentList.m_removeId = ++currentId;
        currentList.m_required = var.m_required;
        currentList.m_singleSelection = var.m_singleSelection;

        const long style = currentList.m_singleSelection ?
                               (wxLC_REPORT | wxLC_NO_HEADER | wxLC_SINGLE_SEL) :
                               (wxLC_REPORT | wxLC_NO_HEADER);
        currentList.m_list = addVarControls(
            currentList.m_addId, currentList.m_removeId,
            (currentList.m_required ? currentList.m_label :
                                      wxString::Format(_(L"%s (optional)"), currentList.m_label)),
            style);
        m_varLists.push_back(std::move(currentList));
        }

    // make list columns growable, but not button columns
    varsSizer->AddGrowableCol(0);
    varsSizer->AddGrowableCol(2);

    mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                   wxSizerFlags(0).Expand().Border(wxALL, wxSizerFlags::GetDefaultBorder()));

    SetSizerAndFit(mainSizer);

    // when items are selected in any list, enable the buttons
    Bind(
        wxEVT_LIST_ITEM_SELECTED, [this](wxCommandEvent&) { UpdateButtonStates(); }, wxID_ANY);
    Bind(
        wxEVT_LIST_ITEM_DESELECTED, [this](wxCommandEvent&) { UpdateButtonStates(); }, wxID_ANY);

    // connect the Add & Remove button events
    for (const auto& varList : m_varLists)
        {
        assert(varList.m_list && L"User-defined list not created!");
        Bind(
            wxEVT_BUTTON,
            [&, this](wxCommandEvent&)
            {
                MoveSelectedVariablesBetweenLists(m_mainVarlist, varList.m_list);
                UpdateButtonStates();
            },
            varList.m_addId);
        Bind(
            wxEVT_BUTTON,
            [&, this](wxCommandEvent&)
            {
                MoveSelectedVariablesBetweenLists(varList.m_list, m_mainVarlist);
                UpdateButtonStates();
            },
            varList.m_removeId);
        }

    UpdateButtonStates();
    }

//-------------------------------------------------------------
bool VariableSelectDlg::Validate()
    {
    // make sure any variable lists set to required have something selected
    for (const auto& varList : m_varLists)
        {
        if (varList.m_required && varList.m_list && varList.m_list->GetItemCount() == 0)
            {
            wxMessageBox(wxString::Format(_(L"Variable(s) must be selected for the '%s' list."),
                                          varList.m_label),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING | wxCENTRE);
            return false;
            }
        }
    return true;
    }
