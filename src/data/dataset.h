/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_DATASET_H__
#define __WISTERIA_DATASET_H__

#include <vector>
#include <map>
#include <set>
#include <algorithm>
#include <optional>
#include <limits>
#include <cinttypes>
#include <variant>
#include <memory>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/colour.h>
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/uilocale.h>
#include <wx/filename.h>
#include <wx/numformatter.h>
#include <wx/regex.h>
#include <wx/xlocale.h>
#include "excelreader.h"
#include "../import/text_matrix.h"
#include "../import/text_preview.h"
#include "../base/enums.h"
#include "../math/mathematics.h"
#include "../util/frequencymap.h"
#include "../debug/debug_assert.h"

namespace Wisteria
    {
    // forward declarations for friendships
    class ReportBuilder;
    }

/** @brief %Data management classes for graphs.*/
namespace Wisteria::Data
    {
    // forward declarations for friendships
    class Dataset;
    class DatasetClone;
    class Pivot;

    /// @brief Map of regular expressions and their replacement strings.
    /// @details Technically this is a vector, so the order that this map is created
    ///     is preserved.
    /// @internal Needs to be shared pointers because @c wxRegEx has a private CTOR.
    using RegExMap = std::vector<std::pair<std::shared_ptr<wxRegEx>, wxString>>;
    /// @brief The integral type used for looking up a label from a grouping column's string table.
    /// @details Grouping column string tables are maps that consist of a @c GroupIdType
    ///     as its lookup key and a string as its value.
    using GroupIdType = uint64_t;
    /// @brief The different types that could appear in a dataset.
    using DatasetValueType = std::variant<wxString, double, GroupIdType, wxDateTime>;
    /// @brief Value used in either an ID or categorical column.
    using CategoricalOrIdDataType = std::variant<wxString, GroupIdType>;

    /// @brief Helper functor to compare wxStrings case insensitively.
    /// @details This is useful as a predicate for maps and sets.
    class wxStringLessNoCase
        {
    public:
        /// @private
        [[nodiscard]]
        bool operator()(const wxString& lhs, const wxString& rhs) const
            { return lhs.CmpNoCase(rhs) < 0; };
        };

    /// @private
    enum class ColumnType
        {
        ID,
        Date,
        Continuous,
        Categorical
        };

    /// @brief A column of data.
    template<typename T>
    class Column
        {
        // Functions that affect the size of the column should only be
        // accessible to the parent dataset; client should not be able to alter
        // the dimensions of the column, only its content.
        friend class Dataset;
        friend class DatasetClone;
        friend class Wisteria::ReportBuilder;
    public:
        /// @brief The data type stored in this column.
        using DataType = T;

        /** @brief Constructor.
            @param title The title of the column.
                This is useful for identifying the column in a dataset.*/
        explicit Column(const wxString& title) : m_name(title)
            {}
        /// @private
        Column() = default;
        /// @private
        virtual ~Column() = default;

        /// @name Data Management Functions
        /// @brief Functions relating to accessing and transforming data in the column.
        /// @{

        /// @returns The raw data.
        [[nodiscard]]
        const std::vector<T>& GetValues() const noexcept
            { return m_data; }
        /** @returns A value from the data.
            @param index The index into the data to set.*/
        [[nodiscard]]
        const T& GetValue(const size_t index) const
            {
            wxASSERT_MSG(index < m_data.size(), L"Invalid index in call to Column::GetValue()");
            return m_data.at(index);
            }
        /** @brief Sets a value in the data.
            @param index The index into the data to set.
            @param val The new value.*/
        void SetValue(const size_t index, const T& val)
            {
            wxASSERT_MSG(index < m_data.size(), L"Invalid index in call to Column::SetValue()");
            if (index >= m_data.size())
                { return; }
            m_data.at(index) = val;
            }
        /// @returns @c true if value at the given index is missing data.
        /// @param index The index into the data to read.
        [[nodiscard]]
        virtual bool IsMissingData(const size_t index) const
            {
            wxASSERT_MSG(index < m_data.size(), L"Invalid index in call to Column::IsMissingData()");
            if (index >= m_data.size())
                { return false; }
            if constexpr (std::is_floating_point<T>())
                { return GetValue(index) == std::numeric_limits<T>::quiet_NaN(); }
            else if constexpr (std::is_same_v<T, wxString>)
                { return GetValue(index).empty(); }
            else if constexpr (std::is_same_v<T, wxDateTime>)
                { return !GetValue(index).IsValid(); }
            else
                { return false; }
            }
        /// @brief Recodes all instances of a value in the column to a new value.
        /// @param oldValue The value to replace.
        /// @param newValue The new value to replace with.
        void Recode(const T& oldValue, const T& newValue)
            { std::replace(m_data.begin(), m_data.end(), oldValue, newValue); }
        /** @brief Fills the data with a value.
            @param val The value to fill the data with.*/
        void Fill(const T& val)
            { std::fill(m_data.begin(), m_data.end(), val); }
        /** @brief Fills the data with a vector of values.
            @param values A @c std::vector of values to fill the data with.
            @note If @c values is smaller than the number of rows, then it will
                be copied into the column and the trailing values in the column
                will remain unchanged. If @c values is larger than the column, then throws.
            @throws std::runtime_error If the vector is larger than the column,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void Fill(const std::vector<T>& values)
            {
            if (values.size() <= m_data.size())
                { std::copy(values.cbegin(), values.cend(), m_data.begin()); }
            else
                {
                throw std::runtime_error(
                    _(L"Data being copied into dataset is larger than the column's row count.").
                    ToUTF8());
                }
            }
        /// @brief Fills the column with missing data.
        ///     Missing data will be dependent on the columns data type. For example,
        ///     a date column will be filled with @c wxInvalidDateTime.
        virtual void FillWithMissingData()
            {
            if constexpr (std::is_floating_point<T>())
                { Fill(std::numeric_limits<T>::quiet_NaN()); }
            else if constexpr (std::is_same_v<T, wxString>)
                { Fill(wxEmptyString); }
            else if constexpr (std::is_same_v<T, wxDateTime>)
                { Fill(wxInvalidDateTime); }
            }
        /// @}

        /// @returns The number of rows.
        [[nodiscard]]
        size_t GetRowCount() const noexcept
            { return m_data.size(); }
        /// @returns The name of the column.
        [[nodiscard]]
        const wxString& GetName() const noexcept
            { return m_name; }
        /// @brief Sets the column's name.
        /// @param name The name.
        void SetName(const wxString& name)
            { m_name = name; }
    protected:
        /// @brief Removes all data.
        virtual void Clear() noexcept
            { m_data.clear(); }
        /** @brief Allocates space for the data.
            @param rowCount The number of rows to allocate space for.*/
        void Reserve(const size_t rowCount)
            { m_data.reserve(rowCount); }
        /** @brief Resizes the number of rows.
            @param rowCount The new number of rows.*/
        void Resize(const size_t rowCount)
            { m_data.resize(rowCount); }
        /** @brief Resizes the number of rows.
            @param rowCount The new number of rows.
            @param val Value to initialize any new rows with.*/
        void Resize(const size_t rowCount, const T& val)
            { m_data.resize(rowCount, val); }
        /** @brief Adds a value to the data.
            @param val The new value.*/
        void AddValue(const T& val)
            { m_data.push_back(val); }
    private:
        wxString m_name;
        std::vector<T> m_data;
        };

    /// @brief An integral column with a string lookup table.
    /// @details This is useful for categorical data that is stored as numeric codes,
    ///     but have string labels that represent them.
    /// @note The strings in this table are case insensitive.
    class ColumnWithStringTable final : public Column<GroupIdType>
        {
        friend class Dataset;
    public:
        /// @brief The string table type (i.e., an integer key and string value).
        using StringTableType = std::map<GroupIdType, wxString>;

        /** @brief Constructor.
            @param title The title of the column.
                This is useful for identifying the column in a dataset.*/
        explicit ColumnWithStringTable(const wxString& title) : Column(title)
            {}
        /// @private
        ColumnWithStringTable() = default;

        /// @brief Gets/sets the string table.
        /// @details This can be called to fill or edit the table.
        /// @returns The string table.
        [[nodiscard]]
        StringTableType& GetStringTable() noexcept
            { return m_stringTable; }
        /// @private
        [[nodiscard]]
        const StringTableType& GetStringTable() const noexcept
            { return m_stringTable; }
        /// @brief Sets the string table.
        /// @param sTable The new string table for the column.
        /// @warning If the column already contains data, then it is the caller's responsibility
        ///     to recode any of the existing data. This includes missing data in the column,
        ///     which by default will be mapped to @c 0.
        ///     (The new string table may have a different code for MD,
        ///     so the existing values in the column will need to be recoded in that case.)
        void SetStringTable(const StringTableType& sTable)
            { m_stringTable = sTable; }

        /// @brief Fills the column with missing data.
        void FillWithMissingData() final;
        /// @returns @c true if value at the given index is missing data.
        /// @param index The index into the data to read.
        /// @warning If reviewing numerous rows, then it is recommended to call
        ///     FindMissingDataCode() and compare values from GetValue() against that instead.
        ///     This function has to call FindMissingDataCode() (making it less than optimal)
        ///     and is only meant for convenience.
        [[nodiscard]]
        bool IsMissingData(const size_t index) const final;

        /// @returns @c true if the column contains any missing data.
        [[nodiscard]]
        bool ContainsMissingData() const;

        /** @brief Applies a regular expression string replacement for all values in
                the string table.
            @param pattern The regex pattern to replace.
            @param replace The replacement text.
            @note If recoding causes duplicate entries in the string table, then those duplicates
                will be removed and the data will be recoded accordingly. In other words, the
                string table may be collapsed in the case of duplicates.
            @throws std::runtime_error If the regex pattern is invalid, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void RecodeRE(const wxString& pattern, const wxString& replace);

        /** @brief Collapses strings which appear fewer than a minimum number of times.
            @details This is useful for suppression in higher education reporting.
                For example, you can specify @c 5 as the min value to combine a categorical
                column (e.g., county residency) for student data.\n
                This is similar to what @c forcats::fct_lump_min() does in R.
            @param minVal The minimum number of times a string must appear in the column to
                remain in the string table. If its frequency is less than @c minVal, then
                the value will be lumped into an "other" label.
            @param otherLabel The label to use for the new category where low-frequency
                values are lumped into.*/
        void CollapseMin(const size_t minVal, const wxString& otherLabel = _("Other"));

        /** @brief Collapses strings into an "Other" categorical,
                except for a list of provided labels.
            @param labelsToKeep The strings to preserve; all others will be lumped
                into a new "Other" category.
            @param otherLabel The label to use for the new category "Other" category.*/
        void CollapseExcept(const std::vector<wxString>& labelsToKeep,
                            const wxString& otherLabel = _("Other"));

        /** @brief Gets the label from the string table given the numeric code,
                or the code formatted as a string if not found.
            @returns The label from the string table, or the code as a string if not found.
            @param code The ID to look up.*/
        [[nodiscard]]
        wxString GetLabelFromID(const GroupIdType code) const
            {
            const auto foundLabel = m_stringTable.find(code);
            if (foundLabel != m_stringTable.cend())
                { return foundLabel->second; }
            else
                { return std::to_wstring(code); }
            }
        /** @brief Gets the underlying numeric code from the string table given the label,
                or @c std::nullopt if not found.
            @returns The ID from the string table, or @c std::nullopt if not found.
            @param str The string value to look up.*/
        [[nodiscard]]
        std::optional<GroupIdType> GetIDFromLabel(const wxString& str) const
            {
            // cppcheck-suppress unassignedVariable
            for (const auto& [key, value] : m_stringTable)
                {
                if (value.CmpNoCase(str) == 0)
                    { return key; }
                }
            return std::nullopt;
            }

        /** @returns The value at the given index as a string.
            @param index The index into the data to set.*/
        [[nodiscard]]
        const wxString GetValueAsLabel(const size_t index) const
            { return GetLabelFromID(GetValue(index)); }

        /// @returns The key value from a string table that represents missing data
        ///     (i.e., empty string), or @c std::nullopt if not found.
        /// @param stringTable The string table to review.
        [[nodiscard]]
        static std::optional<GroupIdType> FindMissingDataCode(
            const StringTableType& stringTable)
            {
            // cppcheck-suppress unassignedVariable
            for (const auto& [key, value] : stringTable)
                {
                if (value.empty())
                    { return key; }
                }
            return std::nullopt;
            }
        /// @returns The key value from the string table that represents missing data
        ///     (i.e., empty string), or @c std::nullopt if not found.
        [[nodiscard]]
        std::optional<GroupIdType> FindMissingDataCode() const
            { return FindMissingDataCode(m_stringTable); }

        /** @returns The next group ID that can be inserted into a string table.
            @param stringTable The string table to review.*/
        [[nodiscard]]
        static GroupIdType GetNextKey(const StringTableType& stringTable)
            {
            if (stringTable.size())
                {
                // cppcheck-suppress unassignedVariable
                const auto& [key, value] = *stringTable.crbegin();
                return key + 1;
                }
            else
                { return 0; }
            }

        /** @returns The next group ID that can be inserted into the string table.*/
        [[nodiscard]]
        GroupIdType GetNextKey()
            { return GetNextKey(GetStringTable()); }
    private:
        /// @brief Combines duplicate values in the string table and recodes
        ///     the numeric values down the column.
        void CollapseStringTable();
        /// @brief Removes all data and clears the string table.
        void Clear() noexcept final
            {
            Column::Clear();
            m_stringTable.clear();
            }

        StringTableType m_stringTable;
        };

    /// @brief Constant iterator to a datetime column.
    using DateColumnConstIterator =
        std::vector<Wisteria::Data::Column<wxDateTime>>::const_iterator;
    /// @brief Constant iterator to a continuous column.
    using ContinuousColumnConstIterator =
        std::vector<Wisteria::Data::Column<double>>::const_iterator;
    /// @brief Constant iterator to a categorical column.
    using CategoricalColumnConstIterator =
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator;

    /// @brief Iterator to a datetime column.
    using DateColumnIterator =
        std::vector<Wisteria::Data::Column<wxDateTime>>::iterator;
    /// @brief Iterator to a continuous column.
    using ContinuousColumnIterator =
        std::vector<Wisteria::Data::Column<double>>::iterator;
    /// @brief Iterator to a categorical column.
    using CategoricalColumnIterator =
        std::vector<Wisteria::Data::ColumnWithStringTable>::iterator;

    /// @brief Const iterator to date, continuous, or categorical column,
    ///     or a const pointer to the ID column.
    using ColumnConstIterator = std::variant<const Column<wxString>*, CategoricalColumnConstIterator,
                                             ContinuousColumnConstIterator, DateColumnConstIterator>;
    /// @brief Iterator to date, continuous, or categorical column, or a pointer to the ID column.
    using ColumnIterator = std::variant<Column<wxString>*, CategoricalColumnIterator,
                                        ContinuousColumnIterator, DateColumnIterator>;

    /** @brief Class for filling a row in a dataset.
        @details This can be used to chain multiple fields together in a call to @c AddRow():
        @code
            dataset.AddRow(DataInfo().Continuous({ 4, 120 }).Categoricals({ 1 }));
        @endcode*/
    class RowInfo
        {
        friend class Dataset;
    public:
        /** @brief Sets the codes for the categorical columns.
            @details The order of the values is important.

             As an example:

             `info().Categoricals({ 0, 1 })`

             Will result in 0 being used for the first categorical column and 1
             for the second categorical column.
            @param categoricalValues The values (i.e., codes) to fill the categorical columns with.
            @returns A self reference.*/
        RowInfo& Categoricals(const std::vector<GroupIdType>& categoricalValues)
            {
            m_categoryValues = categoricalValues;
            return *this;
            }
        /// @private
        RowInfo& Categoricals(std::vector<GroupIdType>&& categoricalValues)
            {
            m_categoryValues = std::move(categoricalValues);
            return *this;
            }
        /** @brief Sets the values for the continuous columns.
            @details The order of the values is important.

             As an example:

             @code
             info().Continuous({ 3.1, 5.5 })
             @endcode

             This will result in @c 3.1 being set for the first continuous column
             and @c 5.5 for the second continuous column.
            @param values The column names.
            @returns A self reference.*/
        RowInfo& Continuous(const std::vector<double>& values)
            {
            m_continuousValues = values;
            return *this;
            }
        /// @private
        RowInfo& Continuous(std::vector<double>&& values)
            {
            m_continuousValues = std::move(values);
            return *this;
            }
        /** @brief Sets the dates for the date columns.
            @details The order of the values is important.

             As an example:

             @code
             info().Dates({ dt1, dt2})
             @endcode

             Will result in @c dt1 being used for the first date column and @c dt2
             for the second date column.
            @param dateValues The values to fill the date columns with.
            @returns A self reference.*/
        RowInfo& Dates(const std::vector<wxDateTime>& dateValues)
            {
            m_dateColumns = dateValues;
            return *this;
            }
        /// @private
        RowInfo& Dates(std::vector<wxDateTime>&& dateValues)
            {
            m_dateColumns = std::move(dateValues);
            return *this;
            }
        /** @brief Sets the ID/name of the point.
            @param id The value for the ID.
            @returns A self reference.*/
        RowInfo& Id(const wxString& id)
            {
            m_id = id;
            return *this;
            }
        /// @private
        RowInfo& Id(wxString&& id)
            {
            m_id = std::move(id);
            return *this;
            }
    private:
        std::vector<GroupIdType> m_categoryValues;
        std::vector<double> m_continuousValues;
        std::vector<wxDateTime> m_dateColumns;
        wxString m_id;
        };

    /// @brief How date column data should be read while importing.
    /// @sa ImportInfo.
    enum class DateImportMethod
        {
        IsoDate,              /*!< Parse using the "YYYY-MM-DD" format.*/
        IsoCombined,          /*!< Parse using the "YYYY-MM-DDTHH:MM:SS" format.*/
        Rfc822,               /*!< Uses @c wxDateTime::ParseRfc822Date(), which will be
                                   "looking for a date formatted according to the RFC 822 in it."*/
        Automatic,            /*!< Uses @c wxDateTime::ParseDateTime(), which "Parses the string
                                   containing the date and time in free format. This function tries
                                   as hard as it can to interpret the given string as date and time."
                                   If @c ParseDateTime() fails (because a time component isn't found),
                                   then @c ParseDate() will be attempted. If that fails, then
                                   @c ParseTime() will be called to load the value as a time
                                   (setting the date to today).*/
        StrptimeFormatString, /*!< Parse using a strptime()-like format string (e.g., "%Y-%m-%d").
                                   Please see the description of the ANSI C function @c strftime(3)
                                   for the syntax of the format string.*/
        Time                  /*!< Parse as a time, using the "HH:MM:SS" format.
                                   (The date will be set to today.)*/
        };

    /// @brief How categorical column data should be read while importing.
    /// @sa ImportInfo.
    enum class CategoricalImportMethod
        {
        ReadAsIntegers, /*!< Read the column as integral codes.\n
                             Caller should set the respective string values after the import.*/
        ReadAsStrings   /*!< Read the column as strings. Respective integer codes will be
                             arbitrarily assigned in the order that strings appear.*/
        };

    /// @brief Class for specifying which columns from an input file to use in the dataset
    ///     and how to map them.
    /// @details The fields in this class are chainable, so you can set multiple properties
    ///     in place as you construct it.
    class ImportInfo
        {
        friend class Dataset;
    public:
        /// @brief Structure defining how to import a date column.
        struct DateImportInfo
            {
            /// @brief The name of the column.
            wxString m_columnName;
            /// @brief The method to import the column with.
            DateImportMethod m_importMethod{ DateImportMethod::Automatic };
            /// @brief If using @c StrptimeFormatString, this will be the format string to use.
            wxString m_strptimeFormatString;
            };

        /// @brief Structure defining how to import a categorical/grouping column.
        struct CategoricalImportInfo
            {
            /// @brief The name of the column.
            wxString m_columnName;
            /// @brief The method to import the column with.
            /// @details The numeric code assigned to missing data (i.e., empty string)
            ///  is non-deterministic. It will be whatever the next ID in the sequence is when
            ///  the first empty value is encountered in the column.
            ///  `ColumnWithStringTable::FindMissingDataCode()` can be used to
            ///  find this code after loading the data.
            CategoricalImportMethod m_importMethod{ CategoricalImportMethod::ReadAsStrings };
            /// @brief The default missing data code.
            /// @details This is only used if using the @c ReadAsIntegers method.
            ///  When an empty value is encountered in the column, this value will be assigned
            ///  to it.\n
            ///  Caller is responsible for assigning an empty string to this code when
            ///  connecting a string table to this column after import.
            GroupIdType m_mdCode{ 0 };
            };

        /** @brief Sets the names of the input columns to import for the date columns.

             As an example:

            @code
             info().DateColumns({
                 // parse using the format "%Y-%m-%d"
                 { L"StartDate", DateImportMethod::StrptimeFormatString, L"%Y-%m-%d" },
                 // YYYY-MM-DD strings in the data
                 { L"EndDate", DateImportMethod::IsoDate }
                 })
            @endcode

             Will result in `StartDate` being imported as the first date column and `EndDate`
             as the second date column. Also, note that `StartDate` will be read using a custom
             format, while `EndDate` will read the date in ISO format.
            @param dateColumns The column names and their respective important methods
             (and optional format string, if method is @c StrptimeFormatString).\n
             For the import methods, you can specify how to parse the dates from the input,
             including providing your own format string.
            @sa DateImportInfo for more info.
            @returns A self reference.*/
        ImportInfo& DateColumns(const std::vector<DateImportInfo>& dateColumns)
            {
            m_dateColumns = dateColumns;
            return *this;
            }
        /// @private
        ImportInfo& DateColumns(std::vector<DateImportInfo>&& dateColumns)
            {
            m_dateColumns = std::move(dateColumns);
            return *this;
            }

        /** @brief Sets the names of the input columns to import for the categorical columns.

             As an example:

            @code
             info().CategoricalColumns({
                 { L"AGE RANGE" },
                 { L"GENDER", CategoricalImportMethod::ReadAsIntegers }
                 })
            @endcode

             Will result in `AGE RANGE` being imported as the first categorical column and `GENDER`
             as the second categorical column. Also, note that `GENDER` will be read as discrete
             numbers, while `AGE RANGE` will use the default of being imported as strings.
            @param categoricalColumns The column names and their respective important methods.\n
             For the import methods, you can either import the column as strings
             and have integer codes automatically (and arbitrarily) assigned to them, or import
             integer codes that you later assign strings to.
            @sa Data::CategoricalImportMethod for more info.
            @returns A self reference.*/
        ImportInfo& CategoricalColumns(const std::vector<CategoricalImportInfo>& categoricalColumns)
            {
            m_categoricalColumns = categoricalColumns;
            return *this;
            }
        /// @private
        ImportInfo& CategoricalColumns(std::vector<CategoricalImportInfo>&& categoricalColumns)
            {
            m_categoricalColumns = std::move(categoricalColumns);
            return *this;
            }
        /** @brief Sets the names of the input columns to import for the continuous columns.

            @code
             info().ContinuousColumns({ L"COMPASS SCORES", L"GPA" })
            @endcode

                This will result in `COMPASS SCORES` being imported as the first continuous column
                and `GPA` as the second continuous column.
            @param colNames The column names.
            @returns A self reference.*/
        ImportInfo& ContinuousColumns(const std::vector<wxString>& colNames)
            {
            m_continuousColumns = colNames;
            return *this;
            }
        /// @private
        ImportInfo& ContinuousColumns(std::vector<wxString>&& colNames)
            {
            m_continuousColumns = std::move(colNames);
            return *this;
            }
        /** @brief Sets the name of the input column to use for the ID column.
            @param colName The column name.
            @returns A self reference.*/
        ImportInfo& IdColumn(const wxString& colName)
            {
            m_idColumn = colName;
            return *this;
            }
        /// @private
        ImportInfo& IdColumn(wxString&& colName)
            {
            m_idColumn = std::move(colName);
            return *this;
            }
        /** @brief Set the value to replace missing data in continuous cells
                during import. (The default value is NaN.)
            @details Missing data can be either empty cells or text values in a
                numeric column that can't be converted to a number.\n
                This can be useful for replacing MD with zero or even an extreme value
                (e.g., `-9999`) to draw attention to missing data.
            @param recodeVal The value to replace missing data with.
            @sa ReplacementStrings() for recoding missing data in categorical columns.
            @returns A self reference.*/
        ImportInfo& ContinuousMDRecodeValue(const double recodeVal)
            {
            m_continuousMDRecodeValue = recodeVal;
            return *this;
            }
        /** @brief Set the number of rows to skip before reading the text.
            @param startRow The starting row in the text to begin the parse.
            @returns A self reference.*/
        ImportInfo& SkipRows(const size_t startRow)
            {
            m_skipRows = startRow;
            return *this;
            }
        /** @brief Set the value to treat as missing data during an import.
            @param mdCode The value to treat as missing data.
            @returns A self reference.*/
        ImportInfo& MDCode(const std::optional<std::wstring>& mdCode)
            {
            m_mdCode = mdCode;
            return *this;
            }
        /** @brief Set whether to import numeric columns with leading zeros as text.
            @details This is useful for fixed-width numeric IDs that should actually
                be treated as labels (rather than continuous values).
            @param leadZeros @c true to import numeric columns with leading zeros as text.
            @returns A self reference.*/
        ImportInfo& TreatLeadingZerosAsText(const bool leadZeros)
            {
            m_treatLeadingZerosAsText = leadZeros;
            return *this;
            }
        /** @brief Sets a map of regular expressions to look for in imported text
                (i.e., categorical) columns and what to replace them with.
            @details This is useful for recoding values to missing data,
                missing data to a vale, or fixing misspellings in the input data.
            @par Example:
            @code
             auto commentsData = std::make_shared<Data::Dataset>();
             commentsData->ImportCSV(L"Comments.csv",
                Data::ImportInfo().CategoricalColumns({
                    { L"Comments", CategoricalImportMethod::ReadAsStrings }
                    }).
                ReplacementStrings({
                    // replace cells that contain only something like
                    // 'NA' or 'n/a' with empty string
                    { std::make_shared<wxRegEx>(L"^[nN][/]?[aA]$"), L"" },
                    // replace 'foot ball' with 'football'
                    { std::make_shared<wxRegEx>(L"(?i)foot ball"), L"football" }
                    }));
            @endcode
            @param replaceStrings The map of regular expressions to match against
                and what to replace them with.
            @returns A self reference.
            @sa ContinuousMDRecodeValue() for recoding missing data in continuous columns.*/
        ImportInfo& ReplacementStrings(const RegExMap& replaceStrings)
            {
            m_textImportReplacements = replaceStrings;
            return *this;
            }
        /** @brief Builds a regex map from a dataset.
            @details This can be useful for loading a file containing a list of regexes
                and their replacement values from a file and passing that to ReplacementStrings().
            @param dataset The dataset containing the patterns to replace and what to
                replace them with.
            @param regexColumnName The name of the column containing the patterns to replace.\n
                Any regular expressions that fail to compile will be logged and then ignored.
            @param replacementColumnName The name of the column containing the replacements.
            @returns The map of regular expression patterns to replace and what to replace them with.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        static RegExMap DatasetToRegExMap(const std::shared_ptr<Dataset>& dataset,
            const wxString& regexColumnName,
            const wxString& replacementColumnName);
        /// @private
        ImportInfo& ReplacementStrings(RegExMap&& replaceStrings)
            {
            m_textImportReplacements = std::move(replaceStrings);
            return *this;
            }
    private:
        std::vector<DateImportInfo> m_dateColumns;
        std::vector<CategoricalImportInfo> m_categoricalColumns;
        std::vector<wxString> m_continuousColumns;
        wxString m_idColumn;
        RegExMap m_textImportReplacements;
        double m_continuousMDRecodeValue{ std::numeric_limits<double>::quiet_NaN() };
        std::optional<std::wstring> m_mdCode{ std::nullopt };
        size_t m_skipRows{ 0 };
        bool m_treatLeadingZerosAsText{ false };
        };

    /** @brief %Dataset interface for graphs.
        @details Contains columns for continuous data, categoricals, groupings,
            dates, and observation names/IDs.
         
        @sa [Importing Data](../../ImportingData.md) and [Building Data](../../BuildingData.md)
            for dataset overviews.*/
    class Dataset
        {
        friend class DatasetClone;
        friend class Pivot;
        friend class Wisteria::ReportBuilder;
    public:
        /// @brief Description of a column's deduced type when previewing.
        /// @details This is used by ReadColumnInfo().
        enum class ColumnImportType
            {
            String,   /*!< %Column is text.*/
            Discrete, /*!< %Column is integral (small range, representing codes).*/
            Numeric,  /*!< %Column is floating point or wide range of integers.*/
            Date      /*!< %Column is a date.*/
            };

        /// @brief The names and data types of columns in a dataset.
        /// @details This is what is returned from ReadColumnInfo() and can be used
        ///     for selecting variables before importing a dataset.
        using ColumnPreviewInfo = std::vector<std::pair<wxString, ColumnImportType>>;

        /// @returns The name of the dataset.
        [[nodiscard]]
        const std::wstring& GetName() const noexcept
            { return m_name; }

        /** @brief Removes a column name(s) from a list of columns.
            @details As an example, this is useful for removing a grouping column from a list
                of other categoricals returned from GetCategoricalColumnNames().
            @param[in,out] colNames The list of column names.
            @param colsToRemove The column name(s) to remove.
            @sa GetContinuousColumnNames(), GetCategoricalColumnNames(), GetDateColumnNames().*/
        static void RemoveColumnNamesFromList(std::vector<wxString>& colNames,
                                              const std::initializer_list<wxString>& colsToRemove)
            {
            auto endOfColumns = std::remove_if(colNames.begin(), colNames.end(),
                [&](const auto& colName)
                {
                for (const auto& removeName : colsToRemove)
                    {
                    if (colName.CmpNoCase(removeName) == 0)
                        { return true; }
                    }
                return false; }
                );
            colNames.erase(endOfColumns, colNames.end());
            }

        /** @brief Determines if there are any valid IDs in the ID column.
            @returns @c true if there are any ID values in the ID column.*/
        [[nodiscard]]
        bool HasValidIdData() const;

        /// @returns The number of rows (observations) in the data.
        [[nodiscard]]
        size_t GetRowCount() const noexcept
            { return m_idColumn.GetRowCount(); }

        /** @name Data Management Functions
            @brief Functions related to accessing and editing
                the dataset's data.*/
        /// @{

        /// @brief Reserves memory for the data.
        /// @param rowCount The number of rows to allocate memory for.
        void Reserve(const size_t rowCount)
            {
            m_idColumn.Reserve(rowCount);
            for (auto& column : m_dateColumns)
                { column.Reserve(rowCount); }
            for (auto& column : m_categoricalColumns)
                { column.Reserve(rowCount); }
            for (auto& column : m_continuousColumns)
                { column.Reserve(rowCount); }
            }
        /// @brief Resizes the dataset.
        /// @param rowCount The number of rows to set the dataset to. If increasing the
        ///     number or rows, new rows will be filled with missing data. If decreasing the
        ///     number of rows, then the data will be truncated.
        /// @warning Prefer using Reserve() and AddRow() to allocate memory and to write
        ///     data to the dataset. This should only be used if using Fill() for
        ///     each column to load data directly into them. (The various Add___Column()
        ///     functions should be called first to create the structure of the dataset.)
        void Resize(const size_t rowCount)
            {
            m_idColumn.Resize(rowCount, wxEmptyString);
            for (auto& column : m_dateColumns)
                { column.Resize(rowCount, wxInvalidDateTime); }
            for (auto& column : m_categoricalColumns)
                {
                auto MDCode = column.FindMissingDataCode();
                if (!MDCode.has_value())
                    {
                    column.GetStringTable().insert(
                        std::make_pair(column.GetNextKey(), wxEmptyString));
                    MDCode = column.FindMissingDataCode();
                    wxASSERT_MSG(MDCode, L"Error creating MD label when resizing column!");
                    }
                column.Resize(rowCount, MDCode.value_or(0) );
                }
            for (auto& column : m_continuousColumns)
                { column.Resize(rowCount, std::numeric_limits<double>::quiet_NaN()); }
            }
        /// @brief Removes all data from the dataset.
        /// @details The ID column and all continuous, categorical, and date columns
        ///     will be cleared.
        void Clear() noexcept
            {
            m_idColumn.Clear();
            for (auto& column : m_dateColumns)
                { column.Clear(); }
            for (auto& column : m_categoricalColumns)
                { column.Clear(); }
            for (auto& column : m_continuousColumns)
                { column.Clear(); }
            }
        /** @brief Adds a single data point.
            @details This is a lower-level method for manually filling a dataset;
             prefer the `ImportXXX()` family of functions for a simpler interface.
            @param dataInfo The data to fill the row with.
            @warning All calls to this function should use a RowInfo with
             the same columns throughout it, in the same order; otherwise, the columns will
             have different lengths or data will be out of order.
             Generally, this should be called in a loop,
             where the RowInfo is constructed with the same set of columns each time.
             Also, it is recommended to add columns prior to calling this
             (e.g., call AddDateColumn()); otherwise, this function will create any
             necessary columns with generically generated names.*/
        void AddRow(const RowInfo& dataInfo);
        /// @}

        /** @name Column Access Functions
            @brief Functions related to adding and accessing columns.*/
        /// @{

        /** @brief Adds a new continuous column (if not already in the dataset).
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.*/
        void AddContinuousColumn(const wxString& columnName)
            {
            wxASSERT_MSG(columnName.length(),
                L"Column name is empty in call to AddContinuousColumn()!");
            // see if already in the dataset
            auto foundColumn = GetContinuousColumn(columnName);
            if (foundColumn != GetContinuousColumns().end())
                { return; }

            m_continuousColumns.resize(m_continuousColumns.size()+1);
            m_continuousColumns.back().SetName(columnName);
            m_continuousColumns.back().Resize(GetRowCount(),
                                              std::numeric_limits<double>::quiet_NaN());
            }
        /** @brief Adds a new categorical column (i.e., ColumnWithStringTable)
                (if not already in the dataset).
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.*/
        void AddCategoricalColumn(const wxString& columnName);
        /** @brief Adds a new categorical column (i.e., ColumnWithStringTable)
                (if not already in the dataset).
            @param columnName The name of the column.
            @param stringTable A string table to assign to the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.*/
        void AddCategoricalColumn(const wxString& columnName,
            const ColumnWithStringTable::StringTableType& stringTable);

        /** @brief Adds a new date column (if not already in the dataset).
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.*/
        void AddDateColumn(const wxString& columnName)
            {
            wxASSERT_MSG(columnName.length(),
                L"Date name is empty in call to AddDateColumn()!");
            // see if already in the dataset
            auto foundColumn = GetDateColumn(columnName);
            if (foundColumn != GetDateColumns().end())
                { return; }

            m_dateColumns.resize(m_dateColumns.size()+1);
            m_dateColumns.back().SetName(columnName);
            m_dateColumns.back().Resize(GetRowCount(), wxInvalidDateTime);
            }

        /** @brief During import, sets the column names to the names
                that the client specified.
            @param info The import specification used when importing the
                data. The column names are extracted from this.*/
        void SetColumnNames(const ImportInfo& info);
        /// @brief Finds a column by name, regardless of type.
        /// @param colName The name of the column.
        /// @returns A column iterator if found, @c std::nullopt otherwise.
        ///     Note that the returned iterator is a @c std::variant that needs to be unwrapped.
        [[nodiscard]]
        std::optional<ColumnIterator> FindColumn(const wxString& colName);

        /// @returns The ID column.
        [[nodiscard]]
        Column<wxString>& GetIdColumn() noexcept
            { return m_idColumn; }

        /** @brief Gets an iterator to a categorical column by name.
            @param columnName The name of the categorical column to look for.
            @returns An iterator to the categorical column if found,
                `GetCategoricalColumns().cend()` otherwise.
            @note Check the return against `GetCategoricalColumns().cend()`
                to confirm that the column was found prior to using it.*/
        [[nodiscard]]
        CategoricalColumnConstIterator GetCategoricalColumn(const wxString& columnName) const
            {
            return std::find_if(GetCategoricalColumns().cbegin(),
                GetCategoricalColumns().cend(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets a writable iterator to a categorical column by name.
            @param columnName The name of the categorical column to look for.
            @returns An iterator to the categorical column if found,
                `GetCategoricalColumns().cend()` otherwise.
            @note Check the return against `GetCategoricalColumns().end()`
                to confirm that the column was found prior to using it.\n
                Also, prefer using GetCategoricalColumn() unless you need to edit the column.*/
        [[nodiscard]]
        CategoricalColumnIterator GetCategoricalColumn(const wxString& columnName)
            {
            return std::find_if(GetCategoricalColumns().begin(),
                GetCategoricalColumns().end(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets an iterator to a date column by name.
            @param columnName The name of the date column to look for.
            @returns An iterator to the group column if found,
                `GetDateColumns().cend()` otherwise.
            @note Check the return against `GetDateColumns().cend()`
                to confirm that the column was found prior to using it.*/
        [[nodiscard]]
        DateColumnConstIterator GetDateColumn(const wxString& columnName) const
            {
            return std::find_if(GetDateColumns().cbegin(),
                GetDateColumns().cend(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets a writable iterator to a date column by name.
            @param columnName The name of the date column to look for.
            @returns An iterator to the group column if found,
                `GetDateColumns().cend()` otherwise.
            @note Check the return against `GetDateColumns().end()`
                to confirm that the column was found prior to using it.\n
                Also, prefer using GetDateColumn() unless you need to edit the column.*/
        [[nodiscard]]
        DateColumnIterator GetDateColumn(const wxString& columnName)
            {
            return std::find_if(GetDateColumns().begin(),
                GetDateColumns().end(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets an iterator to a continuous column by name.
            @param columnName The name of the continuous column to look for.
            @returns An iterator to the group column if found,
                `GetContinuousColumns().end()` otherwise.
            @note Check the return against `GetContinuousColumns().cend()`
                to confirm that the column was found prior to using it.*/
        [[nodiscard]]
        ContinuousColumnConstIterator
            GetContinuousColumn(const wxString& columnName) const
            {
            return std::find_if(GetContinuousColumns().cbegin(),
                GetContinuousColumns().cend(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets a writable iterator to a continuous column by name.
            @param columnName The name of the continuous column to look for.
            @returns An iterator to the group column if found,
                `GetContinuousColumns().end()` otherwise.
            @note Check the return against `GetContinuousColumns().end()`
                to confirm that the column was found prior to using it.\n
                Also, prefer using GetContinuousColumns() unless you need to edit the column.*/
        [[nodiscard]]
        ContinuousColumnIterator GetContinuousColumn(const wxString& columnName)
            {
            return std::find_if(GetContinuousColumns().begin(),
                GetContinuousColumns().end(),
                [&columnName](const auto& item)
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }

        /// @returns The continuous columns.
        [[nodiscard]]
        std::vector<Column<double>>& GetContinuousColumns() noexcept
            { return m_continuousColumns; }
        /// @returns A vector of all continuous column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]]
        std::vector<wxString> GetContinuousColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetContinuousColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }
        /// @returns The categorical columns.
        [[nodiscard]]
        std::vector<ColumnWithStringTable>& GetCategoricalColumns() noexcept
            { return m_categoricalColumns; }
        /// @returns A vector of all categorical column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]]
        std::vector<wxString> GetCategoricalColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetCategoricalColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }
        /// @returns The date columns.
        [[nodiscard]]
        std::vector<Column<wxDateTime>>& GetDateColumns() noexcept
            { return m_dateColumns; }
        /// @returns A vector of all date column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]]
        std::vector<wxString> GetDateColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetDateColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }

        /** @brief Determines if a column name already exists in the dataset.
            @param colName The column name to look for.
            @returns @c true if the column name is in the dataset.*/
        [[nodiscard]]
        bool ContainsColumn(const wxString& colName) const noexcept;
        /// @}

        /** @name Column Manipulation Functions
            @brief Functions related to calculating values from columns,
                as well as column data manipulation.*/
        /// @{

        /** @brief Gets the min and max string values from the specified categorical column,
                (optionally) where the group ID is @c groupId.
            @param column The name of the categorical column.
            @param groupColumn The (optional) group column to filter with.
            @param groupId The grouping ID to filter on (if a grouping column is being used).\n
                If there is no grouping being used, then the full categorical column is reviewed.
            @returns The min and max string values.
            @note If there are no valid values, then will return a pair of empty strings.\n
                Also, the comparison being performed is involves comparing the string values
                in the string table (case-insensitively), not the underlying (numeric) codes.
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        std::pair<wxString, wxString> GetCategoricalMinMax(const wxString& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;
        /** @brief Returns the valid N (i.e., non-empty strings) of the specified categorical column,
                (optionally) where the group ID is @c groupId.
            @param column The name of the categorical column.
            @param groupColumn The (optional) group column to filter with.\n
                If there is no grouping being used, then the full categorical column is reviewed.
            @param groupId The group ID to filter on (if a grouping column was supplied).
            @returns The number of valid N for the given categorical column,
                filtered by group (if provided).
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        size_t GetCategoricalColumnValidN(const wxString& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;

        /** @brief Gets the min and max values from the specified continuous column,
                (optionally) where the group ID is @c groupId.
            @param column The name or index of the continuous column.
            @param groupColumn The (optional) group column to filter with.
            @param groupId The grouping ID to filter on (if a grouping column is being used).\n
                If there is no grouping being used, then the full continuous column is reviewed.
            @returns The min and max values, or a pair of NaNs if no valid observations.
            @note If there are no valid values, then will return a pair of NaNs.
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        std::pair<double, double> GetContinuousMinMax(
            const std::variant<wxString, size_t>& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;
        /** @brief Gets the total from the specified continuous column,
                (optionally) where the group ID is @c groupId.
            @param column The name or index of the continuous column.
            @param groupColumn The (optional) group column to filter with.
            @param groupId The grouping ID to filter on (if a grouping column is being used).\n
                If there is no grouping being used, then the full continuous column is totaled.
            @returns The total, or NaN if no valid observations.
            @note If there are no valid values, then will return a pair of NaNs.
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        double GetContinuousTotal(const std::variant<wxString, size_t>& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;
        /** @brief Returns the valid N (i.e., non-NaN) of the specified continuous column,
                (optionally) where the group ID is @c groupId.
            @param column The name or index of the continuous column.
            @param groupColumn The (optional) group column to filter with.\n
                If there is no grouping being used, then the full continuous column is reviewed.
            @param groupId The group ID to filter on (if a grouping column was supplied).
            @returns The number of valid N for the given continuous column,
                filtered by group (if provided).
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        size_t GetContinuousColumnValidN(const std::variant<wxString, size_t>& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;

        /** @brief Renames a column.
            @param colName The column to rename.
            @param newColName The new name for the column.
            @throws std::runtime_error If column not found, throws an exception.*/
        void RenameColumn(const wxString& colName, const wxString& newColName);

        /** @brief Renames a column using regular expressions.
            @param colNamePattern The column regex pattern to search for and rename.
            @param newColNamePattern The new name for the column.\n
                Can be a regex such as "\2" that references @c colNamePattern.
            @code
             // this will rename "Four Year Graduation Rate Numerator - Class of 2021"
             // to "GRAD YEAR 2021"
             dataset->RenameColumnRE("Four Year Graduation Rate Numerator \\- Class of ([[:digit:]]{4})",
                       "GRAD YEAR \\1");
            @endcode
            @throws std::runtime_error If column not found or regex pattern is invalid, throws an exception.*/
        void RenameColumnRE(const wxString& colNamePattern, const wxString& newColNamePattern);

        /** @brief Applies a regular expression string replacement for all values in
                the specified categorical column.
            @param colName The categorical column to edit.
            @param pattern The regex pattern to replace.
            @param replace The replacement text.
            @throws std::runtime_error If the regex pattern is invalid, throws an exception.*/
        void RecodeRE(const wxString& colName, const wxString& pattern, const wxString& replace);

        /** @brief Collapses strings which appear fewer than a minimum number of times.
                See `ColumnWithStringTable::CollapseMin()` for further details.
            @param colName The categorical column to edit.
            @param minVal The minimum number of times a string must appear in the column to
                remain in the string table. If its frequency is less than @c minVal, then
                the value will be lumped into an "other" label.
            @param otherLabel The label to use for the new category where low-frequency values are lumped into.*/
        void CollapseMin(const wxString& colName, const size_t minVal,
                         const wxString& otherLabel = _("Other"));

        /** @brief Collapses strings into an "Other" categorical,
                except for a list of provided labels.
            @param colName The categorical column to edit.
            @param labelsToKeep The strings to preserve; all others will be lumped
                into a new "Other" category.
            @param otherLabel The label to use for the new category "Other" category.*/
        void CollapseExcept(const wxString& colName,
                            const std::vector<wxString>& labelsToKeep,
                            const wxString& otherLabel = _("Other"));

        /** @brief Fills an existing (or new, if not found) categorical column with values
                based on the values in another categorical column.
            @details This will use a map of regular expression patterns and corresponding
                replacements, where each value in the source column is compared against
                the regex patterns. When the first match is encountered, then the corresponding
                replacement will then be applied to the target column.\n
                This is somewhat simple version of `mutate()` from R.
            @param srcColumnName The source column to compare against.
            @param targetColumnName The target column to alter (or create).
            @param replacementMap The map of regular expressions to match against the source column
                and their respective values to apply to the target column upon a match.\n
                If no matches are found, then the target value will be missing data.*/
        void MutateCategoricalColumn(const wxString& srcColumnName, const wxString& targetColumnName,
                                     const RegExMap& replacementMap);
        /// @}

        /** @name Import Functions
            @brief Functions related to importing data into a dataset.*/
        /// @{

        /** @brief Reads the column names from a file and deduces their data types.
            @param filePath The path to the data file.
            @param rowPreviewCount The number of rows to read when deducing column types.
            @param importInfo Import settings (row start and MD code are used).
            @param worksheet If loading an Excel workbook, the name or
                1-based index of the worksheet.
            @returns A vector of column names and their respective data types.\n
                This can be especially useful for determining whether a categorical column
                should be imported as strings or codes (i.e., discrete numbers).
            @throws std::runtime_error If the file can't be read or contains duplicate
                column names, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @sa ImportInfoFromPreview().*/
        [[nodiscard]]
        static ColumnPreviewInfo ReadColumnInfo(const wxString& filePath,
            const ImportInfo& importInfo = ImportInfo{},
            std::optional<size_t> rowPreviewCount = std::nullopt,
            const std::variant<wxString, size_t>& worksheet = L"");
        /** @brief Reads the column names from a text buffer and deduces their data types.
            @param delimiter The delimiter to parse the columns with.
            @param fileText The text to analyze.
            @param rowPreviewCount The number of rows to read when deducing column types.
            @param importInfo Import settings (row start and MD code are used).
            @returns A vector of column names and their respective data types.\n
                This can be especially useful for determining whether a categorical column
                should be imported as strings or codes (i.e., discrete numbers).
            @throws std::runtime_error If the file can't be read or contains duplicate
                column names, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @sa ImportInfoFromPreview().*/
        [[nodiscard]]
        static ColumnPreviewInfo ReadColumnInfoRaw(const wxString& fileText,
            const wchar_t delimiter,
            const ImportInfo& importInfo = ImportInfo{},
            std::optional<size_t> rowPreviewCount = std::nullopt);

        /** @brief Set the value to replace missing data in continuous cells
                during import. (The default value is NaN.)
            @details Missing data can be either empty cells or text values in a
                numeric column that can't be converted to a number.
            @param recodeVal The value to replace missing data with.*/
        void SetImportContinuousMDRecodeValue(const double recodeVal) noexcept
            { m_importContinuousMDRecodeValue = recodeVal; }

        /** @brief Sets the value to treat as missing data during an import.
            @details This will be applied to all cells in the data during the import.
            @param mdCode The value to treat as missing data.*/
        void SetImportMDRecode(const std::optional<std::wstring>& mdCode) noexcept
            { m_mdCode = mdCode; }

        /** @brief Converts previewed column information into an ImportInfo object
                that can be passed to an import function.
            @param previewInfo A file's preview information (from a call to ReadColumnInfo()).
            @returns An ImportInfo object that can be used for the various `Import___()` functions.
            @throws std::runtime_error If the file can't be read, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @note This method will not import any column as the ID column; all string columns
                will be imported as categorical. If you require a string column to be used as the
                ID column, then you will need to define your own column definitions.
            @sa ReadColumnInfo(), ImportCSV(), ImportTSV().*/
        [[nodiscard]]
        static ImportInfo ImportInfoFromPreview(const ColumnPreviewInfo& previewInfo);
        /** @brief Imports a text file into the dataset.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @param delimiter The delimiter to parse the columns with.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @sa ImportTextRaw(), ImportCSV(), ImportTSV(), ReadColumnInfo(),
                ImportInfoFromPreview().
            @par Example:
            @code
            xyData->ImportText(L"Sense of Belonging.csv",
                ImportInfo().
                // order is important: first the Y data, then the X
                ContinuousColumns({ L"Year", L"Belong" }).
                GroupColumns({ L"Name" }),
                L',');
            @endcode*/
        void ImportText(const wxString& filePath, const ImportInfo& info,
                        const wchar_t delimiter);
        /** @brief Imports raw text into the dataset.
            @param fileText The text buffer to parse and load into the dataset.
            @param info The definition for which columns to import and how to map them.
            @param delimiter The delimiter to parse the columns with.
            @throws std::runtime_error If any named columns aren't found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
            @sa ImportText(), ReadColumnInfo(), ImportInfoFromPreview().*/
        void ImportTextRaw(const wxString& fileText, const ImportInfo& info,
                           const wchar_t delimiter);
        /** @brief Imports a comma-separated file into the dataset.
            @details This is a shortcut for ImportText(), using commas as the column separator.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ImportCSV(const wxString& filePath, const ImportInfo& info)
            { ImportText(filePath, info, L','); }
        /** @brief Imports a tab-separated file into the dataset.
            @details This is a shortcut for ImportText(), using tabs as the column separator.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ImportTSV(const wxString& filePath, const ImportInfo& info)
            { ImportText(filePath, info, L'\t'); }
        /** @brief Imports a Excel workbook (*.xlsx) into the dataset.
            @param filePath The path to the data file.
            @param worksheet The name or 1-based index of the worksheet.
            @param info The definition for which columns to import and how to map them.
            @throws std::runtime_error If the worksheet can't be read or named columns aren't found,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ImportExcel(const wxString& filePath, const std::variant<wxString, size_t>& worksheet,
                        const ImportInfo& info);
        /** @brief Exports the dataset to a text file.
            @details Columns are exported in the following order:
                - ID
                - Categoricals
                - Dates
                - Continuous

                Continuous columns are exported with six-point precision and date
                columns are exported in ISO date & time format.
            @param filePath The file path to save to.
            @param delimiter The delimiter to save with.
            @param quoteColumns Whether the columns should be quoted.
            @throws std::runtime_error If the file can't be written to.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ExportText(const wxString& filePath,
                        const wchar_t delimiter,
                        const bool quoteColumns) const;
        /** @brief Exports the dataset to as a tab-delimited text file.
            @details This is a shortcut for ExportText(), using tabs as the column separator.
            @param filePath The file path to save to.
            @throws std::runtime_error If the file can't be written to.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ExportTSV(const wxString& filePath) const
            { ExportText(filePath, L'\t', false); }
        /** @brief Exports the dataset to as a comma-delimited text file.
            @details This is a shortcut for ExportText(),
                using commas as the column separator and quoting the columns.
            @param filePath The file path to save to.
            @throws std::runtime_error If the file can't be written to.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void ExportCSV(const wxString& filePath) const
            { ExportText(filePath, L',', true); }
        /// @}

        /// @private
        [[nodiscard]]
        const Column<wxString>& GetIdColumn() const noexcept
            { return m_idColumn; }
        /// @private
        [[nodiscard]]
        const std::vector<ColumnWithStringTable>& GetCategoricalColumns() const noexcept
            { return m_categoricalColumns; }
        /// @private
        [[nodiscard]]
        const std::vector<Column<wxDateTime>>& GetDateColumns() const noexcept
            { return m_dateColumns; }
        /// @private
        [[nodiscard]]
        const std::vector<Column<double>>& GetContinuousColumns() const noexcept
            { return m_continuousColumns; }
    private:
        /// @returns The specified continuous column by name or index.
        [[nodiscard]]
        ContinuousColumnConstIterator
            GetContinuousColumn(const std::variant<wxString, size_t>& column) const noexcept
            {
            if (const auto strVal{ std::get_if<wxString>(&column) };
                strVal != nullptr)
                { return GetContinuousColumn(*strVal); }
            else if (const auto indexVal{ std::get_if<size_t>(&column) };
                indexVal != nullptr)
                {
                if (*indexVal >= m_continuousColumns.size())
                    { return GetContinuousColumns().cend(); }
                return m_continuousColumns.cbegin() + *indexVal;
                }
            return GetContinuousColumns().cend();
            }
        /// @returns The specified continuous column.
        /// @param column The index into the list of continuous columns.
        [[nodiscard]]
        Column<double>& GetContinuousColumn(const size_t column) noexcept
            { return m_continuousColumns.at(column); }
        [[nodiscard]]
        const Column<double>& GetContinuousColumn(const size_t column) const noexcept
            { return m_continuousColumns.at(column); }
        /// @private
        [[nodiscard]]
        const ColumnWithStringTable& GetCategoricalColumn(const size_t column) const noexcept
            { return m_categoricalColumns.at(column); }
        /// @returns The specified categorical column.
        /// @param column The index into the list of categorical columns.
        [[nodiscard]]
        ColumnWithStringTable& GetCategoricalColumn(const size_t column) noexcept
            { return m_categoricalColumns.at(column); }
        /// @private
        [[nodiscard]]
        const Column<wxDateTime>& GetDateColumn(const size_t column) const noexcept
            { return m_dateColumns.at(column); }
        /// @returns The specified date column.
        /// @param column The index into the list of date columns.
        [[nodiscard]]
        Column<wxDateTime>& GetDateColumn(const size_t column) noexcept
            { return m_dateColumns.at(column); }
        // conversion helpers
        [[nodiscard]]
        static double ConvertToDouble(const std::wstring& input, double MDRecodeValue);
        [[nodiscard]]
        static GroupIdType ConvertToGroupId(const std::wstring& input,
                                            const GroupIdType mdCode);
        [[nodiscard]]
        static wxDateTime ConvertToDate(const wxString& input,
                                        const DateImportMethod method,
                                        const wxString& formatStr);

        std::wstring m_name;

        double m_importContinuousMDRecodeValue{ std::numeric_limits<double>::quiet_NaN() };
        std::optional<std::wstring> m_mdCode{ std::nullopt };

        // actual data
        Column<wxString> m_idColumn;
        std::vector<Column<wxDateTime>> m_dateColumns;
        std::vector<ColumnWithStringTable> m_categoricalColumns;
        std::vector<Column<double>> m_continuousColumns;
        };
    }

/** @}*/

#endif //__WISTERIA_DATASET_H__
