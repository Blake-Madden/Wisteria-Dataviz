///////////////////////////////////////////////////////////////////////////////
// Name:        formattedtextctrl.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include <wx/paper.h>
#ifdef __WXOSX__
    #include "wx/osx/private.h"
#endif
#include "formattedtextctrl.h"
#include "../dialogs/radioboxdlg.h"
#include "../../import/rtf_encode.h"
#include "../../import/html_encode.h"

using namespace Wisteria::UI;

wxIMPLEMENT_DYNAMIC_CLASS(FormattedTextCtrl, wxTextCtrl)

/** @brief Printing system for Windows and Linux.
    @note macOS's text control has its own printing mechanism that we
        patch into wxWidgets and use, so we don't have a dedicated printout
        interface for that platform.
    @todo Eventually Linux should have native printer interface too,
        not wxHTML printer interface which is slow.*/
#if defined(__WXMSW__)
class wxFormattedTextCtrlPrintout final : public wxPrintout
    {
public:
    /// @brief Constructor.
    /// @param control The FormattedTextCtrl to print.
    /// @param title The printout title.
    wxFormattedTextCtrlPrintout(FormattedTextCtrl* control, const wxString& title) :
        wxPrintout(title), m_control(control)
        {}
    /// @private
    ~wxFormattedTextCtrlPrintout()
        {
        // clean up from printing operation
        ::SendMessage(m_control->GetHWND(), EM_FORMATRANGE, FALSE, NULL);
        }
    /// @returns @c true if @c pageNum (one-index) is a valid page number.
    /// @param pageNum The page number to verify.
    [[nodiscard]]
    bool HasPage(int pageNum) final
        { return (pageNum >= 1 && pageNum <= static_cast<int>(m_pageStarts.size())); }
    /// @private
    void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) final
        {
        *minPage = 1;
        *maxPage = static_cast<int>(m_pageStarts.size());
        *selPageFrom = 1;
        *selPageTo = static_cast<int>(m_pageStarts.size());
        }
    /// @private
    bool OnPrintPage(int page) final
        {
        if (HasPage(page))
            {
            m_currentPage = page;
            PrintSection(m_pageStarts[page-1], true);
            return true;
            }
        else
            { return false; }
        }
    /// @private
    void OnPreparePrinting() final
        {
        m_pageStarts.clear();
        m_currentPage = 0;

        const wxTextPos lastCharacter = m_control->GetLastPosition();
        long index{ 0 };
        // always at least one page
        m_pageStarts.push_back(index);
        // calculate which characters start each page
        while (index < lastCharacter)
            {
            index = PrintSection(index, false);
            if (index >= lastCharacter)
                {
                // free the cache once we hit the last page
                ::SendMessage(m_control->GetHWND(), EM_FORMATRANGE, FALSE, NULL);
                break;
                }
            m_pageStarts.push_back(index);
            }
        }
    /// @returns The page count.
    [[nodiscard]]
    size_t GetPageCount() const noexcept
        { return m_pageStarts.size(); }
private:
    [[nodiscard]]
    wxString ExpandPrintString(const wxString& printString) const
        {
        wxString expandedString = printString;

        expandedString.Replace(L"@PAGENUM@", std::to_wstring(m_currentPage), true);
        expandedString.Replace(L"@PAGESCNT@", std::to_wstring(GetPageCount()), true);
        const wxDateTime now = wxDateTime::Now();
        expandedString.Replace(L"@TITLE@", m_control->GetTitleName(), true);
        expandedString.Replace(L"@DATE@", now.FormatDate(), true);
        expandedString.Replace(L"@TIME@", now.FormatTime(), true);

        return expandedString;
        }
    /// @brief Gets the scaling factor going from the page size to the screen size.
    /// @note This falls back to a 1:1 ratio upon failure.
    /// @param[out] scaleX The horizontal page scaling.
    /// @param[out] scaleY The vertical page scaling.
    void GetScreenToPageScaling(double& scaleX, double& scaleY) const
        {
        int ppiPrinterX, ppiPrinterY, ppiScreenX, ppiScreenY;
        GetPPIPrinter(&ppiPrinterX, &ppiPrinterY);
        GetPPIScreen(&ppiScreenX, &ppiScreenY);

        scaleX = safe_divide<double>(ppiPrinterX,ppiScreenX);
        scaleY = safe_divide<double>(ppiPrinterY,ppiScreenY);
        if (scaleX == 0)
            { scaleX = 1; }
        if (scaleY == 0)
            { scaleY = 1; }
        }
    /// @returns The margin around the printing area.
    [[nodiscard]]
    wxCoord GetMarginPadding() const
        { return 10*m_control->GetDPIScaleFactor(); }

    /// @prints a section of the control's text.
    /// @param charStart The starting position.
    /// @param renderPage @c true to both measure and render the page.
    ///     @c false to only measure it.
    /// @returns The return code.
    long PrintSection(long charStart, bool renderPage)
        {
        wxDC* dc = GetDC();
        if (dc)
            {
            dc->SetMapMode(wxMM_TEXT);

            double scaleX{ 0 }, scaleY{ 0 };
            GetScreenToPageScaling(scaleX, scaleY);

            // set a suitable scaling factor
            const double scaleXReciprical = safe_divide<double>(1.0f, scaleX);
            const double scaleYReciprical = safe_divide<double>(1.0f, scaleY);
            dc->SetUserScale(scaleX, scaleY);

            // get the size of the DC's drawing area in pixels
            int drawingWidth, drawingHeight;
            int dcWidth, dcHeight;
            dc->GetSize(&dcWidth, &dcHeight);
            dc->GetSize(&drawingWidth, &drawingHeight);
            drawingWidth *= scaleXReciprical;
            drawingHeight *= scaleYReciprical;

            // let's have at least 10 device units margin
            const auto marginX = GetMarginPadding();
            const auto marginY = GetMarginPadding();

            // remove the margins from the drawing area size
            drawingWidth -= 2*marginX;
            int topMargin = marginY;
            int bottomMargin = marginY;
            // remove space for the headers and footers (if being used)
            wxCoord textWidth{ 0 }, textHeight{ 0 };
            wxCoord bodyStart = marginY;
            dc->GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
            if (m_control->GetLeftPrinterHeader().length() ||
                m_control->GetCenterPrinterHeader().length() ||
                m_control->GetRightPrinterHeader().length())
                {
                topMargin += textHeight;
                bodyStart += textHeight+marginY;
                }
            if (m_control->GetLeftPrinterFooter().length() ||
                m_control->GetCenterPrinterFooter().length() ||
                m_control->GetRightPrinterFooter().length())
                { bottomMargin += textHeight; }
            drawingHeight -= (topMargin+bottomMargin);

            const auto mearsureAndDrawText = [this, charStart, renderPage](wxDC& dc)
                {
                // https://devblogs.microsoft.com/oldnewthing/20070112-02/?p=28423
                FORMATRANGE fr{ 0 };

                const auto HCD = dc.GetTempHDC();
                fr.hdcTarget = fr.hdc = HCD.GetHDC();

                wxCopyRectToRECT(m_control->GetPageRect(), fr.rcPage);
                wxCopyRectToRECT(m_control->GetPrintRect(), fr.rc);

                fr.chrg.cpMin = charStart;
                fr.chrg.cpMax = -1;

                // measure and/or render the formatted text
                const long retval = ::SendMessage(m_control->GetHWND(),
                                                  EM_FORMATRANGE, (WPARAM)renderPage, (LPARAM)&fr);

                return retval;
                };

            const auto drawHeadersAndFooters =
                [this, marginX, marginY, drawingWidth, drawingHeight,
                 topMargin, &textWidth, &textHeight]
                (wxDC& dc)
                {
                // draw the headers
                dc.SetDeviceOrigin(0, 0);
                if (m_control->GetLeftPrinterHeader().length() ||
                    m_control->GetCenterPrinterHeader().length() ||
                    m_control->GetRightPrinterHeader().length())
                    {
                    if (m_control->GetLeftPrinterHeader().length())
                        {
                        dc.DrawText(ExpandPrintString(m_control->GetLeftPrinterHeader()),
                            static_cast<wxCoord>(marginX),
                            static_cast<wxCoord>(marginY/2));
                        }
                    if (m_control->GetCenterPrinterHeader().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetCenterPrinterHeader()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetCenterPrinterHeader()),
                            static_cast<wxCoord>(safe_divide<double>(drawingWidth,2) -
                                                 safe_divide<double>(textWidth,2)),
                            static_cast<wxCoord>(marginY/2));
                        }
                    if (m_control->GetRightPrinterHeader().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetRightPrinterHeader()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetRightPrinterHeader()),
                            static_cast<wxCoord>(drawingWidth - (marginX+textWidth)),
                            static_cast<wxCoord>(marginY/2));
                        }
                    }
                // draw the footers
                if (m_control->GetLeftPrinterFooter().length() ||
                    m_control->GetCenterPrinterFooter().length() ||
                    m_control->GetRightPrinterFooter().length())
                    {
                    dc.GetTextExtent(L"MeasurementTestString", &textWidth, &textHeight);
                    // move down past the print header area, drawing (tables) area,
                    // and half the bottom margin (to center the footer vertically)
                    const wxCoord yPos = topMargin+drawingHeight + (marginY/2);
                    if (m_control->GetLeftPrinterFooter().length())
                        {
                        dc.DrawText(ExpandPrintString(m_control->GetLeftPrinterFooter()),
                            static_cast<wxCoord>(marginX),
                            yPos);
                        }
                    if (m_control->GetCenterPrinterFooter().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetCenterPrinterFooter()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetCenterPrinterFooter()),
                            static_cast<wxCoord>(safe_divide<double>(drawingWidth,2) -
                                                 safe_divide<double>(textWidth,2)),
                            yPos);
                        }
                    if (m_control->GetRightPrinterFooter().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetRightPrinterFooter()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetRightPrinterFooter()),
                            static_cast<wxCoord>((drawingWidth - (marginX+textWidth))),
                            yPos);
                        }
                    }
                };

            if (m_printCanvas.GetSize() != dc->GetSize())
                { m_printCanvas.Create(dc->GetSize()); }
            wxMemoryDC memDc(m_printCanvas);
            memDc.Clear();
            wxGCDC gcdc(memDc);

            // will just calculate pagination if not rendering
            const auto retval = mearsureAndDrawText(gcdc);
            if (renderPage)
                {
                drawHeadersAndFooters(gcdc);
                Wisteria::Canvas::DrawWatermarkLabel(gcdc, wxRect(wxSize(drawingWidth, drawingHeight)),
                                                     m_control->GetWatermark());
                // copy renderings back into printer DC
                dc->Blit(0, 0, dc->GetSize().GetWidth(), dc->GetSize().GetWidth(), &memDc, 0, 0);
                }
            memDc.SelectObject(wxNullBitmap);

            return retval;
            }
        else return -1;
        }
    std::vector<long> m_pageStarts;
    FormattedTextCtrl* m_control{ nullptr };
    FormattedTextCtrl* m_controlForPrinting{ nullptr };
    int m_currentPage{ 0 };
    wxBitmap m_printCanvas;
    };
