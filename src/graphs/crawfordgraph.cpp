///////////////////////////////////////////////////////////////////////////////
// Name:        crawfordgraph.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "crawfordgraph.h"

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Graphs::CrawfordGraph, Wisteria::Graphs::GroupGraph2D)

    namespace Wisteria::Graphs
    {
    ///----------------------------------------------------------------
    CrawfordGraph::CrawfordGraph(
        Wisteria::Canvas * canvas,
        const std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme>& colors /*= nullptr*/,
        const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes /*= nullptr*/)
        : GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors : Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
                                           std::make_unique<Wisteria::Icons::Schemes::IconScheme>(
                                               Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"Crawford Graph"));
            GetCanvas()->SetName(_(L"Crawford Graph"));
            }
        GetTitle() = GraphItems::Label(GraphItems::GraphItemInfo(_(L"SPANISH READABILITY GRAPH"))
                                           .Scaling(GetScaling())
                                           .Pen(wxNullPen));
        GetLeftYAxis().GetTitle().SetText(_(L"Number of Syllables per 100 Words"));
        GetBottomXAxis().GetTitle().SetText(_(L"Approximate Grade Level of Reading Difficulty"));

        // set up the X axis
        GetBottomXAxis().SetRange(0.5, 7.0, 1, 0.5, 1);
        GetBottomXAxis().GetGridlinePen() = wxNullPen;
        GetBottomXAxis().ShowOuterLabels(false);

        // set up the Y axis
        GetLeftYAxis().SetRange(166, 222, 0, 2, 1);
        GetLeftYAxis().GetGridlinePen() = wxNullPen;
        GetLeftYAxis().ShowOuterLabels(false);
        GetLeftYAxis().EnableAutoStacking(false);
        }

    //----------------------------------------------------------------
    void CrawfordGraph::SetData(
        const std::shared_ptr<const Data::Dataset>& data, const wxString& scoreColumnName,
        const wxString& syllablesPer100WordsColumnName,
        const std::optional<const wxString>& groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            {
            return;
            }

        SetGroupColumn(groupColumnName);

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            {
            BuildGroupIdMap();
            }

        m_scoresColumn = GetContinuousColumnRequired(scoreColumnName);
        m_syllablesPer100WordsColumn = GetContinuousColumnRequired(syllablesPer100WordsColumnName);
        }

    //----------------------------------------------------------------
    void CrawfordGraph::RecalcSizes(wxDC & dc)
        {
        Graph2D::RecalcSizes(dc);

        wxPoint pt;
        if (GetPhysicalCoordinates(2.0, 218, pt))
            {
            auto sentenceLabel = std::make_unique<GraphItems::Label>(
                GraphItems::GraphItemInfo(_(L"Number of Sentences\nper 100 Words"))
                    .Scaling(GetScaling())
                    .Pen(wxNullPen)
                    .AnchorPoint(pt));
            sentenceLabel->SetTextAlignment(TextAlignment::Centered);
            AddObject(std::move(sentenceLabel));
            }

        const wxSize commonLabelSize = dc.ToDIP(dc.GetTextExtent(L"99.9"));

        const auto addTextPoint = [this, commonLabelSize](const double xValue, const double yValue,
                                                          const double textNumber,
                                                          const int precision)
        {
            wxPoint textPt;
            if (GetPhysicalCoordinates(xValue, yValue, textPt))
                {
                AddObject(std::make_unique<GraphItems::Label>(
                    GraphItems::GraphItemInfo()
                        .Scaling(GetScaling())
                        .Pen(wxNullPen)
                        .Padding(0, 0, 0, 0)
                        .MinimumUserSizeDIPs(commonLabelSize.GetWidth(), std::nullopt)
                        .LabelAlignment(TextAlignment::Centered)
                        .LabelPageHorizontalAlignment(PageHorizontalAlignment::RightAligned)
                        .FontColor(GetLeftYAxis().GetFontColor())
                        .Text(wxNumberFormatter::ToString(textNumber, precision,
                                                          wxNumberFormatter::Style::Style_None))
                        .AnchorPoint(textPt)));
                }
        };

        // 1.0 score
        addTextPoint(1.0, 186, 23.0, 1);
        addTextPoint(1.0, 184, 22.5, 1);
        addTextPoint(1.0, 182, 22.0, 1);
        addTextPoint(1.0, 180, 21.5, 1);
        addTextPoint(1.0, 178, 21.0, 1);
        addTextPoint(1.0, 176, 20.6, 1);
        addTextPoint(1.0, 174, 20.1, 1);
        addTextPoint(1.0, 172, 19.6, 1);
        addTextPoint(1.0, 170, 19.1, 1);
        addTextPoint(1.0, 168, 18.7, 1);

        // 1.5 score
        addTextPoint(1.5, 186, 20.5, 1);
        addTextPoint(1.5, 184, 20.0, 1);
        addTextPoint(1.5, 182, 19.6, 1);
        addTextPoint(1.5, 180, 19.1, 1);
        addTextPoint(1.5, 178, 18.6, 1);
        addTextPoint(1.5, 176, 18.1, 1);
        addTextPoint(1.5, 174, 17.7, 1);
        addTextPoint(1.5, 172, 17.2, 1);
        addTextPoint(1.5, 170, 16.7, 1);
        addTextPoint(1.5, 168, 16.2, 1);

        // 2.0 score
        addTextPoint(2.0, 190, 19.0, 1);
        addTextPoint(2.0, 188, 18.6, 1);
        addTextPoint(2.0, 186, 18.1, 1);
        addTextPoint(2.0, 184, 17.6, 1);
        addTextPoint(2.0, 182, 17.1, 1);
        addTextPoint(2.0, 180, 16.6, 1);
        addTextPoint(2.0, 178, 16.2, 1);
        addTextPoint(2.0, 176, 15.7, 1);
        addTextPoint(2.0, 174, 15.2, 1);
        addTextPoint(2.0, 172, 14.7, 1);
        addTextPoint(2.0, 170, 14.3, 1);

        // 2.5 score
        addTextPoint(2.5, 190, 16.6, 1);
        addTextPoint(2.5, 188, 16.1, 1);
        addTextPoint(2.5, 186, 15.6, 1);
        addTextPoint(2.5, 184, 15.2, 1);
        addTextPoint(2.5, 182, 14.7, 1);
        addTextPoint(2.5, 180, 14.2, 1);
        addTextPoint(2.5, 178, 13.7, 1);
        addTextPoint(2.5, 176, 13.3, 1);
        addTextPoint(2.5, 174, 12.8, 1);
        addTextPoint(2.5, 172, 12.3, 1);
        addTextPoint(2.5, 170, 11.8, 1);

        // 3.0 score
        addTextPoint(3.0, 200, 16.6, 1);
        addTextPoint(3.0, 198, 16.1, 1);
        addTextPoint(3.0, 196, 15.6, 1);
        addTextPoint(3.0, 194, 15.1, 1);
        addTextPoint(3.0, 192, 14.6, 1);
        addTextPoint(3.0, 190, 14.2, 1);
        addTextPoint(3.0, 188, 13.7, 1);
        addTextPoint(3.0, 186, 13.2, 1);
        addTextPoint(3.0, 184, 12.7, 1);
        addTextPoint(3.0, 182, 12.2, 1);
        addTextPoint(3.0, 180, 11.8, 1);
        addTextPoint(3.0, 178, 11.3, 1);
        addTextPoint(3.0, 176, 10.8, 1);

        // 3.5 score
        addTextPoint(3.5, 200, 14.1, 1);
        addTextPoint(3.5, 198, 13.6, 1);
        addTextPoint(3.5, 196, 13.2, 1);
        addTextPoint(3.5, 194, 12.7, 1);
        addTextPoint(3.5, 192, 12.2, 1);
        addTextPoint(3.5, 190, 11.7, 1);
        addTextPoint(3.5, 188, 11.2, 1);
        addTextPoint(3.5, 186, 10.8, 1);
        addTextPoint(3.5, 184, 10.3, 1);
        addTextPoint(3.5, 182, 9.8, 1);
        addTextPoint(3.5, 180, 9.3, 1);
        addTextPoint(3.5, 178, 8.9, 1);
        addTextPoint(3.5, 176, 8.4, 1);

        // 4.0 score
        addTextPoint(4.0, 206, 13.1, 1);
        addTextPoint(4.0, 204, 12.6, 1);
        addTextPoint(4.0, 202, 12.2, 1);
        addTextPoint(4.0, 200, 11.7, 1);
        addTextPoint(4.0, 198, 11.2, 1);
        addTextPoint(4.0, 196, 10.7, 1);
        addTextPoint(4.0, 194, 10.2, 1);
        addTextPoint(4.0, 192, 9.8, 1);
        addTextPoint(4.0, 190, 9.3, 1);
        addTextPoint(4.0, 188, 8.8, 1);
        addTextPoint(4.0, 186, 8.3, 1);
        addTextPoint(4.0, 184, 7.8, 1);
        addTextPoint(4.0, 182, 7.4, 1);
        addTextPoint(4.0, 180, 6.9, 1);
        addTextPoint(4.0, 178, 6.4, 1);
        addTextPoint(4.0, 176, 5.9, 1);

        // 4.5 score
        addTextPoint(4.5, 206, 10.7, 1);
        addTextPoint(4.5, 204, 10.2, 1);
        addTextPoint(4.5, 202, 9.7, 1);
        addTextPoint(4.5, 200, 9.2, 1);
        addTextPoint(4.5, 198, 8.8, 1);
        addTextPoint(4.5, 196, 8.3, 1);
        addTextPoint(4.5, 194, 7.8, 1);
        addTextPoint(4.5, 192, 7.3, 1);
        addTextPoint(4.5, 190, 6.8, 1);
        addTextPoint(4.5, 188, 6.4, 1);
        addTextPoint(4.5, 186, 5.9, 1);
        addTextPoint(4.5, 184, 5.4, 1);
        addTextPoint(4.5, 182, 4.9, 1);
        addTextPoint(4.5, 180, 4.5, 1);
        addTextPoint(4.5, 178, 4.0, 1);
        addTextPoint(4.5, 176, 3.5, 1);

        // 5.0 score
        addTextPoint(5.0, 212, 9.7, 1);
        addTextPoint(5.0, 210, 9.2, 1);
        addTextPoint(5.0, 208, 8.7, 1);
        addTextPoint(5.0, 206, 8.2, 1);
        addTextPoint(5.0, 204, 7.8, 1);
        addTextPoint(5.0, 202, 7.3, 1);
        addTextPoint(5.0, 200, 6.8, 1);
        addTextPoint(5.0, 198, 6.3, 1);
        addTextPoint(5.0, 196, 5.8, 1);
        addTextPoint(5.0, 194, 5.4, 1);
        addTextPoint(5.0, 192, 4.5, 1);

        // 5.5 score
        addTextPoint(5.5, 212, 7.2, 1);
        addTextPoint(5.5, 210, 6.7, 1);
        addTextPoint(5.5, 208, 6.3, 1);
        addTextPoint(5.5, 206, 5.8, 1);
        addTextPoint(5.5, 204, 5.3, 1);
        addTextPoint(5.5, 202, 4.8, 1);
        addTextPoint(5.5, 200, 4.4, 1);
        addTextPoint(5.5, 198, 3.9, 1);
        addTextPoint(5.5, 196, 3.4, 1);
        addTextPoint(5.5, 194, 2.9, 1);
        addTextPoint(5.5, 192, 2.4, 1);

        // 6.0 score
        addTextPoint(6.0, 214, 5.3, 1);
        addTextPoint(6.0, 212, 4.8, 1);
        addTextPoint(6.0, 210, 4.3, 1);
        addTextPoint(6.0, 208, 3.8, 1);
        addTextPoint(6.0, 206, 3.4, 1);
        addTextPoint(6.0, 204, 2.9, 1);
        addTextPoint(6.0, 202, 2.4, 1);
        addTextPoint(6.0, 200, 1.9, 1);

        // 6.5 score
        addTextPoint(6.5, 220, 4.3, 1);
        addTextPoint(6.5, 218, 3.8, 1);
        addTextPoint(6.5, 216, 3.3, 1);
        addTextPoint(6.5, 214, 2.8, 1);
        addTextPoint(6.5, 212, 2.3, 1);
        addTextPoint(6.5, 210, 1.9, 1);
        addTextPoint(6.5, 208, 1.4, 1);
        addTextPoint(6.5, 206, 1.0, 1);

        if (GetDataset() == nullptr)
            {
            return;
            }

        // plot the data
        auto points = std::make_unique<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_scoresColumn->GetValue(i)) ||
                std::isnan(m_syllablesPer100WordsColumn->GetValue(i)))
                {
                continue;
                }

            const auto currentScore = std::clamp<double>(m_scoresColumn->GetValue(i), 0.5, 7.0);
            const auto currentSyllableCount =
                std::clamp<double>(m_syllablesPer100WordsColumn->GetValue(i), 166, 222);

            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex =
                IsUsingGrouping() ? GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) : 0;

            if (GetPhysicalCoordinates(currentScore, currentSyllableCount, pt))
                {
                points->AddPoint(
                    GraphItems::Point2D(
                        GraphItems::GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i))
                            .AnchorPoint(pt)
                            .Pen(Wisteria::Colors::ColorContrast::BlackOrWhiteContrast(
                                GetPlotOrCanvasColor()))
                            .Brush(GetColorScheme()->GetColor(colorIndex)),
                        Settings::GetPointRadius(), GetShapeScheme()->GetShape(colorIndex)),
                    dc);
                }
            }
        AddObject(std::move(points));
        }
    } // namespace Wisteria::Graphs
