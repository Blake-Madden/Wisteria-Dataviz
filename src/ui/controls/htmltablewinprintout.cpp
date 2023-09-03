/////////////////////////////////////////////////////////////////////////////
// Name:        htmltablewinprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "htmltablewinprintout.h"

//--------------------------------------------
bool HtmlTablePrintout::OnPrintPage(int page)
    {
    if (HasPage(page))
        {
        m_currentPage = page;
        wxDC* dc = GetDC();
        if (dc)
            {
            float scaleX{ 0 }, scaleY{ 0 };
            GetScreenToPageScaling(scaleX, scaleY);

            // set a suitable scaling factor
            const float scaleXReciprocal = safe_divide<float>(1.0f, scaleX);
            const float scaleYReciprocal = safe_divide<float>(1.0f, scaleY);
            dc->SetUserScale(scaleX, scaleY);

            // get the size of the DC's drawing area in pixels
            int drawingWidth, drawingHeight;
            int dcWidth, dcHeight;
            dc->GetSize(&drawingWidth, &drawingHeight);
            dc->GetSize(&dcWidth, &dcHeight);
            drawingWidth *= scaleXReciprocal;
            drawingHeight *= scaleYReciprocal;

            // let's have at least 10 device units margin
            const float marginX = GetMarginPadding();
            const float marginY = GetMarginPadding();

            // remove the margins from the drawing area size
            drawingWidth -= static_cast<wxCoord>(2*marginX);
            int topMargin = marginY;
            int bottomMargin = marginY;
            // remove space for the headers and footers (if being used)
            wxCoord textWidth{ 0 }, textHeight{ 0 };
            wxCoord bodyStart = marginY;
            dc->GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
            if (GetLeftPrinterHeader().length() ||
                GetCenterPrinterHeader().length() ||
                GetRightPrinterHeader().length())
                {
                topMargin += textHeight;
                bodyStart += textHeight+marginY;
                }
            if (GetLeftPrinterFooter().length() ||
                GetCenterPrinterFooter().length() ||
                GetRightPrinterFooter().length())
                { bottomMargin += textHeight; }
            drawingHeight -= (topMargin+bottomMargin);

            const auto drawTables =
                [this, page, marginX, bodyStart, drawingWidth, drawingHeight]
                (wxDC& drawDC)
                {
                // draw the tables
                wxHtmlDCRenderer htmlRenderer;
                htmlRenderer.SetDC(&drawDC);
                htmlRenderer.SetSize(drawingWidth, drawingHeight);
                int currentPageHeight = 0;
                for (int i = m_pageStarts[page-1].first; i <= m_pageStarts[page-1].second; ++i)
                    {
                    htmlRenderer.SetHtmlText(m_htmlTables[i]);
                    htmlRenderer.Render(marginX,bodyStart+currentPageHeight);
                    currentPageHeight += htmlRenderer.GetTotalHeight() +
                                            wxSizerFlags::GetDefaultBorder();
                    }
                };

            const auto drawHeadersAndFooters =
                [this, marginX, marginY, drawingWidth, drawingHeight, topMargin,
                    &textWidth, &textHeight]
                (wxDC& drawDC)
                {
                // draw the headers
                drawDC.SetDeviceOrigin(0, 0);
                if (GetLeftPrinterHeader().length() ||
                    GetCenterPrinterHeader().length() ||
                    GetRightPrinterHeader().length())
                    {
                    if (GetLeftPrinterHeader().length())
                        {
                        drawDC.DrawText(ExpandPrintString(GetLeftPrinterHeader()),
                            static_cast<wxCoord>(marginX),
                            static_cast<wxCoord>(marginY/2));
                        }
                    if (GetCenterPrinterHeader().length())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetCenterPrinterHeader()),
                                                            &textWidth, &textHeight);
                        drawDC.DrawText(ExpandPrintString(GetCenterPrinterHeader()),
                            static_cast<wxCoord>(safe_divide<float>((drawingWidth),2) -
                                                    safe_divide<float>(textWidth,2)),
                            static_cast<wxCoord>(marginY/2));
                        }
                    if (GetRightPrinterHeader().length())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetRightPrinterHeader()),
                                                            &textWidth, &textHeight);
                        drawDC.DrawText(ExpandPrintString(GetRightPrinterHeader()),
                            static_cast<wxCoord>((drawingWidth) - (marginX+textWidth)),
                            static_cast<wxCoord>(marginY/2));
                        }
                    }
                // draw the footers
                if (GetLeftPrinterFooter().length() ||
                    GetCenterPrinterFooter().length() ||
                    GetRightPrinterFooter().length())
                    {
                    drawDC.GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                    // move down past the print header area, drawing (tables) area,
                    // and half the bottom margin (to center the footer vertically)
                    const wxCoord yPos = topMargin+drawingHeight+(marginY/2);
                    if (GetLeftPrinterFooter().length())
                        {
                        drawDC.DrawText(ExpandPrintString(GetLeftPrinterFooter()),
                            static_cast<wxCoord>(marginX),
                            yPos);
                        }
                    if (GetCenterPrinterFooter().length())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetCenterPrinterFooter()),
                                                            &textWidth, &textHeight);
                        drawDC.DrawText(ExpandPrintString(GetCenterPrinterFooter()),
                            static_cast<wxCoord>(safe_divide<float>(drawingWidth,2) -
                                                    safe_divide<float>(textWidth,2)),
                            yPos);
                        }
                    if (GetRightPrinterFooter().length())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetRightPrinterFooter()),
                                                            &textWidth, &textHeight);
                        drawDC.DrawText(ExpandPrintString(GetRightPrinterFooter()),
                            static_cast<wxCoord>((drawingWidth - (marginX+textWidth))),
                            yPos);
                        }
                    }
                };

            // need to use wxGCDC for any color transparency
            if (!m_printCanvas.IsOk() ||
                m_printCanvas.GetSize() != wxSize(dcWidth, dcHeight))
                { m_printCanvas.Create(dcWidth, dcHeight); }
            wxMemoryDC memDc(m_printCanvas);
            memDc.Clear();
            wxGCDC gcdc(memDc);

            drawTables(gcdc);
            drawHeadersAndFooters(gcdc);
            Wisteria::Canvas::DrawWatermarkLabel(gcdc,
                wxRect(wxSize(drawingWidth, drawingHeight)), m_waterMark);
            // copy renderings back into printer DC
            dc->Blit(0, 0, dcWidth, dcHeight, &memDc, 0, 0);
            memDc.SelectObject(wxNullBitmap);

            return true;
            }
        else return false;
        }
    else
        { return false; }
    }

