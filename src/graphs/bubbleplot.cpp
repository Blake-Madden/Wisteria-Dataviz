///////////////////////////////////////////////////////////////////////////////
// Name:        bubbleplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "bubbleplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::BubblePlot, Wisteria::Graphs::ScatterPlot)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    BubblePlot::BubblePlot(Canvas * canvas,
                           const std::shared_ptr<Colors::Schemes::ColorScheme>& colors,
                           const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes,
                           const std::shared_ptr<LineStyleScheme>& regressionLineStyles)
        : ScatterPlot(canvas, colors, shapes, regressionLineStyles)
        {
        }

    //----------------------------------------------------------------
    void BubblePlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                             const wxString& yColumnName, const wxString& xColumnName,
                             const wxString& sizeColumnName,
                             const std::optional<wxString>& groupColumnName)
        {
        // call parent SetData to set up axes, grouping, regression, etc.
        ScatterPlot::SetData(data, yColumnName, xColumnName, groupColumnName);

        m_sizeColumnName = sizeColumnName;
        m_sizeMin = 0;
        m_sizeMax = 0;

        // validate size column exists and calculate min/max
        if (GetDataset() != nullptr)
            {
            const auto sizeColumn = GetContinuousColumn(m_sizeColumnName);
            m_sizeMin = std::numeric_limits<double>::max();
            m_sizeMax = std::numeric_limits<double>::lowest();
            for (const auto& val : sizeColumn->GetValues())
                {
                if (std::isfinite(val))
                    {
                    m_sizeMin = std::min(m_sizeMin, val);
                    m_sizeMax = std::max(m_sizeMax, val);
                    }
                }
            // handle edge case where no valid values found
            if (m_sizeMin > m_sizeMax)
                {
                m_sizeMin = 0;
                m_sizeMax = 0;
                }
            }
        }

    //----------------------------------------------------------------
    void BubblePlot::RecalcSizes(wxDC & dc)
        {
        // call Graph2D::RecalcSizes, not ScatterPlot::RecalcSizes
        // because we need to draw points with variable sizes
        Graph2D::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto groupColumn = GetGroupColumn();
        const auto xColumn = GetContinuousColumn(GetXColumnName());
        const auto yColumn = GetContinuousColumn(GetYColumnName());
        const auto sizeColumn = GetContinuousColumn(m_sizeColumnName);

        // find min/max size values for scaling (excluding NaN)
        double sizeMin = std::numeric_limits<double>::max();
        double sizeMax = std::numeric_limits<double>::lowest();
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            const double sizeVal = sizeColumn->GetValue(i);
            if (std::isfinite(sizeVal))
                {
                sizeMin = std::min(sizeMin, sizeVal);
                sizeMax = std::max(sizeMax, sizeVal);
                }
            }

        // handle edge case where all values are the same
        const bool uniformSize = (sizeMin >= sizeMax);

        // calculate area range for scaling
        // area = π * r², so r = sqrt(area / π)
        // we scale areas proportionally, then compute radii
        const double minArea =
            static_cast<double>(m_minBubbleRadius) * static_cast<double>(m_minBubbleRadius);
        const double maxArea =
            static_cast<double>(m_maxBubbleRadius) * static_cast<double>(m_maxBubbleRadius);

        for (const auto& series : GetSeriesList())
            {
            // draw regression line and confidence bands first (via parent logic)
            if (IsShowingRegressionLines() && series.GetRegressionResults().is_valid())
                {
                const auto& stats = series.GetRegressionResults();
                const auto [xAxisMin, xAxisMax] = GetBottomXAxis().GetRange();
                const auto [yAxisMin, yAxisMax] = GetLeftYAxis().GetRange();

                // draw confidence bands if enabled
                if (IsShowingConfidenceBands() && stats.n > 2 &&
                    std::isfinite(stats.standard_error) && std::isfinite(stats.mean_x) &&
                    std::isfinite(stats.ss_xx) && stats.ss_xx > 0)
                    {
                    const double alpha = 1.0 - GetConfidenceLevel();
                    const double df = static_cast<double>(stats.n) - 2.0;
                    const double tCritical =
                        statistics::t_distribution_quantile(1.0 - alpha * 0.5, df);

                    if (std::isfinite(tCritical))
                        {
                        constexpr size_t numPoints{ 50 };
                        std::vector<wxPoint> upperBand;
                        std::vector<wxPoint> lowerBand;
                        upperBand.reserve(numPoints);
                        lowerBand.reserve(numPoints);

                        const double xStep =
                            safe_divide<double>(xAxisMax - xAxisMin, numPoints - 1);
                        const double nRecip =
                            safe_divide<double>(1.0, static_cast<double>(stats.n));

                        for (size_t i = 0; i < numPoints; ++i)
                            {
                            const double x = xAxisMin + static_cast<double>(i) * xStep;
                            const double yPred = stats.slope * x + stats.intercept;

                            const double xDev = x - stats.mean_x;
                            const double seY =
                                stats.standard_error *
                                std::sqrt(nRecip + safe_divide<double>(xDev * xDev, stats.ss_xx));

                            const double margin = tCritical * seY;
                            double yUpper = yPred + margin;
                            double yLower = yPred - margin;

                            yUpper = std::clamp(yUpper, yAxisMin, yAxisMax);
                            yLower = std::clamp(yLower, yAxisMin, yAxisMax);

                            wxPoint ptUpper, ptLower;
                            if (GetPhysicalCoordinates(x, yUpper, ptUpper) &&
                                GetPhysicalCoordinates(x, yLower, ptLower))
                                {
                                upperBand.push_back(ptUpper);
                                lowerBand.push_back(ptLower);
                                }
                            }

                        if (upperBand.size() >= 2)
                            {
                            std::vector<wxPoint> bandPolygon;
                            bandPolygon.reserve(upperBand.size() + lowerBand.size());
                            bandPolygon.insert(bandPolygon.end(), upperBand.begin(),
                                               upperBand.end());
                            bandPolygon.insert(bandPolygon.end(), lowerBand.rbegin(),
                                               lowerBand.rend());

                            auto confidenceBand = std::make_unique<GraphItems::Polygon>(
                                GraphItems::GraphItemInfo{}
                                    .Pen(wxPen{ wxColour{ 128, 128, 128, 128 }, 1 })
                                    .Brush(wxBrush{ wxColour{ 128, 128, 128, 64 } })
                                    .Scaling(GetScaling()),
                                bandPolygon);
                            confidenceBand->SetDPIScaleFactor(GetDPIScaleFactor());
                            AddObject(std::move(confidenceBand));
                            }
                        }
                    }

                // draw regression line
                double x1 = xAxisMin;
                double x2 = xAxisMax;
                double y1 = stats.slope * x1 + stats.intercept;
                double y2 = stats.slope * x2 + stats.intercept;

                if (stats.slope != 0.0)
                    {
                    if (y1 < yAxisMin)
                        {
                        x1 = safe_divide<double>(yAxisMin - stats.intercept, stats.slope);
                        y1 = yAxisMin;
                        }
                    else if (y1 > yAxisMax)
                        {
                        x1 = safe_divide<double>(yAxisMax - stats.intercept, stats.slope);
                        y1 = yAxisMax;
                        }
                    if (y2 < yAxisMin)
                        {
                        x2 = safe_divide<double>(yAxisMin - stats.intercept, stats.slope);
                        y2 = yAxisMin;
                        }
                    else if (y2 > yAxisMax)
                        {
                        x2 = safe_divide<double>(yAxisMax - stats.intercept, stats.slope);
                        y2 = yAxisMax;
                        }
                    }

                wxPoint startPt, endPt;
                if (GetPhysicalCoordinates(x1, y1, startPt) &&
                    GetPhysicalCoordinates(x2, y2, endPt))
                    {
                    auto regressionLine = std::make_unique<GraphItems::Lines>(
                        series.GetRegressionPen(), GetScaling());
                    regressionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                    regressionLine->SetLineStyle(series.GetRegressionLineStyle());
                    regressionLine->AddLine(startPt, endPt);
                    AddObject(std::move(regressionLine));
                    }
                }

            // draw bubble points
            auto points = std::make_unique<GraphItems::Points2D>(wxNullPen);
            points->SetScaling(GetScaling());
            points->SetDPIScaleFactor(GetDPIScaleFactor());
            points->Reserve(GetDataset()->GetRowCount());

            wxPoint pt;
            for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
                {
                // skip if from different group
                if (IsUsingGrouping() && groupColumn->GetValue(i) != series.GetGroupId())
                    {
                    continue;
                    }
                // skip invalid data (including invalid size)
                if (!std::isfinite(xColumn->GetValue(i)) || !std::isfinite(yColumn->GetValue(i)) ||
                    !std::isfinite(sizeColumn->GetValue(i)))
                    {
                    continue;
                    }
                if (!GetPhysicalCoordinates(xColumn->GetValue(i), yColumn->GetValue(i), pt))
                    {
                    continue;
                    }

                // calculate bubble radius based on size value
                // scale area proportionally, then compute radius
                size_t bubbleRadius = m_minBubbleRadius;
                if (!uniformSize)
                    {
                    const double sizeVal = sizeColumn->GetValue(i);
                    // normalize to 0-1 range
                    const double normalized =
                        safe_divide<double>(sizeVal - sizeMin, sizeMax - sizeMin);
                    // interpolate area
                    const double area = minArea + normalized * (maxArea - minArea);
                    // radius from area (area = r², omitting π as it cancels out)
                    bubbleRadius = static_cast<size_t>(std::sqrt(area));
                    bubbleRadius = std::clamp(bubbleRadius, m_minBubbleRadius, m_maxBubbleRadius);
                    }

                points->AddPoint(
                    GraphItems::Point2D{
                        GraphItems::GraphItemInfo{ GetDataset()->GetIdColumn().GetValue(i) }
                            .AnchorPoint(pt)
                            .Pen(series.GetColor())
                            .Brush(Colors::ColorContrast::ChangeOpacity(series.GetColor(), 100)),
                        bubbleRadius, series.GetShape() },
                    dc);
                }
            AddObject(std::move(points));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> BubblePlot::CreateLegend(const LegendOptions& options)
        {
        // get the base legend from ScatterPlot
        auto legend = ScatterPlot::CreateLegend(options);

        // if no legend was created or no valid size range, return as-is
        if (legend == nullptr || m_sizeMin >= m_sizeMax)
            {
            return legend;
            }

        // add separator before size legend
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::HorizontalSeparator,
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
            wxBrush{
                Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()) },
            Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));

        wxString legendText = legend->GetText();
        // space in line is needed for SVG exporting; otherwise, the blank line gets removed
        legendText.append(L"\n \n");

        // get the color from the first series for the icon
        const auto& seriesList = GetSeriesList();
        const wxColour iconColor = Colors::ColorContrast::ChangeOpacity(
            seriesList.empty() ? *wxBLACK : seriesList[0].GetColor(), 100);

        // determine decimal precision based on value range
        const int precision = (m_sizeMax - m_sizeMin < 10) ? 2 : 0;

        // add single line: "Size ([column]): [min] – [max]"
        legend->GetLegendIcons().emplace_back(
            Icons::IconShape::Circle, iconColor,
            wxBrush{ Colors::ColorContrast::ChangeOpacity(iconColor, 100) }, iconColor);
        legendText.append(wxString::Format(
            _(L"Size (%s): %s–%s"), m_sizeColumnName,
            wxNumberFormatter::ToString(m_sizeMin, precision, Settings::GetDefaultNumberFormat()),
            wxNumberFormatter::ToString(m_sizeMax, precision, Settings::GetDefaultNumberFormat())));

        legend->SetText(legendText);
        return legend;
        }
    } // namespace Wisteria::Graphs
