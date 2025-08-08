/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_LINE_PLOT_H
#define WISTERIA_LINE_PLOT_H

#include "groupgraph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief %Line plot, which shows a continuous series of X and Y points.

        | %Line Plot      | %Line Plot (more customized)      |
        | :-------------- | :-------------------------------- |
        | @image html LinePlot.svg width=90% | @image html LinePlotCustomized.svg width=90% |

        @par %Data:
            This plot accepts a Data::Dataset, where a continuous column is the Y values
            (i.e., the dependent measurements) and another column is the X values.
            (X can either be a continuous or categorical column.)
            A grouping column can optionally be used to create separate lines for different groups
            in the data.

        @par Missing Data:
            - Missing data in the group column will be shown as an empty legend label.
            - If either the X or Y value is missing data, then a gap in the line will be shown
              at where the observation appeared in the series. Because the points are drawn
              along the X axis as they appear in the data, a missing data value will not be included
              in the line, but will break the line. The following valid point in the series will
              restart the line.\n
              For example, if five points are being plotted and the third item contains missing
              data, then there will be a line going from the first to second point,
              then a break in the line, then a line between the fourth and fifth point.

        @note This differs from MultiSeriesLinePlot in that it only plots one series of data.
            To create separate lines, an optional grouping column is used to split the data.

        @warning Unlike other applications, the order of the data for line plots is
            important in %Wisteria. The line(s) connecting the points is drawn in the order of the
            points as they appear in the data, whereas most other applications will simply connect
            the points going from left-to-right.\n
            \n
            This is by design so that missing data can be shown on the plot
            (as a break in the line), as well as drawing zig-zagging/spiral lines.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the line plot
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto spellingTestData = std::make_shared<Data::Dataset>();
         spellingTestData->ImportCSV(L"/home/rdoyle/data/Spelling Grades.csv",
            ImportInfo().
            // Note that the order of the continuous columns is important.
            // The first one will be the Y data, the second the X data.
            ContinuousColumns({ L"AVG_GRADE" }).
            CategoricalColumns({
            { L"GENDER", CategoricalImportMethod::ReadAsStrings },
            { L"WEEK_NAME", CategoricalImportMethod::ReadAsStrings }
            }));
         auto linePlot = std::make_shared<LinePlot>(canvas,
            // use a different color scheme
            std::make_shared<Colors::Schemes::Decade1960s>(),
            // or create your own scheme
            // std::make_shared<Colors::Schemes::ColorScheme>
            //     (Colors::Schemes::ColorScheme{
            //         ColorBrewer::GetColor(Colors::Color::Auburn),
            //         ColorBrewer::GetColor(Colors::Color::OctoberMist) }),

            // turn off markers by using a shape scheme filled with blank icons
            // (having just one icon in this scheme will get recycled for each line)
            std::make_shared<IconScheme>(IconScheme{ IconShape::BlankIcon }));
         // add padding around the plot
         linePlot->SetCanvasMargins(5, 5, 5, 5);

         // set the data and use the grouping column from the dataset to create separate lines
         linePlot->SetData(linePlotData, L"AVG_GRADE", L"WEEK_NAME", L"Gender");
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
            GraphItemInfo(_(L"What happened this week?\nAre we sure this is correct???")).
            Pen(*wxLIGHT_GREY).FontBackgroundColor(*wxWHITE).
            Anchoring(Anchoring::TopRightCorner).Padding(4, 4, 4, 4));
         linePlot->AddAnnotation(note,
            // top corner of note
            wxPoint(3, 38),
            // the suspect data point to make the note point to
            wxPoint(4, 59));

         // add some titles
         linePlot->GetTitle().SetText(_(L"Average Grades"));
         linePlot->GetSubtitle().SetText(_(L"Average grades taken from\n"
                                            "last 5 weeks' spelling tests."));
         linePlot->GetCaption().SetText(_(L"Note: not all grades have been\n"
                                           "entered yet for last week."));

         // add the line plot and its legend to the canvas
         canvas->SetFixedObject(0, 0, linePlot);
         canvas->SetFixedObject(0, 1,
            linePlot->CreateLegend(
                LegendOptions().
                    IncludeHeader(true).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph)) );
        @endcode
    */
    class LinePlot : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(LinePlot);
        LinePlot() = default;

      public:
        /// @brief A data series drawn on a line plot.
        class Line
            {
          public:
            /// @name Line Display Functions
            /// @brief Functions relating to the visual display of the line
            ///     connecting the points.
            /// @{

            /// @returns The line pen. This can be customized to change the pattern,
            ///     color, and width of the line.
            /// @note Set this to transparent or @c wxNullPen to turn off the line
            ///     (e.g., if you only want to show the points).
            [[nodiscard]]
            wxPen& GetPen() noexcept
                {
                return m_linePen;
                }

            /// @returns How the segments between the points on a line are connected.
            [[nodiscard]]
            LineStyle GetStyle() const noexcept
                {
                return m_lineStyle;
                }

            /// @brief How the segments between the points on a line are connected.
            /// @param lineStyle The line style.
            void SetStyle(const LineStyle lineStyle) noexcept { m_lineStyle = lineStyle; }

            /// @}

            /// @returns The label for the line.
            /// @note This is only applicable if grouping is being used.
            [[nodiscard]]
            const wxString& GetText() const noexcept
                {
                return m_label;
                }

            /// @private
            [[nodiscard]]
            const wxPen& GetPen() const noexcept
                {
                return m_linePen;
                }

            /// @brief Sets the grouping information connected to this line.
            /// @param groupColumnName The grouping column to use. This is used for
            ///     data validation later.
            /// @param groupId The group ID for this line. Data points from @c data will
            ///     only be used for this line if their group ID is @c groupId.
            /// @param groupName The display name of the group.\n
            ///     This is useful for a client to find a line by name and then customize it.
            void SetGroupInfo(const std::optional<wxString>& groupColumnName,
                              const Data::GroupIdType groupId, const wxString& groupName)
                {
                m_groupColumnName = groupColumnName;
                m_groupId = groupId;
                m_label = groupName;
                }

            /// @returns The shape.
            [[nodiscard]]
            Icons::IconShape GetShape() const noexcept
                {
                return m_shape;
                }

            /// @brief Sets the shape.
            /// @param shape The shape.
            void SetShape(const Icons::IconShape shape) { m_shape = shape; }

            /// @returns The shape image.
            [[nodiscard]]
            const wxBitmapBundle& GetShapeImage() const noexcept
                {
                return m_shapeImg;
                }

            /// @brief Sets the shape image.
            /// @param shapeImg The shape image.
            void SetShapeImage(const wxBitmapBundle& shapeImg) { m_shapeImg = shapeImg; }

            /// @returns The group ID.
            [[nodiscard]]
            Data::GroupIdType GetGroupId() const noexcept
                {
                return m_groupId;
                }

            /// @returns The group column name.
            [[nodiscard]]
            std::optional<wxString> GetGroupColumnName() const
                {
                return m_groupColumnName;
                }

          private:
            std::optional<wxString> m_groupColumnName;
            Data::GroupIdType m_groupId{ 0 };
            wxString m_label;

            LineStyle m_lineStyle{ LineStyle::Lines };
            Icons::IconShape m_shape{ Icons::IconShape::Circle };
            wxBitmapBundle m_shapeImg;
            wxPen m_linePen{ wxPen{ *wxBLACK, 2 } };
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the line plot on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.\n
                Set to a new shape scheme filled with `IconShape::BlankIcon` to not
                show markers for certain lines/groups.
            @code
             // to turn off markers, pass this in as the shape scheme argument
             std::make_shared<IconScheme>(IconScheme{IconShape::BlankIcon});

             // to show an image as the point markers, use this
             std::make_shared<IconScheme>(IconScheme({IconShape::ImageIcon},
                wxBitmapBundle::FromSVGFile(L"logo.svg",
                    Image::GetSVGSize(L"logo.svg"))));

             // or show a different image for each group
             std::make_shared<IconScheme>(IconScheme({IconShape::ImageIcon},
                { wxBitmapBundle::FromSVGFile(L"hs.svg",
                    Image::GetSVGSize(L"hs.svg")),
                  wxBitmapBundle::FromSVGFile(L"university.svg",
                    Image::GetSVGSize(L"university.svg")) }));
            @endcode
            @param linePenStyles The line styles to use for the lines.
                The default is to use solid, straight lines.\n
                Set to a new line scheme filled with `wxPenStyle::wxTRANSPARENT`
                to not show any lines.*/
        explicit LinePlot(
            Canvas* canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr,
            const std::shared_ptr<LineStyleScheme>& linePenStyles = nullptr)
            : GroupGraph2D(canvas),
              m_linePenStyles(linePenStyles != nullptr ?
                                  linePenStyles :
                                  std::make_shared<LineStyleScheme>(LineStyleScheme{
                                      { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Lines } }))
            {
            SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
            SetShapeScheme(shapes != nullptr ?
                               shapes :
                               std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                   Wisteria::Icons::Schemes::StandardShapes()));
            GetBottomXAxis().GetGridlinePen() = wxNullPen;
            GetLeftYAxis().StartAtZero(true);
            }

