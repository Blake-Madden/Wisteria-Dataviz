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
#include <algorithm>
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
                                               SVGReportOptions options)
    {
    // refresh auto-generated accessibility text on every page before rendering,
    // so descriptions reflect each canvas's current state
    for (auto* canvas : canvases)
        {
        if (canvas != nullptr)
            {
            canvas->ApplyAutoAccessibilityAttributes();
            }
        }

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

    // if only one page, then don't need duplex and such options
    if (canvases.size() < 2)
        {
        options.LayoutOptions(false).Slideshow(false);
        }

    // collect distinct layers in order of first appearance (empty layer means always visible)
    std::vector<wxString> distinctLayers;
    for (const auto* canvas : canvases)
        {
        if (canvas != nullptr && !canvas->GetLayer().empty() &&
            std::find(distinctLayers.cbegin(), distinctLayers.cend(), canvas->GetLayer()) ==
                distinctLayers.cend())
            {
            distinctLayers.push_back(canvas->GetLayer());
            }
        }
    const bool hasLayerControls = options.HasLayerControls(distinctLayers);

    // the layout size controls the viewBox and page spacing;
    // the rendering size (pageSizes) stays at each canvas's own paper size
    const bool useOverrideSize =
        !options.m_useGlobalPrintSettings && (options.m_pageSize != wxDefaultSize);
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
            totalHeight += PAGE_GAP;
            }
        }

    // layer checkbox grid geometry, shared by the toolbar-height calc and the SVG rendering
    constexpr int LAYER_ITEM_WIDTH{ 160 };
    constexpr int LAYER_ROW_HEIGHT{ 28 };
    constexpr int LAYER_BAR_PADDING{ 12 };
    const int layerItemsPerRow = std::max(1, maxWidth / LAYER_ITEM_WIDTH);
    const int layerRowCount =
        hasLayerControls ?
            static_cast<int>((distinctLayers.size() + static_cast<size_t>(layerItemsPerRow) - 1) /
                             static_cast<size_t>(layerItemsPerRow)) :
            0;

    // toolbar height: button bar (50px if layout/darkmode buttons) + layer bar + nav hint
    const int buttonBarHeight{ (options.m_includeLayoutOptions || options.m_includeDarkModeToggle) ?
                                   50 :
                                   0 };
    const int layerBarHeight{ hasLayerControls ?
                                  layerRowCount * LAYER_ROW_HEIGHT + LAYER_BAR_PADDING :
                                  0 };
    const int navHintBarHeight{ options.m_includeSlideshow ? 16 : 0 };
    const int toolbarHeight{ buttonBarHeight + layerBarHeight + navHintBarHeight };

    wxString svgContent;

    // build the main body first
    svgContent +=
        wxString::Format(L"<g id=\"pageset\" transform=\"translate(0,%d)\">\n", toolbarHeight);

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
                // Normally, calling SetSize before CalcRowDimensions() is not necessary,
                // but for SVG we need to because some internals look at the window size.
                // Note that doing this in report printout breaks things doing this,
                // this is an SVG only quirk.
                canvas->SetSize(canvas->FromDIP(wxSize(layoutWidth, scaledHeight)));
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
            canvas->SetSize(origSize);
            canvas->CalcRowDimensions();
            canvas->SetSize(origSize);
            }
        canvas->ResetResizeDelay();
        canvas->ZoomReset();
        canvas->SendSizeEvent();
        canvas->Refresh();

        const wxString escapedLayer = EscapeXmlAttr(canvas->GetLayer());
        svgContent += wxString::Format(
            L"<g class=\"page\" data-index=\"%zu\" data-layer=\"%s\" data-width=\"%d\" "
            "data-height=\"%d\" transform=\"translate(0,%d)\"%s>\n",
            pageIndex - 1, escapedLayer, layoutWidth, layoutHeight, yOffset,
            options.m_includePageShadow ? L" filter=\"url(#page-shadow)\"" : L"");
        svgContent += StripSvgTags(svgDC.GetSVGDocument());

        if (options.m_includeDarkModeToggle)
            {
            svgContent += wxString::Format(
                L"\n<rect class=\"page-outline\" x=\"0\" y=\"0\" width=\"%d\" "
                "height=\"%d\" fill=\"none\" stroke=\"#CCCCCC\" stroke-width=\"2\"/>",
                layoutWidth, layoutHeight);
            }

        svgContent += L"\n</g>\n";

        yOffset += (layoutHeight + PAGE_GAP);
        }

    svgContent += L"</g>\n";

    // build the header and SVGReportOptions feature and prepend it into the content
    wxString header;

    // when the slideshow is on, make the root focusable so keyboard users can Tab
    // into the report and drive page navigation even when it is embedded in a page
    wxString svgFocusAttrs;
    if (options.m_includeSlideshow)
        {
        // name the focusable region with the report's own title, falling back to a
        // generic label, so screen readers announce something meaningful
        wxString reportTitle;
        for (const auto* canvas : canvases)
            {
            if (canvas != nullptr && !canvas->GetLabel().empty())
                {
                reportTitle = canvas->GetLabel();
                break;
                }
            }
        if (reportTitle.empty())
            {
            reportTitle = _(L"Report");
            }
        svgFocusAttrs = wxString::Format(
            L"tabindex=\"0\" aria-label=\"%s\" ",
            EscapeXmlAttr(
                reportTitle + L". " +
                _(L"Use the arrow keys or Page Up and Page Down to move between pages.")));
        }

    header += L"<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"no\"?>\n";
    header += L"<!DOCTYPE svg PUBLIC \"-//W3C//DTD SVG 1.1//EN\" "
              "\"http://www.w3.org/Graphics/SVG/1.1/DTD/svg11.dtd\">\n";
    header += wxString::Format(L"<svg xmlns=\"http://www.w3.org/2000/svg\" "
                               "xmlns:xlink=\"http://www.w3.org/1999/xlink\" version=\"1.1\" "
                               "width=\"100%%\" height=\"%d\" %s"
                               "preserveAspectRatio=\"xMidYMin meet\" viewBox=\"0 0 %d %d\">\n",
                               totalHeight + toolbarHeight, svgFocusAttrs, maxWidth,
                               totalHeight + toolbarHeight);

    if (options.HasInteractiveFeatures() || hasLayerControls)
        {
        header += L"<style type=\"text/css\">\n"
                  "  <![CDATA[\n";

        if (options.m_includeTransitions)
            {
            header += L"    .page { transition: transform 0.6s cubic-bezier(0.4, 0, 0.2, 1); }\n";
            }
        if (options.m_includeHighlighting)
            {
            header += L"    .page rect:hover, .page circle:hover, .page path:hover { \n"
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
                "    .page.active-page { animation: page-arrive 0.8s ease-out forwards; }\n",
                options.m_themeColor.Red(), options.m_themeColor.Green(),
                options.m_themeColor.Blue(), options.m_themeColor.Red(),
                options.m_themeColor.Green(), options.m_themeColor.Blue(),
                options.m_themeColor.Red(), options.m_themeColor.Green(),
                options.m_themeColor.Blue());
            }

        if (options.HasUILayer() || hasLayerControls)
            {
            const wxString btnHex = options.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX);
            const wxString textHex =
                Wisteria::Colors::ColorContrast::IsLight(options.m_themeColor) ? L"black" :
                                                                                 L"white";

            header += wxString::Format(
                L"    #ui-layer { \n"
                "      pointer-events: auto;\n"
                "    }\n"
                "    .btn { fill: %s; cursor: pointer; transition: transform 0.1s, fill 0.2s; "
                "fill-opacity: 0.9; }\n"
                "    .btn:hover { fill-opacity: 1.0; transform: translateY(-1px); }\n"
                "    .btn-text { fill: %s; font-family: sans-serif; font-size: 12px; font-weight: "
                "bold; pointer-events: none; text-anchor: middle; }\n"
                "    .btn:focus-visible, .layer-toggle:focus-visible "
                "{ outline: 2px solid Highlight; outline-offset: 2px; }\n"
                "    .btn:focus:not(:focus-visible), .layer-toggle:focus:not(:focus-visible) "
                "{ outline: none; }\n"
                // fallback focus ring for renderers that don't paint outline on SVG nodes
                "    .btn:focus-visible { fill-opacity: 1.0; stroke: Highlight; stroke-width: 2; "
                "}\n"
                "    .layer-toggle:focus-visible rect.box { stroke-width: 2.5; }\n"
                "    @media print { #ui-layer { display: none; } }\n",
                btnHex, textHex);
            }
        if (hasLayerControls)
            {
            const wxString layerBtnHex = options.m_themeColor.GetAsString(wxC2S_HTML_SYNTAX);
            // lighter variant for dark mode (mix 50% towards white)
            const int r = options.m_themeColor.Red();
            const int g = options.m_themeColor.Green();
            const int b = options.m_themeColor.Blue();
            const wxColour lightLayerColor(r + (255 - r) / 2, g + (255 - g) / 2, b + (255 - b) / 2);
            const wxString layerBtnLightHex = lightLayerColor.GetAsString(wxC2S_HTML_SYNTAX);
            header += wxString::Format(
                L"    .page.hidden { display: none; }\n"
                "    .layer-toggle { cursor: pointer; }\n"
                "    .layer-toggle rect.box { fill: white; stroke: %s; stroke-width: 1.5; }\n"
                "    .layer-toggle.disabled { opacity: 0.45; }\n"
                "    .layer-toggle .checkmark { pointer-events: none; font-family: sans-serif; "
                "font-size: 12px; font-weight: bold; fill: %s; }\n"
                "    .layer-toggle .layer-label { pointer-events: none; font-family: sans-serif; "
                "font-size: 12px; fill: #222222; }\n"
                "    @media print { .page.hidden { display: none !important; } }\n",
                layerBtnHex, layerBtnHex);
            if (options.m_includeDarkModeToggle)
                {
                header += wxString::Format(
                    L"    svg.dark-mode .layer-toggle rect.box { fill: #222222; stroke: %s; }\n"
                    "    svg.dark-mode .layer-toggle .checkmark { fill: %s; }\n"
                    "    svg.dark-mode .layer-toggle .layer-label { fill: #E8E8E8; }\n",
                    layerBtnLightHex, layerBtnLightHex);
                }
            }
        if (options.m_includeSlideshow)
            {
            header += L"    .nav-hint { pointer-events: none; font-family: sans-serif; font-size: "
                      L"12px; fill: #000000; }\n"
                      "    @media print { .nav-hint { display: none; } }\n";
            if (options.m_includeDarkModeToggle)
                {
                header += L"    svg.dark-mode .nav-hint { fill: #E8E8E8; }\n";
                }
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
                      "{ stroke: #e8e8e8; }\n"
                      "      svg.dark-mode [fill=\"#1F4387\"] { fill: #87CEFA; }\n"
                      "      svg.dark-mode [stroke=\"#1F4387\"] { stroke: #87CEFA; }\n" +
                      GenerateDarkModeFillReplacements(std::wstring_view{ svgContent }) +
                      L"      svg.dark-mode [stroke=\"#FFFFFF\"] { stroke: #000000; }\n"
                      "      svg.dark-mode #svg-bg { fill: #000000; }\n"
                      "      svg.dark-mode .page-outline "
                      "{ stroke: #FFFFFF; stroke-width: 2; fill: none; }\n"
                      "    }\n";
            }
        header += L"  ]]>\n"
                  "</style>\n";
        }

    if (options.HasUILayer() || hasLayerControls)
        {
        header += L"<script type=\"text/javascript\"><![CDATA[\n";

        // page-visibility helpers for layer filtering; with no layers every page
        // stays visible, so applyLayout() and the slideshow can use them unconditionally
        const bool needsPageHelpers =
            hasLayerControls || options.m_includeLayoutOptions || options.m_includeSlideshow;
        if (needsPageHelpers)
            {
            wxString jsLayersArray;
            bool firstLayer{ true };
            for (const auto& layer : distinctLayers)
                {
                if (!firstLayer)
                    {
                    jsLayersArray += L", ";
                    }
                jsLayersArray += wxString::Format(L"'%s'", EscapeJsString(layer));
                firstLayer = false;
                }
            header += wxString::Format(L"  const allLayers = [%s];\n"
                                       "  const activeLayers = new Set(allLayers);\n"
                                       "  function isPageVisible(page) {\n"
                                       "    const l = page.getAttribute('data-layer') || '';\n"
                                       "    return !l || activeLayers.has(l);\n"
                                       "  }\n"
                                       "  function getVisiblePages() {\n"
                                       "    return Array.from(document.querySelectorAll('.page'))\n"
                                       "      .filter(p => !p.classList.contains('hidden'));\n"
                                       "  }\n",
                                       jsLayersArray);
            }

        if (hasLayerControls)
            {
            header += L"  function toggleLayer(layer) {\n"
                      "    if (activeLayers.has(layer)) activeLayers.delete(layer);\n"
                      "    else activeLayers.add(layer);\n"
                      "    document.querySelectorAll('.layer-toggle').forEach(g => {\n"
                      "      const l = g.getAttribute('data-layer');\n"
                      "      const on = activeLayers.has(l);\n"
                      "      g.classList.toggle('disabled', !on);\n"
                      "      g.setAttribute('aria-checked', on ? 'true' : 'false');\n"
                      "      const check = g.querySelector('.checkmark');\n"
                      "      if (check) check.style.display = on ? 'block' : 'none';\n"
                      "    });\n"
                      "    applyLayout();\n"
                      "  }\n";
            }

        // layout cycle: 0=single, 1=duplex, 2=stacked (books)
        const int initialLayout =
            (options.m_layout == Wisteria::SVGReportOptions::PageLayout::Duplex)  ? 1 :
            (options.m_layout == Wisteria::SVGReportOptions::PageLayout::Stacked) ? 2 :
                                                                                    0;
        if (options.m_includeLayoutOptions || hasLayerControls)
            {
            header += wxString::Format(L"  let layout = %d;\n", initialLayout);

            if (options.m_includeLayoutOptions)
                {
                header += wxString::Format(
                    L"  function toggleLayout() {\n"
                    "    const needsRestore = layout === 2;\n"
                    "    layout = (layout + 1) %% 3;\n"
                    "    if (needsRestore) {\n"
                    "      const ps = document.getElementById('pageset');\n"
                    "      Array.from(document.querySelectorAll('.page'))\n"
                    "        .sort((a,b) => parseInt(a.getAttribute('data-index')) - "
                    "parseInt(b.getAttribute('data-index')))\n"
                    "        .forEach(p => ps.appendChild(p));\n"
                    "      window.stackedInitialized = false;\n"
                    "      if (typeof currentPage !== 'undefined') currentPage = 0;\n"
                    "    }\n"
                    "    const btnText = document.getElementById('toggle-btn-text');\n"
                    "    if (btnText) {\n"
                    "      if (layout === 0) btnText.textContent = '\U0001F4C4\U0001F4C4 %s';\n"
                    "      else if (layout === 1) btnText.textContent = '\U0001F4DA %s';\n"
                    "      else btnText.textContent = '\U0001F4C4 %s';\n"
                    "    }\n"
                    "    if (needsRestore) {\n"
                    "      requestAnimationFrame(() => requestAnimationFrame(() => "
                    "applyLayout()));\n"
                    "    } else {\n"
                    "      applyLayout();\n"
                    "    }\n"
                    "  }\n",
                    _(L"Duplex"), _(L"Stacked"), _(L"Single"));
                }

            header += wxString::Format(
                L"  function applyLayout() {\n"
                "    const pages = document.querySelectorAll('.page');\n"
                "    const svg = document.querySelector('svg');\n"
                "    const pageset = document.getElementById('pageset');\n"
                "    if (pages.length === 0) return;\n"
                "    pages.forEach(p => {\n"
                "      const vis = isPageVisible(p);\n"
                "      p.classList.toggle('hidden', !vis);\n"
                "      p.style.display = vis ? '' : 'none';\n"
                "    });\n"
                "    const visible = getVisiblePages();\n"
                "    const w = parseInt(pages[0].getAttribute('data-width'));\n"
                "    const h = parseInt(pages[0].getAttribute('data-height'));\n"
                "    const gap = %d;\n"
                "    const sideGap = 25;\n"
                "    const topOffset = %d;\n"
                "    const stackedOffset = 18;\n"
                "    if (visible.length === 0) {\n"
                "      svg.setAttribute('viewBox', `0 0 ${w} ${topOffset}`);\n"
                "      svg.setAttribute('height', topOffset);\n"
                "      return;\n"
                "    }\n"
                "    if (layout === 1) {\n"
                "      visible.forEach((p, i) => {\n"
                "        const x = (i %% 2) * (w + sideGap);\n"
                "        const y = Math.floor(i / 2) * (h + gap);\n"
                "        p.style.transform = `translate(${x}px, ${y}px)`;\n"
                "      });\n"
                "      const duplexHeight = topOffset + Math.ceil(visible.length / 2) * (h + "
                "gap);\n"
                "      svg.setAttribute('viewBox', `0 0 ${2 * w + sideGap} ${duplexHeight}`);\n"
                "      svg.setAttribute('height', duplexHeight);\n"
                "    } else if (layout === 2) {\n"
                "      if (!window.stackedInitialized) {\n"
                "        for (let i = visible.length - 1; i >= 0; --i) "
                "pageset.appendChild(visible[i]);\n"
                "        window.stackedInitialized = true;\n"
                "      }\n"
                "      const visibleNow = getVisiblePages();\n"
                "      visibleNow.forEach((p, i) => {\n"
                "        const x = (visibleNow.length - 1 - i) * stackedOffset;\n"
                "        const y = (visibleNow.length - 1 - i) * stackedOffset;\n"
                "        p.style.transform = `translate(${x}px, ${y}px)`;\n"
                "      });\n"
                "      const carW = w + (visibleNow.length - 1) * stackedOffset;\n"
                "      const carH = topOffset + h + (visibleNow.length - 1) * stackedOffset;\n"
                "      svg.setAttribute('viewBox', `0 0 ${carW} ${carH}`);\n"
                "      svg.setAttribute('height', carH);\n"
                "    } else {\n"
                "      visible.forEach((p, i) => {\n"
                "        p.style.transform = `translate(0, ${i * (h + gap)}px)`;\n"
                "      });\n"
                "      const stackedHeight = topOffset + visible.length * (h + gap);\n"
                "      svg.setAttribute('viewBox', `0 0 ${w} ${stackedHeight}`);\n"
                "      svg.setAttribute('height', stackedHeight);\n"
                "    }\n"
                "  }\n"
                "  window.addEventListener('load', applyLayout);\n",
                PAGE_GAP, toolbarHeight);
            }

        if (options.m_includeDarkModeToggle)
            {
            header += L"  function toggleDarkMode() {\n"
                      "    const svg = document.querySelector('svg');\n"
                      "    svg.classList.toggle('dark-mode');\n"
                      "    const on = svg.classList.contains('dark-mode');\n"
                      "    const btn = document.getElementById('darkmode-btn-text');\n"
                      "    if (btn) btn.textContent = on ? '\u2600\uFE0F' : '\U0001F319';\n"
                      "    const box = document.getElementById('darkmode-btn');\n"
                      "    if (box) box.setAttribute('aria-pressed', on ? 'true' : 'false');\n"
                      "  }\n";
            }

        // wire pointer and keyboard activation for the overlay controls
        if (options.m_includeLayoutOptions || options.m_includeDarkModeToggle || hasLayerControls)
            {
            header += L"  window.addEventListener('load', function() {\n"
                      "    function onActivate(el, fn) {\n"
                      "      if (!el) return;\n"
                      "      el.addEventListener('click', fn);\n"
                      "      el.addEventListener('keydown', function(e) {\n"
                      "        if (e.key === 'Enter' || e.key === ' ' || e.key === 'Spacebar')\n"
                      "          { e.preventDefault(); fn(e); }\n"
                      "      });\n"
                      "    }\n";
            if (options.m_includeLayoutOptions)
                {
                header += L"    onActivate(document.getElementById('layout-btn'), toggleLayout);\n";
                }
            if (options.m_includeDarkModeToggle)
                {
                header +=
                    L"    onActivate(document.getElementById('darkmode-btn'), toggleDarkMode);\n";
                }
            if (hasLayerControls)
                {
                header += L"    document.querySelectorAll('.layer-toggle').forEach(function(g) {\n"
                          "      const layer = g.getAttribute('data-layer');\n"
                          "      onActivate(g, function() { toggleLayer(layer); });\n"
                          "    });\n";
                }
            header += L"  });\n";
            }

        if (options.m_includeSlideshow)
            {
            header +=
                L"  let currentPage = 0;\n"
                "  function goToPage(i) {\n"
                "    if (typeof layout !== 'undefined' && layout === 2) {\n"
                "      const pages = getVisiblePages();\n"
                "      if (pages.length <= 1) return;\n"
                "      i = Math.max(0, Math.min(i, pages.length - 1));\n"
                "      const pageset = document.getElementById('pageset');\n"
                "      const steps = (i - currentPage + pages.length) % pages.length;\n"
                "      for (let s = 0; s < steps; ++s) {\n"
                "        const pagesNow = getVisiblePages();\n"
                "        const last = pagesNow[pagesNow.length - 1];\n"
                "        if (last) pageset.insertBefore(last, pagesNow[0]);\n"
                "      }\n"
                "      currentPage = i;\n"
                "      applyLayout();\n"
                "      const newTop = getVisiblePages()[getVisiblePages().length - 1];\n"
                "      if (newTop) {\n"
                "        newTop.classList.add('active-page');\n"
                "        newTop.addEventListener('animationend', () => "
                "newTop.classList.remove('active-page'), { once: true });\n"
                "      }\n"
                "      return;\n"
                "    }\n"
                "    const pages = getVisiblePages();\n"
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
                "  }\n"
                "  function prevPage() {\n"
                "    if (typeof layout !== 'undefined' && layout === 2) {\n"
                "      const pages = getVisiblePages();\n"
                "      if (pages.length <= 1) return;\n"
                "      const pageset = document.getElementById('pageset');\n"
                "      const first = pages[0];\n"
                "      if (first) pageset.appendChild(first);\n"
                "      currentPage = (currentPage - 1 + pages.length) % pages.length;\n"
                "      applyLayout();\n"
                "      return;\n"
                "    }\n"
                "    goToPage(currentPage - 1);\n"
                "  }\n"
                "  function nextPage() {\n"
                "    if (typeof layout !== 'undefined' && layout === 2) {\n"
                "      const pages = getVisiblePages();\n"
                "      if (pages.length <= 1) return;\n"
                "      const pageset = document.getElementById('pageset');\n"
                "      const last = pages[pages.length - 1];\n"
                "      if (last) pageset.insertBefore(last, pages[0]);\n"
                "      currentPage = (currentPage + 1) % pages.length;\n"
                "      applyLayout();\n"
                "      return;\n"
                "    }\n"
                "    goToPage(currentPage + 1);\n"
                "  }\n"
                // only claim the arrow / page keys when this SVG is the whole document
                // or actually holds focus, so an embedding page keeps normal scrolling
                "  const navSvg = document.querySelector('svg');\n"
                "  const navStandalone = "
                "document.documentElement.tagName.toLowerCase() === 'svg';\n"
                "  window.addEventListener('keydown', function(e) {\n"
                "    if (e.defaultPrevented || e.ctrlKey || e.metaKey || e.altKey) return;\n"
                "    if (!navStandalone && navSvg && !navSvg.contains(document.activeElement)) "
                "return;\n"
                "    if (e.key === 'ArrowLeft' || e.key === 'ArrowUp' || e.key === 'PageUp')\n"
                "      { e.preventDefault(); prevPage(); }\n"
                "    else if (e.key === 'ArrowRight' || e.key === 'ArrowDown' || e.key === "
                "'PageDown')\n"
                "      { e.preventDefault(); nextPage(); }\n"
                "  });\n";
            }

        header += L"]]></script>\n";
        }

    if (options.m_includePageShadow)
        {
        header +=
            L"<defs>\n"
            "  <filter id=\"page-shadow\" x=\"-5%\" y=\"-5%\" width=\"115%\" height=\"115%\">\n"
            "    <feGaussianBlur in=\"SourceAlpha\" stdDeviation=\"4\" result=\"blur\"/>\n"
            "    <feOffset dx=\"6\" dy=\"6\" result=\"offsetBlur\"/>\n"
            "    <feFlood flood-color=\"#000000\" flood-opacity=\"0.5\" result=\"color\"/>\n"
            "    <feComposite in=\"color\" in2=\"offsetBlur\" operator=\"in\" result=\"shadow\"/>\n"
            "    <feMerge>\n"
            "      <feMergeNode in=\"shadow\"/>\n"
            "      <feMergeNode in=\"SourceGraphic\"/>\n"
            "    </feMerge>\n"
            "  </filter>\n"
            "</defs>\n";
        }

    if (options.m_includeDarkModeToggle)
        {
        header += L"<rect id=\"svg-bg\" x=\"0\" y=\"0\" width=\"100%\" height=\"100%\" "
                  "fill=\"#FFFFFF\"/>\n";
        }

    svgContent.Prepend(header);

    if (options.HasUILayer() || hasLayerControls || options.m_includeSlideshow)
        {
        svgContent += L"<g id=\"ui-layer\">\n";

        // Buttons are right-aligned; btnRight is the right edge of the rightmost button.
        const int btnRight = maxWidth - 10;

        if (options.m_includeLayoutOptions)
            {
            wxString layoutIcon;
            wxString layoutLabel;
            // show next layout (what clicking will switch to), like dark-mode button
            if (options.m_layout == Wisteria::SVGReportOptions::PageLayout::Single)
                {
                layoutIcon = L"\U0001F4C4\U0001F4C4";
                layoutLabel = _(L"Duplex");
                }
            else if (options.m_layout == Wisteria::SVGReportOptions::PageLayout::Duplex)
                {
                layoutIcon = L"\U0001F4DA";
                layoutLabel = _(L"Stacked");
                }
            else
                {
                layoutIcon = L"\U0001F4C4";
                layoutLabel = _(L"Single");
                }
            // leave room for dark-mode button to the right if present
            const int layoutX =
                options.m_includeDarkModeToggle ? btnRight - 30 - 10 - 120 : btnRight - 120;
            svgContent += wxString::Format(
                L"  <rect id=\"layout-btn\" class=\"btn\" x=\"%d\" y=\"10\" width=\"120\" "
                "height=\"30\" rx=\"15\" tabindex=\"0\" role=\"button\" aria-label=\"%s\">"
                "<title>%s</title></rect>\n"
                "  <text id=\"toggle-btn-text\" class=\"btn-text\" x=\"%d\" y=\"29\">"
                "%s %s</text>\n",
                layoutX, EscapeXmlAttr(_(L"Change page layout")),
                _(L"Toggle between single, duplex and stacked page layout"), layoutX + 60,
                layoutIcon, layoutLabel);
            }

        if (options.m_includeDarkModeToggle)
            {
            const int dmX = btnRight - 30;
            svgContent += wxString::Format(
                L"  <rect id=\"darkmode-btn\" class=\"btn\" x=\"%d\" y=\"10\" width=\"30\" "
                "height=\"30\" rx=\"15\" tabindex=\"0\" role=\"button\" aria-pressed=\"false\" "
                "aria-label=\"%s\"><title>%s</title></rect>\n"
                "  <text id=\"darkmode-btn-text\" class=\"btn-text\" x=\"%d\" y=\"29\">"
                "\U0001F319</text>\n",
                dmX, EscapeXmlAttr(_(L"Dark mode")), _(L"Toggle dark mode"), dmX + 15);
            }

        if (hasLayerControls)
            {
            // layer checkboxes: pure SVG, left-aligned, below button bar
            constexpr int boxSize{ 14 };
            const int startY = buttonBarHeight + LAYER_BAR_PADDING / 2;
            int idx{ 0 };
            for (const auto& layer : distinctLayers)
                {
                const int col = idx % layerItemsPerRow;
                const int row = idx / layerItemsPerRow;
                const int x = 10 + col * LAYER_ITEM_WIDTH;
                const int y = startY + row * LAYER_ROW_HEIGHT;
                const wxString escAttr = EscapeXmlAttr(layer);
                const wxString escText = EscapeXmlText(layer);
                // activation is wired up in JS by data-layer; keep the markup handler-free
                svgContent += wxString::Format(
                    L"  <g class=\"layer-toggle\" data-layer=\"%s\" tabindex=\"0\" "
                    "role=\"checkbox\" aria-checked=\"true\" aria-label=\"%s\">\n"
                    "    <rect class=\"box\" x=\"%d\" y=\"%d\" width=\"%d\" height=\"%d\" "
                    "rx=\"3\"/>\n"
                    "    <text class=\"checkmark\" x=\"%d\" y=\"%d\">\u2713</text>\n"
                    "    <text class=\"layer-label\" x=\"%d\" y=\"%d\">%s</text>\n"
                    "    <title>%s</title>\n"
                    "  </g>\n",
                    escAttr, escAttr, x, y, boxSize, boxSize, x + 3, y + 11, x + boxSize + 6,
                    y + 11, escText, escAttr);
                ++idx;
                }
            }

        if (options.m_includeSlideshow)
            {
            const int tipY = toolbarHeight - 6;
            svgContent += wxString::Format(
                L"  <text class=\"nav-hint\" x=\"10\" y=\"%d\" font-family=\"sans-serif\" "
                "font-size=\"12\" fill=\"#000000\">%s</text>\n",
                tipY, _(L"Tip: Use arrow keys or Page Up/Down to navigate pages"));
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
    constexpr double TENTHS_MM_PER_INCH = 254.0;
    constexpr double DIPS_PER_INCH = 96.0;
    if (paperType != nullptr)
        {
        const wxSize sizeMM = paperType->GetSize();
        const int widthDIPs =
            wxRound(safe_divide<double>(sizeMM.GetWidth(), TENTHS_MM_PER_INCH) * DIPS_PER_INCH);
        const int heightDIPs =
            wxRound(safe_divide<double>(sizeMM.GetHeight(), TENTHS_MM_PER_INCH) * DIPS_PER_INCH);

        if (printData.GetOrientation() == wxLANDSCAPE)
            {
            return { heightDIPs, widthDIPs };
            }
        return { widthDIPs, heightDIPs };
        }

    // fallback: US Letter (8.5" x 11") at 96 DPI
    const int widthDIPs = static_cast<int>(8.5 * DIPS_PER_INCH);
    const int heightDIPs = static_cast<int>(11 * DIPS_PER_INCH);
    return (printData.GetOrientation() == wxLANDSCAPE) ? wxSize{ heightDIPs, widthDIPs } :
                                                         wxSize{ widthDIPs, heightDIPs };
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

//------------------------------------------------
wxString Wisteria::SVGReportPrintout::EscapeXmlAttr(const wxString& str)
    {
    wxString result = str;
    result.Replace(L"&", L"&amp;");
    result.Replace(L"\"", L"&quot;");
    result.Replace(L"'", L"&apos;");
    result.Replace(L"<", L"&lt;");
    result.Replace(L">", L"&gt;");
    return result;
    }

//------------------------------------------------
wxString Wisteria::SVGReportPrintout::EscapeXmlText(const wxString& str)
    {
    wxString result = str;
    result.Replace(L"&", L"&amp;");
    result.Replace(L"<", L"&lt;");
    result.Replace(L">", L"&gt;");
    return result;
    }

//------------------------------------------------
wxString Wisteria::SVGReportPrintout::EscapeJsString(const wxString& str)
    {
    wxString result = str;
    result.Replace(L"\\", L"\\\\");
    result.Replace(L"'", L"\\'");
    result.Replace(L"\n", L"\\n");
    result.Replace(L"\r", wxString{});
    return result;
    }
