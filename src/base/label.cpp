///////////////////////////////////////////////////////////////////////////////
// Name:        label.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "label.h"

namespace Wisteria::GraphItems
    {
    //-------------------------------------------
    void Label::SetLine(const size_t line, const wxString& lineText)
        {
        wxString newString;
        size_t currentRow{0};
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

        // if multiline, then see which line is the longest
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        if (lineTokenizer.CountTokens() > 1)
            {
            m_lineCount = 0;
            size_t longestLineCharacterCount(0);
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
    void Label::SetBoundingBox(const wxRect& rect, wxDC& dc, const double parentScaling)
        {
        InvalidateCachedBoundingBox();

        wxASSERT_LEVEL_2_MSG(!IsFreeFloating(), L"SetBoundingBox() should only be called on fixed objects!");
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

        // readjust the scaling to fit the new bounding box
        double currentScaling{ 1.0 };
        wxCoord measuredWidth{ 0 }, measuredHeight{ 0 };
        while (measuredWidth <= rect.GetWidth() &&
             measuredHeight <= rect.GetHeight())
            {
            SetScaling(currentScaling);
            GetSize(dc, measuredWidth, measuredHeight);
            if (measuredWidth > rect.GetWidth() ||
                measuredHeight > rect.GetHeight())
                {
                currentScaling -= 0.1;
                SetScaling(currentScaling);
                break;
                }
            currentScaling += 0.1;
            }
        
        // used for page alignment
        SetMinimumUserSizeDIPs(dc.ToDIP(rect.GetWidth()), dc.ToDIP(rect.GetHeight()));

        GetSize(dc, measuredWidth, measuredHeight);
        SetCachedContentBoundingBox(wxRect(wxPoint(rect.GetTopLeft()),
            wxSize(measuredWidth, measuredHeight)));
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
            (GetMinimumUserWidthDIPs() ? dc.FromDIP(GetMinimumUserWidthDIPs().value()) : 0));
        wxCoord height = std::max<wxCoord>(measuredHeight,
            (GetMinimumUserHeightDIPs() ? dc.FromDIP(GetMinimumUserHeightDIPs().value()) : 0));

        wxRect boundingBox;
        if (GetTextOrientation() == Orientation::Horizontal)
            {
            if (GetAnchoring() == Anchoring::Center)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width/2,height/2),
                                     GetAnchorPoint()+wxPoint(width/2,height/2));
                }
            else if (GetAnchoring() == Anchoring::TopLeftCorner)
                { boundingBox = wxRect(GetAnchorPoint(), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::TopRightCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxSize(width,0), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::BottomLeftCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxPoint(0,height), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::BottomRightCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,height), wxSize(width,height)); }
            }
        else
            {
            if (GetAnchoring() == Anchoring::Center)
                {
                boundingBox = wxRect(GetAnchorPoint()-wxPoint(width/2,height/2),
                                     GetAnchorPoint()+wxPoint(width/2,height/2));
                }
            else if (GetAnchoring() == Anchoring::TopLeftCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxPoint(0,height), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::TopRightCorner)
                { boundingBox = wxRect(GetAnchorPoint(), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::BottomLeftCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,height), wxSize(width,height)); }
            else if (GetAnchoring() == Anchoring::BottomRightCorner)
                { boundingBox = wxRect(GetAnchorPoint()-wxPoint(width,0), wxSize(width,height)); }
            }
        if (IsFreeFloating())
            { boundingBox.Offset((boundingBox.GetLeftTop()*GetScaling())-boundingBox.GetLeftTop()); }

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
        if (tokenizer.CountTokens() == 0)
            { return; }
        const wxCoord spaceBetweenLines = (tokenizer.CountTokens()-1) *
                                           std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));

        if (GetTextOrientation() == Orientation::Horizontal)
            {
            // If top line is a header, then we need to not include that in the initial measure,
            // just measure the rest of the text. (This is because headers have their own left/right margins.)
            const auto firstLineEnd = GetText().find_first_of(L"\r\n", 0, 2);
            const auto secondLineStart = GetText().find_first_not_of(L"\r\n",
                ((firstLineEnd != std::wstring::npos) ? firstLineEnd : 0), 2);
            if (GetHeaderInfo().IsEnabled() && firstLineEnd != std::wstring::npos)
                { dc.GetMultiLineTextExtent(GetText().substr(secondLineStart), &width, &height); }
            else
                { dc.GetMultiLineTextExtent(GetText(), &width, &height); }
            // bounding box is padded four (horizontal) and two (vertical) pixels around text (if outlined)
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
                    GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
                const auto topLineSize = dc.GetMultiLineTextExtent(GetText().substr(0, firstLineEnd));
                width = std::max(topLineSize.GetWidth(), width);
                height += topLineSize.GetHeight();
                }
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
                    GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
                const auto topLineSize = dc.GetMultiLineTextExtent(GetText().substr(0, firstLineEnd));
                height = std::max(topLineSize.GetWidth(), height);
                width += topLineSize.GetHeight();
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
        wxASSERT_LEVEL_2_MSG(GetLegendIcons().size() == 0 ||
            (GetLegendIcons().size() && GetTextOrientation() == Orientation::Horizontal),
            L"Vertical legend not supported!");
        wxASSERT_LEVEL_2_MSG(GetLegendIcons().size() == 0 || !HasLegendIcons() ||
            (GetTextOrientation() == Orientation::Horizontal &&
             GetLeftPadding() >= GetMinLegendWidth()),
            wxString::Format(L"Left margin of text label should be at least %d DIPs "
                "if using legend icons! It is currently %d.",
                GetMinLegendWidth(), GetLeftPadding()));

        wxASSERT(GetFont().IsOk());
        wxDCFontChanger fc(dc, GetFont().Scaled(GetScaling()));
        wxPen scaledPen(GetPen());
        if (scaledPen.IsOk())
            {
            scaledPen.SetWidth(
                std::max<double>(ScaleToScreenAndCanvas(scaledPen.GetWidth()), 1));
            }
        // scaledPen might be bogus (if outlining isn't wanted),
        // just do this to reset when we are done.
        wxDCPenChanger pc(dc, *wxBLACK_PEN);
        wxDCBrushChanger bc(dc, *wxBLACK_BRUSH);

        const wxRect boundingBox = GetBoundingBox(dc);
        const wxRect contenBoundingBox = GetCachedContentBoundingBox();

        // draw the shadow
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

        // used for drawing the paper lines
        const wxPoint textOffset = ((GetTextOrientation() == Orientation::Horizontal) ?
            wxPoint(ScaleToScreenAndCanvas(GetLeftPadding()),
                ScaleToScreenAndCanvas(GetTopPadding())) :
            wxPoint(ScaleToScreenAndCanvas(GetTopPadding()),
                ScaleToScreenAndCanvas((GetRightPadding())) -
                ScaleToScreenAndCanvas((GetLeftPadding()))) );

        // get the uniform height of text
        wxCoord dummyX(0), dummyY(0), averageLineHeight(0);
        dc.GetMultiLineTextExtent(GetText(), &dummyX, &dummyY, &averageLineHeight);
        // draw the text
        dc.SetTextForeground(GetFontColor());
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
                    dc.SetPen(wxPen((i == 1) ?
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
            DrawMultiLineText(dc, boundingBox.GetLeftTop(), ScaleToScreenAndCanvas(GetLeftPadding()));
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
                    dc.SetPen(wxPen((i == 1) ?
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
                    dc.SetPen(wxPen(wxColour(0, 0, 255, Settings::GetTranslucencyValue()),
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
            DrawVerticalMultiLineText(dc, GetCachedContentBoundingBox().GetLeftTop(),
                ScaleToScreenAndCanvas(GetLeftPadding()));
            }
        // draw the outline
        if (scaledPen.IsOk())
            { dc.SetPen(scaledPen); }
        if (IsSelected())
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
            if (Settings::IsDebugFlagEnabled(DebugSettings::DrawBoundingBoxesOnSelection))
                {
                wxDCPenChanger pcDebug(dc, wxPen(*wxRED, ScaleToScreenAndCanvas(2), wxPENSTYLE_DOT));
                dc.DrawRectangle(GetCachedContentBoundingBox());
                }
            }
        else if (GetPen().IsOk())
            {
            wxDCBrushChanger bcBg(dc, *wxTRANSPARENT_BRUSH);
            if (GetBoxCorners() == BoxCorners::Rounded)
                {
                dc.DrawRoundedRectangle(boundingBox, Settings::GetBoxRoundedCornerRadius());
                }
            else
                {
                dc.DrawRectangle(boundingBox);
                }
            }

        // draw as a legend (if applicable)
        if (GetTextOrientation() == Orientation::Horizontal &&
            GetLegendIcons().size())
            {
            wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
            const auto topLine = lineTokenizer.GetNextToken();
            wxCoord topLineHeight{ 0 };
            // measure top line in case it is used as a header
                {
                wxDCFontChanger fc2(dc,
                    GetHeaderInfo().GetFont().IsOk() ?
                    GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
                topLineHeight = dc.GetTextExtent(topLine).GetHeight();
                }
            for (auto iconPos = GetLegendIcons().cbegin();
                 iconPos != GetLegendIcons().cend();
                 ++iconPos)
                {
                wxPen scaledIconPen(iconPos->m_pen.IsOk() ? iconPos->m_pen : dc.GetPen());
                scaledIconPen.SetWidth(
                    ScaleToScreenAndCanvas(
                        // if a line icon, make it a minimum of 2 pixels wide
                        (iconPos->m_shape == IconShape::HorizontalLineIcon ?
                         std::max(scaledIconPen.GetWidth(), 2) :
                         scaledIconPen.GetWidth())) );
                wxDCPenChanger pc2(dc, scaledIconPen);
                wxDCBrushChanger bc2(dc, iconPos->m_brush.IsOk() ? iconPos->m_brush : dc.GetBrush());
                const size_t currentIndex = (iconPos-GetLegendIcons().begin());
                wxCoord middleOfCurrentRow = static_cast<wxCoord>((averageLineHeight*currentIndex) +
                                                                   safe_divide<size_t>(averageLineHeight, 2)) +
                    (static_cast<wxCoord>(currentIndex)*std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()))) +
                    // spaces between proceeding lines
                    ScaleToScreenAndCanvas(GetTopPadding());
                const auto iconAreaWidth{ averageLineHeight };
                const auto iconRadius{ iconAreaWidth*.3 };
                const auto leftPadding{ iconAreaWidth*.5 };
                // if there is a minimum height that is taller than the text, then center
                // the text vertically
                const wxCoord yOffset = (GetHeaderInfo().IsEnabled() ?
                    topLineHeight + std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())) : 0);
                middleOfCurrentRow += yOffset;
                const auto midPoint = wxPoint(contenBoundingBox.GetTopLeft() +
                                              wxPoint(leftPadding, middleOfCurrentRow));
                wxPoint polygonPoints[6];
                wxRect boxRect{ wxRect(contenBoundingBox.GetTopLeft() +
                                       wxPoint(leftPadding, middleOfCurrentRow),
                                       wxSize(1, 1)).Inflate(iconRadius) };
                // draw the current shape now
                switch (iconPos->m_shape)
                    {
                    case IconShape::BlankIcon:
                        break;
                    case IconShape::HorizontalLineIcon:
                        dc.DrawLine(contenBoundingBox.GetTopLeft() +
                            wxPoint(std::ceil(leftPadding-(iconRadius)), middleOfCurrentRow),
                            contenBoundingBox.GetTopLeft() +
                            wxPoint(std::ceil(leftPadding+(iconRadius)), middleOfCurrentRow));
                        break;
                    case IconShape::ArrowRightIcon:
                        GraphItems::Polygon::DrawArrow(dc,
                            contenBoundingBox.GetTopLeft() +
                            wxPoint(std::ceil(leftPadding-(iconRadius)), middleOfCurrentRow),
                            contenBoundingBox.GetTopLeft() +
                            wxPoint(std::ceil(leftPadding+(iconRadius)), middleOfCurrentRow),
                            ScaleToScreenAndCanvas(LegendIcon::GetArrowheadSize()));
                        break;
                    case IconShape::CircleIcon:
                        dc.DrawCircle(contenBoundingBox.GetTopLeft() +
                            wxPoint(leftPadding, middleOfCurrentRow),
                            iconRadius);
                        break;
                    case IconShape::SquareIcon:
                        dc.DrawRectangle(
                            wxRect(contenBoundingBox.GetTopLeft() +
                                   wxPoint(leftPadding, middleOfCurrentRow),
                                   wxSize(1, 1)).Inflate(iconRadius));
                        break;
                    case IconShape::BoxPlotIcon:
                        // whisker
                        dc.DrawLine(wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2),
                                            boxRect.GetTop()),
                                    wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2),
                                            boxRect.GetBottom()));
                        dc.DrawLine(wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2)-boxRect.GetWidth()/4,
                                            boxRect.GetTop()),
                                    wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2)+boxRect.GetWidth()/4,
                                            boxRect.GetTop()));
                        dc.DrawLine(wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2)-boxRect.GetWidth()/4,
                                            boxRect.GetBottom()),
                                    wxPoint(boxRect.GetLeft()+(boxRect.GetWidth()/2)+boxRect.GetWidth()/4,
                                            boxRect.GetBottom()));
                        boxRect.y += (boxRect.GetHeight()/2)-(boxRect.GetHeight()/4); // center
                        boxRect.SetHeight(boxRect.GetHeight()/2);
                        dc.DrawRectangle(boxRect);
                        // median line
                        dc.DrawLine(wxPoint(boxRect.GetLeft(), boxRect.GetTop()+(boxRect.GetHeight()/2)),
                                    wxPoint(boxRect.GetRight(), boxRect.GetTop()+(boxRect.GetHeight()/2)));
                        break;
                    case IconShape::TriangleUpwardIcon:
                        polygonPoints[0] = midPoint + wxPoint(0, -iconRadius);
                        polygonPoints[1] = midPoint + wxPoint(-iconRadius, iconRadius);
                        polygonPoints[2] = midPoint + wxPoint(iconRadius, iconRadius);
                        dc.DrawPolygon(3, polygonPoints);
                        break;
                    case IconShape::TriangleDownwardIcon:
                        polygonPoints[0] = midPoint + wxPoint(0, iconRadius);
                        polygonPoints[1] = midPoint + wxPoint(-iconRadius, -iconRadius);
                        polygonPoints[2] = midPoint + wxPoint(iconRadius, -iconRadius);
                        dc.DrawPolygon(3, polygonPoints);
                        break;
                    case IconShape::TriangleRightIcon:
                        polygonPoints[0] = midPoint + wxPoint(iconRadius, 0);
                        polygonPoints[1] = midPoint + wxPoint(-iconRadius, iconRadius);
                        polygonPoints[2] = midPoint + wxPoint(-iconRadius, -iconRadius);
                        dc.DrawPolygon(3, polygonPoints);
                        break;
                    case IconShape::TriangleLeftIcon:
                        polygonPoints[0] = midPoint + wxPoint(-iconRadius, 0);
                        polygonPoints[1] = midPoint + wxPoint(iconRadius, iconRadius);
                        polygonPoints[2] = midPoint + wxPoint(iconRadius, -iconRadius);
                        dc.DrawPolygon(3, polygonPoints);
                        break;
                    case IconShape::DiamondIcon:
                        polygonPoints[0] = midPoint + wxPoint(0, -iconRadius);
                        polygonPoints[1] = midPoint + wxPoint(iconRadius, 0);
                        polygonPoints[2] = midPoint + wxPoint(0, iconRadius);
                        polygonPoints[3] = midPoint + wxPoint(-iconRadius, 0);
                        dc.DrawPolygon(4, polygonPoints);
                        break;
                    case IconShape::CrossIcon:
                        {
                        wxDCPenChanger dpc(dc, wxPen(wxPenInfo(dc.GetBrush().GetColour(),
                                                     dc.GetPen().GetWidth() * 2)));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                                    wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                                    wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
                        }
                        break;
                    case IconShape::AsteriskIcon:
                        {
                        wxDCPenChanger dpc(dc, wxPen(wxPenInfo(dc.GetBrush().GetColour(),
                                           dc.GetPen().GetWidth() * 2)));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(0, -iconRadius)),
                                    wxPoint(wxPoint(midPoint + wxPoint(0, iconRadius))));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, 0)),
                                    wxPoint(wxPoint(midPoint + wxPoint(iconRadius, 0))));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(iconRadius, iconRadius)),
                                    wxPoint(wxPoint(midPoint + wxPoint(-iconRadius, -iconRadius))));
                        dc.DrawLine(wxPoint(midPoint + wxPoint(-iconRadius, iconRadius)),
                                    wxPoint(wxPoint(midPoint + wxPoint(iconRadius, -iconRadius))));
                        }
                        break;
                    case IconShape::HexagonIcon:
                        polygonPoints[0] = midPoint + wxPoint(-iconRadius/2, -iconRadius);
                        polygonPoints[1] = midPoint + wxPoint(-iconRadius, 0);
                        polygonPoints[2] = midPoint + wxPoint(-iconRadius/2, iconRadius);
                        polygonPoints[3] = midPoint + wxPoint(iconRadius/2, iconRadius);
                        polygonPoints[4] = midPoint + wxPoint(iconRadius, 0);
                        polygonPoints[5] = midPoint + wxPoint(iconRadius/2, -iconRadius);
                        dc.DrawPolygon(6, polygonPoints);
                        break;
                    case IconShape::ImageIcon:
                        if (iconPos->m_img.IsOk())
                            {
                            const auto downScaledSize = geometry::calculate_downscaled_size(
                                            std::make_pair<double, double>(iconPos->m_img.GetWidth(),
                                                                           iconPos->m_img.GetHeight()),
                                            std::make_pair(ScaleToScreenAndCanvas(LegendIcon::GetIconWidth()),
                                                           static_cast<double>(averageLineHeight)));
                            wxImage scaledImg = iconPos->m_img.Scale(downScaledSize.first, downScaledSize.second,
                                                                     wxIMAGE_QUALITY_HIGH);
                            dc.DrawBitmap(wxBitmap(scaledImg),contenBoundingBox.GetTopLeft() +
                                          wxPoint(0, middleOfCurrentRow-(scaledImg.GetHeight()/2)));
                            }
                        break;
                    // Horizontal separators
                    //----------------------
                    case IconShape::HorizontalSeparator:
                        dc.DrawLine(contenBoundingBox.GetTopLeft() +
                            wxPoint((ScaleToScreenAndCanvas(2)), middleOfCurrentRow),
                            contenBoundingBox.GetTopLeft() +
                            wxPoint(contenBoundingBox.GetWidth()-((ScaleToScreenAndCanvas(2))),
                                middleOfCurrentRow));
                        break;
                    case IconShape::HorizontalArrowRightSeparator:
                        GraphItems::Polygon::DrawArrow(dc,
                            contenBoundingBox.GetTopLeft() +
                            wxPoint((ScaleToScreenAndCanvas(2)), middleOfCurrentRow),
                            contenBoundingBox.GetTopLeft() +
                            wxPoint(contenBoundingBox.GetWidth()-((ScaleToScreenAndCanvas(2))),
                                middleOfCurrentRow),
                            ScaleToScreenAndCanvas(iconPos->GetArrowheadSize()));
                        break;
                    // full-length icons
                    //------------------
                    case IconShape::ImageWholeLegend:
                        if (iconPos->m_img.IsOk())
                            {
                            wxRect legendArea = contenBoundingBox;
                            legendArea.SetHeight(averageLineHeight * GetLineCountWithoutHeader());

                            const auto downScaledSize = geometry::calculate_downscaled_size(
                                            std::make_pair<double, double>(iconPos->m_img.GetWidth(),
                                                                           iconPos->m_img.GetHeight()),
                                            std::make_pair(ScaleToScreenAndCanvas(GetLeftPadding()),
                                                legendArea.GetHeight() - yOffset -
                                                             ScaleToScreenAndCanvas(GetTopPadding()) -
                                                             ScaleToScreenAndCanvas(GetBottomPadding())));
                            wxImage scaledImg = iconPos->m_img.Scale(downScaledSize.first,downScaledSize.second,
                                                                     wxIMAGE_QUALITY_HIGH);
                            dc.DrawBitmap(wxBitmap(scaledImg),
                                legendArea.GetTopLeft() +
                                wxPoint(0, ScaleToScreenAndCanvas(GetTopPadding())+yOffset));
                            }
                        break;
                    case IconShape::ColorGradientIcon:
                        if (iconPos->m_colors.size() >= 2)
                            {
                            // we need to see how many colors there are and draw separate gradients between each
                            // pair, until the full spectrum is shown.
                            wxRect legendArea = contenBoundingBox;
                            legendArea.y += yOffset+ScaleToScreenAndCanvas(GetTopPadding());
                            legendArea.SetHeight(averageLineHeight * GetLineCountWithoutHeader());
                            legendArea.SetWidth(ScaleToScreenAndCanvas(LegendIcon::GetIconWidth()));

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
                    };
                }
            }

        return boundingBox;
        }

    //-------------------------------------------
    void Label::SplitTextByCharacter()
        {
        wxString splitText;
        for (size_t i = 0; i < GetText().length(); ++i)
            { splitText.append(GetText().at(i)).append(L"\n"); }
        SetText(splitText);
        }

    //-------------------------------------------
    void Label::SplitTextToFitLength(const size_t suggestedLineLength)
        {
        if (GetText().length() < suggestedLineLength)
            { return; }
        // if multiline, see if any of its lines are too long. If so,
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
                    Trim(true).append(L"\n");
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
    void Label::DrawVerticalMultiLineText(wxDC& dc, wxPoint pt, const wxCoord leftOffset) const
        {
        if (!IsOk())
            { return; }

        const wxCoord spaceBetweenLines = (GetLineCount()-1) *
                                           std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));

        pt.y += GetCachedContentBoundingBox().GetHeight();

        // render the text
        wxCoord lineX{ 0 }, lineY{ 0 }, offest{ 0 };
        wxSize fullTextSz = GetBoundingBox(dc).GetSize();
        fullTextSz.SetWidth(fullTextSz.GetWidth() -
            (spaceBetweenLines +
            ScaleToScreenAndCanvas(GetTopPadding()) +
            ScaleToScreenAndCanvas(GetBottomPadding())) );
        fullTextSz.SetHeight(fullTextSz.GetHeight() -
            (ScaleToScreenAndCanvas(GetLeftPadding()) +
            ScaleToScreenAndCanvas(GetRightPadding())) );
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        size_t currentLineNumber{ 0 };
        std::vector<wxString> tokenizedLineWords;

        // Measure 10 hair spaces and divide by 10 to fairly precise
        // measure of how wide one is. Measuring just one will double
        // up and cause the calculation to be way off.
        constexpr wchar_t hairSpace{ 0x200A };
        const double hairSpaceWidth = safe_divide<double>(
            dc.GetTextExtent(wxString(hairSpace, 10)).GetWidth(), 10);

        const auto trackTextLine = [&](wxString& textLine)
            {
            tokenizedLineWords.clear();
            // if line is shorter than the longest line, then fill it with
            // more spaces (spread evenly throughout) until it fits
            if (dc.GetTextExtent(textLine).GetWidth() < fullTextSz.GetHeight())
                {
                wxStringTokenizer wordTokenizer(textLine, L" ", wxTOKEN_RET_EMPTY);
                wxString wordStr;
                while (wordTokenizer.HasMoreTokens())
                    {
                    tokenizedLineWords.emplace_back(wordTokenizer.GetNextToken());
                    wordStr += tokenizedLineWords.back();
                    }
                // need at least two words for justifying text
                if (tokenizedLineWords.size() < 2)
                    { return; }
                // use hair spaces between words for more precise tracking
                auto lineDiff = fullTextSz.GetHeight() - dc.GetTextExtent(wordStr).GetWidth();
                const auto hairSpacesNeeded = std::ceil(safe_divide<double>(lineDiff, hairSpaceWidth));
                const auto wordSpaces = tokenizedLineWords.size() - 1;
                const auto thinSpacesPerWordPair = std::max(1.0,
                    std::floor(safe_divide<double>(hairSpacesNeeded, wordSpaces)));
                auto extraSpaces = hairSpacesNeeded - (thinSpacesPerWordPair*wordSpaces);
                // rebuild the text line with more spaces in it
                textLine.clear();
                for (const auto& word : tokenizedLineWords)
                    {
                    textLine.append(word).append(thinSpacesPerWordPair +
                                                 (extraSpaces > 0 ? 1 : 0), L' ');
                    --extraSpaces;
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
                    GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
                dc.GetTextExtent(token, &lineX, &lineY);
                if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushLeft)
                    { offest = HasLegendIcons() ? 0 : leftOffset; }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Centered)
                    {
                    offest = (safe_divide<double>(fullTextSz.GetHeight(),2) - safe_divide<double>(lineX, 2)) +
                             (safe_divide<double>(leftOffset, 2));
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushRight)
                    { offest = (fullTextSz.GetHeight()-lineX) + (HasLegendIcons() ? 0 : leftOffset); }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Justified)
                    {
                    offest = HasLegendIcons() ? 0 : leftOffset;
                    trackTextLine(token);
                    }
                }
            else
                {
                if (GetTextAlignment() == TextAlignment::FlushLeft)
                    { offest = 0; }
                else if (GetTextAlignment() == TextAlignment::Centered)
                    { offest = (safe_divide<double>(fullTextSz.GetHeight(),2) - safe_divide<double>(lineX,2)); }
                else if (GetTextAlignment() == TextAlignment::FlushRight)
                    { offest = (fullTextSz.GetHeight()-lineX); }
                else if (GetTextAlignment() == TextAlignment::Justified)
                    {
                    offest = 0;
                    trackTextLine(token);
                    }
                }
            if (!dc.GetFont().IsOk())
                {
                wxLogWarning(L"Invalid font used in graphics; will be replaced by system default.");
                dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
                }
            const auto currentLineOffset =
                (GetLinesIgnoringLeftMargin().find(currentLineNumber) != GetLinesIgnoringLeftMargin().cend()) ? 0 :
                ((GetHeaderInfo().IsEnabled() && currentLineNumber == 0) ? 0 : leftOffset);
            const bool isHeader{ (currentLineNumber == 0 &&
                                  GetLineCount() > 1 &&
                                  GetHeaderInfo().IsEnabled() &&
                                  GetHeaderInfo().GetFont().IsOk()) };
            wxDCFontChanger fc(dc,
                isHeader ?
                GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
            wxDCTextColourChanger tcc(dc,
                isHeader ?
                GetHeaderInfo().GetFontColor() : dc.GetTextForeground());
            dc.DrawRotatedText(token, pt.x, pt.y-offest-currentLineOffset, 90+m_tiltAngle);
            // move over for next line
            pt.x += (lineY+std::ceil(ScaleToScreenAndCanvas(GetLineSpacing())));
            ++currentLineNumber;
            }
        }

    //-------------------------------------------
    void Label::DrawMultiLineText(wxDC& dc, wxPoint pt, wxCoord leftOffset) const
        {
        if (!IsOk())
            { return; }
        const wxCoord spaceBetweenLines = (GetLineCount()-1) *
                                           std::ceil(ScaleToScreenAndCanvas(GetLineSpacing()));

        pt.y += CalcPageVerticalOffset(dc) + ScaleToScreenAndCanvas(GetTopPadding());
        leftOffset += CalcPageHorizontalOffset(dc);

        // render the text
        wxCoord lineX{ 0 }, lineY{ 0 }, offest{ 0 };
        wxSize fullTextSz = GetBoundingBox(dc).GetSize();
        fullTextSz.SetWidth(fullTextSz.GetWidth() -
            (ScaleToScreenAndCanvas(GetLeftPadding()) +
            ScaleToScreenAndCanvas(GetRightPadding())) );
        fullTextSz.SetHeight(fullTextSz.GetHeight() -
            (spaceBetweenLines +
            ScaleToScreenAndCanvas(GetTopPadding()) +
            ScaleToScreenAndCanvas(GetBottomPadding())) );
        wxStringTokenizer lineTokenizer(GetText(), L"\r\n", wxTOKEN_RET_EMPTY);
        size_t currentLineNumber{ 0 };
        std::vector<wxString> tokenizedLineWords;

        // Measure 10 hair spaces and divide by 10 to fairly precise
        // measure of how wide one is. Measuring just one will double
        // up and cause the calculation to be way off.
        constexpr wchar_t hairSpace{ 0x200A };
        const double hairSpaceWidth = safe_divide<double>(
            dc.GetTextExtent(wxString(hairSpace, 10)).GetWidth(), 10);

        const auto trackTextLine = [&](wxString& textLine)
            {
            tokenizedLineWords.clear();
            // if line is shorter than the longest line, then fill it with
            // more spaces (spread evenly throughout) until it fits
            if (dc.GetTextExtent(textLine).GetWidth() < fullTextSz.GetWidth())
                {
                // wxTOKEN_RET_EMPTY will preserve any extra (e.g., leading) spaces
                // from the original line; we will want that, in case the client
                // is simulating tabbing
                wxStringTokenizer wordTokenizer(textLine, L" ", wxTOKEN_RET_EMPTY);
                wxString wordStr;
                while (wordTokenizer.HasMoreTokens())
                    {
                    tokenizedLineWords.emplace_back(wordTokenizer.GetNextToken());
                    wordStr += tokenizedLineWords.back();
                    }
                // need at least two words for justifying text
                if (tokenizedLineWords.size() < 2)
                    { return; }
                // use hair spaces between words for more precise tracking
                auto lineDiff = fullTextSz.GetWidth() - dc.GetTextExtent(wordStr).GetWidth();
                const auto hairSpacesNeeded = std::ceil(safe_divide<double>(lineDiff, hairSpaceWidth));
                const auto wordSpaces = tokenizedLineWords.size() - 1;
                const auto hairSpacesPerWordPair = std::max(1.0,
                    std::floor(safe_divide<double>(hairSpacesNeeded, wordSpaces)));
                // the last gap between words may not need as many spaces as the others
                auto extraSpaces = hairSpacesNeeded - (hairSpacesPerWordPair*wordSpaces);
                // rebuild the text line with more (hair) spaces in it
                textLine.clear();
                for (const auto& word : tokenizedLineWords)
                    {
                    textLine.append(word).append(hairSpacesPerWordPair +
                                                 (extraSpaces > 0 ? 1 : 0), hairSpace);
                    --extraSpaces;
                    }
                lineDiff = dc.GetTextExtent(textLine).GetWidth();
                lineDiff = fullTextSz.GetWidth() - dc.GetTextExtent(textLine).GetWidth();
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
                    GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
                dc.GetTextExtent(token, &lineX, &lineY);
                // if pushed to the left and it's a legend, then it should be to the edge;
                // otherwise, align with the rest of the text
                if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushLeft)
                    { offest = HasLegendIcons() ? 0 : leftOffset; }
                // note that for centering we need to add half of the margin back in
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Centered)
                    {
                    offest = ((fullTextSz.GetWidth()/2)-lineX/2) +
                              (leftOffset/2);
                    }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::FlushRight)
                    { offest = (fullTextSz.GetWidth()-lineX) + (HasLegendIcons() ? 0 : leftOffset); }
                else if (GetHeaderInfo().GetLabelAlignment() == TextAlignment::Justified)
                    {
                    offest = HasLegendIcons() ? 0 : leftOffset;
                    trackTextLine(token);
                    }
                }
            else
                {
                if (GetTextAlignment() == TextAlignment::FlushLeft)
                    { offest = 0; }
                else if (GetTextAlignment() == TextAlignment::Centered)
                    { offest = ((fullTextSz.GetWidth()/2)-lineX/2); }
                else if (GetTextAlignment() == TextAlignment::FlushRight)
                    { offest = (fullTextSz.GetWidth()-lineX); }
                else if (GetTextAlignment() == TextAlignment::Justified)
                    {
                    offest = 0;
                    trackTextLine(token);
                    }
                }
            if (!dc.GetFont().IsOk())
                {
                wxLogWarning(L"Invalid font used in graphics; will be replaced by system default.");
                dc.SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));
                }
            const auto currentLineOffset =
                (GetLinesIgnoringLeftMargin().find(currentLineNumber) != GetLinesIgnoringLeftMargin().cend()) ? 0 :
                ((GetHeaderInfo().IsEnabled() && currentLineNumber == 0) ? 0 : leftOffset);
            const bool isHeader{ (currentLineNumber == 0 &&
                                  GetLineCount() > 1 &&
                                  GetHeaderInfo().IsEnabled() &&
                                  GetHeaderInfo().GetFont().IsOk()) };
            wxDCFontChanger fc(dc,
                isHeader ?
                GetHeaderInfo().GetFont().Scaled(GetScaling()) : dc.GetFont());
            wxDCTextColourChanger tcc(dc,
                isHeader ?
                GetHeaderInfo().GetFontColor() : dc.GetTextForeground());
            if (m_tiltAngle != 0)
                { dc.DrawRotatedText(token,pt.x+offest+currentLineOffset, pt.y, m_tiltAngle); }
            else
                { dc.DrawText(token,pt.x+offest+currentLineOffset, pt.y); }
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
            dc.SetFont(resizedFont);
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
            dc.SetFont(resizedFont);
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
