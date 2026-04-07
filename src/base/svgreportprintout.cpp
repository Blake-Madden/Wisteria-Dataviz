///////////////////////////////////////////////////////////////////////////////
// Name:        svgreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgreportprintout.h"
#include <wx/dcsvg.h>
#include <wx/file.h>
#include <wx/paper.h>

//------------------------------------------------------
Wisteria::SVGReportPrintout::SVGReportPrintout(const std::vector<Canvas*>& canvases,
                                               const wxString& filePath,
                                               const wxSize& pageSize /*= wxDefaultSize*/)
    {
    // collect per-canvas paper sizes for rendering
    std::vector<wxSize> pageSizes;
    pageSizes.reserve(canvases.size());
    for (const auto* canvas : canvases)
        {
        if (canvas == nullptr)
            {
            continue;
            }
        pageSizes.push_back(GetPaperSizeDIPs(canvas));
        }

    // the layout size controls the viewBox and page spacing;
    // the rendering size (pageSizes) stays at each canvas's own paper size
    const bool useOverrideSize = (pageSize != wxDefaultSize);
    int maxWidth{ 0 };
    int totalHeight{ 0 };
    for (const auto& ps : pageSizes)
        {
        const int layoutWidth = useOverrideSize ? pageSize.GetWidth() : ps.GetWidth();
        const int layoutHeight = useOverrideSize ? pageSize.GetHeight() : ps.GetHeight();
        maxWidth = std::max(maxWidth, layoutWidth);
        totalHeight += layoutHeight;
        }

    wxString svgContent;

    svgContent += L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    svgContent += L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
                  "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    svgContent += wxString::Format(L"<svg xmlns=\"http://www.w3.org/2000/svg\" "
                                   "xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" "
                                   "width=\"100%%\" height=\"%d\" "
                                   "preserveAspectRatio=\"xMidYMin meet\" viewBox=\"0 0 %d %d\">\n",
                                   totalHeight, maxWidth, totalHeight);
    svgContent += L"<g id=\"pageset\">\n";

    int yOffset{ 0 };
    size_t pageIndex{ 0 };
    for (auto* canvas : canvases)
        {
        if (canvas == nullptr)
            {
            continue;
            }

        const wxSize renderSize = pageSizes[pageIndex];
        const int layoutWidth = useOverrideSize ? pageSize.GetWidth() : renderSize.GetWidth();
        const int layoutHeight = useOverrideSize ? pageSize.GetHeight() : renderSize.GetHeight();
        ++pageIndex;

        // render at the layout size so bitmaps are rasterized at the target resolution
        wxSVGFileDC svgDC{ wxString{}, layoutWidth, layoutHeight, wxSVG_DEFAULT_DPI,
                           canvas->GetLabel() };
        svgDC.SetBitmapHandler(new wxSVGBitmapEmbedHandler{});

        // freeze the canvas to hide the resize flicker
        const wxWindowUpdateLocker updateLocker{ canvas };

        // temporarily resize the canvas to match the target page dimensions
        const int origMinWidth = canvas->GetCanvasMinWidthDIPs();
        const int origMinHeight = canvas->GetCanvasMinHeightDIPs();
        const wxSize origSize = canvas->GetSize();

        if (canvas->IsFittingToPageWhenPrinting())
            {
            const auto scaledHeight =
                geometry::rescaled_height(std::make_pair(layoutWidth, layoutHeight), layoutWidth);
            if (scaledHeight > 0)
                {
                canvas->SetCanvasMinWidthDIPs(layoutWidth);
                canvas->SetCanvasMinHeightDIPs(scaledHeight);
                canvas->CalcRowDimensions();
                canvas->SetSize(canvas->FromDIP(wxSize(layoutWidth, scaledHeight)));
                }
            }

            {
            // block events only during rendering to the SVG DC
            const wxEventBlocker blocker{ canvas };
            canvas->CalcAllSizes(svgDC);
            canvas->OnDraw(svgDC);
            canvas->DrawWatermarkLabel(svgDC);
            }

        // restore original canvas dimensions
        if (canvas->IsFittingToPageWhenPrinting())
            {
            canvas->SetCanvasMinWidthDIPs(origMinWidth);
            canvas->SetCanvasMinHeightDIPs(origMinHeight);
            canvas->CalcRowDimensions();
            canvas->SetSize(origSize);
            }
        canvas->ResetResizeDelay();
        canvas->SendSizeEvent();
        canvas->Refresh();

        svgContent += wxString::Format(L"<g class=\"page\" data-width=\"%d\" data-height=\"%d\" "
                                       "transform=\"translate(0,%d)\">\n",
                                       layoutWidth, layoutHeight, yOffset);
        svgContent += StripSvgTags(svgDC.GetSVGDocument());

        svgContent += L"\n</g>\n";

        yOffset += layoutHeight;
        }

    svgContent += L"</g>\n";
    svgContent += L"</svg>\n";

    wxFile outFile(filePath, wxFile::write);
    if (outFile.IsOpened())
        {
        outFile.Write(svgContent, wxConvUTF8);
        }
    else
        {
        wxMessageBox(wxString::Format(_(L"Failed to save SVG report to \"%s\"."), filePath),
                     _(L"Export Error"), wxOK | wxICON_ERROR);
        }
    }

