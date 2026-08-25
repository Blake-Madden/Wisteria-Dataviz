///////////////////////////////////////////////////////////////////////////////
// Name:        wilmarth_bridge_plot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "wilmarth_bridge_plot.h"
#include "../math/safe_math.h"
#include <set>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WilmarthBridgePlot, Wisteria::Graphs::Graph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WilmarthBridgePlot::WilmarthBridgePlot(Canvas * canvas) : Graph2D(canvas)
        {
        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);

        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().GetAxisLinePen() = wxNullPen;
        }

    //----------------------------------------------------------------
    void WilmarthBridgePlot::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& labelColumnName,
        const wxString& exitColumnName,
        const std::optional<wxString>& entryColumnName /*= std::nullopt*/,
        const std::optional<wxString>& statusColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        GetSelectedIds().clear();
        m_observations.clear();
        m_periods.clear();
        m_usingDateColumns = false;

        if (GetDataset() == nullptr)
            {
            return;
            }

        m_labelColumnName = labelColumnName;
        m_exitColumnName = exitColumnName;
        m_entryColumnName = entryColumnName.value_or(wxString{});
        m_statusColumnName = statusColumnName.value_or(wxString{});

        // labels: required, categorical
        const auto labelColumn = GetCategoricalColumn(m_labelColumnName);

        // exit: required, resolved as continuous first and date second
        auto exitContinuousCol = GetDataset()->GetContinuousColumn(m_exitColumnName);
        auto exitDateCol = GetDataset()->GetDateColumns().cend();
        if (exitContinuousCol == GetDataset()->GetContinuousColumns().cend())
            {
            exitDateCol = GetDataset()->GetDateColumn(m_exitColumnName);
            if (exitDateCol == GetDataset()->GetDateColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': exit column not found for Wilmarth bridge plot."),
                                     m_exitColumnName)
                        .ToUTF8());
                }
            m_usingDateColumns = true;
            }

        // entry: optional, must match the exit column's resolved type
        auto entryContinuousCol = GetDataset()->GetContinuousColumns().cend();
        auto entryDateCol = GetDataset()->GetDateColumns().cend();
        const bool hasEntryColumn = entryColumnName.has_value() && !entryColumnName->empty();
        if (hasEntryColumn)
            {
            if (m_usingDateColumns)
                {
                entryDateCol = GetDataset()->GetDateColumn(entryColumnName.value());
                if (entryDateCol == GetDataset()->GetDateColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': entry column not found (expected a date "
                                           "column, matching the exit column) for Wilmarth "
                                           "bridge plot."),
                                         entryColumnName.value())
                            .ToUTF8());
                    }
                }
            else
                {
                entryContinuousCol = GetDataset()->GetContinuousColumn(entryColumnName.value());
                if (entryContinuousCol == GetDataset()->GetContinuousColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': entry column not found (expected a "
                                           "continuous column, matching the exit column) "
                                           "for Wilmarth bridge plot."),
                                         entryColumnName.value())
                            .ToUTF8());
                    }
                }
            }

        // status: optional, continuous (1 = event, 0 = censored)
        const bool hasStatusColumn = statusColumnName.has_value() && !statusColumnName->empty();
        const auto statusCol = hasStatusColumn ? GetContinuousColumn(statusColumnName.value()) :
                                                 GetDataset()->GetContinuousColumns().cend();

        const auto toAxisValue = [](const wxDateTime& date)
        {
            return date.IsValid() ? date.GetJulianDayNumber() :
                                    std::numeric_limits<double>::quiet_NaN();
        };

        // grid column = row order, so a missing exit value just leaves its column blank
        m_observations.reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            const bool exitMissing = m_usingDateColumns ? exitDateCol->IsMissingData(i) :
                                                          exitContinuousCol->IsMissingData(i);
            if (exitMissing)
                {
                continue;
                }

            Observation obs;
            obs.m_datasetRow = i;
            obs.m_label = labelColumn->GetValueAsLabel(i);
            obs.m_exit = m_usingDateColumns ? toAxisValue(exitDateCol->GetValue(i)) :
                                              exitContinuousCol->GetValue(i);

            // defaulted afterward if no entry column, or the entry value is missing
            obs.m_entered = std::numeric_limits<double>::quiet_NaN();
            if (hasEntryColumn)
                {
                if (m_usingDateColumns && !entryDateCol->IsMissingData(i))
                    {
                    obs.m_entered = toAxisValue(entryDateCol->GetValue(i));
                    }
                else if (!m_usingDateColumns && !entryContinuousCol->IsMissingData(i))
                    {
                    obs.m_entered = entryContinuousCol->GetValue(i);
                    }
                }

            // a missing status value defaults to an event, same as no status column
            obs.m_censored = hasStatusColumn && !statusCol->IsMissingData(i) &&
                             compare_doubles(statusCol->GetValue(i), 0.0);

            m_observations.push_back(std::move(obs));
            }

        if (m_observations.empty())
            {
            return;
            }

        // default any missing entry values to the earliest exit period seen in the data
        const double earliestExit = std::min_element(m_observations.cbegin(), m_observations.cend(),
                                                     [](const auto& first, const auto& second)
                                                     { return first.m_exit < second.m_exit; })
                                        ->m_exit;
        for (auto& obs : m_observations)
            {
            if (!std::isfinite(obs.m_entered))
                {
                obs.m_entered = earliestExit;
                }
            }

        m_periods = BuildPeriods();
        UpdateAxes();
        }

    //----------------------------------------------------------------
    std::vector<double> WilmarthBridgePlot::BuildPeriods() const
        {
        std::set<double, double_less> periodSet;
        for (const auto& obs : m_observations)
            {
            periodSet.insert(obs.m_entered);
            periodSet.insert(obs.m_exit);
            }
        return { periodSet.cbegin(), periodSet.cend() };
        }

    //----------------------------------------------------------------
    size_t WilmarthBridgePlot::AtRiskCount(const double period) const
        {
        return static_cast<size_t>(
            std::count_if(m_observations.cbegin(), m_observations.cend(),
                          [period](const auto& obs)
                          {
                              return compare_doubles_less_or_equal(obs.m_entered, period) &&
                                     compare_doubles_greater_or_equal(obs.m_exit, period);
                          }));
        }

    //----------------------------------------------------------------
    size_t WilmarthBridgePlot::EventCount(const double period) const
        {
        return static_cast<size_t>(
            std::count_if(m_observations.cbegin(), m_observations.cend(), [period](const auto& obs)
                          { return !obs.m_censored && compare_doubles(obs.m_exit, period); }));
        }

    //----------------------------------------------------------------
    double WilmarthBridgePlot::SurvivalProbability(const double period) const
        {
        // Kaplan-Meier: product of the surviving fraction at each event period up to this one
        double survival{ 1.0 };
        for (const auto& thisPeriod : m_periods)
            {
            if (compare_doubles_greater(thisPeriod, period))
                {
                break;
                }
            const size_t atRisk = AtRiskCount(thisPeriod);
            const size_t events = EventCount(thisPeriod);
            if (atRisk > 0)
                {
                survival *= (1.0 - safe_divide<double>(events, atRisk));
                }
            }
        return survival;
        }

    //----------------------------------------------------------------
    wxString WilmarthBridgePlot::FormatPeriod(const double period) const
        {
        if (m_usingDateColumns)
            {
            const wxDateTime dateVal{ period };
            return dateVal.IsValid() ? dateVal.FormatDate() : wxString{};
            }
        return wxNumberFormatter::ToString(period, 0,
                                           wxNumberFormatter::Style::Style_NoTrailingZeroes);
        }

    //----------------------------------------------------------------
    wxColour WilmarthBridgePlot::FadeColor(const wxColour& baseColor, const Observation& obs,
                                           const double period) const
        {
        if (m_fadeEffect == FadeEffect::None || m_periods.empty())
            {
            return baseColor;
            }

        // 1.0 = full ink, 0.0 = the floor below (never fully invisible)
        double lifeFraction{ 1.0 };
        if (m_fadeEffect == FadeEffect::RemainingLifetime)
            {
            const double lifespan = obs.m_exit - obs.m_entered;
            const double remaining = obs.m_exit - period;
            lifeFraction =
                (lifespan > 0) ? std::clamp(safe_divide(remaining, lifespan), 0.0, 1.0) : 1.0;
            }
        else // ElapsedTime
            {
            const auto periodPos = std::ranges::find_if(m_periods, [period](const double val)
                                                        { return compare_doubles(val, period); });
            const auto periodIndex = std::distance(m_periods.cbegin(), periodPos);
            const auto lastIndex = static_cast<std::ptrdiff_t>(m_periods.size()) - 1;
            lifeFraction =
                (lastIndex > 0) ? (1.0 - safe_divide<double>(periodIndex, lastIndex)) : 1.0;
            }

        constexpr uint8_t minOpacity{ 60 };
        const auto opacity = static_cast<uint8_t>(
            minOpacity + std::lround(lifeFraction * (wxALPHA_OPAQUE - minOpacity)));
        return Colors::ColorContrast::ChangeOpacity(baseColor, opacity);
        }

    //----------------------------------------------------------------
    wxString WilmarthBridgePlot::FormatSurvivalStatText(const double period) const
        {
        wxString statText;
        if (m_survivalDisplay == SurvivalDisplay::AtRiskCount ||
            m_survivalDisplay == SurvivalDisplay::Both)
            {
            statText +=
                wxNumberFormatter::ToString(static_cast<double>(AtRiskCount(period)), 0,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes);
            }
        if (m_survivalDisplay == SurvivalDisplay::SurvivalPercent ||
            m_survivalDisplay == SurvivalDisplay::Both)
            {
            if (!statText.empty())
                {
                statText += L"  ";
                }
            /* TRANSLATORS: a survival percentage; %s is the number, '%%' is a
               literal percent sign. */
            statText += wxString::Format(
                _(L"%s%%"),
                wxNumberFormatter::ToString(SurvivalProbability(period) * 100, 0,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes));
            }
        return statText;
        }

    //----------------------------------------------------------------
    void WilmarthBridgePlot::UpdateAxes()
        {
        if (GetDataset() == nullptr || m_periods.empty())
            {
            return;
            }

        GetBottomXAxis().SetRange(0, static_cast<double>(GetDataset()->GetRowCount()), 0);

        // the terminal row asserts that every observation had faded (had its event) by
        // then, so it is only truthful to draw when nothing is still censored
        const bool showTerminalRow = !m_terminalRowLabel.empty() &&
                                     std::none_of(m_observations.cbegin(), m_observations.cend(),
                                                  [](const auto& obs) { return obs.m_censored; });
        const double terminalPeriod = m_periods.back() + 1;

        // row axis: one point per period, plus a blank padding point just past the
        // first so the top row doesn't sit flush on the plot edge, and either a blank
        // padding point or the terminal row past the last
        GetLeftYAxis().GetAxisPoints().clear();
        GetLeftYAxis().AddUnevenAxisPoint(m_periods.front() - 1, L" ");
        for (const auto& period : m_periods)
            {
            GetLeftYAxis().AddUnevenAxisPoint(period, FormatPeriod(period));
            }
        GetLeftYAxis().AddUnevenAxisPoint(terminalPeriod,
                                          showTerminalRow ? m_terminalRowLabel : wxString{ L" " });
        GetLeftYAxis().AdjustRangeToLabels();
        GetLeftYAxis().Reverse(true);

        // survival statistics, mirrored onto the right axis, only if requested
        const bool showingStats = (m_survivalDisplay != SurvivalDisplay::None);
        GetRightYAxis().Show(showingStats);
        if (showingStats)
            {
            GetRightYAxis().GetAxisPoints().clear();
            GetRightYAxis().AddUnevenAxisPoint(m_periods.front() - 1, L" ");
            for (const auto& period : m_periods)
                {
                GetRightYAxis().AddUnevenAxisPoint(period, FormatSurvivalStatText(period));
                }
            GetRightYAxis().AddUnevenAxisPoint(
                terminalPeriod,
                showTerminalRow ? FormatSurvivalStatText(terminalPeriod) : wxString{ L" " });
            GetRightYAxis().AdjustRangeToLabels();
            GetRightYAxis().Reverse(true);
            }
        }

    //----------------------------------------------------------------
    void WilmarthBridgePlot::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        if (m_observations.empty() || m_periods.empty() || GetDataset() == nullptr)
            {
            return;
            }

        const auto columnCount = GetDataset()->GetRowCount();

        // one cell box per column/row, used to fit (and, if needed, shrink) each label
        const auto plotArea = GetPlotAreaBoundingBox();
        const wxSize cellSize{ static_cast<int>(safe_divide<double>(
                                   plotArea.GetWidth(), std::max<size_t>(columnCount, 1))),
                               static_cast<int>(safe_divide<double>(
                                   plotArea.GetHeight(), std::max<size_t>(m_periods.size(), 1))) };
        const wxColour baseFontColor = GetLeftYAxis().GetFontColor();

        double smallestTextScaling{ std::numeric_limits<double>::max() };
        std::vector<std::unique_ptr<GraphItems::Label>> cellLabels;
        std::vector<wxPoint> censorArrowPts;

        for (const auto& obs : m_observations)
            {
            std::optional<size_t> lastActivePeriodIndex;
            for (size_t periodIdx = 0; periodIdx < m_periods.size(); ++periodIdx)
                {
                const double period = m_periods[periodIdx];
                if (compare_doubles_less(period, obs.m_entered) ||
                    compare_doubles_greater(period, obs.m_exit))
                    {
                    continue;
                    }

                wxPoint pt;
                if (GetPhysicalCoordinates(static_cast<double>(obs.m_datasetRow) + 0.5, period, pt))
                    {
                    auto cellLabel = std::make_unique<GraphItems::Label>(
                        GraphItems::GraphItemInfo{ obs.m_label }
                            .Scaling(GetScaling())
                            .DPIScaling(GetDPIScaleFactor())
                            .Pen(wxNullPen)
                            .Padding(0, 0, 0, 0)
                            .FontColor(FadeColor(baseFontColor, obs, period))
                            .LabelPageVerticalAlignment(PageVerticalAlignment::Centered)
                            .LabelPageHorizontalAlignment(PageHorizontalAlignment::Centered)
                            .Anchoring(Anchoring::Center)
                            .AnchorPoint(pt));
                    cellLabel->SetBoundingBox(
                        wxRect{ pt - wxPoint{ cellSize.GetWidth() / 2, cellSize.GetHeight() / 2 },
                                cellSize },
                        dc, GetScaling());
                    smallestTextScaling = std::min(smallestTextScaling, cellLabel->GetScaling());
                    cellLabels.push_back(std::move(cellLabel));
                    }
                lastActivePeriodIndex = periodIdx;
                }

            // a censored observation gets an arrow past its last cell, never extended
            // to the bottom of the chart
            if (obs.m_censored && m_showCensoredMarkers && lastActivePeriodIndex.has_value() &&
                lastActivePeriodIndex.value() + 1 < m_periods.size())
                {
                wxPoint arrowPt;
                if (GetPhysicalCoordinates(static_cast<double>(obs.m_datasetRow) + 0.5,
                                           m_periods[lastActivePeriodIndex.value() + 1], arrowPt))
                    {
                    censorArrowPts.push_back(arrowPt);
                    }
                }
            }

        // every cell label shares this final scaling: the smallest one needed to fit
        if (smallestTextScaling == std::numeric_limits<double>::max())
            {
            smallestTextScaling = GetScaling();
            }

        for (auto& cellLabel : cellLabels)
            {
            const wxRect cachedBoundBox = cellLabel->GetBoundingBox(dc);
            cellLabel->SetScaling(smallestTextScaling);
            cellLabel->LockBoundingBoxScaling();
            cellLabel->SetBoundingBox(cachedBoundBox, dc, GetScaling());
            cellLabel->UnlockBoundingBoxScaling();
            AddObject(std::move(cellLabel));
            }

        const wxSize censorArrowSize{ wxRound(ScaleToScreenAndCanvas(6)),
                                      wxRound(ScaleToScreenAndCanvas(6)) };
        for (const auto& arrowPt : censorArrowPts)
            {
            AddObject(std::make_unique<GraphItems::Shape>(GraphItems::GraphItemInfo{}
                                                              .Pen(wxPen{ baseFontColor })
                                                              .Brush(wxBrush{ baseFontColor })
                                                              .Scaling(smallestTextScaling)
                                                              .DPIScaling(GetDPIScaleFactor())
                                                              .Anchoring(Anchoring::Center)
                                                              .AnchorPoint(arrowPt),
                                                          Icons::IconShape::ArrowRight,
                                                          censorArrowSize));
            }
        }

    //----------------------------------------------------------------
    void WilmarthBridgePlot::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A Wilmarth bridge plot") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");
        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");

        if (!m_observations.empty() && !m_periods.empty())
            {
            const size_t survivingCount = AtRiskCount(m_periods.back());
            label += L". ";
            label += wxString::Format(
                /* TRANSLATORS: Wilmarth bridge plot accessibility summary.
                   1st %zu is the observation count, 1st %s and 2nd %s are the first and
                   last period, 2nd %zu is how many observations reached the last period. */
                _(L"%zu observations from %s to %s, %zu still active at the final period"),
                m_observations.size(), FormatPeriod(m_periods.front()),
                FormatPeriod(m_periods.back()), survivingCount);
            }

        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
