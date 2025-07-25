///////////////////////////////////////////////////////////////////////////////
// Name:        formattedtextctrl.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include <wx/paper.h>
#ifdef __WXOSX__
    #include "wx/osx/private.h"
#elif defined(__WXGTK__)
    #include "gtk/gtktextview-helper.h"
    #include <gtk/gtk.h>
    #include <gtk/gtktextchild.h>
    #include <gtk/gtktextiter.h>
    #include <gtk/gtktexttag.h>
#endif
#include "../../import/html_encode.h"
#include "../../import/rtf_encode.h"
#include "../dialogs/radioboxdlg.h"
#include "formattedtextctrl.h"

using namespace Wisteria::UI;

wxIMPLEMENT_DYNAMIC_CLASS(FormattedTextCtrl, wxTextCtrl)

/** @brief Printing system for Windows.
    @note macOS's text control has its own printing mechanism that we
        patch into wxWidgets and use, so we don't have a dedicated printout
        interface for that platform.*/
#if defined(__WXMSW__)
    class wxFormattedTextCtrlPrintout final : public wxPrintout
    {
  public:
    /// @brief Constructor.
    /// @param control The FormattedTextCtrl to print.
    /// @param title The printout title.
    wxFormattedTextCtrlPrintout(FormattedTextCtrl* control, const wxString& title)
        : wxPrintout(title), m_control(control)
        {
        }

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
        {
        return (pageNum >= 1 && pageNum <= static_cast<int>(m_pageStarts.size()));
        }

    /// @private
    void GetPageInfo(int* minPage, int* maxPage, int* selPageFrom, int* selPageTo) final
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
            PrintSection(m_pageStarts[page - 1], true);
            return true;
            }
        else
            {
            return false;
            }
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
        {
        return m_pageStarts.size();
        }

  private:
    [[nodiscard]]
    wxString ExpandPrintString(const wxString& printString) const
        {
        const wxDateTime now = wxDateTime::Now();
        wxString expandedString = printString;

        expandedString.Replace(
            L"@PAGENUM@",
            wxNumberFormatter::ToString(m_currentPage, 0,
                                        wxNumberFormatter::Style::Style_WithThousandsSep),
            true);
        expandedString.Replace(
            L"@PAGESCNT@",
            wxNumberFormatter::ToString(GetPageCount(), 0,
                                        wxNumberFormatter::Style::Style_WithThousandsSep),
            true);
        expandedString.Replace(L"@TITLE@", m_control->GetTitleName(), true);
        expandedString.Replace(L"@USER@", wxGetUserName(), true);
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

        scaleX = safe_divide<double>(ppiPrinterX, ppiScreenX);
        scaleY = safe_divide<double>(ppiPrinterY, ppiScreenY);
        if (scaleX == 0)
            {
            scaleX = 1;
            }
        if (scaleY == 0)
            {
            scaleY = 1;
            }
        }

    /// @returns The margin around the printing area.
    [[nodiscard]]
    wxCoord GetMarginPadding() const
        {
        return 10 * m_control->GetDPIScaleFactor();
        }

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
            drawingWidth -= 2 * marginX;
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
                bodyStart += textHeight + marginY;
                }
            if (m_control->GetLeftPrinterFooter().length() ||
                m_control->GetCenterPrinterFooter().length() ||
                m_control->GetRightPrinterFooter().length())
                {
                bottomMargin += textHeight;
                }
            drawingHeight -= (topMargin + bottomMargin);

            const auto mearsureAndDrawText = [this, charStart, renderPage](wxDC& dc)
            {
                // https://devblogs.microsoft.com/oldnewthing/20070112-02/?p=28423
                FORMATRANGE fr{ 0 };

                const auto HCD = dc.GetTempHDC();
                fr.hdcTarget = fr.hdc = HCD.GetHDC();

                wxCopyRectToRECT(m_control->GetPageRect(), fr.rcPage);
                wxCopyRectToRECT(m_control->GetPageContentRect(), fr.rc);

                fr.chrg.cpMin = charStart;
                fr.chrg.cpMax = -1;

                // measure and/or render the formatted text
                const long retval = ::SendMessage(m_control->GetHWND(), EM_FORMATRANGE,
                                                  (WPARAM)renderPage, (LPARAM)&fr);

                return retval;
            };

            const auto drawHeadersAndFooters = [this, marginX, marginY, drawingWidth, drawingHeight,
                                                topMargin, &textWidth, &textHeight](wxDC& dc)
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
                                    static_cast<wxCoord>(marginY / 2));
                        }
                    if (m_control->GetCenterPrinterHeader().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetCenterPrinterHeader()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetCenterPrinterHeader()),
                                    static_cast<wxCoord>(safe_divide<double>(drawingWidth, 2) -
                                                         safe_divide<double>(textWidth, 2)),
                                    static_cast<wxCoord>(marginY / 2));
                        }
                    if (m_control->GetRightPrinterHeader().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetRightPrinterHeader()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetRightPrinterHeader()),
                                    static_cast<wxCoord>(drawingWidth - (marginX + textWidth)),
                                    static_cast<wxCoord>(marginY / 2));
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
                    const wxCoord yPos = topMargin + drawingHeight + (marginY / 2);
                    if (m_control->GetLeftPrinterFooter().length())
                        {
                        dc.DrawText(ExpandPrintString(m_control->GetLeftPrinterFooter()),
                                    static_cast<wxCoord>(marginX), yPos);
                        }
                    if (m_control->GetCenterPrinterFooter().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetCenterPrinterFooter()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetCenterPrinterFooter()),
                                    static_cast<wxCoord>(safe_divide<double>(drawingWidth, 2) -
                                                         safe_divide<double>(textWidth, 2)),
                                    yPos);
                        }
                    if (m_control->GetRightPrinterFooter().length())
                        {
                        dc.GetTextExtent(ExpandPrintString(m_control->GetRightPrinterFooter()),
                                         &textWidth, &textHeight);
                        dc.DrawText(ExpandPrintString(m_control->GetRightPrinterFooter()),
                                    static_cast<wxCoord>((drawingWidth - (marginX + textWidth))),
                                    yPos);
                        }
                    }
            };

            if (m_printCanvas.GetSize() != dc->GetSize())
                {
                m_printCanvas.Create(dc->GetSize());
                }
            wxMemoryDC memDc(m_printCanvas);
            memDc.Clear();
            wxGCDC gcdc(memDc);

            // will just calculate pagination if not rendering
            const auto retval = mearsureAndDrawText(gcdc);
            if (renderPage)
                {
                drawHeadersAndFooters(gcdc);
                Wisteria::Canvas::DrawWatermarkLabel(
                    gcdc, wxRect(wxSize(drawingWidth, drawingHeight)), m_control->GetWatermark());
                // copy renderings back into printer DC
                dc->Blit(0, 0, dc->GetSize().GetWidth(), dc->GetSize().GetWidth(), &memDc, 0, 0);
                }
            memDc.SelectObject(wxNullBitmap);

            return retval;
            }
        else
            {
            return -1;
            }
        }

    std::vector<long> m_pageStarts;
    FormattedTextCtrl* m_control{ nullptr };
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
           and obviously we don't want to print a black page.

           The workaround is to create a hidden text control, set its
           background to white, copy in RFT meant for a white background,
           and use that for printing.

           Note that Windows has a headerless RichEdit control via the
           ITextServices interface, but that API is a poorly documented
           COM interface that requires implementing a lot of boilerplate
           interface functionality. It's simpler to just create a
           hidden RichEdit control.*/