#endif

//------------------------------------------------------
void FormattedTextCtrl::OnPrint([[maybe_unused]] wxCommandEvent& event)
    {
    /* On Windows and macOS, we copy RTF designed for a white
       background into another dummy text control and then use that
       control for printing. If our current control is dark themed,
       then that background will be printed (both on Windows and macOS),
       and obviouosly we don't want to print a black page.

       The workaround is to create a hidden text control, set its
       background to white, copy in RFT meant for a white background,
       and use that for printing.

       Note that Windows has a windowless RichEdit control via the
       ITextServices interface, but that API is a poorly documented
       COM interface that requires implementing a lot of boilerplate
       interface functionality. It's simpler to just create a
       hidden RichEdit control.*/
#if defined (__WXMSW__) || defined (__WXOSX__)
    if (m_printWindow == nullptr)
        { m_printWindow = new FormattedTextCtrl(this); }
    m_printWindow->Show(false);
    m_printWindow->SetBackgroundColour(*wxWHITE);
    m_printWindow->SetFormattedText(GetUnthemedFormattedText().length() ?
        GetUnthemedFormattedTextRtf(false) : GetFormattedTextRtf(false));
#endif

#if defined(__WXMSW__)
    wxFormattedTextCtrlPrintout* printOut =
        new wxFormattedTextCtrlPrintout(m_printWindow, GetTitleName());

    wxPrinterDC* dc = nullptr;

    if (m_printData)
        {
        dc = new wxPrinterDC(*m_printData);
        SetPrintOrientation(m_printData->GetOrientation());
        if (m_printData->GetPaperId() == wxPAPER_NONE)
            {
            SetPaperSizeInMillimeters(m_printData->GetPaperSize());
            }
        else
            {
            /* values in here are hard coded, so a little more precise than
               converting from millimeters to twips*/
            SetPaperSize(m_printData->GetPaperId());
            }
        }
    else
        {
        wxPrintData pd;
        dc = new wxPrinterDC(pd);
        }
    CopyPrintSettings(m_printWindow);
    printOut->SetDC(dc);

    wxPrinter printer;
    if (m_printData)
        {
        printer.GetPrintDialogData().SetPrintData(*m_printData);
        }
    if (!printer.Print(this, printOut, true) )
        {
        // just show a message if a real error occurred.
        // They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK|wxICON_QUESTION);
            }
        }
    if (m_printData)
        {
        *m_printData = printer.GetPrintDialogData().GetPrintData();
        }
    wxDELETE(dc);
    wxDELETE(printOut);
