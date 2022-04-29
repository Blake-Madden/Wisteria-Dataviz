/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_LINE_PLOT_H__
#define __WISTERIA_LINE_PLOT_H__

#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief %Line plot, which shows a continuous series of X and Y points.

        | %Line Plot      | %Line Plot (more customized)      |
        | :-------------- | :-------------------------------- |
        | @image html LinePlot.svg width=90% | @image html LinePlotCustomized.svg width=90% |

        @par %Data:
         This plot accepts a Data::Dataset, where a continuous column is the Y values
         (i.e., the dependent measurements) and another continuous column is the X values.
         A grouping column can optionally be used to create separate lines for different groups
         in the data.

        @par Missing Data:
         - Missing data in the group column will be shown as an empty legend label.
         - If either the X or Y value is missing data, then a gap in the line will be shown
           at where the observation appeared in the series. Because the points are drawn
           along the X axis as the appear in the data, a missing data value will not be included
           in the line, but will break the line. The following valid point in the series will
           restart the line.\n
           For example, if five points are being plotted and the third item contains missing data,
           then there will be a line going from the first to second point, then a break in the line,
           then a line between the fourth and fifth point.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the line plot
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto spellingTestData = std::make_shared<Data::Dataset>();
         spellingTestData->ImportCSV(L"Spelling Grades.csv",
            ImportInfo().
            // Note that the order of the continuous columns is important.
            // The first one will be the Y data, the second the X data.
            ContinuousColumns({ L"AVG_GRADE", L"WEEK" }).
            CategoricalColumns({ { L"GENDER", CategoricalImportMethod::ReadAsStrings } }));
         auto linePlot = std::make_shared<LinePlot>(canvas,
            // use a different color scheme
            std::make_shared<Colors::Schemes::Decade1960s>(),
            // or create your own scheme
            // std::make_shared<Colors::Schemes::ColorScheme>
            //     (Colors::Schemes::ColorScheme{
            //         ColorBrewer::GetColor(Colors::Color::CadmiumRed),
            //         ColorBrewer::GetColor(Colors::Color::OctoberMist) }),

            // turn off markers by using a shape scheme filled with blank icons
            // (having just one icon in this scheme will get recycled for each line)
            std::make_shared<IconShapeScheme>(IconShapeScheme{ IconShape::BlankIcon }));
         // add padding around the plot
         linePlot->SetCanvasMargins(5, 5, 5, 5);

         // set the data and use the grouping column from the dataset to create separate lines
         linePlot->SetData(linePlotData, L"AVG_GRADE", L"WeeK", L"Gender");
         // after setting the data, customize the appearance of one of the lines by index
         linePlot->GetLine(1).GetPen().SetStyle(wxPenStyle::wxPENSTYLE_DOT_DASH);
         // iterate through the lines and change their color based on their names
         // (which will override the color scheme)
         for (auto& line : linePlot->GetLines())
            {
            if (line.GetText().CmpNoCase(L"Male") == 0)
                {
                line.GetPen().SetColour(
                    ColorBrewer::GetColor(Colors::Color::CelestialBlue));
                }
            else
                {
                line.GetPen().SetColour(
                    ColorBrewer::GetColor(Colors::Color::PinkSherbet));
                }
            }

         // add a note
         auto note = std::make_shared<Label>(
            GraphItemInfo(L"What happened this week?\nAre we sure this is correct???").
            Pen(*wxLIGHT_GREY).FontBackgroundColor(*wxWHITE).
            Anchoring(Anchoring::TopRightCorner).Padding(4, 4, 4, 4));
         linePlot->AddEmbeddedObject(note,
            // top corner of note
            wxPoint(3, 38),
            // the suspect data point to make to note point to
            wxPoint(4, 59));

         // add some titles
         linePlot->GetTitle().SetText(_(L"Average Grades"));
         linePlot->GetSubtitle().SetText(_(L"Average grades taken from\n"
                                            "last 5 weeks' spelling tests."));
         linePlot->GetCaption().SetText(_(L"Note: not all grades have been\n"
                                           "entered yet for last week."));

         // customize the X axis labels
         for (int i = 1; i < 6; ++i)
            {
            linePlot->GetBottomXAxis().SetCustomLabel(i,
                Label(wxString::Format(_(L"Week %i"), i)));
            }

         // add the line plot and its legend to the canvas
         canvas->SetFixedObject(0, 0, linePlot);
         canvas->SetFixedObject(0, 1, linePlot->CreateLegend(
                                          LegendCanvasPlacementHint::RightOrLeftOfGraph,
                                          true));
        @endcode
    */
    class LinePlot : public Graph2D
        {
    public:
        /// @brief A data series drawn on a line plot.
        class Line
            {
            friend class LinePlot;
        public:
            /// @name Line Display Functions
            /// @brief Functions relating to the visual display of the line
            ///  connecting the points.
            /// @{

            /// @returns The line pen. This can be customized to change the pattern,
            ///  color, and width of the line.
            /// @note Set this to transparent or @c wxNullPen to turn off the line
            ///  (e.g., if you only want to show the points).
            [[nodiscard]] wxPen& GetPen() noexcept
                { return m_linePen; }

            /// @returns How the segments between the points on a line are connected.
            [[nodiscard]] LineStyle GetStyle() const noexcept
                { return m_lineStyle; }
            /// @brief How the segments between the points on a line are connected.
            /// @param lineStyle The line style.
            void SetStyle(const LineStyle lineStyle) noexcept
                { m_lineStyle = lineStyle; }
            /// @}

            /// @returns The label for the line.
            /// @note This is only applicable if grouping is being used.
            [[nodiscard]] const wxString& GetText() const noexcept
                { return m_label; }

            /// @private
            [[nodiscard]] const wxPen& GetPen() const noexcept
                { return m_linePen; }
        private:
            /// @brief Sets the data.
            /// @param data The data to use for the line plot.
            /// @param yColumnName The Y column data.
            /// @param xColumnName The X column data.
            /// @param groupColumnName The (optional) grouping column to use.
            /// @param groupId The group ID for this line. Data points from @c data will
            ///  only be used for this line if their group ID is @c groupId.
            /// @throws std::runtime_error If any columns can't be found by name, throws an exception.
            void SetData(std::shared_ptr<const Data::Dataset> data,
                         const wxString& yColumnName,
                         const wxString& xColumnName,
                         std::optional<const wxString>& groupColumnName,
                         const Data::GroupIdType groupId);
            [[nodiscard]] const std::shared_ptr<const Data::Dataset>& GetData() const
                { return m_data; }

            std::shared_ptr<const Data::Dataset> m_data;
            std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
            std::vector<Wisteria::Data::Column<double>>::const_iterator m_yColumn;
            std::vector<Wisteria::Data::Column<double>>::const_iterator m_xColumn;
            wxString m_yColumnName;
            wxString m_xColumnName;
            std::optional<wxString> m_groupColumnName;

            Data::GroupIdType m_groupId{ 0 };
            wxString m_label;

            LineStyle m_lineStyle{ LineStyle::Lines };
            wxPen m_linePen{ wxPen(*wxBLACK, 2) };
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the line plot on.
            @param colors The color scheme to apply to the points.
             Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points.
             Leave as null to use the standard shapes.\n
             Set to a new shape scheme filled with IconShape::BlankIcon to not
             show markers for certain lines/groups.
            @param linePenStyles The line styles to use for the lines.
             The default is to use solid, straight lines.\n
             Set to a new line scheme filled with `wxPenStyle::wxTRANSPARENT`
             to not show any lines.*/
        explicit LinePlot(Canvas* canvas,
            std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<IconShapeScheme> shapes = nullptr,
            std::shared_ptr<LineStyleScheme> linePenStyles = nullptr) :
            Graph2D(canvas),
            m_colorScheme(colors != nullptr ? colors :
                Settings::GetDefaultColorScheme()),
            m_shapeScheme(shapes != nullptr ? shapes :
                std::make_shared<IconShapeScheme>(StandardShapes())),
            m_linePenStyles(linePenStyles != nullptr ?
                linePenStyles :
                std::make_shared<LineStyleScheme>(
                    LineStyleScheme{ { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Lines } }))
            {
            GetBottomXAxis().GetGridlinePen() = wxNullPen;
            GetLeftYAxis().StartAtZero(true);
            }

        /** @brief Sets the data.
            @details Along with the X and Y points, separate lines will be created based
             on the grouping column in the data. The group ID assigned to each line will
             also select which color, marker shape, and line style to use.
            @param data The data to use for the line plot.
            @param yColumnName The Y column data.
            @param xColumnName The X column data.
            @param groupColumnName The (optional) grouping column to use.
             This will split the data into separate lines based on this grouping column.
            @note To add missing points to the data so that a gap in the line will appear,
             set the point in question to NaN (@c std::numeric_limits<double>::quiet_NaN()).
            @warning The data points are drawn in the order that they appear in the dataset.
             The plot will make no effort to sort the data or ensure that it is.
             This is by design in case you need a line series to go backwards in certain
             spots (e.g., a downward spiral).
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        virtual void SetData(std::shared_ptr<const Data::Dataset> data,
                             const wxString& yColumnName,
                             const wxString& xColumnName,
                             std::optional<const wxString> groupColumnName = std::nullopt);

        /** @brief Sets an additional function to assign a point's color to something different
             from the rest of its group based on a set of criteria.
            @details This will be any @c std::function
             (or lambda) that takes two doubles (the X and Y values) and returns a color if the
             X and/or Y values meet a certain criteria. If the values don't meet the criteria, then
             an uninitialized @c wxColour should be returned. If the function returns an invalid
             @c wxColour (implying that the point didn't meet the criteria), then the parent line's
             color will be used.
            @param criteria The function/lambda to use as your criteria.
            @code
             // change the color for any point less than 60 to red to show if failing
             linePlot->SetPointColorCriteria(
                [](const double x, const double y)
                    {
                    return (y < 60.0) ?
                        wxColour(255, 0, 0) :
                        wxColour();
                    });
            @endcode*/
        void SetPointColorCriteria(PointColorCriteria criteria)
            { m_colorIf = criteria; }

        /** @brief Get the lines at the specified index.
            @param index The index of the line to get.
            @returns The line.
            @note This should be called after SetData().
            @sa GetLineCount().*/
        [[nodiscard]] Line& GetLine(const size_t index) noexcept
            {
            wxASSERT_LEVEL_2_MSG(index < m_lines.size(), L"Invalid index in GetLine()!");
            return m_lines.at(index);
            }
        /// @brief Gets the lines so that you can iterate through them and make edits
        ///  (e.g., changing the line color based on the label).
        /// @returns Direct access to the lines.
        /// @note This should be called after SetData().
        [[nodiscard]] std::vector<Line>& GetLines() noexcept
            { return m_lines; }
        /** @brief Gets the number of lines on the plot.
            @returns The number of lines.
            @note This should be called after SetData().*/
        [[nodiscard]] size_t GetLineCount() const noexcept
            { return m_lines.size(); }

        /** @brief When lines zigzag (i.e., go back-and-forth along the X axis),
             setting this to `true` will change the line to be drawn as a spline.
             This is useful when plotting a line that shows a downward spiral
             (refer to WCurvePlot as an example).
             @param autoSpline `true` to enable auto splining.*/
        void AutoSpline(const bool autoSpline) noexcept
            { m_autoSpline = autoSpline; }
        /// @returns `true` if auto splining is enabled.
        [[nodiscard]] bool IsAutoSplining() const noexcept
            { return m_autoSpline; }

        /// @name Layout Functions
        /// @brief Functions relating to layout of the plot.
        /// @{

        /// @brief Gets the maximum number of points displayed before the parent canvas
        ///  is forced to be made wider (which will make this plot easier to read).
        /// @returns The most points that can be plotted before the parent canvas will be widened.
        [[nodiscard]] size_t GetPointsPerDefaultCanvasSize() const noexcept
            { return m_pointsPerDefaultCanvasSize; }
        /** @brief Sets the maximum number of points displayed before the parent canvas is
             forced to be made wider.
            @details Adjusting this is useful for when you have a large number of points and the
             display looks too condensed. Increasing this value will widen the plot, allowing for
             more space to spread the points out. The default is 100 points.
            @param pointsPerDefaultCanvasSize The number points to display before requiring the
             canvas to be made wider.*/
        void SetPointsPerDefaultCanvasSize(const size_t pointsPerDefaultCanvasSize)
            {
            m_pointsPerDefaultCanvasSize = pointsPerDefaultCanvasSize;
            UpdateCanvasForPoints();
            }
        /// @}

        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can be then be managed by the parent canvas and placed next to the plot.
            @param hint A hint about where the legend will be placed after construction.
              This is used for defining the legend's padding, outlining, canvas proportions, etc.
            @param includeHeader `true` to show the group column name as the header.
            @returns The legend for the chart.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
                                                         const LegendCanvasPlacementHint hint,
                                                         const bool includeHeader);
    protected:
        /// @returns The plot's dataset.
        [[nodiscard]] const std::shared_ptr<const Data::Dataset>& GetData() const noexcept
            { return m_data; }
        /// @returns The max value from the current X column (or NaN if dataset is invalid).
        [[nodiscard]] double GetMaxXValue() const noexcept
            {
            if (m_data == nullptr)
                { return std::numeric_limits<double>::quiet_NaN(); }
            return *std::max_element(
                m_xColumn->GetValues().cbegin(),
                m_xColumn->GetValues().cend());
            }
        /// @returns `true` if data is being grouped.
        [[nodiscard]] bool IsGrouping() const noexcept
            { return m_useGrouping; }
    private:
        /** @brief Adds a line to the plot.
            @param line The line to add.*/
        void AddLine(const Line& line);
        /// @brief Recalculates the size of embedded objects on the plot.
        void RecalcSizes(wxDC& dc) final;
        /// @brief Get the shape scheme used for the points.
        /// @returns The shape scheme used for the points.
        [[nodiscard]] const std::shared_ptr<IconShapeScheme>& GetShapeScheme() const noexcept
            { return m_shapeScheme; }

        /// @brief Get the color scheme used for the points.
        /// @returns The color scheme used for the points.
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }

        [[nodiscard]] const std::shared_ptr<LineStyleScheme>& GetPenStyleScheme() const noexcept
            { return m_linePenStyles; }

        [[nodiscard]] bool IsDataSingleDirection(std::shared_ptr<const Data::Dataset>& data,
                                                 const Data::GroupIdType group) const noexcept;
        void UpdateCanvasForPoints()
            {
            for (const auto& line : m_lines)
                {
                if (line.GetData()->GetRowCount() > GetPointsPerDefaultCanvasSize())
                    {
                    GetCanvas()->SetCanvasMinWidthDIPs(GetCanvas()->GetDefaultCanvasWidthDIPs() *
                        std::ceil(safe_divide<double>(line.GetData()->GetRowCount(),
                                                      GetPointsPerDefaultCanvasSize())));
                    }
                }
            }

        std::shared_ptr<const Data::Dataset> m_data;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_xColumn;

        std::vector<Line> m_lines;
        size_t m_pointsPerDefaultCanvasSize{ 100 };
        bool m_useGrouping{ false };
        bool m_autoSpline{ true };

        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        std::shared_ptr<IconShapeScheme> m_shapeScheme;
        std::shared_ptr<LineStyleScheme> m_linePenStyles;

        PointColorCriteria m_colorIf;
        };
    }

/** @}*/

#endif //__WISTERIA_LINE_PLOT_H__
