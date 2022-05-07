/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_PIE_CHART_H__
#define __WISTERIA_PIE_CHART_H__

#include <map>
#include "graph2d.h"
#include "../base/colorbrewer.h"
#include "../math/mathematics.h"

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
        PieSlice(const GraphItemInfo& info, const wxRect& pieRect,
                 const double startAngle, const double endAngle,
                 const double value, const double percent) :
            m_pieArea(pieRect), m_startAngle(startAngle), m_endAngle(endAngle),
            m_value(value), m_percent(percent)
            { GetGraphItemInfo() = info; }
        /** @brief Creates a label to display in the middle of the slice.
             This is usually a raw count of observations in the slice, or its percentage of the
             overall pie.
            @param dc The device context to measure the Label with.
            @param pieProportion The proportion of the pie area that this slice's ring consumes.
            @param labelDisplay What to display in the label.
            @returns The label, which will already be anchored to the middle of the slice.*/
        [[nodiscard]] std::shared_ptr<Wisteria::GraphItems::Label> CreateMiddleLabel(
                          wxDC& dc, const double pieProportion, const BinLabelDisplay labelDisplay);
        /** @brief Creates a label to display at the outer ring of the pie.
             This is usually the group label of the slice.
            @param dc The device context to measure the Label with.
            @returns The label, which will already be anchored to the middle of the slices's outer ring.*/
        [[nodiscard]] std::shared_ptr<Wisteria::GraphItems::Label> CreateOuterLabel(wxDC& dc);
        /** @brief Creates a label to display at the outer ring of the pie.
             This is usually the group label of the slice.
            @param dc The device context to measure the Label with.
            @param pieArea A different pie area from what the slice is currently using. This is useful
             for inner ring slices that need its outer labels to be outside the parent ring.
            @returns The label, which will already be anchored to the middle of the slices's outer ring.*/
        [[nodiscard]] std::shared_ptr<Wisteria::GraphItems::Label> CreateOuterLabel(
                          wxDC& dc, const wxRect& pieArea);
        /// @returns The middle point of the outer rim of the slice's arc.
        /// @param pieProportion The proportion of the pie that this arc should consume.
        ///  For example, `.5` would be an arc in the middle of the pie, and `1` would be the outer
        ///  arc of the pie.
        [[nodiscard]] std::pair<double, double> GetMiddleOfArc(const double pieProportion) const noexcept;
        /// @returns The middle point of the outer rim of the slice's arc.
        /// @param pieProportion The proportion of the pie that this arc should consume.
        ///  For example, `.5` would be an arc in the middle of the pie, and `1` would be the outer
        ///  arc of the pie.
        /// @param pieArea A custom area to align the point with. This is useful for moving an outer label
        ///  further out from the slice.
        [[nodiscard]] std::pair<double, double> GetMiddleOfArc(const double pieProportion,
                                                               const wxRect pieArea) const noexcept;
        /// @returns The (approximate) polygon of the slice.
        std::vector<wxPoint> GetPolygon() const noexcept;
    private:
        wxRect Draw(wxDC& dc) const final;

        bool HitTest(const wxPoint pt, wxDC& dc) const final
            {
            auto points = GetPolygon();
            return Polygon::IsInsidePolygon(pt, &points[0], points.size());
            }

        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            { return Polygon::GetPolygonBoundingBox(GetPolygon()); }

        // obligatory virtual interfaces that aren't implemented
        [[deprecated("Not implemented")]]
        void SetBoundingBox(const wxRect&, wxDC& dc, const double) final
            { wxFAIL_MSG("Not implemented for PieSlice"); }
        [[deprecated("Not implemented")]]
        void Offset(const int, const int) final
            { wxFAIL_MSG("Not implemented for PieSlice"); }

        wxRect m_pieArea;
        double m_startAngle{ 0 };
        double m_endAngle{ 0 };
        double m_value{ 0 };
        double m_percent{ 0 };
        };
    }