#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
        /** @brief Sets the data.
            @details Along with the X and Y points, separate lines can be created based
                on the (optional) grouping column in the data.
                The group ID assigned to each line will select which color, marker shape,
                and line style to use.
            @param data The data to use for the line plot.
            @param yColumnName The Y column data (a continuous column).
            @param xColumnName The X column data (a continuous, categorical, or date column).\n
                If a categorical column, the columns labels will be assigned to the X axis.
                Also, the categories will be placed along the X axis in the order of their
                underlying numeric values (usually the order that they were read from a file).
            @param groupColumnName The (optional) categorical column to use for grouping.
                This will split the data into separate lines based on this grouping column.
            @note To add missing points to the data so that a gap in the line will appear,
                set the point in question to NaN (@c std::numeric_limits<double>::quiet_NaN()).\n
                Also, call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @warning The data points are drawn in the order that they appear in the dataset.
                The plot will make no effort to sort the data or ensure that it is.\n
                This is by design in case you need a line series to go backwards in certain
                spots (e.g., a downward spiral).
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        virtual void SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& yColumnName, const wxString& xColumnName,
                             const std::optional<wxString>& groupColumnName = std::nullopt);
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

        /** @brief Sets an additional function to assign a point's color to something different
                from the rest of its group based on a set of criteria.
            @details This will be any @c std::function
                (or lambda) that takes two doubles (the X and Y values) and returns a color if the
                X and/or Y values meet a certain criteria. If the values don't meet the criteria,
                then an uninitialized @c wxColour should be returned. If the function
                returns an invalid @c wxColour (implying that the point didn't meet the criteria),
                then the parent line's color will be used.
            @param criteria The function/lambda to use as your criteria.
            @code
             // change the color for any point less than 60 to red to show if failing
             linePlot->SetPointColorCriteria(
                [](const double x, const double y)
                    {
                    return (y < 60.0) ?
                        wxColour(255, 0, 0) :
                        wxNullColour;
                    });
            @endcode*/
        void SetPointColorCriteria(const PointColorCriteria& criteria) { m_colorIf = criteria; }

        /// @name Line Functions
        /// @brief Functions relating to accessing and customizing the lines.
        /// @{

        /** @brief Get the lines at the specified index.
            @param index The index of the line to get.
            @returns The line.
            @note This should be called after SetData().
            @sa GetLineCount().*/
        [[nodiscard]]
        Line& GetLine(const size_t index) noexcept
            {
            assert(index < m_lines.size() && L"Invalid index in GetLine()!");
            return m_lines.at(index);
            }

        /// @brief Gets the lines so that you can iterate through them and make edits
        ///     (e.g., changing the line color based on the label).
        /// @returns Direct access to the lines.
        /// @note This should be called after SetData().
        [[nodiscard]]
        std::vector<Line>& GetLines() noexcept
            {
            return m_lines;
            }

        /** @brief Gets the number of lines on the plot.
            @returns The number of lines.
            @note This should be called after SetData().*/
        [[nodiscard]]
        size_t GetLineCount() const noexcept
            {
            return m_lines.size();
            }

        /// @returns @c true if auto splining is enabled.
        [[nodiscard]]
        bool IsAutoSplining() const noexcept
            {
            return m_autoSpline;
            }

        /** @brief When lines zigzag (i.e., go back-and-forth along the X axis),
                setting this to @c true will change the line to be drawn as a spline.\n
                This is useful when plotting a line that shows a downward spiral
                (refer to WCurvePlot as an example).
            @param autoSpline @c true to enable auto splining.*/
        void AutoSpline(const bool autoSpline) noexcept { m_autoSpline = autoSpline; }

        /** @brief Sets the specified lines (by group label) to be fully opaque,
                and all others to a lighter opacity.
            @param labels The lines to showcase.
            @note Call SetGhostOpacity() prior to this to control how translucent
                the non-showcased (i.e., "ghosted") lines/points are.
            @warning This will only take effect if called after SetData().\
                Also, for LinePlot, this will only take effect if grouping is enabled.
            @sa SetGhostOpacity().*/
        void ShowcaseLines(const std::vector<wxString>& labels) { m_showcasedLines = labels; }

        /// @returns The opacity level applied if being "ghosted".
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /** @brief Sets the opacity level for "ghosted" lines/points.\n
                This is only used if ShowcaseLines() is called; this is the
                opacity applied to lines/points not being showcased.
            @param opacity The opacity level (should be between @c 0 and @c 255).
            @sa ShowcaseLines().*/
        void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

        /// @}

        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.\n
            @returns The legend for the plot.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

      protected:
        /// @brief Returns true if the value at @c index in the X column is valid (i.e., not NaN).
        /// @param index The row in the X column to review.
        /// @returns @c true if the given row in the X column is valid.
        [[nodiscard]]
        bool IsXValid(const size_t index) const
            {
            if (index >= GetDataset()->GetRowCount())
                {
                return std::numeric_limits<double>::quiet_NaN();
                }
            // continuous X
            if (IsXContinuous())
                {
                return !std::isnan(m_xColumnContinuous->GetValue(index));
                }
            // categorical X, which in this case anything would be OK (even empty string)
            else if (IsXCategorical())
                {
                return true;
                }
            else if (IsXDates())
                {
                return GetBottomXAxis()
                    .FindDatePosition(m_xColumnDate->GetValue(index))
                    .has_value();
                }
            else
                {
                return false;
                }
            }

        /// @brief Returns the value at @c index of the X column.
        /// @param index The row in the X column to retrieve.
        /// @returns The value of the given row in the X column.\n
        ///     Note that this value may be NaN if invalid.
        [[nodiscard]]
        double GetXValue(const size_t index) const
            {
            if (index >= GetDataset()->GetRowCount())
                {
                return std::numeric_limits<double>::quiet_NaN();
                }
            // continuous X
            if (IsXContinuous())
                {
                return m_xColumnContinuous->GetValue(index);
                }
            // categorical X, just want the underlying number code as that is what
            // goes along the X axis
            else if (IsXCategorical())
                {
                return static_cast<double>(m_xColumnCategorical->GetValue(index));
                }
            else if (IsXDates())
                {
                const auto foundDate =
                    GetBottomXAxis().FindDatePosition(m_xColumnDate->GetValue(index));
                return (foundDate.has_value() ? foundDate.value() :
                                                std::numeric_limits<double>::quiet_NaN());
                }
            else
                {
                return std::numeric_limits<double>::quiet_NaN();
                }
            }

        /** @brief Gets the min and max values of the X column (if a date axis).
            @returns The X column's min and max dates, which can be
                @c wxInvalidDateTime if invalid.*/
        [[nodiscard]]
        std::pair<wxDateTime, wxDateTime> GetXMinMaxDates() const
            {
            assert(IsXDates() &&
                   L"GetXMinMaxDates() should only be called if X axis is date based!");
            const auto [fullXDataMin, fullXDataMax] = std::minmax_element(
                m_xColumnDate->GetValues().cbegin(), m_xColumnDate->GetValues().cend());
            return std::make_pair(*fullXDataMin, *fullXDataMax);
            }

        /** @brief Gets the min and max values of the X column.
            @returns The X column's min and max values, which can be NaN if invalid.*/
        [[nodiscard]]
        std::pair<double, double> GetXMinMax() const
            {
            assert(!IsXDates() && L"GetXMinMaxDates() should be called instead!");
            if (GetDataset()->GetRowCount() == 0)
                {
                return std::make_pair(std::numeric_limits<double>::quiet_NaN(),
                                      std::numeric_limits<double>::quiet_NaN());
                }
            // continuous X
            if (IsXContinuous())
                {
                const auto [fullXDataMin, fullXDataMax] =
                    std::minmax_element(m_xColumnContinuous->GetValues().cbegin(),
                                        m_xColumnContinuous->GetValues().cend());
                return std::make_pair(*fullXDataMin, *fullXDataMax);
                }
            // categorical X, just want the underlying numeric code as that is what
            // goes along the X axis
            else if (IsXCategorical())
                {
                const auto [fullXDataMin, fullXDataMax] =
                    std::minmax_element(m_xColumnCategorical->GetValues().cbegin(),
                                        m_xColumnCategorical->GetValues().cend());
                return std::make_pair(static_cast<double>(*fullXDataMin),
                                      static_cast<double>(*fullXDataMax));
                }
            else
                {
                return std::make_pair(std::numeric_limits<double>::quiet_NaN(),
                                      std::numeric_limits<double>::quiet_NaN());
                }
            }

        /// @returns The pen styles used for the line(s).
        [[nodiscard]]
        const std::shared_ptr<LineStyleScheme>& GetPenStyleScheme() const noexcept
            {
            return m_linePenStyles;
            }

        /// @brief Returns whether the X column data is ordered ascendingly.
        /// @param data The dataset to review.
        /// @param group The group ID to use while stepping through the data.
        /// @returns @c true if X data is ordered (i.e., not weaving back-and-forth).
        /// @note If X is dates or categorical, then this simply return @c true.
        [[nodiscard]]
        bool IsDataSingleDirection(const std::shared_ptr<const Data::Dataset>& data,
                                   Data::GroupIdType group) const noexcept;

        /// @returns Whether X was loaded from a continuous column.
        [[nodiscard]]
        bool IsXContinuous() const noexcept
            {
            return (m_xColumnContinuous != GetDataset()->GetContinuousColumns().cend());
            }

        /// @returns Whether X was loaded from a categorical column.
        [[nodiscard]]
        bool IsXCategorical() const noexcept
            {
            return (m_xColumnCategorical != GetDataset()->GetCategoricalColumns().cend());
            }

        /// @returns Whether X was loaded from a date column.
        [[nodiscard]]
        bool IsXDates() const noexcept
            {
            return (m_xColumnDate != GetDataset()->GetDateColumns().cend());
            }

        /** @brief Returns a color that is ghosted if ghosting is enabled.
            @param color The base color.
            @param isGhosted Whether to ghost the color.
            @returns The (possibly) ghosted color.
         */
        [[nodiscard]]
        wxColour GetMaybeGhostedColor(const wxColour& color, const bool isGhosted) const
            {
            return (isGhosted && color.IsOk()) ?
                       Wisteria::Colors::ColorContrast::ChangeOpacity(color, GetGhostOpacity()) :
                       color;
            }

        /// @returns The showcased lines' names.
        [[nodiscard]]
        const std::vector<wxString>& GetShowcasedLines() const noexcept
            {
            return m_showcasedLines;
            }

        /// @returns The functor for determining how to color a point.
        [[nodiscard]]
        PointColorCriteria GetColorIf() const
            {
            return m_colorIf;
            }

        /// @brief Sets the X column from the dataset.
        /// @param xColumnName The X column from the dataset to use.
        ///     Can be continuous, categorical, or a data column.
        /// @details This will set the iterator to the proper column.
        /// @warning This must be called after setting the dataset.
        void SetXColumn(const wxString& xColumnName);

        /// @returns The iterator to the categorical column.
        /// @details This would usually be used to gather axis labels
        ///     from the categorical column's string table.
        /// @warning Call IsXCategorical() first to ensure that this iterator is valid.
        [[nodiscard]]
        Data::CategoricalColumnConstIterator GetXCategoricalColumnIterator()
            {
            return m_xColumnCategorical;
            }

      private:
        /// @brief Resets the X column iterators.
        /// @warning This must be called after setting the dataset.
        void ResetXColumns()
            {
            m_xColumnContinuous = GetDataset()->GetContinuousColumns().cend();
            m_xColumnCategorical = GetDataset()->GetCategoricalColumns().cend();
            m_xColumnDate = GetDataset()->GetDateColumns().cend();
            }

        /** @brief Adds a line to the plot.
            @param line The line to add.*/
        void AddLine(const Line& line);

        /// @brief Recalculates the size of embedded objects on the plot.
        void RecalcSizes(wxDC& dc) override;

        Data::ContinuousColumnConstIterator m_xColumnContinuous;
        Data::CategoricalColumnConstIterator m_xColumnCategorical;
        Data::DateColumnConstIterator m_xColumnDate;
        Data::ContinuousColumnConstIterator m_yColumn;
        wxString m_yColumnName;

        std::vector<Line> m_lines;
        bool m_autoSpline{ true };

        std::shared_ptr<LineStyleScheme> m_linePenStyles{ nullptr };

        PointColorCriteria m_colorIf;

        std::vector<wxString> m_showcasedLines;
        // the default ghost effect is OK for bars, but for thin lines it needs to be a
        // bit more opaque to be able to see anything
        uint8_t m_ghostOpacity{ std::max<uint8_t>(75, Wisteria::Settings::GHOST_OPACITY) };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_LINE_PLOT_H
