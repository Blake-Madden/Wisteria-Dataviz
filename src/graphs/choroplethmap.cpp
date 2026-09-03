///////////////////////////////////////////////////////////////////////////////
// Name:        choroplethmap.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "choroplethmap.h"
#include "../base/lines.h"
#include "../base/points.h"
#include "../base/polygon.h"
#include "../math/mathematics.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <limits>
#include <map>
#include <set>
#include <utility>
#include <wx/dc.h>
#include <wx/log.h>
#include <wx/math.h>
#include <wx/numformatter.h>

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChoroplethMap, Wisteria::Graphs::Graph2D)

    wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::ChoroplethLegend,
                              Wisteria::GraphItems::GraphItemBase)

        namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    void ChoroplethRegion::AddOuterRing(std::vector<wxPoint> ring)
        {
        if (ring.size() < 3)
            {
            return;
            }
        const wxRect ringBox = GraphItems::Polygon::GetPolygonBoundingBox(ring);
        m_boundingBox = m_outerRings.empty() ? ringBox : m_boundingBox.Union(ringBox);
        m_outerRings.push_back(std::move(ring));
        }

    //----------------------------------------------------------------
    void ChoroplethRegion::Offset(const int xToMove, const int yToMove)
        {
        const wxPoint shift{ xToMove, yToMove };
        for (auto& ring : m_outerRings)
            {
            for (auto& point : ring)
                {
                point += shift;
                }
            }
        for (auto& ring : m_holeRings)
            {
            for (auto& point : ring)
                {
                point += shift;
                }
            }
        m_boundingBox.Offset(xToMove, yToMove);
        }

    //----------------------------------------------------------------
    bool ChoroplethRegion::HitTest(const wxPoint pt, [[maybe_unused]] wxDC& dc) const
        {
        if (!m_boundingBox.Contains(pt))
            {
            return false;
            }
        const bool inOuter =
            std::ranges::any_of(m_outerRings, [&pt](const auto& ring)
                                { return geometry::is_inside_polygon_crossing_number(pt, ring); });
        if (!inOuter)
            {
            return false;
            }
        return std::ranges::none_of(
            m_holeRings, [&pt](const auto& ring)
            { return geometry::is_inside_polygon_crossing_number(pt, ring); });
        }

    //----------------------------------------------------------------
    wxRect ChoroplethRegion::Draw(wxDC & dc) const
        {
        if (!IsShown() || m_outerRings.empty())
            {
            return {};
            }

        wxPen drawPen(GetPen().IsOk() ? GetPen() : *wxTRANSPARENT_PEN);
        drawPen.SetWidth(std::max<int>(1, static_cast<int>(ScaleToScreenAndCanvas(
                                              drawPen.GetWidth() > 0 ? drawPen.GetWidth() : 1))));
        if (IsSelected())
            {
            const bool penIsLight{ drawPen.GetColour().IsOk() &&
                                   Colors::ColorContrast::IsLight(drawPen.GetColour()) };
            drawPen = wxPen(penIsLight ? Colors::ColorBrewer::GetColor(Colors::Color::White) :
                                         Colors::ColorBrewer::GetColor(Colors::Color::Black),
                            2 * drawPen.GetWidth(), wxPENSTYLE_DOT);
            }

        const wxDCPenChanger penChanger(dc, drawPen);
        const wxDCBrushChanger brushChanger(dc,
                                            GetBrush().IsOk() ? GetBrush() : *wxTRANSPARENT_BRUSH);

        for (const auto& ring : m_outerRings)
            {
            if (ring.size() >= 3)
                {
                dc.DrawPolygon(static_cast<int>(ring.size()), ring.data());
                }
            }

        return m_boundingBox;
        }

    //----------------------------------------------------------------
    void ChoroplethRegion::DrawSelectionLabel(wxDC & dc, const double scaling,
                                              const wxRect boundingBox) const
        {
        if (!IsSelected() || !IsShowingLabelWhenSelected() || GetText().empty())
            {
            return;
            }

        const wxRect itemBoundingBox{ GetBoundingBox(dc) };
        GraphItems::Label selectionLabel(
            GraphItems::GraphItemInfo{ GetText() }
                .Scaling(scaling)
                .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Black))
                .DPIScaling(GetDPIScaleFactor())
                .Padding(2, 2, 2, 2)
                .FontBackgroundColor(Colors::ColorBrewer::GetColor(Colors::Color::White))
                .Anchoring(Anchoring::Center)
                .AnchorPoint(
                    itemBoundingBox.GetTopLeft() +
                    wxPoint(itemBoundingBox.GetWidth() / 2, itemBoundingBox.GetHeight() / 2)));
        // The default selection label is a bit heavy for a region label. Take it down
        // a step, keeping it scaled to the canvas.
        auto& labelFont = selectionLabel.GetFont();
        labelFont.SetFractionalPointSize(labelFont.GetFractionalPointSize() *
                                         math_constants::two_thirds);

        const wxRect selectionLabelBox = selectionLabel.GetBoundingBox(dc);
        if (!boundingBox.IsEmpty() && selectionLabelBox.GetBottom() > boundingBox.GetBottom())
            {
            selectionLabel.SetAnchorPoint(
                { selectionLabel.GetAnchorPoint().x,
                  selectionLabel.GetAnchorPoint().y -
                      (selectionLabelBox.GetBottom() - boundingBox.GetBottom()) });
            }
        if (!boundingBox.IsEmpty() && selectionLabelBox.GetTop() < boundingBox.GetTop())
            {
            selectionLabel.SetAnchorPoint(
                { selectionLabel.GetAnchorPoint().x,
                  selectionLabel.GetAnchorPoint().y +
                      (boundingBox.GetTop() - selectionLabelBox.GetTop()) });
            }
        if (!boundingBox.IsEmpty() && selectionLabelBox.GetRight() > boundingBox.GetRight())
            {
            selectionLabel.SetAnchorPoint(
                { selectionLabel.GetAnchorPoint().x -
                      (selectionLabelBox.GetRight() - boundingBox.GetRight()),
                  selectionLabel.GetAnchorPoint().y });
            }
        if (!boundingBox.IsEmpty() && selectionLabelBox.GetLeft() < boundingBox.GetLeft())
            {
            selectionLabel.SetAnchorPoint(
                { selectionLabel.GetAnchorPoint().x +
                      (boundingBox.GetLeft() - selectionLabelBox.GetLeft()),
                  selectionLabel.GetAnchorPoint().y });
            }
        selectionLabel.Draw(dc);
        }

    //----------------------------------------------------------------
    double ChoroplethLegend::RingRadius(const double value) const
        {
        const double maxRadius = ScaleToScreenAndCanvas(22);
        const double minRadius = ScaleToScreenAndCanvas(3);
        const double topValue = m_entries.empty() ? 1.0 : m_entries.front().m_value;
        if (topValue <= 0.0)
            {
            return minRadius;
            }
        return std::max(minRadius, maxRadius * std::sqrt(safe_divide<double>(value, topValue)));
        }

    //----------------------------------------------------------------
    wxSize ChoroplethLegend::LayOutKey(wxDC & dc, const wxPoint& origin, const bool draw) const
        {
        if (m_entries.empty())
            {
            return { 0, 0 };
            }

        const auto gap = static_cast<int>(ScaleToScreenAndCanvas(3));
        const auto leaderLength = static_cast<int>(ScaleToScreenAndCanvas(8));

        const auto makeLabel = [this](const wxString& text, const Anchoring anchoring)
        {
            return GraphItems::Label(GraphItems::GraphItemInfo{ text }
                                         .Pen(wxNullPen)
                                         .Scaling(GetScaling())
                                         .DPIScaling(GetDPIScaleFactor())
                                         .FontColor(m_textColor)
                                         .Anchoring(anchoring));
        };

        // the heading (usually the symbol column name)
        int headingHeight{ 0 };
        int headingWidth{ 0 };
        if (!m_heading.empty())
            {
            GraphItems::Label heading = makeLabel(m_heading, Anchoring::TopLeftCorner);
            heading.SetAnchorPoint(origin);
            const wxRect headingBox = heading.GetBoundingBox(dc);
            headingWidth = headingBox.GetWidth();
            headingHeight = headingBox.GetHeight() + gap;
            if (draw)
                {
                heading.Draw(dc);
                }
            }

        std::vector<int> radii;
        radii.reserve(m_entries.size());
        int widestLabel{ 0 };
        int labelHeight{ 0 };
        for (const auto& entry : m_entries)
            {
            radii.push_back(static_cast<int>(std::ceil(RingRadius(entry.m_value))));
            const wxRect labelBox = makeLabel(entry.m_label, Anchoring::Center).GetBoundingBox(dc);
            widestLabel = std::max(widestLabel, labelBox.GetWidth());
            labelHeight = std::max(labelHeight, labelBox.GetHeight());
            }

        const int topRadius = radii.front();
        const int baselineY = origin.y + headingHeight + (2 * topRadius);
        const int centerX = origin.x + topRadius;
        const int leaderEndX = origin.x + (2 * topRadius) + leaderLength;

        if (draw)
            {
            wxPen ringPen{ m_outlineColor, 1 };
            ringPen.SetWidth(std::max<int>(1, static_cast<int>(ScaleToScreenAndCanvas(1))));
            const wxDCPenChanger penChanger{ dc, ringPen };
            const wxDCBrushChanger brushChanger{ dc, *wxTRANSPARENT_BRUSH };
            for (size_t ring = 0; ring < m_entries.size(); ++ring)
                {
                const int radius = radii[ring];
                const int ringTopY = baselineY - (2 * radius);
                dc.DrawCircle(wxPoint{ centerX, baselineY - radius }, radius);
                dc.DrawLine(wxPoint{ centerX, ringTopY }, wxPoint{ leaderEndX, ringTopY });

                GraphItems::Label valueLabel =
                    makeLabel(m_entries[ring].m_label, Anchoring::Center);
                const wxRect valueLabelBox = valueLabel.GetBoundingBox(dc);
                valueLabel.SetAnchorPoint(
                    wxPoint{ leaderEndX + gap + (valueLabelBox.GetWidth() / 2), ringTopY });
                valueLabel.Draw(dc);
                }
            }

        const int keyWidth =
            std::max(headingWidth, (2 * topRadius) + leaderLength + gap + widestLabel);
        // the top ring's label is centered on the ring top, so it rises half a line
        // above the rings
        const int keyHeight = headingHeight + (2 * topRadius) + (labelHeight / 2);
        return { keyWidth, keyHeight };
        }

    //----------------------------------------------------------------
    void ChoroplethLegend::RecalcSizes(wxDC & dc)
        {
        const wxPoint position = m_rect.GetPosition();

        int scaleHeight{ 0 };
        if (m_scaleLegend != nullptr)
            {
            scaleHeight = m_scaleLegend->GetBoundingBox(dc).GetHeight();
            }

        const int sectionGap = (m_scaleLegend != nullptr && !m_entries.empty()) ?
                                   static_cast<int>(ScaleToScreenAndCanvas(6)) :
                                   0;
        const wxSize keySize = LayOutKey(dc, position, false);
        int contentWidth = keySize.GetWidth();
        if (m_scaleLegend != nullptr)
            {
            contentWidth = std::max(contentWidth, m_scaleLegend->GetBoundingBox(dc).GetWidth());
            }

        m_rect = wxRect{ position,
                         wxSize{ contentWidth, scaleHeight + sectionGap + keySize.GetHeight() } };
        }

    //----------------------------------------------------------------
    wxRect ChoroplethLegend::Draw(wxDC & dc) const
        {
        if (!IsShown() || m_rect.IsEmpty())
            {
            return {};
            }

        int scaleHeight{ 0 };
        if (m_scaleLegend != nullptr)
            {
            // anchor at natural size; SetBoundingBox() would rescale the font to fill
            scaleHeight = m_scaleLegend->GetBoundingBox(dc).GetHeight();
            m_scaleLegend->SetAnchorPoint(m_rect.GetTopLeft());
            m_scaleLegend->Draw(dc);
            }

        const int sectionGap = (m_scaleLegend != nullptr && !m_entries.empty()) ?
                                   static_cast<int>(ScaleToScreenAndCanvas(6)) :
                                   0;
        LayOutKey(dc, wxPoint{ m_rect.GetLeft(), m_rect.GetTop() + scaleHeight + sectionGap },
                  true);

        return m_rect;
        }

    //----------------------------------------------------------------
    ChoroplethMap::ChoroplethMap(
        Canvas * canvas, const std::shared_ptr<Colors::Schemes::ColorScheme>& colors /*= nullptr*/)
        : Graph2D(canvas)
        {
        // The scheme's first color is the low end of the shading ramp, the last color
        // the high end. When there is no value column, regions are filled with that
        // last color, so the color-scheme control also sets the plain region color.
        SetColorScheme(
            colors != nullptr ?
                colors :
                std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::ColorScheme{
                    Colors::ColorBrewer::GetColor(Colors::Color::White), wxColour(L"#6F9FD8") }));

        GetPen().SetColour(L"#808080");
        GetBrush().SetColour(L"#C9D8E8");

        // the map draws directly in plot-area pixels, so the axes are just a frame
        GetBottomXAxis().SetRange(0, 10, 0, 1, 1);
        GetLeftYAxis().SetRange(0, 10, 0, 1, 1);
        GetBottomXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetTopXAxis().Show(false);
        GetRightYAxis().Show(false);
        }

    //----------------------------------------------------------------
    void ChoroplethMap::SetData(const std::shared_ptr<const Data::GeoDataset>& data,
                                const std::optional<wxString>& valueColumnName /*= std::nullopt*/)
        {
        m_geoData = data;
        SetDataset(data);
        GetSelectedIds().clear();
        m_regionColors.clear();
        m_colorSpectrum.clear();
        m_valueRange = { 0.0, 0.0 };
        m_hasValues = false;
        m_valueTotal = 0.0;
        m_isCategorical = false;
        m_categoryLegend.clear();
        m_categoryRowCounts.clear();
        m_categorizedRegionCount = 0;
        m_isClassified = false;
        m_classBreaks.clear();
        m_classColors.clear();
        m_valueColumnName = valueColumnName.value_or(wxString{});
        m_symbolColumnName.clear();

        if (m_geoData == nullptr)
            {
            return;
            }

        if (m_valueColumnName.empty())
            {
            return;
            }

        const auto continuousColumn = m_geoData->GetContinuousColumn(m_valueColumnName);
        if (continuousColumn != m_geoData->GetContinuousColumns().cend())
            {
            const bool hasFiniteValue = std::ranges::any_of(
                continuousColumn->GetValues(), [](const auto val) { return std::isfinite(val); });
            if (hasFiniteValue)
                {
                m_colorSpectrum = GetColorScheme()->GetColors();
                for (const auto value : continuousColumn->GetValues())
                    {
                    if (std::isfinite(value))
                        {
                        m_valueTotal += value;
                        }
                    }

                // A classification splits the values into discrete classes, each with
                // one color; otherwise, the value is mapped onto the continuous ramp.
                if (m_classificationMethod == ClassificationMethod::Unclassed ||
                    !BuildClassifiedColors(continuousColumn->GetValues()))
                    {
                    Colors::ColorBrewer colorBrewer;
                    colorBrewer.SetColorScale(m_colorSpectrum.cbegin(), m_colorSpectrum.cend());
                    m_regionColors = colorBrewer.BrewColors(continuousColumn->GetValues().cbegin(),
                                                            continuousColumn->GetValues().cend());
                    m_valueRange = colorBrewer.GetRange();
                    }
                m_hasValues = true;
                }
            return;
            }

        const auto categoricalColumn = m_geoData->GetCategoricalColumn(m_valueColumnName);
        if (categoricalColumn != m_geoData->GetCategoricalColumns().cend())
            {
            BuildCategoricalColors(*categoricalColumn);
            return;
            }

        throw std::runtime_error(
            wxString::Format(_(L"'%s': column not found for choropleth map."), m_valueColumnName)
                .ToUTF8());
        }

    //----------------------------------------------------------------
    void ChoroplethMap::BuildCategoricalColors(const Data::ColumnWithStringTable& column)
        {
        const auto& stringTable = column.GetStringTable();

        // The palette is the graph's color scheme when it carries enough distinct
        // colors. The default scheme is a two-color ramp meant for continuous
        // shading, so fall back to a qualitative scheme for categories.
        std::vector<wxColour> palette;
        if (GetColorScheme() != nullptr && GetColorScheme()->GetColors().size() >= 3)
            {
            palette = GetColorScheme()->GetColors();
            }
        else
            {
            palette = Colors::Schemes::Decade1980s{}.GetColors();
            }
        if (palette.empty())
            {
            return;
            }

        // which category codes actually occur on a region, and how many regions
        // carry each one (missing data does not count as a category)
        std::set<Data::GroupIdType> usedCodes;
        for (size_t row = 0; row < column.GetRowCount(); ++row)
            {
            const auto code = column.GetValue(row);
            usedCodes.insert(code);
            ++m_categoryRowCounts[code];
            if (!column.IsMissingData(row))
                {
                ++m_categorizedRegionCount;
                }
            }

        // walk the string table in code order so the legend and colors are stable
        std::map<Data::GroupIdType, wxColour> codeColors;
        size_t nextColor{ 0 };
        for (const auto& [code, label] : stringTable)
            {
            if (label.empty() || !usedCodes.contains(code))
                {
                continue;
                }
            const wxColour categoryColor = palette[nextColor % palette.size()];
            ++nextColor;
            codeColors.emplace(code, categoryColor);
            m_categoryLegend.emplace_back(label, categoryColor);
            }

        if (codeColors.empty())
            {
            return;
            }

        m_regionColors.assign(column.GetRowCount(), wxColour{});
        for (size_t row = 0; row < column.GetRowCount(); ++row)
            {
            const auto foundColor = codeColors.find(column.GetValue(row));
            if (foundColor != codeColors.cend())
                {
                m_regionColors[row] = foundColor->second;
                }
            }
        m_isCategorical = true;
        m_hasValues = true;
        }

    //----------------------------------------------------------------
    std::vector<double> ChoroplethMap::JenksNaturalBreaks(std::vector<double> values,
                                                          const size_t classCount)
        {
        if (classCount < 2 || values.size() < classCount)
            {
            return {};
            }
        std::ranges::sort(values);
        const size_t valueCount = values.size();

        // For the best split of the first endIndex values into classIndex classes,
        // lowerClassLimits holds the 1-based index where the last class starts and
        // varianceCombinations holds that split's total within-class variance.
        std::vector<std::vector<size_t>> lowerClassLimits(valueCount + 1,
                                                          std::vector<size_t>(classCount + 1, 0));
        std::vector<std::vector<double>> varianceCombinations(
            valueCount + 1,
            std::vector<double>(classCount + 1, std::numeric_limits<double>::max()));

        for (size_t classIndex = 1; classIndex <= classCount; ++classIndex)
            {
            lowerClassLimits[1][classIndex] = 1;
            varianceCombinations[1][classIndex] = 0.0;
            }

        for (size_t endIndex = 2; endIndex <= valueCount; ++endIndex)
            {
            double sum = 0.0;
            double sumOfSquares = 0.0;
            double count = 0.0;
            double variance = 0.0;
            for (size_t offset = 1; offset <= endIndex; ++offset)
                {
                const size_t startIndex = endIndex - offset + 1;
                const double value = values[startIndex - 1];
                count += 1.0;
                sum += value;
                sumOfSquares += value * value;
                variance = sumOfSquares - ((sum * sum) / count);
                const size_t priorIndex = startIndex - 1;
                if (priorIndex != 0)
                    {
                    for (size_t classIndex = 2; classIndex <= classCount; ++classIndex)
                        {
                        const double candidate =
                            variance + varianceCombinations[priorIndex][classIndex - 1];
                        if (varianceCombinations[endIndex][classIndex] >= candidate)
                            {
                            lowerClassLimits[endIndex][classIndex] = startIndex;
                            varianceCombinations[endIndex][classIndex] = candidate;
                            }
                        }
                    }
                }
            lowerClassLimits[endIndex][1] = 1;
            varianceCombinations[endIndex][1] = variance;
            }

        // walk the class limits back from the top class to recover the boundaries
        std::vector<double> breaks(classCount + 1, 0.0);
        breaks[classCount] = values[valueCount - 1];
        breaks[0] = values[0];
        size_t index = valueCount;
        for (size_t classIndex = classCount; classIndex >= 2; --classIndex)
            {
            const size_t limit = lowerClassLimits[index][classIndex];
            if (limit < 2)
                {
                return {};
                }
            breaks[classIndex - 1] = values[limit - 2];
            index = limit - 1;
            }

        return breaks;
        }

    //----------------------------------------------------------------
    bool ChoroplethMap::BuildClassifiedColors(const std::vector<double>& values)
        {
        if (m_colorSpectrum.size() < 2)
            {
            return false;
            }

        std::vector<double> finiteValues;
        finiteValues.reserve(values.size());
        for (const auto value : values)
            {
            if (std::isfinite(value))
                {
                finiteValues.push_back(value);
                }
            }
        if (finiteValues.size() < 3)
            {
            return false;
            }
        if (m_classificationMethod == ClassificationMethod::JenksNaturalBreaks &&
            finiteValues.size() > MAX_JENKS_VALUE_COUNT)
            {
            wxLogWarning(_(L"Choropleth value column has %zu values; natural-breaks "
                           L"classification is skipped above %zu."),
                         finiteValues.size(), MAX_JENKS_VALUE_COUNT);
            return false;
            }

        const size_t requestedClasses =
            std::clamp<size_t>(m_classCount, 2, std::min<size_t>(finiteValues.size(), 12));

        std::vector<double> breaks;
        if (m_classificationMethod == ClassificationMethod::JenksNaturalBreaks)
            {
            breaks = JenksNaturalBreaks(finiteValues, requestedClasses);
            }
        if (breaks.size() < 2)
            {
            return false;
            }

        // drop boundaries the data does not actually separate; a run of equal
        // values cannot be split into two classes
        breaks.erase(std::unique(breaks.begin(), breaks.end()), breaks.end());
        if (breaks.size() < 3)
            {
            return false;
            }
        const size_t classCount = breaks.size() - 1;

        // one color per class, sampled evenly across the color scheme
        Colors::ColorBrewer classBrewer;
        classBrewer.SetColorScale(m_colorSpectrum.cbegin(), m_colorSpectrum.cend());
        std::vector<double> classIndices(classCount, 0.0);
        for (size_t classIndex = 0; classIndex < classCount; ++classIndex)
            {
            classIndices[classIndex] = static_cast<double>(classIndex);
            }
        std::vector<wxColour> classColors = classBrewer.BrewColors(classIndices);
        if (classColors.size() != classCount)
            {
            return false;
            }

        m_classBreaks = std::move(breaks);
        m_classColors = std::move(classColors);
        m_valueRange = { m_classBreaks.front(), m_classBreaks.back() };

        const auto classForValue = [this, classCount](const double value) -> size_t
        {
            for (size_t classIndex = 1; classIndex < classCount; ++classIndex)
                {
                if (value <= m_classBreaks[classIndex])
                    {
                    return classIndex - 1;
                    }
                }
            return classCount - 1;
        };

        m_regionColors.assign(values.size(), wxColour{});
        for (size_t row = 0; row < values.size(); ++row)
            {
            if (std::isfinite(values[row]))
                {
                m_regionColors[row] = m_classColors[classForValue(values[row])];
                }
            }
        m_isClassified = true;
        return true;
        }

    //----------------------------------------------------------------
    wxString ChoroplethMap::BuildRegionLabel(const size_t row) const
        {
        if (m_geoData == nullptr || row >= m_geoData->GetRowCount())
            {
            return wxString{};
            }

        const wxString regionName = m_geoData->GetRegionGeometry(row).m_name;

        if (m_labelDisplay == BinLabelDisplay::NoDisplay)
            {
            return wxString{};
            }
        if (m_labelDisplay == BinLabelDisplay::BinName || !m_hasValues)
            {
            return regionName;
            }

        // the region's mapped value as text, plus a percentage where one applies
        wxString valueStr;
        wxString percentStr;
        if (m_isCategorical)
            {
            const auto categoricalColumn = m_geoData->GetCategoricalColumn(m_valueColumnName);
            if (categoricalColumn != m_geoData->GetCategoricalColumns().cend())
                {
                const auto code = categoricalColumn->GetValue(row);
                valueStr = categoricalColumn->GetLabelFromID(code);
                if (const auto foundCount = m_categoryRowCounts.find(code);
                    foundCount != m_categoryRowCounts.cend() && m_categorizedRegionCount > 0)
                    {
                    percentStr = wxNumberFormatter::ToString(
                        safe_divide<double>(foundCount->second, m_categorizedRegionCount) * 100.0,
                        1, Settings::GetDefaultNumberFormat());
                    }
                }
            }
        else
            {
            const auto continuousColumn = m_geoData->GetContinuousColumn(m_valueColumnName);
            if (continuousColumn != m_geoData->GetContinuousColumns().cend())
                {
                const auto value = continuousColumn->GetValue(row);
                if (std::isfinite(value))
                    {
                    valueStr =
                        wxNumberFormatter::ToString(value, 6, Settings::GetDefaultNumberFormat());
                    if (m_valueTotal != 0.0)
                        {
                        percentStr = wxNumberFormatter::ToString(
                            safe_divide<double>(value, m_valueTotal) * 100.0, 1,
                            Settings::GetDefaultNumberFormat());
                        }
                    }
                }
            }

        if (valueStr.empty())
            {
            return regionName;
            }

        switch (m_labelDisplay)
            {
        case BinLabelDisplay::BinValue:
            return valueStr;
        case BinLabelDisplay::BinNameAndValue:
            return wxString::Format(L"%s (%s)", regionName, valueStr);
        case BinLabelDisplay::BinPercentage:
            return percentStr.empty() ? valueStr : percentStr + L"%";
        case BinLabelDisplay::BinNameAndPercentage:
            return percentStr.empty() ? wxString::Format(L"%s (%s)", regionName, valueStr) :
                                        wxString::Format(L"%s (%s%%)", regionName, percentStr);
        case BinLabelDisplay::BinValueAndPercentage:
            return percentStr.empty() ? valueStr :
                                        wxString::Format(L"%s (%s%%)", valueStr, percentStr);
        default:
            return regionName;
            }
        }

    //----------------------------------------------------------------
    void ChoroplethMap::PrepareProjection()
        {
        m_projLon0 = wxDegToRad((m_dataBounds.m_minLongitude + m_dataBounds.m_maxLongitude) / 2.0);
        m_projLat0 = wxDegToRad((m_dataBounds.m_minLatitude + m_dataBounds.m_maxLatitude) / 2.0);

        // resolve Automatic from how much of the globe the data covers
        m_effectiveProjection = m_projection;
        if (m_effectiveProjection == MapProjection::Automatic)
            {
            const double latSpan = m_dataBounds.GetHeight();
            const bool straddlesEquator =
                (m_dataBounds.m_minLatitude < 0.0 && m_dataBounds.m_maxLatitude > 0.0);
            if (latSpan >= 90.0 || (straddlesEquator && latSpan >= 20.0))
                {
                m_effectiveProjection = MapProjection::EqualEarth;
                }
            else if (latSpan <= 6.0 && m_dataBounds.GetWidth() <= 6.0)
                {
                m_effectiveProjection = MapProjection::Equirectangular;
                }
            else
                {
                m_effectiveProjection = MapProjection::AlbersEqualAreaConic;
                }
            }

        // for the equirectangular case, longitude shrinks toward the poles
        m_lonScale = std::cos(m_projLat0);
        if (!std::isfinite(m_lonScale) || m_lonScale <= 0.0)
            {
            m_lonScale = 1.0;
            }

        // for Albers, put the standard parallels one sixth in from the north and
        // south edges of the data
        const double lat1 =
            wxDegToRad(m_dataBounds.m_minLatitude + (m_dataBounds.GetHeight() / 6.0));
        const double lat2 =
            wxDegToRad(m_dataBounds.m_maxLatitude - (m_dataBounds.GetHeight() / 6.0));
        m_albersN = (std::sin(lat1) + std::sin(lat2)) / 2.0;
        // aAn n near zero means the parallels straddle the equator symmetrically and
        // the cone degenerates. Nudge it so the math stays finite.
        if (std::fabs(m_albersN) < 1e-6)
            {
            m_albersN = (m_albersN < 0.0) ? -1e-6 : 1e-6;
            }
        m_albersC = (std::cos(lat1) * std::cos(lat1)) + (2.0 * m_albersN * std::sin(lat1));
        const double rho0Arg = m_albersC - (2.0 * m_albersN * std::sin(m_projLat0));
        m_albersRho0 = (rho0Arg > 0.0) ? (std::sqrt(rho0Arg) / m_albersN) : 0.0;
        }

    //----------------------------------------------------------------
    std::pair<double, double> ChoroplethMap::Project(const Data::GeoCoordinate& coord) const
        {
        const double lon = wxDegToRad(coord.m_longitude);
        const double lat = wxDegToRad(coord.m_latitude);

        if (m_effectiveProjection == MapProjection::AlbersEqualAreaConic)
            {
            const double rhoArg = m_albersC - (2.0 * m_albersN * std::sin(lat));
            const double rho = (rhoArg > 0.0) ? (std::sqrt(rhoArg) / m_albersN) : 0.0;
            const double theta = m_albersN * (lon - m_projLon0);
            return { rho * std::sin(theta), m_albersRho0 - (rho * std::cos(theta)) };
            }
        if (m_effectiveProjection == MapProjection::EqualEarth)
            {
            constexpr double a1{ 1.340264 };
            constexpr double a2{ -0.081106 };
            constexpr double a3{ 0.000893 };
            constexpr double a4{ 0.003796 };
            const double sqrt3{ std::sqrt(3.0) };
            const double parametricLat = std::asin((sqrt3 / 2.0) * std::sin(lat));
            const double p2 = parametricLat * parametricLat;
            const double p6 = p2 * p2 * p2;
            const double denom =
                3.0 * ((9.0 * a4 * p6 * p2) + (7.0 * a3 * p6) + (3.0 * a2 * p2) + a1);
            const double xVal =
                (2.0 * sqrt3 * (lon - m_projLon0) * std::cos(parametricLat)) / denom;
            const double yVal = (a4 * p6 * parametricLat * p2) + (a3 * p6 * parametricLat) +
                                (a2 * p2 * parametricLat) + (a1 * parametricLat);
            return { xVal, yVal };
            }
        // Equirectangular
        return { (lon - m_projLon0) * m_lonScale, lat - m_projLat0 };
        }

    //----------------------------------------------------------------
    wxPoint ChoroplethMap::GeoToScreen(const Data::GeoCoordinate& coord) const
        {
        const auto [planeX, planeY] = Project(coord);
        // plane y increases northward; screen y increases downward, so flip it
        const double screenX = m_geoOrigin.x + ((planeX - m_planeMin.first) * m_geoScale);
        const double screenY = m_geoOrigin.y + ((m_planeMax.second - planeY) * m_geoScale);
        // a non-finite projection, or a point well outside the fitted extent, would
        // overflow the cast to int (undefined behavior), so keep both axes to a
        // wide but bounded pixel range
        constexpr double screenLimit{ 1'000'000.0 };
        return { static_cast<int>(
                     std::isfinite(screenX) ? std::clamp(screenX, -screenLimit, screenLimit) : 0.0),
                 static_cast<int>(std::isfinite(screenY) ?
                                      std::clamp(screenY, -screenLimit, screenLimit) :
                                      0.0) };
        }

    //----------------------------------------------------------------
    void ChoroplethMap::RecalcSizes(wxDC & dc)
        {
        if (m_geoData == nullptr || m_geoData->GetGeometries().empty())
            {
            return;
            }

        Graph2D::RecalcSizes(dc);

        m_dataBounds = m_geoData->GetGeoBoundingBox();
        if (!m_dataBounds.IsOk())
            {
            return;
            }

        PrepareProjection();

        // project every ring point once to find the extent of the projected shapes
        // (a conic projection curves, so the min/max are not simply the lat/long corners)
        m_planeMin = { std::numeric_limits<double>::max(), std::numeric_limits<double>::max() };
        m_planeMax = { std::numeric_limits<double>::lowest(),
                       std::numeric_limits<double>::lowest() };
        const auto growPlane = [this](const Data::GeoCoordinate& coord)
        {
            const auto [planeX, planeY] = Project(coord);
            if (!std::isfinite(planeX) || !std::isfinite(planeY))
                {
                return;
                }
            m_planeMin.first = std::min(m_planeMin.first, planeX);
            m_planeMin.second = std::min(m_planeMin.second, planeY);
            m_planeMax.first = std::max(m_planeMax.first, planeX);
            m_planeMax.second = std::max(m_planeMax.second, planeY);
        };
        for (const auto& region : m_geoData->GetGeometries())
            {
            for (const auto& geoPolygon : region.m_polygons)
                {
                for (const auto& coord : geoPolygon.m_outerBoundary)
                    {
                    growPlane(coord);
                    }
                // holes are projected and drawn too, so the fitted extent has to
                // cover them; otherwise, a hole vertex outside the outer rings maps
                // far off the plot
                for (const auto& innerRing : geoPolygon.m_innerBoundaries)
                    {
                    for (const auto& coord : innerRing)
                        {
                        growPlane(coord);
                        }
                    }
                }
            }
        if (m_planeMin.first > m_planeMax.first || m_planeMin.second > m_planeMax.second)
            {
            return;
            }

        wxRect plotRect = GetPlotAreaBoundingBox();

        // The graticule's coordinate labels sit in a strip taken off the top and
        // left of the plot area. The map is fitted into what is left.
        wxSize graticuleGutter{ 0, 0 };
        if (m_showGraticule)
            {
            graticuleGutter = MeasureGraticuleGutter(dc);
            if (plotRect.GetWidth() > graticuleGutter.GetWidth() * 4 &&
                plotRect.GetHeight() > graticuleGutter.GetHeight() * 4)
                {
                plotRect.x += graticuleGutter.GetWidth();
                plotRect.width -= graticuleGutter.GetWidth();
                plotRect.y += graticuleGutter.GetHeight();
                plotRect.height -= graticuleGutter.GetHeight();
                }
            else
                {
                graticuleGutter = wxSize{ 0, 0 };
                }
            }

        const double planeWidth = std::max(m_planeMax.first - m_planeMin.first, 1e-9);
        const double planeHeight = std::max(m_planeMax.second - m_planeMin.second, 1e-9);

        // one scale for both axes keeps the projected shapes undistorted
        m_geoScale = std::min(safe_divide<double>(plotRect.GetWidth(), planeWidth),
                              safe_divide<double>(plotRect.GetHeight(), planeHeight));

        const double drawnWidth = planeWidth * m_geoScale;
        const double drawnHeight = planeHeight * m_geoScale;
        m_geoOrigin = {
            plotRect.GetLeft() + static_cast<int>((plotRect.GetWidth() - drawnWidth) / 2.0),
            plotRect.GetTop() + static_cast<int>((plotRect.GetHeight() - drawnHeight) / 2.0)
        };

        // color for regions that are not data-shaded: the high end of the color scheme
        const wxColour flatFillColor =
            (GetColorScheme() != nullptr && !GetColorScheme()->GetColors().empty()) ?
                GetColorScheme()->GetColors().back() :
                GetBrush().GetColour();

        for (size_t row = 0; row < m_geoData->GetRowCount(); ++row)
            {
            const auto& region = m_geoData->GetRegionGeometry(row);
            const wxString regionLabelText = BuildRegionLabel(row);

            wxColour fillColor{ flatFillColor };
            bool regionHasNoData{ false };
            if (m_hasValues && row < m_regionColors.size())
                {
                if (m_regionColors[row].IsOk())
                    {
                    fillColor = m_regionColors[row];
                    }
                else
                    {
                    fillColor = m_noDataColor;
                    regionHasNoData = true;
                    }
                }
            else if (m_hasValues)
                {
                fillColor = m_noDataColor;
                regionHasNoData = true;
                }

            // a no-data region uses the chosen fill style, so it can be hatched
            // rather than filled flat
            wxBrush regionBrush{ fillColor };
            if (regionHasNoData)
                {
                regionBrush.SetStyle(m_noDataFillStyle);
                }

            // every ring of the region goes into one selectable object, so a click
            // anywhere on the region selects it and anchors a single name label on
            // the region's own bounding box
            GraphItems::GraphItemInfo regionInfo;
            regionInfo.Pen(GetPen())
                .Brush(regionBrush)
                .Selectable(true)
                .Text(regionLabelText)
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor());
            auto regionObject = std::make_unique<ChoroplethRegion>(regionInfo);

            for (const auto& geoPolygon : region.m_polygons)
                {
                if (geoPolygon.m_outerBoundary.size() >= 3)
                    {
                    std::vector<wxPoint> outerScreen;
                    outerScreen.reserve(geoPolygon.m_outerBoundary.size());
                    for (const auto& coord : geoPolygon.m_outerBoundary)
                        {
                        outerScreen.push_back(GeoToScreen(coord));
                        }
                    regionObject->AddOuterRing(std::move(outerScreen));
                    }
                for (const auto& innerRing : geoPolygon.m_innerBoundaries)
                    {
                    if (innerRing.size() < 3)
                        {
                        continue;
                        }
                    std::vector<wxPoint> holeScreen;
                    holeScreen.reserve(innerRing.size());
                    for (const auto& coord : innerRing)
                        {
                        holeScreen.push_back(GeoToScreen(coord));
                        }
                    regionObject->AddHoleRing(std::move(holeScreen));
                    }
                }

            if (regionObject->HasRings())
                {
                AddObject(std::move(regionObject));
                }

            if (m_showLabels && !regionLabelText.empty() && region.m_boundingBox.IsOk())
                {
                auto regionLabel = std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo{ regionLabelText }
                        .Pen(wxNullPen)
                        .Scaling(GetScaling())
                        .DPIScaling(GetDPIScaleFactor())
                        .Anchoring(Anchoring::Center)
                        .AnchorPoint(GeoToScreen(region.m_boundingBox.GetCenter()))
                        .FontColor(GetLeftYAxis().GetFontColor()));
                AddObject(std::move(regionLabel));
                }
            }

        if (m_showGraticule)
            {
            AddGraticule(plotRect, graticuleGutter);
            }

        if (!m_symbolColumnName.empty())
            {
            AddProportionalSymbols(plotRect);
            }
        }

    //----------------------------------------------------------------
    wxString ChoroplethMap::GraticuleDegreeText(const double degrees, const wchar_t* negSuffix,
                                                const wchar_t* posSuffix, const bool wraps)
        {
        const double magnitude = std::fabs(degrees);
        wxString suffix;
        if (magnitude > 1e-6 && (!wraps || magnitude < 180.0 - 1e-6))
            {
            suffix = (degrees < 0.0) ? negSuffix : posSuffix;
            }
        return wxNumberFormatter::ToString(magnitude, 4,
                                           wxNumberFormatter::Style::Style_NoTrailingZeroes) +
               L"°" + suffix;
        }

    //----------------------------------------------------------------
    double ChoroplethMap::NiceNumberFloor(const double value)
        {
        if (!std::isfinite(value) || value <= 0.0)
            {
            return 0.0;
            }
        double powerOfTen = std::pow(10.0, std::floor(std::log10(value)));
        double fraction = safe_divide<double>(value, powerOfTen);
        // a floating-point log10 can land just under an exact power of ten
        if (fraction >= 10.0)
            {
            fraction /= 10.0;
            powerOfTen *= 10.0;
            }
        const double niceFraction = (fraction >= 5.0) ? 5.0 : (fraction >= 2.0) ? 2.0 : 1.0;
        return niceFraction * powerOfTen;
        }

    //----------------------------------------------------------------
    wxSize ChoroplethMap::MeasureGraticuleGutter(wxDC & dc) const
        {
        // sized to a label about as wide as the widest coordinate can get
        GraphItems::Label probe(
            GraphItems::GraphItemInfo{ GraticuleDegreeText(179.75, L"W", L"E", true) }
                .Scaling(GetScaling())
                .DPIScaling(GetDPIScaleFactor())
                .Padding(2, 4, 2, 4));
        const wxRect probeBox = probe.GetBoundingBox(dc);
        const int gap = static_cast<int>(4 * GetScaling());
        return { probeBox.GetWidth() + gap, probeBox.GetHeight() + gap };
        }

    //----------------------------------------------------------------
    void ChoroplethMap::AddGraticule(const wxRect& mapRect, const wxSize& gutterSize)
        {
        const double latSpan = m_dataBounds.GetHeight();
        const double lonSpan = m_dataBounds.GetWidth();
        const double widerSpan = std::max(latSpan, lonSpan);
        if (!std::isfinite(widerSpan) || widerSpan <= 0.0)
            {
            return;
            }

        // a spacing (in degrees) that puts a handful of lines across the data extent
        constexpr std::array<double, 9> niceSteps = { 30.0, 20.0, 15.0, 10.0, 5.0,
                                                      2.0,  1.0,  0.5,  0.25 };
        double step = niceSteps.back();
        for (const double candidate : niceSteps)
            {
            if (safe_divide<double>(widerSpan, candidate) >= 3.0)
                {
                step = candidate;
                break;
                }
            }

        wxColour lineColor = GetLeftYAxis().GetFontColor();
        if (!lineColor.IsOk())
            {
            lineColor = Colors::ColorBrewer::GetColor(Colors::Color::Black);
            }
        // faint, so the region shading stays dominant
        const wxPen graticulePen{ Colors::ColorContrast::ChangeOpacity(lineColor, 55), 1 };

        auto graticule = std::make_unique<GraphItems::Lines>(graticulePen, GetScaling());
        graticule->SetSelectable(false);
        // clip to the whole plot area, not just the map, so the leader lines that
        // run each grid line out to its label in the gutter are kept
        graticule->SetClippingRect(GetPlotAreaBoundingBox());

        // each parallel or meridian is projected as a polyline of short segments, so
        // it follows the curve a conic or pseudocylindrical projection gives it
        constexpr int sampleCount{ 64 };

        // The labels sit in the gutter just outside the map, each centered on its
        // line. Longitude labels along a row above the map, latitude labels down a
        // column to its left.
        const int lonLabelY = mapRect.GetTop() - (gutterSize.GetHeight() / 2);
        const int latLabelX = mapRect.GetLeft() - (gutterSize.GetWidth() / 2);
        const auto clampX = [&mapRect](const int xVal) -> int
        { return std::clamp(xVal, mapRect.GetLeft(), mapRect.GetRight()); };
        const auto clampY = [&mapRect](const int yVal) -> int
        { return std::clamp(yVal, mapRect.GetTop(), mapRect.GetBottom()); };

        struct EdgeLabel
            {
            wxString m_text;
            wxPoint m_anchor;
            };

        std::vector<EdgeLabel> edgeLabels;

        // meridians: constant longitude, latitude sweeping the visible extent
        const double firstLon = std::ceil(m_dataBounds.m_minLongitude / step) * step;
        const int meridianCount =
            static_cast<int>(std::floor((m_dataBounds.m_maxLongitude - firstLon) / step)) + 1;
        for (int lineIndex = 0; lineIndex < meridianCount; ++lineIndex)
            {
            const double lon = firstLon + (lineIndex * step);
            wxPoint previous;
            for (int sample = 0; sample <= sampleCount; ++sample)
                {
                const double lat = m_dataBounds.m_minLatitude +
                                   (latSpan * safe_divide<double>(sample, sampleCount));
                // a GeoCoordinate is { longitude, latitude }
                const wxPoint current = GeoToScreen(Data::GeoCoordinate{ lon, lat });
                if (sample > 0)
                    {
                    graticule->AddLine(previous, current);
                    }
                previous = current;
                }
            // The final point is the max-latitude (northern) end. GeoToScreen flips y.
            // Run the line up through the gutter to meet its label.
            graticule->AddLine(previous, wxPoint{ previous.x, lonLabelY });
            edgeLabels.push_back({ GraticuleDegreeText(lon, L"W", L"E", true),
                                   wxPoint{ clampX(previous.x), lonLabelY } });
            }

        // parallels: constant latitude, longitude sweeping the visible extent
        const double firstLat = std::ceil(m_dataBounds.m_minLatitude / step) * step;
        const int parallelCount =
            static_cast<int>(std::floor((m_dataBounds.m_maxLatitude - firstLat) / step)) + 1;
        for (int lineIndex = 0; lineIndex < parallelCount; ++lineIndex)
            {
            const double lat = firstLat + (lineIndex * step);
            wxPoint previous;
            wxPoint westEnd;
            for (int sample = 0; sample <= sampleCount; ++sample)
                {
                const double lon = m_dataBounds.m_minLongitude +
                                   (lonSpan * safe_divide<double>(sample, sampleCount));
                const wxPoint current = GeoToScreen(Data::GeoCoordinate{ lon, lat });
                if (sample == 0)
                    {
                    westEnd = current;
                    }
                else
                    {
                    graticule->AddLine(previous, current);
                    }
                previous = current;
                }
            // Sample 0 is the min-longitude (western) end. Run the line out through
            // the gutter to meet its label.
            graticule->AddLine(westEnd, wxPoint{ latLabelX, westEnd.y });
            edgeLabels.push_back({ GraticuleDegreeText(lat, L"S", L"N", false),
                                   wxPoint{ latLabelX, clampY(westEnd.y) } });
            }

        if (!graticule->GetLines().empty())
            {
            AddObject(std::move(graticule));
            }

        for (const auto& edgeLabel : edgeLabels)
            {
            // the background masks the leader line where it would cross the text
            AddObject(std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo{ edgeLabel.m_text }
                    .Pen(wxNullPen)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .Anchoring(Anchoring::Center)
                    .AnchorPoint(edgeLabel.m_anchor)
                    .FontColor(GetLeftYAxis().GetFontColor())
                    .FontBackgroundColor(GetPlotOrCanvasColor())));
            }
        }

    //----------------------------------------------------------------
    void ChoroplethMap::AddProportionalSymbols(const wxRect& mapRect)
        {
        if (m_geoData == nullptr || m_symbolColumnName.empty())
            {
            return;
            }
        const auto symbolColumn = m_geoData->GetContinuousColumn(m_symbolColumnName);
        if (symbolColumn == m_geoData->GetContinuousColumns().cend())
            {
            return;
            }

        // The circle area is made proportional to the value, so the radius runs with
        // the square root of it. Only a finite value above zero gets a circle.
        double maxValue{ 0.0 };
        bool haveValue{ false };
        for (const auto value : symbolColumn->GetValues())
            {
            if (std::isfinite(value) && value > 0.0)
                {
                maxValue = std::max(maxValue, value);
                haveValue = true;
                }
            }
        if (!haveValue || maxValue <= 0.0)
            {
            return;
            }

        // The largest circle covers a fraction of the shorter side of the map, held
        // to a usable on-screen range. The smallest keeps a low value visible.
        const double shorterSide = std::min(mapRect.GetWidth(), mapRect.GetHeight());
        const double maxRadiusPx = std::clamp(shorterSide * 0.09, ScaleToScreenAndCanvas(6.0),
                                              ScaleToScreenAndCanvas(60.0));
        const double minRadiusPx = std::min(ScaleToScreenAndCanvas(3.0), maxRadiusPx);

        const auto radiusForValue = [minRadiusPx, maxRadiusPx, maxValue](const double val) -> double
        {
            return std::max(minRadiusPx,
                            maxRadiusPx * std::sqrt(safe_divide<double>(val, maxValue)));
        };

        const wxBrush symbolBrush{ Colors::ColorContrast::ChangeOpacity(m_symbolColor, 150) };
        const wxPen symbolPen{ m_symbolColor, 1 };

        for (size_t row = 0; row < m_geoData->GetRowCount(); ++row)
            {
            const double value = symbolColumn->GetValue(row);
            if (!std::isfinite(value) || value <= 0.0)
                {
                continue;
                }
            const auto& region = m_geoData->GetRegionGeometry(row);
            if (!region.m_boundingBox.IsOk())
                {
                continue;
                }

            const double radiusPx = radiusForValue(value);
            const auto radiusDIPs = static_cast<size_t>(
                std::max<double>(1.0, std::round(DownscaleFromScreenAndCanvas(radiusPx))));

            auto symbol = std::make_unique<GraphItems::Point2D>(
                GraphItems::GraphItemInfo{}
                    .Brush(symbolBrush)
                    .Pen(symbolPen)
                    .Selectable(false)
                    .Scaling(GetScaling())
                    .DPIScaling(GetDPIScaleFactor())
                    .AnchorPoint(GeoToScreen(region.m_boundingBox.GetCenter())),
                radiusDIPs, Icons::IconShape::Circle);
            symbol->SetClippingRect(GetPlotAreaBoundingBox());
            AddObject(std::move(symbol));
            }
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChoroplethMap::CreateLegend(const LegendOptions& options)
        {
        SetLegendInfo(options);
        if (!m_hasValues)
            {
            return nullptr;
            }
        if (m_isCategorical)
            {
            return CreateCategoricalLegend(options);
            }
        if (m_isClassified)
            {
            return CreateClassifiedLegend(options);
            }
        if (m_colorSpectrum.empty())
            {
            return nullptr;
            }

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{
                // spaces on the blank lines keep the SVG exporter from dropping them
                wxString::Format(L"%s\n \n \n%s",
                                 wxNumberFormatter::ToString(m_valueRange.second, 6,
                                                             Settings::GetDefaultNumberFormat()),
                                 wxNumberFormatter::ToString(m_valueRange.first, 6,
                                                             Settings::GetDefaultNumberFormat())) }
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs() * 1.5)
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor()));

        if (options.IsIncludingHeader())
            {
            const wxString headerText =
                options.GetTitle().empty() ? m_valueColumnName : options.GetTitle();
            legend->SetText(wxString::Format(L"%s\n", headerText) + legend->GetText());
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }

        // the gradient icon runs top-to-bottom, so hand it the colors high-to-low
        std::vector<wxColour> legendSpectrum{ m_colorSpectrum };
        std::ranges::reverse(legendSpectrum);
        legend->GetLegendIcons().emplace_back(legendSpectrum);

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChoroplethMap::CreateCategoricalLegend(
        const LegendOptions& options)
        {
        if (m_categoryLegend.empty())
            {
            return nullptr;
            }

        wxString legendText;
        for (const auto& category : m_categoryLegend)
            {
            legendText += category.first + L"\n";
            }
        legendText.Trim();

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{ legendText }
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor()));

        for (const auto& category : m_categoryLegend)
            {
            legend->GetLegendIcons().emplace_back(
                Icons::IconShape::Square,
                wxPen{ Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()) },
                wxBrush{ category.second });
            }

        if (options.IsIncludingHeader())
            {
            const wxString headerText =
                options.GetTitle().empty() ? m_valueColumnName : options.GetTitle();
            legend->SetText(headerText + L"\n" + legend->GetText());
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<GraphItems::Label> ChoroplethMap::CreateClassifiedLegend(
        const LegendOptions& options)
        {
        if (m_classBreaks.size() < 2 || m_classColors.empty())
            {
            return nullptr;
            }

        const auto formatValue = [](const double value)
        { return wxNumberFormatter::ToString(value, 6, Settings::GetDefaultNumberFormat()); };

        wxString legendText;
        for (size_t classIndex = 0; classIndex < m_classColors.size(); ++classIndex)
            {
            legendText += formatValue(m_classBreaks[classIndex]) + L" – " +
                          formatValue(m_classBreaks[classIndex + 1]) + L"\n";
            }
        legendText.Trim();

        auto legend = std::make_unique<GraphItems::Label>(
            GraphItems::GraphItemInfo{ legendText }
                .Padding(0, 0, 0, GraphItems::Label::GetMinLegendWidthDIPs())
                .DPIScaling(GetDPIScaleFactor())
                .Anchoring(Anchoring::TopLeftCorner)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor()));

        for (const auto& classColor : m_classColors)
            {
            legend->GetLegendIcons().emplace_back(
                Icons::IconShape::Square,
                wxPen{ Colors::ColorContrast::BlackOrWhiteContrast(GetPlotOrCanvasColor()) },
                wxBrush{ classColor });
            }

        if (options.IsIncludingHeader())
            {
            const wxString headerText =
                options.GetTitle().empty() ? m_valueColumnName : options.GetTitle();
            legend->SetText(headerText + L"\n" + legend->GetText());
            legend->GetHeaderInfo()
                .Enable(true)
                .LabelAlignment(TextAlignment::FlushLeft)
                .FontColor(GetLeftYAxis().GetFontColor());
            }

        AddReferenceLinesAndAreasToLegend(*legend);
        AdjustLegendSettings(*legend, options.GetPlacementHint());
        return legend;
        }

    //----------------------------------------------------------------
    std::unique_ptr<ChoroplethLegend> ChoroplethMap::CreateChoroplethLegend(
        const LegendOptions& options)
        {
        if (m_geoData == nullptr || m_symbolColumnName.empty())
            {
            return nullptr;
            }
        const auto symbolColumn = m_geoData->GetContinuousColumn(m_symbolColumnName);
        if (symbolColumn == m_geoData->GetContinuousColumns().cend())
            {
            return nullptr;
            }

        double maxValue{ 0.0 };
        bool haveValue{ false };
        for (const auto value : symbolColumn->GetValues())
            {
            if (std::isfinite(value) && value > 0.0)
                {
                maxValue = std::max(maxValue, value);
                haveValue = true;
                }
            }
        if (!haveValue || maxValue <= 0.0)
            {
            return nullptr;
            }

        // a few nice round values, largest first
        std::vector<ChoroplethLegend::Entry> entries;
        const double topValue = NiceNumberFloor(maxValue);
        for (const double keyValue :
             { topValue, NiceNumberFloor(topValue / 2.0), NiceNumberFloor(topValue / 8.0) })
            {
            if (keyValue <= 0.0 || std::ranges::any_of(entries, [keyValue](const auto& existing)
                                                       { return existing.m_value == keyValue; }))
                {
                continue;
                }
            entries.push_back({ keyValue, wxNumberFormatter::ToString(
                                              keyValue, 6, Settings::GetDefaultNumberFormat()) });
            }
        if (entries.empty())
            {
            return nullptr;
            }

        wxColour textColor = GetLeftYAxis().GetFontColor();
        if (!textColor.IsOk())
            {
            textColor = Colors::ColorBrewer::GetColor(Colors::Color::Black);
            }

        auto legend = std::make_unique<ChoroplethLegend>(
            GraphItems::GraphItemInfo{}.DPIScaling(GetDPIScaleFactor()).Pen(wxNullPen));
        legend->SetScaleLegend(CreateLegend(options));
        legend->SetEntries(std::move(entries));
        if (options.IsIncludingHeader())
            {
            legend->SetHeading(m_symbolColumnName);
            }
        legend->SetOutlineColor(m_symbolColor);
        legend->SetTextColor(textColor);

        // size the column to the content and let it sit in its own cell
        if (GetCanvas() != nullptr)
            {
            const auto hint = options.GetPlacementHint();
            if (hint == LegendCanvasPlacementHint::RightOfGraph ||
                hint == LegendCanvasPlacementHint::LeftOfGraph)
                {
                legend->SetCanvasWidthProportion(GetCanvas()->CalcMinWidthProportion(*legend));
                legend->SetPageHorizontalAlignment(hint == LegendCanvasPlacementHint::RightOfGraph ?
                                                       PageHorizontalAlignment::RightAligned :
                                                       PageHorizontalAlignment::LeftAligned);
                legend->GetGraphItemInfo().CanvasPadding(4, 4, 4, 4).FixedWidthOnCanvas(true);
                }
            }

        return legend;
        }

    //----------------------------------------------------------------
    void ChoroplethMap::SetAutoAccessibilityAttributes()
        {
        if (m_geoData == nullptr || m_geoData->GetGeometries().empty())
            {
            return;
            }

        wxString description = _(L"A choropleth map");
        AddAccessibilityAttribute(description, GetTitle().GetText(), L": ");
        AddAccessibilityAttribute(description, GetSubtitle().GetText(), L", ");

        description +=
            L". " + wxString::Format(_(L"%zu regions"), m_geoData->GetGeometries().size());
        if (m_isCategorical)
            {
            description += L". " + wxString::Format(_(L"Shaded by %s, in %zu categories"),
                                                    m_valueColumnName, m_categoryLegend.size());
            }
        else if (m_isClassified)
            {
            description +=
                L". " +
                wxString::Format(
                    _(L"Shaded by %s, in %zu classes from %s to %s"), m_valueColumnName,
                    m_classColors.size(),
                    wxNumberFormatter::ToString(m_valueRange.first, 6,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes),
                    wxNumberFormatter::ToString(m_valueRange.second, 6,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
            }
        else if (m_hasValues)
            {
            description +=
                L". " +
                wxString::Format(
                    _(L"Shaded by %s, ranging from %s to %s"), m_valueColumnName,
                    wxNumberFormatter::ToString(m_valueRange.first, 6,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes),
                    wxNumberFormatter::ToString(m_valueRange.second, 6,
                                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
            }

        if (!m_symbolColumnName.empty())
            {
            description += L". " + wxString::Format(_(L"With proportional symbols sized by %s"),
                                                    m_symbolColumnName);
            }

        AddAccessibilityAttribute(description, GetCaption().GetText(), L". ");
        if (!description.EndsWith(L"."))
            {
            description += L".";
            }

        GetAutoAccessibilityAttributes() =
            wxSVGAttributes{}.Role(_DT(L"img")).AriaLabel(description);
        }
    } // namespace Wisteria::Graphs
