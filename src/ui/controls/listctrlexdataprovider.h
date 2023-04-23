/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __LISTCTRL_DATA_PROVIDERS_H__
#define __LISTCTRL_DATA_PROVIDERS_H__

#include <algorithm>
#include <limits>
#include <vector>
#include <map>
#include <execution>
#include <wx/wx.h>
#include <wx/listctrl.h>
#include <wx/math.h>
#include <wx/numformatter.h>
#include "../../i18n-check/src/string_util.h"
#include "../../util/numberformat.h"
#include "../../base/graphitems.h"

/// @brief Base class for a virtual data interface to a list control.
class ListCtrlExDataProviderBase
    {
public:
    /// @private
    ListCtrlExDataProviderBase(const ListCtrlExDataProviderBase&) = delete;
    /// @private
    ListCtrlExDataProviderBase& operator=(ListCtrlExDataProviderBase&) = delete;
    /// @private
    virtual ~ListCtrlExDataProviderBase() {}
    /// @brief Base class for a cell in a list control.
    struct ListCell
        {
        /// @private
        ListCell() = default;
        /// @private
        ListCell(const ListCell& that) :
            m_attributes(that.m_attributes ? new wxItemAttr(*that.m_attributes) : nullptr),
            m_format(that.m_format)
            {}
        /// @private
        ListCell& operator=(const ListCell& that)
            {
            wxDELETE(m_attributes);
            m_attributes = that.m_attributes ? new wxItemAttr(*that.m_attributes) : nullptr;
            m_format = that.m_format;
            return *this;
            }
        /// @brief Destructor.
        ~ListCell()
            { wxDELETE(m_attributes); }
        /// @returns The attributes for the cell (e.g., background color).
        [[nodiscard]]
        const wxItemAttr* GetItemAttributes() const noexcept
            { return m_attributes; }
        /// @brief Sets the attributes for an item.
        /// @param attrib The item attributes to add.
        void SetItemAttributes(const wxItemAttr& attrib)
            {
            wxDELETE(m_attributes);
            m_attributes = new wxItemAttr(attrib);
            }
        /// @brief Sets The numeric format display.
        /// @param format The format to use.
        void SetNumberFormatType(const Wisteria::NumberFormatInfo& format) noexcept
            { m_format = format; }
        /// @returns The number display format.
        [[nodiscard]]
        Wisteria::NumberFormatInfo GetNumberFormatType() const noexcept
            { return m_format; }
        /// @returns The image list index for the item.
        [[nodiscard]]
        int GetImage() const noexcept
            { return m_image; }
        /// @brief Sets the image list index for the item.
        /// @param image The image list index to use for the item's icon.
        void SetImage(const int image) noexcept
            { m_image = image; }
    protected:
        /// @private
        wxItemAttr* m_attributes{ nullptr };
        /// @private
        Wisteria::NumberFormatInfo m_format
            { Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting };
        /// @private
        int m_image{ -1 }; // no image
        };

    /// @brief A cell that can either be a number or a string.
    struct DoubleWithLabel : public ListCell
        {
        /// @private
        DoubleWithLabel() = default;
        /// @returns @c true if label is being displayed.
        [[nodiscard]]
        bool IsDisplayingLabel() const noexcept
            { return (m_labelCode != 0); }
        /// @brief The numeric value.
        double m_numericValue{ std::numeric_limits<double>::quiet_NaN() };
        /// @brief The code into the string table.
        long m_labelCode{ 0 };
        };

    /// @brief Data representation for a listctrl string cell.
    /// @details Includes the string value and attributes.
    struct ListCellString : public std::wstring, ListCell
        {
        /// @private
        ListCellString() = default;
        /// @brief Constructor.
        /// @param str The string to put in the cell.
        /// @param len The length of the string.
        ListCellString(const wchar_t* str, const size_t len) :
            std::wstring(str, len)
            {}
        /// @private
        [[nodiscard]]
        bool operator<(const ListCellString& that) const
            { return Compare(that) < 0; }
        /// @private
        [[nodiscard]]
        bool operator>(const ListCellString& that) const
            { return Compare(that) > 0; }
        /// @private
        [[nodiscard]]
        bool operator<=(const ListCellString& that) const
            { return Compare(that) <= 0; }
        /// @private
        [[nodiscard]]
        bool operator>=(const ListCellString& that) const
            { return Compare(that) >= 0; }
        /// @private
        [[nodiscard]]
        bool operator==(const ListCellString& that) const
            { return Compare(that) == 0; }
        /// @private
        [[nodiscard]]
        int Compare(const ListCellString& that) const
            { return string_util::strnatordncasecmp(c_str(), that.c_str()); }
        };

    /// @private
    using StringMatrix = std::vector<std::vector<ListCellString>>;
    /// @private
    using DoubleWithLabelMatrix = std::vector<std::vector<DoubleWithLabel>>;

    /// @private
    ListCtrlExDataProviderBase() = default;
    /// @brief Sets the number formatting interface for the entire grid.
    /// @param format The format to use.
    void SetNumberFormatter(const Wisteria::NumberFormat* format) noexcept
        { m_formatNumber = format; }

    // Virtual interfaces to be implemented by derived class
    //------------------------------------------------------
    /// @returns The underlying value of a cell.
    /// @param row The row to access.
    /// @param column The column to access.
    virtual wxString GetItemText(const size_t row, const size_t column) const = 0;
    /// @returns The (possibly) formatted value of a cell.
    /// @param row The row to access.
    /// @param column The column to access.
    virtual wxString GetItemTextFormatted(const size_t row, const size_t column) const = 0;
    /// @returns The item's index into the image list if it has an icon.
    /// @param row The row to access.
    /// @param column The column to access.
    virtual int GetItemImage(const size_t row, const size_t column) const = 0;
    // @brief Sets the item's index into the image list.
    /// @param row The row to access.
    /// @param column The column to access.
    /// @param image The image to use (index into the image list).
    virtual void SetItemImage(const size_t row, const size_t column, const int image) = 0;
    /** @brief Sets the cell's text.
        @param row The row of the cell.
        @param column The column of the cell.
        @param text The text to assign to the cell.
        @param format The number format used to display the value
        @param sortableValue An underlying value that can be assigned to the cell
            for when it is compared to other cells during a sort operation.*/
    virtual void SetItemText(const size_t row, const size_t column, const wxString& text,
        const Wisteria::NumberFormatInfo format =
            Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting,
        const double sortableValue = std::numeric_limits<double>::quiet_NaN()) = 0;
    /// @returns The row's attributes (visual look).
    /// @param row The row to return.
    virtual const wxItemAttr* GetRowAttributes(const size_t row) const = 0;
    /// @brief Sets the row's attributes (visual look).
    /// @param row The row to edit.
    /// @param attribs The attributes to apply.
    virtual void SetRowAttributes(const size_t row, const wxItemAttr& attribs) = 0;
    /** @brief Sets the number of rows and columns.
        @param rowCount The number of rows.
        @param columnCount The number of columns.
        @warning If @c rowCount is less than the current number of rows or
            @c columnCount is less than the number of columns, then the data will be shrunk.*/
    virtual void SetSize(const size_t rowCount, const size_t columnCount) = 0;
    /** @brief Sets the number of rows.
        @param rowCount The number of rows.
        @note The number of columns will be preserved.
        @warning If @c rowCount is less than the current number of rows
            then the data will be shrunk.*/
    virtual void SetSize(const size_t rowCount) = 0;
    /// @returns The number of rows.
    virtual size_t GetItemCount() const = 0;
    /// @return The number of columns in the data.
    virtual size_t GetColumnCount() const = 0;
    /// @brief Deletes a row.
    /// @param row The row (by index) to delete.
    virtual void DeleteItem(const size_t row) = 0;
    /// @brief Clears all data from the grid.
    virtual void DeleteAllItems() = 0;
    /// @brief Swaps two rows.
    /// @param row1 The first row.
    /// @param row2 The second row.
    virtual void SwapRows(const size_t row1, const size_t row2) = 0;
    /** @brief Compares a cell with a string.
        @param row The cell's row.
        @param col The cell's column.
        @param text The text to compare against.
        @returns The comparison result.*/
    virtual int CompareItem(const size_t row, const size_t col, const wchar_t* text) const = 0;
    /** @brief Compares two cells.
        @param row1 The first cell's row.
        @param col1 The first cell's column.
        @param row2 The second cell's row.
        @param col2 The second cell's column.
        @returns The comparison result.*/
    virtual int CompareItems(const size_t row1, const size_t col1,
                             const size_t row2, const size_t col2) const = 0;
    /** @brief Finds a text items as it is displayed to the user (even if it is custom formatted).
        @param textToFind The text to find.
        @param startIndex The row to start the search from.
        @returns The index of the row if text is found, @c wxNOT_FOUND otherwise.*/
    virtual long Find(const wchar_t* textToFind, const size_t startIndex = 0) const = 0;
    /// @brief Sorts a column.
    /// @param column The column to sort.
    /// @param direction The direction to sort.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    virtual void Sort(const size_t column, const Wisteria::SortDirection direction,
                      const size_t low /*= 0*/, const size_t high /*= -1*/) = 0;
    /// @brief Sorts multiple columns.
    /// @param columns The columns to sort and their respective directions.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    virtual void Sort(const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columns,
                      const size_t low /*= 0*/, const size_t high /*= -1*/) = 0;
