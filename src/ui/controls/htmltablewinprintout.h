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

#include <wx/wx.h>
#include "../../base/canvas.h"
#include "../../import/html_encode.h"

/// @private
class HtmlTablePrintout final : public wxPrintout
    {
public:
    HtmlTablePrintout(const wxString& title) : wxPrintout(title)
        {}
    void AddTable(const wxString& table)
        {
        std::wstring strippedTable{ table.wc_str() };
        lily_of_the_valley::html_format::strip_hyperlinks(strippedTable, false);
        m_htmlTables.push_back(strippedTable);
        }
    [[nodiscard]]
    bool HasPage(int pageNum) final
        { return (pageNum >= 1 && pageNum <= static_cast<int>(m_pageStarts.size())); }
    void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) final
        {
        *minPage = 1;
        *maxPage = static_cast<int>(m_pageStarts.size());
        *selPageFrom = 1;
        *selPageTo = static_cast<int>(m_pageStarts.size());
        }
    bool OnPrintPage(int page) final;
    void OnPreparePrinting() final;
    [[nodiscard]]
    size_t GetPageCount() const noexcept
        { return m_pageStarts.size(); }
    // printer header functions
    void SetLeftPrinterHeader(const wxString& header)
        { m_leftPrinterHeader = header; }
    [[nodiscard]]
    const wxString& GetLeftPrinterHeader() const noexcept
        { return m_leftPrinterHeader; }

    void SetCenterPrinterHeader(const wxString& header)
        { m_centerPrinterHeader = header; }
    [[nodiscard]]
    const wxString& GetCenterPrinterHeader() const noexcept
        { return m_centerPrinterHeader; }

    void SetRightPrinterHeader(const wxString& header)
        { m_rightPrinterHeader = header; }
    [[nodiscard]]
    const wxString& GetRightPrinterHeader() const noexcept
        { return m_rightPrinterHeader; }
    // printer footer functions
    void SetLeftPrinterFooter(const wxString& header)
        { m_leftPrinterFooter = header; }
    [[nodiscard]]
    const wxString& GetLeftPrinterFooter() const noexcept
        { return m_leftPrinterFooter; }

    void SetCenterPrinterFooter(const wxString& header)
        { m_centerPrinterFooter = header; }
    [[nodiscard]]
    const wxString& GetCenterPrinterFooter() const noexcept
        { return m_centerPrinterFooter; }

    void SetRightPrinterFooter(const wxString& header)
        { m_rightPrinterFooter = header; }
    [[nodiscard]]
    const wxString& GetRightPrinterFooter() const noexcept
        { return m_rightPrinterFooter; }

    void SetDPIScaleFactor(const double scaling) noexcept
        { m_dpiScaling = scaling; }

    /// @brief Sets the watermark for the list when printed.
    /// @param watermark The watermark information.
    void SetWatermark(const Wisteria::Canvas::Watermark& watermark) noexcept
        { m_waterMark = watermark; }
private:
    /// @returns The margin around the printing area.
    [[nodiscard]]
    wxCoord GetMarginPadding() const noexcept
        { return 10 * m_dpiScaling; }
    /// Gets the scaling factor going from the page size to the screen size
    /// This falls back to a 1:1 ratio upon failure
    void GetScreenToPageScaling(float& scaleX, float&scaleY) const
        {
        int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
        GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        GetPPIScreen(&ppiScreenX, &ppiScreenY);

        scaleX = safe_divide<float>(ppiPrinterX,ppiScreenX);
        scaleY = safe_divide<float>(ppiPrinterY,ppiScreenY);
        if (scaleX == 0)
            { scaleX = 1; }
        if (scaleY == 0)
            { scaleY = 1; }
        }
    void GetPageToScreenScaling(float& scaleX, float&scaleY) const
        {
        int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
        GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        GetPPIScreen(&ppiScreenX, &ppiScreenY);

        scaleX = safe_divide<float>(ppiScreenX, ppiPrinterX);
        scaleY = safe_divide<float>(ppiScreenY, ppiPrinterY);
        if (scaleX == 0)
            { scaleX = 1; }
        if (scaleY == 0)
            { scaleY = 1; }
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
    std::vector<std::pair<int,int>> m_pageStarts;
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

/** @}*/

#endif //__HTMLTABLEPRINTOUT_H__
