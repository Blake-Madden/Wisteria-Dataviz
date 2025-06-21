///////////////////////////////////////////////////////////////////////////////
// Name:        barchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "barchart.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::BarChart, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //-----------------------------------
    BarChart::BarChart(Wisteria::Canvas * canvas) : GroupGraph2D(canvas)
        {
        SetBarOrientation(m_barOrientation);
        }

    //-----------------------------------
    void BarChart::UpdateBarLabel(Bar & bar)
        {
        const double grandTotal =
            std::accumulate(GetBars().cbegin(), GetBars().cend(), 0.0,
                            [](auto lhv, auto rhv) noexcept { return lhv + rhv.GetLength(); });

        const double percentage = safe_divide<double>(bar.GetLength(), grandTotal) * 100;
        const wxString labelStr =
            (bar.GetLength() == 0 || GetBinLabelDisplay() == BinLabelDisplay::NoDisplay) ?
                wxString{} :
            (GetBinLabelDisplay() == BinLabelDisplay::BinName) ?
                bar.GetAxisLabel().GetText() :
            (GetBinLabelDisplay() == BinLabelDisplay::BinNameAndValue) ?
                bar.GetAxisLabel().GetText() +
                    wxString::Format(
                        L" (%s)", wxNumberFormatter::ToString(bar.GetLength(), 0,
                                                              Settings::GetDefaultNumberFormat())) :
            (GetBinLabelDisplay() == BinLabelDisplay::BinNameAndPercentage) ?
                wxString::Format(
                    /* TRANSLATORS: Bar axis label (%s), percent value (%s)
                       and percentage symbol (%%). '%%' can be changed and/or
                       moved elsewhere in the string. */
                    _(L"%s (%s%%)"), bar.GetAxisLabel().GetText(),
                    wxNumberFormatter::ToString(percentage, (percentage < 1) ? 2 : 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
            (GetBinLabelDisplay() == BinLabelDisplay::BinValue) ?
                wxNumberFormatter::ToString(bar.GetLength(), 0,
                                            Settings::GetDefaultNumberFormat()) :
            (GetBinLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                // if less than 1%, then use higher precision so that it doesn't just show as "0%"
                wxString::Format(
                    /* TRANSLATORS: Percentage value (%s) and % symbol (%%).
                       '%%' can be changed and/or moved elsewhere in the string. */
                    _(L"%s%%"),
                    wxNumberFormatter::ToString(percentage, (percentage < 1) ? 2 : 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes)) :
                // BinValueAndPercentage
                wxString::Format(
                    /* TRANSLATORS: Bar length, percentage of overall length,
                       and percent symbol (%%).
                       '%%' can be changed or moved. */
                    _(L"%s (%s%%)"),
                    wxNumberFormatter::ToString(bar.GetLength(), 0,
                                                Settings::GetDefaultNumberFormat()),
                    wxNumberFormatter::ToString(percentage, (percentage < 1) ? 2 : 0,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
        bar.GetLabel().SetText(labelStr + m_binLabelSuffix);
        }

    //-----------------------------------
    void BarChart::AddBarGroup(const wxString& firstBarLabel, const wxString& lastBarLabel,
                               std::optional<wxString> decal, std::optional<wxColour> color,
                               std::optional<wxBrush> brush)
        {
        const auto firstBar = FindBar(firstBarLabel);
        const auto lastBar = FindBar(lastBarLabel);
        if (firstBar && lastBar)
            {
            m_barGroups.push_back(
                { std::make_pair(firstBar.value(), lastBar.value()),
                  decal.has_value() ? decal.value() : wxString{},
                  brush.has_value() ? brush.value() : GetBrushScheme()->GetBrush(0),
                  color.has_value() ?
                      color.value() :
                      (GetColorScheme() ? GetColorScheme()->GetColor(0) : wxTransparentColour) });
            }
        else
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': bar label not found when adding bar group."),
                                 firstBar.has_value() ? lastBarLabel : firstBarLabel)
                    .ToUTF8());
            }
        AdjustScalingAxisFromBarGroups();
        }

    //-----------------------------------
    void BarChart::AddBarGroup(const double firstBarAxisPosition, const double lastBarAxisPosition,
                               std::optional<wxString> decal, std::optional<wxColour> color,
                               std::optional<wxBrush> brush)
        {
        const auto firstBar = FindBar(firstBarAxisPosition);
        const auto lastBar = FindBar(lastBarAxisPosition);
        if (firstBar && lastBar)
            {
            m_barGroups.push_back(
                { std::make_pair(firstBar.value(), lastBar.value()),
                  decal.has_value() ? decal.value() : wxString{},
                  brush.has_value() ? brush.value() : GetBrushScheme()->GetBrush(0),
                  color.has_value() ?
                      color.value() :
                      (GetColorScheme() ? GetColorScheme()->GetColor(0) : wxTransparentColour) });
            }
        else
            {
            throw std::runtime_error(
                wxString(_(L"Bar label not found when adding bar group.")).ToUTF8());
            }
        AdjustScalingAxisFromBarGroups();
        }

    //-----------------------------------
    void BarChart::AddBarIcon(const wxString& bar, const wxBitmapBundle& img)
        {
        const auto barPos = FindBar(bar);
        if (barPos.has_value())
            {
            auto& theBar{ GetBars().at(barPos.value()) };
            theBar.GetAxisLabel().SetLeftImage(img);
            GetBarAxis().SetCustomLabel(theBar.GetAxisPosition(), theBar.GetAxisLabel());
            }
        }

    //-----------------------------------
    std::optional<size_t> BarChart::FindBar(const wxString& axisLabel)
        {
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (GetBars()[i].GetAxisLabel().GetText().CmpNoCase(axisLabel) == 0)
                {
                return i;
                }
            }
        return std::nullopt;
        }

    //-----------------------------------
    std::optional<size_t> BarChart::FindBar(const double axisPosition)
        {
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            if (compare_doubles(GetBars()[i].GetAxisPosition(), axisPosition))
                {
                return i;
                }
            }
        return std::nullopt;
        }

    //-----------------------------------
    std::optional<double> BarChart::FindBarBlockStart(const size_t barIndex,
                                                      const wxString& blockTag) const
        {
        if (GetScalingAxis().IsReversed())
            {
            return std::nullopt;
            }
        assert(barIndex < GetBars().size() && L"Bar index out of range!");
        if (barIndex >= GetBars().size())
            {
            return std::nullopt;
            }

        const auto& bar = GetBars()[barIndex];

        auto block = bar.FindBlock(blockTag);
        if (block == bar.GetBlocks().cend())
            {
            return std::nullopt;
            }
        return std::accumulate(bar.GetBlocks().cbegin(), block, GetScalingAxis().GetRange().first,
                               [](const auto& initVal, const auto& val) noexcept
                               { return initVal + val.GetLength(); });
        }

    //-----------------------------------
    std::optional<double> BarChart::FindBarBlockEnd(const size_t barIndex, const wxString& blockTag)
        const
        {
        if (GetScalingAxis().IsReversed())
            {
            return std::nullopt;
            }
        assert(barIndex < GetBars().size() && L"Bar index out of range!");
        if (barIndex >= GetBars().size())
            {
            return std::nullopt;
            }

        const auto& bar = GetBars()[barIndex];

        auto block = bar.FindBlock(blockTag);
        if (block == bar.GetBlocks().cend())
            {
            return std::nullopt;
            }
        return std::accumulate(bar.GetBlocks().cbegin(), ++block, GetScalingAxis().GetRange().first,
                               [](const auto& initVal, const auto& val) noexcept
                               { return initVal + val.GetLength(); });
        }

    //-----------------------------------
    void BarChart::AddFirstBarBracket(const wxString& firstBarBlock, const wxString& lastBarBlock,
                                      const wxString& bracketLabel)
        {
        assert(GetBars().size() && L"No bars available when adding an axis bracket!");
        if (GetBars().empty())
            {
            throw std::runtime_error(_(L"No bars available when adding an axis bracket.").ToUTF8());
            }

        const auto blocksStart = FindBarBlockStart(0, firstBarBlock);
        const auto blocksEnd = FindBarBlockEnd(0, lastBarBlock);

        if (blocksStart.has_value() && blocksEnd.has_value())
            {
            GetScalingAxis().AddBracket(Wisteria::GraphItems::Axis::AxisBracket(
                blocksStart.value(), blocksEnd.value(),
                safe_divide(blocksStart.value() + blocksEnd.value(), 2.0), bracketLabel));
            }
        else if (!blocksStart.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Bar block '%s' not found when adding an axis bracket."),
                                 firstBarBlock)
                    .ToUTF8());
            }
        else
            {
            throw std::runtime_error(
                wxString::Format(_(L"Bar block '%s' not found when adding an axis bracket."),
                                 lastBarBlock)
                    .ToUTF8());
            }
        }

    //-----------------------------------
    void BarChart::AddFirstBarBracketRE(const wxString& firstBarBlockPattern,
                                        const wxString& lastBarBlockPattern,
                                        const wxString& bracketLabel)
        {
        assert(GetBars().size() && L"No bars available when adding an axis bracket!");
        if (GetBars().empty())
            {
            throw std::runtime_error(_(L"No bars available when adding an axis bracket.").ToUTF8());
            }

        const auto firstBlock = GetBars()[0].FindFirstBlockRE(firstBarBlockPattern);
        const auto lastBlock = GetBars()[0].FindLastBlockRE(lastBarBlockPattern);

        if (firstBlock == GetBars()[0].GetBlocks().cend())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                    firstBarBlockPattern)
                    .ToUTF8());
            }
        if (lastBlock == GetBars()[0].GetBlocks().crend())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                    lastBarBlockPattern)
                    .ToUTF8());
            }

        AddFirstBarBracket(firstBlock->GetTag(), lastBlock->GetTag(), bracketLabel);
        }

    //-----------------------------------
    void BarChart::AddLastBarBracket(const wxString& firstBarBlock, const wxString& lastBarBlock,
                                     const wxString& bracketLabel)
        {
        assert(GetBars().size() && L"No bars when adding an axis bracket!");
        if (GetBars().empty())
            {
            throw std::runtime_error(_(L"No bars when adding an axis bracket.").ToUTF8());
            }
        const size_t barIndex{ GetBars().size() - 1 };

        const auto blocksStart = FindBarBlockStart(barIndex, firstBarBlock);
        const auto blocksEnd = FindBarBlockEnd(barIndex, lastBarBlock);

        if (blocksStart.has_value() && blocksEnd.has_value())
            {
            if (GetBarOrientation() == Orientation::Vertical)
                {
                MirrorYAxis(true);
                }
            else
                {
                MirrorXAxis(true);
                }
            GetOppositeScalingAxis().AddBracket(Wisteria::GraphItems::Axis::AxisBracket(
                blocksStart.value(), blocksEnd.value(),
                safe_divide(blocksStart.value() + blocksEnd.value(), 2.0), bracketLabel));
            }
        else if (!blocksStart.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"Bar block '%s' not found when adding an axis bracket."),
                                 firstBarBlock)
                    .ToUTF8());
            }
        else
            {
            throw std::runtime_error(
                wxString::Format(_(L"Bar block '%s' not found when adding an axis bracket."),
                                 lastBarBlock)
                    .ToUTF8());
            }
        }

    //-----------------------------------
    void BarChart::AddLastBarBracketRE(const wxString& firstBarBlockPattern,
                                       const wxString& lastBarBlockPattern,
                                       const wxString& bracketLabel)
        {
        assert(GetBars().size() && L"No bars when adding an axis bracket!");
        if (GetBars().empty())
            {
            throw std::runtime_error(_(L"No bars when adding an axis bracket.").ToUTF8());
            }
        const size_t barIndex{ GetBars().size() - 1 };

        const auto firstBlock = GetBars()[barIndex].FindFirstBlockRE(firstBarBlockPattern);
        const auto lastBlock = GetBars()[barIndex].FindLastBlockRE(lastBarBlockPattern);

        if (firstBlock == GetBars()[barIndex].GetBlocks().cend())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                    firstBarBlockPattern)
                    .ToUTF8());
            }
        if (lastBlock == GetBars()[barIndex].GetBlocks().crend())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"Bar block '%s' pattern not found when adding an axis bracket."),
                    lastBarBlockPattern)
                    .ToUTF8());
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
            const wxPen gridlinePen =
                (GetBarAxis().GetGridlinePen().IsOk() ? GetBarAxis().GetGridlinePen() :
                                                        GetScalingAxis().GetGridlinePen());
            GetBarAxis().GetGridlinePen() = wxNullPen;
            GetScalingAxis().GetGridlinePen() = gridlinePen;
            }
        }

    //-----------------------------------
    void BarChart::AdjustScalingAxisFromBarGroups()
        {
        for (const auto& bar : GetBars())
            {
            UpdateScalingAxisFromBar(bar);
            }
        const auto originalLongestBar{ m_longestBarLength };
        double longestGroupBarWithSubBars{ 0 };
        for (const auto& barGroup : m_barGroups)
            {
            double groupBarLength{ 0 };
            double longestSubBarLength{ 0 };
            for (size_t i = barGroup.m_barPositions.first; i <= barGroup.m_barPositions.second; ++i)
                {
                const auto& bar = GetBars().at(i);
                // where the bar actually ends on the scaling axis
                const auto barEnd =
                    bar.GetLength() + bar.GetCustomScalingAxisStartPosition().value_or(0);
                longestSubBarLength = std::max(longestSubBarLength, barEnd);
                groupBarLength += barEnd;
                }
            m_longestBarLength = std::max(m_longestBarLength, longestSubBarLength + groupBarLength);
            longestGroupBarWithSubBars =
                std::max(longestGroupBarWithSubBars, longestSubBarLength + groupBarLength);
            }
        // If a group of bars with a supergroup bar (with connection braces) are larger
        // than the tallest regular bar, then we need to adjust the intervals.
        if (originalLongestBar < (longestGroupBarWithSubBars + GetScalingAxis().GetInterval()))
            {
            // Add an extra interval for the connection braces
            // (it will consume one) and extra space if bin labels are included.
            // AdjustScalingAxisFromBarLength() will add an extra interval for the bin label on the
            // longest regular bar, but we will be including another bin label on the group bar
            // also.
            const auto extraIntervalsNeeded =
                ((GetBinLabelDisplay() != BinLabelDisplay::NoDisplay) ? 2 : 1);
            if (GetScalingAxis().GetRange().second <
                (m_longestBarLength + (GetScalingAxis().GetInterval() * extraIntervalsNeeded)))
                {
                AdjustScalingAxisFromBarLength(
                    m_longestBarLength + (GetScalingAxis().GetInterval() * extraIntervalsNeeded));
                }
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
            {
            m_highestBarAxisPosition = bar.GetAxisPosition() + customWidth;
            }

        if (m_lowestBarAxisPosition > bar.GetAxisPosition() - customWidth)
            {
            m_lowestBarAxisPosition = bar.GetAxisPosition() - customWidth;
            }

        GetBarAxis().SetRange(m_lowestBarAxisPosition - GetBarAxis().GetInterval(),
                              m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                              GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                              GetBarAxis().GetDisplayInterval());
        if (bar.GetAxisLabel().IsShown() && bar.GetAxisLabel().GetText().length())
            {
            GetBarAxis().SetCustomLabel(bar.GetAxisPosition(), bar.GetAxisLabel());
            }

        if (adjustScalingAxis)
            {
            UpdateScalingAxisFromBar(bar);
            }
        }

    //-----------------------------------
    void BarChart::AdjustScalingAxisFromBarLength(const double barLength)
        {
        const auto originalRange = GetScalingAxis().GetRange();

        // tweak scaling
        if (barLength >= 3'000'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 7), GetScalingAxis().GetPrecision(),
                                      1'000'000, 1);
            }
        else if (barLength >= 1'500'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 7), GetScalingAxis().GetPrecision(),
                                      500'000, 1);
            }
        else if (barLength >= 800'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 6), GetScalingAxis().GetPrecision(),
                                      100'000, 1);
            }
        else if (barLength >= 400'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 6), GetScalingAxis().GetPrecision(),
                                      50'000, 1);
            }
        else if (barLength >= 50'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 5), GetScalingAxis().GetPrecision(),
                                      10'000, 1);
            }
        else if (barLength >= 20'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 4), GetScalingAxis().GetPrecision(),
                                      5'000, 1);
            }
        else if (barLength >= 10'000)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 4), GetScalingAxis().GetPrecision(),
                                      1'000, 1);
            }
        else if (barLength >= 1'500)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 4), GetScalingAxis().GetPrecision(),
                                      500, 1);
            }
        else if (barLength > 300)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 3), GetScalingAxis().GetPrecision(),
                                      100, 1);
            }
        else if (barLength > 100)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 3), GetScalingAxis().GetPrecision(),
                                      50, 1);
            }
        else if (barLength > 50)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 2), GetScalingAxis().GetPrecision(),
                                      10, 1);
            }
        else if (barLength > 10)
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 2), GetScalingAxis().GetPrecision(),
                                      5, 1);
            }
        else
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first,
                                      next_interval(barLength, 1), GetScalingAxis().GetPrecision(),
                                      1, 1);
            }

        // if showing labels and we just re-adjusted the range, then add an
        // extra interval for the label
        if (const auto currentRange = GetScalingAxis().GetRange();
            (GetBinLabelDisplay() != BinLabelDisplay::NoDisplay) && originalRange != currentRange)
            {
            const auto extraSpaceAfterBar{ m_longestBarLength -
                                           (currentRange.second - GetScalingAxis().GetInterval()) };
            const auto barPercentOfLastInterval{ safe_divide<double>(
                extraSpaceAfterBar, GetScalingAxis().GetInterval()) };
            // but only add a new interval if the longest bar is consuming more than
            // 20% of the current last interval; otherwise, there already is plenty of space
            // for the label
            if (barPercentOfLastInterval > math_constants::fifth)
                {
                GetScalingAxis().SetRange(
                    currentRange.first, currentRange.second + GetScalingAxis().GetInterval(),
                    GetScalingAxis().GetPrecision(), GetScalingAxis().GetInterval(),
                    GetScalingAxis().GetDisplayInterval());
                }
            }
        }

    //-----------------------------------
    void BarChart::UpdateScalingAxisFromBar(const Bar& bar)
        {
        // where the bar actually ends on the scaling axis
        const auto barEnd = bar.GetLength() + bar.GetCustomScalingAxisStartPosition().value_or(0);

        // if this bar is longer than previous ones, then update the scaling
        if (m_longestBarLength < barEnd)
            {
            m_longestBarLength = barEnd;
            GetScalingAxis().SetRange(
                0, m_longestBarLength, 0,
                // add a little extra padding to the scaling axis if we are using labels
                (GetBinLabelDisplay() != BinLabelDisplay::NoDisplay));

            AdjustScalingAxisFromBarLength(m_longestBarLength);
            }
        }

    //-----------------------------------
    void BarChart::ShowcaseBars(const std::vector<wxString>& labels)
        {
        for (auto& bar : GetBars())
            {
            const auto foundPos =
                std::find_if(labels.cbegin(), labels.cend(), [&bar](const auto& label)
                             { return label.CmpNoCase(bar.GetAxisLabel().GetText()) == 0; });
            bar.SetOpacity((foundPos == labels.cend()) ? GetGhostOpacity() : wxALPHA_OPAQUE);
            bar.m_barLabel.Show(foundPos != labels.cend());
            }
        }

    //-----------------------------------
    void BarChart::ShowcaseBars(const std::vector<double>& positions)
        {
        for (auto& bar : GetBars())
            {
            const auto foundPos =
                std::find_if(positions.cbegin(), positions.cend(), [&bar](const auto& position)
                             { return compare_doubles(bar.GetAxisPosition(), position); });
            bar.SetOpacity((foundPos == positions.cend()) ? GetGhostOpacity() : wxALPHA_OPAQUE);
            bar.m_barLabel.Show(foundPos != positions.cend());
            }
        }

    //-----------------------------------
    void BarChart::SortBars(std::vector<wxString> labels, const Wisteria::SortDirection direction)
        {
        assert(IsSortable() && L"Bars are not sortable. "
                               "Call SetSortable(true) prior to calling SortBars().");
        m_sortDirection = direction;
        if (!IsSortable() || direction == SortDirection::NoSort || GetBarAxis().IsReversed())
            {
            return;
            }
        std::set<wxString> barLabels;
        // make sure all bar labels are unique
        for (const auto& bar : GetBars())
            {
            barLabels.insert(bar.GetAxisLabel().GetText());
            }
        if (barLabels.size() != GetBars().size())
            {
            return;
            }

        // bar groups and brackets connected to bars' positions will need to be removed
        m_barGroups.clear();
        GetBarAxis().ClearBrackets();

        // verify that provided labels are in the existing bars
        // (if not, then add an empty bar for it)
        for (const auto& label : labels)
            {
            const auto foundPos =
                std::find_if(GetBars().cbegin(), GetBars().cend(), [&label](const auto& bar)
                             { return bar.GetAxisLabel().GetText().CmpNoCase(label) == 0; });
            // if bar with label not found, then add an empty one
            if (foundPos == GetBars().cend())
                {
                const auto maxAxisPos =
                    GetBars().size() ?
                        std::max_element(GetBars().cbegin(), GetBars().cend(),
                                         [](const auto& lhv, const auto& rhv) noexcept
                                         { return lhv.GetAxisPosition() < rhv.GetAxisPosition(); })
                            ->GetAxisPosition() :
                        0.0;
                AddBar(Bar{ maxAxisPos + GetBarAxis().GetTickMarkInterval(),
                            { BarBlock{} },
                            label,
                            Wisteria::GraphItems::Label{ label },
                            GetBarEffect(),
                            GetBarOpacity() });
                }
            }
        // resort in case bars were added
        std::sort(GetBars().begin(), GetBars().end(), [](const auto& lhv, const auto& rhv) noexcept
                  { return lhv.GetAxisPosition() < rhv.GetAxisPosition(); });
        // if not all labels were provided, then get the other bar labels that weren't
        // provided and sort those, pushing them all beneath the provided bars
        if (labels.size() != GetBars().size())
            {
            std::vector<wxString> otherLabelBars;
            // get the bar labels that the caller did not specify
            for (const auto& bar : GetBars())
                {
                const auto foundPos =
                    std::find_if(labels.cbegin(), labels.cend(), [&bar](const auto& label)
                                 { return label.CmpNoCase(bar.GetAxisLabel().GetText()) == 0; });
                if (foundPos == labels.cend())
                    {
                    otherLabelBars.push_back(bar.GetAxisLabel().GetText());
                    }
                }
            // sort them, set the expected order, and copy them after the labels the caller provided
            std::sort(otherLabelBars.begin(), otherLabelBars.end(),
                      [](const auto& lhv, const auto& rhv) { return lhv.CmpNoCase(rhv) < 0; });
            if ((direction == SortDirection::SortDescending &&
                 GetBarOrientation() == Orientation::Vertical) ||
                (direction == SortDirection::SortAscending &&
                 GetBarOrientation() == Orientation::Horizontal))
                {
                std::reverse(otherLabelBars.begin(), otherLabelBars.end());
                }
            std::copy(otherLabelBars.cbegin(), otherLabelBars.cend(), std::back_inserter(labels));
            }
        // shouldn't happen, sanity test
        if (labels.size() != GetBars().size())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"Bar label count (%zu) is different from bar count (%zu) when sorting."),
                    labels.size(), GetBars().size())
                    .ToUTF8());
            }

        // adapted from
        // https://stackoverflow.com/questions/838384/reorder-vector-using-a-vector-of-indices
        std::vector<size_t> indices;
        const auto reorderBars = [&indices, this]()
        {
            for (size_t s{ 1 }, d{ 0 }; s < indices.size(); ++s)
                {
                for (d = indices[s]; d < s; d = indices[d])
                    ;
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
                      { return lhv.GetAxisPosition() < rhv.GetAxisPosition(); });
        };

        // Reorder the provided labels (if necessary) to match the sorting direction
        // along the Y axis. The Y axis origin is at the bottom and goes upwards,
        // making it necessary to reverse the order of the labels to match that.
        if ((direction == SortDirection::SortDescending &&
             GetBarOrientation() == Orientation::Vertical) ||
            (direction == SortDirection::SortAscending &&
             GetBarOrientation() == Orientation::Horizontal))
            {
            std::reverse(labels.begin(), labels.end());
            }
        // get the indices into the bars based on the order of the provided labels
        for (const auto& label : labels)
            {
            const auto foundPos =
                std::find_if(GetBars().cbegin(), GetBars().cend(), [&label](const auto& bar)
                             { return bar.GetAxisLabel().GetText().CmpNoCase(label) == 0; });
            // shouldn't happen, sanity test
            if (foundPos == GetBars().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': bar label not found when sorting."), label)
                        .ToUTF8());
                }
            indices.push_back(std::distance(GetBars().cbegin(), foundPos));
            }

        const bool isDisplayingOuterLabels = GetBarAxis().IsShowingOuterLabels();
        GetBarAxis().ClearCustomLabels();

        reorderBars();
        // reset the bar axis's labels
        for (const auto& bar : GetBars())
            {
            GetBarAxis().SetCustomLabel(bar.GetAxisPosition(), bar.GetAxisLabel());
            }

        GetBarAxis().ShowOuterLabels(isDisplayingOuterLabels);
        if (GetCanvas() != nullptr)
            {
            wxMemoryDC measureDC;
            GetCanvas()->CalcAllSizes(measureDC);
            }
        }

    //-----------------------------------
    void BarChart::SortBars(const BarSortComparison sortMethod,
                            const Wisteria::SortDirection direction)
        {
        assert(IsSortable() && L"Bars are not sortable. "
                               "Call SetSortable(true) prior to calling SortBars().");
        m_sortDirection = direction;
        if (!IsSortable() || direction == SortDirection::NoSort || GetBarAxis().IsReversed())
            {
            return;
            }

        // bar groups and brackets connected to bars' positions will need to be removed
        m_barGroups.clear();
        GetBarAxis().ClearBrackets();

        const bool isDisplayingOuterLabels = GetBarAxis().IsShowingOuterLabels();
        GetBarAxis().ClearCustomLabels();

        // sorts smallest-to-largest
        if (sortMethod == BarSortComparison::SortByBarLength)
            {
            std::sort(m_bars.begin(), m_bars.end());
            }
        else
            {
            std::sort(m_bars.begin(), m_bars.end(),
                      [](const Bar& left, const Bar& right)
                      {
                          return wxUILocale::GetCurrent().CompareStrings(
                                     left.GetAxisLabel().GetText(), right.GetAxisLabel().GetText(),
                                     wxCompare_CaseInsensitive) < 0;
                      });
            }
        // Because we start at the origin, descending when horizontal goes the opposite way
        // internally. When it's displayed, descending will be shown as going largest-to-smallest as
        // one would expect.
        if ((direction == SortDirection::SortAscending &&
             GetBarOrientation() == Orientation::Vertical) ||
            (direction == SortDirection::SortDescending &&
             GetBarOrientation() == Orientation::Horizontal))
            {
            for (auto pos = m_bars.begin(); pos != m_bars.end(); ++pos)
                {
                pos->SetAxisPosition(m_highestBarAxisPosition =
                                         m_lowestBarAxisPosition +
                                         (GetBarAxis().GetInterval() * (pos - m_bars.begin())));
                GetBarAxis().SetCustomLabel(pos->GetAxisPosition(), pos->GetAxisLabel());
                }
            GetBarAxis().SetRange((m_lowestBarAxisPosition - GetBarAxis().GetInterval()),
                                  m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                                  GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                                  GetBarAxis().GetDisplayInterval());
            }
        else
            {
            for (auto pos = m_bars.rbegin(); pos != m_bars.rend(); ++pos)
                {
                pos->SetAxisPosition(m_highestBarAxisPosition =
                                         m_lowestBarAxisPosition +
                                         (GetBarAxis().GetInterval() * (pos - m_bars.rbegin())));
                GetBarAxis().SetCustomLabel(pos->GetAxisPosition(), pos->GetAxisLabel());
                }
            GetBarAxis().SetRange((m_lowestBarAxisPosition - GetBarAxis().GetInterval()),
                                  m_highestBarAxisPosition + GetBarAxis().GetInterval(),
                                  GetBarAxis().GetPrecision(), GetBarAxis().GetInterval(),
                                  GetBarAxis().GetDisplayInterval());
            }
        GetBarAxis().ShowOuterLabels(isDisplayingOuterLabels);
        if (GetCanvas() != nullptr)
            {
            wxMemoryDC measureDC;
            GetCanvas()->CalcAllSizes(measureDC);
            }
        }

    //-----------------------------------
    wxPoint BarChart::DrawBarBlockHorizontal(const Bar& bar, const size_t barIndex,
                                             const BarBlock& barBlock, BarRenderInfo& barRenderInfo,
                                             BarBlockRenderInfo& barBlockRenderInfo,
                                             const bool measureOnly /*= false*/)
        {
        const wxRect drawArea{ GetDrawArea() };

        wxPoint arrowPoints[7]{ { 0, 0 } };
        wxPoint boxPoints[4]{ { 0, 0 } };

        /* If the bar (or block) is set to cover a specific range
           (e.g., histograms do this) then calculate
           the width of the bar based on the coordinates.
           Otherwise, just divvy up the bars evenly to fit the plot window.*/
        if (barBlock.GetCustomWidth().has_value())
            {
            wxPoint topRightPointOfBar, bottomRightPointOfBar;
            GetPhysicalCoordinates(barBlock.GetLength() /* offset doesn't matter here */,
                                   bar.GetAxisPosition() -
                                       safe_divide<double>(barBlock.GetCustomWidth().value(), 2),
                                   topRightPointOfBar);
            GetPhysicalCoordinates(barBlock.GetLength(),
                                   bar.GetAxisPosition() +
                                       safe_divide<double>(barBlock.GetCustomWidth().value(), 2),
                                   bottomRightPointOfBar);
            barRenderInfo.m_barWidth = (topRightPointOfBar.y - bottomRightPointOfBar.y);
            }
        else if (bar.GetCustomWidth().has_value())
            {
            wxPoint topRightPointOfBar, bottomRightPointOfBar;
            GetPhysicalCoordinates(barBlock.GetLength() /* offset doesn't matter here */,
                                   bar.GetAxisPosition() -
                                       safe_divide<double>(bar.GetCustomWidth().value(), 2),
                                   topRightPointOfBar);
            GetPhysicalCoordinates(barBlock.GetLength(),
                                   bar.GetAxisPosition() +
                                       safe_divide<double>(bar.GetCustomWidth().value(), 2),
                                   bottomRightPointOfBar);
            barRenderInfo.m_barWidth = (topRightPointOfBar.y - bottomRightPointOfBar.y);
            }
        else
            {
            const size_t barSlots = GetBarSlotCount();
            const size_t overallBarSpacing = (barRenderInfo.m_barSpacing * (barSlots - 1));
            barRenderInfo.m_barWidth = safe_divide<double>(
                GetPlotAreaBoundingBox().GetHeight() -
                    (overallBarSpacing < GetPlotAreaBoundingBox().GetWidth() + barSlots ?
                         overallBarSpacing :
                         0),
                (barSlots + 1));
            }

        wxCoord lineXStart{ 0 }; // set the left (starting point) of the bar
        if (GetScalingAxis().IsReversed())
            {
            if (bar.GetCustomScalingAxisStartPosition().has_value())
                {
                GetPhysicalCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset,
                                       bar.GetAxisPosition(),
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                wxPoint customStartPt;
                GetPhysicalCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       bar.GetAxisPosition(), customStartPt);
                lineXStart = customStartPt.x +
                             (barBlockRenderInfo.m_axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                }
            else
                {
                // right side of the block
                GetPhysicalCoordinates(
                    GetScalingAxis().GetRange().first + barBlockRenderInfo.m_axisOffset,
                    bar.GetAxisPosition(), barBlockRenderInfo.m_middlePointOfBarEnd);
                // left side of the block
                wxPoint pt;
                GetPhysicalCoordinates(GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       bar.GetAxisPosition(), pt);
                // if the first block, push it over 1 pixel so that it doesn't overlap the
                // bar axis
                lineXStart =
                    pt.x + (barBlockRenderInfo.m_axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                }
            }
        else
            {
            if (bar.GetCustomScalingAxisStartPosition().has_value())
                {
                GetPhysicalCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       bar.GetAxisPosition(),
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                wxPoint customStartPt;
                GetPhysicalCoordinates(bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset,
                                       bar.GetAxisPosition(), customStartPt);
                lineXStart = customStartPt.x +
                             (barBlockRenderInfo.m_axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                }
            else
                {
                // right side of the block
                GetPhysicalCoordinates(GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       bar.GetAxisPosition(),
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                // left side of the block
                wxPoint pt;
                GetPhysicalCoordinates(GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset,
                                       bar.GetAxisPosition(), pt);
                // if the first block, push it over 1 pixel so that it doesn't overlap the
                // bar axis
                lineXStart =
                    pt.x + (barBlockRenderInfo.m_axisOffset == 0 ? ScaleToScreenAndCanvas(1) : 0);
                }
            }

        const wxCoord barLength = barBlockRenderInfo.m_middlePointOfBarEnd.x - lineXStart;
        barBlockRenderInfo.m_axisOffset += barBlock.GetLength();

        const wxCoord lineYStart = barBlockRenderInfo.m_middlePointOfBarEnd.y -
                                   safe_divide<double>(barRenderInfo.m_barWidth, 2.0);
        const auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();
        barRenderInfo.m_barRect =
            wxRect(lineXStart, lineYStart, barLength, barRenderInfo.m_barWidth);
        wxRect barNeckRect = barRenderInfo.m_barRect;

        // if just measuring, then we're done
        if (measureOnly)
            {
            return barBlockRenderInfo.m_middlePointOfBarEnd;
            }

        // draw the bar (block)
        if (barBlock.IsShown() && barLength > 0)
            {
            // if block has a customized opacity, then use that instead of the
            // bar's opacity
            const wxColour blockColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                                            Wisteria::Colors::ColorContrast::ChangeOpacity(
                                                barBlock.GetBrush().GetColour(), bar.GetOpacity()) :
                                            barBlock.GetBrush().GetColour();
            const wxColour blockLightenedColor =
                barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                    Wisteria::Colors::ColorContrast::ChangeOpacity(barBlock.GetLightenedColor(),
                                                                   bar.GetOpacity()) :
                    barBlock.GetLightenedColor();
            wxBrush blockBrush{ barBlock.GetBrush() };
            blockBrush.SetColour(blockColor);

            if (bar.GetEffect() == BoxEffect::CommonImage && barRenderInfo.m_scaledCommonImg.IsOk())
                {
                wxRect imgSubRect{ barRenderInfo.m_barRect };
                imgSubRect.Offset(-GetPlotAreaBoundingBox().GetX(),
                                  -GetPlotAreaBoundingBox().GetY());
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(GetImageOutlineColor())
                        .AnchorPoint(wxPoint(lineXStart, lineYStart)),
                    barRenderInfo.m_scaledCommonImg.GetSubImage(imgSubRect));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                barImage->SetShadowType((GetShadowType() != ShadowType::NoDisplay) ?
                                            ShadowType::RightSideAndBottomShadow :
                                            ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::Image && GetImageScheme() != nullptr)
                {
                const auto& barScaledImage = GetImageScheme()->GetImage(barIndex);
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(GetImageOutlineColor())
                        .AnchorPoint(wxPoint(lineXStart, lineYStart)),
                    Wisteria::GraphItems::Image::CropImageToRect(
                        barScaledImage.GetBitmap(barScaledImage.GetDefaultSize()).ConvertToImage(),
                        barRenderInfo.m_barRect, true));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                barImage->SetShadowType((GetShadowType() != ShadowType::NoDisplay) ?
                                            ShadowType::RightSideAndBottomShadow :
                                            ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::StippleImage && GetStippleBrush().IsOk())
                {
                assert((bar.GetShape() == BarShape::Rectangle) &&
                       L"Non-rectangular shapes not currently "
                       "supported with stipple bar effect.");
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(wxNullPen)
                        .AnchorPoint(wxPoint(lineXStart, lineYStart)),
                    Wisteria::GraphItems::Image::CreateStippledImage(
                        GetStippleBrush()
                            .GetBitmap(GetStippleBrush().GetDefaultSize())
                            .ConvertToImage(),
                        wxSize(barLength, barRenderInfo.m_barWidth), Orientation::Horizontal,
                        (GetShadowType() != ShadowType::NoDisplay), ScaleToScreenAndCanvas(4)));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                // note that stipples have their own shadows (a silhouette), so turn off
                // the Image's native shadow renderer.
                barImage->SetShadowType(ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::StippleShape)
                {
                assert((bar.GetShape() == BarShape::Rectangle) &&
                       L"Non-rectangular shapes not currently "
                       "supported with stipple bar effect.");
                auto shapeWidth{ barRenderInfo.m_barWidth };
                // These particular icons are drawn with a ratio where the width
                // is 60% of the height if the drawing area is square. To prevent
                // having large gaps between the icons, adjust the width of the icons'
                // drawing areas so that they aren't drawn inside of squares.
                if (GetStippleShape() == Icons::IconShape::BusinessWoman ||
                    GetStippleShape() == Icons::IconShape::Woman ||
                    GetStippleShape() == Icons::IconShape::Man)
                    {
                    shapeWidth *= 0.6;
                    }
                else if (GetStippleShape() == Icons::IconShape::Ruler)
                    {
                    shapeWidth *= 0.4;
                    }
                // likewise, handle icons that are wider than others
                if (GetStippleShape() == Icons::IconShape::Car ||
                    GetStippleShape() == Icons::IconShape::Blackboard)
                    {
                    shapeWidth *= 1.25;
                    }
                auto currentXLeft = lineXStart;
                while (currentXLeft < (lineXStart + barLength))
                    {
                    const wxSize stippleImgSize(shapeWidth, barRenderInfo.m_barWidth);
                    auto shape = std::make_unique<Wisteria::GraphItems::Shape>(
                        Wisteria::GraphItems::GraphItemInfo{}
                            .Pen(wxNullPen)
                            .Brush(GetStippleShapeColor())
                            .AnchorPoint(wxPoint(currentXLeft, lineYStart))
                            .Anchoring(Anchoring::TopLeftCorner)
                            .DPIScaling(GetDPIScaleFactor())
                            .Scaling(GetScaling()),
                        GetStippleShape(), stippleImgSize);
                    shape->SetBoundingBox(wxRect(wxPoint(currentXLeft, lineYStart),
                                                 wxSize(shapeWidth, barRenderInfo.m_barWidth)),
                                          barRenderInfo.m_dc, GetScaling());
                    shape->SetClippingRect(barRenderInfo.m_barRect);
                    AddObject(std::move(shape));
                    currentXLeft += stippleImgSize.GetWidth();
                    }
                }
            // color-filled bar
            else
                {
                std::unique_ptr<GraphItems::Polygon> box{ nullptr };
                GraphItems::Polygon::GetRectPoints(barRenderInfo.m_barRect, boxPoints);
                if (bar.GetShape() == BarShape::Rectangle)
                    {
                    // Polygons don't support drop shadows,
                    // so need to manually add a shadow as another polygon.
                    // Also, only use a shadow if the fill color is opaque and
                    // the bar block isn't too small.
                    if (GetShadowType() != ShadowType::NoDisplay && blockBrush.GetColour().IsOk() &&
                        blockBrush.GetColour().GetAlpha() == wxALPHA_OPAQUE &&
                        barBlock.GetLength() > rangeStart)
                        {
                        // in case this bar is way too small because of the
                        // scaling then don't bother with the shadow
                        if (barRenderInfo.m_barRect.GetHeight() >
                            barRenderInfo.m_scaledShadowOffset)
                            {
                            wxPoint shadowPts[7] = {
                                barRenderInfo.m_barRect.GetLeftBottom(),
                                barRenderInfo.m_barRect.GetLeftBottom() +
                                    wxPoint(0, barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightBottom() +
                                    wxPoint(barRenderInfo.m_scaledShadowOffset,
                                            barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightTop() +
                                    wxPoint(barRenderInfo.m_scaledShadowOffset,
                                            barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightTop() +
                                    wxPoint(0, barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightBottom(),
                                barRenderInfo.m_barRect.GetLeftBottom() // close polygon
                            };
                            AddObject(std::make_unique<Wisteria::GraphItems::Polygon>(
                                Wisteria::GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(
                                    GraphItemBase::GetShadowColor()),
                                shadowPts, std::size(shadowPts)));
                            }
                        }
                    box = std::make_unique<GraphItems::Polygon>(
                        Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                            .Pen(*wxBLACK_PEN)
                            .Brush(blockBrush)
                            .Scaling(GetScaling())
                            .Outline(true, true, true, true)
                            .ShowLabelWhenSelected(true),
                        boxPoints, std::size(boxPoints));
                    }
                else if (bar.GetShape() == BarShape::Arrow)
                    {
                    assert(!(GetShadowType() != ShadowType::NoDisplay) &&
                           L"Drop shadow not supported for arrow shape currently.");
                    barNeckRect.Deflate(wxSize(0, safe_divide(barNeckRect.GetHeight(), 5)));
                    barNeckRect.SetRight(barNeckRect.GetRight() -
                                         (safe_divide(barNeckRect.GetWidth(), 10)));
                    arrowPoints[0] = barNeckRect.GetTopLeft();
                    arrowPoints[1] = barNeckRect.GetTopRight();
                    arrowPoints[2] =
                        wxPoint(barNeckRect.GetRight(), barRenderInfo.m_barRect.GetTop());
                    arrowPoints[3] =
                        wxPoint(barRenderInfo.m_barRect.GetRight(),
                                barRenderInfo.m_barRect.GetTop() +
                                    (safe_divide((barRenderInfo.m_barRect.GetHeight()), 2)));
                    arrowPoints[4] =
                        wxPoint(barNeckRect.GetRight(), barRenderInfo.m_barRect.GetBottom());
                    arrowPoints[5] = barNeckRect.GetBottomRight();
                    arrowPoints[6] = barNeckRect.GetBottomLeft();
                    box = std::make_unique<Wisteria::GraphItems::Polygon>(
                        Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                            .Pen(*wxBLACK_PEN)
                            .Brush(blockBrush)
                            .Scaling(GetScaling())
                            .Outline(true, true, true, true)
                            .ShowLabelWhenSelected(true),
                        arrowPoints, std::size(arrowPoints));
                    }

                assert(box);

                if (barBlock.GetOutlinePen().IsOk())
                    {
                    box->GetPen() = barBlock.GetOutlinePen();
                    }
                else
                    {
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxWHITE :
                            *wxBLACK);
                    }
                if (bar.GetEffect() == BoxEffect::FadeFromBottomToTop)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(
                        Colors::GradientFill(blockColor, blockLightenedColor, FillDirection::East));
                    }
                else if (bar.GetEffect() == BoxEffect::Glassy)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(Colors::GradientFill(blockColor,
                                                                blockColor, // second color not used
                                                                FillDirection::South));
                    }
                else if (bar.GetEffect() == BoxEffect::FadeFromTopToBottom)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(
                        Colors::GradientFill(blockColor, blockLightenedColor, FillDirection::West));
                    }
                // in case an explicit color is used for the background
                // and the brush is perhaps a hatch to be draw on top of it
                else if (barBlock.GetColor().IsOk())
                    {
                    box->SetBackgroundFill(Colors::GradientFill(barBlock.GetColor()));
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxWHITE :
                            *wxBLACK);
                    }

                // if the bar is totally transparent, then draw a contrasting outline
                // (unless the client also made the outline explicitly transparent)
                if (bar.GetOpacity() == wxALPHA_TRANSPARENT && box->GetPen().IsOk() &&
                    box->GetPen() != *wxTRANSPARENT_PEN)
                    {
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxBLACK :
                            *wxWHITE);
                    }

                // if the box is really thin, then don't use the outline pen on its sides
                if (DownscaleFromScreenAndCanvas(barRenderInfo.m_barRect.GetWidth()) < 5)
                    {
                    box->GetGraphItemInfo().Outline(true, false, true, false);
                    }
                box->SetShape((bar.GetEffect() == BoxEffect::WaterColor) ?
                                  GraphItems::Polygon::PolygonShape::WaterColorRectangle :
                              (bar.GetEffect() == BoxEffect::ThickWaterColor) ?
                                  GraphItems::Polygon::PolygonShape::ThickWaterColorRectangle :
                              (bar.GetShape() == BarShape::Arrow) ?
                                  GraphItems::Polygon::PolygonShape::Irregular :
                              (bar.GetEffect() == BoxEffect::Glassy) ?
                                  GraphItems::Polygon::PolygonShape::GlassyRectangle :
                                  GraphItems::Polygon::PolygonShape::Rectangle);
                // along with a second coat, we will make the thick water color
                // brush use a more opaque value than the system's default
                if (bar.GetEffect() == BoxEffect::ThickWaterColor && box->GetBrush().IsOk() &&
                    box->GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE &&
                    Settings::GetTranslucencyValue() < 200)
                    {
                    box->GetBrush().SetColour(Wisteria::Colors::ColorContrast::ChangeOpacity(
                        box->GetBrush().GetColour(), 200));
                    }
                // flip outline logic so that we have a hard outline since we are
                // not "drawing within the lines" (also, don't clip)
                if (bar.GetEffect() == BoxEffect::WaterColor ||
                    bar.GetEffect() == BoxEffect::ThickWaterColor)
                    {
                    // ...but only use hard outline if there isn't a user-defined outline
                    if (!barBlock.GetOutlinePen().IsOk())
                        {
                        box->GetPen().SetColour(
                            Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                *wxBLACK :
                                *wxWHITE);
                        }
                    SetDefaultLegendShape(Icons::IconShape::WaterColorRectangle);
                    }
                // clip box to not be on top of axes
                else
                    {
                    box->SetClippingRect(drawArea);
                    SetDefaultLegendShape(Icons::IconShape::Square);
                    }
                // add the box to the plot item collection
                AddObject(std::move(box));
                }
            }
        // add the decal (if there is one)
        if (barBlock.IsShown() && barBlock.GetLength() > 0 &&
            !barBlock.GetDecal().GetText().empty())
            {
            const wxCoord leftPadding = ScaleToScreenAndCanvas(2);
            wxRect decalRect(barNeckRect);
            decalRect.Deflate(leftPadding, 0);

            auto decalLabel = std::make_unique<GraphItems::Label>(barBlock.GetDecal());
            decalLabel->GetGraphItemInfo()
                .Pen(wxNullPen)
                .Text(barBlock.ExpandDecalLabel())
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Padding(2, 2, 2, 2);
            decalLabel->GetFont().MakeSmaller().MakeSmaller();
            if (decalLabel->GetLabelFit() == LabelFit::ScaleFontToFit)
                {
                decalLabel->SetBoundingBox(decalRect, barRenderInfo.m_dc, GetScaling());
                decalLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                }
            else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFit)
                {
                decalLabel->SplitTextToFitBoundingBox(barRenderInfo.m_dc, decalRect.GetSize());
                }
            else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFitWidth)
                {
                decalLabel->SplitTextToFitBoundingBox(
                    barRenderInfo.m_dc,
                    wxSize(decalRect.GetWidth(), std::numeric_limits<int>::max()));
                }
            // if drawing as-is, then draw a box around the label
            // if it's larger than the parent block
            else if (decalLabel->GetLabelFit() == LabelFit::DisplayAsIsAutoFrame)
                {
                const auto actualDecalRect = decalLabel->GetBoundingBox(barRenderInfo.m_dc);
                // allow a little wiggle room
                if (actualDecalRect.GetWidth() - ScaleToScreenAndCanvas(1) > decalRect.GetWidth() ||
                    actualDecalRect.GetHeight() - ScaleToScreenAndCanvas(1) > decalRect.GetHeight())
                    {
                    decalLabel->GetGraphItemInfo()
                        .FontBackgroundColor(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                            decalLabel->GetFontColor()))
                        .Pen(*wxBLACK_PEN);
                    }
                }
            // make multiline decals a little more compact so that
            // they have a better chance of fitting
            decalLabel->SetLineSpacing(0);
            decalLabel->SetShadowType(ShadowType::NoDisplay);
            decalLabel->SetTextAlignment(TextAlignment::FlushLeft);
            decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
            // allow selecting the bar underneath this label
            decalLabel->SetSelectable(false);
            // if font is way too small, then show it as a label
            // overlapping the bar instead of a decal
            if (decalLabel->GetLabelFit() != LabelFit::DisplayAsIs &&
                decalLabel->GetLabelFit() != LabelFit::DisplayAsIsAutoFrame &&
                decalLabel->GetFont().GetPointSize() < barRenderInfo.m_defaultFontPointSize / 2)
                {
                decalLabel->GetFont().SetPointSize(barRenderInfo.m_defaultFontPointSize);
                decalLabel->GetPen().SetColour(*wxBLACK);
                decalLabel->SetFontColor(*wxBLACK);
                decalLabel->SetFontBackgroundColor(*wxWHITE);
                }
            const wxRect labelBox = decalLabel->GetBoundingBox(barRenderInfo.m_dc);
            if (decalLabel->GetRelativeAlignment() == RelativeAlignment::FlushLeft)
                {
                decalLabel->SetAnchorPoint(
                    wxPoint((barNeckRect.GetLeft() + leftPadding),
                            (barNeckRect.GetTop() +
                             safe_divide(barNeckRect.GetHeight() - labelBox.GetHeight(), 2))));
                }
            else if (decalLabel->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                decalLabel->SetAnchorPoint(
                    wxPoint((barNeckRect.GetLeft() +
                             safe_divide(barNeckRect.GetWidth() - labelBox.GetWidth(), 2)),
                            (barNeckRect.GetTop() +
                             safe_divide(barNeckRect.GetHeight() - labelBox.GetHeight(), 2))));
                }
            else // flush right
                {
                decalLabel->SetAnchorPoint(
                    wxPoint((barNeckRect.GetRight() - (labelBox.GetWidth() + leftPadding)),
                            (barNeckRect.GetTop() +
                             safe_divide(barNeckRect.GetHeight() - labelBox.GetHeight(), 2))));
                }
            // if drawing a color and hatch pattern, then show the decal with an outline
            // to make it easier to read
            if (bar.GetEffect() == BoxEffect::Solid && barBlock.GetColor().IsOk() &&
                barBlock.GetBrush().GetStyle() != wxBrushStyle::wxBRUSHSTYLE_SOLID)
                {
                decalLabel->GetPen().SetColour(*wxBLACK);
                decalLabel->SetFontColor(*wxBLACK);
                decalLabel->SetFontBackgroundColor(*wxWHITE);
                }
            // This will be added to the plot's collection of object AFTER
            // all blocks have been added.
            // This ensures that decals that go outside of their block are
            // eclipsed by the next block.
            barRenderInfo.m_decals.push_back(std::move(decalLabel));
            }

        return barBlockRenderInfo.m_middlePointOfBarEnd;
        }

    //-----------------------------------
    wxPoint BarChart::DrawBarBlockVertical(const Bar& bar, const size_t barIndex,
                                           const BarBlock& barBlock, BarRenderInfo& barRenderInfo,
                                           BarBlockRenderInfo& barBlockRenderInfo,
                                           const bool measureOnly /*= false*/)
        {
        const wxRect drawArea{ GetDrawArea() };

        wxPoint boxPoints[4]{ { 0, 0 } };
        wxPoint arrowPoints[7]{ { 0, 0 } };

        /* if the bar (or block) is set to cover a specific range
           (e.g., histograms do this when using cutpoints) then calculate
           the width of the bar based on the coordinates.
           Otherwise, just divvy up the bars evenly to fit the plot window.*/
        if (barBlock.GetCustomWidth().has_value())
            {
            wxPoint leftPointOfBar, rightPointOfBar;
            GetPhysicalCoordinates(
                bar.GetAxisPosition() - safe_divide<double>(barBlock.GetCustomWidth().value(), 2),
                barBlock.GetLength() /* offset doesn't matter here */, leftPointOfBar);
            GetPhysicalCoordinates(bar.GetAxisPosition() +
                                       safe_divide<double>(barBlock.GetCustomWidth().value(), 2),
                                   barBlock.GetLength(), rightPointOfBar);
            barRenderInfo.m_barWidth =
                ((rightPointOfBar.x - leftPointOfBar.x) - barRenderInfo.m_barSpacing);
            }
        else if (bar.GetCustomWidth().has_value())
            {
            wxPoint leftPointOfBar, rightPointOfBar;
            GetPhysicalCoordinates(
                bar.GetAxisPosition() - safe_divide<double>(bar.GetCustomWidth().value(), 2),
                barBlock.GetLength() /* offset doesn't matter here */, leftPointOfBar);
            GetPhysicalCoordinates(bar.GetAxisPosition() +
                                       safe_divide<double>(bar.GetCustomWidth().value(), 2),
                                   barBlock.GetLength(), rightPointOfBar);
            barRenderInfo.m_barWidth =
                ((rightPointOfBar.x - leftPointOfBar.x) - barRenderInfo.m_barSpacing);
            }
        else
            {
            const size_t barSlots = GetBarSlotCount();
            const size_t overallBarSpacing = (barRenderInfo.m_barSpacing * (barSlots - 1));
            barRenderInfo.m_barWidth = safe_divide<double>(
                // the plot area, minus the cumulative spaces between their bars
                // (unless the spacing is too aggressive)
                GetPlotAreaBoundingBox().GetWidth() -
                    (overallBarSpacing < GetPlotAreaBoundingBox().GetWidth() + barSlots ?
                         overallBarSpacing :
                         0),
                // add an "extra" bar to account for the half bar space
                // around the first and last bars
                (barSlots + 1));
            }

        // set the bottom (starting point) of the bar
        wxCoord lineYStart{ 0 };
        if (GetScalingAxis().IsReversed())
            {
            if (bar.GetCustomScalingAxisStartPosition().has_value())
                {
                // top of block
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset,
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                // bottom of block
                wxPoint customStartPt;
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       customStartPt);
                lineYStart = customStartPt.y;
                }
            else
                {
                // top of block
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset,
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                // bottom of block
                wxPoint pt;
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       pt);
                lineYStart = pt.y;
                }
            }
        else
            {
            if (bar.GetCustomScalingAxisStartPosition().has_value())
                {
                // top of block
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                // bottom of block
                wxPoint customStartPt;
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       bar.GetCustomScalingAxisStartPosition().value() +
                                           barBlockRenderInfo.m_axisOffset,
                                       customStartPt);
                lineYStart = customStartPt.y;
                }
            else
                {
                // top of block
                GetPhysicalCoordinates(bar.GetAxisPosition(),
                                       GetScalingAxis().GetRange().first +
                                           barBlockRenderInfo.m_axisOffset + barBlock.GetLength(),
                                       barBlockRenderInfo.m_middlePointOfBarEnd);
                // bottom of block
                wxPoint pt;
                GetPhysicalCoordinates(
                    bar.GetAxisPosition(),
                    GetScalingAxis().GetRange().first + barBlockRenderInfo.m_axisOffset, pt);
                lineYStart = pt.y;
                }
            }

        barBlockRenderInfo.m_axisOffset += barBlock.GetLength();
        const wxCoord barLength = lineYStart - barBlockRenderInfo.m_middlePointOfBarEnd.y;
        const wxCoord lineYEnd = lineYStart - barLength;
        const wxCoord lineXStart = barBlockRenderInfo.m_middlePointOfBarEnd.x -
                                   safe_divide<double>(barRenderInfo.m_barWidth, 2.0);
        const auto [rangeStart, rangeEnd] = GetLeftYAxis().GetRange();
        barRenderInfo.m_barRect = wxRect(lineXStart, lineYEnd, barRenderInfo.m_barWidth, barLength);
        wxRect barNeckRect = barRenderInfo.m_barRect;

        // if just measuring then we're done
        if (measureOnly)
            {
            return barBlockRenderInfo.m_middlePointOfBarEnd;
            }

        // draw the bar
        if (barBlock.IsShown() && barLength > 0)
            {
            // if block has a customized opacity, then use that instead of the bar's opacity
            const wxColour blockColor = barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                                            Wisteria::Colors::ColorContrast::ChangeOpacity(
                                                barBlock.GetBrush().GetColour(), bar.GetOpacity()) :
                                            barBlock.GetBrush().GetColour();
            const wxColour blockLightenedColor =
                barBlock.GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE ?
                    Wisteria::Colors::ColorContrast::ChangeOpacity(barBlock.GetLightenedColor(),
                                                                   bar.GetOpacity()) :
                    barBlock.GetLightenedColor();
            wxBrush blockBrush{ barBlock.GetBrush() };
            blockBrush.SetColour(blockColor);

            if (bar.GetEffect() == BoxEffect::CommonImage && barRenderInfo.m_scaledCommonImg.IsOk())
                {
                wxRect imgSubRect{ barRenderInfo.m_barRect };
                imgSubRect.Offset(-GetPlotAreaBoundingBox().GetX(),
                                  -GetPlotAreaBoundingBox().GetY());
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(GetImageOutlineColor())
                        .AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                    barRenderInfo.m_scaledCommonImg.GetSubImage(imgSubRect));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                barImage->SetShadowType((GetShadowType() != ShadowType::NoDisplay) ?
                                            ShadowType::RightSideShadow :
                                            ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::Image && GetImageScheme() != nullptr)
                {
                const auto& barScaledImage = GetImageScheme()->GetImage(barIndex);
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(GetImageOutlineColor())
                        .AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                    Wisteria::GraphItems::Image::CropImageToRect(
                        barScaledImage.GetBitmap(barScaledImage.GetDefaultSize()).ConvertToImage(),
                        barRenderInfo.m_barRect, true));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                barImage->SetShadowType((GetShadowType() != ShadowType::NoDisplay) ?
                                            ShadowType::RightSideAndBottomShadow :
                                            ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::StippleImage && GetStippleBrush().IsOk())
                {
                assert((bar.GetShape() == BarShape::Rectangle) &&
                       L"Non-rectangular shapes not currently "
                       "supported with stipple bar effect.");
                auto barImage = std::make_unique<Wisteria::GraphItems::Image>(
                    Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                        .Pen(wxNullPen)
                        .AnchorPoint(wxPoint(lineXStart, lineYEnd)),
                    Wisteria::GraphItems::Image::CreateStippledImage(
                        GetStippleBrush()
                            .GetBitmap(GetStippleBrush().GetDefaultSize())
                            .ConvertToImage(),
                        wxSize(barRenderInfo.m_barWidth, barLength), Orientation::Vertical,
                        (GetShadowType() != ShadowType::NoDisplay), ScaleToScreenAndCanvas(4)));
                barImage->SetOpacity(bar.GetOpacity());
                barImage->SetAnchoring(Anchoring::TopLeftCorner);
                // note that stipples have their own shadows (a silhouette), so turn off
                // the Image's native shadow renderer.
                barImage->SetShadowType(ShadowType::NoDisplay);
                barImage->SetClippingRect(drawArea);
                AddObject(std::move(barImage));
                }
            else if (bar.GetEffect() == BoxEffect::StippleShape)
                {
                assert((bar.GetShape() == BarShape::Rectangle) &&
                       L"Non-rectangular shapes not currently "
                       "supported with stipple bar effect.");
                auto shapeHeight{ barRenderInfo.m_barWidth };
                // These particular icons are drawn with a ratio where the height
                // is 75% of the width if the drawing area is square. To prevent
                // having large gaps between the icons, adjust the height of the icons'
                // drawing areas so that they aren't drawn inside of squares.
                if (GetStippleShape() == Icons::IconShape::Car ||
                    GetStippleShape() == Icons::IconShape::Blackboard)
                    {
                    shapeHeight *= 0.75;
                    }
                auto currentYTop = lineYStart - shapeHeight;
                while ((currentYTop + shapeHeight) > lineYEnd)
                    {
                    const wxSize stippleImgSize(barRenderInfo.m_barWidth, shapeHeight);
                    auto shape = std::make_unique<Wisteria::GraphItems::Shape>(
                        Wisteria::GraphItems::GraphItemInfo{}
                            .Pen(wxNullPen)
                            .Brush(GetStippleShapeColor())
                            .AnchorPoint(wxPoint(lineXStart, currentYTop))
                            .Anchoring(Anchoring::TopLeftCorner)
                            .DPIScaling(GetDPIScaleFactor())
                            .Scaling(GetScaling()),
                        GetStippleShape(), stippleImgSize);
                    shape->SetBoundingBox(wxRect(wxPoint(lineXStart, currentYTop),
                                                 wxSize(barRenderInfo.m_barWidth, shapeHeight)),
                                          barRenderInfo.m_dc, GetScaling());
                    shape->SetClippingRect(barRenderInfo.m_barRect);
                    AddObject(std::move(shape));
                    currentYTop -= stippleImgSize.GetHeight();
                    }
                }
            else
                {
                std::unique_ptr<GraphItems::Polygon> box{ nullptr };
                GraphItems::Polygon::GetRectPoints(barRenderInfo.m_barRect, boxPoints);
                if (bar.GetShape() == BarShape::Rectangle)
                    {
                    // polygons don't support drop shadows,
                    // so need to manually add a shadow as another polygon
                    if (GetShadowType() != ShadowType::NoDisplay && blockBrush.GetColour().IsOk() &&
                        blockBrush.GetColour().GetAlpha() == wxALPHA_OPAQUE &&
                        barBlock.GetLength() > rangeStart)
                        {
                        // in case this bar is way too small because of the scaling,
                        // then don't bother with the shadow
                        if (barRenderInfo.m_barRect.GetHeight() >
                            barRenderInfo.m_scaledShadowOffset)
                            {
                            const wxPoint shadowPts[4] = {
                                barRenderInfo.m_barRect.GetRightBottom() +
                                    wxPoint(barRenderInfo.m_scaledShadowOffset, 0),
                                barRenderInfo.m_barRect.GetRightTop() +
                                    wxPoint(barRenderInfo.m_scaledShadowOffset,
                                            barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightTop() +
                                    wxPoint(0, barRenderInfo.m_scaledShadowOffset),
                                barRenderInfo.m_barRect.GetRightBottom()
                            };
                            AddObject(std::make_unique<Wisteria::GraphItems::Polygon>(
                                Wisteria::GraphItems::GraphItemInfo().Pen(wxNullPen).Brush(
                                    GraphItemBase::GetShadowColor()),
                                shadowPts, std::size(shadowPts)));
                            }
                        }

                    box = std::make_unique<Wisteria::GraphItems::Polygon>(
                        Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                            .Pen(*wxBLACK_PEN)
                            .Brush(blockBrush)
                            .Scaling(GetScaling())
                            .Outline(true, true, true, true)
                            .ShowLabelWhenSelected(true),
                        boxPoints, std::size(boxPoints));
                    }
                else if (bar.GetShape() == BarShape::Arrow)
                    {
                    assert(!(GetShadowType() != ShadowType::NoDisplay) &&
                           L"Drop shadow not supported for arrow shape currently.");
                    barNeckRect.Deflate(wxSize(safe_divide(barNeckRect.GetWidth(), 5), 0));
                    const auto arrowHeadSize = safe_divide(barNeckRect.GetHeight(), 10);
                    barNeckRect.SetTop(barNeckRect.GetTop() + arrowHeadSize);
                    barNeckRect.SetHeight(barNeckRect.GetHeight() - arrowHeadSize);
                    arrowPoints[0] = barNeckRect.GetBottomLeft();
                    arrowPoints[1] = barNeckRect.GetTopLeft();
                    arrowPoints[2] =
                        wxPoint(barRenderInfo.m_barRect.GetLeft(), barNeckRect.GetTop());
                    arrowPoints[3] =
                        wxPoint(barRenderInfo.m_barRect.GetLeft() +
                                    (safe_divide(barRenderInfo.m_barRect.GetWidth(), 2)),
                                barRenderInfo.m_barRect.GetTop());
                    arrowPoints[4] =
                        wxPoint(barRenderInfo.m_barRect.GetRight(), barNeckRect.GetTop());
                    arrowPoints[5] = barNeckRect.GetTopRight();
                    arrowPoints[6] = barNeckRect.GetBottomRight();
                    box = std::make_unique<Wisteria::GraphItems::Polygon>(
                        Wisteria::GraphItems::GraphItemInfo(barBlock.GetSelectionLabel().GetText())
                            .Pen(*wxBLACK_PEN)
                            .Brush(blockBrush)
                            .Outline(true, true, true, true)
                            .Scaling(GetScaling())
                            .ShowLabelWhenSelected(true),
                        arrowPoints, std::size(arrowPoints));
                    }

                assert(box);
                if (barBlock.GetOutlinePen().IsOk())
                    {
                    box->GetPen() = barBlock.GetOutlinePen();
                    }
                else
                    {
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxWHITE :
                            *wxBLACK);
                    }

                if (bar.GetEffect() == BoxEffect::FadeFromBottomToTop)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(Colors::GradientFill(blockColor, blockLightenedColor,
                                                                FillDirection::North));
                    }
                else if (bar.GetEffect() == BoxEffect::FadeFromTopToBottom)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(Colors::GradientFill(blockColor, blockLightenedColor,
                                                                FillDirection::South));
                    }
                else if (bar.GetEffect() == BoxEffect::Glassy)
                    {
                    box->GetBrush() = wxNullBrush;
                    box->SetBackgroundFill(
                        Colors::GradientFill(blockColor, blockColor, FillDirection::East));
                    }
                // in case an explicit color is used for the background
                // and the brush is perhaps a hatch to be draw on top of it
                else if (barBlock.GetColor().IsOk())
                    {
                    box->SetBackgroundFill(Colors::GradientFill(barBlock.GetColor()));
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxWHITE :
                            *wxBLACK);
                    }

                // if the bar is totally transparent, then draw a contrasting outline
                // (unless the client also made the outline explicitly transparent)
                if (bar.GetOpacity() == wxALPHA_TRANSPARENT && box->GetPen().IsOk() &&
                    box->GetPen() != *wxTRANSPARENT_PEN)
                    {
                    box->GetPen().SetColour(
                        Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                            *wxBLACK :
                            *wxWHITE);
                    }

                // if the box is really thin, then don't use the outline pen on the
                // top/bottom
                if (DownscaleFromScreenAndCanvas(barRenderInfo.m_barRect.GetWidth()) < 5)
                    {
                    box->GetGraphItemInfo().Outline(false, true, false, true);
                    }
                box->SetShape((bar.GetEffect() == BoxEffect::WaterColor) ?
                                  GraphItems::Polygon::PolygonShape::WaterColorRectangle :
                              (bar.GetEffect() == BoxEffect::ThickWaterColor) ?
                                  GraphItems::Polygon::PolygonShape::ThickWaterColorRectangle :
                              (bar.GetShape() == BarShape::Arrow) ?
                                  GraphItems::Polygon::PolygonShape::Irregular :
                              (bar.GetEffect() == BoxEffect::Glassy) ?
                                  GraphItems::Polygon::PolygonShape::GlassyRectangle :
                                  GraphItems::Polygon::PolygonShape::Rectangle);
                // along with a second coat, we will make the thick water color
                // brush use a more opaque value than the system's default
                if (bar.GetEffect() == BoxEffect::ThickWaterColor && box->GetBrush().IsOk() &&
                    box->GetBrush().GetColour().Alpha() == wxALPHA_OPAQUE &&
                    Settings::GetTranslucencyValue() < 200)
                    {
                    box->GetBrush().SetColour(Wisteria::Colors::ColorContrast::ChangeOpacity(
                        box->GetBrush().GetColour(), 200));
                    }
                // flip outline logic so that we have a hard outline since we are
                // not "drawing within the lines" (also, don't clip)
                if (bar.GetEffect() == BoxEffect::WaterColor ||
                    bar.GetEffect() == BoxEffect::ThickWaterColor)
                    {
                    // ...but only use hard outline if there isn't a user-defined outline
                    if (!barBlock.GetOutlinePen().IsOk())
                        {
                        box->GetPen().SetColour(
                            Wisteria::Colors::ColorContrast::IsLight(GetPlotOrCanvasColor()) ?
                                *wxBLACK :
                                *wxWHITE);
                        }
                    SetDefaultLegendShape(Icons::IconShape::WaterColorRectangle);
                    }
                // clip box to not be on top of axes
                else
                    {
                    box->SetClippingRect(drawArea);
                    SetDefaultLegendShape(Icons::IconShape::Square);
                    }
                // add the box to the plot item collection
                AddObject(std::move(box));
                }
            }
        // add the decal (if there is one)
        if (barBlock.IsShown() && barBlock.GetLength() > 0 &&
            !barBlock.GetDecal().GetText().empty())
            {
            const wxCoord leftPadding = ScaleToScreenAndCanvas(2);
            wxRect decalRect(barNeckRect);
            decalRect.Deflate(leftPadding, 0);

            auto decalLabel = std::make_unique<GraphItems::Label>(barBlock.GetDecal());
            decalLabel->GetGraphItemInfo()
                .Pen(wxNullPen)
                .Text(barBlock.ExpandDecalLabel())
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Padding(2, 2, 2, 2);
            decalLabel->GetFont().MakeSmaller().MakeSmaller();
            if (decalLabel->GetLabelFit() == LabelFit::ScaleFontToFit)
                {
                decalLabel->SetBoundingBox(decalRect, barRenderInfo.m_dc, GetScaling());
                decalLabel->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                decalLabel->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
                }
            else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFit)
                {
                decalLabel->SplitTextToFitBoundingBox(barRenderInfo.m_dc, decalRect.GetSize());
                }
            else if (decalLabel->GetLabelFit() == LabelFit::SplitTextToFitWidth)
                {
                decalLabel->SplitTextToFitBoundingBox(
                    barRenderInfo.m_dc,
                    wxSize(decalRect.GetWidth(), std::numeric_limits<int>::max()));
                }
            // if drawing as-is, then draw a box around the label
            // if it's larger than the parent block
            else if (decalLabel->GetLabelFit() == LabelFit::DisplayAsIsAutoFrame)
                {
                const auto actualDecalRect = decalLabel->GetBoundingBox(barRenderInfo.m_dc);
                if (actualDecalRect.GetWidth() - ScaleToScreenAndCanvas(1) > decalRect.GetWidth() ||
                    actualDecalRect.GetHeight() - ScaleToScreenAndCanvas(1) > decalRect.GetHeight())
                    {
                    decalLabel->GetGraphItemInfo().FontBackgroundColor(
                        Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                            decalLabel->GetFontColor()));
                    }
                }
            // make multiline decals a little more compact so that they
            // have a better chance of fitting
            decalLabel->SetLineSpacing(0);
            decalLabel->SetShadowType(ShadowType::NoDisplay);
            decalLabel->SetTextAlignment(TextAlignment::FlushLeft);
            decalLabel->SetTextOrientation(Orientation::Horizontal);
            decalLabel->SetAnchoring(Wisteria::Anchoring::BottomLeftCorner);
            // allow selecting the bar underneath this label
            decalLabel->SetSelectable(false);
            // if font is way too small, then show it as a label
            // overlapping the bar instead of a decal.
            if (decalLabel->GetLabelFit() != LabelFit::DisplayAsIs &&
                decalLabel->GetLabelFit() != LabelFit::DisplayAsIsAutoFrame &&
                decalLabel->GetFont().GetPointSize() < barRenderInfo.m_defaultFontPointSize / 2)
                {
                decalLabel->GetFont().SetPointSize(barRenderInfo.m_defaultFontPointSize);
                decalLabel->SetFontColor(*wxBLACK);
                decalLabel->GetPen().SetColour(*wxBLACK);
                decalLabel->SetFontBackgroundColor(*wxWHITE);
                }
            const wxRect labelBoundingBox = decalLabel->GetBoundingBox(barRenderInfo.m_dc);
            if (decalLabel->GetRelativeAlignment() == RelativeAlignment::FlushBottom)
                {
                decalLabel->SetAnchorPoint(
                    wxPoint((barNeckRect.GetLeft() +
                             safe_divide(barNeckRect.GetWidth() - labelBoundingBox.GetWidth(), 2)),
                            (barNeckRect.GetBottom() - leftPadding)));
                }
            else if (decalLabel->GetRelativeAlignment() == RelativeAlignment::Centered)
                {
                decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                decalLabel->SetAnchorPoint(wxPoint(
                    (barNeckRect.GetLeft() +
                     safe_divide(barNeckRect.GetWidth() - labelBoundingBox.GetWidth(), 2)),
                    (barNeckRect.GetTop() +
                     safe_divide(barNeckRect.GetHeight() - labelBoundingBox.GetHeight(), 2))));
                }
            else // flush top
                {
                decalLabel->SetAnchoring(Wisteria::Anchoring::TopLeftCorner);
                decalLabel->SetAnchorPoint(
                    wxPoint((barNeckRect.GetLeft() +
                             safe_divide(barNeckRect.GetWidth() - labelBoundingBox.GetWidth(), 2)),
                            (barNeckRect.GetTop() + leftPadding)));
                }
            // if drawing a color and hatch pattern, then show the decal with an outline
            // to make it easier to read
            if (bar.GetEffect() == BoxEffect::Solid && barBlock.GetColor().IsOk() &&
                barBlock.GetBrush().GetStyle() != wxBrushStyle::wxBRUSHSTYLE_SOLID)
                {
                decalLabel->GetPen().SetColour(*wxBLACK);
                decalLabel->SetFontColor(*wxBLACK);
                decalLabel->SetFontBackgroundColor(*wxWHITE);
                }
            barRenderInfo.m_decals.push_back(std::move(decalLabel));
            }

        return barBlockRenderInfo.m_middlePointOfBarEnd;
        }

    //-----------------------------------
    wxRect BarChart::GetDrawArea() const
        {
        wxRect drawArea{ GetPlotAreaBoundingBox() };
        if (GetLeftYAxis().GetPen().IsOk())
            {
            drawArea.SetWidth(drawArea.GetWidth() -
                              ScaleToScreenAndCanvas(GetLeftYAxis().GetPen().GetWidth()));
            drawArea.SetLeft(drawArea.GetLeft() +
                             ScaleToScreenAndCanvas(GetLeftYAxis().GetPen().GetWidth()));
            }
        if (GetBottomXAxis().GetPen().IsOk())
            {
            drawArea.SetHeight(drawArea.GetHeight() -
                               ScaleToScreenAndCanvas(GetBottomXAxis().GetPen().GetWidth()));
            drawArea.SetTop(drawArea.GetTop() +
                            ScaleToScreenAndCanvas(GetBottomXAxis().GetPen().GetWidth()));
            }
        return drawArea;
        }

    //-----------------------------------
    wxPoint BarChart::DrawBar(Bar & bar, const size_t barIndex, BarRenderInfo& barRenderInfo,
                              const bool measureOnly)
        {
        BarBlockRenderInfo barBlockRenderInfo{};

        for (const auto& barBlock : bar.GetBlocks())
            {
            if (GetBarOrientation() == Orientation::Horizontal)
                {
                DrawBarBlockHorizontal(bar, barIndex, barBlock, barRenderInfo, barBlockRenderInfo,
                                       measureOnly);
                }
            else
                {
                DrawBarBlockVertical(bar, barIndex, barBlock, barRenderInfo, barBlockRenderInfo,
                                     measureOnly);
                }
            }

        bar.GetLabel().SetScaling(GetScaling());
        bar.GetLabel().SetDPIScaleFactor(GetDPIScaleFactor());
        bar.GetLabel().SetShadowType(GetShadowType());
        bar.GetLabel().SetFontColor(GetBarAxis().GetFontColor());
        bar.GetLabel().SetFont(GetBarAxis().GetFont());

        // after all blocks are built, add the label at the end of the full bar
        if (GetBarOrientation() == Orientation::Horizontal && bar.GetLabel().IsShown())
            {
            auto bBox = bar.GetLabel().GetBoundingBox(barRenderInfo.m_dc);
            bar.GetLabel().SetAnchorPoint(wxPoint(barBlockRenderInfo.m_middlePointOfBarEnd.x +
                                                      barRenderInfo.m_labelSpacingFromLine +
                                                      (bBox.GetWidth() / 2),
                                                  barBlockRenderInfo.m_middlePointOfBarEnd.y));

            auto barLabel = std::make_unique<GraphItems::Label>(bar.GetLabel());
            bBox = barLabel->GetBoundingBox(barRenderInfo.m_dc);

            if (!Wisteria::GraphItems::Polygon::IsRectInsideRect(bBox, GetPlotAreaBoundingBox()))
                {
                barLabel->Offset((GetPlotAreaBoundingBox().GetRight() - bBox.GetRight()), 0);
                bBox.Offset((GetPlotAreaBoundingBox().GetRight() - bBox.GetRight()) +
                                ScaleToScreenAndCanvas(2),
                            0);
                if (barRenderInfo.m_barRect.Intersects(bBox))
                    {
                    barLabel->SetPadding(2, 2, 2, 2);
                    barLabel->GetPen() = *wxBLACK_PEN;
                    barLabel->SetFontBackgroundColor(
                        Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                            barLabel->GetFontColor()));
                    }
                }

            AddObject(std::move(barLabel));
            barBlockRenderInfo.m_middlePointOfBarEnd.x +=
                bBox.GetWidth() + (barRenderInfo.m_labelSpacingFromLine * 2);
            }
        else if (GetBarOrientation() == Orientation::Vertical && bar.GetLabel().IsShown())
            {
            auto bBox = bar.GetLabel().GetBoundingBox(barRenderInfo.m_dc);
            bar.GetLabel().SetAnchorPoint(
                wxPoint(barBlockRenderInfo.m_middlePointOfBarEnd.x,
                        barBlockRenderInfo.m_middlePointOfBarEnd.y -
                            (barRenderInfo.m_labelSpacingFromLine + (bBox.GetHeight() / 2))));

            auto barLabel = std::make_unique<GraphItems::Label>(bar.GetLabel());
            bBox = barLabel->GetBoundingBox(barRenderInfo.m_dc);

            if (!Wisteria::GraphItems::Polygon::IsRectInsideRect(bBox, GetPlotAreaBoundingBox()))
                {
                barLabel->Offset(0, (GetPlotAreaBoundingBox().GetTop() - bBox.GetTop()));
                bBox.Offset(0, (GetPlotAreaBoundingBox().GetTop() - bBox.GetTop()) -
                                   // wiggle room before adding outlining
                                   // that will stand out from the other labels
                                   ScaleToScreenAndCanvas(2));
                if (barRenderInfo.m_barRect.Intersects(bBox))
                    {
                    barLabel->SetPadding(2, 2, 2, 2);
                    barLabel->GetPen() = *wxBLACK_PEN;
                    barLabel->SetFontBackgroundColor(
                        Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                            barLabel->GetFontColor()));
                    }
                }

            AddObject(std::move(barLabel));
            barBlockRenderInfo.m_middlePointOfBarEnd.y -=
                bBox.GetHeight() + (barRenderInfo.m_labelSpacingFromLine * 2);
            }

        return barBlockRenderInfo.m_middlePointOfBarEnd;
        }

    //-----------------------------------
    void BarChart::DrawBarGroups(BarRenderInfo & barRenderInfo)
        {
        std::optional<wxCoord> maxBracketStartPos{ 0.0 };
        for (auto& barGroup : m_barGroups)
            {
            barGroup.m_maxBarPos.reset();
            const auto startPos =
                std::min(barGroup.m_barPositions.first, barGroup.m_barPositions.second);
            const auto endPos =
                std::max(barGroup.m_barPositions.first, barGroup.m_barPositions.second);

            for (size_t i = startPos; i <= endPos; ++i)
                {
                const wxPoint brackPos = barRenderInfo.m_barMiddleEndPositions[i];
                if (GetBarOrientation() == Orientation::Horizontal)
                    {
                    maxBracketStartPos =
                        std::max({ brackPos.x, maxBracketStartPos.value_or(brackPos.x) });
                    barGroup.m_maxBarPos =
                        std::max(brackPos.x, barGroup.m_maxBarPos.value_or(brackPos.x));
                    }
                else
                    {
                    maxBracketStartPos =
                        std::min({ brackPos.y, maxBracketStartPos.value_or(brackPos.y) });
                    barGroup.m_maxBarPos =
                        std::min(brackPos.y, barGroup.m_maxBarPos.value_or(brackPos.y));
                    }
                }
            }

        for (const auto& barGroup : m_barGroups)
            {
            wxPoint brackPos1 =
                barRenderInfo.m_barMiddleEndPositions[barGroup.m_barPositions.first];
            wxPoint brackPos2 =
                barRenderInfo.m_barMiddleEndPositions[barGroup.m_barPositions.second];
            double grandTotal{ 0 };
            // the bars specified in the group may be in different order, so use
            // min and max to make sure you are using the true start and end bars
            for (size_t i = std::min(barGroup.m_barPositions.first, barGroup.m_barPositions.second);
                 i <= std::max(barGroup.m_barPositions.first, barGroup.m_barPositions.second); ++i)
                {
                grandTotal += GetBars()[i].GetLength();
                }

            // if all bars in group are empty, then don't add the bar block for the groups
            if (grandTotal == 0)
                {
                continue;
                }

            double bracesWidth{ DownscaleFromScreenAndCanvas(
                GetScalingAxis().GetIntervalPhysicalLength()) };
            double scalingAxisPos{ 0 }, barAxisPos{ 0 };
            if (GetBarOrientation() == Orientation::Horizontal)
                {
                const wxCoord brackStartXPos =
                    std::max({ brackPos1.x, brackPos2.x,
                               (m_barGroupPlacement == LabelPlacement::Flush ?
                                    maxBracketStartPos.value_or(brackPos1.x) :
                                    barGroup.m_maxBarPos.value_or(brackPos1.x)) });
                if (GetScalingAxis().GetValueFromPhysicalCoordinate(
                        brackStartXPos + ScaleToScreenAndCanvas(bracesWidth), scalingAxisPos))
                    {
                    // make the curly braces stretch from the top of the first bar
                    // to the bottom of the last one
                    const auto yOffset =
                        (brackPos1.y < brackPos2.y) ?
                            safe_divide<double>(
                                GetBars()[barGroup.m_barPositions.first].GetCustomWidth().value_or(
                                    barRenderInfo.m_barWidth),
                                2) :
                            safe_divide<double>(
                                GetBars()[barGroup.m_barPositions.second].GetCustomWidth().value_or(
                                    barRenderInfo.m_barWidth),
                                2);
                    const auto barsWidth =
                        std::abs(brackPos1.y - brackPos2.y) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().value_or(
                                barRenderInfo.m_barWidth),
                            2) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().value_or(
                                barRenderInfo.m_barWidth),
                            2);

                    const auto yPos = std::min(brackPos1.y, brackPos2.y) +
                                      safe_divide<double>(std::abs(brackPos1.y - brackPos2.y), 2);
                    if (GetBarAxis().GetValueFromPhysicalCoordinate(yPos, barAxisPos))
                        {
                        Bar theBar(
                            grandTotal,
                            { BarBlock(
                                BarBlockInfo(grandTotal)
                                    .Brush(barGroup.m_barBrush)
                                    .Color(barGroup.m_barColor)
                                    .Decal(Wisteria::GraphItems::Label(
                                        Wisteria::GraphItems::GraphItemInfo(barGroup.m_barDecal)
                                            .LabelFitting(LabelFit::SplitTextToFit)
                                            .ChildAlignment(RelativeAlignment::Centered)
                                            .FontColor(Wisteria::Colors::ColorContrast::
                                                           BlackOrWhiteContrast(
                                                               barGroup.m_barBrush.IsOk() ?
                                                                   barGroup.m_barBrush.GetColour() :
                                                                   barGroup.m_barColor))))) },
                            wxString{}, Wisteria::GraphItems::Label{}, GetBarEffect(),
                            GetBarOpacity());
                        UpdateBarLabel(theBar);
                        // if the superbar is going outside of the plot area, then
                        // shove it back over and adjust the width of the bracket as well
                        if ((scalingAxisPos + grandTotal) > GetScalingAxis().GetRange().second)
                            {
                            scalingAxisPos -= ((scalingAxisPos + grandTotal) -
                                               GetScalingAxis().GetRange().second);
                            wxCoord newBracketRight{ 0 };
                            if (GetScalingAxis().GetPhysicalCoordinate(scalingAxisPos,
                                                                       newBracketRight))
                                {
                                bracesWidth = newBracketRight - brackStartXPos;
                                }
                            }
                        theBar.SetCustomScalingAxisStartPosition(scalingAxisPos);
                        theBar.SetAxisPosition(barAxisPos);

                        AddObject(std::make_unique<Wisteria::GraphItems::Shape>(
                            Wisteria::GraphItems::GraphItemInfo()
                                .Pen(wxPen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                               GetPlotOrCanvasColor()),
                                           2))
                                .Scaling(GetScaling())
                                .DPIScaling(GetDPIScaleFactor())
                                .AnchorPoint(wxPoint(brackStartXPos,
                                                     std::min(brackPos1.y, brackPos2.y) - yOffset))
                                .Anchoring(Anchoring::TopLeftCorner),
                            Icons::IconShape::RightCurlyBrace,
                            wxSize(bracesWidth, DownscaleFromScreenAndCanvas(barsWidth)), nullptr));
                        DrawBar(theBar, 0, barRenderInfo);
                        // cppcheck-suppress knownEmptyContainer
                        for (auto& decal : barRenderInfo.m_decals)
                            {
                            AddObject(std::move(decal));
                            }
                        }
                    }
                }
            else
                {
                const wxCoord brackStartYPos =
                    std::min({ brackPos1.y, brackPos2.y,
                               (m_barGroupPlacement == LabelPlacement::Flush ?
                                    maxBracketStartPos.value_or(brackPos1.y) :
                                    barGroup.m_maxBarPos.value_or(brackPos1.y)) });
                if (GetScalingAxis().GetValueFromPhysicalCoordinate(
                        brackStartYPos -
                            // space for the braces and a couple DIPs between that and the group bar
                            ScaleToScreenAndCanvas(bracesWidth + 2),
                        scalingAxisPos))
                    {
                    // make the curly braces stretch from the top of the first bar
                    // to the bottom of the last one
                    const auto xOffset =
                        (brackPos1.x < brackPos2.x) ?
                            safe_divide<double>(
                                GetBars()[barGroup.m_barPositions.first].GetCustomWidth().value_or(
                                    barRenderInfo.m_barWidth),
                                2) :
                            safe_divide<double>(
                                GetBars()[barGroup.m_barPositions.second].GetCustomWidth().value_or(
                                    barRenderInfo.m_barWidth),
                                2);
                    const auto barsWidth =
                        std::abs(brackPos1.x - brackPos2.x) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.first].GetCustomWidth().value_or(
                                barRenderInfo.m_barWidth),
                            2) +
                        safe_divide<double>(
                            GetBars()[barGroup.m_barPositions.second].GetCustomWidth().value_or(
                                barRenderInfo.m_barWidth),
                            2);

                    const auto xPos = std::min(brackPos1.x, brackPos2.x) +
                                      safe_divide<double>(std::abs(brackPos1.x - brackPos2.x), 2);
                    if (GetBarAxis().GetValueFromPhysicalCoordinate(xPos, barAxisPos))
                        {
                        Bar theBar(
                            grandTotal,
                            { BarBlock(
                                BarBlockInfo(grandTotal)
                                    .Brush(barGroup.m_barBrush)
                                    .Color(barGroup.m_barColor)
                                    .Decal(Wisteria::GraphItems::Label(
                                        Wisteria::GraphItems::GraphItemInfo(barGroup.m_barDecal)
                                            .LabelFitting(LabelFit::SplitTextToFit)
                                            .ChildAlignment(RelativeAlignment::Centered)
                                            .FontColor(Wisteria::Colors::ColorContrast::
                                                           BlackOrWhiteContrast(
                                                               barGroup.m_barBrush.IsOk() ?
                                                                   barGroup.m_barBrush.GetColour() :
                                                                   barGroup.m_barColor))))) },
                            wxString{}, Wisteria::GraphItems::Label{}, GetBarEffect(),
                            GetBarOpacity());
                        UpdateBarLabel(theBar);
                        // if the superbar is going outside of the plot area, then
                        // shove it back down and adjust the height of the bracket as well
                        if ((scalingAxisPos + grandTotal) > GetScalingAxis().GetRange().second)
                            {
                            scalingAxisPos -= ((scalingAxisPos + grandTotal) -
                                               GetScalingAxis().GetRange().second);
                            wxCoord newBracketTop{ 0 };
                            if (GetScalingAxis().GetPhysicalCoordinate(scalingAxisPos,
                                                                       newBracketTop))
                                {
                                bracesWidth = brackStartYPos - newBracketTop;
                                }
                            }
                        theBar.SetCustomScalingAxisStartPosition(scalingAxisPos);
                        theBar.SetAxisPosition(barAxisPos);

                        AddObject(std::make_unique<Wisteria::GraphItems::Shape>(
                            Wisteria::GraphItems::GraphItemInfo()
                                .Pen(wxPen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                               GetPlotOrCanvasColor()),
                                           2))
                                .Scaling(GetScaling())
                                .DPIScaling(GetDPIScaleFactor())
                                .AnchorPoint(
                                    wxPoint(std::min(brackPos1.x, brackPos2.x) - xOffset,
                                            brackStartYPos - ScaleToScreenAndCanvas(bracesWidth)))
                                .Anchoring(Anchoring::TopLeftCorner),
                            Icons::IconShape::TopCurlyBrace,
                            wxSize(DownscaleFromScreenAndCanvas(barsWidth), bracesWidth), nullptr));
                        DrawBar(theBar, 0, barRenderInfo);
                        // cppcheck-suppress knownEmptyContainer
                        for (auto& decal : barRenderInfo.m_decals)
                            {
                            AddObject(std::move(decal));
                            }
                        }
                    }
                }
            }
        }

    //-----------------------------------
    void BarChart::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        // if no bars then just draw a blank 10x10 grid
        if (GetBars().empty())
            {
            GetRightYAxis().Reset();
            GetBarAxis().Reset();
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            GetTopXAxis().Reset();
            GetScalingAxis().Reset();
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        BarRenderInfo barRenderInfo{ dc };

        barRenderInfo.m_barSpacing = m_includeSpacesBetweenBars ? ScaleToScreenAndCanvas(10) : 0;
        barRenderInfo.m_scaledShadowOffset = ScaleToScreenAndCanvas(GetShadowOffset());
        barRenderInfo.m_labelSpacingFromLine = ScaleToScreenAndCanvas(5);
        barRenderInfo.m_defaultFontPointSize =
            wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize();
        // scale the common image to the plot area's size
        barRenderInfo.m_scaledCommonImg =
            GetCommonBoxImage().IsOk() ? Wisteria::GraphItems::Image::CropImageToRect(
                                             GetCommonBoxImage()
                                                 .GetBitmap(GetCommonBoxImage().GetDefaultSize())
                                                 .ConvertToImage(),
                                             GetPlotAreaBoundingBox().GetSize(), true) :
                                         wxNullImage;

        // draw the bars
        barRenderInfo.m_barMiddleEndPositions.reserve(GetBars().size());
        for (size_t i = 0; i < GetBars().size(); ++i)
            {
            // keep track of where each bar ends
            barRenderInfo.m_barMiddleEndPositions.push_back(
                DrawBar(GetBars()[i], i, barRenderInfo));
            }

        // draw the decals on top of the blocks
        for (auto& decal : barRenderInfo.m_decals)
            {
            AddObject(std::move(decal));
            }
        barRenderInfo.m_decals.clear();

        // add the brackets and bars for any bar groups
        DrawBarGroups(barRenderInfo);
        }
    }