protected:
    /// @private
    const Wisteria::NumberFormat* m_formatNumber{ nullptr };
    };

/** @brief String/Numeric value management class for a data provider.
    @details Designed to store numbers and strings in the same column, and
        unique strings are stored in a lookup table (which saves memory).*/
class ListCtrlLabelManager
    {
public:
    /// @private
    using LabelIDMap = std::map<wxString,long>;
    /// @private
    using IDLabelMap = std::map<long, LabelIDMap::const_iterator>;

    /// @brief Default constructor.
    ListCtrlLabelManager() : m_currentLabelId(1)
        {
        // Insert a mapping of empty string to ID #1 before anything else is inserted.
        // If a client wants to create an empty string label, then that will already be in here.
        std::pair<LabelIDMap::const_iterator,bool> insertionPos =
            m_lablesInUse.insert(std::make_pair(wxString{}, m_currentLabelId));
        m_labelsMap.insert(std::make_pair(m_currentLabelId,insertionPos.first));
        }

    /// @private
    ListCtrlLabelManager(const ListCtrlLabelManager&) = delete;
    /// @private
    ListCtrlLabelManager& operator=(const ListCtrlLabelManager&) = delete;

    /** Either adds the label to the label manager
            (if not in there already [case-sensitive comparison]) and
            returns its ID, or returns the ID of the label that's already in there.
        @param label The label to add (or search for).
        @returns The ID of the label (whether it was already in the manager or just added).*/
    [[nodiscard]]
    long CreateLabelId(const wxString& label)
        {
        const auto [iterator, inserted] =
            m_lablesInUse.insert(std::make_pair(label,m_currentLabelId+1));
        // if the label is new, then add it to both maps and update the ID counter
        if (inserted)
            {
            m_labelsMap.insert(std::make_pair(++m_currentLabelId,iterator));
            return m_currentLabelId;
            }
        else
            { return iterator->second; }
        }
    /// @returns The labels map.
    [[nodiscard]]
    const IDLabelMap& GetLabels() const noexcept
        { return m_labelsMap; }
    /// @returns The label from the provided ID.
    /// @param id The ID to look up for the label.
    [[nodiscard]]
    const wxString& GetLabel(const long id) const
        {
        IDLabelMap::const_iterator pos = m_labelsMap.find(id);
        if (pos != m_labelsMap.cend())
            { return pos->second->first; }
        else
            { return m_emptyCell; }
        }
private:
    IDLabelMap m_labelsMap;
    LabelIDMap m_lablesInUse;
    long m_currentLabelId{ 0 };
    static const wxString m_emptyCell;
    };

