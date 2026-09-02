///////////////////////////////////////////////////////////////////////////////
// Name:        barchart_serpentine.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "../base/shapes.h"
#include "barchart.h"
#include "categoricalbarchart.h"
#include <algorithm>
#include <numeric>

namespace Wisteria::Graphs
    {
    //-----------------------------------
    bool BarChart::IsBarFoldingSupported() const noexcept
        {
        // Only a plain or categorical bar chart folds. Match the exact type, not
        // IsKindOf(), so other subclasses are excluded unless they override this.
        const wxClassInfo* const classInfo{ GetClassInfo() };
        return classInfo == wxCLASSINFO(BarChart) || classInfo == wxCLASSINFO(CategoricalBarChart);
        }

    //-----------------------------------
    void BarChart::ClassifySerpentineBars()
        {
        m_serpentineBarIndices.clear();

        if (GetSerpentineMode() == SerpentineMode::None || GetBars().size() < 3 ||
            !IsBarFoldingSupported() || GetScalingAxis().IsReversed())
            {
            return;
            }

        // a grouped bar is measured against its sub-bars, so folding it would
        // misplace the group's brackets
        const auto isInBarGroup = [this](const size_t barIndex)
        {
            return std::ranges::any_of(m_barGroups,
                                       [barIndex](const auto& barGroup)
                                       {
                                           return barIndex >= barGroup.m_barPositions.first &&
                                                  barIndex <= barGroup.m_barPositions.second;
                                       });
        };
        const auto isEligible = [&, this](const size_t barIndex)
        {
            const auto& bar = GetBars()[barIndex];
            return bar.GetBlocks().size() == 1 &&
                   !bar.GetCustomScalingAxisStartPosition().has_value() &&
                   std::isfinite(bar.GetLength()) && bar.GetLength() > 0 && !isInBarGroup(barIndex);
        };

        std::set<double, std::greater<>> eligibleLengths;
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (isEligible(i))
                {
                eligibleLengths.insert(GetBars()[i].GetLength());
                }
            }
        // the fold width comes from the longest unfolded bar, so a second length
        // is needed to measure against
        if (eligibleLengths.size() < 2)
            {
            return;
            }
        const double referenceLength{ *std::next(eligibleLengths.cbegin()) };

        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (isEligible(i) &&
                GetBars()[i].GetLength() >= GetSerpentineThreshold() * referenceLength)
                {
                // too many folds to draw legibly, so fold nothing
                if (GetBars()[i].GetLength() > MAX_SERPENTINE_FOLDS * referenceLength)
                    {
                    m_serpentineBarIndices.clear();
                    return;
                    }
                m_serpentineBarIndices.insert(i);
                }
            }

        // with every bar folded there is nothing left to set the fold width
        if (m_serpentineBarIndices.size() >= GetBars().size())
            {
            m_serpentineBarIndices.clear();
            }
        }

    //-----------------------------------
    void BarChart::AdjustScalingAxisFromBars()
        {
        m_longestBarLength = 0;
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (m_serpentineBarIndices.contains(i))
                {
                continue;
                }
            UpdateScalingAxisFromBar(GetBars()[i]);
            }
        }

    //-----------------------------------
    std::optional<size_t> BarChart::FindSerpentineEatTarget(const size_t serpentineBarIndex) const
        {
        if (GetSerpentineMode() != SerpentineMode::AggressiveSerpentine)
            {
            return std::nullopt;
            }

        // a full fold spans the tallest unfolded bar
        const double foldSpan{ m_longestBarLength };
        if (foldSpan <= 0)
            {
            return std::nullopt;
            }

        // Aggressive folding is only worth it with an odd number of full folds plus
        // a leftover run, so that leftover run turns back down into a neighbor.
        const double barLength{ GetBars()[serpentineBarIndex].GetLength() };
        const auto fullFolds{ static_cast<size_t>(std::floor(safe_divide(barLength, foldSpan))) };
        const double remainder{ barLength - (static_cast<double>(fullFolds) * foldSpan) };
        if (fullFolds < 1 || (fullFolds % 2) == 0 || remainder <= 0)
            {
            return std::nullopt;
            }

        std::vector<size_t> axisOrder(GetBars().size());
        std::iota(axisOrder.begin(), axisOrder.end(), static_cast<size_t>(0));
        std::ranges::sort(
            axisOrder, [this](const size_t left, const size_t right)
            { return GetBars()[left].GetAxisPosition() < GetBars()[right].GetAxisPosition(); });
        const auto foldedOrderPos =
            std::ranges::find(axisOrder, serpentineBarIndex) - axisOrder.cbegin();

        for (auto pos{ foldedOrderPos + 1 }; std::cmp_less(pos, axisOrder.size()); ++pos)
            {
            if (m_serpentineBarIndices.contains(axisOrder[pos]))
                {
                continue;
                }
            // The leftover run stops at foldSpan (remainder in the neighbor's
            // column). Require at least half a fold of clear space below it, or a
            // blank row would be no worse.
            const double clearGap{ foldSpan - remainder - GetBars()[axisOrder[pos]].GetLength() };
            return (clearGap >= foldSpan / 2.0) ? std::optional<size_t>{ axisOrder[pos] } :
                                                  std::nullopt;
            }
        return std::nullopt;
        }

    //-----------------------------------
    std::vector<BarChart::SerpentineRun>
    BarChart::CalcSerpentineRuns(const double serpentineBarLength, const double foldSpan,
                                 const bool lastRunReturns) const
        {
        std::vector<SerpentineRun> runs;
        if (foldSpan <= 0 || serpentineBarLength <= 0)
            {
            return runs;
            }

        // Every full fold is one fold-span tall. Any leftover is a shorter final run.
        const auto fullFolds{ static_cast<size_t>(
            std::floor(safe_divide(serpentineBarLength, foldSpan))) };
        const double lastRun{ serpentineBarLength - (static_cast<double>(fullFolds) * foldSpan) };
        const size_t runCount{ fullFolds + ((lastRun > 0) ? 1 : 0) };
        // force at least one turn
        if (runCount < 2)
            {
            runs.push_back(SerpentineRun{ 0.0, serpentineBarLength / 2.0, true });
            runs.push_back(SerpentineRun{ 0.0, serpentineBarLength / 2.0, false });
            return runs;
            }

        runs.reserve(runCount);
        for (size_t i = 0; i < runCount; ++i)
            {
            SerpentineRun run;
            // even runs travel away from the origin and odd ones come back
            run.m_forward = ((i % 2) == 0);
            run.m_scaleStart = 0;
            run.m_scaleEnd = foldSpan;
            const bool isLast{ i + 1 == runCount };
            // aggressive folding forces the last run to return into a neighbor
            if (isLast && lastRunReturns)
                {
                run.m_forward = false;
                }
            if (isLast && lastRun > 0)
                {
                if (run.m_forward)
                    {
                    // a climbing final run rises only as far as needed
                    run.m_scaleEnd = lastRun;
                    }
                else
                    {
                    // a returning final run hangs from the top of the fold, not
                    // down to the axis
                    run.m_scaleStart = foldSpan - lastRun;
                    }
                }
            runs.push_back(run);
            }

        return runs;
        }

    //-----------------------------------
    std::vector<BarChart::SerpentineRowAssignment>
    BarChart::AssignSerpentineRows(const std::vector<SerpentineRun>& runs,
                                   const std::optional<size_t> eatTarget) const
        {
        std::vector<SerpentineRowAssignment> assignments;
        if (runs.size() < 2)
            {
            return assignments;
            }
        assignments.reserve(runs.size() - 1);

        // Without an eat target, every run turns through its own blank row. With
        // one, only the last run goes into the neighbor's column.
        size_t nextBlankRow{ 1 };
        for (size_t i = 1; i < runs.size(); ++i)
            {
            SerpentineRowAssignment assignment;
            const bool isLastRun{ i + 1 == runs.size() };
            if (isLastRun && eatTarget.has_value())
                {
                assignment.m_eatTargetIndex = eatTarget;
                }
            else
                {
                assignment.m_blankRowIndex = nextBlankRow++;
                }
            assignments.push_back(assignment);
            }

        return assignments;
        }

    //-----------------------------------
    void BarChart::ApplyBarAxisRangeAndLabels()
        {
        m_lowestBarAxisPosition = std::numeric_limits<double>::max();
        m_highestBarAxisPosition = std::numeric_limits<double>::lowest();
        for (const auto& bar : GetBars())
            {
            const auto customWidth = bar.GetCustomWidth().has_value() ?
                                         safe_divide<double>(bar.GetCustomWidth().value(), 2) :
                                         0;
            m_lowestBarAxisPosition =
                std::min(m_lowestBarAxisPosition, bar.GetAxisPosition() - customWidth);
            m_highestBarAxisPosition =
                std::max(m_highestBarAxisPosition, bar.GetAxisPosition() + customWidth);
            }
        // Blank fold rows carry no bar, so the loop above misses them. Stretch the
        // axis to cover them, or a fold lands outside the plot.
        for (const auto& [serpentineIndex, segments] : m_serpentineSegments)
            {
            for (const auto& segment : segments)
                {
                m_lowestBarAxisPosition =
                    std::min(m_lowestBarAxisPosition, segment.m_rowAxisPosition);
                m_highestBarAxisPosition =
                    std::max(m_highestBarAxisPosition, segment.m_rowAxisPosition);
                }
            }
        GetBarAxis().SetRange(m_lowestBarAxisPosition - GetBarAxis().GetInterval(),
                              m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                              GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                              GetBarAxis().GetDisplayInterval());

        // ClearCustomLabels() re-enables the outer labels, so restore that flag afterwards
        const bool isDisplayingOuterLabels{ GetBarAxis().IsShowingOuterLabels() };
        GetBarAxis().ClearCustomLabels();
        for (const auto& bar : GetBars())
            {
            if (bar.GetAxisLabel().IsShown() && !bar.GetAxisLabel().GetText().empty())
                {
                GetBarAxis().SetCustomLabel(bar.GetAxisPosition(), bar.GetAxisLabel());
                }
            }
        GetBarAxis().ShowOuterLabels(isDisplayingOuterLabels);
        }

    //-----------------------------------
    void BarChart::UpdateSerpentineLayout()
        {
        if (m_inSerpentineLayout || GetBars().empty())
            {
            return;
            }
        m_inSerpentineLayout = true;

        const bool hadFolds{ !m_serpentineSegments.empty() };

        // A prior pass may have shifted bars along the axis to open room for folds.
        // Put them back before anything reads their positions, or a re-snapshot
        // below records the shifted spots as the originals.
        if (hadFolds)
            {
            const size_t restoreCount{ std::min(m_originalBarAxisPositions.size(),
                                                GetBars().size()) };
            for (size_t i = 0; i < restoreCount; ++i)
                {
                m_bars[i].SetAxisPosition(m_originalBarAxisPositions[i]);
                }
            }

        // re-snapshot on a size mismatch too, since later passes index it by bar
        if (!m_serpentineSnapshotValid || m_originalBarAxisPositions.size() != GetBars().size())
            {
            m_originalBarAxisPositions.clear();
            m_originalBarAxisPositions.reserve(GetBars().size());
            for (const auto& bar : GetBars())
                {
                m_originalBarAxisPositions.push_back(bar.GetAxisPosition());
                }
            m_serpentineSnapshotValid = true;
            }

        // reset bars to their original positions, so repeated passes converge
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            m_bars[i].SetAxisPosition(m_originalBarAxisPositions[i]);
            }
        m_serpentineSegments.clear();
        m_serpentineExtraRowCount = 0;

        ClassifySerpentineBars();
        if (m_serpentineBarIndices.empty())
            {
            // Only touch the axes if an earlier pass folded.
            // A never-folded chart is left alone.
            if (hadFolds)
                {
                // an earlier pass shrank the scaling axis to fit the folds, so grow it back
                AdjustScalingAxisFromBars();
                ApplyBarAxisRangeAndLabels();
                }
            m_inSerpentineLayout = false;
            return;
            }

        AdjustScalingAxisFromBars();
        // Fold to the tallest unfolded bar, not the scaling axis range.
        // The axis has end-label padding that would make every fold overshoot.
        const double foldSpan{ m_longestBarLength };
        const double rowInterval{ GetBarAxis().GetInterval() };
        if (foldSpan <= 0 || rowInterval <= 0)
            {
            m_serpentineBarIndices.clear();
            if (hadFolds)
                {
                AdjustScalingAxisFromBars();
                if (rowInterval > 0)
                    {
                    ApplyBarAxisRangeAndLabels();
                    }
                }
            m_inSerpentineLayout = false;
            return;
            }

        // Work out each fold's runs and the rows they want. Final row positions
        // depend on how many blank rows precede them, known only after every fold
        // has asked.
        std::map<size_t, std::vector<SerpentineRun>> barRuns;
        std::map<size_t, std::vector<SerpentineRowAssignment>> barAssignments;
        std::map<size_t, size_t> blankRowsAfterBar;
        for (const auto serpentineIndex : m_serpentineBarIndices)
            {
            const auto eatTarget = FindSerpentineEatTarget(serpentineIndex);
            auto runs = CalcSerpentineRuns(GetBars()[serpentineIndex].GetLength(), foldSpan,
                                           eatTarget.has_value());
            if (runs.empty())
                {
                continue;
                }
            auto assignments = AssignSerpentineRows(runs, eatTarget);
            const auto blankCount = static_cast<size_t>(
                std::ranges::count_if(assignments, [](const auto& assignment)
                                      { return !assignment.m_eatTargetIndex.has_value(); }));
            // Blank rows go straight after the folded bar. An eaten neighbor keeps
            // its place and shifts along with everything after it.
            blankRowsAfterBar[serpentineIndex] = blankCount;
            m_serpentineExtraRowCount += blankCount;
            barRuns[serpentineIndex] = std::move(runs);
            barAssignments[serpentineIndex] = std::move(assignments);
            }

        // hand out final positions, walking the bars in the order they sit on the axis
        std::vector<size_t> barOrder(GetBars().size());
        std::iota(barOrder.begin(), barOrder.end(), static_cast<size_t>(0));
        std::ranges::sort(
            barOrder, [this](const size_t left, const size_t right)
            { return m_originalBarAxisPositions[left] < m_originalBarAxisPositions[right]; });

        std::map<size_t, std::vector<double>> blankRowPositions;
        double cumulativeShift{ 0 };
        for (const auto barIndex : barOrder)
            {
            const double newPosition{ m_originalBarAxisPositions[barIndex] + cumulativeShift };
            m_bars[barIndex].SetAxisPosition(newPosition);

            const auto blankPos = blankRowsAfterBar.find(barIndex);
            if (blankPos != blankRowsAfterBar.cend() && blankPos->second > 0)
                {
                // blank rows follow the folded bar, so everything after it shifts down
                auto& positions = blankRowPositions[barIndex];
                positions.reserve(blankPos->second);
                for (size_t blank = 1; blank <= blankPos->second; ++blank)
                    {
                    positions.push_back(newPosition + (rowInterval * static_cast<double>(blank)));
                    }
                cumulativeShift += rowInterval * static_cast<double>(blankPos->second);
                }
            }

        // every row is placed now, so resolve the named rows to positions
        for (const auto& [serpentineIndex, runs] : barRuns)
            {
            std::vector<SerpentineSegment> segments;
            segments.reserve(runs.size());
            segments.push_back(SerpentineSegment{
                m_bars[serpentineIndex].GetAxisPosition(), runs.front().m_scaleStart,
                runs.front().m_scaleEnd, runs.front().m_forward });

            const auto& assignments = barAssignments[serpentineIndex];
            const auto& blanks = blankRowPositions[serpentineIndex];
            for (size_t i = 1; i < runs.size() && (i - 1) < assignments.size(); ++i)
                {
                const auto& assignment = assignments[i - 1];
                double rowPosition{ 0 };
                if (assignment.m_eatTargetIndex.has_value())
                    {
                    // the run lies directly over the eaten bar's row
                    rowPosition = m_bars[assignment.m_eatTargetIndex.value()].GetAxisPosition();
                    }
                else if (assignment.m_blankRowIndex >= 1 &&
                         (assignment.m_blankRowIndex - 1) < blanks.size())
                    {
                    rowPosition = blanks[assignment.m_blankRowIndex - 1];
                    }
                else
                    {
                    continue;
                    }
                segments.push_back(SerpentineSegment{ rowPosition, runs[i].m_scaleStart,
                                                      runs[i].m_scaleEnd, runs[i].m_forward });
                }
            m_serpentineSegments[serpentineIndex] = std::move(segments);
            }

        ApplyBarAxisRangeAndLabels();
        m_inSerpentineLayout = false;
        }

    //-----------------------------------
    wxPoint BarChart::DrawSerpentineBar(Bar& bar, BarRenderInfo& barRenderInfo,
                                        BarBlockRenderInfo& barBlockRenderInfo,
                                        const std::vector<SerpentineSegment>& segments,
                                        const bool measureOnly)
        {
        if (segments.empty() || bar.GetBlocks().empty())
            {
            return barBlockRenderInfo.m_middlePointOfBarEnd;
            }

        const auto& barBlock = bar.GetBlocks().front();
        const BlockColors blockColors{ ResolveBlockColors(bar, barBlock) };
        barRenderInfo.m_barWidth = CalcBarWidth(bar, barBlock, barRenderInfo);
        const auto barWidth = static_cast<wxCoord>(barRenderInfo.m_barWidth);
        if (barWidth <= 0)
            {
            return barBlockRenderInfo.m_middlePointOfBarEnd;
            }

        const bool isHorizontal{ GetBarOrientation() == Orientation::Horizontal };
        const double rangeStart{ GetScalingAxis().GetRange().first };
        // Returns false when the point falls outside the axes. The caller then drops
        // the whole ribbon, rather than drawing a stray band across the plot.
        const auto plotPoint = [this, isHorizontal, rangeStart](const double scaleValue,
                                                                const double rowPosition,
                                                                wxPoint& resultPt)
        {
            return isHorizontal ?
                       GetPhysicalCoordinates(rangeStart + scaleValue, rowPosition, resultPt) :
                       GetPhysicalCoordinates(rowPosition, rangeStart + scaleValue, resultPt);
        };

        // trace the ribbon's center line run by run. Each row change is a turn.
        std::vector<wxPoint> centerLine;
        std::vector<wxRect> segmentRects;
        centerLine.reserve(segments.size() * 2);
        segmentRects.reserve(segments.size());
        // A turn is a corner one band wide, so a run ending in one stops half a band
        // short. Its outer edge then lands on the value, like an ordinary bar end.
        const wxCoord turnInset{ barWidth / 2 };
        for (size_t i = 0; i < segments.size(); ++i)
            {
            const auto& segment = segments[i];
            const double entryValue{ segment.m_forward ? segment.m_scaleStart :
                                                         segment.m_scaleEnd };
            const double exitValue{ segment.m_forward ? segment.m_scaleEnd : segment.m_scaleStart };
            wxPoint entryPt;
            wxPoint exitPt;
            wxPoint lowPt;
            wxPoint highPt;
            if (!plotPoint(entryValue, segment.m_rowAxisPosition, entryPt) ||
                !plotPoint(exitValue, segment.m_rowAxisPosition, exitPt) ||
                !plotPoint(segment.m_scaleStart, segment.m_rowAxisPosition, lowPt) ||
                !plotPoint(segment.m_scaleEnd, segment.m_rowAxisPosition, highPt))
                {
                return barBlockRenderInfo.m_middlePointOfBarEnd;
                }

            const bool insetEntry{ i > 0 };
            const bool insetExit{ i + 1 < segments.size() };
            const wxCoord insetCount{ static_cast<wxCoord>(insetEntry ? 1 : 0) +
                                      static_cast<wxCoord>(insetExit ? 1 : 0) };
            const wxCoord runLength{ isHorizontal ? std::abs(exitPt.x - entryPt.x) :
                                                    std::abs(exitPt.y - entryPt.y) };
            // A run too short for its turns would invert. Cap the inset at an even
            // share of the run instead of skipping it.
            const wxCoord appliedInset{ (insetCount > 0) ?
                                            std::min<wxCoord>(turnInset, runLength / insetCount) :
                                            0 };
            if (isHorizontal)
                {
                const wxCoord runSign{ (exitPt.x >= entryPt.x) ? 1 : -1 };
                entryPt.x += insetEntry ? runSign * appliedInset : 0;
                exitPt.x -= insetExit ? runSign * appliedInset : 0;
                }
            else
                {
                const wxCoord runSign{ (exitPt.y >= entryPt.y) ? 1 : -1 };
                entryPt.y += insetEntry ? runSign * appliedInset : 0;
                exitPt.y -= insetExit ? runSign * appliedInset : 0;
                }
            centerLine.push_back(entryPt);
            centerLine.push_back(exitPt);

            segmentRects.push_back(isHorizontal ?
                                       wxRect(std::min(lowPt.x, highPt.x), lowPt.y - (barWidth / 2),
                                              std::abs(highPt.x - lowPt.x), barWidth) :
                                       wxRect(lowPt.x - (barWidth / 2), std::min(lowPt.y, highPt.y),
                                              barWidth, std::abs(highPt.y - lowPt.y)));
            }

        // where the value label goes depends on how the ribbon finishes
        const auto& lastSegment{ segments.back() };
        const bool lastRunHangs{ !lastSegment.m_forward && lastSegment.m_scaleStart > 0 };
        const bool endsAtOrigin{ !lastSegment.m_forward && lastSegment.m_scaleStart <= 0 };
        if (lastRunHangs)
            {
            // The last run hangs in mid-air, so the label sits past its tip. Measure
            // it so the clearance does not scale with band width.
            const wxPoint tipPt{ centerLine.back() };
            barBlockRenderInfo.m_middlePointOfBarEnd = tipPt;
            bar.GetLabel().SetScaling(GetScaling());
            bar.GetLabel().SetDPIScaleFactor(GetDPIScaleFactor());
            bar.GetLabel().SetFont(GetBarAxis().GetFont());
            const wxRect labelBox{ bar.GetLabel().GetBoundingBox(barRenderInfo.m_dc) };
            const wxCoord spacing{ barRenderInfo.m_labelSpacingFromLine };
            if (isHorizontal)
                {
                // PlaceEndOfBarLabel() runs the label rightwards, away from the tip's
                // clear space. Shift left by the full label width plus its spacing.
                barBlockRenderInfo.m_middlePointOfBarEnd.x -= labelBox.GetWidth() + (2 * spacing);
                }
            else
                {
                // The band ends flat at the tip, so drop by the label height.
                // PlaceEndOfBarLabel() lifts it back to just under the tip.
                barBlockRenderInfo.m_middlePointOfBarEnd.y += labelBox.GetHeight() + (2 * spacing);
                }
            barRenderInfo.m_barRect = segmentRects.back();
            }
        else if (endsAtOrigin)
            {
            // the ribbon ends back at the origin, so box the label above the axis line
            barBlockRenderInfo.m_middlePointOfBarEnd = centerLine.back();
            barRenderInfo.m_barRect = segmentRects.back();
            }
        else
            {
            // The ribbon ends at its tip. Push out half a band to clear the feeding turn.
            const wxPoint tipPt{ centerLine.back() };
            const wxPoint priorPt{ centerLine[centerLine.size() - 2] };
            barBlockRenderInfo.m_middlePointOfBarEnd = tipPt;
            if (isHorizontal)
                {
                barBlockRenderInfo.m_middlePointOfBarEnd.x +=
                    (tipPt.x >= priorPt.x) ? turnInset : -turnInset;
                }
            else
                {
                barBlockRenderInfo.m_middlePointOfBarEnd.y +=
                    (tipPt.y >= priorPt.y) ? turnInset : -turnInset;
                }
            barRenderInfo.m_barRect = segmentRects.back();
            }

        if (measureOnly)
            {
            return barBlockRenderInfo.m_middlePointOfBarEnd;
            }

        wxBrush ribbonBrush{ barBlock.GetBrush() };
        ribbonBrush.SetColour(blockColors.m_fill);

        // the decal rides on the first run, so save its rect before the ribbon takes it
        const wxRect firstRunRect{ segmentRects.front() };

        // outline the ribbon like an ordinary bar block, so folded and unfolded bars match
        const wxPen contrastPen{ Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                     *wxTRANSPARENT_PEN :
                                     wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) } };
        wxPen ribbonPen{ barBlock.GetOutlinePen().IsOk() ? barBlock.GetOutlinePen() : contrastPen };
        // an explicit block color drops the outline again, as the color-filled path does
        if (barBlock.GetColor().IsOk() && !barBlock.GetColor().IsTransparent())
            {
            ribbonPen = contrastPen;
            }
        // a transparent bar needs a contrasting outline to be visible, unless none
        // was asked for
        if (bar.GetOpacity() == wxALPHA_TRANSPARENT && ribbonPen.IsOk() &&
            !ribbonPen.GetColour().IsTransparent())
            {
            ribbonPen = wxPen{ Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                GetPlotOrCanvasColor()) };
            }

        auto ribbon = std::make_unique<SerpentineRibbon>(
            Wisteria::GraphItems::GraphItemInfo{ barBlock.GetSelectionLabel().GetText() }
                .Pen(ribbonPen)
                .Brush(ribbonBrush)
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .ShowLabelWhenSelected(true),
            std::move(centerLine), std::move(segmentRects), barRenderInfo.m_barWidth,
            barRenderInfo.m_scaledShadowOffset, m_showSerpentineFoldArrows, isHorizontal);
        // Turns land on the plot edge, so the outline pen straddling it sits half
        // outside. Pad the clip by that much so corners are not shaved flat.
        wxRect ribbonClipRect{ GetDrawArea() };
        ribbonClipRect.Inflate(wxRound(ScaleToScreenAndCanvas(2)));
        ribbon->SetClippingRect(ribbonClipRect);
        AddObject(std::move(ribbon));

        if (barBlock.GetDecal().IsShown() && !barBlock.GetDecal().GetText().empty())
            {
            barRenderInfo.m_decals.push_back(
                BuildBarBlockDecal(bar, barBlock, firstRunRect, barRenderInfo));
            }

        return barBlockRenderInfo.m_middlePointOfBarEnd;
        }

    //-----------------------------------
    BarChart::SerpentineRibbon::SerpentineRibbon(const GraphItems::GraphItemInfo& itemInfo,
                                                 std::vector<wxPoint> centerLine,
                                                 std::vector<wxRect> segmentRects,
                                                 const double thickness, const wxCoord shadowOffset,
                                                 const bool showFoldArrows,
                                                 const bool barsAreHorizontal)
        : GraphItems::GraphItemBase(itemInfo), m_centerLine(std::move(centerLine)),
          m_segmentRects(std::move(segmentRects)), m_thickness(thickness),
          m_shadowOffset(shadowOffset), m_showFoldArrows(showFoldArrows),
          m_barsAreHorizontal(barsAreHorizontal)
        {
        for (const auto& segmentRect : m_segmentRects)
            {
            m_boundingBox =
                m_boundingBox.IsEmpty() ? segmentRect : m_boundingBox.Union(segmentRect);
            }
        // the outline and the drop shadow both reach past the runs themselves
        if (!m_boundingBox.IsEmpty())
            {
            m_boundingBox.Inflate(wxRound(thickness / 2) + shadowOffset + 1);
            }
        }

    //-----------------------------------
    wxRect BarChart::SerpentineRibbon::GetBoundingBox([[maybe_unused]] wxDC& dc) const
        {
        return m_boundingBox;
        }

    //-----------------------------------
    void BarChart::SerpentineRibbon::SetBoundingBox([[maybe_unused]] const wxRect& rect,
                                                    [[maybe_unused]] wxDC& dc,
                                                    [[maybe_unused]] const double scaling)
        {
        }

    //-----------------------------------
    bool BarChart::SerpentineRibbon::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        return std::ranges::any_of(m_segmentRects, [pt](const auto& segmentRect)
                                   { return segmentRect.Contains(pt); });
        }

    //-----------------------------------
    void BarChart::SerpentineRibbon::Offset(const int xOffset, const int yOffset)
        {
        for (auto& linePt : m_centerLine)
            {
            linePt += wxPoint(xOffset, yOffset);
            }
        for (auto& segmentRect : m_segmentRects)
            {
            segmentRect.Offset(xOffset, yOffset);
            }
        m_boundingBox.Offset(xOffset, yOffset);
        }

    //-----------------------------------
    wxGraphicsPath BarChart::SerpentineRibbon::BuildPath(wxGraphicsContext* gc,
                                                         const wxCoord xOffset,
                                                         const wxCoord yOffset,
                                                         const wxCoord endExtension) const
        {
        // pushes an end of the band out along its own run, away from its neighbor
        const auto extendedEnd =
            [this, endExtension](const size_t endIndex, const size_t neighborIndex)
        {
            wxPoint endPt{ m_centerLine[endIndex] };
            if (endExtension == 0)
                {
                return endPt;
                }
            const wxPoint neighborPt{ m_centerLine[neighborIndex] };
            if (endPt.x != neighborPt.x)
                {
                endPt.x += (endPt.x > neighborPt.x) ? endExtension : -endExtension;
                }
            else if (endPt.y != neighborPt.y)
                {
                endPt.y += (endPt.y > neighborPt.y) ? endExtension : -endExtension;
                }
            return endPt;
        };

        wxGraphicsPath path = gc->CreatePath();
        const wxPoint startPt{ extendedEnd(0, 1) };
        path.MoveToPoint(startPt.x + xOffset, startPt.y + yOffset);
        for (size_t i = 1; i + 1 < m_centerLine.size(); ++i)
            {
            path.AddLineToPoint(m_centerLine[i].x + xOffset, m_centerLine[i].y + yOffset);
            }
        const wxPoint endPt{ extendedEnd(m_centerLine.size() - 1, m_centerLine.size() - 2) };
        path.AddLineToPoint(endPt.x + xOffset, endPt.y + yOffset);

        return path;
        }

    //-----------------------------------
    wxRect BarChart::SerpentineRibbon::Draw(wxDC& dc) const
        {
        if (!IsShown() || m_centerLine.size() < 2 || m_thickness <= 0 || m_boundingBox.IsEmpty())
            {
            return m_boundingBox;
            }

        const GraphItems::GraphicsContextFallback gcf{ &dc, m_boundingBox };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return m_boundingBox;
            }

        if (GetClippingRect().has_value())
            {
            const auto& clipRect = GetClippingRect().value();
            gc->Clip(clipRect.GetX(), clipRect.GetY(), clipRect.GetWidth(), clipRect.GetHeight());
            }
        gc->SetBrush(*wxTRANSPARENT_BRUSH);

        const auto strokeBand =
            [gc](const wxGraphicsPath& path, const wxColour& color, const double width)
        {
            if (width <= 0 || !color.IsOk())
                {
                return;
                }
            gc->SetPen(
                gc->CreatePen(wxGraphicsPenInfo(color, width).Cap(wxCAP_BUTT).Join(wxJOIN_ROUND)));
            gc->StrokePath(path);
        };

        // draw the shadow first, so the band covers where they overlap
        if (GetShadowType() != ShadowType::NoDisplay && GetBrush().GetColour().IsOpaque() &&
            m_shadowOffset > 0)
            {
            strokeBand(BuildPath(gc, m_shadowOffset, m_shadowOffset, 0),
                       GraphItemBase::GetShadowColor(), m_thickness);
            }

        // a transparent pen is still valid and carries opaque black, so test its
        // style, not its color alpha
        const bool hasOutline{ GetPen().IsOk() && GetPen().GetStyle() != wxPENSTYLE_TRANSPARENT &&
                               !GetPen().GetColour().IsTransparent() };
        if (!hasOutline)
            {
            strokeBand(BuildPath(gc, 0, 0, 0), GetBrush().GetColour(), m_thickness);
            DrawFoldArrows(gc);
            return m_boundingBox;
            }

        // the outline runs past both ends to close them, then the fill sits just inside
        const wxCoord outlineWidth{ std::max(1, GetPen().GetWidth()) };
        strokeBand(BuildPath(gc, 0, 0, outlineWidth), GetPen().GetColour(), m_thickness);
        strokeBand(BuildPath(gc, 0, 0, 0), GetBrush().GetColour(),
                   m_thickness - (outlineWidth * 2));

        DrawFoldArrows(gc);

        return m_boundingBox;
        }

    //-----------------------------------
    void BarChart::SerpentineRibbon::DrawFoldArrows(wxGraphicsContext* gc) const
        {
        if (!m_showFoldArrows || gc == nullptr || m_centerLine.size() < 4 || m_thickness <= 0)
            {
            return;
            }

        // read and build points as (major is along a run, minor is across the rows)
        const auto majorOf = [this](const wxPoint& linePt) noexcept
        {
            return m_barsAreHorizontal ? static_cast<double>(linePt.x) :
                                         static_cast<double>(linePt.y);
        };
        const auto minorOf = [this](const wxPoint& linePt) noexcept
        {
            return m_barsAreHorizontal ? static_cast<double>(linePt.y) :
                                         static_cast<double>(linePt.x);
        };
        const auto makePt = [this](const double major, const double minor) noexcept
        {
            return m_barsAreHorizontal ? wxPoint2DDouble(major, minor) :
                                         wxPoint2DDouble(minor, major);
        };

        const wxColour arrowColor{ Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
            GetBrush().GetColour()) };

        const double arrowCap{ ScaleToScreenAndCanvas(28) };
        const double shaftWidth{ std::max(
            1.5, std::min(m_thickness * 0.09, ScaleToScreenAndCanvas(3))) };
        const size_t turnCount{ (m_centerLine.size() / 2) - 1 };

        for (size_t turn = 0; turn < turnCount; ++turn)
            {
            const wxPoint& runInStart{ m_centerLine[2 * turn] };
            const wxPoint& runInEnd{ m_centerLine[(2 * turn) + 1] };
            const wxPoint& runOutStart{ m_centerLine[(2 * turn) + 2] };
            const wxPoint& runOutEnd{ m_centerLine[(2 * turn) + 3] };

            const double dirIn{ (majorOf(runInEnd) >= majorOf(runInStart)) ? 1.0 : -1.0 };
            const double shortRun{ std::min(std::abs(majorOf(runInEnd) - majorOf(runInStart)),
                                            std::abs(majorOf(runOutEnd) - majorOf(runOutStart))) };

            const double shaftRun{ std::min(
                { shortRun * 0.3, m_thickness * 0.9, arrowCap * 2.0 }) };
            const double headLen{ std::min({ shaftRun * 0.8, m_thickness * 0.4, arrowCap }) };
            if (shaftRun <= 2.0 || headLen <= 1.5)
                {
                continue;
                }
            const double headHalf{ std::min(
                { headLen * 0.85, m_thickness * 0.22, arrowCap * 0.7 }) };
            const double bulge{ std::min(
                { std::abs(minorOf(runOutStart) - minorOf(runInEnd)) * 0.45, m_thickness * 0.32,
                  arrowCap * 1.4 }) };

            // tail on the incoming run, curving around the turn to the head on the outgoing run
            const wxPoint2DDouble tail{ makePt(majorOf(runInEnd) - (dirIn * shaftRun),
                                               minorOf(runInEnd)) };
            const wxPoint2DDouble ctrlIn{ makePt(majorOf(runInEnd) + (dirIn * bulge),
                                                 minorOf(runInEnd)) };
            const wxPoint2DDouble ctrlOut{ makePt(majorOf(runOutStart) + (dirIn * bulge),
                                                  minorOf(runOutStart)) };
            const wxPoint2DDouble neck{ makePt(majorOf(runOutStart) - (dirIn * shaftRun),
                                               minorOf(runOutStart)) };
            const wxPoint2DDouble tip{ makePt(majorOf(runOutStart) - (dirIn * (shaftRun + headLen)),
                                              minorOf(runOutStart)) };
            const wxPoint2DDouble neckLeft{ makePt(majorOf(runOutStart) - (dirIn * shaftRun),
                                                   minorOf(runOutStart) - headHalf) };
            const wxPoint2DDouble neckRight{ makePt(majorOf(runOutStart) - (dirIn * shaftRun),
                                                    minorOf(runOutStart) + headHalf) };

            wxGraphicsPath shaft{ gc->CreatePath() };
            shaft.MoveToPoint(tail.m_x, tail.m_y);
            shaft.AddCurveToPoint(ctrlIn.m_x, ctrlIn.m_y, ctrlOut.m_x, ctrlOut.m_y, neck.m_x,
                                  neck.m_y);
            gc->SetPen(gc->CreatePen(
                wxGraphicsPenInfo(arrowColor, shaftWidth).Cap(wxCAP_ROUND).Join(wxJOIN_ROUND)));
            gc->SetBrush(*wxTRANSPARENT_BRUSH);
            gc->StrokePath(shaft);

            wxGraphicsPath head{ gc->CreatePath() };
            head.MoveToPoint(tip.m_x, tip.m_y);
            head.AddLineToPoint(neckLeft.m_x, neckLeft.m_y);
            head.AddLineToPoint(neckRight.m_x, neckRight.m_y);
            head.CloseSubpath();
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->SetBrush(wxBrush{ arrowColor });
            gc->FillPath(head);
            }
        }
    } // namespace Wisteria::Graphs
