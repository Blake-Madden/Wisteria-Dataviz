/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __LISTCTRL_EX_H__
#define __LISTCTRL_EX_H__

#include <wx/busyinfo.h>
#include <wx/clipbrd.h>
#include <wx/dataobj.h>
#include <wx/dcbuffer.h>
#include <wx/dnd.h>
#include <wx/fdrepdlg.h>
#include <wx/file.h>
#include <wx/html/htmprint.h>
#include <wx/image.h>
#include <wx/imaglist.h>
#include <wx/mimetype.h>
#include <wx/paper.h>
#include <wx/print.h>
#include <wx/ribbon/buttonbar.h>
#include <wx/spinctrl.h>
#include <wx/tokenzr.h>
#include <wx/wupdlock.h>
#include <wx/wx.h>
#include <wx/xrc/xmlres.h>
#ifdef __WXMSW__
    #include <commctrl.h>
#endif
#include "../../base/canvas.h"
#include "../../math/mathematics.h"
#include "../../i18n-check/src/donttranslate.h"
#include "../../util/fileutil.h"
#include "../dialogs/gridexportdlg.h"
#include "../dialogs/listctrlitemviewdlg.h"
#include "listctrlexdataprovider.h"

#ifndef HDF_SORTUP
    #define HDF_SORTUP 0x400
#endif
#ifndef HDF_SORTDOWN
    #define HDF_SORTDOWN 0x200
#endif

/// @private
wxDECLARE_EVENT(wxEVT_LISTCTRLEX_EDITED, wxCommandEvent);

/// @private
#define EVT_LISTCTRLEX_EDITED(winid, fn) \
    wx__DECLARE_EVT1(wxEVT_LISTCTRLEX_EDITED, winid, wxCommandEventHandler(fn))

class ListCtrlEx;

/// @brief "Floating" combobox used to edit cells in the list control.
/// @private
class ListEditComboBox final : public wxComboBox
    {
public:
    ListEditComboBox(wxWindow* parent, ListCtrlEx* owner, const wxArrayString& choices,
                       wxWindowID id = wxID_ANY, const wxString& value = wxString{},
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxCB_DROPDOWN, const wxValidator& validator = wxDefaultValidator,
                       const wxString& name = L"ListEditComboBox") :
                            wxComboBox(parent, id, value, pos, size, choices, style, validator, name),
                            m_owner(owner), m_editedRow(wxNOT_FOUND), m_editedColumn(wxNOT_FOUND)
        {}
    void OnEndEditTextCtrl([[maybe_unused]] wxCommandEvent& event);
    void OnEndEditKillFocusTextCtrl(wxFocusEvent& event);
    void SetCurrentItem(const long row, const long column)
        {
        m_editedRow = row;
        m_editedColumn = column;
        }
    void Cancel()
        {
        m_editedRow = m_editedColumn = wxNOT_FOUND;
        Hide();
        }
    void OnKeyDown(wxKeyEvent& event)
        {
        if (event.GetKeyCode() == WXK_ESCAPE)
            { Cancel(); }
        else
            { event.Skip(); }
        }
private:
    ListCtrlEx* m_owner{ nullptr };
    long m_editedRow{ wxNOT_FOUND };
    long m_editedColumn{ wxNOT_FOUND };
    wxDECLARE_NO_COPY_CLASS(ListEditComboBox);
    wxDECLARE_EVENT_TABLE();
    };

/// @brief "Floating" spin control used to edit cells in the list control.
/// @private
class ListEditSpinCtrl final : public wxSpinCtrl
    {
public:
    ListEditSpinCtrl() = delete;
    ListEditSpinCtrl(wxWindow* parent, ListCtrlEx* owner, wxWindowID id = wxID_ANY, const wxString& value = wxString{},
                       const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
                       long style = wxSP_ARROW_KEYS, int Min = 1, int Max = 100, int initial = 1,
                       const wxString& name = L"ListEditSpinCtrl") :
                            wxSpinCtrl(parent, id, value, pos, size, style, Min, Max, initial, name),
                            m_owner(owner), m_editedRow(wxNOT_FOUND), m_editedColumn(wxNOT_FOUND)
        {
        Bind(wxEVT_KILL_FOCUS, &ListEditSpinCtrl::OnEndEditKillFocus, this, wxID_ANY);
        Bind(wxEVT_CHAR_HOOK, &ListEditSpinCtrl::OnKeyDown, this);
        }
    void OnEndEditKillFocus(wxFocusEvent& event);
    void SetCurrentItem(const long row, const long column)
        {
        m_editedRow = row;
        m_editedColumn = column;
        }
    void Cancel()
        {
        m_editedRow = m_editedColumn = wxNOT_FOUND;
        Hide();
        }
    void Accept();
    void OnKeyDown(wxKeyEvent& event);
private:
    ListCtrlEx* m_owner{ nullptr };
    long m_editedRow{ wxNOT_FOUND };
    long m_editedColumn{ wxNOT_FOUND };
    wxDECLARE_NO_COPY_CLASS(ListEditSpinCtrl);
    };

