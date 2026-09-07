///////////////////////////////////////////////////////////////////////////////
// Name:        nightingale_rose_chart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "nightingale_rose_chart.h"
#include "../base/colorbrewer.h"
#include "../base/shapes.h"
#include "../math/mathematics.h"
#include "../math/safe_math.h"
#include <algorithm>
#include <cmath>
#include <wx/log.h>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::NightingaleRoseChart, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    NightingaleRoseChart::RoseWedge::RoseWedge(
        const wxPoint& center, const double startAngleDeg, const double endAngleDeg,
        const double innerRadius, const double outerRadius, const wxBrush& brush, const wxPen& pen)
        : m_center(center), m_startAngle(startAngleDeg), m_endAngle(endAngleDeg),
          m_innerRadius(innerRadius), m_outerRadius(outerRadius)
        {
        GetGraphItemInfo().Brush(brush).Pen(pen);
        }

    //----------------------------------------------------------------
    std::vector<wxPoint> NightingaleRoseChart::RoseWedge::GetPolygon() const
        {
        std::vector<wxPoint> points;
        // Approximate the arcs with chords. This feeds hit-testing and the dotted
        // selection outline. The visible fill is drawn from true arcs in Draw().
        const double angleSpan = m_endAngle - m_startAngle;
        const auto stepCount =
            static_cast<size_t>(std::isfinite(angleSpan) ?
                                    std::clamp(std::ceil(std::abs(angleSpan) * 2.0), 2.0, 720.0) :
                                    2.0);

        const auto pointAt = [this](const double radius, const double angleDeg)
        {
            const double angleRad = geometry::degrees_to_radians(angleDeg);
            return wxPoint{ wxRound(m_center.x + (radius * std::cos(angleRad))),
                            wxRound(m_center.y + (radius * std::sin(angleRad))) };
        };

        points.reserve((stepCount + 1) * 2);
        for (size_t step = 0; step <= stepCount; ++step)
            {
            const double angle =
                m_startAngle + ((m_endAngle - m_startAngle) * safe_divide<double>(step, stepCount));
            points.push_back(pointAt(m_outerRadius, angle));
            }

        if (m_innerRadius <= 1.0)
            {
            points.push_back(m_center);
            }
        else
            {
            for (size_t step = 0; step <= stepCount; ++step)
                {
                const double angle = m_endAngle - ((m_endAngle - m_startAngle) *
                                                   safe_divide<double>(step, stepCount));
                points.push_back(pointAt(m_innerRadius, angle));
                }
            }

        return points;
        }

    //----------------------------------------------------------------
    wxRect NightingaleRoseChart::RoseWedge::GetBoundingBox([[maybe_unused]]
                                                           wxDC &
                                                           dc) const
        {
        const auto penWidth = GetPen().IsOk() ? GetPen().GetWidth() : 1;
        // small cushion beyond the pen width to cover the fill bleed from Draw()
        const auto extent = wxRound(m_outerRadius + ScaleToScreenAndCanvas(penWidth) +
                                    ScaleToScreenAndCanvas(1.0)) +
                            1;
        return wxRect{ wxPoint{ m_center.x - extent, m_center.y - extent },
                       wxSize{ extent * 2, extent * 2 } };
        }

    //----------------------------------------------------------------
    wxRect NightingaleRoseChart::RoseWedge::Draw(wxDC & dc) const
        {
        const auto boundingBox = GetBoundingBox(dc);
        const GraphItems::GraphicsContextFallback gcf{ &dc, boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return boundingBox;
            }

        // Bleed the fill and outline outward slightly so neighboring wedges overlap.
        // Each wedge is anti-aliased on its own, so an exact shared edge leaves a
        // seam of background color between them.
        const double radiusBleed = ScaleToScreenAndCanvas(0.75);
        const double angleBleed = geometry::radians_to_degrees(
            safe_divide<double>(radiusBleed, std::max(m_outerRadius, 1.0)));
        const double startAngle = m_startAngle - angleBleed;
        const double endAngle = m_endAngle + angleBleed;
        const double outerRadius = m_outerRadius + radiusBleed;
        const double innerRadius =
            (m_innerRadius > 1.0) ? std::max(m_innerRadius - radiusBleed, 0.0) : 0.0;

        const double startRad = geometry::degrees_to_radians(startAngle);
        const double endRad = geometry::degrees_to_radians(endAngle);

        const auto pointAt = [this](const double radius, const double angleRad)
        {
            return wxPoint2DDouble(m_center.x + (radius * std::cos(angleRad)),
                                   m_center.y + (radius * std::sin(angleRad)));
        };

        // build the wedge from true circular arcs, like wxDC::DrawEllipticArc does
        wxGraphicsPath path = gc->CreatePath();
        if (innerRadius <= 0.0)
            {
            path.MoveToPoint(m_center.x, m_center.y);
            const auto outerStart = pointAt(outerRadius, startRad);
            path.AddLineToPoint(outerStart.m_x, outerStart.m_y);
            path.AddArc(m_center.x, m_center.y, outerRadius, startRad, endRad, true);
            path.CloseSubpath();
            }
        else
            {
            const auto outerStart = pointAt(outerRadius, startRad);
            path.MoveToPoint(outerStart.m_x, outerStart.m_y);
            path.AddArc(m_center.x, m_center.y, outerRadius, startRad, endRad, true);
            const auto innerEnd = pointAt(innerRadius, endRad);
            path.AddLineToPoint(innerEnd.m_x, innerEnd.m_y);
            path.AddArc(m_center.x, m_center.y, innerRadius, endRad, startRad, false);
            path.CloseSubpath();
            }

        gc->SetBrush(gc->CreateBrush(GetGraphItemInfo().GetBrush()));
        if (GetPen().IsOk())
            {
            gc->SetPen(gc->CreatePen(wxGraphicsPenInfo{
                GetPen().GetColour(), ScaleToScreenAndCanvas(GetPen().GetWidth()) }
                                         .Join(wxJOIN_ROUND)));
            }
        else
            {
            gc->SetPen(*wxTRANSPARENT_PEN);
            }
        gc->DrawPath(path);

        if (IsSelected())
            {
            const auto polygon = GetPolygon();
            wxGraphicsPath selectionPath = gc->CreatePath();
            selectionPath.MoveToPoint(polygon.front().x, polygon.front().y);
            for (size_t i = 1; i < polygon.size(); ++i)
                {
                selectionPath.AddLineToPoint(polygon[i].x, polygon[i].y);
                }
            selectionPath.CloseSubpath();
            gc->SetPen(gc->CreatePen(
                wxGraphicsPenInfo{ *wxBLACK, ScaleToScreenAndCanvas(2) }.Style(wxPENSTYLE_DOT)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->StrokePath(selectionPath);
            }

        return boundingBox;
        }

    //----------------------------------------------------------------
    bool NightingaleRoseChart::RoseWedge::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        return geometry::is_inside_polygon(pt, GetPolygon());
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::RoseWedge::Offset(const int xOffset, const int yOffset)
        {
        m_center.x += xOffset;
        m_center.y += yOffset;
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::RoseWedge::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                         [[maybe_unused]] wxDC& dc,
                                                         [[maybe_unused]] const double scaling)
        {
        }

    //----------------------------------------------------------------
    NightingaleRoseChart::NightingaleRoseChart(
        Canvas * canvas,
        const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes /*= nullptr*/,
        const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetBrushScheme(brushes != nullptr ? brushes :
                                            std::make_shared<Brushes::Schemes::BrushScheme>(
                                                Settings::GetDefaultColorScheme()));
        SetColorScheme(colors);

        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);

        GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::White) };
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::SetStartAngle(double angleDeg) noexcept
        {
        if (!std::isfinite(angleDeg))
            {
            return;
            }
        m_startAngle = std::fmod(std::fmod(angleDeg, 360.0) + 360.0, 360.0);
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::GhostWedge(const wxString& groupLabel, const wxString& categoryLabel)
        {
        m_ghostedWedges.emplace_back(groupLabel, categoryLabel);
        }

    //----------------------------------------------------------------
    bool NightingaleRoseChart::IsWedgeGhosted(const wxString& categoryLabel,
                                              const wxString& groupLabel) const
        {
        return std::ranges::any_of(
            m_ghostedWedges,
            [&](const auto& ghostSpec)
            {
                return Data::CmpNoCaseIgnoreControlChars(ghostSpec.first, groupLabel) == 0 &&
                       (ghostSpec.second.empty() ||
                        Data::CmpNoCaseIgnoreControlChars(ghostSpec.second, categoryLabel) == 0);
            });
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::SetData(
        const std::shared_ptr<const Data::Dataset>& data,
        const std::optional<wxString>& aggregateColumnName, const wxString& categoryColumnName,
        const std::optional<wxString>& groupColumnName /*= std::nullopt*/)
        {
        if (data == nullptr)
            {
            return;
            }
        GetSelectedIds().clear();
        m_slices.clear();
        m_wedgeCount = 0;
        ResetGrouping();

        SetDataset(data);

        m_categoryColumnName = categoryColumnName;
        m_aggregateColumnName = aggregateColumnName.value_or(wxString{});

        const auto categoryColumn = GetCategoricalColumn(m_categoryColumnName);
        const auto aggregateColumn =
            aggregateColumnName.has_value() ?
                std::optional(GetContinuousColumn(aggregateColumnName.value())) :
                std::nullopt;

        SetGroupColumn(groupColumnName);
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            }
        const auto groupColumn = IsUsingGrouping() ? std::optional(GetGroupColumn()) : std::nullopt;

        // category ID -> (series ID -> aggregated value), keyed (and later iterated)
        // in category and series code order
        std::map<Data::GroupIdType, std::map<Data::GroupIdType, double>> accum;
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            const double val =
                aggregateColumn.has_value() ? aggregateColumn.value()->GetValue(i) : 1.0;

            if (!std::isfinite(val) || val < 0)
                {
                continue;
                }
            const auto categoryId = categoryColumn->GetValue(i);
            const auto seriesId = groupColumn.has_value() ? groupColumn.value()->GetValue(i) : 0;
            accum[categoryId][seriesId] += val;
            }

        // a rose chart becomes unreadable (and slow to hit-test) well before this
        // many wedges, so cap the slice count and note the truncation
        constexpr size_t MAX_SLICES{ 100 };

        m_slices.reserve(std::min(accum.size(), MAX_SLICES));
        for (const auto& [categoryId, seriesMap] : accum)
            {
            if (m_slices.size() >= MAX_SLICES)
                {
                break;
                }
            SliceInfo slice;
            slice.m_categoryId = categoryId;
            slice.m_label = categoryColumn->GetLabelFromID(categoryId);
            slice.m_seriesValues.reserve(seriesMap.size());
            for (const auto& [seriesId, value] : seriesMap)
                {
                slice.m_seriesValues.emplace_back(seriesId, value);
                }
            m_slices.push_back(std::move(slice));
            }

        if (accum.size() > MAX_SLICES)
            {
            wxLogWarning(L"Nightingale rose chart limited to %zu slices; "
                         L"%zu categories truncated.",
                         MAX_SLICES, accum.size() - MAX_SLICES);
            GetCaption().SetText(wxString::Format(
                _(L"Note: only the first %zu categories are being displayed."), MAX_SLICES));
            }
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::RecalcSizes(wxDC & dc)
        {
        GroupGraph2D::RecalcSizes(dc);

        if (m_slices.empty())
            {
            return;
            }
        m_wedgeCount = 0;

        const auto plotArea = GetPlotAreaBoundingBox();
        const wxPoint center{ plotArea.GetLeft() + plotArea.GetWidth() / 2,
                              plotArea.GetTop() + plotArea.GetHeight() / 2 };
        const double outerRadius =
            std::min(plotArea.GetWidth(), plotArea.GetHeight()) *
            (m_showLabels ? m_labeledRadiusProportion : m_plainRadiusProportion);

        const double anglePerSlice =
            safe_divide<double>(360.0, static_cast<double>(m_slices.size()));

        double maxScaleValue{ 0 };
        for (const auto& slice : m_slices)
            {
            if (m_seriesDisplay == SeriesDisplay::Overlaid)
                {
                for (const auto& seriesValue : slice.m_seriesValues)
                    {
                    maxScaleValue = std::max(maxScaleValue, seriesValue.second);
                    }
                }
            else
                {
                maxScaleValue = std::max(maxScaleValue, slice.GetTotal());
                }
            }
        if (maxScaleValue <= 0)
            {
            return;
            }

        const auto radiusForValue = [&](const double value) -> double
        {
            if (!std::isfinite(value) || value <= 0)
                {
                return 0.0;
                }
            const double ratio = std::clamp(safe_divide<double>(value, maxScaleValue), 0.0, 1.0);
            return outerRadius *
                   (m_radialScaling == RadialScaling::AreaProportional ? std::sqrt(ratio) : ratio);
        };

        const auto brushCount = GetBrushScheme() ? GetBrushScheme()->GetBrushes().size() : 0;

        // faint leader lines from a too-small slice out to its perimeter-pushed label
        auto labelConnectorLines = std::make_unique<GraphItems::Lines>(
            wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::LightGray), 2,
                   wxPenStyle::wxPENSTYLE_DOT },
            GetScaling());

        for (size_t sliceIdx = 0; sliceIdx < m_slices.size(); ++sliceIdx)
            {
            const auto& slice = m_slices[sliceIdx];
            const double sliceStart =
                m_startAngle + (static_cast<double>(sliceIdx) * anglePerSlice);
            const double sliceEnd = sliceStart + anglePerSlice;

            const auto brushForSeries = [&](const Data::GroupIdType seriesId) -> wxBrush
            {
                const size_t brushIdx =
                    IsUsingGrouping() ? GetSchemeIndexFromGroupId(seriesId) : sliceIdx;
                // an explicit color scheme takes precedence over the brush scheme,
                // which may use hatch patterns
                wxBrush wedgeBrush =
                    GetColorScheme() ?
                        wxBrush{ GetColorScheme()->GetRecycledColor(brushIdx) } :
                    GetBrushScheme() ?
                        GetBrushScheme()->GetBrush(safe_modulus(brushIdx, brushCount)) :
                        wxBrush{ *wxBLUE };
                if (!m_ghostedWedges.empty() && IsUsingGrouping() &&
                    IsWedgeGhosted(slice.m_label, GetGroupColumn()->GetLabelFromID(seriesId)))
                    {
                    wedgeBrush.SetColour(Colors::ColorContrast::ChangeOpacity(
                        wedgeBrush.GetColour(), m_ghostOpacity));
                    }
                return wedgeBrush;
            };

            // how far this slice's own wedges reach, so a label can hug it
            // instead of the chart's overall outer radius
            double sliceReachRadius{ 0 };

            if (m_seriesDisplay == SeriesDisplay::Overlaid)
                {
                struct OverlaidWedge
                    {
                    double m_radius{ 0 };
                    Data::GroupIdType m_seriesId{ 0 };
                    wxBrush m_brush;
                    };

                std::vector<OverlaidWedge> wedges;
                wedges.reserve(slice.m_seriesValues.size());
                for (const auto& [seriesId, value] : slice.m_seriesValues)
                    {
                    wedges.push_back({ radiusForValue(value), seriesId, brushForSeries(seriesId) });
                    }
                std::sort(wedges.begin(), wedges.end(),
                          [](const auto& first, const auto& second) noexcept
                          {
                              return (first.m_radius != second.m_radius) ?
                                         (first.m_radius > second.m_radius) :
                                         (first.m_seriesId < second.m_seriesId);
                          });
                for (const auto& wedge : wedges)
                    {
                    AddObject(std::make_unique<RoseWedge>(center, sliceStart, sliceEnd, 0.0,
                                                          wedge.m_radius, wedge.m_brush, GetPen()));
                    ++m_wedgeCount;
                    sliceReachRadius = std::max(sliceReachRadius, wedge.m_radius);
                    }
                }
            else // Stacked
                {
                double cumulative{ 0 };
                for (const auto& [seriesId, value] : slice.m_seriesValues)
                    {
                    const double innerRadius = radiusForValue(cumulative);
                    cumulative += value;
                    const double outerWedgeRadius = radiusForValue(cumulative);
                    AddObject(std::make_unique<RoseWedge>(center, sliceStart, sliceEnd, innerRadius,
                                                          outerWedgeRadius,
                                                          brushForSeries(seriesId), GetPen()));
                    ++m_wedgeCount;
                    sliceReachRadius = std::max(sliceReachRadius, outerWedgeRadius);
                    }
                }

            if (m_showLabels)
                {
                const double midAngle = sliceStart + (anglePerSlice / 2.0);
                // Place the label just past the slice's own outer edge.
                // A slice that barely reaches out from the center would crowd its label against
                // its neighbors, so when its reach is below this fraction of the chart radius,
                // push the label out to the perimeter instead.
                // This matches the ring of labels around Nightingale's original plate.
                constexpr double minReachProportion{ 0.35 };
                const bool pushedOutToPerimeter =
                    sliceReachRadius < outerRadius * minReachProportion;
                const double labelRadius = (pushedOutToPerimeter ? outerRadius : sliceReachRadius) +
                                           ScaleToScreenAndCanvas(m_labelGapDIPs);
                const double midAngleRad = geometry::degrees_to_radians(midAngle);
                const wxPoint anchorPt{ wxRound(center.x + (labelRadius * std::cos(midAngleRad))),
                                        wxRound(center.y + (labelRadius * std::sin(midAngleRad))) };

                if (pushedOutToPerimeter)
                    {
                    const double lineStartRadius =
                        std::max(sliceReachRadius, ScaleToScreenAndCanvas(4.0));
                    const wxPoint lineStart{
                        wxRound(center.x + (lineStartRadius * std::cos(midAngleRad))),
                        wxRound(center.y + (lineStartRadius * std::sin(midAngleRad)))
                    };
                    labelConnectorLines->AddLine(lineStart, anchorPt);
                    }

                const double cosineOfAngle = std::cos(midAngleRad);
                const Anchoring anchoring = (cosineOfAngle > 0.3)  ? Anchoring::TopLeftCorner :
                                            (cosineOfAngle < -0.3) ? Anchoring::TopRightCorner :
                                                                     Anchoring::Center;

                auto label = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo{ slice.m_label }
                        .Scaling(GetScaling() * m_labelScaling)
                        .DPIScaling(GetDPIScaleFactor())
                        .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                        .Anchoring(anchoring)
                        .AnchorPoint(anchorPt));
                label->SetShape(LabelShape::NoShape);
                label->SetShadowType(ShadowType::NoDisplay);
                AddObject(std::move(label));
                }
            }

        if (!labelConnectorLines->GetLines().empty())
            {
            AddObject(std::move(labelConnectorLines));
            }
        }

    //----------------------------------------------------------------
    void NightingaleRoseChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A Nightingale rose chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");

        for (const auto& slice : m_slices)
            {
            label += L". " + wxString::Format(
                                 L"%s: %s", slice.m_label,
                                 wxNumberFormatter::ToString(slice.GetTotal(), 0,
                                                             wxNumberFormatter::Style::Style_None));
            if (IsUsingGrouping())
                {
                for (const auto& [seriesId, value] : slice.m_seriesValues)
                    {
                    if (value <= 0)
                        {
                        continue;
                        }
                    label +=
                        wxString::Format(L" (%s: %s)", GetGroupColumn()->GetLabelFromID(seriesId),
                                         wxNumberFormatter::ToString(
                                             value, 0, wxNumberFormatter::Style::Style_None));
                    }
                }
            }

        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