/// @brief Comparison base class for double/string data provider.
class DoubleWithTextCompare
    {
public:
    /** @brief Constructor.
        @param textValues The text values (and IDs) to use.
        @param columnsToCompare When sorting, the columns in the row to sort by.*/
    DoubleWithTextCompare(const ListCtrlLabelManager::IDLabelMap& textValues,
                          const std::vector<std::pair<size_t, Wisteria::SortDirection>>& columnsToCompare) :
        m_columnsToCompare(columnsToCompare), m_labelsMap(textValues)
        {}
protected:
    /// @returns The comparison result to two items.
    /// @param cell1 The first cell to compare.
    /// @param cell2 The second cell to compare.
    [[nodiscard]]
    int Compare(const ListCtrlExDataProviderBase::DoubleWithLabel& cell1,
                const ListCtrlExDataProviderBase::DoubleWithLabel& cell2) const;
    /// @returns The label associated with the given @c id.
    /// @param id The ID to lookup.
    [[nodiscard]]
    const wxString& GetLabel(const long id) const;
    /// @private
    const std::vector<std::pair<size_t, Wisteria::SortDirection>> m_columnsToCompare;
    /// @private
    const ListCtrlLabelManager::IDLabelMap& m_labelsMap;
    /// @private
    static const wxString m_emptyCell;
    };

/// @brief Comparison (multi-directional) for double/string data provider.
class DoubleWithTextValuesMultiDirectional final : public DoubleWithTextCompare
    {
public:
    /** @brief Constructor.
        @param textValues The text values (and IDs) to use.
        @param columnsToCompare When sorting, the columns in the row to sort by.*/
    DoubleWithTextValuesMultiDirectional(const ListCtrlLabelManager::IDLabelMap& textValues,
            const std::vector<std::pair<size_t, Wisteria::SortDirection>>& columnsToCompare) :
        DoubleWithTextCompare(textValues, columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row1,
                           const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            {
            const int result =
                Compare(row1[m_columnsToCompare[0].first], row2[m_columnsToCompare[0].first]);
            return (m_columnsToCompare[0].second == Wisteria::SortDirection::SortAscending) ?
                result < 0 : result > 0;
            }
        else
            {
            // go through all the columns
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result =
                    Compare(row1[m_columnsToCompare[i].first], row2[m_columnsToCompare[i].first]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                return (m_columnsToCompare[i].second == Wisteria::SortDirection::SortAscending) ?
                    result < 0 : result > 0;
                }
            // all the columns are the same, so it's neither less than nor greater than
            return false;
            }
        }
    };

/// @brief Comparison (less than) for double/string data provider.
class DoubleWithTextValuesLessThan final : public DoubleWithTextCompare
    {
public:
    /** @brief Constructor.
        @param textValues The text values (and IDs) to use.
        @param columnsToCompare When sorting, the columns in the row to sort by.*/
    DoubleWithTextValuesLessThan(const ListCtrlLabelManager::IDLabelMap& textValues,
            const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columnsToCompare) :
        DoubleWithTextCompare(textValues, columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row1,
                           const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            {
            return (Compare(row1[m_columnsToCompare[0].first], row2[m_columnsToCompare[0].first]) < 0);
            }
        else
            {
            // go through all the columns, except the last
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result =
                    Compare(row1[m_columnsToCompare[i].first], row2[m_columnsToCompare[i].first]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                else
                    { return result < 0; }
                }
            // all the columns are the same
            return false;
            }
        }
    };

/// @brief Comparison (greater than) for double/string data provider.
class DoubleWithTextValuesGreaterThan final : public DoubleWithTextCompare
    {
public:
    /** @brief Constructor.
        @param textValues The text values (and IDs) to use.
        @param columnsToCompare When sorting, the columns in the row to sort by.*/
    DoubleWithTextValuesGreaterThan(const ListCtrlLabelManager::IDLabelMap& textValues,
             const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columnsToCompare) :
        DoubleWithTextCompare(textValues, columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row1,
                           const std::vector<ListCtrlExDataProviderBase::DoubleWithLabel>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            { return (Compare(row1[m_columnsToCompare[0].first], row2[m_columnsToCompare[0].first]) > 0); }
        else
            {
            // go through all the columns
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result =
                    Compare(row1[m_columnsToCompare[i].first], row2[m_columnsToCompare[i].first]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                else
                    { return result > 0; }
                }
            // all the columns are the same
            return false;
            }
        }
    };

