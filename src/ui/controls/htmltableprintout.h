/** @addtogroup UI
    @brief Utility classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __HTMLTABLEPRINTOUT_H__
#define __HTMLTABLEPRINTOUT_H__

#include "../../base/canvas.h"
#include "../../import/html_encode.h"
#include <wx/wx.h>

namespace Wisteria::UI
    {
    /// @private
    class HtmlTablePrintout final : public wxPrintout
        {
      public:
        HtmlTablePrintout(const wxString& title) : wxPrintout(title) {}

        void AddTable(const wxString& table)
            {
            std::wstring strippedTable = table;
            lily_of_the_valley::html_format::strip_hyperlinks(strippedTable, false);
            m_htmlTables.push_back(strippedTable);
            }

        [[nodiscard]]
        bool HasPage(int pageNum) final
            {
            return (pageNum >= 1 && pageNum <= static_cast<int>(m_pageStarts.size()));
            }

        void GetPageInfo(int* minPage, int* maxPage, int* selPageFrom, int* selPageTo) final
            {
            *minPage = 1;
            *maxPage = static_cast<int>(m_pageStarts.size());
            *selPageFrom = 1;
            *selPageTo = static_cast<int>(m_pageStarts.size());
            }

        bool OnPrintPage(int page) final
            {
            if (HasPage(page))
                {
                m_currentPage = page;
                wxDC* dc = GetDC();
                if (dc)
                    {
                    float scaleX, scaleY;
                    GetScreenToPageScaling(scaleX, scaleY);

                    // set a suitable scaling factor
                    const float scaleXReciprical = safe_divide<float>(1.0f, scaleX);
                    const float scaleYReciprical = safe_divide<float>(1.0f, scaleY);
                    dc->SetUserScale(scaleX, scaleY);

                    // get the size of the DC's drawing area in pixels
                    int drawingWidth, drawingHeight;
                    int dcWidth, dcHeight;
                    dc->GetSize(&drawingWidth, &drawingHeight);
                    dc->GetSize(&dcWidth, &dcHeight);
                    drawingWidth *= scaleXReciprical;
                    drawingHeight *= scaleYReciprical;

                    // let's have at least 10 device units margin
                    const float marginX = GetMarginPadding();
                    const float marginY = GetMarginPadding();

                    // remove the margins from the drawing area size
                    drawingWidth -= static_cast<wxCoord>(2 * marginX);
                    int topMargin = marginY;
                    int bottomMargin = marginY;
                    // remove space for the headers and footers (if being used)
                    wxCoord textWidth(0), textHeight(0);
                    wxCoord bodyStart = marginY;
                    dc->GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                    if (GetLeftPrinterHeader().length() || GetCenterPrinterHeader().length() ||
                        GetRightPrinterHeader().length())
                        {
                        topMargin += textHeight;
                        bodyStart += textHeight + marginY;
                        }
                    if (GetLeftPrinterFooter().length() || GetCenterPrinterFooter().length() ||
                        GetRightPrinterFooter().length())
                        {
                        bottomMargin += textHeight;
                        }
                    drawingHeight -= (topMargin + bottomMargin);

                    const auto drawTables =
                        [this, page, marginX, bodyStart, drawingWidth, drawingHeight](wxDC& dc)
                    {
                        // draw the tables
                        wxHtmlDCRenderer htmlRenderer;
                        htmlRenderer.SetDC(&dc);
                        htmlRenderer.SetSize(drawingWidth, drawingHeight);
                        int currentPageHeight = 0;
                        for (int i = m_pageStarts[page - 1].first;
                             i <= m_pageStarts[page - 1].second; ++i)
                            {
                            htmlRenderer.SetHtmlText(m_htmlTables[i]);
                            htmlRenderer.Render(marginX, bodyStart + currentPageHeight);
                            currentPageHeight +=
                                htmlRenderer.GetTotalHeight() + wxSizerFlags::GetDefaultBorder();
                            }
                    };

                    const auto drawHeadersAndFooters = [this, marginX, marginY, drawingWidth,
                                                        drawingHeight, topMargin, &textWidth,
                                                        &textHeight](wxDC& dc)
                    {
                        // draw the headers
                        dc.SetDeviceOrigin(0, 0);
                        if (GetLeftPrinterHeader().length() || GetCenterPrinterHeader().length() ||
                            GetRightPrinterHeader().length())
                            {
                            if (GetLeftPrinterHeader().length())
                                {
                                dc.DrawText(ExpandPrintString(GetLeftPrinterHeader()),
                                            static_cast<wxCoord>(marginX),
                                            static_cast<wxCoord>(marginY / 2));
                                }
                            if (GetCenterPrinterHeader().length())
                                {
                                dc.GetTextExtent(ExpandPrintString(GetCenterPrinterHeader()),
                                                 &textWidth, &textHeight);
                                dc.DrawText(
                                    ExpandPrintString(GetCenterPrinterHeader()),
                                    static_cast<wxCoord>(safe_divide<float>((drawingWidth), 2) -
                                                         safe_divide<float>(textWidth, 2)),
                                    static_cast<wxCoord>(marginY / 2));
                                }
                            if (GetRightPrinterHeader().length())
                                {
                                dc.GetTextExtent(ExpandPrintString(GetRightPrinterHeader()),
                                                 &textWidth, &textHeight);
                                dc.DrawText(
                                    ExpandPrintString(GetRightPrinterHeader()),
                                    static_cast<wxCoord>((drawingWidth) - (marginX + textWidth)),
                                    static_cast<wxCoord>(marginY / 2));
                                }
                            }
                        // draw the footers
                        if (GetLeftPrinterFooter().length() || GetCenterPrinterFooter().length() ||
                            GetRightPrinterFooter().length())
                            {
                            dc.GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                            // move down past the print header area, drawing (tables) area,
                            // and half the bottom margin (to center the footer vertically)
                            const wxCoord yPos = topMargin + drawingHeight + (marginY / 2);
                            if (GetLeftPrinterFooter().length())
                                {
                                dc.DrawText(ExpandPrintString(GetLeftPrinterFooter()),
                                            static_cast<wxCoord>(marginX), yPos);
                                }
                            if (GetCenterPrinterFooter().length())
                                {
                                dc.GetTextExtent(ExpandPrintString(GetCenterPrinterFooter()),
                                                 &textWidth, &textHeight);
                                dc.DrawText(
                                    ExpandPrintString(GetCenterPrinterFooter()),
                                    static_cast<wxCoord>(safe_divide<float>(drawingWidth, 2) -
                                                         safe_divide<float>(textWidth, 2)),
                                    yPos);
                                }
                            if (GetRightPrinterFooter().length())
                                {
                                dc.GetTextExtent(ExpandPrintString(GetRightPrinterFooter()),
                                                 &textWidth, &textHeight);
                                dc.DrawText(
                                    ExpandPrintString(GetRightPrinterFooter()),
                                    static_cast<wxCoord>((drawingWidth - (marginX + textWidth))),
                                    yPos);
                                }
                            }
                    };

                    // need to use wxGCDC for any color transparency
                    if (!m_printCanvas.IsOK() ||
                        m_printCanvas.GetSize() != wxSize(dcWidth, dcHeight))
                        {
                        m_printCanvas.Create(dcWidth, dcHeight);
                        }
                    wxMemoryDC memDc(m_printCanvas);
                    memDc.Clear();
                    wxGCDC gcdc(memDc);

                    drawTables(gcdc);
                    drawHeadersAndFooters(gcdc);
                    Wisteria::Canvas::DrawWatermarkLabel(
                        gcdc, wxRect(wxSize(drawingWidth, drawingHeight)), m_waterMark);
                    // copy renderings back into printer DC
                    dc->Blit(0, 0, dcWidth, dcHeight, &memDc, 0, 0);
                    memDc.SelectObject(wxNullBitmap);

                    return true;
                    }
                else
                    {
                    return false;
                    }
                }
            else
                {
                return false;
                }
            }

        void OnPreparePrinting() final
            {
            m_pageStarts.clear();
            m_currentPage = 0;

            // calculate lines per page and line height
            wxDC* dc = GetDC();
            if (dc)
                {
                // adjust user scaling
                float scaleX, scaleY;
                GetScreenToPageScaling(scaleX, scaleY);
                const float scaleXReciprical = safe_divide<float>(1.0f, scaleX);
                const float scaleYReciprical = safe_divide<float>(1.0f, scaleY);
                dc->SetUserScale(scaleX, scaleY);

                // Get the size of the DC's drawing area in pixels
                wxCoord dcWidth, dcHeight;
                dc->GetSize(&dcWidth, &dcHeight);
                const wxCoord drawingWidth = (dcWidth * scaleXReciprical) -
                                             // side margins
                                             (GetMarginPadding() * 2);

                // Measure a standard line of text
                wxCoord textWidth, textHeight;
                dc->GetTextExtent(L"AQ", &textWidth, &textHeight);

                // Remove the margins from the drawing area size
                wxCoord heightMargin = GetMarginPadding() * 2;
                // Remove space for the headers and footers (if being used)
                if (GetLeftPrinterHeader().length() || GetCenterPrinterHeader().length() ||
                    GetRightPrinterHeader().length())
                    {
                    heightMargin += textHeight + GetMarginPadding();
                    }
                if (GetLeftPrinterFooter().length() || GetCenterPrinterFooter().length() ||
                    GetRightPrinterFooter().length())
                    {
                    heightMargin += textHeight + GetMarginPadding();
                    }
                const wxCoord drawingHeight = (dcHeight * scaleYReciprical) - heightMargin;

                // paginate by measuring each table and storing which tables
                // should be on which page
                wxHtmlDCRenderer htmlRenderer;
                wxMemoryDC dummyDC(dc);
                htmlRenderer.SetDC(&dummyDC);
                htmlRenderer.SetSize(drawingWidth, drawingHeight);
                int currentPageHeight = 0;
                int currentPageFirstTable = 0;
                std::vector<wxString>::const_iterator tablesIter;
                for (tablesIter = m_htmlTables.begin(); tablesIter != m_htmlTables.end();
                     ++tablesIter)
                    {
                    htmlRenderer.SetHtmlText(*tablesIter);
                    // note that we are rendering to a memory DC when calculating the page layout;
                    // otherwise, all of this rendering will appear stacked on top
                    // of the real output
                    htmlRenderer.Render(0, 0);
                    const int currentTableHeight =
                        htmlRenderer.GetTotalHeight() + wxSizerFlags::GetDefaultBorder();
                    if (currentPageHeight + currentTableHeight > drawingHeight)
                        {
                        m_pageStarts.push_back(std::make_pair(
                            currentPageFirstTable,
                            std::max(static_cast<int>(tablesIter - m_htmlTables.begin()), 1) - 1));
                        currentPageFirstTable = static_cast<int>(tablesIter - m_htmlTables.begin());
                        currentPageHeight = currentTableHeight;
                        }
                    else
                        {
                        currentPageHeight += currentTableHeight;
                        }
                    }
                // add the last page
                m_pageStarts.push_back(std::make_pair(
                    currentPageFirstTable,
                    std::max(static_cast<int>(tablesIter - m_htmlTables.begin()), 1) - 1));
                }
            }

        [[nodiscard]]
        size_t GetPageCount() const noexcept
            {
            return m_pageStarts.size();
            }

        // printer header functions
        void SetLeftPrinterHeader(const wxString& header) { m_leftPrinterHeader = header; }

        [[nodiscard]]
        const wxString& GetLeftPrinterHeader() const noexcept
            {
            return m_leftPrinterHeader;
            }

        void SetCenterPrinterHeader(const wxString& header) { m_centerPrinterHeader = header; }

        [[nodiscard]]
        const wxString& GetCenterPrinterHeader() const noexcept
            {
            return m_centerPrinterHeader;
            }

        void SetRightPrinterHeader(const wxString& header) { m_rightPrinterHeader = header; }

        [[nodiscard]]
        const wxString& GetRightPrinterHeader() const noexcept
            {
            return m_rightPrinterHeader;
            }

        // printer footer functions
        void SetLeftPrinterFooter(const wxString& header) { m_leftPrinterFooter = header; }

        [[nodiscard]]
        const wxString& GetLeftPrinterFooter() const noexcept
            {
            return m_leftPrinterFooter;
            }

        void SetCenterPrinterFooter(const wxString& header) { m_centerPrinterFooter = header; }

        [[nodiscard]]
        const wxString& GetCenterPrinterFooter() const noexcept
            {
            return m_centerPrinterFooter;
            }

        void SetRightPrinterFooter(const wxString& header) { m_rightPrinterFooter = header; }

        [[nodiscard]]
        const wxString& GetRightPrinterFooter() const noexcept
            {
            return m_rightPrinterFooter;
            }

        void SetDPIScaleFactor(const double scaling) noexcept { m_dpiScaling = scaling; }

        /// @brief Sets the watermark for the list when printed.
        /// @param watermark The watermark information.
        void SetWatermark(const Wisteria::Canvas::Watermark& watermark) noexcept
            {
            m_waterMark = watermark;
            }

      private:
        /// @returns The margin around the printing area.
        [[nodiscard]]
        wxCoord GetMarginPadding() const noexcept
            {
            return 10 * m_dpiScaling;
            }

        /// Gets the scaling factor going from the page size to the screen size
        /// This falls back to a 1:1 ratio upon failure
        void GetScreenToPageScaling(float& scaleX, float& scaleY) const
            {
            int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
            GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
            GetPPIScreen(&ppiScreenX, &ppiScreenY);

            scaleX = safe_divide<float>(ppiPrinterX, ppiScreenX);
            scaleY = safe_divide<float>(ppiPrinterY, ppiScreenY);
            if (scaleX == 0)
                {
                scaleX = 1;
                }
            if (scaleY == 0)
                {
                scaleY = 1;
                }
            }

        void GetPageToScreenScaling(float& scaleX, float& scaleY) const
            {
            int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
            GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
            GetPPIScreen(&ppiScreenX, &ppiScreenY);

            scaleX = safe_divide<float>(ppiScreenX, ppiPrinterX);
            scaleY = safe_divide<float>(ppiScreenY, ppiPrinterY);
            if (scaleX == 0)
                {
                scaleX = 1;
                }
            if (scaleY == 0)
                {
                scaleY = 1;
                }
            }

        [[nodiscard]]
        wxString ExpandPrintString(const wxString& printString) const
            {
            wxString expandedString = printString;

            expandedString.Replace(L"@PAGENUM@", std::to_wstring(m_currentPage), true);
            expandedString.Replace(L"@PAGESCNT@", std::to_wstring(GetPageCount()), true);
            const wxDateTime now = wxDateTime::Now();
            expandedString.Replace(L"@TITLE@", GetTitle(), true);
            expandedString.Replace(L"@DATE@", now.FormatDate(), true);
            expandedString.Replace(L"@TIME@", now.FormatTime(), true);

            return expandedString;
            }

        std::vector<wxString> m_htmlTables;
        // the first and last table in each page
        std::vector<std::pair<int, int>> m_pageStarts;
        size_t m_currentPage{ 0 };
        double m_dpiScaling{ 1 };

        Wisteria::Canvas::Watermark m_waterMark;

        wxBitmap m_printCanvas;

        // headers
        wxString m_leftPrinterHeader;
        wxString m_centerPrinterHeader;
        wxString m_rightPrinterHeader;
        // footers
        wxString m_leftPrinterFooter;
        wxString m_centerPrinterFooter;
        wxString m_rightPrinterFooter;
        };
    } // namespace Wisteria::UI

/** @}*/

#endif //__HTMLTABLEPRINTOUT_H__
