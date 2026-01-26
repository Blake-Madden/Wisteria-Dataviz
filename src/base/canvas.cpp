///////////////////////////////////////////////////////////////////////////////
// Name:        canvas.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "canvas.h"
#include "axis.h"
#include "colorbrewer.h"
#include "reportprintout.h"
#include <memory>
#include <utility>
#include <wx/xrc/xmlres.h>

wxDEFINE_EVENT(wxEVT_WISTERIA_CANVAS_DCLICK, wxCommandEvent);

wxIMPLEMENT_DYNAMIC_CLASS(Wisteria::Canvas, wxScrolledWindow)

    namespace Wisteria
    {
    //------------------------------------------------------
    void Canvas::SetSizeFromPaperSize()
        {
        auto printOut = std::make_unique<ReportPrintout>(std::vector<Canvas*>{ this }, GetLabel());
#if defined(__WXMSW__) || defined(__WXOSX__)
        wxPrinterDC dc = wxPrinterDC(GetPrinterSettings());
#else
        wxPostScriptDC dc = wxPostScriptDC(GetPrinterSettings());
#endif
        printOut->SetUp(dc);

        const auto originalMinWidth = GetCanvasMinWidthDIPs();

        int w{ 0 }, h{ 0 };
        printOut->GetPageSizePixels(&w, &h);
        const auto scaledHeight = geometry::rescaled_height(std::make_pair(w, h), originalMinWidth);

        if (scaledHeight > 0) // sanity check in case page size calc failed
            {
            SetCanvasMinHeightDIPs(scaledHeight);
            // recalculate the row and column proportions for the new drawing area
            CalcRowDimensions();
            CalcAllSizes(dc);
            }
        }

    //------------------------------------------------------
    void Canvas::OnPrint([[maybe_unused]]
                         wxCommandEvent &
                         event)
        {
        auto printOut = std::make_unique<ReportPrintout>(std::vector<Canvas*>{ this }, GetLabel());
#if defined(__WXMSW__) || defined(__WXOSX__)
        wxPrinterDC dc = wxPrinterDC(GetPrinterSettings());
#else
        wxPostScriptDC dc = wxPostScriptDC(GetPrinterSettings());
#endif
        printOut->SetUp(dc);

        wxPrinter printer;
        printer.GetPrintDialogData().SetPrintData(GetPrinterSettings());
        printer.GetPrintDialogData().SetAllPages(true);
        printer.GetPrintDialogData().SetFromPage(1);
        printer.GetPrintDialogData().SetMinPage(1);
        printer.GetPrintDialogData().EnableSelection(false);

        if (!printer.Print(this, printOut.get(), true))
            {
            // just show a message if a real error occurred. They may have just cancelled.
            if (wxPrinter::GetLastError() == wxPRINTER_ERROR)
                {
                wxMessageBox(_(L"An error occurred while printing.\n"
                               "Your default printer may not be set correctly."),
                             _(L"Print"), wxOK | wxICON_WARNING);
                }
            }
        SetPrinterSettings(printer.GetPrintDialogData().GetPrintData());
        }

    //------------------------------------------------------
    void Canvas::OnPreview([[maybe_unused]]
                           wxCommandEvent &
                           event)
        {
            // note that previewing isn't done on macOS or GTK+ as it has its own native previewing
            // built into its print dialog
#if defined(__WXMSW__)
        // From wxWidgets's docs:
        // Do not explicitly delete the printout objects once this constructor has been called,
        // since they will be deleted in the wxPrintPreview destructor.
        // The same does not apply to the (printer) data argument.
        ReportPrintout* printOut = new ReportPrintout(std::vector<Canvas*>{ this }, GetLabel());
        ReportPrintout* printOutForPrinting =
            new ReportPrintout(std::vector<Canvas*>{ this }, GetLabel());
        wxPrinterDC dc = wxPrinterDC(GetPrinterSettings());
        wxPrinterDC dc2 = wxPrinterDC(GetPrinterSettings());

        printOut->SetUp(dc);
        printOutForPrinting->SetUp(dc2);

        // wxPreviewFrame may take ownership, don't use smart pointer here
        wxPrintPreview* preview =
            new wxPrintPreview(printOut, printOutForPrinting, &GetPrinterSettings());
        preview->GetPrintDialogData().SetPrintData(GetPrinterSettings());
        preview->GetPrintDialogData().SetAllPages(true);
        preview->GetPrintDialogData().SetFromPage(1);
        preview->GetPrintDialogData().SetMinPage(1);
        preview->GetPrintDialogData().EnableSelection(false);

        if (!preview->IsOk())
            {
            wxDELETE(preview);
            wxMessageBox(_(L"An error occurred while previewing.\n"
                           "Your default printer may not be set correctly."),
                         _(L"Print Preview"), wxOK | wxICON_WARNING);
            return;
            }
        int x{ 0 }, y{ 0 }, width{ 0 }, height{ 0 };
        wxClientDisplayRect(&x, &y, &width, &height);
        wxPreviewFrame* frame = new wxPreviewFrame(preview, this, _(L"Print Preview"),
                                                   wxDefaultPosition, wxSize(width, height));

        frame->Centre(wxBOTH);
        frame->Initialize();
        frame->Show();
#else
        wxFAIL_MSG(L"Print preview is Windows only!");
#endif
        }

    //------------------------------------------------------
    void Canvas::OnContextMenu([[maybe_unused]]
                               wxContextMenuEvent &
                               event)
        {
        PopupMenu(&m_menu);
        }

    //------------------------------------------------------
    void Canvas::OnCopy([[maybe_unused]]
                        wxCommandEvent &
                        event)
        {
        if (wxTheClipboard->Open())
            {
            wxTheClipboard->Clear();
            // new bitmap to be used by memory DC
            wxBitmap canvasBitmap;
            canvasBitmap.CreateWithDIPSize(
                wxSize(GetCanvasRectDIPs().GetWidth(), GetCanvasRectDIPs().GetHeight()),
                GetDPIScaleFactor());
            wxMemoryDC memDc(canvasBitmap);
            memDc.Clear();
#ifdef __WXMSW__
            wxGraphicsContext* context{ nullptr };
            auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (renderer)
                {
                context = renderer->CreateContext(memDc);
                }

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

            wxTheClipboard->SetData(new wxBitmapDataObject(canvasBitmap));
            wxTheClipboard->Close();
            }
        }

    //------------------------------------------------------
    void Canvas::OnSave([[maybe_unused]]
                        wxCommandEvent &
                        event)
        {
        const wxString openTag(L"<span style='font-weight:bold;'>");
        const wxString closeTag(L"</span><br />");
        wxArrayString choices, descriptions;
        choices.Add(L"PNG");
        descriptions.Add(
            openTag + _(L"Portable Network Graphic") + closeTag +
            _(L"A lossless-compressed image format. "
              "The image can be compressed to a smaller size without any loss of quality."));
        choices.Add(L"JPG");
        descriptions.Add(openTag + _DT(L"Joint Photographic Experts Group") + closeTag +
                         _(L"A lossy-compressed image format. Some image quality may be lost, "
                           "compared to other formats such as PNG."));
        choices.Add(L"BMP");
        descriptions.Add(openTag + _DT(L"Bitmap") + closeTag +
                         _(L"An uncompressed raster (i.e., pixel based) image format."));
        choices.Add(L"TIFF");
        descriptions.Add(openTag + _DT(L"Tagged Image File Format") + closeTag +
                         _(L"This format can either be compressed or uncompressed "
                           "and offers both lossy and lossless compression. "
                           "This format is generally preferred for desktop publishing."));
        choices.Add(L"TARGA");
        descriptions.Add(openTag + _DT(L"Truevision Advanced Raster Graphics Adapter") + closeTag +
                         _(L"A raster (i.e., pixel based) image format."));
        choices.Add(L"GIF");
        descriptions.Add(openTag + _DT(L"Graphics Interchange Format") + closeTag +
                         _(L"A raster (i.e., pixel based) image format. "
                           "Note that this image format is limited to 256 colors."));
        choices.Add(L"SVG");
        descriptions.Add(openTag + _DT(L"Scalable Vector Graphics") + closeTag +
                         _(L"A format that uses vector (rather than raster) drawing. "
                           "Vector-based images can be scaled to much larger sizes, "
                           "without the loss of quality that raster images would experience."));
        choices.Add(L"WebP");
        descriptions.Add(openTag + _DT(L"Web Picture") + closeTag +
                         _(L"A replacement for JPEG, PNG, and GIF file formats "
                           "which supports both lossy and lossless compression."));
        UI::RadioBoxDlg exportTypesDlg(this, _(L"Select Image Format"), wxEmptyString,
                                       _(L"Image formats:"), _(L"Export Image"), choices,
                                       descriptions);
        if (exportTypesDlg.ShowModal() != wxID_OK)
            {
            return;
            }
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
        case 7:
            fileFilter = _DT(L"WebP (*.webp)|*.webp");
            break;
        default:
            fileFilter = L"PNG (*.png)|*.png";
            };
        wxFileDialog dialog(this, _(L"Save Image"), wxEmptyString, GetLabel(), fileFilter,
                            wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

        if (dialog.ShowModal() != wxID_OK)
            {
            return;
            }

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
            case 7:
                filePath.SetExt(L"webp");
                break;
            default:
                filePath.SetExt(L"png");
                };
            }

        const wxFileName fn(filePath);

        // new bitmap to be used by preview image
        // (scale down size if on HiDPI)
        wxBitmap previewImg;
        previewImg.CreateWithDIPSize(GetCanvasRectDIPs().GetSize(), GetDPIScaleFactor());
        wxMemoryDC memDc(previewImg);
        memDc.Clear();
        wxGCDC gcdc(memDc);
        OnDraw(gcdc);
        DrawWatermarkLabel(gcdc);
        memDc.SelectObject(wxNullBitmap);

        UI::ImageExportOptions imgOptions;
        imgOptions.m_imageSize = GetCanvasRectDIPs().GetSize();

        wxString ext{ fn.GetExt() };
        UI::ImageExportDlg optionsDlg(this, GraphItems::Image::GetImageFileTypeFromExtension(ext),
                                      previewImg, imgOptions);
        optionsDlg.SetHelpTopic(m_helpProjectPath, m_exportHelpTopic);
        // no options for SVG (since size doesn't matter),
        // so don't bother showing the dialog for that
        if (ext.CmpNoCase(L"svg") != 0)
            {
            if (optionsDlg.ShowModal() != wxID_OK)
                {
                return;
                }
            }

        Save(filePath, optionsDlg.GetOptions());
        }

    //--------------------------------------------------
    bool Canvas::Save(const wxFileName& filePath, const UI::ImageExportOptions& options)
        {
        // immediately recalc everything when we change the canvas size
        const CanvasResizeDelayChanger resizeDelay{ *this };
        DelayResizing(false);

        // create the folder to the filepath, if necessary
        wxFileName::Mkdir(filePath.GetPath(), wxS_DIR_DEFAULT, wxPATH_MKDIR_FULL);

        wxFileName(filePath.GetFullPath()).SetPermissions(wxS_DEFAULT);

        wxCoord width = GetCanvasRectDIPs().GetWidth(), height = GetCanvasRectDIPs().GetHeight();

        // use custom size for image if supplied by caller; otherwise, just use the client size
        if (options.m_imageSize.GetWidth() > 0)
            {
            width = options.m_imageSize.GetWidth();
            }
        if (options.m_imageSize.GetHeight() > 0)
            {
            height = options.m_imageSize.GetHeight();
            }

        const wxWindowUpdateLocker wl(this);
        const FitToSaveOptionsChanger fpc(this, wxSize(width, height));

        if (filePath.GetExt().CmpNoCase(L"svg") == 0)
            {
            wxSize canvasMinSize = GetCanvasRectDIPs().GetSize();
            canvasMinSize.SetWidth(std::max(GetCanvasMinWidthDIPs(), canvasMinSize.GetWidth()));
            canvasMinSize.SetHeight(std::max(GetCanvasMinHeightDIPs(), canvasMinSize.GetHeight()));

            wxSVGFileDC svg(filePath.GetFullPath(), canvasMinSize.GetWidth(),
                            canvasMinSize.GetHeight(), 72.0, GetLabel());
            svg.SetBitmapHandler(new wxSVGBitmapEmbedHandler());
            // rescale everything to the SVG DC's scaling
            const wxEventBlocker blocker(this); // prevent resize event
            CalcAllSizes(svg);
            OnDraw(svg);
            DrawWatermarkLabel(svg);
            // readjust the measurements to the canvas's DC
            wxGCDC gdc(this);
            CalcAllSizes(gdc);
            return true;
            }

        wxString ext{ filePath.GetExt() };
        const wxBitmapType imageType = GraphItems::Image::GetImageFileTypeFromExtension(ext);

        // new bitmap to be used by memory DC
        wxBitmap exportFile;
        exportFile.CreateWithDIPSize(wxSize{ width, height }, GetDPIScaleFactor());
        GraphItems::Image::SetOpacity(exportFile, wxALPHA_OPAQUE, false);
        wxMemoryDC memDc(exportFile);
        memDc.Clear();
#ifdef __WXMSW__
        wxGraphicsContext* context{ nullptr };
        auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (renderer)
            {
            context = renderer->CreateContext(memDc);
            }

        if (context)
            {
            wxGCDC gcdc(context);
            OnDraw(gcdc);
            DrawWatermarkLabel(gcdc);
            }
        else
            {
            wxGCDC gcdc(memDc);
            OnDraw(gcdc);
            DrawWatermarkLabel(gcdc);
            }
#else
        wxGCDC gcdc(memDc);
        OnDraw(gcdc);
        DrawWatermarkLabel(gcdc);
#endif
        // unlock the image from the DC
        memDc.SelectObject(wxNullBitmap);

        // save image with contents of the DC to a file
        wxImage img(exportFile.ConvertToImage());

        img.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
        img.SetOption(wxIMAGE_OPTION_RESOLUTIONX, Settings::GetImageResolutionDPI().GetWidth());
        img.SetOption(wxIMAGE_OPTION_RESOLUTIONY, Settings::GetImageResolutionDPI().GetHeight());

        // color mode
        if (options.m_mode ==
            static_cast<decltype(options.m_mode)>(UI::ImageExportOptions::ColorMode::Grayscale))
            {
            img = img.ConvertToGreyscale();
            }

        // image specific options
        if (imageType == wxBITMAP_TYPE_TIF)
            {
            img.SetOption(wxIMAGE_OPTION_COMPRESSION, static_cast<int>(options.m_tiffCompression));
            }
        else if (imageType == wxBITMAP_TYPE_JPEG)
            {
            img.SetOption(wxIMAGE_OPTION_QUALITY, 100);
            }
        else if (imageType == wxBITMAP_TYPE_PNG)
            {
            img.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
            if (!GetLabel().empty())
                {
                img.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, GetLabel());
                }
            } // max compression
        else if (imageType == wxBITMAP_TYPE_GIF)
            {
            // "dumb" image down to 256 colors
            wxQuantize::Quantize(img, img, 256);
            img.ConvertAlphaToMask();
            // use the comment field too
            if (!GetLabel().empty())
                {
                img.SetOption(wxIMAGE_OPTION_GIF_COMMENT, GetLabel());
                }
            }

        if (!img.SaveFile(filePath.GetFullPath(), imageType))
            {
            wxMessageBox(
                wxString::Format(_(L"Failed to save image\n(%s)."), filePath.GetFullPath()),
                _(L"Save Error"), wxOK | wxICON_EXCLAMATION);
            return false;
            }
        return true;
        }

    //------------------------------------------
    Canvas::Canvas(wxWindow * parent, int itemId, const wxPoint& pos, const wxSize& size,
                   const long flags)
        : wxScrolledWindow(parent, itemId, pos, size,
                           flags | wxBORDER_NONE | wxVSCROLL | wxHSCROLL | wxALWAYS_SHOW_SB |
                               wxFULL_REPAINT_ON_RESIZE)
        {
        m_watermarkFont.MakeBold();
        SetCanvasMinWidthDIPs(GetDefaultCanvasWidthDIPs());
        SetCanvasMinHeightDIPs(GetDefaultCanvasHeightDIPs());
        wxWindow::SetBackgroundStyle(wxBG_STYLE_CUSTOM);
        wxWindow::SetBackgroundColour(Colors::ColorBrewer::GetColor(Colors::Color::White));
        SetVirtualSize(size);
        SetScrollRate(10, 10);
        wxGCDC gdc(this);
        CalcAllSizes(gdc);

        m_menu.Append(wxID_COPY, _(L"Copy"));
        m_menu.AppendSeparator();
        m_menu.Append(wxID_PRINT, _(L"Print"));
        // wx framework and arbitrarily disable wxID_SAVE in higher level menus if the active
        // document is not dirty, so use our own internal Save ID
        m_menu.Append(XRCID("ID_SAVE_ITEM"), _(L"Save"));

        m_resizeTimer.SetOwner(this);

        Bind(wxEVT_MENU, [this]([[maybe_unused]] wxCommandEvent&) { ZoomIn(); }, wxID_ZOOM_IN);

        Bind(wxEVT_MENU, [this]([[maybe_unused]] wxCommandEvent&) { ZoomOut(); }, wxID_ZOOM_OUT);

        Bind(wxEVT_MENU, [this]([[maybe_unused]] wxCommandEvent&) { ZoomReset(); }, wxID_ZOOM_FIT);

        Bind(wxEVT_TIMER,
             [this]([[maybe_unused]] wxTimerEvent&)
             {
                 m_blockResize = false;
                 wxSizeEvent event(GetSize());
                 OnResize(event);
                 Refresh();
                 Update();
             });

        Bind(wxEVT_KEY_DOWN, &Canvas::OnKeyDown, this);
        Bind(wxEVT_PAINT, &Canvas::OnPaint, this);
        Bind(wxEVT_SIZE, &Canvas::OnResize, this);
        Bind(wxEVT_CONTEXT_MENU, &Canvas::OnContextMenu, this);
        Bind(wxEVT_MENU, &Canvas::OnSave, this, wxID_SAVE);
        Bind(wxEVT_MENU, &Canvas::OnSave, this, XRCID("ID_SAVE_ITEM"));
        Bind(wxEVT_MENU, &Canvas::OnCopy, this, wxID_COPY);
        Bind(wxEVT_MENU, &Canvas::OnPreview, this, wxID_PREVIEW);
        Bind(wxEVT_MENU, &Canvas::OnPrint, this, wxID_PRINT);
        // numerous mouse events
        Bind(wxEVT_LEFT_DOWN, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_LEFT_UP, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MIDDLE_DOWN, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MIDDLE_UP, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_RIGHT_DOWN, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_RIGHT_UP, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MOTION, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_ENTER_WINDOW, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_LEAVE_WINDOW, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_LEFT_DCLICK, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MIDDLE_DCLICK, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_RIGHT_DCLICK, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MOUSEWHEEL, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX1_DOWN, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX1_UP, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX1_DCLICK, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX2_DOWN, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX2_UP, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_AUX2_DCLICK, &Canvas::OnMouseEvents, this);
        Bind(wxEVT_MAGNIFY, &Canvas::OnMouseEvents, this);
        }

    //----------------------------------------------------------------
    void Canvas::ContrastTitleLabel(GraphItems::Label & title) const
        {
        const wxColour contrastingColor{ Colors::ColorContrast::BlackOrWhiteContrast(
            GetBackgroundColor()) };
        if (title.GetFontBackgroundColor().IsOk() &&
            !title.GetFontBackgroundColor().IsTransparent())
            {
            if (title.GetFontBackgroundColor() == GetBackgroundColor())
                {
                title.SetFontBackgroundColor(contrastingColor);
                }
            }
        else if (title.GetFontColor().IsOk() && !title.GetFontColor().IsTransparent() &&
                 title.GetFontColor() == GetBackgroundColor())
            {
            title.SetFontColor(contrastingColor);
            }
        if (title.GetHeaderInfo().IsEnabled() && title.GetHeaderInfo().GetFontColor().IsOk() &&
            !title.GetHeaderInfo().GetFontColor().IsTransparent() &&
            title.GetHeaderInfo().GetFontColor() == GetBackgroundColor() &&
            // if a font background color is valid, then don't adjust the font color
            !(title.GetFontBackgroundColor().IsOk() &&
              !title.GetFontBackgroundColor().IsTransparent()))
            {
            title.GetHeaderInfo().FontColor(contrastingColor);
            }
        }

    //----------------------------------------------------------------
    long Canvas::CalcLeftTitles(wxDC & dc, const long spacingWidth)
        {
        long leftMarginWidth{ 0 };
        // add the left titles
        for (auto& title : m_leftTitles)
            {
            title.SetDPIScaleFactor(dc.FromDIP(1));
            title.SetScaling(GetScaling());
            title.SetTextOrientation(Orientation::Vertical);
            title.SetAnchorPoint(wxPoint(leftMarginWidth, GetCanvasRect(dc).GetHeight()));
            title.SetAnchoring(Anchoring::TopLeftCorner);
            title.SetMinimumUserSizeDIPs(
                std::nullopt, safe_divide<wxCoord>(GetCanvasRectDIPs().GetHeight(), GetScaling()));
            title.SetPageVerticalAlignment(
                (title.GetRelativeAlignment() == RelativeAlignment::Centered) ?
                    PageVerticalAlignment::Centered :
                (title.GetRelativeAlignment() == RelativeAlignment::FlushRight) ?
                    PageVerticalAlignment::TopAligned :
                    PageVerticalAlignment::BottomAligned);
            leftMarginWidth += title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            // contrast the title if its font color (or background color, if in use)
            // is the same as the canvas background
            ContrastTitleLabel(title);
            GetTitles().push_back(std::make_unique<GraphItems::Label>(title));
            }
        return leftMarginWidth;
        }

    //----------------------------------------------------------------
    long Canvas::CalcRightTitles(wxDC & dc, const long spacingWidth)
        {
        long rightMarginWidth{ 0 };
        long position = GetCanvasRect(dc).GetWidth() - spacingWidth;
        // add the right titles
        for (auto& title : m_rightTitles)
            {
            title.SetDPIScaleFactor(dc.FromDIP(1));
            title.SetScaling(GetScaling());
            title.SetTextOrientation(Orientation::Vertical);
            title.SetAnchorPoint(wxPoint(position, GetCanvasRect(dc).GetHeight()));
            title.SetAnchoring(Anchoring::BottomLeftCorner);
            title.SetMinimumUserSizeDIPs(
                std::nullopt, safe_divide<wxCoord>(GetCanvasRectDIPs().GetHeight(), GetScaling()));
            title.SetPageVerticalAlignment(
                (title.GetRelativeAlignment() == RelativeAlignment::Centered) ?
                    PageVerticalAlignment::Centered :
                (title.GetRelativeAlignment() == RelativeAlignment::FlushRight) ?
                    PageVerticalAlignment::TopAligned :
                    PageVerticalAlignment::BottomAligned);
            position -= title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            rightMarginWidth += title.GetBoundingBox(dc).GetWidth() + spacingWidth;
            ContrastTitleLabel(title);
            GetTitles().push_back(std::make_unique<GraphItems::Label>(title));
            }
        return rightMarginWidth;
        }

    //----------------------------------------------------------------
    long Canvas::CalcTopTitles(wxDC & dc, const long spacingWidth)
        {
        long topMarginHeight{ 0 };
        // add the top titles
        for (auto& title : m_topTitles)
            {
            title.SetDPIScaleFactor(dc.FromDIP(1));
            title.SetScaling(GetScaling());
            title.SetAnchorPoint(wxPoint(0, topMarginHeight));
            title.SetAnchoring(Anchoring::TopLeftCorner);
            title.SetMinimumUserSizeDIPs(
                safe_divide<wxCoord>(GetCanvasRectDIPs().GetWidth(), GetScaling()), std::nullopt);
            title.SetPageHorizontalAlignment(
                (title.GetRelativeAlignment() == RelativeAlignment::Centered) ?
                    PageHorizontalAlignment::Centered :
                (title.GetRelativeAlignment() == RelativeAlignment::FlushRight) ?
                    PageHorizontalAlignment::RightAligned :
                    PageHorizontalAlignment::LeftAligned);
            topMarginHeight += title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            ContrastTitleLabel(title);
            GetTitles().push_back(std::make_unique<GraphItems::Label>(title));
            }
        return topMarginHeight;
        }

    //----------------------------------------------------------------
    long Canvas::CalcBottomTitles(wxDC & dc, const long spacingWidth)
        {
        long bottomMarginHeight{ 0 };
        long position = GetCanvasRect(dc).GetHeight() - spacingWidth;
        // add the bottom titles
        for (auto& title : m_bottomTitles)
            {
            title.SetDPIScaleFactor(dc.FromDIP(1));
            title.SetScaling(GetScaling());
            title.SetAnchorPoint(wxPoint(0, position));
            title.SetAnchoring(Anchoring::BottomLeftCorner);
            title.SetMinimumUserSizeDIPs(
                safe_divide<wxCoord>(GetCanvasRectDIPs().GetWidth(), GetScaling()), std::nullopt);
            title.SetPageHorizontalAlignment(
                (title.GetRelativeAlignment() == RelativeAlignment::Centered) ?
                    PageHorizontalAlignment::Centered :
                (title.GetRelativeAlignment() == RelativeAlignment::FlushRight) ?
                    PageHorizontalAlignment::RightAligned :
                    PageHorizontalAlignment::LeftAligned);
            position -= title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            bottomMarginHeight += title.GetBoundingBox(dc).GetHeight() + spacingWidth;
            ContrastTitleLabel(title);
            GetTitles().push_back(std::make_unique<GraphItems::Label>(title));
            }
        return bottomMarginHeight;
        }

    //---------------------------------------------------
    void Canvas::OnResize(wxSizeEvent & event)
        {
        if (m_delayResize && m_blockResize && IsShownOnScreen())
            {
            m_resizeTimer.Stop();
            m_resizeTimer.StartOnce(500); // half a second
            event.Skip();
            return;
            }

        m_resizeTimer.Stop();
        m_blockResize = true;

        wxGCDC gdc(this);
        // if the new size is larger than the canvas itself, then turn off zooming
        if (GetClientRect().GetWidth() > GetCanvasRect(gdc).GetWidth() &&
            GetClientRect().GetHeight() > GetCanvasRect(gdc).GetHeight())
            {
            m_zoomLevel = 0;
            }
        // resize if canvas isn't zoomed into
        if (m_zoomLevel <= 0)
            {
            m_rectDIPs = GetClientRect();
            m_rectDIPs.SetWidth(gdc.ToDIP(m_rectDIPs.GetWidth()));

            if (IsMaintainingAspectRatio())
                {
                const auto heightToWidthRatio =
                    safe_divide<double>(GetCanvasMinHeightDIPs(), GetCanvasMinWidthDIPs());
                m_rectDIPs.SetHeight(m_rectDIPs.GetWidth() * heightToWidthRatio);
                }
            else
                {
                m_rectDIPs.SetHeight(gdc.ToDIP(m_rectDIPs.GetHeight()));
                }

            CalcAllSizes(gdc);
            }

        event.Skip();
        }

    //---------------------------------------------------
    void Canvas::CalcAllSizes(wxDC & dc)
        {
        assert((std::accumulate(m_rowsInfo.cbegin(), m_rowsInfo.cend(), 0.0,
                                [](const auto initVal, const auto val) noexcept
                                { return initVal + val.GetHeightProportion(); })) <= 1 &&
               L"Canvas row proportions are more than 100%!");

        /* The rendering area must have a minimum size of 700x500;
           otherwise, it will be crunched up and look bad.*/
        wxSize canvasMinSize = GetCanvasRectDIPs().GetSize();
        canvasMinSize.SetWidth(std::max(GetCanvasMinWidthDIPs(), canvasMinSize.GetWidth()));
        canvasMinSize.SetHeight(std::max(GetCanvasMinHeightDIPs(), canvasMinSize.GetHeight()));
        m_rectDIPs.SetSize(canvasMinSize);

        const wxCoord titleSpacingWidth = ScaleToScreenAndCanvas(2, dc);

        // calculate the left/right margins around the canvas and construct the titles
        GetTitles().clear();
        const long leftBorder = CalcLeftTitles(dc, titleSpacingWidth);
        const long topBorder = CalcTopTitles(dc, titleSpacingWidth);
        const long bottomBorder = CalcBottomTitles(dc, titleSpacingWidth);
        const long rightBorder = CalcRightTitles(dc, titleSpacingWidth);

        wxRect fixedObjectRect = GetCanvasRect(dc);
        fixedObjectRect.x += leftBorder;
        fixedObjectRect.y += topBorder;
        fixedObjectRect.SetWidth(fixedObjectRect.GetWidth() - (leftBorder + rightBorder));
        fixedObjectRect.SetHeight(fixedObjectRect.GetHeight() - (topBorder + bottomBorder));

        // reset all objects' canvas alignments and DPI scaling
        for (auto& fixedObjectsRow : GetFixedObjects())
            {
            for (auto& object : fixedObjectsRow)
                {
                if (object != nullptr)
                    {
                    object->SetContentTop(std::nullopt);
                    object->SetContentBottom(std::nullopt);
                    object->SetContentLeft(std::nullopt);
                    object->SetContentRight(std::nullopt);
                    object->SetDPIScaleFactor(dc.FromDIP(1));
                    }
                }
            }

        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            m_debugInfo = wxString::Format(
                _DT(L"Canvas scaling: %s\n"
                    "Area height: %s\nGrid height: %s\n"),
                wxNumberFormatter::ToString(GetScaling(), 3,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes),
                wxNumberFormatter::ToString(GetCanvasRect(dc).GetHeight(), 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep),
                wxNumberFormatter::ToString(fixedObjectRect.GetHeight(), 0,
                                            wxNumberFormatter::Style::Style_WithThousandsSep));
            }
        size_t rowHeightOffset{ 0 };
        // go through each row of items (e.g., subplots, legends) and resize and
        // move them into their grid area
        for (auto fixedObjectsRowPos = GetFixedObjects().begin();
             fixedObjectsRowPos != GetFixedObjects().end(); ++fixedObjectsRowPos)
            {
            auto& currentRow = *fixedObjectsRowPos;
            const auto currentRowIndex =
                std::distance(GetFixedObjects().begin(), fixedObjectsRowPos);
            assert(std::cmp_less(currentRowIndex, m_rowsInfo.size()) &&
                   L"Canvas row out of range!");

            const size_t rowHeightGridArea =
                fixedObjectRect.GetHeight() * GetRowInfo(currentRowIndex).GetHeightProportion();
            const size_t rowHeightFullCanvas =
                (GetCanvasRect(dc).GetHeight() *
                 GetRowInfo(currentRowIndex).GetHeightProportion()) +
                (!currentRow.empty() && currentRow.at(0) ?
                     ScaleToScreenAndCanvas(currentRow.at(0)->GetTopCanvasMargin(), dc) +
                         ScaleToScreenAndCanvas(currentRow.at(0)->GetBottomCanvasMargin(), dc) :
                     0);
            // is row proportional to the drawing area (the norm), or the entire canvas?
            const size_t rowHeight =
                (GetRowInfo(currentRowIndex).IsProportionLocked() ? rowHeightFullCanvas :
                                                                    rowHeightGridArea);
            // If row's proportion is locked to the whole page, then previous items need
            // to have their layouts adjusted.
            // This is normally just done for the last (or first) items on the page,
            // and usually something like a legend. This is done to keep the legend close
            // to its original height calculation; otherwise, canvas titles could steal
            // real estate for the legend and make it too small.
            if (GetRowInfo(currentRowIndex).IsProportionLocked() && currentRowIndex > 0)
                {
                const auto rowHeightDiff = rowHeightFullCanvas - rowHeightGridArea;
                rowHeightOffset -= rowHeightDiff;
                const auto rowHeightDiffForPreviousRows =
                    safe_divide<double>(rowHeightDiff, currentRowIndex);
                // previous rows (and their objects) pushed up and made smaller to make room for
                // current row which is being made taller
                for (size_t previousRowIndex = 0; std::cmp_less(previousRowIndex, currentRowIndex);
                     ++previousRowIndex)
                    {
                    auto& previousRow = GetFixedObjects().at(previousRowIndex);
                    for (auto& previousRowObject : previousRow)
                        {
                        if (previousRowObject != nullptr)
                            {
                            auto bBox = previousRowObject->GetBoundingBox(dc);
                            bBox.SetHeight(bBox.GetHeight() - rowHeightDiffForPreviousRows);
                            bBox.Offset(
                                wxPoint(0, -(rowHeightDiffForPreviousRows * previousRowIndex)));
                            previousRowObject->SetBoundingBox(bBox, dc, GetScaling());
                            previousRowObject->RecalcSizes(dc);
                            previousRowObject->UpdateSelectedItems();
                            }
                        }
                    }
                }

            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
                {
                m_debugInfo += wxString::Format(
                    _DT(L"Row %s: height %s, proportion %s\n"),
                    wxNumberFormatter::ToString(currentRowIndex, 0,
                                                wxNumberFormatter::Style::Style_None),
                    wxNumberFormatter::ToString(rowHeight, 0,
                                                wxNumberFormatter::Style::Style_WithThousandsSep),
                    wxNumberFormatter::ToString(
                        GetRowInfo(std::distance(GetFixedObjects().begin(), fixedObjectsRowPos))
                            .GetHeightProportion(),
                        3, wxNumberFormatter::Style::Style_NoTrailingZeroes));
                }
            size_t currentXPos{ 0 };
            int extraSpaceFromPreviousItemsForThisItem{ 0 };
            for (size_t i = 0; i < currentRow.size(); ++i)
                {
                auto& objectsPos = currentRow[i];
                if (objectsPos != nullptr)
                    {
                    // set the scaling from the canvas and get the bounding box for it to fit in
                    objectsPos->SetScaling(GetScaling());
                    const auto currentObjHeight =
                        objectsPos->GetCanvasHeightProportion() ?
                            objectsPos->GetCanvasHeightProportion().value() *
                                fixedObjectRect.GetHeight() :
                            rowHeight;
                    wxRect boundingRect(
                        wxPoint(fixedObjectRect.x + currentXPos,
                                fixedObjectRect.y + rowHeightOffset),
                        wxSize(fixedObjectRect.GetWidth() * objectsPos->GetCanvasWidthProportion(),
                               currentObjHeight));
                    // if any previous items were resized to be smaller, then add
                    // this object's share of that extra space
                    boundingRect.SetWidth(boundingRect.GetWidth() +
                                          extraSpaceFromPreviousItemsForThisItem);
                    wxRect nonPaddedBoundingRect{ boundingRect };
                    // subtract the canvas margins from the object's allocated space
                    // and center its drawing area within that
                    boundingRect.y += ScaleToScreenAndCanvas(objectsPos->GetTopCanvasMargin(), dc);
                    boundingRect.x += ScaleToScreenAndCanvas(objectsPos->GetLeftCanvasMargin(), dc);
                    boundingRect.SetWidth(std::max<double>(
                        0, boundingRect.GetWidth() -
                               ScaleToScreenAndCanvas(objectsPos->GetLeftCanvasMargin(), dc) -
                               ScaleToScreenAndCanvas(objectsPos->GetRightCanvasMargin(), dc)));
                    boundingRect.SetHeight(std::max<double>(
                        0, boundingRect.GetHeight() -
                               ScaleToScreenAndCanvas(objectsPos->GetTopCanvasMargin(), dc) -
                               ScaleToScreenAndCanvas(objectsPos->GetBottomCanvasMargin(), dc)));

                    objectsPos->SetBoundingBox(boundingRect, dc, GetScaling());
                    // Some items like legends and common axis that are the full length of the area
                    // won't need as much width from when their proportion was originally
                    // calculated. Because of this, get its measured width and remove any extra
                    // space around its sides, and then give that extra space back to the items
                    // previously calculated to the left of it. Note that the newly measured area is
                    // within the bounding box that had the canvas margins subtracted from it, so if
                    // we use this new measurement the margins will be preserved.
                    auto measuredBox = objectsPos->GetBoundingBox(dc);
                    if (measuredBox.GetWidth() < boundingRect.GetWidth() &&
                        objectsPos->IsFixedWidthOnCanvas())
                        {
                        const auto originalWidth = boundingRect.GetWidth();
                        const auto widthDiff = (originalWidth - measuredBox.GetWidth());
                        // how much space to give back to previous items in the row
                        const auto extraSpaceForPreviousItems =
                            // if first item in row, then there aren't previous items and page
                            // alignment is irrelevant
                            (i == 0 ? 0 :
                             objectsPos->GetPageHorizontalAlignment() ==
                                     PageHorizontalAlignment::Centered ?
                                      (widthDiff / 2) :
                             objectsPos->GetPageHorizontalAlignment() ==
                                     PageHorizontalAlignment::LeftAligned ?
                                      0 :
                                      widthDiff);
                        // the full amount of space left over from this object to
                        // add to following objects
                        const auto extraSpaceForFollowingItems =
                            widthDiff - extraSpaceForPreviousItems;
                        extraSpaceFromPreviousItemsForThisItem += safe_divide<int>(
                            extraSpaceForFollowingItems, currentRow.size() - (i + 1));

                        // if there are items to the left, then move this object back
                        boundingRect.x -= extraSpaceForPreviousItems;
                        boundingRect.SetWidth(measuredBox.GetWidth());
                        objectsPos->SetBoundingBox(boundingRect, dc, GetScaling());

                        nonPaddedBoundingRect.SetWidth(nonPaddedBoundingRect.GetWidth() -
                                                       widthDiff);
                        // adjust previously laid out items by making them wider
                        // and pushing them over
                        if (i > 0 && extraSpaceForPreviousItems > 0)
                            {
                            const auto averageWidthToAdd = safe_divide<double>(
                                extraSpaceForPreviousItems, i /* # of previous items*/);
                            for (long backCounter = static_cast<long>(i - 1); backCounter >= 0;
                                 --backCounter)
                                {
                                auto& backItem = currentRow[backCounter];
                                auto backItemBoundingBox = backItem->GetBoundingBox(dc);
                                backItemBoundingBox.x += averageWidthToAdd * backCounter;
                                backItemBoundingBox.SetWidth(backItemBoundingBox.GetWidth() +
                                                             averageWidthToAdd);
                                backItem->SetBoundingBox(backItemBoundingBox, dc, GetScaling());
                                }
                            }
                        }
                    currentXPos += nonPaddedBoundingRect.GetWidth();

                    objectsPos->RecalcSizes(dc);
                    objectsPos->UpdateSelectedItems();
                    }
                }
            if (IsRowContentAligned())
                {
                for (auto& fixedObjectsRowItems : GetFixedObjects())
                    {
                    std::vector<wxCoord> topPoints;
                    std::vector<wxCoord> bottomPoints;
                    for (auto& object : fixedObjectsRowItems)
                        {
                        if (object != nullptr && !object->GetContentRect().IsEmpty())
                            {
                            topPoints.push_back(object->GetContentRect().GetTop());
                            bottomPoints.push_back(object->GetContentRect().GetBottom());
                            }
                        }
                    if (!topPoints.empty() && !bottomPoints.empty())
                        {
                        const auto topPt = *std::ranges::max_element(std::as_const(topPoints));
                        const auto bottomPt =
                            *std::ranges::min_element(std::as_const(bottomPoints));
                        for (auto& object : fixedObjectsRowItems)
                            {
                            if (object != nullptr && !object->GetContentRect().IsEmpty() &&
                                topPt <= bottomPt)
                                {
                                object->SetContentTop(topPt);
                                object->SetContentBottom(bottomPt);
                                object->RecalcSizes(dc);
                                object->UpdateSelectedItems();
                                }
                            }
                        }
                    }
                }
            rowHeightOffset += rowHeight;
            }

        if (IsColumnContentAligned())
            {
            if (!GetFixedObjects().empty() && !GetFixedObjects().at(0).empty())
                {
                bool noMoreColumns{ false };
                const auto& topRow = GetFixedObjects().at(0);
                for (size_t colIndex = 0; colIndex < topRow.size(); ++colIndex)
                    {
                    std::vector<wxCoord> leftPoints;
                    std::vector<wxCoord> rightPoints;
                    // go through each row and gather the current column's width and position
                    for (auto fixedObjectsRowPos = GetFixedObjects().begin();
                         fixedObjectsRowPos != GetFixedObjects().end(); ++fixedObjectsRowPos)
                        {
                        if (colIndex >= fixedObjectsRowPos->size())
                            {
                            noMoreColumns = true;
                            break;
                            }
                        const auto& objectPos = fixedObjectsRowPos->at(colIndex);
                        if (objectPos == nullptr)
                            {
                            wxLogWarning(wxString::Format(
                                L"Alignment of columns on canvas failed because row %zu "
                                "has null objects instead of empty sizers.",
                                std::distance(GetFixedObjects().begin(), fixedObjectsRowPos)));
                            noMoreColumns = true;
                            break;
                            }
                        if (!objectPos->GetContentRect().IsEmpty())
                            {
                            leftPoints.push_back(objectPos->GetContentRect().GetLeft());
                            rightPoints.push_back(objectPos->GetContentRect().GetRight());
                            }
                        }
                    // the grid is jagged, so stop aligning the columns
                    if (noMoreColumns)
                        {
                        wxLogWarning(L"Alignment of columns on canvas failed because some rows "
                                     "have more columns than others.");
                        break;
                        }
                    if (!leftPoints.empty() && !rightPoints.empty())
                        {
                        // apply the smallest content area to the items in the column
                        const auto leftPt = *std::ranges::max_element(std::as_const(leftPoints));
                        const auto rightPt = *std::ranges::min_element(std::as_const(rightPoints));
                        for (auto& fixedObjectsRowPos : GetFixedObjects())
                            {
                            auto& objectPos = fixedObjectsRowPos.at(colIndex);
                            if (objectPos != nullptr && !objectPos->GetContentRect().IsEmpty() &&
                                rightPt >= leftPt)
                                {
                                objectPos->SetContentLeft(leftPt);
                                objectPos->SetContentRight(rightPt);
                                // recalculate the size of the object after adjusting its content
                                // area; if that changed the size of the object (should be smaller),
                                // then push everything to the right of it over to the left.
                                const auto oldBoundingBox = objectPos->GetBoundingBox(dc);
                                objectPos->RecalcSizes(dc);
                                const auto newBoundingBox = objectPos->GetBoundingBox(dc);
                                const auto rightDiff = std::max(
                                    oldBoundingBox.GetRight() - newBoundingBox.GetRight(), 0);
                                for (size_t remainingRowsItem = colIndex + 1;
                                     remainingRowsItem < fixedObjectsRowPos.size();
                                     ++remainingRowsItem)
                                    {
                                    if (fixedObjectsRowPos.at(remainingRowsItem) != nullptr)
                                        {
                                        fixedObjectsRowPos.at(remainingRowsItem)
                                            ->Offset(-rightDiff, 0);
                                        }
                                    }
                                objectPos->UpdateSelectedItems();
                                }
                            }
                        }
                    }
                }
            }

        SetVirtualSize(GetCanvasRect(dc).GetSize());
        }

    //---------------------------------------------------
    void Canvas::SetCanvasMinHeightDIPs(const int minHeight)
        {
        // adjust any rows whose height proportion was locked to
        // the canvas height to take into account the new height
        const auto heightAdjustmentScale =
            safe_divide<double>(m_canvasMinSizeDIPs.GetHeight(), minHeight);

        double cumulativeProportionDiff{ 0 };
        size_t nonLockedRows{ 0 };
        // adjust the proportion height for rows that are relying on its
        // proportion to the entire canvas (not just its sub-objects grid)
        for (auto& rowInfo : m_rowsInfo)
            {
            if (rowInfo.IsProportionLocked())
                {
                cumulativeProportionDiff += rowInfo.GetHeightProportion() -
                                            (rowInfo.GetHeightProportion() * heightAdjustmentScale);
                rowInfo.HeightProportion(rowInfo.GetHeightProportion() * heightAdjustmentScale);
                }
            else
                {
                ++nonLockedRows;
                }
            }
        // add or subtract the proportion changes for the locked rows and distribute
        // that to the unlocked rows (i.e., rows whose proportion simply relies
        // on the canvas's sub-object grid)
        const auto propDiffPerNonLockedRows =
            safe_divide<double>(cumulativeProportionDiff, nonLockedRows);
        for (auto& rowInfo : m_rowsInfo)
            {
            if (!rowInfo.IsProportionLocked())
                {
                rowInfo.HeightProportion(rowInfo.GetHeightProportion() + propDiffPerNonLockedRows);
                }
            }

        // now, set the new height
        m_canvasMinSizeDIPs.SetHeight(minHeight);
        }

    //---------------------------------------------------
    void Canvas::CalcRowDimensions()
        {
        // In case the aspect ratio changed, reset the area rect so that scaling will be 1.0
        m_rectDIPs.SetSize(wxSize(GetCanvasMinWidthDIPs(), GetCanvasMinHeightDIPs()));
        // This will only work when the canvas is at the default 1.0 scaling
        // because it needs to call CalcMinHeightProportion().
        // These are just sanity tests, the above should force the scaling to 1.0.
        assert(compare_doubles(GetScaling(), 1.0) &&
               L"Scaling of canvas must be one when calling CalcRowDimensions()!");
        if (!compare_doubles(GetScaling(), 1.0))
            {
            return;
            }
        // clear the current specs, as we will be resetting them here
        for (auto& rowInfo : m_rowsInfo)
            {
            rowInfo.HeightProportion(0);
            }

        size_t currentRow{ 0 };
        size_t rowsBeingFit{ 0 };
        double overallScaling{ 1.0 };
        for (auto& row : GetFixedObjects())
            {
            // Go through the items in the row and see if there are any having the row fit their
            // content. If so, use the tallest one in the row when we are done.
            std::optional<double> rowHeightProportion;
            size_t validObjectsInRow{ 0 };
            for (auto& object : row)
                {
                if (object != nullptr && object->IsFittingCanvasRowHeightToContent())
                    {
                    rowHeightProportion = rowHeightProportion.has_value() ?
                                              std::max(rowHeightProportion.value(),
                                                       CalcMinHeightProportion(*object)) :
                                              CalcMinHeightProportion(*object);
                    }
                // also re-adjust the width if being fit with its content width-wise
                if (object != nullptr && object->IsFixedWidthOnCanvas())
                    {
                    object->SetCanvasWidthProportion(CalcMinWidthProportion(*object));
                    CalcColumnWidths(currentRow);
                    }
                validObjectsInRow += ((object != nullptr) ? 1 : 0);
                }
            GetRowInfo(currentRow).RowCount((validObjectsInRow >= 1) ? 1 : 0);
            // set the row height if an item's content is setting its height
            if (rowHeightProportion.has_value())
                {
                GetRowInfo(currentRow).HeightProportion(rowHeightProportion.value());
                overallScaling -= rowHeightProportion.value();
                ++rowsBeingFit;
                }
            ++currentRow;
            }
        for (size_t i = 0; i < m_rowsInfo.size(); ++i)
            {
            if (GetRowInfo(i).GetRowCount() == 0 && i > 0)
                {
                long reverseI = i - 1;
                while (reverseI >= 0 && GetRowInfo(reverseI).GetRowCount() == 0)
                    {
                    --reverseI;
                    }
                if (reverseI >= 0)
                    {
                    GetRowInfo(reverseI).RowCount(GetRowInfo(reverseI).GetRowCount() + 1);
                    }
                }
            }
        // divide the remaining space amongst the rows being auto-fitted
        // (i.e., the rows with items whose heights don't need to be a particular value).
        const size_t autoFitRows = m_rowsInfo.size() - rowsBeingFit;
        const auto avgAutoFitRowHeight = safe_divide<double>(overallScaling, autoFitRows);
        for (auto& rowInfo : m_rowsInfo)
            {
            if (rowInfo.GetHeightProportion() == 0)
                {
                rowInfo.HeightProportion(avgAutoFitRowHeight * rowInfo.GetRowCount());
                }
            }
        // finally, see if there is any overflow and scale everything down
        // proportionally to fit
        const auto totalHeightProportion =
            std::accumulate(m_rowsInfo.cbegin(), m_rowsInfo.cend(), 0.0,
                            [](const auto& initVal, const auto& val) noexcept
                            { return initVal + val.GetHeightProportion(); });
        if (totalHeightProportion > 1)
            {
            const auto proportionDiff = safe_divide<double>(1.0, totalHeightProportion);
            for (auto& rowInfo : m_rowsInfo)
                {
                rowInfo.HeightProportion(rowInfo.GetHeightProportion() * proportionDiff);
                }
            }
        }

    //---------------------------------------------------
    std::pair<size_t, size_t> Canvas::GetFixedObjectsGridSize() const
        {
        std::pair<size_t, size_t> result(0, 0);
        result.first = m_fixedObjects.size();
        if (!m_fixedObjects.empty())
            {
            result.second = m_fixedObjects[0].size();
            }
        return result;
        }

    //---------------------------------------------------
    void Canvas::SetFixedObjectsGridSize(const size_t rows, const size_t columns)
        {
        m_fixedObjects.resize(rows);
        for (auto& fixedObject : m_fixedObjects)
            {
            fixedObject.resize(columns, nullptr);
            }

        // a full reset is needed
        m_rowsInfo.clear();
        m_rowsInfo.resize(rows, CanvasRowInfo{ safe_divide<double>(1.0, rows) });
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::GraphItemBase> Canvas::GetFixedObject(const size_t row,
                                                                      const size_t column)
        {
        assert(!GetFixedObjects().empty());
        assert(row < GetFixedObjects().size());
        assert(column < GetFixedObjects().at(0).size());
        if (GetFixedObjects().empty() || row >= GetFixedObjects().size() ||
            column >= GetFixedObjects().at(0).size())
            {
            return nullptr;
            }
        return GetFixedObjects().at(row).at(column);
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::GraphItemBase> Canvas::FindFixedObject(const long itemId) noexcept
        {
        for (const auto& row : GetFixedObjects())
            {
            for (const auto& object : row)
                {
                if (object != nullptr && object->GetId() == itemId)
                    {
                    return object;
                    }
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::GraphItemBase> Canvas::GetFixedObject(const size_t row,
                                                                      const size_t column) const
        {
        assert(!GetFixedObjects().empty());
        assert(row < GetFixedObjects().size());
        assert(column < GetFixedObjects().at(0).size());
        if (GetFixedObjects().empty() || row >= GetFixedObjects().size() ||
            column >= GetFixedObjects().at(0).size())
            {
            return nullptr;
            }
        return GetFixedObjects().at(row).at(column);
        }

    //---------------------------------------------------
    void Canvas::CalcColumnWidths(const size_t row)
        {
        // how much of the canvas is being consumed by the row
        // that this item was just added to
        const auto tallyColumnsPercent = [this, &row]()
        {
            return std::accumulate(
                GetFixedObjects().at(row).cbegin(), GetFixedObjects().at(row).cend(), 0.0,
                [](const auto initVal, const auto& item) noexcept
                { return initVal + (item == nullptr ? 0 : item->GetCanvasWidthProportion()); });
        };
        const double totalPercent = tallyColumnsPercent();
        // if more than 100%, then we need to trim the other items in the row
        if (!compare_doubles(totalPercent, 1.0))
            {
            const size_t flexibleObjects = std::ranges::count_if(
                std::as_const(GetFixedObjects().at(row)), [](const auto& obj) noexcept
                { return (obj != nullptr && !obj->IsFixedWidthOnCanvas()); });
            const double totalDiff{ totalPercent - 1.0 };
            const auto avgWidthDiff = safe_divide<double>(totalDiff, flexibleObjects);
            // this is the only object in the row and doesn't have a fixed width, set it to 100%
            if (GetFixedObjects().at(row).size() == 1 &&
                GetFixedObjects().at(row).at(0) != nullptr &&
                !GetFixedObjects().at(row).at(0)->IsFixedWidthOnCanvas())
                {
                GetFixedObjects().at(row).at(0)->SetCanvasWidthProportion(math_constants::full);
                }
            // resize all (or just non-fixed width) objects to fit
            else
                {
                for (auto& currentItem : GetFixedObjects().at(row))
                    {
                    if (currentItem != nullptr &&
                        // if all object are fixed width, then adjust all of them;
                        // otherwise, just adjust non-fixed ones
                        (!currentItem->IsFixedWidthOnCanvas() || flexibleObjects == 0))
                        {
                        currentItem->SetCanvasWidthProportion(
                            currentItem->GetCanvasWidthProportion() - avgWidthDiff);
                        }
                    }
                }
            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::LogExtraInfo))
                {
                if (compare_doubles_greater(tallyColumnsPercent(), math_constants::full))
                    {
                    wxString widthInfo;
                    for (const auto& item : GetFixedObjects().at(row))
                        {
                        if (item != nullptr)
                            {
                            widthInfo += wxString::Format(
                                L"%d, ", static_cast<int>(item->GetCanvasWidthProportion() * 100));
                            }
                        }
                    wxLogWarning(L"Items in canvas row consuming more than 100%%: " +
                                 widthInfo.RemoveLast());
                    }
                }
            }
        }

    //---------------------------------------------------
    void Canvas::SetFixedObject(const size_t row, const size_t column,
                                const std::shared_ptr<GraphItems::GraphItemBase>& object)
        {
        // cache the original scaling in case when need to recalculate
        // new canvas dimensions later
        if (object != nullptr)
            {
            object->SetOriginalCanvasScaling(object->GetScaling());
            }
        // resize the grid, if necessary
        auto currentColumnCount = (GetFixedObjects().empty() ? 0 : GetFixedObjects().at(0).size());
        if (row >= GetFixedObjects().size())
            {
            SetFixedObjectsGridSize(row + 1, std::max(column + 1, currentColumnCount));
            currentColumnCount = GetFixedObjects().at(0).size();
            }
        if (column >= currentColumnCount)
            {
            SetFixedObjectsGridSize(GetFixedObjects().size(), column + 1);
            }
        GetFixedObjects().at(row).at(column) = object;
        // re-adjust the proportional width
        if (object != nullptr)
            {
            object->SetCanvasWidthProportion(CalcMinWidthProportion(*object));
            }
        // recalc layout of column widths, unless the row is currently just
        // filled with null placeholders
        const size_t validItemsInRow =
            std::accumulate(GetFixedObjects().at(row).cbegin(), GetFixedObjects().at(row).cend(), 0,
                            [](const auto& initVal, const auto& item) noexcept
                            { return initVal + (item == nullptr ? 0 : 1); });
        // once all slots in the row have been filled with objects,
        // then re-calibrate the item's proportional widths
        if (validItemsInRow == GetFixedObjects().at(row).size())
            {
            CalcColumnWidths(row);
            }
        }

    // override the paint event so that we can use double buffering
    //---------------------------------------------------
    void Canvas::OnPaint([[maybe_unused]]
                         wxPaintEvent &
                         event)
        {
#ifdef __WXMSW__
        wxAutoBufferedPaintDC pdc(this);
        pdc.Clear();
        wxGraphicsContext* context{ nullptr };
        auto renderer = wxGraphicsRenderer::GetDirect2DRenderer();
        if (renderer)
            {
            context = renderer->CreateContext(pdc);
            }

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
    void Canvas::OnDraw(wxDC & dc)
        {
        dc.Clear();
        // fill in the background color with a linear gradient (if there is a user defined color)
        if (m_bgColorUseLinearGradient && GetBackgroundColor().IsOk())
            {
            dc.GradientFillLinear(GetCanvasRect(dc), GetBackgroundColor(),
                                  Colors::ColorBrewer::GetColor(Colors::Color::White), wxSOUTH);
            }
        else
            {
            // If background color is bad, then just fill the canvas with white.
            // Otherwise, fill with color
            const wxDCBrushChanger bc(
                dc, !GetBackgroundColor().IsOk() ?
                        wxBrush{ Colors::ColorBrewer::GetColor(Colors::Color::White) } :
                        wxBrush(GetBackgroundColor()));
            dc.DrawRectangle(GetCanvasRect(dc));
            }

        // fill in the background image (if there is one)
        if (GetBackgroundImage().IsOk() && m_bgOpacity != wxALPHA_TRANSPARENT)
            {
            GraphItems::Image img(GetBackgroundImage()
                                      .GetBitmap(GetBackgroundImage().GetDefaultSize())
                                      .ConvertToImage());
            img.SetDPIScaleFactor(dc.FromDIP(1));
            img.SetAnchoring(Anchoring::Center);
            img.SetAnchorPoint(wxPoint(
                GetCanvasRect(dc).GetLeft() + safe_divide(GetCanvasRect(dc).GetWidth(), 2),
                GetCanvasRect(dc).GetTop() + safe_divide(GetCanvasRect(dc).GetHeight(), 2)));
            // we clip the image a little so that it fits the area better
            img.SetBestSize(GetCanvasRect(dc).GetSize() + dc.FromDIP(wxSize(100, 100)));
            img.SetOpacity(m_bgOpacity);
            img.Draw(dc);
            }

        // draw the actual objects on the canvas
        for (const auto& fixedObjectsRow : GetFixedObjects())
            {
            for (const auto& objectPtr : fixedObjectsRow)
                {
                if (objectPtr != nullptr)
                    {
                    objectPtr->Draw(dc);
                    }
                }
            }

        // draw the titles
        for (const auto& title : GetTitles())
            {
            if (title != nullptr)
                {
                title->Draw(dc);
                }
            }

        // draw the movable objects (these sit on top of everything else)
        for (auto& objectPtr : GetFreeFloatingObjects())
            {
            if (objectPtr != nullptr)
                {
                objectPtr->SetScaling(GetScaling());
                objectPtr->Draw(dc);
                }
            }

        // show a label on top of the selected items
        for (const auto& fixedObjectsRow : GetFixedObjects())
            {
            for (const auto& objectPtr : fixedObjectsRow)
                {
                if (objectPtr != nullptr)
                    {
                    objectPtr->DrawSelectionLabel(dc, GetScaling(), wxRect{});
                    }
                }
            }

        DrawWatermarkLogo(dc);

        if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::DrawExtraInformation))
            {
            m_debugInfo.Trim();
            const auto bBox = GetCanvasRect(dc);
            const GraphItems::Label infoLabel(
                GraphItems::GraphItemInfo(m_debugInfo)
                    .AnchorPoint(bBox.GetBottomRight())
                    .Anchoring(Anchoring::BottomRightCorner)
                    .FontColor(Colors::ColorBrewer::GetColor(Colors::Color::Blue))
                    .Pen(Colors::ColorBrewer::GetColor(Colors::Color::Blue))
                    .DPIScaling(GetDPIScaleFactor())
                    .FontBackgroundColor(Colors::ColorBrewer::GetColor(Colors::Color::White))
                    .Padding(2, 2, 2, 2));
            infoLabel.Draw(dc);
            }
        }

    //-------------------------------------------
    void Canvas::SetBackgroundImage(const wxBitmapBundle& backgroundImage,
                                    const uint8_t opacity /*= wxALPHA_OPAQUE*/) noexcept
        {
        m_bgImage = backgroundImage;
        m_bgOpacity = opacity;
        }

    //-------------------------------------------
    wxString Canvas::ExpandWatermark(wxString label)
        {
        label.Replace(L"@DATE@", wxDateTime::Now().FormatDate());
        label.Replace(L"@TIME@", wxDateTime::Now().FormatTime());
        label.Replace(L"@DATETIME@",
                      wxDateTime::Now().FormatDate() + L" " + wxDateTime::Now().FormatTime());
        // backward compatibility
        label.Replace(L"[DATE]", wxDateTime::Now().FormatDate());
        label.Replace(L"[TIME]", wxDateTime::Now().FormatTime());
        label.Replace(L"[DATETIME]",
                      wxDateTime::Now().FormatDate() + L" " + wxDateTime::Now().FormatTime());
        return label;
        }

    //-------------------------------------------
    void Canvas::DrawWatermarkLabel(wxDC & dc) const
        {
        if (!GetWatermark().m_label.empty())
            {
            const wxDCFontChanger fc{ dc, m_watermarkFont };
            Watermark watermark = GetWatermark();
            watermark.m_label = ExpandWatermark(watermark.m_label);
            DrawWatermarkLabel(dc, GetCanvasRect(dc), watermark, GetScaling());
            }
        }

    //-------------------------------------------
    void Canvas::DrawWatermarkLogo(wxDC & dc) const
        {
        if (GetCanvasRect(dc).GetWidth() == 0 || GetCanvasRect(dc).GetHeight() == 0)
            {
            return;
            }

        if (m_watermarkImg.IsOk())
            {
            GraphItems::Image img(
                m_watermarkImg.GetBitmap(m_watermarkImg.GetDefaultSize()).ConvertToImage());
            img.GetPen() = wxNullPen;
            img.SetDPIScaleFactor(dc.FromDIP(1));
            img.SetBestSize(wxSize(ScaleToScreenAndCanvas(m_watermarkImgSizeDIPs.GetWidth(), dc),
                                   ScaleToScreenAndCanvas(m_watermarkImgSizeDIPs.GetHeight(), dc)));
            // Make logo image mildly translucent.
            // Clamp translucency to valid 0–255 range (logo is twice as opaque as system
            // translucency)
            img.SetOpacity(static_cast<uint8_t>(
                std::min<int>(Settings::GetTranslucencyValue() * 2, wxALPHA_OPAQUE)));

            img.SetAnchoring(Anchoring::BottomRightCorner);
            img.SetAnchorPoint(
                wxPoint(GetCanvasRect(dc).GetWidth(), GetCanvasRect(dc).GetHeight()));
            img.Draw(dc);
            }
        }

    //-------------------------------------------
    void Canvas::DrawWatermarkLabel(wxDC & dc, wxRect drawingRect, Watermark watermark,
                                    double scaling)
        {
        if (drawingRect.GetWidth() == 0 || drawingRect.GetHeight() == 0)
            {
            return;
            }

        drawingRect.Deflate(drawingRect.GetWidth() * math_constants::tenth);

        if (!watermark.m_label.empty())
            {
            watermark.m_label = ExpandWatermark(watermark.m_label);
            if (watermark.m_direction == WatermarkDirection::Diagonal)
                {
                const double angle = std::atan(safe_divide<double>(drawingRect.GetHeight(),
                                                                   drawingRect.GetWidth())) *
                                     (180 / std::numbers::pi);

                // set the font size so that the text will fit diagonally
                wxFont labelFont = dc.GetFont();
                labelFont.SetFractionalPointSize(GraphItems::Label::CalcDiagonalFontSize(
                    dc, labelFont, drawingRect, angle, watermark.m_label));
                labelFont.MakeBold();

                GraphItems::Label waterLabel(GraphItems::GraphItemInfo(watermark.m_label)
                                                 .Font(labelFont)
                                                 .Anchoring(Anchoring::TopLeftCorner)
                                                 .Padding(0, 0, 0, 0)
                                                 .Pen(wxNullPen)
                                                 .DPIScaling(dc.GetContentScaleFactor())
                                                 .FontColor(watermark.m_color));
                const auto boundingBox = waterLabel.GetBoundingBox(dc);
                const auto widthOfWatermark =
                    (boundingBox.GetWidth() *
                     std::abs(std::cos(geometry::degrees_to_radians(angle)))) -
                    (boundingBox.GetHeight() *
                     std::abs(std::sin(geometry::degrees_to_radians(angle))));
                const auto heightOfWatermark =
                    (boundingBox.GetWidth() *
                     std::abs(std::sin(geometry::degrees_to_radians(angle)))) +
                    (boundingBox.GetHeight() *
                     std::abs(std::cos(geometry::degrees_to_radians(angle))));

                waterLabel.SetAnchorPoint({ drawingRect.x + (drawingRect.GetWidth() / 2) -
                                                static_cast<wxCoord>(widthOfWatermark / 2),
                                            drawingRect.y + (drawingRect.GetHeight() / 2) -
                                                static_cast<wxCoord>(heightOfWatermark / 2) });

                waterLabel.Tilt(std::negate{}(angle));
                waterLabel.Draw(dc);
                }
            else
                {
                GraphItems::Label waterLabel(
                    GraphItems::GraphItemInfo(watermark.m_label)
                        .Anchoring(Anchoring::Center)
                        .Padding(0, 0, 0, 0)
                        .Pen(wxNullPen)
                        .LabelAlignment(TextAlignment::Centered)
                        .LabelPageVerticalAlignment(PageVerticalAlignment::Centered)
                        .LabelPageHorizontalAlignment(PageHorizontalAlignment::Centered)
                        .DPIScaling(dc.GetContentScaleFactor())
                        .FontColor(watermark.m_color));
                waterLabel.GetFont().MakeBold();
                waterLabel.SetBoundingBox(drawingRect, dc, scaling);

                waterLabel.Draw(dc);
                }
            }
        }

    //-------------------------------------------
    void Canvas::OnMouseEvents(wxMouseEvent & event)
        {
        static DragMode dragMode = DragMode::DraggingNone;
        static wxPoint dragStartPos;
        static std::shared_ptr<GraphItems::GraphItemBase> currentlyDraggedShape;
        wxPoint unscrolledPosition;
        CalcUnscrolledPosition(event.GetPosition().x, event.GetPosition().y, &unscrolledPosition.x,
                               &unscrolledPosition.y);
        wxGCDC gdc(this);
        const wxCoord refreshPadding = ScaleToScreenAndCanvas(10, gdc);

        if (event.LeftDown())
            {
            assert(currentlyDraggedShape == nullptr &&
                   L"Item being dragged should be null upon left mouse down!");
            // unselect any selected items (if Control/Command isn't held down),
            // as we are now selecting (and possibly dragging) something else.
            if (!wxGetMouseState().CmdDown())
                {
                for (auto& polygonPtr : GetFreeFloatingObjects())
                    {
                    if (polygonPtr && polygonPtr->IsSelected())
                        {
                        polygonPtr->SetSelected(false);
                        }
                    }
                for (auto& fixedObjectsRow : GetFixedObjects())
                    {
                    for (auto& objectPtr : fixedObjectsRow)
                        {
                        if (objectPtr != nullptr)
                            {
                            objectPtr->ClearSelections();
                            }
                        }
                    }
                for (auto& title : GetTitles())
                    {
                    if (title != nullptr && title->IsSelected())
                        {
                        title->SetSelected(false);
                        }
                    }
                }
            // see if a movable object is being selected.
            if (const auto movableObjectsPos = FindFreeFloatingObject(unscrolledPosition, gdc);
                movableObjectsPos != GetFreeFloatingObjects().rend())
                {
                // We tentatively start dragging, but wait for
                // mouse movement before dragging properly.
                dragMode = DragMode::DragStart;
                dragStartPos = unscrolledPosition;
                (*movableObjectsPos)->SetSelected(!(*movableObjectsPos)->IsSelected());
                Refresh(true);
                Update();
                m_dragImage = std::make_unique<wxDragImage>((*movableObjectsPos)->ToBitmap(gdc),
                                                            wxCursor(wxCURSOR_HAND));
                (*movableObjectsPos)->SetInDragState(true);
                currentlyDraggedShape = (*movableObjectsPos);
                event.Skip();
                return; // we have our selection, so bail before hit testing everything else
                }

            wxASSERT_MSG(currentlyDraggedShape == nullptr,
                         L"Item being dragged should be null upon left mouse down!");
            currentlyDraggedShape = nullptr;

            // ...or the fixed items connected to the canvas's grid
            for (auto& fixedObjectsRow : GetFixedObjects())
                {
                for (const auto& object : fixedObjectsRow)
                    {
                    if (object && object->SelectObjectAtPoint(unscrolledPosition, gdc))
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

            assert(currentlyDraggedShape && "Drag image is null while mouse up, "
                                            "although drag mode isn't set to none!");
            if (m_dragImage != nullptr)
                {
                m_dragImage->Hide();
                m_dragImage->EndDrag();
                m_dragImage = nullptr;
                }

            assert(currentlyDraggedShape && "Item being dragged is null while mouse up, "
                                            "although drag mode isn't set to none!");
            if (currentlyDraggedShape)
                {
                const wxPoint movePt(unscrolledPosition - dragStartPos);
                currentlyDraggedShape->Offset(movePt.x, movePt.y);
                currentlyDraggedShape->SetInDragState(false);
                wxRect boundingBox(
                    currentlyDraggedShape->GetBoundingBox(gdc).Inflate(refreshPadding));
                boundingBox.Offset(event.GetPosition() - unscrolledPosition);
                currentlyDraggedShape = nullptr;
                Refresh(true, &boundingBox);
                }
            }
        else if (event.Dragging() && dragMode != DragMode::DraggingNone)
            {
            assert(currentlyDraggedShape && "Item being dragged is null while mouse drag, "
                                            "although drag mode isn't set to none!");
            if (dragMode == DragMode::DragStart && currentlyDraggedShape)
                {
                dragStartPos = unscrolledPosition;

                // start the drag
                dragMode = DragMode::Dragging;

                // redraw the item being dragged
                // (we refresh a few pixels around the object to prevent any shearing)
                wxRect boundingBox(
                    currentlyDraggedShape->GetBoundingBox(gdc).Inflate(refreshPadding));
                boundingBox.Offset(event.GetPosition() - unscrolledPosition);
                Refresh(true, &boundingBox);
                Update();

                // the offset between the top-left of the shape image and
                // the current shape position
                const wxPoint beginDragHotSpot =
                    dragStartPos - currentlyDraggedShape->GetBoundingBox(gdc).GetPosition();
                // now we do this inside the implementation: always assume
                // coordinates relative to the capture window (client coordinates)
                if (!m_dragImage->BeginDrag(beginDragHotSpot, this, false))
                    {
                    m_dragImage = nullptr;
                    dragMode = DragMode::DraggingNone;
                    }
                else
                    {
                    // note that this should be the window coordinates,
                    // not the unscrolled coordinates
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
                {
                ZoomIn();
                }
            else if (event.GetWheelRotation() < 0)
                {
                ZoomOut();
                }
            else
                {
                event.Skip();
                }
            }
        else if (event.LeftDClick())
            {
            wxCommandEvent dEvent(wxEVT_WISTERIA_CANVAS_DCLICK, GetId());
            dEvent.SetEventObject(this);
            GetEventHandler()->ProcessEvent(dEvent);
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void Canvas::OnKeyDown(wxKeyEvent & event)
        {
        if (event.GetKeyCode() == WXK_NUMPAD_ADD)
            {
            ZoomIn();
            }
        else if (event.GetKeyCode() == WXK_NUMPAD_SUBTRACT)
            {
            ZoomOut();
            }
        else if (event.GetKeyCode() == WXK_NUMPAD_MULTIPLY)
            {
            ZoomReset();
            }
        // get out of full screen mode
        else if (event.GetKeyCode() == WXK_ESCAPE)
            {
            auto* parent = GetParent();
            while (parent != nullptr && parent->IsKindOf(CLASSINFO(wxFrame)))
                {
                auto* parentFrame = dynamic_cast<wxFrame*>(parent);
                if (parentFrame != nullptr)
                    {
                    parentFrame->ShowFullScreen(false);
                    }
                parent = parent->GetParent();
                }
            }
        // moving draggable objects
        else if (event.GetKeyCode() == WXK_NUMPAD_DOWN || event.GetKeyCode() == WXK_DOWN ||
                 event.GetKeyCode() == WXK_NUMPAD_UP || event.GetKeyCode() == WXK_UP ||
                 event.GetKeyCode() == WXK_NUMPAD_LEFT || event.GetKeyCode() == WXK_LEFT ||
                 event.GetKeyCode() == WXK_NUMPAD_RIGHT || event.GetKeyCode() == WXK_RIGHT)
            {
            const wxGCDC gdc(this);
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
                        floatingObj->Offset(0, ScaleToScreenAndCanvas(1, gdc));
                        break;
                    // up
                    case WXK_NUMPAD_UP:
                        [[fallthrough]];
                    case WXK_UP:
                        floatingObj->Offset(0, ScaleToScreenAndCanvas(-1, gdc));
                        break;
                    // left
                    case WXK_NUMPAD_LEFT:
                        [[fallthrough]];
                    case WXK_LEFT:
                        floatingObj->Offset(ScaleToScreenAndCanvas(-1, gdc), 0);
                        break;
                    // right
                    case WXK_NUMPAD_RIGHT:
                        [[fallthrough]];
                    case WXK_RIGHT:
                        floatingObj->Offset(ScaleToScreenAndCanvas(1, gdc), 0);
                        break;
                    default:
                        // no-op
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
                {
                event.Skip();
                }
            }
        else
            {
            event.Skip();
            }
        }

    //------------------------------------------------------
    void Canvas::ZoomIn()
        {
        assert(m_zoomLevel >= 0);
        if (m_zoomLevel >= 40) // don't allow zooming into a nonsensical depth
            {
            return;
            }
        ++m_zoomLevel;
        wxGCDC gdc(this);

        m_rectDIPs.SetWidth(m_rectDIPs.GetWidth() * ZOOM_FACTOR);
        m_rectDIPs.SetHeight(m_rectDIPs.GetHeight() * ZOOM_FACTOR);

        CalcAllSizes(gdc);
        Refresh();
        Update();
        }

    //------------------------------------------------------
    void Canvas::ZoomOut()
        {
        assert(m_zoomLevel >= 0);
        if (m_zoomLevel <= 0)
            {
            return;
            }
        --m_zoomLevel;
        wxGCDC gdc(this);

        m_rectDIPs.SetWidth(m_rectDIPs.GetWidth() / ZOOM_FACTOR);
        m_rectDIPs.SetHeight(m_rectDIPs.GetHeight() / ZOOM_FACTOR);

        CalcAllSizes(gdc);
        Refresh();
        Update();
        }

    //------------------------------------------------------
    void Canvas::ZoomReset()
        {
        assert(m_zoomLevel >= 0);
        if (m_zoomLevel == 0)
            {
            return;
            }
        m_zoomLevel = 0;
        wxGCDC gdc(this);

        m_rectDIPs = GetClientRect();
        m_rectDIPs.SetWidth(gdc.ToDIP(m_rectDIPs.GetWidth()));
        m_rectDIPs.SetHeight(gdc.ToDIP(m_rectDIPs.GetHeight()));

        CalcAllSizes(gdc);
        Refresh();
        Update();
        }

    //------------------------------------------------------
    std::vector<std::shared_ptr<GraphItems::GraphItemBase>>::reverse_iterator
    Canvas::FindFreeFloatingObject(const wxPoint& pt, wxDC& dc)
        {
        for (auto shapePos = GetFreeFloatingObjects().rbegin();
             shapePos != GetFreeFloatingObjects().rend(); ++shapePos)
            {
            if (*shapePos != nullptr && (*shapePos)->HitTest(pt, dc))
                {
                return shapePos;
                }
            }
        return GetFreeFloatingObjects().rend();
        }

    //------------------------------------------------------
    double Canvas::CalcMinHeightProportion(GraphItems::GraphItemBase & item)
        {
        wxGCDC gdc(this);
        const CanvasItemScalingChanger sc(item);
        item.SetMinimumUserSizeDIPs(std::nullopt, std::nullopt);
        item.RecalcSizes(gdc);
        const auto bBox = item.GetBoundingBox(gdc);
        auto bBoxHeight = bBox.GetHeight();
        // Large images will likely be wider than their canvas.
        // In that case (and if they should have their row's height fit just the image,
        // then adjust their height calculation based on the canvas width
        // (keeping the aspect ratio). When the canvas is laid out later,
        // the image's width will be set to the canvas's width and the height
        // calculated here earlier will be used. This will ensure that there won't
        // be dead space above and below the image.
        if (item.IsKindOf(wxCLASSINFO(GraphItems::Image)) &&
            item.IsFittingCanvasRowHeightToContent())
            {
            const auto canvasWidth = GetCanvasRect(gdc).GetWidth();
            if (bBox.GetWidth() > canvasWidth)
                {
                bBoxHeight *= (safe_divide<double>(canvasWidth, bBox.GetWidth()));
                }
            }
        return safe_divide<double>(bBoxHeight + gdc.FromDIP(item.GetTopCanvasMargin()) +
                                       gdc.FromDIP(item.GetBottomCanvasMargin()),
                                   gdc.FromDIP(GetCanvasMinHeightDIPs()));
        }
    } // namespace Wisteria
