/////////////////////////////////////////////////////////////////////////////
// Name:        htmltablewinprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
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
        if (wxDC* dc = GetDC(); dc != nullptr)
            {
            double scaleX{ 0 }, scaleY{ 0 };
            GetScreenToPageScaling(scaleX, scaleY);

            // set a suitable scaling factor
            const auto scaleXReciprocal = safe_divide<double>(1.0, scaleX);
            const auto scaleYReciprocal = safe_divide<double>(1.0, scaleY);
            dc->SetUserScale(scaleX, scaleY);

            // get the size of the DC's drawing area in pixels
            int drawingWidth{ 0 }, drawingHeight{ 0 };
            int dcWidth{ 0 }, dcHeight{ 0 };
            dc->GetSize(&drawingWidth, &drawingHeight);
            dc->GetSize(&dcWidth, &dcHeight);
            drawingWidth *= scaleXReciprocal;
            drawingHeight *= scaleYReciprocal;

            // let's have at least 10 device units margin
            const auto marginX = GetMarginPadding();
            const auto marginY = GetMarginPadding();

            // remove the margins from the drawing area size
            drawingWidth -= static_cast<wxCoord>(2 * marginX);
            int topMargin = marginY;
            int bottomMargin = marginY;
            // remove space for the headers and footers (if being used)
            wxCoord textWidth{ 0 }, textHeight{ 0 };
            wxCoord bodyStart = marginY;
            dc->GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
            if (!GetLeftPrinterHeader().empty() || !GetCenterPrinterHeader().empty() ||
                !GetRightPrinterHeader().empty())
                {
                topMargin += textHeight;
                bodyStart += textHeight + marginY;
                }
            if (!GetLeftPrinterFooter().empty() || !GetCenterPrinterFooter().empty() ||
                !GetRightPrinterFooter().empty())
                {
                bottomMargin += textHeight;
                }
            drawingHeight -= (topMargin + bottomMargin);

            const auto drawTables =
                [this, page, marginX, bodyStart, drawingWidth, drawingHeight](wxDC& drawDC)
            {
                // draw the tables
                wxHtmlDCRenderer htmlRenderer;
                htmlRenderer.SetDC(&drawDC);
                htmlRenderer.SetSize(drawingWidth, drawingHeight);
                int currentPageHeight = 0;
                for (int i = m_pageStarts[page - 1].first; i <= m_pageStarts[page - 1].second; ++i)
                    {
                    if (static_cast<size_t>(i) < m_htmlTables.size())
                        {
                        htmlRenderer.SetHtmlText(m_htmlTables[i]);
                        htmlRenderer.Render(marginX, bodyStart + currentPageHeight);
                        currentPageHeight +=
                            htmlRenderer.GetTotalHeight() + wxSizerFlags::GetDefaultBorder();
                        }
                    }
            };

            const auto drawHeadersAndFooters = [this, marginX, marginY, drawingWidth, drawingHeight,
                                                topMargin, &textWidth, &textHeight](wxDC& drawDC)
            {
                // draw the headers
                drawDC.SetDeviceOrigin(0, 0);
                if (!GetLeftPrinterHeader().empty() || !GetCenterPrinterHeader().empty() ||
                    !GetRightPrinterHeader().empty())
                    {
                    if (!GetLeftPrinterHeader().empty())
                        {
                        drawDC.DrawText(ExpandPrintString(GetLeftPrinterHeader()),
                                        static_cast<wxCoord>(marginX),
                                        static_cast<wxCoord>(marginY / 2));
                        }
                    if (!GetCenterPrinterHeader().empty())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetCenterPrinterHeader()),
                                             &textWidth, &textHeight);
                        drawDC.DrawText(
                            ExpandPrintString(GetCenterPrinterHeader()),
                            static_cast<wxCoord>(safe_divide<double>((drawingWidth), 2) -
                                                 safe_divide<double>(textWidth, 2)),
                            static_cast<wxCoord>(marginY / 2));
                        }
                    if (!GetRightPrinterHeader().empty())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetRightPrinterHeader()), &textWidth,
                                             &textHeight);
                        drawDC.DrawText(
                            ExpandPrintString(GetRightPrinterHeader()),
                            static_cast<wxCoord>((drawingWidth) - (marginX + textWidth)),
                            static_cast<wxCoord>(marginY / 2));
                        }
                    }
                // draw the footers
                if (!GetLeftPrinterFooter().empty() || !GetCenterPrinterFooter().empty() ||
                    !GetRightPrinterFooter().empty())
                    {
                    drawDC.GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                    // move down past the print header area, drawing (tables) area,
                    // and half the bottom margin (to center the footer vertically)
                    const wxCoord yPos = topMargin + drawingHeight + (marginY / 2);
                    if (!GetLeftPrinterFooter().empty())
                        {
                        drawDC.DrawText(ExpandPrintString(GetLeftPrinterFooter()),
                                        static_cast<wxCoord>(marginX), yPos);
                        }
                    if (!GetCenterPrinterFooter().empty())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetCenterPrinterFooter()),
                                             &textWidth, &textHeight);
                        drawDC.DrawText(ExpandPrintString(GetCenterPrinterFooter()),
                                        static_cast<wxCoord>(safe_divide<double>(drawingWidth, 2) -
                                                             safe_divide<double>(textWidth, 2)),
                                        yPos);
                        }
                    if (!GetRightPrinterFooter().empty())
                        {
                        drawDC.GetTextExtent(ExpandPrintString(GetRightPrinterFooter()), &textWidth,
                                             &textHeight);
                        drawDC.DrawText(
                            ExpandPrintString(GetRightPrinterFooter()),
                            static_cast<wxCoord>((drawingWidth - (marginX + textWidth))), yPos);
                        }
                    }
            };

            // need to use wxGCDC for any color transparency
            if (!m_printCanvas.IsOk() || m_printCanvas.GetSize() != wxSize(dcWidth, dcHeight))
                {
                m_printCanvas.Create(dcWidth, dcHeight);
                }
            wxMemoryDC memDc(m_printCanvas);
            memDc.Clear();
            wxGCDC gcdc(memDc);

            drawTables(gcdc);
            drawHeadersAndFooters(gcdc);
            Wisteria::Canvas::DrawWatermarkLabel(gcdc, wxRect(wxSize(drawingWidth, drawingHeight)),
                                                 m_waterMark, 1.0);
            // copy renderings back into printer DC
            dc->Blit(0, 0, dcWidth, dcHeight, &memDc, 0, 0);
            memDc.SelectObject(wxNullBitmap);

            return true;
            }
        return false;
        }

    return false;
    }

