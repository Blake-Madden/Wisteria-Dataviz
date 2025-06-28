///////////////////////////////////////////////////////////////////////////////
// Name:        danielsonbryan2plot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "danielsonbryan2plot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::DanielsonBryan2Plot, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    DanielsonBryan2Plot::DanielsonBryan2Plot(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_shared<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));
        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"Danielson-Bryan Plot"));
            GetCanvas()->SetName(_(L"Danielson-Bryan Plot"));
            }

        GetBottomXAxis().SetRange(0, 2, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);

        AdjustAxes();
        }

    //----------------------------------------------------------------
    void DanielsonBryan2Plot::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& scoreColumnName,
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

            jitterPoints.insert(std::clamp<size_t>(datum, 0, 100));
            }
        m_jitter.CalcSpread(jitterPoints);
        }

    //----------------------------------------------------------------
    void DanielsonBryan2Plot::AdjustAxes()
        {
        GetLeftYAxis().SetRange(0, 10, 0);

        // these are managed by the plot (not canvas), so clear them here
        GetCustomAxes().clear();

            {
            GraphItems::Axis leftRuler(Wisteria::AxisType::LeftYAxis);
            leftRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            leftRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            leftRuler.SetCustomXPosition(.7f);
            leftRuler.SetCustomYPosition(8);
            leftRuler.SetRange(0, 8, 0);
            leftRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            leftRuler.SetId(100);
            leftRuler.GetAxisLinePen() = wxNullPen;
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(2, 2, 2, L"  0-29"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(3, 3, 3, L"30-49"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(4, 4, 4, L"50-59"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(5, 5, 5, L"60-69"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(6, 6, 6, L"70-79"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(7, 7, 7, L"80-89"));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(8, 8, 8, L"90-100"));
            for (auto& bracket : leftRuler.GetBrackets())
                {
                bracket.SetTickmarkLength(0);
                bracket.SetBracketLineStyle(BracketLineStyle::NoConnectionLines);
                bracket.SetPerpendicularLabelConnectionLinesAlignment(
                    AxisLabelAlignment::AlignWithBoundary);
                }
            AddCustomAxis(leftRuler);
            }

            {
            GraphItems::Axis middleRuler(Wisteria::AxisType::LeftYAxis);
            middleRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            middleRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            middleRuler.SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::CenterOnAxisLine);
            middleRuler.SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
            middleRuler.SetCustomLabel(2, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(3, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(4, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(5, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(6, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(7, GraphItems::Label(L"="));
            middleRuler.SetCustomLabel(8, GraphItems::Label(L"="));
            middleRuler.GetAxisLinePen() = wxNullPen;
            middleRuler.SetCustomXPosition(0.75);
            middleRuler.SetCustomYPosition(8);
            middleRuler.SetRange(0, 8, 0);
            middleRuler.SetId(101);
            AddCustomAxis(middleRuler);
            }

            {
            GraphItems::Axis rightRuler(Wisteria::AxisType::RightYAxis);
            rightRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            rightRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            rightRuler.SetCustomXPosition(0.8);
            rightRuler.SetCustomYPosition(8);
            rightRuler.SetRange(0, 8, 0);
            rightRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            rightRuler.SetId(100);
            rightRuler.GetAxisLinePen() = wxNullPen;
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(2, 2, 2, _(L"very difficult, college level")));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(3, 3, 3, _(L"difficult, high school level")));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                4, 4, 4, _(L"fairly difficult, junior high school level")));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(5, 5, 5, _(L"standard, sixth-grade level")));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(6, 6, 6, _(L"fairly easy, fifth-grade level")));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(7, 7, 7, _(L"easy, fourth-grade level")));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(8, 8, 8, _(L"very easy, third-grade level")));
            for (auto& bracket : rightRuler.GetBrackets())
                {
                bracket.SetTickmarkLength(0);
                bracket.SetBracketLineStyle(BracketLineStyle::NoConnectionLines);
                }
            AddCustomAxis(rightRuler);
            }
        }

    //----------------------------------------------------------------
    void DanielsonBryan2Plot::RecalcSizes(wxDC & dc)
        {
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
            const auto currentScore = std::clamp<size_t>(m_scoresColumn->GetValue(i), 0, 100);

            const auto yAxisPos = is_within<size_t>(std::make_pair(0, 29), currentScore)   ? 2 :
                                  is_within<size_t>(std::make_pair(30, 49), currentScore)  ? 3 :
                                  is_within<size_t>(std::make_pair(50, 59), currentScore)  ? 4 :
                                  is_within<size_t>(std::make_pair(60, 69), currentScore)  ? 5 :
                                  is_within<size_t>(std::make_pair(70, 79), currentScore)  ? 6 :
                                  is_within<size_t>(std::make_pair(80, 89), currentScore)  ? 7 :
                                  is_within<size_t>(std::make_pair(90, 100), currentScore) ? 8 :
                                                                                             8;
            wxCoord yPt{ 0 };
            assert(middleRuler.GetPhysicalCoordinate(yAxisPos, yPt) &&
                   L"Unable to find point on DB2 Plot!");
            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex =
                IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) : 0;

            if (middleRuler.GetPhysicalCoordinate(yAxisPos, yPt))
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
