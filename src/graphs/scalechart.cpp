///////////////////////////////////////////////////////////////////////////////
// Name:        scalechart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "scalechart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ScaleChart, Wisteria::Graphs::BarChart)

    namespace Wisteria::Graphs
    {
    ScaleChart::ScaleChart(
        Wisteria::Canvas * canvas,
        std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors /*= nullptr*/,
        std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes /*= nullptr*/)
        : Wisteria::Graphs::BarChart(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));

        SetBarOrientation(Wisteria::Orientation::Vertical);
        GetScalingAxis().SetRange(0, 100, 0, 10, 10);

        AddBar(Bar{ 1, std::vector<BarBlock>{}, wxString{}, Wisteria::GraphItems::Label{},
                    Wisteria::BoxEffect::Solid, wxALPHA_TRANSPARENT });
        GetOppositeBarAxis().SetCustomLabel(1, GraphItems::Label{ _(L"Scale") });
        AddBar(Bar{ 2, std::vector<BarBlock>{}, wxString{}, Wisteria::GraphItems::Label{},
                    Wisteria::BoxEffect::Solid, wxALPHA_TRANSPARENT });
        GetOppositeBarAxis().SetCustomLabel(2, GraphItems::Label{ _(L"Scores") });

        GetBarAxis().Show(false);
        GetScalingAxis().Show(false);
        GetOppositeScalingAxis().Show(false);
        GetOppositeBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        }

    //----------------------------------------------------------------
    void ScaleChart::SetData(
        std::shared_ptr<const Data::Dataset> data, const wxString& scoreColumnName,
        const std::optional<const wxString>& groupColumnName /*= std::nullopt*/)
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
    void ScaleChart::SetMainScaleValues(const std::vector<double>& values, uint8_t precision)
        {
        m_scaleValues = values;
        m_precision = precision;
        }

    //----------------------------------------------------------------
    void ScaleChart::AddScale(std::vector<BarChart::BarBlock> blocks,
                              const std::optional<double> scalingAxisStart /*= std::nullopt*/,
                              const wxString& header /*= wxString{}*/)
        {
        Bar theBar{ static_cast<double>(GetBars().size() + 1), blocks, wxString{},
                    Wisteria::GraphItems::Label{}, Wisteria::BoxEffect::Solid };
        if (scalingAxisStart)
            {
            theBar.SetCustomScalingAxisStartPosition(scalingAxisStart.value());
            }
        AddBar(theBar, theBar.GetLength() >
                           GetScalingAxis().GetRange().second - GetScalingAxis().GetRange().first);
        if (!header.empty())
            {
            GetOppositeBarAxis().SetCustomLabel(theBar.GetAxisPosition(),
                                                GraphItems::Label{ header });
            }
        // re-adjust the bar scale so that there isn't any spaces around the outer bars
        const auto [rangeStart, rangeEnd] = GetBarAxis().GetRange();
        GetBarAxis().SetRange(rangeStart + 0.5, rangeEnd - 0.5, 1, 1, 1);
        GetOppositeBarAxis().SetRange(rangeStart + 0.5, rangeEnd - 0.5, 1, 0.5, 1);
        }

    //----------------------------------------------------------------
    void ScaleChart::AdjustAxes()
        {
        GetCustomAxes().clear();

            // score custom axis that jitter points are drawn around
            {
            GraphItems::Axis scoreRuler(Wisteria::AxisType::LeftYAxis);
            scoreRuler.SetDPIScaleFactor(GetDPIScaleFactor());
            const auto [yStart, yEnd] = GetScalingAxis().GetRange();
            scoreRuler.SetCustomYPosition(yEnd);
            scoreRuler.SetRange(yStart, yEnd, 0, GetScalingAxis().GetInterval(), 1);
            scoreRuler.SetCustomXPosition(1.5);
            scoreRuler.SetId(101);
            scoreRuler.Show(false);
            AddCustomAxis(scoreRuler);

            scoreRuler.SetId(102);
            scoreRuler.SetCustomXPosition(2);
            AddCustomAxis(scoreRuler);

            scoreRuler.SetId(103);
            scoreRuler.SetCustomXPosition(2.5);
            AddCustomAxis(scoreRuler);
            }
        }

    //----------------------------------------------------------------
    void ScaleChart::RecalcSizes(wxDC & dc)
        {
        AdjustAxes();

        BarChart::RecalcSizes(dc);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // draw the numeric points along the scale section
        const auto commonLabelSizeIterator = std::max_element(
            m_scaleValues.cbegin(), m_scaleValues.cend(),
            [this, &dc](const auto& lhv, const auto& rhv)
            {
                return dc
                           .ToDIP(dc.GetTextExtent(wxNumberFormatter::ToString(
                               lhv, m_precision, wxNumberFormatter::Style::Style_WithThousandsSep)))
                           .GetWidth() <
                       dc
                           .ToDIP(dc.GetTextExtent(wxNumberFormatter::ToString(
                               rhv, m_precision, wxNumberFormatter::Style::Style_WithThousandsSep)))
                           .GetWidth();
            });
        const int commonLabelWidth =
            dc
                .ToDIP(dc.GetTextExtent(wxNumberFormatter::ToString(
                    ((commonLabelSizeIterator != m_scaleValues.cend()) ? *commonLabelSizeIterator :
                                                                         100),
                    m_precision, wxNumberFormatter::Style::Style_WithThousandsSep)))
                .GetWidth();

        const auto addTextPoint = [this, commonLabelWidth](const double xValue, const double yValue,
                                                           const double textNumber,
                                                           const int precision)
        {
            wxPoint textPt;
            if (GetPhysicalCoordinates(xValue, yValue, textPt))
                {
                AddObject(std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo()
                        .Scaling(GetScaling())
                        .Pen(wxNullPen)
                        .Padding(0, 0, 0, 0)
                        .MinimumUserSizeDIPs(commonLabelWidth, std::nullopt)
                        .LabelAlignment(TextAlignment::Centered)
                        .LabelPageHorizontalAlignment(PageHorizontalAlignment::RightAligned)
                        .FontColor(GetLeftYAxis().GetFontColor())
                        .Text(wxNumberFormatter::ToString(
                            textNumber, precision,
                            wxNumberFormatter::Style::Style_WithThousandsSep))
                        .AnchorPoint(textPt)));
                }
        };

        std::for_each(m_scaleValues.cbegin(), m_scaleValues.cend(),
                      [&addTextPoint, this](const auto& val)
                      { addTextPoint(1, val, val, m_precision); });

        // start plotting the scores
        const auto [yStart, yEnd] = GetScalingAxis().GetRange();
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

            // constrain scores to the scale
            const auto currentScore = std::clamp<double>(m_scoresColumn->GetValue(i), yStart, yEnd);

            wxCoord yPt{ 0 };
            assert(middleRuler.GetPhysicalCoordinate(currentScore, yPt) &&
                   L"Unable to find point on scale chart!");
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
