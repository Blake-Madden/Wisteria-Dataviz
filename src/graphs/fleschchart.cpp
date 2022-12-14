﻿///////////////////////////////////////////////////////////////////////////////
// Name:        fleschchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "fleschchart.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    FleschChart::FleschChart(Wisteria::Canvas* canvas,
            std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors /*= nullptr*/,
            std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes /*= nullptr*/) :
        GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
            Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
            std::make_shared<Wisteria::Icons::Schemes::IconScheme>(
                Wisteria::Icons::Schemes::StandardShapes()));

        GetCanvas()->SetLabel(_(L"Flesch Readability Chart"));
        GetCanvas()->SetName(_(L"Flesch Readability Chart"));
        GetTitle() = GraphItems::Label(GraphItemInfo(_(L"How Easy?")).Pen(wxNullPen));

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
            Axis sentenceRuler(Wisteria::AxisType::LeftYAxis);
            sentenceRuler.SetCustomXPosition(0.5f);
            sentenceRuler.SetCustomYPosition(50);
            sentenceRuler.SetRange(5, 40, 0, 5, 1);
            sentenceRuler.SetTickMarkDisplay(Axis::TickMark::DisplayType::Crossed);
            sentenceRuler.SetTickMarkInterval(1);
            sentenceRuler.SetDoubleSidedAxisLabels(true);
            sentenceRuler.ReverseScale(true);
            sentenceRuler.GetHeader().SetText(_(L"WORDS PER\nSENTENCE"));
            sentenceRuler.GetHeader().GetPen() = wxNullPen;
            sentenceRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            sentenceRuler.SetId(100);
            AddCustomAxis(sentenceRuler);
            }

        // readability score ruler
            {
            Axis scoreRuler(Wisteria::AxisType::LeftYAxis);
            scoreRuler.SetCustomXPosition(2);
            scoreRuler.SetCustomYPosition(75);
            scoreRuler.SetRange(0, 100, 0, 5, 1);
            scoreRuler.SetTickMarkDisplay(Axis::TickMark::DisplayType::Crossed);
            scoreRuler.SetTickMarkInterval(1);
            scoreRuler.SetDoubleSidedAxisLabels(true);
            scoreRuler.AddBracket(Axis::AxisBracket(100, 90, 95, _(L"Very Easy"),
                ColorBrewer::GetColor(Color::BondiBlue)));
            scoreRuler.AddBracket(Axis::AxisBracket(89, 80, 85, _(L"Easy"),
                ColorBrewer::GetColor(Color::BondiBlue)));
            scoreRuler.AddBracket(Axis::AxisBracket(79, 70, 75, _(L"Fairly Easy"),
                ColorBrewer::GetColor(Color::BondiBlue)));
            scoreRuler.AddBracket(Axis::AxisBracket(69, 60, 65, _(L"Standard"),
                ColorBrewer::GetColor(Color::Black)));
            scoreRuler.AddBracket(Axis::AxisBracket(59, 50, 55, _(L"Fairly Difficult"),
                ColorBrewer::GetColor(Color::RedTomato)));
            scoreRuler.AddBracket(Axis::AxisBracket(49, 30, 40, _(L"Difficult"),
                ColorBrewer::GetColor(Color::RedTomato)));
            scoreRuler.AddBracket(Axis::AxisBracket(29, 0, 15, _(L"Very Difficult"),
                ColorBrewer::GetColor(Color::RedTomato)));
            for (auto& bracket : scoreRuler.GetBrackets())
                { bracket.GetLabel().SetFontColor(ColorBrewer::GetColor(Color::Black)); }
            scoreRuler.GetHeader().SetText(_(L"READABILITY\nSCORE"));
            scoreRuler.GetHeader().GetPen() = wxNullPen;
            scoreRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            scoreRuler.SetId(101);
            AddCustomAxis(scoreRuler);
            }

        // syllables per 100 words ruler
            {
            Axis syllableRuler(Wisteria::AxisType::LeftYAxis);
            syllableRuler.SetCustomXPosition(3.5f);
            syllableRuler.SetCustomYPosition(100);
            syllableRuler.SetRange(120, 200, 0, 5, 1);
            syllableRuler.SetTickMarkDisplay(Axis::TickMark::DisplayType::Crossed);
            syllableRuler.SetTickMarkInterval(1);
            syllableRuler.SetDoubleSidedAxisLabels(true);
            syllableRuler.ReverseScale(true);
            syllableRuler.GetHeader().SetText(_(L"SYLLABLES PER\n100 WORDS"));
            syllableRuler.GetHeader().GetPen() = wxNullPen;
            syllableRuler.GetHeader().SetTextAlignment(TextAlignment::Centered);
            syllableRuler.SetId(102);
            AddCustomAxis(syllableRuler);
            }
        }

    //----------------------------------------------------------------
    void FleschChart::SetData(std::shared_ptr<const Wisteria::Data::Dataset> data,
        const wxString& wordsPerSentenceColumnName,
        const wxString& scoreColumnName,
        const wxString& syllablesPerWordColumnName,
        std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        m_wordsPerSentenceColumn = m_scoresColumn = m_syllablesPerWordColumn = nullptr;
        m_jitterWords.ResetJitterData();
        m_jitterScores.ResetJitterData();
        m_jitterSyllables.ResetJitterData();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            { return; }

        SetGroupColumn(groupColumnName);

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            { BuildGroupIdMap(); }

        m_wordsPerSentenceColumn =
            GetContinuousColumnRequired(wordsPerSentenceColumnName);
        m_scoresColumn =
            GetContinuousColumnRequired(scoreColumnName);
        m_syllablesPerWordColumn =
            GetContinuousColumnRequired(syllablesPerWordColumnName);

        // words
            {
            frequency_set<double> jitterPoints;
            for (const auto& wordsPerSentence : m_wordsPerSentenceColumn->GetValues())
                {
                if (std::isnan(wordsPerSentence))
                    { continue; }

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
                    { continue; }

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
                    { continue; }

                jitterPoints.insert(std::clamp<double>(syllablesPerWord*100, 120, 200));
                }
            m_jitterSyllables.CalcSpread(jitterPoints);
            }
        }

    //----------------------------------------------------------------
    void FleschChart::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        // axis headers manage their own font, so update these to be the main axis font
        for (auto& customAxis : GetCustomAxes())
            {
            customAxis.GetHeader().GetFont() = GetLeftYAxis().GetFont();
            customAxis.GetHeader().SetFontColor(GetLeftYAxis().GetFontColor());
            }

        // add instruction label
        auto legend = std::make_shared<GraphItems::Label>(
                GraphItemInfo(_(L"HOW TO USE THIS CHART\n"
                    "       Take a pencil or ruler and connect your\n"
                    "“Words per Sentence” figure (left) with your\n"
                    "“Syllables per 100 Words” figure (right).  The\n"
                    "intersection of the pencil or ruler with the\n"
                    "center line shows your “Reading Ease” score.")).
                Pen(*wxBLACK_PEN).FontBackgroundColor(*wxWHITE).
                Scaling(GetScaling()).
                Font(wxFont(GetBottomXAxis().GetFont()).MakeSmaller()).
                LabelAlignment(TextAlignment::JustifiedAtWord).Padding(5, 5, 5, 5).
                AnchorPoint(wxPoint(GetPlotAreaBoundingBox().GetX() + ScaleToScreenAndCanvas(10),
                                    GetPlotAreaBoundingBox().GetY())) );
        legend->GetHeaderInfo().Enable(true).
            LabelAlignment(TextAlignment::Centered).
            GetFont().MakeBold().MakeSmaller();
        legend->SetBoxCorners(BoxCorners::Straight);
        legend->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
        AddObject(legend);

        if (GetDataset() == nullptr)
            { return; }

        // plot the points
        const auto wordsRuler{ GetCustomAxes()[0] };
        m_jitterWords.SetJitterWidth(wordsRuler.CalcTickMarkOuterWidth()*2);

        const auto middleRuler{ GetCustomAxes()[1] };
        m_jitterScores.SetJitterWidth(middleRuler.CalcTickMarkOuterWidth()*2);

        const auto syllablesRuler{ GetCustomAxes()[2] };
        m_jitterSyllables.SetJitterWidth(syllablesRuler.CalcTickMarkOuterWidth()*2);

        auto points = std::make_shared<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount() * 3); // point for each ruler
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_wordsPerSentenceColumn->GetValue(i)) ||
                std::isnan(m_scoresColumn->GetValue(i)) ||
                std::isnan(m_syllablesPerWordColumn->GetValue(i)))
                { continue; }

            const auto wordsPerSentence =
                std::clamp<double>(m_wordsPerSentenceColumn->GetValue(i), 5, 40);
            const auto score =
                std::clamp<size_t>(m_scoresColumn->GetValue(i), 0, 100);
            const auto syllablesPerWord =
                std::clamp<double>(m_syllablesPerWordColumn->GetValue(i)*100, 120, 200);

            wxCoord coord1{ 0 }, coord2{ 0 }, coord3{ 0 };
            wxASSERT_LEVEL_2(wordsRuler.GetPhysicalCoordinate(wordsPerSentence, coord1));
            wxASSERT_LEVEL_2(middleRuler.GetPhysicalCoordinate(score, coord2));
            wxASSERT_LEVEL_2(syllablesRuler.GetPhysicalCoordinate(syllablesPerWord, coord3));
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
                    const auto linePen =
                        wxPen(wxColour(0, 0, 255,
                            (GetDataset()->GetRowCount() > 10) ? 100 : wxALPHA_OPAQUE));
                    AddObject(std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo().Pen(linePen).Brush(*wxBLUE_BRUSH).
                        Scaling(GetScaling()),
                        linePts, 2));
                    AddObject(std::make_shared<GraphItems::Polygon>(
                        GraphItemInfo().Pen(linePen).Brush(*wxBLUE_BRUSH).
                        Scaling(GetScaling()),
                        linePts+2, 2));
                    }

                // Convert group ID into color scheme index
                // (index is ordered by labels alphabetically).
                // Note that this will be zero if grouping is not in use.
                const size_t colorIndex = IsUsingGrouping() ?
                    GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) :
                    0;

                // points on the rulers
                points->AddPoint(Point2D(
                    GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i)).
                    AnchorPoint(pt1).Scaling(GetScaling()).
                    Brush(GetColorScheme()->GetColor(colorIndex)),
                    Settings::GetPointRadius(),
                    GetShapeScheme()->GetShape(colorIndex)), dc);
                points->AddPoint(Point2D(
                    GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i)).
                    AnchorPoint(pt2).Scaling(GetScaling()).
                    Brush(GetColorScheme()->GetColor(colorIndex)),
                    Settings::GetPointRadius(),
                    GetShapeScheme()->GetShape(colorIndex)), dc);
                points->AddPoint(Point2D(
                    GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i)).
                    AnchorPoint(pt3).Scaling(GetScaling()).
                    Brush(GetColorScheme()->GetColor(colorIndex)),
                    Settings::GetPointRadius(),
                    GetShapeScheme()->GetShape(colorIndex)), dc);
                }
            }
        AddObject(points);
        }
    }