#elif defined(__WXGTK__)
    // format the header
    wxString headerText;
    if (GetLeftPrinterHeader().length() ||
        GetCenterPrinterHeader().length() ||
        GetRightPrinterHeader().length())
        {
        headerText = L"<table style=\"width:100%;\"><tr><td width=\"33%\">" +
            GetLeftPrinterHeader() + L"</td>";
        headerText += L"<td width=\"33%\" align=\"center\">" +
            GetCenterPrinterHeader() + L"</td>";
        headerText += L"<td width=\"33%\" align=\"right\">" +
            GetRightPrinterHeader() + L"</td></tr></table>";
        }
    //format the footer
    wxString footerText;
    if (GetLeftPrinterFooter().length() ||
        GetCenterPrinterFooter().length() ||
        GetRightPrinterFooter().length())
        {
        footerText = "<table style=\"width:100%;\"><tr><td width=\"33%\">" +
            GetLeftPrinterFooter() + L"</td>";
        footerText += "<td width=\"33%\" align=\"center\">" +
            GetCenterPrinterFooter() + L"</td>";
        footerText += "<td width=\"33%\" align=\"right\">" +
            GetRightPrinterFooter() + "</td></tr></table>";
        }

    wxString outputText = GetFormattedTextHtml();
    wxHtmlPrintout* printOut = new wxHtmlPrintout(GetTitleName());
    printOut->SetHtmlText(outputText);
    if (headerText.length())
        { printOut->SetHeader(headerText); }
    if (footerText.length())
        { printOut->SetFooter(footerText); }

    wxPrinter printer;
    if (m_printData)
        {
        printer.GetPrintDialogData().SetPrintData(*m_printData);
        }
    if (!printer.Print(this, printOut, true) )
        {
        // just show a message if a real error occurred.
        // They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                        _(L"Print"), wxOK|wxICON_QUESTION);
            }
        }
    if (m_printData)
        {
        *m_printData = printer.GetPrintDialogData().GetPrintData();
        }
    wxDELETE(dc);
    wxDELETE(printOut);
#else
    const wxSize paperSize = wxThePrintPaperDatabase->GetSize(m_printData->GetPaperId());
    const double PaperWidthInInches = (paperSize.GetWidth()/10) * 0.0393700787;
    const double PaperHeightInInches = (paperSize.GetHeight()/10) * 0.0393700787;

    wxClientDC dc(this);
    wxFont fixedFont(12, wxFontFamily::wxFONTFAMILY_MODERN,
                     wxFontStyle::wxFONTSTYLE_NORMAL, wxFontWeight::wxFONTWEIGHT_NORMAL,
                     false, L"Courier New" );
    dc.SetFont(fixedFont);
    wxCoord textWidth, textHeight;
    dc.GetTextExtent(L" ", &textWidth, &textHeight);
    const size_t spacesCount = (m_printData->GetOrientation() == wxPORTRAIT)?
        safe_divide<size_t>((PaperWidthInInches- .5f) * 72, textWidth) :
        safe_divide<size_t>((PaperHeightInInches- .5f) * 72, textWidth);

    // format the header
    wxString expandedLeftHeader = ExpandMacPrintString(GetLeftPrinterHeader());
    wxString expandedCenterHeader = ExpandMacPrintString(GetCenterPrinterHeader());
    wxString expandedRightHeader = ExpandMacPrintString(GetRightPrinterHeader());

    wxString fullHeader = expandedLeftHeader;
    if (spacesCount >=
        (expandedLeftHeader.length() + expandedCenterHeader.length() + expandedRightHeader.length()))
        {
        const size_t paddingSize = spacesCount -
            (expandedLeftHeader.length() + expandedCenterHeader.length() + expandedRightHeader.length());
        fullHeader.Pad(paddingSize/2);
        fullHeader += expandedCenterHeader;
        fullHeader.Pad(paddingSize/2+(is_even(paddingSize) ? 0 : 1));
        fullHeader += expandedRightHeader;
        }
    else
        {
        fullHeader += expandedCenterHeader;
        fullHeader += expandedRightHeader;
        }

    // format the footer
    wxString expandedLeftFooter = ExpandMacPrintString(GetLeftPrinterFooter());
    wxString expandedCenterFooter = ExpandMacPrintString(GetCenterPrinterFooter());
    wxString expandedRightFooter = ExpandMacPrintString(GetRightPrinterFooter());

    wxString fullFooter = expandedLeftFooter;
    if (spacesCount >=
        (expandedLeftFooter.length() + expandedCenterFooter.length() + expandedRightFooter.length()))
        {
        const size_t paddingSize = spacesCount -
            (expandedLeftFooter.length() + expandedCenterFooter.length() + expandedRightFooter.length());
        fullFooter.Pad(paddingSize/2);
        fullFooter += expandedCenterFooter;
        fullFooter.Pad(paddingSize/2 + (is_even(paddingSize) ? 0 : 1));
        fullFooter += expandedRightFooter;
        }
    else
        {
        fullFooter += expandedCenterFooter;
        fullFooter += expandedRightFooter;
        }
    /// @todo test this
    m_printWindow->GetTextPeer()->Print(wxSize(PaperWidthInInches*72, PaperHeightInInches*72),
        static_cast<int>(m_printData->GetOrientation()), fullHeader, fullFooter);
#endif
    }

//------------------------------------------------------
void FormattedTextCtrl::OnPreview([[maybe_unused]] wxCommandEvent& event)
    {
    // note that previewing isn't done on macOS as it has its own native previewing
    // built into its print dialog
#if defined(__WXMSW__)
    if (m_printWindow == nullptr)
        { m_printWindow = new FormattedTextCtrl(this); }
    m_printWindow->Show(false);
    m_printWindow->SetBackgroundColour(*wxWHITE);
    m_printWindow->SetFormattedText(GetUnthemedFormattedText().length() ?
        GetUnthemedFormattedTextRtf(false) : GetFormattedTextRtf(false));
    wxFormattedTextCtrlPrintout* printOut =
        new wxFormattedTextCtrlPrintout(m_printWindow, GetTitleName());
    wxFormattedTextCtrlPrintout* printOutForPrinting =
        new wxFormattedTextCtrlPrintout(m_printWindow, GetTitleName());

    wxPrinterDC* dc = nullptr;
    wxPrinterDC* dc2 = nullptr;

    if (m_printData)
        {
        dc = new wxPrinterDC(*m_printData);
        dc2 = new wxPrinterDC(*m_printData);

        SetPrintOrientation(m_printData->GetOrientation());
        if (m_printData->GetPaperId() == wxPAPER_NONE)
            {
            SetPaperSizeInMillimeters(m_printData->GetPaperSize());
            }
        else
            {
            /* values in here are hard coded, so a little more precise than
               converting from millimeters to twips*/
            SetPaperSize(m_printData->GetPaperId());
            }
        }
    else
        {
        wxPrintData pd;
        dc = new wxPrinterDC(pd);
        dc2 = new wxPrinterDC(pd);
        }
    CopyPrintSettings(m_printWindow);
    printOut->SetDC(dc);
    printOutForPrinting->SetDC(dc2);

    wxPrintPreview* preview = new wxPrintPreview(printOut, printOutForPrinting, m_printData);

    if (!preview->Ok())
        {
        wxDELETE(preview); wxDELETE(dc); wxDELETE(dc2);
        wxMessageBox(_(L"An error occurred while previewing.\n"
                       "Your default printer may not be set correctly."),
                     _(L"Print Preview"), wxOK|wxICON_QUESTION);
        return;
        }
    int x, y, width, height;
    wxClientDisplayRect(&x, &y, &width, &height);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                                wxDefaultPosition, wxSize(width, height));

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);

    wxDELETE(dc); wxDELETE(dc2);
