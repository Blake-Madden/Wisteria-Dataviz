/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef SANKEY_DIAGRAM_H
#define SANKEY_DIAGRAM_H

#include "../util/frequencymap.h"
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief Flow diagram, showing the flow of one series of groups into another series of groups.
        @details This implementation supports a two-level flow (one column of groups flowing into another column).

        @par %Data:
         This plot accepts a Data::Dataset where two categorical columns represent the start and end
         groups. Optionally, two continuous columns can also be used as weight columns for the
         "from" and "to" columns. Finally, an optional group column can be provided to group the "from"
         groups on the left-side.

         The following dataset shows using a "from" and "to" column, where the frequency counts of
         the columns' groups will be used. The gender groups (i.e., "Male", "Female") will appear on
         the left, and the number of those that passed and failed will flow to the right side of the diagram.

         | From   | To   |
         | :--    | :--  |
         | Male   | Pass |
         | Male   | Pass |
         | Female | Fail |
         | Female | Pass |
         | Female | Pass |

         ...

         This example dataset includes the "from" and "to" groups, and also weight variables.
         *Graduated* is the weight applied to *From* and *Enrolled* would be applied to *To*.
         By doing this, the diagram would show large groups for the feeder schools (e.g., "West HS")
         on the left, and how many of their respective graduating students matriculated to
         "Miskatonic University." Finally, the "County" column can be used to sort the groups
         for the "From" column. This will order the labels from the "From" column into groups
         and show axis brackets to the left of these groups showing the counties' names.
         (Note that if any group from "From" appears in multiple values from "County,"
          then it will be sorted by the first county that it encountered.)

         | From        | Graduated | To                    | Enrolled | County    |
         | :--         | --:       | :--                   | --:      | :--       |
         | Westland HS | 150       | Miskatonic University | 13       | Berkshire |
         | Lincoln HS  | 175       | Miskatonic University | 2        | Franklin  |
         | West HS     | 197       | Miskatonic University | 0        | Berkshire |

         ...

        @par Missing Data:
         - Any missing data in an observation will result in listwise deletion.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 1);

         SetTitle(_(L"Sankey Diagram"));

         auto sankeyData = std::make_shared<Data::Dataset>();
         try
             {
             sankeyData->ImportCSV(appDir + L"/datasets/historical/Titanic.csv",
                 ImportInfo().
                 CategoricalColumns({
                     { L"Sex", CategoricalImportMethod::ReadAsStrings },
                     { L"Embarked", CategoricalImportMethod::ReadAsStrings },
                     { L"Pclass", CategoricalImportMethod::ReadAsIntegers },
                     { L"Survived", CategoricalImportMethod::ReadAsIntegers }
                     }));
             }
         catch (const std::exception& err)
             {
             wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                          _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
             return;
             }

         auto sankey = std::make_shared<SankeyDiagram>(canvas);
         sankey->SetData(sankeyData, L"Sex", L"Survived", std::nullopt);
         sankey->SetCanvasMargins(5, 5, 5, 5);

         canvas->SetFixedObject(0, 0, sankey);

        @endcode
    */
    // clang-format on

    class SankeyDiagram : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(SankeyDiagram);
        SankeyDiagram() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the diagram on.
            @param brushes The brush scheme to draw with.*/
        explicit SankeyDiagram(
            Canvas* canvas,
            const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr);

        /** @brief Sets the data.
            @param data The data to use for the plot.
            @param fromColumnName The column containing the left-side groups.
            @param toColumnName The column containing the right-side groups that the
                left-side groups flow into.
            @param fromWeightColumnName The (optional) column containing the multiplier value for
                the "from" column.
            @param toWeightColumnName The (optional) column containing the multiplier value for
                the "to" column.
            @param fromSortColumnName The (optional) column used to sort the groups in the
                from column.\n
                Note that if a group from @c fromWeightColumnName occurs with multiple values
                from this column, then it will be sorted by the first value from this column
                that it encountered.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                Also throws an exception if only @c fromWeightColumnName is provided but
                @c toWeightColumnName was not (or vice versa).\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& fromColumnName, const wxString& toColumnName,
                     const std::optional<wxString>& fromWeightColumnName,
                     const std::optional<wxString>& toWeightColumnName,
                     const std::optional<wxString>& fromSortColumnName);

        /// @name Style functions
        /// @brief Functions relating to how the diagram is displayed.
        /// @{

        /// @returns The shape of the streams going between the groups.
        [[nodiscard]]
        FlowShape GetFlowShape() const noexcept
            {
            return m_flowShape;
            }

        /// @brief Sets the shape of the streams going between the groups.
        /// @param shape The shape to use.
        void SetFlowShape(FlowShape shape) noexcept { m_flowShape = shape; }

        /// @returns The format of the labels shown on the group boxes.
        [[nodiscard]]
        BinLabelDisplay GetGroupLabelDisplay() const noexcept
            {
            return m_groupLabelDisplay;
            }

        /// @brief Sets the format of the labels shown on the group boxes.
        /// @param labelDisplay The format to use.
        void SetGroupLabelDisplay(const BinLabelDisplay labelDisplay) noexcept
            {
            m_groupLabelDisplay = labelDisplay;
            }

        /// @returns How the columns (i.e., groups from each variable) display
        ///     their variable's name.
        [[nodiscard]]
        GraphColumnHeader GetColumnHeaderDisplay() const noexcept
            {
            return m_columnDisplay;
            }

        /// @brief Sets how the columns (i.e., groups from each variable) display
        ///     their variable's name.
        /// @param columnDisplay The display method to use.
        /// @sa SetColumnHeaders().
        void SetColumnHeaderDisplay(const GraphColumnHeader columnDisplay) noexcept
            {
            m_columnDisplay = columnDisplay;
            }

        /// @brief Sets the column headers to display above or below the columns.
        /// @details Syntax such as `@COLUMNNAME@` and `@COUNT@` can be
        ///     embedded in this string. (These expand to the column and
        ///     number of observations, respectively).
        /// @param colHeaders The strings to use as the columns' headers.
        /// @sa SetColumnHeaderDisplay().
        void SetColumnHeaders(const std::vector<wxString>& colHeaders)
            {
            m_columnHeaders = colHeaders;
            }

        /// @}

        /// @private
        [[deprecated("Sankey diagram does not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            wxFAIL_MSG(L"Sankey diagram does not support legends.");
            return nullptr;
            }

      private:
        class SankeyGroup
            {
          public:
            using DownStreamGroups = aggregate_frequency_set<wxString, Data::wxStringLessNoCase>;

            explicit SankeyGroup(wxString label) : m_label(std::move(label)) {}

            SankeyGroup(wxString label, const double freq, DownStreamGroups downStreamGroups)
                : m_label(std::move(label)), m_frequency(freq),
                  m_downStreamGroups(std::move(downStreamGroups))
                {
                }

            wxString m_label;
            double m_frequency{ 0 };
            double m_percentOfColumn{ 0 };
            double m_currentYAxisPosition{ 0 };
            double m_yAxisTopPosition{ 0 };
            double m_yAxisBottomPosition{ 0 };
            double m_yAxisWidth{ 0 };
            double m_xAxisLeft{ 0 };
            double m_xAxisRight{ 0 };
            bool m_isShown{ true };
            DownStreamGroups m_downStreamGroups;

            void OffsetY(const double offset) noexcept
                {
                m_yAxisTopPosition += offset;
                m_yAxisBottomPosition += offset;
                m_currentYAxisPosition += offset;
                }

            [[nodiscard]]
            bool operator<(const SankeyGroup& that) const
                {
                return m_label.CmpNoCase(that.m_label) < 0;
                }

            [[nodiscard]]
            bool operator==(const SankeyGroup& that) const
                {
                return m_label.CmpNoCase(that.m_label) == 0;
                }
            };

        struct SankeyAxisGroup
            {
            wxString m_label;
            size_t m_startGroup{ 0 };
            size_t m_endGroup{ 0 };
            };

        using SankeyColumn = std::vector<SankeyGroup>;

        [[nodiscard]]
        wxString ExpandColumnHeader(const size_t index) const
            {
            wxString expandedStr{ m_columnHeaders[index] };
            expandedStr.Replace(L"@COLUMNNAME@", m_columnsNames[index]);
            expandedStr.Replace(L"@COUNT@", wxNumberFormatter::ToString(
                                                m_columnTotals[index], 0,
                                                wxNumberFormatter::Style_WithThousandsSep |
                                                    wxNumberFormatter::Style_NoTrailingZeroes));
            return expandedStr;
            }

        std::vector<SankeyColumn> m_sankeyColumns;
        std::vector<SankeyAxisGroup> m_fromAxisGroups;
        std::vector<wxString> m_columnsNames;
        std::vector<double> m_columnTotals;

        FlowShape m_flowShape{ FlowShape::Curvy };
        BinLabelDisplay m_groupLabelDisplay{ BinLabelDisplay::BinName };
        GraphColumnHeader m_columnDisplay{ GraphColumnHeader::NoDisplay };
        std::vector<wxString> m_columnHeaders;

        // after setting the data, homogenizes the columns and their groups
        void AdjustColumns();
        /// @brief Recalculates the size of embedded objects on the plot.
        void RecalcSizes(wxDC& dc) final;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // SANKEY_DIAGRAM_H
