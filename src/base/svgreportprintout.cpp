///////////////////////////////////////////////////////////////////////////////
// Name:        svgreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgreportprintout.h"
#include "../base/colorbrewer.h"
#include <wx/dcsvg.h>
#include <wx/file.h>
#include <wx/paper.h>

//------------------------------------------------------
Wisteria::SVGReportPrintout::SVGReportPrintout(const std::vector<Canvas*>& canvases,
                                               const SVGReportOptions& options)
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
    const bool useOverrideSize = (options.m_pageSize != wxDefaultSize);
    int maxWidth{ 0 };
    int totalHeight{ 0 };
    for (size_t i = 0; i < pageSizes.size(); ++i)
        {
        const auto& ps = pageSizes[i];
        const int layoutWidth = useOverrideSize ? options.m_pageSize.GetWidth() : ps.GetWidth();
        const int layoutHeight = useOverrideSize ? options.m_pageSize.GetHeight() : ps.GetHeight();
        maxWidth = std::max(maxWidth, layoutWidth);
        totalHeight += layoutHeight;
        if (i < pageSizes.size() - 1)
            {
            totalHeight += options.m_horizontalGap;
            }
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

    if (options.HasInteractiveFeatures())
        {
        svgContent += L"<style type=\"text/css\">\n"
                      "  <![CDATA[\n";
        if (options.m_includeTransitions)
            {
            svgContent +=
                L"    .page { transition: transform 0.6s cubic-bezier(0.4, 0, 0.2, 1); }\n";
            }
        if (options.m_includeHighlighting)
            {
            svgContent += L"    .page:hover { filter: drop-shadow(0 0 10px rgba(0,0,0,0.1)); }\n"
                          "    .page rect:hover, .page circle:hover, .page path:hover { \n"
                          "      filter: brightness(1.2); cursor: pointer; \n"
                          "    }\n";
            }
        if (options.m_includeLayoutToggle)
            {
            const wxString btnHex = options.m_buttonColor.GetAsString(wxC2S_HTML_SYNTAX);
            const wxString textHex =
                Wisteria::Colors::ColorContrast::IsLight(options.m_buttonColor) ? L"black" :
                                                                                  L"white";

            svgContent += wxString::Format(
                L"    #ui-layer { \n"
                "      opacity: 0; \n"
                "      transition: opacity 0.4s ease-in-out; \n"
                "      pointer-events: none;\n"
                "    }\n"
                "    svg:hover #ui-layer { \n"
                "      opacity: 1; \n"
                "      pointer-events: auto;\n"
                "    }\n"
                "    .btn { fill: %s; cursor: pointer; transition: transform 0.1s, fill 0.2s; "
                "fill-opacity: 0.9; }\n"
                "    .btn:hover { fill-opacity: 1.0; transform: translateY(-1px); }\n"
                "    .btn-text { fill: %s; font-family: sans-serif; font-size: 12px; font-weight: "
                "bold; pointer-events: none; text-anchor: middle; }\n",
                btnHex, textHex);
            }
        svgContent += L"  ]]>\n"
                      "</style>\n";
        }

    if (options.m_includeLayoutToggle)
        {
        svgContent += wxString::Format(
            L"<script type=\"text/javascript\"><![CDATA[\n"
            "  let isDuplex = false;\n"
            "  let currentGap = %d;\n"
            "  function toggleLayout() {\n"
            "    isDuplex = !isDuplex;\n"
            "    const btnText = document.getElementById('toggle-btn-text');\n"
            "    if (btnText) btnText.textContent = isDuplex ? '\U0001F4C4 %s' : "
            "'\U0001F4C4\U0001F4C4 %s';\n"
            "    applyLayout();\n"
            "  }\n"
            "  function adjustGap(delta) {\n"
            "    currentGap = Math.max(0, currentGap + delta);\n"
            "    applyLayout();\n"
            "  }\n"
            "  function applyLayout() {\n"
            "    const pages = document.querySelectorAll('.page');\n"
            "    const svg = document.querySelector('svg');\n"
            "    if (pages.length === 0) return;\n"
            "    const w = parseInt(pages[0].getAttribute('data-width'));\n"
            "    const h = parseInt(pages[0].getAttribute('data-height'));\n"
            "    const gap = currentGap;\n"
            "    const sideGap = 25;\n"
            "    if (isDuplex) {\n"
            "      pages.forEach((p, i) => {\n"
            "        const x = (i %% 2) * (w + sideGap);\n"
            "        const y = Math.floor(i / 2) * (h + gap);\n"
            "        p.setAttribute('transform', `translate(${x}, ${y})`);\n"
            "      });\n"
            "      const duplexHeight = Math.ceil(pages.length / 2) * (h + gap);\n"
            "      svg.setAttribute('viewBox', `0 0 ${2 * w + sideGap} ${duplexHeight}`);\n"
            "      svg.setAttribute('height', duplexHeight);\n"
            "    } else {\n"
            "      pages.forEach((p, i) => {\n"
            "        p.setAttribute('transform', `translate(0, ${i * (h + gap)})`);\n"
            "      });\n"
            "      const stackedHeight = pages.length * (h + gap);\n"
            "      svg.setAttribute('viewBox', `0 0 ${w} ${stackedHeight}`);\n"
            "      svg.setAttribute('height', stackedHeight);\n"
            "    }\n"
            "  }\n"
            "]]></script>\n",
            options.m_horizontalGap, _(L"STACKED"), _(L"DUPLEX"));
        }

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
        const int layoutWidth =
            useOverrideSize ? options.m_pageSize.GetWidth() : renderSize.GetWidth();
        const int layoutHeight =
            useOverrideSize ? options.m_pageSize.GetHeight() : renderSize.GetHeight();
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

        yOffset += (layoutHeight + options.m_horizontalGap);
        }

    svgContent += L"</g>\n";

    if (options.m_includeLayoutToggle)
        {
        svgContent += wxString::Format(
            L"<g id=\"ui-layer\">\n"
            "  <rect class=\"btn\" x=\"10\" y=\"10\" width=\"120\" height=\"30\" rx=\"15\" "
            "onclick=\"toggleLayout()\" />\n"
            "  <text id=\"toggle-btn-text\" class=\"btn-text\" x=\"70\" y=\"29\">"
            "\U0001F4C4\U0001F4C4 %s</text>\n"
            "  <rect class=\"btn\" x=\"10\" y=\"50\" width=\"30\" height=\"30\" rx=\"15\" "
            "onclick=\"adjustGap(-10)\" />\n"
            "  <text class=\"btn-text\" x=\"25\" y=\"69\">-</text>\n"
            "  <rect class=\"btn\" x=\"50\" y=\"50\" width=\"140\" height=\"30\" rx=\"15\" />\n"
            "  <text class=\"btn-text\" x=\"120\" y=\"69\">%s</text>\n"
            "  <rect class=\"btn\" x=\"200\" y=\"50\" width=\"30\" height=\"30\" rx=\"15\" "
            "onclick=\"adjustGap(10)\" />\n"
            "  <text class=\"btn-text\" x=\"215\" y=\"69\">+</text>\n"
            "</g>\n",
            _(L"DUPLEX"), _(L"PAGE GAP"));
        }

    svgContent += L"</svg>\n";

    wxFile outFile(options.m_filePath, wxFile::write);
    if (outFile.IsOpened())
        {
        outFile.Write(svgContent, wxConvUTF8);
        }
    else
        {
        wxMessageBox(
            wxString::Format(_(L"Failed to save SVG report to \"%s\"."), options.m_filePath),
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
