///////////////////////////////////////////////////////////////////////////////
// Name:        listctrlex.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "listctrlex.h"
#include "../../debug/debug_profile.h"
#include "../../import/html_encode.h"
#include "../dialogs/listctrlitemviewdlg.h"
#include "../dialogs/listctrlsortdlg.h"
#include "../dialogs/radioboxdlg.h"

wxDEFINE_EVENT(wxEVT_LISTCTRLEX_EDITED, wxCommandEvent);

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::UI::ListCtrlEx, wxListView)

    namespace Wisteria::UI
    {
    //------------------------------------------------------
    ListEditTextCtrl::ListEditTextCtrl(
        wxWindow * parent, ListCtrlEx * owner, wxWindowID id /*= wxID_ANY*/,
        wxString value /*= wxString{}*/, const wxPoint& pos /*= wxDefaultPosition*/,
        const wxSize& size /*= wxDefaultSize*/, long style /*= 0*/,
        const wxValidator& validator /*= wxDefaultValidator*/,
        const wxString& name /*= L"ListEditTextCtrl"*/)
        : wxTextCtrl(parent, id, value, pos, size, style, validator, name), m_owner(owner),
          m_editedRow(wxNOT_FOUND), m_editedColumn(wxNOT_FOUND)
        {
        Bind(wxEVT_TEXT_ENTER, &ListEditTextCtrl::OnEnter, this);
        Bind(wxEVT_KILL_FOCUS, &ListEditTextCtrl::OnKillFocus, this);
        Bind(wxEVT_CHAR_HOOK, &ListEditTextCtrl::OnChar, this);
        }

    //------------------------------------------------------
    void ListEditTextCtrl::SetCurrentItem(const long row, const long column)
        {
        m_editedRow = row;
        m_editedColumn = column;
        }

    //------------------------------------------------------
    void ListEditTextCtrl::OnKillFocus([[maybe_unused]]
                                       wxFocusEvent &
                                       event)
        {
        Hide();
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) != GetValue()))
            {
            m_owner->SetItemText(m_editedRow, m_editedColumn, GetValue());
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        }

    //------------------------------------------------------
    void ListEditTextCtrl::OnEnter([[maybe_unused]]
                                   wxCommandEvent &
                                   event)
        {
        Accept(wxDirection::wxDOWN);
        }

    //------------------------------------------------------
    void ListEditTextCtrl::OnChar(wxKeyEvent & event)
        {
        if (event.GetKeyCode() == WXK_ESCAPE)
            {
            Cancel();
            }
        else if (event.GetKeyCode() == WXK_DOWN || event.GetKeyCode() == WXK_TAB)
            {
            Accept(wxDirection::wxDOWN);
            }
        else if (event.GetKeyCode() == WXK_UP)
            {
            Accept(wxDirection::wxUP);
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListEditTextCtrl::Cancel()
        {
        m_editedRow = m_editedColumn = wxNOT_FOUND;
        Hide();
        }

    //------------------------------------------------------
    void ListEditTextCtrl::Accept(wxDirection direction)
        {
        Hide();
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) != GetValue()))
            {
            m_owner->SetItemText(m_editedRow, m_editedColumn, GetValue());
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        // move focus to next (or previous) item in the parent list
        // (or insert a new row at the end and go to that, if adding is enabled)
        // and put it in edit mode
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND)
            {
            if (direction == wxDirection::wxDOWN)
                {
                if (m_editedRow + 1 < m_owner->GetItemCount())
                    {
                    m_owner->DeselectAll();
                    m_owner->EnsureVisible(m_editedRow + 1);
                    m_owner->Select(m_editedRow + 1);
                    m_owner->Focus(m_editedRow + 1);
                    m_owner->EditItem(m_editedRow + 1, 0);
                    }
                else if (m_owner->IsItemAddingEnabled())
                    {
                    const auto newSelection = m_owner->AddRow();
                    m_owner->DeselectAll();
                    m_owner->EnsureVisible(newSelection);
                    m_owner->Select(newSelection);
                    m_owner->Focus(newSelection);
                    m_owner->EditItem(newSelection, 0);
                    }
                }
            if (direction == wxDirection::wxUP)
                {
                if (m_editedRow > 0)
                    {
                    m_owner->DeselectAll();
                    m_owner->EnsureVisible(m_editedRow - 1);
                    m_owner->Select(m_editedRow - 1);
                    m_owner->Focus(m_editedRow - 1);
                    m_owner->EditItem(m_editedRow - 1, 0);
                    }
                }
            }
        }

    //------------------------------------------------------
    void ListEditComboBox::OnKillFocus(const wxFocusEvent& event)
        {
        Hide();
        /* The kill focus event is a little quirky when the combobox is not read only.
           When you first click on it, the combobox losses focus to its own text box. When
           this happens, the window of the focus event will be null, so ignore this event
           in that case.*/
        const wxWindow* focusedWindow = event.GetWindow();
        if (!focusedWindow || (focusedWindow && focusedWindow->GetParent() == this))
            {
            return;
            }
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) != GetValue()))
            {
            m_owner->SetItemText(m_editedRow, m_editedColumn, GetValue());
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        }

    //------------------------------------------------------
    void ListEditComboBox::OnEnter([[maybe_unused]]
                                   wxCommandEvent &
                                   event)
        {
        Hide();
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) != GetValue()))
            {
            m_owner->SetItemText(m_editedRow, m_editedColumn, GetValue());
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        }

    //------------------------------------------------------
    void ListEditSpinCtrl::OnChar(wxKeyEvent & event)
        {
        if (event.GetKeyCode() == WXK_ESCAPE)
            {
            Cancel();
            }
        else if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER ||
                 event.GetKeyCode() == WXK_TAB)
            {
            Accept();
            }
        // number or arrow key was probably typed, process it
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListEditSpinCtrl::Accept()
        {
        Hide();
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) !=
             wxString::Format(L"%0.1f", GetValue())))
            {
            // if user deleted contents of control,
            // then set the cell in the list control to empty string as well
            m_owner->SetItemText(m_editedRow, m_editedColumn,
                                 wxString::Format(L"%0.1f", GetValue()));
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        }

    //------------------------------------------------------
    void ListEditSpinCtrl::OnEndEditKillFocus(wxFocusEvent & event)
        {
        Accept();
        event.Skip();
        }

    //------------------------------------------------------
    ListEditSpinCtrlDouble::ListEditSpinCtrlDouble(
        wxWindow * parent, ListCtrlEx * owner, wxWindowID id /*= wxID_ANY*/,
        const wxString& value /*= wxString{}*/, const wxPoint& pos /*= wxDefaultPosition*/,
        const wxSize& size /*= wxDefaultSize*/, long style /*= wxSP_ARROW_KEYS*/,
        double Min /* = 1.0*/, double Max /*= 100.0*/, double initial /*= 1.0*/,
        const wxString& name /*= L"ListEditSpinCtrlDouble"*/)
        : wxSpinCtrlDouble(parent, id, value, pos, size, style, Min, Max, initial, 1.0, name),
          m_owner(owner), m_editedRow(wxNOT_FOUND), m_editedColumn(wxNOT_FOUND)
        {
        SetDigits(1);
        Bind(wxEVT_KILL_FOCUS, &ListEditSpinCtrlDouble::OnEndEditKillFocus, this);
        Bind(wxEVT_CHAR_HOOK, &ListEditSpinCtrlDouble::OnChar, this);
        }

    //------------------------------------------------------
    void ListEditSpinCtrlDouble::OnChar(wxKeyEvent & event)
        {
        if (event.GetKeyCode() == WXK_ESCAPE)
            {
            Cancel();
            }
        else if (event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER ||
                 event.GetKeyCode() == WXK_TAB)
            {
            Accept();
            }
        // number or arrow key was probably typed, process it
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListEditSpinCtrlDouble::Accept()
        {
        Hide();
        if (m_editedRow != wxNOT_FOUND && m_editedColumn != wxNOT_FOUND &&
            (m_owner->GetItemTextEx(m_editedRow, m_editedColumn) !=
             wxString::Format(L"%0.1f", GetValue())))
            {
            // if user deleted contents of control,
            // then set the cell in the list control to empty string as well
            m_owner->SetItemText(m_editedRow, m_editedColumn,
                                 std::isnan(GetValue()) ? wxString{} :
                                                          wxString::Format(L"%0.1f", GetValue()));
            m_owner->Refresh();
            m_owner->SetItemBeenEditedByUser(true);
            }
        }

    //------------------------------------------------------
    void ListEditSpinCtrlDouble::OnEndEditKillFocus(wxFocusEvent & event)
        {
        Accept();
        event.Skip();
        }

    //------------------------------------------------------
    ListCtrlEx::ListCtrlEx(wxWindow * parent, const wxWindowID id,
                           const wxPoint& pos /*= wxDefaultPosition*/,
                           const wxSize& size /*= wxDefaultSize*/, long style /*= 0*/,
                           const wxValidator& validator /*= wxDefaultValidator*/)
        : wxListView(parent, id, pos, size, style, validator, L"ListCtrlEx")
        {
        if (IsVirtual())
            {
            EnableAlternateRowColours(true);
            }
        Bind(wxEVT_KEY_DOWN, &ListCtrlEx::OnKeyDown, this);
        Bind(wxEVT_SIZE, &ListCtrlEx::OnResize, this);
        Bind(wxEVT_LIST_COL_CLICK, &ListCtrlEx::OnColClick, this);
        Bind(wxEVT_FIND, &ListCtrlEx::OnFind, this);
        Bind(wxEVT_FIND_NEXT, &ListCtrlEx::OnFind, this);
        Bind(wxEVT_FIND_CLOSE, &ListCtrlEx::OnFind, this);
        Bind(wxEVT_CONTEXT_MENU, &ListCtrlEx::OnContextMenu, this);
        Bind(wxEVT_RIBBONBUTTONBAR_CLICKED, &ListCtrlEx::OnRibbonButton, this);
        Bind(wxEVT_LIST_DELETE_ALL_ITEMS, &ListCtrlEx::OnDeleteAllItems, this);
        Bind(wxEVT_LIST_DELETE_ITEM, &ListCtrlEx::OnDeleteItem, this);
        // in-place editing or viewing row
        Bind(wxEVT_LEFT_DCLICK, &ListCtrlEx::OnDblClick, this);
        // we will just handle these in the activate event
        Bind(wxEVT_LIST_BEGIN_LABEL_EDIT, &ListCtrlEx::OnIgnoreEvent, this);
        Bind(wxEVT_LIST_END_LABEL_EDIT, &ListCtrlEx::OnIgnoreEvent, this);
        // menus
        Bind(wxEVT_MENU, &ListCtrlEx::OnMultiColumSort, this, XRCID("ID_LIST_SORT"));
        Bind(wxEVT_MENU, &ListCtrlEx::OnSelectAll, this, wxID_SELECTALL);
        Bind(wxEVT_MENU, &ListCtrlEx::OnCopy, this, wxID_COPY);
        Bind(wxEVT_MENU, &ListCtrlEx::OnCopyFirstColumn, this, XRCID("ID_COPY_FIRST_COLUMN"));
        Bind(wxEVT_MENU, &ListCtrlEx::OnCopyWithColumnHeaders, this,
             XRCID("ID_COPY_WITH_COLUMN_HEADERS"));
        Bind(wxEVT_MENU, &ListCtrlEx::OnCopyAll, this, XRCID("ID_COPY_ALL"));
        Bind(wxEVT_MENU, &ListCtrlEx::OnPaste, this, wxID_PASTE);
        Bind(wxEVT_MENU, &ListCtrlEx::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &ListCtrlEx::OnPreview, this, wxID_PREVIEW);
        Bind(wxEVT_MENU, &ListCtrlEx::OnPrint, this, wxID_PRINT);
        Bind(wxEVT_MENU, &ListCtrlEx::OnViewItem, this, XRCID("ID_VIEW_ITEM"));
        }

    //------------------------------------------------------
    ListCtrlEx::~ListCtrlEx()
        {
        // Under GTK+, DoDeleteAllItems() is called from the base DTOR,
        // so need to unbind this event. Otherwise, our method in this derived
        // class will be called after destruction.
        Unbind(wxEVT_LIST_DELETE_ALL_ITEMS, &ListCtrlEx::OnDeleteAllItems, this);

        wxDELETE(m_menu);
        wxDELETE(m_editTextCtrl);
        wxDELETE(m_editSpinCtrl);
        wxDELETE(m_editSpinCtrlDouble);
        wxDELETE(m_editComboBox);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnDeleteAllItems(wxListEvent & event)
        {
        if (IsVirtual())
            {
            if (m_virtualData != nullptr)
                {
                m_virtualData->DeleteAllItems();
                }
            SetItemCount(0);
            Refresh();
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnDeleteItem(wxListEvent & event)
        {
        // ListCtrl::DeleteItem will decrement the item count
        if (IsVirtual())
            {
            if (m_virtualData != nullptr)
                {
                m_virtualData->DeleteItem(event.GetIndex());
                }
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnIgnoreEvent(wxListEvent & event) { event.Veto(); }

    //------------------------------------------------------
    void ListCtrlEx::OnDblClick(wxMouseEvent & event)
        {
        SetFocus();
        // see which row is selected
        wxPoint pt = event.GetPosition();
        int flags = wxLIST_HITTEST_ONITEM;
        long selectedRow = HitTest(pt, flags);

        if (GetWindowStyle() & wxLC_EDIT_LABELS)
            {
            if (selectedRow == wxNOT_FOUND && IsItemAddingEnabled())
                {
                // add a new row if they clicked outside any valid rows (or if there are no rows)
                selectedRow = AddRow();
                }
            // update our point to take the scrollbar positions into account AFTER
            // hit test is called
            pt.x += GetScrollPos(wxHORIZONTAL);
            pt.y += GetScrollPos(wxVERTICAL);

            // figure out which column is selected
            long currentWidth = 0;
            long selectedColumn = 0;
            for (selectedColumn = 0; selectedColumn < GetColumnCount(); ++selectedColumn)
                {
                currentWidth += GetColumnWidth(selectedColumn);
                if (pt.x < currentWidth)
                    {
                    break;
                    }
                }
            EditItem(selectedRow, selectedColumn);
            }
        else if (m_enableItemViewable && (GetWindowStyle() & wxLC_REPORT))
            {
            ViewItem(selectedRow);
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnRibbonButton(const wxRibbonButtonBarEvent& event)
        {
        wxCommandEvent cmd(wxEVT_MENU, event.GetId());
        ProcessWindowEvent(cmd);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnViewItem([[maybe_unused]]
                                wxCommandEvent &
                                event)
        {
        ViewItem(GetFocusedItem());
        }

    /** Shows a dialog with all the values displayed vertically.
        This can be called directly if the m_enableItemViewable is disabled.
        m_enableItemViewable only controls whether double-clicking or an ENTER return
        will fire this event.*/
    //------------------------------------------------------
    void ListCtrlEx::ViewItem(const long selectedRow)
        {
        if ((GetWindowStyle() & wxLC_REPORT))
            {
            if (selectedRow == wxNOT_FOUND)
                {
                wxMessageBox(_(L"Please select an item in the list to view."), _(L"View Item"),
                             wxOK | wxICON_INFORMATION);
                return;
                }
            ListCtrlItemViewDlg dlg;
            FilePathResolverBase fileResolve;
            for (long i = 0; i < GetColumnCount(); ++i)
                {
                dlg.AddValue(GetColumnName(i), GetItemTextFormatted(selectedRow, i));
                }
            dlg.Create(this);
            dlg.ShowModal();
            }
        }

    //------------------------------------------------------
    long ListCtrlEx::FindColumn(const wchar_t* columnName) const
        {
        for (long i = 0; i < GetColumnCount(); ++i)
            {
            if (GetColumnName(i).CmpNoCase(columnName) == 0)
                {
                return i;
                }
            }
        return wxNOT_FOUND;
        }

    //------------------------------------------------------
    long ListCtrlEx::FindEx(const wchar_t* textToFind, const long startIndex /*= 0*/)
        {
        if (IsVirtual() && m_virtualData != nullptr)
            {
            return m_virtualData->Find(textToFind, startIndex);
            }
        else
            {
            return FindItem((startIndex == 0) ? -1 : startIndex, textToFind);
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::RemoveAll(const wxString& valueToRemove)
        {
        wxWindowUpdateLocker noUpdates(this);
        const long style = GetExtraStyle();
        SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
        long index = 0;
        while (index != wxNOT_FOUND)
            {
            index = FindEx(valueToRemove, index);
            if (index != wxNOT_FOUND)
                {
                DeleteItem(index);
                }
            else
                {
                break;
                }
            }
        SetExtraStyle(style);
        SetItemBeenEditedByUser(true);
        }

    //------------------------------------------------------
    void ListCtrlEx::DeleteSelectedItems()
        {
        long item = wxNOT_FOUND;
        const long firstSelected = GetNextItem(wxNOT_FOUND, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
        for (;;)
            {
            item = GetNextItem(item, wxLIST_NEXT_ALL, wxLIST_STATE_SELECTED);
            if (item == wxNOT_FOUND)
                {
                break;
                }
            if (m_enableFileDelete)
                {
                const wxString filePath = GetItemFilePath(item);
                if (filePath.length() && wxFile::Exists(filePath) && m_deletePrompt.length())
                    {
                    if (wxMessageBox(m_deletePrompt, _(L"Delete Item"),
                                     wxYES_NO | wxICON_WARNING) == wxYES)
                        {
                        SendToRecycleBinOrDelete(filePath);
                        DeleteItem(item--);
                        }
                    }
                }
            else
                {
                DeleteItem(item--);
                }
            Refresh();
            }

        // select item after the one that was deleted
        if (firstSelected < GetItemCount())
            {
            Select(firstSelected);
            }
        // if last item was deleted then step back
        else if (firstSelected > 0 && firstSelected - 1 < GetItemCount())
            {
            Select(firstSelected - 1);
            }
        Refresh();
        SetItemBeenEditedByUser(true);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnMultiColumSort([[maybe_unused]]
                                      wxCommandEvent &
                                      event)
        {
        if (GetColumnCount() == 0)
            {
            return;
            }
        wxArrayString columnChoices;
        for (long i = 0; i < GetColumnCount(); ++i)
            {
            columnChoices.Add(GetColumnName(i));
            }
        ListCtrlSortDlg dlg(this, columnChoices);
        dlg.FillSortCriteria(GetSortedColumns());
        dlg.SetHelpTopic(m_helpProjectPath, m_sortHelpTopic);
        if (dlg.ShowModal() == wxID_OK)
            {
            std::vector<std::pair<wxString, Wisteria::SortDirection>> columnsInfo =
                dlg.GetColumnsInfo();
            std::vector<std::pair<size_t, Wisteria::SortDirection>> columns;
            for (size_t i = 0; i < columnsInfo.size(); ++i)
                {
                long index = FindColumn(columnsInfo[i].first);
                if (index != wxNOT_FOUND)
                    {
                    columns.push_back(std::make_pair(index, columnsInfo[i].second));
                    }
                }
            SortColumns(columns);
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnKeyDown(wxKeyEvent & event)
        {
        // If the listctrl is editable and the user is doing a CTRL+V on the list then paste in
        // text from the clipboard as a new item
        if ((GetWindowStyle() & wxLC_EDIT_LABELS) && event.ControlDown() &&
            event.GetKeyCode() == L'V')
            {
            Paste();
            }
        else if (event.ControlDown() && event.GetKeyCode() == L'C')
            {
            Copy(true, false);
            }
        else if (event.ControlDown() && event.GetKeyCode() == L'A')
            {
            SelectAll();
            }
        // copy a specific column
        else if (event.ControlDown() &&
                 (event.GetKeyCode() == WXK_NUMPAD1 || event.GetKeyCode() == WXK_NUMPAD2 ||
                  event.GetKeyCode() == WXK_NUMPAD3 || event.GetKeyCode() == WXK_NUMPAD4 ||
                  event.GetKeyCode() == WXK_NUMPAD5 || event.GetKeyCode() == WXK_NUMPAD6 ||
                  event.GetKeyCode() == WXK_NUMPAD7 || event.GetKeyCode() == WXK_NUMPAD8 ||
                  event.GetKeyCode() == WXK_NUMPAD9))
            {
            int columnToCopy{ event.GetKeyCode() - WXK_NUMPAD1 };
            wxString selectedFormattedText;
            FormatToHtml(selectedFormattedText, false, ExportRowSelection::ExportSelected, 0, -1,
                         columnToCopy, columnToCopy, false, true);

            wxString selectedText;
            FormatToText(selectedText, ExportRowSelection::ExportSelected, 0, -1, columnToCopy,
                         columnToCopy, false);
            if (wxTheClipboard->Open())
                {
                // an empty cell should clear the clipboard
                wxTheClipboard->Clear();
                wxDataObjectComposite* obj = new wxDataObjectComposite();
                obj->Add(new wxHTMLDataObject(selectedFormattedText), true);
                obj->Add(new wxTextDataObject(selectedText));
                wxTheClipboard->SetData(obj);
                wxTheClipboard->Close();
                }
            }
        else if (event.ControlDown() && event.GetKeyCode() == WXK_INSERT && IsItemAddingEnabled())
            {
            EditItem(AddRow(), 0);
            }
        else if (IsItemDeletionEnabled() &&
                 (event.GetKeyCode() == WXK_DELETE || event.GetKeyCode() == WXK_NUMPAD_DELETE ||
                  event.GetKeyCode() == WXK_BACK))
            {
            if (m_deletePrompt.length())
                {
                if (wxMessageBox(m_deletePrompt, _(L"Delete Item"), wxYES_NO | wxICON_WARNING) ==
                    wxNO)
                    {
                    return;
                    }
                }
            DeleteSelectedItems();
            }
        else if ((GetWindowStyle() & wxLC_EDIT_LABELS) && event.GetKeyCode() == WXK_F2)
            {
            EditItem(GetFocusedItem(), 0);
            }
        else if ((event.GetKeyCode() == WXK_RETURN || event.GetKeyCode() == WXK_NUMPAD_ENTER) &&
                 m_enableItemViewable && (GetWindowStyle() & wxLC_REPORT))
            {
            ViewItem(GetFocusedItem());
            }
        // go to bottom or top row
        else if (event.ControlDown() && event.GetKeyCode() == WXK_DOWN && GetItemCount() > 0)
            {
            DeselectAll();
            EnsureVisible(GetItemCount() - 1);
            Select(GetItemCount() - 1);
            Focus(GetItemCount() - 1);
            }
        else if (event.ControlDown() && event.GetKeyCode() == WXK_UP && GetItemCount() > 0)
            {
            DeselectAll();
            EnsureVisible(0);
            Select(0);
            Focus(0);
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    class ListCtrlExPrintout : public wxPrintout
        {
      public:
        ListCtrlExPrintout(const ListCtrlEx* list, const wxString& title)
            : wxPrintout(title), m_list(list), m_lastRow(list ? list->GetItemCount() - 1 : -1),
              m_lastColumn(list ? list->GetColumnCount() - 1 : -1)
            {
            }

        /// @brief Column details.
        struct PrintColumnInfo
            {
            wxCoord m_width{ 0 };
            bool m_multiline{ false };
            bool m_included{ false };
            };

        /// @brief Page details.
        struct PrintPageInfo
            {
            PrintPageInfo(const std::vector<long>& rowStarts, const long rowsPerPage)
                : m_rowStarts(rowStarts), m_rowsPerPage(rowsPerPage)
                {
                }

            /// @returns The rows of data per page (does not include the header).
            [[nodiscard]]
            long GetRowsPerPage() const noexcept
                {
                return m_rowsPerPage;
                }

            std::vector<long> m_rowStarts;
            long m_rowsPerPage{ 0 };
            };

        bool HasPage(int pageNum) final
            {
            return (pageNum >= 1 && pageNum <= static_cast<int>(m_pageStarts.size()));
            }

        void GetPageInfo(int* minPage, int* maxPage, int* selPageFrom, int* selPageTo) final
            {
            *minPage = 1;
            *maxPage = static_cast<int>(m_pageStarts.size());
            *selPageFrom = 1;
            *selPageTo = static_cast<int>(m_pageStarts.size());
            }

        bool OnPrintPage(int page) final
            {
            if (HasPage(page))
                {
                m_currentPage = page;
                wxDC* dc = GetDC();
                if (dc)
                    {
                    dc->SetFont(m_list->GetFont());

                    double scaleX{ 0 }, scaleY{ 0 };
                    GetScreenToPageScaling(scaleX, scaleY);

                    // set a suitable scaling factor
                    const double scaleXReciprical = safe_divide<double>(1.0f, scaleX);
                    const double scaleYReciprical = safe_divide<double>(1.0f, scaleY);
                    dc->SetUserScale(scaleX, scaleY);

                    // get the size of the DC's drawing area in pixels
                    int drawingWidth, drawingHeight;
                    int dcWidth, dcHeight;
                    dc->GetSize(&dcWidth, &dcHeight);
                    dc->GetSize(&drawingWidth, &drawingHeight);
                    drawingWidth *= scaleXReciprical;
                    drawingHeight *= scaleYReciprical;

                    // let's have at least 10 device units margin
                    const auto marginX = GetMarginPadding();
                    const auto marginY = GetMarginPadding();

                    // remove the margins from the drawing area size
                    drawingWidth -= 2 * marginX;
                    int topMargin = marginY;
                    int bottomMargin = marginY;
                    // remove space for the headers and footers (if being used)
                    wxCoord textWidth{ 0 }, textHeight{ 0 };
                    wxCoord bodyStart = marginY;
                    dc->GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                    if (m_list->GetLeftPrinterHeader().length() ||
                        m_list->GetCenterPrinterHeader().length() ||
                        m_list->GetRightPrinterHeader().length())
                        {
                        topMargin += textHeight;
                        bodyStart += textHeight + marginY;
                        }
                    if (m_list->GetLeftPrinterFooter().length() ||
                        m_list->GetCenterPrinterFooter().length() ||
                        m_list->GetRightPrinterFooter().length())
                        {
                        bottomMargin += textHeight;
                        }
                    drawingHeight -= (topMargin + bottomMargin);

                    const auto drawTables = [this, page, marginX, &bodyStart](wxDC& drawDC)
                    {
                        // start drawing the table(s) on the page.
                        // if the list only has a few, narrow columns and the paper is wide, then
                        // we split the data to fit more on the page.
                        wxCoord currentTableOffset{ 0 };
                        const auto& currentPage = GetPagesInfo()[page - 1];
                        // draw table caption (title) if requested
                        if (IsIncludingTableCaption() && page == 1)
                            {
                            wxFont captionFont(drawDC.GetFont());
                            captionFont.SetPointSize(captionFont.GetPointSize() * 2);

                            GraphItems::Label caption(
                                GraphItems::GraphItemInfo(GetTitle())
                                    .Pen(wxNullPen)
                                    .DPIScaling(m_list->GetDPIScaleFactor())
                                    .Font(captionFont)
                                    .AnchorPoint(
                                        wxPoint(marginX, bodyStart + GetCellTopPadding())));
                            caption.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            caption.Draw(drawDC);
                            bodyStart += caption.GetBoundingBox(drawDC).GetHeight() +
                                         (GetCellTopPadding() * 2);
                            }
                        for (const auto currentPageTableRowStart : currentPage.m_rowStarts)
                            {
                            wxCoord currentX = marginX + currentTableOffset;
                            // draw the column headers' background
                            drawDC.SetBrush(wxColour(L"#337BC4"));
                            drawDC.DrawRectangle(currentX, bodyStart, GetTableWidth(),
                                                 GetCoulmnHeight());
                            // draw the cell borders
                            //-----------------
                            // horizontal lines
                            wxCoord currentY = bodyStart + GetCoulmnHeight();
                            for (auto i = currentPageTableRowStart;
                                 i <= /*bottom line*/ currentPageTableRowStart +
                                          currentPage.GetRowsPerPage() &&
                                 i <= GetLastRow() + 1 /*bottom line*/;
                                 ++i)
                                {
                                // Draw background color (if customized).
                                // (Don't consider "zebra-striped" rows as custom colors.)
                                if (i <= GetLastRow() && !m_list->GetAlternateRowColour().IsOk())
                                    {
                                    // get the row formatting
                                    const wxItemAttr* const virtualAttrib =
                                        m_list->OnGetItemAttr(i);
                                    const wxItemAttr rowAttributes =
                                        (m_list->IsVirtual() && virtualAttrib) ?
                                            *virtualAttrib :
                                            wxItemAttr(m_list->GetItemTextColour(i),
                                                       m_list->GetItemBackgroundColour(i),
                                                       m_list->GetItemFont(i));

                                    // fill cell background color
                                    if (rowAttributes.GetBackgroundColour().IsOk())
                                        {
                                        wxDCBrushChanger cellBCH(
                                            drawDC, rowAttributes.GetBackgroundColour());
                                        drawDC.DrawRectangle(currentX, currentY, GetTableWidth(),
                                                             GetLineHeight());
                                        }
                                    }

                                drawDC.DrawLine(currentX, currentY, currentX + GetTableWidth(),
                                                currentY);
                                currentY += GetLineHeight();
                                }
                            // vertical lines
                            drawDC.DrawLine(currentX, bodyStart, currentX,
                                            (currentY - GetLineHeight()));
                            for (auto j = 0; j < m_list->GetColumnCount(); ++j)
                                {
                                if (GetColumnsInfo()[j].m_included)
                                    {
                                    drawDC.DrawLine(currentX + GetColumnsInfo()[j].m_width,
                                                    bodyStart,
                                                    currentX + GetColumnsInfo()[j].m_width,
                                                    (currentY - GetLineHeight()));
                                    currentX += GetColumnsInfo()[j].m_width;
                                    }
                                }
                            // draw the column header text
                            drawDC.SetTextForeground(*wxWHITE);
                            currentX = marginX + GetCellSidePadding() + currentTableOffset;
                            for (auto columnCounter = 0; columnCounter < m_list->GetColumnCount();
                                 ++columnCounter)
                                {
                                if (GetColumnsInfo()[columnCounter].m_included)
                                    {
                                    drawDC.DrawText(m_list->GetColumnName(columnCounter), currentX,
                                                    bodyStart + GetCellTopPadding());
                                    currentX += GetColumnsInfo()[columnCounter].m_width;
                                    }
                                }
                            drawDC.SetTextForeground(*wxBLACK);
                            // draw the data
                            for (auto i = currentPageTableRowStart;
                                 i < currentPageTableRowStart + currentPage.GetRowsPerPage() &&
                                 i <= GetLastRow();
                                 ++i)
                                {
                                // draw the cells
                                currentX = marginX + GetCellSidePadding() + currentTableOffset;
                                for (auto j = 0; j < m_list->GetColumnCount(); ++j)
                                    {
                                    if (GetColumnsInfo()[j].m_included)
                                        {
                                        const auto yCoord =
                                            bodyStart + GetCoulmnHeight() + GetCellTopPadding() +
                                            (GetLineHeight() * (i - currentPageTableRowStart));
                                        const auto cellTextDrawingRect =
                                            wxRect(currentX, yCoord,
                                                   GetColumnsInfo()[j].m_width -
                                                       (2 * GetCellSidePadding()),
                                                   GetLineHeight() - (2 * GetCellTopPadding()) -
                                                       m_list->GetDPIScaleFactor() /*border*/);

                                        // draw cell icon (if there is one)
                                        if (m_list->GetImageList(wxIMAGE_LIST_SMALL))
                                            {
                                            wxListItem Item;
                                            Item.SetMask(wxLIST_MASK_IMAGE);
                                            Item.SetColumn(j);
                                            Item.SetId(i);
                                            if (m_list->GetItem(Item))
                                                {
                                                if (Item.GetImage() >= 0 &&
                                                    Item.GetImage() <
                                                        m_list->GetImageList(wxIMAGE_LIST_SMALL)
                                                            ->GetImageCount())
                                                    {
                                                    const auto bmp =
                                                        m_list->GetImageList(wxIMAGE_LIST_SMALL)
                                                            ->GetBitmap(Item.GetImage());
                                                    if (bmp.IsOk())
                                                        {
                                                        drawDC.DrawBitmap(
                                                            bmp, wxPoint(currentX, yCoord), true);
                                                        currentX +=
                                                            bmp.GetWidth() + GetCellSidePadding();
                                                        }
                                                    }
                                                }
                                            }

                                        // get the row formatting
                                        const wxItemAttr* const virtualAttrib =
                                            m_list->OnGetItemAttr(i);
                                        const wxItemAttr rowAttributes =
                                            (m_list->IsVirtual() && virtualAttrib) ?
                                                *virtualAttrib :
                                                wxItemAttr(m_list->GetItemTextColour(i),
                                                           m_list->GetItemBackgroundColour(i),
                                                           m_list->GetItemFont(i));
                                        // fill cell background color
                                        wxColour cellTextColor =
                                            rowAttributes.GetTextColour().IsOk() ?
                                                rowAttributes.GetTextColour() :
                                                *wxBLACK;

                                        // if no custom text color but a custom background is being
                                        // used, then set the font color for this cell to contrast
                                        // nicely against the background
                                        if (!m_list->GetAlternateRowColour().IsOk() &&
                                            rowAttributes.GetBackgroundColour().IsOk() &&
                                            !rowAttributes.GetTextColour().IsOk())
                                            {
                                            cellTextColor = (rowAttributes.GetBackgroundColour()
                                                                 .GetLuminance() < .5f) ?
                                                                *wxWHITE :
                                                                *wxBLACK;
                                            }

                                        wxDCTextColourChanger cellTextCCH(drawDC, cellTextColor);

                                        if (GetColumnsInfo()[j].m_multiline)
                                            {
                                            // if column text was too wide for the page,
                                            // then it needs to be split up and drawn as
                                            // multiline text
                                            GraphItems::Label label(
                                                GraphItems::GraphItemInfo(
                                                    m_list->GetItemTextFormatted(i, j))
                                                    .Pen(wxNullPen)
                                                    .DPIScaling(m_list->GetDPIScaleFactor())
                                                    .Font(drawDC.GetFont())
                                                    .AnchorPoint(wxPoint(currentX, yCoord)));
                                            label.SetLineSpacing(1 * m_list->GetDPIScaleFactor());
                                            label.SplitTextToFitBoundingBox(
                                                drawDC, cellTextDrawingRect.GetSize());
                                            label.SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                                            label.SetTextAlignment(TextAlignment::FlushLeft);
                                            label.Draw(drawDC);
                                            }
                                        else
                                            {
                                            drawDC.DrawText(m_list->GetItemTextFormatted(i, j),
                                                            currentX, yCoord);
                                            }
                                        currentX += GetColumnsInfo()[j].m_width;
                                        }
                                    }
                                }
                            currentTableOffset += GetTableWidth() + GetTablePadding();
                            }
                    };

                    const auto drawHeadersAndFooters = [this, marginX, marginY, drawingWidth,
                                                        drawingHeight, topMargin, &textWidth,
                                                        &textHeight](wxDC& drawDC)
                    {
                        // draw the headers
                        drawDC.SetDeviceOrigin(0, 0);
                        if (m_list->GetLeftPrinterHeader().length() ||
                            m_list->GetCenterPrinterHeader().length() ||
                            m_list->GetRightPrinterHeader().length())
                            {
                            if (m_list->GetLeftPrinterHeader().length())
                                {
                                drawDC.DrawText(ExpandPrintString(m_list->GetLeftPrinterHeader()),
                                                static_cast<wxCoord>(marginX),
                                                static_cast<wxCoord>(marginY / 2));
                                }
                            if (m_list->GetCenterPrinterHeader().length())
                                {
                                drawDC.GetTextExtent(
                                    ExpandPrintString(m_list->GetCenterPrinterHeader()), &textWidth,
                                    &textHeight);
                                drawDC.DrawText(
                                    ExpandPrintString(m_list->GetCenterPrinterHeader()),
                                    static_cast<wxCoord>(safe_divide<double>(drawingWidth, 2) -
                                                         safe_divide<double>(textWidth, 2)),
                                    static_cast<wxCoord>(marginY / 2));
                                }
                            if (m_list->GetRightPrinterHeader().length())
                                {
                                drawDC.GetTextExtent(
                                    ExpandPrintString(m_list->GetRightPrinterHeader()), &textWidth,
                                    &textHeight);
                                drawDC.DrawText(
                                    ExpandPrintString(m_list->GetRightPrinterHeader()),
                                    static_cast<wxCoord>(drawingWidth - (marginX + textWidth)),
                                    static_cast<wxCoord>(marginY / 2));
                                }
                            }
                        // draw the footers
                        if (m_list->GetLeftPrinterFooter().length() ||
                            m_list->GetCenterPrinterFooter().length() ||
                            m_list->GetRightPrinterFooter().length())
                            {
                            drawDC.GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                            // move down past the print header area,
                            // drawing (tables) area, and half the bottom margin
                            // (to center the footer vertically)
                            const wxCoord yPos = topMargin + drawingHeight + (marginY / 2);
                            if (m_list->GetLeftPrinterFooter().length())
                                {
                                drawDC.DrawText(ExpandPrintString(m_list->GetLeftPrinterFooter()),
                                                static_cast<wxCoord>(marginX), yPos);
                                }
                            if (m_list->GetCenterPrinterFooter().length())
                                {
                                drawDC.GetTextExtent(
                                    ExpandPrintString(m_list->GetCenterPrinterFooter()), &textWidth,
                                    &textHeight);
                                drawDC.DrawText(
                                    ExpandPrintString(m_list->GetCenterPrinterFooter()),
                                    static_cast<wxCoord>(safe_divide<double>(drawingWidth, 2) -
                                                         safe_divide<double>(textWidth, 2)),
                                    yPos);
                                }
                            if (m_list->GetRightPrinterFooter().length())
                                {
                                drawDC.GetTextExtent(
                                    ExpandPrintString(m_list->GetRightPrinterFooter()), &textWidth,
                                    &textHeight);
                                drawDC.DrawText(
                                    ExpandPrintString(m_list->GetRightPrinterFooter()),
                                    static_cast<wxCoord>((drawingWidth - (marginX + textWidth))),
                                    yPos);
                                }
                            }
                    };

                    // need to use wxGCDC for any color transparency
                    if (!m_printCanvas.IsOk() ||
                        m_printCanvas.GetSize() != wxSize(dcWidth, dcHeight))
                        {
                        m_printCanvas.Create(dcWidth, dcHeight);
                        }
                    wxMemoryDC memDc(m_printCanvas);
                    memDc.Clear();
                    wxGCDC gcdc(memDc);

                    drawTables(gcdc);
                    drawHeadersAndFooters(gcdc);
                    Wisteria::Canvas::DrawWatermarkLabel(
                        gcdc, wxRect(wxSize(drawingWidth, drawingHeight)), m_list->GetWatermark());
                    // copy renderings back into printer DC
                    dc->Blit(0, 0, dcWidth, dcHeight, &memDc, 0, 0);
                    memDc.SelectObject(wxNullBitmap);

                    return true;
                    }
                else
                    {
                    return false;
                    }
                }
            else
                {
                return false;
                }
            }

        /// @brief Calculates the pagination.
        void OnPreparePrinting() final
            {
            m_pageStarts.clear();
            m_columnWidths.clear();
            m_columnWidths.resize(m_list->GetColumnCount());
            m_currentPage = 0;
            m_columnHeight = 0;

            m_firstRow = std::clamp<long>(m_firstRow, 0, m_list->GetItemCount() - 1);
            m_lastRow = std::clamp<long>(m_lastRow, GetFirstRow(), m_list->GetItemCount() - 1);
            m_firstColumn = std::clamp<long>(m_firstColumn, 0, m_list->GetColumnCount() - 1);
            m_lastColumn =
                std::clamp<long>(m_lastColumn, GetFirstColumn(), m_list->GetColumnCount() - 1);

            // calculate lines per page and line height
            wxDC* dc = GetDC();
            if (dc)
                {
                dc->SetFont(m_list->GetFont());

                double scaleDownX{ 0 }, scaleDownY{ 0 };
                GetPageToScreenScaling(scaleDownX, scaleDownY);

                // get the size of the DC's drawing area in pixels
                wxCoord dcWidth{ 0 }, dcHeight{ 0 };
                dc->GetSize(&dcWidth, &dcHeight);
                dc->SetUserScale(safe_divide<double>(1.0f, scaleDownX),
                                 safe_divide<double>(1.0f, scaleDownX));

                const wxCoord drawingWidth = static_cast<wxCoord>(dcWidth * scaleDownX) -
                                             (2 * GetMarginPadding()) /*side margins*/;

                // measure a standard line of text
                wxCoord textWidth, textHeight;
                dc->GetTextExtent(L"A", &textWidth, &textHeight);

                // remove the margins from the drawing area size
                wxCoord heightMargin = GetMarginPadding() * 2;
                // remove space for the headers and footers (if being used)
                if (m_list->GetLeftPrinterHeader().length() ||
                    m_list->GetCenterPrinterHeader().length() ||
                    m_list->GetRightPrinterHeader().length())
                    {
                    heightMargin += textHeight + GetMarginPadding();
                    }
                if (m_list->GetLeftPrinterFooter().length() ||
                    m_list->GetCenterPrinterFooter().length() ||
                    m_list->GetRightPrinterFooter().length())
                    {
                    heightMargin += textHeight + GetMarginPadding();
                    }
                const wxCoord drawingHeight =
                    static_cast<wxCoord>(dcHeight * scaleDownY) - heightMargin;

                // calculate the widths of each column by finding each column's longest cell
                wxCoord cellTextWidth{ 0 }, cellTextHeigth{ 0 }, columnHeaderWidth{ 0 },
                    columnHeaderHeight{ 0 };
                wxString longestString;
                for (long columnCounter = 0; columnCounter < m_list->GetColumnCount();
                     ++columnCounter)
                    {
                    if (columnCounter < GetFirstColumn() || columnCounter > GetLastColumn())
                        {
                        m_columnWidths[columnCounter].m_included = false;
                        continue;
                        }
                    long longestCellText = 0;
                    for (long rowCounter = GetFirstRow(); rowCounter <= GetLastRow(); ++rowCounter)
                        {
                        const wxString cellText =
                            m_list->GetItemTextFormatted(rowCounter, columnCounter);
                        dc->GetTextExtent(cellText, &cellTextWidth, &cellTextHeigth);
                        longestCellText = std::max<long>(longestCellText, cellTextWidth);
                        if (cellText.length() > longestString.length())
                            {
                            longestString = cellText;
                            }
                        }

                    // if an image list, then add padding for any possible images in the cells
                    if (m_list->GetImageList(wxIMAGE_LIST_SMALL) &&
                        m_list->GetImageList(wxIMAGE_LIST_SMALL)->GetImageCount())
                        {
                        longestCellText +=
                            m_list->GetImageList(wxIMAGE_LIST_SMALL)->GetSize().GetWidth() +
                            GetCellSidePadding();
                        }
                    // now see if the column header is bigger than the data in the column
                    dc->GetMultiLineTextExtent(m_list->GetColumnName(columnCounter),
                                               &columnHeaderWidth, &columnHeaderHeight);
                    m_columnHeight =
                        std::max<long>(m_columnHeight, columnHeaderHeight + GetCellTopPadding());
                    m_columnWidths[columnCounter].m_width =
                        std::max<long>(columnHeaderWidth + (2 * GetCellSidePadding()),
                                       longestCellText + (2 * GetCellSidePadding()));
                    m_columnWidths[columnCounter].m_included = true;
                    }
                // are columns too wide to fit on the page?
                while (GetTableWidth() > drawingWidth)
                    {
                    // have we already adjusted all columns and the table is still too wide?
                    // we will need to just make everything fit evenly then
                    const size_t multilineColCount =
                        std::accumulate(m_columnWidths.cbegin(), m_columnWidths.cend(), 0,
                                        [](const wxCoord init, const auto& column)
                                        { return init + (column.m_multiline ? 1 : 0); });
                    if (multilineColCount == m_columnWidths.size())
                        {
                        const auto avgWidth = safe_divide<wxCoord>(
                            std::accumulate(m_columnWidths.cbegin(), m_columnWidths.cend(), 0,
                                            [](const wxCoord init, const auto& column)
                                            { return init + column.m_width; }),
                            m_columnWidths.size());
                        // apply average width to all columns
                        std::ranges::for_each(m_columnWidths, [avgWidth](auto& column)
                                              { column.m_width = avgWidth; });

                        // remeasure the height of the longest string from the table
                        // into the average cell's width, this will be our overall line height
                        GraphItems::Label measureLabel(GraphItems::GraphItemInfo(longestString)
                                                           .Font(dc->GetFont())
                                                           .DPIScaling(m_list->GetDPIScaleFactor())
                                                           .Pen(wxNullPen));
                        measureLabel.SetLineSpacing(1 * m_list->GetDPIScaleFactor());
                        measureLabel.SplitTextToFitBoundingBox(
                            *dc, wxSize(avgWidth - (2 * GetCellSidePadding()),
                                        (drawingHeight - GetCoulmnHeight()) -
                                            (2 * GetCellTopPadding())));

                        textHeight = measureLabel.GetBoundingBox(*dc).GetHeight();
                        break;
                        }

                    // grab the widest column and make it less wide
                    auto longestColumn = std::ranges::max_element(
                        m_columnWidths, [](const auto& a, const auto& b) noexcept
                        { return a.m_width < b.m_width; });
                    if (longestColumn != m_columnWidths.end())
                        {
                        // make the widest column a little more narrow, remeasure,
                        // and check things again
                        longestColumn->m_width *= .75f;
                        longestColumn->m_multiline = true;

                        // remeasure the height of the longest string from the table
                        // into the current cell's width, this will be our overall line height
                        GraphItems::Label measureLabel(GraphItems::GraphItemInfo(longestString)
                                                           .Font(dc->GetFont())
                                                           .DPIScaling(m_list->GetDPIScaleFactor())
                                                           .Pen(wxNullPen));
                        measureLabel.SetLineSpacing(1 * m_list->GetDPIScaleFactor());
                        measureLabel.SplitTextToFitBoundingBox(
                            *dc, wxSize(longestColumn->m_width - (2 * GetCellSidePadding()),
                                        (drawingHeight - GetCoulmnHeight()) -
                                            (2 * GetCellTopPadding())));

                        textHeight = measureLabel.GetBoundingBox(*dc).GetHeight();
                        }
                    }

                // 2 pixels around the text and a border
                // (border is 1 pixel because the rows share one border when adjacent row)
                m_lineHeight =
                    textHeight + GetCellTopPadding() * 2 + m_list->GetDPIScaleFactor() /*border*/;
                const long rowsPerPage = std::max<long>(
                    safe_divide<long>(drawingHeight - GetCoulmnHeight(), m_lineHeight), 1);
                const long tablesPerPage =
                    std::max<long>(static_cast<long>(std::floor(safe_divide<double>(
                                       drawingWidth, GetTableWidth() + GetTablePadding()))),
                                   1);
                // the caption will need enough space for twice the
                // font size as the rest of the data
                const auto linesNeededForTableCaption =
                    std::max<long>(static_cast<long>(std::ceil(safe_divide<double>(
                                       (dc->GetMultiLineTextExtent(GetTitle()).GetHeight() * 2) +
                                           (GetCellTopPadding() * 2),
                                       GetLineHeight()))),
                                   1);

                // now paginate
                long currentRow = GetFirstRow();
                // calculate which rows start each page
                // (and possibly each table, where the list is split to
                // fit more content across the paper)
                std::vector<long> rowStarts;
                while (currentRow <= GetLastRow())
                    {
                    rowStarts.clear();
                    const auto currentPageRows =
                        (IsIncludingTableCaption() && m_pageStarts.empty()) ?
                            rowsPerPage - linesNeededForTableCaption :
                            rowsPerPage;
                    for (long i = 0; i < tablesPerPage; ++i)
                        {
                        rowStarts.push_back(currentRow);
                        currentRow += currentPageRows;
                        if (currentRow > GetLastRow())
                            {
                            break;
                            }
                        }
                    m_pageStarts.push_back(PrintPageInfo(rowStarts, currentPageRows));
                    }
                }
            }

        /// @returns The number of pages.
        [[nodiscard]]
        size_t GetPageCount() const noexcept
            {
            return GetPagesInfo().size();
            }

        /// @returns The top / bottom padding inside the cells.
        [[nodiscard]]
        wxCoord GetCellTopPadding() const
            {
            return 2 * m_list->GetDPIScaleFactor();
            }

        /// @returns The left / right padding inside the cells.
        [[nodiscard]]
        wxCoord GetCellSidePadding() const
            {
            return 5 * m_list->GetDPIScaleFactor();
            }

        /// @returns The margin around the printing area.
        [[nodiscard]]
        wxCoord GetMarginPadding() const
            {
            return 10 * m_list->GetDPIScaleFactor();
            }

        /// @returns The space between tables
        ///     (when the content is split to fit more on the page).
        [[nodiscard]]
        wxCoord GetTablePadding() const
            {
            return 20 * m_list->GetDPIScaleFactor();
            }

        /// @returns The left and right padding inside the cells.
        [[nodiscard]]
        wxCoord GetMinColumnWidth() const
            {
            return (2 * GetCellSidePadding()) + (40 * m_list->GetDPIScaleFactor());
            }

        /// @returns The height of each line (includes cell padding).
        [[nodiscard]]
        long GetLineHeight() const noexcept
            {
            return m_lineHeight;
            }

        /// @returns The height of the column header area (includes cell padding).
        [[nodiscard]]
        long GetCoulmnHeight() const noexcept
            {
            return m_columnHeight;
            }

        /// @returns The columns' info (widths and columns that are multiline)
        [[nodiscard]]
        const std::vector<PrintColumnInfo>& GetColumnsInfo() const noexcept
            {
            return m_columnWidths;
            }

        /// @returns The pages' info (widths and columns that are multiline)
        [[nodiscard]]
        const std::vector<PrintPageInfo>& GetPagesInfo() const noexcept
            {
            return m_pageStarts;
            }

        /// @brief Sets the starting row to print.
        void SetFirstRow(const long first) noexcept { m_firstRow = first; }

        /// @returns The starting row to print.
        [[nodiscard]]
        long GetFirstRow() const noexcept
            {
            return m_firstRow;
            }

        /// @brief Sets the last row to print.
        void SetLastRow(const long last) noexcept { m_lastRow = last; }

        /// @returns The last row to print.
        [[nodiscard]]
        long GetLastRow() const noexcept
            {
            return m_lastRow;
            }

        /// @brief Sets the starting column to print.
        void SetFirstColumn(const long first) noexcept { m_firstColumn = first; }

        /// @returns The starting column to print.
        [[nodiscard]]
        long GetFirstColumn() const noexcept
            {
            return m_firstColumn;
            }

        /// @brief Sets the last column to print.
        void SetLastColumn(const long last) noexcept { m_lastColumn = last; }

        /// @returns The last column to print.
        [[nodiscard]]
        long GetLastColumn() const noexcept
            {
            return m_lastColumn;
            }

        /// @returns Whether a caption is printed above first page.
        [[nodiscard]]
        bool IsIncludingTableCaption() const noexcept
            {
            return m_includeTableCaption;
            }

        /// @brief Sets whether a caption should be printed above the first page.
        ///     (The label passed to the CTOR will be the caption.)
        void IncludeTableCaption(const bool include) noexcept { m_includeTableCaption = include; }

      private:
        /// Gets the scaling factor going from the page size to the screen size.
        /// This falls back to a 1:1 ratio upon failure.
        void GetScreenToPageScaling(double& scaleX, double& scaleY) const
            {
            int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
            GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
            GetPPIScreen(&ppiScreenX, &ppiScreenY);

            scaleX = safe_divide<double>(ppiPrinterX, ppiScreenX);
            scaleY = safe_divide<double>(ppiPrinterY, ppiScreenY);
            if (scaleX == 0)
                {
                scaleX = 1;
                }
            if (scaleY == 0)
                {
                scaleY = 1;
                }
            }

        void GetPageToScreenScaling(double& scaleX, double& scaleY) const
            {
            int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
            GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
            GetPPIScreen(&ppiScreenX, &ppiScreenY);

            scaleX = safe_divide<double>(ppiScreenX, ppiPrinterX);
            scaleY = safe_divide<double>(ppiScreenY, ppiPrinterY);
            if (scaleX == 0)
                {
                scaleX = 1;
                }
            if (scaleY == 0)
                {
                scaleY = 1;
                }
            }

        [[nodiscard]]
        wxString ExpandPrintString(const wxString& printString) const
            {
            wxString expandedString = printString;

            expandedString.Replace(
                L"@PAGENUM@",
                wxNumberFormatter::ToString(m_currentPage, 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep),
                true);
            expandedString.Replace(
                L"@PAGESCNT@",
                wxNumberFormatter::ToString(GetPageCount(), 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep),
                true);
            expandedString.Replace(L"@TITLE@", m_list->GetLabel(), true);
            expandedString.Replace(L"@USER@", wxGetUserName(), true);
            const wxDateTime now = wxDateTime::Now();
            expandedString.Replace(L"@DATE@", now.FormatDate(), true);
            expandedString.Replace(L"@TIME@", now.FormatTime(), true);

            return expandedString;
            }

        /// @returns The width of the drawn table (includes cell padding).
        [[nodiscard]]
        long GetTableWidth() const noexcept
            {
            return std::accumulate(m_columnWidths.cbegin(), m_columnWidths.cend(), 0,
                                   [](const wxCoord init, const auto& column)
                                   { return init + column.m_width; });
            }

        const ListCtrlEx* m_list{ nullptr };

        std::vector<PrintColumnInfo> m_columnWidths;
        long m_lineHeight{ 0 };
        long m_columnHeight{ 0 };
        long m_firstRow{ 0 };
        long m_lastRow{ 0 };
        long m_firstColumn{ 0 };
        long m_lastColumn{ 0 };

        bool m_includeTableCaption{ false };
        wxBitmap m_printCanvas;

        std::vector<PrintPageInfo> m_pageStarts;
        size_t m_currentPage{ 0 };
        };

    //------------------------------------------------------
    void ListCtrlEx::OnPrint([[maybe_unused]]
                             wxCommandEvent &
                             event)
        {
        ListCtrlExPrintout* printOut = new ListCtrlExPrintout(this, GetLabel());
#if defined(__WXMSW__) || defined(__WXOSX__)
        wxPrinterDC* dc = nullptr;
#else
        wxPostScriptDC* dc = nullptr;
#endif
        if (m_printData)
            {
#if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(*m_printData);
#else
            dc = new wxPostScriptDC(*m_printData);
#endif
            }
        else
            {
            wxPrintData pd;
#if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(pd);
#else
            dc = new wxPostScriptDC(pd);
#endif
            }
        printOut->SetDC(dc);

        wxPrinter printer;
        if (m_printData)
            {
            printer.GetPrintDialogData().SetPrintData(*m_printData);
            }
        printer.GetPrintDialogData().SetAllPages(true);
        printer.GetPrintDialogData().SetFromPage(1);
        printer.GetPrintDialogData().SetMinPage(1);
        printer.GetPrintDialogData().EnableSelection(false);
        if (!printer.Print(this, printOut, true))
            {
            // just show a message if a real error occurred. They may have just cancelled.
            if (printer.GetLastError() == wxPRINTER_ERROR)
                {
                wxMessageBox(_(L"An error occurred while printing.\n"
                               "Your default printer may not be set correctly."),
                             _(L"Print"), wxOK | wxICON_WARNING);
                }
            }
        if (m_printData)
            {
            *m_printData = printer.GetPrintDialogData().GetPrintData();
            }
        wxDELETE(printOut);
        wxDELETE(dc);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnPreview([[maybe_unused]]
                               wxCommandEvent &
                               event)
        {
            // note that previewing isn't done on macOS or GTK+ as it has its own native previewing
            // built into its print dialog
#if defined(__WXMSW__)
        ListCtrlExPrintout* printOut = new ListCtrlExPrintout(this, GetLabel());
        ListCtrlExPrintout* printOutForPrinting = new ListCtrlExPrintout(this, GetLabel());
        wxPrinterDC* dc = nullptr;
        wxPrinterDC* dc2 = nullptr;
        if (m_printData)
            {
            dc = new wxPrinterDC(*m_printData);
            dc2 = new wxPrinterDC(*m_printData);
            }
        else
            {
            wxPrintData pd;
            dc = new wxPrinterDC(pd);
            dc2 = new wxPrinterDC(pd);
            }
        printOut->SetDC(dc);
        printOutForPrinting->SetDC(dc2);

        wxPrintPreview* preview = new wxPrintPreview(printOut, printOutForPrinting, m_printData);
        preview->GetPrintDialogData().SetAllPages(true);
        preview->GetPrintDialogData().SetFromPage(1);
        preview->GetPrintDialogData().SetMinPage(1);
        preview->GetPrintDialogData().EnableSelection(false);
        if (!preview->IsOk())
            {
            wxDELETE(preview);
            wxDELETE(dc);
            wxDELETE(dc2);
            wxMessageBox(_(L"An error occurred while previewing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print Preview"), wxOK | wxICON_WARNING);
            return;
            }

        int x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };
        wxClientDisplayRect(&x, &y, &width, &height);
        wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                                   wxDefaultPosition, wxSize(width, height));

        frame->Centre(wxBOTH);
        frame->Initialize();
        frame->Show(true);

        delete dc;
        delete dc2;
#else
        wxFAIL_MSG(L"Print preview is Windows only!");
#endif
        }

    //------------------------------------------------------
    void ListCtrlEx::SetColumnImage(int col, int image)
        {
        wxListItem item;
        item.SetMask(wxLIST_MASK_IMAGE);
        item.SetImage(image);
        SetColumn(col, item);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnPaste([[maybe_unused]]
                             wxCommandEvent &
                             event)
        {
        Paste();
        }

    //------------------------------------------------------
    void ListCtrlEx::Paste()
        {
        if (wxTheClipboard->Open() && IsItemAddingEnabled())
            {
            if (wxTheClipboard->IsSupported(wxDF_TEXT))
                {
                wxTextDataObject data;
                wxTheClipboard->GetData(data);
                // split the string into separate lines if it has CRLFs in it
                wxStringTokenizer tkz(data.GetText(), L"\r\n", wxTOKEN_STRTOK);
                while (tkz.HasMoreTokens())
                    {
                    AddRow(tkz.GetNextToken());
                    }

                SetColumnWidth(0, EstimateColumnWidth(0));
                SetItemBeenEditedByUser(true);
                }
            wxTheClipboard->Close();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnCopyFirstColumn([[maybe_unused]]
                                       wxCommandEvent &
                                       event)
        {
        wxString selectedFormattedText;
        FormatToHtml(selectedFormattedText, false, ExportRowSelection::ExportSelected, 0, -1, 0, 0,
                     false, true);

        wxString selectedText;
        FormatToText(selectedText, ExportRowSelection::ExportSelected, 0, -1, 0, 0, false);
        if (wxTheClipboard->Open())
            {
            if (selectedText.length())
                {
                wxTheClipboard->Clear();
                wxDataObjectComposite* obj = new wxDataObjectComposite();
                obj->Add(new wxHTMLDataObject(selectedFormattedText), true);
                obj->Add(new wxTextDataObject(selectedText));
                wxTheClipboard->SetData(obj);
                }
            wxTheClipboard->Close();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnCopy([[maybe_unused]]
                            wxCommandEvent &
                            event)
        {
        Copy(true, false);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnCopyWithColumnHeaders([[maybe_unused]]
                                             wxCommandEvent &
                                             event)
        {
        Copy(true, true);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnCopyAll([[maybe_unused]]
                               wxCommandEvent &
                               event)
        {
        Copy(false, true);
        }

    //------------------------------------------------------
    void ListCtrlEx::Copy(const bool onlyIncludeSelectedRows, const bool includeColumnHeaders)
        {
        // note that if the SHIFT key is down, then only the first column will be copied
        wxString selectedFormattedText;
        FormatToHtml(selectedFormattedText, false,
                     onlyIncludeSelectedRows ? ExportRowSelection::ExportSelected :
                                               ExportRowSelection::ExportAll,
                     0, -1, 0, wxGetMouseState().ShiftDown() ? 0 : -1, includeColumnHeaders, true);

        wxString selectedText;
        FormatToText(selectedText,
                     onlyIncludeSelectedRows ? ExportRowSelection::ExportSelected :
                                               ExportRowSelection::ExportAll,
                     0, -1, 0, wxGetMouseState().ShiftDown() ? 0 : -1, includeColumnHeaders);
        if (wxTheClipboard->Open())
            {
            if (selectedText.length())
                {
                wxTheClipboard->Clear();
                wxDataObjectComposite* obj = new wxDataObjectComposite();
                obj->Add(new wxHTMLDataObject(selectedFormattedText), true);
                obj->Add(new wxTextDataObject(selectedText));
                wxTheClipboard->SetData(obj);
                }
            wxTheClipboard->Close();
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnContextMenu([[maybe_unused]]
                                   wxContextMenuEvent &
                                   event)
        {
        if (m_menu)
            {
            PopupMenu(m_menu);
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::DeselectAll()
        {
        wxWindowUpdateLocker noUpdates(this);
        const long style = GetExtraStyle();
        SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
        for (long i = 0; i < GetItemCount(); ++i)
            {
            if (IsSelected(i))
                {
                Select(i, false);
                }
            }
        SetExtraStyle(style);
        }

    //------------------------------------------------------
    void ListCtrlEx::OnSelectAll([[maybe_unused]]
                                 wxCommandEvent &
                                 event)
        {
        SelectAll();
        }

    //------------------------------------------------------
    void ListCtrlEx::SelectAll()
        {
        wxWindowUpdateLocker noUpdates(this);
        const long style = GetExtraStyle();
        SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
        for (long i = 0; i < GetItemCount(); ++i)
            {
            Select(i);
            }
        SetExtraStyle(style);
        }

    // resize the columns to fit their content
    //------------------------------------------------------
    void ListCtrlEx::DistributeColumns(const long maxColumnWidth /*= 300*/)
        {
        PROFILE();
        wxWindowUpdateLocker noUpdates(this);
        for (long i = 0; i < GetColumnCount(); ++i)
            {
            const long estimatedWidth = EstimateColumnWidth(i);
            // adjust columns that are too wide
            if ((maxColumnWidth != -1 && estimatedWidth > (maxColumnWidth * GetDPIScaleFactor())) ||
                estimatedWidth > GetSize().GetWidth())
                {
                SetColumnWidth(i, maxColumnWidth * GetDPIScaleFactor());
                }
            else
                {
                SetColumnWidth(i, estimatedWidth);
                }
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnFind(const wxFindDialogEvent& event)
        {
        const wxEventType type = event.GetEventType();
        long result = wxNOT_FOUND;

        bool matchCase = false;
        bool matchWholeWord = false;
        bool searchBackwards = false;

        const long currentlyFocusedItem = (GetFocusedItem() == -1) ? 0 : GetFocusedItem();

        if (type == wxEVT_COMMAND_FIND || type == wxEVT_COMMAND_FIND_NEXT)
            {
            const int flags = event.GetFlags();
            if (flags & wxFR_MATCHCASE)
                {
                matchCase = true;
                }

            if (flags & wxFR_WHOLEWORD)
                {
                matchWholeWord = true;
                }

            if (flags & wxFR_DOWN)
                {
                searchBackwards = false;
                }
            else
                {
                searchBackwards = true;
                }

            int compVal = -1;
            long i = 0;
            for (i = searchBackwards ? currentlyFocusedItem - 1 : currentlyFocusedItem + 1;
                 searchBackwards ? (i >= 0) : (i < GetItemCount()); searchBackwards ? --i : ++i)
                {
                for (long j = 0; j < GetColumnCount(); ++j)
                    {
                    if (matchWholeWord)
                        {
                        compVal = (matchCase == true) ?
                                      GetItemTextFormatted(i, j).Cmp(event.GetFindString()) :
                                      GetItemTextFormatted(i, j).CmpNoCase(event.GetFindString());
                        if (compVal != 0)
                            {
                            compVal = -1;
                            }
                        }
                    else
                        {
                        const bool matchResult =
                            (matchCase == true) ?
                                (std::wcsstr(GetItemTextFormatted(i, j).wc_str(),
                                             event.GetFindString()) != nullptr) :
                                (string_util::stristr<wchar_t>(GetItemTextFormatted(i, j),
                                                               event.GetFindString()) != nullptr);
                        compVal = matchResult ? 1 : -1;
                        }
                    if (compVal != -1)
                        {
                        break;
                        }
                    }
                if (compVal != -1)
                    {
                    break;
                    }
                }
            // if not found and searching down, then start from the beginning and try again
            if ((compVal == -1) && (flags & wxFR_DOWN) && (currentlyFocusedItem > 0))
                {
                for (i = 0; i < currentlyFocusedItem; ++i)
                    {
                    for (long j = 0; j < GetColumnCount(); ++j)
                        {
                        if (matchWholeWord)
                            {
                            compVal =
                                (matchCase == true) ?
                                    GetItemTextFormatted(i, j).Cmp(event.GetFindString()) :
                                    GetItemTextFormatted(i, j).CmpNoCase(event.GetFindString());
                            if (compVal != 0)
                                {
                                compVal = -1;
                                }
                            }
                        else
                            {
                            const bool matchResult =
                                (matchCase == true) ?
                                    (std::wcsstr(GetItemTextFormatted(i, j).wc_str(),
                                                 event.GetFindString()) != nullptr) :
                                    (string_util::stristr<wchar_t>(GetItemTextFormatted(i, j),
                                                                   event.GetFindString()) !=
                                     nullptr);
                            compVal = matchResult ? 1 : -1;
                            }
                        if (compVal != -1)
                            {
                            break;
                            }
                        }
                    if (compVal != -1)
                        {
                        break;
                        }
                    }
                }
            // if not found and searching up, then start from the end and
            // try again by going back up to where we started
            else if ((compVal == -1) && (flags ^ wxFR_DOWN) &&
                     (currentlyFocusedItem < GetItemCount() - 1))
                {
                for (i = GetItemCount() - 1; i > currentlyFocusedItem; --i)
                    {
                    for (long j = 0; j < GetColumnCount(); ++j)
                        {
                        if (matchWholeWord)
                            {
                            compVal =
                                (matchCase == true) ?
                                    GetItemTextFormatted(i, j).Cmp(event.GetFindString()) :
                                    GetItemTextFormatted(i, j).CmpNoCase(event.GetFindString());
                            if (compVal != 0)
                                {
                                compVal = -1;
                                }
                            }
                        else
                            {
                            const bool matchResult =
                                (matchCase == true) ?
                                    (std::wcsstr(GetItemTextFormatted(i, j).wc_str(),
                                                 event.GetFindString()) != nullptr) :
                                    (string_util::stristr<wchar_t>(GetItemTextFormatted(i, j),
                                                                   event.GetFindString()) !=
                                     nullptr);
                            compVal = matchResult ? 1 : -1;
                            }
                        if (compVal != -1)
                            {
                            break;
                            }
                        }
                    if (compVal != -1)
                        {
                        break;
                        }
                    }
                }

            if (compVal != -1)
                {
                result = i;
                DeselectAll();
                Select(i);
                Focus(i);
                }
            }
        else if (type == wxEVT_COMMAND_FIND_CLOSE)
            {
            return;
            }

        if (result == wxNOT_FOUND)
            {
            wxMessageDialog(this,
                            (type == wxEVT_COMMAND_FIND_NEXT) ?
                                _(L"No further occurrences found.") :
                                _(L"The text could not be found."),
                            _(L"Text Not Found"))
                .ShowModal();
            }
        }

    // resize the columns so that they fit the entire window evenly
    //------------------------------------------------------
    void ListCtrlEx::OnResize(wxSizeEvent & event)
        {
        if (GetColumnCount() == 1)
            {
            SetColumnWidth(0, event.GetSize().GetWidth() -
                                  wxSystemSettings::GetMetric(wxSYS_VSCROLL_X));
            }
        event.Skip();
        }

    //------------------------------------------------------
    void ListCtrlEx::OnColClick(const wxListEvent& event)
        {
        if (!IsSortable())
            {
            return;
            }
        // User clicked on header using left mouse button
        if (event.GetColumn() == GetSortedColumn())
            {
            m_sortedCols[0].second =
                (m_sortedCols[0].second == Wisteria::SortDirection::SortAscending) ?
                    Wisteria::SortDirection::SortDescending :
                    Wisteria::SortDirection::SortAscending;
            SetSortedColumn(event.GetColumn(), m_sortedCols[0].second);
            }
        else
            {
            SetSortedColumn(event.GetColumn(), Wisteria::SortDirection::SortAscending);
            }

        Resort();
        }

    //------------------------------------------------------
    void ListCtrlEx::SortColumn(const long nCol, const Wisteria::SortDirection direction)
        {
        if (!IsSortable())
            {
            return;
            }
        SetSortedColumn(nCol, direction);
        if (nCol < 0 || nCol >= GetColumnCount())
            {
            return;
            }
        SetCursor(*wxHOURGLASS_CURSOR);
        // freeze the window and also (temporarily) block
        // its events to optimize the sorting process
        wxWindowUpdateLocker noUpdates(this);
        const long style = GetExtraStyle();
        SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);
        if (IsVirtual() && m_virtualData != nullptr)
            {
            m_virtualData->Sort(nCol, direction, m_sortableRange.first, m_sortableRange.second);
            }
        else
            {
            SortTextItems(nCol, (direction == Wisteria::SortDirection::SortAscending),
                          m_sortableRange.first, m_sortableRange.second);
            }
#ifdef __WXMSW__
        // draw up or down arrow on sorted column
        if (GetSortedColumn() >= 0)
            {
            HWND hwndHdr = ListView_GetHeader(GetHWND());

            /* go through each column and reset the arrow flags and then
               set the arrow for the currently sorted columns*/
            for (long i = 0; i < GetColumnCount(); ++i)
                {
                HD_ITEM hdItem;
                std::memset(&hdItem, 0, sizeof(HD_ITEM));
                hdItem.mask = HDI_FORMAT;
                Header_GetItem(hwndHdr, i, &hdItem);
                hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                Header_SetItem(hwndHdr, i, &hdItem);
                }
            for (size_t i = 0; i < GetSortedColumns().size(); ++i)
                {
                HD_ITEM hdItem;
                std::memset(&hdItem, 0, sizeof(HD_ITEM));
                hdItem.mask = HDI_FORMAT;
                Header_GetItem(hwndHdr, GetSortedColumns().at(i).first, &hdItem);
                hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                hdItem.fmt |=
                    (GetSortedColumns().at(i).second == Wisteria::SortDirection::SortAscending) ?
                        HDF_SORTDOWN :
                        HDF_SORTUP;
                Header_SetItem(hwndHdr, GetSortedColumns().at(i).first, &hdItem);
                }
            }
#endif
        SetExtraStyle(style);
        SetCursor(wxNullCursor);
        }

    //------------------------------------------------------
    void ListCtrlEx::SortColumns(
        const std::vector<std::pair<size_t, Wisteria::SortDirection>>& columns)
        {
        PROFILE();
        if (columns.empty() || !IsSortable())
            {
            return;
            }

        if (!IsVirtual())
            {
            if (columns.size() == 1)
                {
                SortColumn(columns[0].first, columns[0].second);
                }
            else
                {
                wxFAIL_MSG(L"Listctrl must be virtual to support multi-column sorting.");
                }
            }
        else
            {
            SetSortedColumns(columns);

            SetCursor(*wxHOURGLASS_CURSOR);
            // freeze the window and also (temporarily) block
            // its events to optimize the sorting process
            wxWindowUpdateLocker noUpdates(this);
            const long style = GetExtraStyle();
            SetExtraStyle(style | wxWS_EX_BLOCK_EVENTS);

            m_virtualData->Sort(columns, m_sortableRange.first, m_sortableRange.second);
#ifdef __WXMSW__
            // draw up or down arrow on sorted column
            if (GetSortedColumn() >= 0)
                {
                HWND hwndHdr = ListView_GetHeader(GetHWND());

                /* go through each column and reset the arrow flags and
                   set the arrow for the currently sorted column(s)*/
                for (long i = 0; i < GetColumnCount(); ++i)
                    {
                    HD_ITEM hdItem;
                    std::memset(&hdItem, 0, sizeof(HD_ITEM));
                    hdItem.mask = HDI_FORMAT;
                    Header_GetItem(hwndHdr, i, &hdItem);
                    hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                    Header_SetItem(hwndHdr, i, &hdItem);
                    }
                for (size_t i = 0; i < GetSortedColumns().size(); ++i)
                    {
                    HD_ITEM hdItem;
                    std::memset(&hdItem, 0, sizeof(HD_ITEM));
                    hdItem.mask = HDI_FORMAT;
                    Header_GetItem(hwndHdr, GetSortedColumns().at(i).first, &hdItem);
                    hdItem.fmt &= ~(HDF_SORTDOWN | HDF_SORTUP);
                    hdItem.fmt |= (GetSortedColumns().at(i).second ==
                                   Wisteria::SortDirection::SortAscending) ?
                                      HDF_SORTDOWN :
                                      HDF_SORTUP;
                    Header_SetItem(hwndHdr, GetSortedColumns().at(i).first, &hdItem);
                    }
                }
#endif
            SetExtraStyle(style);
            Refresh();
            SetCursor(wxNullCursor);
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::Resort() { SortColumns(GetSortedColumns()); }

    //------------------------------------------------------
    void ListCtrlEx::CacheImageList(const int whichList)
        {
        m_encodedImages.clear();
        if (!GetImageList(whichList))
            {
            return;
            }
        for (auto i = 0; i < GetImageList(whichList)->GetImageCount(); ++i)
            {
            wxImage img(GetImageList(whichList)->GetBitmap(i).ConvertToImage());
            const auto tempFilePath = wxFileName::CreateTempFileName(L"RSI");
            if (tempFilePath.length() &&
                img.SaveFile(tempFilePath, wxBitmapType::wxBITMAP_TYPE_PNG))
                {
                    // map and unmap
                    {
                    MemoryMappedFile mf(tempFilePath, true, true);
                    if (mf.IsOk())
                        {
                        m_encodedImages.push_back(
                            wxString::Format(L"<img src='data:png;base64, %s'></img>",
                                             wxBase64Encode(mf.GetStream(), mf.GetMapSize())));
                        }
                    }
                wxRemoveFile(tempFilePath);
                }
            }
        }

    //------------------------------------------------------
    wxString ListCtrlEx::GetItemTextNonVirtual(long item, long column) const
        {
        wxListItem Item;
        Item.SetMask(wxLIST_MASK_TEXT);
        Item.SetColumn(column);
        Item.SetId(item);
        GetItem(Item);
        return Item.GetText();
        }

    //------------------------------------------------------
    bool ListCtrlEx::SortTextItems(const long nCol, const bool ascending, long low /*= 0*/,
                                   long high /*= -1*/)
        {
        if (GetItemCount() == 0)
            {
            return false;
            }
        if (nCol >= GetColumnCount())
            {
            return false;
            }

        if (high == -1 || high > GetItemCount() - 1)
            {
            high = GetItemCount() - 1;
            }

        long lo = low;
        long hi = high;

        if (hi <= lo)
            {
            return false;
            }

        const wxString midItem = GetItemTextEx((lo + hi) / 2, nCol);

        // loop through the list until indices cross
        int compRetVal = 0;
        while (lo <= hi)
            {
            /* find the first element that is greater than or equal to
               the partition element starting from the left Index.*/
            if (ascending)
                {
                if (IsVirtual())
                    {
                    compRetVal = m_virtualData->CompareItem(lo, nCol, midItem.wc_str());
                    }
                else
                    {
                    compRetVal = string_util::strnatordncasecmp(GetItemTextEx(lo, nCol).wc_str(),
                                                                midItem.wc_str());
                    }
                while ((lo < high) && (compRetVal < 0))
                    {
                    ++lo;
                    if (IsVirtual())
                        {
                        compRetVal = m_virtualData->CompareItem(lo, nCol, midItem.wc_str());
                        }
                    else
                        {
                        compRetVal = string_util::strnatordncasecmp(
                            GetItemTextEx(lo, nCol).wc_str(), midItem.wc_str());
                        }
                    }
                }
            else
                {
                if (IsVirtual())
                    {
                    compRetVal = m_virtualData->CompareItem(lo, nCol, midItem.wc_str());
                    }
                else
                    {
                    compRetVal = string_util::strnatordncasecmp(GetItemTextEx(lo, nCol).wc_str(),
                                                                midItem.wc_str());
                    }
                while ((lo < high) && (compRetVal > 0))
                    {
                    ++lo;
                    if (IsVirtual())
                        {
                        compRetVal = m_virtualData->CompareItem(lo, nCol, midItem.wc_str());
                        }
                    else
                        {
                        compRetVal = string_util::strnatordncasecmp(
                            GetItemTextEx(lo, nCol).wc_str(), midItem.wc_str());
                        }
                    }
                }

            /* find an element that is smaller than or equal to
               the partition element starting from the right Index.*/
            if (ascending)
                {
                if (IsVirtual())
                    {
                    compRetVal = m_virtualData->CompareItem(hi, nCol, midItem.wc_str());
                    }
                else
                    {
                    compRetVal = string_util::strnatordncasecmp(GetItemTextEx(hi, nCol).wc_str(),
                                                                midItem.wc_str());
                    }
                while ((hi > low) && (compRetVal > 0))
                    {
                    --hi;
                    if (IsVirtual())
                        {
                        compRetVal = m_virtualData->CompareItem(hi, nCol, midItem.wc_str());
                        }
                    else
                        {
                        compRetVal = string_util::strnatordncasecmp(
                            GetItemTextEx(hi, nCol).wc_str(), midItem.wc_str());
                        }
                    }
                }
            else
                {
                if (IsVirtual())
                    {
                    compRetVal = m_virtualData->CompareItem(hi, nCol, midItem.wc_str());
                    }
                else
                    {
                    compRetVal = string_util::strnatordncasecmp(GetItemTextEx(hi, nCol).wc_str(),
                                                                midItem.wc_str());
                    }
                while ((hi > low) && (compRetVal < 0))
                    {
                    --hi;
                    if (IsVirtual())
                        {
                        compRetVal = m_virtualData->CompareItem(hi, nCol, midItem.wc_str());
                        }
                    else
                        {
                        compRetVal = string_util::strnatordncasecmp(
                            GetItemTextEx(hi, nCol).wc_str(), midItem.wc_str());
                        }
                    }
                }

            // if the indexes have not crossed, swap if the items are not equal
            if (lo <= hi)
                {
                bool valuesDifferent = false;
                if (IsVirtual())
                    {
                    valuesDifferent = (m_virtualData->CompareItems(lo, nCol, hi, nCol) != 0);
                    }
                else
                    {
                    valuesDifferent =
                        (string_util::strnatordncasecmp(GetItemTextEx(lo, nCol).wc_str(),
                                                        GetItemTextEx(hi, nCol).wc_str()) != 0);
                    }
                // swap only if the items are not equal
                if (valuesDifferent)
                    {
                    // swap the rows
                    if (IsVirtual())
                        {
                        // get the selection, focused, and checked states of the rows
                        const int lowSelection =
                            GetItemState(lo, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        const int highSelection =
                            GetItemState(hi, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        const bool lowChecked = IsItemChecked(lo);
                        const bool highChecked = IsItemChecked(hi);

                        // swap the rows' text
                        m_virtualData->SwapRows(lo, hi);

                        // swap the rows' states
                        SetItemState(lo, highSelection,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        CheckItem(lo, highChecked);

                        SetItemState(hi, lowSelection,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        CheckItem(hi, lowChecked);
                        }
                    else
                        {
                        // hold all column text for one row (used for non-virtual control)
                        std::vector<wxString> rowText(GetColumnCount());

                        // get the selection, focused, and checked states
                        const int lowSelection =
                            GetItemState(lo, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        const int highSelection =
                            GetItemState(hi, wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        const bool lowChecked = IsItemChecked(lo);
                        const bool highChecked = IsItemChecked(hi);
                        wxItemAttr lowAttr, hiAttr;
                        lowAttr.SetTextColour(GetItemTextColour(lo));
                        hiAttr.SetTextColour(GetItemTextColour(hi));
                        lowAttr.SetBackgroundColour(GetItemBackgroundColour(lo));
                        hiAttr.SetBackgroundColour(GetItemBackgroundColour(hi));

                        wxListItem lvitemlo, lvitemhi;
                        long i;
                        for (i = 0; i < GetColumnCount(); ++i)
                            {
                            rowText[i] = GetItemTextEx(lo, i);
                            }

                        lvitemlo.SetColumn(0);
                        lvitemlo.SetId(lo);
                        lvitemlo.SetMask(lvitemhi.GetMask());

                        lvitemhi.SetColumn(0);
                        lvitemhi.SetId(hi);
                        lvitemhi.SetMask(lvitemlo.GetMask());

                        GetItem(lvitemlo);
                        GetItem(lvitemhi);

                        // swap the low item and set its focused state if focused beforehand
                        for (i = 0; i < GetColumnCount(); ++i)
                            {
                            SetItem(lo, i, GetItemTextEx(hi, i));
                            }

                        SetItemState(lo, highSelection,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        CheckItem(lo, highChecked);
                        SetRowAttributes(lo, hiAttr);

                        lvitemhi.SetId(lo);
                        SetItem(lvitemhi);

                        // swap the high item and set its focused state if focused before
                        for (i = 0; i < GetColumnCount(); ++i)
                            {
                            SetItem(hi, i, rowText[i]);
                            }
                        SetItemState(hi, lowSelection,
                                     wxLIST_STATE_SELECTED | wxLIST_STATE_FOCUSED);
                        CheckItem(hi, lowChecked);
                        SetRowAttributes(hi, lowAttr);

                        lvitemlo.SetId(hi);
                        SetItem(lvitemlo);
                        }
                    }
                ++lo;
                --hi;
                }
            }

        /* If the right index has not reached the left side of array
           must now sort the left partition.*/
        if (low < hi)
            {
            SortTextItems(nCol, ascending, low, hi);
            }

        /* If the left index has not reached the right side of array
           must now sort the right partition.*/
        if (lo < high)
            {
            SortTextItems(nCol, ascending, lo, high);
            }

        return true;
        }

    //------------------------------------------------------
    wxItemAttr* ListCtrlEx::OnGetItemAttr(long item) const
        {
        if (!IsVirtual() || m_virtualData == nullptr)
            {
            return nullptr;
            }
        else if (GetAlternateRowColour().IsOk())
            {
            return wxListCtrl::OnGetItemAttr(item);
            }
        else
            {
            return const_cast<wxItemAttr*>(m_virtualData->GetRowAttributes(item));
            }
        }

    //------------------------------------------------------
    wxString ListCtrlEx::OnGetItemText(long item, long column) const
        {
        /// List control (at least on Windows) truncates returned strings to 255 and forgets
        /// to null terminate them, so some nasty corruption appears at the end of long strings.
        /// Here we truncate it ourselves and put a nice little ellipsis at the end.
        wxString retval = GetItemTextFormatted(item, column);
        if (retval.length() >= 255)
            {
            retval.Truncate(254).Append(wchar_t{ 8230 });
            }
        return retval;
        }

    //------------------------------------------------------
    void ListCtrlEx::SetItemColumnImageEx(const long row, const long column, const int image)
        {
        if (!IsVirtual())
            {
            SetItemColumnImage(row, column, image);
            }
        else
            {
            m_virtualData->SetItemImage(row, column, image);
            }
        }

    //------------------------------------------------------
    wxString ListCtrlEx::GetItemTextFormatted(const long item, const long column) const
        {
        if (GetWindowStyle() & wxLC_REPORT)
            {
            if (IsVirtual() && m_virtualData != nullptr)
                {
                const wxString retVal = m_virtualData->GetItemTextFormatted(item, column);
                if (GetColumnFilePathTruncationMode(column) ==
                    ColumnInfo::ColumnFilePathTruncationMode::TruncatePaths)
                    {
                    return GetShortenedFilePath(retVal);
                    }
                else if (GetColumnFilePathTruncationMode(column) ==
                         ColumnInfo::ColumnFilePathTruncationMode::OnlyShowFileNames)
                    {
                    wxFileName fn(retVal);
                    // sometimes URLs look like directories and won't have a filename
                    return fn.GetFullName().empty() ? retVal : fn.GetFullName();
                    }
                else
                    {
                    return retVal;
                    }
                }
            else
                {
                return GetItemTextNonVirtual(item, column);
                }
            }
        // not report view, so this call makes no sense--return blank
        else
            {
            return wxString{};
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::OnSave([[maybe_unused]]
                            wxCommandEvent &
                            event)
        {
        wxArrayString choices, descriptions;
        choices.Add(_DT(L"HTML"));
        descriptions.Add(
            _DT(L"<span style='font-weight:bold;'>Hyper Text Markup Language</span><br />") +
            _(L"This format will include formatting and can be displayed in Internet browsers "
              "or most word-processing programs."));
        choices.Add(_DT(L"TXT"));
        descriptions.Add(wxString::Format(
            L"<span style='font-weight:bold;'>%s</span><br />%s", _(L"Text"),
            _(L"This format will write the list as a tab-delimited file with no formatting.")));
        choices.Add(_DT(L"LaTeX"));
        descriptions.Add(wxString::Format(
            L"<span style='font-weight:bold;'>%s</span><br />%s", _DT(L"<tt>LaTeX</tt>"),
            // TRANSLATORS: Do no translate "<tt>longtable{}</tt>" or <tt>LaTeX</tt>;
            // "longtable" is a LaTeX command that is not translated.
            _(L"This format will write the list in a <tt>longtable{}</tt> environment that can be "
              "included in a larger <tt>LaTeX</tt> document.")));
        RadioBoxDlg exportTypesDlg(this, _(L"Select List Format"), wxString{}, _(L"List formats:"),
                                   _(L"Export List"), choices, descriptions);
        if (exportTypesDlg.ShowModal() != wxID_OK)
            {
            return;
            }
        wxString fileFilter;
        switch (exportTypesDlg.GetSelection())
            {
        case 0:
            fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
            break;
        case 1:
            fileFilter = _(L"Text") + _DT(L" (*.txt)|*.txt");
            break;
        case 2:
            fileFilter = _DT(L"TeX (*.tex)|*.tex");
            break;
        default:
            fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
            };
        wxFileDialog dialog(this, _(L"Save As"), wxString{}, GetLabel(), fileFilter,
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_OK)
            {
            return;
            }

        wxFileName filePath = dialog.GetPath();
        // in case the extension is missing then use the selected filter
        if (filePath.GetExt().empty())
            {
            switch (exportTypesDlg.GetSelection())
                {
            case 0:
                filePath.SetExt(L"htm");
                break;
            case 1:
                filePath.SetExt(L"txt");
                break;
            case 2:
                filePath.SetExt(L"tex");
                break;
            default:
                filePath.SetExt(L"htm");
                };
            }

        const GridExportFormat exportFormat = (filePath.GetExt().CmpNoCase(L"HTM") == 0 ||
                                               filePath.GetExt().CmpNoCase(L"HTML") == 0) ?
                                                  GridExportFormat::ExportHtml :
                                              (filePath.GetExt().CmpNoCase(L"TEX") == 0) ?
                                                  GridExportFormat::ExportLaTeX :
                                                  GridExportFormat::ExportText;
        GridExportDlg exportOptionsDlg(GetParent(), GetItemCount(), GetColumnCount(), exportFormat);
        exportOptionsDlg.SetHelpTopic(m_helpProjectPath, m_exportHelpTopic);
        if (exportOptionsDlg.ShowModal() != wxID_OK)
            {
            return;
            }

        wxBusyCursor wait;
        Save(filePath, exportOptionsDlg.GetExportOptions());
        }

    //------------------------------------------------------
    void ListCtrlEx::FormatToText(
        wxString & outputText,
        const ExportRowSelection rowSelection /*= ExportRowSelection::ExportAll*/,
        long firstRow /*= 0*/, long lastRow /*= -1*/, long firstColumn /*= 0*/,
        long lastColumn /*= -1*/, const bool includeColumnHeader /*= true*/) const
        {
        outputText.clear();

        if (rowSelection == ExportRowSelection::ExportSelected && GetSelectedItemCount() == 0)
            {
            return;
            }
        // if saving only selected items, then go through the full range
        // (selected items will be distinguished as we go through everything)
        if (rowSelection == ExportRowSelection::ExportSelected)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            }
        // or if exporting all, then set the range to everything
        else if (rowSelection == ExportRowSelection::ExportAll)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            firstColumn = 0;
            lastColumn = GetColumnCount() - 1;
            }
        // otherwise, use the provided range

        // range check columns
        if (lastColumn < 0 || lastColumn >= GetColumnCount())
            {
            lastColumn = GetColumnCount() - 1;
            }
        if (firstColumn < 0)
            {
            firstColumn = 0;
            }
        if (firstColumn > lastColumn)
            {
            return;
            }
        // range check rows
        if (lastRow < 0 || lastRow >= GetItemCount())
            {
            lastRow = GetItemCount() - 1;
            }
        if (firstRow < 0)
            {
            firstRow = 0;
            }
        if (firstRow >= GetItemCount() || firstRow > lastRow)
            {
            return;
            }

        // allocate buffer to fit the cells that we are writing into it
        const long numberOfRows = (lastRow - firstRow) + 1;
        const long numberOfColumns = (lastColumn - firstColumn) + 1;
        // would always be the case, but check anyway
        if (numberOfRows > 0 && numberOfColumns > 0)
            {
            // let's say 5 for each cell and its surrounding delimiters
            outputText.reserve(numberOfRows * numberOfColumns * 5);
            }

        // format column headers
        if (includeColumnHeader)
            {
            wxListItem Item;
            Item.SetMask(wxLIST_MASK_TEXT);
            for (long i = firstColumn; i <= lastColumn; ++i)
                {
                GetColumn(i, Item);
                outputText += Item.GetText();
                if (i < lastColumn)
                    {
                    outputText += L"\t";
                    }
                }
            outputText += L"\n";
            }
        // format the data
        for (long i = firstRow; i <= lastRow; ++i)
            {
            if (rowSelection == ExportRowSelection::ExportSelected && !IsSelected(i))
                {
                continue;
                }
            for (long j = firstColumn; j <= lastColumn; ++j)
                {
                outputText += GetItemTextFormatted(i, j);
                if (j < lastColumn)
                    {
                    outputText += L"\t";
                    }
                }
            outputText += L"\n";
            }
        // trim off the trailing newline
        outputText.Trim();
        }

    //------------------------------------------------------
    void ListCtrlEx::FormatToHtml(
        wxString & outputText, bool usePrinterSettings,
        const ExportRowSelection rowSelection /*= ExportRowSelection::ExportAll*/,
        long firstRow /*= 0*/, long lastRow /*= -1*/, long firstColumn /*= 0*/,
        long lastColumn /*= -1*/, const bool includeColumnHeader /*= true*/,
        const bool formatAsStandAloneFile /*= false*/,
        const wxString& tableCaption /*= wxString{}*/) const
        {
        outputText.clear();

        // validate the input
        if (rowSelection == ExportRowSelection::ExportSelected && GetSelectedItemCount() == 0)
            {
            return;
            }
        // if saving only selected items, then go through the full range
        // (selected items will be distinguished as we go through everything)
        if (rowSelection == ExportRowSelection::ExportSelected)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            // can't paginate when using discontiguous rows
            usePrinterSettings = false;
            }
        // or if exporting all, then set the range to everything
        else if (rowSelection == ExportRowSelection::ExportAll)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            firstColumn = 0;
            lastColumn = GetColumnCount() - 1;
            }
        // otherwise, use the provided range

        // range check columns
        if (lastColumn < 0 || lastColumn >= GetColumnCount())
            {
            lastColumn = GetColumnCount() - 1;
            }
        if (firstColumn < 0)
            {
            firstColumn = 0;
            }
        if (firstColumn > lastColumn)
            {
            return;
            }
        // range check rows
        if (lastRow < 0 || lastRow >= GetItemCount())
            {
            lastRow = GetItemCount() - 1;
            }
        if (firstRow < 0)
            {
            firstRow = 0;
            }
        if (firstRow >= GetItemCount() || firstRow > lastRow)
            {
            return;
            }

        wxFont listFont = GetFont();
        ListCtrlExPrintout printOut(this, (tableCaption.length() ? tableCaption : GetLabel()));
        if (!m_printData)
            {
            usePrinterSettings = false;
            }
        else
            {
#if defined(__WXMSW__) || defined(__WXOSX__)
            wxPrinterDC dc(*m_printData);
#else
            wxPostScriptDC dc(*m_printData);
#endif
            printOut.SetFirstRow(firstRow);
            printOut.SetLastRow(lastRow);
            printOut.SetFirstColumn(firstColumn);
            printOut.SetLastColumn(lastColumn);
            printOut.IncludeTableCaption(tableCaption.length());
            printOut.SetUp(dc);
            printOut.OnPreparePrinting();
            listFont = dc.GetFont();
            }
        const wxString pageBreak = L"<div style='page-break-before:always'></div><br />\n";

        // allocate buffer to fit the cells that we are writing into it
        const long numberOfRows = (lastRow - firstRow) + 1;
        const long numberOfColumns = (lastColumn - firstColumn) + 1;
        if (numberOfRows > 0 && numberOfColumns > 0) // would always be the case, but check anyway
            {
            // assume that the average length for a cell is 5, and then throw in another 10
            // for all the HTML tags surrounding everything
            outputText.reserve(numberOfRows * numberOfColumns * 15);
            }

        if (formatAsStandAloneFile)
            {
            outputText = wxString::Format(
                L"<!DOCTYPE html>\n<html>\n<head>"
                "\n    <meta http-equiv='content-type' content='text/html; charset=UTF-8' />"
                "\n    <title>%s</title>"
                "\n</head>\n<body>",
                GetLabel());
            }

        const wxString tableStart =
            wxString::Format(L"\n<table border='1' style='font-family:%s; font-size:%dpt; "
                             "border-collapse:collapse;'>",
                             listFont.GetFaceName(), listFont.GetPointSize());
        const wxString tableEnd = L"\n</table>";

        lily_of_the_valley::html_encode_text encode;

        // format column widths (if printer settings are used)
        wxString colGroup = L"\n    <colgroup>";
        for (const auto& colInfo : printOut.GetColumnsInfo())
            {
            if (colInfo.m_included)
                {
                colGroup +=
                    wxString::Format(L"\n        <col style='width:%dpx'>",
                                     static_cast<int>(std::ceil(safe_divide<double>(
                                         colInfo.m_width + (2 * printOut.GetCellSidePadding()),
                                         GetDPIScaleFactor()))));
                }
            }
        colGroup += L"\n    </colgroup>";
        // format column headers (this will just be left blank if headers aren't being included)
        wxString columnHeader;
        if (includeColumnHeader)
            {
            wxListItem Item;
            Item.SetMask(wxLIST_MASK_TEXT);
            // format column headers
            columnHeader += L"\n    <thead><tr style='background:#337BC4; color:white;'>";
            for (long i = firstColumn; i <= lastColumn; ++i)
                {
                GetColumn(i, Item);
                wxString itemText = Item.GetText();
                if (encode.needs_to_be_encoded({ itemText.wc_str(), itemText.length() }))
                    {
                    itemText = encode({ itemText.wc_str(), itemText.length() }, true).c_str();
                    }
                columnHeader += wxString::Format(L"<td>%s</td>", itemText);
                }
            columnHeader += L"</tr></thead>";
            }

        wxString itemText;
        const auto formatRow = [this, &outputText, &encode, &itemText, firstColumn,
                                lastColumn](const long i, const int rowHeight = -1)
        {
            outputText += L"\n    <tr";
            // get the row formatting
            const wxItemAttr* const virtualAttrib = OnGetItemAttr(i);
            const wxItemAttr rowAttributes =
                (IsVirtual() && virtualAttrib) ?
                    *virtualAttrib :
                    wxItemAttr(GetItemTextColour(i), GetItemBackgroundColour(i), GetItemFont(i));
            wxString rowStyle =
                (rowHeight == -1) ? wxString{} : wxString::Format(L"height:%dpx;", rowHeight);
            // set the row's background color if customized and not the (browser) default white
            // (Don't consider "zebra-striped" rows as custom colors,
            // that looks odd exporting data like that)
            if (!GetAlternateRowColour().IsOk() && rowAttributes.GetBackgroundColour().IsOk() &&
                rowAttributes.GetBackgroundColour() != *wxWHITE)
                {
                rowStyle += wxString::Format(
                    _DT(L"background:%s;"),
                    rowAttributes.GetBackgroundColour().GetAsString(wxC2S_HTML_SYNTAX));
                }
            if (rowAttributes.GetFont().IsOk())
                {
                if (rowAttributes.GetFont().GetStrikethrough())
                    {
                    rowStyle += L"text-decoration:line-through;";
                    }
                if (rowAttributes.GetFont().GetWeight() == wxFONTWEIGHT_BOLD)
                    {
                    rowStyle += L"font-weight:bold;";
                    }
                if (rowAttributes.GetFont().GetUnderlined())
                    {
                    rowStyle += L"font-style:italic;";
                    }
                }
            if (rowStyle.length())
                {
                rowStyle = L" style='" + rowStyle + L"'>";
                }
            else
                {
                rowStyle = L">";
                }
            outputText += rowStyle;

            for (long j = firstColumn; j <= lastColumn; ++j)
                {
                itemText = GetItemTextFormatted(i, j);
                if (encode.needs_to_be_encoded({ itemText.wc_str(), itemText.length() }))
                    {
                    itemText = encode({ itemText.wc_str(), itemText.length() }, true).c_str();
                    }

                // see if there is an icon in front of the text (if there is an image list)
                if (m_encodedImages.size())
                    {
                    wxListItem Item;
                    Item.SetMask(wxLIST_MASK_IMAGE);
                    Item.SetColumn(j);
                    Item.SetId(i);
                    if (GetItem(Item))
                        {
                        if (Item.GetImage() >= 0 &&
                            static_cast<size_t>(Item.GetImage()) < m_encodedImages.size())
                            {
                            itemText.Prepend(m_encodedImages[Item.GetImage()] + L"&nbsp;");
                            }
                        }
                    }

                // Set the text color for the cell if customized and not the (browser) default
                // black. This has to be done at the cell level because color at the row level
                // changes the border color as well--we just want to change the text color.
                outputText += L"<td>";
                if (rowAttributes.GetTextColour().IsOk() &&
                    rowAttributes.GetTextColour() != *wxBLACK)
                    {
                    outputText += wxString::Format(
                        L"<span style='color:%s;'>%s</span>",
                        rowAttributes.GetTextColour().GetAsString(wxC2S_HTML_SYNTAX), itemText);
                    }
                // if no custom text color but a custom background is being used, then set the font
                // color for this cell to contrast nicely against the background
                else if (!GetAlternateRowColour().IsOk() &&
                         rowAttributes.GetBackgroundColour().IsOk() &&
                         !rowAttributes.GetTextColour().IsOk())
                    {
                    const auto cellTextColor =
                        (rowAttributes.GetBackgroundColour().GetLuminance() < .5f) ? *wxWHITE :
                                                                                     *wxBLACK;
                    outputText +=
                        wxString::Format(L"<span style='color:%s;'>%s</span>",
                                         cellTextColor.GetAsString(wxC2S_HTML_SYNTAX), itemText);
                    }
                else
                    {
                    outputText += itemText;
                    }
                outputText += L"</td>";
                }
            outputText += L"</tr>";
        };

        if (tableCaption.length())
            {
            outputText += wxString::Format(L"\n<div class='caption'>%s</div>", tableCaption);
            }

        // format the data
        if (usePrinterSettings)
            {
            for (size_t pageCounter = 0; pageCounter < printOut.GetPagesInfo().size();
                 ++pageCounter)
                {
                // if multiple tables on the same page, wrap them in a flex box
                if (printOut.GetPagesInfo()[pageCounter].m_rowStarts.size() > 1)
                    {
                    outputText += L"\n<div style='display:flex;'>";
                    }
                const auto& pageTable = printOut.GetPagesInfo()[pageCounter];
                for (size_t pageTableCounter = 0; pageTableCounter < pageTable.m_rowStarts.size();
                     ++pageTableCounter)
                    {
                    // last (or only) table on the page?
                    // Don't add the spacing after it, just wrap in a div
                    if (pageTable.m_rowStarts.size() == 1 ||
                        (pageTable.m_rowStarts.size() > 1 &&
                         pageTableCounter == pageTable.m_rowStarts.size() - 1))
                        {
                        outputText += L"\n<div>";
                        }
                    // padding between tables on the same page
                    else if (pageTable.m_rowStarts.size() > 1)
                        {
                        outputText +=
                            wxString::Format(L"\n<div style='padding-right:%dpx;'>",
                                             static_cast<int>(safe_divide<double>(
                                                 printOut.GetTablePadding(), GetDPIScaleFactor())));
                        }
                    // start next table (on the same page)
                    outputText += tableStart + colGroup + columnHeader;
                    for (long i = pageTable.m_rowStarts[pageTableCounter];
                         i < pageTable.m_rowStarts[pageTableCounter] +
                                 printOut.GetPagesInfo()[pageCounter].GetRowsPerPage() &&
                         i <= printOut.GetLastRow();
                         ++i)
                        {
                        // calculated line height will be scaled to the screen's DPI,
                        // so rescale it to pixel units that HTML output will need
                        formatRow(i, static_cast<int>(safe_divide<double>(printOut.GetLineHeight(),
                                                                          GetDPIScaleFactor())));
                        }
                    outputText += tableEnd + L"\n</div>";
                    }
                // end the page tables wrapper
                if (pageTable.m_rowStarts.size() > 1)
                    {
                    outputText += L"\n</div>";
                    }
                // add the page break (unless this is the last or only page)
                if (pageCounter != printOut.GetPagesInfo().size() - 1)
                    {
                    outputText += "\n" + pageBreak;
                    }
                }
            }
        else
            {
            outputText += tableStart + columnHeader;
            for (long i = firstRow; i <= lastRow; ++i)
                {
                if (rowSelection == ExportRowSelection::ExportSelected && !IsSelected(i))
                    {
                    continue;
                    }
                formatRow(i, -1);
                }
            outputText += tableEnd;
            }

        if (formatAsStandAloneFile)
            {
            outputText += L"\n</body>\n</html>";
            }

        outputText.Trim(true);
        outputText.Trim(false);
        }

    //------------------------------------------------------
    wxString ListCtrlEx::FormatToLaTeX(
        const ExportRowSelection rowSelection /*= ExportRowSelection::ExportAll*/,
        long firstRow /*= 0*/, long lastRow /*= -1*/, long firstColumn /*= 0*/,
        long lastColumn /*= -1*/, const bool includeColumnHeader /*= true*/,
        const wxString& tableCaption /*= wxString{}*/) const
        {
        wxString outputText;

        // validate the input
        if (rowSelection == ExportRowSelection::ExportSelected && GetSelectedItemCount() == 0)
            {
            return wxString{};
            }
        // if saving only selected items, then go through the full range
        // (selected items will be distinguished as we go through everything)
        if (rowSelection == ExportRowSelection::ExportSelected)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            }
        // or if exporting all, then set the range to everything
        else if (rowSelection == ExportRowSelection::ExportAll)
            {
            firstRow = 0;
            lastRow = GetItemCount() - 1;
            firstColumn = 0;
            lastColumn = GetColumnCount() - 1;
            }
        // otherwise, use the provided range

        // range check columns
        if (lastColumn < 0 || lastColumn >= GetColumnCount())
            {
            lastColumn = GetColumnCount() - 1;
            }
        if (firstColumn < 0)
            {
            firstColumn = 0;
            }
        if (firstColumn > lastColumn)
            {
            return wxString{};
            }
        // range check rows
        if (lastRow < 0 || lastRow >= GetItemCount())
            {
            lastRow = GetItemCount() - 1;
            }
        if (firstRow < 0)
            {
            firstRow = 0;
            }
        if (firstRow >= GetItemCount() || firstRow > lastRow)
            {
            return wxString{};
            }

        // allocate buffer to fit the cells that we are writing into it
        const long numberOfRows = (lastRow - firstRow) + 1;
        const long numberOfColumns = (lastColumn - firstColumn) + 1;
        if (numberOfRows > 0 && numberOfColumns > 0) // would always be the case, but check anyway
            {
            // assume that the average length for a cell is 5, and then throw in another 10
            // for all the HTML tags surrounding everything
            outputText.reserve(numberOfRows * numberOfColumns * 15);
            }

        const wxString tableStart = [this]()
        {
            wxString header = L"\\begin{longtable}{";
            for (int i = 0; i < GetColumnCount(); ++i)
                {
                header += L"|l";
                }
            return header + L"|}";
        }();
        const wxString tableEnd = L"\n\\end{longtable}\n";

        // format column headers (this will just be left blank if headers aren't being included)
        wxString columnHeader;
        if (includeColumnHeader)
            {
            wxListItem Item;
            Item.SetMask(wxLIST_MASK_TEXT);
            // format column headers
            columnHeader += L"\\hline ";
            for (long i = firstColumn; i <= lastColumn; ++i)
                {
                GetColumn(i, Item);
                wxString itemText = Item.GetText();
                /// @todo Needs LaTeX encoder
                // if (encode.needs_to_be_encoded({ itemText.wc_str(), itemText.length() }))
                //     { itemText = encode({ itemText.wc_str(), itemText.length() }, true).c_str();
                //     }
                columnHeader +=
                    wxString::Format(L"\\multicolumn{1}{|c|}{\\textbf{%s}} & ", itemText);
                }
            if (columnHeader.length() > 3)
                {
                columnHeader.erase(columnHeader.length() - 3);
                }
            columnHeader += L" \\\\ \\hline\n";
            }

        wxString itemText;
        const auto formatRow = [this, &outputText, &itemText, firstColumn, lastColumn](const long i)
        {
            for (long j = firstColumn; j <= lastColumn; ++j)
                {
                itemText = GetItemTextFormatted(i, j);
                /// @todo Needs LaTeX encoder
                // if (encode.needs_to_be_encoded({ itemText.wc_str(), itemText.length() }))
                //    { itemText = encode({ itemText.wc_str(), itemText.length() }, true).c_str(); }

                outputText += itemText + L" & ";
                }
            if (outputText.length() > 3)
                {
                outputText.erase(outputText.length() - 3);
                }
            outputText += L" \\\\\n";
        };

        // format the data
        outputText += tableStart;
        if (tableCaption.length())
            {
            outputText += wxString::Format(L"\n\\caption{%s} \\label{tab:long} \\\\", tableCaption);
            }
        outputText += L"\n" + columnHeader + L"\\endfirsthead\n\n";
        outputText +=
            wxString::Format(_DT(L"\\multicolumn{%d}{c}%%\n"
                                 "{{\\bfseries \\tablename\\ \\thetable{} %s}} \\\\\n"
                                 "%s"
                                 "\\endhead\n\n"
                                 "\\hline \\multicolumn{%d}{|r|}{{%s}} \\\\ \\hline\n"
                                 "\\endfoot\n\n"
                                 "\\hline\n"
                                 "\\endlastfoot\n\n"),
                             GetColumnCount(), _(L"-- continued from previous page"), columnHeader,
                             GetColumnCount(), _(L"Continued on next page"));
        for (long i = firstRow; i <= lastRow; ++i)
            {
            if (rowSelection == ExportRowSelection::ExportSelected && !IsSelected(i))
                {
                continue;
                }
            formatRow(i);
            }
        outputText += tableEnd;
        outputText.Trim();

        return outputText;
        }

    // Saves the list view's content as a table in an external format
    //------------------------------------------------------
    bool ListCtrlEx::Save(const wxFileName& path, GridExportOptions exportOptions)
        {
        if (exportOptions.m_exportSelected && GetSelectedItemCount() == 0)
            {
            wxMessageBox(_(L"You requested to export only selected items, "
                           "but no items are selected in the list."),
                         _(L"Export Error"), wxOK | wxICON_EXCLAMATION);
            return false;
            }
        // create the folder to the filepath, if necessary
        wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        const GridExportFormat exportFormat =
            (path.GetExt().CmpNoCase(L"HTM") == 0 || path.GetExt().CmpNoCase(L"HTML") == 0) ?
                GridExportFormat::ExportHtml :
            (path.GetExt().CmpNoCase(L"TEX") == 0) ? GridExportFormat::ExportLaTeX :
                                                     GridExportFormat::ExportText;

        if (exportOptions.m_toRow == -1)
            {
            exportOptions.m_toRow = GetItemCount();
            }
        if (exportOptions.m_toColumn == -1)
            {
            exportOptions.m_toColumn = GetColumnCount();
            }
        wxString outputText;

        if (exportFormat == GridExportFormat::ExportHtml)
            {
            FormatToHtml(outputText, exportOptions.m_pageUsingPrinterSettings,
                         exportOptions.m_exportAll      ? ExportRowSelection::ExportAll :
                         exportOptions.m_exportSelected ? ExportRowSelection::ExportSelected :
                                                          ExportRowSelection::ExportRange,
                         exportOptions.m_fromRow - 1, exportOptions.m_toRow - 1,
                         exportOptions.m_fromColumn - 1, exportOptions.m_toColumn - 1,
                         exportOptions.m_includeColumnHeaders, true);
            }
        else if (exportFormat == GridExportFormat::ExportLaTeX)
            {
            outputText =
                FormatToLaTeX(exportOptions.m_exportAll      ? ExportRowSelection::ExportAll :
                              exportOptions.m_exportSelected ? ExportRowSelection::ExportSelected :
                                                               ExportRowSelection::ExportRange,
                              exportOptions.m_fromRow - 1, exportOptions.m_toRow - 1,
                              exportOptions.m_fromColumn - 1, exportOptions.m_toColumn - 1,
                              exportOptions.m_includeColumnHeaders);
            }
        else
            {
            FormatToText(outputText,
                         exportOptions.m_exportAll      ? ExportRowSelection::ExportAll :
                         exportOptions.m_exportSelected ? ExportRowSelection::ExportSelected :
                                                          ExportRowSelection::ExportRange,
                         exportOptions.m_fromRow - 1, exportOptions.m_toRow - 1,
                         exportOptions.m_fromColumn - 1, exportOptions.m_toColumn - 1,
                         exportOptions.m_includeColumnHeaders);
            }

        wxFileName(path).SetPermissions(wxS_DEFAULT);
        wxFile file(path.GetFullPath(), wxFile::write);
        const bool retval = file.Write(outputText, wxConvUTF8);
        if (!retval)
            {
            wxMessageBox(wxString::Format(_(L"Failed to save list\n(%s)."), path.GetFullPath()),
                         _(L"Export Error"), wxOK | wxICON_EXCLAMATION);
            }
        return retval;
        }

    //------------------------------------------------------
    long ListCtrlEx::AddRow(const wxString& value /*= wxString{}*/)
        {
        if (IsVirtual())
            {
            SetVirtualDataSize(GetItemCount() + 1, GetColumnCount());
            const auto newRowIndex{ GetItemCount() - 1 };
            SetItemText(newRowIndex, 0, value);
            EnsureVisible(newRowIndex);
            SetItemBeenEditedByUser(true);
            return newRowIndex;
            }
        else
            {
            const auto newRowIndex = InsertItem(GetItemCount(), value);
            EnsureVisible(newRowIndex);
            SetItemBeenEditedByUser(true);
            return newRowIndex;
            }
        }

    //------------------------------------------------------
    void ListCtrlEx::EditItem(const long selectedRow, const long selectedColumn)
        {
        if (selectedRow == wxNOT_FOUND || selectedRow >= GetItemCount() ||
            selectedColumn >= GetColumnCount() || !(GetWindowStyle() & wxLC_EDIT_LABELS))
            {
            return;
            }
        wxRect itemRect;
        GetSubItemRect(selectedRow, selectedColumn, itemRect);

        const wxString currentItemText = GetItemTextEx(selectedRow, selectedColumn);
        // widen the edit control if the text is wider than the cell in the listctrl
#ifdef __WXMSW__
        const int textWidth = ListView_GetStringWidth(GetHWND(), currentItemText.wc_str());
#else
        wxClientDC dc(this);
        wxCoord textWidth{ 0 }, textHeight{ 0 };
        dc.GetTextExtent(currentItemText, &textWidth, &textHeight);
#endif
        itemRect.SetWidth(std::max<long>(textWidth, itemRect.GetWidth()));
        // add a little extra room for the borders
        itemRect.SetHeight(itemRect.GetHeight() + (4 * GetDPIScaleFactor()));
        if (GetColumnEditMode(selectedColumn).m_editMode == ColumnInfo::ColumnEditMode::NoEdit)
            {
            return;
            }
        else if (GetColumnEditMode(selectedColumn).m_editMode ==
                 ColumnInfo::ColumnEditMode::TextEdit)
            {
            // populate and show the edit control
            if (!m_editTextCtrl)
                {
                m_editTextCtrl = new ListEditTextCtrl(
                    this, this, wxID_ANY, currentItemText, wxPoint(itemRect.x, itemRect.y),
                    wxSize(itemRect.width, itemRect.height),
                    wxTE_PROCESS_TAB | wxTE_PROCESS_ENTER | wxBORDER_SUNKEN);
                m_editTextCtrl->SetCurrentItem(selectedRow, selectedColumn);
                }
            else
                {
                m_editTextCtrl->SetCurrentItem(selectedRow, selectedColumn);
                m_editTextCtrl->SetValue(currentItemText);
                m_editTextCtrl->Move(wxPoint(itemRect.x, itemRect.y));
                m_editTextCtrl->SetSize(wxSize(itemRect.width, itemRect.height));
                // move caret to end of the text
                m_editTextCtrl->SetSelection(currentItemText.length(), currentItemText.length());
                }

            Select(selectedRow, false);
            m_editTextCtrl->Show(true);
            m_editTextCtrl->SetFocus();
            }
        else if (GetColumnEditMode(selectedColumn).m_editMode ==
                 ColumnInfo::ColumnEditMode::IntegerEdit)
            {
            long initialValue{ static_cast<long>(
                GetColumnEditMode(selectedColumn).m_numericMinValue) };
            if (!currentItemText.ToLong(&initialValue))
                {
                initialValue = GetColumnEditMode(selectedColumn).m_numericMinValue;
                }
            if (!m_editSpinCtrl)
                {
                m_editSpinCtrl = new ListEditSpinCtrl(
                    this, this, wxID_ANY, wxString{}, wxPoint(itemRect.x, itemRect.y),
                    wxSize(itemRect.width, itemRect.height), wxSP_ARROW_KEYS,
                    static_cast<int>(GetColumnEditMode(selectedColumn).m_numericMinValue),
                    static_cast<int>(GetColumnEditMode(selectedColumn).m_numericMaxValue),
                    static_cast<int>(initialValue));
                m_editSpinCtrl->SetCurrentItem(selectedRow, selectedColumn);
                }
            else
                {
                m_editSpinCtrl->SetCurrentItem(selectedRow, selectedColumn);
                m_editSpinCtrl->SetRange(
                    static_cast<int>(GetColumnEditMode(selectedColumn).m_numericMinValue),
                    static_cast<int>(GetColumnEditMode(selectedColumn).m_numericMaxValue));
                m_editSpinCtrl->Move(wxPoint(itemRect.x, itemRect.y));
                m_editSpinCtrl->SetSize(wxSize(itemRect.width, itemRect.height));
                }
            Select(selectedRow, false);
            m_editSpinCtrl->SetValue(initialValue);
            m_editSpinCtrl->Show(true);
            m_editSpinCtrl->SetSelection(-1, -1);
            m_editSpinCtrl->SetFocus();
            }
        else if (GetColumnEditMode(selectedColumn).m_editMode ==
                 ColumnInfo::ColumnEditMode::DoubleEdit)
            {
            double initialValue{ GetColumnEditMode(selectedColumn).m_numericMinValue };
            if (!currentItemText.ToDouble(&initialValue))
                {
                initialValue = GetColumnEditMode(selectedColumn).m_numericMinValue;
                }
            if (!m_editSpinCtrlDouble)
                {
                m_editSpinCtrlDouble = new ListEditSpinCtrlDouble(
                    this, this, wxID_ANY, wxString{}, wxPoint(itemRect.x, itemRect.y),
                    wxSize(itemRect.width, itemRect.height), wxSP_ARROW_KEYS,
                    GetColumnEditMode(selectedColumn).m_numericMinValue,
                    GetColumnEditMode(selectedColumn).m_numericMaxValue, initialValue);
                m_editSpinCtrlDouble->SetCurrentItem(selectedRow, selectedColumn);
                }
            else
                {
                m_editSpinCtrlDouble->SetCurrentItem(selectedRow, selectedColumn);
                m_editSpinCtrlDouble->SetRange(GetColumnEditMode(selectedColumn).m_numericMinValue,
                                               GetColumnEditMode(selectedColumn).m_numericMaxValue);
                m_editSpinCtrlDouble->Move(wxPoint(itemRect.x, itemRect.y));
                m_editSpinCtrlDouble->SetSize(wxSize(itemRect.width, itemRect.height));
                }
            Select(selectedRow, false);
            m_editSpinCtrlDouble->SetValue(initialValue);
            m_editSpinCtrlDouble->Show(true);
            m_editSpinCtrlDouble->SetSelection(-1, -1);
            m_editSpinCtrlDouble->SetFocus();
            }
        else if (GetColumnEditMode(selectedColumn).m_editMode ==
                     ColumnInfo::ColumnEditMode::ComboBoxEdit ||
                 GetColumnEditMode(selectedColumn).m_editMode ==
                     ColumnInfo::ColumnEditMode::ComboBoxEditReadOnly)
            {
            if (m_editComboBox)
                {
                wxDELETE(m_editComboBox);
                }
            m_editComboBox = new ListEditComboBox(
                this, this, GetColumnEditMode(selectedColumn).m_selectableValues, wxID_ANY,
                wxString{}, wxPoint(itemRect.x, itemRect.y), wxDefaultSize,
                GetColumnEditMode(selectedColumn).m_editMode ==
                        ColumnInfo::ColumnEditMode::ComboBoxEditReadOnly ?
                    wxCB_DROPDOWN | wxCB_READONLY :
                    wxCB_DROPDOWN);
            Select(selectedRow, false);
            m_editComboBox->SetCurrentItem(selectedRow, selectedColumn);
            m_editComboBox->SetValue(currentItemText);
            /* combobox by default is the width of its largest string, but if that is smaller
               than the cell then make it fit the cell*/
            m_editComboBox->Show(true);
            if (m_editComboBox->GetSize().GetWidth() < itemRect.GetWidth())
                {
                m_editComboBox->SetSize(wxSize(itemRect.GetWidth(), itemRect.GetHeight() + 4));
                }
            m_editComboBox->Popup();
            m_editComboBox->SetFocus();
            }
        }

    //------------------------------------------------------
    long ListCtrlEx::EstimateColumnWidth(const long column)
        {
        wxClientDC dc(this);
        dc.SetFont(GetFont());
        wxCoord textWidth{ 0 }, textHeight{ 0 };
        // default to the column name's width
        dc.GetTextExtent(GetColumnName(column), &textWidth, &textHeight);
        // extra space for arrow icon if column is sorted
        long widestLabel = textWidth + (20 * GetDPIScaleFactor());

        // sample the widths of the first few items and see which is the longest
        for (long i = 0; i < 25 && i < GetItemCount(); ++i)
            {
            dc.GetTextExtent(GetItemTextFormatted(i, column), &textWidth, &textHeight);
            widestLabel = std::max<long>(widestLabel, textWidth);
            }
        // a little extra padding around the label
        return widestLabel + (20 * GetDPIScaleFactor());
        }
    } // namespace Wisteria::UI
