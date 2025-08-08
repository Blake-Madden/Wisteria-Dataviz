/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_MULTI_SERIES_LINE_PLOT_H
#define WISTERIA_MULTI_SERIES_LINE_PLOT_H

#include "lineplot.h"

namespace Wisteria::Graphs
    {
    /** @brief %Line plot, which shows a separate line for multiple continuous series of data.

        @par %Data:
            This plot accepts a Data::Dataset, where continuous columns are the separate lines
            (i.e., the dependent measurements) and another column is the X values.
            (X can either be a continuous or categorical column.)

        @par Missing Data:
              - If either the X or Y value is missing data, then a gap in a line will be shown
              at where the observation appeared in the series. Because the points are drawn
              along the X axis as they appear in the data, a missing data value will not be included
              in the line, but will break the line. The following valid point in the series will
              restart the line.\n
              For example, if five points are being plotted and the third item contains missing
              data, then there will be a line going from the first to second point,
              then a break in the line, then a line between the fourth and fifth point.

        @note This differs from LinePlot in that it does not use grouping to split data into
            separate lines. Instead, separate series of data are used for each line. In other words,
            multiple columns of data can be used to plot different lines.

        @warning Unlike other applications, the order of the data for line plots is
            important in %Wisteria. The line(s) connecting the points is drawn in the order of the
            points as they appear in the data, whereas most other applications will simply connect
            the points going from left-to-right.\n
            \n
            This is by design so that missing data can be shown on the plot
            (as a break in the line), as well as drawing zig-zagging/spiral lines.
    */
    class MultiSeriesLinePlot : public LinePlot
        {
        wxDECLARE_DYNAMIC_CLASS(MultiSeriesLinePlot);
        MultiSeriesLinePlot() = default;

      public:
        /** @brief Constructor.
            @param canvas The canvas to draw the line plot on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.\n
                Set to a new shape scheme filled with `IconShape::BlankIcon` to not
                show markers for certain lines.
            @code
             // to turn off markers, pass this in as the shape scheme argument
             std::make_shared<IconScheme>(IconScheme{IconShape::BlankIcon});

             // to show an image as the point markers, use this
             std::make_shared<IconScheme>(IconScheme({IconShape::ImageIcon},
                wxBitmapBundle::FromSVGFile(L"logo.svg",
                    Image::GetSVGSize(L"logo.svg"))));

             // or show a different image for each line
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
        explicit MultiSeriesLinePlot(
            Canvas* canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr,
            const std::shared_ptr<LineStyleScheme>& linePenStyles = nullptr)
            : LinePlot(canvas, colors, shapes, linePenStyles)
            {
            }

#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic push
    #pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
        /** @brief Sets the data.
            @details Separate lines are created for each Y column of data provided.
            @param data The data to use for the line plot.
            @param yColumnNames The Y columns (must be continuous columns).
            @param xColumnName The X column data (a continuous, categorical, or date column).\n
                If a categorical column, the columns labels will be assigned to the X axis.
                Also, the categories will be placed along the X axis in the order of their
                underlying numeric values (usually the order that they were read from a file).
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
                             const std::vector<wxString>& yColumnNames,
                             const wxString& xColumnName);
#if defined(__clang__) || defined(__GNUC__)
    #pragma GCC diagnostic pop
#endif

        /** @brief Builds and returns a legend using the current colors and labels.
            @details This can then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.\n
            @returns The legend for the plot.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

      private:
        void AddLine(const LinePlot::Line& line, const wxString& yColumnName);
        void RecalcSizes(wxDC& dc) final;
        std::vector<Data::ContinuousColumnConstIterator> m_yColumns;
        std::vector<wxString> m_yColumnNames;
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_MULTI_SERIES_LINE_PLOT_H
