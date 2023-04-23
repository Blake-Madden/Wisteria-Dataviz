///////////////////////////////////////////////////////////////////////////////
// Name:        lixgaugegerman.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "lixgaugegerman.h"

using namespace Wisteria;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    LixGaugeGerman::LixGaugeGerman(Wisteria::Canvas* canvas,
        std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/,
        std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes /*= nullptr*/) :
        GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
            Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
            std::make_shared<Wisteria::Icons::Schemes::IconScheme>(
                Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"German Lix Gauge"));
            GetCanvas()->SetName(_(L"German Lix Gauge"));
            }

        GetBottomXAxis().SetRange(0, 2, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 100, 0, 5, 1);
        GetLeftYAxis().ReverseScale(true);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void LixGaugeGerman::SetData(std::shared_ptr<const Data::Dataset> data,
                            const wxString& scoreColumnName,
                            std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        m_scoresColumn = nullptr;
        m_jitter.ResetJitterData();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            { return; }

        SetGroupColumn(groupColumnName);

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            { BuildGroupIdMap(); }

        // get the score data
        m_scoresColumn = GetContinuousColumnRequired(scoreColumnName);

        frequency_set<double> jitterPoints;
        for (const auto& datum : m_scoresColumn->GetValues())
            {
            if (std::isnan(datum))
                { continue; }

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
                    m_scoresColumn->GetValues().cbegin(),
                    m_scoresColumn->GetValues().cend());
                const auto minYAxis = m_scoresColumn->GetValues().size() ?
                                        std::min(20.0, previous_interval(*minVal, 2)) :
                                        20.0;
                const auto maxYAxis = m_scoresColumn->GetValues().size() ?
                                        std::max(70.0, next_interval(*maxVal, 2)) :
                                        70.0;
                return std::make_pair(minYAxis, maxYAxis);
                }
            else
                { return std::make_pair(20.0, 70.0); }
            };

        const auto [minYAxis, maxYAxis] = getMinMaxForRange();
        GetLeftYAxis().SetRange(minYAxis, maxYAxis, 0, 5, 1);

        // these are managed by the plot (not canvas), so clear them here
        GetCustomAxes().clear();

            {
            Axis leftRuler(AxisType::LeftYAxis);
            leftRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            leftRuler.SetCustomXPosition(0.9f);
            leftRuler.SetCustomYPosition(minYAxis);
            leftRuler.SetRange(minYAxis,maxYAxis, 0, 5, 1);
            leftRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            leftRuler.ReverseScale(true);
            leftRuler.SetId(100);
            leftRuler.GetAxisLinePen() = wxNullPen;
            leftRuler.AddBracket(Axis::AxisBracket(25, 25, 25,
                IsUsingEnglishLabels() ? _("very easy text") :
                wxString(_DT("Sehr leichter Text")), wxColour(66, 51, 251)));
            leftRuler.AddBracket(Axis::AxisBracket(35, 35, 35,
                IsUsingEnglishLabels() ? _("easy text") : wxString(_DT("Leichter Text")),
                wxColour(163,182,250)));
            leftRuler.AddBracket(Axis::AxisBracket(45, 45, 45,
                IsUsingEnglishLabels() ? _("average text") :
                wxString(_DT("Durchschnittlicher Text")), wxColour(239, 173, 186)));
            leftRuler.AddBracket(Axis::AxisBracket(55, 55, 55,
                IsUsingEnglishLabels() ? _("difficult text") :
                wxString(_DT("Schwieriger Text")), wxColour(237, 27, 37)));
            leftRuler.AddBracket(Axis::AxisBracket(65, 65, 65,
                IsUsingEnglishLabels() ? _("very difficult text") :
                wxString(_DT("Sehr schwieriger Text")), wxColour(250, 0, 0)));
            for (auto& bracket : leftRuler.GetBrackets())
                {
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                bracket.SetBracketLineStyle(BracketLineStyle::ReverseArrow);
                bracket.SetPerpendicularLabelConnectionLinesAlignment(
                    AxisLabelAlignment::AlignWithBoundary);
                bracket.GetLabel().SetFontColor(*wxBLACK);
                bracket.GetLabel().SetTextAlignment(TextAlignment::FlushLeft);
                }
            AddCustomAxis(leftRuler);
            }

            {
            Axis middleRuler(Wisteria::AxisType::LeftYAxis);
            middleRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            middleRuler.SetPerpendicularLabelAxisAlignment(AxisLabelAlignment::CenterOnAxisLine);
            middleRuler.GetAxisLinePen() = wxNullPen;
            middleRuler.SetOutlineSize(wxSize(15, 5));
            middleRuler.SetCustomXPosition(1);
            middleRuler.SetCustomYPosition(minYAxis);
            middleRuler.SetRange(minYAxis,maxYAxis, 0, 5, 1);
            middleRuler.ReverseScale(true);
            middleRuler.SetId(101);
            AddCustomAxis(middleRuler);
            }

            {
            Axis rightRuler(Wisteria::AxisType::RightYAxis);
            rightRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            rightRuler.SetCustomXPosition(1.1f);
            rightRuler.SetCustomYPosition(minYAxis);
            rightRuler.SetRange(minYAxis,maxYAxis, 0, 5, 1);
            rightRuler.SetLabelDisplay(AxisLabelDisplay::NoDisplay);
            rightRuler.ReverseScale(true);
            rightRuler.SetId(102);
            rightRuler.GetAxisLinePen() = wxNullPen;
            rightRuler.AddBracket(Axis::AxisBracket(30, 30, 30,
                IsUsingEnglishLabels() ?
                _("children and youth\nliterature (for ages 8-16)") :
                wxString(DONTTRANSLATE(L"Kinder- und Jugendb\U000000FCcher", DTExplanation::DirectQuote)),
                wxColour(138,163,249)));
            rightRuler.AddBracket(Axis::AxisBracket(40, 40, 40,
                IsUsingEnglishLabels() ?
                _("bellestristic texts (prose\nfiction for adults)") :
                wxString(DONTTRANSLATE(L"Belletristik")), wxColour(207, 217, 252)));
            rightRuler.AddBracket(Axis::AxisBracket(50, 50, 50,
                IsUsingEnglishLabels() ?
                _("informational (non fiction)\ntexts (Sachliteratur)") :
                wxString(DONTTRANSLATE(L"Sachliteratur")), wxColour(245, 126, 133)));
            rightRuler.AddBracket(Axis::AxisBracket(60, 60, 60,
                IsUsingEnglishLabels() ?
                _("technical texts\n(Fachliteratur)") :
                wxString(DONTTRANSLATE(L"Fachliteratur")), wxColour(237, 10, 10)));
            for (auto& bracket : rightRuler.GetBrackets())
                {
                bracket.GetLinePen().SetWidth(2);
                bracket.SetTickmarkLength(30);
                bracket.SetBracketLineStyle(BracketLineStyle::ReverseArrow);
                bracket.GetLabel().SetFontColor(*wxBLACK);
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
    void LixGaugeGerman::RecalcSizes(wxDC& dc)
        {
        AdjustAxes();

        Graph2D::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            { return; }

        // start plotting the points
        const auto middleRuler{ GetCustomAxes()[1] };
        const double ptLeft{ GetCustomAxes()[0].GetPhysicalCustomXPosition() };
        const double ptRight{ GetCustomAxes()[2].GetPhysicalCustomXPosition() };

        m_jitter.SetJitterWidth(static_cast<size_t>(ptRight-ptLeft));

        auto points = std::make_shared<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_scoresColumn->GetValue(i)))
                { continue; }

            // sensical scores fall within 0-100
            const auto currentScore = std::clamp<double>(m_scoresColumn->GetValue(i), 0, 100);

            wxCoord yPt{ 0 };
            wxASSERT_LEVEL_2(middleRuler.GetPhysicalCoordinate(currentScore, yPt));

            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex = IsUsingGrouping() ?
                GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) :
                0;

            if (middleRuler.GetPhysicalCoordinate(currentScore, yPt))
                {
                wxPoint pt(middleRuler.GetPhysicalCustomXPosition(), yPt);
                m_jitter.JitterPoint(pt);
                // points on the middle ruler
                points->AddPoint(Point2D(
                    GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i)).
                    AnchorPoint(pt).
                    Brush(GetColorScheme()->GetColor(colorIndex)),
                    Settings::GetPointRadius(),
                    GetShapeScheme()->GetShape(colorIndex)), dc);
                }
            }
        AddObject(points);
        }
    }
