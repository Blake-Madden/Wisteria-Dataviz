#include "reportprintout.h"

using namespace Wisteria;

//------------------------------------------------------
bool ReportPrintout::OnPrintPage(int page)
    {
    wxDC* dc = GetDC();
    auto canvas = GetCanvasFromPageNumber(page);
    assert(dc && L"Invalid printing DC!");
    assert(canvas && L"Invalid page when printing report!");
    if (dc != nullptr && canvas != nullptr)
        {
        wxWindowUpdateLocker wl(canvas);
        PrintFitToPageChanger fpc(canvas, this);

        dc->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

        // get the size of the canvas
        wxGCDC gdc;
        wxCoord maxX = canvas->GetCanvasRect(gdc).GetWidth(),
                maxY = canvas->GetCanvasRect(gdc).GetHeight();

        // let's have at least 10 device units margin
        const float marginX = GetMarginPadding(page);
        const float marginY = GetMarginPadding(page);

        // add the margin to the graphic size
        maxX += static_cast<wxCoord>(2*marginX);
        maxY += static_cast<wxCoord>(2*marginY);

        // add space for the headers and footers (if being used)
        // measure a standard line of text
        // (and add 50% for padding)
        const auto textHeight = dc->GetTextExtent(L"Aq").GetHeight();
        long headerFooterUsedHeight{ 0 };
        if (canvas->GetLeftPrinterHeader().length() ||
            canvas->GetCenterPrinterHeader().length() ||
            canvas->GetRightPrinterHeader().length())
            {
            maxY += textHeight * 1.5;
            headerFooterUsedHeight += textHeight * 1.5;
            }
        if (canvas->GetLeftPrinterFooter().length() ||
            canvas->GetCenterPrinterFooter().length() ||
            canvas->GetRightPrinterFooter().length())
            {
            maxY += textHeight * 1.5;
            headerFooterUsedHeight += textHeight * 1.5;
            }

        // get the size of the DC's drawing area in pixels
        int dcWidth{ 0 }, dcHeight{ 0 };
        dc->GetSize(&dcWidth, &dcHeight);

        // calculate a suitable scaling factor
        const float scaleX = safe_divide<float>(dcWidth, maxX);
        const float scaleY = safe_divide<float>(dcHeight, maxY);
        const float scaleXReciprocal = safe_divide<float>(1.0f, scaleX);
        const float scaleYReciprocal = safe_divide<float>(1.0f, scaleY);

        // calculate the position on the DC for centering the graphic
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

        canvas->OnDraw(gcdc);
#endif
        dc->Blit(0, 0, dcWidth, dcHeight, &memDc, 0, 0);

        // draw decorations around canvas content
        double userScalingXBackup{ 0 }, userScalingYBackup{ 0 };
        dc->GetUserScale(&userScalingXBackup, &userScalingYBackup);
        const auto deviceOriginBackup = dc->GetDeviceOrigin();
        const auto mapModeBackup = dc->GetMapMode();

        dc->SetUserScale(scaleX, scaleY);
        dc->SetDeviceOrigin(0, 0);
        dc->SetMapMode(wxMM_TEXT);
        wxCoord width{ 0 }, height{ 0 };

        // draw the headers
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
                    static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprocal),2) -
                                            safe_divide<float>(width,2)),
                    static_cast<wxCoord>(marginY));
                }
            if (canvas->GetRightPrinterHeader().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetRightPrinterHeader(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetRightPrinterHeader(), page),
                    static_cast<wxCoord>((dcWidth*scaleXReciprocal) - (marginX+width)),
                    static_cast<wxCoord>(marginY));
                }
            }
        // draw the footers
        if (canvas->GetLeftPrinterFooter().length() ||
            canvas->GetCenterPrinterFooter().length() ||
            canvas->GetRightPrinterFooter().length())
            {
            dc->GetTextExtent(L"MeasurementTestString", &width, &height);
            const long yPos = (dcHeight*scaleYReciprocal)-(marginY+height);
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
                    static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprocal),2) -
                        safe_divide<float>(width,2)),
                    yPos);
                }
            if (canvas->GetRightPrinterFooter().length())
                {
                dc->GetTextExtent(ExpandPrintString(
                    canvas->GetRightPrinterFooter(), page), &width, &height);
                dc->DrawText(ExpandPrintString(canvas->GetRightPrinterFooter(), page),
                    static_cast<wxCoord>(((dcWidth*scaleXReciprocal) - (marginX+width))),
                    yPos);
                }
            }

        // restore for next page
        dc->SetUserScale(userScalingXBackup, userScalingYBackup);
        dc->SetDeviceOrigin(deviceOriginBackup.x, deviceOriginBackup.y);
        dc->SetMapMode(mapModeBackup);

        return true;
        }
    else
        { return false; }
    }

