///////////////////////////////////////////////////////////////////////////////
// Name:        duboisspiralchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "duboisspiralchart.h"
#include "../math/safe_math.h"
#include <wx/geometry.h>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::DuBoisSpiralChart, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    wxPoint2DDouble DuBoisSpiralChart::SpiralPointAt(const double centerX, const double centerY,
                                                     const double radius, const double angleDeg)
        {
        const double angle = geometry::degrees_to_radians(angleDeg);
        return wxPoint2DDouble{ centerX + (radius * std::cos(angle)),
                                centerY + (radius * std::sin(angle)) };
        }

    //----------------------------------------------------------------
    DuBoisSpiralChart::PathSegment::PathSegment(const double thickness, const wxColour& color)
        : m_thickness(thickness)
        {
        GetGraphItemInfo().Brush(wxBrush{ color }).Pen(wxPen{ color });
        }

    //----------------------------------------------------------------
    wxRect DuBoisSpiralChart::PathSegment::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }
        StrokeShape(*gc);
        return boundingBox;
        }

    //----------------------------------------------------------------
    bool DuBoisSpiralChart::PathSegment::HitTest(const wxPoint pt, wxDC& dc) const
        {
        return GetBoundingBox(dc).Contains(pt) && IsOnShape(pt);
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::PathSegment::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                        [[maybe_unused]] wxDC& dc,
                                                        [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    DuBoisSpiralChart::SpiralSegment::SpiralSegment(
        const wxPoint& center, const double startAngle, const double sweepAngle,
        const double startRadius, const double radiusPerDegree, const double thickness,
        const wxColour& color)
        : PathSegment(thickness, color), m_center(center), m_startAngle(startAngle),
          m_sweepAngle(sweepAngle), m_startRadius(startRadius), m_radiusPerDegree(radiusPerDegree)
        {
        }

    //----------------------------------------------------------------
    wxRect DuBoisSpiralChart::SpiralSegment::GetBoundingBox([[maybe_unused]]
                                                            wxDC &
                                                            dc) const
        {
        const auto extent = wxRound(m_startRadius + (GetThickness() / 2)) + 1;
        return wxRect{ wxPoint{ m_center.x - extent, m_center.y - extent },
                       wxSize{ extent * 2, extent * 2 } };
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::SpiralSegment::StrokeShape(wxGraphicsContext & gc) const
        {
        // sampling as a polyline (rather than AddArc) keeps sweeps longer than a full
        // revolution intact and lets the radius vary along the run
        const auto stepCount =
            static_cast<size_t>(std::clamp(std::ceil(m_sweepAngle * 2), 2.0, 4000.0));

        wxGraphicsPath path = gc.CreatePath();
        for (size_t i = 0; i <= stepCount; ++i)
            {
            const double sweptAngle = m_sweepAngle * safe_divide<double>(i, stepCount);
            const double radius = m_startRadius - (m_radiusPerDegree * sweptAngle);
            const wxPoint2DDouble pt =
                SpiralPointAt(m_center.x, m_center.y, radius, m_startAngle + sweptAngle);
            if (i == 0)
                {
                path.MoveToPoint(pt.m_x, pt.m_y);
                }
            else
                {
                path.AddLineToPoint(pt.m_x, pt.m_y);
                }
            }

        gc.SetPen(gc.CreatePen(
            wxGraphicsPenInfo{ GetGraphItemInfo().GetBrush().GetColour(), GetThickness() }
                .Cap(wxCAP_BUTT)
                .Join(wxJOIN_ROUND)));
        gc.SetBrush(*wxTRANSPARENT_BRUSH);
        gc.StrokePath(path);
        }

    //----------------------------------------------------------------
    bool DuBoisSpiralChart::SpiralSegment::IsOnShape(const wxPoint pt) const
        {
        if (!(m_radiusPerDegree > 0))
            {
            return false;
            }
        const double distX = pt.x - m_center.x;
        const double distY = pt.y - m_center.y;
        const double dist = std::hypot(distX, distY);
        // how far the spiral has to travel to point in this direction for the first time
        double firstSweep =
            std::fmod(geometry::radians_to_degrees(std::atan2(distY, distX)) - m_startAngle, 360.0);
        if (firstSweep < 0)
            {
            firstSweep += 360.0;
            }
        if (firstSweep > m_sweepAngle)
            {
            return false;
            }
        // of the revolutions passing through this direction, use the one whose
        // radius is closest to the point's distance from the center
        const double sweepAtDist = safe_divide(m_startRadius - dist, m_radiusPerDegree);
        const double maxTurns = std::floor(safe_divide(m_sweepAngle - firstSweep, 360.0));
        const double turns =
            std::clamp(std::round(safe_divide(sweepAtDist - firstSweep, 360.0)), 0.0, maxTurns);
        const double sweep = firstSweep + (turns * 360.0);
        return std::abs(dist - (m_startRadius - (m_radiusPerDegree * sweep))) <= GetHitReach();
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::SpiralSegment::Offset(const int xOffset, const int yOffset)
        {
        m_center.x += xOffset;
        m_center.y += yOffset;
        }

    //----------------------------------------------------------------
    DuBoisSpiralChart::StraightSegment::StraightSegment(
        const wxPoint& start, const wxPoint& end, const double thickness, const wxColour& color)
        : PathSegment(thickness, color), m_start(start), m_end(end)
        {
        }

    //----------------------------------------------------------------
    wxRect DuBoisSpiralChart::StraightSegment::GetBoundingBox([[maybe_unused]]
                                                              wxDC &
                                                              dc) const
        {
        const auto pad = wxRound(GetThickness() / 2) + 1;
        return wxRect{
            wxPoint{ std::min(m_start.x, m_end.x) - pad, std::min(m_start.y, m_end.y) - pad },
            wxPoint{ std::max(m_start.x, m_end.x) + pad, std::max(m_start.y, m_end.y) + pad }
        };
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::StraightSegment::StrokeShape(wxGraphicsContext & gc) const
        {
        wxGraphicsPath path = gc.CreatePath();
        path.MoveToPoint(m_start.x, m_start.y);
        path.AddLineToPoint(m_end.x, m_end.y);

        // Flat butt line body with round discs at both ends. The discs form
        // rounded corners at the joints, with the predecessor's disc drawn
        // later (on top) winning the joint.
        const double thickness = GetThickness();
        const wxColour color = GetGraphItemInfo().GetBrush().GetColour();
        gc.SetPen(
            gc.CreatePen(wxGraphicsPenInfo{ color, thickness }.Cap(wxCAP_BUTT).Join(wxJOIN_MITER)));
        gc.SetBrush(*wxTRANSPARENT_BRUSH);
        gc.StrokePath(path);
        gc.SetBrush(gc.CreateBrush(wxBrush{ color }));
        gc.SetPen(*wxTRANSPARENT_PEN);
        const double radius = thickness * math_constants::half;
        gc.DrawEllipse(static_cast<double>(m_start.x) - radius,
                       static_cast<double>(m_start.y) - radius, thickness, thickness);
        gc.DrawEllipse(static_cast<double>(m_end.x) - radius, static_cast<double>(m_end.y) - radius,
                       thickness, thickness);
        }

    //----------------------------------------------------------------
    bool DuBoisSpiralChart::StraightSegment::IsOnShape(const wxPoint pt) const
        {
        // distance from the point to the closest spot on the line
        const double lineX = m_end.x - m_start.x;
        const double lineY = m_end.y - m_start.y;
        const double along =
            safe_divide<double>(((pt.x - m_start.x) * lineX) + ((pt.y - m_start.y) * lineY),
                                (lineX * lineX) + (lineY * lineY));
        const double clampedAlong = std::clamp(along, 0.0, 1.0);
        const double closestX = m_start.x + (clampedAlong * lineX);
        const double closestY = m_start.y + (clampedAlong * lineY);
        return std::hypot(pt.x - closestX, pt.y - closestY) <= GetHitReach();
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::StraightSegment::Offset(const int xOffset, const int yOffset)
        {
        m_start.x += xOffset;
        m_start.y += yOffset;
        m_end.x += xOffset;
        m_end.y += yOffset;
        }

    //----------------------------------------------------------------
    DuBoisSpiralChart::DuBoisSpiralChart(
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
    void DuBoisSpiralChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                    const wxString& valueColumnName,
                                    const wxString& labelColumnName)
        {
        if (data == nullptr)
            {
            return;
            }
        // the column lookups go through the graph's dataset, so put the previous
        // one back if they fail
        const std::shared_ptr<const Data::Dataset> previousData{ GetDataset() };
        SetDataset(data);
        try
            {
            const auto valueCol = GetContinuousColumn(valueColumnName);
            const auto labelCol = GetCategoricalColumn(labelColumnName);

            const auto brushCount = GetBrushScheme() ? GetBrushScheme()->GetBrushes().size() : 0;
            std::vector<SpiralNodeInfo> nodes;
            nodes.reserve(data->GetRowCount());
            for (size_t i = 0; i < data->GetRowCount(); ++i)
                {
                const auto val = valueCol->GetValue(i);
                if (!std::isfinite(val) || val <= 0)
                    {
                    continue;
                    }
                const wxString label = labelCol->GetValueAsLabel(i);

                const wxColour nodeColor =
                    GetColorScheme() ? GetColorScheme()->GetRecycledColor(nodes.size()) :
                                       (GetBrushScheme() && brushCount > 0 ?
                                            GetBrushScheme()
                                                ->GetBrush(safe_modulus(nodes.size(), brushCount))
                                                .GetColour() :
                                            wxColour{ *wxBLUE });

                nodes.emplace_back(label, val, nodeColor);
                }

            GetSelectedIds().clear();
            m_nodes = std::move(nodes);
            m_valueColumnName = valueColumnName;
            m_labelColumnName = labelColumnName;
            }
        catch (...)
            {
            SetDataset(previousData);
            throw;
            }
        }

    //----------------------------------------------------------------
    wxString DuBoisSpiralChart::FormatValue(const double value, const bool thousandsSeparator) const
        {
        // Whole numbers show no decimals (trailing zeroes are dropped),
        // and fractions keep two significant digits.
        const auto decimalsFor = [](const double num)
        {
            const double magnitude = std::abs(num);
            if (magnitude == 0 || magnitude >= 1)
                {
                return 2;
                }
            return std::clamp(1 - static_cast<int>(std::floor(std::log10(magnitude))), 2, 6);
        };
        const int numberStyle = wxNumberFormatter::Style_NoTrailingZeroes |
                                (thousandsSeparator ? wxNumberFormatter::Style_WithThousandsSep :
                                                      wxNumberFormatter::Style_None);

        if (m_valueFormat == NumberDisplay::Currency)
            {
            return wxNumberFormatter::ToString(value, 2,
                                               numberStyle | wxNumberFormatter::Style_Currency |
                                                   wxNumberFormatter::Style_CurrencySymbol);
            }
        if (m_valueFormat == NumberDisplay::Percentage)
            {
            return wxString::Format(
                /* TRANSLATORS: Percentage value and percentage symbol (%%).
                   '%%' can be changed and/or moved within string. */
                _(L"%s%%"),
                wxNumberFormatter::ToString(value * 100, decimalsFor(value * 100), numberStyle));
            }
        if (m_valueFormat == NumberDisplay::ValueSimple)
            {
            return wxNumberFormatter::ToString(value, decimalsFor(value),
                                               wxNumberFormatter::Style_NoTrailingZeroes);
            }
        return wxNumberFormatter::ToString(value, decimalsFor(value), numberStyle);
        }

    //----------------------------------------------------------------
    wxString DuBoisSpiralChart::FormatNodeText(const SpiralNodeInfo& node,
                                               const bool thousandsSeparator) const
        {
        return wxString::Format(_DT(L"%s: %s"), FormatValue(node.GetValue(), thousandsSeparator),
                                node.GetLabel());
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_nodes.empty())
            {
            return;
            }

        const auto plotArea = GetPlotAreaBoundingBox();
        if (plotArea.GetWidth() <= 10 || plotArea.GetHeight() <= 10)
            {
            return;
            }
        const double minDim =
            static_cast<double>(std::min(plotArea.GetWidth(), plotArea.GetHeight()));
        // pixel floors below are proportions of the smaller plot dimension,
        // so they grow and shrink with the plot instead of staying fixed
        constexpr double MIN_HOLE_PROPORTION{ 0.03 };
        constexpr double MIN_LINE_PROPORTION{ 0.004 };
        constexpr double THINNEST_LINE_PROPORTION{ 0.002 };
        constexpr double PLOT_PAD_PROPORTION{ 0.012 };
        constexpr double LABEL_GAP_PROPORTION{ 0.016 };
        constexpr double LABEL_PAD_PROPORTION{ 0.008 };
        // bounded iteration caps for the layout searches below
        constexpr int THICKNESS_SEARCH_STEPS{ 20 };
        constexpr int SCALE_SHRINK_STEPS{ 40 };
        constexpr int SCALE_GROWTH_STEPS{ 10 };
        constexpr int SCALE_BISECT_STEPS{ 25 };
        constexpr int LABEL_SHRINK_STEPS{ 10 };
        constexpr double LABEL_SHRINK_FACTOR{ 0.75 };
        // starting thickness; thinned below if the spiral needs room
        double lineThickness =
            std::max(minDim * m_lineThicknessProportion, minDim * MIN_LINE_PROPORTION);
        // distance between successive turns of the spiral for a given thickness
        const auto spacingFor = [&](const double thick)
        { return thick * (1.0 + m_spiralGapRatio); };
        // the spiral hole holds only the value label, so keep it small but legible
        const auto holeFor = [&](const double thick)
        { return std::max(thick * 2.0, minDim * MIN_HOLE_PROPORTION); };
        double radiusPerDegree{ 0 };
        double minRadius{ 0 };
        // sets the line thickness along with the spiral values derived from it
        const auto applyThickness = [&](const double thick)
        {
            lineThickness = thick;
            radiusPerDegree = safe_divide(spacingFor(thick), 360.0);
            minRadius = holeFor(thick);
        };

        const double angleRad = geometry::degrees_to_radians(m_zigZagAngleDeg);
        const double cosAngle = std::cos(angleRad);
        const double sinAngle = std::sin(angleRad);

        const double pad = minDim * PLOT_PAD_PROPORTION;
        const auto availW = static_cast<double>(plotArea.GetWidth()) - (pad * 2);
        const auto availH = static_cast<double>(plotArea.GetHeight()) - (pad * 2);
        if (availW <= 10 || availH <= 10)
            {
            return;
            }
        // the spiral never grows past this radius; oversized content
        // shrinks the overall scale instead
        const double maxSpiralRadius = std::min(availW, availH) * m_outerRadiusProportion;
        // cap the thickness up front so the hole stays inside the spiral
        // budget; otherwise, every simulation fails and valid data renders blank
        applyThickness(
            std::min(lineThickness, std::max(minDim * THINNEST_LINE_PROPORTION,
                                             maxSpiralRadius * math_constants::half * 0.99)));

        // plain (shapeless) label builder, shared by measuring and drawing
        const auto createPlainLabel = [&](const wxString& text, const double scaling)
        {
            GraphItems::Label label(GraphItems::GraphItemInfo{ text }
                                        .Scaling(scaling)
                                        .DPIScaling(GetDPIScaleFactor())
                                        .Pen(wxNullPen)
                                        .FontColor(Colors::ColorContrast::BlackOrWhiteContrast(
                                            GetPlotOrCanvasColor())));
            label.SetShape(LabelShape::NoShape);
            label.SetBoxCorners(BoxCorners::Straight);
            label.SetShadowType(ShadowType::NoDisplay);
            return label;
        };

        // The simulation reserves the spiral's name label height (zero when labels are hidden)
        // and the side label ("value: label") boxes, measured at the label scaling.
        double nameLabelHeight{ 0 };
        std::vector<wxSize> sideLabelSizes;
        double labelScaling = GetScaling();
        const auto measureLabels = [&](const double scaling)
        {
            nameLabelHeight = 0;
            sideLabelSizes.clear();
            if (!m_showLabels)
                {
                return;
                }
            GraphItems::Label nameMeasure = createPlainLabel(m_nodes.back().GetLabel(), scaling);
            nameLabelHeight = static_cast<double>(nameMeasure.GetBoundingBox(dc).GetHeight());
            sideLabelSizes.reserve(m_nodes.size() - 1);
            for (size_t i = 0; i + 1 < m_nodes.size(); ++i)
                {
                GraphItems::Label sideMeasure =
                    createPlainLabel(FormatNodeText(m_nodes[i]), scaling);
                sideLabelSizes.push_back(sideMeasure.GetBoundingBox(dc).GetSize());
                }
        };
        measureLabels(labelScaling);

        // Edge-to-edge box for the side label of segment `index`.
        // The first bar's label sits above it, odd zigzags' labels sit to the right,
        // and even zigzags' labels sit to the left.
        const auto sideLabelRect = [&](const size_t index, const wxPoint2DDouble& segStart,
                                       const wxPoint2DDouble& segEnd, const wxSize& labelSize)
        {
            const auto labelW = static_cast<double>(labelSize.GetWidth());
            const auto labelH = static_cast<double>(labelSize.GetHeight());
            const double halfThick = lineThickness * math_constants::half;
            const double segLeft = std::min(segStart.m_x, segEnd.m_x) - halfThick;
            const double segRight = std::max(segStart.m_x, segEnd.m_x) + halfThick;
            const double segTop = std::min(segStart.m_y, segEnd.m_y) - halfThick;
            const double midX = (segStart.m_x + segEnd.m_x) * math_constants::half;
            const double midY = (segStart.m_y + segEnd.m_y) * math_constants::half;
            const double gap = minDim * LABEL_GAP_PROPORTION;
            if (index == 0)
                {
                return wxRect2DDouble{ midX - (labelW * math_constants::half),
                                       segTop - gap - labelH, labelW, labelH };
                }
            if ((index % 2) == 1)
                {
                return wxRect2DDouble{ segRight + gap, midY - (labelH * math_constants::half),
                                       labelW, labelH };
                }
            return wxRect2DDouble{ segLeft - gap - labelW, midY - (labelH * math_constants::half),
                                   labelW, labelH };
        };

        // Side label boxes for every segment, with labels on the same side pushed down
        // as needed so they don't overlap. Zigzag segments descend in order,
        // so each side's labels stack top to bottom in segment order.
        const auto layoutSideLabels =
            [&](const std::vector<std::pair<wxPoint2DDouble, wxPoint2DDouble>>& segments,
                const std::vector<wxSize>& sizes)
        {
            const size_t count = std::min(segments.size(), sizes.size());
            std::vector<wxRect2DDouble> boxes;
            boxes.reserve(count);
            for (size_t i = 0; i < count; ++i)
                {
                boxes.push_back(sideLabelRect(i, segments[i].first, segments[i].second, sizes[i]));
                }
            const double labelSpacing = minDim * LABEL_PAD_PROPORTION;
            // odd zigzags are on the right, even zigzags (after the first bar) on the left
            for (size_t firstOnSide = 1; firstOnSide <= 2; ++firstOnSide)
                {
                double previousBottom{ std::numeric_limits<double>::lowest() };
                for (size_t i = firstOnSide; i < count; i += 2)
                    {
                    boxes[i].m_y = std::max(boxes[i].m_y, previousBottom + labelSpacing);
                    previousBottom = boxes[i].m_y + boxes[i].m_height;
                    }
                }
            return boxes;
        };

        struct SimResult
            {
            bool m_ok{ false };
            double m_sweepDeg{ 0 };
            double m_minX{ 0 };
            double m_maxX{ 0 };
            double m_minY{ 0 };
            double m_maxY{ 0 };
            std::vector<std::pair<wxPoint2DDouble, wxPoint2DDouble>> m_segments;
            wxPoint2DDouble m_spiralCenter{ 0, 0 };
            double m_spiralRadius{ 0 };
            wxPoint2DDouble m_lastDiagStart{ 0, 0 };
            wxPoint2DDouble m_lastDiagEnd{ 0, 0 };
            bool m_hasLastDiag{ false };
            };

        // outer radius holding a target arc length (closed-form Archimedean)
        const auto contentRadiusFor = [&](const double targetLen)
        {
            const double spacing = radiusPerDegree * 360.0;
            return std::sqrt(safe_divide(targetLen * spacing, std::numbers::pi) +
                             (minRadius * minRadius));
        };

        // Sweep (in degrees) needed to wind a target length in from the outer radius.
        // The spiral's arc length has a closed form, so the radius where the target
        // is reached is found by bisection. Returns a negative value if the spiral
        // bottoms out at the hole first.
        const auto sweepForLength = [&](const double targetLen, const double contentR)
        {
            constexpr int RADIUS_BISECT_STEPS{ 60 };
            // radius lost per radian traveled
            const double lossPerRad = radiusPerDegree * safe_divide(180.0, std::numbers::pi);
            if (!(lossPerRad > 0))
                {
                return -1.0;
                }
            // antiderivative of sqrt(r * r + lossPerRad * lossPerRad) with respect to r
            const auto integral = [&](const double radius)
            {
                return (radius * std::hypot(radius, lossPerRad)) +
                       (lossPerRad * lossPerRad * std::asinh(safe_divide(radius, lossPerRad)));
            };
            // arc length from the outer radius in to the given radius
            const auto arcLengthTo = [&](const double radius)
            { return safe_divide(integral(contentR) - integral(radius), 2.0 * lossPerRad); };
            if (arcLengthTo(minRadius) < targetLen)
                {
                return -1.0;
                }
            // the arc length is at least the target at the hole and zero at the outer radius
            double innerRadius = minRadius;
            double outerRadius = contentR;
            for (int i = 0; i < RADIUS_BISECT_STEPS; ++i)
                {
                const double midRadius = (innerRadius + outerRadius) * math_constants::half;
                if (arcLengthTo(midRadius) >= targetLen)
                    {
                    innerRadius = midRadius;
                    }
                else
                    {
                    outerRadius = midRadius;
                    }
                }
            return safe_divide(contentR - innerRadius, radiusPerDegree);
        };

        // tries the full path at a candidate scale; reports box and sweep
        const auto simulate = [&](const double scale)
        {
            SimResult result;
            if (!(scale > 0) || !std::isfinite(scale))
                {
                return result;
                }
            const size_t nodeCount = m_nodes.size();
            std::vector<double> lengths;
            lengths.reserve(nodeCount);
            for (const auto& node : m_nodes)
                {
                const double len = node.GetValue() * scale;
                if (!std::isfinite(len) || len < 0)
                    {
                    return result;
                    }
                lengths.push_back(len);
                }

            wxPoint2DDouble pos{ 0, 0 };
            result.m_minX = 0;
            result.m_maxX = 0;
            result.m_minY = 0;
            result.m_maxY = 0;
            const auto trackPoint = [&](const wxPoint2DDouble& pt)
            {
                result.m_minX = std::min(result.m_minX, pt.m_x);
                result.m_maxX = std::max(result.m_maxX, pt.m_x);
                result.m_minY = std::min(result.m_minY, pt.m_y);
                result.m_maxY = std::max(result.m_maxY, pt.m_y);
            };

            const auto trackCircle = [&](const wxPoint2DDouble& center, const double radius)
            {
                trackPoint(wxPoint2DDouble{ center.m_x - radius, center.m_y - radius });
                trackPoint(wxPoint2DDouble{ center.m_x + radius, center.m_y + radius });
            };

            // winds a spiral holding `targetLen`, starting at the top of its circle at `entry`.
            // Returns false if it can't fit within the spiral radius budget.
            const auto windSpiral = [&](const wxPoint2DDouble& entry, const double targetLen)
            {
                const double contentR = contentRadiusFor(targetLen);
                if (!(contentR > 0) || !std::isfinite(contentR) || contentR > maxSpiralRadius)
                    {
                    return false;
                    }
                const wxPoint2DDouble center{ entry.m_x, entry.m_y + contentR };
                const double swept = sweepForLength(targetLen, contentR);
                if (!(swept > 0))
                    {
                    return false;
                    }
                result.m_spiralCenter = center;
                result.m_spiralRadius = contentR;
                result.m_sweepDeg = swept;
                trackCircle(center, contentR + lineThickness);
                if (m_showLabels)
                    {
                    // reserve room for the name beneath the spiral
                    trackPoint(wxPoint2DDouble{ center.m_x, center.m_y + contentR + lineThickness +
                                                                nameLabelHeight +
                                                                (minDim * LABEL_PAD_PROPORTION) });
                    }
                return true;
            };

            // reserves the side labels' boxes (none if the labels are hidden)
            const auto trackLabelBoxes = [&]()
            {
                for (const wxRect2DDouble& box :
                     layoutSideLabels(result.m_segments, sideLabelSizes))
                    {
                    trackPoint(box.GetLeftTop());
                    trackPoint(box.GetRightBottom());
                    }
            };

            if (nodeCount == 1)
                {
                // single node draws as a spiral on its own, sized to its length
                result.m_ok = windSpiral(wxPoint2DDouble{ 0, 0 }, lengths.front());
                return result;
                }

            // first node: horizontal
            const wxPoint2DDouble start{ 0, 0 };
            pos = wxPoint2DDouble{ lengths.front(), 0 };
            result.m_segments.emplace_back(start, pos);
            trackPoint(pos);

            // middle nodes: alternate down-left, down-right starting leftward
            for (size_t i = 1; i + 1 < nodeCount; ++i)
                {
                const wxPoint2DDouble segStart = pos;
                const double direction = ((i % 2) == 1) ? -1.0 : 1.0;
                pos = wxPoint2DDouble{ pos.m_x + (direction * lengths[i] * cosAngle),
                                       pos.m_y + (lengths[i] * sinAngle) };
                result.m_segments.emplace_back(segStart, pos);
                trackPoint(pos);
                }
            trackLabelBoxes();

            // last node: zig down toward the spiral, then wind the remainder
            const double nodeLen = lengths.back();
            const double lastDirection = (((nodeCount - 1) % 2) == 1) ? -1.0 : 1.0;
            const double dropLen = safe_divide(availH * 0.28, sinAngle);
            const double zigLen = std::min(dropLen, nodeLen * math_constants::half);
            const wxPoint2DDouble zigEnd{ pos.m_x + (lastDirection * zigLen * cosAngle),
                                          pos.m_y + (zigLen * sinAngle) };
            result.m_lastDiagStart = pos;
            result.m_lastDiagEnd = zigEnd;
            result.m_hasLastDiag = true;
            trackPoint(zigEnd);

            // spiral sized to the remaining length, starting at top of its circle
            if (!windSpiral(zigEnd, nodeLen - zigLen))
                {
                return result;
                }
            // straight segments paint half a thickness around their
            // centerlines; expand the fitted box to cover it
            const double halfThickPad = lineThickness * math_constants::half;
            result.m_minX -= halfThickPad;
            result.m_maxX += halfThickPad;
            result.m_minY -= halfThickPad;
            result.m_maxY += halfThickPad;
            result.m_ok = true;
            return result;
        };

        const auto fitsBox = [&](const SimResult& res)
        {
            return res.m_ok && ((res.m_maxX - res.m_minX) <= availW) &&
                   ((res.m_maxY - res.m_minY) <= availH);
        };

        // longest spiral arc that fits inside the radius budget for a
        // candidate thickness (closed form Archimedean capacity)
        const auto spiralCapacity = [&](const double thick)
        {
            const double spacing = spacingFor(thick);
            const double hole = holeFor(thick);
            if (!(maxSpiralRadius > hole) || !(spacing > 0))
                {
                return 0.0;
                }
            return (std::numbers::pi * ((maxSpiralRadius * maxSpiralRadius) - (hole * hole))) /
                   spacing;
        };

        double highScale{ 0 };
        if (m_nodes.size() == 1)
            {
            // a lone node is all spiral, so scale from capacity
            highScale = safe_divide(spiralCapacity(lineThickness), m_nodes.back().GetValue());
            }
        else
            {
            // first group takes half the width; thin the line until
            // the spiral holds the final value
            const double firstValue = m_nodes.front().GetValue();
            double targetScale = safe_divide(availW * math_constants::half, firstValue);
            if (!(targetScale > 0) || !std::isfinite(targetScale))
                {
                return;
                }
            const double lastValue = m_nodes.back().GetValue();
            const double requiredLen = lastValue * targetScale;
            const double minThick = minDim * THINNEST_LINE_PROPORTION;
            // keep the thickest line that still holds the required length
            double feasibleThick = lineThickness;
            if (spiralCapacity(lineThickness) < requiredLen)
                {
                feasibleThick = -1.0;
                double lowThick = minThick;
                double searchHigh = lineThickness;
                for (int i = 0; i < THICKNESS_SEARCH_STEPS; ++i)
                    {
                    const double midThick = (lowThick + searchHigh) * math_constants::half;
                    if (spiralCapacity(midThick) >= requiredLen)
                        {
                        feasibleThick = midThick;
                        lowThick = midThick;
                        }
                    else
                        {
                        searchHigh = midThick;
                        }
                    }
                }
            if (feasibleThick < 0)
                {
                // thinnest line still can't hold it: shrink the scale to fit
                feasibleThick = minThick;
                targetScale *= safe_divide(spiralCapacity(minThick), requiredLen);
                }
            applyThickness(feasibleThick);
            highScale = targetScale;
            }
        if (!(highScale > 0) || !std::isfinite(highScale))
            {
            return;
            }
        // backs off until the zigzag fits, then bisects up to the largest fit
        // (the result doesn't fit if nothing does)
        const auto findLargestFit = [&](double scale)
        {
            SimResult found = simulate(scale);
            int shrinkSteps{ 0 };
            while (!fitsBox(found) && shrinkSteps < SCALE_SHRINK_STEPS)
                {
                scale *= math_constants::half;
                found = simulate(scale);
                ++shrinkSteps;
                }
            if (!fitsBox(found))
                {
                return found;
                }
            double lowScale = scale;
            double searchHigh = scale * 2.0;
            for (int i = 0; i < SCALE_GROWTH_STEPS && fitsBox(simulate(searchHigh)); ++i)
                {
                lowScale = searchHigh;
                searchHigh *= 2.0;
                }
            found = simulate(lowScale);
            for (int i = 0; i < SCALE_BISECT_STEPS; ++i)
                {
                const double midScale = (lowScale + searchHigh) * math_constants::half;
                const SimResult mid = simulate(midScale);
                if (fitsBox(mid))
                    {
                    lowScale = midScale;
                    found = mid;
                    }
                else
                    {
                    searchHigh = midScale;
                    }
                }
            return found;
        };

        SimResult fit = findLargestFit(highScale);
        // The label boxes don't shrink along with the geometry, so if nothing fits
        // (e.g., wide labels on a narrow plot), shrink the labels and try again.
        for (int i = 0; i < LABEL_SHRINK_STEPS && m_showLabels && !fitsBox(fit); ++i)
            {
            labelScaling *= LABEL_SHRINK_FACTOR;
            measureLabels(labelScaling);
            fit = findLargestFit(highScale);
            }
        if (!fitsBox(fit))
            {
            return;
            }

        // center the simulated path inside the plot area
        const double boxW = fit.m_maxX - fit.m_minX;
        const double boxH = fit.m_maxY - fit.m_minY;
        const double offsetX = static_cast<double>(plotArea.GetLeft()) + pad +
                               ((availW - boxW) * math_constants::half) - fit.m_minX;
        const double offsetY = static_cast<double>(plotArea.GetTop()) + pad +
                               ((availH - boxH) * math_constants::half) - fit.m_minY;
        const auto toScreen = [&](const wxPoint2DDouble& pt)
        { return wxPoint{ wxRound(pt.m_x + offsetX), wxRound(pt.m_y + offsetY) }; };
        // adds one straight segment in screen coordinates
        const auto addStraight =
            [&](const wxPoint2DDouble& from, const wxPoint2DDouble& to, const wxColour& color)
        {
            AddObject(std::make_unique<StraightSegment>(toScreen(from), toScreen(to), lineThickness,
                                                        color));
        };

        // render in reverse so each segment tucks under its predecessor,
        // hiding the corner overlaps
        const size_t nodeCount = m_nodes.size();
        // spiral (or the single-node spiral)
        const wxPoint spiralCenter = toScreen(fit.m_spiralCenter);
        AddObject(std::make_unique<SpiralSegment>(spiralCenter, 270.0, fit.m_sweepDeg,
                                                  fit.m_spiralRadius, radiusPerDegree,
                                                  lineThickness, m_nodes.back().GetColor()));
        if (fit.m_hasLastDiag)
            {
            addStraight(fit.m_lastDiagStart, fit.m_lastDiagEnd, m_nodes.back().GetColor());
            }
        if (nodeCount > 1)
            {
            for (size_t back = fit.m_segments.size(); back > 0; --back)
                {
                const auto& seg = fit.m_segments[back - 1];
                addStraight(seg.first, seg.second, m_nodes[back - 1].GetColor());
                }
            }

        // straight-segment labels sit outside the line, and the spiral holds
        // its value with its name beneath
        if (m_showLabels)
            {
            const double labelGap =
                (minDim * LABEL_GAP_PROPORTION) + (lineThickness * math_constants::half);
            // measure all labels at the full scaling; find one common factor
            // that keeps every label inside the plot area
            const double fullScaling = labelScaling;
            double smallScaling = fullScaling;
                {
                // the side labels and the name were already measured at full scaling
                double maxLabelHeight = nameLabelHeight;
                for (const wxSize& labelSize : sideLabelSizes)
                    {
                    maxLabelHeight =
                        std::max(maxLabelHeight, static_cast<double>(labelSize.GetHeight()));
                    }
                GraphItems::Label valueMeasure =
                    createPlainLabel(FormatValue(m_nodes.back().GetValue()), fullScaling);
                maxLabelHeight =
                    std::max(maxLabelHeight,
                             static_cast<double>(valueMeasure.GetBoundingBox(dc).GetHeight()));
                if (maxLabelHeight > 0 && maxLabelHeight > availH)
                    {
                    smallScaling = fullScaling * safe_divide(availH, maxLabelHeight);
                    }
                }
            const auto makeLabel =
                [&](const wxString& text, const wxPoint& anchor, const double scaling)
            {
                auto label = std::make_unique<GraphItems::Label>(createPlainLabel(text, scaling));
                label->SetAnchoring(Anchoring::Center);
                label->SetAnchorPoint(anchor);
                return label;
            };
            // The value label takes the common plot-height fit, further shrunk so
            // that it fits inside the spiral hole. Only this label shrinks for the hole.
            double valueScaling = smallScaling;
                {
                GraphItems::Label valueMeasure =
                    createPlainLabel(FormatValue(m_nodes.back().GetValue()), smallScaling);
                const wxSize valueSize = valueMeasure.GetBoundingBox(dc).GetSize();
                const double availD =
                    (minRadius * 2.0) - lineThickness - (minDim * LABEL_PAD_PROPORTION);
                if (valueSize.GetWidth() > 0 && valueSize.GetHeight() > 0 && availD > 0)
                    {
                    valueScaling =
                        smallScaling *
                        std::min(
                            1.0,
                            std::min(
                                safe_divide(availD, static_cast<double>(valueSize.GetWidth())),
                                safe_divide(availD, static_cast<double>(valueSize.GetHeight()))));
                    }
                }
            if (nodeCount > 1)
                {
                std::vector<std::unique_ptr<GraphItems::Label>> sideLabels;
                std::vector<wxSize> sideSizes;
                sideLabels.reserve(nodeCount - 1);
                sideSizes.reserve(nodeCount - 1);
                for (size_t i = 0; i + 1 < nodeCount; ++i)
                    {
                    sideLabels.push_back(
                        makeLabel(FormatNodeText(m_nodes[i]), wxPoint{}, smallScaling));
                    sideSizes.push_back(sideLabels.back()->GetBoundingBox(dc).GetSize());
                    }
                const auto boxes = layoutSideLabels(fit.m_segments, sideSizes);
                for (size_t i = 0; i < sideLabels.size(); ++i)
                    {
                    sideLabels[i]->SetAnchorPoint(toScreen(boxes[i].GetCentre()));
                    AddObject(std::move(sideLabels[i]));
                    }
                }
            AddObject(
                makeLabel(FormatValue(m_nodes.back().GetValue()), spiralCenter, valueScaling));
            if (!m_nodes.back().GetLabel().empty())
                {
                const wxPoint2DDouble namePos{ fit.m_spiralCenter.m_x,
                                               fit.m_spiralCenter.m_y + fit.m_spiralRadius +
                                                   (lineThickness * math_constants::half) +
                                                   labelGap };
                AddObject(makeLabel(m_nodes.back().GetLabel(), toScreen(namePos), smallScaling));
                }
            }
        }

    //----------------------------------------------------------------
    void DuBoisSpiralChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A Du Bois spiral chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");

        for (const auto& node : m_nodes)
            {
            label += L". " + FormatNodeText(node, false);
            }

        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
