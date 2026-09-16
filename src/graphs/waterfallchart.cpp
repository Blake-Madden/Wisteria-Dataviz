///////////////////////////////////////////////////////////////////////////////
// Name:        waterfallchart.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "waterfallchart.h"
#include <cmath>
#include <set>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::WaterfallChart, Wisteria::Graphs::BarChart)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    WaterfallChart::WaterfallChart(Wisteria::Canvas * canvas) : BarChart(canvas)
        {
        m_increaseBrush = wxBrush{ Colors::ColorBrewer::GetColor(Colors::Color::Emerald) };
        m_increasePen = DeriveOutlinePen(Colors::ColorBrewer::GetColor(Colors::Color::Emerald));
        m_decreaseBrush = wxBrush{ Colors::ColorBrewer::GetColor(Colors::Color::Tangerine) };
        m_decreasePen = DeriveOutlinePen(Colors::ColorBrewer::GetColor(Colors::Color::Tangerine));
        m_totalBrush = wxBrush{ Colors::ColorBrewer::GetColor(Colors::Color::BabyBlue) };
        m_totalPen = DeriveOutlinePen(Colors::ColorBrewer::GetColor(Colors::Color::BabyBlue));
        // bar labels are managed by this class (signed values), so keep the
        // base class from overwriting them
        SetBinLabelDisplay(BinLabelDisplay::NoDisplay);
        ApplyAxisAppearance();
        }

    //----------------------------------------------------------------
    void WaterfallChart::ApplyAxisAppearance()
        {
        GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
        GetBarAxis().ShowOuterLabels(false);
        // steps read left-to-right when vertical, top-to-bottom when horizontal
        GetBarAxis().Reverse(GetBarOrientation() == Orientation::Horizontal);
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetIncreaseColor(const wxColour& color)
        {
        m_increaseBrush = wxBrush{ color };
        m_increasePen = DeriveOutlinePen(color);
        RebuildBars();
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetDecreaseColor(const wxColour& color)
        {
        m_decreaseBrush = wxBrush{ color };
        m_decreasePen = DeriveOutlinePen(color);
        RebuildBars();
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetTotalColor(const wxColour& color)
        {
        m_totalBrush = wxBrush{ color };
        m_totalPen = DeriveOutlinePen(color);
        RebuildBars();
        }

    //----------------------------------------------------------------
    wxPen WaterfallChart::DeriveOutlinePen(const wxColour& color)
        {
        // rive the luminance low instead so that every bar gets a crisp, hue-consistent edge
        return wxPen{ Colors::ColorContrast::Shade(color, 0.15), 1 };
        }

    //----------------------------------------------------------------
    void WaterfallChart::ShowBarValues(const bool show)
        {
        if (m_showValues == show)
            {
            return;
            }
        m_showValues = show;
        RebuildBars();
        }

    //----------------------------------------------------------------
    void WaterfallChart::ShowBlockValues(const bool show)
        {
        if (m_showBlockValues == show)
            {
            return;
            }
        m_showBlockValues = show;
        RebuildBars();
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetValueDisplay(const NumberDisplay display)
        {
        if (m_valueDisplay == display)
            {
            return;
            }
        m_valueDisplay = display;
        RebuildBars();
        }

    //----------------------------------------------------------------
    bool WaterfallChart::IsTotalFlagLabel(wxString flag)
        {
        flag.Trim(true).Trim(false);
        if (flag.CmpNoCase(L"1") == 0 || flag.CmpNoCase(L"true") == 0 ||
            flag.CmpNoCase(L"yes") == 0 || flag.CmpNoCase(L"y") == 0 ||
            flag.CmpNoCase(L"total") == 0 || flag.CmpNoCase(L"subtotal") == 0)
            {
            return true;
            }
        long flagValue{ 0 };
        return (flag.ToLong(&flagValue) && flagValue == 1);
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                 const wxString& labelColumnName, const wxString& valueColumnName,
                                 const std::optional<wxString>& totalFlagColumnName)
        {
        SetDataset(data);
        m_labelColumnName = labelColumnName;
        m_valueColumnName = valueColumnName;
        m_totalFlagColumnName = totalFlagColumnName;
        GetSelectedIds().clear();
        m_rows.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        const auto valueColumn = GetContinuousColumn(m_valueColumnName);

        // labels can come from the ID column or a categorical column
        std::vector<wxString> labels(GetDataset()->GetRowCount());
        if (GetDataset()->GetIdColumn().GetName().CmpNoCase(m_labelColumnName) == 0)
            {
            for (size_t row = 0; row < GetDataset()->GetRowCount(); ++row)
                {
                labels[row] = GetDataset()->GetIdColumn().GetValue(row);
                }
            }
        else
            {
            const auto labelColumn = GetCategoricalColumn(m_labelColumnName);
            for (size_t row = 0; row < GetDataset()->GetRowCount(); ++row)
                {
                labels[row] = labelColumn->GetValueAsLabel(row);
                }
            }

        // resolve the optional total flags (false means every row is a change)
        std::vector<bool> totalFlags(GetDataset()->GetRowCount(), false);
        if (m_totalFlagColumnName.has_value() && !m_totalFlagColumnName->empty())
            {
            const auto catColumn =
                GetDataset()->GetCategoricalColumn(m_totalFlagColumnName.value());
            if (catColumn != GetDataset()->GetCategoricalColumns().cend())
                {
                // resolve which string table codes mean "total" once, rather than
                // resolving every row's code back to a label and re-parsing it
                std::set<Data::GroupIdType> totalCodes;
                for (const auto& [code, label] : catColumn->GetStringTable())
                    {
                    if (IsTotalFlagLabel(label))
                        {
                        totalCodes.insert(code);
                        }
                    }
                for (size_t row = 0; row < GetDataset()->GetRowCount(); ++row)
                    {
                    totalFlags[row] = totalCodes.contains(catColumn->GetValue(row));
                    }
                }
            else
                {
                const auto contColumn =
                    GetDataset()->GetContinuousColumn(m_totalFlagColumnName.value());
                if (contColumn == GetDataset()->GetContinuousColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': categorical or continuous column not found for graph."),
                            m_totalFlagColumnName.value())
                            .ToUTF8());
                    }
                for (size_t row = 0; row < GetDataset()->GetRowCount(); ++row)
                    {
                    const double flagValue{ contColumn->GetValue(row) };
                    totalFlags[row] = std::isfinite(flagValue) && flagValue != 0;
                    }
                }
            }

        m_rows.reserve(GetDataset()->GetRowCount());
        for (size_t row = 0; row < GetDataset()->GetRowCount(); ++row)
            {
            m_rows.push_back({ labels[row], static_cast<double>(m_rows.size() + 1),
                               valueColumn->GetValue(row), totalFlags[row] });
            }

        RebuildBars();
        }

    //----------------------------------------------------------------
    void WaterfallChart::RebuildBars()
        {
        ClearBars();
        // ClearBars() resets the axes, so recreate them
        ApplyAxisAppearance();
        // keep the base class from overwriting the signed value labels
        SetBinLabelDisplay(BinLabelDisplay::NoDisplay);

        if (m_rows.empty())
            {
            return;
            }

        GetBarAxis().GetTitle().SetText(m_labelColumnName);
        GetScalingAxis().GetTitle().SetText(m_valueColumnName);

        const auto formatValue = [this](const double value)
        {
            switch (m_valueDisplay)
                {
            case NumberDisplay::Currency:
                return wxNumberFormatter::ToString(
                    value, 2,
                    wxNumberFormatter::Style::Style_WithThousandsSep |
                        wxNumberFormatter::Style::Style_Currency |
                        wxNumberFormatter::Style::Style_CurrencySymbol |
                        wxNumberFormatter::Style::Style_NoTrailingZeroes);
            case NumberDisplay::ValueSimple:
                return wxNumberFormatter::ToString(value, 2, wxNumberFormatter::Style::Style_None);
            case NumberDisplay::Percentage:
                return wxString::Format(
                    L"%s%%", wxNumberFormatter::ToString(
                                 value, 2, wxNumberFormatter::Style::Style_NoTrailingZeroes));
            case NumberDisplay::Value:
                [[fallthrough]];
            default:
                return wxNumberFormatter::ToString(
                    value, 2, wxNumberFormatter::Style::Style_NoTrailingZeroes);
                }
        };

        double runningTotal{ 0 };
        double minExtent{ 0 };
        double maxExtent{ 0 };

        for (const auto& row : m_rows)
            {
            // missing amounts are skipped without affecting the running total
            if (!row.m_isTotal && !std::isfinite(row.m_value))
                {
                continue;
                }

            double barStart{ 0 };
            double barLength{ 0 };
            double displayValue{ 0 };
            const wxBrush* barBrush{ &m_totalBrush };
            const wxPen* barPen{ &m_totalPen };
            if (row.m_isTotal)
                {
                // the value column is ignored for totals; the bar shows the
                // running sum so far and leaves it unchanged
                displayValue = runningTotal;
                barStart = std::min(0.0, runningTotal);
                barLength = std::fabs(runningTotal);
                }
            else
                {
                const double newTotal{ runningTotal + row.m_value };
                displayValue = row.m_value;
                barStart = std::min(runningTotal, newTotal);
                barLength = std::fabs(row.m_value);
                runningTotal = newTotal;
                if (row.m_value >= 0)
                    {
                    barBrush = &m_increaseBrush;
                    barPen = &m_increasePen;
                    }
                else
                    {
                    barBrush = &m_decreaseBrush;
                    barPen = &m_decreasePen;
                    }
                }

            minExtent = std::min(minExtent, barStart);
            maxExtent = std::max(maxExtent, barStart + barLength);

            // values drawn across the bar itself (contrasting text for readability;
            // bars too narrow to fit get a framed label automatically)
            Wisteria::GraphItems::Label blockDecal;
            if (m_showBlockValues)
                {
                blockDecal = Wisteria::GraphItems::Label{
                    Wisteria::GraphItems::GraphItemInfo{ formatValue(displayValue) }
                        .FontColor(
                            Colors::ColorContrast::BlackOrWhiteContrast(barBrush->GetColour()))
                        .Pen(wxNullPen)
                };
                }

            Bar theBar{ row.m_axisPosition,
                        { BarBlock{
                            BarBlockInfo(barLength).Brush(*barBrush).OutlinePen(*barPen).Decal(
                                blockDecal) } },
                        m_showValues ? formatValue(displayValue) : wxString{},
                        Wisteria::GraphItems::Label{ row.m_label },
                        BoxEffect::Solid };
            theBar.SetCustomScalingAxisStartPosition(barStart);
            AddBar(theBar);
            }

        // AddBar() only grows the scaling axis upward from zero, so explicitly
        // stretch it to fit bars floating below zero (or totals beneath it)
        const auto [scaleStart, scaleEnd] = GetScalingAxis().GetRange();
        const double newStart{ std::min(scaleStart, minExtent) };
        const double newEnd{ std::max(scaleEnd, maxExtent) };
        if (newStart != scaleStart || newEnd != scaleEnd)
            {
            GetScalingAxis().SetRange(newStart, newEnd, GetScalingAxis().GetPrecision(),
                                      GetScalingAxis().GetInterval(),
                                      GetScalingAxis().GetDisplayInterval());
            }
        }

    //----------------------------------------------------------------
    void WaterfallChart::SetAutoAccessibilityAttributes()
        {
        wxString label{ _(L"A waterfall chart") };
        AddAccessibilityAttribute(label, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(label, GetSubtitle().GetText(), L", ");

        label += L". ";
        label += wxString::Format(
            /* TRANSLATORS: waterfall chart accessibility: step count. %zu is the count. */
            _(L"%zu steps"), m_rows.size());

        for (const auto& row : m_rows)
            {
            label += L". ";
            label += row.m_label;
            if (row.m_isTotal)
                {
                label = wxString::Format(_(L"%s (total)"), label);
                }
            else if (std::isfinite(row.m_value))
                {
                label = wxString::Format(
                    /* TRANSLATORS: waterfall chart accessibility: change amount.
                       %s are the label and value. */
                    _(L"%s, change %s"), label,
                    wxNumberFormatter::ToString(row.m_value, 2,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            }

        AddAccessibilityAttribute(label, GetCaption().GetText(), L". ");
        if (!label.EndsWith(L"."))
            {
            label += L".";
            }
        GetAutoAccessibilityAttributes() = wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(label);
        }
    } // namespace Wisteria::Graphs