//--------------------------------------------
void HtmlTablePrintout::OnPreparePrinting()
    {
    m_pageStarts.clear();
    m_currentPage = 0;

    // calculate lines per page and line height
    if (wxDC* dc = GetDC(); dc != nullptr)
        {
        // adjust user scaling
        double scaleDownX{ 0 }, scaleDownY{ 0 };
        GetPageToScreenScaling(scaleDownX, scaleDownY);

        // Get the size of the DC's drawing area in pixels
        wxCoord dcWidth{ 0 }, dcHeight{ 0 };
        dc->GetSize(&dcWidth, &dcHeight);
        dc->SetUserScale(safe_divide<double>(1.0, scaleDownX),
                         safe_divide<double>(1.0, scaleDownY));

        const wxCoord drawingWidth =
            (dcWidth * scaleDownX) - (GetMarginPadding() * 2) /*side margins*/;

        // Measure a standard line of text
        wxCoord textWidth{ 0 }, textHeight{ 0 };
        dc->GetTextExtent(_DT(L"AQ"), &textWidth, &textHeight);

        // Remove the margins from the drawing area size
        wxCoord heightMargin = GetMarginPadding() * 2;
        // Remove space for the headers and footers (if being used)
        if (!GetLeftPrinterHeader().empty() || !GetCenterPrinterHeader().empty() ||
            !GetRightPrinterHeader().empty())
            {
            heightMargin += textHeight + GetMarginPadding();
            }
        if (!GetLeftPrinterFooter().empty() || !GetCenterPrinterFooter().empty() ||
            !GetRightPrinterFooter().empty())
            {
            heightMargin += textHeight + GetMarginPadding();
            }
        const wxCoord drawingHeight = (dcHeight * scaleDownY) - heightMargin;

        // paginate by measuring each table and storing which tables
        // should be on which page
        if (!m_printCanvas.IsOk() || m_printCanvas.GetSize() != wxSize(dcWidth, dcHeight))
            {
            m_printCanvas.Create(dcWidth, dcHeight);
            }
        wxMemoryDC memDc(m_printCanvas);
        memDc.Clear();
        wxGCDC gcdc(memDc);
        gcdc.SetUserScale(safe_divide<double>(1.0, scaleDownX),
                          safe_divide<double>(1.0, scaleDownY));
        wxHtmlDCRenderer htmlRenderer;
        htmlRenderer.SetDC(&gcdc);
        htmlRenderer.SetSize(drawingWidth, drawingHeight);
        int currentPageHeight{ 0 };
        int currentPageFirstTable{ 0 };
        std::vector<wxString>::const_iterator tablesIter;
        for (tablesIter = m_htmlTables.cbegin(); tablesIter != m_htmlTables.cend(); ++tablesIter)
            {
            htmlRenderer.SetHtmlText(*tablesIter);
            // note that we are rendering to a memory DC when calculating the page layout;
            // otherwise, all of this rendering will appear stacked on top of the real output
            htmlRenderer.Render(0, 0);
            const int currentTableHeight =
                htmlRenderer.GetTotalHeight() + wxSizerFlags::GetDefaultBorder();
            if (currentPageHeight + currentTableHeight > drawingHeight)
                {
                m_pageStarts.emplace_back(
                    currentPageFirstTable,
                    std::max(static_cast<int>(tablesIter - m_htmlTables.cbegin()), 1) - 1);
                currentPageFirstTable = static_cast<int>(tablesIter - m_htmlTables.cbegin());
                currentPageHeight = currentTableHeight;
                }
            else
                {
                currentPageHeight += currentTableHeight;
                }
            }
        // add the last page
        m_pageStarts.emplace_back(
            currentPageFirstTable,
            std::max(static_cast<int>(tablesIter - m_htmlTables.cbegin()), 1) - 1);
        }
    }
