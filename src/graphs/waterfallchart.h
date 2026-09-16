/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_WATERFALLCHART_H
#define WISTERIA_WATERFALLCHART_H

#include "barchart.h"
#include <optional>
#include <vector>

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A waterfall chart, showing how sequential changes accumulate
            from a starting value to an ending value.
        @details Each bar floats: it starts where the previous bar ended.
            Positive changes rise (green by default), negative changes fall
            (red by default). Rows flagged as totals draw from zero to the
            current cumulative value, marking a checkpoint.\n
            Totals only appear when explicitly flagged through the optional
            total-flag column (@c 0 means a change, @c 1 means a total).
            Without that column, every row is treated as a change.

        @par %Data:
         This plot accepts a Data::Dataset where one column holds the step labels
         (a categorical or ID column) and one continuous column holds the amounts.
         An optional third column flags totals (@c 0 = change, @c 1 = total);
         it may be a categorical or continuous column.

         | Step     | Amount | IsTotal |
         | :--      | --:    | --:     |
         | Start    | 100    | 0       |
         | Sales    | 50     | 0       |
         | Costs    | -30    | 0       |
         | Q1 Total |        | 1       |

         @note The value in a total row is not used. The total is the running
            sum of the changes so far. This should be @c NaN; any other value is ignored.

        @par Missing Data:
         - A row with a missing amount is skipped entirely, unless the row is flagged
            as a total, in which case it shows the running sum so far.

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         auto flowData = std::make_shared<Data::Dataset>();
         flowData->AddCategoricalColumn(L"Step");
         flowData->AddContinuousColumn(L"Amount");
         flowData->AddCategoricalColumn(L"IsTotal");
         // ... fill rows in display order ...

         auto plot = std::make_shared<WaterfallChart>(canvas);
         plot->SetData(flowData, L"Step", L"Amount", L"IsTotal");

         canvas->SetFixedObject(0, 0, plot);
        @endcode*/
    // clang-format on

    class WaterfallChart final : public Wisteria::Graphs::BarChart
        {
        wxDECLARE_DYNAMIC_CLASS(WaterfallChart);
        WaterfallChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.*/
        explicit WaterfallChart(Wisteria::Canvas* canvas);

        /** @brief Sets the data.
            @param data The data to use.
            @param labelColumnName The column containing the step labels
                (a categorical or ID column). Rows are plotted in dataset order.
            @param valueColumnName The column containing the change amounts
                (a continuous column). Ignored for rows flagged as totals.
            @param totalFlagColumnName An optional column flagging totals
                (@c 0 = change, @c 1 = total). May be a categorical column
                (labels @c "0"/@c "1", or @c "total" also counts as a total)
                or a continuous column (non-zero counts as a total).\n
                If omitted (or empty), every row is treated as a change.
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& labelColumnName, const wxString& valueColumnName,
                     const std::optional<wxString>& totalFlagColumnName = std::nullopt);

        /// @returns The brush used for increase (positive change) bars.
        [[nodiscard]]
        wxBrush& GetIncreaseBrush() noexcept
            {
            return m_increaseBrush;
            }

        /// @returns The color of the increase (positive change) bars.
        [[nodiscard]]
        wxColour GetIncreaseColor() const
            {
            return m_increaseBrush.GetColour();
            }

        /// @returns The pen used to outline the increase bars.
        [[nodiscard]]
        wxPen& GetIncreasePen() noexcept
            {
            return m_increasePen;
            }

        /** @brief Sets the color of the increase (positive change) bars.
            @param color The color to fill the bars with.\n
                The outline pen is derived from this (a shaded variant),
                so the bar reads as one cohesive color.
            @sa GetIncreaseBrush(), GetIncreasePen() for finer-grained control.*/
        void SetIncreaseColor(const wxColour& color);

        /// @returns The brush used for decrease (negative change) bars.
        [[nodiscard]]
        wxBrush& GetDecreaseBrush() noexcept
            {
            return m_decreaseBrush;
            }

        /// @returns The color of the decrease (negative change) bars.
        [[nodiscard]]
        wxColour GetDecreaseColor() const
            {
            return m_decreaseBrush.GetColour();
            }

        /// @returns The pen used to outline the decrease bars.
        [[nodiscard]]
        wxPen& GetDecreasePen() noexcept
            {
            return m_decreasePen;
            }

        /** @brief Sets the color of the decrease (negative change) bars.
            @param color The color to fill the bars with.\n
                The outline pen is derived from this (a shaded variant).
            @sa GetDecreaseBrush(), GetDecreasePen() for finer-grained control.*/
        void SetDecreaseColor(const wxColour& color);

        /// @returns The brush used for total bars.
        [[nodiscard]]
        wxBrush& GetTotalBrush() noexcept
            {
            return m_totalBrush;
            }

        /// @returns The color of the total bars.
        [[nodiscard]]
        wxColour GetTotalColor() const
            {
            return m_totalBrush.GetColour();
            }

        /// @returns The pen used to outline the total bars.
        [[nodiscard]]
        wxPen& GetTotalPen() noexcept
            {
            return m_totalPen;
            }

        /** @brief Sets the color of the total bars.
            @param color The color to fill the bars with.\n
                The outline pen is derived from this (a shaded variant).
            @sa GetTotalBrush(), GetTotalPen() for finer-grained control.*/
        void SetTotalColor(const wxColour& color);

        /// @returns @c true if the (signed) values are shown on the bars.
        [[nodiscard]]
        bool IsShowingBarValues() const noexcept
            {
            return m_showValues;
            }

        /** @brief Shows (or hides) the (signed) values on the bars.
            @param show @c true to show the values. For change bars this is the
                change amount; for total bars this is the cumulative total.
                Defaults to @c true.*/
        void ShowBarValues(const bool show);

        /// @returns @c true if the (signed) values are drawn on the bar blocks
        ///     (as decals across the bars).
        [[nodiscard]]
        bool IsShowingBlockValues() const noexcept
            {
            return m_showBlockValues;
            }

        /** @brief Shows (or hides) the (signed) values on the bar blocks
                (as decals across the bars).
            @param show @c true to draw the values across the bars. For change bars,
                this is the change amount. For total bars, this is the cumulative total.
                Defaults to @c false.
            @note This is independent of ShowBarValues(): both, either, or neither
                may be shown.*/
        void ShowBlockValues(const bool show);

        /// @returns How the values on the bars and blocks are displayed.
        [[nodiscard]]
        NumberDisplay GetValueDisplay() const noexcept
            {
            return m_valueDisplay;
            }

        /** @brief Sets how the values on the bars and blocks are displayed.
            @param display How to display the values (@c Value by default).\n
                @c Currency uses the current locale's currency formatting,
                @c Percentage appends a "%" suffix (purely cosmetic;
                the underlying value is not transformed).
            @note This applies to both ShowBarValues() and ShowBlockValues().*/
        void SetValueDisplay(NumberDisplay display);

        /// @returns The name of the label column.
        [[nodiscard]]
        const wxString& GetLabelColumnName() const noexcept
            {
            return m_labelColumnName;
            }

        /// @returns The name of the value column.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns The name of the total-flag column, or @c std::nullopt if unused.
        [[nodiscard]]
        const std::optional<wxString>& GetTotalFlagColumnName() const noexcept
            {
            return m_totalFlagColumnName;
            }

        /// @private
        [[deprecated("Waterfall charts do not support legends.")]] [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateLegend([[maybe_unused]] const LegendOptions& options) final
            {
            wxFAIL_MSG(L"Waterfall charts do not support legends.");
            return nullptr;
            }

      private:
        /// @brief One waterfall step, resolved from the dataset.
        struct RowInfo
            {
            wxString m_label;
            double m_axisPosition{ 0 };
            double m_value{ std::numeric_limits<double>::quiet_NaN() };
            bool m_isTotal{ false };
            };

        /// @brief Applies the axis styling used by this chart
        ///     (bar axis showing only custom step labels, etc.).
        void ApplyAxisAppearance();

        /// @brief Rebuilds every bar from the current @c m_rows,
        ///     computing the running total and floating offsets.
        void RebuildBars();

        /// @returns An outline pen that visibly separates from @p color.
        /// @param color The bar fill color to derive the outline from.
        [[nodiscard]]
        static wxPen DeriveOutlinePen(const wxColour& color);

        /// @returns Whether a total-flag column's label text denotes a total
        ///     (as opposed to a change).
        /// @param flag The label text to check (e.g., a string table entry's label).
        [[nodiscard]]
        static bool IsTotalFlagLabel(wxString flag);

        void SetAutoAccessibilityAttributes() final;

        std::vector<RowInfo> m_rows;

        wxString m_labelColumnName;
        wxString m_valueColumnName;
        std::optional<wxString> m_totalFlagColumnName;

        wxBrush m_increaseBrush;
        wxPen m_increasePen;
        wxBrush m_decreaseBrush;
        wxPen m_decreasePen;
        wxBrush m_totalBrush;
        wxPen m_totalPen;

        bool m_showValues{ true };
        bool m_showBlockValues{ false };
        NumberDisplay m_valueDisplay{ NumberDisplay::Value };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_WATERFALLCHART_H
