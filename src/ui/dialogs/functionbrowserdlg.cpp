//////////////////////////////////////////////////////////////////////////////
// Name:        functionbrowserdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "functionbrowserdlg.h"
#include "wx/wupdlock.h"

namespace Wisteria::UI
    {
    //------------------------------------------------
    void FunctionBrowserCtrl::OnHyperlinkClicked(const wxHtmlLinkEvent& event)
        {
        const auto [parentPos, childPos] =
            m_categoryList->FindSubItem(event.GetLinkInfo().GetHref());
        if (parentPos.has_value())
            {
            m_categoryList->SelectSubItem(parentPos.value(), childPos.value());
            }
        // clicked on something that is not a known class name--in this case,
        // insert it into the parent editor.
        else
            {
            if (m_editWindow != nullptr && m_editWindow->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
                {
                auto* styleWindow = dynamic_cast<wxStyledTextCtrl*>(m_editWindow);
                styleWindow->AddText(event.GetLinkInfo().GetHref());
                styleWindow->SetSelection(styleWindow->GetCurrentPos(),
                                          styleWindow->GetCurrentPos());
                }
            }
        }

    //------------------------------------------------
    void FunctionBrowserCtrl::FinalizeCategories()
        {
        // this is used for quick searching when we format function signatures
        m_categoryNames.clear();
        for (const auto& func : m_functionCollection)
            {
            if (func.m_parentId == wxID_ANY)
                {
                m_categoryList->InsertItem(m_categoryList->GetFolderCount(), func.m_name, wxID_ANY,
                                           func.m_iconIndex);
                }
            else
                {
                m_categoryList->InsertSubItemById(func.m_parentId, func.m_name, wxID_ANY,
                                                  func.m_iconIndex);
                }
            m_categoryNames.insert(func.m_name);
            }
        for (size_t i = 0; i < m_categoryList->GetFolderCount(); ++i)
            {
            m_categoryList->GetFolder(i).SortSubItems();
            }
        if (m_categoryList->GetFolderCount())
            {
            m_categoryList->SelectFolder(0);
            }

        m_categoryList->AdjustWidthToFitItems();
        Layout();
        }

    //------------------------------------------------
    void FunctionBrowserCtrl::OnListSelected(wxCommandEvent& event)
        {
        wxWindowUpdateLocker noUpdates(this);
        if (event.GetId() == ID_CATEGORY_LIST)
            {
            auto pos = m_functionCollection.find(CategoryInfo(event.GetString().ToStdWstring()));
            if (pos != m_functionCollection.cend())
                {
                wxArrayString funcNames;
                wxStringTokenizer tkz;
                m_currentFunctionsAndDescriptions.clear();
                wxString currentFunctionSignature;
                wxString currentDescriptionAndRetVal;
                for (const auto& func : pos->m_functions)
                    {
                    if (const auto retPos = func.find(L"->"); retPos != wxString::npos)
                        {
                        currentFunctionSignature = func.substr(0, retPos);
                        funcNames.Add(GetFunctionName(currentFunctionSignature));
                        const wxString retVal = func.substr(retPos + 2);
                        currentDescriptionAndRetVal.clear();
                        if (!retVal.empty())
                            {
                            currentDescriptionAndRetVal =
                                wxString::Format(L"<br />%s<tt><span style='font-weight:bold;'>"
                                                 "<span style=\"color:#00A2E8\"><a "
                                                 "href=\"%s\">%s</a></span></span></tt>.",
                                                 _(L"Returns: "), retVal, retVal);
                            }
                        m_currentFunctionsAndDescriptions.emplace_back(
                            FormatFunctionSignature(currentFunctionSignature),
                            currentDescriptionAndRetVal);
                        }
                    else
                        {
                        tkz.SetString(func, L"\t");
                        // the function signature
                        currentFunctionSignature = tkz.GetNextToken();
                        funcNames.Add(GetFunctionName(currentFunctionSignature));
                        // description
                        currentDescriptionAndRetVal = tkz.GetNextToken();
                        // a special return type
                        if (tkz.HasMoreTokens())
                            {
                            const wxString retVal = tkz.GetNextToken();
                            currentDescriptionAndRetVal +=
                                wxString::Format(L"<br />%s<tt><span style='font-weight:bold;'>"
                                                 "<span style=\"color:#00A2E8\"><a "
                                                 "href=\"%s\">%s</a></span></span></tt>.",
                                                 _(L"Returns: "), retVal, retVal);
                            }
                        m_currentFunctionsAndDescriptions.emplace_back(
                            FormatFunctionSignature(currentFunctionSignature),
                            currentDescriptionAndRetVal);
                        }
                    }
                m_functionList->Clear();
                m_functionList->Append(funcNames);
                }
            if (m_functionList->GetCount())
                {
                m_functionList->SetSelection(
                    (pos->m_lastSelectedItem == -1) ? 0 : pos->m_lastSelectedItem);
                m_functionDescriptionWindow->SetPage(wxString::Format(
                    L"<body bgcolor=%s text=%s>%s<br />%s</body>",
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetAsString(wxC2S_HTML_SYNTAX),
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                        .GetAsString(wxC2S_HTML_SYNTAX),
                    m_currentFunctionsAndDescriptions[m_functionList->GetSelection()].first,
                    m_currentFunctionsAndDescriptions[m_functionList->GetSelection()].second));
                }
            else
                {
                m_functionDescriptionWindow->SetPage(wxEmptyString);
                }
            }
        else if (event.GetId() == ID_FUNCTION_LIST)
            {
            if (m_functionList->GetSelection() < 0 ||
                static_cast<size_t>(m_functionList->GetSelection()) >=
                    m_currentFunctionsAndDescriptions.size())
                {
                m_functionDescriptionWindow->SetPage(wxEmptyString);
                }
            else
                {
                // keep track of the currently selected item in the function list
                auto pos = m_functionCollection.find(
                    CategoryInfo(m_categoryList->GetSelectedLabel().ToStdWstring()));
                if (pos != m_functionCollection.end())
                    {
                    auto nh = m_functionCollection.extract(pos);
                    nh.value().m_lastSelectedItem = m_functionList->GetSelection();
                    m_functionCollection.insert(std::move(nh));
                    }
                // update the description area
                m_functionDescriptionWindow->SetPage(wxString::Format(
                    L"<body bgcolor=%s text=%s>%s<br />%s</body>",
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW).GetAsString(wxC2S_HTML_SYNTAX),
                    wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOWTEXT)
                        .GetAsString(wxC2S_HTML_SYNTAX),
                    m_currentFunctionsAndDescriptions[m_functionList->GetSelection()].first,
                    m_currentFunctionsAndDescriptions[m_functionList->GetSelection()].second));
                }
            }
        }

    //------------------------------------------------
    wxString FunctionBrowserCtrl::FormatFunctionSignature(wxString signature) const
        {
        lily_of_the_valley::html_encode_text encode;
        signature = encode({ signature.wc_str(), signature.length() }, true);

        size_t firstParanPos = signature.find(L'(');
        const size_t lastParanPos = signature.find_last_of(L')');
        if (firstParanPos == wxString::npos || lastParanPos == wxString::npos ||
            lastParanPos == firstParanPos + 1)
            {
            return L"<tt><span style='font-weight:bold;'>" + signature + L"</span></tt>";
            }

        // function name and first parenthesis
        wxString formattedSignature = L"<tt><span style='font-weight:bold;'>";
        formattedSignature += signature.substr(0, ++firstParanPos);
        formattedSignature += L"</span><span style='font-style:italic;'>";
        // Function body. Note that we chop up the params and piece them together
        // using a customizable list separator.
        // We also set params as a hyperlink if a category by the same name is present.
        wxStringTokenizer tkz(signature.substr(firstParanPos, lastParanPos - firstParanPos), L",");
        while (tkz.HasMoreTokens())
            {
            const wxString param = tkz.GetNextToken();
            if (m_categoryNames.contains(param.wc_str()))
                {
                formattedSignature +=
                    wxString::Format(L"<a href=\"%s\">%s</a>%c", param, param, m_paramSeparator);
                }
            else
                {
                formattedSignature += wxString::Format(L"%s%c", param, m_paramSeparator);
                }
            }
        if (formattedSignature.RemoveLast(1) == m_paramSeparator)
            {
            formattedSignature.RemoveLast(1);
            }
        // last parenthesis
        formattedSignature += L"</span><span style='font-weight:bold;'>";
        formattedSignature += signature.substr(lastParanPos);
        formattedSignature += L"</span></tt>";
        return formattedSignature;
        }

    //------------------------------------------------
    void FunctionBrowserCtrl::InsertFunction()
        {
        if (m_functionList->GetSelection() == wxNOT_FOUND ||
            m_functionList->GetSelection() >=
                static_cast<long>(m_currentFunctionsAndDescriptions.size()))
            {
            wxMessageBox(_(L"Please select an item in the function list to insert."),
                         _(L"Invalid Selection"), wxOK | wxICON_INFORMATION);
            return;
            }
        if (m_editWindow != nullptr && m_editWindow->IsKindOf(CLASSINFO(wxStyledTextCtrl)))
            {
            lily_of_the_valley::html_extract_text filter_html;
            wxString functionStr =
                m_currentFunctionsAndDescriptions[m_functionList->GetSelection()].first;
            functionStr = filter_html(functionStr, functionStr.length(), true, false);
            wxString paramText;
            const bool hasParams = SplitFunctionAndParams(functionStr, paramText);
            wxStyledTextCtrl* styleWindow = dynamic_cast<wxStyledTextCtrl*>(m_editWindow);
            assert(styleWindow);
            if (hasParams)
                {
                styleWindow->AddText(functionStr + L"(");
                styleWindow->SetSelection(styleWindow->GetCurrentPos(),
                                          styleWindow->GetCurrentPos());
                styleWindow->CallTipShow(styleWindow->GetCurrentPos(), paramText + L")");
                }
            else
                {
                styleWindow->AddText(functionStr);
                styleWindow->SetSelection(styleWindow->GetCurrentPos(),
                                          styleWindow->GetCurrentPos());
                }
            m_editWindow->SetFocus();
            }
        }

    //------------------------------------------------
    void FunctionBrowserCtrl::CreateControls(const wxString& firstWindowCaption,
                                             const wxString& secondWindowCaption)
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        auto* listsSizer = new wxBoxSizer(wxHORIZONTAL);

        m_categoryList = new Wisteria::UI::SideBar(this, ID_CATEGORY_LIST);
        m_categoryList->SetImageList(GetImageList());
        auto* categorySizer = new wxBoxSizer(wxVERTICAL);
        categorySizer->Add(new wxStaticText(this, wxID_STATIC, firstWindowCaption));
        categorySizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        categorySizer->Add(m_categoryList, wxSizerFlags{ 1 }.Expand());
        listsSizer->Add(categorySizer, wxSizerFlags{}.Expand());

        listsSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());

        m_functionList =
            new wxListBox(this, ID_FUNCTION_LIST, wxDefaultPosition, FromDIP(wxSize(275, 400)), 0,
                          nullptr, wxBORDER_THEME | wxLB_SINGLE | wxLB_HSCROLL | wxLB_NEEDED_SB);
        auto* functionSizer = new wxBoxSizer(wxVERTICAL);
        functionSizer->Add(new wxStaticText(this, wxID_STATIC, secondWindowCaption));
        functionSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        functionSizer->Add(m_functionList, wxSizerFlags{ 1 }.Expand());
        listsSizer->Add(functionSizer, wxSizerFlags{ 1 }.Expand());

        mainSizer->Add(listsSizer, wxSizerFlags{ 1 }.Expand().Border());

        m_functionDescriptionWindow =
            new wxHtmlWindow(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(500, 150)),
                             wxHW_SCROLLBAR_AUTO | wxBORDER_THEME | wxHW_NO_SELECTION);
        mainSizer->Add(m_functionDescriptionWindow, wxSizerFlags{}.Expand().Border());

        SetSizer(mainSizer);

        Bind(wxEVT_SIDEBAR_CLICK, &FunctionBrowserCtrl::OnListSelected, this);
        Bind(wxEVT_LISTBOX, &FunctionBrowserCtrl::OnListSelected, this);
        Bind(wxEVT_HTML_LINK_CLICKED, &FunctionBrowserCtrl::OnHyperlinkClicked, this);
        Bind(
            wxEVT_LISTBOX_DCLICK, [this]([[maybe_unused]] wxCommandEvent&) { InsertFunction(); },
            FunctionBrowserCtrl::ID_FUNCTION_LIST);
        }

    //------------------------------------------------
    bool FunctionBrowserDlg::Create(
        wxWindow* parent, wxWindow* editor, const wxWindowID id /*= wxID_ANY*/,
        const wxString& caption /*= _(L"Function Browser")*/,
        const wxString& firstWindowCaption /*= _(L"Categories:")*/,
        const wxString& secondWindowCaption /*= _(L"Functions/Operators:")*/,
        const wxPoint& pos /*= wxDefaultPosition*/, const wxSize& size /*= wxDefaultSize*/,
        const long style /*= wxDEFAULT_DIALOG_STYLE | wxCLIP_CHILDREN | wxRESIZE_BORDER*/)
        {
        SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        DialogWithHelp::Create(parent, id, caption, pos, size, style);

        auto* mainSizer = new wxBoxSizer(wxVERTICAL);

        m_funcBrowserControl = new FunctionBrowserCtrl(this, editor, wxID_ANY, firstWindowCaption,
                                                       secondWindowCaption);
        mainSizer->Add(m_funcBrowserControl, wxSizerFlags{ 1 }.Expand());

        // Close and Insert buttons
        mainSizer->Add(CreateSeparatedButtonSizer(wxOK | wxCANCEL | wxHELP),
                       wxSizerFlags{}.Expand().Border());

        SetSizerAndFit(mainSizer);

        auto* insertButton = FindWindow(wxID_OK);
        if (insertButton != nullptr)
            {
            insertButton->SetId(ID_INSERT_BUTTON);
            insertButton->SetLabel(_(L"&Insert"));
            }
        auto* closeButton = FindWindow(wxID_CANCEL);
        if (closeButton != nullptr)
            {
            closeButton->SetLabel(_(L"&Close"));
            }
        Centre();

        // move this window over a bit so that you can see the parent formula editor behind it.
        Move(wxSystemSettings::GetMetric(wxSYS_SCREEN_X) -
                 (GetSize().GetWidth() + wxSizerFlags::GetDefaultBorder()),
             GetScreenPosition().y);

        // connect events
        Bind(
            wxEVT_BUTTON,
            [this]([[maybe_unused]]
                   wxCommandEvent& event) { m_funcBrowserControl->InsertFunction(); },
            FunctionBrowserDlg::ID_INSERT_BUTTON);

        return true;
        }
    } // namespace Wisteria::UI
