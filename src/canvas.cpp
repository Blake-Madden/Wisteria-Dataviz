///////////////////////////////////////////////////////////////////////////////
// Name:        canvas.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2022 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "canvas.h"
#include "colorbrewer.h"
#include "axis.h"

DEFINE_EVENT_TYPE(EVT_WISTERIA_CANVAS_DCLICK)

using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::UI;

namespace Wisteria
    {
    /// @brief Printing interface for canvas.
    class CanvasPrintout final : public wxPrintout
        {
    public:
        /// @brief Constructor.
        CanvasPrintout(Canvas* cvs, const wxString& title) : wxPrintout(title), m_canvas(cvs)
            {}
        /** @returns `true` if specified page number is within the range of pages being printed.
            @param pageNum The page number to check for.*/
        bool HasPage(int pageNum) noexcept final
            { return (pageNum == 1); }
        /** @brief Retrieves page information for printing.
            @param[out] minPage The lowest possible page index.
            @param[out] maxPage The highest possible page index.
            @param[out] selPageFrom The starting page.
            @param[out] selPageTo The ending page.*/
        void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) final
            {
            *minPage = 1;
            *maxPage = 1;
            *selPageFrom = 1;
            *selPageTo = 1;
            }
        /** @brief Prints the specified page number.
            @param page The page to print.
            @returns `true` if printing page was successful.*/
        bool OnPrintPage(int page) final
            {
            wxDC* dc = GetDC();
            if (dc && (page == 1))
                {
                dc->SetFont(wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT));

                // get the size of the canvas
                wxCoord maxX = m_canvas->GetCanvasRect().GetWidth(),
                        maxY = m_canvas->GetCanvasRect().GetHeight();

                // Let's have at least 10 device units margin
                const float marginX = GetMarginPadding();
                const float marginY = GetMarginPadding();

                // add the margin to the graphic size
                maxX += static_cast<wxCoord>(2*marginX);
                maxY += static_cast<wxCoord>(2*marginY);

                // add space for the headers and footers (if being used)
                // measure a standard line of text
                const auto textHeight = dc->GetTextExtent(L"Aq").GetHeight();
                long headerFooterUsedHeight{ 0 };
                if (m_canvas->GetLeftPrinterHeader().length() ||
                    m_canvas->GetCenterPrinterHeader().length() ||
                    m_canvas->GetRightPrinterHeader().length())
                    {
                    maxY += textHeight;
                    headerFooterUsedHeight += textHeight;
                    }
                if (m_canvas->GetLeftPrinterFooter().length() ||
                    m_canvas->GetCenterPrinterFooter().length() ||
                    m_canvas->GetRightPrinterFooter().length())
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
                    wxSize(m_canvas->ToDIP(dcWidth), m_canvas->ToDIP(dcHeight)),
                    m_canvas->GetDPIScaleFactor());
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

                    m_canvas->OnDraw(gcdc);
                    }
                else
                    {
                    wxGCDC gcdc(memDc);

                    gcdc.SetUserScale(std::min(scaleX, scaleY), std::min(scaleX, scaleY));
                    gcdc.SetDeviceOrigin(static_cast<wxCoord>(posX), static_cast<wxCoord>(posY));

                    m_canvas->OnDraw(gcdc);
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
                if (m_canvas->GetLeftPrinterHeader().length() ||
                    m_canvas->GetCenterPrinterHeader().length() ||
                    m_canvas->GetRightPrinterHeader().length())
                    {
                    if (m_canvas->GetLeftPrinterHeader().length())
                        {
                        dc->DrawText(ExpandPrintString(m_canvas->GetLeftPrinterHeader()),
                            static_cast<wxCoord>(marginX),
                            static_cast<wxCoord>(marginY));
                        }
                    if (m_canvas->GetCenterPrinterHeader().length())
                        {
                        dc->GetTextExtent(ExpandPrintString(m_canvas->GetCenterPrinterHeader()), &width, &height);
                        dc->DrawText(ExpandPrintString(m_canvas->GetCenterPrinterHeader()),
                            static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprical),2) -
                                                 safe_divide<float>(width,2)),
                            static_cast<wxCoord>(marginY));
                        }
                    if (m_canvas->GetRightPrinterHeader().length())
                        {
                        dc->GetTextExtent(ExpandPrintString(m_canvas->GetRightPrinterHeader()), &width, &height);
                        dc->DrawText(ExpandPrintString(m_canvas->GetRightPrinterHeader()),
                            static_cast<wxCoord>((dcWidth*scaleXReciprical) - (marginX+width)),
                            static_cast<wxCoord>(marginY));
                        }
                    }
                // draw the footers
                if (m_canvas->GetLeftPrinterFooter().length() ||
                    m_canvas->GetCenterPrinterFooter().length() ||
                    m_canvas->GetRightPrinterFooter().length())
                    {
                    dc->GetTextExtent(L"MeasurementTestString", &width, &height);
                    const long yPos = (dcHeight*scaleYReciprical)-(marginY+height);
                    if (m_canvas->GetLeftPrinterFooter().length())
                        {
                        dc->DrawText(ExpandPrintString(m_canvas->GetLeftPrinterFooter()),
                            static_cast<wxCoord>(marginX),
                            yPos);
                        }
                    if (m_canvas->GetCenterPrinterFooter().length())
                        {
                        dc->GetTextExtent(ExpandPrintString(m_canvas->GetCenterPrinterFooter()), &width, &height);
                        dc->DrawText(ExpandPrintString(m_canvas->GetCenterPrinterFooter()),
                            static_cast<wxCoord>(safe_divide<float>((dcWidth*scaleXReciprical),2) - safe_divide<float>(width,2)),
                            yPos);
                        }
                    if (m_canvas->GetRightPrinterFooter().length())
                        {
                        dc->GetTextExtent(ExpandPrintString(m_canvas->GetRightPrinterFooter()), &width, &height);
                        dc->DrawText(ExpandPrintString(m_canvas->GetRightPrinterFooter()),
                            static_cast<wxCoord>(((dcWidth*scaleXReciprical) - (marginX+width))),
                            yPos);
                        }
                    }

                return true;
                }
            else return false;
            }
    private:
        /// @returns The margin around the printing area.
        [[nodiscard]] wxCoord GetMarginPadding() const
            { return 10*m_canvas->GetDPIScaleFactor(); }
        wxString ExpandPrintString(const wxString& printString) const
            {
            wxString expandedString = printString;

            // it's always just one page
            expandedString.Replace(L"@PAGENUM@", L"1", true);
            expandedString.Replace(L"@PAGESCNT@", L"1", true);

            const wxDateTime now = wxDateTime::Now();
            expandedString.Replace(L"@TITLE@", m_canvas->GetLabel(), true);
            expandedString.Replace(L"@DATE@", now.FormatDate(), true);
            expandedString.Replace(L"@TIME@", now.FormatTime(), true);

            return expandedString;
            }

        Canvas* m_canvas{ nullptr };
        };

    //------------------------------------------------------
    void Canvas::OnPrint([[maybe_unused]] wxCommandEvent& event)
        {
        CanvasPrintout* printOut = new CanvasPrintout(this, GetLabel());
    #if defined(__WXMSW__) || defined(__WXOSX__)
        wxPrinterDC* dc = nullptr;
    #else
        wxPostScriptDC* dc = nullptr;
    #endif
        if (m_printData)
            {
        #if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(*m_printData);
        #else
            dc = new wxPostScriptDC(*m_printData);
        #endif
            }
        else
            {
            wxPrintData pd;
        #if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(pd);
        #else
            dc = new wxPostScriptDC(pd);
        #endif
            }
        printOut->SetDC(dc);

        wxPrinter printer;
        if (m_printData)
            {
            printer.GetPrintDialogData().SetPrintData(*m_printData);
            }
        if (!printer.Print(this, printOut, true) )
            {
            // just show a message if a real error occurred. They may have just cancelled.
            if (printer.GetLastError() == wxPRINTER_ERROR)
                {
                wxMessageBox(_(L"An error occurred while printing.\nYour default printer may not be set correctly."),
                             _(L"Print"), wxOK|wxICON_WARNING);
                }
            }
        if (m_printData)
            {
            *m_printData = printer.GetPrintDialogData().GetPrintData();
            }
        wxDELETE(printOut);
        wxDELETE(dc);
        }

    //------------------------------------------------------
    void Canvas::OnPreview([[maybe_unused]] wxCommandEvent& event)
        {
        CanvasPrintout* printOut = new CanvasPrintout(this, GetLabel());
        CanvasPrintout* printOutForPrinting = new CanvasPrintout(this, GetLabel());
    #if defined(__WXMSW__) || defined(__WXOSX__)
        wxPrinterDC* dc = nullptr;
        wxPrinterDC* dc2 = nullptr;
    #else
        wxPostScriptDC* dc = nullptr;
        wxPostScriptDC* dc2 = nullptr;
    #endif
        if (m_printData)
            {
        #if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(*m_printData);
            dc2 = new wxPrinterDC(*m_printData);
        #else
            dc = new wxPostScriptDC(*m_printData);
            dc2 = new wxPostScriptDC(*m_printData);
        #endif
            }
        else
            {
            wxPrintData pd;
        #if defined(__WXMSW__) || defined(__WXOSX__)
            dc = new wxPrinterDC(pd);
            dc2 = new wxPrinterDC(pd);
        #else
            dc = new wxPostScriptDC(pd);
            dc2 = new wxPostScriptDC(pd);
        #endif
            }
        printOut->SetDC(dc);
        printOutForPrinting->SetDC(dc2);

        wxPrintPreview* preview = new wxPrintPreview(printOut, printOutForPrinting, m_printData);
        if (!preview->IsOk())
            {
            wxDELETE(preview); wxDELETE(dc); wxDELETE(dc2);
            wxMessageBox(_(L"An error occurred while previewing.\n"
                            "Your default printer may not be set correctly."),
                         _(L"Print Preview"), wxOK|wxICON_WARNING);
            return;
            }
        int x{0}, y{0}, width{0}, height{0};
        wxClientDisplayRect(&x, &y, &width, &height);
        wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"LPrint Preview"),
                                                   wxDefaultPosition, wxSize(width, height));

        frame->Centre(wxBOTH);
        frame->Initialize();
        frame->Show();

        delete dc; delete dc2;
        }

    //------------------------------------------------------
    void Canvas::OnContextMenu([[maybe_unused]] wxContextMenuEvent& event)
        {
        if (m_menu != nullptr)
            { PopupMenu(m_menu); }
        }

    //------------------------------------------------------
    void Canvas::OnCopy([[maybe_unused]] wxCommandEvent& event)
        {
        if (wxTheClipboard->Open())
            {
            // new bitmap to be used by memory DC
            wxBitmap canvasBitmap;
            canvasBitmap.CreateWithDIPSize(
                wxSize(ToDIP(GetCanvasRect().GetWidth()),
                       ToDIP(GetCanvasRect().GetHeight())),
                GetDPIScaleFactor());
            wxMemoryDC memDc(canvasBitmap);
            memDc.Clear();
#ifdef __WXMSW__
            wxGraphicsContext* context{ nullptr };
            auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (renderer)
                { context = renderer->CreateContext(memDc); }

            if (context)
                {
                wxGCDC gcdc(context);
                OnDraw(gcdc);
                }
            else
                {
                wxGCDC gcdc(memDc);
                OnDraw(gcdc);
                }
#else
            wxGCDC gcdc(memDc);
            OnDraw(gcdc);
#endif
            // unlock the image from the DC
            memDc.SelectObject(wxNullBitmap);

            if (wxTheClipboard->SetData(new wxBitmapDataObject(canvasBitmap)) )
                { wxTheClipboard->Close(); }
            }
        }

    //------------------------------------------------------
    void Canvas::OnSave([[maybe_unused]] wxCommandEvent& event)
        {
        const wxString openTag(L"<span style='font-weight:bold;'>");
        const wxString closeTag(L"</span><br />");
        wxArrayString choices, descriptions;
        choices.Add(L"PNG");
            descriptions.Add(openTag + _(L"Portable Network Graphic") + closeTag +
                _(L"A lossless-compressed image format. "
                   "The image can be compressed to a smaller size without any loss of quality."));
        choices.Add(L"JPG");
            descriptions.Add(openTag + _(L"Joint Photographic Experts Group") + closeTag +
                _(L"A lossy-compressed image format. Some image quality may be lost, "
                   "compared to other formats such as PNG."));
        choices.Add(L"BMP");
            descriptions.Add(openTag + _(L"Bitmap") + closeTag +
                _(L"An uncompressed raster (i.e., pixel based) image format."));
        choices.Add(L"TIFF");
            descriptions.Add(openTag + _(L"Tagged Image File Format") + closeTag +
                _(L"This format can either be compressed or uncompressed "
                   "and offers both lossy and lossless compression. "
                   "This format is generally preferred for desktop publishing."));
        choices.Add(L"TARGA");
            descriptions.Add(openTag + _(L"Truevision Advanced Raster Graphics Adapter") + closeTag +
                _(L"A raster (i.e., pixel based) image format."));
        choices.Add(L"GIF");
            descriptions.Add(openTag + _(L"Graphics Interchange Format") + closeTag +
                _(L"A raster (i.e., pixel based) image format. "
                   "Note that this image format is limited to 256 colors."));
        choices.Add(L"SVG");
            descriptions.Add(openTag + _(L"Scalable Vector Graphics") + closeTag +
                _(L"A format that uses vector (rather than raster) drawing. "
                   "Vector-based images can be scaled to much larger sizes, "
                   "without the loss of quality that raster images would experience."));
        RadioBoxDlg exportTypesDlg(this, _(L"Select Image Format"), wxEmptyString,
            _(L"Image formats:"), _(L"Export Image"),
            choices, descriptions);
        if (exportTypesDlg.ShowModal() != wxID_OK)
            { return; }
        wxString fileFilter;
        switch (exportTypesDlg.GetSelection())
            {
        case 0:
            fileFilter = L"PNG (*.png)|*.png";
            break;
        case 1:
            fileFilter = L"JPEG (*.jpg;*.jpeg;*.jpe)|*.jpg;*.jpeg;*.jpe";
            break;
        case 2:
            fileFilter = L"Bitmap (*.bmp)|*.bmp";
            break;
        case 3:
            fileFilter = L"TIFF (*.tif;*.tiff)|*.tif;*.tiff";
            break;
        case 4:
            fileFilter = L"TARGA (*.tga)|*.tga";
            break;
        case 5:
            fileFilter = L"GIF (*.gif)|*.gif";
            break;
        case 6:
            fileFilter = L"SVG (*.svg)|*.svg";
            break;
        default:
            fileFilter = L"PNG (*.png)|*.png";
            };
        wxFileDialog dialog(this, _(L"Save Image"), wxEmptyString, GetLabel(), fileFilter,
                wxFD_SAVE|wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_OK)
            { return; }

        wxFileName filePath = dialog.GetPath();
        // in case the extension is missing then use the selected filter
        if (filePath.GetExt().empty())
            {
            switch (exportTypesDlg.GetSelection())
                {
            case 0:
                filePath.SetExt(L"png");
                break;
            case 1:
                filePath.SetExt(L"jpg");
                break;
            case 2:
                filePath.SetExt(L"bmp");
                break;
            case 3:
                filePath.SetExt(L"tif");
                break;
            case 4:
                filePath.SetExt(L"tga");
                break;
            case 5:
                filePath.SetExt(L"gif");
                break;
            case 6:
                filePath.SetExt(L"svg");
                break;
            default:
                filePath.SetExt(L"png");
                };
            }

        wxFileName fn(filePath);

        // create a preview image (scale down size if on HiDPI)
        const wxCoord width = ToDIP(GetCanvasRect().GetWidth());
        const wxCoord height = ToDIP(GetCanvasRect().GetHeight());

        // new bitmap to be used by preview image
        wxBitmap previewImg;
        previewImg.CreateWithDIPSize(wxSize(width, height), GetDPIScaleFactor());
        wxMemoryDC memDc(previewImg);
        memDc.Clear();
        wxGCDC gcdc(memDc);
        OnDraw(gcdc);
        memDc.SelectObject(wxNullBitmap);

        ImageExportOptions imgOptions;
        imgOptions.m_imageSize = wxSize(width, height);

        wxString ext{ fn.GetExt() };
        ImageExportDlg optionsDlg(this, Image::GetImageFileTypeFromExtension(ext),
            previewImg, imgOptions);
        optionsDlg.SetHelpTopic(m_helpProjectPath, m_exportHelpTopic);
        if (optionsDlg.ShowModal() != wxID_OK)
            { return; }

        Save(filePath, optionsDlg.GetOptions());
        }

    //--------------------------------------------------
    bool Canvas::Save(const wxFileName& filePath, const ImageExportOptions& options)
        {
        // create the folder to the filepath, if necessary
        wxFileName::Mkdir(filePath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        wxFileName(filePath.GetFullPath()).SetPermissions(wxS_DEFAULT);

        wxCoord width = GetCanvasRect().GetWidth(), height = GetCanvasRect().GetHeight();

        const wxCoord originalWidth = width;
        const wxCoord originalHeight = height;
        // use custom size for image if supplied by caller; otherwise, just use the client size
        if (options.m_imageSize.GetWidth() > 0)
            { width = options.m_imageSize.GetWidth(); }
        if (options.m_imageSize.GetHeight() > 0)
            { height = options.m_imageSize.GetHeight(); }

        if (filePath.GetExt().CmpNoCase(L"svg") == 0)
            {
            // wxWidgets BR #22130 fixed font scaling for wxMemoryDC, but not
            // wxSVGFileDC, so need to get that fixed. Note that setting
            // the DC's window to "this" doesn't workaround it either.
            wxSVGFileDC svg(filePath.GetFullPath(),
                width, height, 72.0, GetLabel());
            svg.SetUserScale(safe_divide<double>(width, originalWidth),
                             safe_divide<double>(height, originalHeight));
            svg.SetBitmapHandler(new wxSVGBitmapEmbedHandler());
            // rescale everything to the SVG DC's scaling
            CalcAllSizes(svg);
            OnDraw(svg);
            // readjust the measurements to the canvas's DC
            wxGCDC gdc(this);
            CalcAllSizes(gdc);
            return true;
            }
        else
            {
            wxString ext{ filePath.GetExt() };
            const wxBitmapType imageType = Image::GetImageFileTypeFromExtension(ext);

            // new bitmap to be used by memory DC
            wxBitmap exportFile;
            exportFile.CreateWithDIPSize(
                wxSize(width, height),
                GetDPIScaleFactor());
            wxMemoryDC memDc(exportFile);
            memDc.Clear();
#ifdef __WXMSW__
            wxGraphicsContext* context{ nullptr };
            auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (renderer)
                { context = renderer->CreateContext(memDc); }

            if (context)
                {
                wxGCDC gcdc(context);
                OnDraw(gcdc);
                }
            else
                {
                wxGCDC gcdc(memDc);
                OnDraw(gcdc);
                }
#else
            wxGCDC gcdc(memDc);
            OnDraw(gcdc);
#endif
            // unlock the image from the DC
            memDc.SelectObject(wxNullBitmap);
            Image::SetOpacity(exportFile, wxALPHA_OPAQUE);

            // save image with contents of the DC to a file
            wxImage img(exportFile.ConvertToImage());

            // color mode
            if (options.m_mode == static_cast<decltype(options.m_mode)>(ImageExportOptions::ColorMode::Grayscale) )
                { img = img.ConvertToGreyscale(); }

            // image specific options
            if (imageType == wxBITMAP_TYPE_TIF)
                { img.SetOption(wxIMAGE_OPTION_COMPRESSION, static_cast<int>(options.m_tiffCompression)); }
            else if (imageType == wxBITMAP_TYPE_JPEG)
                { img.SetOption(wxIMAGE_OPTION_QUALITY, 100); }
            else if (imageType == wxBITMAP_TYPE_PNG)
                { img.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9); } // max compression
            else if (imageType == wxBITMAP_TYPE_GIF)
                {
                // "dumb" image down to 256 colors
                wxQuantize::Quantize(img, img, 256);
                img.ConvertAlphaToMask();
                // use the comment field too
                img.SetOption(wxIMAGE_OPTION_GIF_COMMENT, GetLabel());
                }

            if (!img.SaveFile(filePath.GetFullPath(), imageType))
                {
                wxMessageBox(wxString::Format(_(L"Failed to save image\n(%s)."), filePath.GetFullPath()),
                    _(L"Save Error"), wxOK|wxICON_EXCLAMATION);
                return false;
                }
            return true;
            }
        }

    //------------------------------------------
    Canvas::Canvas(wxWindow* parent, int itemId,
                const wxPoint& pos,
                const wxSize& size,
                const long flags)
        : wxScrolledWindow(parent, itemId, pos, size,
                           flags|wxBORDER_NONE|wxVSCROLL|wxHSCROLL|wxFULL_REPAINT_ON_RESIZE)
        {
        m_dpiScaleFactor = GetDPIScaleFactor();
        m_watermarkFont.MakeBold();
        m_canvasMinWidth = GetDefaultCanvasWidth();
        m_canvasMinHeight = GetDefaultCanvasHeight();
        SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        SetBackgroundColour(*wxWHITE);
        SetScrollbars(10, 10, 0, 0);
        SetVirtualSize(size);
        wxGCDC gdc(this);
        CalcAllSizes(gdc);

        Bind(wxEVT_MENU,
            [this]([[maybe_unused]] wxCommandEvent&)
                { ZoomIn(); },
            wxID_ZOOM_IN);

        Bind(wxEVT_MENU,
            [this]([[maybe_unused]] wxCommandEvent&)
                { ZoomOut(); },
            wxID_ZOOM_OUT);

        Bind(wxEVT_MENU,
            [this]([[maybe_unused]] wxCommandEvent&)
                { ZoomReset(); },
            wxID_ZOOM_FIT);

        Bind(wxEVT_KEY_DOWN, &Canvas::OnKeyDown, this);
        Bind(wxEVT_PAINT, &Canvas::OnPaint, this);
        Bind(wxEVT_SIZE, &Canvas::OnResize, this);
        Bind(wxEVT_CONTEXT_MENU, &Canvas::OnContextMenu, this);
        Bind(wxEVT_MENU, &Canvas::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &Canvas::OnCopy, this, wxID_COPY);
        Bind(wxEVT_MENU, &Canvas::OnPreview, this, wxID_PREVIEW);
        Bind(wxEVT_MENU, &Canvas::OnPrint, this, wxID_PRINT);
        // numerous mouse events
        Bind(wxEVT_LEFT_DOWN, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_LEFT_UP, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MIDDLE_DOWN, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MIDDLE_UP, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_RIGHT_DOWN, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_RIGHT_UP, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MOTION, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_ENTER_WINDOW, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_LEAVE_WINDOW, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_LEFT_DCLICK, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MIDDLE_DCLICK, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_RIGHT_DCLICK, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MOUSEWHEEL, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX1_DOWN, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX1_UP, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX1_DCLICK, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX2_DOWN, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX2_UP, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_AUX2_DCLICK, &Canvas::OnMouseEvent, this);
        Bind(wxEVT_MAGNIFY, &Canvas::OnMouseEvent, this);
        }

    long Canvas::CalcLeftTitles(wxDC& dc, const long spacingWidth)
        {
        long leftMarginWidth{ 0 };
        // add the left titles
        for (auto& title : m_leftTitles)
            {
            title.SetDPIScaleFactor(m_dpiScaleFactor);
            title.SetScaling(GetScaling());
            title.SetTextOrientation(Orientation::Vertical);
            const wxCoord textWidth = (title.GetAnchoring() == Anchoring::BottomLeftCorner ||
                    title.GetAnchoring() == Anchoring::TopLeftCorner) ? 0 :
                (title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                    title.GetBoundingBox(dc).GetWidth();
            title.SetAnchorPoint(
                // lined up to the left (bottom) of the canvas
                // (need to adjust for anchoring so that it doesn't go off the canvas)
                title.GetRelativeAlignment() == RelativeAlignment::FlushLeft ? wxPoint(leftMarginWidth+textWidth,
                    GetCanvasRect().GetHeight() -
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 :
                        (title.GetAnchoring() == Anchoring::TopLeftCorner ||
                         title.GetAnchoring() == Anchoring::TopRightCorner) ?
                        title.GetBoundingBox(dc).GetHeight() : 0)) :
                // lined up to the right (top)
                title.GetRelativeAlignment() == RelativeAlignment::FlushRight ? wxPoint(leftMarginWidth+textWidth,
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 :
                        (title.GetAnchoring() == Anchoring::BottomLeftCorner ||
                         title.GetAnchoring() == Anchoring::BottomRightCorner) ?
                        title.GetBoundingBox(dc).GetHeight() : 0)) :
                // centered (note that anchoring will actually be applied here)
                wxPoint(leftMarginWidth+textWidth, (GetCanvasRect().GetHeight()/2)) );
            leftMarginWidth += title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            GetTitles().emplace_back(std::make_shared<GraphItems::Label>(title));
            }
        return leftMarginWidth;
        }

    long Canvas::CalcRightTitles(wxDC& dc, const long spacingWidth)
        {
        long rightMarginWidth{ 0 };
        long position = GetCanvasRect().GetWidth() - spacingWidth;
        // add the right titles
        for (auto& title : m_rightTitles)
            {
            title.SetDPIScaleFactor(m_dpiScaleFactor);;
            title.SetScaling(GetScaling());
            title.SetTextOrientation(Orientation::Vertical);
            const wxCoord textWidth = (title.GetAnchoring() == Anchoring::BottomRightCorner ||
                    title.GetAnchoring() == Anchoring::TopRightCorner) ? 0 :
                (title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                    title.GetBoundingBox(dc).GetWidth();
            title.SetAnchorPoint(
                // lined up to the left (bottom) of the canvas
                // (need to adjust for anchoring so that it doesn't go off the canvas)
                title.GetRelativeAlignment() == RelativeAlignment::FlushLeft ? wxPoint(position-textWidth,
                    GetCanvasRect().GetHeight() -
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 :
                        (title.GetAnchoring() == Anchoring::TopLeftCorner ||
                         title.GetAnchoring() == Anchoring::TopRightCorner) ?
                        title.GetBoundingBox(dc).GetHeight() : 0)) :
                // lined up to the right (top)
                title.GetRelativeAlignment() == RelativeAlignment::FlushRight ? wxPoint(position-textWidth,
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 :
                        (title.GetAnchoring() == Anchoring::BottomLeftCorner ||
                         title.GetAnchoring() == Anchoring::BottomRightCorner) ?
                        title.GetBoundingBox(dc).GetHeight() : 0)) :
                // centered (note that anchoring will actually be applied here)
                wxPoint(position-textWidth, (GetCanvasRect().GetHeight()/2)) );
            position -= title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            rightMarginWidth += title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            GetTitles().emplace_back(std::make_shared<GraphItems::Label>(title));
            }
        return rightMarginWidth;
        }

    long Canvas::CalcTopTitles(wxDC& dc, const long spacingWidth)
        {
        long topMarginWidth{ 0 };
        // add the top titles
        for (auto& title : m_topTitles)
            {
            title.SetDPIScaleFactor(m_dpiScaleFactor);;
            title.SetScaling(GetScaling());
            const wxCoord textHeight = (title.GetAnchoring() == Anchoring::BottomLeftCorner ||
                title.GetAnchoring() == Anchoring::BottomRightCorner) ? title.GetBoundingBox(dc).GetHeight() :
                (title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 : 0;
            title.SetAnchorPoint(
                // lined up to the left of the canvas
                // (need to adjust for anchoring so that it doesn't go off the canvas)
                title.GetRelativeAlignment() == RelativeAlignment::FlushLeft ? wxPoint(
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                      (title.GetAnchoring() == Anchoring::TopRightCorner ||
                       title.GetAnchoring() == Anchoring::BottomRightCorner) ?
                        title.GetBoundingBox(dc).GetWidth() : 0), topMarginWidth+textHeight) :
                // lined up to the right
                title.GetRelativeAlignment() == RelativeAlignment::FlushRight ? wxPoint(GetCanvasRect().GetWidth() -
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                      (title.GetAnchoring() == Anchoring::TopLeftCorner ||
                       title.GetAnchoring() == Anchoring::BottomLeftCorner) ?
                        title.GetBoundingBox(dc).GetWidth() : 0), topMarginWidth+textHeight) :
                // centered (note that anchoring will actually be applied here)
                wxPoint((GetCanvasRect().GetWidth()/2), topMarginWidth+textHeight));
            topMarginWidth += title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            GetTitles().emplace_back(std::make_shared<GraphItems::Label>(title));
            }
        return topMarginWidth;
        }

    long Canvas::CalcBottomTitles(wxDC& dc, const long spacingWidth)
        {
        long bottomMarginWidth{ 0 };
        long position = GetCanvasRect().GetHeight() - spacingWidth;
        // add the bottom titles
        for (auto& title : m_bottomTitles)
            {
            title.SetDPIScaleFactor(m_dpiScaleFactor);;
            title.SetScaling(GetScaling());
            const wxCoord textHeight = (title.GetAnchoring() == Anchoring::TopLeftCorner ||
                title.GetAnchoring() == Anchoring::TopRightCorner) ? title.GetBoundingBox(dc).GetHeight() :
                (title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetHeight()/2 : 0;
            title.SetAnchorPoint(
                // lined up to the left of the canvas
                // (need to adjust for anchoring so that it doesn't go off the canvas)
                title.GetRelativeAlignment() == RelativeAlignment::FlushLeft ? wxPoint(
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                      (title.GetAnchoring() == Anchoring::TopRightCorner ||
                       title.GetAnchoring() == Anchoring::BottomRightCorner) ?
                        title.GetBoundingBox(dc).GetWidth() : 0), position-textHeight) :
                // lined up to the right
                title.GetRelativeAlignment() == RelativeAlignment::FlushRight ? wxPoint(GetCanvasRect().GetWidth() -
                    ((title.GetAnchoring() == Anchoring::Center) ? title.GetBoundingBox(dc).GetWidth()/2 :
                      (title.GetAnchoring() == Anchoring::TopLeftCorner ||
                       title.GetAnchoring() == Anchoring::BottomLeftCorner) ?
                        title.GetBoundingBox(dc).GetWidth() : 0), position-textHeight) :
                // centered (note that anchoring will actually be applied here)
                wxPoint((GetCanvasRect().GetWidth()/2),position-textHeight));
            position -= title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            bottomMarginWidth += title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            GetTitles().emplace_back(std::make_shared<GraphItems::Label>(title));
            }
        return bottomMarginWidth;
        }

    void Canvas::OnResize([[maybe_unused]] wxSizeEvent& event)
        {
        //if the new size is larger than the canvas itself, then turn off zooming.
        if (GetClientRect().GetWidth() > GetCanvasRect().GetWidth() &&
            GetClientRect().GetHeight() > GetCanvasRect().GetHeight())
            { m_zoomLevel = 0; }
        //don't resize if canvas is zoomed into
        if (m_zoomLevel <= 0)
            {
            m_rect = GetClientRect();
            wxGCDC gdc(this);
            CalcAllSizes(gdc);
            SetVirtualSize(GetCanvasRect().GetSize());
            }
        }

    void Canvas::CalcAllSizes(wxDC& dc)
        {
        wxASSERT_MSG(
            (std::accumulate(m_rowProportions.cbegin(), m_rowProportions.cend(), 0.0)) <= 1,
            "Canvas row proportions are more than 100%!");

        /* The rendering area must have a minimum size of 700x500;
           otherwise, it will be crunched up and look bad.*/
        wxSize CanvasMinSize = GetCanvasRect().GetSize();
        CanvasMinSize.SetWidth(std::max(GetCanvasMinWidth(), CanvasMinSize.GetWidth()));
        CanvasMinSize.SetHeight(std::max(GetCanvasMinHeight(), CanvasMinSize.GetHeight()));
        m_rect.SetSize(CanvasMinSize);

        const wxCoord titleSpacingWidth = ScaleToScreenAndCanvas(2);

        // calculate the left/right margins around the canvas and construct the titles
        GetTitles().clear();
        const long leftBorder = CalcLeftTitles(dc, titleSpacingWidth);
        const long topBorder = CalcTopTitles(dc, titleSpacingWidth);
        const long bottomBorder = CalcBottomTitles(dc, titleSpacingWidth);
        const long rightBorder = CalcRightTitles(dc, titleSpacingWidth);

        wxRect fixedObjectRect = GetCanvasRect();
        fixedObjectRect.x += leftBorder;
        fixedObjectRect.y += topBorder;
        fixedObjectRect.SetWidth(fixedObjectRect.GetWidth()-(leftBorder+rightBorder));
        fixedObjectRect.SetHeight(fixedObjectRect.GetHeight()-(topBorder+bottomBorder));

        // reset all objects' canvas alignments and DPI scaling
        for (auto fixedObjectsRowPos = GetFixedObjects().begin();
            fixedObjectsRowPos != GetFixedObjects().end();
            ++fixedObjectsRowPos)
            {
            for (auto objectsPos = fixedObjectsRowPos->begin();
                objectsPos != fixedObjectsRowPos->end();
                ++objectsPos)
                {
                if ((*objectsPos) != nullptr)
                    {
                    (*objectsPos)->SetContentTop(std::nullopt);
                    (*objectsPos)->SetContentBottom(std::nullopt);
                    (*objectsPos)->SetContentLeft(std::nullopt);
                    (*objectsPos)->SetContentRight(std::nullopt);
                    (*objectsPos)->SetDPIScaleFactor(m_dpiScaleFactor);
                    }
                }
            }

        size_t rowHeightOffset(0);
        // go through each row of items (e.g., subplots) and resize and move them their its grid area
        for (auto fixedObjectsRowPos = GetFixedObjects().begin();
             fixedObjectsRowPos != GetFixedObjects().end();
             ++fixedObjectsRowPos)
            {
            wxASSERT_MSG(std::distance(GetFixedObjects().begin(), fixedObjectsRowPos) <
                static_cast<ptrdiff_t>(m_rowProportions.size()), "Canvas row proportions size is wrong!");
            const size_t objectHeight = fixedObjectRect.GetHeight() *
                m_rowProportions.at(std::distance(GetFixedObjects().begin(), fixedObjectsRowPos));
            size_t currentXPos{ 0 };
            for (auto& objectsPos : *fixedObjectsRowPos)
                {
                if (objectsPos != nullptr)
                    {
                    // set the scaling from the canvas and get the bounding box for it to fit in
                    objectsPos->SetScaling(GetScaling());
                    const auto currentObjHeight = objectsPos->GetCanvasHeightProportion() ?
                        objectsPos->GetCanvasHeightProportion().value() * fixedObjectRect.GetHeight() :
                        objectHeight;
                    wxRect boundingRect(wxPoint(fixedObjectRect.x + currentXPos,
                                                fixedObjectRect.y + rowHeightOffset),
                        wxSize(fixedObjectRect.GetWidth() * objectsPos->GetCanvasWidthProportion(),
                               currentObjHeight));
                    const wxRect nonPaddedBoundingRect{ boundingRect };
                    // adjust for margins
                    boundingRect.y += ScaleToScreenAndCanvas(objectsPos->GetTopCanvasMargin());
                    boundingRect.x += ScaleToScreenAndCanvas(objectsPos->GetLeftCanvasMargin());
                    boundingRect.SetWidth(std::max<double>(0, boundingRect.GetWidth() -
                        ScaleToScreenAndCanvas(objectsPos->GetLeftCanvasMargin()) -
                        ScaleToScreenAndCanvas(objectsPos->GetRightCanvasMargin())) );
                    boundingRect.SetHeight(std::max<double>(0, boundingRect.GetHeight() -
                        ScaleToScreenAndCanvas(objectsPos->GetTopCanvasMargin()) -
                        ScaleToScreenAndCanvas(objectsPos->GetBottomCanvasMargin())) );

                    objectsPos->SetBoundingBox(boundingRect, dc, GetScaling());
                    currentXPos += nonPaddedBoundingRect.GetWidth();

                    objectsPos->RecalcSizes(dc);
                    objectsPos->UpdateSelectedItems();
                    }
                }
            if (IsRowContentAligned())
                {
                for (auto fixedObjectsRowItems = GetFixedObjects().begin();
                     fixedObjectsRowItems != GetFixedObjects().end();
                     ++fixedObjectsRowItems)
                    {
                    std::vector<wxCoord> topPoints;
                    std::vector<wxCoord> bottomPoints;
                    for (auto& objectsPos : *fixedObjectsRowItems)
                        {
                        if (objectsPos != nullptr &&
                            !objectsPos->GetContentRect().IsEmpty())
                            {
                            topPoints.emplace_back(objectsPos->GetContentRect().GetTop());
                            bottomPoints.emplace_back(objectsPos->GetContentRect().GetBottom());
                            }
                        }
                    if (topPoints.size() && bottomPoints.size())
                        {
                        const auto topPt = *std::max_element(topPoints.cbegin(), topPoints.cend());
                        const auto bottomPt = *std::min_element(bottomPoints.cbegin(), bottomPoints.cend());
                        for (auto& objectsPos : *fixedObjectsRowItems)
                            {
                            if (objectsPos != nullptr &&
                                !objectsPos->GetContentRect().IsEmpty())
                                {
                                objectsPos->SetContentTop(topPt);
                                objectsPos->SetContentBottom(bottomPt);
                                objectsPos->RecalcSizes(dc);
                                objectsPos->UpdateSelectedItems();
                                }
                            }
                        }
                    }
                }
            rowHeightOffset += objectHeight;
            }

        if (IsColumnContentAligned())
            {
            if (GetFixedObjects().size() && GetFixedObjects().at(0).size())
                {
                bool noMoreRows{ false };
                const auto& topRow = GetFixedObjects().at(0);
                for (size_t i = 0; i < topRow.size(); ++i)
                    {
                    std::vector<wxCoord> leftPoints;
                    std::vector<wxCoord> rightPoints;
                    // go through each row and adjust the current column
                    for (auto fixedObjectsRowPos = GetFixedObjects().begin();
                            fixedObjectsRowPos != GetFixedObjects().end();
                            ++fixedObjectsRowPos)
                        {
                        if (fixedObjectsRowPos->size() == (i + 1))
                            {
                            noMoreRows = true;
                            break;
                            }
                        auto& objectPos = fixedObjectsRowPos->at(i);
                        if (objectPos != nullptr &&
                            !objectPos->GetContentRect().IsEmpty())
                            {
                            leftPoints.emplace_back(objectPos->GetContentRect().GetLeft());
                            rightPoints.emplace_back(objectPos->GetContentRect().GetRight());
                            }
                        }
                    // the grid is jagged, so stop aligning the columns
                    if (noMoreRows)
                        { break; }
                    if (leftPoints.size() && rightPoints.size())
                        {
                        const auto leftPt = *std::max_element(leftPoints.cbegin(), leftPoints.cend());
                        const auto rightPt = *std::min_element(rightPoints.cbegin(), rightPoints.cend());
                        for (auto fixedObjectsRowPos = GetFixedObjects().begin();
                             fixedObjectsRowPos != GetFixedObjects().end();
                             ++fixedObjectsRowPos)
                            {
                            auto& objectPos = fixedObjectsRowPos->at(i);
                            if (objectPos != nullptr &&
                                !objectPos->GetContentRect().IsEmpty())
                                {
                                objectPos->SetContentLeft(leftPt);
                                objectPos->SetContentRight(rightPt);
                                objectPos->RecalcSizes(dc);
                                objectPos->UpdateSelectedItems();
                                }
                            }
                        }
                    }
                }
            }

        SetVirtualSize(GetCanvasRect().GetSize());
        }

    //---------------------------------------------------
    std::pair<size_t,size_t> Canvas::GetFixedObjectsGridSize() const
        {
        std::pair<size_t,size_t> result(0,0);
        result.first = m_fixedObjects.size();
        if (m_fixedObjects.size())
            { result.second = m_fixedObjects[0].size(); }
        return result;
        }

    //---------------------------------------------------
    void Canvas::SetFixedObjectsGridSize(const size_t rows, const size_t columns)
        {
        m_fixedObjects.resize(rows);
        for (auto pos = m_fixedObjects.begin();
             pos != m_fixedObjects.end();
             ++pos)
            { pos->resize(columns, nullptr); }

        // a full reset is needed
        m_rowProportions.clear();
        m_rowProportions.resize(rows, safe_divide<double>(1.0, rows));
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::GraphItemBase> Canvas::GetFixedObject(const size_t row, const size_t column)
        {
        wxASSERT(GetFixedObjects().size());
        wxASSERT(row < GetFixedObjects().size());
        wxASSERT(column < GetFixedObjects().at(0).size());
        if (GetFixedObjects().size() == 0 ||
            row >= GetFixedObjects().size() ||
            column >= GetFixedObjects().at(0).size())
            { return nullptr; }
        return GetFixedObjects().at(row).at(column);
        }

    //---------------------------------------------------
    const std::shared_ptr<GraphItems::GraphItemBase> Canvas::GetFixedObject(const size_t row, const size_t column) const
        {
        wxASSERT(GetFixedObjects().size());
        wxASSERT(row < GetFixedObjects().size());
        wxASSERT(column < GetFixedObjects().at(0).size());
        if (GetFixedObjects().size() == 0 ||
            row >= GetFixedObjects().size() ||
            column >= GetFixedObjects().at(0).size())
            { return nullptr; }
        return GetFixedObjects().at(row).at(column);
        }

    //---------------------------------------------------
    void Canvas::SetFixedObject(const size_t row, const size_t column,
                                std::shared_ptr<GraphItems::GraphItemBase> object)
        {
        wxASSERT(object);
        wxASSERT(GetFixedObjects().size());
        wxASSERT(column < GetFixedObjects().at(0).size());
        if (GetFixedObjects().size() == 0 ||
            row >= GetFixedObjects().size() ||
            column >= GetFixedObjects().at(0).size())
            { return; }
        if (object)
            { object->SetDPIScaleFactor(m_dpiScaleFactor); }
        GetFixedObjects().at(row).at(column) = object;
        // how much of the canvas is being consumed by the row
        // that this item was just added to
        double totalPercent = std::accumulate(GetFixedObjects().at(row).cbegin(),
                                              GetFixedObjects().at(row).cend(), 0.0f,
            [](const auto initVal, const auto& item) noexcept
            { return initVal + (item == nullptr ? 0 : item->GetCanvasWidthProportion()); });
        // if more than 100%, then we need to trim the other items in the row
        if (totalPercent > 1)
            {
            const double totalDiff{ totalPercent - 1.0 };
            // this is the only object in the row, but it was set over 100%
            // for some odd reason, so set it to 100%
            if (GetFixedObjects().at(row).size() == 1 &&
                GetFixedObjects().at(row).at(0) != nullptr)
                {
                GetFixedObjects().at(row).at(0)->SetCanvasWidthProportion(1.0);
                }
            // a large object and there are other large objects, then
            // resize everything to fit
            else if (object && object->GetCanvasWidthProportion() > .5)
                {
                const double trimPercent{ safe_divide<double>(totalDiff,
                                            (GetFixedObjects().at(row).size())) };
                for (size_t item = 0; item < GetFixedObjects().at(row).size(); ++item)
                    {
                    auto& currentItem = GetFixedObjects().at(row).at(item);
                    if (currentItem != nullptr)
                        {
                        currentItem->SetCanvasWidthProportion(
                            currentItem->GetCanvasWidthProportion()-trimPercent);
                        }
                    }
                }
            // otherwise, if a smaller object (e.g., a legend), then keep
            // its size and shrink everything else
            else
                {
                // leave the percentage width of the current item the same,
                // but evenly shrink everything else in the row to get it
                // down to 100%
                const double trimPercent{ safe_divide<double>(totalDiff,
                                            (GetFixedObjects().at(row).size()-1)) };
                for (size_t item = 0; item < GetFixedObjects().at(row).size(); ++item)
                    {
                    auto& currentItem = GetFixedObjects().at(row).at(item);
                    if (item != column && currentItem != nullptr)
                        {
                        currentItem->SetCanvasWidthProportion(
                            currentItem->GetCanvasWidthProportion()-trimPercent);
                        }
                    }
                }
            }
        }

    // override the paint event so that we can use double buffering
    //---------------------------------------------------
    void Canvas::OnPaint([[maybe_unused]] wxPaintEvent& event)
        {
#ifdef __WXMSW__
        wxAutoBufferedPaintDC pdc(this);
        pdc.Clear();
        wxGraphicsContext* context{ nullptr };
        auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (renderer)
            { context = renderer->CreateContext(pdc); }

        if (context)
            {
            wxGCDC dc(context);
            PrepareDC(dc);
            OnDraw(dc);
            }
        else
            {
            wxGCDC dc(pdc);
            PrepareDC(dc);
            OnDraw(dc);
            }
#else
        wxAutoBufferedPaintDC pdc(this);
        pdc.Clear();
        wxGCDC dc(pdc);
        PrepareDC(dc);
        OnDraw(dc);
#endif
        }

    //-------------------------------------------
    void Canvas::OnDraw(wxDC& dc)
        {
        m_dpiScaleFactor = dc.GetDPIScaleFactor();

        dc.Clear();
        // fill in the background color with a linear gradient (if there is a user defined color)
        if (m_bgColorUseLinearGradient && GetBackgroundColor().IsOk())
            { dc.GradientFillLinear(GetCanvasRect(), GetBackgroundColor(), *wxWHITE, wxSOUTH); }
        else
            {
            //if background color is bad, then just fill the canvas with white. Otherwise, fill with color
            wxDCBrushChanger bc(dc, !GetBackgroundColor().IsOk() ? *wxWHITE_BRUSH : wxBrush(GetBackgroundColor()) );
            dc.DrawRectangle(GetCanvasRect());
            }

        // fill in the background image (if there is one)
        if (GetBackgroundImage().IsOk() && m_bgOpacity != wxALPHA_TRANSPARENT)
            {
            GetBackgroundImage().SetDPIScaleFactor(m_dpiScaleFactor);;
            GetBackgroundImage().SetAnchoring(Anchoring::Center);
            GetBackgroundImage().SetAnchorPoint(
                wxPoint(GetCanvasRect().GetLeft() + safe_divide(GetCanvasRect().GetWidth(), 2),
                        GetCanvasRect().GetTop() + safe_divide(GetCanvasRect().GetHeight(), 2)));
            // we clip the image a little so that it fits the area better
            GetBackgroundImage().SetBestSize(GetCanvasRect().GetSize() +
                                             wxSize(100*m_dpiScaleFactor, 100*m_dpiScaleFactor));
            GetBackgroundImage().SetOpacity(m_bgOpacity);
            GetBackgroundImage().Draw(dc);
            }

        // draw the actual objects on the canvas
        for (const auto& fixedObjectsRow : GetFixedObjects())
            {
            for (const auto& objectPtr : fixedObjectsRow)
                {
                if (objectPtr != nullptr)
                    { objectPtr->Draw(dc); }
                }
            }

        // draw the titles
        for (const auto& title : GetTitles())
            {
            if (title != nullptr)
                { title->Draw(dc); }
            }

        // draw the movable objects (these sit on top of everything else)
        for (auto& objectPtr : GetFreeFloatingObjects())
            {
            objectPtr->SetScaling(GetScaling());
            objectPtr->Draw(dc);
            }

        // show a label on top of the selected items
        for (const auto& fixedObjectsRow : GetFixedObjects())
            {
            for (const auto& objectPtr : fixedObjectsRow)
                {
                if (objectPtr)
                    { objectPtr->DrawSelectionLabel(dc, GetScaling()); }
                }
            }

        DrawWatermarkLogo(dc);

        // draw label
            {
            wxDCFontChanger fc(dc, m_watermarkFont);
            DrawWatermarkLabel(dc, GetCanvasRect(),
                WaterMark{ GetWatermark(),
                ColorBrewer::GetColor(Color::Red, Settings::GetTranslucencyValue()),
                WatermarkDirection::Diagonal } );
            }
        }

    //-------------------------------------------
    void Canvas::SetBackgroundImage(GraphItems::Image& backgroundImage,
                                    const uint8_t opacity /*= wxALPHA_OPAQUE*/) noexcept
        {
        m_bgImage = backgroundImage;
        m_bgOpacity = opacity;
        }

    //-------------------------------------------
    wxString Canvas::GetWatermark() const
        {
        wxString watermark = m_watermark;
        watermark.Replace(L"@[DATE]", wxDateTime().Now().FormatDate());
        watermark.Replace(L"@[TIME]", wxDateTime().Now().FormatTime());
        watermark.Replace(L"@[DATETIME]", wxDateTime().Now().FormatDate() + L" " +
            wxDateTime().Now().FormatTime());
        return watermark;
        }

    //-------------------------------------------
    void Canvas::DrawWatermarkLogo(wxDC& dc)
        {
        if (GetCanvasRect().GetWidth() == 0 || GetCanvasRect().GetHeight() == 0)
            { return; }

        if (m_watermarkImg.IsOk())
            {
            m_watermarkImg.SetBestSize(wxSize(ScaleToScreenAndCanvas(100),
                                              ScaleToScreenAndCanvas(100)));
            // make logo image mildly translucent
            // (twice as opaque as the system translucency).
            m_watermarkImg.SetOpacity(Settings::GetTranslucencyValue()*2);
            m_watermarkImg.SetAnchoring(Anchoring::BottomRightCorner);
            m_watermarkImg.SetAnchorPoint(wxPoint(GetCanvasRect().GetWidth(),
                                                  GetCanvasRect().GetHeight()));
            m_watermarkImg.Draw(dc);
            }
        }

    //-------------------------------------------
    void Canvas::DrawWatermarkLabel(wxDC& dc, const wxRect drawingRect, const WaterMark& watermark)
        {
        wxDCTextColourChanger cc(dc, watermark.m_color);

        if (drawingRect.GetWidth() == 0 || drawingRect.GetHeight() == 0)
            { return; }

        if (watermark.m_label.length())
            {
            wxCoord labelWidth{ 0 }, labelHeight{ 0 };
            if (watermark.m_direction == WatermarkDirection::Diagonal)
                {
                const double angle = std::atan(safe_divide<double>(drawingRect.GetHeight(),
                                                                   drawingRect.GetWidth())) * (180 / M_PI);

                // set the font size so that the text will fit diagonally
                wxFont labelFont = dc.GetFont();
                labelFont.SetPointSize(Label::CalcDiagonalFontSize(dc, labelFont, drawingRect, angle, watermark.m_label));
                labelFont.MakeBold();
                wxDCFontChanger fc(dc, labelFont);

                dc.GetMultiLineTextExtent(watermark.m_label, &labelWidth, &labelHeight);

                const float widthOfWatermark = labelWidth*std::abs(std::cos(geometry::degrees_to_radians(angle))) -
                    labelHeight*std::abs(std::sin(geometry::degrees_to_radians(angle)));
                const float heightOfWatermark = labelWidth*std::abs(std::sin(geometry::degrees_to_radians(angle))) +
                    labelHeight*std::abs(std::cos(geometry::degrees_to_radians(angle)));

                std::negate<double> neg;
                dc.DrawRotatedText(watermark.m_label,
                    (drawingRect.GetWidth()/2) - static_cast<wxCoord>(widthOfWatermark/2),
                    (drawingRect.GetHeight()/2) - static_cast<wxCoord>(heightOfWatermark/2),
                    neg(angle));
                }
            else
                {
                wxFont labelFont = dc.GetFont();
                labelFont.SetPointSize(
                    Label::CalcFontSizeToFitBoundingBox(dc, labelFont, drawingRect, watermark.m_label));
                labelFont.MakeBold();
                wxDCFontChanger fc(dc, labelFont);

                dc.GetMultiLineTextExtent(watermark.m_label, &labelWidth, &labelHeight);
                dc.DrawText(watermark.m_label,
                    wxPoint((drawingRect.GetWidth()/2) - (labelWidth/2),
                            (drawingRect.GetHeight()/2) - (labelHeight/2)));
                }
            }
        }

    //-------------------------------------------
    void Canvas::OnMouseEvent(wxMouseEvent& event)
        {
        static DragMode dragMode = DragMode::DraggingNone;
        static wxPoint dragStartPos;
        static std::shared_ptr<GraphItems::GraphItemBase> currentlyDraggedShape;
        wxPoint unscrolledPosition;
        CalcUnscrolledPosition(event.GetPosition().x, event.GetPosition().y,
                               &unscrolledPosition.x, &unscrolledPosition.y);
        const wxCoord refreshPadding = ScaleToScreenAndCanvas(10);

        wxGCDC gdc(this);

        if (event.LeftDown())
            {
            wxASSERT_LEVEL_2_MSG(currentlyDraggedShape == nullptr,
                                 "Item being dragged should be null upon left mouse down!");
            // unselect any selected items (if Control/Command isn't held down),
            // as we are now selecting (and possibly dragging) something else.
            if (!wxGetMouseState().CmdDown())
                {
                for (auto& polygonPtr : GetFreeFloatingObjects())
                    {
                    if (polygonPtr && polygonPtr->IsSelected())
                        { polygonPtr->SetSelected(false); }
                    }
                for (auto& fixedObjectsRow : GetFixedObjects())
                    {
                    for (auto& objectPtr : fixedObjectsRow)
                        {
                        if (objectPtr != nullptr)
                            { objectPtr->ClearSelections(); }
                        }
                    }
                for (auto& title : GetTitles())
                    {
                    if (title != nullptr && title->IsSelected())
                        { title->SetSelected(false); }
                    }
                }
            //see if a movable object is being selected.
            if (auto movableObjectsPos = FindFreeFloatingObject(unscrolledPosition, gdc);
                movableObjectsPos != GetFreeFloatingObjects().rend())
                {
                //We tentatively start dragging, but wait for
                //mouse movement before dragging properly.
                dragMode = DragMode::DragStart;
                dragStartPos = unscrolledPosition;
                (*movableObjectsPos)->SetSelected(!(*movableObjectsPos)->IsSelected());
                Refresh(true);
                Update();
                m_dragImage.reset(new wxDragImage((*movableObjectsPos)->ToBitmap(), wxCursor(wxCURSOR_HAND)));
                (*movableObjectsPos)->SetInDragState(true);
                currentlyDraggedShape = (*movableObjectsPos);
                event.Skip();
                return;//we have our selection, so bail before hit testing everything else
                }
            else
                {
                wxASSERT_LEVEL_2_MSG(currentlyDraggedShape == nullptr,
                                     "Item being dragged should be null upon left mouse down!");
                currentlyDraggedShape = nullptr;
                }
            //or the fixed items connected to the canvas's grid
            for (auto fixedObjectsRowPos = GetFixedObjects().begin();
                fixedObjectsRowPos != GetFixedObjects().end();
                ++fixedObjectsRowPos)
                {
                for (auto fixedObjectsPos = fixedObjectsRowPos->begin();
                     fixedObjectsPos != fixedObjectsRowPos->end();
                     ++fixedObjectsPos)
                    {
                    if ((*fixedObjectsPos) && (*fixedObjectsPos)->SelectObjectAtPoint(unscrolledPosition, gdc))
                        {
                        Refresh(true);
                        Update();
                        event.Skip();
                        return;
                        }
                    }
                }
            for (const auto& title : GetTitles())
                {
                if (title != nullptr && title->SelectObjectAtPoint(unscrolledPosition, gdc))
                    {
                    Refresh(true);
                    Update();
                    event.Skip();
                    return;
                    }
                }
            Refresh(true);
            Update();
            event.Skip();
            }
        else if (event.LeftUp() && dragMode != DragMode::DraggingNone)
            {
            // finished dragging
            dragMode = DragMode::DraggingNone;

            wxASSERT_LEVEL_2_MSG(currentlyDraggedShape,
                                 "Drag image is null while mouse up, although drag mode isn't set to none!");
            if (m_dragImage != nullptr)
                {
                m_dragImage->Hide();
                m_dragImage->EndDrag();
                m_dragImage = nullptr;
                }

            wxASSERT_LEVEL_2_MSG(currentlyDraggedShape,
                                 "Item being dragged is null while mouse up, although drag mode isn't set to none!");
            if (currentlyDraggedShape)
                {
                const wxPoint movePt(unscrolledPosition-dragStartPos);
                currentlyDraggedShape->Offset(movePt.x, movePt.y);
                currentlyDraggedShape->SetInDragState(false);
                wxRect boundingBox(currentlyDraggedShape->GetBoundingBox().Inflate(refreshPadding));
                boundingBox.Offset(event.GetPosition()-unscrolledPosition);
                currentlyDraggedShape = nullptr;
                Refresh(true, &boundingBox);
                }
            }
        else if (event.Dragging() && dragMode != DragMode::DraggingNone)
            {
            wxASSERT_LEVEL_2_MSG(currentlyDraggedShape,
                                 "Item being dragged is null while mouse drag, although drag mode isn't set to none!");
            if (dragMode == DragMode::DragStart && currentlyDraggedShape)
                {
                dragStartPos = unscrolledPosition;

                // start the drag
                dragMode = DragMode::Dragging;

                // redraw the item being dragged
                // (we refresh a few pixels around the object to prevent any shearing)
                wxRect boundingBox(currentlyDraggedShape->GetBoundingBox().Inflate(refreshPadding));
                boundingBox.Offset(event.GetPosition()-unscrolledPosition);
                Refresh(true, &boundingBox);
                Update();

                // the offset between the top-left of the shape image and the current shape position
                const wxPoint beginDragHotSpot = dragStartPos-currentlyDraggedShape->GetBoundingBox().GetPosition();
                // now we do this inside the implementation: always assume
                // coordinates relative to the capture window (client coordinates)
                if (!m_dragImage->BeginDrag(beginDragHotSpot, this, false))
                    {
                    m_dragImage = nullptr;
                    dragMode = DragMode::DraggingNone;
                    }
                else
                    {
                    // note that this should be the window coordinates, not the unscrolled coordinates
                    m_dragImage->Move(event.GetPosition());
                    m_dragImage->Show();
                    }
                }
            else if (dragMode == DragMode::Dragging)
                {
                // move and show the image again
                m_dragImage->Move(event.GetPosition());
                m_dragImage->Show();
                }
            }
        // zoom in and out when using mouse wheel and CTRL is held down
        else if (event.GetEventType() == wxEVT_MOUSEWHEEL && wxGetMouseState().ControlDown())
            {
            if (event.GetWheelRotation() > 0)
                { ZoomIn(); }
            else if (event.GetWheelRotation() < 0)
                { ZoomOut(); }
            else
                { event.Skip(); }
            }
        else if (event.LeftDClick())
            {
            wxCommandEvent devent(EVT_WISTERIA_CANVAS_DCLICK, GetId());
            devent.SetEventObject(this);
            GetEventHandler()->ProcessEvent(devent);
            }
        else
            { event.Skip(); }
        }

    //------------------------------------------------------
    void Canvas::OnKeyDown(wxKeyEvent& event)
        {
        if (event.GetKeyCode() == WXK_NUMPAD_ADD)
            { ZoomIn(); }
        else if (event.GetKeyCode() == WXK_NUMPAD_SUBTRACT)
            { ZoomOut(); }
        else if (event.GetKeyCode() == WXK_NUMPAD_MULTIPLY)
            { ZoomReset(); }
        // get out of full screen mode
        else if (event.GetKeyCode() == WXK_ESCAPE)
            {
            auto parent = GetParent();
            while (parent && parent->IsKindOf(CLASSINFO(wxFrame)))
                {
                auto parentFrame = dynamic_cast<wxFrame*>(parent);
                if (parentFrame != nullptr)
                    { parentFrame->ShowFullScreen(false); }
                parent = parent->GetParent();
                }
            }
        // moving draggable objects
        else if (event.GetKeyCode() == WXK_NUMPAD_DOWN ||
            event.GetKeyCode() == WXK_DOWN ||
            event.GetKeyCode() == WXK_NUMPAD_UP ||
            event.GetKeyCode() == WXK_UP ||
            event.GetKeyCode() == WXK_NUMPAD_LEFT ||
            event.GetKeyCode() == WXK_LEFT ||
            event.GetKeyCode() == WXK_NUMPAD_RIGHT ||
            event.GetKeyCode() == WXK_RIGHT)
            {
            bool movingFloatingObjects{ false };
            for (auto& floatingObj : GetFreeFloatingObjects())
                {
                if (floatingObj && floatingObj->IsSelected())
                    {
                    movingFloatingObjects = true;
                    switch (event.GetKeyCode())
                        {
                        // down
                        case WXK_NUMPAD_DOWN:
                            [[fallthrough]];
                        case WXK_DOWN:
                            floatingObj->Offset(0, ScaleToScreenAndCanvas(1));
                            break;
                        // up
                        case WXK_NUMPAD_UP:
                            [[fallthrough]];
                        case WXK_UP:
                            floatingObj->Offset(0, ScaleToScreenAndCanvas(-1));
                            break;
                        // left
                        case WXK_NUMPAD_LEFT:
                            [[fallthrough]];
                        case WXK_LEFT:
                            floatingObj->Offset(ScaleToScreenAndCanvas(-1), 0);
                            break;
                        // right
                        case WXK_NUMPAD_RIGHT:
                            [[fallthrough]];
                        case WXK_RIGHT:
                            floatingObj->Offset(ScaleToScreenAndCanvas(1), 0);
                            break;
                        }
                    }
                }
            if (movingFloatingObjects)
                {
                Refresh();
                Update();
                }
            else
                { event.Skip(); }
            }
        else
            { event.Skip(); }
        }

    //------------------------------------------------------
    void Canvas::ZoomIn()
        {
        wxASSERT(m_zoomLevel >= 0);
        if (m_zoomLevel >= 40) // don't allow zooming into a nonsensical depth
            { return; }
        ++m_zoomLevel;
        m_rect.SetWidth(GetCanvasRect().GetWidth()*ZOOM_FACTOR);
        m_rect.SetHeight(GetCanvasRect().GetHeight()*ZOOM_FACTOR);
        wxGCDC gdc(this);
        CalcAllSizes(gdc);
        SetVirtualSize(GetCanvasRect().GetSize());
        Refresh();
        Update();
        }

    //------------------------------------------------------
    void Canvas::ZoomOut()
        {
        wxASSERT(m_zoomLevel >= 0);
        if (m_zoomLevel <= 0)
            { return; }
        --m_zoomLevel;
        m_rect.SetWidth(GetCanvasRect().GetWidth()/ZOOM_FACTOR);
        m_rect.SetHeight(GetCanvasRect().GetHeight()/ZOOM_FACTOR);
        wxGCDC gdc(this);
        CalcAllSizes(gdc);
        SetVirtualSize(GetCanvasRect().GetSize());
        Refresh();
        Update();
        }

    //------------------------------------------------------
    void Canvas::ZoomReset()
        {
        wxASSERT(m_zoomLevel >= 0);
        if (m_zoomLevel == 0)
            { return; }
        m_zoomLevel = 0;
        m_rect = GetClientRect();
        wxGCDC gdc(this);
        CalcAllSizes(gdc);
        SetVirtualSize(GetCanvasRect().GetSize());
        Refresh();
        Update();
        }

    //------------------------------------------------------
    std::vector<std::shared_ptr<GraphItems::GraphItemBase>>::reverse_iterator
        Canvas::FindFreeFloatingObject(const wxPoint& pt, wxDC& dc)
        {
        for (auto shapePos = GetFreeFloatingObjects().rbegin();
             shapePos != GetFreeFloatingObjects().rend();
             ++shapePos)
            {
            if ((*shapePos)->HitTest(pt, dc))
                { return shapePos; }
            }
        return GetFreeFloatingObjects().rend();
        }
    }
