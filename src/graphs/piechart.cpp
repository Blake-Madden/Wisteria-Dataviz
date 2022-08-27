///////////////////////////////////////////////////////////////////////////////
// Name:        wcurveplot.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "piechart.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------------
    std::shared_ptr<Label> PieSlice::CreateMiddleLabel(wxDC& dc,
        const double pieProportion,
        const BinLabelDisplay labelDisplay,
        const std::shared_ptr<const TextReplace> abbreviate /*= nullptr*/)
        {
        const auto angle = m_startAngle + ((m_endAngle - m_startAngle) / 2);
        const auto arcMiddle = GetMiddleOfArc(pieProportion);
        auto pieLabel = std::make_shared<Label>(GetGraphItemInfo());
        switch (labelDisplay)
            {
        case BinLabelDisplay::BinValue:
            pieLabel->SetText(
                wxNumberFormatter::ToString(m_value, 0,
                    Settings::GetDefaultNumberFormat()));
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            pieLabel->SetText(wxString::Format(L"%s%%\n(%s)",
                wxNumberFormatter::ToString((m_percent * 100), 0),
                wxNumberFormatter::ToString(m_value, 0,
                    Settings::GetDefaultNumberFormat())) );
            break;
        case BinLabelDisplay::BinPercentage:
            pieLabel->SetText(wxNumberFormatter::ToString((m_percent * 100), 0) + L"%");
            break;
        case BinLabelDisplay::NoDisplay:
            pieLabel->SetText(wxEmptyString);
            break;
        case BinLabelDisplay::BinName:
            [[fallthrough]];
        default:
            // leave as the name of the slice
            break;
            }

        pieLabel->GetGraphItemInfo().Pen(wxNullPen).
            Scaling(GetScaling()).
            LabelAlignment(TextAlignment::Centered).
            Selectable(true).
            Anchoring(Anchoring::Center).
            AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second)).
            FontColor(ColorContrast::BlackOrWhiteContrast(GetBrush().GetColour()));
        pieLabel->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
        pieLabel->GetHeaderInfo().Enable(false);
        // in case we need to split it to possibly fit
        auto pieLabelSplit = std::make_shared<Label>(*pieLabel);
        // abbreviate if enabled and showing a textual label
        if (abbreviate && labelDisplay == BinLabelDisplay::BinName)
            { pieLabelSplit->SetText((*abbreviate)(pieLabelSplit->GetText())); }
        pieLabelSplit->SplitTextAuto();

        // make it fit in the slice and return it (or null if too small)
        const auto fitLabelToSlice = [this, &dc](auto pieLabel)
            {
            auto points = GetPolygon();
            bool middleLabelIsTooSmall{ false };
            for (;;)
                {
                auto labelBox = pieLabel->GetBoundingBox(dc);
                if (Polygon::IsInsidePolygon(
                        labelBox.GetTopLeft(), &points[0], points.size()) &&
                    Polygon::IsInsidePolygon(
                        labelBox.GetBottomLeft(), &points[0], points.size()) &&
                    Polygon::IsInsidePolygon(
                        labelBox.GetTopRight(), &points[0], points.size()) &&
                    Polygon::IsInsidePolygon(
                        labelBox.GetBottomRight(), &points[0], points.size()))
                    { break; }
                else
                    {
                    const auto currentFontSize = pieLabel->GetFont().GetFractionalPointSize();
                    pieLabel->GetFont().Scale(.95f);
                    // either too small for our taste or couldn't be scaled down anymore
                    if ((pieLabel->GetFont().GetFractionalPointSize() * GetScaling()) <= 6 ||
                        compare_doubles(pieLabel->GetFont().GetFractionalPointSize(),
                            currentFontSize))
                        {
                        middleLabelIsTooSmall = true;
                        break;
                        }
                    }
                }
            return middleLabelIsTooSmall ? nullptr : pieLabel;
            };

        auto scaledPieLabel = fitLabelToSlice(pieLabel);
        if (scaledPieLabel == nullptr)
            { return fitLabelToSlice(pieLabelSplit); }
        else
            { return scaledPieLabel; }
        }

    //----------------------------------------------------------------
    std::shared_ptr<Label> PieSlice::CreateOuterLabel(wxDC& dc)
        { return CreateOuterLabel(dc, m_pieArea); }

    //----------------------------------------------------------------
    std::shared_ptr<Label> PieSlice::CreateOuterLabel(wxDC& dc, const wxRect& pieArea)
        {
        const auto angle = m_startAngle + ((m_endAngle - m_startAngle) / 2);
        const auto arcMiddle = GetMiddleOfArc(1.0, pieArea);
        auto pieLabel = std::make_shared<Label>(GetGraphItemInfo());
        pieLabel->GetGraphItemInfo().Pen(wxNullPen).
            Scaling(GetScaling()).
            Padding(4, 4, 4, 4).
            Selectable(true).
            Anchoring(
                is_within<double>(std::make_pair(0, 90), angle) ?
                    Anchoring::BottomLeftCorner :
                is_within<double>(std::make_pair(90, 180), angle) ?
                    Anchoring::BottomRightCorner :
                is_within<double>(std::make_pair(180, 270), angle) ?
                    Anchoring::TopRightCorner :
                Anchoring::TopLeftCorner).
            AnchorPoint(wxPoint(arcMiddle.first, arcMiddle.second)).
            LabelAlignment(
                (is_within<double>(std::make_pair(0, 90), angle) ||
                 is_within<double>(std::make_pair(270, 360), angle)) ?
                TextAlignment::FlushLeft :
                TextAlignment::FlushRight);
        // outer labels can have headers
        pieLabel->GetHeaderInfo().LabelAlignment(
            (is_within<double>(std::make_pair(0, 90), angle) ||
             is_within<double>(std::make_pair(270, 360), angle)) ?
            TextAlignment::FlushLeft :
            TextAlignment::FlushRight).
            FontColor(GetHeaderInfo().GetFontColor()).
            GetFont().MakeBold();

        return pieLabel;
        }

    //----------------------------------------------------------------
    std::pair<double, double> PieSlice::GetMiddleOfArc(
        const double pieProportion, const wxRect pieArea) const noexcept
        {
        const auto shrinkProportion{ 1 - pieProportion };
        wxRect outerRect{ pieArea };
        outerRect.SetWidth(outerRect.GetWidth() - (outerRect.GetWidth()*shrinkProportion));
        outerRect.SetHeight(outerRect.GetHeight() - (outerRect.GetHeight()*shrinkProportion));
        outerRect.Offset(
            wxPoint(safe_divide(pieArea.GetWidth()-outerRect.GetWidth(), 2),
                    safe_divide(pieArea.GetHeight()-outerRect.GetHeight(), 2)));
        auto midPt = geometry::calc_arc_vertex(
            std::make_pair(outerRect.GetWidth(), outerRect.GetHeight()),
            m_startAngle + ((m_endAngle - m_startAngle) / 2));
        // in case the rect doesn't start at (0, 0), offset the point
        midPt.first += outerRect.GetTopLeft().x;
        midPt.second += outerRect.GetTopLeft().y;
        return midPt;
        }

    //----------------------------------------------------------------
    std::pair<double, double> PieSlice::GetMiddleOfArc(const double pieProportion) const noexcept
        { return GetMiddleOfArc(pieProportion, m_pieArea); }

    //----------------------------------------------------------------
    std::vector<wxPoint> PieSlice::GetPolygon() const noexcept
        {
        std::vector<wxPoint> points;

        auto startSweep = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_startAngle);
        // in case the rect doesn't start at (0, 0), offset the point
        startSweep.first += m_pieArea.GetTopLeft().x;
        startSweep.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(startSweep));

        auto middleSweep1 = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle-m_startAngle) * math_constants::quarter) + m_startAngle);
        middleSweep1.first += m_pieArea.GetTopLeft().x;
        middleSweep1.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(middleSweep1));

        auto middleSweep2 = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle-m_startAngle) * math_constants::half) + m_startAngle);
        middleSweep2.first += m_pieArea.GetTopLeft().x;
        middleSweep2.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(middleSweep2));

        auto middleSweep3 = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle-m_startAngle) * math_constants::three_quarters) + m_startAngle);
        middleSweep3.first += m_pieArea.GetTopLeft().x;
        middleSweep3.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(middleSweep3));

        auto endSweep = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_endAngle);
        endSweep.first += m_pieArea.GetTopLeft().x;
        endSweep.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(endSweep));

        // center of pie
        points.emplace_back(wxPoint(m_pieArea.GetLeft() + (m_pieArea.GetWidth() / 2),
            m_pieArea.GetTop() + (m_pieArea.GetHeight() / 2)));

        return points;
        }

    //----------------------------------------------------------------
    wxRect PieSlice::Draw(wxDC& dc) const
        {
        wxDCBrushChanger bCh(dc, GetBrush());
        wxPen scaledPen(GetPen().IsOk() ? GetPen() : *wxTRANSPARENT_PEN);
        scaledPen.SetWidth(ScaleToScreenAndCanvas(scaledPen.GetWidth()));

        const wxPoint centerPoint{ m_pieArea.GetWidth() / 2 + m_pieArea.GetLeft(),
                                   m_pieArea.GetHeight() / 2 + m_pieArea.GetTop() };

        // Outer arc
        // Note that if the start and end angles are the same, then an entire pie
        // is drawn with the current brush (per wx's docs), which is not what we want;
        // in that case, don't draw the outer arc (which would be zero length anyway).
        if (!compare_doubles(m_startAngle, m_endAngle))
            {
            wxPen scaledArcPen(
                (GetArcPen().has_value() && GetArcPen().value().IsOk()) ?
                 GetArcPen().value() :
                 GetPen().IsOk() ? GetPen() : *wxTRANSPARENT_PEN);
            scaledArcPen.SetWidth(ScaleToScreenAndCanvas(scaledArcPen.GetWidth()));

            wxDCPenChanger pc(dc, scaledArcPen);
            dc.DrawEllipticArc(m_pieArea.GetTopLeft(), m_pieArea.GetSize(),
                               m_startAngle, m_endAngle);
            }
        // line from the pie center to the start of the arc
        auto arcStart = geometry::calc_arc_vertex(std::make_pair(
            m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_startAngle);
        arcStart.first += m_pieArea.GetTopLeft().x;
        arcStart.second += m_pieArea.GetTopLeft().y;
            {
            wxDCPenChanger pc(dc, scaledPen);
            dc.DrawLine(centerPoint, wxPoint(arcStart.first, arcStart.second));
            }
        // line from the pie center to the end of the arc
        auto arcEnd = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_endAngle);
        arcEnd.first += m_pieArea.GetTopLeft().x;
        arcEnd.second += m_pieArea.GetTopLeft().y;
            {
            wxDCPenChanger pc(dc, scaledPen);
            dc.DrawLine(centerPoint, wxPoint(arcEnd.first, arcEnd.second));
            }

        if (IsSelected())
            {
            auto points = GetPolygon();
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
            dc.DrawLines(points.size(), &points[0]);
            // highlight the selected protruding bounding box in debug mode
            if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                wxPoint debugOutline[5];
                GraphItems::Polygon::GetRectPoints(m_pieArea, debugOutline);
                debugOutline[4] = debugOutline[0];
                wxDCPenChanger pcDebug(dc, wxPen(*wxGREEN, ScaleToScreenAndCanvas(2), wxPENSTYLE_SHORT_DASH));
                dc.DrawLines(std::size(debugOutline), debugOutline);
                }
            }

        return GetBoundingBox(dc);
        }
    }