//--------------------------------------------
void HtmlTablePrintout::OnPreparePrinting()
    {
    m_pageStarts.clear();
    m_currentPage = 0;

    // calculate lines per page and line height
    wxDC* dc = GetDC();
    if (dc)
        {
        // adjust user scaling
        float scaleX{ 0 }, scaleY{ 0 };
        GetScreenToPageScaling(scaleX, scaleY);
        const float scaleXReciprocal = safe_divide<float>(1.0f, scaleX);
        const float scaleYReciprocal = safe_divide<float>(1.0f, scaleY);
        dc->SetUserScale(scaleX, scaleY);

        // Get the size of the DC's drawing area in pixels
        wxCoord dcWidth, dcHeight;
        dc->GetSize(&dcWidth, &dcHeight);
        const wxCoord drawingWidth = (dcWidth*scaleXReciprocal) -
                                        (GetMarginPadding()*2)/*side margins*/;

        // Measure a standard line of text
        wxCoord textWidth, textHeight;
        dc->GetTextExtent(L"AQ", &textWidth, &textHeight);

        // Remove the margins from the drawing area size
        wxCoord heightMargin = GetMarginPadding()*2;
        // Remove space for the headers and footers (if being used)
        if (GetLeftPrinterHeader().length() ||
            GetCenterPrinterHeader().length() ||
            GetRightPrinterHeader().length())
            { heightMargin += textHeight+GetMarginPadding(); }
        if (GetLeftPrinterFooter().length() ||
            GetCenterPrinterFooter().length() ||
            GetRightPrinterFooter().length())
            { heightMargin += textHeight+GetMarginPadding(); }
        const wxCoord drawingHeight = (dcHeight*scaleYReciprocal)-heightMargin;

        // paginate by measuring each table and storing which tables
        // should be on which page
        wxHtmlDCRenderer htmlRenderer;
        wxMemoryDC dummyDC(dc);
        wxGCDC gcdc(dc);
        htmlRenderer.SetDC(&gcdc);
        htmlRenderer.SetSize(drawingWidth, drawingHeight);
        int currentPageHeight{ 0 };
        int currentPageFirstTable{ 0 };
        std::vector<wxString>::const_iterator tablesIter;
        for (tablesIter = m_htmlTables.cbegin();
             tablesIter != m_htmlTables.cend();
             ++tablesIter)
            {
            htmlRenderer.SetHtmlText(*tablesIter);
            // note that we are rendering to a memory DC when calculating the page layout;
            // otherwise, all of this rendering will appear stacked on top of the real output
            htmlRenderer.Render(0, 0);
            const int currentTableHeight = htmlRenderer.GetTotalHeight() +
                                            wxSizerFlags::GetDefaultBorder();
            if (currentPageHeight+currentTableHeight > drawingHeight)
                {
                m_pageStarts.push_back(
                    std::make_pair(currentPageFirstTable,
                                    std::max(static_cast<int>(tablesIter - m_htmlTables.cbegin()),1)-1) );
                currentPageFirstTable = static_cast<int>(tablesIter - m_htmlTables.cbegin());
                currentPageHeight = currentTableHeight;
                }
            else
                { currentPageHeight += currentTableHeight; }
            }
        // add the last page
        m_pageStarts.push_back(
            std::make_pair(currentPageFirstTable,
                            std::max(static_cast<int>(tablesIter - m_htmlTables.cbegin()),1)-1) );
        }
    }