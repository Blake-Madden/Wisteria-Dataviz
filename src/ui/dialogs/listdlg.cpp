///////////////////////////////////////////////////////////////////////////////
// Name:        listdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listdlg.h"
#include "../ribbon/artmetro.h"

//------------------------------------------------------
void ListDlg::OnFind(wxFindDialogEvent& event)
    {
    if (m_list)
        {
        m_list->ProcessWindowEvent(event);
        m_list->SetFocus();
        }
    }

//------------------------------------------------------
void ListDlg::OnSort(wxRibbonButtonBarEvent& event)
    {
    if (m_list)
        { m_list->OnMultiColumSort(event); }
    }

//------------------------------------------------------
void ListDlg::OnSave(wxRibbonButtonBarEvent& event)
    {
    if (m_usecheckBoxes)
        {
        wxFAIL_MSG(L"Save not supported for checklist control");
        }
    else if (m_list)
        { m_list->OnSave(event); }
    }

//------------------------------------------------------
void ListDlg::OnPrint(wxRibbonButtonBarEvent& event)
    {
    if (m_usecheckBoxes)
        {
        wxFAIL_MSG(L"Print not supported for checklist control");
        }
    else if (m_list)
        { m_list->OnPrint(event); }
    }

//------------------------------------------------------
void ListDlg::OnSelectAll([[maybe_unused]] wxRibbonButtonBarEvent& event)
    {
    if (m_usecheckBoxes && m_checkList)
        {
        for (size_t i = 0; i < m_checkList->GetCount(); ++i)
            { m_checkList->Check(i); }
        }
    else if (m_list)
        { m_list->SelectAll(); }
    }

//------------------------------------------------------
void ListDlg::OnCopy([[maybe_unused]] wxRibbonButtonBarEvent& event)
    {
    if (m_usecheckBoxes && m_checkList)
        {
        wxString selectedText;
        wxString currentSelectedItem;
        for (size_t i = 0; i < m_checkList->GetCount(); ++i)
            {
            if (m_checkList->IsSelected(i))
                {
                currentSelectedItem = m_checkList->GetString(i);
                // unescape mnemonics
                currentSelectedItem.Replace(L"&&", L"&", true);
                selectedText += currentSelectedItem + wxString(L"\n");
                }
            }
        selectedText.Trim(true); selectedText.Trim(false);
        if (wxTheClipboard->Open() && selectedText.length() > 0)
            {
            wxTheClipboard->Clear();
            wxDataObjectComposite* obj = new wxDataObjectComposite();
            obj->Add(new wxTextDataObject(selectedText) );
            wxTheClipboard->AddData(obj);
            wxTheClipboard->Close();
            }
        }
    else if (m_list)
        { m_list->Copy(true,false); }
    }

//------------------------------------------------------
ListDlg::ListDlg(wxWindow* parent, const wxArrayString& values, const bool usecheckBoxes,
            const wxColour bkColor,
            const wxColour hoverColor,
            const wxColour foreColor,
            const long buttonStyle /*= LD_NO_BUTTONS*/,
            wxWindowID id /*= wxID_ANY*/, const wxString& caption /*= wxString{}*/,
            const wxString& label /*= wxString{}*/,
            const wxPoint& pos /*= wxDefaultPosition*/,
            const wxSize& size /*= wxSize(600, 250)*/,
            long style /*= wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER*/) :
            m_usecheckBoxes(usecheckBoxes),
            m_buttonStyle(buttonStyle), m_label(label), m_hoverColor(hoverColor),
            m_values(values)
    {
    GetData()->SetValues(values);
    SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);
    SetMinSize(FromDIP(wxSize(600, 250)));

    SetBackgroundColour(bkColor);
    SetForegroundColour(foreColor);

    CreateControls();
    Centre();
    BindEvents();
    }

//------------------------------------------------------
ListDlg::ListDlg(wxWindow* parent,
        const wxColour bkColor,
        const wxColour hoverColor,
        const wxColour foreColor,
        const long buttonStyle /*= LD_NO_BUTTONS*/,
        wxWindowID id /*= wxID_ANY*/, const wxString& caption /*= wxString{}*/,
        const wxString& label /*= wxString{}*/,
        const wxPoint& pos /*= wxDefaultPosition*/,
        const wxSize& size /*= wxSize(600, 250)*/,
        long style /*= wxDEFAULT_DIALOG_STYLE|wxRESIZE_BORDER*/) :
        m_usecheckBoxes(false),
        m_buttonStyle(buttonStyle), m_label(label), m_hoverColor(hoverColor)
    {
    SetExtraStyle(GetExtraStyle()|wxWS_EX_BLOCK_EVENTS);
    wxDialog::Create(parent, id, caption, pos, size, style);
    SetMinSize(FromDIP(wxSize(600, 250)));

    SetBackgroundColour(bkColor);
    SetForegroundColour(foreColor);

    CreateControls();
    Centre();
    BindEvents();
    }

