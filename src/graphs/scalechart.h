/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_SCALE_CHART_H
#define WISTERIA_SCALE_CHART_H

#include "../data/jitter.h"
#include "barchart.h"

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A chart that shows a numeric scale to the left, point values to the right
            of that, and an optional series of other scales.
        @details These other scales consist of stackable blocks that can be colorful and
            even have brush, stipple, or image patterns drawn on them.
            (These blocks are essentially bar chart blocks and inherit all the functionality
             from there.)
        @image html ScaleChart.svg width=90%

        @par %Data:
         This plot accepts a Data::Dataset where one continuous column contains the value(s).
         The ID column's labels will be associated with each point,
         so it is recommended to fill this column with meaningful names.

         A categorical column can also optionally be used as a grouping variable.

         | ID     | Score | Group |
         | :--    | --:   | --:   |
         | week 1 | 52    | Anna  |
         | week 2 | 50    | Anna  |
         | week 1 | 62    | Joe   |
         ...

        @par Missing Data:
         - Values that are missing data will not be plotted.
         - Blank IDs will apply blank selection labels to their respective points.
         - Blank group labels will be lumped into a "[NO GROUP]" category.

         @par Example:
         @code
          // "this" will be a parent wxWidgets frame or dialog, "canvas"
          // is a scrolled window derived object that will hold the box plot
          auto canvas = new Wisteria::Canvas(this);
          canvas->SetFixedObjectsGridSize(1, 1);
          auto testScoresData = std::make_shared<Data::Dataset>();
          try
            {
            testScoresData->ImportCSV(
                appDir + L"/datasets/Student Scores.csv",
                ImportInfo()
                    .ContinuousColumns({ L"test_score" })
                    .IdColumn(L"Week")
                    .CategoricalColumns({ { L"NAME", CategoricalImportMethod::ReadAsStrings } }));
            }
          catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                            wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }

          auto plot = std::make_shared<ScaleChart>(subframe->m_canvas);
          plot->AddScale(
            std::vector<BarChart::BarBlock>{
                BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(59)
                                        .Brush(Colors::ColorBrewer::GetColor
                                                (Colors::Color::PastelRed, 150))
                                        .Decal(
                        GraphItems::Label(GraphItems::GraphItemInfo{ L"F (fail)" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(Colors::ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(
                        GraphItems::Label(GraphItems::GraphItemInfo{ L"D" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(
                        GraphItems::Label(GraphItems::GraphItemInfo{ L"C" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(10)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(
                        GraphItems::Label(GraphItems::GraphItemInfo{ L"A" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) } },
            std::nullopt,
            _(L"Grades"));
          plot->AddScale(
            std::vector<BarChart::BarBlock>{
                BarChart::BarBlock{ Wisteria::Graphs::BarChart::BarBlockInfo(59)
                                        .Brush(ColorBrewer::GetColor(Colors::Color::PastelRed, 150))
                                        .Decal(GraphItems::Label(
                                            GraphItems::GraphItemInfo{ L"F (fail)" }.LabelFitting(
                                                LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D-" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Corn, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"D+" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C-" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::EvergreenFog, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"C+" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B-" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::FernGreen, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"B+" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A-" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(4)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
                BarChart::BarBlock{
                    Wisteria::Graphs::BarChart::BarBlockInfo(3)
                        .Brush(ColorBrewer::GetColor(Colors::Color::Emerald, 150))
                        .Decal(GraphItems::Label(GraphItems::GraphItemInfo{ L"A+" }.LabelFitting(
                            LabelFit::ScaleFontToFit))) },
            },
            std::nullopt,
            _(L"Grades"));
          plot->SetMainScaleValues({ 10, 20, 30, 40, 50, 60, 70, 80, 90 }, 0);
          plot->SetMainScaleColumnHeader(_(L"Grade Level"));
          plot->SetData(testScoresData, L"TEST_SCORE", L"NAME");
          plot->SetDataColumnHeader(_(L"Test Scores"));

          canvas->SetFixedObject(0, 0, plot);
         @endcode*/
    // clang-format on

    class ScaleChart : public Wisteria::Graphs::BarChart
        {
        wxDECLARE_DYNAMIC_CLASS(ScaleChart);
        ScaleChart() = default;

      public:
        /** @brief Constructor.
            @param canvas The parent canvas to render on.
            @param colors The color scheme to apply to the points.
                Leave as @c nullptr to use the default theme.
            @param shapes The shape scheme to use for the points.
                Leave as @c nullptr to use the standard shapes.
         */
        explicit ScaleChart(
            Wisteria::Canvas* canvas,
            const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr);

        /** @brief Displays a series of numbers along the left side,
                positioned at their given value.
            @param values The series of values to show.
            @param precision The precision to display the numbers with.
            @note The default scaling axis is 0-100 (but can be changed by calling
                `GetScalingAxis().SetRange()`). Values provided that don't fall
                within this range will be ignored.
            @sa SetMainScaleColumnHeader().
         */
        void SetMainScaleValues(const std::vector<double>& values, uint8_t precision);

        /** @brief Sets the data.
            @param data The data to use.
            @param scoreColumnName The column containing the documents' scores
                (a continuous column).
            @param groupColumnName The (optional) categorical column to use for grouping.
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @sa SetDataColumnHeader().
            @throws std::runtime_error If any columns can't be found, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.
         */
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& scoreColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /** @brief Adds a color-block scale along the right side of the scores
                and numeric scale.
            @param blocks A series of stackable block to represent a scale.
                Along with color, this should contain a decal label.
            @param scalingAxisStart Where along the scaling axis the bar should start.\n
                This is optional, and the default will be to start where the axis starts.
            @param header An optional label to display above the scale.
            @warning The default scaling axis is 0-100. If you intend to change that,
                then do that by calling `GetScalingAxis().SetRange()` before any calls
                to this function.\n
                Also, all scales should be set up before calling SetData().
         */
        void AddScale(const std::vector<BarChart::BarBlock>& blocks,
                      std::optional<double> scalingAxisStart = std::nullopt,
                      const wxString& header = wxString{});

        /** @brief Sets the header over the main scale column.
            @param header The label to use.
            @sa SetMainScaleValues().
         */
        void SetMainScaleColumnHeader(const wxString& header)
            {
            if (!header.empty() && !GetBars().empty())
                {
                GetOppositeBarAxis().SetCustomLabel(GetBars()[0].GetAxisPosition(),
                                                    GraphItems::Label{ header });
                }
            }

        /** @brief Sets the header over the scores (plotted from SetData()).
            @param header The label to use.
            @sa SetData().
         */
        void SetDataColumnHeader(const wxString& header)
            {
            if (!header.empty() && GetBars().size() > 1)
                {
                GetOppositeBarAxis().SetCustomLabel(GetBars()[1].GetAxisPosition(),
                                                    GraphItems::Label{ header });
                }
            }

        /// @returns Whether the score is being showcased.
        /// @sa ShowcaseScore().
        [[nodiscard]]
        bool IsShowcasingScore() const noexcept
            {
            return m_showcaseScore;
            }

        /// @brief Makes most areas of the graph translucent, except for where the score is.
        ///     This helps draw attention to the areas of the scales that have scores falling
        ///     into them.
        /// @param showcase @c true to showcase where the score is.
        /// @note If there are multiple scores, then every area that has a score in it
        ///     will be showcased.
        void ShowcaseScore(const bool showcase) noexcept { m_showcaseScore = showcase; }

      private:
        void RecalcSizes(wxDC& dc) override;

        void AdjustAxes();

        void GhostAllBars();

        wxString m_scoresColumnName;
        Wisteria::Data::Jitter m_jitter{ Wisteria::AxisType::LeftYAxis };

        std::vector<double> m_scaleValues;
        uint8_t m_precision{ 1 };
        bool m_showcaseScore{ false };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_SCALE_CHART_H
