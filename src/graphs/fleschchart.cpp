///////////////////////////////////////////////////////////////////////////////
// Name:        fleschchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "fleschchart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::FleschChart, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    FleschChart::FleschChart(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"Flesch Readability Chart"));
            GetCanvas()->SetName(_(L"Flesch Readability Chart"));
            }
        GetTitle() = GraphItems::Label(GraphItems::GraphItemInfo(_(L"How Easy?")).Pen(wxNullPen));

        // Set up the X axis
        GetBottomXAxis().SetRange(0, 4, 0, 1, 1);
        GetBottomXAxis().ShowOuterLabels(false);
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);

        // set up the Y axis
        GetLeftYAxis().SetRange(0, 110, 0, 10, 1);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);

            // words per sentence ruler
            {
            GraphItems::Axis sentenceRuler(Wisteria::AxisType::LeftYAxis);
            sentenceRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            sentenceRuler.SetCustomXPosition(0.5F);
            sentenceRuler.SetCustomYPosition(50);
            sentenceRuler.SetRange(5, 40, 0, 5, 1);
            sentenceRuler.SetTickMarkDisplay(GraphItems::Axis::TickMark::DisplayType::Crossed);
            sentenceRuler.SetTickMarkInterval(1);
            sentenceRuler.SetDoubleSidedAxisLabels(true);
            sentenceRuler.Reverse();
            sentenceRuler.GetHeader().SetText(_(L"WORDS PER\nSENTENCE"));
            sentenceRuler.GetHeader().GetPen() = wxNullPen;
            sentenceRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            sentenceRuler.SetId(100);
            AddCustomAxis(sentenceRuler);
            }

            // readability score ruler
            {
            GraphItems::Axis scoreRuler(Wisteria::AxisType::LeftYAxis);
            scoreRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            scoreRuler.SetCustomXPosition(2);
            scoreRuler.SetCustomYPosition(75);
            scoreRuler.SetRange(0, 100, 0, 5, 1);
            scoreRuler.SetTickMarkDisplay(GraphItems::Axis::TickMark::DisplayType::Crossed);
            scoreRuler.SetTickMarkInterval(1);
            scoreRuler.SetDoubleSidedAxisLabels(true);
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                100, 90, 95, _(L"Very Easy"),
                Colors::ColorBrewer::GetColor(Colors::Color::BondiBlue)));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                89, 80, 85, _(L"Easy"), Colors::ColorBrewer::GetColor(Colors::Color::BondiBlue)));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                79, 70, 75, _(L"Fairly Easy"),
                Colors::ColorBrewer::GetColor(Colors::Color::BondiBlue)));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                69, 60, 65, _(L"Standard"),
                Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor())));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                59, 50, 55, _(L"Fairly Difficult"),
                Colors::ColorBrewer::GetColor(Colors::Color::RedTomato)));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                49, 30, 40, _(L"Difficult"),
                Colors::ColorBrewer::GetColor(Colors::Color::RedTomato)));
            scoreRuler.AddBracket(GraphItems::Axis::AxisBracket(
                29, 0, 15, _(L"Very Difficult"),
                Colors::ColorBrewer::GetColor(Colors::Color::RedTomato)));
            for (auto& bracket : scoreRuler.GetBrackets())
                {
                bracket.GetLabel().SetFontColor(GetLeftYAxis().GetFontColor());
                }
            scoreRuler.GetHeader().SetText(_(L"READABILITY\nSCORE"));
            scoreRuler.GetHeader().GetPen() = wxNullPen;
            scoreRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            scoreRuler.SetId(101);
            AddCustomAxis(scoreRuler);
            }

            // syllables per 100 words ruler
            {
            GraphItems::Axis syllableRuler(Wisteria::AxisType::RightYAxis);
            syllableRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            syllableRuler.SetCustomXPosition(3.5F);
            syllableRuler.SetCustomYPosition(100);
            syllableRuler.SetRange(120, 200, 0, 5, 1);
            syllableRuler.SetTickMarkDisplay(GraphItems::Axis::TickMark::DisplayType::Crossed);
            syllableRuler.SetTickMarkInterval(1);
            syllableRuler.SetDoubleSidedAxisLabels(true);
            syllableRuler.Reverse();
            syllableRuler.GetHeader().SetText(_(L"SYLLABLES PER\n100 WORDS"));
            syllableRuler.GetHeader().GetPen() = wxNullPen;
            syllableRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            syllableRuler.SetId(102);
            AddCustomAxis(syllableRuler);
            }
        }

    //----------------------------------------------------------------
    void FleschChart::SetData(const std::shared_ptr<const Wisteria::Data::Dataset>& data,
                              const wxString& wordsPerSentenceColumnName,
                              const wxString& scoreColumnName,
                              const wxString& syllablesPerWordColumnName,
                              const std::optional<wxString>& groupColumnName /*= std::nullopt*/,
                              bool includeSyllableRulerDocumentGroups /*= false*/)
        {
        SetDataset(data);
        ResetGrouping();
        m_wordsPerSentenceColumn = m_scoresColumn = m_syllablesPerWordColumn = nullptr;
        m_jitterWords.ResetJitterData();
        m_jitterScores.ResetJitterData();
        m_jitterSyllables.ResetJitterData();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            }

        m_wordsPerSentenceColumn = GetContinuousColumnRequired(wordsPerSentenceColumnName);
        m_scoresColumn = GetContinuousColumnRequired(scoreColumnName);
        m_syllablesPerWordColumn = GetContinuousColumnRequired(syllablesPerWordColumnName);

            // words
            {
            frequency_set<double> jitterPoints;
            for (const auto& wordsPerSentence : m_wordsPerSentenceColumn->GetValues())
                {
                if (std::isnan(wordsPerSentence))
                    {
                    continue;
                    }

                jitterPoints.insert(std::clamp<double>(wordsPerSentence, 5, 40));
                }
            m_jitterWords.CalcSpread(jitterPoints);
            }
            // scores
            {
            frequency_set<double> jitterPoints;
            for (const auto& score : m_scoresColumn->GetValues())
                {
                if (std::isnan(score))
                    {
                    continue;
                    }

                jitterPoints.insert(std::clamp<size_t>(score, 0, 100));
                }
            m_jitterScores.CalcSpread(jitterPoints);
            }
            // syllables
            {
            frequency_set<double> jitterPoints;
            for (const auto& syllablesPerWord : m_syllablesPerWordColumn->GetValues())
                {
                if (std::isnan(syllablesPerWord))
                    {
                    continue;
                    }

                jitterPoints.insert(std::clamp<double>(syllablesPerWord * 100, 120, 200));
                }
            m_jitterSyllables.CalcSpread(jitterPoints);

            auto& syllableRuler{ GetCustomAxes()[2] };
            syllableRuler.ClearBrackets();
            if (includeSyllableRulerDocumentGroups && data->HasValidIdData() && // needed for labels
                data->GetRowCount() > 1 && data->GetRowCount() <= 50)
                {
                struct rulerBucket
                    {
                    double m_start{ 0 };
                    double m_end{ 0 };
                    wxString m_label;
                    };

                rulerBucket bucket1{ 120.0, 139.0, wxString{} };
                rulerBucket bucket2{ 140.0, 159.0, wxString{} };
                rulerBucket bucket3{ 160.0, 179.0, wxString{} };
                rulerBucket bucket4{ 180.0, 200.0, wxString{} };

                for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                    {
                    if (std::isnan(m_syllablesPerWordColumn->GetValue(i)))
                        {
                        continue;
                        }
                    const auto currentValue{ std::clamp<size_t>(
                        m_syllablesPerWordColumn->GetValue(i) * 100, 120, 200) };

                    if (currentValue >= bucket1.m_start && currentValue <= bucket1.m_end)
                        {
                        bucket1.m_label.append(GetDataset()->GetIdColumn().GetValue(i))
                            .append(L'\n');
                        }
                    else if (currentValue >= bucket2.m_start && currentValue <= bucket2.m_end)
                        {
                        bucket2.m_label.append(GetDataset()->GetIdColumn().GetValue(i))
                            .append(L'\n');
                        }
                    else if (currentValue >= bucket3.m_start && currentValue <= bucket3.m_end)
                        {
                        bucket3.m_label.append(GetDataset()->GetIdColumn().GetValue(i))
                            .append(L'\n');
                        }
                    else if (currentValue >= bucket4.m_start && currentValue <= bucket4.m_end)
                        {
                        bucket4.m_label.append(GetDataset()->GetIdColumn().GetValue(i))
                            .append(L'\n');
                        }
                    }
                bucket1.m_label.Trim();
                bucket2.m_label.Trim();
                bucket3.m_label.Trim();
                bucket4.m_label.Trim();

                syllableRuler.MirrorBracketsWhenDoubleSided(false);
                syllableRuler.AddBracket(GraphItems::Axis::AxisBracket(
                    bucket1.m_start, bucket1.m_end,
                    bucket1.m_start + ((bucket1.m_end - bucket1.m_start) * math_constants::half),
                    bucket1.m_label));
                syllableRuler.GetBrackets().back().GetLabel().SetLineSpacing(0);
                syllableRuler.GetBrackets().back().GetLabel().SetRightPadding(5);
                syllableRuler.AddBracket(GraphItems::Axis::AxisBracket(
                    bucket2.m_start, bucket2.m_end,
                    bucket2.m_start + ((bucket2.m_end - bucket2.m_start) * math_constants::half),
                    bucket2.m_label));
                syllableRuler.GetBrackets().back().GetLabel().SetLineSpacing(0);
                syllableRuler.GetBrackets().back().GetLabel().SetRightPadding(5);
                syllableRuler.AddBracket(GraphItems::Axis::AxisBracket(
                    bucket3.m_start, bucket3.m_end,
                    bucket3.m_start + ((bucket3.m_end - bucket3.m_start) * math_constants::half),
                    bucket3.m_label));
                syllableRuler.GetBrackets().back().GetLabel().SetLineSpacing(0);
                syllableRuler.GetBrackets().back().GetLabel().SetRightPadding(5);
                syllableRuler.AddBracket(GraphItems::Axis::AxisBracket(
                    bucket4.m_start, bucket4.m_end,
                    bucket4.m_start + ((bucket4.m_end - bucket4.m_start) * math_constants::half),
                    bucket4.m_label));
                syllableRuler.GetBrackets().back().GetLabel().SetLineSpacing(0);
                syllableRuler.GetBrackets().back().GetLabel().SetRightPadding(5);
                }
            }
        }

    //----------------------------------------------------------------
    void FleschChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        // axis headers manage their own font, so update these to be the main axis font
        for (auto& customAxis : GetCustomAxes())
            {
            customAxis.GetHeader().GetFont() = GetLeftYAxis().GetFont();
            customAxis.GetHeader().SetFontColor(GetLeftYAxis().GetFontColor());
            }

        const auto legendColor =
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor());
        const auto legendBkColor =
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(legendColor);

        // add instruction label
        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo(
                _(L"HOW TO USE THIS CHART\n"
                  "       Take a pencil or ruler and connect your\n"
                  "\U0000201CWords per Sentence\U0000201D figure (left) with your\n"
                  "\U0000201CSyllables per 100 Words\U0000201D figure (right).  The\n"
                  "intersection of the pencil or ruler with the\n"
                  "center line shows your \U0000201CReading Ease\U0000201D score."))
                .Pen(legendColor)
                .FontBackgroundColor(legendBkColor)
                .FontColor(legendColor)
                .Scaling(GetScaling())
                .Font(wxFont(GetBottomXAxis().GetFont()).MakeSmaller())
                .LabelAlignment(TextAlignment::JustifiedAtWord)
                .Padding(5, 5, 5, 5)
                .AnchorPoint(wxPoint(GetPlotAreaBoundingBox().GetX() + ScaleToScreenAndCanvas(10),
                                     GetPlotAreaBoundingBox().GetY())));
        legend->GetHeaderInfo()
            .Enable(true)
            .LabelAlignment(TextAlignment::Centered)
            .FontColor(legendColor)
            .GetFont()
            .MakeBold()
            .MakeSmaller();
        legend->SetBoxCorners(BoxCorners::Straight);
        legend->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        AddObject(std::move(legend));

        // make "Standard" bracket white or black
        if (GetCustomAxes().size() >= 1 && GetCustomAxes().at(1).GetBrackets().size() >= 3)
            {
            GetCustomAxes().at(1).GetBrackets().at(3).GetLinePen().SetColour(
                Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
            }

        if (GetDataset() == nullptr)
            {
            return;
            }

        // plot the points
        const auto& wordsRuler{ GetCustomAxes()[0] };
        m_jitterWords.SetJitterWidth(wordsRuler.CalcTickMarkOuterWidth() * 2);

        const auto& middleRuler{ GetCustomAxes()[1] };
        m_jitterScores.SetJitterWidth(middleRuler.CalcTickMarkOuterWidth() * 2);

        const auto& syllablesRuler{ GetCustomAxes()[2] };
        m_jitterSyllables.SetJitterWidth(syllablesRuler.CalcTickMarkOuterWidth() * 2);

        auto points = std::make_unique<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount() * 3); // point for each ruler
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_wordsPerSentenceColumn->GetValue(i)) ||
                std::isnan(m_scoresColumn->GetValue(i)) ||
                std::isnan(m_syllablesPerWordColumn->GetValue(i)))
                {
                continue;
                }

            const auto wordsPerSentence =
                std::clamp<size_t>(m_wordsPerSentenceColumn->GetValue(i), 5, 40);
            const auto score = std::clamp<size_t>(m_scoresColumn->GetValue(i), 0, 100);
            const auto syllablesPerWord =
                std::clamp<size_t>(m_syllablesPerWordColumn->GetValue(i) * 100, 120, 200);

            wxCoord coord1{ 0 }, coord2{ 0 }, coord3{ 0 };
            assert(wordsRuler.GetPhysicalCoordinate(wordsPerSentence, coord1));
            assert(middleRuler.GetPhysicalCoordinate(score, coord2));
            assert(syllablesRuler.GetPhysicalCoordinate(syllablesPerWord, coord3));
            if (wordsRuler.GetPhysicalCoordinate(wordsPerSentence, coord1) &&
                middleRuler.GetPhysicalCoordinate(score, coord2) &&
                syllablesRuler.GetPhysicalCoordinate(syllablesPerWord, coord3))
                {
                wxPoint pt1(wordsRuler.GetPhysicalCustomXPosition(), coord1);
                m_jitterWords.JitterPoint(pt1);
                wxPoint pt2(middleRuler.GetPhysicalCustomXPosition(), coord2);
                m_jitterScores.JitterPoint(pt2);
                wxPoint pt3(syllablesRuler.GetPhysicalCustomXPosition(), coord3);
                m_jitterSyllables.JitterPoint(pt3);
                // connection line
                if (IsShowingConnectionLine())
                    {
                    const wxPoint linePts[4] = { pt1, pt2, pt2, pt3 };
                    wxColour lineColor{ Colors::ColorContrast::ShadeOrTintIfClose(
                        Wisteria::Colors::ColorBrewer::GetColor(Wisteria::Colors::Color::BondiBlue),
                        GetPlotOrCanvasColor()) };
                    if (GetDataset()->GetRowCount() > 10)
                        {
                        lineColor = Wisteria::Colors::ColorContrast::ChangeOpacity(lineColor, 100);
                        }
                    const wxPen linePen{ lineColor };
                    AddObject(std::make_unique<GraphItems::Polygon>(
                        GraphItems::GraphItemInfo().Pen(linePen).Scaling(GetScaling()), linePts,
                        2));
                    AddObject(std::make_unique<GraphItems::Polygon>(
                        GraphItems::GraphItemInfo().Pen(linePen).Scaling(GetScaling()), linePts + 2,
                        2));
                    }

                // Convert group ID into color scheme index
                // (index is ordered by labels alphabetically).
                // Note that this will be zero if grouping is not in use.
                const size_t colorIndex =
                    IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) :
                                        0;

                // points on the rulers
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt1)
                            .Pen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                GetPlotOrCanvasColor()))
                            .Scaling(GetScaling())
                            .Brush(GetColorScheme()->GetColor(colorIndex)),
                        Settings::GetPointRadius(), GetShapeScheme()->GetShape(colorIndex)),
                    dc);
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt2)
                            .Pen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                GetPlotOrCanvasColor()))
                            .Scaling(GetScaling())
                            .Brush(GetColorScheme()->GetColor(colorIndex)),
                        Settings::GetPointRadius(), GetShapeScheme()->GetShape(colorIndex)),
                    dc);
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt3)
                            .Pen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                GetPlotOrCanvasColor()))
                            .Scaling(GetScaling())
                            .Brush(GetColorScheme()->GetColor(colorIndex)),
                        Settings::GetPointRadius(), GetShapeScheme()->GetShape(colorIndex)),
                    dc);
                }
            }
        AddObject(std::move(points));
        }
    } // namespace Wisteria::Graphs