/// @brief Comparison (multi-directional) for string data provider.
class StringCellMultiDirectional
    {
public:
    /// @brief Constructor.
    /// @param columnsToCompare The columns to compare.
    explicit StringCellMultiDirectional(
        const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columnsToCompare) :
        m_columnsToCompare(columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::ListCellString>& row1,
                           const std::vector<ListCtrlExDataProviderBase::ListCellString>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            {
            if (m_columnsToCompare[0].second == Wisteria::SortDirection::SortAscending)
                { return row1[m_columnsToCompare[0].first] < row2[m_columnsToCompare[0].first]; }
            else
                { return row1[m_columnsToCompare[0].first] > row2[m_columnsToCompare[0].first]; }
            }
        else
            {
            // go through all the columns
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result =
                    row1[m_columnsToCompare[i].first].Compare(row2[m_columnsToCompare[i].first]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                if (m_columnsToCompare[i].second == Wisteria::SortDirection::SortAscending)
                    { return result < 0; }
                else
                    { return result > 0; }
                }
            // all the columns are the same
            return false;
            }
        }
private:
    const std::vector<std::pair<size_t,Wisteria::SortDirection>> m_columnsToCompare;
    };

/// @brief Comparison (less than) for string data provider.
class StringCellLessThan
    {
public:
    /// @brief Constructor.
    /// @param columnsToCompare The columns to compare.
    explicit StringCellLessThan(const std::vector<size_t>& columnsToCompare) :
        m_columnsToCompare(columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::ListCellString>& row1,
                           const std::vector<ListCtrlExDataProviderBase::ListCellString>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            { return row1[m_columnsToCompare[0]] < row2[m_columnsToCompare[0]]; }
        else
            {
            // go through all the columns
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result = row1[m_columnsToCompare[i]].Compare(row2[m_columnsToCompare[i]]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                else
                    { return result < 0; }
                }
            // all the columns are the same
            return false;
            }
        }
private:
    const std::vector<size_t> m_columnsToCompare;
    };

/// @brief Comparison (greater than) for string data provider.
class StringCellGreaterThan
    {
public:
    /// @brief Constructor.
    /// @param columnsToCompare The columns to compare.
    explicit StringCellGreaterThan(const std::vector<size_t>& columnsToCompare) :
        m_columnsToCompare(columnsToCompare)
        {}
    /// @private
    [[nodiscard]]
    inline bool operator()(const std::vector<ListCtrlExDataProviderBase::ListCellString>& row1,
                           const std::vector<ListCtrlExDataProviderBase::ListCellString>& row2) const
        {
        wxASSERT(m_columnsToCompare.size());
        if (m_columnsToCompare.size() == 1)
            { return row1[m_columnsToCompare[0]] > row2[m_columnsToCompare[0]]; }
        else
            {
            // go through all the columns
            for (size_t i = 0; i < m_columnsToCompare.size(); ++i)
                {
                const int result =
                    row1[m_columnsToCompare[i]].Compare(row2[m_columnsToCompare[i]]);
                // if the cells are equal, then we need to compare the next column as a tie breaker
                if (result == 0)
                    { continue; }
                // otherwise, we have our result, so return it
                else
                    { return result > 0; }
                }
            // all the columns are the same
            return false;
            }
        }
private:
    const std::vector<size_t> m_columnsToCompare;
    };

