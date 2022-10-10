///////////////////////////////////////////////////////////////////////////////
// Name:        barchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////


#include "barchart.h"

using namespace Wisteria::Colors;
using namespace Wisteria::GraphItems;

namespace Wisteria::Graphs
    {
    //-----------------------------------
    BarChart::BarChart(Wisteria::Canvas* canvas) :
        Graph2D(canvas)
        { SetBarOrientation(m_barOrientation); }

    //-----------------------------------
    void BarChart::UpdateBarLabel(Bar& bar)
        {
        const double grandTotal = std::accumulate(
            GetBars().cbegin(), GetBars().cend(), 0.0,
            [](auto lhv, auto rhv) noexcept
            { return lhv + rhv.GetLength(); });

        const double percentage = safe_divide<double>(bar.GetLength(), grandTotal) * 100;
        const wxString labelStr =
            (bar.GetLength() == 0 ||
                GetBinLabelDisplay() == BinLabelDisplay::NoDisplay) ?
                wxString(wxEmptyString) :
            (GetBinLabelDisplay() == BinLabelDisplay::BinName) ?
                bar.GetAxisLabel().GetText() :
            (GetBinLabelDisplay() == BinLabelDisplay::BinValue) ?
                wxNumberFormatter::ToString(bar.GetLength(), 0,
                                        Settings::GetDefaultNumberFormat()) :
            (GetBinLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                // if less than 1%, then use higher precision so that it doesn't just show as "0%"
                wxNumberFormatter::ToString(percentage, (percentage < 1) ? 2 : 0,
                                        wxNumberFormatter::Style::Style_NoTrailingZeroes) +
                L"%" :
                // BinValueAndPercentage
                wxNumberFormatter::ToString(bar.GetLength(), 0, Settings::GetDefaultNumberFormat()) +
                L" (" +
                wxNumberFormatter::ToString(percentage, (percentage < 1) ? 2 : 0,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes) +
                L"%)";
        bar.GetLabel().SetText(labelStr);
        }

    //-----------------------------------
    void BarChart::AddBarGroup(const wxString& firstBarLabel, const wxString& lastBarLabel,
                         std::optional<wxString> decal,
                         std::optional<wxColour> color,
                         std::optional<wxBrush> brush)
        {
        const auto firstBar = FindBar(firstBarLabel);
        const auto lastBar = FindBar(lastBarLabel);
        if (firstBar && lastBar)
            {
            m_barGroups.push_back(
                {
                std::make_pair(firstBar.value(), lastBar.value()),
                decal.has_value() ? decal.value() : wxString(wxEmptyString),
                brush.has_value() ? brush.value() : GetBrushScheme()->GetBrush(0),
                color.has_value() ? color.value() :
                    (GetColorScheme() ? GetColorScheme()->GetColor(0) : wxTransparentColour)
                });
            }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': bar label not found when adding bar group."),
                firstBar.has_value() ? lastBarLabel : firstBarLabel).ToUTF8());
            }
        }

    //-----------------------------------
    std::optional<size_t> BarChart::FindBar(const wxString& axisLabel)
        {
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (GetBars()[i].GetAxisLabel().GetText().CmpNoCase(axisLabel) == 0)
                { return i; }
            }
        return std::nullopt;
        }

    //-----------------------------------
    std::optional<double> BarChart::FindBarBlockStart(const size_t barIndex,
                                                      const wxString& blockTag) const
        {
        if (GetScalingAxis().IsReversed())
            { return std::nullopt; }
        wxASSERT_MSG(barIndex < GetBars().size(), L"Bar index out of range!");
        if (barIndex >= GetBars().size())
            { return std::nullopt; }

        const auto& bar = GetBars()[barIndex];

        auto block = bar.FindBlock(blockTag);
        if (block == bar.GetBlocks().cend())
            { return std::nullopt; }
        return std::accumulate(bar.GetBlocks().cbegin(), block,
            GetScalingAxis().GetRange().first,
            [](const auto& initVal, const auto& val) noexcept
            { return initVal + val.GetLength(); });
        }

    //-----------------------------------
    std::optional<double> BarChart::FindBarBlockEnd(const size_t barIndex,
                                                    const wxString& blockTag) const
        {
        if (GetScalingAxis().IsReversed())
            { return std::nullopt; }
        wxASSERT_MSG(barIndex < GetBars().size(), L"Bar index out of range!");
        if (barIndex >= GetBars().size())
            { return std::nullopt; }

        const auto& bar = GetBars()[barIndex];

        auto block = bar.FindBlock(blockTag);
        if (block == bar.GetBlocks().cend())
            { return std::nullopt; }
        return std::accumulate(bar.GetBlocks().cbegin(), ++block,
            GetScalingAxis().GetRange().first,
            [](const auto& initVal, const auto& val) noexcept
            { return initVal + val.GetLength(); });
        }

    //-----------------------------------
    void BarChart::AddFirstBarBracket(const wxString& firstBarBlock,
                                      const wxString& lastBarBlock, const wxString& bracketLabel)
        {
        wxASSERT_MSG(GetBars().size(), L"No bars available when adding an axis bracket!");
        if (GetBars().size() == 0)
            {
            throw std::runtime_error(
                _(L"No bars available when adding an axis bracket.").ToUTF8());
            }

        const auto blocksStart = FindBarBlockStart(0, firstBarBlock);
        const auto blocksEnd = FindBarBlockEnd(0, lastBarBlock);

        if (blocksStart.has_value() && blocksEnd.has_value())
            {
            GetScalingAxis().AddBracket(
                Axis::AxisBracket(blocksStart.value(), blocksEnd.value(),
                    safe_divide(blocksStart.value() + blocksEnd.value(), 2.0),
                    bracketLabel));
            }
        else if (!blocksStart.has_value())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' not found when adding an axis bracket."),
                firstBarBlock).ToUTF8());
            }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' not found when adding an axis bracket."),
                lastBarBlock).ToUTF8());
            }
        }

    //-----------------------------------
    void BarChart::AddFirstBarBracketRE(const wxString& firstBarBlockPattern,
                                        const wxString& lastBarBlockPattern,
                                        const wxString& bracketLabel)
        {
        wxASSERT_MSG(GetBars().size(), L"No bars available when adding an axis bracket!");
        if (GetBars().size() == 0)
            {
            throw std::runtime_error(
                _(L"No bars available when adding an axis bracket.").ToUTF8());
            }

        const auto firstBlock = GetBars()[0].FindFirstBlockRE(firstBarBlockPattern);
        const auto lastBlock = GetBars()[0].FindLastBlockRE(lastBarBlockPattern);

        if (firstBlock == GetBars()[0].GetBlocks().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                firstBarBlockPattern).ToUTF8());
            }
        if (lastBlock == GetBars()[0].GetBlocks().crend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                lastBarBlockPattern).ToUTF8());
            }

        AddFirstBarBracket(firstBlock->GetTag(), lastBlock->GetTag(), bracketLabel);
        }

    //-----------------------------------
    void BarChart::AddLastBarBracket(const wxString& firstBarBlock,
                                     const wxString& lastBarBlock, const wxString& bracketLabel)
        {
        wxASSERT_MSG(GetBars().size(), L"No bars when adding an axis bracket!");
        if (GetBars().size() == 0)
            {
            throw std::runtime_error(
                _(L"No bars when adding an axis bracket.").ToUTF8());
            }
        const size_t barIndex{ GetBars().size() - 1 };

        const auto blocksStart = FindBarBlockStart(barIndex, firstBarBlock);
        const auto blocksEnd = FindBarBlockEnd(barIndex, lastBarBlock);

        if (blocksStart.has_value() && blocksEnd.has_value())
            {
            if (GetBarOrientation() == Orientation::Vertical)
                { MirrorYAxis(true); }
            else
                { MirrorXAxis(true); }
            GetOppositeScalingAxis().AddBracket(
                Axis::AxisBracket(blocksStart.value(), blocksEnd.value(),
                    safe_divide(blocksStart.value() + blocksEnd.value(), 2.0),
                    bracketLabel));
            }
        else if (!blocksStart.has_value())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' not found when adding an axis bracket."),
                firstBarBlock).ToUTF8());
            }
        else
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' not found when adding an axis bracket."),
                lastBarBlock).ToUTF8());
            }
        }

    //-----------------------------------
    void BarChart::AddLastBarBracketRE(const wxString& firstBarBlockPattern,
                                       const wxString& lastBarBlockPattern,
                                       const wxString& bracketLabel)
        {
        wxASSERT_MSG(GetBars().size(), L"No bars when adding an axis bracket!");
        if (GetBars().size() == 0)
            {
            throw std::runtime_error(
                _(L"No bars when adding an axis bracket.").ToUTF8());
            }
        const size_t barIndex{ GetBars().size() - 1 };

        const auto firstBlock = GetBars()[barIndex].FindFirstBlockRE(firstBarBlockPattern);
        const auto lastBlock = GetBars()[barIndex].FindLastBlockRE(lastBarBlockPattern);

        if (firstBlock == GetBars()[barIndex].GetBlocks().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                firstBarBlockPattern).ToUTF8());
            }
        if (lastBlock == GetBars()[barIndex].GetBlocks().crend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                lastBarBlockPattern).ToUTF8());
            }

        AddLastBarBracket(firstBlock->GetTag(), lastBlock->GetTag(), bracketLabel);
        }

    //-----------------------------------
    void BarChart::SetBarOrientation(const Orientation orient)
        {
        if (GetBarOrientation() != orient)
            {
            GetBarAxis().ClearBrackets();
            GetScalingAxis().ClearBrackets();
            }
        m_barOrientation = orient;
        // if both axis grid lines are turned off then don't do anything, but if one of them
        // is turned on then intelligently display just the one relative to the new orientation
        if (GetBarAxis().GetGridlinePen().IsOk() || GetScalingAxis().GetGridlinePen().IsOk())
            {
            const wxPen gridlinePen = (GetBarAxis().GetGridlinePen().IsOk() ?
                GetBarAxis().GetGridlinePen() : GetScalingAxis().GetGridlinePen());
            GetBarAxis().GetGridlinePen() = wxNullPen;
            GetScalingAxis().GetGridlinePen() = gridlinePen;
            }
        }

    //-----------------------------------
    void BarChart::SetBarsPerDefaultCanvasSize(const size_t barsPerDefaultCanvasSize)
        {
        m_barsPerDefaultCanvasSize = barsPerDefaultCanvasSize;
        UpdateCanvasForBars();
        }

    //-----------------------------------
    void BarChart::UpdateCanvasForBars()
        {
        if (GetBars().size() > GetBarsPerDefaultCanvasSize())
            {
            GetCanvas()->SetCanvasMinHeightDIPs(GetCanvas()->GetDefaultCanvasHeightDIPs() *
                std::ceil(safe_divide<double>(GetBars().size(), GetBarsPerDefaultCanvasSize())));
            }
        }

    //-----------------------------------
    void BarChart::AddBar(Bar bar, const bool adjustScalingAxis /*= true*/)
        {
        m_bars.push_back(bar);

        const auto customWidth = bar.GetCustomWidth().has_value() ?
                                 safe_divide<double>(bar.GetCustomWidth().value(), 2) :
                                 0;

        // adjust the bar axis to hold the bar
        if (m_highestBarAxisPosition < bar.GetAxisPosition() + customWidth)
            { m_highestBarAxisPosition = bar.GetAxisPosition() + customWidth; }

        if (m_lowestBarAxisPosition > bar.GetAxisPosition() - customWidth)
            { m_lowestBarAxisPosition = bar.GetAxisPosition() - customWidth; }

        GetBarAxis().SetRange(
                m_lowestBarAxisPosition - GetBarAxis().GetInterval(),
                m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                GetBarAxis().GetDisplayInterval());
        if (bar.GetAxisLabel().IsShown() && bar.GetAxisLabel().GetText().length())
            { GetBarAxis().SetCustomLabel(bar.GetAxisPosition(), bar.GetAxisLabel()); }

        if (adjustScalingAxis)
            { UpdateScalingAxisFromBar(bar); }
        }

    //-----------------------------------
    void BarChart::UpdateScalingAxisFromBar(const Bar& bar)
        {
        // where the bar actually ends on the scaling axis
        const auto barEnd = bar.GetLength() +
            bar.GetCustomScalingAxisStartPosition().value_or(0);

        // if this bar is longer than previous ones, then update the scaling
        if (m_longestBarLength < barEnd)
            {
            m_longestBarLength = barEnd;
            GetScalingAxis().SetRange(0, m_longestBarLength, 0,
                // add a little extra padding to the scaling axis if we are using labels
                IsShowingBarLabels());
            const auto originalRange = GetScalingAxis().GetRange();

            // tweak scaling
            if (m_longestBarLength >= 50'000)
                {
                GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                    next_interval(m_longestBarLength, 5),
                    GetScalingAxis().GetPrecision(), 10'000, 1);
                }
            else if (m_longestBarLength >= 20'000)
                {
                GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                    next_interval(m_longestBarLength, 4),
                    GetScalingAxis().GetPrecision(), 5'000, 1);
                }
            else if (m_longestBarLength >= 10'000)
                {
                GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                    next_interval(m_longestBarLength, 4),
                    GetScalingAxis().GetPrecision(), 1'000, 1);
                }
            else if (m_longestBarLength >= 1'500)
                {
                GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                    next_interval(m_longestBarLength, 4),
                    GetScalingAxis().GetPrecision(), 500, 1);
                }
            else if (m_longestBarLength > 300)
                {
                GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                    next_interval(m_longestBarLength, 3),
                    GetScalingAxis().GetPrecision(), 100, 1);
                }

            // if showing labels and we just re-adjusted the range, then add an
            // extra interval for the label
            if (const auto currentRange = GetScalingAxis().GetRange();
                IsShowingBarLabels() && originalRange != currentRange)
                {
                const auto extraSpaceAfterBar{ m_longestBarLength -
                                               (currentRange.second - GetScalingAxis().GetInterval()) };
                const auto barPercentOfLastInterval{
                    safe_divide<double>(extraSpaceAfterBar, GetScalingAxis().GetInterval()) };
                // but only add a new interval if the longest bar is consuming more than
                // 20% of the current last interval; otherwise, there already is plenty of space
                // for the label
                if (barPercentOfLastInterval > math_constants::fifth)
                    {
                    GetScalingAxis().SetRange(currentRange.first,
                        currentRange.second + GetScalingAxis().GetInterval(),
                        GetScalingAxis().GetPrecision(),
                        GetScalingAxis().GetInterval(),
                        GetScalingAxis().GetDisplayInterval());
                    }
                }
            }

        UpdateCanvasForBars();
        }

    //-----------------------------------
    void BarChart::SortBars(std::vector<wxString> labels,
                            const Wisteria::SortDirection direction)
        {
        wxASSERT_LEVEL_2_MSG(IsSortable(),
                             L"Bars are not sortable. "
                              "Call SetSortable(true) prior to calling SortBars().");
        wxASSERT_LEVEL_2_MSG(labels.size() == GetBars().size(),
                             L"Number of labels need to match the number of bars when sorting.");
        m_sortDirection = direction;
        if (!IsSortable() || direction == SortDirection::NoSort ||
            GetBarAxis().IsReversed())
            { return; }
        else if (labels.size() != GetBars().size())
            {
            throw std::runtime_error(wxString::Format(
                _(L"Bar label count (%zu) is different from bar count (%zu) when sorting."),
                labels.size(), GetBars().size()).ToUTF8());
            }

        // adapted from https://stackoverflow.com/questions/838384/reorder-vector-using-a-vector-of-indices
        std::vector<size_t> indices;
        const auto reorderBars = [&indices, this]()
            {
            for (size_t s{ 1 }, d{ 0 }; s < indices.size(); ++s)
                {
                for (d = indices[s]; d < s; d = indices[d]);
                if (d == s)
                    {
                    while (d = indices[d], d != s)
                        {
                        std::swap(GetBars().at(s).m_axisPosition, GetBars().at(d).m_axisPosition);
                        }
                    }
                }

            // sort the bars back into the proper axis positions
            std::sort(GetBars().begin(), GetBars().end(),
                [](const auto& lhv, const auto& rhv) noexcept
                    {
                    return lhv.GetAxisPosition() < rhv.GetAxisPosition();
                    });
            };
        
        // reorder the provided labels (if necessary) to match the sorting direction
        if ((direction == SortDirection::SortDescending && GetBarOrientation() == Orientation::Vertical) ||
            (direction == SortDirection::SortAscending && GetBarOrientation() == Orientation::Horizontal))
            { std::reverse(labels.begin(), labels.end()); }
        // get the indices into the bars based on the order of the provided labels
        for (const auto& label : labels)
            {
            const auto foundPos = std::find_if(GetBars().cbegin(), GetBars().cend(),
                [&label](const auto& bar)
                    {
                    return bar.GetAxisLabel().GetText().CmpNoCase(label) == 0;
                    });
            if (foundPos == GetBars().cend())
                {
                throw std::runtime_error(wxString::Format(
                    _(L"'%s': bar label not found when sorting."), label).ToUTF8());
                }
            indices.push_back(std::distance(GetBars().cbegin(), foundPos));
            }

        const bool isDisplayingOuterLabels = GetBarAxis().IsShowingOuterLabels();
        GetBarAxis().ClearCustomLabels();

        reorderBars();
        // reset the bar axis's labels
        for (const auto& bar : GetBars())
            { GetBarAxis().SetCustomLabel(bar.GetAxisPosition(), bar.GetAxisLabel()); }

        GetBarAxis().ShowOuterLabels(isDisplayingOuterLabels);
        wxMemoryDC measureDC;
        GetCanvas()->CalcAllSizes(measureDC);
        }

    //-----------------------------------
    void BarChart::SortBars(const BarSortComparison sortMethod,
                            const Wisteria::SortDirection direction)
        {
        wxASSERT_LEVEL_2_MSG(IsSortable(),
                             L"Bars are not sortable. "
                              "Call SetSortable(true) prior to calling SortBars().");
        m_sortDirection = direction;
        if (!IsSortable() || direction == SortDirection::NoSort ||
            GetBarAxis().IsReversed())
            { return; }

        // bar grpoups connected to bars' positions will need to be removed
        m_barGroups.clear();

        const bool isDisplayingOuterLabels = GetBarAxis().IsShowingOuterLabels();
        GetBarAxis().ClearCustomLabels();

        // sorts smallest-to-largest
        if (sortMethod == BarSortComparison::SortByBarLength)
            { std::sort(m_bars.begin(), m_bars.end()); }
        else
            {
            std::sort(m_bars.begin(), m_bars.end(),
                [](const Bar& left, const Bar& right)
                {
                return wxUILocale::GetCurrent().CompareStrings(
                    left.GetAxisLabel().GetText(),
                    right.GetAxisLabel().GetText(), wxCompare_CaseInsensitive) < 0;
                });
            }
        // Because we start at the origin, descending when horizontal goes the opposite way internally.
        // When it's displayed, descending will be shown as going largest-to-smallest as one
        // would expect.
        if ((direction == SortDirection::SortAscending && GetBarOrientation() == Orientation::Vertical) ||
            (direction == SortDirection::SortDescending && GetBarOrientation() == Orientation::Horizontal))
            {
            for (auto pos = m_bars.begin();
                pos != m_bars.end();
                ++pos)
                {
                pos->SetAxisPosition(m_highestBarAxisPosition = m_lowestBarAxisPosition +
                    (GetBarAxis().GetInterval()*(pos-m_bars.begin())));
                GetBarAxis().SetCustomLabel(pos->GetAxisPosition(), pos->GetAxisLabel());
                }
            GetBarAxis().SetRange((m_lowestBarAxisPosition-GetBarAxis().GetInterval()),
                m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                GetBarAxis().GetDisplayInterval());
            }
        else
            {
            for (auto pos = m_bars.rbegin();
                pos != m_bars.rend();
                ++pos)
                {
                pos->SetAxisPosition(m_highestBarAxisPosition = m_lowestBarAxisPosition +
                    (GetBarAxis().GetInterval()*(pos-m_bars.rbegin())));
                GetBarAxis().SetCustomLabel(pos->GetAxisPosition(), pos->GetAxisLabel());
                }
            GetBarAxis().SetRange((m_lowestBarAxisPosition-GetBarAxis().GetInterval()),
                m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                GetBarAxis().GetDisplayInterval());
            }
        GetBarAxis().ShowOuterLabels(isDisplayingOuterLabels);
        wxMemoryDC measureDC;
        GetCanvas()->CalcAllSizes(measureDC);
        }

    //-----------------------------------
    void BarChart::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        // if no bars then just draw a blank 10x10 grid
        if (GetBars().size() == 0)
            {
            GetRightYAxis().Reset();
            GetBarAxis().Reset();
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            GetTopXAxis().Reset();
            GetScalingAxis().Reset();
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        const wxCoord barSpacing = m_includeSpacesBetweenBars ? ScaleToScreenAndCanvas(10) : 0;
        const wxCoord scaledShadowOffset = ScaleToScreenAndCanvas(GetShadowOffset());
        const wxCoord labelSpacingFromLine = ScaleToScreenAndCanvas(5);
        
        std::vector<std::shared_ptr<GraphItems::Label>> decals;
        double barWidth{ 0 };
        wxRect barRect;
        wxImage scaledCommonImg;

        // main bar rendering
        const auto drawBar = [&](auto& bar, const bool measureOnly)
            {
            wxPoint middlePointOfBarEnd;
            wxCoord axisOffset{ 0 };
            wxPoint boxPoints[4]{ { 0, 0 } };
            wxPoint arrowPoints[7]{ { 0, 0 } };
            for (const auto& barBlock : bar.GetBlocks())
                {
                if (GetBarOrientation() == Orientation::Horizontal)
                    {
                    /* if the bar (or block) is set to cover a specific range
                       (e.g., histograms do this) then calculate
                       the width of the bar based on the coordinates.
                       Otherwise, just divvy up the bars evenly to fit the plot window.*/
                    if (barBlock.GetCustomWidth().has_value())
                        {
                        wxPoint topRightPointOfBar, bottomRightPointOfBar;
                        GetPhyscialCoordinates(barBlock.GetLength()/*offset doesn't matter here*/,
                            bar.GetAxisPosition() -
                            safe_divide<double>(barBlock.GetCustomWidth().value(),2), topRightPointOfBar);
                        GetPhyscialCoordinates(barBlock.GetLength(),
                            bar.GetAxisPosition() +
                            safe_divide<double>(barBlock.GetCustomWidth().value(),2), bottomRightPointOfBar);
                        barWidth = (topRightPointOfBar.y - bottomRightPointOfBar.y);
                        }
                    else if (bar.GetCustomWidth().has_value())
                        {
                        wxPoint topRightPointOfBar, bottomRightPointOfBar;
                        GetPhyscialCoordinates(barBlock.GetLength()/*offset doesn't matter here*/,
                            bar.GetAxisPosition() -
                            safe_divide<double>(bar.GetCustomWidth().value(),2), topRightPointOfBar);
                        GetPhyscialCoordinates(barBlock.GetLength(),
                            bar.GetAxisPosition() +
                            safe_divide<double>(bar.GetCustomWidth().value(),2), bottomRightPointOfBar);
                        barWidth = (topRightPointOfBar.y - bottomRightPointOfBar.y);
                        }
                    else
                        {
                        const size_t barSlots = GetBarSlotCount();
                        const size_t overallBarSpacing = (barSpacing*(barSlots-1));
                        barWidth = safe_divide<double>(
                            GetPlotAreaBoundingBox().GetHeight() -
                            (overallBarSpacing < GetPlotAreaBoundingBox().GetWidth()+barSlots ? overallBarSpacing : 0),
                            (barSlots+1));
                        }

                    wxCoord lineXStart{ 0 }; // set the left (starting point) of the bar
                    if (bar.GetCustomScalingAxisStartPosition().has_value())
                        {
                        GetPhyscialCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                            axisOffset + barBlock.GetLength(), bar.GetAxisPosition(), middlePointOfBarEnd);
                        wxPoint customStartPt;
                        GetPhyscialCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                            axisOffset, bar.GetAxisPosition(), customStartPt);
                        lineXStart = customStartPt.x + (axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                        }
                    else
                        {
                        // right side of the block
                        GetPhyscialCoordinates(GetScalingAxis().GetRange().first + axisOffset +
                            barBlock.GetLength(), bar.GetAxisPosition(), middlePointOfBarEnd);
                        // left side of the block
                        wxPoint pt;
                        GetPhyscialCoordinates(GetScalingAxis().GetRange().first +
                            axisOffset, bar.GetAxisPosition(), pt);
                        // if the first block, push it over 1 pixel so that it doesn't overlap the bar axis
                        lineXStart = pt.x + (axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                        }

                    const wxCoord barLength = middlePointOfBarEnd.x - lineXStart;
                    axisOffset += barBlock.GetLength();

                    const wxCoord lineYStart = middlePointOfBarEnd.y - safe_divide<double>(barWidth, 2.0);
                    const auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();
                    barRect = wxRect(lineXStart, lineYStart, barLength, barWidth);
                    wxRect barNeckRect = barRect;

                    // if just measuring then we're done
                    if (measureOnly)
                        { return middlePointOfBarEnd; }

                    // draw the bar (block)
                    if (barBlock.IsShown() && barLength > 0)
                        {
                        // if block has a customized opacity, then use that instead of the bar's opacity
                        const wxColour blockColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                            ColorContrast::ChangeOpacity(barBlock.GetBrush().GetColour(), bar.GetOpacity()) :
                            barBlock.GetBrush().GetColour();
                        const wxColour blockLightenedColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                            ColorContrast::ChangeOpacity(barBlock.GetLightenedColor(), bar.GetOpacity()) :
                            barBlock.GetLightenedColor();
                        wxBrush blockBrush{ barBlock.GetBrush() };
                        blockBrush.SetColour(blockColor);

                        if (bar.GetEffect() == BoxEffect::CommonImage && scaledCommonImg.IsOk())
                            {
                            wxRect barRectAdjustedToPlotArea = barRect;
                            barRectAdjustedToPlotArea.SetLeft(barRect.GetLeft() - GetPlotAreaBoundingBox().GetLeft());
                            barRectAdjustedToPlotArea.SetTop(barRect.GetTop() -
                                 (GetPlotAreaBoundingBox().GetTop() +
                                     safe_divide(GetPlotAreaBoundingBox().GetHeight() - scaledCommonImg.GetHeight(), 2)) );
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(GetImageOulineColor()).
                                AnchorPoint(wxPoint(lineXStart, lineYStart)),
                                scaledCommonImg.GetSubImage(barRectAdjustedToPlotArea));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            barImage->SetShadowType((GetShadowType() != ShadowType::NoShadow) ?
                                ShadowType::RightSideAndBottomShadow : ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        else if (bar.GetEffect() == BoxEffect::Stipple &&
                                 GetStippleBrush().IsOk() )
                            {
                            wxASSERT_LEVEL_2_MSG((bar.GetShape() == BarShape::Rectangle),
                                                 L"Non-rectangular shapes not currently "
                                                  "supported with stipple bar effect.");
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(wxNullPen).
                                AnchorPoint(wxPoint(lineXStart, lineYStart)),
                                Image::CreateStippledImage(
                                    GetStippleBrush().GetBitmap(
                                        GetStippleBrush().GetDefaultSize()).ConvertToImage(),
                                    wxSize(barLength, barWidth),
                                    Orientation::Horizontal, (GetShadowType() != ShadowType::NoShadow),
                                    ScaleToScreenAndCanvas(4)));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            // note that stipples have their own shadows (a silhouette), so turn off the
                            // Image's native shadow renderer.
                            barImage->SetShadowType(ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        else if (bar.GetEffect() == BoxEffect::Glassy)
                            {
                            wxASSERT_LEVEL_2_MSG((bar.GetShape() == BarShape::Rectangle),
                                                 L"Non-rectangular shapes not currently "
                                                  "supported with glassy bar effect.");
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(wxNullPen).
                                AnchorPoint(wxPoint(lineXStart, lineYStart)),
                                Image::CreateGlassEffect(wxSize(barLength, barWidth),
                                    blockColor, Orientation::Vertical));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            barImage->SetShadowType((GetShadowType() != ShadowType::NoShadow) ?
                                ShadowType::RightSideAndBottomShadow : ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        // color-filled bar
                        else
                            {
                            std::shared_ptr<GraphItems::Polygon> box;
                            GraphItems::Polygon::GetRectPoints(barRect, boxPoints);
                            if (bar.GetShape() == BarShape::Rectangle)
                                {
                                // Polygons don't support drop shadows, so need to manually add a shadow as another polygon
                                if ((GetShadowType() != ShadowType::NoShadow) && (barBlock.GetLength() > rangeStart))
                                    {
                                    // in case this bar is way too small because of the scaling then don't bother with the shadow
                                    if (barRect.GetHeight() > scaledShadowOffset)
                                        {
                                        wxPoint shadowPts[7] =
                                            {
                                            barRect.GetLeftBottom(),
                                            barRect.GetLeftBottom() + wxPoint(0,scaledShadowOffset),
                                            barRect.GetRightBottom() + wxPoint(scaledShadowOffset, scaledShadowOffset),
                                            barRect.GetRightTop() + wxPoint(scaledShadowOffset, scaledShadowOffset),
                                            barRect.GetRightTop() + wxPoint(0, scaledShadowOffset),
                                            barRect.GetRightBottom(),
                                            barRect.GetLeftBottom() // close polygon
                                            };
                                        AddObject(std::make_shared<GraphItems::Polygon>(
                                            GraphItemInfo().Pen(wxNullPen).Brush(GraphItemBase::GetShadowColour()),
                                            shadowPts, std::size(shadowPts)));
                                        }
                                    }
                                box = std::make_shared<GraphItems::Polygon>(
                                    GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                    Pen(wxPen(*wxBLACK)).Brush(blockBrush).Scaling(GetScaling()).
                                    ShowLabelWhenSelected(true),
                                    boxPoints, std::size(boxPoints));
                                }
                            else if (bar.GetShape() == BarShape::Arrow)
                                {
                                wxASSERT_LEVEL_2_MSG(!(GetShadowType() != ShadowType::NoShadow),
                                                     L"Drop shadow not supported for arrow shape currently.");
                                barNeckRect.Deflate(wxSize(0,safe_divide(barNeckRect.GetHeight(),5)) );
                                barNeckRect.SetRight(barNeckRect.GetRight()-(safe_divide(barNeckRect.GetWidth(),10)));
                                arrowPoints[0] = barNeckRect.GetTopLeft();
                                arrowPoints[1] = barNeckRect.GetTopRight();
                                arrowPoints[2] = wxPoint(barNeckRect.GetRight(), barRect.GetTop());
                                arrowPoints[3] = wxPoint(barRect.GetRight(), barRect.GetTop()+(safe_divide((barRect.GetHeight()),2)) );
                                arrowPoints[4] = wxPoint(barNeckRect.GetRight(), barRect.GetBottom());
                                arrowPoints[5] = barNeckRect.GetBottomRight();
                                arrowPoints[6] = barNeckRect.GetBottomLeft();
                                box = std::make_shared<GraphItems::Polygon>(
                                    GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                    Pen(wxPen(*wxBLACK)).Brush(blockBrush).Scaling(GetScaling()).ShowLabelWhenSelected(true),
                                    arrowPoints, std::size(arrowPoints));
                                }

                            wxASSERT_LEVEL_2(box);
                            
                            if (barBlock.GetOutlinePen().IsOk())
                                { box->GetPen() = barBlock.GetOutlinePen(); }
                            else
                                { box->GetPen().SetColour(
                                    ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                    *wxWHITE : *wxBLACK); }
                            if (bar.GetEffect() == BoxEffect::FadeFromBottomToTop)
                                {
                                box->GetBrush() = wxNullBrush;
                                box->SetBackgroundFill(Colors::GradientFill(
                                    blockColor,
                                    blockLightenedColor,
                                    FillDirection::East));
                                }
                            else if (bar.GetEffect() == BoxEffect::FadeFromTopToBottom)
                                {
                                box->GetBrush() = wxNullBrush;
                                box->SetBackgroundFill(Colors::GradientFill(
                                    blockColor,
                                    blockLightenedColor,
                                    FillDirection::West));
                                }
                            // in case an explicit color is used for the background
                            // and the brush is perhaps a hatch to be draw on top of it
                            else if (barBlock.GetColor().IsOk())
                                {
                                box->SetBackgroundFill(
                                    Colors::GradientFill(barBlock.GetColor()));
                                box->GetPen().SetColour(
                                    ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                    *wxWHITE : *wxBLACK);
                                }
                            // if the box is really thin, then don't use the outline pen
                            if (DownscaleFromScreenAndCanvas(barRect.GetWidth()) < 5)
                                { box->GetPen() = wxNullPen; }
                            box->SetShape(GraphItems::Polygon::PolygonShape::Rectangle);
                            // add the box to the plot item collection
                            AddObject(box);
                            }
                        }
                    // add the decal (if there is one)
                    if (barBlock.IsShown() && barBlock.GetDecal().GetText().length())
                        {
                        const wxCoord leftPadding = ScaleToScreenAndCanvas(2);
                        wxRect decalRect(barNeckRect); decalRect.Deflate(leftPadding, 0);

                        auto decalLabel = std::make_shared<GraphItems::Label>(barBlock.GetDecal());
                        decalLabel->GetGraphItemInfo().Pen(wxNullPen).
                            Scaling(GetScaling()).
                            DPIScaling(GetDPIScaleFactor()).
                            Padding(2, 2, 2, 2);
                        decalLabel->GetFont().MakeSmaller().MakeSmaller();
                        if (decalLabel->GetLabelFit() == LabelFit::ScaleFontToFit)
                            {
                            decalLabel->SetBoundingBox(decalRect, dc, GetScaling());
                            decalLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                            }
                        else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFit)
                            { decalLabel->SplitTextToFitBoundingBox(dc, decalRect.GetSize()); }
                        else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFitWidth)
                            {
                            decalLabel->SplitTextToFitBoundingBox(
                                dc, wxSize(decalRect.GetWidth(),
                                std::numeric_limits<int>::max()));
                            }
                        // if drawing as-is, then draw a box around the label if it's larger than the parent block
                        else if (decalLabel->GetLabelFit() == LabelFit::DisplayAsIsAutoFrame)
                            {
                            const auto actualDecalRect = decalLabel->GetBoundingBox(dc);
                            // allow a little wiggle room
                            if (actualDecalRect.GetWidth()-ScaleToScreenAndCanvas(1) > decalRect.GetWidth() ||
                                actualDecalRect.GetHeight()-ScaleToScreenAndCanvas(1) > decalRect.GetHeight())
                                {
                                decalLabel->GetGraphItemInfo().FontBackgroundColor(
                                    ColorContrast::BlackOrWhiteContrast(decalLabel->GetFontColor())).
                                    Pen(*wxBLACK_PEN);
                                }
                            }
                        // make multiline decals a little more compact so that
                        // they have a better chance of fitting
                        decalLabel->SetLineSpacing(0);
                        decalLabel->SetShadowType(ShadowType::NoShadow);
                        decalLabel->SetTextAlignment(TextAlignment::FlushLeft);
                        decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                        // allow selecting the bar underneath this label
                        decalLabel->SetSelectable(false);
                        // if font is way too small, then show it as a label overlapping the bar instead of a decal
                        if (decalLabel->GetLabelFit() != LabelFit::DisplayAsIs &&
                            decalLabel->GetLabelFit() != LabelFit::DisplayAsIsAutoFrame &&
                            decalLabel->GetFont().GetPointSize() < wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()/2)
                            {
                            decalLabel->GetFont().SetPointSize(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
                            decalLabel->GetPen().SetColour(*wxBLACK);
                            decalLabel->SetFontColor(*wxBLACK);
                            decalLabel->SetFontBackgroundColor(*wxWHITE);
                            }
                        const wxRect labelBox = decalLabel->GetBoundingBox(dc);
                        if (decalLabel->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                            {
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetLeft() + leftPadding),
                                (barNeckRect.GetTop() + safe_divide(barNeckRect.GetHeight() - labelBox.GetHeight(), 2))));
                            }
                        else if (decalLabel->GetRelativeAlignment() == RelativeAlignment::Centered)
                            {
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetLeft() +
                                safe_divide(barNeckRect.GetWidth()-labelBox.GetWidth(), 2)),
                                (barNeckRect.GetTop()+safe_divide(barNeckRect.GetHeight()-labelBox.GetHeight(), 2))));
                            }
                        else // flush right
                            {
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetRight() - (labelBox.GetWidth()+leftPadding)),
                                (barNeckRect.GetTop()+safe_divide(barNeckRect.GetHeight()-labelBox.GetHeight(), 2))));
                            }
                        // if drawing a color and hatch pattern, then show the decal with an outline
                        // to make it easier to read
                        if (bar.GetEffect() == BoxEffect::Solid &&
                            barBlock.GetColor().IsOk() &&
                            barBlock.GetBrush().GetStyle() != wxBrushStyle::wxBRUSHSTYLE_SOLID)
                            {
                            decalLabel->GetPen().SetColour(*wxBLACK);
                            decalLabel->SetFontColor(*wxBLACK);
                            decalLabel->SetFontBackgroundColor(*wxWHITE);
                            }
                        // This will be added to the plot's collection of object AFTER all blocks have been added.
                        // This ensures that decals that go outside of their block are eclipsed by the next block.
                        decals.push_back(decalLabel);
                        }
                    }
                else
                    {
                    /* if the bar (or block) is set to cover a specific range
                       (e.g., histograms do this when using cutpoints) then calculate
                       the width of the bar based on the coordinates.
                       Otherwise, just divvy up the bars evenly to fit the plot window.*/
                    if (barBlock.GetCustomWidth().has_value())
                        {
                        wxPoint leftPointOfBar, rightPointOfBar;
                        GetPhyscialCoordinates(bar.GetAxisPosition()-safe_divide<double>(barBlock.GetCustomWidth().value(),2),
                            barBlock.GetLength()/*offset doesn't matter here*/, leftPointOfBar);
                        GetPhyscialCoordinates(bar.GetAxisPosition()+safe_divide<double>(barBlock.GetCustomWidth().value(),2),
                            barBlock.GetLength(), rightPointOfBar);
                        barWidth = ((rightPointOfBar.x - leftPointOfBar.x) - barSpacing);
                        }
                    else if (bar.GetCustomWidth().has_value())
                        {
                        wxPoint leftPointOfBar, rightPointOfBar;
                        GetPhyscialCoordinates(bar.GetAxisPosition()-safe_divide<double>(bar.GetCustomWidth().value(),2),
                            barBlock.GetLength()/*offset doesn't matter here*/, leftPointOfBar);
                        GetPhyscialCoordinates(bar.GetAxisPosition()+safe_divide<double>(bar.GetCustomWidth().value(),2),
                            barBlock.GetLength(), rightPointOfBar);
                        barWidth = ((rightPointOfBar.x - leftPointOfBar.x) - barSpacing);
                        }
                    else
                        {
                        const size_t barSlots = GetBarSlotCount();
                        const size_t overallBarSpacing = (barSpacing*(barSlots-1));
                        barWidth = safe_divide<double>(
                            // the plot area, minus the cumulative spaces between their bars
                            // (unless the spacing is too aggressive)
                            GetPlotAreaBoundingBox().GetWidth() -
                            (overallBarSpacing < GetPlotAreaBoundingBox().GetWidth()+barSlots ? overallBarSpacing : 0),
                            // add an "extra" bar to account for the half bar space around the first and last bars
                            (barSlots+1));
                        }

                    // set the bottom (starting point) of the bar
                    wxCoord lineYStart{ 0 };
                    if (bar.GetCustomScalingAxisStartPosition().has_value())
                        {
                        // top of block
                        GetPhyscialCoordinates(bar.GetAxisPosition(),
                            bar.GetCustomScalingAxisStartPosition().value() + axisOffset + barBlock.GetLength(), middlePointOfBarEnd);
                        // bottom of block
                        wxPoint customStartPt;
                        GetPhyscialCoordinates(bar.GetAxisPosition(),
                            bar.GetCustomScalingAxisStartPosition().value() + axisOffset, customStartPt);
                        lineYStart = customStartPt.y;
                        }
                    else
                        {
                        // top of block
                        GetPhyscialCoordinates(bar.GetAxisPosition(),
                            GetScalingAxis().GetRange().first + axisOffset + barBlock.GetLength(), middlePointOfBarEnd);
                        // bottom of block
                        wxPoint pt;
                        GetPhyscialCoordinates(bar.GetAxisPosition(),
                            GetScalingAxis().GetRange().first + axisOffset, pt);
                        lineYStart = pt.y;
                        }

                    axisOffset += barBlock.GetLength();
                    const wxCoord barLength = lineYStart-middlePointOfBarEnd.y;
                    const wxCoord lineYEnd = lineYStart-barLength;
                    const wxCoord lineXStart = middlePointOfBarEnd.x - safe_divide<double>(barWidth, 2.0);
                    const auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();
                    barRect = wxRect(lineXStart, lineYEnd, barWidth, barLength);
                    wxRect barNeckRect = barRect;

                    // if just measuring then we're done
                    if (measureOnly)
                        { return middlePointOfBarEnd; }

                    // draw the bar
                    if (barBlock.IsShown() && barLength > 0)
                        {
                        // if block has a customized opacity, then use that instead of the bar's opacity
                        const wxColour blockColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                            ColorContrast::ChangeOpacity(barBlock.GetBrush().GetColour(), bar.GetOpacity()) :
                            barBlock.GetBrush().GetColour();
                        const wxColour blockLightenedColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                            ColorContrast::ChangeOpacity(barBlock.GetLightenedColor(), bar.GetOpacity()) :
                            barBlock.GetLightenedColor();
                        wxBrush blockBrush{ barBlock.GetBrush() };
                        blockBrush.SetColour(blockColor);

                        if (bar.GetEffect() == BoxEffect::CommonImage && scaledCommonImg.IsOk())
                            {
                            wxRect barRectAdjustedToPlotArea = barRect;
                            barRectAdjustedToPlotArea.SetLeft(barRect.GetLeft() -
                                (GetPlotAreaBoundingBox().GetLeft() +
                                    safe_divide(GetPlotAreaBoundingBox().GetWidth() - scaledCommonImg.GetWidth(), 2)) );
                            barRectAdjustedToPlotArea.SetTop(barRect.GetTop() -
                                (GetPlotAreaBoundingBox().GetTop() +
                                    GetPlotAreaBoundingBox().GetHeight() - scaledCommonImg.GetHeight()));
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(GetImageOulineColor()).
                                AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                                scaledCommonImg.GetSubImage(barRectAdjustedToPlotArea));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            barImage->SetShadowType((GetShadowType() != ShadowType::NoShadow) ?
                                ShadowType::RightSideShadow : ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        else if (bar.GetEffect() == BoxEffect::Stipple &&
                                 GetStippleBrush().IsOk() )
                            {
                            wxASSERT_LEVEL_2_MSG((bar.GetShape() == BarShape::Rectangle),
                                                 L"Non-rectangular shapes not currently "
                                                  "supported with stipple bar effect.");
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(wxNullPen).
                                AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                                Image::CreateStippledImage(
                                    GetStippleBrush().GetBitmap(
                                        GetStippleBrush().GetDefaultSize()).ConvertToImage(),
                                    wxSize(barWidth, barLength), Orientation::Vertical,
                                    (GetShadowType() != ShadowType::NoShadow),
                                    ScaleToScreenAndCanvas(4)));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            // note that stipples have their own shadows (a silhouette), so turn off the
                            // Image's native shadow renderer.
                            barImage->SetShadowType(ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        else if (bar.GetEffect() == BoxEffect::Glassy)
                            {
                            wxASSERT_LEVEL_2_MSG((bar.GetShape() == BarShape::Rectangle),
                                                 L"Non-rectangular shapes not currently "
                                                  "supported with glassy bar effect.");
                            auto barImage = std::make_shared<Image>(
                                GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                Pen(wxNullPen).
                                AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                                Image::CreateGlassEffect(wxSize(barWidth, barLength),
                                    blockColor, Orientation::Horizontal));
                            barImage->SetOpacity(bar.GetOpacity());
                            barImage->SetAnchoring(Anchoring::TopLeftCorner);
                            barImage->SetShadowType((GetShadowType() != ShadowType::NoShadow) ?
                                ShadowType::RightSideShadow : ShadowType::NoShadow);
                            AddObject(barImage);
                            }
                        else
                            {
                            std::shared_ptr<GraphItems::Polygon> box{ nullptr };
                            GraphItems::Polygon::GetRectPoints(barRect, boxPoints);
                            if (bar.GetShape() == BarShape::Rectangle)
                                {
                                // polygons don't support drop shadows, so need to manually add a shadow as another polygon
                                if ((GetShadowType() != ShadowType::NoShadow) && (barBlock.GetLength() > rangeStart))
                                    {
                                    // in case this bar is way too small because of the scaling,
                                    // then don't bother with the shadow
                                    if (barRect.GetHeight() > scaledShadowOffset)
                                        {
                                        wxPoint shadowPts[4];
                                        shadowPts[0] = barRect.GetRightBottom() + wxPoint(scaledShadowOffset,0);
                                        shadowPts[1] = barRect.GetRightTop() + wxPoint(scaledShadowOffset, scaledShadowOffset);
                                        shadowPts[2] = barRect.GetRightTop() + wxPoint(0, scaledShadowOffset);
                                        shadowPts[3] = barRect.GetRightBottom();
                                        AddObject(std::make_shared<GraphItems::Polygon>(
                                            GraphItemInfo().Pen(wxNullPen).Brush(GraphItemBase::GetShadowColour()),
                                            shadowPts, std::size(shadowPts)));
                                        }
                                    }

                                box = std::make_shared<GraphItems::Polygon>(
                                    GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                    Pen(wxPen(*wxBLACK)).Brush(blockBrush).Scaling(GetScaling()).ShowLabelWhenSelected(true),
                                    boxPoints, std::size(boxPoints));
                                }
                            else if (bar.GetShape() == BarShape::Arrow)
                                {
                                wxASSERT_LEVEL_2_MSG(!(GetShadowType() != ShadowType::NoShadow),
                                                     L"Drop shadow not supported for arrow shape currently.");
                                barNeckRect.Deflate(wxSize(safe_divide(barNeckRect.GetWidth(),5), 0) );
                                const auto arrowHeadSize = safe_divide(barNeckRect.GetHeight(),10);
                                barNeckRect.SetTop(barNeckRect.GetTop()+arrowHeadSize);
                                barNeckRect.SetHeight(barNeckRect.GetHeight()-arrowHeadSize);
                                arrowPoints[0] = barNeckRect.GetBottomLeft();
                                arrowPoints[1] = barNeckRect.GetTopLeft();
                                arrowPoints[2] = wxPoint(barRect.GetLeft(), barNeckRect.GetTop());
                                arrowPoints[3] = wxPoint(barRect.GetLeft() +
                                                         (safe_divide(barRect.GetWidth(),2)), barRect.GetTop());
                                arrowPoints[4] = wxPoint(barRect.GetRight(), barNeckRect.GetTop());
                                arrowPoints[5] = barNeckRect.GetTopRight();
                                arrowPoints[6] = barNeckRect.GetBottomRight();
                                box = std::make_shared<GraphItems::Polygon>(
                                    GraphItemInfo(barBlock.GetSelectionLabel().GetText()).
                                    Pen(wxPen(*wxBLACK)).Brush(blockBrush).
                                    Scaling(GetScaling()).ShowLabelWhenSelected(true),
                                    arrowPoints, std::size(arrowPoints));
                                }

                            wxASSERT_LEVEL_2(box);
                            if (barBlock.GetOutlinePen().IsOk())
                                { box->GetPen() = barBlock.GetOutlinePen(); }
                            else
                                {
                                box->GetPen().SetColour(
                                    ColorContrast::IsLight(GetPlotOrCanvasColor()) ? *wxWHITE : *wxBLACK);
                                }

                            if (bar.GetEffect() == BoxEffect::FadeFromBottomToTop)
                                {
                                box->GetBrush() = wxNullBrush;
                                box->SetBackgroundFill(Colors::GradientFill(
                                    blockColor,
                                    blockLightenedColor,
                                    FillDirection::North));
                                }
                            else if (bar.GetEffect() == BoxEffect::FadeFromTopToBottom)
                                {
                                box->GetBrush() = wxNullBrush;
                                box->SetBackgroundFill(Colors::GradientFill(
                                    blockColor,
                                    blockLightenedColor,
                                    FillDirection::South));
                                }
                            // in case an explicit color is used for the background
                            // and the brush is perhaps a hatch to be draw on top of it
                            else if (barBlock.GetColor().IsOk())
                                {
                                box->SetBackgroundFill(
                                    Colors::GradientFill(barBlock.GetColor()));
                                box->GetPen().SetColour(
                                    ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                                           *wxWHITE : *wxBLACK);
                                }
                            // if the box is really thin, then don't use the outline pen
                            if (DownscaleFromScreenAndCanvas(barRect.GetWidth()) < 5)
                                { box->GetPen() = wxNullPen; }
                            box->SetShape(GraphItems::Polygon::PolygonShape::Rectangle);
                            // add the box to the plot item collection
                            AddObject(box);
                            }
                        }
                    // add the decal (if there is one)
                    if (barBlock.IsShown() && barBlock.GetDecal().GetText().length())
                        {
                        const wxCoord leftPadding = ScaleToScreenAndCanvas(2);
                        // rectangle is inverted
                        wxRect decalRect(wxPoint(0,0), wxSize(barNeckRect.GetHeight(), barNeckRect.GetWidth()));
                        decalRect.SetHeight(decalRect.GetHeight()-leftPadding);

                        auto decalLabel = std::make_shared<GraphItems::Label>(barBlock.GetDecal());
                        decalLabel->GetGraphItemInfo().Pen(wxNullPen).
                            Scaling(GetScaling()).
                            DPIScaling(GetDPIScaleFactor()).
                            Padding(2, 2, 2, 2);
                        decalLabel->GetFont().MakeSmaller().MakeSmaller();
                        if (decalLabel->GetLabelFit() == LabelFit::ScaleFontToFit)
                            {
                            decalLabel->SetBoundingBox(decalRect, dc, GetScaling());
                            decalLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                            }
                        else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFit)
                            { decalLabel->SplitTextToFitBoundingBox(dc, decalRect.GetSize()); }
                        else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFitWidth)
                            {
                            decalLabel->SplitTextToFitBoundingBox(
                                dc, wxSize(decalRect.GetWidth(),
                                std::numeric_limits<int>::max()));
                            }
                        // if drawing as-is, then draw a box around the label if it's larger than the parent block
                        else if (decalLabel->GetLabelFit() == LabelFit::DisplayAsIsAutoFrame)
                            {
                            const auto actualDecalRect = decalLabel->GetBoundingBox(dc);
                            if (actualDecalRect.GetWidth() - ScaleToScreenAndCanvas(1) > decalRect.GetWidth() ||
                                actualDecalRect.GetHeight() - ScaleToScreenAndCanvas(1) > decalRect.GetHeight())
                                {
                                decalLabel->GetGraphItemInfo().FontBackgroundColor(
                                    ColorContrast::BlackOrWhiteContrast(decalLabel->GetFontColor()));
                                }
                            }
                        // make multiline decals a little more compact so that they have a better chance of fitting
                        decalLabel->SetLineSpacing(0);
                        decalLabel->SetShadowType(ShadowType::NoShadow);
                        decalLabel->SetTextAlignment(TextAlignment::FlushLeft);
                        decalLabel->SetTextOrientation(Orientation::Horizontal);
                        decalLabel->SetAnchoring(Wisteria::Anchoring::BottomLeftCorner);
                        // allow selecting the bar underneath this label
                        decalLabel->SetSelectable(false);
                        // if font is way too small, then show it as a label overlapping the bar instead of a decal.
                        if (decalLabel->GetLabelFit() != LabelFit::DisplayAsIs &&
                            decalLabel->GetLabelFit() != LabelFit::DisplayAsIsAutoFrame &&
                            decalLabel->GetFont().GetPointSize() < wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()/2)
                            {
                            decalLabel->GetFont().SetPointSize(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
                            decalLabel->SetFontColor(*wxBLACK);
                            decalLabel->GetPen().SetColour(*wxBLACK);
                            decalLabel->SetFontBackgroundColor(*wxWHITE);
                            }
                        const wxRect labelBouningBox = decalLabel->GetBoundingBox(dc);
                        if (decalLabel->GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                            {
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetLeft() +
                                safe_divide(barNeckRect.GetWidth() - labelBouningBox.GetWidth(), 2)),
                                (barNeckRect.GetBottom() - leftPadding)));
                            }
                        else if (decalLabel->GetRelativeAlignment() == RelativeAlignment::Centered)
                            {
                            decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetLeft() +
                                safe_divide(barNeckRect.GetWidth() - labelBouningBox.GetWidth(), 2)),
                                (barNeckRect.GetTop() + safe_divide(barNeckRect.GetHeight() - labelBouningBox.GetHeight(), 2))));
                            }
                        else // flush top
                            {
                            decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                            decalLabel->SetAnchorPoint(wxPoint((barNeckRect.GetLeft() +
                                safe_divide(barNeckRect.GetWidth() - labelBouningBox.GetWidth(), 2)),
                                (barNeckRect.GetTop() + leftPadding)));
                            }
                        // if drawing a color and hatch pattern, then show the decal with an outline
                        // to make it easier to read
                        if (bar.GetEffect() == BoxEffect::Solid &&
                            barBlock.GetColor().IsOk() &&
                            barBlock.GetBrush().GetStyle() != wxBrushStyle::wxBRUSHSTYLE_SOLID)
                            {
                            decalLabel->GetPen().SetColour(*wxBLACK);
                            decalLabel->SetFontColor(*wxBLACK);
                            decalLabel->SetFontBackgroundColor(*wxWHITE);
                            }
                        decals.push_back(decalLabel);
                        }
                    }
                }

            // after all blocks are built, add the label at the end of the full bar
            if (GetBarOrientation() == Orientation::Horizontal &&
                bar.GetLabel().IsShown())
                {
                bar.GetLabel().SetScaling(GetScaling());
                bar.GetLabel().SetDPIScaleFactor(GetDPIScaleFactor());
                bar.GetLabel().SetShadowType(GetShadowType());

                auto bBox = bar.GetLabel().GetBoundingBox(dc);
                bar.GetLabel().SetAnchorPoint(
                    wxPoint(middlePointOfBarEnd.x + labelSpacingFromLine + (bBox.GetWidth()/2),
                            middlePointOfBarEnd.y));

                auto barLabel = std::make_shared<GraphItems::Label>(bar.GetLabel());
                bBox = barLabel->GetBoundingBox(dc);

                if (!Polygon::IsRectInsideRect(bBox, GetPlotAreaBoundingBox()))
                    {
                    barLabel->Offset((GetPlotAreaBoundingBox().GetRight() - bBox.GetRight()), 0);
                    bBox.Offset((GetPlotAreaBoundingBox().GetRight() - bBox.GetRight()) +
                        ScaleToScreenAndCanvas(2), 0);
                    if (barRect.Intersects(bBox))
                        {
                        barLabel->SetPadding(2, 2, 2, 2);
                        barLabel->GetPen() = *wxBLACK_PEN;
                        barLabel->SetFontBackgroundColor(
                            ColorContrast::BlackOrWhiteContrast(barLabel->GetFontColor()));
                        }
                    }

                AddObject(barLabel);
                middlePointOfBarEnd.x += bBox.GetWidth() + (labelSpacingFromLine * 2);
                }
            else if (GetBarOrientation() == Orientation::Vertical &&
                bar.GetLabel().IsShown())
                {
                bar.GetLabel().SetScaling(GetScaling());
                bar.GetLabel().SetDPIScaleFactor(GetDPIScaleFactor());
                bar.GetLabel().SetShadowType(GetShadowType());

                auto bBox = bar.GetLabel().GetBoundingBox(dc);
                bar.GetLabel().SetAnchorPoint(
                    wxPoint(middlePointOfBarEnd.x,
                            middlePointOfBarEnd.y -
                            (labelSpacingFromLine + (bBox.GetHeight()/2))));

                auto barLabel = std::make_shared<GraphItems::Label>(bar.GetLabel());
                bBox = barLabel->GetBoundingBox(dc);

                if (!Polygon::IsRectInsideRect(bBox, GetPlotAreaBoundingBox()))
                    {
                    barLabel->Offset(0, (GetPlotAreaBoundingBox().GetTop() - bBox.GetTop()));
                    bBox.Offset(0, (GetPlotAreaBoundingBox().GetTop() - bBox.GetTop()) -
                        // wiggle room before adding outlining that will stand out from the other labels
                        ScaleToScreenAndCanvas(2));
                    if (barRect.Intersects(bBox))
                        {
                        barLabel->SetPadding(2, 2, 2, 2);
                        barLabel->GetPen() = *wxBLACK_PEN;
                        barLabel->SetFontBackgroundColor(
                            ColorContrast::BlackOrWhiteContrast(barLabel->GetFontColor()));
                        }
                    }

                AddObject(barLabel);
                middlePointOfBarEnd.y -= bBox.GetHeight() + (labelSpacingFromLine * 2);
                }

            return middlePointOfBarEnd;
            };

        std::vector<wxPoint> boxCorners;
        for (auto& bar : GetBars())
            {
            drawBar(bar, true);
            boxCorners.push_back(barRect.GetTopLeft());
            boxCorners.push_back(barRect.GetTopRight());
            boxCorners.push_back(barRect.GetBottomLeft());
            boxCorners.push_back(barRect.GetBottomRight());
            }
        const auto [minX, maxX] = std::minmax_element(boxCorners.cbegin(), boxCorners.cend(),
            [](const auto lhv, const auto rhv) noexcept
            { return lhv.x < rhv.x; });
        const auto [minY, maxY] = std::minmax_element(boxCorners.cbegin(), boxCorners.cend(),
            [](const auto lhv, const auto rhv) noexcept
            { return lhv.y < rhv.y; });

        // scale the common image to the plot area's size
        scaledCommonImg = GetCommonBoxImage().IsOk() ?
            Image::CropImageToRect(
                GetCommonBoxImage().GetBitmap(GetCommonBoxImage().GetDefaultSize()).ConvertToImage(),
                wxSize((maxX->x - minX->x) + ScaleToScreenAndCanvas(5), // add padding for rounding issues
                       (maxY->y - minY->y) + ScaleToScreenAndCanvas(5)), false) :
            wxNullImage;

        // draw the bars
        std::vector<wxPoint> barMiddleEndPositions;
        barMiddleEndPositions.reserve(GetBars().size());
        for (auto& bar : GetBars())
            {
            // keep track of where each bar ends
            barMiddleEndPositions.push_back(drawBar(bar, false));
            }

        // draw the decals on top of the blocks
        for (auto& decal : decals)
            { AddObject(decal); }
        decals.clear();

        for (const auto& barGroup : m_barGroups)
            {
            const wxPoint brackPos1 = barMiddleEndPositions[barGroup.m_barPositions.first];
            const wxPoint brackPos2 = barMiddleEndPositions[barGroup.m_barPositions.second];
            double grandTotal{ 0 };
            // the bars specified in the group may be in different order, so use
            // min and max to make sure you are using the true start and end bars
            for (size_t i = std::min(barGroup.m_barPositions.first, barGroup.m_barPositions.second);
                 i <= std::max(barGroup.m_barPositions.first, barGroup.m_barPositions.second);
                 ++i)
                { grandTotal += GetBars()[i].GetLength(); }

            constexpr double bracesWidth{ 30 };
            double scalingAxisPos{ 0 }, barAxisPos{ 0 };
            if (GetBarOrientation() == Orientation::Horizontal)
                {
                if (GetScalingAxis().GetValueFromPhysicalCoordinate(
                    std::max(brackPos1.x, brackPos2.x) + ScaleToScreenAndCanvas(bracesWidth),
                             scalingAxisPos))
                    {
                    // make the curly braces stretch from the top of the first bar
                    // to the bottom of the last one
                    const auto yOffset = (brackPos1.y < brackPos2.y) ?
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().
                            value_or(barWidth), 2) :
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().
                            value_or(barWidth), 2);
                    const auto barsWidth = std::abs(brackPos1.y - brackPos2.y) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().
                            value_or(barWidth), 2) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().
                            value_or(barWidth), 2);
                    const auto braces = std::make_shared<Shape>(
                        GraphItemInfo().Pen(wxPen(*wxBLACK, 2)).
                        Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                        AnchorPoint(wxPoint(
                            std::max(brackPos1.x, brackPos2.x),
                            std::min(brackPos1.y, brackPos2.y) - yOffset)).
                        Anchoring(Anchoring::TopLeftCorner),
                        Icons::IconShape::RightCurlyBrace,
                        wxSize(bracesWidth, DownscaleFromScreenAndCanvas(barsWidth)),
                        nullptr);
                    
                    const auto yPos = std::min(brackPos1.y, brackPos2.y) +
                        safe_divide<double>(std::abs(brackPos1.y - brackPos2.y), 2);
                    if (GetBarAxis().GetValueFromPhysicalCoordinate(yPos, barAxisPos))
                        {
                        Bar theBar(grandTotal,
                            {
                            BarBlock(BarBlockInfo(grandTotal).
                            Brush(barGroup.m_barBrush).Color(barGroup.m_barColor).
                            Decal(Label(GraphItemInfo(barGroup.m_barDecal).
                                LabelFitting(LabelFit::SplitTextToFit).
                                ChildAlignment(RelativeAlignment::Centered).
                                FontColor(ColorContrast::BlackOrWhiteContrast(
                                    barGroup.m_barBrush.IsOk() ?
                                    barGroup.m_barBrush.GetColour() : barGroup.m_barColor))
                                )))
                            },
                            wxEmptyString, Label(), GetBarEffect(), GetBarOpacity());
                        UpdateBarLabel(theBar);
                        theBar.SetCustomScalingAxisStartPosition(scalingAxisPos);
                        theBar.SetAxisPosition(barAxisPos);

                        AddObject(braces);
                        drawBar(theBar, false);
                        // cppcheck-suppress knownEmptyContainer
                        for (auto& decal : decals)
                            { AddObject(decal); }
                        }
                    }
                }
            else
                {
                if (GetScalingAxis().GetValueFromPhysicalCoordinate(
                    std::min(brackPos1.y, brackPos2.y) -
                        // space for the braces and a couple DIPs between that and the group bar
                        ScaleToScreenAndCanvas(bracesWidth + 2),
                    scalingAxisPos))
                    {
                    // make the curly braces stretch from the top of the first bar
                    // to the bottom of the last one
                    const auto xOffset = (brackPos1.x < brackPos2.x) ?
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().
                            value_or(barWidth), 2) :
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().
                            value_or(barWidth), 2);
                    const auto barsWidth = std::abs(brackPos1.x - brackPos2.x) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().
                            value_or(barWidth), 2) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().
                            value_or(barWidth), 2);

                    const auto braces = std::make_shared<Shape>(
                        GraphItemInfo().Pen(wxPen(*wxBLACK, 2)).
                        Scaling(GetScaling()).DPIScaling(GetDPIScaleFactor()).
                        AnchorPoint(wxPoint(
                            std::min(brackPos1.x, brackPos2.x) - xOffset,
                            std::min(brackPos1.y, brackPos2.y) - ScaleToScreenAndCanvas(bracesWidth))).
                        Anchoring(Anchoring::TopLeftCorner),
                        Icons::IconShape::TopCurlyBrace,
                        wxSize(DownscaleFromScreenAndCanvas(barsWidth), bracesWidth),
                        nullptr);

                    const auto xPos = std::min(brackPos1.x, brackPos2.x) +
                        safe_divide<double>(std::abs(brackPos1.x - brackPos2.x), 2);
                    if (GetBarAxis().GetValueFromPhysicalCoordinate(xPos, barAxisPos))
                        {
                        Bar theBar(grandTotal,
                            {
                            BarBlock(BarBlockInfo(grandTotal).
                            Brush(barGroup.m_barBrush).Color(barGroup.m_barColor).
                            Decal(Label(GraphItemInfo(barGroup.m_barDecal).
                                LabelFitting(LabelFit::SplitTextToFit).
                                ChildAlignment(RelativeAlignment::Centered).
                                FontColor(ColorContrast::BlackOrWhiteContrast(
                                    barGroup.m_barBrush.IsOk() ?
                                    barGroup.m_barBrush.GetColour() : barGroup.m_barColor))
                                )))
                            },
                            wxEmptyString, Label(), GetBarEffect(), GetBarOpacity());
                        UpdateBarLabel(theBar);
                        theBar.SetCustomScalingAxisStartPosition(scalingAxisPos);
                        theBar.SetAxisPosition(barAxisPos);

                        AddObject(braces);
                        drawBar(theBar, false);
                        // cppcheck-suppress knownEmptyContainer
                        for (auto& decal : decals)
                            { AddObject(decal); }
                        }
                    }
                }
            }
        }
    }
