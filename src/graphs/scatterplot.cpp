///////////////////////////////////////////////////////////////////////////////
// Name:        scatterplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "scatterplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ScatterPlot, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    ScatterPlot::ScatterPlot(Canvas * canvas,
                             const std::shared_ptr<Colors::Schemes::ColorScheme>& colors,
                             const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes,
                             const std::shared_ptr<LineStyleScheme>& regressionLineStyles)
        : GroupGraph2D(canvas),
          m_regressionLineStyles(regressionLineStyles != nullptr ?
                                     regressionLineStyles :
                                     std::make_shared<LineStyleScheme>(LineStyleScheme{
                                         { wxPenStyle::wxPENSTYLE_SOLID, LineStyle::Lines } }))
        {
        SetColorScheme(colors != nullptr ? colors :
                                           std::make_shared<Colors::Schemes::ColorScheme>(
                                               Settings::GetDefaultColorScheme()));
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes{}));
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void ScatterPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                              const wxString& yColumnName, const wxString& xColumnName,
                              const std::optional<wxString>& groupColumnName)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();
        m_series.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);
        m_xColumnName = xColumnName;
        m_yColumnName = yColumnName;

        GetLeftYAxis().Reset();
        GetRightYAxis().Reset();
        GetBottomXAxis().Reset();
        GetTopXAxis().Reset();
        GetBottomXAxis().GetTitle().SetText(xColumnName);
        GetLeftYAxis().GetTitle().SetText(yColumnName);

        // get the continuous columns
        const auto xColumn = GetContinuousColumn(m_xColumnName);
        const auto yColumn = GetContinuousColumn(m_yColumnName);

        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            auto groupColumn = GetGroupColumn();
            // create a reverse string table, with it sorted by label
            std::map<wxString, Data::GroupIdType, Data::wxStringLessNoCase> groups;
            for (const auto& [id, str] : groupColumn->GetStringTable())
                {
                groups.insert(std::make_pair(str, id));
                }
            size_t currentIndex{ 0 };
            for (const auto& group : groups)
                {
                Series series;
                series.SetGroupInfo(groupColumnName, group.second,
                                    groupColumn->GetLabelFromID(group.second));
                series.SetColor(GetColorScheme()->GetColor(currentIndex));
                series.SetShape(GetShapeScheme()->GetShape(currentIndex));
                const auto& [penStyle, lineStyle] =
                    m_regressionLineStyles->GetLineStyle(currentIndex);
                series.GetRegressionPen().SetStyle(penStyle);
                series.GetRegressionPen().SetColour(Colors::ColorContrast::ShadeOrTint(
                    GetColorScheme()->GetColor(currentIndex), math_constants::three_quarters));
                series.SetRegressionLineStyle(lineStyle);
                CalculateRegression(series);
                m_series.push_back(series);
                ++currentIndex;
                }
            }
        else
            {
            Series series;
            series.SetGroupInfo(std::nullopt, 0, wxString{});
            series.SetColor(GetColorScheme()->GetColor(0));
            series.SetShape(GetShapeScheme()->GetShape(0));
            const auto& [penStyle, lineStyle] = m_regressionLineStyles->GetLineStyle(0);
            series.GetRegressionPen().SetStyle(penStyle);
            series.GetRegressionPen().SetColour(Colors::ColorContrast::ShadeOrTint(
                GetColorScheme()->GetColor(0), math_constants::three_quarters));
            series.SetRegressionLineStyle(lineStyle);
            CalculateRegression(series);
            m_series.push_back(series);
            }

        // set axis ranges based on data
        const auto [xMin, xMax] =
            std::minmax_element(xColumn->GetValues().cbegin(), xColumn->GetValues().cend(),
                                [](const auto& a, const auto& b)
                                {
                                    if (std::isnan(a))
                                        {
                                        return false;
                                        }
                                    if (std::isnan(b))
                                        {
                                        return true;
                                        }
                                    return a < b;
                                });
        const auto [yMin, yMax] =
            std::minmax_element(yColumn->GetValues().cbegin(), yColumn->GetValues().cend(),
                                [](const auto& a, const auto& b)
                                {
                                    if (std::isnan(a))
                                        {
                                        return false;
                                        }
                                    if (std::isnan(b))
                                        {
                                        return true;
                                        }
                                    return a < b;
                                });

        if (xMin != xColumn->GetValues().cend() && xMax != xColumn->GetValues().cend() &&
            std::isfinite(*xMin) && std::isfinite(*xMax))
            {
            const auto [xStart, xEnd] = adjust_intervals(*xMin, *xMax);
            GetBottomXAxis().SetRange(
                xStart, xEnd, ((get_mantissa(xStart) == 0 && get_mantissa(xEnd) == 0) ? 0 : 1),
                false);
            }
        if (yMin != yColumn->GetValues().cend() && yMax != yColumn->GetValues().cend() &&
            std::isfinite(*yMin) && std::isfinite(*yMax))
            {
            const auto [yStart, yEnd] = adjust_intervals(*yMin, *yMax);
            GetLeftYAxis().SetRange(
                yStart, yEnd, ((get_mantissa(yStart) == 0 && get_mantissa(yEnd) == 0) ? 0 : 1),
                false);
            }
        }

    //----------------------------------------------------------------
    void ScatterPlot::CalculateRegression(Series & series)
        {
        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto xColumn = GetContinuousColumn(m_xColumnName);
        const auto yColumn = GetContinuousColumn(m_yColumnName);
        const auto groupColumn = GetGroupColumn();

        // collect x and y values for this series
        std::vector<double> xValues, yValues;
        xValues.reserve(GetDataset()->GetRowCount());
        yValues.reserve(GetDataset()->GetRowCount());

        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            // skip if from different group
            if (IsUsingGrouping() && groupColumn->GetValue(i) != series.GetGroupId())
                {
                continue;
                }
            xValues.push_back(xColumn->GetValue(i));
            yValues.push_back(yColumn->GetValue(i));
            }

        series.m_regressionResults = statistics::linear_regression(xValues, yValues);
        }

    //----------------------------------------------------------------
    void ScatterPlot::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto groupColumn = GetGroupColumn();
        const auto xColumn = GetContinuousColumn(m_xColumnName);
        const auto yColumn = GetContinuousColumn(m_yColumnName);

        for (const auto& series : m_series)
            {
            // draw regression line first so points appear on top
            if (IsShowingRegressionLines() && series.GetRegressionResults().is_valid())
                {
                const auto& stats = series.GetRegressionResults();
                const auto [xAxisMin, xAxisMax] = GetBottomXAxis().GetRange();
                const auto [yAxisMin, yAxisMax] = GetLeftYAxis().GetRange();

                // calculate Y values at axis endpoints
                double x1 = xAxisMin;
                double x2 = xAxisMax;
                double y1 = stats.slope * x1 + stats.intercept;
                double y2 = stats.slope * x2 + stats.intercept;

                // clip line to Y axis range
                // if slope is not zero, find where line intersects Y boundaries
                if (stats.slope != 0.0)
                    {
                    // x where line crosses yAxisMin: x = (yAxisMin - intercept) / slope
                    // x where line crosses yAxisMax: x = (yAxisMax - intercept) / slope
                    if (y1 < yAxisMin)
                        {
                        x1 = (yAxisMin - stats.intercept) / stats.slope;
                        y1 = yAxisMin;
                        }
                    else if (y1 > yAxisMax)
                        {
                        x1 = (yAxisMax - stats.intercept) / stats.slope;
                        y1 = yAxisMax;
                        }
                    if (y2 < yAxisMin)
                        {
                        x2 = (yAxisMin - stats.intercept) / stats.slope;
                        y2 = yAxisMin;
                        }
                    else if (y2 > yAxisMax)
                        {
                        x2 = (yAxisMax - stats.intercept) / stats.slope;
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

            // draw scatter points
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
                // skip invalid data
                if (!std::isfinite(xColumn->GetValue(i)) || !std::isfinite(yColumn->GetValue(i)))
                    {
                    continue;
                    }
                if (!GetPhysicalCoordinates(xColumn->GetValue(i), yColumn->GetValue(i), pt))
                    {
                    continue;
                    }

                points->AddPoint(
                    GraphItems::Point2D{
                        GraphItems::GraphItemInfo{ GetDataset()->GetIdColumn().GetValue(i) }
                            .AnchorPoint(pt)
                            .Pen(series.GetColor())
                            .Brush(series.GetColor()),
                        Settings::GetPointRadius(), series.GetShape() },
                    dc);
                }
            AddObject(std::move(points));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ScatterPlot::CreateLegend(const LegendOptions& options)
        {
        if (m_series.empty())
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .FontColor(GetLeftYAxis().GetFontColor()));

        wxString legendText;
        size_t seriesIndex{ 0 };

        for (const auto& series : m_series)
            {
            // add group name if grouping is used
            if (IsUsingGrouping())
                {
                wxString currentLabel = series.GetText();
                if (currentLabel.length() > Settings::GetMaxLegendTextLength() &&
                    Settings::GetMaxLegendTextLength() >= 1)
                    {
                    currentLabel.erase(Settings::GetMaxLegendTextLength() - 1);
                    currentLabel.append(L'…');
                    }
                legendText.append(currentLabel).append(L'\n');
                // point marker icon
                legend->GetLegendIcons().emplace_back(
                    series.GetShape(),
                    Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                    wxBrush{ series.GetColor() }, series.GetColor());
                }

            // add regression statistics if available
            const auto& stats = series.GetRegressionResults();
            if (stats.is_valid())
                {
                // regression line icon
                if (IsShowingRegressionLines())
                    {
                    legend->GetLegendIcons().emplace_back(
                        Icons::IconShape::HorizontalLine, series.GetRegressionPen().GetColour(),
                        wxBrush{ series.GetRegressionPen().GetColour() },
                        series.GetRegressionPen().GetColour());
                    legendText.append(_(L"Regression Line")).append(L'\n');
                    }

                // statistics
                // R² (coefficient of determination)
                legend->GetLegendIcons().emplace_back(Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                      *wxTRANSPARENT_BRUSH);
                if (std::isfinite(stats.r_squared))
                    {
                    legendText.append(wxString::Format(
                        L"R²: %s\n", wxNumberFormatter::ToString(
                                         stats.r_squared, 4, Settings::GetDefaultNumberFormat())));
                    }
                else
                    {
                    legendText.append(_(L"R²: Undefined (insufficient variance in outcome)\n"));
                    }
                // r (Pearson correlation coefficient)
                legend->GetLegendIcons().emplace_back(Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                      *wxTRANSPARENT_BRUSH);
                if (std::isfinite(stats.correlation))
                    {
                    legendText.append(wxString::Format(
                        L"r: %s\n", wxNumberFormatter::ToString(
                                        stats.correlation, 4, Settings::GetDefaultNumberFormat())));
                    }
                else
                    {
                    legendText.append(_(L"r: Undefined (constant variable detected)\n"));
                    }
                // p-value
                legend->GetLegendIcons().emplace_back(Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                      *wxTRANSPARENT_BRUSH);
                if (std::isfinite(stats.p_value))
                    {
                    if (compare_doubles_less(stats.p_value, 0.0001))
                        {
                        legendText.append(
                            /* TRANSLATORS: don't translate 'p'. */ _(L"p-value: < 0.0001\n"));
                        }
                    else
                        {
                        legendText.append(wxString::Format(
                            // TRANSLATORS: don't translate 'p'.
                            _(L"p-value: %s\n"),
                            wxNumberFormatter::ToString(stats.p_value, 4,
                                                        Settings::GetDefaultNumberFormat())));
                        }
                    }
                else
                    {
                    legendText.append(_(L"p-value: Undefined (insufficient degrees of freedom)\n"));
                    }
                legend->GetLegendIcons().emplace_back(Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                      *wxTRANSPARENT_BRUSH);
                legendText.append(wxString::Format(L"n: %zu\n", stats.n));
                // regression equation (slope and intercept)
                legend->GetLegendIcons().emplace_back(Icons::IconShape::Blank, *wxTRANSPARENT_PEN,
                                                      *wxTRANSPARENT_BRUSH);
                if (std::isfinite(stats.slope) && std::isfinite(stats.intercept))
                    {
                    legendText.append(wxString::Format(
                        L"y = %sx + %s\n",
                        wxNumberFormatter::ToString(stats.slope, 4,
                                                    Settings::GetDefaultNumberFormat()),
                        wxNumberFormatter::ToString(stats.intercept, 4,
                                                    Settings::GetDefaultNumberFormat())));
                    }
                else if (!std::isfinite(stats.slope))
                    {
                    legendText.append(_(L"Slope: Undefined (zero variance in predictor)\n"));
                    }
                else
                    {
                    legendText.append(_(L"Intercept: Undefined (model not estimable)\n"));
                    }
                }

            // separator between groups
            if (IsUsingGrouping() && seriesIndex < m_series.size() - 1)
                {
                legend->GetLegendIcons().emplace_back(Icons::LegendIcon(
                    Icons::IconShape::HorizontalSeparator,
                    Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()),
                    wxBrush{ Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                        GetPlotOrCanvasColor()) },
                    Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor())));
                legendText.append(L'\n');
                }
            ++seriesIndex;
            }

        // header if requested
        if (options.IsIncludingHeader() && IsUsingGrouping())
            {
            legendText.Prepend(GetGroupColumn()->GetName() + L'\n');
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }

        legend->SetText(legendText.Trim());
        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
