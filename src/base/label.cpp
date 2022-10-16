///////////////////////////////////////////////////////////////////////////////
// Name:        label.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "label.h"
#include "polygon.h"
#include "shapes.h"

using namespace Wisteria::Colors;
using namespace Wisteria::Icons;

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    wxSize Label::CalcLeftImageSize(const wxCoord textHeight) const
        {
        if (!m_leftImage.IsOk())
            { return wxSize(0, 0); }
        const auto imgHeight = std::min(m_leftImage.GetDefaultSize().GetHeight(), textHeight);
        const auto imgWidth = geometry::calculate_rescale_width(
            { m_leftImage.GetDefaultSize().GetWidth(), m_leftImage.GetDefaultSize().GetHeight() },
            imgHeight);
        return wxSize(imgWidth, imgHeight);
        }

    //-------------------------------------------
    wxSize Label::CalcTopImageSize(const wxCoord textWidth) const
        {
        if (!m_topImage.IsOk())
            { return wxSize(0, 0); }
        const auto imgWidth = std::min(m_topImage.GetDefaultSize().GetWidth(), textWidth);
        const auto imgHeight = geometry::calculate_rescale_height(
            { m_topImage.GetDefaultSize().GetWidth(), m_topImage.GetDefaultSize().GetHeight() },
            imgWidth);
        return wxSize(imgWidth, imgHeight);
        }

    //-------------------------------------------
    void Label::SetLine(const size_t line, const wxString& lineText)
        {
        wxString newString;
        size_t currentRow{ 0 };
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        while (lineTokenizer.HasMoreTokens() )
            {
            if (line == currentRow)
                {
                newString += lineText;
                // just step over this line, we are replacing it
                lineTokenizer.GetNextToken();
                }
            else
                { newString += lineTokenizer.GetNextToken(); }
            newString += L"\n";
            ++currentRow;
            }
        newString.Trim();
        SetText(newString);
        }

    //-------------------------------------------
    void Label::CalcLongestLineLength()
        {
        if (GetText().empty())
            {
            m_lineCount = m_longestLineLength = 0;
            return;
            }

        // if multi-line, then see which line is the longest
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        if (lineTokenizer.CountTokens() > 1)
            {
            m_lineCount = 0;
            size_t longestLineCharacterCount{ 0 };
            while (lineTokenizer.HasMoreTokens() )
                {
                ++m_lineCount;
                const auto currentLineLength = lineTokenizer.GetNextToken().length();
                if (currentLineLength > longestLineCharacterCount)
                    { longestLineCharacterCount = currentLineLength; }
                }
            m_longestLineLength = longestLineCharacterCount;
            }
        else
            {
            m_lineCount = 1;
            m_longestLineLength = GetText().length();
            }
        }

    //-------------------------------------------
    wxCoord Label::CalcPageVerticalOffset(wxDC& dc) const
        {
        return !GetMinimumUserHeightDIPs() ? 0 : // if no min height, then no offset needed
            (dc.FromDIP(GetMinimumUserHeightDIPs().value()) <= GetCachedContentBoundingBox().GetHeight()) ?
            0 :
            (GetPageVerticalAlignment() == PageVerticalAlignment::TopAligned) ?
            0 :
            (GetPageVerticalAlignment() == PageVerticalAlignment::Centered) ?
            (dc.FromDIP(GetMinimumUserHeightDIPs().value()) - GetCachedContentBoundingBox().GetHeight()) / 2 :
            (dc.FromDIP(GetMinimumUserHeightDIPs().value()) - GetCachedContentBoundingBox().GetHeight());
        }

    //-------------------------------------------
    wxCoord Label::CalcPageHorizontalOffset(wxDC& dc) const
        {
        return !GetMinimumUserWidthDIPs() ? 0 : // if no min width, then no offset needed
            (dc.FromDIP(GetMinimumUserWidthDIPs().value()) <= GetCachedContentBoundingBox().GetWidth()) ?
            0 :
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::LeftAligned) ?
            0 :
            (GetPageHorizontalAlignment() == PageHorizontalAlignment::Centered) ?
            (dc.FromDIP(GetMinimumUserWidthDIPs().value()) - GetCachedContentBoundingBox().GetWidth()) / 2 :
            (dc.FromDIP(GetMinimumUserWidthDIPs().value()) - GetCachedContentBoundingBox().GetWidth());
        }

    //-------------------------------------------
    void Label::SetBoundingBox(const wxRect& rect, wxDC& dc, [[maybe_unused]] const double parentScaling)
        {
        InvalidateCachedBoundingBox();

        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(),
                             L"SetBoundingBox() should only be called on fixed objects!");
        if (IsFreeFloating())
            { return; }

        if (GetAnchoring() == Anchoring::Center)
            {
            SetAnchorPoint(wxPoint(rect.GetLeft()+(rect.GetWidth()/2),
                           rect.GetTop()+(rect.GetHeight()/2)));
            }
        else if (GetAnchoring() == Anchoring::TopLeftCorner)
            { SetAnchorPoint(rect.GetTopLeft()); }
        else if (GetAnchoring() == Anchoring::TopRightCorner)
            { SetAnchorPoint(rect.GetTopRight()); }
        else if (GetAnchoring() == Anchoring::BottomLeftCorner)
            { SetAnchorPoint(rect.GetBottomLeft()); }
        else if (GetAnchoring() == Anchoring::BottomRightCorner)
            { SetAnchorPoint(rect.GetBottomRight()); }

        // scale up or down to fit the bounding box if necessary
        wxCoord measuredWidth{ 0 }, measureHeight{ 0 };
        GetSize(dc, measuredWidth, measureHeight);
        // adjust area to text drawing area by removing space for the images
        //---------------
        // left image
        auto measureWidthNoSideImages = ((GetTextOrientation() == Orientation::Horizontal) ?
            (measuredWidth - CalcLeftImageSize(measureHeight).GetWidth()) : measuredWidth);
        auto measuredHeightNoSideImages = ((GetTextOrientation() == Orientation::Vertical) ?
            (measureHeight - CalcLeftImageSize(measuredWidth).GetWidth()) : measureHeight);
        // top image
        measuredHeightNoSideImages -= ((GetTextOrientation() == Orientation::Horizontal) ?
            std::max<wxCoord>(CalcTopImageSize(measuredWidth).GetHeight() - m_topImageOffset, 0) :
            0);
        measureWidthNoSideImages -= ((GetTextOrientation() == Orientation::Vertical) ?
            std::max<wxCoord>(CalcTopImageSize(measureHeight).GetHeight() - m_topImageOffset, 0) :
            0);

        if (// too small in both dimensions, so upscale
            (measuredWidth <= rect.GetWidth() &&
             measureHeight <= rect.GetHeight()) ||
            // or too big in one of the dimensions, so downscale
            (measuredWidth > rect.GetWidth() ||
             measureHeight > rect.GetHeight()) )
            {
            // scale the text to fit in the area where the text is going
            // (i.e., the rect with the images removed)
            const wxRect textRect(wxSize(std::min(rect.GetWidth(), measureWidthNoSideImages),
                                  std::min(rect.GetHeight(), measuredHeightNoSideImages)));
            const auto widthFactor = safe_divide<double>(textRect.GetWidth(),
                                                         measureWidthNoSideImages);
            const auto heightFactor = safe_divide<double>(textRect.GetHeight(),
                                                          measuredHeightNoSideImages);
            SetScaling(std::max(GetScaling() * std::min(widthFactor, heightFactor),
                                // don't go too small, though
                                math_constants::tenth));
            }
        
        // used for page alignment
        SetMinimumUserSizeDIPs(dc.ToDIP(rect.GetWidth()), dc.ToDIP(rect.GetHeight()));

        GetSize(dc, measuredWidth, measureHeight);
        SetCachedContentBoundingBox(wxRect(wxPoint(rect.GetTopLeft()),
                                           wxSize(measuredWidth, measureHeight)));
        // if there is a minimum height that is taller than the text, then center
        // the text vertically
        auto contentRect = GetCachedContentBoundingBox();
        contentRect.y += CalcPageVerticalOffset(dc);
        contentRect.x += CalcPageHorizontalOffset(dc);
        SetCachedContentBoundingBox(contentRect);

        if (IsAdjustingBoundingBoxToContent())
            {
            wxRect clippedRect{ rect };
            clippedRect.SetWidth(measuredWidth);
            SetCachedBoundingBox(clippedRect);
            }
        else
            { SetCachedBoundingBox(rect); }
        }

    //-------------------------------------------
    wxRect Label::GetBoundingBox(wxDC& dc) const
        {
        if (!IsOk())
            { wxRect(); }

        if (!GetCachedBoundingBox().IsEmpty())
            { return GetCachedBoundingBox(); }

        wxCoord measuredWidth{ 0 }, measuredHeight{ 0 };
        GetSize(dc, measuredWidth, measuredHeight);
        wxCoord width = std::max<wxCoord>(measuredWidth,
            dc.FromDIP(GetMinimumUserWidthDIPs().value_or(0)) );
        wxCoord height = std::max<wxCoord>(measuredHeight,
            dc.FromDIP(GetMinimumUserHeightDIPs().value_or(0)) );

        wxRect boundingBox;
        if (GetTextOrientation() == Orientation::Horizontal)
            {
            if (GetAnchoring() == Anchoring::Center)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width/2,height/2),
                                     GetAnchorPoint()+wxPoint(width/2,height/2));
                }
            else if (GetAnchoring() == Anchoring::TopLeftCorner)
                {
                boundingBox = wxRect(GetAnchorPoint(), wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner)
                {
                boundingBox = wxRect(GetAnchorPoint() - wxSize(width,0),
                                     wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(0,height),
                                     wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::BottomRightCorner)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,height),
                                     wxSize(width,height));
                }
            }
        else
            {
            if (GetAnchoring() == Anchoring::Center)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width/2,height/2),
                                     GetAnchorPoint()+wxPoint(width/2,height/2));
                }
            else if (GetAnchoring() == Anchoring::TopLeftCorner)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(0,height),
                                     wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::TopRightCorner)
                {
                boundingBox = wxRect(GetAnchorPoint(), wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::BottomLeftCorner)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,height),
                                     wxSize(width,height));
                }
            else if (GetAnchoring() == Anchoring::BottomRightCorner)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,0),
                                     wxSize(width,height));
                }
            }
        if (IsFreeFloating())
            {
            boundingBox.Offset((boundingBox.GetLeftTop()*GetScaling()) -
                                boundingBox.GetLeftTop());
            }

        SetCachedBoundingBox(boundingBox);
        SetCachedContentBoundingBox(wxRect(wxPoint(boundingBox.GetTopLeft()),
                                    wxSize(measuredWidth, measuredHeight)));
        // if there is a minimum height that is taller than the text, then center
        // the text vertically
        auto contentRect = GetCachedContentBoundingBox();
        contentRect.y += CalcPageVerticalOffset(dc);
        contentRect.x += CalcPageHorizontalOffset(dc);
        SetCachedContentBoundingBox(contentRect);
        return boundingBox;
        }

    //-------------------------------------------
    void Label::GetSize(wxDC& dc, wxCoord& width, wxCoord& height) const
        {
        wxASSERT(GetFont().IsOk());

        width = height = 0;

        wxDCFontChanger fc(dc, (GetFont().Scaled(GetScaling())) );

        wxStringTokenizer tokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        const wxCoord spaceBetweenLines = ((tokenizer.CountTokens() == 0) ?
                                            0 :
                                            (tokenizer.CountTokens()-1) *
                                            std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())) );

        if (GetTextOrientation() == Orientation::Horizontal)
            {
            // If top line is a header, then we need to not include that in the initial measure,
            // just measure the rest of the text. (This is because headers have their own left/right margins.)
            const auto firstLineEnd = GetText().find_first_of(L"\r\n", 0, 2);
            const auto secondLineStart = GetText().find_first_not_of(L"\r\n",
                ((firstLineEnd != std::wstring::npos) ? firstLineEnd : 0), 2);
            if (GetHeaderInfo().IsEnabled() && firstLineEnd != std::wstring::npos)
                { dc.GetMultiLineTextExtent(GetText().substr(secondLineStart), &width, &height); }
            else if (GetText().length())
                { dc.GetMultiLineTextExtent(GetText(), &width, &height); }
            // add padding around text
            width += ScaleToScreenAndCanvas(GetLeftPadding()) +
                     ScaleToScreenAndCanvas(GetRightPadding());
            height += spaceBetweenLines +
                      ScaleToScreenAndCanvas(GetTopPadding()) +
                      ScaleToScreenAndCanvas(GetBottomPadding());
            // now, if top line is a header, then measure that and see if needs to increase the width of the box
            if (GetHeaderInfo().IsEnabled() && firstLineEnd != std::wstring::npos &&
                secondLineStart != std::wstring::npos)
                {
                wxDCFontChanger fc2(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                     GetHeaderInfo().GetRelativeScaling()) :
                    dc.GetFont());
                auto topLineSize = dc.GetMultiLineTextExtent(GetText().substr(0, firstLineEnd));
                topLineSize.x += ScaleToScreenAndCanvas(GetLeftPadding()) +
                                 ScaleToScreenAndCanvas(GetRightPadding());
                width = std::max(topLineSize.GetWidth(), width);
                height += topLineSize.GetHeight();
                }
            // if drawing outline, then calculate that also in case the pen width is large
            if (GetPen().IsOk() &&
                (GetGraphItemInfo().IsShowingTopOutline() ||
                 GetGraphItemInfo().IsShowingBottomOutline() ||
                 GetGraphItemInfo().IsShowingLeftOutline() ||
                 GetGraphItemInfo().IsShowingRightOutline()))
                {
                const auto penWidthScaled = ScaleToScreenAndCanvas(GetPen().GetWidth());
                height += (GetGraphItemInfo().IsShowingTopOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingBottomOutline() ? penWidthScaled : 0);
                width += (GetGraphItemInfo().IsShowingRightOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingLeftOutline() ? penWidthScaled : 0);
                // Situation where there is an outline with a thick pen, but little or no text
                // and both sides of a dimension's outline are turned off. In this case,
                // we want to force that dimension to be a thick as the lines of the *other*
                // dimension so that the bounding box doesn't get scaled down below
                // the other dimension.
                const auto leftRightOutlineWidth =
                    (GetGraphItemInfo().IsShowingRightOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingLeftOutline() ? penWidthScaled : 0);
                const auto topBottomOutlineWidth =
                    (GetGraphItemInfo().IsShowingTopOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingBottomOutline() ? penWidthScaled : 0);
                height = std::max<double>(height, leftRightOutlineWidth);
                width = std::max<double>(width, topBottomOutlineWidth);
                }
            width += CalcLeftImageSize(height).GetWidth();
            height += std::max<wxCoord>(
                CalcTopImageSize(width).GetHeight() - m_topImageOffset,
                0);
            }
        else
            {
            const auto firstLineEnd = GetText().find_first_of(L"\r\n", 0, 2);
            const auto secondLineStart = GetText().find_first_not_of(L"\r\n",
                ((firstLineEnd != std::wstring::npos) ? firstLineEnd : 0), 2);
            if (GetHeaderInfo().IsEnabled() && secondLineStart != std::wstring::npos)
                { dc.GetMultiLineTextExtent(GetText().substr(secondLineStart), &height, &width); }
            else
                { dc.GetMultiLineTextExtent(GetText(), &height, &width); }
            height += ScaleToScreenAndCanvas(GetLeftPadding()) +
                      ScaleToScreenAndCanvas(GetRightPadding());
            width += spaceBetweenLines +
                     ScaleToScreenAndCanvas(GetTopPadding()) +
                     ScaleToScreenAndCanvas(GetBottomPadding());
            if (GetHeaderInfo().IsEnabled() && firstLineEnd != std::wstring::npos &&
                secondLineStart != std::wstring::npos)
                {
                wxDCFontChanger fc2(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                     GetHeaderInfo().GetRelativeScaling()) :
                    dc.GetFont());
                auto topLineSize = dc.GetMultiLineTextExtent(GetText().substr(0, firstLineEnd));
                topLineSize.x += ScaleToScreenAndCanvas(GetLeftPadding()) +
                                 ScaleToScreenAndCanvas(GetRightPadding());
                height = std::max(topLineSize.GetWidth(), height);
                width += topLineSize.GetHeight();
                }
            if (GetPen().IsOk() &&
                (GetGraphItemInfo().IsShowingTopOutline() ||
                 GetGraphItemInfo().IsShowingBottomOutline() ||
                 GetGraphItemInfo().IsShowingLeftOutline() ||
                 GetGraphItemInfo().IsShowingRightOutline()))
                {
                const auto penWidthScaled = ScaleToScreenAndCanvas(GetPen().GetWidth());
                width += (GetGraphItemInfo().IsShowingTopOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingBottomOutline() ? penWidthScaled : 0);
                height += (GetGraphItemInfo().IsShowingRightOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingLeftOutline() ? penWidthScaled : 0);

                const auto leftRightOutlineWidth =
                    (GetGraphItemInfo().IsShowingRightOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingLeftOutline() ? penWidthScaled : 0);
                const auto topBottomOutlineWidth =
                    (GetGraphItemInfo().IsShowingTopOutline() ? penWidthScaled : 0) +
                    (GetGraphItemInfo().IsShowingBottomOutline() ? penWidthScaled : 0);
                height = std::max<double>(height, topBottomOutlineWidth);
                width = std::max<double>(width, leftRightOutlineWidth);
                }
            height += CalcLeftImageSize(width).GetWidth();
            width += std::max<wxCoord>(CalcTopImageSize(height).GetHeight() - m_topImageOffset,
                0);
            }
        }

    //-------------------------------------------
    void Label::DrawLegendIcons(wxDC& dc) const
        {
        const wxCoord averageLineHeight = dc.GetCharHeight();
        const wxRect contentBoundingBox = GetCachedContentBoundingBox();

        wxPen scaledPen(GetPen());
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(
                std::max<double>(ScaleToScreenAndCanvas(scaledPen.GetWidth()), 1));
            }

        if (GetTextOrientation() == Orientation::Horizontal &&
            GetLegendIcons().size())
            {
            wxDCPenChanger pc2(dc, scaledPen.IsOk() ? scaledPen : GetPen());
            wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
            const auto topLine = lineTokenizer.GetNextToken();
            wxCoord topLineHeight{ 0 };
            // measure top line in case it is used as a header
                {
                wxDCFontChanger fc2(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                     GetHeaderInfo().GetRelativeScaling()) :
                    GetFont());
                topLineHeight = dc.GetTextExtent(topLine).GetHeight();
                }
            for (auto iconPos = GetLegendIcons().cbegin();
                 iconPos != GetLegendIcons().cend();
                 ++iconPos)
                {
                wxPen scaledIconPen(iconPos->m_pen.IsOk() ? iconPos->m_pen : GetPen());
                if (scaledIconPen.IsOk())
                    {
                    scaledIconPen.SetWidth(
                        ScaleToScreenAndCanvas(
                            // if a line icon, make it a minimum of 2 pixels wide
                            (iconPos->m_shape == IconShape::HorizontalLine ?
                             std::max(scaledIconPen.GetWidth(), 2) :
                             scaledIconPen.GetWidth())) );
                    }
                wxDCPenChanger pc3(dc, scaledIconPen);
                wxDCBrushChanger bc2(dc, iconPos->m_brush.IsOk() ? iconPos->m_brush : GetBrush());
                const size_t currentIndex = (iconPos-GetLegendIcons().begin());
                wxCoord middleOfCurrentRow =
                    static_cast<wxCoord>((averageLineHeight*currentIndex) +
                    safe_divide<size_t>(averageLineHeight, 2)) +
                    (static_cast<wxCoord>(currentIndex) *
                        std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) +
                    // spaces between proceeding lines
                    ScaleToScreenAndCanvas(GetTopPadding());
                const auto iconAreaWidth{ averageLineHeight };
                const auto iconRadius{ iconAreaWidth*.3 };
                const auto iconMiddleX{ iconAreaWidth*.5 };
                // if there is a minimum height that is taller than the text, then center
                // the text vertically
                const wxCoord yOffset = (GetHeaderInfo().IsEnabled() ?
                    topLineHeight + std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())) : 0);
                middleOfCurrentRow += yOffset;
                wxRect boxRect{ wxRect(contentBoundingBox.GetTopLeft() +
                                       wxPoint(iconMiddleX, middleOfCurrentRow),
                                       wxSize(1, 1)).Inflate(iconRadius) };
                // icons only relevant to legends that shape renderer doesn't handle
                if (iconPos->m_shape == IconShape::HorizontalSeparator ||
                    iconPos->m_shape == IconShape::HorizontalArrowRightSeparator ||
                    iconPos->m_shape == IconShape::ColorGradient)
                    {
                    switch (iconPos->m_shape)
                        {
                        // Horizontal separators
                        //----------------------
                        case IconShape::HorizontalSeparator:
                            dc.DrawLine(contentBoundingBox.GetTopLeft() +
                                wxPoint((ScaleToScreenAndCanvas(2)), middleOfCurrentRow),
                                contentBoundingBox.GetTopLeft() +
                                wxPoint(contentBoundingBox.GetWidth()-((ScaleToScreenAndCanvas(2))),
                                    middleOfCurrentRow));
                            break;
                        case IconShape::HorizontalArrowRightSeparator:
                            GraphItems::Polygon::DrawArrow(dc,
                                contentBoundingBox.GetTopLeft() +
                                wxPoint((ScaleToScreenAndCanvas(2)), middleOfCurrentRow),
                                contentBoundingBox.GetTopLeft() +
                                wxPoint(contentBoundingBox.GetWidth()-((ScaleToScreenAndCanvas(2))),
                                    middleOfCurrentRow),
                                ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSizeDIPs()));
                            break;
                        // full-length icons
                        //------------------
                        case IconShape::ColorGradient:
                            if (iconPos->m_colors.size() >= 2)
                                {
                                // we need to see how many colors there are and draw separate
                                // gradients between each pair, until the full spectrum is shown.
                                wxRect legendArea = contentBoundingBox;
                                legendArea.y += yOffset+ScaleToScreenAndCanvas(GetTopPadding());
                                legendArea.SetHeight(averageLineHeight * GetLineCountWithoutHeader());
                                legendArea.SetWidth(ScaleToScreenAndCanvas(LegendIcon::GetIconWidthDIPs()));

                                const int chunkSections = iconPos->m_colors.size()-1;
                                const wxCoord chunkHeight = safe_divide(legendArea.GetHeight(), chunkSections);
                                for (size_t i = 0; i < iconPos->m_colors.size()-1; ++i)
                                    {
                                    wxRect currentLegendChunk = legendArea;
                                    currentLegendChunk.SetHeight(chunkHeight);
                                    currentLegendChunk.y += chunkHeight*i;
                                    dc.GradientFillLinear(currentLegendChunk,
                                                          iconPos->m_colors[i], iconPos->m_colors[i+1], wxDOWN);
                                    }
                                }
                            break;
                        default:
                            break;
                        };
                    }
                else
                    {
                    wxBitmapBundle bmp(iconPos->m_img);
                    Shape sh(
                        GraphItemInfo().
                        Pen(iconPos->m_pen.IsOk() ? iconPos->m_pen : GetPen()).
                        Brush(iconPos->m_brush.IsOk() ? iconPos->m_brush : GetBrush()).
                        BaseColor(iconPos->m_baseColor).
                        Anchoring(Anchoring::TopLeftCorner).
                        Scaling(GetScaling()).
                        DPIScaling(GetDPIScaleFactor()),
                        iconPos->m_shape, boxRect.GetSize(),
                        iconPos->m_img.IsOk() ? &bmp : nullptr);
                    sh.SetBoundingBox(boxRect, dc, GetScaling());
                    sh.Draw(dc);
                    }
                }
            }
        }

    //-------------------------------------------
    void Label::DrawLabelStyling(wxDC& dc) const
        {
        const wxCoord averageLineHeight = dc.GetCharHeight();
        const wxRect boundingBox = GetBoundingBox(dc);
        // used for drawing the paper lines
        const wxPoint textOffset = ((GetTextOrientation() == Orientation::Horizontal) ?
            wxPoint(ScaleToScreenAndCanvas(GetLeftPadding()),
                ScaleToScreenAndCanvas(GetTopPadding())) :
            wxPoint(ScaleToScreenAndCanvas(GetTopPadding()),
                ScaleToScreenAndCanvas((GetRightPadding())) -
                ScaleToScreenAndCanvas((GetLeftPadding()))));
        if (GetTextOrientation() == Orientation::Horizontal)
            {
            // draw and style
            const size_t linesToDrawCount = safe_divide<double>(boundingBox.GetHeight(),
                                            averageLineHeight+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())));
            if (GetLabelStyle() == LabelStyle::NoLabelStyle)
                {
                // NOOP, most likely branch
                }
            else if (GetLabelStyle() == LabelStyle::IndexCard)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen((i == 1) ?
                        wxColour(255, 0, 0, Settings::GetTranslucencyValue()) :
                        wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                              ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                        ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::LinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                        ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::DottedLinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1), wxPENSTYLE_DOT));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                        ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::RightArrowLinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    GraphItems::Polygon::DrawArrow(dc,
                        wxPoint(boundingBox.GetLeftTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x, boundingBox.GetLeftTop().y+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxSize(ScaleToScreenAndCanvas(5), ScaleToScreenAndCanvas(5)));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::LinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x + ScaleToScreenAndCanvas(GetLeftPadding()),
                                        boundingBox.GetLeftTop().y + (averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x - ScaleToScreenAndCanvas(GetLeftPadding()),
                                boundingBox.GetLeftTop().y + (averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) +
                                textOffset.y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::DottedLinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1), wxPENSTYLE_DOT));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x + ScaleToScreenAndCanvas(GetLeftPadding()),
                                        boundingBox.GetLeftTop().y +
                                        (averageLineHeight*i) +
                                        ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x - ScaleToScreenAndCanvas(GetLeftPadding()),
                                boundingBox.GetLeftTop().y + (averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::RightArrowLinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    GraphItems::Polygon::DrawArrow(dc,
                        wxPoint(boundingBox.GetLeftTop().x + ScaleToScreenAndCanvas(GetLeftPadding()),
                                boundingBox.GetLeftTop().y + (averageLineHeight*i)+((i-1) *
                                std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) +
                                textOffset.y),
                        wxPoint(boundingBox.GetRightTop().x - ScaleToScreenAndCanvas(GetRightPadding()),
                                boundingBox.GetLeftTop().y + (averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.y),
                        wxSize(ScaleToScreenAndCanvas(5), ScaleToScreenAndCanvas(5)));
                    }
                }
            }
        else
            {
            // draw and style
            const size_t linesToDrawCount =
                safe_divide<double>(boundingBox.GetWidth(),
                                    averageLineHeight+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())));
            if (GetLabelStyle() == LabelStyle::NoLabelStyle)
                {
                // NOOP, most likely branch
                }
            else if (GetLabelStyle() == LabelStyle::IndexCard)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen((i == 1) ?
                        wxColour(255, 0, 0, Settings::GetTranslucencyValue()) :
                        wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                              ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1) * std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1) * std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::LinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::DottedLinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1), wxPENSTYLE_DOT));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                               ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::RightArrowLinedPaper)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    GraphItems::Polygon::DrawArrow(dc,
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y),
                        wxSize(ScaleToScreenAndCanvas(5), ScaleToScreenAndCanvas(5)));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::LinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1)));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::DottedLinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    wxDCPenChanger pc2(dc, wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
                                    ScaleToScreenAndCanvas(1), wxPENSTYLE_DOT));
                    dc.DrawLine(wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y));
                    }
                }
            else if (GetLabelStyle() == LabelStyle::RightArrowLinedPaperWithMargins)
                {
                wxDCClipper clip(dc, boundingBox);
                for (size_t i = 1; i <= linesToDrawCount; ++i)
                    {
                    GraphItems::Polygon::DrawArrow(dc,
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftTop().y),
                        wxPoint(boundingBox.GetLeftTop().x+(averageLineHeight*i) +
                                ((i-1)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())))+textOffset.x,
                                boundingBox.GetLeftBottom().y),
                        wxSize(ScaleToScreenAndCanvas(5), ScaleToScreenAndCanvas(5)));
                    }
                }
            }
        }

    //-------------------------------------------
    wxRect Label::Draw(wxDC& dc) const
        {
        if (!IsShown())
            { return wxRect(); }
        if (IsInDragState())
            { return GetBoundingBox(dc); }
        if (GetClippingRect())
            { dc.SetClippingRegion(GetClippingRect().value()); }

        wxASSERT_LEVEL_2_MSG(GetLegendIcons().size() == 0 ||
            (GetLegendIcons().size() && GetTextOrientation() == Orientation::Horizontal),
            L"Vertical legend not supported!");
        wxASSERT_LEVEL_2_MSG(GetLegendIcons().size() == 0 || !HasLegendIcons() ||
            (GetTextOrientation() == Orientation::Horizontal &&
             GetLeftPadding() >= GetMinLegendWidthDIPs()),
            wxString::Format(L"Left margin of text label should be at least %d DIPs "
                "if using legend icons! It is currently %d.",
                GetMinLegendWidthDIPs(), GetLeftPadding()));

        wxASSERT_LEVEL_2_MSG(GetFont().IsOk(), L"Invalid font in label!");
        wxDCFontChanger fc(dc, GetFont().Scaled(GetScaling()));

        const wxRect boundingBox = GetBoundingBox(dc);

        // draw the shadow (only if box is outlined)
        if (GetShadowType() != ShadowType::NoShadow && GetPen().IsOk() && !IsSelected())
            {
            wxDCPenChanger pcBg(dc, GetShadowColour());
            wxDCBrushChanger bcBg(dc, GetShadowColour());
            if (GetBoxCorners() == BoxCorners::Rounded)
                {
                dc.DrawRoundedRectangle(
                    wxRect(boundingBox.GetLeftTop() +
                           wxPoint(ScaleToScreenAndCanvas(GetShadowOffset()),
                                   ScaleToScreenAndCanvas(GetShadowOffset())),
                           boundingBox.GetSize()), Settings::GetBoxRoundedCornerRadius());
                }
            else
                {
                dc.DrawRectangle(
                    wxRect(boundingBox.GetLeftTop() +
                        wxPoint(ScaleToScreenAndCanvas(GetShadowOffset()),
                                ScaleToScreenAndCanvas(GetShadowOffset())),
                        boundingBox.GetSize()));
                }
            }
        // draw the background, if we are drawing a box around the text
        // (outline is drawn after the text)
        if (GetFontBackgroundColor().IsOk() && GetFontBackgroundColor() != wxTransparentColour)
            {
            wxDCBrushChanger bcBg(dc, GetFontBackgroundColor());
            wxDCPenChanger pcBg(dc, *wxTRANSPARENT_PEN);
            if (GetBoxCorners() == BoxCorners::Rounded)
                {
                dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius());
                }
            else
                {
                dc.DrawRectangle(boundingBox);
                }
            }

        // draw any styling lines on the background (this usually wouldn't be in use)
        if (GetLabelStyle() != LabelStyle::NoLabelStyle)
            { DrawLabelStyling(dc); }

        // draw left image
        if (m_leftImage.IsOk())
            {
            if (GetTextOrientation() == Orientation::Horizontal)
                {
                const auto bmp = m_leftImage.GetBitmap(
                    CalcLeftImageSize(GetCachedContentBoundingBox().GetHeight()));
                // center vertically
                auto leftCorner{ GetCachedContentBoundingBox().GetTopLeft() };
                leftCorner.y += safe_divide(GetCachedContentBoundingBox().GetHeight(), 2) -
                    safe_divide(bmp.GetHeight(), 2);
                dc.DrawBitmap(bmp, leftCorner);
                }
            else
                {
                const auto img = m_leftImage.GetBitmap(
                    CalcLeftImageSize(GetCachedContentBoundingBox().GetHeight())).
                    ConvertToImage().Rotate90(false);
                // center horizontally
                auto leftCorner{ GetCachedContentBoundingBox().GetBottomLeft() };
                leftCorner.x += safe_divide(GetCachedContentBoundingBox().GetWidth(), 2) -
                    safe_divide(img.GetHeight(), 2);
                leftCorner.y -= img.GetWidth();
                dc.DrawBitmap(img, leftCorner);
                }
            }
        // draw top image
        if (m_topImage.IsOk())
            {
            if (GetTextOrientation() == Orientation::Horizontal)
                {
                const auto bmp = m_topImage.GetBitmap(
                    CalcTopImageSize(GetCachedContentBoundingBox().GetWidth()));
                // center horizontally
                auto leftCorner{ GetCachedContentBoundingBox().GetTopLeft() };
                leftCorner.x += safe_divide(GetCachedContentBoundingBox().GetWidth(), 2) -
                    safe_divide(bmp.GetWidth(), 2);
                // ensure image doesn't go below (and outside of) the text
                leftCorner.y = std::min(leftCorner.y,
                    GetCachedContentBoundingBox().GetBottom() - bmp.GetHeight());
                dc.DrawBitmap(bmp, leftCorner);
                }
            else
                {
                const auto img = m_topImage.GetBitmap(
                    CalcTopImageSize(GetCachedContentBoundingBox().GetHeight())).
                    ConvertToImage().Rotate90(false);
                // center vertically
                auto leftCorner{ GetCachedContentBoundingBox().GetBottomLeft() };
                leftCorner.y -= safe_divide(GetCachedContentBoundingBox().GetHeight(), 2) -
                    safe_divide(img.GetWidth(), 2);
                leftCorner.x = std::min(leftCorner.x,
                    GetCachedContentBoundingBox().GetRight() - img.GetHeight());
                dc.DrawBitmap(img, leftCorner);
                }
            }

        // draw the text
        dc.SetTextForeground(GetFontColor());
        if (GetTextOrientation() == Orientation::Horizontal)
            { DrawMultiLineText(dc, boundingBox.GetLeftTop()); }
        else
            { DrawVerticalMultiLineText(dc, GetCachedContentBoundingBox().GetLeftTop()); }

        // draw the outline
        if (GetPen().IsOk() && !IsSelected())
            {
            wxDCPenChanger pc2(dc, wxPen(wxPenInfo(GetPen().GetColour(),
                                         ScaleToScreenAndCanvas(GetPen().GetWidth()),
                                         GetPen().GetStyle()).Cap(wxPenCap::wxCAP_BUTT)));
            if (GetBoxCorners() == BoxCorners::Rounded)
                {
                wxDCBrushChanger bcBg(dc, *wxTRANSPARENT_BRUSH);
                dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius());
                }
            else if (GetTextOrientation() == Orientation::Horizontal)
                {
                if (GetGraphItemInfo().IsShowingTopOutline())
                    { dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetTopRight()); }
                if (GetGraphItemInfo().IsShowingBottomOutline())
                    { dc.DrawLine(boundingBox.GetBottomLeft(), boundingBox.GetBottomRight()); }

                if (GetGraphItemInfo().IsShowingRightOutline())
                    { dc.DrawLine(boundingBox.GetTopRight(), boundingBox.GetBottomRight()); }
                if (GetGraphItemInfo().IsShowingLeftOutline())
                    { dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetBottomLeft()); }
                }
            else // vertical text
                {
                if (GetGraphItemInfo().IsShowingRightOutline())
                    { dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetTopRight()); }
                if (GetGraphItemInfo().IsShowingLeftOutline())
                    { dc.DrawLine(boundingBox.GetBottomLeft(), boundingBox.GetBottomRight()); }

                if (GetGraphItemInfo().IsShowingBottomOutline())
                    { dc.DrawLine(boundingBox.GetTopRight(), boundingBox.GetBottomRight()); }
                if (GetGraphItemInfo().IsShowingTopOutline())
                    { dc.DrawLine(boundingBox.GetTopLeft(), boundingBox.GetBottomLeft()); }
                }
            }
        else if (IsSelected())
            {
            wxDCPenChanger pc2(dc, wxPen(*wxBLACK, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
            wxDCBrushChanger bcBg(dc, *wxTRANSPARENT_BRUSH);
            if (GetBoxCorners() == BoxCorners::Rounded)
                {
                dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius());
                }
            else
                {
                dc.DrawRectangle(boundingBox);
                }
            if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                dc.DrawRectangle(GetCachedContentBoundingBox());
                if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::DrawInformationOnSelection))
                    {
                    const auto bBox = GetBoundingBox(dc);
                    Label infoLabel(GraphItemInfo(
                        wxString::Format(L"Scaling: %s\n"
                                          "Default font size: %d\n"
                                          "Font size: %d",
                            wxNumberFormatter::ToString(GetScaling(), 1,
                                                        wxNumberFormatter::Style::Style_NoTrailingZeroes),
                            GetFont().GetPointSize(),
                            wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize())).
                        AnchorPoint(bBox.GetTopLeft()).
                        Anchoring(Anchoring::TopLeftCorner).
                        FontColor(*wxBLUE).
                        Pen(*wxBLUE_PEN).DPIScaling(GetDPIScaleFactor()).
                        FontBackgroundColor(*wxWHITE).Padding(2, 2, 2, 2));
                    infoLabel.Draw(dc);
                    }
                }
            }

        // draw as a legend (if applicable)
        if (GetTextOrientation() == Orientation::Horizontal &&
            GetLegendIcons().size())
            { DrawLegendIcons(dc); }

        if (GetClippingRect())
            { dc.DestroyClippingRegion(); }
        return boundingBox;
        }

    //-------------------------------------------
    void Label::SplitTextByCharacter()
        {
        wxString splitText;
        for (size_t i = 0; i < GetText().length(); ++i)
            { splitText.append(GetText().at(i)).append(L'\n'); }
        SetText(splitText);
        }

    //-------------------------------------------
    bool Label::SplitTextByListItems()
        {
        auto splitText{ GetText() };
        // add newline after trailing "and/or" after last comma
        const wxString andOrString{ L"(&|and\\b|und\\b|y\\b|et\\b|or\\b|oder\\b|o\\b|ou\\b)" };
        const wxRegEx reCommaSepWithAnd(wxString::Format(L"(?i)(, )%s", andOrString));
        auto replacedCount = reCommaSepWithAnd.ReplaceAll(&splitText, L"\\1\\2\n");
        // and newline after 
        const wxRegEx reCommaSep(
            wxString::Format(L", ([^%s])", andOrString));
        replacedCount += reCommaSep.ReplaceAll(&splitText, L",\n\\1");
        SetText(splitText);
        return (replacedCount > 0);
        }

    //-------------------------------------------
    bool Label::SplitTextByConjunctions()
        {
        auto splitText{ GetText() };
        // add newline after all conjunctions
        const wxString andOrString{ L"(&|and|und|y|et|or|oder|o|ou)" };
        const wxRegEx reConjuctions(wxString::Format(L"(?i)\\b%s\\b", andOrString));
        auto replacedCount = reConjuctions.ReplaceAll(&splitText, L"\\1\n");
        SetText(splitText);
        return (replacedCount > 0);
        }

    //-------------------------------------------
    bool Label::SplitTextAuto()
        {
        // return if not much to split
        if (GetText().length() < 3)
            { return false; }

        wxString splitText = GetText();

        if (const auto charPos = splitText.find_first_of(L"([/:&");
            charPos != std::wstring::npos && charPos != splitText.length() - 1 &&
            charPos != 0)
            {
            if ((splitText[charPos] == L':' || splitText[charPos] == L'&') &&
                 splitText[charPos+1] == L' ')
                { splitText[charPos+1] = L'\n'; }
            else
                {
                splitText.insert(
                    ((splitText[charPos] == L':' || splitText[charPos] == L'&') ?
                        charPos+1 : charPos),
                    1, L'\n');
                }
            SetText(splitText);
            return true;
            }
        else if (const auto spacePos = splitText.find_first_of(L' ');
            spacePos != std::wstring::npos && spacePos != splitText.length() - 1 &&
            spacePos != 0)
            {
            const auto nextSpacePos = splitText.find_first_of(L' ', spacePos + 1);
            // it just two words (no more spaces), then split on the space
            if (nextSpacePos == std::wstring::npos)
                {
                splitText[spacePos] = L'\n';
                SetText(splitText);
                return true;
                }
            else
                { return false; }
            }
        else
            { return false; }
        }

    //-------------------------------------------
    void Label::SplitTextToFitLength(const size_t suggestedLineLength)
        {
        if (GetText().length() < suggestedLineLength)
            { return; }
        // if multi-line, see if any of its lines are too long. If so,
        // we need to split this string up and reformat it.
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        bool lineIsTooLong(false);
        while (lineTokenizer.HasMoreTokens() )
            {
            // draw the next line
            if (lineTokenizer.GetNextToken().length() > suggestedLineLength)
                {
                lineIsTooLong = true;
                break;
                }
            }
        if (!lineIsTooLong)
            { return; }

        // strip out any newlines from the original string first
        // (we'll be adding new ones when we re-tokenize the string)
        wxString tempStr(GetText());
        tempStr.Replace(L"\r\n",L" ", true);
        tempStr.Replace(L"\r", L" ", true);
        tempStr.Replace(L"\n", L" ", true);
        wxString fittedText;
        // split the string into lines by looking for delimiters close to the
        // suggested line length in each line
        while (tempStr.length() > suggestedLineLength)
            {
            const size_t index = tempStr.find_first_of(L" -", suggestedLineLength);
            if (index != std::wstring::npos)
                {
                fittedText.append(
                    std::wstring_view(tempStr.wc_str()).substr(0, index+1).data(), index+1).
                    Trim(true).append(L'\n');
                tempStr.erase(0, index+1);
                }
            else
                {
                fittedText.append(tempStr.c_str());
                tempStr.clear();
                }
            tempStr.Trim(false);
            }
        if (tempStr.length())
            { fittedText += tempStr; }
        fittedText.Trim(true); fittedText.Trim(false);
        SetText(fittedText);
        }

    //-------------------------------------------
    void Label::SplitTextToFitBoundingBox(wxDC& dc, const wxSize& boundingBoxSize)
        {
        if (!boundingBoxSize.IsFullySpecified())
            { return; }
        // note that fonts should not have their point size DPI scaled, only scaled to the canvas
        wxDCFontChanger fc(dc, GetFont().Scaled(GetScaling()));

        wxString text = GetText();
        text.Trim(false); text.Trim(true);

        wxCoord textWidth = 0, textHeight = 0, totalHeight = 0;

        wxStringTokenizer tok(text);
        text.clear(); // tokenizer will reconstruct this
        wxString currentLine;
        wxString nextToken;
        while (tok.HasMoreTokens())
            {
            nextToken = tok.GetNextToken();
            dc.GetTextExtent(currentLine+L" "+nextToken, &textWidth, &textHeight);
            if (textWidth > boundingBoxSize.GetWidth())
                {
                dc.GetTextExtent(currentLine, &textWidth, &textHeight);
                // if the next line will make this too tall, then show the current line
                // being truncated with an ellipsis and stop
                if ((totalHeight + textHeight +
                    std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) > boundingBoxSize.GetHeight())
                    {
                    if (text.length())
                        { text[text.length()-1] = wxChar{8230}; }
                    break;
                    }
                text += "\n"+currentLine;
                totalHeight += textHeight+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));
                // start the next line with the word that made the previous line too wide
                currentLine = nextToken;
                }
            else
                {
                currentLine += L" " + nextToken;
                }
            }
        // add any trailing line
        dc.GetTextExtent(currentLine, &textWidth, &textHeight);
        if ((static_cast<double>(totalHeight+textHeight) +
             std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) > boundingBoxSize.GetHeight())
            {
            if (text.length())
                { text[text.length()-1] = wxChar{8230}; }
            }
        else
            { text += "\n" + currentLine; }
        text.Trim(false); text.Trim(true);
        SetText(text);
        }

    //-------------------------------------------
    void Label::DrawVerticalMultiLineText(wxDC& dc, wxPoint pt) const
        {
        if (!IsOk())
            { return; }

        const wxCoord spaceBetweenLines = (GetLineCount()-1) *
                                           std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));

        pt.y += GetCachedContentBoundingBox().GetHeight() +
            // if drawing outline, then calculate that also in case the pen width is large
            ((GetPen().IsOk() && GetGraphItemInfo().IsShowingRightOutline()) ?
                ScaleToScreenAndCanvas(GetPen().GetWidth()) : 0);
        const wxCoord leftOffset = CalcPageHorizontalOffset(dc) +
            ((GetPen().IsOk() && GetGraphItemInfo().IsShowingTopOutline()) ?
                ScaleToScreenAndCanvas(GetPen().GetWidth()) : 0);

        // render the text
        wxCoord lineX{ 0 }, lineY{ 0 }, offest{ 0 };
        // if justified, shrink it down to include the padding on all sides
        wxSize fullTextSz = GetCachedContentBoundingBox().GetSize();
        wxSize fullTextSzForHeader = GetCachedContentBoundingBox().GetSize();
        if (GetTextAlignment() == TextAlignment::JustifiedAtCharacter ||
            GetTextAlignment() == TextAlignment::JustifiedAtWord)
            {
            fullTextSz.SetWidth(fullTextSz.GetWidth() -
                (ScaleToScreenAndCanvas(GetTopPadding()) +
                ScaleToScreenAndCanvas(GetBottomPadding())) );
            fullTextSz.SetHeight(fullTextSz.GetHeight() -
                (spaceBetweenLines +
                ScaleToScreenAndCanvas(GetLeftPadding()) +
                ScaleToScreenAndCanvas(GetRightPadding())) );
            }
        if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtCharacter ||
            GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtWord)
            {
            fullTextSzForHeader.SetWidth(fullTextSzForHeader.GetWidth() -
                (ScaleToScreenAndCanvas(GetTopPadding()) +
                ScaleToScreenAndCanvas(GetBottomPadding())) );
            fullTextSzForHeader.SetHeight(fullTextSzForHeader.GetHeight() -
                (spaceBetweenLines +
                ScaleToScreenAndCanvas(GetLeftPadding()) +
                ScaleToScreenAndCanvas(GetRightPadding())) );
            }
        fullTextSz.SetHeight(fullTextSz.GetHeight() -
                CalcLeftImageSize(GetCachedContentBoundingBox().GetWidth()).GetWidth() );
        fullTextSzForHeader.SetHeight(fullTextSzForHeader.GetHeight() -
                CalcLeftImageSize(GetCachedContentBoundingBox().GetWidth()).GetWidth() );

        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        size_t currentLineNumber{ 0 };
        std::vector<wxString> tokenizedLineLetters;

        const auto trackTextLine = [&dc, &tokenizedLineLetters]
                                   (wxString& textLine, const wxSize textSz, bool atCharacter)
            {
            constexpr wchar_t hairSpace{ 0x200A };
            // Measure 10 hair spaces (with the current font)
            // and divide by 10 to more precisely measure of how wide one is.
            // Measuring just one will double up and cause the calculation to be way off.
            double hairSpaceWidth = safe_divide<double>(
                dc.GetTextExtent(wxString(hairSpace, 10)).GetWidth(), 10);
            tokenizedLineLetters.clear();
            // if line is shorter than the longest line, then fill it with
            // more spaces (spread evenly throughout) until it fits
            if (dc.GetTextExtent(textLine).GetWidth() < textSz.GetHeight())
                {
                if (atCharacter)
                    {
                    // break the line into separate letters
                    for (const auto letter : textLine)
                        { tokenizedLineLetters.emplace_back(letter); }
                    }
                else
                    {
                    // split at each space (i.e., word)
                    wxStringTokenizer tkzr(textLine, L" ", wxStringTokenizerMode::wxTOKEN_RET_DELIMS);
                    while (tkzr.HasMoreTokens())
                        { tokenizedLineLetters.emplace_back(tkzr.GetNextToken()); }
                    }
                // need at least two letters for justifying text
                if (tokenizedLineLetters.size() < 2)
                    { return; }
                // use hair spaces between letters for more precise tracking
                const auto lineDiff = textSz.GetHeight() - dc.GetTextExtent(textLine).GetWidth();
                long hairSpacesNeeded = std::ceil(safe_divide<double>(lineDiff, hairSpaceWidth));
                const auto letterSpaces = tokenizedLineLetters.size() - 1;
                const auto hairSpacesPerWordPair = std::max(1.0,
                    std::floor(safe_divide<double>(hairSpacesNeeded, letterSpaces)));
                long extraSpaces = hairSpacesNeeded - (hairSpacesPerWordPair*letterSpaces);
                // rebuild the text line with more (hair) spaces in it
                textLine.clear();
                for (const auto& letter : tokenizedLineLetters)
                    {
                    textLine.append(letter).append(
                                                 (hairSpacesPerWordPair * (hairSpacesNeeded > 0 ? 1 : 0)) +
                                                 (extraSpaces > 0 ? 1 : 0), hairSpace);
                    --extraSpaces;
                    hairSpacesNeeded -= hairSpacesPerWordPair;
                    }
                }
            else
                { return; }
            };

        while (lineTokenizer.HasMoreTokens() )
            {
            // draw the next line
            wxString token = lineTokenizer.GetNextToken();
            dc.GetTextExtent(token, &lineX, &lineY);

            if (GetHeaderInfo().IsEnabled() && currentLineNumber == 0 && GetLineCount() > 1)
                {
                wxDCFontChanger fc(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                     GetHeaderInfo().GetRelativeScaling()) :
                    dc.GetFont());
                dc.GetTextExtent(token, &lineX, &lineY);
                if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushLeft)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Centered)
                    {
                    offest = (safe_divide<double>(fullTextSzForHeader.GetHeight(),2) -
                              safe_divide<double>(lineX, 2)) +
                             leftOffset;
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushRight)
                    {
                    offest = (fullTextSzForHeader.GetHeight()-lineX-ScaleToScreenAndCanvas(GetLeftPadding())) +
                        (HasLegendIcons() ? 0 : leftOffset);
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtCharacter)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSzForHeader, true);
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtWord)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSzForHeader, false);
                    }
                }
            else
                {
                if (GetTextAlignment() == TextAlignment::FlushLeft)
                    { offest = ScaleToScreenAndCanvas(GetLeftPadding()); }
                else if (GetTextAlignment() == TextAlignment::Centered)
                    {
                    offest = (safe_divide<double>(fullTextSz.GetHeight(), 2) -
                              safe_divide<double>(lineX, 2));
                    }
                else if (GetTextAlignment() == TextAlignment::FlushRight)
                    {
                    offest = fullTextSz.GetHeight() - lineX -
                        ScaleToScreenAndCanvas(GetLeftPadding());
                    }
                else if (GetTextAlignment() == TextAlignment::JustifiedAtCharacter)
                    {
                    offest = ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSz, true);
                    }
                else if (GetTextAlignment() == TextAlignment::JustifiedAtWord)
                    {
                    offest = ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSz, false);
                    }
                }
            if (!dc.GetFont().IsOk())
                {
                wxLogWarning(L"Invalid font used in graphics; will be replaced by system default.");
                dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
                }
            const auto currentLineOffset =
                (GetLinesIgnoringLeftMargin().find(currentLineNumber) != GetLinesIgnoringLeftMargin().cend()) ? 0 :
                ((GetHeaderInfo().IsEnabled() && currentLineNumber == 0) ? 0 : leftOffset) +
                CalcLeftImageSize(GetCachedContentBoundingBox().GetWidth()).GetWidth();
            const auto xOffset = std::max<wxCoord>(
                CalcTopImageSize(GetCachedContentBoundingBox().GetHeight()).GetHeight() - m_topImageOffset,
                0);
            const bool isHeader{ (currentLineNumber == 0 &&
                                  GetLineCount() > 1 &&
                                  GetHeaderInfo().IsEnabled() &&
                                  GetHeaderInfo().GetFont().IsOk()) };
            wxDCFontChanger fc(dc,
                isHeader ?
                GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                 GetHeaderInfo().GetRelativeScaling()) :
                dc.GetFont());
            wxDCTextColourChanger tcc(dc,
                isHeader ?
                GetHeaderInfo().GetFontColor() : dc.GetTextForeground());
            dc.DrawRotatedText(token, pt.x + xOffset, pt.y-offest-currentLineOffset, 90+m_tiltAngle);
            // move over for next line
            pt.x += (lineY+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())));
            ++currentLineNumber;
            }
        }

    //-------------------------------------------
    void Label::DrawMultiLineText(wxDC& dc, wxPoint pt) const
        {
        if (!IsOk())
            { return; }
        const wxCoord spaceBetweenLines = (GetLineCount()-1) *
                                           std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));

        pt.y += CalcPageVerticalOffset(dc) + ScaleToScreenAndCanvas(GetTopPadding()) +
            // if drawing outline, then calculate that also in case the pen width is large
            ((GetPen().IsOk() && GetGraphItemInfo().IsShowingTopOutline()) ?
                ScaleToScreenAndCanvas(GetPen().GetWidth()) : 0);
        const wxCoord leftOffset = CalcPageHorizontalOffset(dc) +
            ((GetPen().IsOk() && GetGraphItemInfo().IsShowingLeftOutline()) ?
                ScaleToScreenAndCanvas(GetPen().GetWidth()) : 0);

        // render the text
        wxCoord lineX{ 0 }, lineY{ 0 }, offest{ 0 };
        // if justified, shrink it down to include the padding on all sides
        wxSize fullTextSz = GetCachedContentBoundingBox().GetSize();
        wxSize fullTextSzForHeader = GetCachedContentBoundingBox().GetSize();
        if (GetTextAlignment() == TextAlignment::JustifiedAtCharacter ||
            GetTextAlignment() == TextAlignment::JustifiedAtWord)
            {
            fullTextSz.SetWidth(fullTextSz.GetWidth() -
                (ScaleToScreenAndCanvas(GetLeftPadding()) +
                 ScaleToScreenAndCanvas(GetRightPadding())) );
            fullTextSz.SetHeight(fullTextSz.GetHeight() -
                (spaceBetweenLines +
                ScaleToScreenAndCanvas(GetTopPadding()) +
                ScaleToScreenAndCanvas(GetBottomPadding())) );
            }
        if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtCharacter ||
            GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtWord)
            {
            fullTextSzForHeader.SetWidth(fullTextSzForHeader.GetWidth() -
                (ScaleToScreenAndCanvas(GetLeftPadding()) +
                 ScaleToScreenAndCanvas(GetRightPadding())) );
            fullTextSzForHeader.SetHeight(fullTextSzForHeader.GetHeight() -
                (spaceBetweenLines +
                ScaleToScreenAndCanvas(GetTopPadding()) +
                ScaleToScreenAndCanvas(GetBottomPadding())) );
            }
        fullTextSz.SetWidth(fullTextSz.GetWidth() -
                 CalcLeftImageSize(GetCachedContentBoundingBox().GetHeight()).GetWidth() );
        fullTextSzForHeader.SetWidth(fullTextSzForHeader.GetWidth() -
                 CalcLeftImageSize(GetCachedContentBoundingBox().GetHeight()).GetWidth() );

        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        size_t currentLineNumber{ 0 };
        std::vector<wxString> tokenizedLineLetters;

        const auto trackTextLine = [&dc, &tokenizedLineLetters]
                                   (wxString& textLine, const wxSize textSz, bool atCharacter)
            {
            constexpr wchar_t hairSpace{ 0x200A };

            // Measure 10 hair spaces (with the current font)
            // and divide by 10 to more precisely measure of how wide one is.
            // Measuring just one will double up and cause the calculation to be way off.
            double hairSpaceWidth = safe_divide<double>(
                dc.GetTextExtent(wxString(hairSpace, 10)).GetWidth(), 10);
            tokenizedLineLetters.clear();
            // if line is shorter than the longest line, then fill it with
            // hair spaces (spread evenly throughout) until it fits
            if (dc.GetTextExtent(textLine).GetWidth() < textSz.GetWidth())
                {
                if (atCharacter)
                    {
                    // break the line into separate letters
                    for (const auto letter : textLine)
                        { tokenizedLineLetters.emplace_back(letter); }
                    }
                else
                    {
                    // split at each space (i.e., word)
                    wxStringTokenizer tkzr(textLine, L" ", wxStringTokenizerMode::wxTOKEN_RET_DELIMS);
                    while (tkzr.HasMoreTokens())
                        { tokenizedLineLetters.emplace_back(tkzr.GetNextToken()); }
                    }
                // need at least two letters for justifying text
                if (tokenizedLineLetters.size() < 2)
                    { return; }
                // use hair spaces between letters for more precise tracking
                const auto lineDiff = textSz.GetWidth() - dc.GetTextExtent(textLine).GetWidth();
                long hairSpacesNeeded = std::ceil(safe_divide<double>(lineDiff, hairSpaceWidth));
                const auto letterSpaces = tokenizedLineLetters.size() - 1;
                const auto hairSpacesPerWordPair = std::max(1.0,
                    std::floor(safe_divide<double>(hairSpacesNeeded, letterSpaces)));
                // the last gap between letters may not need as many spaces as the others
                long extraSpaces = hairSpacesNeeded - (hairSpacesPerWordPair*letterSpaces);
                // rebuild the text line with more (hair) spaces in it
                textLine.clear();
                for (const auto& letter : tokenizedLineLetters)
                    {
                    textLine.append(letter).append(
                                                 (hairSpacesPerWordPair * (hairSpacesNeeded > 0 ? 1 : 0)) +
                                                 (extraSpaces > 0 ? 1 : 0), hairSpace);
                    --extraSpaces;
                    hairSpacesNeeded -= hairSpacesPerWordPair;
                    }
                }
            else
                { return; }
            };

        while (lineTokenizer.HasMoreTokens() )
            {
            // draw the next line
            wxString token = lineTokenizer.GetNextToken();
            dc.GetTextExtent(token, &lineX, &lineY);

            if (GetHeaderInfo().IsEnabled() && currentLineNumber == 0 && GetLineCount() > 1)
                {
                // remeasure for (possibly) different font in header
                wxDCFontChanger fc(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                     GetHeaderInfo().GetRelativeScaling()) :
                    dc.GetFont());
                dc.GetTextExtent(token, &lineX, &lineY);
                // if pushed to the left and it's a legend, then it should be to the edge;
                // otherwise, align with the rest of the text
                if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushLeft)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    }
                // note that for centering we need to add half of the margin back in
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Centered)
                    {
                    offest = (safe_divide<double>(fullTextSzForHeader.GetWidth(), 2) -
                              safe_divide<double>(lineX, 2)) +
                        leftOffset;
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushRight)
                    {
                    offest = (fullTextSzForHeader.GetWidth() - lineX -
                              ScaleToScreenAndCanvas(GetLeftPadding())) +
                             (HasLegendIcons() ? 0 : leftOffset);
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtCharacter)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSzForHeader, true);
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::JustifiedAtWord)
                    {
                    offest = HasLegendIcons() ? 0 :
                        leftOffset + ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSzForHeader, false);
                    }
                }
            else
                {
                if (GetTextAlignment() == TextAlignment::FlushLeft)
                    { offest = ScaleToScreenAndCanvas(GetLeftPadding()); }
                else if (GetTextAlignment() == TextAlignment::Centered)
                    {
                    offest = (safe_divide<double>(fullTextSz.GetWidth(), 2) -
                              safe_divide<double>(lineX, 2));
                    }
                else if (GetTextAlignment() == TextAlignment::FlushRight)
                    {
                    offest = fullTextSz.GetWidth() - lineX -
                             ScaleToScreenAndCanvas(GetRightPadding());
                    }
                else if (GetTextAlignment() == TextAlignment::JustifiedAtCharacter)
                    {
                    offest = ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSz, true);
                    }
                else if (GetTextAlignment() == TextAlignment::JustifiedAtWord)
                    {
                    offest = ScaleToScreenAndCanvas(GetLeftPadding());
                    trackTextLine(token, fullTextSz, false);
                    }
                }
            if (!dc.GetFont().IsOk())
                {
                wxLogWarning(L"Invalid font used in graphics; will be replaced by system default.");
                dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
                }
            const auto currentLineOffset =
                (GetLinesIgnoringLeftMargin().find(currentLineNumber) != GetLinesIgnoringLeftMargin().cend()) ? 0 :
                ((GetHeaderInfo().IsEnabled() && currentLineNumber == 0) ? 0 : leftOffset) +
                CalcLeftImageSize(GetCachedContentBoundingBox().GetHeight()).GetWidth();
            const bool isHeader{ (currentLineNumber == 0 &&
                                  GetLineCount() > 1 &&
                                  GetHeaderInfo().IsEnabled() &&
                                  GetHeaderInfo().GetFont().IsOk()) };
            const auto yOffset = std::max<wxCoord>(
                CalcTopImageSize(GetCachedContentBoundingBox().GetWidth()).GetHeight() - m_topImageOffset,
                0);
            wxDCFontChanger fc(dc,
                isHeader ?
                GetHeaderInfo().GetFont().Scaled(GetScaling() *
                                                 GetHeaderInfo().GetRelativeScaling()) :
                dc.GetFont());
            wxDCTextColourChanger tcc(dc,
                isHeader ?
                GetHeaderInfo().GetFontColor() : dc.GetTextForeground());
            if (m_tiltAngle != 0)
                { dc.DrawRotatedText(token,pt.x+offest+currentLineOffset, pt.y + yOffset, m_tiltAngle); }
            else
                { dc.DrawText(token,pt.x+offest+currentLineOffset, pt.y + yOffset); }
            // move down for next line
            pt.y += (lineY+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())));
            ++currentLineNumber;
            }
        }

    //------------------------------------------------------
    void Label::FixFont(wxFont& theFont)
        {
        const wxString originalFaceName = theFont.GetFaceName();
        // fix the point size: a size that is zero can cause a crash on some platforms, and if the size
        // is too small to be supported by the font then it appears blank in a font selection dialog.
        // The smallest size is 8 on Windows and 9 on OSX, so reset bogus sizes if needed.
        constexpr int minimumPointSize =
#ifdef __WXMSW__
        8;
#else
        9;
#endif
        if (theFont.GetPointSize() < minimumPointSize)
            { theFont.SetPointSize(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize()); }
        // Fix the facename. Of interest is that some versions of macOS use hidden fonts for their default
        // font, which won't be displayed in a standard font selection dialog. We have to remap these here.
        // OSX 10.9 font
        if (theFont.GetFaceName() == L".Lucida Grande UI")
            {
            theFont.SetFaceName(wxFontEnumerator::IsValidFacename(L"Lucida Grande") ?
                wxString(L"Lucida Grande") : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName());
            }
        // OSX 10.10 font
        else if (theFont.GetFaceName() == L".Helvetica Neue DeskInterface")
            {
            theFont.SetFaceName(wxFontEnumerator::IsValidFacename(L"Helvetica Neue") ?
                wxString(L"Helvetica Neue") : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName());
            }
        // and finally, make sure the font is valid
        if (!wxFontEnumerator::IsValidFacename(theFont.GetFaceName()) &&
             // system mapped font on macOS 10.15+, leave it alone
             theFont.GetFaceName() != L".AppleSystemUIFont")
            {
            wxArrayString fontNames;
            fontNames.Add(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName());
            fontNames.Add(L"Helvetica Neue");
            fontNames.Add(L"Lucida Grande");
            fontNames.Add(L"Calibri");
            fontNames.Add(L"Arial");
            fontNames.Add(L"Courier New");
            theFont.SetFaceName(GetFirstAvailableFont(fontNames));
            }
        wxASSERT_MSG(theFont.GetFaceName().length(), L"Corrected font facename is empty.");
        // if font is still messed up, fall back to system default
        wxASSERT(theFont.IsOk());
        if (!theFont.IsOk())
            {
            theFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);
            wxLogWarning(L"Unable to correct font '%s'; will be replaced by system default.", theFont.GetFaceName());
            }
        if (originalFaceName != theFont.GetFaceName())
            { wxLogMessage(L"Font remapped from '%s' to '%s'.", originalFaceName, theFont.GetFaceName()); }
        }

    //------------------------------------------------------
    wxString Label::GetFirstAvailableFont(const wxArrayString& possibleFontNames)
        {
        for (size_t i = 0; i < possibleFontNames.GetCount(); ++i)
            {
            if (wxFontEnumerator::IsValidFacename(possibleFontNames[i]) )
                { return possibleFontNames[i]; }
            }
        // fall back to system default if nothing in the provided list is found
        const wxString systemFont = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName();
        // if system font is weird mapping (or empty) that fails (happens on macOS), then fall back to Arial
        return wxFontEnumerator::IsValidFacename(systemFont) ? systemFont : L"Arial";
        }

    //------------------------------------------------------
    wxString Label::GetFirstAvailableCursiveFont()
        {
        wxArrayString cursiveFonts;
        cursiveFonts.Add(L"Gabriola");
        cursiveFonts.Add(L"Brush Script");
        cursiveFonts.Add(L"Segoe Script");
        cursiveFonts.Add(L"AR BERKLEY");
        return GetFirstAvailableFont(cursiveFonts);
        }

    //--------------------------------------------------
    int Label::CalcFontSizeToFitBoundingBox(wxDC& dc, const wxFont& ft, const wxRect& boundingBox, const wxString& text)
        {
        // start with the smallest possible font and work our way up.
        wxFont resizedFont(ft); resizedFont.SetPointSize(1);
        wxDCFontChanger fc(dc, resizedFont);
        wxCoord textWidth{ 0 }, textHeight{ 0 };

        for (;;) // will break when font size is found
            {
            // wxFont::Larger() increases font size by 1.2x, which may cause the
            // point size (integer) to remain the same, messing up the comparison
            // down below when we try to see if the point size can't be increased anymore.
            // Also, increasing by 1.2 will be too aggressive (30pt will become 36pt in one operation),
            // whereas we want to test each point size to find the perfect one.
            resizedFont.SetPointSize(resizedFont.GetPointSize()+1);
            // bail if the font can't be made any larger
            if (resizedFont.GetPointSize() == dc.GetFont().GetPointSize())
                { return resizedFont.GetPointSize(); }
            wxDCFontChanger fc2(dc, resizedFont);
            dc.GetMultiLineTextExtent(text, &textWidth, &textHeight);

            if (textWidth > boundingBox.GetWidth() ||
                textHeight > boundingBox.GetHeight())
                {
                resizedFont.SetPointSize(std::max(1, resizedFont.GetPointSize() - 1));
                break;
                }
            }
        return resizedFont.GetPointSize();
        }

    //--------------------------------------------------
    int Label::CalcDiagonalFontSize(wxDC& dc, const wxFont& ft, const wxRect& boundingBox,
                                         const double angleInDegrees, const wxString& text)
        {
        // start with the smallest possible font and work our way up.
        wxFont resizedFont(ft); resizedFont.SetPointSize(1);
        wxDCFontChanger fc(dc, resizedFont);
        wxCoord textWidth{ 0 }, textHeight{ 0 };

        for (;;) // will break when font size is found
            {
            // wxFont::Larger() increases font size by 1.2f, which may cause the
            // point size (integer) to remain the same, messing up the comparison
            // down below when we try to see if the point size can't be increased anymore.
            // Also, increasing by 1.2 will be too aggressive (30pt will become 36pt in one operation),
            // whereas we want to test each point size to find the perfect one.
            resizedFont.SetPointSize(resizedFont.GetPointSize()+1);
            // bail if the font can't be made any larger
            if (resizedFont.GetPointSize() == dc.GetFont().GetPointSize())
                { return resizedFont.GetPointSize(); }
            wxDCFontChanger fc2(dc, resizedFont);
            dc.GetMultiLineTextExtent(text, &textWidth, &textHeight);

            const float widthOfWatermark = textWidth *
                std::abs(std::cos(geometry::degrees_to_radians(angleInDegrees))) -
                textHeight * std::abs(std::sin(geometry::degrees_to_radians(angleInDegrees)));
            const float heightOfWatermark = textWidth *
                std::abs(std::sin(geometry::degrees_to_radians(angleInDegrees))) +
                textHeight * std::abs(std::cos(geometry::degrees_to_radians(angleInDegrees)));

            if (widthOfWatermark > boundingBox.GetWidth() ||
                heightOfWatermark > boundingBox.GetHeight())
                {
                resizedFont.SetPointSize(std::max(1, resizedFont.GetPointSize() - 1));
                break;
                }
            }
        return resizedFont.GetPointSize();
        }
    }