namespace Wisteria::Graphs
    {
    //----------------------------------------------------------------
    PieChart::PieChart(Canvas* canvas,
                       std::shared_ptr<Colors::Schemes::ColorScheme> colors /*= nullptr*/) :
        Graph2D(canvas),
        m_pieColors(colors != nullptr ?
            colors :
            Settings::GetDefaultColorScheme())
        {
        GetBottomXAxis().Show(false);
        GetTopXAxis().Show(false);
        GetLeftYAxis().Show(false);
        GetRightYAxis().Show(false);

        GetPen() = *wxWHITE_PEN;

        GetDonutHoleLabel().GetGraphItemInfo().LabelAlignment(TextAlignment::Justified);
        }

    //----------------------------------------------------------------
    void PieChart::SetData(const std::shared_ptr<const Data::Dataset>& data,
                           std::optional<const wxString> aggregateColumnName,
                           const wxString& groupColumn1Name,
                           std::optional<const wxString> groupColumn2Name /*= std::nullopt*/)
        {
        wxASSERT_MSG(data->GetCategoricalColumns().size() > 0,
                     L"At least one grouping column required for pie chart!");
        // at least one group column needed
        if (data == nullptr || data->GetCategoricalColumns().size() == 0)
            { return; }

        GetSelectedIds().clear();
        const auto& groupColumn1 = data->GetCategoricalColumn(groupColumn1Name);
        if (groupColumn1 == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for pie chart.").ToUTF8(),
                groupColumn1Name));
            }

        const auto& groupColumn2 = (groupColumn2Name ?
            data->GetCategoricalColumn(groupColumn2Name.value()) :
            data->GetCategoricalColumns().cend());
        if (groupColumn2Name.has_value() &&
            groupColumn2 == data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': inner group column not found for pie chart.").ToUTF8(),
                groupColumn2Name.value()));
            }
        const bool useSubgrouping =
            (groupColumn2 != data->GetCategoricalColumns().cend());

        const auto& aggregateColumn = (aggregateColumnName.has_value() ?
            data->GetContinuousColumn(aggregateColumnName.value()) :
            data->GetContinuousColumns().cend());
        if (aggregateColumnName.has_value() &&
            aggregateColumn == data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': aggregate column not found for pie chart.").ToUTF8(),
                aggregateColumnName.value()));
            }
        const bool useAggregateColumn =
            (aggregateColumn != data->GetContinuousColumns().cend());

        GetInnerPie().clear();
        GetOuterPie().clear();

        using SliceAndValues = std::map<Data::GroupIdType, double>;
        // the outer pie (or only pie, if a single series)
        SliceAndValues outerGroups;

        double totalValue{ 0.0 };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (useAggregateColumn && std::isnan(aggregateColumn->GetValue(i)) )
                { continue; }

            auto [iterator, inserted] = outerGroups.insert(std::make_pair(
                groupColumn1->GetValue(i),
                (useAggregateColumn ? aggregateColumn->GetValue(i) : 1)) );
            // increment counts for group
            if (!inserted)
                {
                iterator->second += (useAggregateColumn ? aggregateColumn->GetValue(i) : 1);
                }
            totalValue += (useAggregateColumn ? aggregateColumn->GetValue(i) : 1);
            }

        // create slices with their percents of the overall total
        for (const auto& group : outerGroups)
            {
            GetOuterPie().emplace_back(
                SliceInfo{ groupColumn1->GetLabelFromID(group.first),
                           group.second,
                           safe_divide(group.second, totalValue) });
            }
        std::sort(GetOuterPie().begin(), GetOuterPie().end());

        // if more grouping columns, then add an inner pie (which is a subgrouping
        // of the main group)
        if (useSubgrouping && data->GetCategoricalColumns().size() > 1)
            {
            std::map<Data::GroupIdType, SliceAndValues> innerGroups;
            totalValue = 0;
            auto searchValue = std::make_pair<Data::GroupIdType,
                                              SliceAndValues>(0, SliceAndValues());
            for (size_t i = 0; i < data->GetRowCount(); ++i)
                {
                if (useAggregateColumn && std::isnan(aggregateColumn->GetValue(i)) )
                    { continue; }

                searchValue.first = groupColumn1->GetValue(i);
                auto [iterator, inserted] = innerGroups.insert(searchValue);
                if (inserted)
                    {
                    iterator->second.insert(std::make_pair(
                        groupColumn2->GetValue(i),
                        (useAggregateColumn ? aggregateColumn->GetValue(i) : 1)));
                    }
                else
                    {
                    auto [subIterator, subInserted] = iterator->second.insert(std::make_pair(
                        groupColumn2->GetValue(i),
                        (useAggregateColumn ? aggregateColumn->GetValue(i) : 1)));
                    // increment counts for group
                    if (!subInserted)
                        {
                        subIterator->second +=
                            (useAggregateColumn ? aggregateColumn->GetValue(i) : 1);
                        }
                    }
                totalValue += (useAggregateColumn ? aggregateColumn->GetValue(i) : 1);
                }

            std::map<wxString, PieInfo, Data::StringCmpNoCase> innerPie;
            // the outer ring (main group) for the inner group slices
            for (const auto& innerGroupOuterRing : innerGroups)
                {
                PieInfo currentOuterSliceSlices;
                // the slices with the current outer ring group
                for (const auto& innerGroup : innerGroupOuterRing.second)
                    {
                    currentOuterSliceSlices.emplace_back(
                        SliceInfo{ groupColumn2->GetLabelFromID(innerGroup.first),
                               innerGroup.second,
                               safe_divide(innerGroup.second, totalValue) });
                    }
                std::sort(currentOuterSliceSlices.begin(), currentOuterSliceSlices.end());
                innerPie.insert(std::make_pair(
                    groupColumn1->GetLabelFromID(innerGroupOuterRing.first),
                    currentOuterSliceSlices));
                }
            // unroll the grouped slices into one large pie
            Data::GroupIdType parentGroupIndex{ 0 };
            for (auto& innerPieSliceGroup : innerPie)
                {
                std::for_each(innerPieSliceGroup.second.begin(),
                    innerPieSliceGroup.second.end(),
                    [&parentGroupIndex](auto& slice) noexcept
                    { slice.m_parentSliceIndex = parentGroupIndex; });
                GetInnerPie().insert(GetInnerPie().end(),
                                     innerPieSliceGroup.second.cbegin(),
                                     innerPieSliceGroup.second.cend());
                ++parentGroupIndex;
                }

            // Don't show their labels on the outside by default though,
            // may overlap the outer pie's labels and be too cluttered;
            // client can turn these on after setting the data.
            ShowInnerPieLabels(false);
            }
        }

    //----------------------------------------------------------------
    void PieChart::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        // get a square inside of the drawing area for the pie
        wxRect pieDrawArea = GetPlotAreaBoundingBox();
        // get 75% of the area width and height for the pie (adding space for any labels),
        // and use the smaller of the two for the pie's area
        const auto pieHeight = (pieDrawArea.GetHeight() * math_constants::three_quarters);
        const auto pieWidth = (pieDrawArea.GetWidth() * math_constants::three_quarters);
        const auto pieDimension = std::min(pieHeight, pieWidth);
        const auto widthDifference = (pieDrawArea.GetWidth() - pieDimension);
        const auto heightDifference = (pieDrawArea.GetHeight() - pieDimension);
        pieDrawArea.SetWidth(pieDimension);
        pieDrawArea.SetX(pieDrawArea.GetX() + (widthDifference / 2));
        pieDrawArea.SetHeight(pieDimension);
        pieDrawArea.SetY(pieDrawArea.GetY() + (heightDifference / 2));

        // make label drawing area square or "golden ratioed,"
        // so that labels don't go up too high or too far over
        wxRect fullDrawArea = GetPlotAreaBoundingBox();
            {
            const auto widthDiff = GetPlotAreaBoundingBox().GetWidth() - pieDrawArea.GetWidth();
            const auto heightDiff = GetPlotAreaBoundingBox().GetHeight() - pieDrawArea.GetHeight();
            if (heightDiff > widthDiff)
                {
                const auto sizeDiff = heightDiff - widthDiff;
                fullDrawArea.SetHeight(fullDrawArea.GetHeight() - sizeDiff);
                fullDrawArea.SetY(fullDrawArea.GetY() + (sizeDiff/2));
                }
            else if (widthDiff > heightDiff)
                {
                // use the golden ratio for the width if we have enough space for it;
                // otherwise, use whatever width we have, making it more of a square
                const auto goldenRatioWidth = fullDrawArea.GetHeight() * math_constants::golden_ratio;
                const auto newWidth = std::min<double>(goldenRatioWidth, fullDrawArea.GetWidth());
                const auto widthDiff = fullDrawArea.GetWidth() - newWidth;
                fullDrawArea.SetWidth(newWidth);
                fullDrawArea.SetX(fullDrawArea.GetX() + (widthDiff/2));
                }
            }

        // make the connection line for inner slices and their labels
        // poke out a little from the pie
        auto outerPieDrawArea = pieDrawArea;
        outerPieDrawArea.width *= 1.1;
        outerPieDrawArea.height *= 1.1;
        outerPieDrawArea.Offset(wxPoint((pieDrawArea.width - outerPieDrawArea.width) / 2,
            (pieDrawArea.height - outerPieDrawArea.height) / 2));

        int smallestOuterLabelFontSize{ GetBottomXAxis().GetFont().GetPointSize() };

        // shrinks an outer label to fit within the plotting area
        // and also draws a connection line from the label to the pie slice
        using labelLinePair = std::vector<std::pair<
            std::shared_ptr<Label>,
            std::shared_ptr<Wisteria::GraphItems::Points2D>>>;
        labelLinePair outerTopLeftLabelAndLines, outerBottomLeftLabelAndLines,
                      outerTopRightLabelAndLines, outerBottomRightLabelAndLines;

        const auto createLabelAndConnectionLine = [&](auto pSlice, bool isInnerSlice)
            {
            auto outerLabel = pSlice->CreateOuterLabel(dc,
                (isInnerSlice ? outerPieDrawArea : pieDrawArea));
            outerLabel->SetDPIScaleFactor(GetDPIScaleFactor());
            if (outerLabel != nullptr)
                {
                // lambda to adjust label to fit in pie's gutters
                const auto measureAndFitLabel = [&dc, &fullDrawArea](auto& label)
                    {
                    const auto labelBox = label->GetBoundingBox(dc);
                    if (!Polygon::IsRectInsideRect(labelBox, fullDrawArea) )
                        {
                        const auto currentFontSize = label->GetFont().GetFractionalPointSize();
                        const auto& [widthInside, heightInside] =
                            Polygon::GetPercentInsideRect(labelBox, fullDrawArea);
                        const auto smallerScale = std::min(widthInside, heightInside);
                        label->GetFont().SetFractionalPointSize(currentFontSize * smallerScale);
                        return smallerScale;
                        }
                    return 1.0;
                    };

                // adjust label to fit
                const auto currentFontSize = outerLabel->GetFont().GetFractionalPointSize();
                const auto textScale = measureAndFitLabel(outerLabel);
                // ...but if it's a little too small and doesn't have a header,
                // then try to split it into multiple lines and resize it again.
                // Note that we don't do this if it has a header because the header
                // implies that the first line break is meaningful, so we can't
                // arbitrarily split this text up.
                if (compare_doubles_less(textScale, math_constants::three_quarters) &&
                    !outerLabel->GetHeaderInfo().IsEnabled())
                    {
                    outerLabel->GetFont().SetFractionalPointSize(currentFontSize);
                    if (!outerLabel->SplitTextAuto())
                        {
                        outerLabel->SplitTextToFitLength(
                            outerLabel->GetText().length() * math_constants::third);
                        }
                    measureAndFitLabel(outerLabel);
                    }
                
                smallestOuterLabelFontSize =
                    std::min(smallestOuterLabelFontSize, outerLabel->GetFont().GetPointSize());

                std::shared_ptr<Points2D> connectionLine{ nullptr };
                const bool isTopLeft = outerLabel->GetAnchoring() == Anchoring::BottomRightCorner;
                const bool isBottomLeft = outerLabel->GetAnchoring() == Anchoring::TopRightCorner;
                const bool isLeft = (isTopLeft || isBottomLeft);
                const bool isTopRight = outerLabel->GetAnchoring() == Anchoring::BottomLeftCorner;
                const bool isBottomRight = outerLabel->GetAnchoring() == Anchoring::TopLeftCorner;
                if (isInnerSlice)
                    {
                    // a line connecting the inner slice to its outside label
                    auto arcMiddle = pSlice->GetMiddleOfArc(1);
                    connectionLine =
                        std::make_shared<Points2D>(GetInnerPieConnectionLinePen());
                    connectionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                    connectionLine->SetSelectable(false);
                    connectionLine->AddPoint(Point2D(
                        GraphItemInfo().AnchorPoint(
                            wxPoint(arcMiddle.first, arcMiddle.second)).Show(false), 0), dc);
                    connectionLine->AddPoint(Point2D(
                        GraphItemInfo().
                        AnchorPoint(outerLabel->GetAnchorPoint()).Show(false), 0), dc);
                    if (GetLabelPlacement() == LabelPlacement::Flush)
                        {
                        connectionLine->AddPoint(Point2D(
                            GraphItemInfo().AnchorPoint(
                                wxPoint(isLeft ?
                                            fullDrawArea.GetLeft() :
                                            fullDrawArea.GetRight(),
                                        outerLabel->GetAnchorPoint().y)).Show(false), 0), dc);
                        // force using lines (instead of arrows) since this will be two lines
                        connectionLine->SetLineStyle(LineStyle::Lines);
                        }
                    else
                        { connectionLine->SetLineStyle(GetInnerPieConnectionLineStyle()); }
                    }
                else
                    {
                    // a line connecting the outer slice to its outside label
                    // (only if pushed over to the side)
                    if (GetLabelPlacement() == LabelPlacement::Flush)
                        {
                        auto arcMiddle = pSlice->GetMiddleOfArc(1);
                        connectionLine =
                            std::make_shared<Points2D>(GetInnerPieConnectionLinePen());
                        connectionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                        connectionLine->SetSelectable(false);
                        connectionLine->AddPoint(Point2D(
                            GraphItemInfo().AnchorPoint(
                                wxPoint(arcMiddle.first, arcMiddle.second)).Show(false), 0), dc);
                        connectionLine->AddPoint(Point2D(
                            GraphItemInfo().AnchorPoint(
                                outerLabel->GetAnchorPoint()).Show(false), 0), dc);
                        connectionLine->AddPoint(Point2D(
                            GraphItemInfo().AnchorPoint(
                                wxPoint(isLeft ?
                                            fullDrawArea.GetLeft() :
                                            fullDrawArea.GetRight(),
                                        outerLabel->GetAnchorPoint().y)).Show(false), 0), dc);
                        connectionLine->SetLineStyle(LineStyle::Lines);
                        }
                    }
                if (isTopLeft)
                    {
                    outerTopLeftLabelAndLines.emplace_back(
                        std::make_pair(outerLabel, connectionLine));
                    }
                else if (isBottomLeft)
                    {
                    outerBottomLeftLabelAndLines.emplace_back(
                        std::make_pair(outerLabel, connectionLine));
                    }
                else if (isTopRight)
                    {
                    outerTopRightLabelAndLines.emplace_back(
                        std::make_pair(outerLabel, connectionLine));
                    }
                else if (isBottomRight)
                    {
                    outerBottomRightLabelAndLines.emplace_back(
                        std::make_pair(outerLabel, connectionLine));
                    }
                }
            };

        // outer (main) pie
        double startAngle{ 0.0 };
        std::vector<std::shared_ptr<Label>> middleLabels;
        int smallestMiddleLabelFontSize{ GetBottomXAxis().GetFont().GetPointSize() };
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            const wxColour sliceColor =
                (GetOuterPie().at(i).IsGhosted() ?
                    ColorContrast::ChangeOpacity(GetColorScheme()->GetColor(i), m_ghostOpacity) :
                    GetColorScheme()->GetColor(i));
            auto pSlice = std::make_shared<PieSlice>(
                GraphItemInfo(GetOuterPie().at(i).GetGroupLabel()).
                Brush(sliceColor).
                DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                Pen(GetPen()),
                pieDrawArea,
                startAngle, startAngle + (GetOuterPie().at(i).m_percent * 360),
                GetOuterPie().at(i).m_value, GetOuterPie().at(i).m_percent);
            pSlice->SetMidPointLabelDisplay(GetOuterPie().at(i).GetMidPointLabelDisplay());
            if (GetOuterPie().at(i).m_description.length())
                {
                pSlice->SetText(GetOuterPie().at(i).GetGroupLabel() + L"\n" +
                    GetOuterPie().at(i).m_description);
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                if (IsUsingColorLabels())
                    { pSlice->GetHeaderInfo().FontColor(GetColorScheme()->GetColor(i)); }
                pSlice->SetFontColor(ColorContrast::ShadeOrTint(pSlice->GetFontColor(), .4));
                }
            else
                {
                if (IsUsingColorLabels())
                    { pSlice->SetFontColor(GetColorScheme()->GetColor(i)); }
                pSlice->GetFont().MakeBold();
                }
            AddObject(pSlice);
            if (GetOuterPie().at(i).m_showText)
                { createLabelAndConnectionLine(pSlice, false); }

            double sliceProportion = 1 - (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
            if (GetInnerPie().size())
                { sliceProportion /= 2; }

            sliceProportion = (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0) +
                safe_divide<double>(sliceProportion, 2) +
                (GetInnerPie().size() ? sliceProportion : 0);
            const auto labelDisplay = (pSlice->GetMidPointLabelDisplay().has_value() ?
                pSlice->GetMidPointLabelDisplay().value() :
                GetOuterPieMidPointLabelDisplay());
            if (labelDisplay != BinLabelDisplay::NoDisplay)
                {
                auto middleLabel = pSlice->CreateMiddleLabel(dc, sliceProportion,
                    labelDisplay, m_abbreviate);
                if (middleLabel != nullptr)
                    {
                    middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                    smallestMiddleLabelFontSize =
                        std::min(smallestMiddleLabelFontSize, middleLabel->GetFont().GetPointSize());
                    }
                middleLabels.emplace_back(middleLabel);
                }

            startAngle += GetOuterPie().at(i).m_percent * 360;
            }
        // make the outer ring middle labels have a common font size
        for (auto& middleLabel : middleLabels)
            {
            if (middleLabel != nullptr)
                {
                middleLabel->GetFont().SetPointSize(smallestMiddleLabelFontSize);
                AddObject(middleLabel);
                }
            }

        // inner pie
        startAngle = 0;
        size_t currentParentSliceIndex{ 0 };
        auto sliceColor{ GetColorScheme()->GetColor(0) };
        middleLabels.clear();
        smallestMiddleLabelFontSize = GetBottomXAxis().GetFont().GetPointSize();

        const double sliceProportion = safe_divide<double>(1 -
                (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), 2) +
                (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
        auto innerDrawArea = pieDrawArea;
        innerDrawArea.width *= sliceProportion;
        innerDrawArea.height *= sliceProportion;
        innerDrawArea.Offset(wxPoint((pieDrawArea.width-innerDrawArea.width)/2,
                                        (pieDrawArea.height-innerDrawArea.height)/2));

        // how much (percentage) of the inner ring area the donut hole consumes
        const double donutHoleInnerProportion = safe_divide<double>(
            (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), sliceProportion);

        // outline of inner slices' sides, which will be half as thick as the
        // outer ring's slice sides
        auto sliceLine{ GetPen() };
        sliceLine.SetWidth(std::max(1,
            (sliceLine.IsOk()? sliceLine.GetWidth() : 2) / 2));

        // note that we do NOT clear outerLabels or its smallest font size,
        // both rings use these
        for (size_t i = 0; i < GetInnerPie().size(); ++i)
            {
            // slightly adjusted color based on the parent slice color
            sliceColor = (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceIndex) ?
                ColorContrast::ShadeOrTint(sliceColor, .1) :
                ColorContrast::ShadeOrTint(
                    GetColorScheme()->GetColor(GetInnerPie().at(i).m_parentSliceIndex, 0.1));
            const wxColour sliceColorForBrush =
                (GetInnerPie().at(i).IsGhosted() ?
                    // inner slices should be twice as translucent as outer slices since
                    // the outer slices will slightly show through it
                    ColorContrast::ChangeOpacity(sliceColor, m_ghostOpacity / 2) :
                    sliceColor);
            currentParentSliceIndex = GetInnerPie().at(i).m_parentSliceIndex;

            auto pSlice = std::make_shared<PieSlice>(
                GraphItemInfo(GetInnerPie().at(i).GetGroupLabel()).
                Brush(sliceColorForBrush).
                DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                Pen(sliceLine),
                innerDrawArea,
                startAngle, startAngle + (GetInnerPie().at(i).m_percent * 360),
                GetInnerPie().at(i).m_value, GetInnerPie().at(i).m_percent);
            pSlice->SetMidPointLabelDisplay(GetInnerPie().at(i).GetMidPointLabelDisplay());
            pSlice->GetArcPen() = GetPen();
            if (GetInnerPie().at(i).m_description.length())
                {
                pSlice->SetText(GetInnerPie().at(i).GetGroupLabel() + L"\n" +
                    GetInnerPie().at(i).m_description);
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                // use the parent slice color for the header, font color for the body
                if (IsUsingColorLabels())
                    {
                    pSlice->SetFontColor(GetColorScheme()->GetColor(
                        GetInnerPie().at(i).m_parentSliceIndex));
                    }
                pSlice->SetFontColor(ColorContrast::ShadeOrTint(pSlice->GetFontColor(), 0.4));
                }
            else
                {
                if (IsUsingColorLabels())
                    {
                    // parent color if using color labels, the inner slice's color
                    // may be too washed out to be legible
                    pSlice->SetFontColor(GetColorScheme()->GetColor(
                        GetInnerPie().at(i).m_parentSliceIndex));
                    }
                pSlice->GetFont().MakeBold();
                }
            AddObject(pSlice);

            if (GetInnerPie().at(i).m_showText)
                { createLabelAndConnectionLine(pSlice, true); }

            const auto labelDisplay = (pSlice->GetMidPointLabelDisplay().has_value() ?
                pSlice->GetMidPointLabelDisplay().value() :
                GetInnerPieMidPointLabelDisplay());
            if (labelDisplay != BinLabelDisplay::NoDisplay)
                {
                auto middleLabel = pSlice->CreateMiddleLabel(dc,
                    // take into account the hole consuming a larger % of the inner
                    // area compared to the full pie area
                    safe_divide(1.0 - donutHoleInnerProportion, 2.0) + donutHoleInnerProportion,
                    labelDisplay, m_abbreviate);
                if (middleLabel != nullptr)
                    {
                    middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                    smallestMiddleLabelFontSize =
                        std::min(smallestMiddleLabelFontSize, middleLabel->GetFont().GetPointSize());
                    }
                middleLabels.emplace_back(middleLabel);
                }

            startAngle += GetInnerPie().at(i).m_percent * 360;
            }

        // sort top quandrant labels (top-to-bottom)
        std::sort(outerTopLeftLabelAndLines.begin(), outerTopLeftLabelAndLines.end(),
            [](const auto& lhv, const auto& rhv) noexcept
            {
            wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
            wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
            return lhv.first->GetAnchorPoint().y < rhv.first->GetAnchorPoint().y;
            });
        // reverse bottom quandrant sort labels (bottom-to-top)
        std::sort(outerBottomLeftLabelAndLines.begin(), outerBottomLeftLabelAndLines.end(),
            [](const auto& lhv, const auto& rhv) noexcept
            {
            wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
            wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
            return rhv.first->GetAnchorPoint().y < lhv.first->GetAnchorPoint().y;
            });
        // Make the left-side outer labels (for both rings) have a common font size.
        // Also, adjust their positioning and connection lines (if necessary).
        wxRect previousLabelBoundingBox;
        // left-side labels, top quadrant
        for (size_t i = 0; i < outerTopLeftLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = outerTopLeftLabelAndLines[i];
            if (outerLabel == nullptr)
                { continue; }
            outerLabel->GetHeaderInfo().GetFont().SetPointSize(smallestOuterLabelFontSize);
            outerLabel->GetFont().SetPointSize(smallestOuterLabelFontSize);

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                std::shared_ptr<Label> nextLabel =
                                        (i+1 < outerTopLeftLabelAndLines.size() ?
                                         outerTopLeftLabelAndLines[i+1].first :
                                         nullptr);
                // push label to the left and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(fullDrawArea.GetLeft(),
                        outerLabel->GetAnchorPoint().y +
                        (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                // Does the top label overlap the one below it?
                // If so, push it all the way up to the top.
                if (i == 0 && nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(fullDrawArea.GetTopLeft());
                        outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                        }
                    }
                else if (nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetBottomLeft());
                        outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);
            AddObject(outerLabel);
            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    // flush always has three points, just a sanity test
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::min(labelBox.GetRight(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight()/2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside of the pie's bounding box
                    if (!pieDrawArea.Contains(calculatedMiddlePt))
                        { middlePt.SetAnchorPoint(calculatedMiddlePt); }
                    }
                AddObject(outerLine);
                }
            }
        // left-side labels, bottom quadrant
        for (size_t i = 0; i < outerBottomLeftLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = outerBottomLeftLabelAndLines[i];
            if (outerLabel == nullptr)
                { continue; }
            outerLabel->GetHeaderInfo().GetFont().SetPointSize(smallestOuterLabelFontSize);
            outerLabel->GetFont().SetPointSize(smallestOuterLabelFontSize);

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                const std::shared_ptr<Label> nextLabel =
                                        (i+1 < outerBottomLeftLabelAndLines.size() ?
                                         outerBottomLeftLabelAndLines[i+1].first :
                                         nullptr);
                // push label to the left and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(fullDrawArea.GetLeft(),
                        outerLabel->GetAnchorPoint().y -
                        (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::TopLeftCorner);
                // Does the bottom label overlap the one above it?
                // If so, push it all the way down to the bottom.
                if (i == 0 && nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetLeft());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight()/2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(fullDrawArea.GetBottomLeft());
                        outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                        }
                    else if (nextLabel)
                        {
                        auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                        nextLabelBox.SetX(fullDrawArea.GetLeft());
                        nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight()/2));
                        if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                            {
                            outerLabel->SetAnchorPoint(
                                // there is already padding on the labels, OK to
                                // not have space between them
                                previousLabelBoundingBox.GetTopLeft());
                            outerLabel->SetAnchoring(Anchoring::BottomLeftCorner);
                            }
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);
            AddObject(outerLabel);
            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // Connect last point to middle of label's right side;
                    // unless the label is oddly wide, then align with the middle point.
                    lastPt.SetAnchorPoint(
                        wxPoint(std::min(labelBox.GetRight(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight()/2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside of the pie's bounding box
                    if (!pieDrawArea.Contains(calculatedMiddlePt))
                        { middlePt.SetAnchorPoint(calculatedMiddlePt); }
                    }
                AddObject(outerLine);
                }
            }

        // do the same for the right-side labels
        std::sort(outerTopRightLabelAndLines.begin(), outerTopRightLabelAndLines.end(),
            [](const auto& lhv, const auto& rhv) noexcept
            {
            wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
            wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
            return lhv.first->GetAnchorPoint().y < rhv.first->GetAnchorPoint().y;
            });
        std::sort(outerBottomRightLabelAndLines.begin(), outerBottomRightLabelAndLines.end(),
            [](const auto& lhv, const auto& rhv) noexcept
            {
            wxASSERT_MSG(lhv.first, L"Invalid pie label when sorting!");
            wxASSERT_MSG(rhv.first, L"Invalid pie label when sorting!");
            return rhv.first->GetAnchorPoint().y < lhv.first->GetAnchorPoint().y;
            });

        // right-side labels, top quadrant
        for (size_t i = 0; i < outerTopRightLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = outerTopRightLabelAndLines[i];
            if (outerLabel == nullptr)
                { continue; }
            outerLabel->GetHeaderInfo().GetFont().SetPointSize(smallestOuterLabelFontSize);
            outerLabel->GetFont().SetPointSize(smallestOuterLabelFontSize);
            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                std::shared_ptr<Label> nextLabel =
                                        (i+1 < outerTopRightLabelAndLines.size() ?
                                         outerTopRightLabelAndLines[i+1].first :
                                         nullptr);
                // push label to the right and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(fullDrawArea.GetRight(),
                        outerLabel->GetAnchorPoint().y +
                        (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                // Does the top label overlap the one below it?
                // If so, push it all the way up to the top.
                if (i == 0 && nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(fullDrawArea.GetTopRight());
                        outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                        }
                    }
                else if (nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() + (nextLabelBox.GetHeight() / 2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetBottomRight());
                        outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);
            AddObject(outerLabel);
            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::max(labelBox.GetLeft(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight()/2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside of the pie's bounding box
                    if (!pieDrawArea.Contains(calculatedMiddlePt))
                        { middlePt.SetAnchorPoint(calculatedMiddlePt); }
                    }
                AddObject(outerLine);
                }
            }
        // right-side labels, bottom quadrant
        for (size_t i = 0; i < outerBottomRightLabelAndLines.size(); ++i)
            {
            auto& [outerLabel, outerLine] = outerBottomRightLabelAndLines[i];
            if (outerLabel == nullptr)
                { continue; }
            outerLabel->GetHeaderInfo().GetFont().SetPointSize(smallestOuterLabelFontSize);
            outerLabel->GetFont().SetPointSize(smallestOuterLabelFontSize);

            if (GetLabelPlacement() == LabelPlacement::Flush)
                {
                std::shared_ptr<Label> nextLabel =
                                        (i+1 < outerBottomRightLabelAndLines.size() ?
                                         outerBottomRightLabelAndLines[i+1].first :
                                         nullptr);
                // push label to the right and center it to its connect line vertically
                outerLabel->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft);
                outerLabel->SetAnchorPoint(
                    wxPoint(fullDrawArea.GetRight(),
                        outerLabel->GetAnchorPoint().y -
                        (outerLabel->GetBoundingBox(dc).GetHeight() / 2)));
                outerLabel->SetAnchoring(Anchoring::TopRightCorner);
                // Does the bottom label overlap the one above it?
                // If so, push it all the way down to the bottom.
                if (i == 0 && nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight()/2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(fullDrawArea.GetBottomRight());
                        outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                        }
                    }
                else if (nextLabel)
                    {
                    auto nextLabelBox = nextLabel->GetBoundingBox(dc);
                    nextLabelBox.SetX(fullDrawArea.GetRight() -
                                      nextLabelBox.GetWidth());
                    nextLabelBox.SetY(nextLabelBox.GetY() - (nextLabelBox.GetHeight()/2));
                    if (outerLabel->GetBoundingBox(dc).Intersects(nextLabelBox))
                        {
                        outerLabel->SetAnchorPoint(
                            // there is already padding on the labels, OK to
                            // not have space between them
                            previousLabelBoundingBox.GetTopRight());
                        outerLabel->SetAnchoring(Anchoring::BottomRightCorner);
                        }
                    }
                }
            previousLabelBoundingBox = outerLabel->GetBoundingBox(dc);
            AddObject(outerLabel);
            // If there is a connection line and label is flush, set the end point
            // to be next to the label; otherwise, just add it.
            if (outerLine != nullptr)
                {
                if (GetLabelPlacement() == LabelPlacement::Flush &&
                    outerLine->GetPoints().size() == 3)
                    {
                    const auto& firstPt = outerLine->GetPoints()[0];
                    auto& middlePt = outerLine->GetPoints()[1];
                    auto& lastPt = outerLine->GetPoints()[2];
                    const auto labelBox = outerLabel->GetBoundingBox(dc);
                    // connect last point to middle of label's right side
                    lastPt.SetAnchorPoint(
                        wxPoint(std::max(labelBox.GetLeft(), middlePt.GetAnchorPoint().x),
                                labelBox.GetTop() + (labelBox.GetHeight()/2)));
                    const auto calculatedMiddlePt =
                        wxPoint(firstPt.GetAnchorPoint().x, lastPt.GetAnchorPoint().y);
                    // move middle point over to make the lines straight,
                    // but only if line connection is outside of the pie's bounding box
                    if (!pieDrawArea.Contains(calculatedMiddlePt))
                        { middlePt.SetAnchorPoint(calculatedMiddlePt); }
                    }
                AddObject(outerLine);
                }
            }

        // make the inner ring center labels have a common font size
        for (auto& middleLabel : middleLabels)
            {
            if (middleLabel != nullptr)
                {
                middleLabel->GetFont().SetPointSize(smallestMiddleLabelFontSize);
                AddObject(middleLabel);
                }
            }

        // center hole, if a donut
        if (IsIncludingDonutHole())
            {
            const wxPoint centerPt(pieDrawArea.GetLeft() + (pieDrawArea.GetWidth() / 2),
                pieDrawArea.GetTop() + (pieDrawArea.GetHeight() / 2));
            auto donutHole = std::make_shared<Point2D>(
                GraphItemInfo().Brush(GetDonutHoleColor()).
                DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                Selectable(false).
                Pen(GetPen()).
                Anchoring(Anchoring::Center).
                AnchorPoint(centerPt),
                0);
            const double holeRadius{ (pieDrawArea.GetWidth() * GetDonutHoleProportion()) / 2 };
            donutHole->SetRadius(donutHole->DownscaleFromScreenAndCanvas(holeRadius));
            AddObject(donutHole);

            if (GetDonutHoleLabel().GetText().length())
                {
                auto donutHoleLabel = std::make_shared<Label>(GetDonutHoleLabel());
                donutHoleLabel->GetGraphItemInfo().Pen(wxNullPen).
                    DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                    LabelPageVerticalAlignment(PageVerticalAlignment::Centered).
                    Anchoring(Anchoring::Center).
                    AnchorPoint(centerPt);

                wxPoint donutHoleLabelCorner{ centerPt };
                auto rectWithinCircleWidth = geometry::radius_to_inner_rect_width(holeRadius);
                donutHoleLabelCorner.x -= rectWithinCircleWidth / 2;
                donutHoleLabelCorner.y -= rectWithinCircleWidth / 2;
                donutHoleLabel->SetBoundingBox(
                    wxRect(donutHoleLabelCorner,
                           wxSize(rectWithinCircleWidth, rectWithinCircleWidth)),
                    dc, GetScaling());
                AddObject(donutHoleLabel);
                }
            }
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestOuterPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetOuterPie().size() == 0)
            { return pieLabels; }

        // find largest percentage
        const auto& maxPie = *std::max_element(GetOuterPie().cbegin(), GetOuterPie().cend(),
            [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv.m_percent, rhv.m_percent); }
            );

        // in case of ties, grab all pie slices with same percentage as the largest one
        std::for_each(GetOuterPie().cbegin(), GetOuterPie().cend(),
            [&](const auto& slice) noexcept
                {
                if (compare_doubles(slice.m_percent, maxPie.m_percent))
                    { pieLabels.emplace_back(slice.m_groupLabel); }
                }
            );

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestOuterPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetOuterPie().size() == 0)
            { return pieLabels; }

        // find smallest percentage
        const auto& minPie = *std::min_element(GetOuterPie().cbegin(), GetOuterPie().cend(),
            [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv.m_percent, rhv.m_percent); }
            );

        // in case of ties, grab all pie slices with same percentage as the smallest one
        std::for_each(GetOuterPie().cbegin(), GetOuterPie().cend(),
            [&](const auto& slice) noexcept
                {
                if (compare_doubles(slice.m_percent, minPie.m_percent))
                    { pieLabels.emplace_back(slice.m_groupLabel); }
                }
            );

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestInnerPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetInnerPie().size() == 0)
            { return pieLabels; }

        // find largest percentage
        const auto& maxPie = *std::max_element(GetInnerPie().cbegin(), GetInnerPie().cend(),
            [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv.m_percent, rhv.m_percent); }
            );

        // in case of ties, grab all pie slices with same percentage as the largest one
        std::for_each(GetInnerPie().cbegin(), GetInnerPie().cend(),
            [&](const auto& slice) noexcept
                {
                if (compare_doubles(slice.m_percent, maxPie.m_percent))
                    { pieLabels.emplace_back(slice.m_groupLabel); }
                }
            );

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetLargestInnerPieSlicesByGroup() const
        {
        std::vector<wxString> pieLabels;
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            // get the inner slices within the current parent slice
            std::vector<const SliceInfo*> innerSlicesForCurrentGroup;
            std::for_each(GetInnerPie().cbegin(), GetInnerPie().cend(),
                [&](const auto& slice)
                {
                if (slice.m_parentSliceIndex == i)
                    { innerSlicesForCurrentGroup.push_back(&slice); }
                }
                );
            if (innerSlicesForCurrentGroup.size() == 0)
                { continue; }
            // find largest percentage within the subgroup of slices
            const auto maxPie = *std::max_element(
                innerSlicesForCurrentGroup.cbegin(), innerSlicesForCurrentGroup.cend(),
                [](const auto& lhv, const auto& rhv) noexcept
                    { return compare_doubles_less(lhv->m_percent, rhv->m_percent); }
                );

            // in case of ties, grab all pie slices with same percentage as the largest one
            std::for_each(innerSlicesForCurrentGroup.cbegin(), innerSlicesForCurrentGroup.cend(),
                [&](const auto& slice) noexcept
                    {
                    if (compare_doubles(slice->m_percent, maxPie->m_percent))
                        { pieLabels.emplace_back(slice->m_groupLabel); }
                    }
                );
            }

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestInnerPieSlices() const
        {
        std::vector<wxString> pieLabels;
        if (GetInnerPie().size() == 0)
            { return pieLabels; }

        // find smallest percentage
        const auto& minPie = *std::min_element(GetInnerPie().cbegin(), GetInnerPie().cend(),
            [](const auto& lhv, const auto& rhv) noexcept
                { return compare_doubles_less(lhv.m_percent, rhv.m_percent); }
            );

        // in case of ties, grab all pie slices with same percentage as the smallest one
        std::for_each(GetInnerPie().cbegin(), GetInnerPie().cend(),
            [&](const auto& slice) noexcept
                {
                if (compare_doubles(slice.m_percent, minPie.m_percent))
                    { pieLabels.emplace_back(slice.m_groupLabel); }
                }
            );

        return pieLabels;
        }

    //----------------------------------------------------------------
    std::vector<wxString> PieChart::GetSmallestInnerPieSlicesByGroup() const
        {
        std::vector<wxString> pieLabels;
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            // get the inner slices within the current parent slice
            std::vector<const SliceInfo*> innerSlicesForCurrentGroup;
            std::for_each(GetInnerPie().cbegin(), GetInnerPie().cend(),
                [&](const auto& slice)
                {
                if (slice.m_parentSliceIndex == i)
                    { innerSlicesForCurrentGroup.push_back(&slice); }
                }
                );
            if (innerSlicesForCurrentGroup.size() == 0)
                { continue; }
            // find smallest percentage within the subgroup of slices
            const auto minPie = *std::min_element(
                innerSlicesForCurrentGroup.cbegin(), innerSlicesForCurrentGroup.cend(),
                [](const auto& lhv, const auto& rhv) noexcept
                    { return compare_doubles_less(lhv->m_percent, rhv->m_percent); }
                );

            // in case of ties, grab all pie slices with same percentage as the smallest one
            std::for_each(innerSlicesForCurrentGroup.cbegin(), innerSlicesForCurrentGroup.cend(),
                [&](const auto& slice) noexcept
                    {
                    if (compare_doubles(slice->m_percent, minPie->m_percent))
                        { pieLabels.emplace_back(slice->m_groupLabel); }
                    }
                );
            }

        return pieLabels;
        }

    //----------------------------------------------------------------
    void PieChart::GhostOuterPieSlices(const bool ghost)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                { slice.Ghost(ghost); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::GhostOuterPieSlices(const bool ghost, const std::vector<wxString>& slicesToGhost)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    slicesToGhost.cbegin(), slicesToGhost.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != slicesToGhost.cend());
                slice.Ghost(inList ? ghost : !ghost);
                }
            );
        }

    //----------------------------------------------------------------
    void PieChart::GhostInnerPieSlices(const bool ghost)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                { slice.Ghost(ghost); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::GhostInnerPieSlices(const bool ghost, const std::vector<wxString>& slicesToGhost)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    slicesToGhost.cbegin(), slicesToGhost.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != slicesToGhost.cend());
                slice.Ghost(inList ? ghost : !ghost);
                }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowcaseOuterPieSlicesAndChildren(const std::vector<wxString>& pieSlices)
        {
        ShowOuterPieLabels(false);
        ShowOuterPieMidPointLabels(true, pieSlices);
        GhostOuterPieSlices(false, pieSlices);

        // get positions of outer slices being showcased
        std::set<size_t> showcasedOuterIndices;
        for (const auto& pieSliceLabel : pieSlices)
            {
            auto foundSlice =
                std::find_if(GetOuterPie().cbegin(), GetOuterPie().cend(),
                    [&pieSliceLabel](const auto& slice)
                    {
                    return (slice.GetGroupLabel().CmpNoCase(pieSliceLabel) == 0);
                    });
            if (foundSlice != GetOuterPie().cend())
                {
                showcasedOuterIndices.insert(
                    std::distance(GetOuterPie().cbegin(), foundSlice));
                }
            }

        std::vector<wxString> innerLabelsForGroups;
        for (const auto& innerSlice : GetInnerPie())
            {
            if (showcasedOuterIndices.find(innerSlice.m_parentSliceIndex) !=
                showcasedOuterIndices.cend())
                { innerLabelsForGroups.emplace_back(innerSlice.GetGroupLabel()); }
            }

        ShowInnerPieLabels(true, innerLabelsForGroups);
        ShowInnerPieMidPointLabels(true, innerLabelsForGroups);
        GhostInnerPieSlices(false, innerLabelsForGroups);
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieLabels(const bool show)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                { slice.ShowGroupLabel(show); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    labelsToShow.cbegin(), labelsToShow.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != labelsToShow.cend());
                slice.ShowGroupLabel(inList ? show : !show);
                }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieMidPointLabels(const bool show)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                { slice.SetMidPointLabelDisplay(show ?
                    std::nullopt :
                    std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay)); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowOuterPieMidPointLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::for_each(GetOuterPie().begin(), GetOuterPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    labelsToShow.cbegin(), labelsToShow.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != labelsToShow.cend());
                slice.SetMidPointLabelDisplay(inList ?
                    std::nullopt :
                    std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
                }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieLabels(const bool show)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                { slice.ShowGroupLabel(show); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    labelsToShow.cbegin(), labelsToShow.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != labelsToShow.cend());
                slice.ShowGroupLabel(inList ? show : !show);
                }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieMidPointLabels(const bool show)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                { slice.SetMidPointLabelDisplay(show ?
                    std::nullopt :
                    std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay)); }
            );
        }

    //----------------------------------------------------------------
    void PieChart::ShowInnerPieMidPointLabels(const bool show, const std::vector<wxString>& labelsToShow)
        {
        std::for_each(GetInnerPie().begin(), GetInnerPie().end(),
            [&](auto& slice) noexcept
                {
                const bool inList = (std::find_if(
                    labelsToShow.cbegin(), labelsToShow.cend(),
                    [&slice](const auto& label)
                    { return label.CmpNoCase(slice.GetGroupLabel()) == 0; }) != labelsToShow.cend());
                slice.SetMidPointLabelDisplay(inList ?
                    std::nullopt :
                    std::optional<BinLabelDisplay>(BinLabelDisplay::NoDisplay));
                }
            );
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> PieChart::CreateInnerPieLegend(
        const LegendCanvasPlacementHint hint)
        {
        wxASSERT_MSG(GetInnerPie().size() > 1,
                     L"Inner ring of pie chart empty, cannot create legend!");
        if (GetInnerPie().size() == 0)
            { return nullptr; }

        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        size_t currentLine{ 0 };

        // space in line is needed for SVG exporting; otherwise, the blank line gets removed
        wxString legendText{ GetOuterPie().at(0).GetGroupLabel() + L"\n \n" };
        legend->GetLinesIgnoringLeftMargin().insert(currentLine);
        currentLine += 2;
        legend->GetLegendIcons().emplace_back(
            LegendIcon(IconShape::HorizontalLineIcon, *wxBLACK_PEN, GetColorScheme()->GetColor(0)));
        legend->GetLegendIcons().emplace_back(
            LegendIcon(IconShape::HorizontalSeparator, *wxBLACK_PEN, *wxBLACK_BRUSH));

        size_t lineCount{ 0 };
        size_t currentParentSliceIndex{ 0 };
        auto sliceColor{ GetColorScheme()->GetColor(0) };
        for (size_t i = 0; i < GetInnerPie().size(); ++i)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                ++currentLine;
                break;
                }
            wxString currentLabel = GetInnerPie().at(i).GetGroupLabel();
            if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                {
                currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                currentLabel.append(L"\u2026");
                ++currentLine;
                }

            // get the color
            // slightly adjusted color based on the parent slice color
            sliceColor = (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceIndex) ?
                ColorContrast::ShadeOrTint(sliceColor, .1) :
                ColorContrast::ShadeOrTint(
                    GetColorScheme()->GetColor(GetInnerPie().at(i).m_parentSliceIndex, .1));
            // starting a new group
            if (currentParentSliceIndex != GetInnerPie().at(i).m_parentSliceIndex)
                {
                currentParentSliceIndex = GetInnerPie().at(i).m_parentSliceIndex;
                legendText.append(
                    GetOuterPie().at(currentParentSliceIndex).GetGroupLabel()).append(L"\n \n");
                legend->GetLinesIgnoringLeftMargin().insert(currentLine);
                currentLine += 2;
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::HorizontalLineIcon,
                        *wxBLACK_PEN,
                        GetColorScheme()->GetColor(currentParentSliceIndex)));
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::HorizontalSeparator, *wxBLACK_PEN, *wxBLACK_BRUSH));
                }

            // add icon and text (after group separator, if needed)
            legendText.append(currentLabel.c_str()).append(L"\n");
            ++currentLine;
            legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::TriangleRightIcon, *wxBLACK_PEN, sliceColor));
            }
        legend->SetText(legendText.Trim());
        // show lines to make sure text is aligned as expected
        if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            { legend->SetLabelStyle(LabelStyle::LinedPaper); }

        AdjustLegendSettings(legend, hint);
        return legend;
        }

    //----------------------------------------------------------------
    std::shared_ptr<GraphItems::Label> PieChart::CreateOuterPieLegend(
        const LegendCanvasPlacementHint hint)
        {
        wxASSERT_MSG(GetOuterPie().size() > 1,
                     L"Outer ring of pie chart empty, cannot create legend!");
        auto legend = std::make_shared<GraphItems::Label>(
            GraphItemInfo().Padding(0, 0, 0, Label::GetMinLegendWidthDIPs()).
            DPIScaling(GetDPIScaleFactor()));

        wxString legendText;
        size_t lineCount{ 0 };
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            if (Settings::GetMaxLegendItemCount() == lineCount)
                {
                legendText.append(L"\u2026");
                break;
                }
            wxString currentLabel = GetOuterPie().at(i).GetGroupLabel();
            if (currentLabel.length() > Settings::GetMaxLegendTextLength())
                {
                currentLabel.resize(Settings::GetMaxLegendTextLength()+1);
                currentLabel.append(L"\u2026");
                }
            legendText.append(currentLabel.c_str()).append(L"\n");
            legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::TriangleRightIcon, *wxBLACK_PEN,
                        GetColorScheme()->GetColor(i)));
            }
        legend->SetText(legendText.Trim());

        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
