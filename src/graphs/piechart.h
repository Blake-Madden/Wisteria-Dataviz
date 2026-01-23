/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PIE_CHART_H
#define WISTERIA_PIE_CHART_H

#include "graph2d.h"
#include <map>

namespace Wisteria::GraphItems
    {
    /// @brief A slice of a pie/donut chart.
    class PieSlice final : public GraphItemBase
        {
      public:
        /** @brief Constructor.
            @param info Base settings for the slice. (The pen and brush are used.)
            @param pieRect The area of the parent pie.
            @param startAngle The starting angle of the slice in relation to its parent pie.
                This assumes a counter-clockwise pie, starting at 3 o'clock.
            @param endAngle The ending angle of the slice.
            @param value The value of the slice (e.g., number of observations).
            @param percent The percent of the pie that this slice consumes.*/
        PieSlice(const GraphItemInfo& info, const wxRect& pieRect, const double startAngle,
                 const double endAngle, const double value, const double percent)
            : m_pieArea(pieRect), m_startAngle(startAngle), m_endAngle(endAngle), m_value(value),
              m_percent(percent)
            {
            GraphItemBase::GetGraphItemInfo() = info;
            }

        /// @returns The pen used for the outer arc of the slice, if different
        ///     from the pen used for the sides.
        [[nodiscard]]
        std::optional<wxPen>& GetArcPen() noexcept
            {
            return m_arcPen;
            }

        /// @private
        [[nodiscard]]
        const std::optional<wxPen>& GetArcPen() const noexcept
            {
            return m_arcPen;
            }

        /** @brief Creates a label to display in the middle of the slice.\n
                This is usually a raw count of observations in the slice, or its percentage of the
                overall pie.
            @param dc The device context to measure the Label with.
            @param pieProportion The proportion of the pie area that this slice's ring consumes.
            @param labelDisplay What to display in the label.
            @param abbreviate Text replacer object that can attempt to abbreviate the label
                to make it fit. Note that this will only be used if @c labelDisplay is
                set to BinLabelDisplay::BinName.
            @returns The label, which will already be anchored to the middle of the slice.*/
        [[nodiscard]]
        std::unique_ptr<Wisteria::GraphItems::Label>
        CreateMiddleLabel(wxDC& dc, double pieProportion, BinLabelDisplay labelDisplay,
                          const std::shared_ptr<const TextReplace>& abbreviate = nullptr);
        /** @brief Creates a label to display at the outer ring of the pie.
                This is usually the group label of the slice.
            @param labelDisplay What to display on the label.
            @returns The label, which will already be anchored to the middle of the slices'
                outer ring.*/
        [[nodiscard]]
        std::unique_ptr<Wisteria::GraphItems::Label> CreateOuterLabel(BinLabelDisplay labelDisplay);
        /** @brief Creates a label to display at the outer ring of the pie.
                This is usually the group label of the slice.
            @param pieArea A different pie area from what the slice is currently using.
                This is useful for inner ring slices that need its outer labels to be outside
                the parent ring.
            @param labelDisplay What to display on the label.
            @returns The label, which will already be anchored to the middle of the slices'
                outer ring.*/
        [[nodiscard]]
        std::unique_ptr<Wisteria::GraphItems::Label> CreateOuterLabel(const wxRect& pieArea,
                                                                      BinLabelDisplay labelDisplay);

        /// @returns The custom midpoint display, specific to this slice.
        [[nodiscard]]
        std::optional<BinLabelDisplay> GetMidPointLabelDisplay() const noexcept
            {
            return m_midPointLabelDisplay;
            }

        /// @brief Sets the custom midpoint display, specific to this slice.
        /// @param blDisplay The display to use.
        /// @note This is usually @c std::nullopt and the parent pie's display is applied.
        void SetMidPointLabelDisplay(const std::optional<BinLabelDisplay>& blDisplay) noexcept
            {
            m_midPointLabelDisplay = blDisplay;
            }

        /// @returns The middle point of the outer rim of the slice's arc.
        /// @param pieProportion The proportion of the pie that this arc should consume.
        ///     For example, @c 0.5 would be an arc in the middle of the pie, and @c 1.0 would be
        ///     the outer arc of the pie.
        [[nodiscard]]
        std::pair<double, double> GetMiddleOfArc(double pieProportion) const noexcept;
        /// @returns The middle point of the outer rim of the slice's arc.
        /// @param pieProportion The proportion of the pie that this arc should consume.
        ///     For example, @c 0.5 would be an arc in the middle of the pie, and @c 1 would be the
        ///     outer arc of the pie.
        /// @param pieArea A custom area to align the point with.
        ///     This is useful for moving an outer label further out from the slice.
        [[nodiscard]]
        std::pair<double, double> GetMiddleOfArc(double pieProportion,
                                                 wxRect pieArea) const noexcept;
        /// @returns The (approximate) polygon of the slice.
        std::vector<wxPoint> GetPolygon() const noexcept;

        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]]
        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            {
            return Polygon::GetPolygonBoundingBox(GetPolygon());
            }

      private:
        wxRect Draw(wxDC& dc) const final;

        [[nodiscard]]
        bool HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const final
            {
            auto points = GetPolygon();
            return geometry::is_inside_polygon(pt, points);
            }

        void Offset(const int x, const int y) final { m_pieArea.Offset(x, y); }

        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double scaling) final
            {
            m_pieArea = rect;
            }

        wxRect m_pieArea;
        double m_startAngle{ 0 };
        double m_endAngle{ 0 };
        double m_value{ 0 };
        double m_percent{ math_constants::empty };

        std::optional<wxPen> m_arcPen;
        std::optional<BinLabelDisplay> m_midPointLabelDisplay;
        };
    } // namespace Wisteria::GraphItems

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A pie chart, which shows group sizes along a radial axis.
        @details An additional grouping variable can be used, which represent
            subgroups within the main grouping variable's slices. The main grouping
            variable is displayed along an outer ring, and the subgrouping variable is
            shown as an inner ring. The subgroups along the inner ring will align with the
            parent groups (i.e., slices) along the outer ring.

            An optional donut hole (with or without a label) can also be included in
            middle of the pie.

            | Pie Chart       | Subgrouped Pie Chart              |
            | :-------------- | :-------------------------------- |
            | @image html PieChart.svg width=90% | @image html PieChartSubgrouped.svg width=90% |

            | Donut Chart     | Subgrouped Donut Chart            |
            | :-------------- | :-------------------------------- |
            | @image html DonutChart.svg width=90% | @image html DonutChartSubgrouped.svg width=90% |

        @note The pie slices will be ordered alphabetically and drawn counter-clockwise, starting
            at 3 o'clock.\n
            This order will also be used for the brush, color, or image scheme that
            is being used to paint each slice. In other words, the first image from your image scheme
            will be used for the slice with the first alphabetically sorted label.

        @par %Data:
            This plot accepts a Data::Dataset where a continuous column is the aggregate counts,
            and a grouping column is the groups that the values are aggregated into.

            The aggregate column is optional and if not provided then the frequency counts of
            the groups are used.

            Optionally, a second grouping column can be used to create subgroups within the main
            slices of the pie. These subgroups are shown along an inner ring, lined up against
            their parent group slices in the outer ring.

            Below is an example where the continuous is `Enrollment`, the main grouping column
            is `College`, and the optional second grouping column is `Course`.

            | Course           | College                   | Enrollment |
            | :--              | :--                       | --:        |
            | Science          | Anatomy & Physiology I    | 52         |
            | Business         | Business Ethics           | 51         |
            | English          | Creative Writing          | 47         |
            | Computer Science | COBOL                     | 45         |
            | Business         | Introduction to Economics | 34         |
            | Business         | Introduction to Economics | 32         |
            | Computer Science | C++                       | 32         |
            | Science          | Organic Chemistry I       | 32         |
            | Business         | Business Communication    | 31         |
            | Science          | Organic Chemistry I       | 28         |
            | Business         | Business Communication    | 27         |
            | Computer Science | C++                       | 27         |
            ...

            With the above data, and outer ring for the colleges will be drawn, where the values from
            `Enrollment` will be aggregated into it. Also, an inner ring for the courses will be drawn,
            showing the aggregated enrollment numbers for each course. The courses' slices will be
            aligned next to parent College slice that they belong to.

            If `Enrollment` is not used as the aggregate column, then the frequency counts for the labels
            in `Course` and/or `College` will be used for their pie slice values.

            Note that continuous columns can also be specified for the grouping columns, but their data
            must be discrete and positive. Any negative or floating-point values will be truncated to
            unsigned integers.

        @par Missing Data:
        - Missing data in the group column(s) will be shown as an empty pie & legend label.
        - Missing data in the value column will be ignored (listwise deletion).

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 1);

         // import the dataset (this is available in the "datasets" folder)
         auto pieData = std::make_shared<Data::Dataset>();
         try
            {
            pieData->ImportCSV(L"/home/rdoyle/data/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                  { L"Course", CategoricalImportMethod::ReadAsStrings },
                  { L"COLLEGE", CategoricalImportMethod::ReadAsStrings }
                  }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
         auto plot = std::make_shared<PieChart>(canvas);
         plot->SetData(pieData, L"Enrollment", L"COLLEGE");

         canvas->SetFixedObject(0, 0, plot);
        @endcode
        @par Subgrouping Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto pieData = std::make_shared<Data::Dataset>();
         try
            {
            pieData->ImportCSV(L"/home/rdoyle/data/Fall Enrollment.csv",
                ImportInfo().
                ContinuousColumns({ L"Enrollment" }).
                CategoricalColumns({
                    { L"COLLEGE", CategoricalImportMethod::ReadAsStrings },
                    { L"Course", CategoricalImportMethod::ReadAsStrings }
                    }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(err.what(), _(L"Import Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return;
            }
         auto plot = std::make_shared<PieChart>(canvas);
         plot->SetData(pieData, L"Enrollment", L"COLLEGE", L"Course");

         // find a group from the outer ring and add a description to it
         auto foundSlice = std::find(plot->GetOuterPie().begin(),
                                     plot->GetOuterPie().end(),
                                     PieChart::SliceInfo{ L"English" });
         if (foundSlice != plot->GetOuterPie().end())
            {
            foundSlice->SetDescription(
                _(L"Includes both literary and composition courses"));
            }
         // show one of the outer labels for the inner ring
         // and add a custom description to it
         std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
            [](auto& slice)
                {
                if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") == 0)
                    {
                    slice.ShowGroupLabel(true);
                    slice.SetDescription(_(L"Drop this from the catalog?"));
                    }
                }
            );

         // apply the slice's colors to its respective outside label
         plot->UseColorLabels(true);
         // add a donut hole
         plot->IncludeDonutHole(true);
         plot->GetDonutHoleLabel().SetText(L"Enrollment\nFall 2023");
         plot->SetDonutHoleProportion(.5);

         canvas->SetFixedObject(0, 0, plot);
         // add a legend for the inner ring (i.e., the subgroup column,
         // which will also show headers for their parent groups)
         canvas->SetFixedObject(0, 1,
                 plot->CreateLegend(
                    LegendOptions().
                        RingPerimeter(Perimeter::Inner).
                        PlacementHint(LegendCanvasPlacementHint::RightOfGraph)) );
        @endcode*/
    // clang-format off
    class PieChart final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(PieChart);
        PieChart() = default;

      public:
        /// @brief Information about a pie slice.
        class SliceInfo
            {
            friend class PieChart;

          public:
            /// @brief Constructor.
            /// @param groupLabel The main label for a slice
            ///     (usually the group label from the dataset).
            /// @param value The value (e.g., number of items in the group) for the slice.
            /// @param percent The percent of the pie that this slice consumes.
            /// @param parentSliceIndex The position along the outer pie of this slice's
            ///     parent slice (only applies to the inner pie).
            explicit SliceInfo(wxString groupLabel, const double value = 0,
                               const double percent = 0, size_t parentSliceIndex = 0)
                : m_groupLabel(std::move(groupLabel)), m_value(value), m_percent(percent),
                  m_parentSliceIndex(parentSliceIndex)
                {
                }

            /// @returns The text shown on the outer ring of the slice.
            [[nodiscard]]
            const wxString& GetGroupLabel() const noexcept
                {
                return m_groupLabel;
                }

            /// @brief The main label for a slice (usually the group label from a dataset).
            /// @param label The label.
            void SetGroupLabel(const wxString& label) { m_groupLabel = label; }

            /// @returns The description shown on the outer ring of the slice
            ///     (beneath the label).
            /// @note This value is not gathered from the dataset, but rather added by
            ///     the client code.
            [[nodiscard]]
            const wxString& GetDescription() const noexcept
                {
                return m_description;
                }

            /// @brief An optional description to show below the main label.
            /// @param desc The description.
            void SetDescription(const wxString& desc) { m_description = desc; }

            /// @returns @c true if the slice is being made translucent.
            [[nodiscard]]
            bool IsGhosted() const noexcept
                {
                return m_ghost;
                }

            /// @brief Sets the slice to be translucent.
            /// @param ghost @c true to make the slice translucent.
            void Ghost(const bool ghost) noexcept { m_ghost = ghost; }

            /// @brief Whether to display the slice's label outside the outer ring of the pie.
            /// @param show @c true to show the label.
            /// @note Setting this to @c false will also hide the description.
            ///     The label and description will still be shown if the slice is selected.
            void ShowGroupLabel(const bool show) noexcept { m_showText = show; }

            /// @returns The custom midpoint display, specific to this slice.
            [[nodiscard]]
            std::optional<BinLabelDisplay> GetMidPointLabelDisplay() const noexcept
                {
                return m_midPointLabelDisplay;
                }

            /// @brief Sets the custom midpoint display, specific to this slice.
            /// @param blDisplay The display to use.
            /// @note This is usually @c std::nullopt and the parent pie's display is applied.
            void SetMidPointLabelDisplay(const std::optional<BinLabelDisplay>& blDisplay) noexcept
                {
                m_midPointLabelDisplay = blDisplay;
                }

            /// @private
            [[nodiscard]]
            bool
            operator==(const SliceInfo& that) const
                {
                return Data::CmpNoCaseIgnoreControlChars(m_groupLabel, that.m_groupLabel) == 0;
                }

            /// @private
            [[nodiscard]]
            bool
            operator<(const SliceInfo& that) const
                {
                return Data::CmpNoCaseIgnoreControlChars(m_groupLabel, that.m_groupLabel) < 0;
                }

          private:
            wxString m_groupLabel;
            wxString m_description;
            bool m_ghost{ false };
            bool m_showText{ true };
            double m_value{ 0.0 };
            double m_percent{ math_constants::empty };
            size_t m_parentSliceIndex{ 0 };
            std::optional<BinLabelDisplay> m_midPointLabelDisplay;
            };

        /// @brief A vector of informational blocks about a pie ring's slices.
        using PieInfo = std::vector<SliceInfo>;

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param brushes The brush scheme, which will contain the color and brush patterns
                to render the slices with.
            @param colors The color scheme to apply to the slices underneath the slices'
                brush patterns.\n
                This is useful if using a hatched brush, as this color will be solid
                and show underneath it. Leave as @c nullptr just to use the brush scheme.*/
        explicit PieChart(Canvas* canvas,
                          const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
                          const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the chart.
            @param data The data to use, which can (optionally) use a continuous column
                containing aggregate counts, the main grouping column, and (optionally)
                a second grouping column to create an inner (subgrouped) ring.
            @param weightColumnName The (optional) weight column.\n
                These are the values accumulated into the respective labels from
                the group column(s). If this column is not provided, then frequency counts
                of the labels from the group column(s) are used.\n
                This is useful for when you have pre-computed values for each group and the
                data just consists of unique labels and their respective totals.
            @param groupColumn1Name The main grouping ring.\n
                This can either be a categorical column, or a continuous column. Note, however,
                that if a continuous column, then all values will be truncated to
                unsigned 64-bit integers. In other words, if only discrete, positive values
                should be used if providing a continuous column.
            @param groupColumn2Name The (optional) second grouping ring.\n
                This inner ring will be shown as subgroups within each slice
                along the (parent) outer ring.\n
                This can either be a categorical column, or a continuous column (with the same
                specifications as @c groupColumn1Name).
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     std::optional<const wxString> weightColumnName,
                     const wxString& groupColumn1Name,
                     std::optional<const wxString> groupColumn2Name = std::nullopt);

        /// @returns @c true if outer slice labels have their font colors match
        ///     their respective pie slice.
        [[nodiscard]]
        bool IsUsingColorLabels() const noexcept
            {
            return m_useColorLabels;
            }

        /// @brief Whether outer slice labels have their font colors match
        ///     their respective pie slice.
        /// @param useColors @c true to use colors for the labels.
        void UseColorLabels(const bool useColors) noexcept { m_useColorLabels = useColors; }

        /// @returns The effect used for drawing the slices.
        [[nodiscard]]
        PieSliceEffect GetPieSliceEffect() const noexcept
            {
            return m_sliceEffect;
            }

        /** @brief Sets the effect for how slices are rendered.
            @param effect The effect to use.
            @note If using PieSliceEffect::Image, you will need to provide an image scheme.\n
                Also, if showcasing inner-ring slices in conjunction with the
                PieSliceEffect::Image style, the "ghosted" slices will appear as translucent images,
                but the showcased inner slices will appear as solid colors. This is because trying
                to have part of an image translucent and other parts of it opaque will not be obvious.\n
                If showcasing outer-ring slices, then showcased slices will be the images at full opacity.\n
                \n
                Images will be stippled (repeated) for slices with angles larger than 90 degrees.
                (The image will be repeated every 90 degrees.)\n
                \n
                Finally, images and solid colors can be mixed and matched by using an image scheme
                with invalid images (i.e., @c wxNullBitmap). When a null bitmap is encountered in
                an image scheme, then the respective slice will fall back to using the brush and
                color schemes.
            @sa SetImageScheme().*/
        void SetPieSliceEffect(const PieSliceEffect effect) noexcept { m_sliceEffect = effect; }
        
        /// @returns The visual style to apply to the pie chart.
        [[nodiscard]]
        PieStyle GetPieStyle() const noexcept
            {
            return m_pieStyle;
            }

        /** @brief Sets the visual style to apply to the pie chart.
            @param style The style to use.*/
        void SetPieStyle(const PieStyle style) noexcept { m_pieStyle = style; }

        /// @returns The opacity level applied to "ghosted" slices.
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /** @brief Sets the opacity level for "ghosted" slices.
            @param opacity The opacity level (should be between @c 0 and @c 255).
            @note If setting this to @c 0 (fully transparent), then you should set
                the pie's pen to a darker color.
            @sa GhostOuterPieSlices(), GhostInnerPieSlices().*/
        void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

        /** @brief Sets the text replacement object to abbreviate midpoint labels
                to make them fit (if necessary).
            @details The default is to use the (aggressive) English abbreviation interface.
            @param abbreviate The text replacement object to abbreviate with.\n
                Set to null to never attempt abbreviating midpoint labels.
            @note This is only meant for midpoint labels, as this may be too aggressive
                for outer labels. If outer labels are too long to fit, set label placement
                to LabelPlacement::Flush.
            @sa SetLabelPlacement().*/
        void SetMidPointAbbreviation(const std::shared_ptr<const TextReplace>& abbreviate)
            {
            m_abbreviate = abbreviate;
            }

        /// @name Margin Functions
        /// @brief Functions for customizing the margins/gutters around the pie.
        /// @details This includes the pie's outer labels, margin note, and whether
        ///     both margins are always shown.
        /// @{

        /// @returns @c true if the pie chart is shifted to one side if either gutter
        ///     has no outer labels.
        [[nodiscard]]
        bool HasDynamicMargins() const noexcept
            {
            return m_dynamicMargins;
            }

        /// @brief Set to @c true to shift the pie chart to one side if either gutter
        ///     has no outer labels and there aren't any margin notes.
        /// @details This is useful when placing multiple showcased pie charts side-by-side,
        ///     where the real estate is at a premium.\n
        ///     This should be set to @c false, however, if you are aligning pie charts
        ///     vertically and you need the pies to align with each other.
        /// @param useDynamic @c true to enable dynamically hiding margins.
        /// @sa GetLeftMarginNote(), GetRightMarginNote().
        void SetDynamicMargins(const bool useDynamic) noexcept { m_dynamicMargins = useDynamic; }

        /// @brief Gets/sets the label shown in the left margin/gutter.
        /// @details This will override dynamically hiding the margins, forcing the left margin
        ///     to always be included.
        /// @note If outer labels are being shown, then this note may overlay it. This is only
        ///     recommended if outer labels are hidden or if the caller is certain that no
        ///     labels will be on the left side of the pie.
        /// @returns A reference to the label.
        /// @sa SetDynamicMargins(), GetRightMarginNote().
        [[nodiscard]]
        GraphItems::Label& GetLeftMarginNote() noexcept
            {
            return m_leftMarginNote;
            }

        /// @brief Gets/sets the label shown in the right margin/gutter.
        /// @details This will override dynamically hiding the margins, forcing the right margin
        ///     to always be included.
        /// @note If outer labels are being shown, then this note may overlay it. This is only
        ///     recommended if outer labels are hidden or if the caller is certain that no
        ///     labels will be on the right side of the pie.
        /// @returns A reference to the label.
        /// @sa SetDynamicMargins(), GetLeftMarginNote().
        [[nodiscard]]
        GraphItems::Label& GetRightMarginNote() noexcept
            {
            return m_rightMarginNote;
            }

        /// @returns What the labels along the margins (i.e., gutters) are displaying.
        [[nodiscard]]
        BinLabelDisplay GetOuterLabelDisplay() const noexcept
            {
            return m_outerLabelDisplay;
            }

        /// @brief Sets what the labels along the margins (i.e., gutters) are displaying.
        /// @param display What to display.
        void SetOuterLabelDisplay(const BinLabelDisplay display) noexcept
            {
            m_outerLabelDisplay = display;
            }

        /// @}

        /// @name Outer Pie Functions
        /// @brief Functions for customizing the outer ring of the pie chart.\n
        ///     If subgrouping is not being used, then this will be the only pie ring.
        /// @note Functions which accept lists of slices by label name (such as
        ///     GhostOuterPieSlices()) can be used functions such as GetLargestOuterPieSlices().
        /// @{

        /// @brief Accesses the outer pie (or the only pie if a single series chart).\n
        ///     Use this to edit the labels and descriptions for slices.
        /// @returns The outer (i.e., main) pie.
        [[nodiscard]]
        PieInfo& GetOuterPie() noexcept
            {
            return m_outerPie;
            }

        /// @returns What the labels on the middle points along the outer ring are displaying.
        /// @note If only using a single grouping column, then this refers to the main pie.
        [[nodiscard]]
        BinLabelDisplay GetOuterPieMidPointLabelDisplay() const noexcept
            {
            return m_outerPieMidPointLabelDisplay;
            }

        /// @brief Sets what the labels on the middle points along the outer ring are displaying.
        /// @param display What to display.
        void SetOuterPieMidPointLabelDisplay(const BinLabelDisplay display) noexcept
            {
            m_outerPieMidPointLabelDisplay = display;
            }

        /** @brief Specifies where the outer labels are placed, relative to the pie area.
            @details Labels can either be next to their respective pie slices, or pushed
                over to the edge of the pie's area. (The latter will left or right align
                the labels relative to each other.)
            @param placement How to place the labels.
            @sa SetInnerPieConnectionLineStyle().*/
        void SetLabelPlacement(const LabelPlacement placement) noexcept
            {
            m_labelPlacement = placement;
            }

        /// @returns Where the outer labels are placed.
        [[nodiscard]]
        LabelPlacement GetLabelPlacement() const noexcept
            {
            return m_labelPlacement;
            }

        /// @brief Brings to focus the specified slice(s) along the outer pie and their
        ///     children inner slices.
        /// @param pieSlices The outer slices to showcase.
        /// @param outerLabelRingToShow Which ring should have its outer labels shown.
        /// @details This will make all other slices (including their inner slices)
        ///     translucent and hide their labels.
        void ShowcaseOuterPieSlices(const std::vector<wxString>& pieSlices,
                                     Perimeter outerLabelRingToShow = Perimeter::Outer);
        /// @brief Brings to focus the largest slice(s) along the outer (or only) pie.
        /// @param outerLabelRingToShow Which ring should have its outer labels shown.
        /// @details This will make all other slices (including their inner slices)
        ///     translucent and hide their labels.
        void ShowcaseLargestOuterPieSlices(Perimeter outerLabelRingToShow = Perimeter::Outer);
        /// @brief Brings to focus the smallest slice(s) along the outer (or only) pie.
        /// @param outerLabelRingToShow Which ring should have its outer labels shown.
        /// @details This will make all other slices (including their inner slices)
        ///     translucent and hide their labels.
        void
        ShowcaseSmallestOuterPieSlices(Perimeter outerLabelRingToShow = Perimeter::Outer);

        /** @brief Gets the labels of the largest slice(s) along the outer (or only) pie.
            @note In the case of ties, multiple labels will be returned.
            @returns The labels of the largest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetLargestOuterPieSlices() const;
        /** @brief Gets the labels of the smallest slice(s) along the outer (or only) pie.
            @note In the case of ties, multiple labels will be returned.
            @returns The labels of the smallest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetSmallestOuterPieSlices() const;

        /** @brief Ghosts or unghosts the slices of the outer (or only) pie.
            @param ghost @c true to make the slices translucent, @c false to make them opaque.
            @note This should be called after SetData().*/
        void GhostOuterPieSlices(bool ghost);
        /** @brief Ghosts or unghosts the slices of the outer (or only) pie.
            @param ghost @c true to make the slices translucent, @c false to make them opaque.
            @param slicesToGhost Which slices to ghost or unghost.\n
                Slice labels not in this list will have the opposite of @c ghost applied to them.
            @note This should be called after SetData().*/
        void GhostOuterPieSlices(bool ghost, const std::vector<wxString>& slicesToGhost);

        /** @brief Shows or hides the outside labels of the outer (or main) pie.
            @param show @c true to show the labels, @c false to hide them.
            @note This should be called after SetData().*/
        void ShowOuterPieLabels(bool show);
        /** @brief Shows or hides the outside labels of the outer (or main) pie.
            @param show @c true to show the labels, @c false to hide them.
            @param labelsToShow Which labels to either show or hide.\n
                Slice labels not in this list will have the opposite of @c show applied to them.
            @note This should be called after SetData().*/
        void ShowOuterPieLabels(bool show, const std::vector<wxString>& labelsToShow);

        /** @brief Shows or hides the mid-point labels of the outer (or main) pie.
            @param show @c true to show the labels, @c false to hide them.
            @note This should be called after SetData().\n
                Also, if @c show is @c true, then the default outer pie's mid-point
                display will be used. If that happens to be `BinLabelDisplay::NoDisplay`,
                then the label will not be shown.
            @sa SetOuterPieMidPointLabelDisplay().*/
        void ShowOuterPieMidPointLabels(bool show);
        /** @brief Shows or hides the mid-point labels of the outer (or main) pie.
            @param show @c true to show the labels, @c false to hide them.
            @param labelsToShow Which labels to either show or hide.\n
                Slice labels not in this list will have the opposite of @c show applied to them.
            @note This should be called after SetData().\n
                Also, if @c show is @c true, then the default outer pie's mid-point
                display will be used. If that happens to be `BinLabelDisplay::NoDisplay`,
                then the label will not be shown.
            @sa SetOuterPieMidPointLabelDisplay().*/
        void ShowOuterPieMidPointLabels(bool show, const std::vector<wxString>& labelsToShow);

        /// @}

        /// @name Inner Pie Functions
        /// @brief Functions for customizing the inner ring of the pie chart.\n
        ///     This is only relevant if using subgrouping.
        /// @{

        /// @brief Accesses the inner pie (if a double series chart).
        ///     Use this to edit the labels for slices.
        /// @returns The inner pie.
        /// @code
        /// // example showing how to uppercase all outer labels for the inner ring
        /// // (where "plot" is a pie chart object)
        /// std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
        ///               [](auto& slice) noexcept
        ///               { slice.SetGroupLabel(slice.GetGroupLabel().Upper()); });
        /// @endcode
        [[nodiscard]]
        PieInfo& GetInnerPie() noexcept
            {
            return m_innerPie;
            }

        /// @brief Gets/sets the pen used for the lines connecting inner slices to their
        ///     labels outside the pie.
        /// @returns The inner pie connection line.
        [[nodiscard]]
        wxPen& GetInnerPieConnectionLinePen() noexcept
            {
            return m_connectionLinePen;
            }

        /// @brief Gets the line style used for the lines connecting inner slices to
        ///     their labels outside the pie.
        /// @returns The inner pie connection line style.
        [[nodiscard]]
        LineStyle GetInnerPieConnectionLineStyle() const noexcept
            {
            return m_connectionLineStyle;
            }

        /** @brief Sets the line style used for the lines connecting inner slices to
                their labels outside the pie.
            @param lStyle The line style to use.
            @note If label placement is LabelPlacement::Flush, then this will be overridden
                to use LineStyle::Lines.\n
                This is because the connection will need to be drawn as
                two lines (one going from the inner slice to outside the pie, and then one going
                from there to the label against the edge of the pie area). Using a style such as
                LineStyle::Arrows will look odd in this situation, so LineStyle::Lines will
                be explicitly used.
             @sa SetLabelPlacement().*/
        void SetInnerPieConnectionLineStyle(const LineStyle lStyle) noexcept
            {
            m_connectionLineStyle = lStyle;
            }

        /// @returns What the labels on the middle points along the inner ring are displaying.
        /// @note This is only relevant if using multiple group columns.
        [[nodiscard]]
        BinLabelDisplay GetInnerPieMidPointLabelDisplay() const noexcept
            {
            return m_innerPieMidPointLabelDisplay;
            }

        /// @brief Sets what the labels on the middle points along the inner ring are displaying.
        /// @param display What to display.
        void SetInnerPieMidPointLabelDisplay(const BinLabelDisplay display) noexcept
            {
            m_innerPieMidPointLabelDisplay = display;
            }

        /// @brief Brings to focus the largest slice(s) along the inner pie.
        /// @details This will make all other slices (including the outer pie) translucent
        ///     and hide their labels.
        /// @param byGroup If @c true, will highlight the largest inner slice(s) within each main
        ///     group from the outer pie. @c false will only highlight the largest slice(s)
        ///     along the entire inner pie.
        /// @param showOuterPieMidPointLabels @c true to show the mid-point labels on
        ///     the outer pie.\n
        ///     Setting this to @c false is useful if you wish to more aggressively hide
        ///     the outer pie.
        /// @note This is only relevant if using multiple group columns.
        void ShowcaseLargestInnerPieSlices(const bool byGroup,
                                           const bool showOuterPieMidPointLabels)
            {
            ShowOuterPieLabels(false);
            ShowOuterPieMidPointLabels(showOuterPieMidPointLabels);
            GhostOuterPieSlices(true);

            const std::vector<wxString> highlightSlices =
                byGroup ? GetLargestInnerPieSlicesByGroup() : GetLargestInnerPieSlices();
            ShowInnerPieLabels(true, highlightSlices);
            ShowInnerPieMidPointLabels(true, highlightSlices);
            GhostInnerPieSlices(false, highlightSlices);
            }

        /// @brief Brings to focus the smallest slice(s) along the inner pie.
        /// @details This will make all other slices (including the outer pie) translucent
        ///     and hide their labels.
        /// @param byGroup If @c true, will highlight the smallest inner slice(s) within each main
        ///     group from the outer pie. @c false will only highlight the smallest slice(s)
        ///     along the entire inner pie.
        /// @param showOuterPieMidPointLabels @c true to show the mid-point labels on
        ///     the outer pie.\n
        ///     Setting this to @c false is useful if you wish to more aggressively hide
        ///     the outer pie.
        /// @note This is only relevant if using multiple group columns.
        void ShowcaseSmallestInnerPieSlices(const bool byGroup,
                                            const bool showOuterPieMidPointLabels)
            {
            ShowOuterPieLabels(false);
            ShowOuterPieMidPointLabels(showOuterPieMidPointLabels);
            GhostOuterPieSlices(true);

            const std::vector<wxString> highlightSlices =
                byGroup ? GetSmallestInnerPieSlicesByGroup() : GetSmallestInnerPieSlices();
            ShowInnerPieLabels(true, highlightSlices);
            ShowInnerPieMidPointLabels(true, highlightSlices);
            GhostInnerPieSlices(false, highlightSlices);
            }

        /** @brief Gets the labels of the largest slice(s) along the inner pie
                (if using a secondary grouping variable).
            @note In the case of ties, multiple labels will be returned.
            @returns The labels of the largest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetLargestInnerPieSlices() const;
        /** @brief Gets the labels of the largest slice(s) along the inner pie for each group
                (i.e., parent pie slice).
            @details This only applies if using a secondary grouping variable.
            @note In the case of ties within a group, multiple labels will be returned.
            @returns The labels of the largest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetLargestInnerPieSlicesByGroup() const;
        /** @brief Gets the labels of the smallest slice(s) along the inner pie
                (if using a secondary grouping variable).
            @note In the case of ties, multiple labels will be returned.
            @returns The labels of the smallest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetSmallestInnerPieSlices() const;
        /** @brief Gets the labels of the smallest slice(s) along the inner pie for each group
                (i.e., parent pie slice).
            @details This only applies if using a secondary grouping variable.
            @note In the case of ties within a group, multiple labels will be returned.
            @returns The labels of the smallest pie slice(s).*/
        [[nodiscard]]
        std::vector<wxString> GetSmallestInnerPieSlicesByGroup() const;

        /** @brief Ghosts or unghosts the slices of the inner pie
                (if using a secondary grouping variable).
            @param ghost @c true to make the slices translucent, @c false to make them opaque.
            @note This should be called after SetData().\n
                Also, inner slices should only be ghosted if its parent slice is ghosted also;
                otherwise, its opaque parent slice will show through it.*/
        void GhostInnerPieSlices(bool ghost);
        /** @brief Ghosts or unghosts the slices of the inner pie
                (if using a secondary grouping variable).
            @details This is useful for pushing most slices to the background and drawing
                attention to the ones that are not ghosted.
            @param ghost @c true to make the slices translucent, @c false to make them opaque.
            @param slicesToGhost Which slices to ghost or unghost.\n
                Slice labels not in this list will have the opposite of @c ghost applied to them.
            @note This should be called after SetData().\n
                Also, inner slices should only be ghosted if its parent slice is ghosted also;
                otherwise, its opaque parent slice will show through it.*/
        void GhostInnerPieSlices(bool ghost, const std::vector<wxString>& slicesToGhost);

        /** @brief Shows or hides the outside labels of the inner pie
                (if using a secondary grouping variable).
            @param show @c true to show the labels, @c false to hide them.
            @note This should be called after SetData().*/
        void ShowInnerPieLabels(bool show);
        /** @brief Shows or hides the outside labels of the inner pie
                (if using a secondary grouping variable).
            @param show @c true to show the labels, @c false to hide them.
            @param labelsToShow Which labels to either show or hide.\n
                Slice labels not in this list will have the opposite of @c show applied to them.
            @note This should be called after SetData().*/
        void ShowInnerPieLabels(bool show, const std::vector<wxString>& labelsToShow);

        /** @brief Shows or hides the mid-point labels of the inner pie
                (if using a secondary grouping variable).
            @param show @c true to show the labels, @c false to hide them.
            @note This should be called after SetData().\n
                Also, if @c show is @c true, then the default inner pie's mid-point
                display will be used. If that happens to be `BinLabelDisplay::NoDisplay`,
                then the label will not be shown.
            @sa SetInnerPieMidPointLabelDisplay().*/
        void ShowInnerPieMidPointLabels(bool show);
        /** @brief Shows or hides the mid-point labels of the inner pie
                (if using a secondary grouping variable).
            @param show @c true to show the labels, @c false to hide them.
            @param labelsToShow Which labels to either show or hide.\n
                Slice labels not in this list will have the opposite of @c show applied to them.
            @note This should be called after SetData().\n
                Also, if @c show is @c true, then the default inner pie's mid-point
                display will be used. If that happens to be `BinLabelDisplay::NoDisplay`,
                then the label will not be shown.
            @sa SetInnerPieMidPointLabelDisplay().*/
        void ShowInnerPieMidPointLabels(bool show, const std::vector<wxString>& labelsToShow);

        /// @}

        /// @name Donut Hole Functions
        /// @brief Functions for adding and customizing a hole in the middle of the pie chart.
        /// @{

        /// @brief Whether a donut hole is being shown.
        /// @returns @c true if a donut hole is being shown.
        [[nodiscard]]
        bool IsIncludingDonutHole() const noexcept
            {
            return m_includeDonutHole;
            }

        /// @brief Whether to include a donut hole at the center of the pie.
        /// @param include @c true to include a donut hole.
        void IncludeDonutHole(const bool include) noexcept { m_includeDonutHole = include; }

        /// @brief Sets/gets the donut hole label.
        /// @returns The label drawn in the middle of the donut hole (if a donut hole is included).
        [[nodiscard]]
        Wisteria::GraphItems::Label& GetDonutHoleLabel() noexcept
            {
            return m_donutHoleLabel;
            }

        /// @returns The proportion of the pie that the donut hole consumes (0.0 - 1.0).
        [[nodiscard]]
        double GetDonutHoleProportion() const noexcept
            {
            return m_donutHoleProportion;
            }

        /// @brief Sets the proportion of the pie that the donut hole consumes.
        /// @param prop The proportion of the pie used for the hole.\n
        ///     Value should be between @c 0.0 and @c 0.95 (i.e., 0 - 95%). (This value is clamped
        ///     to 95% since a hole shouldn't consume the entire pie.)
        void SetDonutHoleProportion(const double prop) noexcept
            {
            m_donutHoleProportion = std::clamp(prop, 0.0, .95);
            }

        /// @returns The color of the donut hole.
        [[nodiscard]]
        wxColour& GetDonutHoleColor() noexcept
            {
            return m_donutHoleColor;
            }

        /// @brief Sets the donut hole color.
        /// @param color The background color of the donut hole.
        void SetDonutHoleColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_donutHoleColor = color;
                }
            }

        /// @}

        /// @name Legend Functions
        /// @brief Functions for creating legends.
        /// @{

        /** @brief Builds and returns a legend.
            @details This can be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.
            @returns The legend for the chart.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final
            {
            return options.GetRingPerimeter() == Perimeter::Inner ?
                CreateInnerPieLegend(options.GetPlacementHint()) :
                CreateOuterPieLegend(options.GetPlacementHint());
            }

        /** @brief Builds and returns a legend for the outer pie
                (or the only pie, if a single data series).
            @details This can be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.\n
                This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @returns The legend for the chart.
            @note Prefer using CreateLegend().*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateOuterPieLegend(LegendCanvasPlacementHint hint);

        /** @brief Builds and returns a legend for the inner pie (if a dual data series).
            @details This can be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.\n
                This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @returns The legend for the chart.
            @note Prefer using CreateLegend().*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label>
        CreateInnerPieLegend(LegendCanvasPlacementHint hint);

        /// @}

        /// @private
        [[nodiscard]]
        const PieInfo& GetInnerPie() const noexcept
            {
            return m_innerPie;
            }

        /// @private
        [[nodiscard]]
        const PieInfo& GetOuterPie() const noexcept
            {
            return m_outerPie;
            }

        /// @private
        [[nodiscard]]
        const Wisteria::GraphItems::Label& GetDonutHoleLabel() const noexcept
            {
            return m_donutHoleLabel;
            }

      private:
        using LabelLinePair = std::vector<std::pair<std::unique_ptr<GraphItems::Label>,
                                                    std::unique_ptr<GraphItems::Points2D>>>;
        struct GutterLabels
            {
            LabelLinePair m_outerTopLeftLabelAndLines;
            LabelLinePair m_outerBottomLeftLabelAndLines;
            LabelLinePair m_outerTopRightLabelAndLines;
            LabelLinePair m_outerBottomRightLabelAndLines;
            };
        struct DrawAreas
            {
            wxRect m_fullDrawArea;
            wxRect m_outerPieDrawArea;
            wxRect m_pieDrawArea;
            };
        /// @returns The indices along the outer pie of the provided slices.
        std::set<size_t> GetOuterPieIndices(const std::vector<wxString>& labels);
        void RecalcSizes(wxDC& dc) final;
        void CreateLabelAndConnectionLine(wxDC& dc, GutterLabels& gutterLabels,
                DrawAreas drawAreas, auto& pSlice,
                double& smallestOuterLabelFontSize,
                bool isInnerSlice);
        void DrawOuterPie(wxDC& dc, GutterLabels& gutterLabels, DrawAreas drawAreas,
                          double& smallestOuterLabelFontSize,
                          std::vector<std::unique_ptr<GraphItemBase>>& addedObjects);
        void DrawInnerPie(wxDC& dc, GutterLabels& gutterLabels, DrawAreas drawAreas,
                          double& smallestOuterLabelFontSize,
                          std::vector<std::unique_ptr<GraphItemBase>>& addedObjects);

        // clockface styling
        void AddClockTicks(const DrawAreas& drawAreas);
        void AddClockHands(const DrawAreas& drawAreas);
        // pizza styling
        void AddCrustRing(const DrawAreas& drawAreas);
        void AddToastedCheeseSpots(const DrawAreas& drawAreas);
        void AddPepperoni(const DrawAreas& drawAreas);

        /// @returns Mozzarella.
        [[nodiscard]]
        static wxColour GetCheeseColor()
            {
            return wxColour(255, 235, 190);
            }

        /** @brief Computes a point on the perimeter of an ellipse defined by a rectangle.

            @details Given a bounding rectangle, treats the rectangle as the axis-aligned
                bounding box of an ellipse and returns the Cartesian point located at the
                specified angular position along that ellipse.

                The angle is measured in degrees, where:
                - 0° lies on the positive X axis (to the right of the center),
                - angles increase clockwise (to match wxWidgets drawing conventions),
                - 90° is at the bottom,
                - 180° is to the left,
                - 270° is at the top.

            @param rect The axis-aligned bounding rectangle of the ellipse.
            @param angleDegrees The angular position along the ellipse, in degrees.
            @return A wxPoint representing the position on the ellipse perimeter
                corresponding to the specified angle.*/
        static wxPoint GetEllipsePointFromRect(const wxRect& rect, const double angleDegrees)
            {
            const double angleRadians = geometry::degrees_to_radians(angleDegrees);

            const double centerX = rect.GetLeft() + rect.GetWidth() * math_constants::half;
            const double centerY = rect.GetTop() + rect.GetHeight() * math_constants::half;

            const double radiusX = rect.GetWidth() * math_constants::half;
            const double radiusY = rect.GetHeight() * math_constants::half;

            return wxPoint{ wxRound(centerX + radiusX * std::cos(angleRadians)),
                            wxRound(centerY + radiusY * std::sin(angleRadians)) };
            }

         static double HashToUnitInterval(const uint32_t hashValue) noexcept
            {
            uint32_t value = hashValue;
            value ^= value << 13;
            value ^= value >> 17;
            value ^= value << 5;
            return safe_divide<double>(value & 0x00FFFFFF, 0x01000000);
            }

        static double RingIrregularity(const double angleDegrees, const uint32_t noiseSeed) noexcept
            {
            const double angleRadians = geometry::degrees_to_radians(angleDegrees);

            const double lowFrequencyWave =
                std::sin(angleRadians * 2.0 + static_cast<double>(noiseSeed % 100) * 0.01);

            const double midFrequencyWave =
                std::sin(angleRadians * 5.0 + static_cast<double>(noiseSeed % 200) * 0.02);

            const double combinedWave = (lowFrequencyWave * 0.65) + (midFrequencyWave * 0.35);

            const uint32_t jitterHash =
                static_cast<uint32_t>(angleDegrees * 10.0) ^ (noiseSeed * 2654435761u);

            const double jitter = (HashToUnitInterval(jitterHash) - math_constants::half) *
                                  math_constants::quarter;

            return combinedWave + jitter;
            }

        PieInfo m_innerPie;
        PieInfo m_outerPie;

        bool m_dynamicMargins{ false };
        GraphItems::Label m_leftMarginNote;
        GraphItems::Label m_rightMarginNote;

        BinLabelDisplay m_innerPieMidPointLabelDisplay{ BinLabelDisplay::BinPercentage };
        BinLabelDisplay m_outerPieMidPointLabelDisplay{ BinLabelDisplay::BinPercentage };
        BinLabelDisplay m_outerLabelDisplay{ BinLabelDisplay::BinName };
        LabelPlacement m_labelPlacement{ LabelPlacement::Flush };

        wxPen m_connectionLinePen{ wxPen(
            wxPenInfo(Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::AshGrey),
                      1, wxPenStyle::wxPENSTYLE_SHORT_DASH)) };
        LineStyle m_connectionLineStyle{ LineStyle::Arrows };

        PieSliceEffect m_sliceEffect{ PieSliceEffect::Solid };
        PieStyle m_pieStyle{ PieStyle::None };

        bool m_useColorLabels{ false };

        uint8_t m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };

        std::shared_ptr<const TextReplace> m_abbreviate{ std::make_shared<AbbreviateEnglish>(
            true) };

        // donut hole
        bool m_includeDonutHole{ false };
        Wisteria::GraphItems::Label m_donutHoleLabel;
        double m_donutHoleProportion{ math_constants::half };
        wxColour m_donutHoleColor{ Colors::ColorBrewer::GetColor(Colors::Color::White) };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_PIE_CHART_H
