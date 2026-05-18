///////////////////////////////////////////////////////////////////////////////
// Name:        shapes_education.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "image.h"
#include "label.h"
#include "lines.h"
#include "shapes.h"
#include <wx/dcgraph.h>
#include <wx/graphics.h>

namespace Wisteria::GraphItems
    {
    //---------------------------------------------------
    void ShapeRenderer::DrawClock(const wxRect rect, wxDC& dc) const
        {
        const wxDCBrushChanger bc{ dc, *wxTRANSPARENT_BRUSH };
        wxRect dcRect{ rect };
        dcRect.Deflate(ScaleToScreenAndCanvas(2));
        const wxPoint centerPt(GetXPosFromLeft(dcRect, math_constants::half),
                               GetYPosFromTop(dcRect, math_constants::half));
        // draw the frame
        const DCPenChangerIfDifferent pc(
            dc, wxPen(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                      ScaleToScreenAndCanvas(2)));
        dc.DrawCircle(centerPt, dcRect.GetWidth() * math_constants::half);

        // draw the minutes
        wxRect intervalsRect{ rect };
        intervalsRect.Deflate(dcRect.GetWidth() * math_constants::fifth);
        const DCPenChangerIfDifferent pc2(
            dc, wxPenInfo(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                          ScaleToScreenAndCanvas(1), wxPENSTYLE_DOT));
        dc.DrawCircle(centerPt, intervalsRect.GetWidth() * math_constants::half);

        // draw the arms (at 4:30)
        wxRect armsRect{ rect };
        armsRect.Deflate(dcRect.GetWidth() * math_constants::quarter);
        const DCPenChangerIfDifferent pc3(
            dc, wxPenInfo(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
                          ScaleToScreenAndCanvas(2)));
        dc.DrawLine(centerPt, wxPoint(centerPt.x + (armsRect.GetWidth() * math_constants::quarter),
                                      armsRect.GetBottom() -
                                          (armsRect.GetHeight() * math_constants::quarter)));
        dc.DrawLine(centerPt, wxPoint(centerPt.x, armsRect.GetBottom()));

        // seconds hand
        const DCPenChangerIfDifferent pc4(
            dc, wxPenInfo(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Red)),
                          ScaleToScreenAndCanvas(1)));
        dc.DrawLine(wxPoint(centerPt.x + (armsRect.GetWidth() * math_constants::tenth), centerPt.y),
                    wxPoint(centerPt.x - (armsRect.GetWidth() * math_constants::half), centerPt.y));
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBlackboard(const wxRect rect, wxDC& dc) const
        {
        wxRect dcRect{ rect };
        dcRect.Deflate(ScaleToScreenAndCanvas(2));
        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedHeight{ dcRect.GetHeight() * 0.6 };
            const auto adjustTop{ (dcRect.GetHeight() - adjustedHeight) * math_constants::half };
            dcRect.SetHeight(adjustedHeight);
            dcRect.Offset(wxPoint(0, adjustTop));
            }

        const wxCoord frameWidth = dcRect.GetWidth() * math_constants::tenth;

        dc.GradientFillLinear(
            dcRect, TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::WarmGray)),
            TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)), wxEAST);

        const wxDCPenChanger pc(
            dc, wxPen(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::YellowPepper)),
                      frameWidth));
        const wxDCBrushChanger bc(dc, wxColour{ 0, 0, 0, 0 });
        dc.DrawRectangle(dcRect);

        // draw "ABC" on the board
        wxRect textRect{ dcRect };
        textRect.SetWidth(dcRect.GetWidth() * math_constants::half);
        textRect.SetHeight(dcRect.GetHeight() * math_constants::half);
        textRect.Offset(wxPoint(frameWidth, frameWidth));

        Label boardText(
            GraphItemInfo{ /* TRANSLATORS: Simple placeholder text of any sort */
                           _("ABC") }
                .FontColor(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White)))
                .Pen(wxNullPen)
                .DPIScaling(GetDPIScaleFactor())
                .Scaling(GetScaling()));
        boardText.GetFont().MakeBold().SetFaceName(Label::GetFirstAvailableCursiveFont());
        boardText.SetBoundingBox(textRect, dc, GetScaling());
        boardText.Draw(dc);

        // draw a piece of chalk
        const wxDCPenChanger pc2(
            dc, wxPenInfo(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::White)),
                          frameWidth / 2)
                    .Cap(wxCAP_BUTT));
        wxPoint chalkRight{ dcRect.GetBottomRight() };
        chalkRight.y -= frameWidth - (frameWidth / 4);
        chalkRight.x -= ScaleToScreenAndCanvas(2);
        wxPoint chalkLeft{ chalkRight };
        chalkLeft.x -= dcRect.GetWidth() * math_constants::fifth;
        dc.DrawLine(chalkLeft, chalkRight);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawNewspaper(const wxRect rect, wxDC& dc) const
        {
        // write random horizontal lines of dots inside a rect to simulate text
        const auto writeText = [&dc, &rect, this](const wxRect textRect)
        {
            wxPoint textLeft{ textRect.GetTopLeft() };
            wxPoint textRight{ textRect.GetTopRight() };
            // use a random selection of pen dash styles for each line to simulate text
            std::uniform_int_distribution<> randPenStyle(wxPENSTYLE_LONG_DASH, wxPENSTYLE_DOT_DASH);
            size_t currentLine{ 0 };
            while (textLeft.y < textRect.GetBottom())
                {
                const wxDCPenChanger pc3{
                    dc, wxPen{ ApplyColorOpacity(
                                   Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)),
                               std::max<int>(1, ScaleToScreenAndCanvas(
                                                    rect.GetWidth() <= ScaleToScreenAndCanvas(16) ?
                                                        math_constants::whole :
                                                        math_constants::half)),
                               static_cast<wxPenStyle>(randPenStyle(GetRNG())) }
                };
                if ((currentLine % 10) > 0)
                    {
                    dc.DrawLine(textLeft, textRight);
                    }
                // indent every 10th line
                else
                    {
                    dc.DrawLine(wxPoint{ static_cast<int>(textLeft.x + (textRect.GetWidth() *
                                                                        math_constants::fifth)),
                                         textLeft.y },
                                textRight);
                    }
                textLeft.y += std::max<int>(1, ScaleToScreenAndCanvas(2));
                textRight.y += std::max<int>(1, ScaleToScreenAndCanvas(2));
                ++currentLine;
                }
        };

        const wxDCPenChanger pc{
            dc, wxPen{ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)),
                       std::max<int>(1, ScaleToScreenAndCanvas(1)) }
        };
        const wxDCBrushChanger bc{ dc, Colors::ColorBrewer::GetColor(Colors::Color::White) };

        wxRect frontPageRect{ rect };
        frontPageRect.Deflate(frontPageRect.GetSize() * 0.1);

        auto backPage{ frontPageRect };
        backPage.SetWidth(frontPageRect.GetWidth() * 1.1);
        backPage.SetHeight(frontPageRect.GetHeight() * math_constants::three_fourths);
        backPage.Offset(0, frontPageRect.GetHeight() - backPage.GetHeight());

            // draw the lower (folded) section of the backpage
            {
            auto bottomRect{ backPage };
            bottomRect.SetHeight(bottomRect.GetHeight() * math_constants::half);
            bottomRect.Offset(0, backPage.GetHeight() - bottomRect.GetHeight());
            wxRect currentClipRect;
            wxRegion clipRegion(bottomRect);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            dc.DrawRoundedRectangle(backPage, ScaleToScreenAndCanvas(3));
            }
            // draw the upper half of the backpage
            {
            auto topRect{ backPage };
            topRect.SetHeight(topRect.GetHeight() *
                              // avoid a gap in the lines
                              (math_constants::half + 0.05));
            wxRect currentClipRect;
            wxRegion clipRegion(topRect);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            dc.DrawRectangle(backPage);
            }
            // draw the front page
            {
            auto topRect{ frontPageRect };
            topRect.SetHeight(topRect.GetHeight() * 0.9);
            wxRect currentClipRect;
            wxRegion clipRegion(topRect);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            dc.DrawRectangle(frontPageRect);
            }

        const wxDCPenChanger pc2{ dc, *wxTRANSPARENT_PEN };

        // headline
        const wxPen sepLinePen{ ApplyColorOpacity(
                                    Colors::ColorBrewer::GetColor(Colors::Color::WarmGray)),
                                std::max<int>(1, ScaleToScreenAndCanvas(math_constants::half)) };
        auto headlineBox{ frontPageRect };
        headlineBox.SetHeight(headlineBox.GetHeight() * math_constants::third);
        headlineBox.Deflate(ScaleToScreenAndCanvas(2));
        // TRANSLATORS: Name of a newspaper drawn on newspaper icons used for graphs.
        Label headline(GraphItemInfo{ _("DAYTON TIMES") }
                           .DPIScaling(GetDPIScaleFactor())
                           .Scaling(GetScaling())
                           .Pen(sepLinePen)
                           .Outline(false, false, true, false));
        headline.SetFontColor(ApplyColorOpacity(headline.GetFontColor()));
        headline.SetBoundingBox(headlineBox, dc, GetScaling());
        headline.Draw(dc);
        headlineBox.Offset(wxPoint{ 0, static_cast<int>(ScaleToScreenAndCanvas(1)) });

        // picture on the front page
        auto pictureBox{ frontPageRect };
        pictureBox.SetHeight(frontPageRect.GetHeight() * math_constants::quarter);
        pictureBox.SetWidth(frontPageRect.GetWidth() * math_constants::fourth);
        pictureBox.SetTop(headlineBox.GetBottom() + ScaleToScreenAndCanvas(1));
        pictureBox.Offset(wxPoint{ static_cast<int>(ScaleToScreenAndCanvas(2)), 0 });
        dc.DrawRectangle(pictureBox);
        dc.GradientFillLinear(
            pictureBox, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Afternoon)),
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::BlueSky)));
        wxRect sunRect{ pictureBox };
        sunRect.SetWidth(sunRect.GetWidth() * math_constants::three_quarters);
        sunRect.SetHeight(sunRect.GetWidth());
            {
            wxRect currentClipRect;
            wxRegion clipRegion(pictureBox);
            if (dc.GetClippingBox(currentClipRect))
                {
                clipRegion.Intersect(currentClipRect);
                }
            const wxDCClipper clip{ dc, clipRegion };
            DrawSun(sunRect, dc);
            }

        // TOC below the picture
        auto tocBox{ pictureBox };
        tocBox.SetTop(pictureBox.GetBottom() + ScaleToScreenAndCanvas(2));
        tocBox.SetBottom(frontPageRect.GetBottom() - ScaleToScreenAndCanvas(2));
        dc.DrawRectangle(tocBox);
        writeText(tocBox.Deflate(ScaleToScreenAndCanvas(1)));

        // column separator
        wxPoint columnTop(pictureBox.GetTopRight());
        columnTop.x += ScaleToScreenAndCanvas(1);
        const wxPoint columnBottom(columnTop.x,
                                   frontPageRect.GetBottom() - ScaleToScreenAndCanvas(2));
        const wxDCPenChanger sepLinePenChanger{ dc, sepLinePen };
        dc.DrawLine(columnTop, columnBottom);

        // text on the right side
        auto rightTextRect{ frontPageRect };
        rightTextRect.SetWidth(frontPageRect.GetRight() - columnTop.x - ScaleToScreenAndCanvas(4));
        headlineBox.Offset(wxPoint{ 0, static_cast<int>(ScaleToScreenAndCanvas(1)) });
        rightTextRect.SetHeight(frontPageRect.GetBottom() - headlineBox.GetBottom() -
                                ScaleToScreenAndCanvas(4));
        rightTextRect.SetTopLeft(columnTop);
        rightTextRect.Offset(ScaleToScreenAndCanvas(2), ScaleToScreenAndCanvas(2));

        writeText(rightTextRect);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawBook(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard(dc, Colors::ColorBrewer::GetColor(Colors::Color::Black));
        const wxDCBrushChanger brushGuard(dc, Colors::ColorBrewer::GetColor(Colors::Color::Black));

        const wxColour bookColor{ TintIfUsingOpacity(Colors::ColorContrast::ChangeOpacity(
            GetGraphItemInfo().GetBrush().GetColour(), wxALPHA_OPAQUE)) };

        const std::array<wxPoint, 4> bookCover = {
            wxPoint(GetXPosFromLeft(rect, math_constants::tenth),
                    GetYPosFromTop(rect, math_constants::half)),
            wxPoint(GetXPosFromLeft(rect, 0.6), GetYPosFromTop(rect, 0.1)),
            wxPoint(GetXPosFromLeft(rect, 0.9), GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, 0.4),
                    GetYPosFromTop(rect, math_constants::three_quarters))
        };

        std::array<wxPoint, 4> bookCoverBottom = { bookCover };
        const double yOffset = GetYPosFromTop(rect, 0.9) - bookCover[3].y;
        for (auto& pt : bookCoverBottom)
            {
            pt.y += yOffset;
            }

        const std::array<wxPoint, 4> spine = { bookCover[0], bookCover[1], bookCoverBottom[1],
                                               bookCoverBottom[0] };

        // the pages
        auto frontOfPagesTopLeft =
            geometry::point_along_line(std::make_pair(bookCover[0].x, bookCover[0].y),
                                       std::make_pair(bookCover[3].x, bookCover[3].y), 0.1);
        auto frontOfPagesTopRight =
            geometry::point_along_line(std::make_pair(bookCover[0].x, bookCover[0].y),
                                       std::make_pair(bookCover[3].x, bookCover[3].y), 0.95);
        auto frontOfPagesBottomLeft = geometry::point_along_line(
            std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
            std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y), 0.1);
        auto frontOfPagesBottomRight = geometry::point_along_line(
            std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
            std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y), 0.95);
        std::array<wxPoint, 4> pagesFront = {
            wxPoint(frontOfPagesTopLeft.first, frontOfPagesTopLeft.second),
            wxPoint(frontOfPagesTopRight.first, frontOfPagesTopRight.second),
            wxPoint(frontOfPagesBottomRight.first, frontOfPagesBottomRight.second),
            wxPoint(frontOfPagesBottomLeft.first, frontOfPagesBottomLeft.second),
        };

        auto sideOfPagesTopRight =
            geometry::point_along_line(std::make_pair(bookCover[1].x, bookCover[1].y),
                                       std::make_pair(bookCover[2].x, bookCover[2].y), 0.95);
        auto sideOfPagesBottomRight = geometry::point_along_line(
            std::make_pair(bookCoverBottom[1].x, bookCoverBottom[1].y),
            std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y), 0.95);
        std::array<wxPoint, 4> pagesSide = {
            pagesFront[1], wxPoint(sideOfPagesTopRight.first, sideOfPagesTopRight.second),
            wxPoint(sideOfPagesBottomRight.first, sideOfPagesBottomRight.second), pagesFront[2]
        };

        wxPen scaledPenMain(ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Red)),
                            ScaleToScreenAndCanvas(1));
        scaledPenMain.SetCap(wxPenCap::wxCAP_BUTT);
        const DCPenChangerIfDifferent pcMain{ dc, scaledPenMain };

            // draw the bottom of the book
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            const DCPenChangerIfDifferent pc{ dc, scaledPen };
            const DCBrushChangerIfDifferent bc(dc, bookColor);
            dc.DrawPolygon(bookCoverBottom.size(), bookCoverBottom.data());
            // a highlight along the bottom edge
            scaledPen.SetColour(
                Colors::ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), 0.4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            const DCPenChangerIfDifferent pc2{ dc, scaledPen };
            dc.DrawLine(bookCoverBottom[0], bookCoverBottom[3]);

            // gold trim on edges of book
            scaledPen.SetWidth(ScaleToScreenAndCanvas(1));
            scaledPen.SetColour(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::GoldLeaf)));
            const DCPenChangerIfDifferent pc3{ dc, scaledPen };
            auto topCornerLeft = geometry::point_along_line(
                std::make_pair(bookCoverBottom[1].x, bookCoverBottom[1].y),
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y), 0.9);
            auto topCornerRight = geometry::point_along_line(
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y), 0.1);
            std::array<wxPoint, 3> topLeftGoldLeaf = {
                wxPoint(topCornerLeft.first, topCornerLeft.second), bookCoverBottom[2],
                wxPoint(topCornerRight.first, topCornerRight.second)
            };
            auto bottomCornerLeft = geometry::point_along_line(
                std::make_pair(bookCoverBottom[2].x, bookCoverBottom[2].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y), 0.9);
            auto bottomCornerRight = geometry::point_along_line(
                std::make_pair(bookCoverBottom[0].x, bookCoverBottom[0].y),
                std::make_pair(bookCoverBottom[3].x, bookCoverBottom[3].y), 0.9);
            std::array<wxPoint, 3> bottomLeftGoldLeaf = {
                wxPoint(bottomCornerLeft.first, bottomCornerLeft.second), bookCoverBottom[3],
                wxPoint(bottomCornerRight.first, bottomCornerRight.second)
            };
            dc.DrawLines(topLeftGoldLeaf.size(), topLeftGoldLeaf.data());
            dc.DrawLines(bottomLeftGoldLeaf.size(), bottomLeftGoldLeaf.data());

            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetColour(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Gold)));
            const DCPenChangerIfDifferent pc4{ dc, scaledPen };
            dc.DrawLines(topLeftGoldLeaf.size(), topLeftGoldLeaf.data());
            dc.DrawLines(bottomLeftGoldLeaf.size(), bottomLeftGoldLeaf.data());
            }

            // draw the spine
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            const DCPenChangerIfDifferent pc{ dc, scaledPen };
            const DCBrushChangerIfDifferent bc(dc, bookColor);
            dc.DrawPolygon(spine.size(), spine.data());
            // a highlight along the edge
            scaledPen.SetColour(
                Colors::ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), 0.4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            const DCPenChangerIfDifferent pc2{ dc, scaledPen };
            dc.DrawLine(spine[0], spine[3]);
            }

            // draw the pages
            {
            const DCBrushChangerIfDifferent bc(
                dc, TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::AntiqueWhite)));
            const DCPenChangerIfDifferent pc{ dc, wxColour{ 0, 0, 0, 0 } };
            dc.DrawPolygon(pagesFront.size(), pagesFront.data());
            }

            {
            const DCBrushChangerIfDifferent bc(
                dc, TintIfUsingOpacity(Colors::ColorBrewer::GetColor(Colors::Color::LightGray)));
            const DCPenChangerIfDifferent pc{ dc, wxColour{ 0, 0, 0, 0 } };
            dc.DrawPolygon(pagesSide.size(), pagesSide.data());
            }

            // draw the cover
            {
            wxPen scaledPen(GetGraphItemInfo().GetBrush().GetColour(), ScaleToScreenAndCanvas(1));
            const DCPenChangerIfDifferent pc{ dc, scaledPen };
            const DCBrushChangerIfDifferent bc(dc, bookColor);
            dc.DrawPolygon(bookCover.size(), bookCover.data());
            // a highlight along the bottom edge
            scaledPen.SetColour(
                Colors::ColorContrast::ShadeOrTint(GetGraphItemInfo().GetBrush().GetColour(), 0.4));
            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
            scaledPen.SetCap(wxPenCap::wxCAP_ROUND);
            const DCPenChangerIfDifferent pc2{ dc, scaledPen };
            dc.DrawLine(bookCover[0], bookCover[3]);

                // gold leaf on cover of book
                {
                std::array<std::pair<double, double>, 4> goldLeafPoints = {
                    std::make_pair(bookCover[0].x, bookCover[0].y),
                    std::make_pair(bookCover[1].x, bookCover[1].y),
                    std::make_pair(bookCover[2].x, bookCover[2].y),
                    std::make_pair(bookCover[3].x, bookCover[3].y)
                };
                geometry::deflate_rect(goldLeafPoints[0], goldLeafPoints[1], goldLeafPoints[2],
                                       goldLeafPoints[3], 0.8);
                std::array<wxPoint, 5> goldLeafPointsPt = {
                    wxPoint(goldLeafPoints[0].first, goldLeafPoints[0].second),
                    wxPoint(goldLeafPoints[1].first, goldLeafPoints[1].second),
                    wxPoint(goldLeafPoints[2].first, goldLeafPoints[2].second),
                    wxPoint(goldLeafPoints[3].first, goldLeafPoints[3].second),
                    wxPoint(goldLeafPoints[0].first, goldLeafPoints[0].second)
                };
                scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::half));
                scaledPen.SetColour(
                    ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::GoldLeaf)));
                const DCPenChangerIfDifferent pc3{ dc, scaledPen };
                dc.DrawLines(goldLeafPointsPt.size(), goldLeafPointsPt.data());
                }

            // gold trim on edges of book
            scaledPen.SetWidth(ScaleToScreenAndCanvas(1));
            scaledPen.SetColour(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::GoldLeaf)));
            const DCPenChangerIfDifferent pc3{ dc, scaledPen };
            auto topCornerLeft =
                geometry::point_along_line(std::make_pair(bookCover[1].x, bookCover[1].y),
                                           std::make_pair(bookCover[2].x, bookCover[2].y), 0.9);
            auto topCornerRight =
                geometry::point_along_line(std::make_pair(bookCover[2].x, bookCover[2].y),
                                           std::make_pair(bookCover[3].x, bookCover[3].y), 0.1);
            std::array<wxPoint, 3> topLeftGoldLeaf = {
                wxPoint(topCornerLeft.first, topCornerLeft.second), bookCover[2],
                wxPoint(topCornerRight.first, topCornerRight.second)
            };
            auto bottomCornerLeft =
                geometry::point_along_line(std::make_pair(bookCover[2].x, bookCover[2].y),
                                           std::make_pair(bookCover[3].x, bookCover[3].y), 0.9);
            auto bottomCornerRight =
                geometry::point_along_line(std::make_pair(bookCover[0].x, bookCover[0].y),
                                           std::make_pair(bookCover[3].x, bookCover[3].y), 0.9);
            std::array<wxPoint, 3> bottomLeftGoldLeaf = {
                wxPoint(bottomCornerLeft.first, bottomCornerLeft.second), bookCover[3],
                wxPoint(bottomCornerRight.first, bottomCornerRight.second)
            };
            dc.DrawLines(topLeftGoldLeaf.size(), topLeftGoldLeaf.data());
            dc.DrawLines(bottomLeftGoldLeaf.size(), bottomLeftGoldLeaf.data());

            scaledPen.SetWidth(ScaleToScreenAndCanvas(math_constants::quarter));
            scaledPen.SetColour(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Gold)));
            const DCPenChangerIfDifferent pc4{ dc, scaledPen };
            dc.DrawLines(topLeftGoldLeaf.size(), topLeftGoldLeaf.data());
            dc.DrawLines(bottomLeftGoldLeaf.size(), bottomLeftGoldLeaf.data());
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawGraduationCap(const wxRect rect, wxDC& dc) const
        {
        if (rect.GetWidth() == rect.GetHeight())
            {
            SetYOffsetPercentage(0.05);
            }

        wxPen scaledPen(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray),
                        ScaleToScreenAndCanvas(1.0));
        const DCPenChangerIfDifferent pc{ dc, scaledPen };

        const std::array<wxPoint, 4> hatTop = {
            wxPoint(GetXPosFromLeft(rect, 0), GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half), GetYPosFromTop(rect, 0)),
            wxPoint(GetXPosFromLeft(rect, math_constants::full),
                    GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, math_constants::half),
                    GetYPosFromTop(rect, math_constants::two_thirds))
        };

        const auto hatTopLeftMidPoint = geometry::point_along_line(
            std::make_pair(hatTop[0].x, hatTop[0].y), std::make_pair(hatTop[3].x, hatTop[3].y),
            math_constants::third);
        const auto hatTopRightMidPoint = geometry::point_along_line(
            std::make_pair(hatTop[3].x, hatTop[3].y), std::make_pair(hatTop[2].x, hatTop[2].y),
            math_constants::two_thirds);

        const std::array<wxPoint, 6> hatStem = {
            wxPoint(hatTopLeftMidPoint.first, hatTopLeftMidPoint.second),
            hatTop[3],
            wxPoint(hatTopRightMidPoint.first, hatTopRightMidPoint.second),
            wxPoint(hatTopRightMidPoint.first,
                    GetYPosFromTop(rect, math_constants::three_fourths - 0.1)),
            wxPoint(hatTop[3].x, GetYPosFromTop(rect, math_constants::full - 0.1)),
            wxPoint(hatTopLeftMidPoint.first,
                    GetYPosFromTop(rect, math_constants::three_fourths - 0.1)),
        };

            {
            const wxBrush shadowedBrush(
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SmokyBlack)));
            const DCBrushChangerIfDifferent bc(dc, shadowedBrush);
            dc.DrawPolygon(hatStem.size(), hatStem.data());
            }

            {
            const DCBrushChangerIfDifferent bc(
                dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::TricornBlack)));
            dc.DrawPolygon(hatTop.size(), hatTop.data());
            }

        scaledPen.SetColour(
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::HarvestGold)));
        const DCPenChangerIfDifferent pc2{ dc, scaledPen };
        const DCBrushChangerIfDifferent bc(
            dc, ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::HarvestGold)));
        const wxPoint hatTopMidPoint(GetXPosFromLeft(rect, math_constants::half),
                                     GetYPosFromTop(rect, math_constants::third));

        // button holding the thread to the top of the hat
        const auto threadWidth{ std::ceil(safe_divide<double>(rect.GetWidth(), 32)) };
        dc.DrawEllipticArc(hatTopMidPoint - wxPoint((threadWidth * 1.5), (threadWidth * 1.5)),
                           wxSize((threadWidth * 3), (threadWidth * 3)), 0, 180);
        dc.DrawEllipticArc(hatTopMidPoint - wxPoint((threadWidth * 1.5), (threadWidth)),
                           wxSize((threadWidth * 3), (threadWidth * 1.5)), 180, 360);

        // thread dangling over the hat
        scaledPen.SetWidth(threadWidth);
        const DCPenChangerIfDifferent pc3{ dc, scaledPen };
        dc.DrawLine(hatTopMidPoint, wxPoint(GetXPosFromLeft(rect, 0.98),
                                            GetYPosFromTop(rect, math_constants::third)));
        dc.DrawLine(
            wxPoint(GetXPosFromLeft(rect, 0.98), GetYPosFromTop(rect, math_constants::third)),
            wxPoint(GetXPosFromLeft(rect, 0.98), GetYPosFromTop(rect, math_constants::two_thirds)));

        // tassel
        const std::array<wxPoint, 3> tassel = {
            wxPoint(GetXPosFromLeft(rect, 0.98), GetYPosFromTop(rect, math_constants::two_thirds)),
            wxPoint(GetXPosFromLeft(rect, 0.99),
                    GetYPosFromTop(rect, math_constants::two_thirds + 0.1)),
            wxPoint(GetXPosFromLeft(rect, 0.94),
                    GetYPosFromTop(rect, math_constants::two_thirds + 0.1))
        };
        dc.DrawPolygon(tassel.size(), tassel.data());

        scaledPen.SetColour(TintIfUsingOpacity(
            Colors::ColorContrast::Shade(Colors::ColorBrewer::GetColor(Colors::Color::Silver))));
        scaledPen.SetCap(wxPenCap::wxCAP_BUTT);
        scaledPen.SetWidth(scaledPen.GetWidth() + ScaleToScreenAndCanvas(1.5));
        const DCPenChangerIfDifferent pc4{ dc, scaledPen };
        dc.DrawLine(
            wxPoint(GetXPosFromLeft(rect, 0.98),
                    GetYPosFromTop(rect, math_constants::two_thirds - 0.05)),
            wxPoint(GetXPosFromLeft(rect, 0.98), GetYPosFromTop(rect, math_constants::two_thirds)));

        SetYOffsetPercentage(0.0);
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawRuler(const wxRect rect, wxDC& dc) const
        {
        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        if (gc == nullptr)
            {
            return;
            }

        const wxPen scaledPen(
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)),
            ScaleToScreenAndCanvas(rect.GetWidth() <= ScaleToScreenAndCanvas(32) ?
                                       math_constants::half :
                                       math_constants::full));

        wxRect2DDouble drawRect(rect.x, rect.y, rect.width, rect.height);
        const auto borderPadding =
            GetGraphItemInfo().GetPen().IsOk() ?
                ScaleToScreenAndCanvas(GetGraphItemInfo().GetPen().GetWidth()) :
                0.0;
        drawRect.Inset(borderPadding, borderPadding);

        // adjust to center it horizontally inside square area
        if (rect.GetWidth() == rect.GetHeight())
            {
            const auto adjustedWidth{ drawRect.m_width * 0.4 };
            const auto adjustLeft{ (drawRect.m_width - adjustedWidth) * math_constants::half };
            drawRect.m_width = adjustedWidth;
            drawRect.m_x += adjustLeft;
            }
            // add padding
            {
            const auto adjustedWidth{ drawRect.m_width * 0.8 };
            const auto adjustLeft{ (drawRect.m_width - adjustedWidth) * math_constants::half };
            drawRect.m_width = adjustedWidth;
            drawRect.m_x += adjustLeft;
            }

        gc->SetBrush(gc->CreateLinearGradientBrush(
            drawRect.GetLeft(), drawRect.GetTop(), drawRect.GetRight(), drawRect.GetTop(),
            ApplyColorOpacity(Colors::ColorContrast::Shade(
                Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow),
                math_constants::three_fourths)),
            ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::SchoolBusYellow))));
        gc->SetPen(*wxTRANSPARENT_PEN);
        gc->DrawRectangle(drawRect.m_x, drawRect.m_y, drawRect.m_width, drawRect.m_height);

        gc->SetBrush(*wxTRANSPARENT_BRUSH);
        gc->SetPen(scaledPen);
        gc->DrawRectangle(drawRect.m_x, drawRect.m_y, drawRect.m_width, drawRect.m_height);

        double currentY{ drawRect.GetTop() + ScaleToScreenAndCanvas(2) };
        int currentLine{ 0 };
        while (currentY < drawRect.GetBottom())
            {
            const auto startX =
                drawRect.GetLeft() +
                (drawRect.m_width *
                 ((currentLine % 4 == 0) ? math_constants::half : math_constants::three_fourths));
            gc->StrokeLine(startX, currentY, drawRect.GetRight(), currentY);
            currentY += ScaleToScreenAndCanvas(GetScaling() <= 2.0 ? 2 : 1);
            ++currentLine;
            }
        }

    //---------------------------------------------------
    void ShapeRenderer::DrawMonitor(const wxRect rect, wxDC& dc) const
        {
        // just to reset when we are done
        const wxDCPenChanger penGuard{ dc, Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCBrushChanger brushGuard{ dc,
                                           Colors::ColorBrewer::GetColor(Colors::Color::Black) };
        const wxDCFontChanger fc{ dc };

        wxRect2DDouble drawRect{ rect };
        drawRect.Deflate(ScaleToScreenAndCanvas(1));

        const GraphicsContextFallback gcf{ &dc, rect };
        auto* gc = gcf.GetGraphicsContext();
        wxASSERT_MSG(gc, L"Failed to get graphics context for monitor icon!");
        if (gc != nullptr)
            {
            wxRect2DDouble monitorOuterRect{ drawRect };
            monitorOuterRect.SetHeight(monitorOuterRect.GetHeight() * math_constants::half);
            monitorOuterRect.Offset(0, drawRect.GetHeight() * math_constants::quarter);

            wxRect2DDouble monitorRect{ monitorOuterRect };
            monitorRect.Deflate(ScaleToScreenAndCanvas(math_constants::half));

            // stand pole
            // (Note that we do not apply a translucency to the white backgrounds
            //  as that would allow the monitor stand to show through it.
            //  Instead, we only make the outline and monitor content translucent.)
            gc->SetBrush(Colors::ColorBrewer::GetColor(Colors::Color::White));
            gc->SetPen({ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)),
                         static_cast<int>(ScaleToScreenAndCanvas(1)) });
            wxRect2DDouble poleRect{ monitorRect };
            poleRect.SetWidth(monitorRect.GetWidth() * math_constants::tenth);
            poleRect.Offset((monitorRect.GetWidth() * math_constants::half) -
                                (poleRect.GetWidth() * math_constants::half),
                            poleRect.GetHeight() * math_constants::half);
            gc->DrawRectangle(poleRect);

            // stand base
            wxRect2DDouble standBaseRect{ poleRect };

            standBaseRect.SetHeight(standBaseRect.GetHeight() * math_constants::third);
            standBaseRect.SetWidth(monitorRect.GetWidth() * math_constants::half);
            standBaseRect.MoveLeftTo(monitorRect.GetLeft() +
                                     (monitorRect.GetWidth() * math_constants::half) -
                                     (standBaseRect.GetWidth() * math_constants::half));
            standBaseRect.MoveBottomTo(drawRect.GetBottom());

            // draw everything
            gc->SetBrush(Colors::ColorBrewer::GetColor(Colors::Color::White));
            gc->SetPen({ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)),
                         static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->DrawRectangle(monitorOuterRect);

            const auto boardBrush = gc->CreateLinearGradientBrush(
                GetXPosFromLeft(monitorRect, -math_constants::three_quarters),
                GetYPosFromTop(monitorRect, -math_constants::three_quarters),
                GetXPosFromLeft(monitorRect, 1), GetYPosFromTop(monitorRect, 1),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)),
                ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::Black)));
            gc->SetBrush(boardBrush);
            gc->SetPen(wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::White),
                              static_cast<int>(ScaleToScreenAndCanvas(1)) });
            gc->DrawRectangle(monitorRect);

            gc->SetBrush(Colors::ColorBrewer::GetColor(Colors::Color::White));
            gc->SetPen({ ApplyColorOpacity(Colors::ColorBrewer::GetColor(Colors::Color::DarkGray)),
                         static_cast<int>(ScaleToScreenAndCanvas(1)) });

            auto standBasePath = gc->CreatePath();
            standBasePath.MoveToPoint(standBaseRect.GetLeftBottom());
            standBasePath.AddLineToPoint(standBaseRect.GetRightBottom());
            standBasePath.AddQuadCurveToPoint(standBaseRect.GetRight(), standBaseRect.GetTop(),
                                              GetXPosFromLeft(standBaseRect, math_constants::half),
                                              GetYPosFromTop(standBaseRect, 0));
            standBasePath.AddQuadCurveToPoint(standBaseRect.GetLeftTop(),
                                              standBaseRect.GetLeftBottom());

            standBasePath.CloseSubpath();
            gc->FillPath(standBasePath);
            gc->StrokePath(standBasePath);
            }
        }
    } // namespace Wisteria::GraphItems