namespace Wisteria::Graphs
    {
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

        @par %Data:
         This plot accepts a Data::Dataset where a continuous column is the measurement,
         and a grouping column is the groups that the continuous values are aggregated into.
         Optionally, a second grouping column can be used to create subgroups within the main slices
         of the pie. These subgroups are shown along an inner ring, lined up against their parent
         group slices in the outer ring.

         Below is an example where the continuous is `Enrollment`, the main grouping column is `College`,
         and the optional second grouping column is `Course`.

         | Course            | College                   | Enrollment |
         | :--               | :--                       | --:        |
         | Science           | Anatomy & Physiology I    | 52         |
         | Business          | Business Ethics           | 51         |
         | English           | Creative Writing          | 47         |
         | Computer Science  | COBOL                     | 45         |
         | Business          | Introduction to Economics | 34         |
         | Business          | Introduction to Economics | 32         |
         | Computer Science  | C++                       | 32         |
         | Science           | Organic Chemistry I       | 32         |
         | Business          | Business Communication    | 31         |
         | Science           | Organic Chemistry I       | 28         |
         | Business          | Business Communication    | 27         |
         | Computer Science  | C++                       | 27         |

         ...

         With the above data, and outer ring for the colleges will be drawn, where the values from
         `Enrollment` will be aggregated into it. Also, an inner ring for the courses will be drawn,
         showing the aggregated enrollment numbers for each courses. The courses' slices will be
         aligned next to parent College slice that they belong to.

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
            pieData->ImportCSV(L"datasets/Fall Enrollment.csv",
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
         // turn off all but one of the outer labels for the inner ring,
         // and also add a custom description to one of the inner slices
         std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
            [](auto& slice) noexcept
                {
                if (slice.GetGroupLabel().CmpNoCase(L"Visual Basic.NET") != 0)
                    { slice.ShowGroupLabel(false); }
                else
                    { slice.SetDescription(_(L"Drop this from the catalog?")); }
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
                 plot->CreateInnerPieLegend(LegendCanvasPlacementHint::RightOrLeftOfGraph));
        @endcode*/
    class PieChart final : public Graph2D
        {
    public:
        /// @brief Information about a pie slice.
        class SliceInfo
            {
            friend class PieChart;
        public:
            /// @brief Constructor.
            /// @param groupLabel The main label for a slice
            ///  (usually the group label from the dataset).
            /// @param value The value (e.g., number of items in the group) for the slice.
            /// @param percent The percent of the pie that this slice consumes.
            /// @param parentSliceGroup The slice's parent slice
            ///  (only applies only to the inner pie).
            explicit SliceInfo(const wxString& groupLabel, const double value = 0,
                               const double percent = 0,
                               Wisteria::Data::GroupIdType parentSliceGroup = 0) :
                m_groupLabel(groupLabel), m_value(value),
                m_percent(percent), m_parentSliceGroup(parentSliceGroup)
                {}

            /// @returns The text shown on the outer ring of the slice.
            [[nodiscard]] const wxString& GetGroupLabel() const noexcept
                { return m_groupLabel; }
            /// @brief The main label for a slice (usually the group label from a dataset).
            /// @param label The label.
            void SetGroupLabel(const wxString& label)
                { m_groupLabel = label; }
            /// @returns The description shown on the outer ring of the slice
            ///  (beneath the label).
            /// @note This value is not gathered from the dataset, but rather added by
            ///  the client code.
            [[nodiscard]] const wxString& GetDescription() const noexcept
                { return m_description; }
            /// @brief An optional description to show below the main label.
            /// @param desc The description.
            void SetDescription(const wxString& desc)
                { m_description = desc; }
            /// @brief Whether to display the slice's label outside the outer ring of the pie.
            /// @param show `true` to show the label.
            /// @note Setting this to `false` will also hide the description.
            ///  The label and description will still be shown if the slice is selected.
            void ShowGroupLabel(const bool show) noexcept
                { m_showText = show; }

            /// @private
            [[nodiscard]] bool operator==(const SliceInfo& that) const noexcept
                { return m_groupLabel.CmpNoCase(that.m_groupLabel) == 0; }
            /// @private
            [[nodiscard]] bool operator<(const SliceInfo& that) const noexcept
                { return m_groupLabel.CmpNoCase(that.m_groupLabel) < 0; }
        private:
            wxString m_groupLabel;
            wxString m_description;
            bool m_showText{ true };
            double m_value{ 0.0 };
            double m_percent{ 0.0 };
            Wisteria::Data::GroupIdType m_parentSliceGroup{ 0 };
            };

        /// @brief A vector of informational blocks about a pie ring's slices.
        using PieInfo = std::vector<SliceInfo>;

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param colors The color scheme to apply to the points.
             Leave as null to use the default theme.*/
        explicit PieChart(Canvas* canvas,
                          std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr);

        /** @brief Sets the data for the chart.
            @param data The data to use, which will use a continuous column
             for the values, a grouping column for dividing the values, and (optionally)
             the second grouping column to create an inner (subgrouped) ring.
            @param continuousColumnName The data column.
            @param groupColumn1Name The main grouping ring.
            @param groupColumn2Name The (optional) second grouping ring.
             This inner ring will be shown as subgroups within each slice
             within the (parent) outer ring.
            @throws std::runtime_error If any columns can't be found by name,
             throws an exception.\n
             The exception's @c what() message is UTF-8 encoded, so pass it to @c wxString::FromUTF8()
             when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
            const wxString& continuousColumnName,
            const wxString& groupColumn1Name,
            std::optional<const wxString> groupColumn2Name = std::nullopt);

        /// @returns `true` if outer slice labels have their font colors match
        ///  their respective pie slice.
        [[nodiscard]] bool IsUsingColorLabels() const noexcept
            { return m_useColorLabels; }
        /// @brief Whether outer slice labels have their font colors match
        ///  their respective pie slice.
        /// @param useColors `true` to use colors for the labels.
        void UseColorLabels(const bool useColors) noexcept
            { m_useColorLabels = useColors; }

        /// @name Outer Pie Functions
        /// @brief Functions for customizing the outer ring of the pie chart.
        ///  If subgrouping is not being used, then this will be the only pie ring.
        /// @{

        /// @brief Accesses the outer pie (or the only pie if a single series chart).
        ///  Use this to edit the labels and descriptions for slices.
        /// @returns The outer (i.e., main) pie.
        [[nodiscard]] PieInfo& GetOuterPie() noexcept
            { return m_outerPie; }

        /// @returns What the labels on the middle points along the outer ring are displaying.
        /// @note If only using a single grouping column, then this refers to the main pie.
        [[nodiscard]] BinLabelDisplay GetOuterPieMidPointLabelDisplay() const noexcept
            { return m_outerPieMidPointLabelDisplay; }
        /// @brief Sets what the labels on the middle points along the outer ring are displaying.
        /// @param display What to display.
        void SetOuterPieMidPointLabelDisplay(const BinLabelDisplay display) noexcept
            { m_outerPieMidPointLabelDisplay = display; }
        /// @}

        /// @name Inner Pie Functions
        /// @brief Functions for customizing the inner ring of the pie chart.
        ///  This is only relevant if using subgrouping.
        /// @{

        /// @brief Accesses the inner pie (if a double series chart).
        ///  Use this to edit the labels for slices.
        /// @returns The inner pie.
        /// @code
        /// // example showing how to turn off all outer labels for the inner ring
        /// std::for_each(plot->GetInnerPie().begin(), plot->GetInnerPie().end(),
        ///               [](auto& slice) noexcept
        ///               { slice.SetGroupLabel(false); });
        /// @endcode
        [[nodiscard]] PieInfo& GetInnerPie() noexcept
            { return m_innerPie; }
        /// @brief Gets/sets the pen used for the lines connecting inner slices to their
        ///  labels outside of the pie.
        /// @returns The inner pie connection line.
        [[nodiscard]] wxPen& GetInnerPieConnectionLinePen() noexcept
            { return m_connectionLinePen; }

        /// @brief Gets/sets the line style used for the lines connecting inner slices to
        ///  their labels outside of the pie.
        /// @returns The inner pie connection line style.
        [[nodiscard]] LineStyle& GetInnerPieConnectionLineStyle() noexcept
            { return m_connectionLineStyle; }

        /// @returns What the labels on the middle points along the inner ring are displaying.
        /// @note This is only relevant if using multiple group columns.
        [[nodiscard]] BinLabelDisplay GetInnerPieMidPointLabelDisplay() const noexcept
            { return m_innerPieMidPointLabelDisplay; }
        /// @brief Sets what the labels on the middle points along the inner ring are displaying.
        /// @param display What to display.
        void SetInnerPieMidPointLabelDisplay(const BinLabelDisplay display) noexcept
            { m_innerPieMidPointLabelDisplay = display; }
        /// @}

        /// @name Donut Hole Functions
        /// @brief Functions for adding and customizing a hole in the middle of the pie chart.
        /// @{

        /// @brief Whether a donut hole is being shown.
        /// @returns `true` if a donut hole is being shown.
        [[nodiscard]] bool IsIncludingDonutHole() const noexcept
            { return m_includeDonutHole; }
        /// @brief Whether to include a donut hole at the center of the pie.
        /// @param include `true` to include a donut hole.
        void IncludeDonutHole(const bool include) noexcept
            { m_includeDonutHole = include; }

        /// @brief Sets/gets the donut hole label.
        /// @returns The label drawn in the middle of the donut hole (if a donut hole is included).
        [[nodiscard]] Wisteria::GraphItems::Label& GetDonutHoleLabel() noexcept
            { return m_donutHoleLabel; }

        /// @returns The proportion of the pie that the donut hole consumes (0.0 - 1.0).
        [[nodiscard]] double GetDonutHoleProportion() const noexcept
            { return m_donutHoleProportion; }
        /// @brief Sets the proportion of the pie that the donut hole consumes.
        /// @param prop The proportion of the pie used for the hole.
        ///  Value should be between 0.0 and .95 (i.e., 0 - 95%). This value is clamped
        ///  to 95% since a hole shouldn't consume the entire pie.
        void SetDonutHoleProportion(const double prop) noexcept
            { m_donutHoleProportion = std::clamp(prop, 0.0, .95); }

        /// @returns The color of the donut hole.
        [[nodiscard]] wxColour& GetDonutHoleColor() noexcept
            { return m_donutHoleColor; }
        /// @brief Sets the donut hole color.
        /// @param color The background color of the donut hole.
        void SetDonutHoleColor(const wxColour color) noexcept
            { m_donutHoleColor = color; }
        /// @}

        /// @name Legend Functions
        /// @brief Functions for creating legends.
        /// @{

        /** @brief Builds and returns a legend for the outer pie
            (or the only pie, if a single data series).
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
             This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateOuterPieLegend(
            const LegendCanvasPlacementHint hint);

        /** @brief Builds and returns a legend for the inner pie (if a dual data series).
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
             This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateInnerPieLegend(
            const LegendCanvasPlacementHint hint);
        /// @}

        /// @private
        [[nodiscard]] const PieInfo& GetInnerPie() const noexcept
            { return m_innerPie; }
        /// @private
        [[nodiscard]] const PieInfo& GetOuterPie() const noexcept
            { return m_outerPie; }
        /// @private
        [[nodiscard]] const Wisteria::GraphItems::Label& GetDonutHoleLabel() const noexcept
            { return m_donutHoleLabel; }
    private:
        void RecalcSizes(wxDC& dc) final;

        /// @brief Get the color scheme used for the slices.
        /// @returns The color scheme used for the slices.
        std::shared_ptr<Colors::Schemes::ColorScheme> GetColorScheme() const noexcept
            { return m_pieColors; }

        PieInfo m_innerPie;
        PieInfo m_outerPie;

        BinLabelDisplay m_innerPieMidPointLabelDisplay{ BinLabelDisplay::BinPercentage };
        BinLabelDisplay m_outerPieMidPointLabelDisplay{ BinLabelDisplay::BinPercentage };

        wxPen m_connectionLinePen{ wxPen(
            wxPenInfo(Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::AshGrey, 200),
                      ScaleToScreenAndCanvas(1), wxPenStyle::wxPENSTYLE_SHORT_DASH)) };
        LineStyle m_connectionLineStyle{ LineStyle::Arrows };

        std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> m_pieColors;

        std::shared_ptr<const Wisteria::Data::Dataset> m_data{ nullptr };
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn1;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn2;

        bool m_useColorLabels{ false };

        // donut hole
        bool m_includeDonutHole{ false };
        Wisteria::GraphItems::Label m_donutHoleLabel;
        double m_donutHoleProportion{ .5 };
        wxColour m_donutHoleColor{ *wxWHITE };
        };
    }

/** @}*/

#endif //__WISTERIA_PIE_CHART_H__
