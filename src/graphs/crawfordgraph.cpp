///////////////////////////////////////////////////////////////////////////////
// Name:        crawfordgraph.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "crawfordgraph.h"

using namespace Wisteria;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Graphs;

namespace Wisteria::Graphs
    {
    ///----------------------------------------------------------------
    CrawfordGraph::CrawfordGraph(Wisteria::Canvas* canvas,
            std::shared_ptr<Wisteria::Colors::Schemes::ColorScheme> colors /*= nullptr*/,
            std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> shapes /*= nullptr*/) :
        GroupGraph2D(canvas)
        {
        SetColorScheme(colors != nullptr ? colors :
            Settings::GetDefaultColorScheme());
        SetShapeScheme(shapes != nullptr ? shapes :
            std::make_shared<Wisteria::Icons::Schemes::IconScheme>(
                Wisteria::Icons::Schemes::StandardShapes()));

        if (GetCanvas() != nullptr)
            {
            GetCanvas()->SetLabel(_(L"Crawford Graph"));
            GetCanvas()->SetName(_(L"Crawford Graph"));
            }
        GetTitle() = GraphItems::Label(GraphItemInfo(_(L"SPANISH READABILITY GRAPH")).
                Scaling(GetScaling()).Pen(wxNullPen));
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
                            std::shared_ptr<const Data::Dataset> data,
                            const wxString& scoreColumnName,
                            const wxString& syllablesPer100WordsColumnName,
                            std::optional<const wxString> groupColumnName /*= std::nullopt*/)
        {
        SetDataset(data);
        ResetGrouping();
        GetSelectedIds().clear();

        if (GetDataset() == nullptr)
            { return; }

        SetGroupColumn(groupColumnName);

        // if grouping, build the list of group IDs, sorted by their respective labels
        if (IsUsingGrouping())
            { BuildGroupIdMap(); }

        m_scoresColumn = GetContinuousColumnRequired(scoreColumnName);
        m_syllablesPer100WordsColumn =
            GetContinuousColumnRequired(syllablesPer100WordsColumnName);
        }

    //----------------------------------------------------------------
    void CrawfordGraph::RecalcSizes(wxDC& dc)
        {
        Graph2D::RecalcSizes(dc);

        wxPoint pt;
        if (GetPhysicalCoordinates(2.0, 218, pt) )
            {
            auto sentenceLabel = std::make_shared<GraphItems::Label>(
                GraphItemInfo(_(L"Number of Sentences\nper 100 Words")).
                Scaling(GetScaling()).Pen(wxNullPen).AnchorPoint(pt));
            sentenceLabel->SetTextAlignment(TextAlignment::Centered);
            AddObject(sentenceLabel);
            }

        const auto addTextPoint = [this]
            (const double xValue, const double yValue,
            const double textNumber, const int precision)
            {
            wxPoint textPt;
            if (GetPhysicalCoordinates(xValue, yValue, textPt) )
                {
                AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                    Scaling(GetScaling()).Pen(wxNullPen).
                    Text(wxNumberFormatter::ToString(textNumber, precision,
                         wxNumberFormatter::Style::Style_None)).
                    AnchorPoint(textPt)));
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
        if (GetPhysicalCoordinates(1.5, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(20.5, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(20.0, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(19.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(19.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(18.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).Text(
                    wxNumberFormatter::ToString(18.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 174, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(17.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 172, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(17.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 170, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(1.5, 168, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 2.0 score
        if (GetPhysicalCoordinates(2.0, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(19.0, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 188, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(18.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(18.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(17.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(17.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 174, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 172, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.0, 170, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 2.5 score
        if (GetPhysicalCoordinates(2.5, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 188, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt))); }
        if (GetPhysicalCoordinates(2.5, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 174, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 172, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.3, 1, wxNumberFormatter::Style::Style_None))
                .AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(2.5, 170, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 3.0 score
        if (GetPhysicalCoordinates(3.0, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(16.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(15.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 188, pt) )
            { AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.0, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 3.5 score
        if (GetPhysicalCoordinates(3.5, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(14.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 188, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt))); }
        if (GetPhysicalCoordinates(3.5, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.3, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.9, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(3.5, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.4, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        // 4.0 score
        if (GetPhysicalCoordinates(4.0, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(13.1, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 204, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.6, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 202, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(12.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(11.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.3, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 188, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.3, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.4, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.9, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.4, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.0, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.9, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        // 4.5 score
        if (GetPhysicalCoordinates(4.5, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 204, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(10.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 202, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.7, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.2, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt))); }
        if (GetPhysicalCoordinates(4.5, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.3, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.3, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 190, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.8, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 188, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.4, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 186, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.9, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt))); }
        if (GetPhysicalCoordinates(4.5, 184, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.4, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 182, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.9, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 180, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.5, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 178, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.0, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(4.5, 176, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.5, 1, wxNumberFormatter::Style::Style_None).
                    Prepend(L" ")).AnchorPoint(pt)));
            }
        // 5.0 score
        if (GetPhysicalCoordinates(5.0, 212, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 210, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(9.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 208, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(8.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 204, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 202, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.0, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.5, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 5.5 score
        if (GetPhysicalCoordinates(5.5, 212, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(7.2, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 210, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.7, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 208, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(6.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 204, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 202, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 198, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.9, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 196, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 194, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.9, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(5.5, 192, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 6.0 score
        if (GetPhysicalCoordinates(6.0, 214, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(5.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 212, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 210, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 208, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 204, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.9, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 202, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.0, 200, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(1.9, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        // 6.5 score
        if (GetPhysicalCoordinates(6.5, 220, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(4.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 218, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 216, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(3.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 214, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.8, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 212, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(2.3, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 210, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(1.9, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 208, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(1.4, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }
        if (GetPhysicalCoordinates(6.5, 206, pt) )
            {
            AddObject(std::make_shared<GraphItems::Label>(GraphItemInfo().
                Scaling(GetScaling()).Pen(wxNullPen).
                Text(wxNumberFormatter::ToString(1.0, 1, wxNumberFormatter::Style::Style_None)).
                AnchorPoint(pt)));
            }

        if (GetDataset() == nullptr)
            { return; }

        // plot the data
        auto points = std::make_shared<GraphItems::Points2D>(wxNullPen);
        points->SetScaling(GetScaling());
        points->SetDPIScaleFactor(GetDPIScaleFactor());
        points->Reserve(GetDataset()->GetRowCount());
        for (size_t i = 0; i < GetDataset()->GetRowCount(); ++i)
            {
            if (std::isnan(m_scoresColumn->GetValue(i)) ||
                std::isnan(m_syllablesPer100WordsColumn->GetValue(i)))
                { continue; }

            const auto currentScore =
                std::clamp<double>(m_scoresColumn->GetValue(i), 0.5, 7.0);
            const auto currentSyllableCount =
                std::clamp<double>(m_syllablesPer100WordsColumn->GetValue(i), 166, 222);

            // Convert group ID into color scheme index
            // (index is ordered by labels alphabetically).
            // Note that this will be zero if grouping is not in use.
            const size_t colorIndex = IsUsingGrouping() ?
                GetSchemeIndexFromGroupId(GetGroupColumn()->GetValue(i)) :
                0;

            if (GetPhysicalCoordinates(currentScore, currentSyllableCount, pt))
                {
                points->AddPoint(Point2D(
                    GraphItemInfo(GetDataset()->GetIdColumn().GetValue(i)).
                    AnchorPoint(pt).
                    Brush(GetColorScheme()->GetColor(colorIndex)),
                    Settings::GetPointRadius(),
                    GetShapeScheme()->GetShape(colorIndex)), dc);
                }
            }
        AddObject(points);
        }
    }