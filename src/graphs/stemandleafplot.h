/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_STEM_AND_LEAF_PLOT_H
#define WISTERIA_STEM_AND_LEAF_PLOT_H

#include "groupgraph2d.h"
#include <map>

namespace Wisteria::Graphs
    {
    /** @brief Stem-and-leaf plot, displayed as a colorful table-like layout.

         Values from a continuous column are silently floored to integers.
         An optional grouping column (with exactly 2 groups) produces a
         back-to-back display, where the left group's leaves are shown in
         reverse order (right-to-left toward the stem).

         | Single Series   | Back-to-Back |
         | :-------------- | :----------- |
         | @image html stem-leaf.svg width=90% | @image html stem-leaf-grouped.svg width=90% |

        @par %Data:
         This plot accepts a Data::Dataset where a continuous column provides
         the values. An optional categorical grouping column (with exactly
         2 groups) creates a back-to-back display.

        @par Missing Data:
         - Missing data in the group column will be shown as an empty label.
         - Missing data (NaN) in the value column will be ignored
           (pairwise deletion).

        @par Citation:
            Tukey, John W.
            *Exploratory Data Analysis.*
            Addison-Wesley, 1977.

        @par Example:
        @code
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         auto testData = std::make_shared<Data::Dataset>();
         testData->ImportCSV(L"/home/daphne/data/scores.csv",
            ImportInfo().ContinuousColumns({ L"Score" }).
            CategoricalColumns({
                { L"Gender",
                  CategoricalImportMethod::ReadAsStrings } }));

         auto plot = std::make_shared<StemAndLeafPlot>(canvas);
         plot->SetData(testData, L"Score", L"Gender");

         canvas->SetFixedObject(0, 0, plot);
         canvas->SetFixedObject(0, 1,
            plot->CreateLegend(
                LegendOptions{}.
                    IncludeHeader(true).
                    PlacementHint(
                        LegendCanvasPlacementHint::RightOfGraph)));
        @endcode*/
    class StemAndLeafPlot final : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(StemAndLeafPlot);
        StemAndLeafPlot() = default;

      public:
        /// @brief Recommended maximum number of observations.
        /// @details Beyond this, the plot becomes difficult to read.
        ///     A warning is logged if exceeded, but the plot is still rendered.
        constexpr static size_t MAX_RECOMMENDED_OBS = 100;

        /** @brief Constructor.
            @param canvas The parent canvas that the plot is being drawn on.*/
        explicit StemAndLeafPlot(Canvas* canvas);

        /// @private
        StemAndLeafPlot(const StemAndLeafPlot&) = delete;
        /// @private
        StemAndLeafPlot& operator=(const StemAndLeafPlot&) = delete;

        /** @brief Sets the data for the stem-and-leaf plot.
            @param data The dataset to use.
            @param continuousColumnName The continuous column whose values
                will be displayed. Values are floored to integers.
            @param groupColumnName An optional categorical grouping column.
                If provided, it must contain exactly 2 groups for
                back-to-back display.
            @throws std::runtime_error If columns are not found, or if the
                group column has more or fewer than 2 groups.\n
                The exception's @c what() message is UTF-8 encoded, so
                pass it to @c wxString::FromUTF8() when formatting it
                for an error message.
            @warning If the dataset contains more than @c MAX_RECOMMENDED_OBS observations,
                a warning will be logged but the plot will still be rendered.
                Consider using a histogram or boxplot for larger datasets.
            @note Call the parent canvas's `CalcAllSizes()` when setting
                to a new dataset to re-plot the data.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& continuousColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /** @brief Builds and returns a legend.
            @details Returns a text-only explanatory key legend
                (e.g., "Key: 2 | 0 means 20"). For back-to-back
                plots, also includes the group labels.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

        /// @name Header Color Functions
        /// @brief Functions for customizing the header colors.
        /// @{

        /// @brief Sets the background color for the "Stem" header.
        /// @param color The color to use.
        void SetStemHeaderColor(const wxColour& color) noexcept { m_stemHeaderColor = color; }

        /// @returns The background color for the "Stem" header.
        [[nodiscard]]
        const wxColour& GetStemHeaderColor() const noexcept
            {
            return m_stemHeaderColor;
            }

        /// @brief Sets the font color for the "Stem" header.
        /// @param color The color to use.
        /// @note Setting this to `std::nullopt` will select either black or white for the
        ///     font color (whichever contrasts with the header background).
        void SetStemHeaderFontColor(const std::optional<wxColour>& color) noexcept
            {
            m_stemHeaderFontColor = color;
            }

        /// @returns The font color for the "Stem" header.
        [[nodiscard]]
        const std::optional<wxColour>& GetStemHeaderFontColor() const noexcept
            {
            return m_stemHeaderFontColor;
            }

        /// @brief Sets the font color for the "Stem" values.
        /// @param color The color to use.
        /// @note Setting this to `std::nullopt` will select either black or white for the
        ///     font color (whichever contrasts with the background).
        void SetStemValueFontColor(const std::optional<wxColour>& color) noexcept
            {
            m_stemValueFontColor = color;
            }

        /// @returns The font color for the "Stem" values.
        [[nodiscard]]
        const std::optional<wxColour>& GetStemValueFontColor() const noexcept
            {
            return m_stemValueFontColor;
            }

        /// @brief Sets the background color for the "Leaf" header(s).
        /// @param color The color to use.
        void SetLeafHeaderColor(const wxColour& color) noexcept { m_leafHeaderColor = color; }

        /// @returns The background color for the "Leaf" header(s).
        [[nodiscard]]
        const wxColour& GetLeafHeaderColor() const noexcept
            {
            return m_leafHeaderColor;
            }

        /// @brief Sets the font color for the "Leaf" header.
        /// @param color The color to use.
        /// @note Setting this to `std::nullopt` will select either black or white for the
        ///     font color (whichever contrasts with the header background).
        void SetLeafHeaderFontColor(const std::optional<wxColour>& color) noexcept
            {
            m_leafHeaderFontColor = color;
            }

        /// @returns The font color for the "Leaf" header.
        [[nodiscard]]
        const std::optional<wxColour>& GetLeafHeaderFontColor() const noexcept
            {
            return m_leafHeaderFontColor;
            }

        /// @brief Sets the font color for the "Leaf" values.
        /// @param color The color to use.
        /// @note Setting this to `std::nullopt` will select either black or white for the
        ///     font color (whichever contrasts with the background).
        void SetLeafValueFontColor(const std::optional<wxColour>& color) noexcept
            {
            m_leafValueFontColor = color;
            }

        /// @returns The font color for the "Leaf" values.
        [[nodiscard]]
        const std::optional<wxColour>& GetLeafValueFontColor() const noexcept
            {
            return m_leafValueFontColor;
            }

        /// @}

      private:
        /// @brief Data for one row (stem) of the display.
        struct StemData
            {
            int64_t m_stem{ 0 };
            /// @brief Leaves for group 1 (back-to-back only).
            std::vector<int> m_leftLeaves;
            /// @brief Leaves for group 2 (or the only group).
            std::vector<int> m_rightLeaves;
            };

        void RecalcSizes(wxDC& dc) final;

        /// @brief Builds a space-separated leaf string.
        /// @param leaves The leaf digits (sorted ascending).
        /// @returns A space-separated string of leaf digits.
        [[nodiscard]]
        static wxString FormatLeaves(const std::vector<int>& leaves);

        /// @brief Builds a reversed space-separated leaf string.
        /// @param leaves The leaf digits (sorted ascending).
        /// @returns A space-separated string in reverse order.
        [[nodiscard]]
        static wxString FormatLeavesReversed(const std::vector<int>& leaves);

        std::vector<StemData> m_stems;
        wxString m_continuousColumnName;

        // group labels for back-to-back mode
        wxString m_leftGroupLabel;
        wxString m_rightGroupLabel;

        // IDs for the two groups (when grouping)
        Data::GroupIdType m_leftGroupId{ 0 };
        Data::GroupIdType m_rightGroupId{ 0 };

        size_t m_maxLeftLeafCount{ 0 };
        size_t m_maxRightLeafCount{ 0 };

        // header colors
        wxColour m_stemHeaderColor{ Colors::ColorBrewer::GetColor(Colors::Color::CafeAuLait) };
        wxColour m_leafHeaderColor{ Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#74B447") };

        std::optional<wxColour> m_stemHeaderFontColor{ *wxWHITE };
        std::optional<wxColour> m_leafHeaderFontColor{ *wxWHITE };
        std::optional<wxColour> m_stemValueFontColor{ Colors::ColorBrewer::GetColor(
            Colors::Color::CafeAuLait) };
        std::optional<wxColour> m_leafValueFontColor{ Colors::ColorBrewer::CSS_HEX_TO_LONG(
            L"#74B447") };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_STEM_AND_LEAF_PLOT_H