//------------------------------------------------------
wxSize Wisteria::SVGReportPrintout::GetPaperSizeDIPs(const Canvas* canvas)
    {
    wxASSERT_MSG(canvas, L"Invalid canvas passed to SVGReportPrintout!");
    if (canvas == nullptr)
        {
        return { 800, 600 };
        }

    const auto& printData = canvas->GetPrinterSettings();
    const wxPrintPaperType* paperType =
        wxThePrintPaperDatabase->FindPaperType(printData.GetPaperId());
    // paper size is in tenths of a millimeter;
    // divide by 254 (25.4mm per inch * 10) to get inches,
    // then multiply by 96 (DIPs per inch) to get DIPs
    constexpr double tenthsMmPerInch = 254.0;
    constexpr double dipsPerInch = 96.0;
    if (paperType != nullptr)
        {
        const wxSize sizeMM = paperType->GetSize();
        const int widthDIPs =
            wxRound(safe_divide<double>(sizeMM.GetWidth(), tenthsMmPerInch) * dipsPerInch);
        const int heightDIPs =
            wxRound(safe_divide<double>(sizeMM.GetHeight(), tenthsMmPerInch) * dipsPerInch);

        if (printData.GetOrientation() == wxLANDSCAPE)
            {
            return { heightDIPs, widthDIPs };
            }
        return { widthDIPs, heightDIPs };
        }

    // fallback: US Letter (8.5" x 11") at 96 DPI
    return { static_cast<int>(8.5 * dipsPerInch), static_cast<int>(11 * dipsPerInch) };
    }

//------------------------------------------------------
wxString Wisteria::SVGReportPrintout::StripSvgTags(const wxString& svgDoc)
    {
    wxString result = svgDoc;

    // remove <?xml ...?> declaration
    const auto xmlPos = result.find(L"<?xml");
    if (xmlPos != wxString::npos)
        {
        const auto endPos = result.find(L"?>");
        if (endPos != wxString::npos)
            {
            result = result.substr(0, xmlPos) + result.substr(endPos + 2);
            }
        }

    // remove <!DOCTYPE ...> declaration
    const auto docTypePos = result.find(L"<!DOCTYPE");
    if (docTypePos != wxString::npos)
        {
        const auto endPos = result.find(L'>', docTypePos);
        if (endPos != wxString::npos)
            {
            result = result.substr(0, docTypePos) + result.substr(endPos + 1);
            }
        }

    // remove opening <svg ...> tag
    const auto svgOpenPos = result.find(L"<svg");
    if (svgOpenPos != wxString::npos)
        {
        const auto endPos = result.find(L'>', svgOpenPos);
        if (endPos != wxString::npos)
            {
            result = result.substr(0, svgOpenPos) + result.substr(endPos + 1);
            }
        }

    // remove closing </svg> tag
    const auto svgClosePos = result.find(L"</svg>");
    if (svgClosePos != wxString::npos)
        {
        result = result.substr(0, svgClosePos);
        }

    return result.Trim(true).Trim(false);
    }