//------------------------------------------------------
void ListDlg::BindEvents()
    {
    Bind(wxEVT_BUTTON, &ListDlg::OnNegative, this, wxID_CANCEL);
    Bind(wxEVT_BUTTON, &ListDlg::OnNegative, this, wxID_CLOSE);
    Bind(wxEVT_BUTTON, &ListDlg::OnNegative, this, wxID_NO);

    Bind(wxEVT_BUTTON, &ListDlg::OnAffirmative, this, wxID_YES);
    Bind(wxEVT_BUTTON, &ListDlg::OnAffirmative, this, wxID_OK);

    Bind(wxEVT_CLOSE_WINDOW, &ListDlg::OnClose, this);

    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnSave, this, wxID_SAVE);
    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnPrint, this, wxID_PRINT);
    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnCopy, this, wxID_COPY);
    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnSelectAll, this, wxID_SELECTALL);
    Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnSort, this, XRCID("ID_LIST_SORT"));

    Bind(wxEVT_FIND, &ListDlg::OnFind, this);
    Bind(wxEVT_FIND_NEXT, &ListDlg::OnFind, this);
    }

//------------------------------------------------------
void ListDlg::CreateControls()
    {
    wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
    mainSizer->SetMinSize(FromDIP(wxSize(800, 600)));

    // the top label
    if (m_label.length())
        {
        wxBoxSizer* labelSizer = new wxBoxSizer(wxHORIZONTAL);
        labelSizer->Add(new wxStaticText(this, wxID_STATIC, m_label), 0, wxALIGN_CENTER|wxALL, 0);
        labelSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
        mainSizer->Add(labelSizer, 0, wxALL, wxSizerFlags::GetDefaultBorder());
        }

    if (m_buttonStyle & LD_FIND_BUTTON)
        {
        wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);
        searchSizer->AddStretchSpacer(1);
        auto searcher = new Wisteria::UI::SearchPanel(this, wxID_ANY);
        searcher->SetBackgroundColour(GetBackgroundColour());
        searchSizer->Add(searcher, 0);
        mainSizer->Add(searchSizer, 0, wxEXPAND);
        }
    if (m_buttonStyle & LD_COPY_BUTTON || m_buttonStyle & LD_SELECT_ALL_BUTTON ||
        m_buttonStyle & LD_SORT_BUTTON || m_buttonStyle & LD_SAVE_BUTTON ||
        m_buttonStyle & LD_PRINT_BUTTON)
        {
        m_ribbon = new wxRibbonBar(this, wxID_ANY, wxDefaultPosition,
                                              wxDefaultSize, wxRIBBON_BAR_FLOW_HORIZONTAL);
        wxRibbonPage* homePage = new wxRibbonPage(m_ribbon, wxID_ANY, wxString{});
        // export
        if (m_buttonStyle & LD_SAVE_BUTTON || m_buttonStyle & LD_PRINT_BUTTON)
            {
            wxRibbonPanel* exportPage = new wxRibbonPanel(homePage, wxID_ANY, _(L"Export"),
                                                          wxNullBitmap, wxDefaultPosition,
                                                          wxDefaultSize,
                                                          wxRIBBON_PANEL_NO_AUTO_MINIMISE);
            wxRibbonButtonBar* buttonBar = new wxRibbonButtonBar(exportPage);
            if (m_buttonStyle & LD_SAVE_BUTTON)
                {
                buttonBar->AddButton(wxID_SAVE, _(L"Save"),
                    wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON, FromDIP(wxSize(32, 32))).ConvertToImage(),
                    _(L"Save the list."));
                }
            if (m_buttonStyle & LD_PRINT_BUTTON)
                {
                buttonBar->AddButton(wxID_PRINT, _(L"Print"),
                    wxArtProvider::GetBitmap(wxART_PRINT, wxART_BUTTON, FromDIP(wxSize(32, 32))).ConvertToImage(),
                    _(L"Print the list."));
                }
            }
        // edit
        if (m_buttonStyle & LD_COPY_BUTTON || m_buttonStyle & LD_SELECT_ALL_BUTTON ||
            m_buttonStyle & LD_SORT_BUTTON)
            {
            wxRibbonPanel* editPage = new wxRibbonPanel(homePage, wxID_ANY, _(L"Edit"),
                                                        wxNullBitmap, wxDefaultPosition,
                                                        wxDefaultSize,
                                                        wxRIBBON_PANEL_NO_AUTO_MINIMISE);
            wxRibbonButtonBar* buttonBar = new wxRibbonButtonBar(editPage);
            if (m_buttonStyle & LD_COPY_BUTTON)
                {
                buttonBar->AddButton(wxID_COPY, _(L"Copy Selection"),
                    wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON,
                        FromDIP(wxSize(32, 32))).ConvertToImage(), _(L"Copy the selected items."));
                }
            if (m_buttonStyle & LD_SELECT_ALL_BUTTON)
                {
                buttonBar->AddButton(wxID_SELECTALL, _(L"Select All"),
                    wxArtProvider::GetBitmap(L"ID_SELECT_ALL", wxART_BUTTON,
                        FromDIP(wxSize(32, 32))).ConvertToImage(), _(L"Select the entire list."));
                }
            if (m_buttonStyle & LD_SORT_BUTTON)
                {
                buttonBar->AddButton(XRCID("ID_LIST_SORT"), _(L"Sort"),
                    wxArtProvider::GetBitmap(L"ID_LIST_SORT", wxART_BUTTON,
                        FromDIP(wxSize(32, 32))).ConvertToImage(), _(L"Sort the list."));
                }
            }
        m_ribbon->SetArtProvider(new Wisteria::UI::RibbonMetroArtProvider);
        mainSizer->Add(m_ribbon, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());
        m_ribbon->Realise();
        }

    if (m_usecheckBoxes)
        {
        for (size_t i = 0; i < m_values.GetCount(); ++i)
            { m_values[i] = wxControl::EscapeMnemonics(m_values[i]); }
        m_checkList = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition,
            wxDefaultSize, m_values, wxLB_EXTENDED|wxLB_SORT);
        mainSizer->Add(m_checkList, 1, wxEXPAND);
        }
    else
        {
        long flags{ wxLC_VIRTUAL | wxLC_REPORT | wxLC_ALIGN_LEFT };
        if (!(m_buttonStyle & LD_COLUMN_HEADERS))
            { flags |= wxLC_NO_HEADER; }
        if (m_buttonStyle & LD_SINGLE_SELECTION)
            { flags |= wxLC_SINGLE_SEL; }
        m_list = new ListCtrlEx(this, wxID_ANY, wxDefaultPosition, GetSize(), flags);
        m_list->SetLabel(GetLabel());
        m_list->EnableGridLines();
        m_list->EnableItemViewOnDblClick();
        m_list->InsertColumn(0, wxString{});
        m_list->SetVirtualDataProvider(m_data);
        m_list->SetVirtualDataSize(m_data->GetItemCount(), 1);
        m_list->DistributeColumns();

        mainSizer->Add(m_list, 1, wxEXPAND);
        }

    wxSizer* OkCancelSizer = nullptr;
    if (m_buttonStyle & LD_OK_CANCEL_BUTTONS)
        {
        OkCancelSizer = CreateButtonSizer(wxOK|wxCANCEL);
        mainSizer->Add(OkCancelSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());
        SetAffirmativeId(wxID_OK);
        SetEscapeId(wxID_CANCEL);
        }
    else if (m_buttonStyle & LD_YES_NO_BUTTONS)
        {
        OkCancelSizer = CreateButtonSizer(wxYES_NO);
        mainSizer->Add(OkCancelSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());
        SetAffirmativeId(wxID_YES);
        SetEscapeId(wxID_NO);
        }
    else if (m_buttonStyle & LD_CLOSE_BUTTON)
        {
        OkCancelSizer = CreateButtonSizer(wxCLOSE);
        mainSizer->Add(OkCancelSizer, 0, wxEXPAND|wxALL, wxSizerFlags::GetDefaultBorder());
        SetAffirmativeId(wxID_CLOSE);
        }

    if ((m_buttonStyle & LD_DONT_SHOW_AGAIN) && OkCancelSizer)
        {
        m_checkBox = new wxCheckBox(this, wxID_ANY, _(L"Don't show this again"),
                                    wxDefaultPosition , wxDefaultSize, wxCHK_2STATE,
                                    wxGenericValidator(&m_dontShowAgain));
        OkCancelSizer->Insert(0, m_checkBox, 0, wxALL|wxEXPAND, wxSizerFlags::GetDefaultBorder());
        }

    SetSizerAndFit(mainSizer);
    }