/// @brief Data provider filled with text (numbers would be formatted as text).
class ListCtrlExDataProvider final : public ListCtrlExDataProviderBase
    {
public:
    /// @returns The underlying value of a cell.
    /// @param row The row to access.
    /// @param column The column to access.
    [[nodiscard]]
    wxString GetItemText(const size_t row, const size_t column) const final
        { return m_virtualData.operator[](row).operator[](column).c_str(); }
    /// @returns The (possibly) formatted value of a cell.
    /// @param row The row to access.
    /// @param column The column to access.
    [[nodiscard]]
    wxString GetItemTextFormatted(const size_t row, const size_t column) const final
        {
        wxASSERT_LEVEL_2(row < m_virtualData.size());
        wxASSERT_LEVEL_2(column < m_virtualData.operator[](row).size());
        const ListCellString& cell = m_virtualData.operator[](row).operator[](column);
        if (cell.GetNumberFormatType().m_type ==
            Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting)
            { return cell.c_str(); }
        else
            {
            wxASSERT_LEVEL_2(m_formatNumber);
            return m_formatNumber ?
                m_formatNumber->GetFormattedValue(cell.c_str(), cell.GetNumberFormatType()) :
                wxString(cell.c_str());
            }
        }
    /** @brief Sets the cell's text.
        @param row The row of the cell.
        @param column The column of the cell.
        @param text The text to assign to the cell.
        @param format The number format used to display the value
        @param sortableValue An underlying value that can be assigned to the cell
            for when it is compared to other cells during a sort operation.*/
    void SetItemText(const size_t row, const size_t column, const wxString& text,
                     const Wisteria::NumberFormatInfo format =
                        Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting,
                     const double sortableValue =
                        std::numeric_limits<double>::quiet_NaN()/*Not used here*/) final
        {
        wxASSERT_LEVEL_2_MSG(std::isnan(sortableValue),
            L"Numeric sortable value not supported by ListCtrlExDataProvider, "
             "use ListCtrlExNumericDataProvider instead.");
        wxUnusedVar(sortableValue);
        ListCellString& cell = m_virtualData.operator[](row).operator[](column);
        cell.assign(static_cast<const wchar_t*>(text));
        cell.SetNumberFormatType(format);
        }
    /// @returns The item's index into the image list if it has an icon.
    /// @param row The row to access.
    /// @param column The column to access.
    [[nodiscard]]
    int GetItemImage(const size_t row, const size_t column) const final
        {
        return m_virtualData.operator[](row).operator[](column).GetImage();
        }
    /// @brief Sets the item's index into the image list.
    /// @param row The row to access.
    /// @param column The column to access.
    /// @param image The image to use (index into the image list).
    void SetItemImage(const size_t row, const size_t column, const int image) final
        {
        m_virtualData.operator[](row).operator[](column).SetImage(image);
        }

    /// @returns The row's attributes (visual look).
    /// @param row The row to return.
    [[nodiscard]]
    const wxItemAttr* GetRowAttributes(const size_t row) const final
        { return m_virtualData.operator[](row).operator[](0).GetItemAttributes(); }
    /// @brief Sets the row's attributes (visual look).
    /// @param row The row to edit.
    /// @param attribs The attributes to apply.
    void SetRowAttributes(const size_t row, const wxItemAttr& attribs) final
        { m_virtualData.operator[](row).operator[](0).SetItemAttributes(attribs); }

    /** @brief Sets the number of rows and columns.
        @param rowCount The number of rows.
        @param columnCount The number of columns.
        @warning If @c rowCount is less than the current number of rows or
            @c columnCount is less than the number of columns, then the data will be shrunk.*/
    void SetSize(const size_t rowCount, const size_t columnCount) final
        {
        m_virtualData.resize(rowCount);
        for (StringMatrix::iterator pos = m_virtualData.begin();
            pos != m_virtualData.end();
            ++pos)
            { pos->resize(columnCount); }
        }
    /** @brief Sets the number of rows.
        @param rowCount The number of rows.
        @note The number of columns will be preserved.
        @warning If @c rowCount is less than the current number of rows
            then the data will be shrunk.*/
    void SetSize(const size_t rowCount) final
        {
        if (rowCount > GetItemCount())
            { SetSize(rowCount, (GetColumnCount() == 0) ? 1 : GetColumnCount()); }
        else
            { m_virtualData.resize(rowCount); }
        }
    /// @returns The number of rows.
    [[nodiscard]]
    size_t GetItemCount() const final
        { return m_virtualData.size(); }
    /// @returns The number of columns.
    [[nodiscard]]
    size_t GetColumnCount() const final
        { return m_virtualData.size() ? m_virtualData.begin()->size() : 0; }
    /// @brief Deletes a row.
    /// @param row The row (by index) to delete.
    void DeleteItem(const size_t row) final
        { m_virtualData.erase(m_virtualData.begin()+row); }
    /// @brief Deletes all items from the grid.
    void DeleteAllItems() final
        { m_virtualData.clear(); }
    /// @brief Swaps two rows.
    /// @param row1 The first row.
    /// @param row2 The second row.
    void SwapRows(const size_t row1, const size_t row2) final
        { m_virtualData[row1].swap(m_virtualData[row2]); }
    /** @brief Compares a cell with a string.
        @param row The cell's row.
        @param col The cell's column.
        @param text The text to compare against.
        @returns The comparison result.*/
    [[nodiscard]]
    int CompareItem(const size_t row, const size_t col, const wchar_t* text) const final
        { return string_util::strnatordncasecmp(GetItemText(row, col).wc_str(), text); }
    /** @brief Compares two cells.
        @param row1 The first cell's row.
        @param col1 The first cell's column.
        @param row2 The second cell's row.
        @param col2 The second cell's column.
        @returns The comparison result.*/
    [[nodiscard]]
    int CompareItems(const size_t row1, const size_t col1,
                            const size_t row2, const size_t col2) const final
        {
        return string_util::strnatordncasecmp(GetItemText(row1, col1).wc_str(),
                                              GetItemText(row2, col2).wc_str());
        }
    /** @brief Finds a text items as it is displayed to the user (even if it is custom formatted).
        @param textToFind The text to find.
        @param startIndex The row to start the search from.
        @returns The index of the row if text is found, @c wxNOT_FOUND otherwise.*/
    [[nodiscard]]
    long Find(const wchar_t* textToFind, const size_t startIndex = 0) const final
        {
        if (GetColumnCount() == 0)
            { return wxNOT_FOUND; }
        for (size_t i = startIndex; i < GetItemCount(); ++i)
            {
            if (CompareItem(i, 0, textToFind) == 0)
                { return static_cast<long>(i); }
            }
        return wxNOT_FOUND;
        }
    /// @returns The underlying matrix (i.e., grid) of data.
    [[nodiscard]]
    inline StringMatrix& GetMatrix() noexcept
        { return m_virtualData; }
    /// @brief Sets the first column from a wxArrayString.
    /// @param arr The string array to copy the values from.
    void SetValues(const wxArrayString& arr)
        {
        SetSize(arr.GetCount());
        for (size_t i = 0; i < arr.GetCount(); ++i)
            { SetItemText(i, 0, arr.Item(i)); }
        }
    /// @brief Sorts a column.
    /// @param column The column to sort.
    /// @param direction The direction to sort.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    void Sort(const size_t column, const Wisteria::SortDirection direction,
               const size_t low /*= 0*/, const size_t high /*= -1*/) final
        {
        if (column >= GetColumnCount())
            { return; }
        StringMatrix::iterator dataEndToSortTo = m_virtualData.end();
        // if the starting point is beyond the range of rows, then nothing to sort
        if (low >= m_virtualData.size())
            { return; }
        if (high != static_cast<size_t>(-1) && high < m_virtualData.size())
            { dataEndToSortTo = m_virtualData.begin()+high+1; }
        if (direction == Wisteria::SortDirection::SortAscending)
            {
            std::sort(std::execution::par, m_virtualData.begin()+low, dataEndToSortTo,
                    StringCellLessThan(std::vector<size_t>(1,column)));
            }
        else
            {
            std::sort(std::execution::par, m_virtualData.begin()+low, dataEndToSortTo,
                    StringCellGreaterThan(std::vector<size_t>(1,column)));
            }
        }
    /// @brief Sorts multiple columns.
    /// @param columns The columns to sort and their respective directions.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    void Sort(const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columns,
               const size_t low /*= 0*/, const size_t high /*= -1*/) final
        {
        for (size_t i = 0; i < columns.size(); ++i)
            {
            if (columns[i].first >= GetColumnCount())
                { return; }
            }
        StringMatrix::iterator dataEndToSortTo = m_virtualData.end();
        // if the starting point is beyond the range of rows, then nothing to sort
        if (low >= m_virtualData.size())
            { return; }
        if (high != static_cast<size_t>(-1) && high < m_virtualData.size())
            { dataEndToSortTo = m_virtualData.begin()+high+1; }
        std::sort(m_virtualData.begin()+low, dataEndToSortTo,
            StringCellMultiDirectional(columns));
        }
    /// @brief Frees memory by shrinking the matrix size to its content.
    void ShrinkToFit()
        { StringMatrix(m_virtualData).swap(m_virtualData); }
