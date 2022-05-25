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

namespace Wisteria::GraphItems
    {
    //----------------------------------------------------------------
    std::shared_ptr<Label> PieSlice::CreateMiddleLabel(wxDC& dc,
                                                       const double pieProportion,
                                                       const BinLabelDisplay labelDisplay)
        {
        const auto angle = m_startAngle + ((m_endAngle - m_startAngle) / 2);
        const auto arcMiddle = GetMiddleOfArc(pieProportion);
        auto pieLabel = std::make_shared<Label>(GetGraphItemInfo());
        switch (labelDisplay)
            {
        case BinLabelDisplay::BinValue:
            pieLabel->SetText(
                wxNumberFormatter::ToString(m_value, 1,
                    Settings::GetDefaultNumberFormat()));
            break;
        case BinLabelDisplay::BinValueAndPercentage:
            pieLabel->SetText(wxString::Format(L"%s%%\n(%s)",
                wxNumberFormatter::ToString((m_percent * 100), 0),
                wxNumberFormatter::ToString(m_value, 1,
                    Settings::GetDefaultNumberFormat())) );
            break;
        case BinLabelDisplay::BinPercentage:
            pieLabel->SetText(wxNumberFormatter::ToString((m_percent * 100), 0) + L"%");
            break;
        default:
            // no display
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

        // make it fit in the slice, or return null if too small
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
                if (pieLabel->GetFont().GetPointSize() <= 4 ||
                    compare_doubles(pieLabel->GetFont().GetFractionalPointSize(),
                        currentFontSize))
                    {
                    middleLabelIsTooSmall = true;
                    break;
                    }
                }
            }

        return middleLabelIsTooSmall ? nullptr : pieLabel;
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
            safe_divide<double>(m_endAngle-m_startAngle, 4) + m_startAngle);
        middleSweep1.first += m_pieArea.GetTopLeft().x;
        middleSweep1.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(middleSweep1));

        auto middleSweep2 = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            safe_divide<double>(m_endAngle-m_startAngle, 2) + m_startAngle);
        middleSweep2.first += m_pieArea.GetTopLeft().x;
        middleSweep2.second += m_pieArea.GetTopLeft().y;
        points.emplace_back(Polygon::PairToPoint(middleSweep2));

        auto middleSweep3 = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()),
            ((m_endAngle-m_startAngle) * .75) + m_startAngle);
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
        wxDCPenChanger pc(dc, scaledPen);

        const wxPoint centerPoint{ m_pieArea.GetWidth() / 2 + m_pieArea.GetLeft(),
                                   m_pieArea.GetHeight() / 2 + m_pieArea.GetTop() };

        // outer arc
        dc.DrawEllipticArc(m_pieArea.GetTopLeft(), m_pieArea.GetSize(),
                           m_startAngle, m_endAngle);
        // line from the pie center to the start of the arc
        auto arcStart = geometry::calc_arc_vertex(std::make_pair(
            m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_startAngle);
        arcStart.first += m_pieArea.GetTopLeft().x;
        arcStart.second += m_pieArea.GetTopLeft().y;
        dc.DrawLine(centerPoint, wxPoint(arcStart.first, arcStart.second));
        // line from the pie center to the end of the arc
        auto arcEnd = geometry::calc_arc_vertex(
            std::make_pair(m_pieArea.GetWidth(), m_pieArea.GetHeight()), m_endAngle);
        arcEnd.first += m_pieArea.GetTopLeft().x;
        arcEnd.second += m_pieArea.GetTopLeft().y;
        dc.DrawLine(centerPoint, wxPoint(arcEnd.first, arcEnd.second));

        if (IsSelected())
            {
            auto points = GetPolygon();
            wxDCPenChanger pc(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
            dc.DrawLines(points.size(), &points[0]);
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

        GetDonutHoleLabel().GetGraphItemInfo().LabelAlignment(TextAlignment::Justified);
        }

    //----------------------------------------------------------------
    void PieChart::SetData(std::shared_ptr<const Data::Dataset> data,
                           const wxString& continuousColumnName,
                           const wxString& groupColumn1Name,
                           std::optional<const wxString> groupColumn2Name /*= std::nullopt*/)
        {
        wxASSERT_MSG(data->GetCategoricalColumns().size() > 0,
                     L"At least one grouping column required for pie chart!");
        // at least one group column needed
        if (data == nullptr || data->GetCategoricalColumns().size() == 0)
            { return; }

        m_data = data;
        GetSelectedIds().clear();
        m_groupColumn1 = m_data->GetCategoricalColumn(groupColumn1Name);
        if (m_groupColumn1 == m_data->GetCategoricalColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': group column not found for pie chart.").ToUTF8(),
                groupColumn1Name));
            }
        m_groupColumn2 = (groupColumn2Name ?
            m_data->GetCategoricalColumn(groupColumn2Name.value()) :
            m_data->GetCategoricalColumns().cend());
        const bool useSubgrouping =
            (m_groupColumn2 != m_data->GetCategoricalColumns().cend());
        m_continuousColumn = m_data->GetContinuousColumn(continuousColumnName);
        if (m_continuousColumn == m_data->GetContinuousColumns().cend())
            {
            throw std::runtime_error(wxString::Format(
                _(L"'%s': continuous column not found for pie chart.").ToUTF8(),
                continuousColumnName));
            }

        GetInnerPie().clear();
        GetOuterPie().clear();

        using SliceAndValues = std::map<Data::GroupIdType, double>;
        // the outer pie (or only pie, if a single series)
        SliceAndValues outerGroups;

        double totalValue{ 0.0 };
        for (size_t i = 0; i < data->GetRowCount(); ++i)
            {
            if (std::isnan(m_continuousColumn->GetValue(i)) )
                { continue; }

            auto [iterator, inserted] = outerGroups.insert(std::make_pair(
                m_groupColumn1->GetValue(i),
                m_continuousColumn->GetValue(i)) );
            // increment counts for group
            if (!inserted)
                { iterator->second += m_continuousColumn->GetValue(i); }
            totalValue += m_continuousColumn->GetValue(i);
            }

        // create slices with their percents of the overall total
        for (const auto& group : outerGroups)
            {
            GetOuterPie().emplace_back(
                SliceInfo{ m_groupColumn1->GetCategoryLabelFromID(group.first),
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
                if (std::isnan(m_continuousColumn->GetValue(i)) )
                    { continue; }

                searchValue.first = m_groupColumn1->GetValue(i);
                auto [iterator, inserted] = innerGroups.insert(searchValue);
                if (inserted)
                    {
                    iterator->second.insert(std::make_pair(
                        m_groupColumn2->GetValue(i),
                        m_continuousColumn->GetValue(i)));
                    }
                else
                    {
                    auto [subIterator, subInserted] = iterator->second.insert(std::make_pair(
                        m_groupColumn2->GetValue(i),
                        m_continuousColumn->GetValue(i)));
                    // increment counts for group
                    if (!subInserted)
                        { subIterator->second += m_continuousColumn->GetValue(i); }
                    }
                totalValue += m_continuousColumn->GetValue(i);
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
                        SliceInfo{ m_groupColumn2->GetCategoryLabelFromID(innerGroup.first),
                               innerGroup.second,
                               safe_divide(innerGroup.second, totalValue) });
                    }
                std::sort(currentOuterSliceSlices.begin(), currentOuterSliceSlices.end());
                innerPie.insert(std::make_pair(
                    m_groupColumn1->GetCategoryLabelFromID(innerGroupOuterRing.first),
                    currentOuterSliceSlices));
                }
            // unroll the grouped slices into one large pie
            Data::GroupIdType parentGroupIndex{ 0 };
            for (auto& innerPieSliceGroup : innerPie)
                {
                std::for_each(innerPieSliceGroup.second.begin(),
                    innerPieSliceGroup.second.end(),
                    [&parentGroupIndex](auto& slice) noexcept
                    { slice.m_parentSliceGroup = parentGroupIndex; });
                GetInnerPie().insert(GetInnerPie().end(), innerPieSliceGroup.second.cbegin(),
                                     innerPieSliceGroup.second.cend());
                ++parentGroupIndex;
                }
            }
        }

    //----------------------------------------------------------------
    void PieChart::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        // get a square inside of the drawing area for the pie
        wxRect drawArea = GetPlotAreaBoundingBox();
        // add space to the top and bottom to fit a label
        const auto labelBox = Label(GraphItemInfo(L"Aq\nAq").
            DPIScaling(GetDPIScaleFactor()).
            Pen(wxNullPen).Scaling(GetScaling())).GetBoundingBox(dc);
        drawArea.SetHeight(drawArea.GetHeight() - labelBox.GetHeight()*2);
        drawArea.SetY(labelBox.GetHeight());
        const auto widthDifference = (drawArea.GetWidth() - drawArea.GetHeight());
        drawArea.SetWidth(drawArea.GetWidth() - widthDifference);
        drawArea.SetX(drawArea.GetX() + widthDifference / 2);

        // make the connection line poke out a little from the pie
        auto outerDrawArea = drawArea;
        outerDrawArea.width *= 1.1;
        outerDrawArea.height *= 1.1;
        outerDrawArea.Offset(wxPoint((drawArea.width - outerDrawArea.width) / 2,
            (drawArea.height - outerDrawArea.height) / 2));

        // shrinks an outer label to fit within the plotting area
        std::vector<std::shared_ptr<Label>> outerLabels;
        int smallestOuterLabelFontSize{ GetBottomXAxis().GetFont().GetPointSize() };
        const auto adjustOuterLabelFont = [&](auto outerLabel)
            {
            if (outerLabel != nullptr)
                {
                bool outerLabelIsTooSmall{ false };
                std::vector<wxPoint> plotAreaPts(4);
                Polygon::GetRectPoints(GetPlotAreaBoundingBox(), &plotAreaPts[0]);
                for (;;)
                    {
                    auto labelBox = outerLabel->GetBoundingBox(dc);
                    if (Polygon::IsInsidePolygon(
                            labelBox.GetTopLeft(), &plotAreaPts[0], plotAreaPts.size()) &&
                        Polygon::IsInsidePolygon(
                            labelBox.GetBottomLeft(), &plotAreaPts[0], plotAreaPts.size()) &&
                        Polygon::IsInsidePolygon(
                            labelBox.GetTopRight(), &plotAreaPts[0], plotAreaPts.size()) &&
                        Polygon::IsInsidePolygon(
                            labelBox.GetBottomRight(), &plotAreaPts[0], plotAreaPts.size()))
                        { break; }
                    else
                        {
                        const auto currentFontSize = outerLabel->GetFont().GetFractionalPointSize();
                        // wxFont::MakeSmaller is a little too aggressive, use Scale directly
                        // with a less aggressive value
                        outerLabel->GetFont().Scale(.95f);
                        outerLabel->GetHeaderInfo().GetFont().Scale(.95f);
                        if (outerLabel->GetFont().GetPointSize() <= 4 ||
                            compare_doubles(outerLabel->GetFont().GetFractionalPointSize(),
                                            currentFontSize))
                            {
                            outerLabelIsTooSmall = true;
                            break;
                            }
                        }
                    }
                smallestOuterLabelFontSize =
                    std::min(smallestOuterLabelFontSize, outerLabel->GetFont().GetPointSize());
                outerLabels.emplace_back(outerLabel);
                }
            };

        // outer (main) pie
        double startAngle{ 0.0 };
        std::vector<std::shared_ptr<Label>> middleLabels;
        int smallestMiddleLabelFontSize{ GetBottomXAxis().GetFont().GetPointSize() };
        for (size_t i = 0; i < GetOuterPie().size(); ++i)
            {
            auto pSlice = std::make_shared<PieSlice>(
                GraphItemInfo(GetOuterPie().at(i).GetGroupLabel()).
                Brush(m_pieColors->GetColor(i)).
                DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                Pen(wxNullPen),
                drawArea,
                startAngle, startAngle + (GetOuterPie().at(i).m_percent * 360),
                GetOuterPie().at(i).m_value, GetOuterPie().at(i).m_percent);
            if (GetOuterPie().at(i).m_description.length())
                {
                pSlice->SetText(GetOuterPie().at(i).GetGroupLabel() + L"\n" +
                    GetOuterPie().at(i).m_description);
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                if (IsUsingColorLabels())
                    { pSlice->GetHeaderInfo().FontColor(m_pieColors->GetColor(i)); }
                pSlice->SetFontColor(ColorContrast::ShadeOrTint(pSlice->GetFontColor(), .4));
                }
            else
                {
                if (IsUsingColorLabels())
                    { pSlice->SetFontColor(m_pieColors->GetColor(i)); }
                pSlice->GetFont().MakeBold();
                }
            AddObject(pSlice);
            if (GetOuterPie().at(i).m_showText)
                {
                auto outerLabel = pSlice->CreateOuterLabel(dc, drawArea);
                outerLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                adjustOuterLabelFont(outerLabel);
                }

            double sliceProportion = 1 - (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
            if (GetInnerPie().size())
                { sliceProportion /= 2; }

            sliceProportion = (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0) +
                safe_divide<double>(sliceProportion, 2) +
                (GetInnerPie().size() ? sliceProportion : 0);
            auto middleLabel = pSlice->CreateMiddleLabel(dc, sliceProportion,
                                                        GetOuterPieMidPointLabelDisplay());
            if (middleLabel != nullptr)
                {
                middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                smallestMiddleLabelFontSize =
                    std::min(smallestMiddleLabelFontSize, middleLabel->GetFont().GetPointSize());
                }
            middleLabels.emplace_back(middleLabel);

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
        auto sliceColor{ m_pieColors->GetColor(0) };
        middleLabels.clear();
        smallestMiddleLabelFontSize = GetBottomXAxis().GetFont().GetPointSize();
        // note that we do NOT clear outerLabels or its smallest font size,
        // both rings use these
        for (size_t i = 0; i < GetInnerPie().size(); ++i)
            {
            const double sliceProportion = safe_divide<double>(1 -
                (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), 2) +
                (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0);
            auto innerDrawArea = drawArea;
            innerDrawArea.width *= sliceProportion;
            innerDrawArea.height *= sliceProportion;
            innerDrawArea.Offset(wxPoint((drawArea.width-innerDrawArea.width)/2,
                                         (drawArea.height-innerDrawArea.height)/2));

            // how much (percentage) of the inner ring area the donut hole consumes
            const double donutHoleInnerProportion = safe_divide<double>(
                (IsIncludingDonutHole() ? GetDonutHoleProportion() : 0), sliceProportion);

            // outline for all inner slices within the current group
            const auto sliceLineColor{
                ColorContrast::ShadeOrTint(
                    m_pieColors->GetColor(GetInnerPie().at(i).m_parentSliceGroup), .4) };
            // slightly adjusted color based on the parent slice color
            sliceColor = (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceGroup) ?
                ColorContrast::ShadeOrTint(sliceColor, .1) :
                ColorContrast::ShadeOrTint(
                    m_pieColors->GetColor(GetInnerPie().at(i).m_parentSliceGroup, .1));
            currentParentSliceIndex = GetInnerPie().at(i).m_parentSliceGroup;

            auto pSlice = std::make_shared<PieSlice>(
                GraphItemInfo(GetInnerPie().at(i).GetGroupLabel()).
                Brush(sliceColor).
                DPIScaling(GetDPIScaleFactor()).Scaling(GetScaling()).
                Pen(sliceLineColor),
                innerDrawArea,
                startAngle, startAngle + (GetInnerPie().at(i).m_percent * 360),
                GetInnerPie().at(i).m_value, GetInnerPie().at(i).m_percent);
            if (GetInnerPie().at(i).m_description.length())
                {
                pSlice->SetText(GetInnerPie().at(i).GetGroupLabel() + L"\n" +
                    GetInnerPie().at(i).m_description);
                pSlice->GetHeaderInfo().Enable(true).Font(pSlice->GetFont());
                if (IsUsingColorLabels())
                    { pSlice->GetHeaderInfo().FontColor(sliceColor); }
                pSlice->SetFontColor(ColorContrast::ShadeOrTint(pSlice->GetFontColor(), .4));
                }
            else
                {
                if (IsUsingColorLabels())
                    { pSlice->SetFontColor(sliceColor); }
                pSlice->GetFont().MakeBold();
                }
            AddObject(pSlice);

            if (GetInnerPie().at(i).m_showText)
                {
                auto outerLabel = pSlice->CreateOuterLabel(dc, outerDrawArea);
                outerLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                adjustOuterLabelFont(outerLabel);

                // a line connecting the inner slice to its outside label
                auto arcMiddle = pSlice->GetMiddleOfArc(1);
                auto connectionLine =
                    std::make_shared<Wisteria::GraphItems::Points2D>(GetInnerPieConnectionLinePen());
                connectionLine->SetDPIScaleFactor(GetDPIScaleFactor());
                connectionLine->SetSelectable(false);
                connectionLine->AddPoint(Point2D(
                    GraphItemInfo().AnchorPoint(
                        wxPoint(arcMiddle.first, arcMiddle.second)).Show(false), 0), dc);
                connectionLine->AddPoint(Point2D(
                    GraphItemInfo().AnchorPoint(outerLabel->GetAnchorPoint()).Show(false), 0), dc);
                connectionLine->SetLineStyle(GetInnerPieConnectionLineStyle());
                AddObject(connectionLine);
                }

            auto middleLabel = pSlice->CreateMiddleLabel(dc,
                // take into account the hole consuming a larger % of the inner
                // area compared to the full pie area
                safe_divide(1 - donutHoleInnerProportion, 2.0) + donutHoleInnerProportion,
                GetInnerPieMidPointLabelDisplay());
            if (middleLabel != nullptr)
                {
                middleLabel->SetDPIScaleFactor(GetDPIScaleFactor());
                smallestMiddleLabelFontSize =
                    std::min(smallestMiddleLabelFontSize, middleLabel->GetFont().GetPointSize());
                }
            middleLabels.emplace_back(middleLabel);

            startAngle += GetInnerPie().at(i).m_percent * 360;
            }
        // make the outer labels (for both rings) have a common font size
        for (auto& outerLabel : outerLabels)
            {
            if (outerLabel != nullptr)
                {
                outerLabel->GetHeaderInfo().GetFont().SetPointSize(smallestOuterLabelFontSize);
                outerLabel->GetFont().SetPointSize(smallestOuterLabelFontSize);
                AddObject(outerLabel);
                }
            }
        // make the inner ring middle labels have a common font size
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
            const wxPoint centerPt(drawArea.GetLeft() + (drawArea.GetWidth() / 2),
                drawArea.GetTop() + (drawArea.GetHeight() / 2));
            auto donutHole = std::make_shared<Point2D>(
                GraphItemInfo().Brush(GetDonutHoleColor()).
                DPIScaling(GetDPIScaleFactor()).
                Selectable(false).
                Pen(wxNullPen).
                Anchoring(Anchoring::Center).
                AnchorPoint(centerPt),
                0);
            const double holeRadius{ (drawArea.GetWidth() * GetDonutHoleProportion()) / 2 };
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
        wxString legendText { GetOuterPie().at(0).GetGroupLabel() + L"\n \n" };
        legend->GetLinesIgnoringLeftMargin().insert(currentLine);
        currentLine += 2;
        legend->GetLegendIcons().emplace_back(
            LegendIcon(IconShape::BlankIcon, *wxBLACK, m_pieColors->GetColor(0)));
        legend->GetLegendIcons().emplace_back(
            LegendIcon(IconShape::HorizontalSeparator, *wxBLACK, *wxBLACK));

        size_t lineCount{ 0 };
        size_t currentParentSliceIndex{ 0 };
        auto sliceColor{ m_pieColors->GetColor(0) };
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
            sliceColor = (currentParentSliceIndex == GetInnerPie().at(i).m_parentSliceGroup) ?
                ColorContrast::ShadeOrTint(sliceColor, .1) :
                ColorContrast::ShadeOrTint(
                    m_pieColors->GetColor(GetInnerPie().at(i).m_parentSliceGroup, .1));
            // starting a new group
            if (currentParentSliceIndex != GetInnerPie().at(i).m_parentSliceGroup)
                {
                currentParentSliceIndex = GetInnerPie().at(i).m_parentSliceGroup;
                legendText.append(
                    GetOuterPie().at(currentParentSliceIndex).GetGroupLabel()).append(L"\n \n");
                legend->GetLinesIgnoringLeftMargin().insert(currentLine);
                currentLine += 2;
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::BlankIcon,
                        *wxBLACK,
                        m_pieColors->GetColor(currentParentSliceIndex)));
                legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::HorizontalSeparator, *wxBLACK, *wxBLACK));
                }

            // add icon and text (after group separator, if needed)
            legendText.append(currentLabel.c_str()).append(L"\n");
            ++currentLine;
            legend->GetLegendIcons().emplace_back(
                    LegendIcon(IconShape::TriangleRightIcon, *wxBLACK, sliceColor));
            }
        legend->SetText(legendText.Trim());
        // show lines to make sure text is aligned as expected
        if (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
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
                    LegendIcon(IconShape::TriangleRightIcon, *wxBLACK,
                        GetColorScheme()->GetColor(i)));
            }
        legend->SetText(legendText.Trim());

        AdjustLegendSettings(legend, hint);
        return legend;
        }
    }
