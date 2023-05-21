/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_W_CURVE_PLOT_H__
#define __WISTERIA_W_CURVE_PLOT_H__

#include "lineplot.h"

namespace Wisteria::Graphs
    {
    /** @brief W-Curve plot, which displays experiential, longitudinal data.
        @details An example of this is students' sense of belonging responses across semesters or years.
            Another example can be customers' satisfaction over the course of product releases.

            In regards to student experiential data, this plot demonstrates W-Curve theory. This postulates
            that students' campus experience begins positively, then follows a pattern of dipping and rising
            over the subsequent semesters.
        @image html WCurve.svg width=90%
        @par %Data:
            This plot accepts a Data::Dataset where one continuous column (i.e., Y) is the dependent measurement,
            another continuous column (i.e., X) is the time interval,
            and a categorical column is the observation's name or ID. Below is an example where X is @c YEAR,
            Y is @c BELONG, and group is @c NAME.

         | YEAR | BELONG | NAME   |
         | --:  | --:    | :--    |
         | 1    | 6      | Nancy  |
         | 2    | 2      | Nancy  |
         | 3    | 5      | Nancy  |
         | 4    | 5      | Nancy  |
         | 1    | 2      | Tina   |
         | 2    | 2.5    | Tina   |
         | 3    | 3.2    | Tina   |
         | 4    | 5.25   | Tina   |
         | 1    | 5.75   | Sharry |
         | 2    | 1      | Sharry |
         | 3    | 4      | Sharry |
         | 4    | 2      | Sharry |

         ...

         Regarding the X column, the values should start at 1 and usually go up
         to 4 (going up to 10 is supported). This represents the semester/year/period that the measurement
         was recorded for the observation.

        @par Missing Data:
            Refer to LinePlot for how missing data is handled.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object
         // that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto BelongingData = std::make_shared<Data::Dataset>();
         BelongingData->ImportCSV(L"/home/rdoyle/data/Sense of Belonging.csv",
             ImportInfo().
             // Note that the order of the continuous columns is important.
             // The first one will be the Y data, the second the X data.
             ContinuousColumns({ L"Belong", L"Year" }).
             CategoricalColumns({ { L"Name", CategoricalImportMethod::ReadAsStrings } }));

         // create the plot
         auto WCurve = std::make_shared<WCurvePlot>(canvas);
         // add padding around the plot
         WCurve->SetCanvasMargins(5, 5, 5, 5);

         // set the data and use the grouping column from the dataset to create separate lines
         // for the students
         WCurve->SetData(BelongingData);

         // add a title
         WCurve->GetTopXAxis().GetTitle().SetText(
            _(L"THE TRANSITION OF FOUR STUDENTS USING THE W-CURVE"));
         WCurve->GetTopXAxis().GetTitle().SetBottomPadding(5);

         // add a story-telling note at the bottom corner
         auto storyNote = std::make_shared<Label>(
             GraphItemInfo(_(L"Frank reported that he experienced a"
                " \u201Cdownward spiral\u201D during his first year on campus.")).
             Anchoring(Anchoring::BottomLeftCorner).
             FontBackgroundColor(ColorBrewer::GetColor(Color::Canary)).
             LabelAlignment(TextAlignment::RaggedRight).
             LabelStyling(LabelStyle::DottedLinedPaper).Padding(4, 4, 4, 4));
         storyNote->GetFont().MakeSmaller();
         storyNote->SplitTextToFitLength(25);

         WCurve->AddAnnotation(storyNote,
                               wxPoint(1, WCurve->GetLeftYAxis().GetRange().first));

         // add the plot and its legend to the canvas
         canvas->SetFixedObject(0, 0, WCurve);
         canvas->SetFixedObject(0, 1,
            WCurve->CreateLegend(
                LegendOptions().
                    IncludeHeader(false).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph)) );
        @endcode
        @par Citation:
            This graphic is adapted from the article "Are We Listening? Using Student Stories as a
            Framework for Persistence" by Monica C. Grau and MaryAnn Swain.
    */
    class WCurvePlot final : public LinePlot
        {
    public:
        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param colors The color scheme to apply to the points.
                Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as null to not show points.
                Set to a new shape scheme filled with IconShape::BlankIcon to not show
                markers for certain lines/groups.
            @param linePenStyles The line styles to use for the lines.
                The default is a mixed series of pen styles and arrow lines.
                Set to a new line scheme filled with @c wxPenStyle::wxTRANSPARENT
                to not show any lines.*/
        explicit WCurvePlot(Canvas* canvas,
            std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<Icons::Schemes::IconScheme > shapes = nullptr,
            std::shared_ptr<LineStyleScheme> linePenStyles = nullptr);
        /** @brief Sets the data.
            @details Along with the X and Y points, separate lines will be created based on
                the grouping column in the data. The group ID assigned to each line will also
                select which color, marker shape, and line style to use.
            @param data The data to use for the plot.
            @param yColumnName The Y column data, which represents the sentiment values.
            @param xColumnName The X column data, which represents the time interval value
                (e.g., which semester the score was recorded).
            @param groupColumnName The grouping column to use.
                This is required and cannot be @c std::nullopt.
            @note To add missing points to the data so that a gap in the line will appear,
                set the point in question to NaN (@c std::numeric_limits<double>::quiet_NaN()).
            @warning The data points are drawn in the order that they appear in the dataset.
                The plot will make no effort to sort the data or ensure that it is.
                This is by design in case you need a line series to go backwards in certain spots
                (e.g., a downward spiral).
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
            const wxString& yColumnName,
            const wxString& xColumnName,
            std::optional<const wxString> groupColumnName) final;
        /// @brief Sets the label for the major time intervals used in the data collection
        ///     (e.g., "semester" or "year").
        ///     This is drawn on the top axis labels.
        /// @param label The time interval label.
        void SetTimeIntervalLabel(const wxString& label)
            {
            m_timeLabel = label;
            ResetTimeLabels();
            }
    private:
        void ResetTimeLabels();
        [[nodiscard]]
        wxString FormatTimeLabel(const uint8_t step) const;

        wxString m_timeLabel{ _(L"year") };
        };
    }

/** @}*/

#endif //__WISTERIA_W_CURVE_PLOT_H__
