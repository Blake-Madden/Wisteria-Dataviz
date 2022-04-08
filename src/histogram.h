/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_HISTOGRAM_H__
#define __WISTERIA_HISTOGRAM_H__

#include "barchart.h"
#include "colorbrewer.h"

namespace Wisteria::Graphs
    {
    /** @brief Graph for showing the counts of items, sorted into categories or intervals.

         Bins can either be plotted as a regular bar or split into (stacked) groups.

         | Regular   | Grouped
         | :-------------- | :--------------------------------
         | @image html Histogram.svg width=90% | @image html GroupedHistogram.svg width=90%

         @par Data:
          This plot accepts a Data::Dataset, where a continuous column (i.e., Y)
          is the dependent measurement. A grouping column can optionally be used to
          create separate blocks within the bins.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog, "canvas"
         // is a scrolled window derived object that will hold the box plot
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto quarterlyPerformanceData = std::make_shared<Data::Dataset>();
         quarterlyPerformanceData->ImportCSV(L"Performance Reviews.csv",
            ImportInfo().ContinuousColumns({ L"PERF" }).
            CategoricalColumns({ { L"QUARTER", CategoricalImportMethod::ReadAsStrings } }));

         auto plot = std::make_shared<Histogram>(canvas);

         plot->SetData(quarterlyPerformanceData, L"PERF",
             L"QUARTER",
             Histogram::BinningMethod::BinUniqueValues,
             RoundingMethod::NoRounding,
             Histogram::IntervalDisplay::Midpoints,
             BinLabelDisplay::BinValueAndPercentage);

         canvas->SetFixedObject(0, 0, plot);
         canvas->SetFixedObject(0, 1, plot->CreateLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        @endcode

        @todo Needs fit lines.*/
    class Histogram final : public Wisteria::Graphs::BarChart
        {
    public:
        /// @brief Methods for sorting data into bins.
        enum class BinningMethod
            {
            BinUniqueValues,      /*!< Each unique value gets its own bin.*/
            BinByRange,           /*!< Values are categorized into ranges (this is the norm for histograms, expect this method
                                       retains the values' floating-point precision when creating the bin size and range).*/
            BinByIntegerRange,    /*!< Values are categorized into ranges, where the bin size and range are integral.
                                       This is usually the norm, classifying data by floating-point precision categories isn't common.*/
            BINNING_METHOD_COUNT  /*!< The number of binning methods.*/
            };

        /// @brief How the bars are being positioned on the axis.
        enum class IntervalDisplay
            {
            Cutpoints,            /*!< In range mode, places the bars in between axis lines so that the range of the
                                       bins are shown on the sides of the bars.*/
            Midpoints,            /*!< Places the bars on top of the axis lines
                                       so that a custom bin range label (for integer range mode) or a midpoint label (non-integer mode)
                                       is shown at the bottom of the bar.*/
            INTERVAL_METHOD_COUNT /*!< The number of ways to interval display methods.*/
            };

        /* Keeps track of a block (group) that makes up a larger bin.
           These are pieced together to make the bars when bins are broken down into subgroups.*/
        struct BinBlock
            {
            double m_bin{ 0 };
            Data::GroupIdType m_block{ 0 };
            [[nodiscard]] bool operator<(const BinBlock& that) const noexcept
                {
                if (m_bin != that.m_bin)
                    { return m_bin < that.m_bin; }
                return m_block < that.m_block;
                }
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param colors The color scheme to apply to the points. Leave as null to use the default theme.*/
        explicit Histogram(Wisteria::Canvas* canvas, std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr) :
            Wisteria::Graphs::BarChart(canvas),
            m_data(nullptr),
            m_colorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme())
            {
            GetBarAxis().GetGridlinePen() = wxNullPen;
            GetBarAxis().GetGridlinePen() = wxNullPen; // doesn't make sense to show these on a histogram
            GetBarAxis().ShowOuterLabels(false);
            GetScalingAxis().GetGridlinePen() = wxNullPen;
            GetRightYAxis().Show(false);
            GetTopXAxis().Show(false);
            }
        /** @brief Sets the data.
            @param data The data to use for the histogram.
            @param continuousColumnName The column from the dataset to sort into bins.
            @param groupColumnName The group column to split the bins (i.e., bars) into (this is optional).
            @param bMethod The binning method.
             Note that column sorting will be disabled if binning method isn't unique values,
             moving the columns around into a different order would look wrong if they are supposed
             to be lined up in a range.
            @param rounding The rounding method to use for binning floating-point numbers.
            @param iDisplay The interval display to use.
            @param blDisplay Which type of labels to display for the bars.
             In range mode, set this to cutpoints to place the bars in between axis lines so that the range of the
             bins are shown on the sides of the bars. Set this to midpoints to place the bars on top of the axis lines
             so that a custom bin range label (for integer range mode) or a midpoint label (non-integer mode)
             is shown at the bottom of the bar.
            @param showFullRangeOfValues `true` if a place for each bin is included on the axis, even if they have no items.
             This specifies whether the axis should display each step (even if no bin is associated with a step) or
             if it should display steps that have categories on them. Setting this to `false` will put all of the bars
             together, but might have an uneven step size on the axis and fit lines won't be able to be drawn.
             This is only used if you are categorizing by unique (non-integer) values.
            @param startBinsValue The value to start the first bin (either the start of the first bin's range or the first bin's value).
             If no values fall into a bin starting at this position, then an empty slot for it will still be included on the bar axis.
             This will ensure that the bar axis begins from the position that you requested here. Set this to NaN for the chart to set the starting
             point based solely on the data.
            @param maxBarCount The maximum number of bins to create when binning the data.
             If binning by unique values and the number of unique values exceeds this, then the
             range-based mode will be used for the binning.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& continuousColumnName,
                     std::optional<const wxString> groupColumnName = std::nullopt,
                     const BinningMethod bMethod = BinningMethod::BinByIntegerRange,
                     const RoundingMethod rounding = RoundingMethod::NoRounding,
                     const IntervalDisplay iDisplay = IntervalDisplay::Cutpoints,
                     const BinLabelDisplay blDisplay = BinLabelDisplay::BinValue,
                     const bool showFullRangeOfValues = true,
                     const double startBinsValue = std::numeric_limits<double>::quiet_NaN(),
                     const size_t maxBarCount = 255);

        /** @brief Gets the number of bins in the graph.
            @note This refers to the number of categories with data in them, not the number of ticks on the bar axis or number of bars.
             If there are possible categories between some bins because of where their values fall,
             then any empty categories are not counted here.
             Also note that SetData() needs to be called first so that this can be calculated.
            @returns The number of bins in the graph.*/
        [[nodiscard]] size_t GetBinCount() const noexcept
            { return m_binCount; }

        /// @name %Bar Display Functions
        /// @brief Functions relating to the visual display of the bars.
        /// @{

        /// @brief Sets the bar opacity.
        /// @param opacity The level of opacity to use.
        void SetBarOpacity(const uint8_t opacity) noexcept
            { m_opacity = opacity; }
        /// @brief Sets the bar effect.
        /// @param effect The bar effect to apply.
        void SetBarEffect(const BoxEffect effect) noexcept
            { m_barEffect = effect; }
        /// @}

        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction. This is used
              for defining the legend's padding, outlining, canvas proportions, etc.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(const LegendCanvasPlacementHint hint) const;

        /// @brief Determines whether the columns (bins) can be sorted (in terms of bar length).
        /// @note Columns can only be sorted if we are showing unique values for the categories (i.e., not ranges)
        ///  and we are just showing bars that actually have values (so that the bars are next to each other).
        /// @returns Whether the columns (bins) can be sorted.
        /// @sa GetSortDirection(), SetSortDirection(), SetSortable(), SortBars().
        [[nodiscard]] bool IsSortable() const noexcept final
            { return BarChart::IsSortable() && GetBinningMethod() == BinningMethod::BinUniqueValues && !IsShowingFullRangeOfValues(); }
    private:
        /// @brief Get the color scheme used for the points.
        /// @returns The color scheme used for the points.
        [[nodiscard]] std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() noexcept
            { return m_colorScheme; }
        /// @private
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }
        /// @returns The maximum number of bins that the histogram will create when binning the data.
        [[nodiscard]] size_t GetMaxNumberOfBins() const noexcept
            { return m_maxBinCount; }
        /** Specifies whether the axis should display each step (even if no bin is associated with a step) or
            if it should display steps that have categories on them. Setting this to `false` will put all of the bars
            together, but might have an uneven step size on the axis and fit lines won't be able to be drawn.
            This is only used if you are categorizing by unique (non-integer) values.
            @param display `true` to display the full range of values.*/
        void ShowFullRangeOfValues(const bool display = true) noexcept
            { m_displayFullRangeOfValues = display; }
        /// @brief Specifies how to categorize and classify the data.
        /// @param bMethod The binning method to use.
        /// @note Column sorting will be disabled if binning method isn't unique values,
        ///  moving the columns around into a different order would look wrong if they are supposed
        ///  to be lined up in a range.
        void SetBinningMethod(const BinningMethod bMethod) noexcept
            { m_binningMethod = bMethod; }

        /// @brief Specifies how to classify floating-precision values.
        /// @param rounding The rounding method to use for binning.
        void SetRoundingMethod(const RoundingMethod rounding) noexcept
            { m_roundingMethod = rounding; }

        /** In range mode, set this to cutpoints to place the bars in between axis lines so that the range of the
            bins are shown on the sides of the bars. Set this to midpoints to place the bars on top of the axis lines
            so that a custom bin range label (for integer range mode) or a midpoint label (non-integer mode)
            is shown at the bottom of the bar.
            @param display The interval display to use.*/
        void SetIntervalDisplay(const IntervalDisplay display) noexcept
            { m_intervalDisplay = display; }

        /// @brief Sets which type of labels to display for the bars.
        /// @param display The bin label display to use.
        void SetHistrogramBinLabelDisplay(const BinLabelDisplay display) noexcept
            { m_binLabelDisplay = display; }
        /// @returns `true` if a place for each bin is included on the axis, even if they have no items.
        /// @sa ShowFullRangeOfValues().
        [[nodiscard]] bool IsShowingFullRangeOfValues() const noexcept
            { return m_displayFullRangeOfValues; }
        /// @returns The method being used to sort the data into bins.
        [[nodiscard]] BinningMethod GetBinningMethod() const noexcept
            { return m_binningMethod; }
        /// @returns The rounding method used for binning.
        [[nodiscard]] RoundingMethod GetRoundingMethod() const noexcept
            { return m_roundingMethod; }
        /// @returns How the bars are being positioned on the axis.
        /// @sa SetIntervalDisplay().
        [[nodiscard]] IntervalDisplay GetIntervalDisplay() const noexcept
            { return m_intervalDisplay; }
        /// @returns The type of labels being shown on the bars.
        [[nodiscard]] BinLabelDisplay GetHistrogramBinLabelDisplay() const noexcept
            { return m_binLabelDisplay; }
        /// @returns Where the first bin starts.
        /// @note This is NaN by default, which will instruct the bins to start at where the data begins.
        [[nodiscard]] double GetBinsStart() const noexcept
            { return m_startBinsValue; }
        /// @returns The number of unique values.
        [[nodiscard]] size_t GetUniqueValueCount() const;
         /** Creates a bin for each unique value in the data. If the number of categories exceeds the
             maximum number of categories, then it will implicitly switch to equal-ranges mode.*/
        void SortByUniqueValues();
        /** Bins the data into a specific number of categories. This is recommended if
            you have a lot of data and want to break data into categories.*/
        void SortByEqualRanges();
        /// Call this when sorting data (in case it needs to be rounded). If rounding is turned off then this simply returns the same value.
        [[nodiscard]] double ConvertToSortableValue(const double& value) const;

        [[nodiscard]] wxString GetCustomBarLabelOrValue(const double& value, const size_t precision = 0);

        /// @brief Calculates the number of bins to use based on the range of data.
        [[nodiscard]] size_t CalcNumberOfBins(const double minVal, const double maxVal) const;

        std::shared_ptr<const Data::Dataset> m_data{ nullptr };
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;

        BinningMethod m_binningMethod{ BinningMethod::BinByIntegerRange };
        RoundingMethod m_roundingMethod{ RoundingMethod::NoRounding };
        BinLabelDisplay m_binLabelDisplay{ BinLabelDisplay::BinValue };
        IntervalDisplay m_intervalDisplay{ IntervalDisplay::Cutpoints };
        size_t m_maxBinCount{ 255 };
        size_t m_binCount{ 0 };
        bool m_displayFullRangeOfValues{ true };
        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        uint8_t m_opacity{ wxALPHA_OPAQUE };
        BoxEffect m_barEffect{ BoxEffect::Glassy };
        double m_startBinsValue{ std::numeric_limits<double>::quiet_NaN() };
        bool m_useGroupingColors{ false };
        bool m_useGrouping{ false };
        std::set<Data::GroupIdType> m_groupIds;

        static constexpr size_t MAX_OBS_IN_LABEL{ 25 };
        };
    }

/** @}*/

#endif //__WISTERIA_HISTOGRAM_H__
