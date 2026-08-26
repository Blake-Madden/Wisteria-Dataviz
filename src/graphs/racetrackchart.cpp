///////////////////////////////////////////////////////////////////////////////
// Name:        racetrackchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "racetrackchart.h"
#include "../base/currencyformat.h"
#include "../math/safe_math.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::RaceTrackChart, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    RaceTrackChart::TrackLaneSegment::TrackLaneSegment(
        const wxPoint& center, const double startAngle, const double sweepAngle,
        const double startRadius, const double radiusPerDegree, const double thickness,
        const wxColour& color)
        : m_center(center), m_startAngle(startAngle), m_sweepAngle(sweepAngle),
          m_startRadius(startRadius), m_radiusPerDegree(radiusPerDegree), m_thickness(thickness)
        {
        GetGraphItemInfo().Brush(wxBrush{ color }).Pen(wxPen{ color });
        }

    //----------------------------------------------------------------
    wxRect RaceTrackChart::TrackLaneSegment::GetBoundingBox([[maybe_unused]]
                                                            wxDC &
                                                            dc) const
        {
        const auto extent = wxRound(m_startRadius + (m_thickness / 2)) + 1;
        return wxRect{ wxPoint{ m_center.x - extent, m_center.y - extent },
                       wxSize{ extent * 2, extent * 2 } };
        }

    //----------------------------------------------------------------
    wxRect RaceTrackChart::TrackLaneSegment::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

        // sampling as a polyline (rather than AddArc) keeps sweeps longer than a full
        // revolution intact and lets the radius vary along the run
        const auto stepCount =
            static_cast<size_t>(std::clamp(std::ceil(m_sweepAngle * 2), 2.0, 4000.0));

        wxGraphicsPath path = gc->CreatePath();
        for (size_t i = 0; i <= stepCount; ++i)
            {
            const double sweptAngle = m_sweepAngle * safe_divide<double>(i, stepCount);
            const double radius = m_startRadius - (m_radiusPerDegree * sweptAngle);
            const double angle = geometry::degrees_to_radians(m_startAngle + sweptAngle);
            const double xPos = m_center.x + (radius * std::cos(angle));
            const double yPos = m_center.y + (radius * std::sin(angle));
            if (i == 0)
                {
                path.MoveToPoint(xPos, yPos);
                }
            else
                {
                path.AddLineToPoint(xPos, yPos);
                }
            }

        gc->SetPen(gc->CreatePen(
            wxGraphicsPenInfo{ GetGraphItemInfo().GetBrush().GetColour(), m_thickness }
                .Cap(wxCAP_BUTT)
                .Join(wxJOIN_ROUND)));
        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->StrokePath(path);

        return boundingBox;
        }

    //----------------------------------------------------------------
    bool RaceTrackChart::TrackLaneSegment::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetBoundingBox(dc).Contains(pt);
        }

    //----------------------------------------------------------------
    void RaceTrackChart::TrackLaneSegment::Offset(const int xOffset, const int yOffset)
        {
        m_center.x += xOffset;
        m_center.y += yOffset;
        }

    //----------------------------------------------------------------
    void RaceTrackChart::TrackLaneSegment::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                          [[maybe_unused]] wxDC& dc,
                                                          [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    RaceTrackChart::RaceTrackChart(
        Canvas * canvas,
        const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/,
        const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : Graph2D(canvas)
        {
        SetBrushScheme(brushes != nullptr ? brushes :
                                            std::make_shared<Brushes::Schemes::BrushScheme>(
                                                Settings::GetDefaultColorScheme()));
        SetColorScheme(colors);

        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void RaceTrackChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                 const wxString& valueColumnName, const wxString& labelColumnName)
        {
        if (data == nullptr)
            {
            return;
            }
        GetSelectedIds().clear();
        m_trackLanes.clear();

        SetDataset(data);

        m_valueColumnName = valueColumnName;
        m_labelColumnName = labelColumnName;

        const auto valueCol = GetContinuousColumn(m_valueColumnName);
        const auto labelCol = GetCategoricalColumn(m_labelColumnName);

        const auto brushCount = GetBrushScheme() ? GetBrushScheme()->GetBrushes().size() : 0;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            const auto val = valueCol->GetValue(i);
            if (!std::isfinite(val) || val <= 0)
                {
                continue;
                }
            const wxString label = labelCol->GetValueAsLabel(i);

            const wxColour laneColor =
                GetColorScheme() ?
                    GetColorScheme()->GetRecycledColor(m_trackLanes.size()) :
                    (GetBrushScheme() ?
                         GetBrushScheme()
                             ->GetBrush(safe_modulus(m_trackLanes.size(), brushCount))
                             .GetColour() :
                         wxColour{ *wxBLUE });

            m_trackLanes.emplace_back(label, val, laneColor);
            }
        }

    //----------------------------------------------------------------
    int RaceTrackChart::ComputeLapCount() const
        {
        if (m_trackCount == TrackCount::One)
            {
            return 1;
            }
        if (m_trackCount == TrackCount::Two)
            {
            return 2;
            }

        if (m_trackLanes.empty())
            {
            return 1;
            }

        const auto [minLane, maxLane] = std::minmax_element(
            m_trackLanes.cbegin(), m_trackLanes.cend(), [](const auto& first, const auto& second)
            { return first.GetValue() < second.GetValue(); });
        const double maxValue = maxLane->GetValue();

        if (maxValue <= 0)
            {
            return 1;
            }

        // if the smallest track lane would be too narrow to read on one lap, use two
        // laps to give every track lane twice the sweep to work with
        const double minAngleOneLap =
            safe_divide(minLane->GetValue(), maxValue) * (360.0 - m_startSeamGapAngle);
        return (minAngleOneLap >= m_minReadableAngle) ? 1 : 2;
        }

    //----------------------------------------------------------------
    std::vector<double> RaceTrackChart::ComputeLaneAngles(const int laps) const
        {
        std::vector<double> angles;
        if (m_trackLanes.empty())
            {
            return angles;
            }

        const double maxValue = std::max_element(m_trackLanes.cbegin(), m_trackLanes.cend(),
                                                 [](const auto& first, const auto& second)
                                                 { return first.GetValue() < second.GetValue(); })
                                    ->GetValue();
        if (maxValue <= 0)
            {
            return angles;
            }

        // Sweeps are proportional to value/max. The longest gets the full budget:
        // a lap short of closing on itself, plus a lap for each one past the first.
        const double availableAngle =
            (laps * 360.0) - m_startSeamGapAngle - ((laps - 1) * m_lapSeamGapAngle);
        angles.reserve(m_trackLanes.size());
        for (const auto& lane : m_trackLanes)
            {
            angles.push_back(safe_divide(lane.GetValue(), maxValue) * availableAngle);
            }

        return angles;
        }

    //----------------------------------------------------------------
    wxString RaceTrackChart::FormatValue(const double value) const
        {
        if (m_valueFormat == NumberDisplay::Currency)
            {
            return ToCurrency(value, true);
            }
        if (m_valueFormat == NumberDisplay::Percentage)
            {
            return wxString::Format(
                /* TRANSLATORS: Percentage value and percentage symbol (%%).
                   '%%' can be changed and/or moved within string. */
                _(L"%s%%"),
                wxNumberFormatter::ToString(value * 100, 0, Settings::GetDefaultNumberFormat()));
            }
        if (m_valueFormat == NumberDisplay::ValueSimple)
            {
            return wxNumberFormatter::ToString(value, 0, wxNumberFormatter::Style::Style_None);
            }
        return wxNumberFormatter::ToString(value, 0, Settings::GetDefaultNumberFormat());
        }

    //----------------------------------------------------------------
    void RaceTrackChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_trackLanes.empty())
            {
            return;
            }

        const auto plotArea = GetPlotAreaBoundingBox();
        const wxPoint center{ plotArea.GetLeft() + plotArea.GetWidth() / 2,
                              plotArea.GetTop() + plotArea.GetHeight() / 2 };
        const double outerRadius = std::min(plotArea.GetWidth(), plotArea.GetHeight()) * 0.45;
        const double trackWidth = outerRadius * m_trackProportion;

        const int laps = ComputeLapCount();
        const auto laneAngles = ComputeLaneAngles(laps);
        const size_t numLanes = m_trackLanes.size();
        if (laneAngles.size() != numLanes)
            {
            return;
            }

        const double longestSweep = *std::max_element(laneAngles.cbegin(), laneAngles.cend());
        // Track lanes past a full revolution need room for a second lap, so all of
        // them wind inward at the same rate. Moving in lockstep keeps them from crossing.
        const bool isSpiraling = (laps > 1) && (longestSweep > 360.0 - m_lapSeamGapAngle);

        const double ringCount = static_cast<double>(numLanes);
        double ringStep{ 0 };
        if (isSpiraling)
            {
            // A track lane's ending radius depends on both its track index and its own
            // sweep, so size the ring off whichever track lane eats into the track
            // width the most. That keeps every track lane's spiral from crossing
            // through the center, rather than just the one with the longest sweep.
            double worstDivisor{ 0 };
            for (size_t i = 0; i < numLanes; ++i)
                {
                const double divisor = (static_cast<double>(i) + 0.5) +
                                       (ringCount * safe_divide(laneAngles[i], 360.0));
                worstDivisor = std::max(worstDivisor, divisor);
                }
            ringStep = safe_divide(trackWidth, worstDivisor);
            }
        else
            {
            ringStep = safe_divide(trackWidth, ringCount);
            }
        const double laneThickness = ringStep * 0.75;
        const double radiusPerDegree = isSpiraling ? safe_divide(ringCount * ringStep, 360.0) : 0;

        // one track per track lane, outermost first
        std::vector<double> trackRadii;
        trackRadii.reserve(numLanes);
        for (size_t i = 0; i < numLanes; ++i)
            {
            const double startRadius = outerRadius - (i * ringStep) - (ringStep / 2);
            trackRadii.push_back(startRadius);

            AddObject(std::make_unique<TrackLaneSegment>(
                center, m_startAngle, laneAngles[i], startRadius, radiusPerDegree, laneThickness,
                m_trackLanes[i].GetColor()));
            }

        // one row per track, sitting on that track's starting point: the track lane's
        // label flush left, its value flush right against the start line, and a
        // connector line filling the gap between them
        if (m_showLabels)
            {
            const double labelLeft = center.x - trackWidth;
            // right-aligning the value lines its right edge up on the start line
            const double labelRight = center.x - ScaleToScreenAndCanvas(4);
            const double labelSpace = labelRight - labelLeft;
            const double connectorGap = ScaleToScreenAndCanvas(4);
            // on a single track the track lanes run beneath the labels, so box the text
            const bool isSingleTrack{ laps == 1 };
            const wxPen labelPen = isSingleTrack ?
                                       wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black)) :
                                       wxNullPen;
            const wxColour labelBackgroundColor =
                isSingleTrack ? Colors::ColorBrewer::GetColor(Colors::Color::White) :
                                wxTransparentColour;
            const wxCoord labelPadding{ isSingleTrack ? 2 : 0 };

            const auto newRowLabel = [&](const wxString& text, const wxColour& color,
                                         const Anchoring anchor, const double anchorX,
                                         const double anchorY)
            {
                auto label = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo{ text }
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Pen(labelPen)
                        .FontBackgroundColor(labelBackgroundColor)
                        // the outline sides default to off, so the pen alone draws nothing
                        .Outline(isSingleTrack, isSingleTrack, isSingleTrack, isSingleTrack)
                        .Padding(labelPadding, labelPadding, labelPadding, labelPadding)
                        .FontColor(color)
                        .Anchoring(anchor)
                        .AnchorPoint(wxPoint{ wxRound(anchorX), wxRound(anchorY) }));
                label->SetShape(LabelShape::NoShape);
                label->SetBoxCorners(BoxCorners::Straight);
                label->SetShadowType(ShadowType::NoDisplay);
                return label;
            };

            const wxColour labelColor = Colors::ColorBrewer::GetColor(Colors::Color::Black);

            std::vector<GraphItems::Label*> nameLabelPtrs;
            std::vector<GraphItems::Label*> valueLabelPtrs;
            nameLabelPtrs.reserve(numLanes);
            valueLabelPtrs.reserve(numLanes);
            for (size_t i = 0; i < numLanes; ++i)
                {
                const double labelY =
                    center.y +
                    (trackRadii[i] * std::sin(geometry::degrees_to_radians(m_startAngle)));

                auto nameLabel = newRowLabel(m_trackLanes[i].GetLabel(), labelColor,
                                             Anchoring::TopLeftCorner, labelLeft, labelY);
                nameLabelPtrs.push_back(nameLabel.get());
                AddObject(std::move(nameLabel));

                const wxString valueText = FormatValue(m_trackLanes[i].GetValue());
                auto valueLabel = newRowLabel(valueText, labelColor, Anchoring::TopRightCorner,
                                              labelRight, labelY);
                valueLabelPtrs.push_back(valueLabel.get());
                AddObject(std::move(valueLabel));
                }

            // one shared factor keeps the labels a common size while fitting the room
            // left of the start line and clearing the label on the next track in
            double widestRow{ 0 };
            double tallestLabel{ 0 };
            for (size_t i = 0; i < numLanes; ++i)
                {
                const auto nameBox = nameLabelPtrs[i]->GetBoundingBox(dc);
                const auto valueBox = valueLabelPtrs[i]->GetBoundingBox(dc);
                widestRow = std::max<double>(widestRow, nameBox.GetWidth() + connectorGap +
                                                            valueBox.GetWidth());
                tallestLabel =
                    std::max<double>({ tallestLabel, static_cast<double>(nameBox.GetHeight()),
                                       static_cast<double>(valueBox.GetHeight()) });
                }
            const double sizeFactor =
                std::clamp(std::min({ safe_divide(labelSpace, widestRow),
                                      safe_divide(ringStep, tallestLabel), 1.0 }),
                           math_constants::tenth, 1.0);

            auto connectorLines = std::make_unique<GraphItems::Lines>(
                wxPen(Colors::ColorBrewer::GetColor(Colors::Color::Black)), GetScaling());
            for (size_t i = 0; i < numLanes; ++i)
                {
                auto* nameLabel = nameLabelPtrs[i];
                auto* valueLabel = valueLabelPtrs[i];
                if (sizeFactor < 1.0)
                    {
                    nameLabel->SetScaling(nameLabel->GetScaling() * sizeFactor);
                    valueLabel->SetScaling(valueLabel->GetScaling() * sizeFactor);
                    }
                // re-center on the track now that the final text heights are known
                const auto nameBox = nameLabel->GetBoundingBox(dc);
                const auto valueBox = valueLabel->GetBoundingBox(dc);
                nameLabel->SetAnchorPoint(nameLabel->GetAnchorPoint() -
                                          wxPoint(0, nameBox.GetHeight() / 2));
                valueLabel->SetAnchorPoint(valueLabel->GetAnchorPoint() -
                                           wxPoint(0, valueBox.GetHeight() / 2));

                // fill the gap between the two labels with a connector line
                const auto finalNameBox = nameLabel->GetBoundingBox(dc);
                const auto finalValueBox = valueLabel->GetBoundingBox(dc);
                const wxCoord lineY = finalNameBox.GetTop() + (finalNameBox.GetHeight() / 2);
                const wxCoord lineStartX = finalNameBox.GetRight() + connectorGap;
                const wxCoord lineEndX = finalValueBox.GetLeft() - connectorGap;
                if (lineStartX < lineEndX)
                    {
                    connectorLines->AddLine(wxPoint{ lineStartX, lineY },
                                            wxPoint{ lineEndX, lineY });
                    }
                }
            AddObject(std::move(connectorLines));
            }
        }

    //----------------------------------------------------------------
    void RaceTrackChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A race track chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");

        for (const auto& lane : m_trackLanes)
            {
            label +=
                L". " + wxString::Format(L"%s: %s", lane.GetLabel(),
                                         wxNumberFormatter::ToString(
                                             lane.GetValue(), 0,
                                             wxNumberFormatter::Style::Style_NoTrailingZeroes));
            }

        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
