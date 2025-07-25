///////////////////////////////////////////////////////////////////////////////
// Name:        lixgaugegerman.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lixgaugegerman.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LixGaugeGerman, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    LixGaugeGerman::LixGaugeGerman(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"German Lix Gauge"));
            GetCanvas()->SetName(_(L"German Lix Gauge"));
            }

        GetBottomXAxis().SetRange(0, 2, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 100, 0, 5, 1);
        GetLeftYAxis().Reverse();
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void LixGaugeGerman::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                 const wxString& scoreColumnName,
                                 const std::optional<wxString>& groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        m_scoresColumn = nullptr;
        m_jitter.ResetJitterData();
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

        // get the score data
        m_scoresColumn = GetContinuousColumnRequired(scoreColumnName);

        frequency_set<double> jitterPoints;
        for (const auto& datum : m_scoresColumn->GetValues())
            {
            if (std::isnan(datum))
                {
                continue;
                }

            jitterPoints.insert(std::clamp<double>(datum, 0, 100));
            }
        m_jitter.CalcSpread(jitterPoints);
        }

    //----------------------------------------------------------------
    void LixGaugeGerman::AdjustAxes()
        {
        const auto getMinMaxForRange = [this]()
        {
            if (GetDataset() != nullptr)
                {
                const auto [minVal, maxVal] = std::minmax_element(
                    m_scoresColumn->GetValues().cbegin(), m_scoresColumn->GetValues().cend());
                const auto minYAxis = m_scoresColumn->GetValues().size() ?
                                          std::min(20.0, previous_interval(*minVal, 2)) :
                                          20.0;
                const auto maxYAxis = m_scoresColumn->GetValues().size() ?
                                          std::max(70.0, next_interval(*maxVal, 2)) :
                                          70.0;
                return std::make_pair(minYAxis, maxYAxis);
                }
            else
                {
                return std::make_pair(20.0, 70.0);
                }
        };

        const auto [minYAxis, maxYAxis] = getMinMaxForRange();
        GetLeftYAxis().SetRange(minYAxis, maxYAxis, 0, 5, 1);

        // these are managed by the plot (not canvas), so clear them here
        GetCustomAxes().clear();

            {
            GraphItems::Axis leftRuler(AxisType::LeftYAxis);
            leftRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            leftRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            leftRuler.SetCustomXPosition(0.9F);
            leftRuler.SetCustomYPosition(maxYAxis);
            leftRuler.SetRange(minYAxis, maxYAxis, 0, 5, 1);
            leftRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            leftRuler.Reverse();
            leftRuler.SetId(100);
            leftRuler.GetAxisLinePen() = wxNullPen;
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                25, 25, 25,
                IsUsingEnglishLabels() ?
                    wxString{ DONTTRANSLATE(L"very easy text", DTExplanation::DirectQuote,
                                            L"These are the English labels from the article and "
                                            "should always appear as this.") } :
                    wxString{ DONTTRANSLATE(L"Sehr leichter Text", DTExplanation::DirectQuote,
                                            L"Original German labels.") },
                wxColour{ 66, 51, 251 }));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                35, 35, 35,
                IsUsingEnglishLabels() ? wxString{ DONTTRANSLATE(L"easy text") } :
                                         wxString{ DONTTRANSLATE("Leichter Text") },
                wxColour{ 163, 182, 250 }));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                45, 45, 45,
                IsUsingEnglishLabels() ? wxString{ DONTTRANSLATE(L"average text") } :
                                         wxString{ DONTTRANSLATE("Durchschnittlicher Text") },
                wxColour{ 239, 173, 186 }));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                55, 55, 55,
                IsUsingEnglishLabels() ? wxString{ DONTTRANSLATE(L"difficult text") } :
                                         wxString{ DONTTRANSLATE("Schwieriger Text") },
                wxColour{ 237, 27, 37 }));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                65, 65, 65,
                IsUsingEnglishLabels() ? wxString{ DONTTRANSLATE(L"very difficult text") } :
                                         wxString{ DONTTRANSLATE("Sehr schwieriger Text") },
                wxColour{ 250, 0, 0 }));
            for (auto& bracket : leftRuler.GetBrackets())
                {
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                bracket.SetBracketLineStyle(BracketLineStyle::ReverseArrow);
                bracket.SetPerpendicularLabelConnectionLinesAlignment(
                    AxisLabelAlignment::AlignWithBoundary);
                bracket.GetLabel().SetFontColor(GetLeftYAxis().GetFontColor());
                bracket.GetLabel().SetTextAlignment(TextAlignment::FlushLeft);
                }
            AddCustomAxis(leftRuler);
            }

            {
            GraphItems::Axis middleRuler(Wisteria::AxisType::LeftYAxis);
            middleRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            middleRuler.SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::CenterOnAxisLine);
            middleRuler.GetAxisLinePen() = wxNullPen;
            middleRuler.SetOutlineSize(wxSize(15, 5));
            middleRuler.SetCustomXPosition(1);
            middleRuler.SetCustomYPosition(maxYAxis);
            middleRuler.SetRange(minYAxis, maxYAxis, 0, 5, 1);
            middleRuler.Reverse();
            middleRuler.SetId(101);
            AddCustomAxis(middleRuler);
            }

            {
            GraphItems::Axis rightRuler(Wisteria::AxisType::RightYAxis);
            rightRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            rightRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            rightRuler.SetCustomXPosition(1.1F);
            rightRuler.SetCustomYPosition(maxYAxis);
            rightRuler.SetRange(minYAxis, maxYAxis, 0, 5, 1);
            rightRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            rightRuler.Reverse();
            rightRuler.SetId(102);
            rightRuler.GetAxisLinePen() = wxNullPen;
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                30, 30, 30,
                IsUsingEnglishLabels() ?
                    wxString{ DONTTRANSLATE(L"children and youth\nliterature (for ages 8-16)") } :
                    wxString{ DONTTRANSLATE(L"Kinder- und Jugendb\U000000FCcher") },
                wxColour{ 138, 163, 249 }));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                40, 40, 40,
                IsUsingEnglishLabels() ?
                    wxString{ DONTTRANSLATE(L"bellestristic texts (prose\nfiction for adults)") } :
                    wxString(DONTTRANSLATE(L"Belletristik")),
                wxColour{ 207, 217, 252 }));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                50, 50, 50,
                IsUsingEnglishLabels() ?
                    wxString{
                        DONTTRANSLATE(L"informational (non fiction)\ntexts (Sachliteratur)") } :
                    wxString{ DONTTRANSLATE(L"Sachliteratur") },
                wxColour{ 245, 126, 133 }));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                60, 60, 60,
                IsUsingEnglishLabels() ?
                    wxString{ DONTTRANSLATE(L"technical texts\n(Fachliteratur)") } :
                    wxString{ DONTTRANSLATE(L"Fachliteratur") },
                wxColour{ 237, 10, 10 }));
            for (auto& bracket : rightRuler.GetBrackets())
                {
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                bracket.SetBracketLineStyle(BracketLineStyle::ReverseArrow);
                bracket.GetLabel().SetFontColor(GetLeftYAxis().GetFontColor());
                bracket.GetLabel().SetTextAlignment(TextAlignment::FlushRight);
                // English labels are multi-line and pushed over to the far right
                if (IsUsingEnglishLabels())
                    {
                    bracket.SetPerpendicularLabelConnectionLinesAlignment(
                        AxisLabelAlignment::AlignWithBoundary);
                    }
                }
            AddCustomAxis(rightRuler);
            }
        }

    //----------------------------------------------------------------
    void LixGaugeGerman::UpdateCustomAxes()
        {
        std::vector<double> activeScoreAreas;
        std::vector<double> activeScoreAreasMainAxis;
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_scoresColumn->GetValue(i)))
                {
                continue;
                }
            const auto currentScore = std::clamp<size_t>(m_scoresColumn->GetValue(i), 0, 100);
            auto leftRulerAxisPos = is_within<size_t>(std::make_pair(0, 24), currentScore)   ? 25 :
                                    is_within<size_t>(std::make_pair(25, 34), currentScore)  ? 35 :
                                    is_within<size_t>(std::make_pair(35, 44), currentScore)  ? 45 :
                                    is_within<size_t>(std::make_pair(45, 54), currentScore)  ? 55 :
                                    is_within<size_t>(std::make_pair(55, 100), currentScore) ? 65 :
                                                                                               65;
            activeScoreAreas.push_back(leftRulerAxisPos);
            // labels on the right side of the score
            activeScoreAreas.push_back(leftRulerAxisPos - 5);

            // center labels along the main axis;
            // handle the labels extending beyond the first and last labels
            activeScoreAreasMainAxis.push_back(leftRulerAxisPos - 5);
            // The axis in the middle is the true value ranges, so 55 or higher
            // is the most difficult. Hence, 55 should be lit up if the score is 55.
            // However, the label to the left is for the area below 55, so it should not
            // be lit up. Thus, we need to showcase 55 for the axis points later, but
            // but don't light up the bracket at 55 on the left axis.
            activeScoreAreasMainAxis.push_back(leftRulerAxisPos - 10);
            if (leftRulerAxisPos == 25)
                {
                leftRulerAxisPos -= 10; // leftRulerAxisPos - 5 already added
                while (leftRulerAxisPos >= 0)
                    {
                    activeScoreAreasMainAxis.push_back(leftRulerAxisPos);
                    leftRulerAxisPos -= 5;
                    }
                }
            else if (leftRulerAxisPos == 65)
                {
                while (leftRulerAxisPos <= 100)
                    {
                    activeScoreAreasMainAxis.push_back(leftRulerAxisPos);
                    leftRulerAxisPos += 5;
                    }
                }
            }
        if (IsShowcasingScore())
            {
            for (auto& customAxis : GetCustomAxes())
                {
                customAxis.ShowcaseAxisPoints(activeScoreAreasMainAxis);
                customAxis.ShowcaseBrackets(activeScoreAreas);
                }
            }
        // reset if previously showcasing items
        else
            {
            for (auto& customAxis : GetCustomAxes())
                {
                customAxis.GhostAllAxisPoints(false);
                customAxis.GhostAllBrackets(false);
                }
            }
        }

    //----------------------------------------------------------------
    void LixGaugeGerman::RecalcSizes(wxDC & dc)
        {
        AdjustAxes();
        UpdateCustomAxes();

        Graph2D::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // start plotting the points
        const auto middleRuler{ GetCustomAxes()[1] };
        const double ptLeft{ GetCustomAxes()[0].GetPhysicalCustomXPosition() };
        const double ptRight{ GetCustomAxes()[2].GetPhysicalCustomXPosition() };

        m_jitter.SetJitterWidth(static_cast<size_t>(ptRight - ptLeft));

        auto points = std::make_unique<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_scoresColumn->GetValue(i)))
                {
                continue;
                }

            // sensical scores fall within 0-100
            const auto currentScore = std::clamp<double>(m_scoresColumn->GetValue(i), 0, 100);

            wxCoord yPt{ 0 };
            assert(middleRuler.GetPhysicalCoordinate(currentScore, yPt));

            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex =
                IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) : 0;

            if (middleRuler.GetPhysicalCoordinate(currentScore, yPt))
                {
                wxPoint pt(middleRuler.GetPhysicalCustomXPosition(), yPt);
                m_jitter.JitterPoint(pt);
                // points on the middle ruler
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt)
                            .Pen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                GetPlotOrCanvasColor()))
                            .Brush(GetColorScheme()->GetColor(colorIndex)),
                        Settings::GetPointRadius(), GetShapeScheme()->GetShape(colorIndex)),
                    dc);
                }
            }
        AddObject(std::move(points));
        }
    } // namespace Wisteria::Graphs