//------------------------------------------------------
wxString ReportPrintout::ExpandPrintString(
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
PrintFitToPageChanger::PrintFitToPageChanger(Canvas* canvas,
                                             const ReportPrintout* printOut) :
    m_canvas(canvas),
    m_originalMinWidth(canvas ? canvas->GetCanvasMinWidthDIPs() : 0),
    m_originalMinHeight(canvas ? canvas->GetCanvasMinHeightDIPs() : 0),
    m_originalSize(canvas ? canvas->GetSize() : wxSize())
    {
    assert(canvas && L"Invalid canvas passed to PrintFitToPageChanger!");
    assert(printOut && L"Invalid printout passed to PrintFitToPageChanger!");
    if (m_canvas != nullptr &&
        printOut != nullptr &&
        m_canvas->IsFittingToPageWhenPrinting())
        {
        int w{ 0 }, h{ 0 };
        printOut->GetPageSizePixels(&w, &h);
        const auto scaledHeight =
            geometry::rescaled_height(std::make_pair(w, h), m_originalMinWidth);

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
PrintFitToPageChanger::~PrintFitToPageChanger()
    {
    if (m_canvas != nullptr && m_canvas->IsFittingToPageWhenPrinting())
        {
        m_canvas->SetCanvasMinWidthDIPs(m_originalMinWidth);
        m_canvas->SetCanvasMinHeightDIPs(m_originalMinHeight);
        m_canvas->CalcRowDimensions();
        m_canvas->SetSize(m_originalSize);
        }
    }

//------------------------------------------------------
FitToSaveOptionsChanger::FitToSaveOptionsChanger(Canvas* canvas,
                                                 const wxSize newSize) :
    m_canvas(canvas),
    m_originalMinWidth(canvas ? canvas->GetCanvasMinWidthDIPs() : 0),
    m_originalMinHeight(canvas ? canvas->GetCanvasMinHeightDIPs() : 0),
    m_originalSize(canvas ? canvas->GetSize() : wxSize())
    {
    assert(canvas && L"Invalid canvas passed to PrintFitToPageChanger!");
    if (m_canvas != nullptr)
        {
        const wxSize currentSize(canvas->GetCanvasRectDIPs().GetWidth(),
                                 canvas->GetCanvasRectDIPs().GetHeight());
        m_sizeChanged = currentSize != newSize;
        if (m_canvas != nullptr && m_sizeChanged)
            {
            m_canvas->SetCanvasMinWidthDIPs(
                std::min(Canvas::GetDefaultCanvasWidthDIPs(), newSize.GetWidth()));
            m_canvas->SetCanvasMinHeightDIPs(
                std::min(Canvas::GetDefaultCanvasHeightDIPs(), newSize.GetHeight()));
            // recalculate the row and column proportions for the new drawing area
            m_canvas->CalcRowDimensions();
            // set the physical size of the window; this will force a call to
            // CalcAllSizes() and fit all the objects to the altered drawing area
            m_canvas->SetSize(m_canvas->FromDIP(newSize));
            }
        }
    }

//------------------------------------------------------
FitToSaveOptionsChanger::~FitToSaveOptionsChanger()
    {
    if (m_canvas != nullptr && m_sizeChanged)
        {
        m_canvas->SetCanvasMinWidthDIPs(m_originalMinWidth);
        m_canvas->SetCanvasMinHeightDIPs(m_originalMinHeight);
        m_canvas->CalcRowDimensions();
        m_canvas->SetSize(m_originalSize);
        }
    }