private:
    StringMatrix m_virtualData;
    };

/// @brief Data provider filled with double values (and optionally sporadic text values).
class ListCtrlExNumericDataProvider final : public ListCtrlExDataProviderBase
    {
public:
    /// @private
    ListCtrlExNumericDataProvider() = default;
    /// @private
    ListCtrlExNumericDataProvider(const ListCtrlExNumericDataProvider& that) = delete;
    /// @private
    ListCtrlExNumericDataProvider& operator=(const ListCtrlExNumericDataProvider& that) = delete;
    /// @brief Sorts multiple columns.
    /// @param columns The columns to sort and their respective directions.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    void Sort(const std::vector<std::pair<size_t,Wisteria::SortDirection>>& columns,
               const size_t low /*= 0*/, const size_t high /*= -1*/) final
        {
        for (size_t i = 0; i < columns.size(); ++i)
            {
            if (columns[i].first >= GetColumnCount())
                { return; }
            }
        DoubleWithLabelMatrix::iterator dataEndToSortTo = m_virtualData.end();
        // if the starting point is beyond the range of rows, then nothing to sort
        if (low >= m_virtualData.size())
            { return; }
        if (high != static_cast<size_t>(-1) && high < m_virtualData.size())
            { dataEndToSortTo = m_virtualData.begin()+high+1; }
        std::sort(std::execution::par, m_virtualData.begin()+low, dataEndToSortTo,
                DoubleWithTextValuesMultiDirectional(m_labelManager.GetLabels(), columns));
        }
    /// @brief Sorts a column.
    /// @param column The column to sort.
    /// @param direction The direction to sort.
    /// @param low The starting row to begin the sort.
    /// @param high The ending row for the sort.
    void Sort(const size_t column, const Wisteria::SortDirection direction,
              const size_t low /*= 0*/, const size_t high /*= -1*/) final
        {
        if (column >= GetColumnCount())
            { return; }
        DoubleWithLabelMatrix::iterator dataEndToSortTo = m_virtualData.end();
        // if the starting point is beyond the range of rows, then nothing to sort
        if (low >= m_virtualData.size())
            { return; }
        if (high != static_cast<size_t>(-1) && high < m_virtualData.size())
            { dataEndToSortTo = m_virtualData.begin()+high+1; }
        if (direction == Wisteria::SortDirection::SortAscending)
            {
            std::sort(std::execution::par, m_virtualData.begin()+low, dataEndToSortTo,
                    DoubleWithTextValuesLessThan(m_labelManager.GetLabels(),
                    std::vector<std::pair<size_t, Wisteria::SortDirection>>
                        (1, std::pair<size_t,Wisteria::SortDirection>(column,direction))));
            }
        else
            {
            std::sort(std::execution::par, m_virtualData.begin()+low, dataEndToSortTo,
                    DoubleWithTextValuesGreaterThan(m_labelManager.GetLabels(),
                    std::vector<std::pair<size_t,Wisteria::SortDirection>>
                        (1, std::pair<size_t,Wisteria::SortDirection>(column,direction))));
            }
        }
    /** @returns The label from the string table, based on ID.
        @param id The key value to retrieve the string value from the string map.*/
    [[nodiscard]]
    const wxString& GetLabel(const long id) const
        { return m_labelManager.GetLabel(id); }
    /// @returns The textual value of a cell, either as a label or with the
    ///     underlying value formatted.
    /// @param row The row of the cell.
    /// @param column The column of the cell.
    [[nodiscard]]
    wxString GetItemText(const size_t row, const size_t column) const final
        {
        const DoubleWithLabel& cell = m_virtualData.operator[](row).operator[](column);
        if (cell.IsDisplayingLabel())
            { return GetLabel(cell.m_labelCode); }
        else
            { return GetItemTextFormatted(row, column); }
        }
    /// @returns The (possibly custom) formatted value of a cell.
    /// @param row The row of the cell.
    /// @param column The column of the cell.
    [[nodiscard]]
    wxString GetItemTextFormatted(const size_t row, const size_t column) const final
        {
        wxASSERT_LEVEL_2(row < m_virtualData.size());
        wxASSERT_LEVEL_2(column < m_virtualData.operator[](row).size());
        const DoubleWithLabel& cell = m_virtualData.operator[](row).operator[](column);
        if (cell.GetNumberFormatType().m_type ==
                Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting ||
            cell.GetNumberFormatType().m_type ==
                Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting)
            {
            const wxString percentageLabel =
                (cell.GetNumberFormatType().m_type ==
                 Wisteria::NumberFormatInfo::NumberFormatType::PercentageFormatting) ?
                    wxString(L"%") : wxString{};
            if (cell.IsDisplayingLabel())
                {
                return GetLabel(cell.m_labelCode) + percentageLabel;
                }
            else
                {
                if (std::isnan(cell.m_numericValue))
                    { return wxString{}; }
                return wxNumberFormatter::ToString(cell.m_numericValue,
                    cell.GetNumberFormatType().m_precision,
                    cell.GetNumberFormatType().m_displayThousandsSeparator ?
                    wxNumberFormatter::Style::Style_WithThousandsSep|
                        wxNumberFormatter::Style::Style_NoTrailingZeroes :
                    wxNumberFormatter::Style::Style_NoTrailingZeroes) +
                    percentageLabel;
                }
            }
        else
            {
            wxASSERT_LEVEL_2(m_formatNumber);
            if (cell.IsDisplayingLabel())
                {
                return m_formatNumber ?
                    m_formatNumber->GetFormattedValue(GetLabel(cell.m_labelCode),
                                                      cell.GetNumberFormatType()) :
                    GetLabel(cell.m_labelCode);
                }
            else
                {
                if (std::isnan(cell.m_numericValue))
                    { return wxString{}; }
                return m_formatNumber ?
                    m_formatNumber->GetFormattedValue(cell.m_numericValue,
                                                      cell.GetNumberFormatType()) :
                    // shouldn't happen, just being robust
                    wxNumberFormatter::ToString(cell.m_numericValue,
                        cell.GetNumberFormatType().m_precision,
                        cell.GetNumberFormatType().m_displayThousandsSeparator ?
                        wxNumberFormatter::Style::Style_WithThousandsSep|
                            wxNumberFormatter::Style::Style_NoTrailingZeroes :
                        wxNumberFormatter::Style::Style_NoTrailingZeroes);
                }
            }
        }
    /** @brief Sets the cell's text.
        @param row The row of the cell.
        @param column The column of the cell.
        @param text The text to assign to the cell.
        @param format The number format used to display the value
        @param sortableValue An underlying value that can be assigned to the cell for when
            it is compared to other cells during a sort operation.*/
    void SetItemText(const size_t row, const size_t column, const wxString& text,
                     const Wisteria::NumberFormatInfo format =
                        Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting,
                     const double sortableValue = std::numeric_limits<double>::quiet_NaN()) final
        {
        DoubleWithLabel& cell = m_virtualData.operator[](row).operator[](column);
        cell.m_numericValue = sortableValue;
        cell.m_labelCode = m_labelManager.CreateLabelId(text);
        cell.SetNumberFormatType(format);
        }
    /** @brief Sets the numeric value of a cell.
        @param row The row of the cell.
        @param column The column of the cell.
        @param value The numeric value for the cell.
        @param format The format to display @c value in.*/
    void SetItemValue(const size_t row, const size_t column, const double value,
                      const Wisteria::NumberFormatInfo format =
                        Wisteria::NumberFormatInfo::NumberFormatType::StandardFormatting)
        {
        DoubleWithLabel& cell = m_virtualData.operator[](row).operator[](column);
        cell.m_numericValue = value;
        // label codes begin at 1, so this will yield an empty string if used
        cell.m_labelCode = 0;
        cell.SetNumberFormatType(format);
        }

    /** @returns The numeric value of a cell.
        @param row The row of the cell.
        @param column The column of the cell.*/
    [[nodiscard]]
    double GetItemValue(const size_t row, const size_t column) const
        { return m_virtualData.operator[](row).operator[](column).m_numericValue; }

    /// @returns The item's index into the image list if it has an icon.
    /// @param row The cell's row.
    /// @param column The cell's column.
    [[nodiscard]]
    int GetItemImage(const size_t row, const size_t column) const final
        {
        return m_virtualData.operator[](row).operator[](column).GetImage();
        }
    /// @brief Sets the item's index into the image list.
    /// @param row The row to access.
    /// @param column The column to access.
    /// @param image The image to use (index into the image list).
    void SetItemImage(const size_t row, const size_t column, const int image) final
        {
        m_virtualData.operator[](row).operator[](column).SetImage(image);
        }

    /// @returns The row's attributes (visual look).
    /// @param row The row to return.
    [[nodiscard]]
    const wxItemAttr* GetRowAttributes(const size_t row) const final
        { return m_virtualData.operator[](row).operator[](0).GetItemAttributes(); }
    /// @brief Sets the row's attributes (visual look).
    /// @param row The row to edit.
    /// @param attribs The attributes to apply.
    void SetRowAttributes(const size_t row, const wxItemAttr& attribs) final
        { m_virtualData.operator[](row).operator[](0).SetItemAttributes(attribs); }

    /** @brief Sets the number of rows and columns.
        @param rowCount The number of rows.
        @param columnCount The number of columns.
        @warning If @c rowCount is less than the current number of rows or
            @c columnCount is less than the number of columns, then the data will be shrunk.*/
    void SetSize(const size_t rowCount, const size_t columnCount) final
        {
        m_virtualData.resize(rowCount);
        for (DoubleWithLabelMatrix::iterator pos = m_virtualData.begin();
            pos != m_virtualData.end();
            ++pos)
            { pos->resize(columnCount); }
        }
    /** @brief Sets the number of rows.
        @param rowCount The number of rows.
        @note The number of columns will be preserved.
        @warning If @c rowCount is less than the current number of rows,
            then the data will be shrunk.*/
    void SetSize(const size_t rowCount) final
        {
        if (rowCount > GetItemCount())
            { SetSize(rowCount, (GetColumnCount() == 0) ? 1 : GetColumnCount()); }
        else
            { m_virtualData.resize(rowCount); }
        }
    /// @return The number of rows in the data.
    [[nodiscard]]
    size_t GetItemCount() const final
        { return m_virtualData.size(); }
    /// @return The number of columns in the data.
    [[nodiscard]]
    size_t GetColumnCount() const final
        { return m_virtualData.size() ? m_virtualData.begin()->size() : 0; }
    /// @brief Deletes a row.
    /// @param row The row (by index) to delete.
    void DeleteItem(const size_t row) final
        { m_virtualData.erase(m_virtualData.begin()+row); }
    /// @brief Clears all data from the grid.
    void DeleteAllItems() final
        { m_virtualData.clear(); }
    /// @brief Swaps two rows.
    /// @param row1 The first row.
    /// @param row2 The second row.
    void SwapRows(const size_t row1, const size_t row2) final
        { m_virtualData[row1].swap(m_virtualData[row2]); }
    /** @brief Compares a cell with a string.
        @param row The cell's row.
        @param col The cell's column.
        @param text The text to compare against.
        @returns The comparison result.*/
    [[nodiscard]]
    int CompareItem(const size_t row, const size_t col, const wchar_t* text) const final
        { return string_util::strnatordncasecmp(GetItemText(row, col).wc_str(), text); }
    /** @brief Compares two cells.
        @param row1 The first cell's row.
        @param col1 The first cell's column.
        @param row2 The second cell's row.
        @param col2 The second cell's column.
        @returns The comparison result.*/
    [[nodiscard]]
    int CompareItems(const size_t row1, const size_t col1,
                     const size_t row2, const size_t col2) const final
        {
        return string_util::strnatordncasecmp(GetItemText(row1, col1).wc_str(),
                                              GetItemText(row2, col2).wc_str()); }
    /** @brief Finds a text items as it is displayed to the user (even if it is custom formatted).
        @param textToFind The text to find.
        @param startIndex The row to start the search from.
        @returns The index of the row if text is found, @c wxNOT_FOUND otherwise.*/
    [[nodiscard]]
    long Find(const wchar_t* textToFind, const size_t startIndex = 0) const final
        {
        if (GetColumnCount() == 0)
            { return wxNOT_FOUND; }
        for (size_t i = startIndex; i < GetItemCount(); ++i)
            {
            if (CompareItem(i, 0, textToFind) == 0)
                { return static_cast<long>(i); }
            }
        return wxNOT_FOUND;
        }
    /// @returns The data.
    [[nodiscard]]
    inline DoubleWithLabelMatrix& GetMatrix() noexcept
        { return m_virtualData; }
    /// @brief Frees memory by shrinking the matrix size to its content.
    void ShrinkToFit()
        { DoubleWithLabelMatrix(m_virtualData).swap(m_virtualData); }
    /// @returns The summation of a numeric column.
    /// @param column The column to sum.
    [[nodiscard]]
    double GetColumnSum(const size_t column) const
        {
        double total{ 0.0 };
        for (size_t i = 0; i < GetItemCount(); ++i)
            {
            const double currentValue = GetItemValue(i, column);
            if (!std::isnan(currentValue))
                { total += currentValue; }
            }
        return total;
        }
private:
    DoubleWithLabelMatrix m_virtualData;
    ListCtrlLabelManager m_labelManager;
    };

/** @}*/

#endif //__LISTCTRL_DATA_PROVIDERS_H__
