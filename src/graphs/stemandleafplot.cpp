///////////////////////////////////////////////////////////////////////////////
// Name:        stemandleafplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "stemandleafplot.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::StemAndLeafPlot, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    StemAndLeafPlot::StemAndLeafPlot(Canvas * canvas) : GroupGraph2D(canvas)
        {
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void StemAndLeafPlot::SetData(const std::shared_ptr<const Data::Dataset>& data,
                                  const wxString& continuousColumnName,
                                  const std::optional<wxString>& groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();
        m_stems.clear();
        m_continuousColumnName = continuousColumnName;
        m_maxLeftLeafCount = 0;
        m_maxRightLeafCount = 0;
        m_leftGroupLabel.clear();
        m_rightGroupLabel.clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);

        const auto continuousColumn = GetDataset()->GetContinuousColumn(continuousColumnName);
        if (continuousColumn == GetDataset()->GetContinuousColumns().cend())
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': continuous column not found for stem-and-leaf plot."),
                                 continuousColumnName)
                    .ToUTF8());
            }

        // validate grouping
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            if (GetGroupCount() != 2)
                {
                throw std::runtime_error(
                    wxString::Format(_(L"Stem-and-leaf plot requires exactly "
                                       "2 groups for back-to-back display, but %zu were found."),
                                     GetGroupCount())
                        .ToUTF8());
                }
            // extract the two group IDs and labels;
            // BuildGroupIdMap() sorts alphabetically,
            // so index 0 = left, index 1 = right
            const auto groupCol = GetGroupColumn();
            bool firstGroup{ true };
            for (const auto& [groupId, schemeIndex] : GetGroupIds())
                {
                if (firstGroup)
                    {
                    m_leftGroupId = groupId;
                    m_leftGroupLabel = groupCol->GetLabelFromID(groupId);
                    firstGroup = false;
                    }
                else
                    {
                    m_rightGroupId = groupId;
                    m_rightGroupLabel = groupCol->GetLabelFromID(groupId);
                    }
                }
            }

        if (GetDataset()->GetRowCount() > MAX_RECOMMENDED_OBS)
            {
            wxLogWarning(_(L"Stem-and-leaf plot has %zu observations "
                           "(recommended maximum is %zu). "
                           "Consider using a histogram or boxplot instead."),
                         GetDataset()->GetRowCount(), MAX_RECOMMENDED_OBS);
            }

        // build stems and leaves
        std::map<int64_t, StemData> stemMap;

        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            const auto val = continuousColumn->GetValue(i);
            if (std::isnan(val))
                {
                continue;
                }

            const auto intVal = static_cast<int64_t>(std::floor(val));
            const auto stem = intVal / 10;
            const auto leaf = static_cast<int>(std::abs(intVal % 10));

            auto& sd = stemMap[stem];
            sd.m_stem = stem;

            if (IsUsingGrouping())
                {
                const auto groupId = GetGroupColumn()->GetValue(i);
                if (groupId == m_leftGroupId)
                    {
                    sd.m_leftLeaves.push_back(leaf);
                    }
                else
                    {
                    sd.m_rightLeaves.push_back(leaf);
                    }
                }
            else
                {
                sd.m_rightLeaves.push_back(leaf);
                }
            }

        // transfer to vector, sort leaves, track max counts
        m_stems.reserve(stemMap.size());
        for (auto& [key, sd] : stemMap)
            {
            std::ranges::sort(sd.m_leftLeaves);
            std::ranges::sort(sd.m_rightLeaves);
            m_maxLeftLeafCount = std::max(m_maxLeftLeafCount, sd.m_leftLeaves.size());
            m_maxRightLeafCount = std::max(m_maxRightLeafCount, sd.m_rightLeaves.size());
            m_stems.push_back(std::move(sd));
            }
        }

    //----------------------------------------------------------------
    wxString StemAndLeafPlot::FormatLeaves(const std::vector<int>& leaves)
        {
        wxString result;
        for (size_t i = 0; i < leaves.size(); ++i)
            {
            if (i > 0)
                {
                result += L"  ";
                }
            result += std::to_wstring(leaves[i]);
            }
        return result;
        }

    //----------------------------------------------------------------
    wxString StemAndLeafPlot::FormatLeavesReversed(const std::vector<int>& leaves)
        {
        wxString result;
        for (auto it = leaves.rbegin(); it != leaves.rend(); ++it)
            {
            if (it != leaves.rbegin())
                {
                result += L"  ";
                }
            result += std::to_wstring(*it);
            }
        return result;
        }

    //----------------------------------------------------------------
    void StemAndLeafPlot::RecalcSizes(wxDC & dc)
        {
        if (GetDataset() == nullptr || m_stems.empty())
            {
            return;
            }

        Graph2D::RecalcSizes(dc);

        const wxRect drawArea = GetPlotAreaBoundingBox();
        const bool isBackToBack = IsUsingGrouping();

        // measure character dimensions
        auto measuringLabel = GraphItems::Label(GraphItems::GraphItemInfo(L"0")
                                                    .Scaling(GetScaling())
                                                    .DPIScaling(GetDPIScaleFactor())
                                                    .Pen(wxNullPen));
        const auto singleCharBox = measuringLabel.GetBoundingBox(dc);
        const auto padding = ScaleToScreenAndCanvas(6);
        const auto rowHeight = singleCharBox.GetHeight() + padding;

        // compute stem column width
        wxCoord stemColWidth{ 0 };
        for (const auto& sd : m_stems)
            {
            measuringLabel.SetText(std::to_wstring(sd.m_stem));
            const auto stemWidth = measuringLabel.GetBoundingBox(dc).GetWidth();
            stemColWidth = std::max(stemColWidth, stemWidth);
            }
        stemColWidth += padding * 2;

        // compute leaf column widths:
        // each leaf takes ~3 char widths ("N  "),
        // but we measure the actual formatted strings
        wxCoord rightLeafColWidth{ 0 };
        wxCoord leftLeafColWidth{ 0 };

        for (const auto& sd : m_stems)
            {
            if (!sd.m_rightLeaves.empty())
                {
                measuringLabel.SetText(FormatLeaves(sd.m_rightLeaves));
                rightLeafColWidth =
                    std::max(rightLeafColWidth, measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            if (isBackToBack && !sd.m_leftLeaves.empty())
                {
                measuringLabel.SetText(FormatLeavesReversed(sd.m_leftLeaves));
                leftLeafColWidth =
                    std::max(leftLeafColWidth, measuringLabel.GetBoundingBox(dc).GetWidth());
                }
            }
        rightLeafColWidth += padding * 2;
        if (isBackToBack)
            {
            leftLeafColWidth += padding * 2;
            }

        // ensure minimum widths for headers
        if (isBackToBack)
            {
            measuringLabel.SetText(wxString::Format(_(L"Leaf (%s)"), m_leftGroupLabel));
            leftLeafColWidth = std::max<wxCoord>(
                leftLeafColWidth, measuringLabel.GetBoundingBox(dc).GetWidth() + padding * 2);
            measuringLabel.SetText(wxString::Format(_(L"Leaf (%s)"), m_rightGroupLabel));
            rightLeafColWidth = std::max<wxCoord>(
                rightLeafColWidth, measuringLabel.GetBoundingBox(dc).GetWidth() + padding * 2);
            }
        else
            {
            measuringLabel.SetText(_(L"Leaf"));
            rightLeafColWidth = std::max<wxCoord>(
                rightLeafColWidth, measuringLabel.GetBoundingBox(dc).GetWidth() + padding * 2);
            }
        measuringLabel.SetText(_(L"Stem"));
        stemColWidth = std::max<wxCoord>(
            stemColWidth, measuringLabel.GetBoundingBox(dc).GetWidth() + padding * 2);

        // gaps
        const auto columnGap = ScaleToScreenAndCanvas(8);
        const auto headerDataGap = ScaleToScreenAndCanvas(4);

        // total table dimensions
        const wxCoord gapCount = isBackToBack ? 2 : 1;
        const wxCoord totalWidth =
            leftLeafColWidth + stemColWidth + rightLeafColWidth + columnGap * gapCount;

        // scale columns horizontally to fit the draw area width,
        // but keep row height at its natural size (the canvas will scroll
        // if the content is taller than the viewport)
        const double widthScale =
            std::min(safe_divide<double>(drawArea.GetWidth(), totalWidth), 1.0);

        const auto scaledStemWidth = static_cast<wxCoord>(stemColWidth * widthScale);
        const auto scaledLeftWidth = static_cast<wxCoord>(leftLeafColWidth * widthScale);
        const auto scaledRightWidth = static_cast<wxCoord>(rightLeafColWidth * widthScale);
        const auto scaledGap = static_cast<wxCoord>(columnGap * widthScale);
        const auto scaledTotalWidth =
            scaledLeftWidth + scaledStemWidth + scaledRightWidth + scaledGap * gapCount;

        // scale rows down vertically if the table is taller than the draw area
        const wxCoord naturalHeight =
            rowHeight * static_cast<wxCoord>(m_stems.size() + 1) + headerDataGap;
        const double heightScale =
            std::min(safe_divide<double>(drawArea.GetHeight(), naturalHeight), 1.0);
        const auto scaledRowHeight = static_cast<wxCoord>(rowHeight * heightScale);
        const auto scaledHeaderDataGap = static_cast<wxCoord>(headerDataGap * heightScale);
        const wxCoord totalHeight =
            scaledRowHeight * static_cast<wxCoord>(m_stems.size() + 1) + scaledHeaderDataGap;

        // center the table horizontally in the draw area
        const wxCoord tableX =
            drawArea.GetX() + safe_divide(drawArea.GetWidth() - scaledTotalWidth, 2);
        const wxCoord tableY = drawArea.GetY();

        // build cell labels
        std::vector<std::unique_ptr<GraphItems::Label>> headerLabels;
        std::vector<std::unique_ptr<GraphItems::Label>> dataLabels;
        double smallestHeaderScaling{ std::numeric_limits<double>::max() };
        double smallestDataScaling{ std::numeric_limits<double>::max() };

        // helper to create a header: a filled rounded Polygon for the background,
        // then a text Label on top. Using a Polygon ensures the header background
        // is the exact same width as the column border Polygon beneath it.
        const auto addHeaderLabel = [&](const wxString& text, const wxRect& cellRect,
                                        const wxColour& bgColor,
                                        const std::optional<wxColour>& fontColor)
        {
            // filled rounded rectangle background
            const std::array<wxPoint, 4> bgPoints{
                wxPoint{ cellRect.GetLeft(), cellRect.GetTop() },
                wxPoint{ cellRect.GetRight(), cellRect.GetTop() },
                wxPoint{ cellRect.GetRight(), cellRect.GetBottom() },
                wxPoint{ cellRect.GetLeft(), cellRect.GetBottom() }
            };
            auto bgPoly =
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{}
                                                          .Pen(wxNullPen)
                                                          .Brush(bgColor)
                                                          .Scaling(GetScaling())
                                                          .DPIScaling(GetDPIScaleFactor()),
                                                      bgPoints);
            bgPoly->SetShape(GraphItems::Polygon::PolygonShape::Rectangle);
            bgPoly->SetBoxCorners(BoxCorners::Rounded);
            AddObject(std::move(bgPoly));

            // text label on top (no background)
            auto label = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ text }
                    .Pen(wxNullPen)
                    .Padding(2, 5, 2, 5)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .FontColor(
                        fontColor.value_or(Colors::ColorContrast::BlackOrWhiteContrast(bgColor)))
                    .Anchoring(Anchoring::TopLeftCorner)
                    .AnchorPoint(cellRect.GetTopLeft()));
            label->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            label->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            label->SetBoundingBox(cellRect, dc, GetScaling());
            smallestHeaderScaling = std::min(label->GetScaling(), smallestHeaderScaling);
            headerLabels.push_back(std::move(label));
        };

        // helper to create a data cell label
        const auto addDataLabel = [&](const wxString& text, const wxRect& cellRect,
                                      PageHorizontalAlignment hAlign,
                                      const std::optional<wxColour>& fontColor)
        {
            auto label = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ !text.empty() ? text : wxString(L" ") }
                    .Pen(wxNullPen)
                    .Padding(2, 2, 2, 2)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .FontColor(fontColor.value_or(
                        Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor())))
                    .Anchoring(Anchoring::TopLeftCorner)
                    .AnchorPoint(cellRect.GetTopLeft()));
            label->SetPageHorizontalAlignment(hAlign);
            label->SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            label->SetBoundingBox(cellRect, dc, GetScaling());
            smallestDataScaling = std::min(label->GetScaling(), smallestDataScaling);
            dataLabels.push_back(std::move(label));
        };

        // column X positions
        wxCoord leftLeafX{ tableX };
        wxCoord stemX{ tableX };
        wxCoord rightLeafX{ tableX };

        if (isBackToBack)
            {
            leftLeafX = tableX;
            stemX = leftLeafX + scaledLeftWidth + scaledGap;
            rightLeafX = stemX + scaledStemWidth + scaledGap;
            }
        else
            {
            stemX = tableX;
            rightLeafX = stemX + scaledStemWidth + scaledGap;
            }

        // header row
        const wxCoord headerY = tableY;

        if (isBackToBack)
            {
            addHeaderLabel(wxString::Format(_(L"Leaf (%s)"), m_leftGroupLabel),
                           wxRect(leftLeafX, headerY, scaledLeftWidth, scaledRowHeight),
                           GetLeafHeaderColor(), GetLeafHeaderFontColor());
            addHeaderLabel(_(L"Stem"), wxRect(stemX, headerY, scaledStemWidth, scaledRowHeight),
                           GetStemHeaderColor(), GetStemHeaderFontColor());
            addHeaderLabel(wxString::Format(_(L"Leaf (%s)"), m_rightGroupLabel),
                           wxRect(rightLeafX, headerY, scaledRightWidth, scaledRowHeight),
                           GetLeafHeaderColor(), GetLeafHeaderFontColor());
            }
        else
            {
            addHeaderLabel(_(L"Stem"), wxRect(stemX, headerY, scaledStemWidth, scaledRowHeight),
                           GetStemHeaderColor(), GetStemHeaderFontColor());
            addHeaderLabel(_(L"Leaf"),
                           wxRect(rightLeafX, headerY, scaledRightWidth, scaledRowHeight),
                           GetLeafHeaderColor(), GetLeafHeaderFontColor());
            }

        // draw column borders around data rows
        const wxCoord dataTop = headerY + scaledRowHeight + scaledHeaderDataGap;
        const auto dataBottom = dataTop + scaledRowHeight * static_cast<wxCoord>(m_stems.size());

        const auto drawColumnBorder =
            [&](wxCoord colX, wxCoord colWidth, const wxColour& outlineColor)
        {
            const std::array<wxPoint, 4> outlinePoints{ wxPoint{ colX, dataTop },
                                                        wxPoint{ colX + colWidth, dataTop },
                                                        wxPoint(colX + colWidth, dataBottom),
                                                        wxPoint(colX, dataBottom) };
            auto colLines =
                std::make_unique<GraphItems::Polygon>(GraphItems::GraphItemInfo{}
                                                          .Pen(wxPen{ outlineColor, 1 })
                                                          .Scaling(GetScaling())
                                                          .DPIScaling(GetDPIScaleFactor()),
                                                      outlinePoints);

            colLines->SetShape(GraphItems::Polygon::PolygonShape::Rectangle);
            colLines->SetBoxCorners(BoxCorners::Rounded);

            AddObject(std::move(colLines));
        };

        if (isBackToBack)
            {
            drawColumnBorder(leftLeafX, scaledLeftWidth, m_leafHeaderColor);
            drawColumnBorder(stemX, scaledStemWidth, m_stemHeaderColor);
            drawColumnBorder(rightLeafX, scaledRightWidth, m_leafHeaderColor);
            }
        else
            {
            drawColumnBorder(stemX, scaledStemWidth, m_stemHeaderColor);
            drawColumnBorder(rightLeafX, scaledRightWidth, m_leafHeaderColor);
            }

        // data rows
        const auto columnPadding = ScaleToScreenAndCanvas(6);
        wxCoord currentY = headerY + scaledRowHeight + scaledHeaderDataGap;

        for (const auto& sd : m_stems)
            {
            if (isBackToBack)
                {
                addDataLabel(FormatLeavesReversed(sd.m_leftLeaves),
                             wxRect(leftLeafX + columnPadding, currentY,
                                    scaledLeftWidth - columnPadding * 2, scaledRowHeight),
                             PageHorizontalAlignment::RightAligned, GetLeafValueFontColor());
                }

            addDataLabel(std::to_wstring(sd.m_stem),
                         wxRect(stemX + columnPadding, currentY,
                                scaledStemWidth - columnPadding * 2, scaledRowHeight),
                         PageHorizontalAlignment::Centered, GetStemValueFontColor());

            addDataLabel(FormatLeaves(sd.m_rightLeaves),
                         wxRect(rightLeafX + columnPadding, currentY,
                                scaledRightWidth - columnPadding * 2, scaledRowHeight),
                         PageHorizontalAlignment::LeftAligned, GetLeafValueFontColor());

            currentY += scaledRowHeight;
            }

        // homogenize text scaling (headers and data separately)
        for (auto& label : headerLabels)
            {
            const wxRect bBox = label->GetBoundingBox(dc);
            label->SetScaling(smallestHeaderScaling);
            label->LockBoundingBoxScaling();
            label->SetBoundingBox(bBox, dc, GetScaling());
            label->UnlockBoundingBoxScaling();
            AddObject(std::move(label));
            }
        for (auto& label : dataLabels)
            {
            const wxRect bBox = label->GetBoundingBox(dc);
            label->SetScaling(smallestDataScaling);
            label->LockBoundingBoxScaling();
            label->SetBoundingBox(bBox, dc, GetScaling());
            label->UnlockBoundingBoxScaling();
            AddObject(std::move(label));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> StemAndLeafPlot::CreateLegend(const LegendOptions& options)
        {
        if (GetDataset() == nullptr || m_stems.empty())
            {
            return nullptr;
            }

        // resolve font colors for stem and leaf values
        const auto stemFontColor = GetStemValueFontColor().value_or(
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));
        const auto leafFontColor = GetLeafValueFontColor().value_or(
            Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()));

        const auto stemHex = stemFontColor.GetAsString(wxC2S_HTML_SYNTAX);
        const auto leafHex = leafFontColor.GetAsString(wxC2S_HTML_SYNTAX);

        // helpers to wrap values in markup spans
        const auto colorSpan = [](const wxString& hex, const wxString& text)
        { return wxString::Format(L"<span style='color: %s;'>%s</span>", hex, text); };
        const auto italicSpan = [](const wxString& text)
        { return wxString::Format(L"<span style='font-style: italic;'>%s</span>", text); };

        // build a key line from a stem and leaf
        const auto buildKeyLine = [&](int64_t stemVal, int leafVal)
        {
            const auto val = stemVal * 10 + leafVal;
            const auto formattedVal = wxNumberFormatter::ToString(
                static_cast<long>(val), wxNumberFormatter::Style::Style_WithThousandsSep);
            return wxString::Format(
                /* TRANSLATORS: Stem-and-leaf legend key line.
                   %1 = stem digit, %2 = "(stem)", %3 = leaf digit,
                   %4 = "(leaf)", %5 = full value.
                   '|' separates stem from leaf. */
                _(L"%s %s | %s %s represents the value %s"),
                colorSpan(stemHex, std::to_wstring(stemVal)), _(L"(stem)"),
                colorSpan(leafHex, std::to_wstring(leafVal)), _(L"(leaf)"),
                italicSpan(formattedVal));
        };

        // use the first and last stem+leaf combos from the data
        const auto& firstStem = m_stems.front();
        const auto& firstLeaves =
            firstStem.m_rightLeaves.empty() ? firstStem.m_leftLeaves : firstStem.m_rightLeaves;
        const int firstLeaf = firstLeaves.empty() ? 0 : firstLeaves.front();

        const auto& lastStem = m_stems.back();
        const auto& lastLeaves =
            lastStem.m_rightLeaves.empty() ? lastStem.m_leftLeaves : lastStem.m_rightLeaves;
        const int lastLeaf = lastLeaves.empty() ? 0 : lastLeaves.back();

        wxString legendText;
        if (options.IsIncludingHeader())
            {
            // blank line after header for the separator to occupy
            legendText = _(L"Key\n \n");
            }

        // show the variable name
        /* TRANSLATORS: legend line showing the plotted variable name. */
        legendText += wxString::Format(_(L"Variable: %s"), italicSpan(m_continuousColumnName));

        // show the grouping variable if back-to-back
        if (IsUsingGrouping())
            {
            /* TRANSLATORS: legend line showing the grouping variable name. */
            legendText += wxString::Format(L"\n" + _(L"Grouped by: %s"),
                                           italicSpan(GetGroupColumn()->GetName()));
            }

        legendText += L"\n";
        legendText += buildKeyLine(firstStem.m_stem, firstLeaf);
        // add a second example if the last combo differs from the first
        if (lastStem.m_stem != firstStem.m_stem || lastLeaf != firstLeaf)
            {
            legendText += L"\n";
            legendText += buildKeyLine(lastStem.m_stem, lastLeaf);
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{}.Pen(wxNullPen).DPIScaling(GetDPIScaleFactor()));
        legend->GetGraphItemInfo().Text(legendText);
        legend->EnableMarkup(true);
        if (options.IsIncludingHeader())
            {
            legend->GetHeaderInfo().Enable(true);
            legend->GetLinesIgnoringLeftMargin().insert(0);
            legend->GetLegendIcons().emplace_back(
                Icons::IconShape::HorizontalSeparator,
                wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black), 2 },
                wxColour{ 0, 0, 0, 0 });
            }

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }
    } // namespace Wisteria::Graphs
