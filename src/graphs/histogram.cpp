///////////////////////////////////////////////////////////////////////////////
// Name:        histogram.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "histogram.h"
#include "../util/frequencymap.h"
#include "../math/statistics.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::Histogram, Wisteria::Graphs::BarChart)

using namespace Wisteria::GraphItems;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void Histogram::SetData(std::shared_ptr<const Data::Dataset> data,
                            const wxString& continuousColumnName,
                            const std::optional<const wxString> groupColumnName /*= std::nullopt*/,
                            const BinningMethod bMethod /*= BinningMethod::BinByIntegerRange*/,
                            const RoundingMethod rounding /*= RoundingMethod::NoRounding*/,
                            const IntervalDisplay iDisplay /*= IntervalDisplay::Cutpoints*/,
                            const BinLabelDisplay blDisplay /*= BinLabelDisplay::BinValue*/,
                            const bool showFullRangeOfValues /*= true*/,
                            const std::optional<double> startBinsValue /*= std::nullopt*/,
                            const std::pair<std::optional<size_t>, std::optional<size_t>> binCountRanges
                                /*= std::make_pair(std::nullopt, std::nullopt)*/,
                            const bool neatIntervals /*= false*/)
        {
        // point to (new) data and reset
        SetDataset(data);

        ResetGrouping();
        GetSelectedIds().clear();
        ClearBars();
        ClearBarGroups();

        if (GetDataset() == nullptr)
            { return; }

        m_neatRanges = neatIntervals;
        m_binningMethod = bMethod;
        m_roundingMethod = rounding;
        m_intervalDisplay = iDisplay;
        SetBinLabelDisplay(blDisplay);
        m_displayFullRangeOfValues = showFullRangeOfValues;
        m_startBinsValue = startBinsValue;

        if (binCountRanges.second)
            { m_maxBinCount = std::min(binCountRanges.second.value(), m_maxBinCount); }

        // set the grouping column (or keep it as null if not in use)
        SetGroupColumn(groupColumnName);

        m_continuousColumn = GetDataset()->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for histogram."),
                continuousColumnName).ToUTF8());
            }

        m_validN = statistics::valid_n(m_continuousColumn->GetValues());

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            { BuildGroupIdMap(); }

        // if no data then just draw a blank 10x10 grid
        if (m_validN == 0)
            {
            GetScalingAxis().SetRange(0, 10, 0, 1, 1);
            GetBarAxis().SetRange(0, 10, 0, 1, 1);
            return;
            }

        // if 4 or less unique values, might as well use unique values instead of ranges
        if (CalcUniqueValuesCount() <= 4)
            { SetBinningMethod(BinningMethod::BinUniqueValues); }

        if (GetBinningMethod() == BinningMethod::BinUniqueValues)
            { SortIntoUniqueValues(binCountRanges.first); }
        else
            { SortIntoRanges(binCountRanges.first); }

        // re-build the bar labels now that the bars are ready
        SetBinLabelDisplay(blDisplay);

        GetBarAxis().ShowOuterLabels(false);

        // set axis labels
        GetBarAxis().GetTitle().SetText(m_continuousColumn->GetName());
        GetScalingAxis().GetTitle().SetText(_(L"Frequency"));
        }

    //----------------------------------------------------------------
    wxString Histogram::GetCustomBarLabelOrValue(const double& value,
                                                 const size_t precision /*= 0*/)
        {
        const auto& customLabel = GetBarAxis().GetCustomLabel(value);
        if (customLabel.IsOk() && customLabel.GetText().length())
            { return customLabel.GetText(); }
        else
            {
            return wxNumberFormatter::ToString(value, precision,
                Settings::GetDefaultNumberFormat());
            }
        }

    //----------------------------------------------------------------
    size_t Histogram::CalcUniqueValuesCount() const
        {
        if (GetDataset() == nullptr)
            { return 0; }

        frequency_set<double> groups;
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (!std::isnan(m_continuousColumn->GetValue(i)))
                {
                groups.insert(ConvertToSortableValue(m_continuousColumn->GetValue(i)));
                }
            }
        return groups.get_data().size();
        }

    //----------------------------------------------------------------
    void Histogram::SortIntoUniqueValues(const std::optional<size_t> binCount)
        {
        if (GetDataset() == nullptr)
            { return; }

        // calculate how many observations are in each group
        multi_value_aggregate_map<BinBlock, wxString, std::less<BinBlock>, Data::wxStringLessNoCase> groups;
        groups.set_values_list_max_size(Settings::GetMaxObservationInBin());
        bool hasFloatingPointValue{ false };

        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_continuousColumn->GetValue(i)))
                { continue; }

            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex = IsUsingGrouping() ?
                GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) :
                0;

            groups.insert(BinBlock{
                ConvertToSortableValue(m_continuousColumn->GetValue(i)),
                (IsUsingGrouping() ?
                    GetGroupColumn()->GetValue(i) : static_cast<Data::GroupIdType>(0)),
                colorIndex,
                (IsUsingGrouping() ?
                    GetGroupColumn()->GetLabelFromID(GetGroupColumn()->GetValue(i)) :
                    wxString()) },
                GetDataset()->GetIdColumn().GetValue(i).wc_str() );
            if ((GetRoundingMethod() == RoundingMethod::NoRounding) &&
                has_fractional_part(m_continuousColumn->GetValue(i)))
                {
                GetBarAxis().SetPrecision(4);
                hasFloatingPointValue = true;
                }
            }
        // if there are going to be too many bars, then switch to range mode
        if (groups.get_data().size() > GetMaxNumberOfBins())
            {
            if (!hasFloatingPointValue)
                { SetBinningMethod(BinningMethod::BinByIntegerRange); }
            SortIntoRanges(binCount);
            return;
            }

        // with (floating point) unique values, we shouldn't distribute the bars evenly
        // (there would be huge amount of bin areas), so we will need to just show the
        // bars and their categories as custom labels.
        if (GetRoundingMethod() == RoundingMethod::NoRounding && hasFloatingPointValue)
            { ShowFullRangeOfValues(false); }
        else
            { GetBarAxis().SetPrecision(0); }

        // add an empty bar at position 1 if there isn't one there already
        // and caller wants the axis to start at a specific point
        if (GetBinsStart() &&
            !std::isnan(GetBinsStart().value()) &&
            IsShowingFullRangeOfValues() &&
            groups.get_data().find(BinBlock{ GetBinsStart().value(), 0, 0, wxEmptyString }) == groups.get_data().end())
            {
            Bar theBar(GetBinsStart().value(),
                { BarBlock(BarBlockInfo().Brush(GetBrushScheme()->GetBrush(0)).
                    Color(GetColorScheme() ? GetColorScheme()->GetColor(0) : wxTransparentColour)) },
                       wxString{},
                GraphItems::Label(wxEmptyString), GetBarEffect(), GetBarOpacity());
            AddBar(theBar);
            }
        // add the bars (block-by-block)
        size_t barNumber{ 1 };
        for (const auto& blockTable : groups.get_data())
            {
            const wxColour blockColor = GetColorScheme() ?
                (IsUsingGrouping() ?
                    GetColorScheme()->GetColor(blockTable.first.m_schemeIndex) : GetColorScheme()->GetColor(0)) :
                wxTransparentColour;
            const wxBrush blockBrush = (IsUsingGrouping() ?
                GetBrushScheme()->GetBrush(blockTable.first.m_schemeIndex) : GetBrushScheme()->GetBrush(0));

            wxString blockLabel{ wxString::Format(_(L"%s item(s)\n"),
                wxNumberFormatter::ToString(blockTable.second.second, 0,
                    Settings::GetDefaultNumberFormat())) };
            // piece the first few observations together as a display label for the block
            for (const auto& obsLabel : blockTable.second.first)
                { blockLabel.append(obsLabel).append(L"\n"); }
            blockLabel.Trim();
            // if observations are added to the selection label, but not all of them, then add ellipsis
            if (blockTable.second.first.size() < blockTable.second.second &&
                blockTable.second.first.size() > 1)
                { blockLabel += L"..."; }

            auto foundBar = std::find_if(GetBars().begin(), GetBars().end(),
                [&blockTable](const auto& bar)
                    { return compare_doubles(bar.GetAxisPosition(), blockTable.first.m_bin); });
            if (foundBar == GetBars().end())
                {
                Bar theBar(IsShowingFullRangeOfValues() ? blockTable.first.m_bin : barNumber,
                    {
                    BarBlock(BarBlockInfo(blockTable.second.second).
                        Tag(blockTable.first.m_groupName).
                        Brush(blockBrush).Color(blockColor).
                        SelectionLabel(GraphItems::Label(blockLabel)))
                    },
                    wxEmptyString,
                    IsShowingFullRangeOfValues() ? GraphItems::Label(wxEmptyString) :
                        GraphItems::Label(wxNumberFormatter::ToString(blockTable.first.m_bin,
                            (has_fractional_part(blockTable.first.m_bin)) ? 2 : 0,
                            Settings::GetDefaultNumberFormat())),
                    GetBarEffect(), GetBarOpacity());
                // if observations added to the selection label, then show it as a report
                if (blockTable.second.first.size() > 1)
                    {
                    theBar.GetBlocks()[0].GetSelectionLabel().
                        GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::Centered);
                    theBar.GetBlocks()[0].GetSelectionLabel().
                        SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    }
                AddBar(theBar);
                }
            else
                {
                BarBlock block{
                    BarBlock(BarBlockInfo(blockTable.second.second).
                        Tag(blockTable.first.m_groupName).
                        Brush(blockBrush).Color(blockColor).
                        SelectionLabel(GraphItems::Label(blockLabel)))
                    };
                if (blockTable.second.first.size() > 1)
                    {
                    block.GetSelectionLabel().SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    block.GetSelectionLabel().GetHeaderInfo().Enable(true).LabelAlignment(TextAlignment::Centered);
                    }
                foundBar->AddBlock(block);
                UpdateScalingAxisFromBar(*foundBar);
                }
            if (!IsShowingFullRangeOfValues())
                {
                GetBarAxis().SetCustomLabel(barNumber++,
                    GraphItems::Label(wxNumberFormatter::ToString(blockTable.first.m_bin,
                        (has_fractional_part(blockTable.first.m_bin)) ? 2 : 0,
                        Settings::GetDefaultNumberFormat())));
                }
            }
        // add the bar labels now that they are built
        for (auto& bar : GetBars())
            { UpdateBarLabel(bar); }
        m_binCount = groups.get_data().size();
        }

    //----------------------------------------------------------------
    void Histogram::SortIntoRanges(const std::optional<size_t> binCount)
        {
        if (GetDataset() == nullptr || m_validN == 0)
            {
            return;
            }
        std::vector<double> validData;
        validData.reserve(GetDataset()->GetRowCount());
        std::copy_if(m_continuousColumn->GetValues().cbegin(),
                     m_continuousColumn->GetValues().cend(), std::back_inserter(validData),
                     [](auto x) { return std::isfinite(x); });
        double minVal = *std::min_element(validData.cbegin(), validData.cend());
        double maxVal = *std::max_element(validData.cbegin(), validData.cend());
        // If data fails into a small range (e.g., < 2), then forcibly turn off rounding and integer binning.
        // Make sure that the range is larger than 0 though (otherwise there will probably just be one bin
        // and integer mode would be better there).
        if ((maxVal - minVal) < 2 && (maxVal - minVal) > 0)
            {
            SetBinningMethod(BinningMethod::BinByRange);
            SetRoundingMethod(RoundingMethod::NoRounding);
            }

        /* if we are creating integer categories, then we need to adjust the range and number of groups to fit an
           even distribution.*/
        const bool isLowestValueBeingAdjusted =
            (std::floor(static_cast<double>(minVal)) == ConvertToSortableValue(minVal)) &&
            !compare_doubles(ConvertToSortableValue(minVal), 0) &&
            (!GetBinsStart() || minVal < GetBinsStart().value());

        if (GetBinsStart() && !std::isnan(GetBinsStart().value()))
            { minVal = std::min(minVal, GetBinsStart().value()); }
        const size_t numOfBins = binCount.has_value() ? binCount.value() :
                                 std::min(CalcNumberOfBins(), GetMaxNumberOfBins());
        if (GetBinningMethod() == BinningMethod::BinByIntegerRange)
            {
            minVal = std::floor(static_cast<double>(minVal));
            /* If in integer mode (with rounding) and the lowest value is rounded down,
               then move the min value of the range to one integer less.
               This will create an extra bin for the low value. Normally, adding the lowest value
               to the first bin (without making an extra new bin for it is OK), but for integer mode
               where rounding is being used, this could make the first bin much bigger than the
               other and cause imbalance. That is to say, if the low value is 1 and a bin of 0-1
               should be created for it because there will probably be other 1 values
               (due to the rounding).
               Throwing the 1s into a 1-2 bin would make this bin much bigger than the others.*/
            if (GetRoundingMethod() != RoundingMethod::NoRounding &&
                isLowestValueBeingAdjusted)
                { minVal -= 1; }
            maxVal = std::ceil(static_cast<double>(maxVal));

            // if starting at forced position, then only pad beyond max value if creating integral intervals
            if (GetBinsStart() && !std::isnan(GetBinsStart().value()))
                {
                while (safe_modulus<size_t>(static_cast<size_t>(maxVal-minVal), numOfBins))
                    { ++maxVal; }
                }
            else
                {
                // if we are splitting the bins into integral integer ranges,
                // then we need to adjust (pad) the min and max values so
                // that the range is evenly divisible by the number of bins.
                bool addHigh = true;
                while (safe_modulus<size_t>(static_cast<size_t>(maxVal-minVal), numOfBins))
                    {
                    addHigh ? ++maxVal : --minVal;
                    addHigh = !addHigh;
                    }
                }
            }
        double binSize = safe_divide<double>((maxVal-minVal), numOfBins);
        if (GetBinningMethod() == BinningMethod::BinByIntegerRange)
            {
            assert(!has_fractional_part(binSize));
            // fit the ranges into neater intervals and unused bins will be discarded later
            if (m_neatRanges)
                {
                if (binSize < 5)
                    { binSize = 5; }
                else if (binSize < 100)
                    { binSize = next_interval(binSize, 2); }
                else if (binSize < 1000)
                    { binSize = next_interval(binSize, 3); }
                }
            }

        // calculate how many observations are in each group
        using valuesCounter = std::pair<double, std::set<wxString, Data::wxStringLessNoCase>>;
        std::vector<std::vector<comparable_first_pair<Data::GroupIdType, valuesCounter>>> bins(numOfBins);
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_continuousColumn->GetValue(i)))
                { continue; }

            for (size_t j = 0; j < bins.size(); ++j)
                {
                const double currentVal = ConvertToSortableValue(m_continuousColumn->GetValue(i));
                /* Logic is a little different for the first bin. The low value in the data needs to
                   go into this bin, even if it is actually less than the bin's range (right on the edge).
                   This prevents us from making an extra bin just for this one value.*/
                if (compare_doubles(currentVal, minVal))
                    {
                    auto foundGroup = std::find(bins[0].begin(), bins[0].end(),
                        comparable_first_pair((IsUsingGrouping() ? GetGroupColumn()->GetValue(i) : 0),
                            valuesCounter(0.0, std::set<wxString, Data::wxStringLessNoCase>{})));
                    if (foundGroup != bins[0].end())
                        {
                        ++foundGroup->second.first;
                        if (foundGroup->second.second.size() < Settings::GetMaxObservationInBin())
                            { foundGroup->second.second.emplace(GetDataset()->GetIdColumn().GetValue(i)); }
                        }
                    else
                        {
                        std::set<wxString, Data::wxStringLessNoCase> theSet;
                        theSet.emplace(GetDataset()->GetIdColumn().GetValue(i).c_str());
                        bins[0].push_back(
                            comparable_first_pair((IsUsingGrouping() ? GetGroupColumn()->GetValue(i) : 0),
                                valuesCounter(1.0, theSet)));
                        }
                    break;
                    }
                else if (compare_doubles_greater(currentVal, (minVal+(j*binSize))) &&
                    compare_doubles_less_or_equal(currentVal, (minVal+(j*binSize)+binSize)))
                    {
                    auto foundGroup = std::find(bins[j].begin(), bins[j].end(),
                        comparable_first_pair((IsUsingGrouping() ? GetGroupColumn()->GetValue(i) : 0),
                            valuesCounter(0.0, std::set<wxString, Data::wxStringLessNoCase>{})));
                    if (foundGroup != bins[j].end())
                        {
                        ++foundGroup->second.first;
                        if (foundGroup->second.second.size() < Settings::GetMaxObservationInBin())
                            { foundGroup->second.second.emplace(GetDataset()->GetIdColumn().GetValue(i)); }
                        }
                    else
                        {
                        std::set<wxString, Data::wxStringLessNoCase> theSet;
                        theSet.emplace(GetDataset()->GetIdColumn().GetValue(i).c_str());
                        bins[j].push_back(
                            comparable_first_pair((IsUsingGrouping() ?
                                GetGroupColumn()->GetValue(i) :
                                static_cast<Data::GroupIdType>(0)), valuesCounter(1, theSet)));
                        }
                    break;
                    }
                }
            }

        const double startingBarAxisPosition = minVal + safe_divide<double>(binSize,2);
        // if the starting point or interval size has floating precision then set the axis to show it
        if (GetBinningMethod() != BinningMethod::BinByIntegerRange &&
            (has_fractional_part(startingBarAxisPosition) || has_fractional_part(binSize)))
            { GetBarAxis().SetPrecision(4); }
        else
            { GetBarAxis().SetPrecision(0); }
        GetBarAxis().SetInterval(binSize);

        // tally up the total group counts
        double total = 0;
        for (size_t i = 0; i < bins.size(); ++i)
            {
            for (const auto& blocks : bins[i])
                { total += blocks.second.first; }
           }

        /* Remove any following bins that do not have anything in them (might happen if the range
           had to be expanded to create integral intervals). Leading bins are handled separately in the
           loop below because the range min value makes removing bins here more tricky.*/
        while (bins.size())
            {
            if (bins.back().size() > 0)
                { break; }
            else
                { bins.erase(bins.end()-1); }
            }

        // add the bars
        bool firstBinWithValuesFound = false;
        for (size_t i = 0; i < bins.size(); ++i)
            {
            Bar theBar(startingBarAxisPosition + (i*binSize),
                std::vector<BarBlock>(),
                wxEmptyString, GraphItems::Label(),
                GetBarEffect(), GetBarOpacity(),
                (GetIntervalDisplay() == IntervalDisplay::Cutpoints) ? binSize : 0);

            // build the bar from its blocks (i.e., subgroups)
            double currentBarBlocksTotal{ 0 };
            for (const auto& block : bins[i])
                {
                // Convert group ID into color scheme index
                // (index is ordered by labels alphabetically).
                // Note that this will be zero if grouping is not in use.
                const size_t colorIndex = IsUsingGrouping() ?
                    GetSchemeIndexFromGroupId(block.first) :
                    0;

                const wxColour blockColor = GetColorScheme() ?
                    (IsUsingGrouping() ?
                        GetColorScheme()->GetColor(colorIndex) : GetColorScheme()->GetColor(0)) :
                    wxTransparentColour;
                const wxBrush blockBrush = (IsUsingGrouping() ?
                    GetBrushScheme()->GetBrush(colorIndex) : GetBrushScheme()->GetBrush(0));

                currentBarBlocksTotal += block.second.first;

                wxString blockLabel{ wxString::Format(_(L"%s item(s)\n"),
                    wxNumberFormatter::ToString(block.second.first, 0,
                        Settings::GetDefaultNumberFormat())) };
                for (const auto& obsLabel : block.second.second)
                    { blockLabel.append(obsLabel).append(L"\n"); }
                blockLabel.Trim();
                if (block.second.second.size() < block.second.first &&
                    block.second.second.size() > 1)
                    { blockLabel += L"..."; }

                const wxString blockTag = (IsUsingGrouping() ?
                    GetGroupColumn()->GetLabelFromID(block.first) :
                    wxString());

                BarBlock theBlock
                    {
                    BarBlock(BarBlockInfo(block.second.first).
                        Tag(blockTag).
                        Brush(blockBrush).Color(blockColor).
                        SelectionLabel(GraphItems::Label(blockLabel)))
                    };
                if (block.second.second.size() > 1)
                    {
                    theBlock.GetSelectionLabel().SetLabelStyle(LabelStyle::DottedLinedPaperWithMargins);
                    theBlock.GetSelectionLabel().GetHeaderInfo().
                        Enable(true).LabelAlignment(TextAlignment::Centered);
                    }

                theBar.AddBlock(theBlock);
                }

            const double barValue = currentBarBlocksTotal;
            const double percentage = safe_divide<double>(barValue, total) * 100;
            const wxString barLabel =
                (barValue == 0 ||
                 GetBinLabelDisplay() == BinLabelDisplay::NoDisplay ||
                 // for ranges, there is no concept of group name
                 GetBinLabelDisplay() == BinLabelDisplay::BinName) ?
                    wxString{} :
                (GetBinLabelDisplay() == BinLabelDisplay::BinValue) ?
                    wxNumberFormatter::ToString(barValue, 0, Settings::GetDefaultNumberFormat()) :
                (GetBinLabelDisplay() == BinLabelDisplay::BinPercentage) ?
                    wxNumberFormatter::ToString(percentage, 0,
                                             wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%" :
                    wxNumberFormatter::ToString(barValue, 0,
                    Settings::GetDefaultNumberFormat()) +
                    L" (" +
                    wxNumberFormatter::ToString(percentage, 0,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes) +
                    L"%)";
            theBar.GetLabel().SetText(barLabel);

            /* If values are being rounded and the intervals are integral, then show the bins
               simply as integer ranges instead of ">= and <" ranges (makes it easier to read).*/
            if (GetBinningMethod() == BinningMethod::BinByIntegerRange &&
                GetRoundingMethod() != RoundingMethod::NoRounding)
                {
                /* The first bin gets an extra value (the lowest value) so that an extra bin
                   doesn't need to be created just for that (unless rounding is turned on and
                   the lowest value is rounded down). If we are rounding down the lowest value,
                   then we need to step the starting point of the range by 1 because that is
                   where the first value really falls.
                   The rest of the bins need to show that they actually begin from the integer
                   starting after the cutpoint.*/
                const double startValue = (i == 0) ?
                    isLowestValueBeingAdjusted ? minVal+1 : minVal :
                        minVal+1+(i*binSize);
                const double endValue = minVal+(i*binSize)+binSize;
                wxString axisLabel;
                if (startValue == endValue)
                    { axisLabel += GetCustomBarLabelOrValue(startValue); }
                else
                    {
                    axisLabel += GetCustomBarLabelOrValue(startValue) + L"-" +
                        GetCustomBarLabelOrValue(endValue);
                    }
                if (GetIntervalDisplay() == IntervalDisplay::Midpoints)
                    {
                    GetBarAxis().SetCustomLabel(startingBarAxisPosition+(i*binSize),
                                                GraphItems::Label(axisLabel));
                    }
                }
            else
                {
                const wxString axisLabel = ((i == 0) ? L">= " : L"> ") +
                    wxNumberFormatter::ToString(minVal+(i*binSize), 6,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes) + _(L" and <= ") +
                    wxNumberFormatter::ToString(minVal+(i*binSize)+binSize, 6,
                        wxNumberFormatter::Style::Style_NoTrailingZeroes);
                if (GetIntervalDisplay() == IntervalDisplay::Midpoints)
                    {
                    GetBarAxis().SetCustomLabel(startingBarAxisPosition+(i*binSize),
                                                GraphItems::Label(axisLabel));
                    }
                }

            /* Remove any leading bins that do not have anything in them
               (might happen if the range had to be expanded to create integral intervals).*/
            if (!firstBinWithValuesFound && barValue <= 0 &&
                // if bins are not forced to start at a certain place,
                // then allow these leading empty bars
                (!GetBinsStart() || theBar.GetAxisPosition() < GetBinsStart().value()))
                { continue; }
            else
                { firstBinWithValuesFound = true; }

            AddBar(theBar);
            }

        m_binCount = bins.size();
        }

    //----------------------------------------------------------------
    size_t Histogram::CalcNumberOfBins() const
        {
        if (GetDataset() == nullptr)
            { return 0; }

        if (m_validN <= 1)
            { return 1; }
        // Sturges
        else if (m_validN < 200)
            {
            return std::ceil(std::log2(m_validN)) + 1;
            }
        // Scott
        else
            {
            std::vector<double> validData;
            validData.reserve(GetDataset()->GetRowCount());
            std::copy_if(m_continuousColumn->GetValues().cbegin(),
                         m_continuousColumn->GetValues().cend(), std::back_inserter(validData),
                         [](auto x) { return std::isfinite(x); });
            const auto minVal = *std::min_element(validData.cbegin(), validData.cend());
            const auto maxVal = *std::max_element(validData.cbegin(), validData.cend());
            const auto sd = statistics::standard_deviation(validData, true);
            return safe_divide(maxVal - minVal, 3.5 * safe_divide(sd, std::cbrt(m_validN)));
            }
        }

    //----------------------------------------------------------------
    double Histogram::ConvertToSortableValue(const double& value) const
        {
        if (GetRoundingMethod() == RoundingMethod::NoRounding)
            { return value; }
        else if (GetRoundingMethod() == RoundingMethod::Round)
            { return round_to_integer(value); }
        else if (GetRoundingMethod() == RoundingMethod::RoundDown)
            { return std::floor(static_cast<double>(value)); }
        else if (GetRoundingMethod() == RoundingMethod::RoundUp)
            { return std::ceil(static_cast<double>(value)); }
        else
            { return value; }
        }
    }
