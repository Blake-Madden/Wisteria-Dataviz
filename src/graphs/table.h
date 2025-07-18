/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_TABLE_H
#define WISTERIA_TABLE_H

#include "../math/mathematics.h"
#include "graph2d.h"
#include <variant>
#include <vector>

namespace Wisteria::Graphs
    {
    /** @brief A display of tabular data, which can either be imported from a dataset
            or be built from scratch.

        | %Table with Aggregates      | Styled %Table         |
        | :-------------- | :-------------------------------- |
        | @image html TableMajors.svg width=90% | @image html TablePrograms.svg width=90% |

        @par %Data:
            A table can accept a Data::Dataset, where any type of column can be included.
            Which of these columns to include (as well as their order) can be controlled
            by the caller.\n
            \n
            The table can use the same structure as the dataset, or be transposed (so that the
            columns are then the rows in the table).\n
            \n
            Consecutively repeated group labels across the rows and columns can be collapsed
            into larger cells, giving the appearance of grouped data
            (see GroupRow() and GroupColumn()).
            Numerous other functions are available for customizing the content and appearance of
            cells, rows, and columns (e.g., GetCell() or BoldRow()). Rows and columns can also be
            inserted, which can be useful for group separators.\n
            \n
            After the data has been edited and
            formatted, aggregate columns and rows (e.g., subtotals) can be added via
            InsertAggregateColumn() and InsertAggregateRow().\n \n Finally, once the table is built,
            annotations (see AddCellAnnotation()) and footnotes (see AddFootnote())
            can be added to the table.

        @par Missing Data:
            Any missing data from the dataset will be displayed as an empty cell.

        @par Table Positioning:
            By default, the table will be drawn with a text scaling of @c 1.0 and be scaled down to
            fit within the provided graph area as necessary. The table will then be placed in the
            top-left corner of the graph area, and any extra space remaining will be below
            and to the right of the table. To change this behavior, you can call
            SetPageHorizontalAlignment() and SetPageVerticalAlignment() to position the table
            in a different place within its graph
            area.

            Note that the table's title and caption will affect its calculated width,
            along with its content.

            A table can also be stretched to fit its entire graph area by calling
            SetMinWidthProportion() or SetMinHeightProportion().

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
         tableGraph->SetRowHorizontalPageAlignment(0, PageHorizontalAlignment::Centered);
         // apply a zebra-stripe look
         tableGraph->ApplyAlternateRowColors(ColorBrewer::GetColor(Color::AzureMist), 1, 1);

         // add the table to the canvas
         canvas->SetFixedObject(0, 0, tableGraph);

         // make canvas taller and less wide
         canvas->SetCanvasMinHeightDIPs(
            canvas->GetDefaultCanvasWidthDIPs());
         canvas->SetCanvasMinWidthDIPs(
            canvas->GetDefaultCanvasHeightDIPs());
         // also, fit it to the entire page when printing (preferably in portrait)
         canvas->FitToPageWhenPrinting(true);
        @endcode

        @par Aggregate Row and Column Example:
        @code
         auto juniorSeniorMajors = std::make_shared<Data::Dataset>();
         try
            {
            juniorSeniorMajors->ImportCSV(
                L"/home/isabelle/data/Junior & Senior Majors (Top 20).csv",
                ImportInfo(). ContinuousColumns({ L"Female", L"Male" })
                . CategoricalColumns({ {
                L"Division" }, { L"Department" }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }

         auto tableGraph = std::make_shared<Table>(canvas);
         tableGraph->SetData(juniorSeniorMajors,
            { L"Division", L"Department", L"Female", L"Male" });
         // group the schools together in the first row
         tableGraph->GroupColumn(0);

         // add ratio aggregate column and group row totals
         const wxColour aggColumnBkColor =
            ColorBrewer::GetColor(Colors::Color::LightGray,
                                  Settings::GetTranslucencyValue());
         tableGraph->InsertAggregateColumn(Table::AggregateInfo(AggregateType::Ratio),
                                          _(L"Ratio"), std::nullopt, aggColumnBkColor);
         tableGraph->InsertRowTotals(aggColumnBkColor);

         // make the headers and row groups bold (and center the headers)
         tableGraph->BoldRow(0);
         tableGraph->BoldColumn(0);
         tableGraph->SetRowHorizontalPageAlignment(0, PageHorizontalAlignment::Centered);

         const auto& ratioOutliers =
            // Find outlier in the female-to-male ratios for the majors.
            // (Note that we use a more liberal search, considering
            // z-scores > 2 as being outliers
            tableGraph->GetOutliers(tableGraph->GetColumnCount()-1, 2);
         // if any outliers, make a note of it off to the side
         tableGraph->AddCellAnnotation(
            { L"Majors with the most lopsided female-to-male ratios",
                ratioOutliers, Side::Right, std::nullopt, wxNullColour }
            );

         // if you also want to place annotations on the left of the table,
         // then center it within its drawing area like so:
         // tableGraph->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);

         // add a title
         canvas->GetTopTitles().push_back(Label(
            GraphItemInfo(_(L"Top 20 Majors for Juniors & Seniors (AY2021-22)")).
            Padding(5, 5, 5, 5).Pen(wxNullPen).
            ChildAlignment(RelativeAlignment::FlushLeft).
            FontBackgroundColor(ColorBrewer::GetColor(Color::MossGreen))) );

         tableGraph->GetCaption().SetText(_(L"Source: Office of Institutional Research"));
         tableGraph->GetCaption().SetPadding(5, 5, 5, 5);

         // add the table to the canvas
         canvas->SetFixedObject(0, 0, tableGraph);

         // make the canvas tall since we it's a long table, but not very wide
         canvas->SetCanvasMinHeightDIPs(
            canvas->GetDefaultCanvasWidthDIPs());
         canvas->SetCanvasMinWidthDIPs(
            canvas->GetDefaultCanvasHeightDIPs());

         canvas->FitToPageWhenPrinting(true);
        @endcode
    */
    class Table final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(Table);
        Table() = default;

      public:
        /// @brief Information about how to build an aggregate column/row.
        struct AggregateInfo
            {
            friend class Table;

          public:
            /// @brief Constructor.
            AggregateInfo() = default;

            /// @brief Constructor.
            /// @param type Which type of aggregation to perform.
            explicit AggregateInfo(const AggregateType type) : m_type(type) {}

            /// @brief The type of aggregate column that this is.
            /// @param type The column type.
            /// @returns A self reference.
            AggregateInfo& Type(const AggregateType type) noexcept
                {
                m_type = type;
                return *this;
                }

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

        /// @brief Types of values that can be used for a cell.
        using CellValueType = std::variant<double, wxString, wxDateTime, std::pair<double, double>>;

        /// @brief The row and column position of a cell.
        struct CellPosition
            {
            /// @brief The row of the cell.
            size_t m_row{ 0 };
            /// @brief The column of the cell.
            size_t m_column{ 0 };
            };

        /// @brief An annotation to add to the table, connected to a set of cells.
        struct CellAnnotation
            {
            /// @brief The note to display in the gutter next to the table.
            wxString m_note;
            /// @brief The cells to highlight and connect the note to.
            std::vector<CellPosition> m_cells;
            /// @brief Which side of the table that the note should be on.
            /// @note This will be overridden if the page placement
            ///     of the table conflicts with this option. For example,
            ///     if the table is left aligned in the drawing area,
            ///     then this value will be ignored and the note will always
            ///     appear to the right of the table.
            Side m_side{ Side::Right };
            /// @brief Pen to draw the line connecting highlighted cells with the
            ///     annotation. If set to @c std::nullopt, then the table
            ///     highlight pen will be used.
            std::optional<wxPen> m_connectionLinePen;
            /// @brief Color to fill the cell with. Default is an invalid color
            ///     that won't be used.
            wxColour m_bgColor;
            };

        /// @brief A cell in the table.
        class TableCell
            {
            friend class Table;

          public:
            /// @brief Constructor.
            /// @param value The value for the cell.
            /// @param bgColor The cell's background color.
            /// @param showTopBorder Whether to show the top border.
            /// @param showRightBorder Whether to show the right border.
            /// @param showBottomBorder Whether to show the bottom border.
            /// @param showLeftBorder Whether to show the left border.
            TableCell(CellValueType value, const wxColour& bgColor, const bool showTopBorder = true,
                      const bool showRightBorder = true, const bool showBottomBorder = true,
                      const bool showLeftBorder = true)
                : m_value(std::move(value)), m_bgColor(bgColor), m_showTopBorder(showTopBorder),
                  m_showRightBorder(showRightBorder), m_showBottomBorder(showBottomBorder),
                  m_showLeftBorder(showLeftBorder)
                {
                }

            /// @brief Gets the value as it is displayed in the cell.
            /// @returns The displayable string for the cell.
            [[nodiscard]]
            wxString GetDisplayValue() const;

            /// @returns @c true if the cell is text.
            [[nodiscard]]
            bool IsText() const noexcept
                {
                return (std::get_if<wxString>(&m_value) != nullptr);
                }

            /// @returns @c true if the cell is a number.
            [[nodiscard]]
            bool IsNumeric() const noexcept
                {
                return (std::get_if<double>(&m_value) != nullptr);
                }

            /// @returns @c true if the cell is a date.
            [[nodiscard]]
            bool IsDate() const noexcept
                {
                return (std::get_if<wxDateTime>(&m_value) != nullptr);
                }

            /// @returns @c true if the cell is a ratio.
            [[nodiscard]]
            bool IsRatio() const noexcept
                {
                return (std::get_if<std::pair<double, double>>(&m_value) != nullptr);
                }

            /// @returns The display format of the cell.
            [[nodiscard]]
            TableCellFormat GetFormat() const noexcept
                {
                return m_valueFormat;
                }

            /// @returns The numeric precision being displayed.
            [[nodiscard]]
            uint8_t GetPrecision() const noexcept
                {
                return m_precision;
                }

            /// @brief Sets the value.
            /// @param value The value to set for the cell.
            void SetValue(const CellValueType& value) { m_value = value; }

            /// @brief Sets the color.
            /// @param color The value to set for the cell.
            void SetBackgroundColor(const wxColour& color)
                {
                if (color.IsOk())
                    {
                    m_bgColor = color;
                    }
                }

            /// @returns Access to the cell's font. This can be useful for changing
            ///     an attribute of the font, rather than entirely setting a new font.
            [[nodiscard]]
            wxFont& GetFont() noexcept
                {
                return m_font;
                }

            /// @brief Sets the cell's font.
            /// @param font The font to use.
            void SetFont(const wxFont& font) { m_font = font; }

            /// @returns @c true if cell is being highlighted.
            [[nodiscard]]
            bool IsHighlighted() const noexcept
                {
                return m_isHighlighted;
                }

            /// @brief Draw a highlighted border around the cell.
            /// @param highlight @c true to highlight the cell.
            void Highlight(const bool highlight) noexcept { m_isHighlighted = highlight; }

            /// @returns The character shown on the left edge of the cell.
            [[nodiscard]]
            const wxString& GetPrefix() const noexcept
                {
                return m_prefix;
                }

            /// @brief Adds a character to be shown on the left edge of the cell.
            /// @details This is usually something like a '$' (when using accounting formatting),
            ///     where the character is separated from the main cell value.
            /// @param prefix The character to display.
            /// @note If the cell's format is Percent or Accounting, then this prefix will be
            ///     managed by the cell.
            void SetPrefix(const wxString& prefix) { m_prefix = prefix; }

            /// @returns The label shown in a cell that is less than the suppression threshold.
            [[nodiscard]]
            const wxString& GetSuppressionLabel() const noexcept
                {
                return m_suppressionLabel;
                }

            /// @brief Sets the label shown in a cell that is less than the suppression threshold.
            /// @param label The label to display.
            /// @note Suppression is only applicable to cell with General or Accounting format.
            /// @sa SetSuppressionThreshold().
            void SetSuppressionLabel(const wxString& label) { m_suppressionLabel = label; }

            /// @returns The minimum value that a cell must be before it is suppressed.
            [[nodiscard]]
            std::optional<double> GetSuppressionThreshold() const noexcept
                {
                return m_suppressionThreshold;
                }

            /// @brief The minimum value that a cell must be before it is suppressed.
            /// @param threshold The threshold to use.
            /// @sa SetSuppressionLabel().
            void SetSuppressionThreshold(const std::optional<double> threshold) noexcept
                {
                m_suppressionThreshold = threshold;
                }

            /// @brief Sets an image to appear to the left of the cell's text.
            /// @param bmp The image to use.\n
            ///     Note that this image will be scaled down to the cell's height.
            void SetLeftImage(const wxBitmapBundle& bmp) { m_leftImage = bmp; }

            /// @brief Sets the number of columns that this cell should consume.
            /// @param colCount The number of cells that this should consume horizontally.
            void SetColumnCount(const size_t colCount) noexcept
                {
                if (colCount == 0)
                    {
                    m_columnCount = 1;
                    }
                else
                    {
                    m_columnCount = colCount;
                    }
                }

            /// @brief Sets the number of rows that this cell should consume.
            /// @param rowCount The number of cells that this should consume vertically.
            void SetRowCount(const size_t rowCount) noexcept
                {
                if (rowCount == 0)
                    {
                    m_rowCount = 1;
                    }
                else
                    {
                    m_rowCount = rowCount;
                    }
                }

            /// @brief Sets the suggested line length for the cell (if text).
            /// @param lineLength The suggested line length.
            void SetSuggestedLineLength(const size_t lineLength) noexcept
                {
                m_suggestedLineLength = lineLength;
                }

            /// @brief Shows or hides the left border of a cell.
            /// @param show @c false to hide the left outer border of a cell.
            void ShowLeftBorder(const bool show) { m_showLeftBorder = show; }

            /// @brief Shows or hides the top border of a cell.
            /// @param show @c false to hide the top outer border of a cell.
            void ShowTopBorder(const bool show) { m_showTopBorder = show; }

            /// @brief Shows or hides the bottom border of a cell.
            /// @param show @c false to hide the bottom outer border of a cell.
            void ShowBottomBorder(const bool show) { m_showBottomBorder = show; }

            /// @brief Shows or hides the right border of a cell.
            /// @param show @c false to hide the right outer border of a cell.
            void ShowRightBorder(const bool show) { m_showRightBorder = show; }

            /** @brief Sets how to horizontally position the cell's content.
                @param alignment How to align the content.*/
            void SetPageHorizontalAlignment(const PageHorizontalAlignment alignment) noexcept
                {
                m_horizontalCellAlignment = alignment;
                }

            /** @brief Sets how to align the textual content of the cell (if multi-line).
                @param alignment How to align the content.*/
            void SetTextAlignment(const TextAlignment alignment) noexcept
                {
                m_textAlignment = alignment;
                }

            /// @brief Sets the display format of the cell.
            /// @param cellFormat The format settings to apply.
            void SetFormat(const TableCellFormat cellFormat) noexcept;

          private:
            /// @brief Returns a double value representing the cell.
            /// @details This is useful for comparing cells (or aggregating them).
            /// @returns If numeric, returns the underlying double value.
            ///     If a ratio, returns the magnitude of larger value compared to
            ///     the smaller one.\n
            ///     Otherwise, returns NaN.
            [[nodiscard]]
            double GetDoubleValue() const noexcept
                {
                if (IsNumeric())
                    {
                    if (const auto dVal{ std::get_if<double>(&m_value) })
                        {
                        return *dVal;
                        }
                    else
                        {
                        return std::numeric_limits<double>::quiet_NaN();
                        }
                    }
                else if (IsRatio())
                    {
                    if (const auto rVals{ std::get_if<std::pair<double, double>>(&m_value) })
                        {
                        return (rVals->first >= rVals->second) ?
                                   safe_divide<double>(rVals->first, rVals->second) :
                                   safe_divide<double>(rVals->second, rVals->first);
                        }
                    else
                        {
                        return std::numeric_limits<double>::quiet_NaN();
                        }
                    }
                else
                    {
                    return std::numeric_limits<double>::quiet_NaN();
                    }
                }

            CellValueType m_value{ std::numeric_limits<double>::quiet_NaN() };
            TableCellFormat m_valueFormat{ TableCellFormat::General };
            uint8_t m_precision{ 0 };
            wxColour m_bgColor{ *wxWHITE };
            wxFont m_font{ wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT) };
            wxString m_prefix;
            // setting to true will make the prefix red if the value is negative
            bool m_colorCodePrefix{ false };
            wxBitmapBundle m_leftImage;

            std::optional<PageHorizontalAlignment> m_horizontalCellAlignment;
            std::optional<TextAlignment> m_textAlignment;
            // default to 100, but client can turn this off with std::nullopt
            std::optional<size_t> m_suggestedLineLength{ 100 };

            size_t m_columnCount{ 1 };
            size_t m_rowCount{ 1 };

            bool m_showTopBorder{ true };
            bool m_showRightBorder{ true };
            bool m_showBottomBorder{ true };
            bool m_showLeftBorder{ true };

            bool m_isHighlighted{ false };
            std::optional<double> m_suppressionThreshold{ std::nullopt };
            wxString m_suppressionLabel{ L"---" };
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
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const std::vector<wxString>& columns, const bool transpose = false);

        /// @name Sort Functions
        /// @brief Functions relating to sorting the table.
        /// @{

        /** @brief Sorts the table by the provided column.
            @param columnToSort The column to sort by.
            @param direction The direction to sort by.
            @warning Call this immediately after calling SetData and before any other functions.\n
                Calling this will clear all aggregations, footnotes, formatting commands, etc.*/
        void Sort(const size_t columnToSort, const SortDirection direction);

        /// @brief Sorts the table by the provided column
        ///     (based on a specified order of labels in the column).
        ///     This is similar to what @c forcats::fct_relevel() does in R.
        /// @param columnToSort The column to sort by.
        /// @param labels The labels in the column, sorted in the new order that they should
        ///     appear going downwards.
        /// @param direction The direction to sort by.
        /// @warning Call this immediately after calling SetData and before any other functions.\n
        ///     Calling this will clear all aggregations, footnotes, formatting commands, etc.
        /// @note All cell labels in the provided column must be unique
        ///     for the label ordering to work;
        ///     otherwise, the sort could not be deterministic. If there are multiple cells
        ///     with the same label, then this will return without sorting.
        void Sort(const size_t columnToSort, std::vector<wxString> labels,
                  const SortDirection direction);

        /// @}

        /// @name Table Functions
        /// @brief Functions for editing the table as a whole.
        /// @note Use `GetPen()` to change the outline of the table's cells.
        /// @{

        /// @brief Sets the size of the table.
        /// @details This should only be used if building a table from scratch.
        ///     Prefer using SetData() instead, unless you plan to manually fill
        ///     the data cell-by-cell.\n
        ///     Another use is to call this after setting the data to increase
        ///     the row and/or column counts so that this table has the same
        ///     dimensions as another table. This is useful if you want tables
        ///     next to each other to have a similar appearance (e.g., text scaling)
        ///     and have their rows line up together.
        /// @sa ClearTrailingRowFormatting().
        /// @param rows The number of rows.
        /// @param cols The number of columns.
        /// @note If the table is being made smaller, then existing content
        ///     outside the new size will be removed; other existing content
        ///     will be preserved.\n
        ///     Call ClearTable() to clear any existing content if you wish to
        ///     reset the table.
        void SetTableSize(const size_t rows, const size_t cols)
            {
            m_table.resize(rows);
            for (auto& row : m_table)
                {
                row.resize(cols, TableCell(std::numeric_limits<double>::quiet_NaN(), *wxWHITE,
                                           m_showTopBorder, m_showRightBorder, m_showBottomBorder,
                                           m_showLeftBorder));
                }
            }

        /// @returns The dimensions of the table (row x column).
        /// @note This includes the column header, so 10 rows of data would
        ///     be returned as 11 rows for the table size.
        std::pair<size_t, size_t> GetTableSize() const noexcept
            {
            if (m_table.empty())
                {
                return std::make_pair(0, 0);
                }
            return std::make_pair(m_table.size(), m_table[0].size());
            }

        /// @brief Hides formatting for any rows beyond the data boundary.
        /// @details Formatting includes cell borders and alternating row colors.
        /// @param clearFormatting @c true to hide the formatting for any trailing, empty rows.
        /// @note This will only be relevant if the table was resized (to a larger size) after
        ///     setting the data.
        /// @sa SetTableSize().
        /// @todo Need to remove custom cell background colors also.
        void ClearTrailingRowFormatting(const bool clearFormatting) noexcept
            {
            m_clearTrailingRowFormatting = clearFormatting;
            }

        /// @brief Resets the table, emptying its contents, aggregates, notes, and footnotes.
        void ClearTable() noexcept
            {
            m_table.clear();
            ClearTableInformation();
            }

        /// @brief Resets the table's aggregates, notes, and footnotes.
        /// @note The table's data will stay intact.
        void ClearTableInformation() noexcept
            {
            m_aggregateColumns.clear();
            m_aggregateRows.clear();
            m_cellAnnotations.clear();
            m_footnotes.clear();
            m_currentAggregateColumns.clear();
            m_currentAggregateRows.clear();
            m_hasGroupHeader = false;
            }

        /** @brief Sets the font for the entire table.
            @param ft The font to apply.*/
        void SetTableFont(const wxFont& ft)
            {
            if (ft.IsOk() && GetColumnCount() > 0)
                {
                for (auto& row : m_table)
                    {
                    for (auto& cell : row)
                        {
                        cell.SetFont(ft);
                        }
                    }
                }
            }

        /// @returns The minimum percent of the drawing area's width that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).\n
        ///     Returning @c std::nullopt indicates that the table is autofitting its width.
        const std::optional<double>& GetMinWidthProportion() const noexcept
            {
            return m_minWidthProportion;
            }

        /// @brief Sets the minimum percent of the drawing area's width that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @details The default behavior is for the table to fit its content, with extra
        ///     space around it (depending on how wide it is).
        /// @param percent The minimum percent of the area's width that the table should consume.\n
        ///     Set to @c std::nullopt to turn this feature off and
        ///     allow the table to autofit its width.
        /// @sa GetPageHorizontalAlignment() for controlling how the table is positioned if this
        ///     value is less than @c 1.0.
        void SetMinWidthProportion(const std::optional<double> percent)
            {
            m_minWidthProportion =
                percent.has_value() ?
                    std::optional<double>(
                        std::clamp(percent.value(), math_constants::empty, math_constants::full)) :
                    std::nullopt;
            }

        /// @returns The minimum percent of the drawing area's height that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).\n
        ///     Returning @c std::nullopt indicates that the table is autofitting its height.
        const std::optional<double>& GetMinHeightProportion() const noexcept
            {
            return m_minHeightProportion;
            }

        /// @brief Sets the minimum percent of the drawing area's height that the
        ///     table should consume (between @c 0.0 to @c 1.0, representing 0% to 100%).
        /// @details The default behavior is for the table to fit its content, with extra
        ///     space around it (depending on how tall it is).
        /// @param percent The minimum percent of the area's height that the table should consume.
        /// @sa GetPageVerticalAlignment() for controlling how the table is positioned if this
        ///     value is less than @c 1.0.
        void SetMinHeightProportion(const std::optional<double> percent)
            {
            m_minHeightProportion =
                percent.has_value() ?
                    std::optional<double>(
                        std::clamp(percent.value(), math_constants::empty, math_constants::full)) :
                    std::nullopt;
            }

        /// @returns The pen used to highlight specific cells (e.g., outliers).
        [[nodiscard]]
        wxPen& GetHighlightPen() noexcept
            {
            return m_highlightPen;
            }

        /// @}

        /// @name Row & Column Functions
        /// @brief Functions for editing rows and columns.
        /// @{

        /// @returns The number of rows.
        /// @warning This will include the first row which contains the original dataset's
        ///     column names (unless it was transposed in the call to SetData()).
        [[nodiscard]]
        size_t GetRowCount() const noexcept
            {
            return m_table.size();
            }

        /// @returns The number of columns.
        /// @warning If the imported file was transposed, then this will also include
        ///     the first column which contains the dataset's original column names.
        [[nodiscard]]
        size_t GetColumnCount() const noexcept
            {
            return (GetRowCount() == 0) ? 0 : m_table[0].size();
            }

        /// @returns The last column that is not an aggregate column.
        [[nodiscard]]
        std::optional<size_t> GetLastDataColumn() const
            {
            if (GetColumnCount() == 0)
                {
                return std::nullopt;
                }
            else if (m_currentAggregateColumns.empty())
                {
                return GetColumnCount() - 1;
                }
            else
                {
                int64_t lastCol = GetColumnCount() - 1;
                while (m_currentAggregateColumns.contains(lastCol))
                    {
                    --lastCol;
                    }
                return (lastCol >= 0) ? std::optional<size_t>(lastCol) : std::nullopt;
                }
            }

        /// @returns The last row that is not an aggregate row.
        [[nodiscard]]
        std::optional<size_t> GetLastDataRow() const
            {
            if (GetRowCount() == 0)
                {
                return std::nullopt;
                }
            else if (m_currentAggregateRows.empty())
                {
                return GetRowCount() - 1;
                }
            else
                {
                int64_t lastRow = GetRowCount() - 1;
                while (m_currentAggregateRows.contains(lastRow))
                    {
                    --lastRow;
                    }
                return (lastRow >= 0) ? std::optional<size_t>(lastRow) : std::nullopt;
                }
            }

        /** @brief Inserts a top row filled with the provided group names
                based on the values in the data's first row.
            @details For example, if the first row contains labels such as "Graduated 1997"
                and "Graduated 1998," then the newly inserted top row will say "Graduated"
                above them (as a combined group). Also, the original labels will now
                be trimmed to "1997" and "1998" as their group header now indicated "Graduated."
            @param groups The groups to look for in the first row.
            @warning This is destructive to the first row, as it will remove the
                prefix from any label that matches anything from @c groups.\n
                Also, it is assumed that the labels in the first row are already
                ordered and grouped together.*/
        void InsertGroupHeader(const std::vector<wxString>& groups);

        /// @brief Inserts an empty row at the given index.
        /// @details For example, an index of @c 0 will insert the row at the
        ///     top of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will be ignored since the
        ///     number of columns is unknown.
        /// @param rowIndex Where to insert the row.
        /// @returns @c true if row was inserted.
        bool InsertRow(const size_t rowIndex)
            {
            if (GetColumnCount())
                {
                m_table.insert(
                    m_table.cbegin() +
                        // clamp indices going beyond the row count to m_table.cend()
                        std::clamp<size_t>(rowIndex, 0, GetRowCount()),
                    std::vector<TableCell>(GetColumnCount(),
                                           TableCell(std::numeric_limits<double>::quiet_NaN(),
                                                     *wxWHITE, m_showTopBorder, m_showRightBorder,
                                                     m_showBottomBorder, m_showLeftBorder)));
                return true;
                }
            return false;
            }

        /// @brief Inserts an empty column at the given index.
        /// @details For example, an index of @c 0 will insert the column on the
        ///     left side of the table.
        /// @note If the table's size has not been established yet (via SetData()
        ///     or SetTableSize()), then calls to this will be ignored since there will
        ///     be no rows to insert columns into.
        /// @param colIndex Where to insert the column.
        void InsertColumn(const size_t colIndex)
            {
            if (GetColumnCount())
                {
                for (auto& row : m_table)
                    {
                    row.insert(row.cbegin() +
                                   // clamp indices going beyond the column count to row.cend()
                                   std::clamp<size_t>(colIndex, 0, row.size()),
                               TableCell(std::numeric_limits<double>::quiet_NaN(), *wxWHITE,
                                         m_showTopBorder, m_showRightBorder, m_showBottomBorder,
                                         m_showLeftBorder));
                    }
                }
            }

        /** @brief Adds an aggregate (e.g., total) row to the end of the table.
            @param aggInfo Which type of aggregation to use in the row.
            @param rowName An optional value for the first column of the new
                row, representing a name for the row.\n
                This will be overwritten by a calculated value if the left-most column is not text.
            @param rowIndex Where to (optionally) insert the row.\n
                The default is to insert as the last row.
            @param bkColor An optional background for the row.
            @param borders An optional override of the default borders for the cells in this column.
            @note This should be called after all data has been set because the
                aggregation values are calculated as this function is called.\n
                Also, if suppression is enabled, then suppressed values will be treated as NaN
                and may affect the aggregate accordingly.
            @sa InsertRowTotals() for a simplified way to insert a total row
                (as well as subtotal rows).*/
        void InsertAggregateRow(const AggregateInfo& aggInfo,
                                std::optional<wxString> rowName = std::nullopt,
                                std::optional<size_t> rowIndex = std::nullopt,
                                const std::optional<wxColour>& bkColor = std::nullopt,
                                std::optional<std::bitset<4>> borders = std::nullopt);
        /** @brief Adds an aggregate (e.g., total) column into the table.
            @param aggInfo Which type of aggregation to use in the column.
            @param colName An optional value for the first row of the new
                column, representing a name for the column.\n
                This will be overwritten by a calculated value if the top row is not text.
            @param colIndex Where to (optionally) insert the column. The default
                is to insert as the last column.
            @param useAdjacentColors @c true to use the color of the cell adjacent to this column.
                @c false will apply a light gray to the column.\n
                @c true is recommended if using alternate row colors.\n
                Note that this will override the @c bkColor if this is @c true.
            @param bkColor A color to fill the column with.\n
                Note that this is ignored if @c useAdjacentColors is @c true.
            @param borders An optional override of the default borders for the
                cells in this column.
            @note This should be called after all data has been set because the
                aggregation values are calculated as this function is called.\n
                Also, if suppression is enabled, then suppressed values will be treated as NaN
                and may affect the aggregate accordingly.*/
        void InsertAggregateColumn(const AggregateInfo& aggInfo,
                                   const std::optional<wxString>& colName = std::nullopt,
                                   const std::optional<size_t> colIndex = std::nullopt,
                                   const std::optional<bool> useAdjacentColors = std::nullopt,
                                   const std::optional<wxColour>& bkColor = std::nullopt,
                                   const std::optional<std::bitset<4>> borders = std::nullopt);
        /** @brief Inserts total (and possibly subtotal) rows into a table.
            @details If the first column contains grouped labels (see GroupColumn())
                and the second column contains labels, then subtotal rows will be inserted
                beneath each parent group. Also, a grand total row will be inserted at the
                bottom of the table.\n
                Otherwise, a single total row will be inserted at the bottom for all rows.
            @param bkColor An optional background for the row(s).*/
        void InsertRowTotals(const std::optional<wxColour>& bkColor = std::nullopt);

        /// @brief Sets the background color for a given row.
        /// @param row The row to change.
        /// @param color The background color to apply to the row.
        /// @param columnStops An optional list of columns within the row to skip.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void
        SetRowBackgroundColor(const size_t row, const wxColour& color,
                              const std::optional<std::set<size_t>>& columnStops = std::nullopt);

        /// @brief Sets the background color for a given column.
        /// @param column The column to change.
        /// @param color The background color to apply to the column.
        /// @param rowStops An optional list of rows within the column to skip.
        /// @note This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void
        SetColumnBackgroundColor(const size_t column, const wxColour& color,
                                 const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (color.IsOk())
                {
                if (GetColumnCount() > 0 && column < GetColumnCount())
                    {
                    for (size_t i = 0; i < m_table.size(); ++i)
                        {
                        if (rowStops.has_value() && rowStops.value().contains(i))
                            {
                            continue;
                            }
                        auto& cell = m_table[i][column];
                        cell.m_bgColor = color;
                        }
                    }
                }
            }

        /** @brief Makes the specified row use a bold font.
            @param row The row to make bold.
            @param columnStops An optional list of columns within the row to skip.*/
        void BoldRow(const size_t row,
                     const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.GetFont().MakeBold();
                    }
                }
            }

        /** @brief Makes the specified column use a bold font.
            @param column The column to make bold.
            @param rowStops An optional list of rows within the column to skip.*/
        void BoldColumn(const size_t column,
                        const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.GetFont().MakeBold();
                    }
                }
            }

        /** @brief Highlights the cells across the specified row.
            @param row The row to make bold.
            @param columnStops An optional list of columns within the row to skip.*/
        void HighlightRow(const size_t row,
                          const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.Highlight(true);
                    }
                }
            }

        /** @brief Highlights the cells down the specified column.
            @param column The column to make bold.
            @param rowStops An optional list of rows within the column to skip.*/
        void HighlightColumn(const size_t column,
                             const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.Highlight(true);
                    }
                }
            }

        /// @returns @c true if the top border for cells is shown by default.
        [[nodiscard]]
        bool IsShowingTopBorder() const noexcept
            {
            return m_showTopBorder;
            }

        /// @returns @c true if the right border for cells is shown by default.
        [[nodiscard]]
        bool IsShowingRightBorder() const noexcept
            {
            return m_showRightBorder;
            }

        /// @returns @c true if the bottom border for cells is shown by default.
        [[nodiscard]]
        bool IsShowingBottomBorder() const noexcept
            {
            return m_showBottomBorder;
            }

        /// @returns @c true if the left border for cells is shown by default.
        [[nodiscard]]
        bool IsShowingLeftBorder() const noexcept
            {
            return m_showLeftBorder;
            }

        /** @brief Sets the borders for all future cells.
            @note Call this prior to adding any rows or columns for this to take effect.
            @param showTopBorder Whether to show the cells' top borders.
            @param showRightBorder Whether to show the cells' right borders.
            @param showBottomBorder Whether to show the cells' bottom borders.
            @param showLeftBorder Whether to show the cells' left borders.*/
        void SetDefaultBorders(const bool showTopBorder, const bool showRightBorder,
                               const bool showBottomBorder, const bool showLeftBorder)
            {
            m_showTopBorder = showTopBorder;
            m_showRightBorder = showRightBorder;
            m_showBottomBorder = showBottomBorder;
            m_showLeftBorder = showLeftBorder;
            }

        /** @brief Sets the borders for all cells across the specified row.
            @param row The row to edit.
            @param showTopBorder Whether to show the cells' top borders.
            @param showRightBorder Whether to show the cells' right borders.
            @param showBottomBorder Whether to show the cells' bottom borders.
            @param showLeftBorder Whether to show the cells' left borders.
            @param columnStops An optional list of columns within the row to skip.*/
        void SetRowBorders(const size_t row, const std::optional<bool> showTopBorder,
                           const std::optional<bool> showRightBorder,
                           const std::optional<bool> showBottomBorder,
                           const std::optional<bool> showLeftBorder,
                           const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    if (showTopBorder.has_value())
                        {
                        cell.ShowTopBorder(showTopBorder.value());
                        }
                    if (showRightBorder.has_value())
                        {
                        cell.ShowRightBorder(showRightBorder.value());
                        }
                    if (showBottomBorder.has_value())
                        {
                        cell.ShowBottomBorder(showBottomBorder.value());
                        }
                    if (showLeftBorder.has_value())
                        {
                        cell.ShowLeftBorder(showLeftBorder.value());
                        }
                    }
                }
            }

        /** @brief Sets the borders for all cells down the specified column.
            @param column The column to edit.
            @param showTopBorder Whether to show the cells' top borders.
            @param showRightBorder Whether to show the cells' right borders.
            @param showBottomBorder Whether to show the cells' bottom borders.
            @param showLeftBorder Whether to show the cells' left borders.
            @param rowStops An optional list of rows within the column to skip.*/
        void SetColumnBorders(const size_t column, std::optional<bool> showTopBorder,
                              const std::optional<bool> showRightBorder,
                              const std::optional<bool> showBottomBorder,
                              const std::optional<bool> showLeftBorder,
                              const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    if (showTopBorder.has_value())
                        {
                        cell.ShowTopBorder(showTopBorder.value());
                        }
                    if (showRightBorder.has_value())
                        {
                        cell.ShowRightBorder(showRightBorder.value());
                        }
                    if (showBottomBorder.has_value())
                        {
                        cell.ShowBottomBorder(showBottomBorder.value());
                        }
                    if (showLeftBorder.has_value())
                        {
                        cell.ShowLeftBorder(showLeftBorder.value());
                        }
                    }
                }
            }

        /** @brief Sets the specified row's precision.
            @param row The row to edit.
            @param precision The precision for the row.
            @param columnStops An optional list of columns within the row to skip.
            @note Cells' default precision is zero.*/
        void SetRowPrecision(const size_t row, const uint8_t precision,
                             const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.m_precision = precision;
                    }
                }
            }

        /** @brief Sets the specified column's precision.
            @param column The column to edit.
            @param precision The precision for the column.
            @param rowStops An optional list of rows within the column to skip.
            @note Cells' default precision is zero.*/
        void SetColumnPrecision(const size_t column, const uint8_t precision,
                                const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.m_precision = precision;
                    }
                }
            }

        /** @brief Sets the specified row's cells' horizontal content alignment.
            @param row The row to have horizontally centered cell content.
            @param alignment How to align the content within the row's cells.
            @param columnStops An optional list of columns within the row to skip.*/
        void SetRowHorizontalPageAlignment(
            const size_t row, const PageHorizontalAlignment alignment,
            const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.SetPageHorizontalAlignment(alignment);
                    }
                }
            }

        /** @brief Sets the specified column's cells' horizontal content alignment.
            @param column The column to have horizontally centered cell content.
            @param alignment How to align the content within the column's cells.
            @param rowStops An optional list of rows within the column to skip.*/
        void SetColumnHorizontalPageAlignment(
            const size_t column, const PageHorizontalAlignment alignment,
            const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.SetPageHorizontalAlignment(alignment);
                    }
                }
            }

        /** @brief Sets how suppression is used for a row.
            @param row The row to edit.
            @param threshold The minimum value that a cell must be before it is suppressed.
            @param suppressionLabel The label shown in a cell that is
                less than the suppression threshold.
            @param columnStops An optional list of columns within the row to skip.*/
        void SetRowSuppression(const size_t row, const std::optional<double> threshold,
                               const std::optional<wxString>& suppressionLabel,
                               const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.SetSuppressionThreshold(threshold);
                    if (suppressionLabel.has_value())
                        {
                        cell.SetSuppressionLabel(suppressionLabel.value());
                        }
                    }
                }
            }

        /** @brief Sets how suppression is used for a column.
            @param column The column to edit.
            @param threshold The minimum value that a cell must be before it is suppressed.
            @param suppressionLabel The label shown in a cell that is
                less than the suppression threshold.
            @param rowStops An optional list of rows within the column to skip.*/
        void SetColumnSuppression(const size_t column, const std::optional<double> threshold,
                                  const std::optional<wxString>& suppressionLabel,
                                  const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.SetSuppressionThreshold(threshold);
                    if (suppressionLabel.has_value())
                        {
                        cell.SetSuppressionLabel(suppressionLabel.value());
                        }
                    }
                }
            }

        /** @brief Sets the specified row's cells' display format.
            @param row The row to edit.
            @param format The display format to apply.
            @param columnStops An optional list of columns within the row to skip.*/
        void SetRowFormat(const size_t row, const TableCellFormat format,
                          const std::optional<std::set<size_t>>& columnStops = std::nullopt)
            {
            if (row < GetRowCount())
                {
                auto& currentRow = m_table[row];
                for (size_t i = 0; i < currentRow.size(); ++i)
                    {
                    if (columnStops.has_value() && columnStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = currentRow[i];
                    cell.SetFormat(format);
                    }
                }
            }

        /** @brief Sets the specified column's cells' display format.
            @param column The column to edit.
            @param format The display format to apply.
            @param rowStops An optional list of rows within the column to skip.*/
        void SetColumnFormat(const size_t column, const TableCellFormat format,
                             const std::optional<std::set<size_t>>& rowStops = std::nullopt)
            {
            if (GetColumnCount() > 0 && column < GetColumnCount())
                {
                for (size_t i = 0; i < m_table.size(); ++i)
                    {
                    if (rowStops.has_value() && rowStops.value().contains(i))
                        {
                        continue;
                        }
                    auto& cell = m_table[i][column];
                    cell.SetFormat(format);
                    }
                }
            }

        /** @brief Across a given row, combines consecutive cells with the same label
                into one cell.
            @details For example, if a top row has three consecutive cells displaying "FY1982,"
                then this will combine them one with "FY1982" centered in it.\n
                This can be useful for showing grouped data.
            @param row The row to combine cells within.*/
        void GroupRow(const size_t row);
        /** @brief Down a given column, combines consecutive cells with the same label
                into one cell.
            @details For example, if the far-left column has three consecutive cells
                displaying "Business," then this will combine them one with "Business"
                centered in it.\n
                This can be useful for showing grouped data.
            @param column The column to combine cells within.*/
        void GroupColumn(const size_t column);

        /** @brief Finds the outliers from the specified column.
            @details This can be used for highlighting outliers (and possibly annotating them).
            @sa AddCellAnnotation().
            @param column The column to review.
            @param outlierThreshold The z-score threshold for determining if a value
                is an outlier. Normally this should be @c 3.0 (representing a value
                being three standard deviations from the mean). A lower value will
                be more liberal in classifying a value as an outlier; a higher value
                will be more strict.
            @returns The cell positions of the outliers.
            @warning Any changes to the structure of the table (inserting more rows or columns)
                will make the returned positions incorrect. This should be
                called after all structural changes to the table have been made
                (including the addition of aggregates).*/
        std::vector<CellPosition> GetOutliers(const size_t column,
                                              const double outlierThreshold = 3.0);

        /** @brief Finds the top N values from the specified column.
            @details This can be used for highlighting max items (and possibly annotating them).
            @sa AddCellAnnotation().
            @param column The column to review.
            @param N The number of top (unique) values to search for.
            @returns The cell positions of the outliers. (May return ties.)
            @note In the case of ties, multiple cells will be returned.
            @warning Any changes to the structure of the table (inserting more rows or columns)
                will make the returned positions incorrect. This should be
                called after all structural changes to the table have been made
                (including the addition of aggregates).*/
        std::vector<CellPosition> GetTopN(const size_t column, const size_t N = 1);

        /// @brief Applies rows of alternating colors ("zebra stripes") to the table.
        /// @param baseColor The base background color, where a shaded or tinted version will
        ///     be applied to ever other row.
        /// @param startRow The row to start from (default is @c 0).
        /// @param columnStops An optional list of columns within the row to skip.
        /// @note If any cells are multi-row, then the zebra stripes will be applied only to
        ///     the visible cells.
        /// @warning This will have no effect until the table's dimensions have been specified
        ///     via SetData() or SetTableSize().
        void
        ApplyAlternateRowColors(const wxColour& baseColor, const size_t startRow = 0,
                                const std::optional<std::set<size_t>>& columnStops = std::nullopt);
        /// @}

        /// @name Cell Functions
        /// @brief Functions for editing cells.
        /// @{

        /// @brief Accesses the cell at a given position.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @returns The requested cell.
        /// @throws std::runtime_error If row or column index is out of range,
        ///     throws an exception.\n
        ///     The exception's @c what() message is UTF-8 encoded, so pass it to
        ///     @c wxString::FromUTF8() when formatting it for an error message.
        [[nodiscard]]
        TableCell& GetCell(const size_t row, const size_t column);

        /// @brief Highlights the specified cell(s) and adds a note pointing to them.
        /// @param cellNote Information about the cell(s) to highlight,
        ///     the note, and where to place it relative to the table.
        /// @note If the table's minimum width is set to fill the entire width, then
        ///     this will be turned off and the table will autofit width-wise.
        void AddCellAnnotation(const CellAnnotation& cellNote);

        /** @brief Searches for the first cell whose content matches the provided text.
            @param textToFind The text to search for.
            @returns A pointer to the first cell that matches the text,
                or null if not found.*/
        [[nodiscard]]
        TableCell* FindCell(const wxString& textToFind);

        /** @brief Searches for the first cell whose content matches the provided text.
            @param textToFind The text to search for.
            @returns The position of the cell, or `std::nullopt` if not found.*/
        [[nodiscard]]
        std::optional<CellPosition> FindCellPosition(const wxString& textToFind) const;

        /** @brief Searches for the position of the first cell whose content matches
                the provided text in the first row.
            @param textToFind The text to search for.
            @returns The index of the column, or @c std::nullopt if not found.*/
        [[nodiscard]]
        std::optional<size_t> FindColumnIndex(const wxString& textToFind) const;

        /** @brief Searches for the position of the first cell whose content
                matches the provided text in the first column.
            @param textToFind The text to search for.
            @returns The index of the row, or @c std::nullopt if not found.*/
        [[nodiscard]]
        std::optional<size_t> FindRowIndex(const wxString& textToFind) const;
        /// @}

        /** @brief Adds a footnote to the table.
            @param cellValue The value (as a displayed string) to look for in the table,
                which will have a citation number added after it.
            @param footnote The respective footnote to add to the caption.
            @note Up to nine footnotes are supported. Also, if the provided @c cellValue
                is not found in the table, then the footnote will not be added.\n
                Also, if @c footnote is empty, then @c cellValue will have a number
                shown after it, but no respective footnote entry will appear in the caption.
            @warning Adding a footnote will overwrite the existing caption.*/
        void AddFootnote(const wxString& cellValue, const wxString& footnote);

        /// @private
        void AddCellAnnotation(CellAnnotation&& cellNote);

      private:
        [[deprecated("Tables do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) override final
            {
            return nullptr;
            }

        void CalcMainTableSize(std::vector<wxCoord>& columnWidths, std::vector<wxCoord>& rowHeights,
                               GraphItems::Label& measuringLabel, wxDC& dc) const;

        /// @returns The rectangular area of the plot area. This is relative to its parent canvas.
        [[nodiscard]]
        wxRect GetContentRect() const noexcept override final
            {
            return wxRect();
            }

        void RecalcSizes(wxDC& dc) override final;

        void AdjustTextLabelToCell(const TableCell& cell,
                                   Wisteria::GraphItems::Label& cellLabel) const;

        /** @brief Determines which gutter a note should go into.
            @details Notes will have its gutter specified, but the tables page
                placement may conflict with this value. This function will determine
                if there is such a conflict and return the appropriate gutter.
            @param note The annotation to review.
            @returns The gutter that the note should go into.*/
        [[nodiscard]]
        Side DeduceGutterSide(const CellAnnotation& note) const noexcept
            {
            return ((note.m_side == Side::Right &&
                     GetPageHorizontalAlignment() != PageHorizontalAlignment::RightAligned) ||
                    // left side, but table is left aligned and there is no space for it
                    (note.m_side == Side::Left &&
                     GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned)) ?
                       Side::Right :
                       Side::Left;
            }

        /** @brief Calculates the table's cell dimensions, overall width and height,
                adjusted drawing area. This drawing area includes the table and its
                gutter annotations.
            @param[out] columnWidths The calculated column widths.
            @param[out] rowHeights The calculated row heights.
            @param[in,out] drawArea The initial and updated drawing area for the table.
            @param dc The DC to measure text labels with.
            @returns The size of the table proper (i.e., the cells, but not outer annotations).\n
                Note that the size of @c drawArea may be larger if the table includes
                annotations along the gutters.*/
        [[nodiscard]]
        wxSize CalculateTableSize(std::vector<wxCoord>& columnWidths,
                                  std::vector<wxCoord>& rowHeights, wxRect& drawArea,
                                  wxDC& dc) const;

        /// @returns The area of a given cell if found; otherwise, an invalid rect.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        /// @warning This should only be called during or after a call to RecalcSizes().
        [[nodiscard]]
        wxRect GetCachedCellRect(const size_t row, const size_t column) const;

        /// @returns If a cell is being eclipsed vertically by the cell above it,
        ///     then return that cell. Otherwise, return @c std::nullopt.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        [[nodiscard]]
        std::optional<TableCell> GetEclipsingRowWiseCell(const size_t row, const size_t column);
        /// @returns If a cell is being eclipsed horizontally by the cell left of it,
        ///     then return that cell. Otherwise, return @c std::nullopt.
        /// @param row The row index of the cell.
        /// @param column The column index of the cell.
        [[nodiscard]]
        std::optional<TableCell> GetEclipsingColumnWiseCell(const size_t row, const size_t column);

        /** @brief Calculates an aggregation from a series of values.
            @param aggInfo Which type of aggregate calculation to perform.
            @param aggCell The cell to place the result.
            @param values The values for the calculation.*/
        static void CalculateAggregate(const AggregateInfo& aggInfo, TableCell& aggCell,
                                       const std::vector<double>& values);

        // row x column
        std::pair<size_t, size_t> m_dataSize{ std::make_pair(0, 0) };
        bool m_clearTrailingRowFormatting{ false };

        std::map<size_t, AggregateType> m_aggregateColumns;
        std::map<size_t, AggregateType> m_aggregateRows;

        // DIPs for annotation connection lines and space between lines
        constexpr static wxCoord m_labelSpacingFromLine{ 5 };
        constexpr static wxCoord m_connectionOverhangWidth{ 10 };

        std::vector<wxString> m_footnotes;

        std::vector<std::vector<TableCell>> m_table;
        std::optional<double> m_minWidthProportion;
        std::optional<double> m_minHeightProportion;

        std::vector<CellAnnotation> m_cellAnnotations;

        wxPen m_highlightPen{ wxPen(*wxRED_PEN) };

        // default borders
        bool m_showTopBorder{ true };
        bool m_showRightBorder{ true };
        bool m_showBottomBorder{ true };
        bool m_showLeftBorder{ true };

        // cached aggregate info
        std::set<size_t> m_currentAggregateColumns;
        std::set<size_t> m_currentAggregateRows;

        bool m_hasGroupHeader{ false };

        // cached values
        std::vector<std::vector<wxRect>> m_cachedCellRects;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_TABLE_H
