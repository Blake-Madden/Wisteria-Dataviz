///////////////////////////////////////////////////////////////////////////////
// Name:        svgreportprintout.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "svgreportprintout.h"
#include "../base/colorbrewer.h"
#include "../import/html_extract_text.h"
#include <set>
#include <wx/dcsvg.h>
#include <wx/file.h>
#include <wx/paper.h>
#include <wx/regex.h>

//------------------------------------------------------
wxString Wisteria::SVGReportPrintout::GenerateDarkModeFillReplacements(std::wstring_view svgContent)
    {
    wxString replacements;
    std::set<wxString> processedColors;

    while (!svgContent.empty())
        {
        auto fillStart = svgContent.find(L"fill=");
        if (fillStart == std::wstring_view::npos)
            {
            break;
            }
        fillStart = svgContent.find_first_of(L"'\"", fillStart);
        if (fillStart == std::wstring_view::npos)
            {
            break;
            }
        const auto fillEnd = svgContent.find(svgContent[fillStart], fillStart + 1);
        if (fillEnd == std::wstring_view::npos)
            {
            break;
            }
        ++fillStart;
        const wxString colorStr{ svgContent.substr(fillStart, (fillEnd - fillStart)) };
        if (!colorStr.empty() && !processedColors.contains(colorStr))
            {
            processedColors.insert(colorStr);
            const wxColour color{ colorStr };
            if (color.IsOk() && Colors::ColorContrast::IsVeryLight(color))
                {
                // if pure white, map to black
                if (color == *wxWHITE)
                    {
                    replacements += wxString::Format(
                        L"      svg.dark-mode [fill=\"%s\"] { fill: #000000; }\n", colorStr);
                    }
                else
                    {
                    // find a darker version
                    const wxColour darkColor =
                        Colors::ColorContrast::Shade(color, math_constants::third);
                    replacements +=
                        wxString::Format(L"      svg.dark-mode [fill=\"%s\"] { fill: %s; }\n",
                                         colorStr, darkColor.GetAsString(wxC2S_HTML_SYNTAX));
                    }
                }
            }
        svgContent.remove_prefix(fillEnd);
        }
    return replacements;
    }

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

    // build the main body first
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

        if (options.m_includeDarkModeToggle)
            {
            svgContent +=
                wxString::Format(L"\n<rect class=\"page-outline\" x=\"0\" y=\"0\" width=\"%d\" "
                                 "height=\"%d\" fill=\"none\" stroke=\"none\"/>",
                                 layoutWidth, layoutHeight);
            }

        svgContent += L"\n</g>\n";

        yOffset += (layoutHeight + options.m_horizontalGap);
        }

    svgContent += L"</g>\n";

    // build the header and SVGReportOptions feature and prepend it into the content
    wxString header;

    header += L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    header += L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
              "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    header += wxString::Format(L"<svg xmlns=\"http://www.w3.org/2000/svg\" "
                               "xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" "
                               "width=\"100%%\" height=\"%d\" "
                               "preserveAspectRatio=\"xMidYMin meet\" viewBox=\"0 0 %d %d\">\n",
                               totalHeight, maxWidth, totalHeight);

    if (options.HasInteractiveFeatures())
        {
        header += L"<style type=\"text/css\">\n"
                  "  <![CDATA[\n";
        if (options.m_includeTransitions)
            {
            header += L"    .page { transition: transform 0.6s cubic-bezier(0.4, 0, 0.2, 1); }\n";
            }
        if (options.m_includeHighlighting)
            {
            header += L"    .page:hover { filter: drop-shadow(0 0 10px rgba(0,0,0,0.1)); }\n"
                      "    .page rect:hover, .page circle:hover, .page path:hover { \n"
                      "      filter: brightness(1.2); cursor: pointer; \n"
                      "    }\n";
            }
        if (options.m_includeSlideshow)
            {
            header += wxString::Format(
                L"    @keyframes page-arrive {\n"
                "      0%%   { filter: drop-shadow(0 0 0px rgba(%d,%d,%d,0)); }\n"
                "      35%%  { filter: drop-shadow(0 0 20px rgba(%d,%d,%d,0.6)); }\n"
                "      100%% { filter: drop-shadow(0 0 0px rgba(%d,%d,%d,0)); }\n"
                "    }\n"
                "    .page.active-page { animation: page-arrive 0.8s ease-out forwards; }\n"
                "    #page-indicator { transition: opacity 0.15s ease; }\n",
                options.m_themeColor.Red(), options.m_themeColor.Green(),
                options.m_themeColor.Blue(), options.m_themeColor.Red(),
                options.m_themeColor.Green(), options.m_themeColor.Blue(),
                options.m_themeColor.Red(), options.m_themeColor.Green(),
                options.m_themeColor.Blue());
            }

        if (options.HasUILayer())
            {
            const wxString btnHex = options.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX);
            const wxString textHex =
                Wisteria::Colors::ColorContrast::IsLight(options.m_themeColor) ? L"black" :
                                                                                 L"white";

            header += wxString::Format(
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
        if (options.m_includeDarkModeToggle)
            {
            // For any background fills that don't play well with white text
            // (that is triggered via dark mode), map their dark-mode fill replacements here.
            //
            // Scope dark-mode rules to screen media only, so printing the SVG
            // always uses the original light colors even if dark mode is active.
            header += L"    @media screen {\n"
                      "      svg.dark-mode { background-color: #000000; }\n"
                      "      svg.dark-mode [fill=\"#000000\"], svg.dark-mode [fill=\"black\"] "
                      "{ fill: #e8e8e8; }\n"
                      "      svg.dark-mode [stroke=\"#000000\"], svg.dark-mode [stroke=\"black\"] "
                      "{ stroke: #e8e8e8; }\n" +
                      GenerateDarkModeFillReplacements(std::wstring_view{ svgContent }) +
                      L"      svg.dark-mode [stroke=\"#FFFFFF\"] { stroke: #000000; }\n"
                      "      svg.dark-mode #svg-bg { fill: #000000; }\n"
                      "      svg.dark-mode .page-outline "
                      "{ stroke: #e8e8e8; stroke-width: 2; fill: none; }\n"
                      "    }\n";
            }
        header += L"  ]]>\n"
                  "</style>\n";
        }

    if (options.HasUILayer())
        {
        header += L"<script type=\"text/javascript\"><![CDATA[\n";

        // keep the UI layer pinned to the top of the viewport while scrolling:
        // translate #ui-layer down by however many viewBox units correspond to
        // the portion of the SVG currently hidden above the viewport
        header += L"  function updateUILayerPosition() {\n"
                  "    const svg = document.querySelector('svg');\n"
                  "    const uiLayer = document.getElementById('ui-layer');\n"
                  "    if (!svg || !uiLayer) return;\n"
                  "    const rect = svg.getBoundingClientRect();\n"
                  "    const viewBox = svg.viewBox.baseVal;\n"
                  "    if (rect.height === 0 || viewBox.height === 0) return;\n"
                  "    const scaleY = viewBox.height / rect.height;\n"
                  "    const hiddenTop = Math.max(0, -rect.top);\n"
                  "    const offsetInSvg = hiddenTop * scaleY;\n"
                  "    uiLayer.setAttribute('transform', "
                  "`translate(0, ${offsetInSvg})`);\n"
                  "  }\n"
                  "  window.addEventListener('scroll', updateUILayerPosition, "
                  "{ passive: true });\n"
                  "  window.addEventListener('resize', updateUILayerPosition);\n"
                  "  window.addEventListener('load', updateUILayerPosition);\n";

        if (options.m_includeLayoutToggle)
            {
            header += wxString::Format(
                L"  let isDuplex = false;\n"
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
                "    updateUILayerPosition();\n"
                "  }\n",
                options.m_horizontalGap, _(L"Stacked"), _(L"Duplex"));
            }

        if (options.m_includeDarkModeToggle)
            {
            header += L"  function toggleDarkMode() {\n"
                      "    const svg = document.querySelector('svg');\n"
                      "    svg.classList.toggle('dark-mode');\n"
                      "    const btn = document.getElementById('darkmode-btn-text');\n"
                      "    if (btn) btn.textContent = svg.classList.contains('dark-mode') ? "
                      "'\u2600\uFE0F' : '\U0001F319';\n"
                      "  }\n";
            }

        if (options.m_includeSlideshow)
            {
            header +=
                L"  let currentPage = 0;\n"
                "  function goToPage(i) {\n"
                "    const pages = document.querySelectorAll('.page');\n"
                "    if (pages.length === 0) return;\n"
                "    i = Math.max(0, Math.min(i, pages.length - 1));\n"
                "    currentPage = i;\n"
                "    const svg = document.querySelector('svg');\n"
                "    const rect = svg.getBoundingClientRect();\n"
                "    const viewBox = svg.viewBox.baseVal;\n"
                "    if (rect.height === 0 || viewBox.height === 0) return;\n"
                "    const scaleY = rect.height / viewBox.height;\n"
                "    const page = pages[i];\n"
                "    const xfrm = page.getAttribute('transform') || '';\n"
                "    const match = "
                "xfrm.match(/translate\\(\\s*[\\d.]+\\s*,\\s*([\\d.]+)\\s*\\)/);\n"
                "    const pageY = match ? parseFloat(match[1]) : 0;\n"
                "    const svgTop = rect.top + window.scrollY;\n"
                "    window.scrollTo({ top: svgTop + pageY * scaleY, behavior: 'smooth' });\n"
                "    // arrival glow: remove class then re-add after two animation frames\n"
                "    pages.forEach(p => p.classList.remove('active-page'));\n"
                "    requestAnimationFrame(() => requestAnimationFrame(() => {\n"
                "      page.classList.add('active-page');\n"
                "      page.addEventListener('animationend',\n"
                "        () => page.classList.remove('active-page'), { once: true });\n"
                "    }));\n"
                "    // page indicator cross-fade\n"
                "    const ind = document.getElementById('page-indicator');\n"
                "    if (ind) {\n"
                "      ind.style.opacity = '0';\n"
                "      setTimeout(() => {\n"
                "        ind.textContent = `Page ${i + 1} / ${pages.length}`;\n"
                "        ind.style.opacity = '1';\n"
                "      }, 150);\n"
                "    }\n"
                "    // dim boundary buttons\n"
                "    const pb = document.getElementById('prev-page-btn');\n"
                "    const nb = document.getElementById('next-page-btn');\n"
                "    if (pb) pb.style.opacity = i === 0 ? '0.35' : '1';\n"
                "    if (nb) nb.style.opacity = i === pages.length - 1 ? '0.35' : '1';\n"
                "  }\n"
                "  function prevPage() { goToPage(currentPage - 1); }\n"
                "  function nextPage() { goToPage(currentPage + 1); }\n"
                "  window.addEventListener('keydown', function(e) {\n"
                "    if (e.key === 'ArrowLeft' || e.key === 'ArrowUp' || e.key === 'PageUp')\n"
                "      { e.preventDefault(); prevPage(); }\n"
                "    else if (e.key === 'ArrowRight' || e.key === 'ArrowDown' || e.key === "
                "'PageDown')\n"
                "      { e.preventDefault(); nextPage(); }\n"
                "  });\n"
                "  window.addEventListener('load', function() {\n"
                "    const pages = document.querySelectorAll('.page');\n"
                "    const ind = document.getElementById('page-indicator');\n"
                "    if (ind && pages.length > 0)\n"
                "      ind.textContent = `Page 1 / ${pages.length}`;\n"
                "    const nb = document.getElementById('next-page-btn');\n"
                "    if (nb) nb.style.opacity = pages.length <= 1 ? '0.35' : '1';\n"
                "  });\n";
            }

        header += L"]]></script>\n";
        }

    if (options.m_includeDarkModeToggle)
        {
        header += L"<rect id=\"svg-bg\" x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
                  "fill=\"#FFFFFF\"/>\n";
        }

    svgContent.Prepend(header);

    if (options.HasUILayer())
        {
        svgContent += L"<g id=\"ui-layer\">\n";

        if (options.m_includeLayoutToggle)
            {
            svgContent += wxString::Format(
                L"  <rect class=\"btn\" x=\"10\" y=\"10\" width=\"120\" height=\"30\" rx=\"15\" "
                "onclick=\"toggleLayout()\"><title>%s</title></rect>\n"
                "  <text id=\"toggle-btn-text\" class=\"btn-text\" x=\"70\" y=\"29\">"
                "\U0001F4C4\U0001F4C4 %s</text>\n"
                "  <rect class=\"btn\" x=\"10\" y=\"50\" width=\"30\" height=\"30\" rx=\"15\" "
                "onclick=\"adjustGap(-1)\"><title>%s</title></rect>\n"
                "  <text class=\"btn-text\" x=\"25\" y=\"69\">-</text>\n"
                "  <rect class=\"btn\" x=\"50\" y=\"50\" width=\"140\" height=\"30\" rx=\"15\" />\n"
                "  <text class=\"btn-text\" x=\"120\" y=\"69\">%s</text>\n"
                "  <rect class=\"btn\" x=\"200\" y=\"50\" width=\"30\" height=\"30\" rx=\"15\" "
                "onclick=\"adjustGap(1)\"><title>%s</title></rect>\n"
                "  <text class=\"btn-text\" x=\"215\" y=\"69\">+</text>\n",
                _(L"Toggle between stacked and duplex page layout"), _(L"Duplex"),
                _(L"Decrease vertical space between pages"), _(L"Page gap"),
                _(L"Increase vertical space between pages"));
            }

        if (options.m_includeDarkModeToggle)
            {
            const int darkModeX = options.m_includeLayoutToggle ? 140 : 10;
            svgContent += wxString::Format(
                L"  <rect class=\"btn\" x=\"%d\" y=\"10\" width=\"30\" height=\"30\" rx=\"15\" "
                "onclick=\"toggleDarkMode()\"><title>%s</title></rect>\n"
                "  <text id=\"darkmode-btn-text\" class=\"btn-text\" x=\"%d\" y=\"29\">"
                "\U0001F319</text>\n",
                darkModeX, _(L"Toggle dark mode"), darkModeX + 15);
            }

        if (options.m_includeSlideshow)
            {
            // place below whichever rows are already present:
            //   layout toggle adds rows at y=10 and y=50 -> slideshow at y=90
            //   dark mode only adds row at y=10              -> slideshow at y=50
            //   slideshow alone                              -> y=10
            const int slideshowY =
                options.m_includeLayoutToggle ? 90 : (options.m_includeDarkModeToggle ? 50 : 10);
            svgContent += wxString::Format(
                L"  <rect id=\"prev-page-btn\" class=\"btn\" x=\"10\" y=\"%d\" width=\"30\" "
                "height=\"30\" rx=\"15\" onclick=\"prevPage()\" style=\"opacity:0.35\">"
                "<title>%s</title></rect>\n"
                "  <text class=\"btn-text\" x=\"25\" y=\"%d\">\u25C0</text>\n"
                "  <rect class=\"btn\" x=\"50\" y=\"%d\" width=\"120\" height=\"30\" rx=\"15\" />\n"
                "  <text id=\"page-indicator\" class=\"btn-text\" x=\"110\" y=\"%d\">"
                "Page 1 / 1</text>\n"
                "  <rect id=\"next-page-btn\" class=\"btn\" x=\"180\" y=\"%d\" width=\"30\" "
                "height=\"30\" rx=\"15\" onclick=\"nextPage()\"><title>%s</title></rect>\n"
                "  <text class=\"btn-text\" x=\"195\" y=\"%d\">\u25B6</text>\n",
                slideshowY, _(L"Go to previous page"), slideshowY + 19, slideshowY, slideshowY + 19,
                slideshowY, _(L"Go to next page"), slideshowY + 19);
            }

        svgContent += L"</g>\n";
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