/// @brief "Floating" spin control used to edit cells in the list control (double values).
/// @private
class ListEditSpinCtrlDouble final : public wxSpinCtrlDouble
    {
public:
    ListEditSpinCtrlDouble() = delete;
    ListEditSpinCtrlDouble(wxWindow* parent, ListCtrlEx* owner,
        wxWindowID id = wxID_ANY, const wxString& value = wxString{},
        const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxDefaultSize,
        long style = wxSP_ARROW_KEYS, double Min = 1.0, double Max = 100.0,
        double initial = 1.0,
        const wxString& name = L"ListEditSpinCtrlDouble");
    void OnEndEditKillFocus(wxFocusEvent& event);
    void SetCurrentItem(const long row, const long column)
        {
        m_editedRow = row;
        m_editedColumn = column;
        }
    void Cancel()
        {
        m_editedRow = m_editedColumn = wxNOT_FOUND;
        Hide();
        }
    void Accept();
    void OnKeyDown(wxKeyEvent& event);
private:
    ListCtrlEx* m_owner{ nullptr };
    long m_editedRow{ wxNOT_FOUND };
    long m_editedColumn{ wxNOT_FOUND };
    wxDECLARE_NO_COPY_CLASS(ListEditSpinCtrlDouble);
    };

/// @brief "Floating" text control used to edit cells in the list control.
/// @private
class ListEditTextCtrl final : public wxTextCtrl
    {
public:
    ListEditTextCtrl(wxWindow* parent, ListCtrlEx* owner, wxWindowID id = wxID_ANY,
                       const wxString& value = wxString{},
                       const wxPoint& pos = wxDefaultPosition,
                       const wxSize& size = wxDefaultSize, long style = 0,
                       const wxValidator& validator = wxDefaultValidator,
                       const wxString& name = L"ListEditTextCtrl") :
                            wxTextCtrl(parent, id, value, pos, size,
                                style, validator, name),
                            m_owner(owner), m_editedRow(wxNOT_FOUND), m_editedColumn(wxNOT_FOUND)
        {}
    void OnEndEditTextCtrl([[maybe_unused]] wxCommandEvent& event);
    void OnEndEditKillFocusTextCtrl([[maybe_unused]] wxFocusEvent& event);
    void SetCurrentItem(const long row, const long column)
        {
        m_editedRow = row;
        m_editedColumn = column;
        }
    void Cancel()
        {
        m_editedRow = m_editedColumn = wxNOT_FOUND;
        Hide();
        }
    void OnKeyDown(wxKeyEvent& event)
        {
        if (event.GetKeyCode() == WXK_ESCAPE)
            { Cancel(); }
        else
            { event.Skip(); }
        }
private:
    ListCtrlEx* m_owner{ nullptr };
    long m_editedRow{ wxNOT_FOUND };
    long m_editedColumn{ wxNOT_FOUND };
    wxDECLARE_NO_COPY_CLASS(ListEditTextCtrl);
    wxDECLARE_EVENT_TABLE();
    };

/// @brief Helper for support to drop text into a list control.
/// @private
class DnDListCtrlEx final : public wxTextDropTarget
    {
public:
    explicit DnDListCtrlEx(wxListCtrl* owner) : m_owner(owner)
        {}
    bool OnDropText(wxCoord x, wxCoord y, const wxString& text) final
        {
        if (m_owner->IsVirtual() )
            { return false; }
        int flags = wxLIST_HITTEST_ONITEM;
        long position = m_owner->HitTest(wxPoint(x,y), flags);
        if (position == wxNOT_FOUND)
            { position = m_owner->GetItemCount(); }
        m_owner->InsertItem(position, text);
        return true;
        }
private:
    wxListCtrl* m_owner{ nullptr };
    wxDECLARE_NO_COPY_CLASS(DnDListCtrlEx);
    };

/** @brief An advanced version of ListCtrl, which has built-in support for sorting, exporting,
    and clipboard operations.
    @details This class is also designed to work with ListCtrlExDataProviderBase compatible
        interfaces for virtual functionality. In other words, a ListCtrlExDataProviderBase-derived
        class can easily be plugged into this control.
    @note Dialogs that use listcontrols with editing features (@sa SetColumnEditable()) need to
        call SetFocusIgnoringChildren() in its close functions to ensure that the value entered
        in the edit field gets applied.*/
