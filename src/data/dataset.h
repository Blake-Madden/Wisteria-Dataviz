/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2022
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
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/colour.h>
#include <wx/file.h>
#include <wx/datetime.h>
#include <wx/uilocale.h>
#include <wx/filename.h>
#include <wx/numformatter.h>
#include <wx/regex.h>
#include "../import/text_matrix.h"
#include "../import/text_preview.h"
#include "../math/mathematics.h"
#include "../debug/debug_assert.h"

namespace Wisteria
    {
    /// @brief How values can be compared.
    enum class Comparison
        {
        /// @brief Items are equal.
        Equals,
        /// @brief Items are not equal.
        NotEquals,
        /// @brief First item is less than the other.
        LessThan,
        /// @brief First item is less than or equal to the other.
        LessThanOrEqualTo,
        /// @brief First item is greater than the other.
        GreaterThan,
        /// @brief First item is greater than or equal to the other.
        GreaterThanOrEqualTo,
        };
    }

/** @brief %Data management classes for graphs.*/
namespace Wisteria::Data
    {
    /// @brief Helper functor to compare wxStrings case insensitively.
    /// @details This is useful as a predicate for maps and sets.
    class StringCmpNoCase
        {
    public:
        /// @private
        [[nodiscard]] bool operator()(const wxString& lhs, const wxString& rhs) const
            { return lhs.CmpNoCase(rhs) < 0; };
        };

    // forward declarations for friendships
    class Dataset;
    class DatasetClone;

    /// @brief The integral type used for looking up a label from a grouping column's string table.
    /// @details Grouping column string tables are maps that consist of a @c GroupIdType
    ///     as its lookup key and a string as its value.
    using GroupIdType = uint64_t;
    /// @brief The different types that could appear in a dataset.
    using DatasetValueType = std::variant<wxString, double, GroupIdType, wxDateTime>;

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
        /// @brief Functions relating to transforming data in the column.
        /// @{

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
        /// @brief Recodes all instances of a value in the column to a new value.
        /// @param oldValue The value to replace.
        /// @param newValue The new value to replace with.
        void Recode(const T& oldValue, const T& newValue)
            { std::replace(m_data.begin(), m_data.end(), oldValue, newValue); }
        /** @brief Fills the data with a value.
            @param val The value to fill the data with.*/
        void Fill(const T& val)
            { std::fill(m_data.begin(), m_data.end(), val); }
        /// @}

        /// @returns The raw data.
        [[nodiscard]] const std::vector<T>& GetValues() const noexcept
            { return m_data; }
        /** @returns A value from the data.
            @param index The index into the data to set.*/
        [[nodiscard]] const T& GetValue(const size_t index) const
            {
            wxASSERT_MSG(index < m_data.size(), L"Invalid index in call to Column::GetValue()");
            return m_data.at(index);
            }
        /// @returns The number of rows.
        [[nodiscard]] size_t GetRowCount() const noexcept
            { return m_data.size(); }
        /// @returns The name of the column.
        [[nodiscard]] const wxString& GetName() const noexcept
            { return m_name; }
        /// @brief Sets the column's name.
        /// @param name The name.
        void SetName(const wxString& name)
            { m_name = name; }
        /// @private
        [[deprecated("Use GetName().")]]
        [[nodiscard]] const wxString& GetTitle() const noexcept
            { return m_name; }
        /// @private
        [[deprecated("Use SetName().")]]
        void SetTitle(const wxString& title)
            { m_name = title; }
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
    ///  but have string labels that represent them.
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
        [[nodiscard]] StringTableType& GetStringTable() noexcept
            { return m_stringTable; }
        /// @private
        [[nodiscard]] const StringTableType& GetStringTable() const noexcept
            { return m_stringTable; }
        /// @brief Sets the string table.
        /// @param sTable The new string table for the column.
        void SetStringTable(const StringTableType& sTable)
            { m_stringTable = sTable; }
        /** @brief Gets the label from the string table given the numeric code,
                or the code formatted as a string if not found.
            @returns The label from the string table, or the code as a string if not found.
            @param code The ID to look up.*/
        [[nodiscard]] wxString GetCategoryLabelFromID(const GroupIdType code) const
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
        [[nodiscard]] std::optional<GroupIdType> GetIDFromCategoryLabel(const wxString& str) const
            {
            for (const auto& [key, value] : m_stringTable)
                {
                if (value.CmpNoCase(str) == 0)
                    { return key; }
                }
            return std::nullopt;
            }
        /// @returns The key value from a string table that's represents missing data
        ///     (i.e., empty string), or @c std::nullopt if not found.
        /// @param stringTable The string table to review.
        [[nodiscard]] static std::optional<GroupIdType> FindMissingDataCode(
            const StringTableType& stringTable)
            {
            for (const auto& [key, value] : stringTable)
                {
                if (value.empty())
                    { return key; }
                }
            return std::nullopt;
            }
        /** @returns The next group ID that can be inserted into a string table.
            @param stringTable The string table to review.*/
        [[nodiscard]] static GroupIdType GetNextKey(const StringTableType& stringTable)
            {
            if (stringTable.size())
                {
                const auto& [key, value] = *stringTable.crbegin();
                return key;
                }
            else
                { return 0; }
            }
    private:
        /// @brief Removes all data and clears the string table.
        void Clear() noexcept final
            {
            Column::Clear();
            m_stringTable.clear();
            }

        StringTableType m_stringTable;
        };

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

             This will result in 3.1 being set for the first continuous column
             and 5.5 for the second continuous column.
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
        IsoDate,             /*!< Parse using the "YYYY-MM-DD" format.*/
        IsoCombined,         /*!< Parse using the "YYYY-MM-DDTHH:MM:SS" format.*/
        Rfc822,              /*!< Uses @c wxDateTime::ParseRfc822Date(), which will be
                                  "looking for a date formatted according to the RFC 822 in it."*/
        Automatic,           /*!< Uses @c wxDateTime::ParseDateTime(), which "Parses the string
                                  containing the date and time in free format. This function tries
                                  as hard as it can to interpret the given string as date and time."
                                  If @c ParseDateTime() fails (because a time component isn't found),
                                  then @c ParseDateTime() will be attempted.*/
        StrptimeFormatString /*!< Parse using a strptime()-like format string (e.g., "%Y-%m-%d").
                                  Please see the description of the ANSI C function @c strftime(3)
                                  for the syntax of the format string.*/
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
    ///  and how to map them.
    /// @details The fields in this class are chainable, so you can set multiple properties
    ///  in place as you construct it.
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
            ///  @c ColumnWithStringTable::FindMissingDataCode() can be usded to
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

        /// @brief Map of regular expressions and their replacement strings.
        /// @internal Needs to be shared pointers because @c wxRegEx has private CTOR.
        using RegExMap = std::map<std::shared_ptr<wxRegEx>, wxString>;

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
        /** @brief Sets a map of regular expressions to look for in imported text
             (i.e., categorical) columns and what to replace them with.
            @details This is useful for recoding values to missing data or fixing
             misspellings in the input data.
            @par Example:
            @code
             auto commentsData = std::make_shared<Data::Dataset>();
             commentsData->ImportCSV(L"Comments.csv",
                Data::ImportInfo().CategoricalColumns({
                    { L"Comments", CategoricalImportMethod::ReadAsStrings }
                    }).
                ReplacementStrings({
                    // replace cells that contain only something like
                    // 'NA' or 'n/a'
                    { std::make_shared<wxRegEx>(L"^[nN][/]?[aA]$"), L"" },
                    // replace 'foot ball' with 'football'
                    { std::make_shared<wxRegEx>(L"(?i)foot ball"), L"football" }
                    }));
            @endcode
            @param replaceStrings The map of regular expressions to match against
             and what to replace them with.
            @returns A self reference.*/
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
            @returns The map of regular expression pattherns to replace and what to replace them with.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
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
        };

    /** @brief %Dataset interface for graphs.
        @details Contains columns for continuous data, categoricals, groupings,
         dates, and observation names/IDs.
         
        @sa [Importing Data](../../ImportingData.md) and [Building Data](../../BuildingData.md)
         for dataset overviews.*/
    class Dataset
        {
        friend DatasetClone;
    public:
        /// @brief Description of a column's deduced type when previewing.
        /// @details This is used by ReadColumnInfo().
        enum class ColumnImportType
            {
            String,  /*!< %Column is text.*/
            Numeric, /*!< %Column is numeric (double or integer).*/
            Date     /*!< %Column is a date.*/
            };

        /// @brief The names and data types of columns in a dataset.
        /// @details This is what is returned from ReadColumnInfo() and can be used
        ///  for selecting variables before importing a dataset.
        using ColumnPreviewInfo = std::vector<std::pair<wxString, ColumnImportType>>;

        /// @returns The name of the dataset.
        [[nodiscard]] const wxString& GetName() const noexcept
            { return m_name; }

        /// @brief Removes all data from the dataset.
        /// @details All continuous, categorical, and date columns will be removed,
        ///  and the ID column will be cleared.
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
        /// @brief Reserves memory for the data.
        /// @param rowCount The number of data points to allocate memory for.
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
        /** @brief Adds a new continuous column.
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.
            @returns The newly added column.*/
        Column<double>& AddContinuousColumn(const wxString& columnName)
            {
            wxASSERT_MSG(columnName.length(),
                L"Column name is empty in call to AddContinuousColumn()!");
            m_continuousColumns.resize(m_continuousColumns.size()+1);
            m_continuousColumns.back().SetName(columnName);
            m_continuousColumns.back().Resize(GetRowCount(),
                                              std::numeric_limits<double>::quiet_NaN());
            return m_continuousColumns.back();
            }
        /** @brief Adds a new categorical column (i.e., ColumnWithStringTable).
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.
            @returns The newly added column.*/
        ColumnWithStringTable& AddCategoricalColumn(const wxString& columnName);
        /** @brief Adds a new categorical column (i.e., ColumnWithStringTable).
            @param columnName The name of the column.
            @param stringTable A string table to assign to the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.
            @returns The newly added column.*/
        ColumnWithStringTable& AddCategoricalColumn(const wxString& columnName,
            const ColumnWithStringTable::StringTableType& stringTable);
        
        /** @brief Adds a new date column.
            @param columnName The name of the column.
            @note It is recommended to call this prior to AddRow();
                otherwise, that function will rely on creating new columns
                with more generic names.
            @returns The newly added column.*/
        Column<wxDateTime>& AddDateColumn(const wxString& columnName)
            {
            wxASSERT_MSG(columnName.length(),
                L"Date name is empty in call to AddDateColumn()!");
            m_dateColumns.resize(m_dateColumns.size()+1);
            m_dateColumns.back().SetName(columnName);
            m_dateColumns.back().Resize(GetRowCount(), wxInvalidDateTime);
            return m_dateColumns.back();
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
             (e.e., call AddDateColumn()); otherwise, this function will create any
             necessary columns with generically generated names.*/
        void AddRow(const RowInfo& dataInfo);
        /** @brief During import, sets the column names to the names
             that the client specified.
            @param info The import specification used when importing the
             data. The column names are extracted from this.*/
        void SetColumnNames(const ImportInfo& info);

        /// @returns The number of rows (observations) in the data.
        [[nodiscard]] size_t GetRowCount() const noexcept
            { return m_idColumn.GetRowCount(); }

        /// @private
        [[nodiscard]] const Column<wxString>& GetIdColumn() const noexcept
            { return m_idColumn; }
        /// @returns The ID column.
        [[nodiscard]] Column<wxString>& GetIdColumn() noexcept
            { return m_idColumn; }

        /** @brief Gets an iterator to a categorical column by name.
            @param columnName The name of the categorical column to look for.
            @returns An iterator to the categorical column if found,
             `GetCategoricalColumns().cend()` otherwise.
            @note Check the return against `GetCategoricalColumns().cend()`
             to confirm that the column was found prior to using it.*/
        [[nodiscard]] const auto GetCategoricalColumn(const wxString& columnName) const noexcept
            {
            return std::find_if(GetCategoricalColumns().cbegin(),
                GetCategoricalColumns().cend(),
                [&columnName](const auto& item) noexcept
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets an iterator to a categorical column by name.
            @param columnName The name of the categorical column to look for.
            @returns An iterator to the categorical column if found,
             `GetCategoricalColumns().end()` otherwise.
            @note Check the return against `GetCategoricalColumns().end()`
             to confirm that the column was found prior to using it.*/
        [[nodiscard]] auto GetCategoricalColumn(const wxString& columnName) noexcept
            {
            return std::find_if(GetCategoricalColumns().begin(),
                GetCategoricalColumns().end(),
                [&columnName](const auto& item) noexcept
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /// @private
        [[nodiscard]] const std::vector<ColumnWithStringTable>& GetCategoricalColumns() const noexcept
            { return m_categoricalColumns; }
        /// @returns The categorical columns.
        [[nodiscard]] std::vector<ColumnWithStringTable>& GetCategoricalColumns() noexcept
            { return m_categoricalColumns; }
        /// @returns A vector of all categorical column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]] std::vector<wxString> GetCategoricalColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetCategoricalColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }

        /** @brief Gets an iterator to a date column by name.
            @param columnName The name of the date column to look for.
            @returns An iterator to the group column if found,
             `GetDateColumns().cend()` otherwise.
            @note Check the return against `GetDateColumns().cend()`
             to confirm that the column was found prior to using it.*/
        [[nodiscard]] const auto GetDateColumn(const wxString& columnName) const noexcept
            {
            return std::find_if(GetDateColumns().cbegin(),
                GetDateColumns().cend(),
                [&columnName](const auto& item) noexcept
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /** @brief Gets an iterator to a date column by name.
            @param columnName The name of the date column to look for.
            @returns An iterator to the group column if found,
             `GetDateColumns().end()` otherwise.
            @note Check the return against `GetDateColumns().end()`
             to confirm that the column was found prior to using it.*/
        [[nodiscard]] auto GetDateColumn(const wxString& columnName) noexcept
            {
            return std::find_if(GetDateColumns().begin(),
                GetDateColumns().end(),
                [&columnName](const auto& item) noexcept
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /// @private
        [[nodiscard]] const std::vector<Column<wxDateTime>>& GetDateColumns() const noexcept
            { return m_dateColumns; }
        /// @returns The date columns.
        [[nodiscard]] std::vector<Column<wxDateTime>>& GetDateColumns() noexcept
            { return m_dateColumns; }
        /// @returns A vector all all date column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]] std::vector<wxString> GetDateColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetDateColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }

        /** @brief Gets an iterator to a continuous column by name.
            @param columnName The name of the continuous column to look for.
            @returns An iterator to the group column if found,
                `GetContinuousColumns().cend()` otherwise.
            @note Check the return against `GetContinuousColumns().cend()`
                to confirm that the column was found prior to using it.*/
        [[nodiscard]] const auto GetContinuousColumn(const wxString& columnName) const noexcept
            {
            return std::find_if(GetContinuousColumns().cbegin(),
                GetContinuousColumns().cend(),
                [&columnName](const auto& item) noexcept
                { return item.GetName().CmpNoCase(columnName) == 0; });
            }
        /// @private
        [[nodiscard]] const std::vector<Column<double>>& GetContinuousColumns() const noexcept
            { return m_continuousColumns; }
        /// @returns The continuous columns.
        [[nodiscard]] std::vector<Column<double>>& GetContinuousColumns() noexcept
            { return m_continuousColumns; }
        /// @returns A vector all all continuous column names.
        /// @sa RemoveColumnNamesFromList().
        [[nodiscard]] std::vector<wxString> GetContinuousColumnNames() const noexcept
            {
            std::vector<wxString> colNames;
            for (const auto& col : GetContinuousColumns())
                { colNames.push_back(col.GetName()); }
            return colNames;
            }
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
        [[nodiscard]] std::pair<wxString, wxString> GetCategoricalMinMax(const wxString& column,
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
        [[nodiscard]] size_t GetCategoricalColumnValidN(const wxString& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;

        /** @brief Gets the min and max values from the specified continuous column,
                (optionally) where the group ID is @c groupId.
            @param column The name of the continuous column.
            @param groupColumn The (optional) group column to filter with.
            @param groupId The grouping ID to filter on (if a grouping column is being used).\n
                If there is no grouping being used, then the full continuous column is reviewed.
            @returns The min and max values.
            @note If there are no valid values, then will return a pair of NaNs.
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] std::pair<double, double> GetContinuousMinMax(const wxString& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;
        /** @brief Returns the valid N (i.e., non-NaN) of the specified continuous column,
                (optionally) where the group ID is @c groupId.
            @param column The name of the continuous column.
            @param groupColumn The (optional) group column to filter with.\n
                If there is no grouping being used, then the full continuous column is reviewed.
            @param groupId The group ID to filter on (if a grouping column was supplied).
            @returns The number of valid N for the given continuous column,
                filtered by group (if provided).
            @throws std::runtime_error If provided columns cannot be found or if a grouping
                column is provided without a group ID, then throws an exception.\n
                The exception's @c what() message is UTF-8 encoded,
                so pass it to @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]] size_t GetContinuousColumnValidN(const wxString& column,
            const std::optional<wxString>& groupColumn = std::nullopt,
            const std::optional<GroupIdType> groupId = std::nullopt) const;

        /** @brief Determines if there are any valid IDs in the ID column.
            @returns @c true if there are any ID values in the ID column.*/
        [[nodiscard]] bool HasValidIdData() const;

        /** @brief Reads the column names from a file and deduces their data types.
            @param delimiter The delimiter to parse the columns with.
            @param filePath The path to the data file.
            @param rowPreviewCount The number of rows to read when deducing column types,
            @returns A vector of column names and their respective data types.\n
             This can be especially useful for determining whether a categorical column
             should be imported as strings or codes (i.e., discrete numbers).
            @throws std::runtime_error If the file can't be read, throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        [[nodiscard]] static ColumnPreviewInfo ReadColumnInfo(const wxString& filePath,
                                                              const wchar_t delimiter,
                                                              const size_t rowPreviewCount = 100);
        /** @brief Imports a text file into the dataset.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @param delimiter The delimiter to parse the columns with.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.
            @sa ImportCSV(), ImportTSV(), ReadColumnInfo().
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
        /** @brief Imports a comma-separated file into the dataset.
            @details This is a shortcut for ImportText(), using commas as the column separator.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void ImportCSV(const wxString& filePath, const ImportInfo& info)
            { ImportText(filePath, info, L','); }
        /** @brief Imports a tab-separated file into the dataset.
            @details This is a shortcut for ImportText(), using tabs as the column separator.
            @param filePath The path to the data file.
            @param info The definition for which columns to import and how to map them.
            @throws std::runtime_error If the file can't be read or named columns aren't found,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void ImportTSV(const wxString& filePath, const ImportInfo& info)
            { ImportText(filePath, info, L'\t'); }
        /** @brief Exports the dataset to a text file.
            @details Continuous columns are exported with six-point precision and date
             columns are exported in ISO date & time format.
            @param filePath The file path to save to.
            @param delimiter The delimiter to save with.
            @param quoteColumns Whether the columns should be quoted.
            @throws std::runtime_error If the file can't be written to.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void ExportText(const wxString& filePath,
                        const wchar_t delimiter,
                        const bool quoteColumns) const;
        /** @brief Exports the dataset to as a tab-delimited text file.
            @details This is a shortcut for ExportText(), using tabs as the column separator.
            @param filePath The file path to save to.
            @throws std::runtime_error If the file can't be written to.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void ExportTSV(const wxString& filePath) const
            { ExportText(filePath, L'\t', false); }
        /** @brief Exports the dataset to as a comma-delimited text file.
            @details This is a shortcut for ExportText(),
             using commas as the column separator and quoting the columns.
            @param filePath The file path to save to.
            @throws std::runtime_error If the file can't be written to.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void ExportCSV(const wxString& filePath) const
            { ExportText(filePath, L',', true); }
    private:
        /// @returns The specified continuous column.
        /// @param column The index into the list of continuous columns.
        [[nodiscard]] Column<double>& GetContinuousColumn(const size_t column) noexcept
            { return m_continuousColumns.at(column); }
        [[nodiscard]] const Column<double>& GetContinuousColumn(const size_t column) const noexcept
            { return m_continuousColumns.at(column); }
        /// @private
        [[nodiscard]] const ColumnWithStringTable& GetCategoricalColumn(const size_t column) const noexcept
            { return m_categoricalColumns.at(column); }
        /// @returns The specified categorical column.
        /// @param column The index into the list of categorical columns.
        [[nodiscard]] ColumnWithStringTable& GetCategoricalColumn(const size_t column) noexcept
            { return m_categoricalColumns.at(column); }
        /// @private
        [[nodiscard]] const Column<wxDateTime>& GetDateColumn(const size_t column) const noexcept
            { return m_dateColumns.at(column); }
        /// @returns The specified date column.
        /// @param column The index into the list of date columns.
        [[nodiscard]] Column<wxDateTime>& GetDateColumn(const size_t column) noexcept
            { return m_dateColumns.at(column); }
        [[nodiscard]] static double ConvertToDouble(const wxString& input);
        [[nodiscard]] static GroupIdType ConvertToGroupId(const wxString& input,
                                                          const GroupIdType mdCode);
        [[nodiscard]] static wxDateTime ConvertToDate(const wxString& input,
                                                      const DateImportMethod method,
                                                      const wxString& formatStr);

        wxString m_name;

        // actual data
        Column<wxString> m_idColumn{ L"IDS" };
        std::vector<Column<wxDateTime>> m_dateColumns;
        std::vector<ColumnWithStringTable> m_categoricalColumns;
        std::vector<Column<double>> m_continuousColumns;
        };
    }

/** @}*/

#endif //__WISTERIA_DATASET_H__