#if defined(__WXMSW__) || defined(__WXOSX__)
    if (m_printWindow == nullptr)
        {
        m_printWindow = new FormattedTextCtrl(this);
        }
    m_printWindow->Show(false);
    m_printWindow->SetBackgroundColour(*wxWHITE);
    m_printWindow->SetFormattedText(GetUnthemedFormattedText().length() ?
                                        GetUnthemedFormattedTextRtf(false) :
                                        GetFormattedTextRtf(false));
#endif

#if defined(__WXMSW__)
    wxFormattedTextCtrlPrintout* printOut =
        new wxFormattedTextCtrlPrintout(m_printWindow, GetTitleName());

    wxPrinterDC* dc = nullptr;

    if (m_printData)
        {
        dc = new wxPrinterDC(*m_printData);

        if (m_printData->GetPaperId() == wxPAPER_NONE)
            {
            if (m_printData->GetPaperSize().GetWidth() > 0)
                {
                SetPaperSizeInMillimeters(m_printData->GetPaperSize());
                }
            else
                {
                SetPaperSizeInMillimeters(dc->GetSizeMM());
                }
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
    printer.GetPrintDialogData().SetAllPages(true);
    printer.GetPrintDialogData().SetFromPage(1);
    printer.GetPrintDialogData().SetMinPage(1);
    printer.GetPrintDialogData().EnableSelection(false);
    if (!printer.Print(m_printWindow, printOut, true))
        {
        // just show a message if a real error occurred.
        // They may have just cancelled.
        if (printer.GetLastError() == wxPRINTER_ERROR)
            {
            wxMessageBox(_(L"An error occurred while printing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print"), wxOK);
            }
        }
    if (m_printData)
        {
        *m_printData = printer.GetPrintDialogData().GetPrintData();
        }
    wxDELETE(dc);
    wxDELETE(printOut);
#elif defined(__WXGTK__)
    GtkPrintOperation* operation = gtk_print_operation_new();

    GtkPrintSettings* settings = gtk_print_settings_new();

    if (m_printData)
        {
        gtk_print_settings_set_orientation(settings, (m_printData->GetOrientation() == wxLANDSCAPE ?
                                                          GTK_PAGE_ORIENTATION_LANDSCAPE :
                                                          GTK_PAGE_ORIENTATION_PORTRAIT));
        gtk_print_settings_set_n_copies(settings, m_printData->GetNoCopies());

        GtkPaperSize* paperSize =
            _GtkGetPaperSize(m_printData->GetPaperId(), m_printData->GetPaperSize());
        gtk_print_settings_set_paper_size(settings, paperSize);
        gtk_paper_size_free(paperSize);
        }
    gtk_print_operation_set_print_settings(operation, settings);

    // page setup tab
    GtkPageSetup* pgSetup = gtk_page_setup_new();
    gtk_page_setup_set_orientation(pgSetup, gtk_print_settings_get_orientation(settings));

    GtkPaperSize* paper_size = gtk_print_settings_get_paper_size(settings);
    if (paper_size != nullptr)
        {
        gtk_page_setup_set_paper_size(pgSetup, paper_size);
        gtk_paper_size_free(paper_size);
        }
    gtk_print_operation_set_default_page_setup(operation, pgSetup);
    gtk_print_operation_set_embed_page_setup(operation, TRUE);
    g_object_unref(pgSetup);

    _GtkPrintData printData;
    printData.m_markupContent = GetUnthemedFormattedText().utf8_string();

    printData.m_leftPrintHeader = ExpandUnixPrintString(GetLeftPrinterHeader());
    printData.m_centerPrintHeader = ExpandUnixPrintString(GetCenterPrinterHeader());
    printData.m_rightPrintHeader = ExpandUnixPrintString(GetRightPrinterHeader());
    printData.m_leftPrintFooter = ExpandUnixPrintString(GetLeftPrinterFooter());
    printData.m_centerPrintFooter = ExpandUnixPrintString(GetCenterPrinterFooter());
    printData.m_rightPrintFooter = ExpandUnixPrintString(GetRightPrinterFooter());

    g_signal_connect(G_OBJECT(operation), "begin-print", G_CALLBACK(_GtkBeginPrint),
                     static_cast<gpointer>(&printData));
    g_signal_connect(G_OBJECT(operation), "draw-page", G_CALLBACK(_GtkDrawPage),
                     static_cast<gpointer>(&printData));
    g_signal_connect(G_OBJECT(operation), "end-print", G_CALLBACK(_GtkEndPrint),
                     static_cast<gpointer>(&printData));

    GError* error{ nullptr };
    const gint printResult =
        gtk_print_operation_run(operation, GTK_PRINT_OPERATION_ACTION_PRINT_DIALOG,
                                GTK_WINDOW(gtk_widget_get_toplevel(this->m_widget)), &error);

    if (printResult == GTK_PRINT_OPERATION_RESULT_APPLY)
        {
        if (settings != nullptr)
            {
            g_object_unref(settings);
            }
        settings = g_object_ref(gtk_print_operation_get_print_settings(operation));

        _GtkUpdatePrintSettingsFromPageSetup(operation, settings, m_printData);
        }
    else if (error)
        {
        wxMessageBox(wxString::Format(_(L"An error occurred while printing.\n%s"), error->message),
                     _(L"Print"), wxOK | wxICON_QUESTION);

        g_error_free(error);
        }

    g_object_unref(operation);
    g_object_unref(settings);
#else
    const wxSize paperSize = wxThePrintPaperDatabase->GetSize(m_printData->GetPaperId());
    const double PaperWidthInInches = (paperSize.GetWidth() / 10) * 0.0393700787;
    const double PaperHeightInInches = (paperSize.GetHeight() / 10) * 0.0393700787;

    wxClientDC dc(this);
    wxFont fixedFont(12, wxFontFamily::wxFONTFAMILY_MODERN, wxFontStyle::wxFONTSTYLE_NORMAL,
                     wxFontWeight::wxFONTWEIGHT_NORMAL, false, L"Courier New");
    dc.SetFont(fixedFont);
    wxCoord textWidth{ 0 }, textHeight{ 0 };
    dc.GetTextExtent(L" ", &textWidth, &textHeight);
    const size_t spacesCount = (m_printData->GetOrientation() == wxPORTRAIT) ?
                                   safe_divide<size_t>((PaperWidthInInches - .5f) * 72, textWidth) :
                                   safe_divide<size_t>((PaperHeightInInches - .5f) * 72, textWidth);

    // format the header
    wxString expandedLeftHeader = ExpandUnixPrintString(GetLeftPrinterHeader());
    wxString expandedCenterHeader = ExpandUnixPrintString(GetCenterPrinterHeader());
    wxString expandedRightHeader = ExpandUnixPrintString(GetRightPrinterHeader());

    wxString fullHeader = expandedLeftHeader;
    if (spacesCount >= (expandedLeftHeader.length() + expandedCenterHeader.length() +
                        expandedRightHeader.length()))
        {
        const size_t paddingSize =
            spacesCount - (expandedLeftHeader.length() + expandedCenterHeader.length() +
                           expandedRightHeader.length());
        fullHeader.Pad(paddingSize / 2);
        fullHeader += expandedCenterHeader;
        fullHeader.Pad(paddingSize / 2 + (is_even(paddingSize) ? 0 : 1));
        fullHeader += expandedRightHeader;
        }
    else
        {
        fullHeader += expandedCenterHeader;
        fullHeader += expandedRightHeader;
        }

    // format the footer
    wxString expandedLeftFooter = ExpandUnixPrintString(GetLeftPrinterFooter());
    wxString expandedCenterFooter = ExpandUnixPrintString(GetCenterPrinterFooter());
    wxString expandedRightFooter = ExpandUnixPrintString(GetRightPrinterFooter());

    wxString fullFooter = expandedLeftFooter;
    if (spacesCount >= (expandedLeftFooter.length() + expandedCenterFooter.length() +
                        expandedRightFooter.length()))
        {
        const size_t paddingSize =
            spacesCount - (expandedLeftFooter.length() + expandedCenterFooter.length() +
                           expandedRightFooter.length());
        fullFooter.Pad(paddingSize / 2);
        fullFooter += expandedCenterFooter;
        fullFooter.Pad(paddingSize / 2 + (is_even(paddingSize) ? 0 : 1));
        fullFooter += expandedRightFooter;
        }
    else
        {
        fullFooter += expandedCenterFooter;
        fullFooter += expandedRightFooter;
        }
    /// @todo test this
    m_printWindow->GetTextPeer()->Print(wxSize(PaperWidthInInches * 72, PaperHeightInInches * 72),
                                        static_cast<int>(m_printData->GetOrientation()), fullHeader,
                                        fullFooter);
#endif
    }

//------------------------------------------------------
void FormattedTextCtrl::OnPreview([[maybe_unused]] wxCommandEvent& event)
    {
        // note that previewing isn't done on macOS or GTK+ as it has its own native previewing
        // built into its print dialog
#if defined(__WXMSW__)
    if (m_printWindow == nullptr)
        {
        m_printWindow = new FormattedTextCtrl(this);
        }
    m_printWindow->Show(false);
    m_printWindow->SetBackgroundColour(*wxWHITE);
    m_printWindow->SetFormattedText(GetUnthemedFormattedText().length() ?
                                        GetUnthemedFormattedTextRtf(false) :
                                        GetFormattedTextRtf(false));
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

        if (m_printData->GetPaperId() == wxPAPER_NONE)
            {
            if (m_printData->GetPaperSize().GetWidth() > 0)
                {
                SetPaperSizeInMillimeters(m_printData->GetPaperSize());
                }
            else
                {
                SetPaperSizeInMillimeters(dc->GetSizeMM());
                }
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
    preview->GetPrintDialogData().SetAllPages(true);
    preview->GetPrintDialogData().SetFromPage(1);
    preview->GetPrintDialogData().SetMinPage(1);
    preview->GetPrintDialogData().EnableSelection(false);

    if (!preview->Ok())
        {
        wxDELETE(preview);
        wxDELETE(dc);
        wxDELETE(dc2);
        wxMessageBox(_(L"An error occurred while previewing.\n"
                       "Your default printer may not be set correctly."),
                     _(L"Print Preview"), wxOK);
        return;
        }
    int x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };
    wxClientDisplayRect(&x, &y, &width, &height);
    wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                               wxDefaultPosition, wxSize(width, height));

    frame->Centre(wxBOTH);
    frame->Initialize();
    frame->Show(true);

    wxDELETE(dc);
    wxDELETE(dc2);
#else
    wxFAIL_MSG(L"Print preview is Windows only!");
#endif
    }