#elif defined(__WXGTK__)
    // format the header
    wxString headerText;
    if (GetLeftPrinterHeader().length() ||
        GetCenterPrinterHeader().length() ||
        GetRightPrinterHeader().length())
        {
        headerText = L"<table style=\"width:100%;\"><tr><td width=\"33%\">" +
            GetLeftPrinterHeader() + L"</td>";
        headerText += L"<td width=\"33%\" align=\"center\">" +
            GetCenterPrinterHeader() + L"</td>";
        headerText += L"<td width=\"33%\" align=\"right\">" +
            GetRightPrinterHeader() + L"</td></tr></table>";
        }
    // format the footer
    wxString footerText;
    if (GetLeftPrinterFooter().length() ||
        GetCenterPrinterFooter().length() ||
        GetRightPrinterFooter().length())
        {
        footerText = L"<table style=\"width:100%;\"><tr><td width=\"33%\">" +
            GetLeftPrinterFooter() + L"</td>";
        footerText += L"<td width=\"33%\" align=\"center\">" +
            GetCenterPrinterFooter() + L"</td>";
        footerText += L"<td width=\"33%\" align=\"right\">" +
            GetRightPrinterFooter() + L"</td></tr></table>";
        }

    wxString outputText;
    GetFormattedTextHtml(outputText);
    wxHtmlPrintout* printOut = new wxHtmlPrintout(GetTitleName());
    wxHtmlPrintout* printOutForPrinting = new wxHtmlPrintout(GetTitleName());
    printOut->SetHtmlText(outputText);
    printOutForPrinting->SetHtmlText(outputText);
    if (headerText.length())
        {
        printOut->SetHeader(headerText);
        printOutForPrinting->SetHeader(headerText);
        }
    if (footerText.length())
        {
        printOut->SetFooter(footerText);
        printOutForPrinting->SetFooter(footerText);
        }
    wxPrintPreview* preview = new wxPrintPreview(printOut, printOutForPrinting, m_printData);
    if (!preview->Ok())
        {
        wxDELETE(preview);
        wxMessageBox(_(L"An error occurred while previewing.\n"
                       "Your default printer may not be set correctly."),
                     _(L"Print Preview"), wxOK|wxICON_QUESTION);
        return;
        }
    int x, y, width, height;
    wxClientDisplayRect(&x, &y, &width, &height);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                                wxDefaultPosition, wxSize(width, height));

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);
#endif
    }

//------------------------------------------------------
void FormattedTextCtrl::SetPaperSize(const wxPaperSize size)
    {
    switch (size)
        {
        case wxPAPER_A_PLUS:
            SetPaperSizeInInches(8.937, 14.015);
            break;
        case wxPAPER_A2:
            SetPaperSizeInInches(16.535, 23.385);
            break;
        case wxPAPER_A3:
            [[fallthrough]];
        case wxPAPER_A3_TRANSVERSE:
            SetPaperSizeInInches(11.692, 16.53);
            break;
        case wxPAPER_A3_EXTRA_TRANSVERSE:
            SetPaperSizeInInches(12.677, 17.519);
            break;
        case wxPAPER_A3_EXTRA:
            SetPaperSizeInInches(12.677, 17.519);
            break;
        // most A4's are the same
        case wxPAPER_A4:
            [[fallthrough]];
        case wxPAPER_A4_TRANSVERSE:
            [[fallthrough]];
        case wxPAPER_A4SMALL:
            SetPaperSizeInInches(8.267, 11.692);
            break;
        case wxPAPER_A4_PLUS:
            SetPaperSizeInInches(8.267, 12.992);
            break;
        case wxPAPER_A5:
            [[fallthrough]];
        case wxPAPER_A5_TRANSVERSE:
            SetPaperSizeInInches(5.826, 8.267);
            break;
        case wxPAPER_A5_EXTRA:
            SetPaperSizeInInches(6.850, 9.251);
            break;
        case  wxPAPER_B_PLUS:
            SetPaperSizeInInches(12.007, 19.173);
            break;
        case wxPAPER_B4:
            SetPaperSizeInInches(9.842, 13.937);
            break;
        case wxPAPER_B5:
            [[fallthrough]];
        case wxPAPER_B5_TRANSVERSE:
            SetPaperSizeInInches(7.165, 10.118);
            break;
        case wxPAPER_B5_EXTRA:
            SetPaperSizeInInches(7.913, 10.866);
            break;
        case wxPAPER_CSHEET:
            SetPaperSizeInInches(17, 22);
            break;
        case wxPAPER_DSHEET:
            SetPaperSizeInInches(22, 34);
            break;
        case wxPAPER_ESHEET:
            SetPaperSizeInInches(34, 44);
            break;
        case wxPAPER_TABLOID:
            SetPaperSizeInInches(11, 17);
            break;
        case wxPAPER_LEDGER:
            SetPaperSizeInInches(17, 11);
            break;
        case wxPAPER_STATEMENT:
            SetPaperSizeInInches(5.5, 8.5);
            break;
        case wxPAPER_EXECUTIVE:
            SetPaperSizeInInches(7.25, 10.5);
            break;
        case wxPAPER_FOLIO:
            SetPaperSizeInInches(8.5, 13);
            break;
        case wxPAPER_9X11:
            SetPaperSizeInInches(9, 11);
            break;
        case wxPAPER_10X11:
            SetPaperSizeInInches(10, 11);
            break;
        case wxPAPER_10X14:
            SetPaperSizeInInches(10, 14);
            break;
        case wxPAPER_11X17:
            SetPaperSizeInInches(11, 17);
            break;
        case wxPAPER_15X11:
            SetPaperSizeInInches(15, 11);
            break;
        case wxPAPER_LEGAL:
            SetPaperSizeInInches(8.5, 14);
            break;
        case wxPAPER_LETTER_EXTRA:
            SetPaperSizeInInches(9.275, 12);
            break;
        case wxPAPER_LEGAL_EXTRA:
            SetPaperSizeInInches(9.275, 15);
            break;
        case wxPAPER_TABLOID_EXTRA:
            SetPaperSizeInInches(11.69, 18);
            break;
        case wxPAPER_A4_EXTRA:
            SetPaperSizeInInches(9.27, 12.69);
            break;
        case wxPAPER_LETTER_TRANSVERSE:
            SetPaperSizeInInches(8.275, 11);
            break;
        case wxPAPER_LETTER_EXTRA_TRANSVERSE:
            SetPaperSizeInInches(9.275, 12);
            break;
        case wxPAPER_LETTER_PLUS:
            SetPaperSizeInInches(8.5, 12.69);
            break;
        case wxPAPER_QUARTO:
            SetPaperSizeInInches(8.464, 10.826);
            break;
        case wxPAPER_ISO_B4:
            SetPaperSizeInInches(9.842, 13.897);
            break;
        case wxPAPER_JAPANESE_POSTCARD:
            SetPaperSizeInInches(3.937, 5.826);
            break;
        // envelopes
        case wxPAPER_ENV_DL:
            SetPaperSizeInInches(4.330, 8.661);
            break;
        case wxPAPER_ENV_B4:
            SetPaperSizeInInches(9.842, 13.897);
            break;
        case wxPAPER_ENV_B5:
            SetPaperSizeInInches(6.929, 9.842);
            break;
        case wxPAPER_ENV_B6:
            SetPaperSizeInInches(6.929, 4.921);
            break;
        case wxPAPER_ENV_C3:
            SetPaperSizeInInches(12.755, 18.031);
            break;
        case wxPAPER_ENV_C4:
            SetPaperSizeInInches(9.015, 12.755);
            break;
        case wxPAPER_ENV_C5:
            SetPaperSizeInInches(6.377, 9.015);
            break;
        case wxPAPER_ENV_C6:
            SetPaperSizeInInches(4.488, 6.377);
            break;
        case wxPAPER_ENV_C65:
            SetPaperSizeInInches(4.488, 9.015);
            break;
        case wxPAPER_ENV_ITALY:
            SetPaperSizeInInches(4.330, 9.055);
            break;
        case wxPAPER_ENV_INVITE:
            SetPaperSizeInInches(8.661, 8.661);
            break;
        case wxPAPER_ENV_9:
            SetPaperSizeInInches(3.875, 8.875);
            break;
        case wxPAPER_ENV_10:
            SetPaperSizeInInches(4.125, 9.5);
            break;
        case wxPAPER_ENV_11:
            SetPaperSizeInInches(4.5, 10.375);
            break;
        case wxPAPER_ENV_12:
            SetPaperSizeInInches(4.75, 11);
            break;
        case wxPAPER_ENV_14:
            SetPaperSizeInInches(5, 11.5);
            break;
        case wxPAPER_ENV_MONARCH:
            SetPaperSizeInInches(3.875, 7.5);
            break;
        case wxPAPER_ENV_PERSONAL:
            SetPaperSizeInInches(3.625, 6.5);
            break;
        // fan folds
        case wxPAPER_FANFOLD_US:
            SetPaperSizeInInches(14.875, 11);
            break;
        case wxPAPER_FANFOLD_STD_GERMAN:
            SetPaperSizeInInches(8.5, 12);
            break;
        case wxPAPER_FANFOLD_LGL_GERMAN:
            SetPaperSizeInInches(8.5, 13);
            break;
        case wxPAPER_LETTER:
            [[fallthrough]];
        case wxPAPER_LETTERSMALL:
            [[fallthrough]];
        case wxPAPER_NOTE:
            [[fallthrough]];
        default:
            SetPaperSizeInInches(8.5, 11);
        }
    }

