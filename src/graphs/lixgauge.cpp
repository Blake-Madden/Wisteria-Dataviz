///////////////////////////////////////////////////////////////////////////////
// Name:        lixgauge.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lixgauge.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::LixGauge, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    LixGauge::LixGauge(Wisteria::Canvas * canvas,
                       std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/,
                       std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? std::move(colors) : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? std::move(shapes) :
                                           std::make_shared<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"Lix Gauge"));
            GetCanvas()->SetName(_(L"Lix Gauge"));
            }

        GetBottomXAxis().SetRange(0, 2, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 100, 0, 5, 1);
        GetLeftYAxis().Reverse();
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);

        GetTitle() = GraphItems::Label(
            GraphItems::GraphItemInfo(_(L"Lix standards")).Scaling(GetScaling()).Pen(wxNullPen));
        }

    //----------------------------------------------------------------
    void LixGauge::SetData(std::shared_ptr<const Data::Dataset> data,
                           const wxString& scoreColumnName,
                           const std::optional<const wxString>& groupColumnName /*= std::nullopt*/)
        {
        SetDataset(std::move(data));
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
    void LixGauge::AdjustAxes()
        {
        const auto getMinMaxForRange = [this]()
        {
            if (GetDataset() != nullptr)
                {
                const auto [minVal, maxVal] = std::minmax_element(
                    m_scoresColumn->GetValues().cbegin(), m_scoresColumn->GetValues().cend());
                // for the lines up top
                constexpr auto axisOffset{ 10 };
                const auto minYAxis =
                    m_scoresColumn->GetValues().size() ?
                        std::min(20.0, previous_interval(*minVal, 2)) - axisOffset :
                        10.0;
                const auto maxYAxis = m_scoresColumn->GetValues().size() ?
                                          std::max(60.0, next_interval(*maxVal, 2)) + axisOffset :
                                          70.0;
                return std::make_pair(minYAxis, maxYAxis);
                }
            else
                {
                return std::make_pair(10.0, 70.0);
                }
        };

        const auto [minYAxis, maxYAxis] = getMinMaxForRange();
        GetLeftYAxis().SetRange(minYAxis, maxYAxis, 0, 5, 1);

        // these are managed by the plot (not parent canvas), so clear them here
        GetCustomAxes().clear();

            {
            GraphItems::Axis leftRuler(Wisteria::AxisType::LeftYAxis);
            leftRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            leftRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            leftRuler.SetCustomXPosition(0.9f);
            leftRuler.SetCustomYPosition(minYAxis);
            leftRuler.SetRange(minYAxis, maxYAxis, 0, 10, 1);
            leftRuler.SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
            leftRuler.SetCustomLabel(20, GraphItems::Label(_DT(L"20", DTExplanation::Constant)));
            leftRuler.SetCustomLabel(30, GraphItems::Label(L"30"));
            leftRuler.SetCustomLabel(40, GraphItems::Label(L"40"));
            leftRuler.SetCustomLabel(50, GraphItems::Label(L"50"));
            leftRuler.SetCustomLabel(60, GraphItems::Label(L"60"));
            leftRuler.Reverse();
            leftRuler.SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::AlignWithBoundary);
            leftRuler.SetId(100);
            leftRuler.GetAxisLinePen() = wxNullPen;
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(20, 20, 20, _(L"Very easy text"),
                                                               wxColour(66, 51, 251)));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(30, 30, 30, _(L"Easy text"),
                                                               wxColour(163, 182, 250)));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(40, 40, 40, _(L"Average text"),
                                                               wxColour(239, 173, 186)));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(50, 50, 50, _(L"Difficult text"),
                                                               wxColour(237, 27, 37)));
            leftRuler.AddBracket(GraphItems::Axis::AxisBracket(
                60, 60, 60, _(L"Very difficult text"), wxColour(250, 0, 0)));
            for (auto& bracket : leftRuler.GetBrackets())
                {
                bracket.GetLinePen().SetStyle(wxPenStyle::wxPENSTYLE_DOT);
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                bracket.SetPerpendicularLabelConnectionLinesAlignment(
                    AxisLabelAlignment::AlignWithBoundary);
                bracket.GetLabel().SetFontColor(*wxBLACK);
                }
            AddCustomAxis(leftRuler);
            }

            {
            GraphItems::Axis middleRuler(Wisteria::AxisType::LeftYAxis);
            middleRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            middleRuler.SetCustomXPosition(1);
            middleRuler.SetCustomYPosition(minYAxis);
            middleRuler.SetRange(minYAxis, maxYAxis, 0, 10, 1);
            middleRuler.Reverse();
            middleRuler.SetId(101);
            middleRuler.Show(false);
            AddCustomAxis(middleRuler);
            }

            {
            GraphItems::Axis rightRuler(Wisteria::AxisType::RightYAxis);
            rightRuler.SetFontColor(GetLeftYAxis().GetFontColor());
            rightRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            rightRuler.SetCustomXPosition(1.1f);
            rightRuler.SetCustomYPosition(minYAxis);
            rightRuler.SetRange(minYAxis, maxYAxis, 0, 5, 1);
            rightRuler.SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
            rightRuler.SetCustomLabel(25, GraphItems::Label(L"25"));
            rightRuler.SetCustomLabel(35, GraphItems::Label(L"35"));
            rightRuler.SetCustomLabel(45, GraphItems::Label(L"45"));
            rightRuler.SetCustomLabel(55, GraphItems::Label(L"55"));
            rightRuler.Reverse();
            rightRuler.SetId(102);
            rightRuler.GetAxisLinePen() = wxNullPen;
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                25, 25, 25, _(L"Books for children"), wxColour(138, 163, 249)));
            rightRuler.AddBracket(
                GraphItems::Axis::AxisBracket(35, 35, 35, _(L"Fiction"), wxColour(207, 217, 252)));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(45, 45, 45, _(L"Factual prose"),
                                                                wxColour(245, 126, 133)));
            rightRuler.AddBracket(GraphItems::Axis::AxisBracket(
                55, 55, 55, _(L"Technical literature"), wxColour(237, 10, 10)));
            for (auto& bracket : rightRuler.GetBrackets())
                {
                bracket.GetLinePen().SetStyle(wxPenStyle::wxPENSTYLE_DOT);
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                }
            AddCustomAxis(rightRuler);
            }
        }

    //----------------------------------------------------------------
    void LixGauge::RecalcSizes(wxDC & dc)
        {
        AdjustAxes();

        Graph2D::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // draw outer lines
        auto outerLines = std::make_unique<GraphItems::Lines>(
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
            GetScaling());
        wxCoord coordX1{ 0 }, coordX2{ 0 }, coordY1{ 0 }, coordY2{ 0 }, coordY3{ 0 };
        const auto [startX, endX] = GetBottomXAxis().GetRange();
        const auto [startY, endY] = GetLeftYAxis().GetRange();
        if (GetBottomXAxis().GetPhysicalCoordinate(startX, coordX1) &&
            GetBottomXAxis().GetPhysicalCoordinate(endX, coordX2) &&
            GetLeftYAxis().GetPhysicalCoordinate(endY, coordY1) &&
            GetLeftYAxis().GetPhysicalCoordinate(endY + 2, coordY2) &&
            GetLeftYAxis().GetPhysicalCoordinate(startY, coordY3))
            {
            outerLines->AddLine(wxPoint(coordX1, coordY1), wxPoint(coordX2, coordY1));
            outerLines->AddLine(wxPoint(coordX1, coordY2), wxPoint(coordX2, coordY2));
            outerLines->AddLine(wxPoint(coordX1, coordY3), wxPoint(coordX2, coordY3));
            }
        AddObject(std::move(outerLines));

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
            assert(middleRuler.GetPhysicalCoordinate(currentScore, yPt) &&
                   L"Unable to find point on Lix gauge!");
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
