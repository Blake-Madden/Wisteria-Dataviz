#include "reportprintout.h"

//------------------------------------------------------
bool Wisteria::ReportPrintout::OnPrintPage(int page)
    {
    wxDC* dc = GetDC();
    auto canvas = GetCanvasFromPageNumber(page);
    wxASSERT_MSG(dc, L"Invalid printing DC!");
    wxASSERT_MSG(canvas, L"Invalid page when printing report!");
    if (dc != nullptr && canvas != nullptr)
        {
        dc->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

        // get the size of the canvas
        wxGCDC gdc;
        wxCoord maxX = canvas->GetCanvasRect(gdc).GetWidth(),
                maxY = canvas->GetCanvasRect(gdc).GetHeight();

        // Let's have at least 10 device units margin
        const float marginX = GetMarginPadding(page);
        const float marginY = GetMarginPadding(page);

        // add the margin to the graphic size
        maxX += static_cast<wxCoord>(2*marginX);
        maxY += static_cast<wxCoord>(2*marginY);

        // add space for the headers and footers (if being used)
        // measure a standard line of text
        const auto textHeight = dc->GetTextExtent(L"Aq").GetHeight();
        long headerFooterUsedHeight{ 0 };
        if (canvas->GetLeftPrinterHeader().length() ||
            canvas->GetCenterPrinterHeader().length() ||
            canvas->GetRightPrinterHeader().length())
            {
            maxY += textHeight;
            headerFooterUsedHeight += textHeight;
            }
        if (canvas->GetLeftPrinterFooter().length() ||
            canvas->GetCenterPrinterFooter().length() ||
            canvas->GetRightPrinterFooter().length())
            {
            maxY += textHeight;
            headerFooterUsedHeight += textHeight;
            }

        // Get the size of the DC's drawing area in pixels
        int dcWidth{ 0 }, dcHeight{ 0 };
        dc->GetSize(&dcWidth, &dcHeight);

        // Calculate a suitable scaling factor
        const float scaleX = safe_divide<float>(dcWidth, maxX);
        const float scaleY = safe_divide<float>(dcHeight, maxY);
        const float scaleXReciprical = safe_divide<float>(1.0f, scaleX);
        const float scaleYReciprical = safe_divide<float>(1.0f, scaleY);

        // Calculate the position on the DC for centring the graphic
        const float posX = safe_divide<float>(
            (dcWidth -((maxX-(2*marginX))* std::min(scaleX,scaleY))), 2);
        const float posY = safe_divide<float>(
            (dcHeight - ((maxY-(headerFooterUsedHeight+(2*marginY))) *
                std::min(scaleX,scaleY))), 2);

        wxBitmap previewImg;
        previewImg.CreateWithDIPSize(
            wxSize(canvas->ToDIP(dcWidth),
                    canvas->ToDIP(dcHeight)),
            canvas->GetDPIScaleFactor());
        wxMemoryDC memDc(previewImg);
        memDc.Clear();
#ifdef __WXMSW__
        // use Direct2D for rendering
        wxGraphicsContext* context{ nullptr };
        auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (renderer)
            { context = renderer->CreateContext(memDc); }

        if (context)
            {
            wxGCDC gcdc(context);

            /* Set the scale and origin.
                Note that we use the same scale factor for x and y to maintain aspect ratio*/
            gcdc.SetUserScale(std::min(scaleX, scaleY), std::min(scaleX, scaleY));
            gcdc.SetDeviceOrigin(static_cast<wxCoord>(posX), static_cast<wxCoord>(posY));

            canvas->OnDraw(gcdc);
            }
        else
            {
            wxGCDC gcdc(memDc);

            gcdc.SetUserScale(std::min(scaleX, scaleY), std::min(scaleX, scaleY));
            gcdc.SetDeviceOrigin(static_cast<wxCoord>(posX), static_cast<wxCoord>(posY));

            canvas->OnDraw(gcdc);
            }
#else
        wxGCDC gcdc(memDc);

        gcdc.SetUserScale(std::min(scaleX, scaleY), std::min(scaleX, scaleY));
        gcdc.SetDeviceOrigin(static_cast<wxCoord>(posX), static_cast<wxCoord>(posY));

        m_canvas->OnDraw(gcdc);
#endif
        dc->Blit(0,0,dcWidth,dcHeight,&memDc,0,0);

        // draw the headers
        wxCoord width{0}, height{0};
        dc->SetUserScale(scaleX, scaleY);
        dc->SetDeviceOrigin(0, 0);
        dc->SetMapMode(wxMM_TEXT);
        if (canvas->GetLeftPrinterHeader().length() ||
            canvas->GetCenterPrinterHeader().length() ||
            canvas->GetRightPrinterHeader().length())
            {
            if (canvas->GetLeftPrinterHeader().length())
                {
                dc->DrawText(ExpandPrintString(canvas->GetLeftPrinterHeader(), page),
                    static_cast<wxCoord>(marginX),
                    static_cast<wxCoord>(marginY));
                }
            if (canvas->GetCenterPrinterHeader().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetCenterPrinterHeader(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetCenterPrinterHeader(), page),
                    static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprical),2) -
                                            safe_divide<float>(width,2)),
                    static_cast<wxCoord>(marginY));
                }
            if (canvas->GetRightPrinterHeader().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetRightPrinterHeader(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetRightPrinterHeader(), page),
                    static_cast<wxCoord>((dcWidth*scaleXReciprical) - (marginX+width)),
                    static_cast<wxCoord>(marginY));
                }
            }
        // draw the footers
        if (canvas->GetLeftPrinterFooter().length() ||
            canvas->GetCenterPrinterFooter().length() ||
            canvas->GetRightPrinterFooter().length())
            {
            dc->GetTextExtent(L"MeasurementTestString", &width, &height);
            const long yPos = (dcHeight*scaleYReciprical)-(marginY+height);
            if (canvas->GetLeftPrinterFooter().length())
                {
                dc->DrawText(ExpandPrintString(canvas->GetLeftPrinterFooter(), page),
                    static_cast<wxCoord>(marginX),
                    yPos);
                }
            if (canvas->GetCenterPrinterFooter().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetCenterPrinterFooter(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetCenterPrinterFooter(), page),
                    static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprical),2) -
                        safe_divide<float>(width,2)),
                    yPos);
                }
            if (canvas->GetRightPrinterFooter().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetRightPrinterFooter(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetRightPrinterFooter(), page),
                    static_cast<wxCoord>(((dcWidth*scaleXReciprical) - (marginX+width))),
                    yPos);
                }
            }

        return true;
        }
    else return false;
    }