//------------------------------------------------------
void FormattedTextCtrl::OnSave([[maybe_unused]] wxCommandEvent& event)
    {
    wxArrayString choices, descriptions;
    choices.Add(_DT(L"HTML"));
    descriptions.Add(_DT(L"<span style='font-weight:bold;'>Hyper Text Markup Language</span><br />") +
        _(L"This format can be displayed in Internet browsers and most word-processing programs."));
    choices.Add(_DT(L"RTF"));
    descriptions.Add(_DT(L"<span style='font-weight:bold;'>Rich Text Format</span><br />") +
        _(L"This format can be displayed in most word-processing programs."));
    RadioBoxDlg exportTypesDlg(this,
        _(L"Select Document Format"), wxString{}, _(L"Document formats:"), _(L"Export Document"),
        choices, descriptions);
    if (exportTypesDlg.ShowModal() != wxID_OK)
        { return; }
    wxString fileFilter;
    switch (exportTypesDlg.GetSelection())
        {
    case 0:
        fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
        break;
    case 1:
        fileFilter = _DT(L"Rich Text Format (*.rtf)|*.rtf");
        break;
    default:
        fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
        };
    wxFileDialog dialog
            (this, _(L"Save As"), wxString{}, GetTitleName(), fileFilter,
            wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK)
        { return; }

    wxBusyCursor wait;
    wxFileName filePath = dialog.GetPath();
    // in case the extension is missing then use the selected filter
    if (filePath.GetExt().IsEmpty())
        {
        switch (exportTypesDlg.GetSelection())
            {
        case 0:
            filePath.SetExt(L"htm");
            break;
        case 1:
            filePath.SetExt(L"rtf");
            break;
        default:
            filePath.SetExt(L"htm");
            };
        }

    if (filePath.GetExt().CmpNoCase(L"rtf") == 0)
        { SaveAsRtf(filePath); }
    else if (filePath.GetExt().CmpNoCase(L"htm") == 0 ||
             filePath.GetExt().CmpNoCase(L"html") == 0)
        { SaveAsHtml(filePath); }
    }

//------------------------------------------------------
bool FormattedTextCtrl::Save(const wxFileName& path) const
    {
    // create the folder to the filepath, if necessary
    wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    if (path.GetExt().CmpNoCase(L"RTF") == 0)
        { return SaveAsRtf(path); }
    else
        { return SaveAsHtml(path); }
    }