//------------------------------------------------------
void FormattedTextCtrl::SetPaperSize(const wxPaperSize size)
    {
    wxSize sz = wxThePrintPaperDatabase->GetSize(size);

    if (sz != wxSize{ 0, 0 })
        {
        // sz is in 10ths of a mm, while paper size is in mm
        sz.x /= 10;
        sz.y /= 10;
        SetPaperSizeInMillimeters(sz);
        }
    }

//------------------------------------------------------
void FormattedTextCtrl::OnSave([[maybe_unused]] wxCommandEvent& event)
    {
    wxArrayString choices, descriptions;
    choices.Add(_DT(L"HTML"));
    descriptions.Add(
        _DT(L"<span style='font-weight:bold;'>Hyper Text Markup Language</span><br />") +
        _(L"This format can be displayed in Internet browsers and most word-processing programs."));
    choices.Add(_DT(L"RTF"));
    descriptions.Add(_DT(L"<span style='font-weight:bold;'>Rich Text Format</span><br />") +
                     _(L"This format can be displayed in most word-processing programs."));
#ifdef __WXGTK__
    choices.Add(_DT(L"Pango"));
    descriptions.Add(
        _DT(L"<span style='font-weight:bold;'>Pango</span><br />") +
        _(L"This format is for rendering text within libraries such as Cairo or FreeType."));
#endif
    RadioBoxDlg exportTypesDlg(this, _(L"Select Document Format"), wxString{},
                               _(L"Document formats:"), _(L"Export Document"), choices,
                               descriptions);
    if (exportTypesDlg.ShowModal() != wxID_OK)
        {
        return;
        }
    wxString fileFilter;
    switch (exportTypesDlg.GetSelection())
        {
    case 0:
        fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
        break;
    case 1:
        fileFilter = _DT(L"Rich Text Format (*.rtf)|*.rtf");
        break;
#ifdef __WXGTK__
    case 2:
        fileFilter = _DT(L"Pango Format (*.pango)|*.pango");
        break;
#endif
    default:
        fileFilter = _DT(L"HTML (*.htm;*.html)|*.htm;*.html");
        };
    wxFileDialog dialog(this, _(L"Save As"), wxString{}, GetTitleName(), fileFilter,
                        wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

    if (dialog.ShowModal() != wxID_OK)
        {
        return;
        }

    wxBusyCursor wait;
    wxFileName filePath = dialog.GetPath();
    // in case the extension is missing then use the selected filter
    if (filePath.GetExt().empty())
        {
        switch (exportTypesDlg.GetSelection())
            {
        case 0:
            filePath.SetExt(L"htm");
            break;
        case 1:
            filePath.SetExt(L"rtf");
            break;
#ifdef __WXGTK__
        case 2:
            filePath.SetExt(L"pango");
            break;
#endif
        default:
            filePath.SetExt(L"htm");
            };
        }

    Save(filePath);
    }

//------------------------------------------------------
bool FormattedTextCtrl::Save(const wxFileName& path)
    {
    // create the folder to the filepath, if necessary
    wxFileName::Mkdir(path.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

    if (path.GetExt().CmpNoCase(L"RTF") == 0)
        {
        return SaveAsRtf(path);
        }
#ifdef __WXGTK__
    else if (path.GetExt().CmpNoCase(L"PANGO") == 0)
        {
        return GtkSaveAsPango(path);
        }
#endif
    else
        {
        return SaveAsHtml(path);
        }
    }

#ifdef __WXGTK__
//------------------------------------------------------
bool FormattedTextCtrl::GtkSaveAsPango(const wxFileName& path)
    {
    const wxString pangoBody = GetUnthemedFormattedText();

    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    const bool retVal = file.Write(pangoBody);
    if (!retVal)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save document\n(%s)."), path.GetFullPath()),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION);
        }
    return retVal;
    }
#endif

//------------------------------------------------------
bool FormattedTextCtrl::SaveAsHtml(const wxFileName& path)
    {
    wxString htmlBody = GetUnthemedFormattedTextHtml();

    wxString htmlText = L"<!DOCTYPE html>\n<html>" + htmlBody + L"\n</html>";

    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    const bool retVal = file.Write(htmlText);
    if (!retVal)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save document\n(%s)."), path.GetFullPath()),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
bool FormattedTextCtrl::SaveAsRtf(const wxFileName& path)
    {
    wxFileName(path.GetFullPath()).SetPermissions(wxS_DEFAULT);
    wxFile file(path.GetFullPath(), wxFile::write);
    // export unthemed text (if available)
    const bool retVal =
        file.Write((GetUnthemedFormattedText().length() ? GetUnthemedFormattedTextRtf() :
                                                          GetFormattedTextRtf()));
    if (!retVal)
        {
        wxMessageBox(wxString::Format(_(L"Failed to save document (%s)."), path.GetFullPath()),
                     _(L"Error"), wxOK | wxICON_EXCLAMATION);
        }
    return retVal;
    }

//------------------------------------------------------
void FormattedTextCtrl::OnFind(const wxFindDialogEvent& myEvent)
    {
    // if they were just hitting Cancel then close
    if (myEvent.GetEventType() == wxEVT_COMMAND_FIND_CLOSE)
        {
        return;
        }
    if (wxNOT_FOUND == FindText(myEvent.GetFindString(),
                                int_to_bool(myEvent.GetFlags() & wxFR_DOWN),
                                int_to_bool(myEvent.GetFlags() & wxFR_WHOLEWORD),
                                int_to_bool(myEvent.GetFlags() & wxFR_MATCHCASE)))
        {
        wxMessageDialog(this,
                        (myEvent.GetEventType() == wxEVT_COMMAND_FIND_NEXT) ?
                            _(L"No further occurrences found.") :
                            _(L"The text could not be found."),
                        _(L"Text Not Found"))
            .ShowModal();
        }
    }

//-----------------------------------------------------------
void FormattedTextCtrl::OnContextMenu([[maybe_unused]] wxContextMenuEvent& event)
    {
    if (wxGetMouseState().ShiftDown())
        {
        wxMessageBox(wxString::Format(_DT(L"Cursor position: %d", DTExplanation::DebugMessage),
                                      GetInsertionPoint()),
                     _DT(L"Position"), wxOK);
        }
    PopupMenu(&m_menu);
    }

//------------------------------------------------------
void FormattedTextCtrl::OnSelectAll([[maybe_unused]] wxCommandEvent& event) { SelectAll(); }

//------------------------------------------------------
void FormattedTextCtrl::OnCopyAll([[maybe_unused]] wxCommandEvent& event)
    {
    wxString FormattedText;
#if defined(__WXMSW__) || defined(__WXOSX__)
    FormattedText =
        GetUnthemedFormattedText().length() ? GetUnthemedFormattedTextRtf() : GetFormattedTextRtf();
    if (wxTheClipboard->Open())
        {
        if (FormattedText.length())
            {
            wxTheClipboard->Clear();
            wxDataObjectComposite* obj = new wxDataObjectComposite();
            obj->Add(new wxRtfDataObject(FormattedText), true);
            obj->Add(new wxTextDataObject(GetValue()));
            wxTheClipboard->SetData(obj);
            }
        wxTheClipboard->Close();
        }
#elif defined(__WXGTK__)
    FormattedText = GetUnthemedFormattedTextHtml();
    if (wxTheClipboard->Open())
        {
        if (FormattedText.length())
            {
            wxTheClipboard->Clear();
            wxDataObjectComposite* obj = new wxDataObjectComposite();
            obj->Add(new wxHTMLDataObject(FormattedText), true);
            obj->Add(new wxTextDataObject(GetValue()));
            wxTheClipboard->SetData(obj);
            }
        wxTheClipboard->Close();
        }
#endif
    }

//-----------------------------------------------------------
long FormattedTextCtrl::FindText(const wchar_t* textToFind, const bool searchDown,
                                 const bool matchWholeWord, const bool caseSensitiveSearch)
    {
    long selStart{ 0 }, selEnd{ 0 };
    GetSelection(&selStart, &selEnd);
    wxTextSearchResult result =
        SearchText(wxTextSearch{ textToFind }
                       .MatchCase(caseSensitiveSearch)
                       .MatchWholeWord(matchWholeWord)
                       .SearchDirection(searchDown ? wxTextSearch::Direction::Down :
                                                     wxTextSearch::Direction::Up)
                       .Start(searchDown ? selEnd : selStart));

    if (result)
        {
        SetSelection(result.m_start, result.m_end);
        ShowPosition(result.m_start);
        return result.m_start;
        }

    // if not found and searching down, ask if they would like to start
    // from the beginning and try again.
    if (searchDown && selStart > 0 &&
        (wxMessageBox(_(L"Search has reached the end of the document. "
                        "Do you wish to restart the search from the beginning?"),
                      _(L"Continue Search"), wxYES_NO | wxICON_QUESTION) == wxYES))
        {
        result = SearchText(wxTextSearch{ textToFind }
                                .MatchCase(caseSensitiveSearch)
                                .MatchWholeWord(matchWholeWord)
                                .SearchDirection(wxTextSearch::Direction::Down)
                                .Start(0));
        if (result)
            {
            SetSelection(result.m_start, result.m_end);
            ShowPosition(result.m_start);
            return result.m_start;
            }
        }

    return wxNOT_FOUND;
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::GetUnthemedFormattedTextHtml(
    [[maybe_unused]] const wxString& CssStylePrefix /*= wxString{}*/)
    {
#if defined(__WXMSW__) || defined(__WXOSX__)
    wxString rtfText = GetUnthemedFormattedText().length() ? GetUnthemedFormattedTextRtf(false) :
                                                             GetFormattedTextRtf(false);

    lily_of_the_valley::rtf_extract_text filter_rtf(
        lily_of_the_valley::rtf_extract_text::rtf_extraction_type::rtf_to_html);
    filter_rtf.set_style_prefix(CssStylePrefix.wc_str());
    wxCharBuffer buf = rtfText.mb_str();
    assert(buf.length() == std::strlen(buf.data()));
    const wchar_t* htmlBody = filter_rtf(buf.data(), buf.length());
    if (!htmlBody)
        {
        return wxString{};
        }
    wxString fontString(filter_rtf.get_font().c_str(), wxConvLibc);
    wxString text = wxString::Format(
        L"\n<head>"
        "\n<meta http-equiv='content-type' content='text/html; charset=UTF-8' />"
        "\n<title>%s</title>"
        "\n<style type='text/css'>\n<!--\n%s\n-->\n</style>\n</head>"
        "\n<body>\n<p style='font-family: %s; font-size: %dpt; color: rgb(%u, %u, %u)'>",
        GetTitleName(), filter_rtf.get_style_section().c_str(), fontString,
        filter_rtf.get_font_size(), filter_rtf.get_font_color().red,
        filter_rtf.get_font_color().green, filter_rtf.get_font_color().blue);

    // step over any leading line breaks
    while (std::wcsncmp(htmlBody, L"<br />", 6) == 0)
        {
        htmlBody += 6;
        }
    text += htmlBody;
    text += L"\n</p>\n</body>";
    return text;
#elif defined(__WXGTK__)
    return GtkGetFormattedText(GtkFormat::HtmlFormat, false);
#endif
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::GetUnthemedFormattedTextRtf(
    [[maybe_unused]] const bool fixHighlightingTags /*= true*/)
    {
#if defined(__WXMSW__) || defined(__WXOSX__)
    return fixHighlightingTags ? FixHighlightingTags(GetUnthemedFormattedText()) :
                                 GetUnthemedFormattedText();
#elif defined(__WXGTK__)
    return GtkGetFormattedText(GtkFormat::RtfFormat, false);
#endif
    }

//-----------------------------------------------------------
wxString
FormattedTextCtrl::GetFormattedTextRtf([[maybe_unused]] const bool fixHighlightingTags /*= true*/)
    {
#ifdef __WXGTK__
    return GtkGetFormattedText(GtkFormat::RtfFormat, true);
#endif

    wxString text = IsRTFSupported() ? GetRTFValue() : wxString{};

    if (fixHighlightingTags)
        {
        text = FixHighlightingTags(text);
        }

    return text;
    }

//-----------------------------------------------------------
wxString FormattedTextCtrl::FixHighlightingTags(const wxString& text)
    {
#ifdef __WXMSW__
    const wxString BACKGROUND_COLOR_TAG = L"\\chcbpat";
    const wxString HIGHLIGHT_TAG = L"\\highlight";
    wxString highlightNumber;
    wxString correctedText;
    correctedText.reserve(text.length() * 1.5);
    // add "chcbpat" to each "highlight"
    long previousPos = 0;
    long highlightTag = text.find(HIGHLIGHT_TAG);
    while (highlightTag != wxNOT_FOUND)
        {
        correctedText.Append(
            text.substr(previousPos, (highlightTag + HIGHLIGHT_TAG.length()) - previousPos));
        previousPos = (highlightTag + HIGHLIGHT_TAG.length());
        // verify that it's a tag and not actually the word "\highlight"
        if (highlightTag == 0 || (highlightTag > 0 && text[highlightTag - 1] != L'\\'))
            {
            const long nextSpace =
                text.find_first_of(L" \n\r\t\\", highlightTag + HIGHLIGHT_TAG.length());
            if (nextSpace != wxNOT_FOUND)
                {
                highlightNumber =
                    text.substr(highlightTag + HIGHLIGHT_TAG.length(),
                                (nextSpace - (highlightTag + HIGHLIGHT_TAG.length())));
                correctedText.Append(highlightNumber + BACKGROUND_COLOR_TAG + highlightNumber);
                previousPos += highlightNumber.length();
                }
            }
        highlightTag = text.find(HIGHLIGHT_TAG, highlightTag + HIGHLIGHT_TAG.length());
        }
    // copy over the rest of the text
    correctedText.Append(text.substr(previousPos));
    return correctedText;
#elif defined(__WXOSX__)
    const wxString BACKGROUND_COLOR_TAG = _DT(L"\\chcbpat");
    const wxString BACKGROUND_COLOR_TAG2 = _DT(L"\\highlight");
    const wxString HIGHLIGHT_TAG{ _DT(L"\\cb") };
    wxString correctedText;
    correctedText.reserve(text.length() * 1.5);
    // add "chcbpat" to each "cb"
    long previousPos = 0;
    long highlightTag = text.find(HIGHLIGHT_TAG);
    while (highlightTag != wxNOT_FOUND)
        {
        correctedText.Append(
            text.substr(previousPos, (highlightTag + HIGHLIGHT_TAG.length()) - previousPos));
        previousPos = (highlightTag + HIGHLIGHT_TAG.length());
        // verify that it's a tag and not actually the word "\cb"
        if (highlightTag == 0 || (highlightTag > 0 && text[highlightTag - 1] != L'\\'))
            {
            const long nextSpace =
                text.find_first_of(L" \n\r\t\\", highlightTag + HIGHLIGHT_TAG.length());
            if (nextSpace != wxNOT_FOUND)
                {
                wxString highlightNumber =
                    text.substr(highlightTag + HIGHLIGHT_TAG.length(),
                                (nextSpace - (highlightTag + HIGHLIGHT_TAG.length())));
                correctedText.Append(highlightNumber + BACKGROUND_COLOR_TAG + highlightNumber +
                                     BACKGROUND_COLOR_TAG2 + highlightNumber);
                previousPos += highlightNumber.length();
                }
            }
        highlightTag = text.find(HIGHLIGHT_TAG, highlightTag + HIGHLIGHT_TAG.length());
        }
    // copy over the rest of the text
    correctedText.Append(text.substr(previousPos));
    return correctedText;
#elif defined(__WXGTK__)
    return text;
#endif
    }

#ifdef __WXGTK__
//-----------------------------------------------------------
wxString FormattedTextCtrl::GtkGetFormattedText(const GtkFormat format,
                                                const bool useThemed /*= false*/)
    {
    GtkTextBuffer* buffer{ nullptr };

    GtkTextIter start, end;

    if (useThemed)
        {
        buffer = GTKGetTextBuffer();
        }
    else
        {
        buffer = gtk_text_buffer_new(nullptr);
        gtk_text_buffer_get_start_iter(buffer, &start);
        gtk_text_buffer_insert_markup(buffer, &start, m_unthemedContent.utf8_str(), -1);
        }

    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gchar* bufferedUTF8Text = gtk_text_buffer_get_text(buffer, &start, &end, false);
    if (!bufferedUTF8Text)
        {
        return wxString{};
        }
    /* Always convert this UTF8 text to Unicode in here while we format it. This makes
       things much easier because the GTK offset functions treat offsets as characters instead
       of bytes.*/
    const wxString bufferedText = wxString::FromUTF8(bufferedUTF8Text);
    g_free(bufferedUTF8Text);
    wxString text;
    text.reserve(bufferedText.length() * 2);

    // read in the tags
    std::vector<wxColour> colorTable;
    std::vector<wxString> fontTable;
    gdouble defaultFontSize = 12;
    gchar* family = nullptr;
    std::wstring currentTagText;
    gtk_text_buffer_get_start_iter(buffer, &start);
    gtk_text_buffer_get_end_iter(buffer, &end);
    gint offset = 0;
    gint previousStart = 0;
    // handle the first tag that defines the default formatting for all the text
    if (gtk_text_iter_starts_tag(&start, nullptr))
        {
        GSList* tags = gtk_text_iter_get_toggled_tags(&start, true);
        wxString firstTag;
        if (format == GtkFormat::HtmlFormat)
            {
            firstTag = _GtkTextTagToHtmlSpanTag(GTK_TEXT_TAG(tags->data));
            }
        else
            {
            firstTag = _GtkTextTagToRtfTag(GTK_TEXT_TAG(tags->data), colorTable, fontTable);
            // just get the font family. The face name in Pango includes other
            // descriptives strings that we don't use here
            g_object_get(G_OBJECT(tags->data), "size-points", &defaultFontSize, "family", &family,
                         nullptr);
            }
        text = firstTag;
        previousStart = gtk_text_iter_get_offset(&start);
        g_slist_free(tags);
        }
    lily_of_the_valley::html_encode_text htmlEncode;
    lily_of_the_valley::rtf_encode_text rtfEncode;
    while (gtk_text_iter_compare(&start, &end) < 0)
        {
        if (!gtk_text_iter_forward_to_tag_toggle(&start, nullptr))
            {
            break;
            }
        // get all the tags at the current iterator (there might be more than one)
        GSList *tags = nullptr, *tagp = nullptr;
        tags = gtk_text_iter_get_toggled_tags(&start, false);
        tags = g_slist_concat(tags, gtk_text_iter_get_toggled_tags(&start, true));
        // clear the formatted command now that we are on a new tag iterator
        currentTagText.clear();
        // go through each of the tags on the iterator
        for (tagp = tags; tagp != nullptr; tagp = tagp->next)
            {
            GtkTextTag* tag = GTK_TEXT_TAG(tagp->data);
            // any tags at the current iterator that might start a new formatting block
            // (there might be more than one, though unlikely)
            if (gtk_text_iter_starts_tag(&start, tag))
                {
                if (format == GtkFormat::HtmlFormat)
                    {
                    currentTagText += _GtkTextTagToHtmlSpanTag(tag);
                    }
                else
                    {
                    currentTagText += _GtkTextTagToRtfTag(tag, colorTable, fontTable);
                    }
                }
            // any tags at the current iterator that might end a formatting block
            // (there might be more than one, though unlikely)
            else if (gtk_text_iter_ends_tag(&start, tag))
                {
                if (format == GtkFormat::HtmlFormat)
                    {
                    currentTagText += L"</span>";
                    }
                else
                    {
                    currentTagText +=
                        wxString::Format(L" \\strike0\\highlight0\\cf0\\ulnone\\b0\\i0\\f0\\fs%u ",
                                         static_cast<guint>(defaultFontSize) * 2)
                            .wc_str();
                    }
                }
            }

        offset = gtk_text_iter_get_offset(&start);

        // get the text between the previous format statement and the current one and encode it
        const std::wstring_view textBetweenTags =
            std::wstring_view(bufferedText).substr(previousStart, offset - previousStart);
        if (format == GtkFormat::HtmlFormat)
            {
            text += htmlEncode(textBetweenTags, true).c_str();
            }
        else if (format == GtkFormat::RtfFormat)
            {
            text += rtfEncode(textBetweenTags).c_str();
            }
        // insert the format statement(s) (that either begin or end a format block).
        text += currentTagText;
        previousStart = offset;
        g_slist_free(tags);
        }

    if (format == GtkFormat::HtmlFormat)
        {
        text += L"</span>";
        }
    else if (format == GtkFormat::RtfFormat)
        {
        const wxString defaultFontFamily(family);
        g_free(family);
        wxString headerText = wxString::Format(
            L"{\\rtf1\\ansi\\deff0\\deflang1033{\\fonttbl{\\f0\\%s\\fcharset0 %s;}}\n",
            defaultFontFamily, defaultFontFamily);
        // add the color table
        if (colorTable.size() > 0)
            {
            headerText += L"{\\colortbl ;";
            for (std::vector<wxColour>::const_iterator colorPos = colorTable.begin();
                 colorPos != colorTable.end(); ++colorPos)
                {
                headerText += wxString::Format(L"\\red%u\\green%u\\blue%u;", colorPos->Red(),
                                               colorPos->Green(), colorPos->Blue());
                }
            headerText += L"}\n";
            }
        text.Prepend(headerText);
        text += L"\\par\n}";
        }
    // use g_object_unref() (instead of g_free()) according to GTK+'s own testing scripts
    if (!useThemed)
        {
        g_object_unref(buffer);
        }

    return text;
    }
#endif

//-----------------------------------------------------------
void FormattedTextCtrl::SetFormattedText(const wxString& formattedText)
    {
    m_rtfLength = static_cast<decltype(m_rtfLength)>(formattedText.length());

#ifdef __WXGTK__
    if (IsMultiLine())
        {
        GTKSetPangoMarkup(formattedText);
        }
#else
    if (IsRTFSupported())
        {
        SetRTFValue(formattedText);
        }
#endif

    SetInsertionPoint(0);
    }