class ListCtrlEx final : public wxListView
    {
    wxDECLARE_DYNAMIC_CLASS(ListCtrlEx);
    ListCtrlEx() = default;
public:
    /** @brief Constructor.
        @param parent The parent window.
        @param id The control's ID.
        @param pos The control's position.
        @param size The control's size.
        @param style The control's style.
        @param validator A validator for the control.*/
    ListCtrlEx(wxWindow *parent,
               const wxWindowID id,
               const wxPoint& pos = wxDefaultPosition,
               const wxSize& size = wxDefaultSize,
               long style = 0,
               const wxValidator& validator = wxDefaultValidator)
        : wxListView(parent, id, pos, size, style, validator, L"ListCtrlEx"),
        m_sortableRange(0, -1)
        {
        if (IsVirtual())
            { EnableAlternateRowColours(true); }
        }

    /// @private
    ListCtrlEx(const ListCtrlEx&) = delete;
    /// @private
    ListCtrlEx& operator=(const ListCtrlEx&) = delete;
    /// @private
    ~ListCtrlEx()
        {
        wxDELETE(m_menu);
        wxDELETE(m_editTextCtrl);
        wxDELETE(m_editSpinCtrl);
        wxDELETE(m_editSpinCtrlDouble);
        wxDELETE(m_editComboBox);
        }

    /// @brief Specifies whether double clicking an item will show it as a pop-up HTML report.
    /// @param enable @c true to enable.
    void EnableItemViewOnDblClick(const bool enable = true) noexcept
        { m_enableItemViewable = enable; }

    /// @brief Specifies whether items can be added by double clicking
    ///     (if wxLC_EDIT_LABELS is also enabled).
    /// @param enable Whether items can be added in edit mode.
    void EnableItemAdd(const bool enable = true) noexcept
        { m_enableItemAdd = enable; }
    /// @brief Indicates whether items can be added by the user.
    /// @returns @c true if items can be added by the user.
    [[nodiscard]]
    bool IsItemAddingEnabled() const noexcept
        { return m_enableItemAdd; }

    /// @brief Specifies whether items can be deleted.
    /// @param enable Whether items can be deleted.
    /// @param deletePrompt Option prompt that can be shown to the user when
    ///     an item is being deleted. Set this to wxString{} to suppress any prompting.
    void EnableItemDeletion(const bool enable = true,
                            const wxString deletePrompt = wxString{})
        {
        m_enableItemDelete = enable;
        m_deletePrompt = deletePrompt;
        }

    /// @brief Indicates whether items can be deleted.
    /// @returns @c true if items can be deleted.
    [[nodiscard]]
    bool IsItemDeletionEnabled() const noexcept
        { return m_enableItemDelete; }
    /// @brief Specifies whether deleting a row will also delete the file mentioned in that row.
    /// @param enable @c true to enable.
    void EnableFileDeletion(const bool enable = true)
        {
        m_enableFileDelete = enable;
        // turn on simple item deletion if asking for file deletion
        // and force prompting about deleting an item
        if (m_enableFileDelete)
            {
            m_enableItemDelete = true;
            m_deletePrompt = _(L"Do you wish to delete the selected file(s)?");
            }
        }

    /// @brief Specifies whether text from other applications can be dropped into the list.
    /// @param enable @c true to enable.
    void EnableDragAndDropText(const bool enable = true)
        { SetDropTarget(enable ? new DnDListCtrlEx(this) : nullptr); }

    /// @brief Specifies whether to show gridlines.
    /// @param enable @c true to enable.
    void EnableGridLines(const bool enable = true)
        {
        SetSingleStyle(wxLC_VRULES, enable);
        SetSingleStyle(wxLC_HRULES, enable);
        }

    /// @brief Specifies whether cells can be edited.
    /// @param enable @c true to enable.
    void EnableLabelEditing(const bool enable = true)
        { SetSingleStyle(wxLC_EDIT_LABELS, enable); }

    /// @brief Safe way to add a row to either a standard or virtual control.
    /// @param value The (optional) value to insert into the first column of the new row.
    /// @returns The index of the inserted item.
    long AddRow(const wxString& value = wxString{});
    /// @brief Enters a cell into edit mode
    /// @param selectedRow The row of the cell to edit.
    /// @param selectedColumn The column of the cell to edit.
    void EditItem(const long selectedRow, const long selectedColumn);

    /// @brief How to export the list.
    enum class ExportRowSelection
        {
        /// @brief All items.
        ExportAll,
        /// @brief Only selected items.
        ExportSelected,
        /// @brief A user-defined range.
        ExportRange
        };

    /// @private
    struct ColumnInfo
        {
        enum class ColumnEditMode
            {
            NoEdit,
            TextEdit,
            IntegerEdit,
            DoubleEdit,
            ComboBoxEdit,
            ComboBoxEditReadOnly
            };
        enum class ColumnFilePathTruncationMode
            {
            TruncatePaths,
            OnlyShowFileNames,
            NoTruncation,
            COLUMN_FILE_PATHS_TRUNCATION_MODE_COUNT
            };
        // CTORs
        ColumnInfo() = default;
        explicit ColumnInfo(const ColumnEditMode editMode) :
            m_editMode(editMode)
            {}
        template<typename T>
        ColumnInfo(const T Min, const T Max) :
            m_editMode(std::is_floating_point_v<T> ?
                ColumnEditMode::DoubleEdit : ColumnEditMode::IntegerEdit),
            m_numericMinValue(Min), m_numericMaxValue(Max)
            {}
        explicit ColumnInfo(const wxArrayString& choices, const bool readOnly = false) :
            m_editMode(readOnly ?
                ColumnEditMode::ComboBoxEditReadOnly : ColumnEditMode::ComboBoxEdit),
            m_selectableValues(choices)
            {}
        ColumnEditMode m_editMode{ ColumnEditMode::NoEdit };
        double m_numericMinValue{ 0 };
        double m_numericMaxValue{ 0 };
        wxArrayString m_selectableValues;
        ColumnFilePathTruncationMode m_truncateFilePathsMode
            { ColumnFilePathTruncationMode::NoTruncation };
        };
    /// @private
    ColumnInfo DefaultColumnInfo;

    /// @returns A column's edit mode.
    /// @param column The column to review.
    [[nodiscard]]
    const ColumnInfo& GetColumnEditMode(const long column) const
        {
        std::map<long, ColumnInfo>::const_iterator pos = m_columnInfo.find(column);
        if (pos != m_columnInfo.end())
            { return pos->second; }
        else
            { return DefaultColumnInfo; }
        }

    /** @brief Sets a column as editable.
        @param column The column to edit.
        @param enable @c true to make editable.*/
    void SetColumnEditable(const long column, const bool enable = true)
        {
        std::map<long,ColumnInfo>::iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            {
            m_columnInfo.insert(std::make_pair(column, ColumnInfo()));
            pos = m_columnInfo.find(column);
            }
        pos->second.m_editMode = enable ?
            ColumnInfo::ColumnEditMode::TextEdit :
            ColumnInfo::ColumnEditMode::NoEdit;
        if (enable)
            { EnableLabelEditing(); }
        }

    /** @brief For editable numeric columns, sets the range to constrain
            its values to.
        @param column The column to edit.
        @param Min The min value.
        @param Max The max value.*/
    template<typename T>
    void SetColumnNumericRange(const long column, const T Min, const T Max)
        {
        std::map<long,ColumnInfo>::iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            {
            m_columnInfo.insert(std::make_pair(column, ColumnInfo()));
            pos = m_columnInfo.find(column);
            }
        pos->second.m_numericMinValue = Min;
        pos->second.m_numericMaxValue = Max;
        pos->second.m_editMode = std::is_floating_point_v<T> ?
            ColumnInfo::ColumnEditMode::DoubleEdit :
            ColumnInfo::ColumnEditMode::IntegerEdit;
        }
    /** @brief For editable text columns, sets the selectable values.
        @param column The column to edit.
        @param choices The choices that the user can select (or edit).*/
    void SetColumnTextSelections(const long column, const wxArrayString& choices)
        {
        std::map<long,ColumnInfo>::iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            {
            m_columnInfo.insert(std::make_pair(column,ColumnInfo()));
            pos = m_columnInfo.find(column);
            }
        pos->second.m_selectableValues = choices;
        pos->second.m_editMode = ColumnInfo::ColumnEditMode::ComboBoxEdit;
        }
    /** @brief For editable (read-only) text columns, sets the selectable values.
        @param column The column to edit.
        @param choices The choices that the user can select.*/
    void SetColumnTextSelectionsReadOnly(const long column, const wxArrayString& choices)
        {
        std::map<long,ColumnInfo>::iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            {
            m_columnInfo.insert(std::make_pair(column,ColumnInfo()));
            pos = m_columnInfo.find(column);
            }
        pos->second.m_selectableValues = choices;
        pos->second.m_editMode = ColumnInfo::ColumnEditMode::ComboBoxEditReadOnly;
        }

    /** @brief Sets a column as truncation mode.
        @param column The column to edit.
        @param TruncMode The truncation mode for the column.*/
    void SetColumnFilePathTruncationMode(const long column,
        const ColumnInfo::ColumnFilePathTruncationMode TruncMode)
        {
        std::map<long,ColumnInfo>::iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            {
            m_columnInfo.insert(std::make_pair(column, ColumnInfo()));
            pos = m_columnInfo.find(column);
            }
        pos->second.m_truncateFilePathsMode = TruncMode;
        }
    /// @returns A column's truncation mode.
    /// @param column The column to review.
    [[nodiscard]]
    ColumnInfo::ColumnFilePathTruncationMode
        GetColumnFilePathTruncationMode(const long column) const
        {
        std::map<long, ColumnInfo>::const_iterator pos = m_columnInfo.find(column);
        if (pos == m_columnInfo.end())
            { return ColumnInfo::ColumnFilePathTruncationMode::NoTruncation; }
        else
            { return pos->second.m_truncateFilePathsMode; }
        }

    /// @private
    void OnMultiColumSort([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnKeyDown(wxKeyEvent& event);
    /// @private
    void OnSave([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnColClick(wxListEvent& event);
    /// @private
    void OnResize(wxSizeEvent& event);
    /// @private
    void OnSelectAll([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnCopy([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnCopyFirstColumn([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnCopyWithColumnHeaders([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnCopyAll([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnContextMenu([[maybe_unused]] wxContextMenuEvent& event);
    /// @private
    void OnFind(wxFindDialogEvent& event);
    /// @private
    void OnDeleteAllItems(wxListEvent& event);
    /// @private
    void OnDeleteItem(wxListEvent& event);
    /// @private
    void OnPreview([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnPrint([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnDblClick(wxMouseEvent& event);
    /// @private
    void OnViewItem([[maybe_unused]] wxCommandEvent& event);
    /// @private
    void OnPaste([[maybe_unused]] wxCommandEvent& event );
    /// @private
    void OnRibbonButton(wxRibbonButtonBarEvent& event);
    /// @private
    void OnIgnoreEvent(wxListEvent& event);

    /// @brief Resizes the columns evenly.
    /// @param maxColumnWidth The maximum width for all columns.\n
    ///     Set to @c -1 to not use a maximum width.
    void DistributeColumns(const long maxColumnWidth = 300);

    /// @brief Saves the list to a file.
    /// @param path The file path to save the list control's contents.
    /// @param exportOptions The export options (a ListCtrlExportOptions object).
    /// @returns @c true if successful.
    bool Save(const wxFileName& path, Wisteria::UI::GridExportOptions exportOptions);

    /// @brief Deletes all rows that are selected.
    void DeleteSelectedItems();

    /// @brief Copies the list data to the clipboard.
    /// @param onlyIncludeSelectedRows @c true to only copy selected rows.
    /// @param includeColumnHeaders @c true to include the column names.
    void Copy(const bool onlyIncludeSelectedRows, const bool includeColumnHeaders);
    /// @brief Pastes clipboard contents into the list (if it is editable).
    void Paste();

    /// @brief Selects all rows.
    void SelectAll();
    /// @brief Deselects all rows.
    void DeselectAll();

    /// @brief Finds the next instance of a given string.
    /// @details FindItem seems buggy with virtual controls. Also, with a virtual control, this
    ///     will compare the underlying value of the data instead of calling OnGetItemText().\n
    ///     For a virtual control, the text comparison method is controlled by the
    ///     virtual data provider.
    /// Note that this function does not wrap around, it simply goes to the end.
    /// @param textToFind The text to search for.
    /// @param startIndex The position to start the search.
    /// @returns The index of the row where the item was found, or wxNOT_FOUND if not found.
    [[nodiscard]]
    long FindEx(const wchar_t* textToFind, const long startIndex = 0);
    /// @brief Finds a column by name.
    /// @param columnName The column name to search for.
    /// @returns The index of column, if found; otherwise, wxNOT_FOUND if column name.
    [[nodiscard]]
    long FindColumn(const wchar_t* columnName);

    /// @brief Removes every row that matches the given string (first column of report view)
    /// @param valueToRemove The value to remove.
    void RemoveAll(const wxString& valueToRemove);

    /// @brief Displays the contents of a given row as an HTML report.
    /// @param selectedRow The row to display.
    void ViewItem(const long selectedRow);

    /// @brief Sets the size for data container for a virtual report view.
    /// @param rowSize The number of rows to resize to.
    /// @param columnSize The number of columns to resize to.
    void SetVirtualDataSize(size_t rowSize, size_t columnSize)
        {
        assert(IsVirtual() );
        if (m_virtualData)
            { m_virtualData->SetSize(rowSize, columnSize); }
        SetItemCount(static_cast<long>(rowSize));
        }

    /// @brief sets the size for data container for a virtual report view.
    /// @details This method will retain the number of current columns.
    /// @param rowSize The number of rows to resize to.
    void SetVirtualDataSize(size_t rowSize)
        {
        assert(IsVirtual() );
        if (m_virtualData)
            { m_virtualData->SetSize(rowSize); }
        SetItemCount(static_cast<long>(rowSize));
        }

    /// @brief Gets the name of the column at a given index.
    /// @param column The column index whose name is being retrieved.
    /// @returns The column's name.
    [[nodiscard]]
    wxString GetColumnName(const long column) const
        {
        if (column < 0 || column >= GetColumnCount())
            { return wxString{}; }
        wxListItem Item;
        Item.SetMask(wxLIST_MASK_TEXT);
        GetColumn(column, Item);
        return Item.GetText();
        }

    /// @brief Gets the text of the first column of the first selected row.
    /// @returns The text of the first column of the first selected row.\n
    ///     Returns empty string if nothing is selected.
    [[nodiscard]]
    wxString GetSelectedText() const
        {
        const long selected = GetFirstSelected();
        if (selected == wxNOT_FOUND)
            { return wxString{}; }
        else
            { return GetItemTextEx(selected, 0); }
        }

    /// Advanced version of GetItemText that supports getting it from a specific column.
    /// If using a custom text display interface, then call GetItemTextFormatted() instead.
    /// @param item The row to get text from.
    /// @param column The column to get text from.
    /// @returns The text from the specified cell.
    [[nodiscard]]
    inline wxString GetItemTextEx(const long item, const long column) const
        {
        if (GetWindowStyle() & wxLC_REPORT)
            {
            return (IsVirtual() ) ?
                wxString(m_virtualData->GetItemText(item, column)) :
                GetItemTextNonVirtual(item, column);
            }
        // not report view, so this call makes no sense--return blank
        else
            { return wxString{}; }
        }

    /// @brief Gets the text for a specified cell, but returns it using a custom
    ///     text display interface (plugged in by caller).
    /// @param item The row to get text from.
    /// @param column The column to get text from.
    /// @returns The text from the specified cell.
    [[nodiscard]]
    wxString GetItemTextFormatted(const long item, const long column) const;

    /// @brief Sets the text in a report view at the given cell.
    /// @param row The row to set text to.
    /// @param column The column to set text to.
    /// @param text The text to insert into the cell.
    /// @param format The NumberFormatInfo object to control how to display the text (optional).
    inline void SetItemText(const size_t row, const size_t column, const wxString& text,
                            const Wisteria::NumberFormatInfo format =
                                Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting)
        {
        if (IsVirtual() )
            { m_virtualData->SetItemText(row, column, text, format); }
        else
            { SetItem(static_cast<long>(row), static_cast<long>(column), text); }
        }

    /// @returns The Estimated width of a column based on the longest label in the first 10 rows.
    /// @details This is needed because wxLIST_AUTOSIZE doesn't work with virtual controls
    ///     on some platforms.
    /// @param column The column to measure.
    [[nodiscard]]
    long EstimateColumnWidth(const long column);

    /// @brief Sets the visual attributes of a cell.
    /// @param item The row to edit.
    /// @param attribs The attributes to apply.
    void SetRowAttributes(long item, const wxItemAttr& attribs)
        {
        if (IsVirtual() )
            { m_virtualData->SetRowAttributes(item, attribs); }
        else
            {
            SetItemTextColour(item, attribs.GetTextColour());
            SetItemBackgroundColour(item, attribs.GetBackgroundColour());
            SetItemFont(item, attribs.GetFont());
            }
        }

    /// @brief Sets the underlying data provider for a virtual list control.
    /// @param dataProvider The data provider for the list control.
    void SetVirtualDataProvider(ListCtrlExDataProviderBase* dataProvider) noexcept
        { m_virtualData = dataProvider; }

    /// @returns The underlying data provider for a virtual list control.
    [[nodiscard]]
    const ListCtrlExDataProviderBase* GetVirtualDataProvider() const noexcept
        { return m_virtualData; }

    /// @brief Sets whether clicking the list can be sorted.
    /// @param sortable @c true to make it sortable.
    void SetSortable(const bool sortable) noexcept
        { m_sortable = sortable; }
    /// @returns @c true if control is sortable.
    [[nodiscard]]
    bool IsSortable() const noexcept
        { return m_sortable; }
    /// @brief Sets the range of items that can be sorted when sort is called.
    /// @param low The starting row of the range that can be sorted.
    /// @param high The ending row of the range that can be sorted.\n
    ///     Set to @c -1 to make the range end on the last item in the list control.
    void SetSortableRange(const long low, const long high)
        {
        // adjust (bogus) negative values to zero
        m_sortableRange.first = std::max<long>(low, 0);
        // adjust bogus values (anything less than -1) to zero
        m_sortableRange.second = std::max<long>(high, -1);
        }
    /// @brief Gets the range of rows that will be sorted when a sort operation is called.
    /// @returns The range of sortable rows.
    [[nodiscard]]
    std::pair<long, long> GetSortableRange() const noexcept
        { return m_sortableRange; }
    /// @brief Sorts the list using the same criteria from the last recorded sort operation
    void Resort();
    /// @brief Sorts by the specified column.
    /// @param nCol The column to sort by.
    /// @param direction The direction to sort (e.g., SortAscending is smallest to largest).
    void SortColumn(const long nCol, const Wisteria::SortDirection direction);
    /// @brief Sorts by the specified columns.
    /// @details Sorts in the order of the columns, when a tie is encountered,
    ///     then the values from the next column specified are compared.
    /// @param columns The columns to sort by and their respective ordering.
    void SortColumns(const std::vector<std::pair<size_t, Wisteria::SortDirection>>& columns);
    /// @returns The index of the sorted column (or -1 if not sorted).
    [[nodiscard]]
    long GetSortedColumn() const
        {
        if (m_sortedCols.size())
            { return static_cast<long>(m_sortedCols[0].first); }
        else
            { return -1; }
        }

    /// @returns The indices of the sorted columns (or -1 if not sorted).
    [[nodiscard]]
    const std::vector<std::pair<size_t, Wisteria::SortDirection>>& GetSortedColumns() const noexcept
        { return m_sortedCols; }
    /** @brief Sets the sorted column.
        @param col The sorted column.
        @param direction The direction that it is sorted.*/
    void SetSortedColumn(const long col, const Wisteria::SortDirection direction)
        {
        m_sortedCols.resize(1);
        m_sortedCols[0].first = col;
        m_sortedCols[0].second = direction;
        }
    /** @brief Sets the sorted columns.
        @param columns The sorted columns and their directions.*/
    void SetSortedColumns(const std::vector<std::pair<size_t, Wisteria::SortDirection>>& columns)
        { m_sortedCols = columns; }

    /// @brief Assigns a context menu to this control and takes ownership of it.
    /// @param menu The menu to assign.
    void AssignContextMenu(wxMenu* menu) noexcept
        {
        delete m_menu;
        m_menu = menu;
        }
    /// @brief Sets the printer information for the list control.
    /// @param printData The printer information to use.
    void SetPrinterSettings(wxPrintData* printData) noexcept
        { m_printData = printData; }
    /// @brief Sets the left printer header.
    /// @param header The string to use for the left header.
    void SetLeftPrinterHeader(const wxString& header)
        { m_leftPrinterHeader = header; }
    /// @returns The left printer header.
    [[nodiscard]]
    const wxString& GetLeftPrinterHeader() const noexcept
        { return m_leftPrinterHeader; }

    /// @brief Sets the center printer header.
    /// @param header The string to use for the center header.
    void SetCenterPrinterHeader(const wxString& header)
        { m_centerPrinterHeader = header; }
    /// @returns The center printer header.
    [[nodiscard]]
    const wxString& GetCenterPrinterHeader() const noexcept
        { return m_centerPrinterHeader; }

    /// @brief Sets the right printer header.
    /// @param header The string to use for the right header.
    void SetRightPrinterHeader(const wxString& header)
        { m_rightPrinterHeader = header; }
    /// @returns The right printer header.
    [[nodiscard]]
    const wxString& GetRightPrinterHeader() const noexcept
        { return m_rightPrinterHeader; }

    /// @brief Sets the left printer footer.
    /// @param footer The string to use for the left footer.
    void SetLeftPrinterFooter(const wxString& footer)
        { m_leftPrinterFooter = footer; }
    /// @returns The left printer footer.
    [[nodiscard]]
    const wxString& GetLeftPrinterFooter() const noexcept
        { return m_leftPrinterFooter; }

    /// @brief Sets the center printer footer.
    /// @param footer The string to use for the center footer.
    void SetCenterPrinterFooter(const wxString& footer)
        { m_centerPrinterFooter = footer; }
    /// @returns The center printer footer.
    [[nodiscard]]
    const wxString& GetCenterPrinterFooter() const noexcept
        { return m_centerPrinterFooter; }

    /// @brief Sets the right printer footer.
    /// @param footer The string to use for the right footer.
    void SetRightPrinterFooter(const wxString& footer)
        { m_rightPrinterFooter = footer; }
    /// @returns The right printer footer.
    [[nodiscard]]
    const wxString& GetRightPrinterFooter() const noexcept
        { return m_rightPrinterFooter; }

    /// @brief Sets the watermark for the list when printed.
    /// @param watermark The watermark information.
    void SetWatermark(const Wisteria::Canvas::Watermark& watermark) noexcept
        { m_waterMark = watermark; }
    /// @returns The printer watermark.
    [[nodiscard]]
    const Wisteria::Canvas::Watermark& GetWatermark() const noexcept
        { return m_waterMark; }

    /// @brief Formats the list control's contents to HTML.
    /// @param[out] outputText The text buffer to write to.
    /// @param usePrinterSettings Whether to format the output the same as if being printed.\n
    ///     This will insert page breaks, resize columns, and possibly split
    ///     narrow tables to fit more of them on one page.\n
    ///     This will be ignored if no printer settings are connected to the control.
    /// @param rowSelection Specifies how rows and columns should be exported.
    /// @param firstRow The starting row of the range to format. Defaults to @c 0.
    /// @param lastRow The ending row of the range to format.\n
    ///     Defaults to @c -1, which specifies the last row.
    /// @param firstColumn The starting column of the range to format. Defaults to @c 0.
    /// @param lastColumn The ending column of the range to format.
    ///     Defaults to @c -1, which specifies the last column.
    /// @param includeColumnHeader Specifies whether to include the column headers,
    ///     which will be the first row.
    /// @param formatAsStandAloneFile Specifies whether this HTML is going to
    ///     represent a stand-alone file. If true, a document declaration is added to the top.
    /// @param tableCaption A caption to be drawn above the data.\n
    ///     Will be inside of a div with class "caption" that can be customized via CSS.
    void FormatToHtml(wxString& outputText,
                      bool usePrinterSettings,
                      const ExportRowSelection rowSelection = ExportRowSelection::ExportAll,
                      long firstRow = 0,
                      long lastRow = -1,
                      long firstColumn = 0,
                      long lastColumn = -1,
                      const bool includeColumnHeader = true,
                      const bool formatAsStandAloneFile = false,
                      const wxString& tableCaption = wxString{}) const;
    /// @brief Formats the list control's contents to LaTeX.
    /// @param rowSelection Specifies how rows and columns should be exported.
    /// @param firstRow The starting row of the range to format. Defaults to @c 0.
    /// @param lastRow The ending row of the range to format.\n
    ///     Defaults to @c -1, which specifies the last row.
    /// @param firstColumn The starting column of the range to format. Defaults to @c 0.
    /// @param lastColumn The ending column of the range to format.
    ///     Defaults to @c -1, which specifies the last column.
    /// @param includeColumnHeader Specifies whether to include the column headers,
    ///     which will be the first row.
    /// @param tableCaption A caption to be drawn above the data.\n
    ///     Will be inside of a div with class "caption" that can be customized via CSS.
    /// @returns The list control's content, formatted as LaTeX.
    [[nodiscard]]
    wxString FormatToLaTeX(const ExportRowSelection rowSelection = ExportRowSelection::ExportAll,
                           long firstRow = 0,
                           long lastRow = -1,
                           long firstColumn = 0,
                           long lastColumn = -1,
                           const bool includeColumnHeader = true,
                           const wxString& tableCaption = wxString{}) const;
    /// @brief Formats the list control's contents to tab delimited text.
    /// @param[out] outputText The text buffer to write to.
    /// @param rowSelection Specifies how rows and columns should be exported.
    /// @param firstRow The starting row of the range to format. Defaults to @c 0.
    /// @param lastRow The ending row of the range to format.
    ///     Defaults to @c -1, which specifies the last row.
    /// @param firstColumn The starting column of the range to format. Defaults to @c 0.
    /// @param lastColumn The ending column of the range to format.
    ///     Defaults to @c -1, which specifies the last column.
    /// @param includeColumnHeader Specifies whether to include the column headers,
    ///     which will be the first row.
    void FormatToText(wxString& outputText,
                      const ExportRowSelection rowSelection = ExportRowSelection::ExportAll,
                      long firstRow = 0,
                      long lastRow = -1,
                      long firstColumn = 0,
                      long lastColumn = -1,
                      const bool includeColumnHeader = true) const;
    /// @brief Sets the image for a column (shown in the header)
    /// @param col The column to set the image to.
    /// @param image The index into the image list to get the image.
    void SetColumnImage(int col, int image);

    /// @note SetItemColumnImage isn't virtual, so need to call this instead
    ///     if your control is virtual.
    /// @param row The row of the cell.
    /// @param column The row of the column.
    /// @param image The image (image list index) to use for the cell.
    void SetItemColumnImageEx(const long row, const long column, const int image);

    /// @returns Whether the user has directly edited an item via a control
    ///     (e.g., floating text box)
    [[nodiscard]]
    bool HasItemBeenEditedByUser() const noexcept
        { return m_hasItemBeenEditedByUser; }
    /// @brief Sets whether an item has been edited in the list.
    /// @details This should be called from an external editing control
    ///     (e.g., a floating text control).
    /// @param edited Whether or not the list has been edited.
    void SetItemBeenEditedByUser(const bool edited = true) noexcept
        {
        m_hasItemBeenEditedByUser = edited;
        if (m_hasItemBeenEditedByUser)
            { SendEditedEvent(); }
        }

    ///Sets the help export topics and images for the list control.
    /// @param helpProjectPath The folder where the help is located.
    /// @param topicPath The subpath to the topic.
    void SetExportResources(const wxString& helpProjectPath,
                            const wxString& topicPath)
        {
        m_helpProjectPath = helpProjectPath;
        m_exportHelpTopic = topicPath;
        }

    /// @brief Sets the help sort topic for the list control.
    /// @param helpProjectPath The folder where the help is located.
    /// @param topicPath The subpath to the topic.
    void SetSortHelpTopic(const wxString& helpProjectPath, const wxString& topicPath)
        {
        m_helpProjectPath = helpProjectPath;
        m_sortHelpTopic = topicPath;
        }
    /// brief Builds a list of base64-encode strings from the given image list, which is later used
    /// when formatting the data to HTML.
    /// @param whichList Which image list to cache (e.g., wxIMAGE_LIST_SMALL or wxIMAGE_LIST_NORMAL).
    /// @note This should be called after AssignImageList() or SetImageList().
    void CacheImageList(const int whichList);

    /// @brief If file deletion is enabled, this is the column representing the
    ///     full file path to delete.
    /// @param col The file column.
    /// @sa EnableFileDeletion().
    void SetFullFilePathColumn(const int col) noexcept
        { m_fullFilePathColumn = col; }
    /// If file deletion is enabled, this is the column representing the folder path to delete.
    /// @param col The folder column.
    /// @sa EnableFileDeletion().
    void SetFolderColumn(const int col) noexcept
        { m_folderColumn = col; }
    /// If file deletion is enabled, this is the column representing the file path to delete.
    /// @param col The file column.
    /// @sa EnableFileDeletion().
    void SetFileColumn(const int col) noexcept
        { m_fileColumn = col; }
    /// @returns The file path attached to a given row.
    /// @param item The row to retrieve the file path from.
    /// @note The file and folder columns must be specified prior to calling this.
    /// @sa SetFullFilePathColumn(), SetFolderColumn(), SetFileColumn().
    [[nodiscard]]
    wxString GetItemFilePath(const long item)
        {
        // if a single column represents the full file path
        if (m_fullFilePathColumn != wxNOT_FOUND && m_fullFilePathColumn < GetColumnCount())
            { return GetItemTextEx(item, m_fullFilePathColumn); }
        // or if they have the path split into columns, then piece them together
        else if (m_fileColumn != wxNOT_FOUND && m_fileColumn < GetColumnCount() &&
                 m_folderColumn != wxNOT_FOUND && m_folderColumn < GetColumnCount())
            {
            return GetItemTextEx(item, m_folderColumn) + wxFileName::GetPathSeparator() +
                GetItemTextEx(item, m_fileColumn);
            }
        else
            {
            wxFAIL_MSG("Folder and File columns must be specified properly before calling GetItemFilePath().");
            return wxString{};
            }
        }
    /// @brief Gets a row's colors and font.
    /// @param item The row to retrieve.
    /// @returns The row's attributes.
    wxItemAttr* OnGetItemAttr(long item) const final;
private:
    bool SortTextItems(const long nCol, const bool ascending,
                       long low = 0, long high = -1);
    /// this is just used to help inline GetItemTextEx() and improve performance for virtual lists
    [[nodiscard]]
    wxString GetItemTextNonVirtual(long item, long column) const;

    ///overloaded functions for virtual mode
    wxString OnGetItemText(long item, long column) const final;
    int OnGetItemImage(long item) const final
        { return m_virtualData->GetItemImage(item, 0); }
    int OnGetItemColumnImage(long item, long column) const final
        { return m_virtualData->GetItemImage(item, column); }

    void SendEditedEvent()
        {
        wxCommandEvent cevent(wxEVT_LISTCTRLEX_EDITED, GetId());
        cevent.SetId(GetId());
        cevent.SetInt(GetId());
        cevent.SetEventObject(this);
        GetEventHandler()->ProcessEvent(cevent);
        }

    wxRect m_largestItemRect;

    bool m_sortable{ true };
    std::vector<std::pair<size_t, Wisteria::SortDirection>> m_sortedCols;
    std::pair<long,long> m_sortableRange;

    std::vector<wxString> m_encodedImages;

    bool m_enableItemAdd{ false };
    bool m_enableItemDelete{ false };
    bool m_enableFileDelete{ false };
    wxString m_deletePrompt;
    int m_fullFilePathColumn{ wxNOT_FOUND };
    int m_fileColumn{ wxNOT_FOUND };
    int m_folderColumn{ wxNOT_FOUND };

    bool m_enableItemViewable{ false };

    // headers
    wxString m_leftPrinterHeader;
    wxString m_centerPrinterHeader;
    wxString m_rightPrinterHeader;
    // footers
    wxString m_leftPrinterFooter;
    wxString m_centerPrinterFooter;
    wxString m_rightPrinterFooter;

    Wisteria::Canvas::Watermark m_waterMark;

    ListCtrlExDataProviderBase* m_virtualData{ nullptr };
    wxMenu* m_menu{ nullptr };
    wxPrintData* m_printData{ nullptr };

    // help
    wxString m_helpProjectPath;
    wxString m_exportHelpTopic;
    wxString m_sortHelpTopic;

    // cell editing controls
    ListEditTextCtrl* m_editTextCtrl{ nullptr };
    ListEditSpinCtrl* m_editSpinCtrl{ nullptr };
    ListEditSpinCtrlDouble* m_editSpinCtrlDouble{ nullptr };
    ListEditComboBox* m_editComboBox{ nullptr };
    std::map<long, ColumnInfo> m_columnInfo;

    bool m_hasItemBeenEditedByUser{ false };

    wxDECLARE_EVENT_TABLE();
    };

/** @}*/

#endif //__LISTCTRL_EX_H__
