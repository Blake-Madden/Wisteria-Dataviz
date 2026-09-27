///////////////////////////////////////////////////////////////////////////////
// Name:        reportslideexportbase.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportslideexportbase.h"
#include "../base/graphitems.h"
#include "../base/settings.h"
#include <wx/dcgraph.h>
#include <wx/dcmemory.h>
#include <wx/dcsvg.h>
#include <wx/graphics.h>
#include <wx/mstream.h>

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::EscapeXml(const wxString& str)
    {
    wxString result;
    result.reserve(static_cast<size_t>(str.length() * 1.1));
    for (size_t charIndex = 0; charIndex < str.length(); ++charIndex)
        {
        const wxUniChar currentChar = str[charIndex];
        if (currentChar == L'&')
            {
            result += L"&amp;";
            }
        else if (currentChar == L'<')
            {
            result += L"&lt;";
            }
        else if (currentChar == L'>')
            {
            result += L"&gt;";
            }
        else if (currentChar == L'"')
            {
            result += L"&quot;";
            }
        else if (currentChar == L'\'')
            {
            result += L"&apos;";
            }
        else if (currentChar >= 0x20 || currentChar == L'\t' || currentChar == L'\n' ||
                 currentChar == L'\r')
            {
            result += currentChar;
            }
        }
    return result;
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::EscapeXmlAttribute(const wxString& str)
    {
    wxString collapsed{ str };
    collapsed.Replace(L"\r\n", L" ");
    collapsed.Replace(L"\r", L" ");
    collapsed.Replace(L"\n", L" ");
    collapsed.Replace(L"\t", L" ");
    while (collapsed.Replace(L"  ", L" ") > 0)
        {
        }
    return EscapeXml(collapsed.Trim(true).Trim(false));
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::CollectAccessibilityText(Canvas* canvas)
    {
    if (canvas == nullptr)
        {
        return {};
        }

    wxArrayString lines;
    const auto addTitles = [&lines](const std::vector<GraphItems::Label>& titles)
    {
        for (const auto& title : titles)
            {
            if (!title.GetText().empty())
                {
                lines.Add(title.GetText());
                }
            }
    };
    addTitles(canvas->GetTopTitles());
    addTitles(canvas->GetLeftTitles());
    addTitles(canvas->GetRightTitles());
    addTitles(canvas->GetBottomTitles());

    const auto [rowCount, columnCount] = canvas->GetFixedObjectsGridSize();
    for (size_t row = 0; row < rowCount; ++row)
        {
        for (size_t column = 0; column < columnCount; ++column)
            {
            const auto fixedObject = canvas->GetFixedObject(row, column);
            if (fixedObject == nullptr)
                {
                continue;
                }
            wxString label = fixedObject->GetAccessibilityAttributes().GetAriaLabel();
            if (label.empty())
                {
                label = fixedObject->GetAutoAccessibilityAttributes().GetAriaLabel();
                }
            if (!label.empty())
                {
                wxString buffer;
                Wisteria::GraphItems::GraphItemBase::AddAccessibilityAttribute(buffer, label,
                                                                               wxString{});
                lines.Add(buffer);
                }
            }
        }

    wxString result;
    for (size_t lineIndex = 0; lineIndex < lines.GetCount(); ++lineIndex)
        {
        if (lineIndex > 0)
            {
            result += L"\n\n";
            }
        result += lines[lineIndex];
        }
    return result;
    }

//------------------------------------------------------
void Wisteria::ReportSlideExportBase::RenderCanvas(Canvas* canvas, const wxSize renderSize,
                                                   wxString& svgOut, wxMemoryBuffer* pngOut)
    {
    const wxSize safeSize{ std::max(1, renderSize.GetWidth()),
                           std::max(1, renderSize.GetHeight()) };

    const wxWindowUpdateLocker updateLocker{ canvas };

    // temporarily lay the canvas out at the render size
    const int origMinWidth{ canvas->GetCanvasMinWidthDIPs() };
    const int origMinHeight{ canvas->GetCanvasMinHeightDIPs() };
    const wxSize origSize{ canvas->GetSize() };
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(safeSize.GetWidth());
        canvas->SetCanvasMinHeightDIPs(safeSize.GetHeight());
        // calling SetSize before CalcRowDimensions() is needed because some
        // internals look at the window size rather than the min width/height
        canvas->SetSize(canvas->FromDIP(safeSize));
        canvas->CalcRowDimensions();
        canvas->SetSize(canvas->FromDIP(safeSize));
        }

    // PNG first, mirroring the raster path in Canvas::Save so the layout the
    // resize produced is drawn without an intervening re-measure.
    if (pngOut != nullptr)
        {
        wxBitmap exportBmp;
        exportBmp.CreateWithDIPSize(safeSize, canvas->GetDPIScaleFactor());
        GraphItems::Image::SetOpacity(exportBmp, wxALPHA_OPAQUE, false);

            {
            wxMemoryDC memDc{ exportBmp };
            memDc.Clear();
            const wxEventBlocker blocker{ canvas };
#ifdef __WXMSW__
            wxGraphicsContext* graphicsContext{ nullptr };
            auto* d2dRenderer = wxGraphicsRenderer::GetDirect2DRenderer();
            if (d2dRenderer != nullptr)
                {
                graphicsContext = d2dRenderer->CreateContext(memDc);
                }
            if (graphicsContext != nullptr)
                {
                wxGCDC gcdc{ graphicsContext };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
            else
                {
                wxGCDC gcdc{ memDc };
                canvas->OnDraw(gcdc);
                canvas->DrawWatermarkLabel(gcdc);
                }
#else
            wxGCDC gcdc{ memDc };
            canvas->OnDraw(gcdc);
            canvas->DrawWatermarkLabel(gcdc);
#endif
            memDc.SelectObject(wxNullBitmap);
            }

        wxImage exportImg{ exportBmp.ConvertToImage() };
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONUNIT, wxIMAGE_RESOLUTION_INCHES);
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONX,
                            Settings::GetImageResolutionDPI().GetWidth());
        exportImg.SetOption(wxIMAGE_OPTION_RESOLUTIONY,
                            Settings::GetImageResolutionDPI().GetHeight());
        exportImg.SetOption(wxIMAGE_OPTION_PNG_COMPRESSION_LEVEL, 9);
        if (!canvas->GetLabel().empty())
            {
            exportImg.SetOption(wxIMAGE_OPTION_PNG_DESCRIPTION, canvas->GetLabel());
            }

        wxMemoryOutputStream pngStream;
        exportImg.SaveFile(pngStream, wxBITMAP_TYPE_PNG);

        const size_t pngLength{ static_cast<size_t>(pngStream.GetLength()) };
        *pngOut = wxMemoryBuffer(pngLength);
        pngStream.CopyTo(pngOut->GetData(), pngLength);
        pngOut->SetDataLen(pngLength);
        }

        // SVG
        {
        wxSVGFileDC svgDc{ wxString{}, safeSize.GetWidth(), safeSize.GetHeight(), wxSVG_DEFAULT_DPI,
                           canvas->GetLabel() };
        svgDc.SetBitmapHandler(new wxSVGBitmapEmbedHandler{});
        const wxEventBlocker blocker{ canvas };
        canvas->CalcAllSizes(svgDc);
        canvas->OnDraw(svgDc);
        canvas->DrawWatermarkLabel(svgDc);
        svgOut = svgDc.GetSVGDocument();
        }

    // restore the canvas's original size and layout
    if (canvas->IsFittingToPageWhenPrinting())
        {
        canvas->SetCanvasMinWidthDIPs(origMinWidth);
        canvas->SetCanvasMinHeightDIPs(origMinHeight);
        canvas->SetSize(origSize);
        canvas->CalcRowDimensions();
        canvas->SetSize(origSize);
        }

        // restore the on-screen measurements
        {
        wxGCDC screenDc{ canvas };
        canvas->CalcAllSizes(screenDc);
        }
    canvas->ResetResizeDelay();
    canvas->ZoomReset();
    canvas->SendSizeEvent();
    canvas->Refresh();
    }

//------------------------------------------------------
wxString Wisteria::ReportSlideExportBase::ColorToHex(const wxColour& color)
    {
    return color.GetAsString(wxC2S_HTML_SYNTAX).Mid(1).Upper();
    }