//------------------------------------------------------
bool FormattedTextCtrl::SaveAsHtml(const wxFileName& path) const
    {
    wxString htmlBody = GetFormattedTextHtml();

    wxString htmlText = L"<!DOCTYPE html>\n<html>" +
                        htmlBody + L"\n</html>";

    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    const bool retVal = file.Write(htmlText);
    if (!retVal)
        {
        wxMessageBox(
            wxString::Format(_(L"Failed to save document\n(%s)."), path.GetFullPath()),
            _(L"Error"), wxOK|wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
bool FormattedTextCtrl::SaveAsRtf(const wxFileName& path) const
    {
    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    // export unthemed text (if available)
    const bool retVal = file.Write((GetUnthemedFormattedText().length() ?
        GetUnthemedFormattedTextRtf() : GetFormattedTextRtf()));
    if (!retVal)
        {
        wxMessageBox(
            wxString::Format(_(L"Failed to save document (%s)."), path.GetFullPath()),
            _(L"Error"), wxOK|wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
void FormattedTextCtrl::OnFind(wxFindDialogEvent &myEvent)
    {
    //if they were just hitting Cancel then close
    if (myEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE)
        { return; }
    if (wxNOT_FOUND == FindText(myEvent.GetFindString(),
                    int_to_bool(myEvent.GetFlags() & wxFR_DOWN),
                    int_to_bool(myEvent.GetFlags() & wxFR_WHOLEWORD),
                    int_to_bool(myEvent.GetFlags() & wxFR_MATCHCASE) ))
        {
        wxMessageDialog(this,
            (myEvent.GetEventType() == wxEVT_COMMAND_FIND_NEXT) ?
            _(L"No further occurrences found.") : _(L"The text could not be found."),
            _(L"Text Not Found")).ShowModal();
        }
    }

//-----------------------------------------------------------
void FormattedTextCtrl::OnContextMenu([[maybe_unused]] wxContextMenuEvent& event)
    {
    if (wxGetMouseState().ShiftDown())
        {
        wxMessageBox(
            wxString::Format(_DT(L"Cursor postion: %d", DTExplanation::DebugMessage),
                GetInsertionPoint()), _DT(L"Position"), wxOK);
        }
    if (m_menu)
        { PopupMenu(m_menu); }
    }

//------------------------------------------------------
void FormattedTextCtrl::OnSelectAll([[maybe_unused]] wxCommandEvent& event )
    { SelectAll(); }

//------------------------------------------------------
void FormattedTextCtrl::OnCopyAll([[maybe_unused]] wxCommandEvent& event )
    {
    wxString FormattedText;
#if defined (__WXMSW__) || defined (__WXOSX__)
    FormattedText = GetUnthemedFormattedText().length() ?
        GetUnthemedFormattedTextRtf() : GetFormattedTextRtf();
    if (wxTheClipboard->Open())
        {
        if (FormattedText.length() )
            {
            wxTheClipboard->Clear();
            wxDataObjectComposite* obj = new wxDataObjectComposite();
            obj->Add(new wxRtfDataObject(FormattedText), true);
            obj->Add(new wxTextDataObject(GetValue()) );
            wxTheClipboard->AddData(obj);
            }
        wxTheClipboard->Close();
        }
#elif defined(__WXGTK__)
    FormattedText = GetFormattedTextHtml();
    if (wxTheClipboard->Open())
        {
        if (FormattedText.length() )
            {
            wxTheClipboard->Clear();
            wxDataObjectComposite* obj = new wxDataObjectComposite();
            obj->Add(new wxHTMLDataObject(FormattedText), true);
            obj->Add(new wxTextDataObject(GetValue()) );
            wxTheClipboard->AddData(obj);
            }
        wxTheClipboard->Close();
        }
#endif
    }

//-----------------------------------------------------------
long FormattedTextCtrl::FindText(const wchar_t* textToFind, const bool searchDown,
        const bool matchWholeWord, const bool caseSensitiveSearch)
    {
#ifdef __WXMSW__
    // set up the flags
    unsigned int flags = 0;
    if (searchDown)
        flags |= FR_DOWN;
    if (matchWholeWord)
        flags |= FR_WHOLEWORD;
    if (caseSensitiveSearch)
        flags |= FR_MATCHCASE;

    FINDTEXTW findText;
    SendMessage(GetHwnd(), EM_EXGETSEL, 0, (LPARAM)&findText.chrg);
    const long startOfSelection = findText.chrg.cpMin;
    if (searchDown)
        {
        // begin search from end of selection
        findText.chrg.cpMin = findText.chrg.cpMax;
        findText.chrg.cpMax = -1;
        }
    else
        {
        // if at the beginning of the window then we can't search up
        if (findText.chrg.cpMin == 0)
            { return wxNOT_FOUND; }
        findText.chrg.cpMax = 0;
        }

    findText.lpstrText = textToFind;
    long retval = SendMessage(GetHwnd(), EM_FINDTEXTW, flags, (LPARAM)&findText);
    if (retval != wxNOT_FOUND)
        { SetSelection(retval, retval + static_cast<long>(wxStrlen(textToFind))); }
    // if not found and searching down, ask if they would like to start
    // from the beginning and try again
    if ((retval == wxNOT_FOUND) &&
        searchDown &&
        (startOfSelection > 0) &&
        (wxMessageBox(_(L"Search has reached the end of the document. "
                        "Do you wish to restart the search from the beginning?"),
                _(L"Continue Search"), wxYES_NO|wxICON_QUESTION) == wxYES))
        {
        findText.chrg.cpMin = 0;
        findText.chrg.cpMax = startOfSelection;
        retval = SendMessage(GetHwnd(), EM_FINDTEXTW, flags, (LPARAM)&findText);
        if (retval != wxNOT_FOUND)
            { SetSelection(retval, retval + static_cast<long>(wxStrlen(textToFind))); }
        }
    return retval;
#elif defined(__WXGTK__)
    GtkTextSearchFlags flags = GTK_TEXT_SEARCH_TEXT_ONLY;

    GtkWidget* text_view = gtk_bin_get_child(GTK_BIN(GTK_SCROLLED_WINDOW(GetHandle())));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view) );

    // get the beginning and end of text buffer
    GtkTextIter textStart, textEnd;
    gtk_text_buffer_get_start_iter(buffer, &textStart);
    gtk_text_buffer_get_end_iter(buffer, &textEnd);

    // get the selection in the text buffer
    GtkTextIter selectionStart, selectionEnd;
    gtk_text_buffer_get_selection_bounds(buffer, &selectionStart, &selectionEnd);

    gboolean found = false;
    if (searchDown)
        {
        //search forward, beginning at the end of the selected text
        const GtkTextIter selStart = selectionStart;
        const GtkTextIter selEnd = selectionEnd;
        found = gtk_text_iter_forward_search_ex(&selEnd,
            wxConvUTF8.cWC2MB(textToFind),
            flags, caseSensitiveSearch, matchWholeWord, &selectionStart, &selectionEnd, nullptr);
        // if not found and searching down, ask if they would like to start
        // from the beginning and try again.
        /// @bug When this wraps around, it finds the word but doesn't highlight it.
        /// You have to hit F3 to highlight it.
        if (!found &&
            (wxMessageBox(_(L"Search has reached the end of the document. "
                            "Do you wish to restart the search from the beginning?"),
                _(L"Continue Search"), wxYES_NO|wxICON_QUESTION) == wxYES))
            {
            gtk_text_buffer_get_start_iter(buffer, &textStart);
            gtk_text_buffer_get_selection_bounds(buffer, &selectionStart, &selectionEnd);
            // does this fix it? If not then give up.
            const GtkTextIter selStart2 = selectionStart;
            found = gtk_text_iter_forward_search_ex(&textStart,
                wxConvUTF8.cWC2MB(textToFind),
                flags, caseSensitiveSearch, matchWholeWord, &selectionStart, &selectionEnd, &selStart2);
            }
        }
    else
        {
        // search backwards, starting at the start of the selected text
        const GtkTextIter selStart = selectionStart;
        found = gtk_text_iter_backward_search_ex(&selStart,
            wxConvUTF8.cWC2MB(textToFind),
            flags, caseSensitiveSearch, matchWholeWord, &selectionStart, &selectionEnd, nullptr);
        }
    if (found)
        {
        // if found, then highlight the text
        gtk_text_buffer_select_range(buffer, &selectionStart, &selectionEnd);
        gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(text_view), &selectionStart,
                                     0.0, FALSE, 0.5, 0.5);
        return gtk_text_iter_get_offset(&selectionStart);
        }
    else
        { return wxNOT_FOUND; }
#else
    long location = GetTextPeer()->Find(textToFind, caseSensitiveSearch, searchDown, matchWholeWord);
    if (location != wxNOT_FOUND)
        { SetSelection(location, location + static_cast<long>(wxStrlen(textToFind))); }
    // if not found and searching down, ask if they would like to start
    // from the beginning and try again
    long startOfSelection, endOfSelection;
    GetSelection(&startOfSelection, &endOfSelection);
    if ((location == wxNOT_FOUND) &&
        searchDown &&
        (startOfSelection > 0) &&
        (wxMessageBox(_(L"Search has reached the end of the document. "
                        "Do you wish to restart the search from the beginning?"),
                      _(L"Continue Search"), wxYES_NO|wxICON_QUESTION) == wxYES))
        {
        SetSelection(0, 0);
        location = GetTextPeer()->Find(textToFind, caseSensitiveSearch, searchDown, matchWholeWord);
        if (location != wxNOT_FOUND)
            { SetSelection(location, location + static_cast<long>(wxStrlen(textToFind))); }
        else
            { SetSelection(startOfSelection, endOfSelection); }
        }
    return location;
#endif
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::GetFormattedTextHtml(
    const wxString& CssStylePrefix /*= wxString{}*/) const
    {
#if defined (__WXMSW__) || defined (__WXOSX__)
    wxString rtfText = GetUnthemedFormattedText().length() ?
        GetUnthemedFormattedTextRtf(false) : GetFormattedTextRtf(false);

    lily_of_the_valley::rtf_extract_text filter_rtf(
        lily_of_the_valley::rtf_extract_text::rtf_extraction_type::rtf_to_html);
    filter_rtf.set_style_prefix(CssStylePrefix.wc_str());
    wxCharBuffer buf = rtfText.mb_str();
    wxASSERT_LEVEL_2(buf.length() == std::strlen(buf.data()));
    const wchar_t* htmlBody = filter_rtf(buf.data(), buf.length());
    if (!htmlBody)
        { return wxString{}; }
    wxString fontString(filter_rtf.get_font().c_str(), wxConvLibc);
    wxString text = wxString::Format(
        L"\n<head>"
         "\n<meta http-equiv='content-type' content='text/html; charset=UTF-8' />"
         "\n<title>%s</title>"
         "\n<style type='text/css'>\n<!--\n%s\n-->\n</style>\n</head>"
         "\n<body>\n<p style='font-family: %s; font-size: %dpt; color: rgb(%u, %u, %u)'>",
        GetTitleName(),
        filter_rtf.get_style_section().c_str(),
        fontString, filter_rtf.get_font_size(),
        filter_rtf.get_font_color().red,
        filter_rtf.get_font_color().green,
        filter_rtf.get_font_color().blue);

    // step over any leading line breaks
    while (std::wcsncmp(htmlBody, L"<br />", 6) == 0)
        { htmlBody += 6; }
    text += htmlBody;
    text += L"\n</p>\n</body>";
    return text;
#elif defined(__WXGTK__)
    GetFormattedTextGtk(text, HtmlFormat);
#endif
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::GetUnthemedFormattedTextRtf(
    const bool fixHighlightingTags /*= true*/) const
    {
#if defined (__WXMSW__) || defined (__WXOSX__)
    return fixHighlightingTags ?
        FixHighlightingTags(GetUnthemedFormattedText()) :
        GetUnthemedFormattedText();
#elif defined(__WXGTK__)
    ???
#endif
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::GetFormattedTextRtf(const bool fixHighlightingTags /*= true*/) const
    {
    wxString text;
#ifdef __WXMSW__
    /* If SetValue was called instead of SetFormattedText
       then liberally assume that the underlying RTF is twice as long.
       Otherwise, make it a little bigger than the original RTF length because
       we need to insert some extra tags.*/
    text.reserve((m_rtfLength > 0) ? m_rtfLength*1.5 : GetValue().length()*2);
    // the Rich Edit control must write to a char buffer,
    // so this won't work with unicode wxStrings
    std::string buffer;
    buffer.reserve((m_rtfLength > 0) ? m_rtfLength*1.5 : GetValue().length()*2);

    // EM_GETTEXTEX simply returns plain text, so stream out to get the RTF instead
    EDITSTREAM es{0};
    es.dwError = 0;
    es.pfnCallback = FormattedTextCtrl::EditStreamOutCallback;
    // our EditStreamOutCallback() expects a std::string to write
    // to (dwCookie is function specific)
    es.dwCookie = reinterpret_cast<DWORD_PTR>(&buffer);

    ::SendMessage(GetHwnd(), EM_STREAMOUT, (WPARAM)SF_RTF|SFF_PLAINRTF, (LPARAM)&es);
    text = wxString(buffer.c_str(), *wxConvCurrent);
    buffer.clear();
#elif defined(__WXGTK__)
    wxUnusedVar(fixHighlightingTags);
    GetFormattedTextGtk(text, RtfFormat);
#else
    text = GetTextPeer()->GetRtfValue();
#endif
    if (fixHighlightingTags)
        { text = FixHighlightingTags(text); }

    return text;
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::FixHighlightingTags(const wxString& text)
    {
#ifdef __WXMSW__
    const wxString BACKGROUND_COLOR_TAG = L"\\chcbpat";
    const wxString HIGHLIGHT_TAG = L"\\highlight";
    wxString highlightNumber;
    wxString correctedText; correctedText.reserve(text.length() * 1.5);
    // add "chcbpat" to each "highlight"
    long previousPos = 0;
    long highlightTag = text.find(HIGHLIGHT_TAG);
    while (highlightTag != wxNOT_FOUND)
        {
        correctedText.Append(text.substr(previousPos,
                            (highlightTag+HIGHLIGHT_TAG.length()) - previousPos));
        previousPos = (highlightTag+HIGHLIGHT_TAG.length());
        // verify that it's a tag and not actually the word "\highlight"
        if (highlightTag == 0 ||
            (highlightTag > 0 && text[highlightTag-1] != L'\\') )
            {
            const long nextSpace = text.find_first_of(L" \n\r\t\\",
                                        highlightTag+HIGHLIGHT_TAG.length());
            if (nextSpace != wxNOT_FOUND)
                {
                highlightNumber = text.substr(highlightTag+HIGHLIGHT_TAG.length(),
                                              (nextSpace-(highlightTag+HIGHLIGHT_TAG.length())) );
                correctedText.Append(highlightNumber+BACKGROUND_COLOR_TAG+highlightNumber);
                previousPos += highlightNumber.length();
                }
            }
        highlightTag = text.find(HIGHLIGHT_TAG, highlightTag+HIGHLIGHT_TAG.length());
        }
    // copy over the rest of the text
    correctedText.Append(text.substr(previousPos));
    return correctedText;
#elif defined(__WXOSX__)
    const wxString BACKGROUND_COLOR_TAG = _DT(L"\\chcbpat");
    const wxString BACKGROUND_COLOR_TAG2 = _DT(L"\\highlight");
    const wxString HIGHLIGHT_TAG{ _DT(L"\\cb") };
    wxString highlightNumber;
    wxString correctedText; correctedText.reserve(text.length() * 1.5);
    // add "chcbpat" to each "cb"
    long previousPos = 0;
    long highlightTag = text.find(HIGHLIGHT_TAG);
    while (highlightTag != wxNOT_FOUND)
        {
        correctedText.Append(text.substr(previousPos,
                             (highlightTag+HIGHLIGHT_TAG.length())-previousPos));
        previousPos = (highlightTag+HIGHLIGHT_TAG.length());
        // verify that it's a tag and not actually the word "\cb"
        if (highlightTag == 0 ||
            (highlightTag > 0 && text[highlightTag-1] != L'\\') )
            {
            const long nextSpace = text.find_first_of(L" \n\r\t\\",
                                   highlightTag+HIGHLIGHT_TAG.length());
            if (nextSpace != wxNOT_FOUND)
                {
                highlightNumber = text.substr(highlightTag+HIGHLIGHT_TAG.length(),
                                              (nextSpace-(highlightTag+HIGHLIGHT_TAG.length())) );
                correctedText.Append(highlightNumber+BACKGROUND_COLOR_TAG+highlightNumber+
                                     BACKGROUND_COLOR_TAG2+highlightNumber);
                previousPos += highlightNumber.length();
                }
            }
        highlightTag = text.find(HIGHLIGHT_TAG, highlightTag+HIGHLIGHT_TAG.length());
        }
    // copy over the rest of the text
    correctedText.Append(text.substr(previousPos));
    return correctedText;
#elif defined(__WXGTK__)
    // nothing to do here
#endif
    }

#ifdef __WXGTK__
//-----------------------------------------------------------
void FormattedTextCtrl::GetFormattedTextGtk(wxString& text, const GtkFormat format) const
    {
    text.Clear();
    GtkWidget* text_view = gtk_bin_get_child(GTK_BIN(GTK_SCROLLED_WINDOW(GetHandle())));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view) );

    GtkTextIter start, end;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar* bufferedUTF8Text = gtk_text_buffer_get_text(buffer, &start, &end, false);
    if (!bufferedUTF8Text)
        { return; }
    /* Always convert this UTF8 text to unicode in here while we format it. This makes
       things much easier because the GTK offset functions treat offsets as characters instead
       of bytes.*/
    const std::wstring bufferedText(
        static_cast<const wchar_t*>(wxConvUTF8.cMB2WC(bufferedUTF8Text)));
    g_free(bufferedUTF8Text);
    text.reserve(bufferedText.length()*2);

    // read in the tags
    std::vector<wxColour> colorTable;
    std::vector<wxString> fontTable;
    gdouble defaultFontSize = 12;
    gchar* family = nullptr;
    std::wstring currentTagText;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    wxString textBetweenTags;
    gint offset = 0;
    gint previousStart = 0;
    // handle the first tag that defines the default formatting for all of the text
    if (gtk_text_iter_begins_tag(&start, nullptr))
        {
        GSList* tags = gtk_text_iter_get_toggled_tags(&start, true);
        wxString firstTag;
        if (format == HtmlFormat)
            { firstTag = wxGtkTextTagToHtmlSpanTag(GTK_TEXT_TAG(tags->data)); }
        else
            {
            firstTag = wxGtkTextTagToRtfTag(GTK_TEXT_TAG(tags->data), colorTable, fontTable);
            // just get the font family. The face name in Pango includes other
            // descriptives strings that we don't use here
            g_object_get(G_OBJECT(tags->data),
                "size-points", &defaultFontSize,
                "family", &family,
                nullptr);
            }
        text = firstTag;
        previousStart = gtk_text_iter_get_offset(&start);
        g_slist_free(tags);
        }
    while (gtk_text_iter_compare (&start, &end) < 0)
        {
        if (!gtk_text_iter_forward_to_tag_toggle(&start, nullptr))
            { break; }
        // get all of the tags at the current iterator (there might be more than one)
        GSList *tags = nullptr, *tagp = nullptr;
        tags = gtk_text_iter_get_toggled_tags(&start, false);
        tags = g_slist_concat(tags, gtk_text_iter_get_toggled_tags(&start, true));
        // clear the formatted command now that we are on a new tag iterator
        currentTagText.clear();
        // go through each of the tags on the iterator
        for (tagp = tags; tagp != nullptr; tagp = tagp->next)
            {
            GtkTextTag* tag = GTK_TEXT_TAG(tagp->data);
            //any tags at the current iterator that might start a new formatting block
            // (there might be more than one, though unlikely)
            if (gtk_text_iter_begins_tag(&start, tag))
                {
                if (format == HtmlFormat)
                    { currentTagText += wxGtkTextTagToHtmlSpanTag(tag); }
                else
                    { currentTagText += wxGtkTextTagToRtfTag(tag, colorTable, fontTable); }
                }
            // any tags at the current iterator that might end a formatting block
            // (there might be more than one, though unlikely)
            else if (gtk_text_iter_ends_tag(&start, tag))
                {
                if (format == HtmlFormat)
                    { currentTagText += L"</span>"; }
                else
                    {
                    currentTagText +=
                        wxString::Format(L" \\cf0\\ulnone\\b0\\i0\\f0\\fs%u ",
                                         static_cast<guint>(defaultFontSize)*2).wc_str();
                    }
                }
            }

        offset = gtk_text_iter_get_offset(&start);

        // get the text between the previous format statement and the current one and encode it
        textBetweenTags = bufferedText.substr(previousStart, offset-previousStart).c_str();
        if (format == HtmlFormat)
            { textBetweenTags = htmlEncode(textBetweenTags, textBetweenTags.length()).c_str(); }
        else if (format == RtfFormat)
            { textBetweenTags = rtfEncode(textBetweenTags,textBetweenTags.length()).c_str(); }
        text += textBetweenTags;
        // insert the format statement(s) (that either begin or end a format block).
        text += currentTagText;
        previousStart = offset;
        g_slist_free(tags);
        }

    if (format == HtmlFormat)
        {
        text += L"</span>";
        }
    else if (format == RtfFormat)
        {
        wxString defaultFontFamily(family, *wxConvCurrent);
        g_free(family);
        wxString headerText =
            wxString::Format(
                L"{\\rtf1\\ansi\\deff0\\deflang1033{\\fonttbl{\\f0\\%s\\fcharset0 %s;}}\n",
            defaultFontFamily, defaultFontFamily);
        // add the color table
        if (colorTable.size() > 0)
            {
            headerText += L"{\\colortbl ;";
            for (std::vector<wxColour>::const_iterator colorPos = colorTable.begin();
                 colorPos != colorTable.end();
                 ++colorPos)
                {
                headerText +=
                    wxString::Format(L"\\red%u\\green%u\\blue%u;",
                                     colorPos->Red(), colorPos->Green(), colorPos->Blue());
                }
            headerText += L"}\n";
            }
        text.Prepend(headerText);
        text += L"\\par\n}";
        }
    }
#endif

//-----------------------------------------------------------
void FormattedTextCtrl::SetFormattedText(const wchar_t* formattedText)
    {
    m_rtfLength = static_cast<unsigned long>(wxStrlen(formattedText));

#ifdef __WXMSW__
    SETTEXTEX textInfo{0};
    textInfo.flags = ST_DEFAULT;
    textInfo.codepage = CP_ACP;

    /* Never pass unicode text here (even in a unicode build).
       If you pass in unicode text, then the control's RTF parser
       never gets called and your formatting code is actually
       displayed in the control! Just make sure that the text you pass
       in here has extended ASCII characters RTF encoded--
       that way you won't lose anything in the conversion.*/
    ::SendMessage(GetHwnd(), EM_SETTEXTEX, (WPARAM)&textInfo,
        (LPARAM)static_cast<const char*>(wxConvCurrent->cWX2MB(formattedText)));

#elif defined(__WXGTK__)
    GtkWidget* text_view = gtk_bin_get_child(GTK_BIN(GTK_SCROLLED_WINDOW(GetHandle())));
    GtkTextBuffer* buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text_view) );
    text_buffer_set_markup(buffer, wxConvUTF8.cWC2MB(formattedText), -1);
#else
    GetTextPeer()->SetRtfValue(formattedText);
#endif
    }

#ifdef __WXMSW__
//-----------------------------------------------------------
DWORD wxCALLBACK FormattedTextCtrl::EditStreamOutCallback(
    DWORD_PTR dwCookie, LPBYTE pbBuff, LONG cb, [[maybe_unused]] LONG* pcb)
    {
#if wxUSE_THREADS
    wxMutexGuiLeaveOrEnter();
#endif // wxUSE_THREADS
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) )
        {
        if (msg.message != WM_QUIT)
            {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            }
        }

    // Address of our string var is in psEntry
    std::string* psEntry = reinterpret_cast<std::string*>(dwCookie);

    //write the text
    (*psEntry).append(reinterpret_cast<const char*>(pbBuff), cb);

    return 0;
    }

#endif