//------------------------------------------------------
wxString Wisteria::ReportPrintout::ExpandPrintString(
    const wxString& printString, const int pageNumber) const
    {
    // page out of range, so don't do anything
    if (GetCanvasFromPageNumber(pageNumber) == nullptr)
        { return printString; }
    wxString expandedString = printString;

    expandedString.Replace(L"@PAGENUM@",
        wxNumberFormatter::ToString(pageNumber, 0,
                                    wxNumberFormatter::Style::Style_WithThousandsSep),
        true);
    expandedString.Replace(L"@PAGESCNT@",
        wxNumberFormatter::ToString(m_canvases.size(), 0,
                                    wxNumberFormatter::Style::Style_WithThousandsSep),
        true);

    const wxDateTime now = wxDateTime::Now();
    expandedString.Replace(L"@TITLE@", GetCanvasFromPageNumber(pageNumber)->GetLabel(), true);
    expandedString.Replace(L"@DATE@", now.FormatDate(), true);
    expandedString.Replace(L"@TIME@", now.FormatTime(), true);

    return expandedString;
    }

//------------------------------------------------------
Wisteria::PrintFitToPageChanger::PrintFitToPageChanger(Canvas* canvas, ReportPrintout* printOut) :
    m_canvas(canvas),
    m_originalMinWidth(canvas ? canvas->GetCanvasMinWidthDIPs() : 0),
    m_originalMinHeight(canvas ? canvas->GetCanvasMinHeightDIPs() : 0),
    m_originalSize(canvas ? canvas->GetSize() : wxSize())
    {
    wxASSERT_MSG(canvas, L"Invalid canvas passed to PrintFitToPageChanger!");
    wxASSERT_MSG(printOut, L"Invalid printout passed to PrintFitToPageChanger!");
    if (m_canvas && printOut && m_canvas->IsFittingToPageWhenPrinting())
        {
        int w{ 0 }, h{ 0 };
        printOut->GetPageSizePixels(&w, &h);
        const auto canvasInDIPs = m_canvas->ToDIP(wxSize(w, h));
        const auto scaledHeight =
            geometry::calculate_rescale_height(std::make_pair(w, h), m_originalMinWidth);

        if (scaledHeight > 0) // sanity check in case page size calc failed
            {
            m_canvas->SetCanvasMinHeightDIPs(scaledHeight);
            // recalculate the row and column proportions for the new drawing area
            m_canvas->CalcRowDimensions();
            // set the physical size of the window to the page's aspect ratio;
            // this will force a call to CalcAllSizes() and fit all the objects to the
            // altered drawing area
            m_canvas->SetSize(
                m_canvas->FromDIP(wxSize(m_canvas->GetCanvasMinWidthDIPs(),
                    m_canvas->GetCanvasMinHeightDIPs())));
            }
        }
    }

//------------------------------------------------------
Wisteria::PrintFitToPageChanger::~PrintFitToPageChanger()
    {
    if (m_canvas->IsFittingToPageWhenPrinting())
        {
        m_canvas->SetCanvasMinWidthDIPs(m_originalMinWidth);
        m_canvas->SetCanvasMinHeightDIPs(m_originalMinHeight);
        m_canvas->CalcRowDimensions();
        m_canvas->SetSize(m_originalSize);
        }
    }
