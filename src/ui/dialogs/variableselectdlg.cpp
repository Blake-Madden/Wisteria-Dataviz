///////////////////////////////////////////////////////////////////////////////
// Name:        variableselectdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "variableselectdlg.h"
#include <set>
#include <tuple>
#include <wx/artprov.h>
#include <wx/gbsizer.h>
#include <wx/statbmp.h>
#include <wx/stattext.h>
#include <wx/wupdlock.h>

namespace Wisteria::UI
    {
    VariableSelectDlg::VariableSelectDlg(wxWindow* parent,
                                         Data::Dataset::ColumnPreviewInfo columnInfo,
                                         const std::vector<VariableListInfo>& varInfo,
                                         wxWindowID id /*= wxID_ANY*/,
                                         const wxString& caption /*= _(L"Select Variables")*/,
                                         const wxPoint& pos /*= wxDefaultPosition*/,
                                         const wxSize& size /*= wxDefaultSize*/,
                                         long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN*/)
        : m_columnInfo(std::move(columnInfo))
        {
        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style);

        CreateControls(varInfo);

        Centre();

        // make the variable columns the width of their boxes
        Bind(wxEVT_SIZE,
             [this](wxSizeEvent& evt)
             {
                 m_mainVarlist->SetColumnWidth(0, m_mainVarlist->GetSize().GetWidth());
                 for (auto& varList : m_varLists)
                     {
                     varList.m_list->SetColumnWidth(0, varList.m_list->GetSize().GetWidth());
                     }
                 evt.Skip();
             });
        }

    //-------------------------------------------------------------
    void VariableSelectDlg::UpdateButtonStates() const
        {
        // enable/disable the Add/Remove buttons
        for (const auto& varList : m_varLists)
            {
            wxASSERT_MSG(m_mainVarlist, L"Main variable list not created!");
            auto* button = FindWindowById(varList.m_removeId, this);
            if (button != nullptr && m_mainVarlist != nullptr)
                {
                button->Enable(varList.m_list->GetSelectedItemCount() > 0);
                }
            button = FindWindowById(varList.m_addId, this);
            if (button != nullptr && m_mainVarlist != nullptr)
                {
                button->Enable(m_mainVarlist->GetSelectedItemCount() > 0);
                }
            }
        }

    //-------------------------------------------------------------
    void VariableSelectDlg::MoveSelectedVariablesBetweenLists(wxListView* list,
                                                              wxListView* otherList)
        {
        wxASSERT_MSG(list, L"Invalid list control!");
        wxASSERT_MSG(otherList, L"Invalid list control!");
        if (list == nullptr || otherList == nullptr)
            {
            return;
            }
        // if target list only supports having one variable, then ensure that it won't
        // have more than one after moving
        if (otherList->HasFlag(wxLC_SINGLE_SEL))
            {
            if (otherList->GetItemCount() != 0 || list->GetSelectedItemCount() > 1)
                {
                wxMessageBox(_(L"Only one variable is allowed in this list."),
                             _(L"Invalid Variable Selection"), wxOK | wxICON_WARNING | wxCENTRE);
                return;
                }
            }
        auto selStrings = GetSelectedVariablesInList(list);
        const wxWindowUpdateLocker noUpdates(otherList);
        // de-select items in the target list, and then select the item(s) being moved into it
        for (int i = 0; i < otherList->GetItemCount(); ++i)
            {
            otherList->Select(i, false);
            }
        for (auto& str : selStrings)
            {
            str.SetId(otherList->GetItemCount());
            otherList->Select(otherList->InsertItem(str));
            }

        RemoveSelectedVariablesFromList(list);
        }

    //-------------------------------------------------------------
    std::vector<wxListItem> VariableSelectDlg::GetSelectedVariablesInList(const wxListView* list)
        {
        std::vector<wxListItem> selStrings;
        long item{ wxNOT_FOUND };
        for (;;)
            {
            item = list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (item == wxNOT_FOUND)
                {
                break;
                }
            wxListItem listItem;
            listItem.SetId(item);
            listItem.SetColumn(0);
            listItem.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
            if (list->GetItem(listItem))
                {
                selStrings.push_back(listItem);
                }
            }
        return selStrings;
        }

    //-------------------------------------------------------------
    void VariableSelectDlg::RemoveSelectedVariablesFromList(wxListView* list)
        {
        const wxWindowUpdateLocker noUpdates(list);
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
        // select top remaining item
        if (list->GetItemCount() > 0)
            {
            list->Select(0);
            }
        }

    //-------------------------------------------------------------
    std::vector<wxString> VariableSelectDlg::GetSelectedVariables(const size_t listIndex) const
        {
        if (listIndex >= m_varLists.size())
            {
            wxFAIL_MSG(L"Invalid index specified for variable list!");
            return {};
            }
        const auto& varList = m_varLists[listIndex];
        std::vector<wxString> strings;
        strings.reserve(varList.m_list->GetItemCount());
        for (auto i = 0; i < varList.m_list->GetItemCount(); ++i)
            {
            strings.push_back(varList.m_list->GetItemText(i));
            }
        return strings;
        }

    //-------------------------------------------------------------
    void VariableSelectDlg::CreateControls(const std::vector<VariableListInfo>& varInfo)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* varsSizer =
            new wxGridBagSizer(wxSizerFlags::GetDefaultBorder(), wxSizerFlags::GetDefaultBorder());
        mainSizer->Add(varsSizer, wxSizerFlags{ 1 }.Expand().Border());

        // split var groups into columns of 5
        constexpr size_t MAXGROUPSPERCOLUMN{ 5 };
        const auto numColumnSets =
            safe_divide(varInfo.size() + MAXGROUPSPERCOLUMN - 1, MAXGROUPSPERCOLUMN);
        const size_t rowsNeeded = std::min(varInfo.size(), MAXGROUPSPERCOLUMN) * 2;

        // fill the main list of variables
        varsSizer->Add(new wxStaticText(this, wxID_ANY, _(L"Variables:")), wxGBPosition{ 0, 0 },
                       wxGBSpan{ 1, 1 });
        m_mainVarlist = new wxListView(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxLC_REPORT | wxLC_NO_HEADER);
        m_mainVarlist->InsertColumn(0, wxString{});
        m_listImages = new wxImageList(FromDIP(16), FromDIP(16));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_CONTINUOUS")
                              .GetBitmap(wxSize{ FromDIP(16), FromDIP(16) }));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_CATEGORICAL")
                              .GetBitmap(wxSize{ FromDIP(16), FromDIP(16) }));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_DISCRETE")
                              .GetBitmap(wxSize{ FromDIP(16), FromDIP(16) }));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_DATE").GetBitmap(
            wxSize{ FromDIP(16), FromDIP(16) }));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_DICHOTOMOUS_CATEGORICAL")
                              .GetBitmap(wxSize{ FromDIP(16), FromDIP(16) }));
        m_listImages->Add(wxArtProvider::GetBitmapBundle("ID_DICHOTOMOUS_DISCRETE")
                              .GetBitmap(wxSize{ FromDIP(16), FromDIP(16) }));
        m_mainVarlist->SetImageList(m_listImages, wxIMAGE_LIST_SMALL);
        for (const auto& [name, type, currencySymbol, excluded, userOverridden] : m_columnInfo)
            {
            if (excluded)
                {
                continue;
                }
            if (type == Data::Dataset::ColumnImportType::Numeric)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 0);
                }
            else if (type == Data::Dataset::ColumnImportType::String)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 1);
                }
            else if (type == Data::Dataset::ColumnImportType::Discrete)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 2);
                }
            else if (type == Data::Dataset::ColumnImportType::Date)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 3);
                }
            else if (type == Data::Dataset::ColumnImportType::DichotomousString)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 4);
                }
            else if (type == Data::Dataset::ColumnImportType::DichotomousDiscrete)
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name, 5);
                }
            else
                {
                m_mainVarlist->InsertItem(m_mainVarlist->GetItemCount(), name);
                }
            }
        // quneiform-suppress-begin
        varsSizer->Add(m_mainVarlist, wxGBPosition{ 1, 0 }, wxGBSpan(rowsNeeded, 1), wxEXPAND);
        // quneiform-suppress-end

        // set up the variable groups on the right side
        const auto addVarControls =
            [&](const auto addId, const auto removeId, const wxString& label,
                const std::vector<Data::Dataset::ColumnImportType> acceptedVarTypes, long listStyle,
                const int buttonCol, const int listCol, const int buttonRow, const int labelRow,
                const int listRow)
        {
            auto* buttonSz = new wxBoxSizer(wxVERTICAL);
            auto* varButtonAdd = new wxButton(this, addId);
            varButtonAdd->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_FORWARD));
            buttonSz->Add(varButtonAdd);
            auto* varButtonRemove = new wxButton(this, removeId);
            varButtonRemove->SetBitmap(wxArtProvider::GetBitmapBundle(wxART_GO_BACK));
            buttonSz->Add(varButtonRemove);
            varsSizer->Add(buttonSz, wxGBPosition{ buttonRow, buttonCol }, wxGBSpan{ 1, 1 },
                           wxALIGN_CENTER_VERTICAL);

            const auto iconSize = wxSize{ FromDIP(16), FromDIP(16) };
            auto* varLabelSz = new wxBoxSizer(wxHORIZONTAL);
            varLabelSz->Add(new wxStaticText(this, wxID_STATIC, label));
            varLabelSz->AddStretchSpacer();
            for (const auto& acceptedType : acceptedVarTypes)
                {
                auto* icon = new wxStaticBitmap(
                    this, wxID_ANY,
                    wxArtProvider::GetBitmapBundle(
                        acceptedType == Data::Dataset::ColumnImportType::Numeric ?
                            "ID_CONTINUOUS" :
                        acceptedType == Data::Dataset::ColumnImportType::Discrete ?
                            "ID_DISCRETE" :
                        acceptedType == Data::Dataset::ColumnImportType::String ?
                            "ID_CATEGORICAL" :
                        acceptedType == Data::Dataset::ColumnImportType::Date ?
                            "ID_DATE" :
                        acceptedType == Data::Dataset::ColumnImportType::DichotomousString ?
                            "ID_DICHOTOMOUS_CATEGORICAL" :
                        acceptedType == Data::Dataset::ColumnImportType::DichotomousDiscrete ?
                            "ID_DICHOTOMOUS_DISCRETE" :
                            "ID_CONTINUOUS")
                        .GetBitmap(iconSize));
                varLabelSz->Add(icon, wxSizerFlags{}.CenterVertical());
                }
            // quneiform-suppress-begin
            varsSizer->Add(varLabelSz, wxGBPosition{ labelRow, listCol }, wxGBSpan{ 1, 1 },
                           wxEXPAND);

            auto* list = new wxListView(this, wxID_ANY, wxDefaultPosition,
                                        wxSize{ -1, FromDIP(75) }, listStyle);
            list->InsertColumn(0, wxString{});
            list->SetImageList(m_listImages, wxIMAGE_LIST_SMALL);
            varsSizer->Add(list, wxGBPosition{ listRow, listCol }, wxGBSpan{ 1, 1 }, wxEXPAND);
            // quneiform-suppress-end

            return list;
        };

        int currentId{ wxID_HIGHEST };

        // create the user-defined variable lists on the right
        for (size_t i = 0; i < varInfo.size(); ++i)
            {
            const auto& var = varInfo[i];
            const size_t columnSet = safe_divide(i, MAXGROUPSPERCOLUMN);
            const size_t rowWithinSet = safe_modulus(i, MAXGROUPSPERCOLUMN);

            const int buttonCol = 1 + (static_cast<int>(columnSet) * 2);
            const int listCol = 2 + (static_cast<int>(columnSet) * 2);
            const int labelRow = static_cast<int>(rowWithinSet) * 2;
            const int listRow = labelRow + 1;
            const int buttonRow = listRow;

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
                (currentList.m_required ?
                     currentList.m_label + L":" :
                     wxString::Format(_(L"%s (optional):"), currentList.m_label)),
                var.m_acceptedTypes, style, buttonCol, listCol, buttonRow, labelRow, listRow);
            m_varLists.push_back(std::move(currentList));
            }

        // make list columns growable horizontally, but not button columns
        varsSizer->AddGrowableCol(0);
        for (size_t cs = 0; cs < numColumnSets; ++cs)
            {
            varsSizer->AddGrowableCol(2 + cs * 2);
            }

        // make the list rows growable vertically
        for (size_t i = 1; i < rowsNeeded; i += 2)
            {
            varsSizer->AddGrowableRow(i);
            }

            // build a legend showing only the variable types actually in the main list
            {
            std::set<Data::Dataset::ColumnImportType> usedTypes;
            for (const auto& [name, type, currencySymbol, excluded, userOverridden] : m_columnInfo)
                {
                if (!excluded)
                    {
                    usedTypes.insert(type);
                    }
                }

            const std::tuple<Data::Dataset::ColumnImportType, wxString, wxString> typeLegend[] = {
                { Data::Dataset::ColumnImportType::Numeric, _(L"Continuous"), L"ID_CONTINUOUS" },
                { Data::Dataset::ColumnImportType::String, _(L"Categorical"), L"ID_CATEGORICAL" },
                { Data::Dataset::ColumnImportType::Discrete, _(L"Discrete"), L"ID_DISCRETE" },
                { Data::Dataset::ColumnImportType::Date, _(L"Date"), L"ID_DATE" },
                { Data::Dataset::ColumnImportType::DichotomousString,
                  _(L"Dichotomous (Categorical)"), L"ID_DICHOTOMOUS_CATEGORICAL" },
                { Data::Dataset::ColumnImportType::DichotomousDiscrete,
                  _(L"Dichotomous (Discrete)"), L"ID_DICHOTOMOUS_DISCRETE" }
            };

            const auto iconSize = wxSize{ FromDIP(16), FromDIP(16) };
            auto* legendSizer = new wxFlexGridSizer(2, wxSizerFlags::GetDefaultBorder(),
                                                    wxSizerFlags::GetDefaultBorder() * 2);
            for (const auto& [colType, label, artId] : typeLegend)
                {
                if (usedTypes.find(colType) == usedTypes.end())
                    {
                    continue;
                    }
                auto* icon = new wxStaticBitmap(
                    this, wxID_ANY, wxArtProvider::GetBitmapBundle(artId).GetBitmap(iconSize));
                legendSizer->Add(icon, wxSizerFlags{}.CenterVertical());
                legendSizer->Add(new wxStaticText(this, wxID_ANY, label),
                                 wxSizerFlags{}.CenterVertical());
                }

            if (legendSizer->GetItemCount() > 0)
                {
                mainSizer->Add(legendSizer, wxSizerFlags{}.Border());
                }
            }

        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        // when items are selected in any list, enable the buttons
        Bind(wxEVT_LIST_ITEM_SELECTED, [this](wxCommandEvent&) { UpdateButtonStates(); }, wxID_ANY);
        Bind(
            wxEVT_LIST_ITEM_DESELECTED, [this](wxCommandEvent&) { UpdateButtonStates(); },
            wxID_ANY);

        // connect the Add & Remove button events
        for (const auto& varList : m_varLists)
            {
            wxASSERT_MSG(varList.m_list, L"User-defined list not created!");
            Bind(
                wxEVT_BUTTON,
                [&, this]([[maybe_unused]] wxCommandEvent&)
                {
                    MoveSelectedVariablesBetweenLists(m_mainVarlist, varList.m_list);
                    UpdateButtonStates();
                },
                varList.m_addId);
            Bind(
                wxEVT_BUTTON,
                [&, this]([[maybe_unused]] wxCommandEvent&)
                {
                    MoveSelectedVariablesBetweenLists(varList.m_list, m_mainVarlist);
                    UpdateButtonStates();
                },
                varList.m_removeId);

            // double-clicking var in a list on the right (or hitting DELETE keys)
            // will remove it and send it back to the main list on the left
            varList.m_list->Bind(wxEVT_LEFT_DCLICK,
                                 [&, this]([[maybe_unused]] wxMouseEvent&)
                                 {
                                     MoveSelectedVariablesBetweenLists(varList.m_list,
                                                                       m_mainVarlist);
                                     UpdateButtonStates();
                                 });
            varList.m_list->Bind(
                wxEVT_KEY_DOWN,
                [&, this](wxKeyEvent& evt)
                {
                    if (evt.GetKeyCode() == WXK_DELETE || evt.GetKeyCode() == WXK_NUMPAD_DELETE ||
                        evt.GetKeyCode() == WXK_BACK)
                        {
                        MoveSelectedVariablesBetweenLists(varList.m_list, m_mainVarlist);
                        UpdateButtonStates();
                        }
                });
            }

        // double-clicking var in the main list will move it to the first list on
        // the right that doesn't have anything in it (will do nothing if all have
        // something already)
        m_mainVarlist->Bind(wxEVT_LEFT_DCLICK,
                            [&, this]([[maybe_unused]] wxMouseEvent&)
                            {
                                for (const auto& varList : m_varLists)
                                    {
                                    if (varList.m_list->GetItemCount() == 0)
                                        {
                                        MoveSelectedVariablesBetweenLists(m_mainVarlist,
                                                                          varList.m_list);
                                        UpdateButtonStates();
                                        break;
                                        }
                                    }
                            });

        // pre-populate sub-lists with any default variables,
        // removing them from the main list
        for (size_t i = 0; i < varInfo.size(); ++i)
            {
            const auto& defaults = varInfo[i].m_defaultVariables;
            if (defaults.empty())
                {
                continue;
                }
            auto* targetList = m_varLists[i].m_list;
            for (const auto& varName : defaults)
                {
                const long idx = m_mainVarlist->FindItem(-1, varName);
                if (idx == wxNOT_FOUND)
                    {
                    continue;
                    }
                wxListItem listItem;
                listItem.SetId(idx);
                listItem.SetColumn(0);
                listItem.SetMask(wxLIST_MASK_TEXT | wxLIST_MASK_IMAGE);
                if (m_mainVarlist->GetItem(listItem))
                    {
                    listItem.SetId(targetList->GetItemCount());
                    targetList->InsertItem(listItem);
                    m_mainVarlist->DeleteItem(idx);
                    }
                }
            }

        UpdateButtonStates();

        if (m_mainVarlist->GetItemCount() > 0)
            {
            m_mainVarlist->Select(0);
            }
        }

    //-------------------------------------------------------------
    bool VariableSelectDlg::Validate()
        {
        // make sure any variable lists set to required have something selected
        if (!std::ranges::all_of(m_varLists,
                                 [](const auto& varList)
                                 {
                                     return !varList.m_required || varList.m_list == nullptr ||
                                            varList.m_list->GetItemCount() > 0;
                                 }))
            {
            wxMessageBox(_(L"Variables must be selected for the required lists."),
                         _(L"Variable Not Specified"), wxOK | wxICON_WARNING | wxCENTRE);
            return false;
            }

        return true;
        }
    } // namespace Wisteria::UI