//------------------------------------------------------
void ListDlg::OnNegative(wxCommandEvent& event)
    {
    // search control locks up app if it has the focus here, so remove the focus from it
    SetFocusIgnoringChildren();

    TransferDataFromWindow();

    if (IsModal())
        { EndModal(event.GetId()); }
    else
        { Show(false); }
    }

//------------------------------------------------------
void ListDlg::OnClose([[maybe_unused]] wxCloseEvent& event)
    {
    // search control locks up app if it has the focus here, so remove the focus from it
    SetFocusIgnoringChildren();

    if (IsModal())
        { EndModal(wxID_CLOSE); }
    else
        { Hide(); }
    }

//------------------------------------------------------
void ListDlg::OnAffirmative(wxCommandEvent& event)
    {
    // search control locks up app if it has the focus here, so remove the focus from it
    SetFocusIgnoringChildren();

    // record what is checked or selected
    if (m_usecheckBoxes)
        {
        wxString currentSelectedItem;
        for (size_t i = 0; i < m_checkList->GetCount(); ++i)
            {
            if (m_checkList->IsChecked(i))
                {
                currentSelectedItem = m_checkList->GetString(i);
                // unescape mnemonics
                currentSelectedItem.Replace(L"&&", L"&", true);
                m_selectedItems.Add(currentSelectedItem);
                }
            }
        }
    else
        {
        long item = wxNOT_FOUND;
        while (true)
            {
            item = m_list->GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (item == wxNOT_FOUND)
                { break; }
            m_selectedItems.Add(m_list->GetItemText(item));
            }
        }

    TransferDataFromWindow();

    if (IsModal())
        { EndModal(event.GetId()); }
    else
        { Show(false); }
    }
