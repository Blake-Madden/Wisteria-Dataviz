/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_TABLE_H__
#define __WISTERIA_TABLE_H__

#include <vector>
#include <variant>
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief A display of tabular data.
        @details A table can either be imported from a dataset or be built from scratch.
        
        @par %Data:
            A table can accept a Data::Dataset, where any type of column can be include.
            Which of these columns to include (as well as their order) can be controlled
            by the caller.\n
            \n
            The table can use the same structure as the dataset, or be transposed (so that the
            columns are then the rows in the table).\n
            \n
            Consecutively repeated group labels across the rows and columns can be collapsed
            into larger cells, given the appearance of grouped data or crosstabs
            (see GroupRow() and GroupColumn()).
            Numerous other functions are available for customizing the content and appearance of cells,
            rows, and columns (e.g., GetCell() or BoldRow()).\n
            \n
            Finally, aggregate columns (e.g., subtotals) can be added to the table via AddAggregateColumn().

        @par Missing Data:
            Any missing data from the dataset will be displayed as an empty cell.
            
        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the table
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         auto programAwards = std::make_shared<Data::Dataset>();
         try
            {
            programAwards->ImportCSV(L"/home/luna/data/Program Awards.csv",
                ImportInfo().
                ContinuousColumns({ L"Doctoral Degrees Awarded",
                    L"Time to Degree Since Entering Graduate School" }).
                CategoricalColumns({
                    { L"School" },
                    { L"Program" }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto tableGraph = std::make_shared<Table>(canvas);
         tableGraph->SetData(programAwards,
            { L"School", L"Program",
              L"Doctoral Degrees Awarded", L"Time to Degree Since Entering Graduate School" });
         // split some of the cells' text into multiple lines
         tableGraph->GetCell(14, 0).SetSuggestedLineLength(15);
         tableGraph->GetCell(0, 2).SetSuggestedLineLength(5);
         tableGraph->GetCell(0, 3).SetSuggestedLineLength(10);
         // group the schools together in the first row
         tableGraph->GroupColumn(0);
         // make the headers and row groups bold (and center the headers)
         tableGraph->BoldRow(0);
         tableGraph->BoldColumn(0);
         tableGraph->CenterRowHorizontally(0);
         // apply a zebra-stripe look
         tableGraph->ApplyAlternateRowColors(ColorBrewer::GetColor(Color::AzureMist), 1, 1);

         // add the table to the canvas
         canvas->SetFixedObject(0, 0, tableGraph);
        @endcode*/
    class Table final : public Graph2D
        {
    public:
        /// @brief How to aggregate a row or column.
        enum class AggregateType
            {
            /// @brief Sums a series of values.
            Total,
            /// @brief Calculates the change from one value to another (as a percentage).
            ChangePercent
            };

        /// @brief Information about how to build an aggregation column.
        struct AggregateInfo
            {
            friend class Table;
        public:
            /// @brief Constructor.
            /// @param type Which type of aggregation to perform.
            explicit AggregateInfo(const AggregateType type) : m_type(type)
                {}
            /// @brief The first column in the series of data.
            /// @param first The first column.
            /// @returns A self reference.
            AggregateInfo& FirstCell(const size_t first) noexcept
                {
                m_cell1 = first;
                return *this;
                }
            /// @brief The last column in the series of data.
            /// @param last The last column.
            /// @returns A self reference.
            AggregateInfo& LastCell(const size_t last) noexcept
                {
                m_cell2 = last;
                return *this;
                }
        private:
            AggregateType m_type{ AggregateType::Total };
            std::optional<size_t> m_cell1;
            std::optional<size_t> m_cell2;
            };

        /// @brief How to display a cell's content.
        enum class CellFormat
            {
            /// @brief Displays a number generically.
            General,
            /// @brief Displays a value such as @c 0.25 as @c 25%.
            Percent
            };

        /// @brief Types of values that can be used for a cell.
        using CellValueType = std::variant<double, wxString, wxDateTime>;

        /// @brief A cell in the table.
        class TableCell
            {
            friend class Table;
        public:
            /// @brief Constructor.
            /// @param value The value for the cell.
            /// @param bgColor The cell's background color.
            TableCell(const CellValueType& value, const wxColour bgColor) :
                m_value(value), m_bgColor(bgColor)
                {}
            /// @private
            TableCell() = default;
            /// @brief Gets the value as it is displayed in the cell.
            /// @returns The displayable string for the cell.
            [[nodiscard]] wxString GetDisplayValue() const
                {
                if (const auto strVal{ std::get_if<wxString>(&m_value) }; strVal)
                    { return *strVal; }
                else if (const auto dVal{ std::get_if<double>(&m_value) }; dVal)
                    {
                    if (std::isnan(*dVal))
                        { return wxEmptyString; }
                    else if (m_valueFormat == CellFormat::Percent)
                        {
                        return wxNumberFormatter::ToString((*dVal)*100, m_precision,
                            wxNumberFormatter::Style::Style_None) + L"%";
                        }
                    else
                        {
                        return wxNumberFormatter::ToString(*dVal, m_precision,
                            wxNumberFormatter::Style::Style_WithThousandsSep);
                        }
                    }
                else if (const auto dVal{ std::get_if<wxDateTime>(&m_value) }; dVal)
                    {
                    if (!dVal->IsValid())
                        { return wxEmptyString; }
                    return dVal->FormatDate();
                    }
                else
                    { return wxEmptyString; }
                }

            /// @returns @c true if the cell is text.
            [[nodiscard]] bool IsText() const noexcept
                { return (std::get_if<wxString>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a number.
            [[nodiscard]] bool IsNumeric() const noexcept
                { return (std::get_if<double>(&m_value) != nullptr); }
            /// @returns @c true if the cell is a date.
            [[nodiscard]] bool IsDate() const noexcept
                { return (std::get_if<wxDateTime>(&m_value) != nullptr); }

            /// @brief Sets the value.
            /// @param value The value to set for the cell.
            void SetValue(const CellValueType& value)
                { m_value = value; }
            /// @brief Sets the color.
            /// @param color The value to set for the cell.
            void SetBackgroundColor(const wxColour color)
                { m_bgColor = color; }
            /// @returns Access to the cell's font. This can be useful for changing
            ///     an attribute of the font, rather than entirely setting a new font.
            [[nodiscard]] wxFont& GetFont() noexcept
                { return m_font; }
            /// @brief Sets the cell's font.
            /// @param font The font to use.
            void SetFont(const wxFont& font)
                { m_font = font; }
            /// @brief Sets the number of columns that this cell should consume.
            /// @param colCount The number of cells that this should consume horizontally.
            void SetColumnCount(const int colCount) noexcept
                {
                if (colCount <= 0)
                    { m_columnCount = 1; }
                else
                    { m_columnCount = colCount; }
                }
            /// @brief Sets the number of rows that this cell should consume.
            /// @param rowCount The number of cells that this should consume vertically.
            void SetRowCount(const int rowCount) noexcept
                {
                if (rowCount <= 0)
                    { m_rowCount = 1; }
                else
                    { m_rowCount = rowCount; }
                }
            /// @brief Sets the suggested line length for the cell (if text).
            /// @param lineLength The suggested line length.
            void SetSuggestedLineLength(const size_t lineLength) noexcept
                { m_suggestedLineLength = lineLength; }
            /// @brief Shows or hides the left border of a cell if it's in the
            ///     first column.
            /// @param show @c false to hide the left outer border of a cell.
            void ShowOuterLeftBorder(const bool show)
                { m_showOuterLeftBorder = show; }
            /// @brief Shows or hides the top border of a cell if it's in the
            ///     first row.
            /// @param show @c false to hide the top outer border of a cell.
            void ShowOuterTopBorder(const bool show)
                { m_showOuterTopBorder = show; }
            /// @brief Shows or hides the bottom border of a cell if it's in the
            ///     last row.
            /// @param show @c false to hide the bottom outer border of a cell.
            void ShowOuterBottomBorder(const bool show)
                { m_showOuterBottomBorder = show; }
            /// @brief Shows or hides the right border of a cell if it's in the
            ///     last column.
            /// @param show @c false to hide the right outer border of a cell.
            void ShowOuterRightBorder(const bool show)
                { m_showOuterRightBorder = show; }
        private:
            [[nodiscard]] double GetDoubleValue() const noexcept
                {
                const auto dVal{ std::get_if<double>(&m_value) };
                if (dVal)
                    { return *dVal; }
                else
                    { return std::numeric_limits<double>::quiet_NaN(); }
                }
            CellValueType m_value{ std::numeric_limits<double>::quiet_NaN() };
            CellFormat m_valueFormat{ CellFormat::General };
            uint8_t m_precision{ 0 };
            wxColour m_bgColor{ *wxWHITE };
            wxFont m_font{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
            std::optional<PageHorizontalAlignment> m_horizontalCellAlignment;
            std::optional<size_t> m_suggestedLineLength;
            int m_columnCount{ 1 };
            int m_rowCount{ 1 };
            bool m_showOuterLeftBorder{ true };
            bool m_showOuterTopBorder{ true };
            bool m_showOuterRightBorder{ true };
            bool m_showOuterBottomBorder{ true };
            };

        /// @brief Constructor.
        /// @param canvas The canvas to draw the table on.
        explicit Table(Wisteria::Canvas* canvas);

        /** @brief Set the data for the table.
            @param data The data.
            @param columns The columns to display in the table.\n
                The columns will appear in the order that you specify here.
            @param transpose @c true to transpose the data (i.e., display the columns
                from the data as rows).
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const std::initializer_list<wxString>& columns,
                     const bool transpose = false);

        /// @name Table Functions
        /// @brief Functions for editing the table as a whole.
        /// @{

        /// @brief Sets the size of the table.
        /// @details This should only be used if building a table from scratch.
        ///     Prefer using SetData() instead.
        /// @param rows The number of rows.
        /// @param cols The number of columns.
        /// @note If the table is being made smaller, then existing content
        ///     outside of the new size will be removed; other existing content
        ///     will be preserved.\n
        ///     Call ClearTable() to clear any existing content if you wish to
        ///     reset the table.
        void SetTableSize(const size_t rows, const size_t cols)
            {
            m_table.resize(rows);
            for (auto& row : m_table)
                { row.resize(cols); }
            }
        /// @brief Empties the contents of the table.
        void ClearTable()
            { m_table.clear(); }
        /** @brief Sets the font for the entire table.
            @param ft The font to apply.*/
        void SetTableFont(const wxFont& ft)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    for (auto& cell : row)
                        { cell.SetFont(ft); }
                    }
                }
            }
        /// @brief Sets the minimum percent of the drawing area's width that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @param percent The minimum percent of the area's width that the table should consume.
        void SetMinWidthProportion(const double percent)
            { m_minWidthProportion = std::clamp(percent, 0.0, 1.0); }
        /// @brief Sets the minimum percent of the drawing area's height that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @param percent The minimum percent of the area's height that the table should consume.
        void SetMinHeightProportion(const double percent)
            { m_minHeightProportion = std::clamp(percent, 0.0, 1.0); }
        /// @}

        /// @name Row & Column Functions
        /// @brief Functions for editing rows and columns.
        /// @{

        /// @returns The number of rows.
        /// @warning This will include the first row which contains the original dataset's
        ///     column names (unless it was transposed in the call to SetData()).
        [[nodiscard]] size_t GetRowCount() const noexcept
            { return m_table.size(); }
        /// @returns The number of columns.
        /// @warning If the imported file was transposed, then this will also include
        ///     the first column which contains the dataset's original column names.
        [[nodiscard]] size_t GetColumnCount() const noexcept
            { return (m_table.size() == 0) ? 0 : m_table[0].size(); }

        /// @brief Inserts an empty row at the given index.
        /// @details For example, an index of @c 0 will insert the row at the
        ///     top of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will ignored since the
        ///     number of columns is unknown.
        /// @param rowIndex Where to insert the row.
        void InsertRow(const size_t rowIndex)
            {
            if (m_table.size())
                {
                m_table.insert(m_table.cbegin() +
                        // clamp indices going beyond the row count to m_table.cend()
                        std::clamp<size_t>(rowIndex, 0, m_table.size()),
                    std::vector<TableCell>(m_table[0].size(), TableCell()));
                }
            }
        /// @brief Inserts an empty column at the given index.
        /// @details For example, an index of @c 0 will insert the column at the
        ///     left side of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will ignored since there will
        ///     be no rows to insert columns into.
        /// @param colIndex Where to insert the column.
        /// @param colName An optional value for the first row of the new
        ///     column, representing a name for the column.
        ///     This will be overwritten if the top row is not column names
        ///     (e.g., if the table was transposed).
        void InsertColumn(const size_t colIndex, std::optional<wxString> colName = std::nullopt)
            {
            if (m_table.size())
                {
                for (auto& row : m_table)
                    {
                    row.insert(row.cbegin() +
                        // clamp indices going beyond the column count to row.cend()
                        std::clamp<size_t>(colIndex, 0, row.size()),
                        TableCell());
                    }
                if (colName.has_value())
                    { m_table[0][colIndex].m_value = colName.value(); }
                }
            }
        /** @brief Adds an aggregate (e.g., total) column to the end of the table.
            @param aggInfo Which type of aggregation to use in the column.
            @param colName An optional value for the first row of the new
                column, representing a name for the column.\n
                This will be overwritten if the top row is not column names
                (e.g., if the table was transposed).*/
        void AddAggregateColumn(const AggregateInfo& aggInfo,
                                std::optional<wxString> colName = std::nullopt);

        /// @brief Sets the background color for a given row.
        /// @param row The row to change.
        /// @param color The background color to apply to the row.
        /// @param startColumn An optional column in the row to start from.\n
        ///     The default is to start from the first column.
        /// @param endColumn An optional column in the row to end at.\n
        ///     The default is to end at the last column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void SetRowBackgroundColor(const size_t row, const wxColour color,
                                   std::optional<size_t> startColumn = std::nullopt,
                                   std::optional<size_t> endColumn = std::nullopt);
        /// @brief Sets the background color for a given column.
        /// @param column The column to change.
        /// @param color The background color to apply to the column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void SetColumnBackgroundColor(const size_t column, const wxColour color)
            {
            if (m_table.size() && column < m_table[0].size())
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].m_bgColor = color; }
                    }
                }
            }

        /** @brief Makes the specified row use a bold font.
            @param row The row to make bold.*/
        void BoldRow(const size_t row)
            {
            if (row < m_table.size())
                {
                auto& currentRow = m_table[row];
                for (auto& cell : currentRow)
                    { cell.GetFont().MakeBold(); }
                }
            }
        /** @brief Makes the specified column use a bold font.
            @param column The column to make bold.*/
        void BoldColumn(const size_t column)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].GetFont().MakeBold(); }
                    }
                }
            }

        /** @brief Makes the specified row's cells have horizontally centered content.
            @param row The row to have horizontally centered cell content.*/
        void CenterRowHorizontally(const size_t row)
            {
            if (row < m_table.size())
                {
                auto& currentRow = m_table[row];
                for (auto& cell : currentRow)
                    { cell.m_horizontalCellAlignment = PageHorizontalAlignment::Centered; }
                }
            }
        /** @brief Makes the specified column's cells have horizontally centered content.
            @param column The column to have horizontally centered cell content.*/
        void CenterColumnHorizontally(const size_t column)
            {
            if (GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    if (column < row.size())
                        { row[column].m_horizontalCellAlignment = PageHorizontalAlignment::Centered; }
                    }
                }
            }

        /** @brief Across a given row, combines consecutive cells with the same label
                into one cell.
            @details For example, if a top row has three consecutive cells displaying "FY1982,"
                then this will combine them one with "FY1982" centered in it.\n
                This can be useful for showing grouped data or crosstabs.
            @param row The row to combine cells within.*/
        void GroupRow(const size_t row);
        /** @brief Down a given column, combines consecutive cells with the same label
                into one cell.
            @details For example, if the far-left column has three consecutive cells
                displaying "Business," then this will combine them one with "Business"
                centered in it.\n
                This can be useful for showing grouped data or crosstabs.
            @param column The column to combine cells within.*/
        void GroupColumn(const size_t column);

        /// @brief Applies rows of alternating colors ("zebra stripes") to the table.
        /// @param alternateColor The background color to apply to ever other row.
        /// @param startRow The row to start from (default is @c 0).
        /// @param startColumn An optional column in the row to start from.\n
        ///     The default is to start from the first column.
        /// @param endColumn An optional column in the row to end at.\n
        ///     The default is to end at the last column.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void ApplyAlternateRowColors(const wxColour alternateColor,
                                     const size_t startRow = 0, 
                                     std::optional<size_t> startColumn = std::nullopt,
                                     std::optional<size_t> endColumn = std::nullopt);
        /// @}

        /// @name Cell Functions
        /// @brief Functions for editing cells.
        /// @{

        /// @brief Accesses the cell at a given position.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @returns The requested cell.
        /// @throws std::runtime_error If row or column index is out of range, throws an exception.\n
        ///     The exception's @c what() message is UTF-8 encoded, so pass it to
        ///     @c wxString::FromUTF8() when formatting it for an error message.
        TableCell& GetCell(const size_t row, const size_t column)
            {
            wxASSERT_MSG(row < m_table.size(),
                wxString::Format(L"Invalid row index (%zu)!", row));
            wxASSERT_MSG(column < m_table[row].size(),
                wxString::Format(L"Invalid column index (%zu)!", column));
            if (row >= m_table.size() || column >= m_table[row].size())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"Invalid cell index (row %zu, column %zu)."),
                                     row, column).ToUTF8());
                }
            return m_table[row][column];
            }
        /// @}
    private:
        void RecalcSizes(wxDC& dc) final;
        std::vector<std::vector<TableCell>> m_table;
        std::optional<double> m_minWidthProportion;
        std::optional<double> m_minHeightProportion;
        };
    }

/** @}*/

#endif //__WISTERIA_TABLE_H__
