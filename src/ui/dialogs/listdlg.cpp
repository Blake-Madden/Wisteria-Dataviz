///////////////////////////////////////////////////////////////////////////////
// Name:        listdlg.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listdlg.h"
#include "../../import/text_matrix.h"
#include <utility>
#include <wx/ribbon/art.h>
#include <wx/valgen.h>

namespace Wisteria::UI
    {
    //------------------------------------------------------
    void ListDlg::OnFind(wxFindDialogEvent& event)
        {
        if (m_list != nullptr)
            {
            m_list->ProcessWindowEvent(event);
            m_list->SetFocus();
            }
        }

    //------------------------------------------------------
    void ListDlg::OnSort(wxRibbonButtonBarEvent& event)
        {
        if (m_list != nullptr)
            {
            m_list->OnMultiColumnSort(event);
            }
        }

    //------------------------------------------------------
    void ListDlg::OnSave(wxRibbonButtonBarEvent& event)
        {
        if (m_useCheckBoxes)
            {
            wxFAIL_MSG(L"Save not supported for checklist control");
            }
        else if (m_list != nullptr)
            {
            m_list->OnSave(event);
            }
        }

    //------------------------------------------------------
    void ListDlg::OnPrint(wxRibbonButtonBarEvent& event)
        {
        if (m_useCheckBoxes)
            {
            wxFAIL_MSG(L"Print not supported for checklist control");
            }
        else if (m_list != nullptr)
            {
            m_list->OnPrint(event);
            }
        }

    //------------------------------------------------------
    void ListDlg::OnSelectAll([[maybe_unused]] wxRibbonButtonBarEvent& event)
        {
        if (m_useCheckBoxes && (m_checkList != nullptr))
            {
            for (size_t i = 0; i < m_checkList->GetCount(); ++i)
                {
                m_checkList->Check(i);
                }
            }
        else if (m_list != nullptr)
            {
            m_list->SelectAll();
            }
        }

    //------------------------------------------------------
    void ListDlg::OnCopy([[maybe_unused]] wxRibbonButtonBarEvent& event)
        {
        if (m_useCheckBoxes && m_checkList != nullptr)
            {
            wxString selectedText;
            for (size_t i = 0; i < m_checkList->GetCount(); ++i)
                {
                if (m_checkList->IsSelected(i))
                    {
                    wxString currentSelectedItem = m_checkList->GetString(i);
                    // unescape mnemonics
                    currentSelectedItem.Replace(L"&&", L"&", true);
                    selectedText += currentSelectedItem + wxString(L"\n");
                    }
                }
            selectedText.Trim(true);
            selectedText.Trim(false);
            if (!selectedText.empty() && wxTheClipboard->Open())
                {
                wxTheClipboard->Clear();
                auto* obj = new wxDataObjectComposite();
                obj->Add(new wxTextDataObject(selectedText));
                wxTheClipboard->SetData(obj);
                wxTheClipboard->Close();
                }
            }
        else if (m_list != nullptr)
            {
            m_list->Copy(true, false);
            }
        }

    //------------------------------------------------------
    ListDlg::ListDlg(wxWindow* parent, const wxArrayString& values, const bool useCheckBoxes,
                     const wxColour& bkColor, const wxColour& hoverColor, const wxColour& foreColor,
                     const long buttonStyle /*= LD_NO_BUTTONS*/, const wxWindowID id /*= wxID_ANY*/,
                     const wxString& caption /*= wxString{}*/, wxString label /*= wxString{}*/,
                     const wxPoint& pos /*= wxDefaultPosition*/,
                     const wxSize& size /*= wxSize(600, 250)*/,
                     const long style /*= wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER*/)
        : m_useCheckBoxes(useCheckBoxes), m_buttonStyle(buttonStyle), m_label(std::move(label)),
          m_hoverColor(hoverColor), m_values(values), m_realTimeTimer(this)
        {
        GetData()->SetValues(values);
        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style);
        wxTopLevelWindowBase::SetMinSize(FromDIP(wxSize(600, 250)));

        wxNonOwnedWindow::SetBackgroundColour(bkColor);
        wxWindow::SetForegroundColour(foreColor);

        CreateControls();
        Centre();
        BindEvents();
        RestartRealtimeUpdate();
        }

    //------------------------------------------------------
    ListDlg::ListDlg(wxWindow* parent, const wxColour& bkColor, const wxColour& hoverColor,
                     const wxColour& foreColor, const long buttonStyle /*= LD_NO_BUTTONS*/,
                     wxWindowID id /*= wxID_ANY*/, const wxString& caption /*= wxString{}*/,
                     wxString label /*= wxString{}*/, const wxPoint& pos /*= wxDefaultPosition*/,
                     const wxSize& size /*= wxSize(600, 250)*/,
                     const long style /*= wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER*/)
        : m_useCheckBoxes(false), m_buttonStyle(buttonStyle), m_label(std::move(label)),
          m_hoverColor(hoverColor), m_realTimeTimer(this)
        {
        wxNonOwnedWindow::SetExtraStyle(GetExtraStyle() | wxWS_EX_BLOCK_EVENTS);
        wxDialog::Create(parent, id, caption, pos, size, style);
        wxTopLevelWindowBase::SetMinSize(FromDIP(wxSize(600, 250)));

        wxNonOwnedWindow::SetBackgroundColour(bkColor);
        wxWindow::SetForegroundColour(foreColor);

        CreateControls();
        Centre();
        BindEvents();
        RestartRealtimeUpdate();
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
        Bind(
            wxEVT_RIBBONBUTTONBAR_CLICKED,
            [this]([[maybe_unused]] wxRibbonButtonBarEvent&)
            {
                if (m_logFile != nullptr && m_list != nullptr)
                    {
                    m_logFile->Clear();
                    m_list->DeleteAllItems();
                    }
            },
            XRCID("ID_CLEAR"));
        Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnReadLog, this, XRCID("ID_REFRESH"));
        Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListDlg::OnRealTimeUpdate, this,
             XRCID("ID_REALTIME_UPDATE"));
        Bind(wxEVT_TIMER, &ListDlg::OnRealTimeTimer, this);
        Bind(
            wxEVT_RIBBONBUTTONBAR_CLICKED,
            [this]([[maybe_unused]]
                   wxRibbonButtonBarEvent& evt)
            {
                m_isLogVerbose = !m_isLogVerbose;
                if (m_logFile != nullptr)
                    {
                    LogFile::SetVerbose(m_isLogVerbose);
                    }
            },
            XRCID("ID_VERBOSE_LOG"));

        Bind(wxEVT_FIND, &ListDlg::OnFind, this);
        Bind(wxEVT_FIND_NEXT, &ListDlg::OnFind, this);
        }

    //------------------------------------------------------
    void ListDlg::CreateControls()
        {
        auto* mainSizer = new wxBoxSizer(wxVERTICAL);
        mainSizer->SetMinSize(FromDIP(wxSize{ 800, 600 }));

        // the top label
        if (!m_label.empty())
            {
            auto* labelSizer = new wxBoxSizer(wxHORIZONTAL);
            labelSizer->Add(new wxStaticText(this, wxID_STATIC, m_label), 0, wxALIGN_CENTER | wxALL,
                            0);
            labelSizer->AddSpacer(wxSizerFlags::GetDefaultBorder());
            mainSizer->Add(labelSizer, wxSizerFlags{}.Border());
            }

        if ((m_buttonStyle & LD_FIND_BUTTON) != 0)
            {
            auto* searchSizer = new wxBoxSizer(wxHORIZONTAL);
            searchSizer->AddStretchSpacer(1);
            auto* searcher = new Wisteria::UI::SearchPanel(this, wxID_ANY);
            searcher->SetBackgroundColour(GetBackgroundColour());
            searchSizer->Add(searcher, 0);
            mainSizer->Add(searchSizer, wxSizerFlags{}.Expand());
            }
        if (((m_buttonStyle & LD_COPY_BUTTON) != 0) ||
            ((m_buttonStyle & LD_SELECT_ALL_BUTTON) != 0) ||
            ((m_buttonStyle & LD_SORT_BUTTON) != 0) || ((m_buttonStyle & LD_SAVE_BUTTON) != 0) ||
            ((m_buttonStyle & LD_PRINT_BUTTON) != 0))
            {
            m_ribbon = new wxRibbonBar(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                       wxRIBBON_BAR_FLOW_HORIZONTAL);
            wxRibbonPage* homePage{ nullptr };
            // export
            if (((m_buttonStyle & LD_SAVE_BUTTON) != 0) || ((m_buttonStyle & LD_PRINT_BUTTON) != 0))
                {
                if (homePage == nullptr) // NOLINT
                    {
                    homePage = new wxRibbonPage(m_ribbon, wxID_ANY, wxString{});
                    }
                auto* exportPage = new wxRibbonPanel(homePage, wxID_ANY, _(L"Export"), wxNullBitmap,
                                                     wxDefaultPosition, wxDefaultSize,
                                                     wxRIBBON_PANEL_NO_AUTO_MINIMISE);
                auto* buttonBar = new wxRibbonButtonBar(exportPage);
                if ((m_buttonStyle & LD_SAVE_BUTTON) != 0)
                    {
                    buttonBar->AddButton(wxID_SAVE, _(L"Save"),
                                         wxArtProvider::GetBitmap(wxART_FILE_SAVE, wxART_BUTTON,
                                                                  FromDIP(wxSize(32, 32)))
                                             .ConvertToImage(),
                                         _(L"Save the list."));
                    }
                if ((m_buttonStyle & LD_PRINT_BUTTON) != 0)
                    {
                    buttonBar->AddButton(
                        wxID_PRINT, _(L"Print"),
                        wxArtProvider::GetBitmap(wxART_PRINT, wxART_BUTTON, FromDIP(wxSize(32, 32)))
                            .ConvertToImage(),
                        _(L"Print the list."));
                    }
                }
            // edit
            if (((m_buttonStyle & LD_COPY_BUTTON) != 0) ||
                ((m_buttonStyle & LD_SELECT_ALL_BUTTON) != 0) ||
                ((m_buttonStyle & LD_SORT_BUTTON) != 0) ||
                ((m_buttonStyle & LD_CLEAR_BUTTON) != 0) ||
                ((m_buttonStyle & LD_REFRESH_BUTTON) != 0) ||
                ((m_buttonStyle & LD_LOG_VERBOSE_BUTTON) != 0))
                {
                if (homePage == nullptr)
                    {
                    homePage = new wxRibbonPage(m_ribbon, wxID_ANY, wxString{});
                    }
                auto* editPage = new wxRibbonPanel(homePage, ID_EDIT_PANEL, _(L"Edit"),
                                                   wxNullBitmap, wxDefaultPosition, wxDefaultSize,
                                                   wxRIBBON_PANEL_NO_AUTO_MINIMISE);
                m_editButtonBar = new wxRibbonButtonBar(editPage, ID_EDIT_BUTTON_BAR);
                if ((m_buttonStyle & LD_COPY_BUTTON) != 0)
                    {
                    m_editButtonBar->AddButton(
                        wxID_COPY, _(L"Copy Selection"),
                        wxArtProvider::GetBitmap(wxART_COPY, wxART_BUTTON, FromDIP(wxSize(32, 32)))
                            .ConvertToImage(),
                        _(L"Copy the selected items."));
                    }
                if ((m_buttonStyle & LD_SELECT_ALL_BUTTON) != 0)
                    {
                    m_editButtonBar->AddButton(wxID_SELECTALL, _(L"Select All"),
                                               wxArtProvider::GetBitmap(L"ID_SELECT_ALL",
                                                                        wxART_BUTTON,
                                                                        FromDIP(wxSize(32, 32)))
                                                   .ConvertToImage(),
                                               _(L"Select the entire list."));
                    }
                if ((m_buttonStyle & LD_SORT_BUTTON) != 0)
                    {
                    m_editButtonBar->AddButton(XRCID("ID_LIST_SORT"), _(L"Sort"),
                                               wxArtProvider::GetBitmap(L"ID_LIST_SORT",
                                                                        wxART_BUTTON,
                                                                        FromDIP(wxSize(32, 32)))
                                                   .ConvertToImage(),
                                               _(L"Sort the list."));
                    }
                if ((m_buttonStyle & LD_CLEAR_BUTTON) != 0)
                    {
                    m_editButtonBar->AddButton(
                        XRCID("ID_CLEAR"), _(L"Clear"),
                        wxArtProvider::GetBitmap(L"ID_CLEAR", wxART_BUTTON, FromDIP(wxSize(32, 32)))
                            .ConvertToImage(),
                        _(L"Clear the log report."));
                    }
                if ((m_buttonStyle & LD_REFRESH_BUTTON) != 0)
                    {
                    m_editButtonBar->AddButton(XRCID("ID_REFRESH"), _(L"Refresh"),
                                               wxArtProvider::GetBitmap(L"ID_REFRESH", wxART_BUTTON,
                                                                        FromDIP(wxSize(32, 32)))
                                                   .ConvertToImage(),
                                               _(L"Refresh the log report."));
                    m_editButtonBar->AddToggleButton(
                        XRCID("ID_REALTIME_UPDATE"), _(L"Auto Refresh"),
                        wxArtProvider::GetBitmap(L"ID_REALTIME_UPDATE", wxART_BUTTON,
                                                 FromDIP(wxSize(32, 32)))
                            .ConvertToImage(),
                        _(L"Refresh the log report automatically."));
                    m_editButtonBar->ToggleButton(XRCID("ID_REALTIME_UPDATE"), m_autoRefresh);
                    }
                if ((m_buttonStyle & LD_LOG_VERBOSE_BUTTON) != 0)
                    {
                    m_editButtonBar->AddToggleButton(
                        XRCID("ID_VERBOSE_LOG"), _(L"Verbose"),
                        wxArtProvider::GetBitmap(wxART_INFORMATION, wxART_BUTTON,
                                                 FromDIP(wxSize(32, 32)))
                            .ConvertToImage(),
                        _(L"Toggles whether the logging system includes "
                          "more detailed information."));
                    m_editButtonBar->ToggleButton(XRCID("ID_VERBOSE_LOG"), m_isLogVerbose);
                    }
                }
            m_ribbon->SetArtProvider(new wxRibbonMSWArtProvider);
            mainSizer->Add(m_ribbon, wxSizerFlags{}.Expand().Border());
            m_ribbon->Realise();
            }

        if (m_useCheckBoxes)
            {
            for (size_t i = 0; i < m_values.GetCount(); ++i)
                {
                m_values[i] = wxControl::EscapeMnemonics(m_values[i]);
                }
            m_checkList = new wxCheckListBox(this, wxID_ANY, wxDefaultPosition, wxDefaultSize,
                                             m_values, wxLB_EXTENDED | wxLB_SORT);
            mainSizer->Add(m_checkList, wxSizerFlags{ 1 }.Expand());
            }
        else
            {
            long flags{ wxLC_VIRTUAL | wxLC_REPORT | wxLC_ALIGN_LEFT };
            if ((m_buttonStyle & LD_COLUMN_HEADERS) == 0)
                {
                flags |= wxLC_NO_HEADER;
                }
            if ((m_buttonStyle & LD_SINGLE_SELECTION) != 0)
                {
                flags |= wxLC_SINGLE_SEL;
                }
            m_list = new ListCtrlEx(this, wxID_ANY, wxDefaultPosition, GetSize(), flags);
            m_list->SetLabel(GetLabel());
            m_list->EnableGridLines();
            m_list->EnableItemViewOnDblClick();
            m_list->InsertColumn(0, wxString{});
            m_list->SetVirtualDataProvider(m_data);
            m_list->SetVirtualDataSize(m_data->GetItemCount(), 1);
            m_list->DistributeColumns();

            mainSizer->Add(m_list, wxSizerFlags{ 1 }.Expand());
            }

        wxSizer* okCancelSizer = nullptr;
        if ((m_buttonStyle & LD_OK_CANCEL_BUTTONS) != 0)
            {
            okCancelSizer = CreateButtonSizer(wxOK | wxCANCEL);
            mainSizer->Add(okCancelSizer, wxSizerFlags{}.Expand().Border());
            SetAffirmativeId(wxID_OK);
            SetEscapeId(wxID_CANCEL);
            }
        else if ((m_buttonStyle & LD_YES_NO_BUTTONS) != 0)
            {
            okCancelSizer = CreateButtonSizer(wxYES_NO);
            mainSizer->Add(okCancelSizer, wxSizerFlags{}.Expand().Border());
            SetAffirmativeId(wxID_YES);
            SetEscapeId(wxID_NO);
            }
        else if ((m_buttonStyle & LD_CLOSE_BUTTON) != 0)
            {
            okCancelSizer = CreateButtonSizer(wxCLOSE);
            mainSizer->Add(okCancelSizer, wxSizerFlags{}.Expand().Border());
            SetAffirmativeId(wxID_CLOSE);
            }

        if (((m_buttonStyle & LD_DONT_SHOW_AGAIN) != 0) && (okCancelSizer != nullptr))
            {
            m_checkBox =
                new wxCheckBox(this, wxID_ANY, _(L"Don't show this again"), wxDefaultPosition,
                               wxDefaultSize, wxCHK_2STATE, wxGenericValidator(&m_dontShowAgain));
            okCancelSizer->Insert(0, m_checkBox, wxSizerFlags{}.Expand().Border());
            }

        SetSizerAndFit(mainSizer);
        }

    //------------------------------------------------------
    void ListDlg::SetActiveLog(LogFile* log)
        {
        m_logFile = log;

        if (m_logFile == nullptr)
            {
            return;
            }

        // toggle the verbose button to match what the logger is doing
        m_isLogVerbose = LogFile::GetVerbose();
        if (m_editButtonBar != nullptr)
            {
            m_editButtonBar->ToggleButton(XRCID("ID_VERBOSE_LOG"), LogFile::GetVerbose());
            }

        RestartRealtimeUpdate();
        }

    //------------------------------------------------------
    void ListDlg::OnReadLog([[maybe_unused]] wxCommandEvent& event)
        {
        // in case the list is being sorted or an item view request was sent,
        // process all that before reloading the list control
        // NOLINTNEXTLINE(cppcoreguidelines-pro-type-static-cast-downcast)
        wxTheApp->Yield();
        if (m_logFile != nullptr && GetListCtrl() != nullptr)
            {
            const long style = GetListCtrl()->GetExtraStyle();
            GetListCtrl()->SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
            const wxWindowUpdateLocker wl{ GetListCtrl() };

            if (GetListCtrl()->GetColumnCount() < 4)
                {
                GetListCtrl()->DeleteAllColumns();
                GetListCtrl()->InsertColumn(0, _(L"Message"));
                GetListCtrl()->InsertColumn(1, _(L"Timestamp"));
                GetListCtrl()->InsertColumn(2, _(L"Function"));
                GetListCtrl()->InsertColumn(3, _(L"Source"));
                }
            // we do custom row highlighting below
            GetListCtrl()->EnableAlternateRowColours(false);

            GetListCtrl()->DeleteAllItems();

            const lily_of_the_valley::text_column_delimited_character_parser parser(L'\t');
            lily_of_the_valley::text_column<
                lily_of_the_valley::text_column_delimited_character_parser>
                myColumn(parser, std::nullopt);
            lily_of_the_valley::text_row<ListCtrlExDataProvider::ListCellString> myRow(
                std::nullopt);
            myRow.treat_consecutive_delimiters_as_one(false);
            myRow.add_column(myColumn);

            lily_of_the_valley::text_matrix<ListCtrlExDataProvider::ListCellString> importer(
                &GetData()->GetMatrix());
            importer.add_row_definition(myRow);

            // see how many lines are in the file
            const wxString logBuffer{ m_logFile->Read() };
            lily_of_the_valley::text_preview preview;
            size_t rowCount = preview(logBuffer, L'\t', true, false);
            // now read it
            rowCount = importer.read(logBuffer, rowCount, 4, true);

            GetListCtrl()->SetVirtualDataSize(rowCount, 4);
            GetListCtrl()->SetItemCount(static_cast<long>((rowCount)));

            for (long i = 0; i < GetListCtrl()->GetItemCount(); ++i)
                {
                const auto currentRow = GetListCtrl()->GetItemText(i, 0);
                const wxColour rowColor =
                    (currentRow.find(_DT(L"Error: ", DTExplanation::LogMessage)) !=
                     wxString::npos) ?
                        wxColour(242, 94, 101) :
                    (currentRow.find(_DT(L"Warning: ")) != wxString::npos) ?
                        Colors::ColorBrewer::GetColor(Colors::Color::Yellow) :
                    (currentRow.find(_DT(L"Debug: ")) != wxString::npos) ? wxColour(143, 214, 159) :
                                                                           wxNullColour;
                if (rowColor.IsOk())
                    {
                    GetListCtrl()->SetRowAttributes(
                        i, wxListItemAttr(wxColour{ 0, 0, 0 }, rowColor, GetListCtrl()->GetFont()));
                    }
                }

            // scroll to most recent item in the log
            if (GetListCtrl()->GetItemCount() > 0)
                {
                GetListCtrl()->EnsureVisible(GetListCtrl()->GetItemCount() - 1);
                }
            GetListCtrl()->SetSortedColumn(0, Wisteria::SortDirection::SortAscending);
            GetListCtrl()->SetExtraStyle(style);
            }
        }

    //------------------------------------------------------
    void ListDlg::OnRealTimeUpdate([[maybe_unused]] wxRibbonButtonBarEvent& event)
        {
        m_autoRefresh = !m_autoRefresh;
        if (m_autoRefresh)
            {
            RestartRealtimeUpdate();
            }
        else
            {
            StopRealtimeUpdate();
            }
        }

    //-------------------------------------------------------
    void ListDlg::OnRealTimeTimer([[maybe_unused]] wxTimerEvent& event)
        {
        StopRealtimeUpdate();
        if (m_logFile != nullptr && wxFile::Exists(m_logFile->GetLogFilePath()))
            {
            m_logFile->Flush();
            // just update the window if the log file changed
            const auto previousModTime{ m_sourceFileLastModified };
            m_sourceFileLastModified =
                wxFileName(m_logFile->GetLogFilePath()).GetModificationTime();
            if (m_sourceFileLastModified.IsValid() && previousModTime.IsValid() &&
                previousModTime < m_sourceFileLastModified)
                {
                ReadLog();
                }
            }
        RestartRealtimeUpdate();
        }

    //------------------------------------------------------
    void ListDlg::OnNegative(const wxCommandEvent& event)
        {
        // search control locks up app if it has the focus here, so remove the focus from it
        SetFocusIgnoringChildren();

        TransferDataFromWindow();

        if (IsModal())
            {
            EndModal(event.GetId());
            }
        else
            {
            Show(false);
            }
        }

    //------------------------------------------------------
    void ListDlg::OnClose([[maybe_unused]] wxCloseEvent& event)
        {
        // search control locks up app if it has the focus here, so remove the focus from it
        SetFocusIgnoringChildren();

        if (IsModal())
            {
            EndModal(wxID_CLOSE);
            }
        else
            {
            Hide();
            }
        }

    //------------------------------------------------------
    void ListDlg::OnAffirmative(const wxCommandEvent& event)
        {
        // search control locks up app if it has the focus here, so remove the focus from it
        SetFocusIgnoringChildren();

        // record what is checked or selected
        if (m_useCheckBoxes)
            {
            for (size_t i = 0; i < m_checkList->GetCount(); ++i)
                {
                if (m_checkList->IsChecked(i))
                    {
                    wxString currentSelectedItem = m_checkList->GetString(i);
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
                    {
                    break;
                    }
                m_selectedItems.Add(m_list->GetItemText(item));
                }
            }

        TransferDataFromWindow();

        if (IsModal())
            {
            EndModal(event.GetId());
            }
        else
            {
            Show(false);
            }
        }
    } // namespace Wisteria::UI
