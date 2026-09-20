///////////////////////////////////////////////////////////////////////////////
// Name:        reportbuilder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"
#include "../data/pivot.h"
#include "../data/subset.h"
#include <format>
#include <utility>

namespace Wisteria
    {
    //---------------------------------------------------
    std::vector<Canvas*> ReportBuilder::LoadConfigurationFile(const wxString& filePath,
                                                              wxWindow* parent)
        {
        // reset from previous calls
        m_tableLinks.clear();
        m_name.clear();
        m_datasets.clear();
        m_svgExportOptions = SVGReportOptions{ wxString{} };
        m_pdfExportOptions = PdfExportOptions{};
        m_powerPointExportOptions = PowerPointExportOptions{};
        m_svgExportOptionsLoaded = false;
        m_pdfExportOptionsLoaded = false;
        m_powerPointExportOptionsLoaded = false;
        m_resolvedMissingDatasets = false;
        m_dpiScaleFactor = parent->GetDPIScaleFactor();

        m_configFilePath = filePath;

        m_pageNumber = 1;

        std::vector<Canvas*> reportPages;

        wxASSERT_MSG(parent, L"Parent window must not be null when building a canvas!");
        if (parent == nullptr)
            {
            return reportPages;
            }
        const auto json = wxSimpleJSON::LoadFile(m_configFilePath);
        if (!json->IsOk())
            {
            wxLogError(L"%s", json->GetLastError());
            m_pendingErrorMessages.push_back(
                { _(L"Configuration File Parsing Error"), json->GetLastError() });
            return reportPages;
            }

        if (const auto reportNameNode = json->GetProperty(_DT(L"name")); reportNameNode->IsOk())
            {
            m_name = reportNameNode->AsString();
            }

        if (const auto subjectNode = json->GetProperty(_DT(L"subject")); subjectNode->IsOk())
            {
            m_subject = subjectNode->AsString();
            }

        if (const auto keywordsNode = json->GetProperty(_DT(L"keywords")); keywordsNode->IsOk())
            {
            m_keywords = keywordsNode->AsString();
            }

        const auto datasetsNode = json->GetProperty(L"datasets");
        try
            {
            LoadDatasets(datasetsNode);
            }
        catch (const std::exception& err)
            {
            const wxString msg{ wxString::FromUTF8(err.what()) };
            wxLogError(L"%s", msg);
            m_pendingErrorMessages.push_back({ _(L"Datasets Section Error"), msg });
            return reportPages;
            }

        try
            {
            LoadConstants(json->GetProperty(L"constants"));
            }
        catch (const std::exception& err)
            {
            const wxString msg{ wxString::FromUTF8(err.what()) };
            wxLogError(L"%s", msg);
            m_pendingErrorMessages.push_back({ _(L"Constants Section Error"), msg });
            return reportPages;
            }

        // Watermark, applied to all pages (can be overridden at individual page level).
        // These can support constants (including ones from the datasets),
        // so should be loaded after that section.
        const auto watermarkProperty = json->GetProperty(L"watermark");
        wxString reportWatermark;
        wxColour reportWatermarkColor;
        if (watermarkProperty->IsOk())
            {
            reportWatermark = ExpandConstants(watermarkProperty->GetProperty(L"label")->AsString());
            reportWatermarkColor = ConvertColor(watermarkProperty->GetProperty(L"color"));
            m_watermarkLabel = reportWatermark;
            m_watermarkColor = reportWatermarkColor;
            }

        // SVG export options
        if (const auto svgExportNode = json->GetProperty(L"svg-export"); svgExportNode->IsOk())
            {
            m_svgExportOptionsLoaded = true;
            const auto boolAttr = [&svgExportNode](const wxString& name, const bool fallback)
            {
                const auto prop = svgExportNode->GetProperty(name);
                return prop->IsOk() ? prop->AsBool(fallback) : fallback;
            };
            const auto numberAttr = [&svgExportNode](const wxString& name) -> std::optional<double>
            {
                const auto prop = svgExportNode->GetProperty(name);
                return prop->IsOk() ? std::optional<double>{ prop->AsDouble(-1) } : std::nullopt;
            };

            if (const auto pageWidth = numberAttr(L"page-width"); pageWidth && *pageWidth > 0)
                {
                m_svgExportOptions.m_pageSize.SetWidth(static_cast<int>(*pageWidth));
                }
            if (const auto pageHeight = numberAttr(L"page-height"); pageHeight && *pageHeight > 0)
                {
                m_svgExportOptions.m_pageSize.SetHeight(static_cast<int>(*pageHeight));
                }
            m_svgExportOptions.m_useGlobalPrintSettings =
                boolAttr(L"use-global-print-settings", m_svgExportOptions.m_useGlobalPrintSettings);
            if (const auto paperId = numberAttr(L"paper-id"); paperId && *paperId >= 0)
                {
                m_svgExportOptions.m_paperId = static_cast<wxPaperSize>(static_cast<int>(*paperId));
                }
            if (const auto orientation = numberAttr(L"orientation");
                orientation && *orientation >= 0)
                {
                m_svgExportOptions.m_paperOrientation =
                    (static_cast<int>(*orientation) == wxLANDSCAPE) ? wxLANDSCAPE : wxPORTRAIT;
                }
            m_svgExportOptions.m_includeTransitions =
                boolAttr(L"transitions", m_svgExportOptions.m_includeTransitions);
            m_svgExportOptions.m_includeHighlighting =
                boolAttr(L"highlighting", m_svgExportOptions.m_includeHighlighting);
            m_svgExportOptions.m_includeLayoutOptions =
                boolAttr(L"layout-options", m_svgExportOptions.m_includeLayoutOptions);
            m_svgExportOptions.m_includeDarkModeToggle =
                boolAttr(L"dark-mode-toggle", m_svgExportOptions.m_includeDarkModeToggle);
            m_svgExportOptions.m_includeSlideshow =
                boolAttr(L"slideshow", m_svgExportOptions.m_includeSlideshow);
            m_svgExportOptions.m_includePageShadow =
                boolAttr(L"page-shadow", m_svgExportOptions.m_includePageShadow);
            m_svgExportOptions.m_includeLayerControls =
                boolAttr(L"layer-controls", m_svgExportOptions.m_includeLayerControls);
            if (const auto colorNode = svgExportNode->GetProperty(L"theme-color");
                colorNode->IsOk())
                {
                if (const wxColour color{ colorNode->AsString() }; color.IsOk())
                    {
                    m_svgExportOptions.m_themeColor = color;
                    }
                }
            if (const auto layout = numberAttr(L"layout"); layout && *layout >= 0)
                {
                m_svgExportOptions.m_layout =
                    (static_cast<int>(*layout) == 0) ? SVGReportOptions::PageLayout::Single :
                    (static_cast<int>(*layout) == 1) ? SVGReportOptions::PageLayout::Duplex :
                                                       SVGReportOptions::PageLayout::Stacked;
                }
            }

        // PDF export options
        if (const auto pdfExportNode = json->GetProperty(L"pdf-export"); pdfExportNode->IsOk())
            {
            m_pdfExportOptionsLoaded = true;
            if (const auto paperIdNode = pdfExportNode->GetProperty(L"paper-id");
                paperIdNode->IsOk())
                {
                if (const double value{ paperIdNode->AsDouble(-1) }; value >= 0)
                    {
                    m_pdfExportOptions.m_paperSize =
                        static_cast<wxPaperSize>(static_cast<int>(value));
                    }
                }
            if (const auto orientationNode = pdfExportNode->GetProperty(L"orientation");
                orientationNode->IsOk())
                {
                if (const double value{ orientationNode->AsDouble(-1) }; value >= 0)
                    {
                    m_pdfExportOptions.m_paperOrientation =
                        (static_cast<int>(value) == wxLANDSCAPE) ? wxLANDSCAPE : wxPORTRAIT;
                    }
                }
            if (const auto compressNode = pdfExportNode->GetProperty(L"compress");
                compressNode->IsOk())
                {
                m_pdfExportOptions.m_compress = compressNode->AsBool(m_pdfExportOptions.m_compress);
                }
            }

        // PowerPoint export options
        if (const auto pptxExportNode = json->GetProperty(L"powerpoint-export");
            pptxExportNode->IsOk())
            {
            m_powerPointExportOptionsLoaded = true;
            const auto boolAttr = [&pptxExportNode](const wxString& name, const bool fallback)
            {
                const auto prop = pptxExportNode->GetProperty(name);
                return prop->IsOk() ? prop->AsBool(fallback) : fallback;
            };
            const auto numberAttr = [&pptxExportNode](const wxString& name) -> std::optional<double>
            {
                const auto prop = pptxExportNode->GetProperty(name);
                return prop->IsOk() ? std::optional<double>{ prop->AsDouble(-1) } : std::nullopt;
            };

            if (const auto slideSize = numberAttr(L"slide-size"); slideSize && *slideSize >= 0)
                {
                const auto value{ static_cast<int>(*slideSize) };
                m_powerPointExportOptions.m_slideSize =
                    (value == 0) ? PowerPointExportOptions::SlideSize::Widescreen16x9 :
                    (value == 1) ? PowerPointExportOptions::SlideSize::Standard4x3 :
                                   PowerPointExportOptions::SlideSize::Custom;
                }
            if (const auto customWidth = numberAttr(L"custom-width");
                customWidth && *customWidth > 0)
                {
                m_powerPointExportOptions.m_customWidthInches = *customWidth;
                }
            if (const auto customHeight = numberAttr(L"custom-height");
                customHeight && *customHeight > 0)
                {
                m_powerPointExportOptions.m_customHeightInches = *customHeight;
                }
            if (const auto transition = numberAttr(L"transition");
                transition && *transition >= 0 &&
                *transition <= static_cast<double>(
                                   static_cast<int>(PowerPointExportOptions::Transition::Morph)))
                {
                m_powerPointExportOptions.m_transition =
                    static_cast<PowerPointExportOptions::Transition>(static_cast<int>(*transition));
                }
            if (const auto transitionSpeed = numberAttr(L"transition-speed");
                transitionSpeed && *transitionSpeed >= 0 &&
                *transitionSpeed <= static_cast<double>(static_cast<int>(
                                        PowerPointExportOptions::TransitionSpeed::Fast)))
                {
                m_powerPointExportOptions.m_transitionSpeed =
                    static_cast<PowerPointExportOptions::TransitionSpeed>(
                        static_cast<int>(*transitionSpeed));
                }
            m_powerPointExportOptions.m_advanceOnClick =
                boolAttr(L"advance-on-click", m_powerPointExportOptions.m_advanceOnClick);
            m_powerPointExportOptions.m_advanceAutomatically = boolAttr(
                L"advance-automatically", m_powerPointExportOptions.m_advanceAutomatically);
            if (const auto advanceSeconds = numberAttr(L"advance-seconds");
                advanceSeconds && *advanceSeconds > 0)
                {
                m_powerPointExportOptions.m_advanceSeconds = static_cast<int>(*advanceSeconds);
                }
            m_powerPointExportOptions.m_loopContinuously =
                boolAttr(L"loop", m_powerPointExportOptions.m_loopContinuously);
            m_powerPointExportOptions.m_includeAccessibilityNotes = boolAttr(
                L"accessibility-notes", m_powerPointExportOptions.m_includeAccessibilityNotes);
            m_powerPointExportOptions.m_includeTitleSlide =
                boolAttr(L"title-slide", m_powerPointExportOptions.m_includeTitleSlide);
            if (const auto themeNode = pptxExportNode->GetProperty(L"title-slide-theme");
                themeNode->IsOk())
                {
                m_powerPointExportOptions.m_titleSlideTheme = themeNode->AsString();
                }
            if (const auto authorNode = pptxExportNode->GetProperty(L"author"); authorNode->IsOk())
                {
                m_powerPointExportOptions.m_author = authorNode->AsString();
                }
            if (const auto publisherNode = pptxExportNode->GetProperty(L"publisher");
                publisherNode->IsOk())
                {
                m_powerPointExportOptions.m_publisher = publisherNode->AsString();
                }
            }

        // start loading the pages
        const auto pagesProperty = json->GetProperty(L"pages");
        if (pagesProperty->IsOk())
            {
            std::vector<std::shared_ptr<Graphs::Graph2D>> embeddedGraphs;
            const auto pages = pagesProperty->AsNodes();
            for (const auto& page : pages)
                {
                // common axes are per page, where they must reference child graphs on the same page
                m_commonAxesPlaceholders.clear();
                embeddedGraphs.clear();
                if (page->IsOk())
                    {
                    // create the canvas used for the page
                    auto* canvas = new Canvas(parent);
                    const auto rawName = page->GetProperty(_DT(L"name"))->AsString();
                    canvas->SetNameTemplate(rawName);
                    canvas->SetLabel(ExpandConstants(rawName));

                    // layer (for SVG layer filtering)
                    if (page->HasProperty(L"layer"))
                        {
                        const auto rawLayer = page->GetProperty(L"layer")->AsString();
                        canvas->SetLayer(ExpandConstants(rawLayer));
                        }

                    // page numbering
                    if (page->HasProperty(L"page-numbering"))
                        {
                        m_pageNumber = 1;
                        canvas->ResetsPageNumbering(true);
                        }

                    // watermark (overrides report-level watermark)
                    const auto watermarkPageProperty = page->GetProperty(L"watermark");
                    Canvas::Watermark watermark{};
                    if (watermarkPageProperty->IsOk())
                        {
                        watermark.m_label =
                            watermarkPageProperty->GetProperty(L"label")->AsString();
                        watermark.m_color =
                            ConvertColor(watermarkPageProperty->GetProperty(L"color"));
                        }
                    else
                        {
                        if (!reportWatermark.empty())
                            {
                            watermark.m_label = reportWatermark;
                            }
                        if (reportWatermarkColor.IsOk())
                            {
                            watermark.m_color = reportWatermarkColor;
                            }
                        }
                    canvas->SetWatermark(watermark);

                    // background color
                    const auto bgColor = ConvertColor(page->GetProperty(L"background-color"));
                    if (bgColor.IsOk())
                        {
                        canvas->SetBackgroundColor(bgColor);
                        }

                    // background image
                    if (page->HasProperty(L"background-image"))
                        {
                        const auto imgPathNode = page->GetProperty(L"background-image");
                        const auto bmp = LoadImageFile(imgPathNode);
                        if (bmp.IsOk())
                            {
                            const wxString rawPath =
                                imgPathNode->IsValueString() ?
                                    imgPathNode->AsString() :
                                    imgPathNode->GetProperty(L"path")->AsString();
                            canvas->SetBackgroundImagePath(NormalizeFilePath(rawPath));
                            const auto opacityNode = imgPathNode->GetProperty(L"opacity");
                            const auto opacity = static_cast<uint8_t>(
                                opacityNode->IsOk() ?
                                    std::clamp(static_cast<uint8_t>(opacityNode->AsDouble()),
                                               wxALPHA_TRANSPARENT, wxALPHA_OPAQUE) :
                                    wxALPHA_OPAQUE);
                            canvas->SetBackgroundImage(wxBitmapBundle{ bmp }, opacity);
                            }
                        }

                    size_t rowCount{ 0 };
                    const auto rowsProperty = page->GetProperty(L"rows");
                    if (rowsProperty->IsOk())
                        {
                        size_t currentRow{ 0 };
                        const auto rows = rowsProperty->AsNodes();
                        rowCount = rows.size();
                        if (rows.empty())
                            {
                            canvas->SetFixedObjectsGridSize(1, 1);
                            }
                        else
                            {
                            canvas->SetFixedObjectsGridSize(rows.size(), 1);
                            }
                        for (const auto& row : rows)
                            {
                            const auto itemsProperty = row->GetProperty(L"items");
                            if (itemsProperty->IsOk())
                                {
                                size_t currentColumn{ 0 };
                                auto items = itemsProperty->AsNodes();
                                if (const auto gridSize = canvas->GetFixedObjectsGridSize();
                                    gridSize.second < items.size())
                                    {
                                    canvas->SetFixedObjectsGridSize(gridSize.first, items.size());
                                    }
                                for (const auto& item : items)
                                    {
                                    const auto typeProperty = item->GetProperty(L"type");
                                    // load the item into the grid cell(s)
                                    if (typeProperty->IsOk())
                                        {
                                        try
                                            {
                                            /* Along with adding graphs to the canvas, we also keep
                                               a list of these graphs in case we need to connect any
                                               of them to a common axis.

                                               Graph loading functions will load the graph to the
                                               canvas themselves because they may need to add an
                                               accompanying legend, which that function will add to
                                               the canvas also.

                                               Other objects like labels and images will be added to
                                               the canvas here though, as we know it will just be
                                               that one object.*/
                                            if (typeProperty->AsString().CmpNoCase(L"line-plot") ==
                                                0)
                                                {
                                                embeddedGraphs.push_back(LoadLinePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"multi-series-line-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadMultiSeriesLinePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"heatmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadHeatMap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"choropleth-map") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadChoroplethMap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"win-loss-sparkline") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWinLossSparkline(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"waffle-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWaffleChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"race-track-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadRaceTrackChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"nightingale-rose-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadNightingaleRoseChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"bullet-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadBulletChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"waterfall-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWaterfallChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"funnel-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadFunnelChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"dubois-spiral-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadDuBoisSpiralChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"wilmarth-bridge-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWilmarthBridgePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"stem-and-leaf-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadStemAndLeafPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"gantt-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadGanttChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"candlestick-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadCandlestickPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"w-curve-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWCurvePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"likert-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadLikertChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"linear-regression-roadmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadLRRoadmap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"pro-con-roadmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadProConRoadmap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"word-cloud") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWordCloud(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"sankey-diagram") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadSankeyDiagram(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"box-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadBoxPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"pie-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadPieChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"histogram") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadHistogram(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"scale-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadScaleChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"categorical-bar-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadCategoricalBarChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"scatter-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadScatterPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"bubble-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadBubblePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"chernoff-faces") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadChernoffFaces(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(L"label") ==
                                                     0)
                                                {
                                                canvas->SetFixedObject(
                                                    currentRow, currentColumn,
                                                    LoadLabel(item, GraphItems::Label{}));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"spacer") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadSpacer());
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"empty-spacer") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadEmptySpacer());
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(L"image") ==
                                                     0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadImage(item));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(L"table") ==
                                                     0)
                                                {
                                                LoadTable(item, canvas, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"common-axis") == 0)
                                                {
                                                // Common axis cannot be created until we know all
                                                // its children have been created. Add a placeholder
                                                // for now and circle back after all other items
                                                // have been added to the grid.
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       nullptr);
                                                LoadCommonAxis(item, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(L"shape") ==
                                                     0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadShape(item));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"fillable-shape") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadFillableShape(item));
                                                }
                                            // explicitly null item is a placeholder,
                                            // or possibly a blank row that will be consumed by the
                                            // previous row to make it twice as tall as others
                                            else if (typeProperty->IsNull())
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       nullptr);
                                                }
                                            }
                                        // show error, but OK to keep going
                                        catch (const std::exception& err)
                                            {
                                            wxLogError(L"%s", wxString::FromUTF8(err.what()));
                                            }
                                        }
                                    ++currentColumn;
                                    }
                                ++currentRow;
                                }
                            }
                        }
                    // if there are common axis queued, add them now
                    if (!m_commonAxesPlaceholders.empty())
                        {
                        std::vector<std::shared_ptr<Graphs::Graph2D>> childGraphs;
                        for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                            {
                            for (const auto& childId : commonAxisInfo.m_childrenIds)
                                {
                                auto childGraph = std::ranges::find_if(
                                    std::as_const(embeddedGraphs),
                                    [&childId](const auto& graph) noexcept
                                    { return graph->GetId() == static_cast<long>(childId); });
                                if (childGraph != embeddedGraphs.end() && (*childGraph) != nullptr)
                                    {
                                    childGraphs.push_back(*childGraph);
                                    }
                                }
                            if (childGraphs.size() > 1)
                                {
                                auto commonAxis =
                                    (commonAxisInfo.m_axisType == AxisType::BottomXAxis ||
                                     commonAxisInfo.m_axisType == AxisType::TopXAxis) ?
                                        CommonAxisBuilder::BuildXAxis(
                                            canvas, childGraphs, commonAxisInfo.m_axisType,
                                            commonAxisInfo.m_commonPerpendicularAxis) :
                                        CommonAxisBuilder::BuildYAxis(canvas, childGraphs,
                                                                      commonAxisInfo.m_axisType);
                                // build a label-to-position map from the common
                                // axis's custom labels; this maps category labels
                                // to their sorted positions so that brackets use
                                // the correct positions instead of raw category IDs
                                std::map<wxString, double> labelPosMap;
                                    {
                                    const auto& axisLabels = commonAxis->GetCustomLabels();
                                    for (const auto& [pos, label] : axisLabels)
                                        {
                                        labelPosMap[label.GetText()] = pos;
                                        }
                                    }
                                LoadAxis(commonAxisInfo.m_node, *commonAxis, labelPosMap);
                                LoadItem(commonAxisInfo.m_node, *commonAxis);
                                    // cache common-axis-specific properties for round-tripping
                                    {
                                    wxString childIdsStr;
                                    for (size_t i = 0; i < commonAxisInfo.m_childrenIds.size(); ++i)
                                        {
                                        if (i > 0)
                                            {
                                            childIdsStr += L",";
                                            }
                                        childIdsStr += std::to_wstring(
                                            static_cast<int>(commonAxisInfo.m_childrenIds[i]));
                                        }
                                    commonAxis->SetPropertyTemplate(L"child-ids", childIdsStr);
                                    if (commonAxisInfo.m_commonPerpendicularAxis)
                                        {
                                        commonAxis->SetPropertyTemplate(
                                            L"common-perpendicular-axis", L"true");
                                        }
                                    }
                                // force the row to its height and no more
                                commonAxis->FitCanvasRowHeightToContent(true);
                                canvas->SetFixedObject(commonAxisInfo.m_gridPosition.first,
                                                       commonAxisInfo.m_gridPosition.second,
                                                       std::move(commonAxis));
                                }
                            }
                        }
                    canvas->CalcRowDimensions();
                    canvas->FitToPageWhenPrinting(true);
                    canvas->SetSizeFromPaperSize();
                    // if multiple rows, then treat it as a report that maintains
                    // the aspect ratio of its content
                    canvas->MaintainAspectRatio(rowCount > 1);
                    reportPages.push_back(canvas);

                    ++m_pageNumber;
                    }
                }
            }

        for (auto& tableLink : m_tableLinks)
            {
            tableLink.SyncTableSizes();
            }

        return reportPages;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush,
                                  GraphItems::GraphItemBase* item /*= nullptr*/,
                                  const wxString& propertyPrefix /*= wxString{}*/) const
        {
        if (brushNode->IsOk())
            {
            if (brushNode->IsValueNull())
                {
                brush = wxNullBrush;
                }
            // just a color string
            else if (brushNode->IsValueString())
                {
                const wxString brushPropertyName = propertyPrefix.empty() ?
                                                       wxString{ L"brush.color" } :
                                                       propertyPrefix + L".color";
                const wxColour brushColor(
                    ConvertColor(brushNode->AsString(), item, brushPropertyName));
                if (brushColor.IsOk())
                    {
                    brush.SetColour(brushColor);
                    }
                }
            // or a full definition
            else
                {
                const wxString brushPropertyName = propertyPrefix.empty() ?
                                                       wxString{ L"brush.color" } :
                                                       propertyPrefix + L".color";
                const auto colorPropNode = brushNode->GetProperty(L"color");
                const wxColour brushColor(
                    (colorPropNode->IsOk() && !colorPropNode->IsValueNull()) ?
                        ConvertColor(colorPropNode->AsString(), item, brushPropertyName) :
                        wxTransparentColour);
                if (brushColor.IsOk())
                    {
                    brush.SetColour(brushColor);
                    }

                const auto brushStr{ brushNode->GetProperty(L"style")->AsString() };
                const auto foundStyle = ReportEnumConvert::ConvertBrushStyle(brushStr);
                if (foundStyle)
                    {
                    brush.SetStyle(foundStyle.value());
                    }
                else if (!brushStr.empty())
                    {
                    wxLogWarning(L"Unknown brush style '%s'. Using default solid.", brushStr);
                    brush.SetStyle(wxBRUSHSTYLE_SOLID);
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen,
                                GraphItems::GraphItemBase* item /*= nullptr*/,
                                const wxString& propertyPrefix /*= wxString{}*/) const
        {
        if (penNode->IsOk())
            {
            if (penNode->IsValueNull())
                {
                pen = wxNullPen;
                }
            // shorthand string form: just a color (e.g., "#808080")
            else if (penNode->IsValueString() && !penNode->HasProperty(L"color"))
                {
                const wxString penPropertyName =
                    propertyPrefix.empty() ? wxString{ L"pen.color" } : propertyPrefix + L".color";
                const wxColour penColor(ConvertColor(penNode->AsString(), item, penPropertyName));
                if (penColor.IsOk())
                    {
                    pen.SetColour(penColor);
                    }
                }
            else
                {
                const wxString penPropertyName =
                    propertyPrefix.empty() ? wxString{ L"pen.color" } : propertyPrefix + L".color";
                const auto colorPropNode = penNode->GetProperty(L"color");
                const wxColour penColor(
                    (colorPropNode->IsOk() && !colorPropNode->IsValueNull()) ?
                        ConvertColor(colorPropNode->AsString(), item, penPropertyName) :
                        wxTransparentColour);
                if (penColor.IsOk())
                    {
                    pen.SetColour(penColor);
                    }

                if (penNode->HasProperty(L"width"))
                    {
                    pen.SetWidth(penNode->GetProperty(L"width")->AsDouble(1));
                    }

                const auto styleStr{ penNode->GetProperty(L"style")->AsString() };
                const auto foundStyle = ReportEnumConvert::ConvertPenStyle(styleStr);
                if (foundStyle.has_value())
                    {
                    pen.SetStyle(foundStyle.value());
                    }
                else if (!styleStr.empty())
                    {
                    wxLogWarning(L"Unknown pen style '%s'. Using default solid.", styleStr);
                    pen.SetStyle(wxPENSTYLE_SOLID);
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis,
                                 const std::map<wxString, double>& labelPositions)
        {
        const auto titleProperty = axisNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty, GraphItems::Label{});
            if (titleLabel != nullptr)
                {
                axis.GetTitle() = *titleLabel;
                // mark as a user override so the serializer knows to round-trip it,
                // even if the title is explicitly blank (overriding the default
                // variable name)
                axis.SetPropertyTemplate(L"title.user-defined", L"true");
                }
            }
        const auto tickmarksProperty = axisNode->GetProperty(L"tickmarks");
        if (tickmarksProperty->IsOk())
            {
            const auto foundTickmark = ReportEnumConvert::ConvertTickMarkDisplay(
                tickmarksProperty->GetProperty(L"display")->AsString());
            if (foundTickmark.has_value())
                {
                axis.SetTickMarkDisplay(foundTickmark.value());
                }
            }
        const auto foundLabelDisplay = ReportEnumConvert::ConvertAxisLabelDisplay(
            axisNode->GetProperty(L"label-display")->AsString());
        if (foundLabelDisplay.has_value())
            {
            axis.SetLabelDisplay(foundLabelDisplay.value());
            }

        const auto numDisplay = ReportEnumConvert::ConvertNumberDisplay(
            axisNode->GetProperty(_DT(L"number-display"))->AsString());
        if (numDisplay)
            {
            axis.SetNumberDisplay(numDisplay.value());
            }

        axis.SetDoubleSidedAxisLabels(axisNode->GetProperty(L"double-sided-labels")->AsBool());

        // pens
        LoadPen(axisNode->GetProperty(L"axis-pen"), axis.GetAxisLinePen());
        LoadPen(axisNode->GetProperty(L"gridline-pen"), axis.GetGridlinePen());

        // max line length
        if (axisNode->HasProperty(L"label-length"))
            {
            axis.SetLabelLineLength(
                axisNode->GetProperty(L"label-length")->AsDouble(axis.GetLabelLineLength()));
            }

        if (axisNode->GetProperty(L"label-length-auto")->AsBool())
            {
            axis.SetLabelLengthAuto();
            }

        const auto rangeNode = axisNode->GetProperty(_DT(L"range"));
        if (rangeNode->IsOk())
            {
            // Use stored range and interval info only if they span beyond the calculated range.
            // If the newly calculated range covers more of a range than the hard coded one in
            // the config file, then we need to use that instead.
            const auto [calculatedRangeStart, calculatedRangeEnd] = axis.GetRange();
            auto rangeStart =
                std::min(rangeNode->GetProperty(_DT(L"start"))->AsDouble(calculatedRangeStart),
                         calculatedRangeStart);
            auto rangeEnd =
                std::max(rangeNode->GetProperty(_DT(L"end"))->AsDouble(calculatedRangeEnd),
                         calculatedRangeEnd);
            const bool axisAdjusted = compare_doubles_greater(rangeStart, calculatedRangeStart) ||
                                      compare_doubles_less(rangeEnd, calculatedRangeEnd);

            const auto interval =
                axisAdjusted ?
                    axis.GetInterval() :
                    rangeNode->GetProperty(_DT(L"interval"))->AsDouble(axis.GetInterval());
            const auto displayInterval = axisAdjusted ?
                                             axis.GetDisplayInterval() :
                                             rangeNode->GetProperty(_DT(L"display-interval"))
                                                 ->AsDouble(axis.GetDisplayInterval());
            // using the hard coded precision is fine either way
            const auto precision =
                rangeNode->GetProperty(_DT(L"precision"))->AsDouble(axis.GetPrecision());

            if (rangeStart > rangeEnd)
                {
                wxLogWarning(_("Invalid axis range (%f to %f) provided."), rangeStart, rangeEnd);
                }
            else
                {
                axis.SetRange(rangeStart, rangeEnd, precision, interval, displayInterval);
                axis.SetPropertyTemplate(L"range.user-defined", L"true");
                }
            }

        axis.SetPrecision(axisNode->GetProperty(_DT(L"precision"))->AsDouble(0));

        // custom labels
        const auto customLabelsNode = axisNode->GetProperty(L"custom-labels");
        if (customLabelsNode->IsOk() && customLabelsNode->IsValueArray())
            {
            const auto customLabels = customLabelsNode->AsNodes();
            for (const auto& customLabel : customLabels)
                {
                if (customLabel->HasProperty(L"value"))
                    {
                    const auto label =
                        LoadLabel(customLabel->GetProperty(L"label"), GraphItems::Label{});
                    // clean custom label
                    wxString labelText{ label->GetText() };
                    labelText.Replace(L"\r", L" ");
                    labelText.Replace(L"\n", L" ");
                    labelText.Trim(true).Trim(false);
                    label->SetText(labelText);
                    axis.SetCustomLabel(customLabel->GetProperty(L"value")->AsDouble(), *label);
                    }
                }
            // mark as a user override so the serializer knows to round-trip
            // them (for common axes, dataset-derived labels are otherwise
            // suppressed to avoid going stale)
            if (!customLabels.empty())
                {
                axis.SetCustomLabelsAreUserOverride(true);
                }
            }

        // brackets
        const auto bracketsNode = axisNode->GetProperty(L"brackets");
        if (bracketsNode->IsOk())
            {
            wxPen bracketPen{ wxPenInfo(Colors::ColorBrewer::GetColor(Colors::Color::Black), 2) };
            // individually defined brackets
            if (bracketsNode->IsValueArray())
                {
                const auto brackets = bracketsNode->AsNodes();
                for (const auto& bracket : brackets)
                    {
                    LoadPen(bracket->GetProperty(L"pen"), bracketPen);

                    const auto foundBracketStyle = ReportEnumConvert::ConvertBracketLineStyle(
                        bracket->GetProperty(L"style")->AsString());

                    const std::optional<double> axisPos1 =
                        FindAxisPosition(axis, bracket->GetProperty(L"start"));
                    const std::optional<double> axisPos2 =
                        FindAxisPosition(axis, bracket->GetProperty(L"end"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        axis.AddBracket(GraphItems::Axis::AxisBracket(
                            axisPos1.value(), axisPos2.value(),
                            safe_divide<double>(axisPos1.value() + axisPos2.value(), 2),
                            bracket->GetProperty(L"label")->AsString(), bracketPen,
                            foundBracketStyle.value_or(BracketLineStyle::CurlyBraces)));
                        }
                    }
                axis.SetBracketsAreDynamic(false);
                }
            // or build a series of brackets from a dataset
            else
                {
                LoadPen(bracketsNode->GetProperty(L"pen"), bracketPen);
                const auto foundBracketStyle = ReportEnumConvert::ConvertBracketLineStyle(
                    bracketsNode->GetProperty(L"style")->AsString());
                // if loading brackets based on the dataset
                if (bracketsNode->HasProperty(L"dataset"))
                    {
                    const wxString dsName = bracketsNode->GetProperty(L"dataset")->AsString();
                    const auto foundDataset = m_datasets.find(dsName);
                    if (foundDataset == m_datasets.cend() || foundDataset->second == nullptr)
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: dataset not found for axis brackets."), dsName)
                                .ToUTF8());
                        }
                    // cache dataset name for round-tripping
                    axis.SetPropertyTemplate(L"brackets.dataset", dsName);

                    const auto variablesNode = bracketsNode->GetProperty(L"variables");
                    if (variablesNode->IsOk())
                        {
                        const auto labelVarName =
                            ExpandAndCache(&axis, L"bracket.label",
                                           variablesNode->GetProperty(L"label")->AsString());
                        const auto valueVarName =
                            ExpandAndCache(&axis, L"bracket.value",
                                           variablesNode->GetProperty(L"value")->AsString());

                        // use label-position map if provided (for common axes
                        // where child graphs have sorted their bars)
                        if (!labelPositions.empty())
                            {
                            axis.AddBrackets(foundDataset->second, labelVarName, valueVarName,
                                             labelPositions);
                            }
                        else
                            {
                            axis.AddBrackets(foundDataset->second, labelVarName, valueVarName);
                            }
                        if (bracketPen.IsOk())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                {
                                bracket.GetLinePen() = bracketPen;
                                }
                            }
                        if (foundBracketStyle.has_value())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                {
                                bracket.SetBracketLineStyle(foundBracketStyle.value());
                                }
                            }
                        axis.SetBracketsAreDynamic(false);
                        }
                    else
                        {
                        throw std::runtime_error(
                            _(L"Variables not defined for brackets.").ToUTF8());
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        _(L"No dataset provided for axis brackets. "
                          "Did you intend to define the brackets as an array of "
                          "start and end points instead?")
                            .ToUTF8());
                    }
                }

            if (bracketsNode->GetProperty(L"simplify")->AsBool())
                {
                axis.SimplifyBrackets();
                }
            }

        // show options
        axis.Show(axisNode->GetProperty(L"show")->AsBool(true));
        axis.ShowOuterLabels(axisNode->GetProperty(L"show-outer-labels")->AsBool(true));
        }

    //---------------------------------------------------
    void ReportBuilder::LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                                       const size_t currentRow, const size_t currentColumn)
        {
        const auto axisType = ReportEnumConvert::ConvertAxisType(
            commonAxisNode->GetProperty(L"axis-type")->AsString());
        if (axisType.has_value())
            {
            m_commonAxesPlaceholders.push_back(
                { axisType.value(), std::make_pair(currentRow, currentColumn),
                  commonAxisNode->GetProperty(L"child-ids")->AsDoubles(),
                  commonAxisNode->GetProperty(L"common-perpendicular-axis")->AsBool(),
                  commonAxisNode });
            }
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadSpacer() const
        {
        return std::make_shared<GraphItems::Label>(
            GraphItems::GraphItemInfo{}.DPIScaling(m_dpiScaleFactor).Scaling(1.0).Show(false));
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadEmptySpacer() const
        {
        return std::make_shared<GraphItems::Label>(GraphItems::GraphItemInfo{}
                                                       .DPIScaling(m_dpiScaleFactor)
                                                       .Scaling(0.0)
                                                       .FixedWidthOnCanvas(true)
                                                       .CanvasHeightProportion(0)
                                                       .Show(false));
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label>
    ReportBuilder::LoadLabel(const wxSimpleJSON::Ptr_t& labelNode,
                             const GraphItems::Label& labelTemplate) const
        {
        // just a string
        if (labelNode->IsValueString())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetDPIScaleFactor(m_dpiScaleFactor);
            label->SetText(ExpandAndCache(label.get(), L"text", labelNode->AsString()));
            label->GetPen() = wxNullPen;

            return label;
            }
        // a fully defined label
        if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetDPIScaleFactor(m_dpiScaleFactor);
            label->SetText(
                ExpandAndCache(label.get(), L"text", labelNode->GetProperty(L"text")->AsString()));
            label->GetPen() = wxNullPen;

            if (const wxColour bgColor(ConvertColor(labelNode->GetProperty(L"background")));
                bgColor.IsOk())
                {
                label->SetFontBackgroundColor(bgColor);
                }
            if (const wxColour color(ConvertColor(labelNode->GetProperty(L"color"))); color.IsOk())
                {
                label->SetFontColor(color);
                }

            // an image to the left side of it
            if (const auto imgNode = labelNode->GetProperty(L"left-image"); imgNode->IsOk())
                {
                const auto importNode = imgNode->GetProperty(L"image-import");
                if (const auto img = LoadImageFile(importNode); img.IsOk())
                    {
                    label->SetLeftImage(img);
                    // cache path for round-tripping
                    if (importNode->IsValueString())
                        {
                        label->SetPropertyTemplate(L"left-image.path", importNode->AsString());
                        }
                    else if (importNode->IsOk())
                        {
                        const auto importPath = importNode->GetProperty(L"path")->AsString();
                        if (!importPath.empty())
                            {
                            label->SetPropertyTemplate(L"left-image.path", importPath);
                            }
                        }
                    }
                }
            // top image
            if (const auto imgNode = labelNode->GetProperty(L"top-image"); imgNode->IsOk())
                {
                const auto importNode = imgNode->GetProperty(L"image-import");
                label->SetTopImage(LoadImageFile(importNode),
                                   imgNode->GetProperty(L"offset")->AsDouble(0));
                // cache path for round-tripping
                if (importNode->IsValueString())
                    {
                    label->SetPropertyTemplate(L"top-image.path", importNode->AsString());
                    }
                else if (importNode->IsOk())
                    {
                    const auto importPath = importNode->GetProperty(L"path")->AsString();
                    if (!importPath.empty())
                        {
                        label->SetPropertyTemplate(L"top-image.path", importPath);
                        }
                    }
                }
            // top shape
            if (const auto topShapeNode = labelNode->GetProperty(L"top-shape");
                topShapeNode->IsOk())
                {
                if (topShapeNode->IsValueArray())
                    {
                    std::vector<GraphItems::ShapeInfo> shapes;
                    auto shapeNodes = topShapeNode->AsNodes();
                    shapes.reserve(shapeNodes.size());
                    for (const auto& shpNode : shapeNodes)
                        {
                        shapes.push_back(LoadShapeInfo(shpNode));
                        }
                    label->SetTopShape(shapes,
                                       labelNode->GetProperty(L"top-shape-offset")->AsDouble(0));
                    }
                else
                    {
                    label->SetTopShape(
                        std::vector<GraphItems::ShapeInfo>{ LoadShapeInfo(topShapeNode) });
                    }
                }

            const auto orientation = labelNode->GetProperty(L"orientation")->AsString();
            if (orientation.CmpNoCase(L"horizontal") == 0)
                {
                label->SetTextOrientation(Orientation::Horizontal);
                }
            else if (orientation.CmpNoCase(L"vertical") == 0)
                {
                label->SetTextOrientation(Orientation::Vertical);
                }

            const auto labelStyle =
                ReportEnumConvert::ConvertLabelStyle(labelNode->GetProperty(L"style")->AsString());
            if (labelStyle.has_value())
                {
                label->SetLabelStyle(labelStyle.value());
                }

            const auto labelShape =
                ReportEnumConvert::ConvertLabelShape(labelNode->GetProperty(L"shape")->AsString());
            if (labelShape.has_value())
                {
                label->SetShape(labelShape.value());
                }

            const auto boxAdjustment = ReportEnumConvert::ConvertLabelBoundingBoxContentAdjustment(
                labelNode->GetProperty(L"box-content-adjustment")->AsString());
            if (boxAdjustment.has_value())
                {
                label->SetBoundingBoxToContentAdjustment(boxAdjustment.value());
                }

            label->SetLineSpacing(labelNode->GetProperty(L"line-spacing")->AsDouble(1));

            // font attributes
            if (labelNode->HasProperty(L"bold"))
                {
                if (labelNode->GetProperty(L"bold")->AsBool())
                    {
                    label->GetFont().MakeBold();
                    }
                else
                    {
                    label->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                    }
                }
            if (labelNode->HasProperty(L"italic"))
                {
                if (labelNode->GetProperty(L"italic")->AsBool())
                    {
                    label->GetFont().MakeItalic();
                    }
                else
                    {
                    label->GetFont().SetStyle(wxFONTSTYLE_NORMAL);
                    }
                }
            if (labelNode->HasProperty(L"font-name"))
                {
                const auto faceName = labelNode->GetProperty(L"font-name")->AsString();
                if (!faceName.empty())
                    {
                    label->GetFont().SetFaceName(faceName);
                    }
                }
            if (labelNode->HasProperty(L"font-size"))
                {
                const auto fontSize = labelNode->GetProperty(L"font-size")->AsDouble();
                if (fontSize > 0)
                    {
                    label->GetFont().SetFractionalPointSize(fontSize);
                    }
                }

            const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                labelNode->GetProperty(L"text-alignment")->AsString());
            if (textAlignment.has_value())
                {
                label->SetTextAlignment(textAlignment.value());
                }

            if (labelNode->HasProperty(L"lock-scaling") &&
                labelNode->GetProperty(L"lock-scaling")->AsBool())
                {
                label->LockBoundingBoxScaling();
                }

            // header info
            auto headerNode = labelNode->GetProperty(L"header");
            if (headerNode->IsOk())
                {
                label->GetHeaderInfo().Enable(true);
                if (headerNode->HasProperty(L"bold"))
                    {
                    if (headerNode->GetProperty(L"bold")->AsBool())
                        {
                        label->GetHeaderInfo().GetFont().MakeBold();
                        }
                    else
                        {
                        label->GetHeaderInfo().GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                        }
                    }
                if (headerNode->HasProperty(L"italic"))
                    {
                    if (headerNode->GetProperty(L"italic")->AsBool())
                        {
                        label->GetHeaderInfo().GetFont().MakeItalic();
                        }
                    else
                        {
                        label->GetHeaderInfo().GetFont().SetStyle(wxFONTSTYLE_NORMAL);
                        }
                    }
                if (headerNode->HasProperty(L"font-name"))
                    {
                    const auto faceName = headerNode->GetProperty(L"font-name")->AsString();
                    if (!faceName.empty())
                        {
                        label->GetHeaderInfo().GetFont().SetFaceName(faceName);
                        }
                    }
                if (headerNode->HasProperty(L"font-size"))
                    {
                    const auto fontSize = headerNode->GetProperty(L"font-size")->AsDouble();
                    if (fontSize > 0)
                        {
                        label->GetHeaderInfo().GetFont().SetFractionalPointSize(fontSize);
                        }
                    }
                const wxColour headerColor(ConvertColor(headerNode->GetProperty(L"color")));
                if (headerColor.IsOk())
                    {
                    label->GetHeaderInfo().FontColor(headerColor);
                    }

                label->GetHeaderInfo().RelativeScaling(
                    headerNode->GetProperty(L"relative-scaling")->AsDouble(1));

                const auto headerTextAlignment = ReportEnumConvert::ConvertTextAlignment(
                    headerNode->GetProperty(L"text-alignment")->AsString());
                if (headerTextAlignment.has_value())
                    {
                    label->GetHeaderInfo().LabelAlignment(headerTextAlignment.value());
                    }
                }

            LoadItem(labelNode, *label);
            return label;
            }
        return nullptr;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadMerges(const wxSimpleJSON::Ptr_t& mergesNode,
                                   const std::shared_ptr<const Data::Dataset>& datasetToMerge)
        {
        if (mergesNode->IsOk())
            {
            // find the parent dataset name
            wxString parentDsName;
            for (const auto& [name, ds] : m_datasets)
                {
                if (ds == datasetToMerge)
                    {
                    parentDsName = name;
                    break;
                    }
                }

            auto merges = mergesNode->AsNodes();
            for (const auto& merge : merges)
                {
                if (merge->IsOk())
                    {
                    const wxString otherDsName = merge->GetProperty(L"other-dataset")->AsString();
                    const auto foundPos = m_datasets.find(otherDsName);
                    if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: dataset not found for dataset merging."),
                                             otherDsName)
                                .ToUTF8());
                        }

                    const auto mergeType =
                        merge->GetProperty(L"type")->AsString(L"left-join-unique-last");
                    if (mergeType.CmpNoCase(L"left-join-unique-last") == 0 ||
                        mergeType.CmpNoCase(L"left-join-unique-first") == 0 ||
                        mergeType.CmpNoCase(L"left-join") == 0 ||
                        mergeType.CmpNoCase(L"inner-join") == 0)
                        {
                        std::vector<std::pair<wxString, wxString>> bys;
                        const auto byCols = merge->GetProperty(L"by")->AsNodes();
                        bys.reserve(byCols.size());
                        for (const auto& byCol : byCols)
                            {
                            bys.emplace_back(byCol->GetProperty(L"left-column")->AsString(),
                                             byCol->GetProperty(L"right-column")->AsString());
                            }
                        const auto suffix = merge->GetProperty(L"suffix")->AsString(L".x");
                        auto mergedData = (mergeType.CmpNoCase(L"inner-join") == 0) ?
                                              Data::DatasetInnerJoin::InnerJoin(
                                                  datasetToMerge, foundPos->second, bys, suffix) :
                                          (mergeType.CmpNoCase(L"left-join-unique-first") == 0) ?
                                              Data::DatasetLeftJoin::LeftJoinUniqueFirst(
                                                  datasetToMerge, foundPos->second, bys, suffix) :
                                          (mergeType.CmpNoCase(L"left-join") == 0) ?
                                              Data::DatasetLeftJoin::LeftJoin(
                                                  datasetToMerge, foundPos->second, bys, suffix) :
                                              Data::DatasetLeftJoin::LeftJoinUniqueLast(
                                                  datasetToMerge, foundPos->second, bys, suffix);

                        if (mergedData)
                            {
                            const auto mergeName = merge->GetProperty(_DT(L"name"))->AsString();
                            if (m_datasets.contains(mergeName))
                                {
                                wxLogWarning(L"Dataset '%s' already exists "
                                             "and cannot be overwritten by a merge.",
                                             mergeName);
                                continue;
                                }
                            m_datasets.insert_or_assign(mergeName, mergedData);
                            m_datasetInsertionOrder.push_back(mergeName);

                            DatasetMergeOptions mergeOpts;
                            mergeOpts.m_sourceDatasetName = parentDsName;
                            mergeOpts.m_otherDatasetName = otherDsName;
                            mergeOpts.m_type = mergeType;
                            mergeOpts.m_byColumns = bys;
                            mergeOpts.m_suffix = suffix;
                            SetDatasetMergeOptions(mergeName, mergeOpts);

                            LoadDatasetTransformations(merge, mergedData);
                            }
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: unrecognized dataset merging method."),
                                             mergeType)
                                .ToUTF8());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadPivots(const wxSimpleJSON::Ptr_t& pivotsNode,
                                   const std::shared_ptr<const Data::Dataset>& parentToPivot,
                                   const wxString& parentName)
        {
        if (pivotsNode->IsOk())
            {
            auto pivots = pivotsNode->AsNodes();
            for (const auto& pivot : pivots)
                {
                if (pivot->IsOk())
                    {
                    const auto pivotType = pivot->GetProperty(L"type")->AsString(L"wider");
                    if (pivotType.CmpNoCase(L"wider") == 0)
                        {
                        const auto idColumns = pivot->GetProperty(L"id-columns")->AsStrings();
                        const auto namesFromColumn =
                            pivot->GetProperty(L"names-from-column")->AsString();
                        const auto valuesFromColumns =
                            pivot->GetProperty(L"values-from-columns")->AsStrings();
                        const auto namesSep =
                            pivot->GetProperty(L"names-separator")->AsString(L"_");
                        const auto namesPrefix = pivot->GetProperty(L"names-prefix")->AsString();
                        const auto fillValue =
                            pivot->GetProperty(L"fill-value")
                                ->AsDouble(std::numeric_limits<double>::quiet_NaN());

                        auto pivotedData = Data::Pivot::PivotWider(
                            parentToPivot, idColumns, namesFromColumn, valuesFromColumns, namesSep,
                            namesPrefix, fillValue);

                        if (pivotedData)
                            {
                            const auto pivotName = pivot->GetProperty(_DT(L"name"))->AsString();
                            if (m_datasets.contains(pivotName))
                                {
                                wxLogWarning(L"Dataset '%s' already exists "
                                             "and cannot be overwritten by a pivot.",
                                             pivotName);
                                continue;
                                }
                            m_datasets.insert_or_assign(pivotName, pivotedData);
                            m_datasetInsertionOrder.push_back(pivotName);

                            DatasetPivotOptions pivotOpts;
                            pivotOpts.m_type = PivotType::Wider;
                            pivotOpts.m_sourceDatasetName = parentName;
                            pivotOpts.m_idColumns = idColumns;
                            pivotOpts.m_namesFromColumn = namesFromColumn;
                            pivotOpts.m_valuesFromColumns = valuesFromColumns;
                            pivotOpts.m_namesSep = namesSep;
                            pivotOpts.m_namesPrefix = namesPrefix;
                            pivotOpts.m_fillValue = fillValue;
                            SetDatasetPivotOptions(pivotName, pivotOpts);

                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else if (pivotType.CmpNoCase(L"longer") == 0)
                        {
                        const auto columnsToKeep =
                            pivot->GetProperty(L"columns-to-keep")->AsStrings();
                        const auto fromColumns = pivot->GetProperty(L"from-columns")->AsStrings();
                        const auto namesTo = pivot->GetProperty(L"names-to")->AsStrings();
                        const auto valuesTo = pivot->GetProperty(L"values-to")->AsString();
                        const auto namesPattern = pivot->GetProperty(L"names-pattern")->AsString();

                        auto pivotedData =
                            Data::Pivot::PivotLonger(parentToPivot, columnsToKeep, fromColumns,
                                                     namesTo, valuesTo, namesPattern);

                        if (pivotedData)
                            {
                            const auto pivotName = pivot->GetProperty(_DT(L"name"))->AsString();
                            if (m_datasets.contains(pivotName))
                                {
                                wxLogWarning(L"Dataset '%s' already exists "
                                             "and cannot be overwritten by a pivot.",
                                             pivotName);
                                continue;
                                }
                            m_datasets.insert_or_assign(pivotName, pivotedData);
                            m_datasetInsertionOrder.push_back(pivotName);

                            DatasetPivotOptions pivotOpts;
                            pivotOpts.m_type = PivotType::Longer;
                            pivotOpts.m_sourceDatasetName = parentName;
                            pivotOpts.m_columnsToKeep = columnsToKeep;
                            pivotOpts.m_fromColumns = fromColumns;
                            pivotOpts.m_namesTo = namesTo;
                            pivotOpts.m_valuesTo = valuesTo;
                            pivotOpts.m_namesPattern = namesPattern;
                            SetDatasetPivotOptions(pivotName, pivotOpts);

                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: unrecognized pivot method."), pivotType)
                                .ToUTF8());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadSubsets(const wxSimpleJSON::Ptr_t& subsetsNode,
                                    const std::shared_ptr<const Data::Dataset>& parentToSubset)
        {
        static const std::map<std::wstring_view, Comparison> cmpOperators = {
            { L"=", Comparison::Equals },      { L"==", Comparison::Equals },
            { L"!=", Comparison::NotEquals },  { L"<>", Comparison::NotEquals },
            { L"<", Comparison::LessThan },    { L"<=", Comparison::LessThanOrEqualTo },
            { L">", Comparison::GreaterThan }, { L">=", Comparison::GreaterThanOrEqualTo }
        };

        // caches the raw filter info from a JSON filter node
        const auto cacheFilterInfo = [](const auto& filterNode)
        {
            DatasetFilterInfo info;
            info.m_column = filterNode->GetProperty(L"column")->AsString();
            info.m_operator = filterNode->GetProperty(L"operator")->AsString(L"=");
            const auto filterValues = filterNode->GetProperty(L"values")->AsNodes();
            for (const auto& filterValue : filterValues)
                {
                if (filterValue->IsValueString())
                    {
                    info.m_values.push_back(filterValue->AsString());
                    }
                else if (filterValue->IsValueNumber())
                    {
                    info.m_values.push_back(
                        wxString{ std::format(L"{}", filterValue->AsDouble()) });
                    }
                }
            return info;
        };

        const auto loadColumnFilter = [this, &parentToSubset](const auto& filterNode)
        {
            const auto foundPos = cmpOperators.find(std::wstring_view(
                filterNode->GetProperty(L"operator")->AsString().MakeLower().wc_str()));
            const Comparison cmp =
                (foundPos != cmpOperators.cend() ? foundPos->second : Comparison::Equals);

            const auto valuesNode = filterNode->GetProperty(L"values");

            if (valuesNode->IsOk())
                {
                Data::ColumnFilterInfo cFilter{ filterNode->GetProperty(L"column")->AsString(), cmp,
                                                std::vector<Data::DatasetValueType>() };
                if (!parentToSubset->ContainsColumn(cFilter.m_columnName) &&
                    !parentToSubset->GetContinuousColumns().empty() &&
                    cFilter.m_columnName.CmpNoCase(L"last-continuous-column") == 0)
                    {
                    cFilter.m_columnName = parentToSubset->GetContinuousColumnNames().back();
                    }
                const auto filterValues = valuesNode->AsNodes();
                if (filterValues.empty())
                    {
                    throw std::runtime_error(
                        _(L"No values were provided for subset filtering.").ToUTF8());
                    }
                for (const auto& filterValue : filterValues)
                    {
                    wxDateTime dt;
                    const bool isDate = (filterValue->IsValueString() &&
                                         (dt.ParseDateTime(filterValue->AsString()) ||
                                          dt.ParseDate(filterValue->AsString())));
                    cFilter.m_values.push_back(isDate ?
                                                   Data::DatasetValueType(dt) :
                                               filterValue->IsValueString() ?
                                                   Data::DatasetValueType(ExpandConstantsForFormula(
                                                       filterValue->AsString())) :
                                                   Data::DatasetValueType(filterValue->AsDouble()));
                    }

                return cFilter;
                }

            throw std::runtime_error(_(L"Comparison value for subset filter missing.").ToUTF8());
        };

        if (subsetsNode->IsOk())
            {
            // find the parent dataset name
            wxString parentDsName;
            for (const auto& [name, ds] : m_datasets)
                {
                if (ds == parentToSubset)
                    {
                    parentDsName = name;
                    break;
                    }
                }

            auto subsets = subsetsNode->AsNodes();
            for (const auto& subset : subsets)
                {
                if (subset->IsOk())
                    {
                    const auto sectionNode = subset->GetProperty(L"section");
                    const auto filterNode = subset->GetProperty(L"filter");
                    const auto filterAndNode = subset->GetProperty(L"filter-and");
                    const auto filterOrNode = subset->GetProperty(L"filter-or");
                    const auto validFilterTypeNodes = (filterNode->IsOk() ? 1 : 0) +
                                                      (filterAndNode->IsOk() ? 1 : 0) +
                                                      (filterOrNode->IsOk() ? 1 : 0);
                    if (validFilterTypeNodes > 1)
                        {
                        throw std::runtime_error(
                            _(L"Only one filter type allowed for a subset.").ToUTF8());
                        }
                    if (validFilterTypeNodes == 0 && sectionNode->IsNull())
                        {
                        throw std::runtime_error(
                            _(L"Subset missing filters or section definition.").ToUTF8());
                        }

                    DatasetSubsetOptions subsetOpts;
                    subsetOpts.m_sourceDatasetName = parentDsName;

                    Data::Subset dataSubsetter;
                    std::shared_ptr<Data::Dataset> subsettedDataset{ nullptr };
                    // single column filter
                    if (filterNode->IsOk())
                        {
                        subsetOpts.m_filterType = DatasetSubsetOptions::FilterType::Single;
                        subsetOpts.m_filters.push_back(cacheFilterInfo(filterNode));
                        subsettedDataset = dataSubsetter.SubsetSimple(parentToSubset,
                                                                      loadColumnFilter(filterNode));
                        }
                    // ANDed filters
                    else if (filterAndNode->IsOk())
                        {
                        subsetOpts.m_filterType = DatasetSubsetOptions::FilterType::And;
                        std::vector<Data::ColumnFilterInfo> cf;
                        const auto filterAndNodes = filterAndNode->AsNodes();
                        if (filterAndNodes.empty())
                            {
                            throw std::runtime_error(_(L"Subset missing filters.").ToUTF8());
                            }
                        cf.reserve(filterAndNodes.size());
                        for (const auto& fAndNode : filterAndNodes)
                            {
                            subsetOpts.m_filters.push_back(cacheFilterInfo(fAndNode));
                            cf.push_back(loadColumnFilter(fAndNode));
                            }

                        subsettedDataset = dataSubsetter.SubsetAnd(parentToSubset, cf);
                        }
                    // ORed filters
                    else if (filterOrNode->IsOk())
                        {
                        subsetOpts.m_filterType = DatasetSubsetOptions::FilterType::Or;
                        std::vector<Data::ColumnFilterInfo> cf;
                        const auto filterOrNodes = filterOrNode->AsNodes();
                        if (filterOrNodes.empty())
                            {
                            throw std::runtime_error(_(L"Subset missing filters.").ToUTF8());
                            }
                        cf.reserve(filterOrNodes.size());
                        for (const auto& fOrNode : filterOrNodes)
                            {
                            subsetOpts.m_filters.push_back(cacheFilterInfo(fOrNode));
                            cf.push_back(loadColumnFilter(fOrNode));
                            }

                        subsettedDataset = dataSubsetter.SubsetOr(parentToSubset, cf);
                        }
                    else if (sectionNode->IsOk())
                        {
                        subsetOpts.m_filterType = DatasetSubsetOptions::FilterType::Section;
                        subsetOpts.m_sectionColumn =
                            sectionNode->GetProperty(_DT(L"column"))->AsString();
                        subsetOpts.m_sectionStartLabel =
                            sectionNode->GetProperty(L"start-label")->AsString();
                        subsetOpts.m_sectionEndLabel =
                            sectionNode->GetProperty(L"end-label")->AsString();
                        subsetOpts.m_sectionIncludeSentinelLabels =
                            sectionNode->GetProperty(L"include-sentinel-labels")->AsBool(true);
                        subsettedDataset = dataSubsetter.SubsetSection(
                            parentToSubset, subsetOpts.m_sectionColumn,
                            subsetOpts.m_sectionStartLabel, subsetOpts.m_sectionEndLabel,
                            subsetOpts.m_sectionIncludeSentinelLabels);
                        }

                    if (subsettedDataset)
                        {
                        const auto subsetName = subset->GetProperty(_DT(L"name"))->AsString();
                        if (m_datasets.contains(subsetName))
                            {
                            wxLogWarning(L"Dataset '%s' already exists "
                                         "and cannot be overwritten by a subset.",
                                         subsetName);
                            continue;
                            }
                        m_datasets.insert_or_assign(subsetName, subsettedDataset);
                        m_datasetInsertionOrder.push_back(subsetName);
                        SetDatasetSubsetOptions(subsetName, subsetOpts);
                        LoadDatasetTransformations(subset, subsettedDataset);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasetTransformations(const wxSimpleJSON::Ptr_t& dsNode,
                                                   const std::shared_ptr<Data::Dataset>& dataset)
        {
        if (dsNode->IsOk())
            {
            // find the dataset name by looking up the pointer
            wxString dsName;
            for (const auto& [name, ds] : m_datasets)
                {
                if (ds == dataset)
                    {
                    dsName = name;
                    break;
                    }
                }

            DatasetTransformOptions transformOpts;

            // column renaming
            auto colRenames = dsNode->GetProperty(L"columns-rename")->AsNodes();
            for (const auto& colRename : colRenames)
                {
                DatasetColumnRename renameOpt;
                if (colRename->HasProperty(_DT(L"name")))
                    {
                    renameOpt.m_name = colRename->GetProperty(_DT(L"name"))->AsString();
                    renameOpt.m_newName = colRename->GetProperty(L"new-name")->AsString();
                    dataset->RenameColumn(renameOpt.m_name, renameOpt.m_newName);
                    }
                if (colRename->HasProperty(L"name-re"))
                    {
                    renameOpt.m_nameRe = colRename->GetProperty(L"name-re")->AsString();
                    renameOpt.m_newNameRe = colRename->GetProperty(L"new-name-re")->AsString();
                    dataset->RenameColumnRE(renameOpt.m_nameRe, renameOpt.m_newNameRe);
                    }
                transformOpts.m_columnRenames.push_back(std::move(renameOpt));
                }

            // column mutations
            auto mutateCats = dsNode->GetProperty(L"mutate-categorical-columns")->AsNodes();
            for (const auto& mutateCat : mutateCats)
                {
                DatasetMutateCategoricalColumn mutateOpt;
                mutateOpt.m_sourceColumn = mutateCat->GetProperty(L"source-column")->AsString();
                mutateOpt.m_targetColumn = mutateCat->GetProperty(L"target-column")->AsString();

                Data::RegExMap reMap;
                const auto replacements = mutateCat->GetProperty(L"replacements")->AsNodes();
                for (const auto& replacement : replacements)
                    {
                    const auto pattern = replacement->GetProperty(L"pattern")->AsString();
                    const auto repl = replacement->GetProperty(L"replacement")->AsString();
                    mutateOpt.m_replacements.emplace_back(pattern, repl);
                    reMap.emplace_back(std::make_unique<wxRegEx>(pattern), repl);
                    }

                dataset->MutateCategoricalColumn(mutateOpt.m_sourceColumn, mutateOpt.m_targetColumn,
                                                 reMap);
                transformOpts.m_mutateCategoricalColumns.push_back(std::move(mutateOpt));
                }

            // column SELECT
            auto selectPattern = dsNode->GetProperty(L"columns-select")->AsString();
            if (!selectPattern.empty())
                {
                transformOpts.m_columnsSelect = selectPattern;
                dataset->SelectColumnsRE(selectPattern);
                }

            // label recoding
            auto recodeREs = dsNode->GetProperty(L"recode-re")->AsNodes();
            for (const auto& recodeRE : recodeREs)
                {
                DatasetRecodeRe recodeOpt;
                recodeOpt.m_column = recodeRE->GetProperty(L"column")->AsString();
                recodeOpt.m_pattern = recodeRE->GetProperty(L"pattern")->AsString();
                recodeOpt.m_replacement = recodeRE->GetProperty(L"replacement")->AsString();
                dataset->RecodeRE(recodeOpt.m_column, recodeOpt.m_pattern, recodeOpt.m_replacement);
                transformOpts.m_recodeREs.push_back(std::move(recodeOpt));
                }

            // category collapsing (min)
            auto collapseMins = dsNode->GetProperty(L"collapse-min")->AsNodes();
            for (const auto& collapseMin : collapseMins)
                {
                DatasetCollapseMin collapseOpt;
                collapseOpt.m_column = collapseMin->GetProperty(L"column")->AsString();
                collapseOpt.m_min = collapseMin->GetProperty(L"min")->AsDouble(2);
                const auto otherLabel = collapseMin->GetProperty(L"other-label")->AsString();
                collapseOpt.m_otherLabel = otherLabel;
                dataset->CollapseMin(collapseOpt.m_column, collapseOpt.m_min,
                                     otherLabel.empty() ? _(L"Other") : otherLabel);
                transformOpts.m_collapseMins.push_back(std::move(collapseOpt));
                }

            // category collapsing (except)
            auto collapseExcepts = dsNode->GetProperty(L"collapse-except")->AsNodes();
            for (const auto& collapseExcept : collapseExcepts)
                {
                DatasetCollapseExcept collapseOpt;
                collapseOpt.m_column = collapseExcept->GetProperty(L"column")->AsString();
                collapseOpt.m_labelsToKeep =
                    collapseExcept->GetProperty(L"labels-to-keep")->AsStrings();
                const auto otherLabel = collapseExcept->GetProperty(L"other-label")->AsString();
                collapseOpt.m_otherLabel = otherLabel;
                dataset->CollapseExcept(collapseOpt.m_column, collapseOpt.m_labelsToKeep,
                                        otherLabel.empty() ? _(L"Other") : otherLabel);
                transformOpts.m_collapseExcepts.push_back(std::move(collapseOpt));
                }

            // cache formula raw strings before evaluation
            const auto formulasNode = dsNode->GetProperty(L"formulas");
            if (formulasNode->IsOk())
                {
                const auto formulas = formulasNode->AsNodes();
                for (const auto& formula : formulas)
                    {
                    if (formula->IsOk())
                        {
                        DatasetFormulaInfo formulaOpt;
                        formulaOpt.m_name = formula->GetProperty(_DT(L"name"))->AsString();
                        if (formula->GetProperty(L"value")->IsValueString())
                            {
                            formulaOpt.m_value = formula->GetProperty(L"value")->AsString();
                            }
                        else if (formula->GetProperty(L"value")->IsValueNumber())
                            {
                            formulaOpt.m_value = wxString{ std::format(
                                L"{}", formula->GetProperty(L"value")->AsDouble()) };
                            }
                        transformOpts.m_formulas.push_back(std::move(formulaOpt));
                        }
                    }
                }

            // load any constants defined with this dataset
            CalcFormulas(formulasNode, dataset);

            // load any subsets of this dataset
            LoadSubsets(dsNode->GetProperty(L"subsets"), dataset);

            // load any pivots of this dataset
            LoadPivots(dsNode->GetProperty(L"pivots"), dataset, dsName);

            // load any merges of this dataset
            LoadMerges(dsNode->GetProperty(L"merges"), dataset);

            transformOpts.m_columnNamesSort = dsNode->GetProperty(L"column-names-sort")->AsBool();
            if (transformOpts.m_columnNamesSort)
                {
                dataset->SortColumnNames();
                }

            if (!dsName.empty())
                {
                SetDatasetTransformOptions(dsName, transformOpts);
                }
            else
                {
                wxLogWarning(_(L"Transformation dataset without a name."),
                             wxOK | wxICON_WARNING | wxCENTRE);
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasets(const wxSimpleJSON::Ptr_t& datasetsNode)
        {
        if (datasetsNode->IsOk())
            {
            auto datasets = datasetsNode->AsNodes();
            bool anyPathResolved{ false };
            for (const auto& datasetNode : datasets)
                {
                if (datasetNode->IsOk())
                    {
                    wxString path = datasetNode->GetProperty(L"path")->AsString();
                    if (path.empty())
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a filepath.")).ToUTF8());
                        }
                    bool pathResolved{ false };
                    if (!wxFileName::FileExists(path))
                        {
                        path = wxFileName{ m_configFilePath }.GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            if (!m_missingDatasetResolver)
                                {
                                throw std::runtime_error(
                                    wxString::Format(_(L"'%s': dataset not found."), path)
                                        .ToUTF8());
                                }
                            const auto resolvedPath = m_missingDatasetResolver(path);
                            if (!resolvedPath.has_value() ||
                                !wxFileName::FileExists(resolvedPath.value()))
                                {
                                wxLogWarning(_(L"'%s': dataset not found and was skipped."), path);
                                continue;
                                }
                            path = resolvedPath.value();
                            pathResolved = true;
                            }
                        }
                    // store a clean absolute path rather than whatever mix of
                    // relative parts and separators the project happened to hold
                    if (wxFileName resolved{ path };
                        resolved.MakeAbsolute(wxFileName{ m_configFilePath }.GetPath()))
                        {
                        path = resolved.GetFullPath();
                        }
                    // supplied name, or if name is blank then use the file name
                    wxString dsName = datasetNode->GetProperty(_DT(L"name"))->AsString();
                    if (dsName.empty())
                        {
                        dsName = wxFileName{ path }.GetName();
                        }

                    const wxString importer = datasetNode->GetProperty(L"importer")->AsString();
                    // read the variables info
                    //------------------------
                    // ID column
                    const wxString idColumn = datasetNode->GetProperty(L"id-column")->AsString();
                    // columns the user explicitly excluded from importing
                    const std::vector<wxString> excludedColumns =
                        datasetNode->GetProperty(L"excluded-columns")->AsStrings();
                    // date columns
                    // (only columns whose type the user explicitly overrode are listed here;
                    //  everything else is deduced fresh from the file below, so changes to
                    //  the source file's columns don't break reimporting)
                    std::vector<Data::ImportInfo::DateImportInfo> dateInfo;
                    const auto dateProperty = datasetNode->GetProperty(L"date-columns");
                    if (dateProperty->IsOk())
                        {
                        const auto dateVars = dateProperty->AsNodes();
                        for (const auto& dateVar : dateVars)
                            {
                            if (dateVar->IsOk())
                                {
                                // get the date column's name and how to load it
                                const wxString dateName =
                                    dateVar->GetProperty(_DT(L"name"))->AsString();
                                if (dateName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Date column must have a name.")).ToUTF8());
                                    }
                                const wxString dateParser =
                                    dateVar->GetProperty(L"parser")->AsString();
                                const wxString dateFormat =
                                    dateVar->GetProperty(L"format")->AsString();
                                dateInfo.push_back(
                                    { dateName,
                                      (dateParser.CmpNoCase(L"iso-date") == 0 ?
                                           Data::DateImportMethod::IsoDate :
                                       dateParser.CmpNoCase(L"time") == 0 ?
                                           Data::DateImportMethod::Time :
                                       dateParser.CmpNoCase(L"iso-combined") == 0 ?
                                           Data::DateImportMethod::IsoCombined :
                                       dateParser.CmpNoCase(L"strptime-format") == 0 ?
                                           Data::DateImportMethod::StrptimeFormatString :
                                       dateParser.CmpNoCase(L"rfc822") == 0 ?
                                           Data::DateImportMethod::Rfc822 :
                                           Data::DateImportMethod::Automatic),
                                      dateFormat });
                                }
                            }
                        }
                    // continuous columns
                    const std::vector<wxString> continuousVars =
                        datasetNode->GetProperty(L"continuous-columns")->AsStrings();
                    // categorical columns
                    std::vector<Data::ImportInfo::CategoricalImportInfo> catInfo;
                    const auto catProperty = datasetNode->GetProperty(L"categorical-columns");
                    if (catProperty->IsOk())
                        {
                        const auto catVars = catProperty->AsNodes();
                        for (const auto& catVar : catVars)
                            {
                            if (catVar->IsOk())
                                {
                                // get the cat column's name and how to load it
                                const wxString catName =
                                    catVar->GetProperty(_DT(L"name"))->AsString();
                                if (catName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Categorical column must have a name."))
                                            .ToUTF8());
                                    }
                                const wxString catParser =
                                    catVar->GetProperty(L"parser")->AsString();
                                catInfo.push_back(
                                    { catName,
                                      (catParser.CmpNoCase(L"as-integers") == 0 ?
                                           Data::CategoricalImportMethod::ReadAsIntegers :
                                           Data::CategoricalImportMethod::ReadAsStrings) });
                                }
                            }
                        }

                    // create the dataset
                    auto dataset = std::make_shared<Data::Dataset>();

                    Data::ImportInfo importDefines;
                    const auto fillImportDefines = [&importDefines, &datasetNode]()
                    {
                        if (datasetNode->HasProperty(L"skip-rows"))
                            {
                            importDefines.SkipRows(
                                datasetNode->GetProperty(L"skip-rows")->AsDouble(0));
                            }
                        if (datasetNode->HasProperty(L"md-codes"))
                            {
                            importDefines.MDCodes(wxStringVectorToWstringVector(
                                datasetNode->GetProperty(L"md-codes")->AsStrings()));
                            }
                        if (datasetNode->HasProperty(L"treat-leading-zeros-as-text"))
                            {
                            importDefines.TreatLeadingZerosAsText(
                                datasetNode->GetProperty(L"treat-leading-zeros-as-text")->AsBool());
                            }
                        if (datasetNode->HasProperty(L"treat-years-as-text"))
                            {
                            importDefines.TreatYearsAsText(
                                datasetNode->GetProperty(L"treat-years-as-text")->AsBool());
                            }
                        if (datasetNode->HasProperty(L"column-names-sort"))
                            {
                            importDefines.ColumnNamesSort(
                                datasetNode->GetProperty(L"column-names-sort")->AsBool());
                            }
                        if (datasetNode->HasProperty(L"max-discrete-value"))
                            {
                            importDefines.MaxDiscreteValue(
                                datasetNode->GetProperty(L"max-discrete-value")->AsDouble());
                            }
                    };
                    fillImportDefines();

                    const auto worksheetNode = datasetNode->GetProperty(L"worksheet");
                    const std::variant<wxString, size_t> worksheet =
                        worksheetNode->IsNull() ?
                            std::variant<wxString, size_t>(static_cast<size_t>(1)) :
                        worksheetNode->IsValueNumber() ?
                            std::variant<wxString, size_t>(
                                static_cast<size_t>(worksheetNode->AsDouble())) :
                            std::variant<wxString, size_t>(worksheetNode->AsString());
                    // Always deduce the columns fresh from the file (in file order), then
                    // layer any persisted user overrides (type changes, exclusions) on top
                    // by name. Overrides for columns no longer in the file are simply
                    // dropped rather than failing the import.
                    Data::Dataset::ColumnPreviewInfo columnPreviewInfo =
                        Data::Dataset::ReadColumnInfo(path, importDefines, std::nullopt, worksheet);

                    const auto findColumn =
                        [&columnPreviewInfo](const wxString& name) -> Data::Dataset::ColumnPreview*
                    {
                        const auto colIt =
                            std::ranges::find_if(columnPreviewInfo, [&name](const auto& col)
                                                 { return col.m_name.CmpNoCase(name) == 0; });
                        return (colIt != columnPreviewInfo.end()) ? &(*colIt) : nullptr;
                    };

                    for (const auto& name : excludedColumns)
                        {
                        if (auto* col = findColumn(name); col != nullptr)
                            {
                            col->m_excluded = true;
                            }
                        }
                    for (const auto& di : dateInfo)
                        {
                        if (auto* col = findColumn(di.m_columnName); col != nullptr)
                            {
                            col->m_type = Data::Dataset::ColumnImportType::Date;
                            col->m_userOverridden = true;
                            }
                        }
                    for (const auto& cv : continuousVars)
                        {
                        if (auto* col = findColumn(cv); col != nullptr)
                            {
                            col->m_type = Data::Dataset::ColumnImportType::Numeric;
                            col->m_userOverridden = true;
                            }
                        }
                    for (const auto& ci : catInfo)
                        {
                        if (auto* col = findColumn(ci.m_columnName); col != nullptr)
                            {
                            col->m_type = (ci.m_importMethod ==
                                           Data::CategoricalImportMethod::ReadAsIntegers) ?
                                              Data::Dataset::ColumnImportType::Discrete :
                                              Data::Dataset::ColumnImportType::String;
                            col->m_userOverridden = true;
                            }
                        }

                    // build the ImportInfo from the deduced (and now overridden) columns;
                    // this fills in the continuous/categorical/date column lists for
                    // everything that wasn't explicitly overridden above
                    Data::Dataset::ColumnPreviewInfo includedColumns;
                    includedColumns.reserve(columnPreviewInfo.size());
                    std::ranges::copy_if(columnPreviewInfo, std::back_inserter(includedColumns),
                                         [](const auto& col) { return !col.m_excluded; });
                    importDefines = Data::Dataset::ImportInfoFromPreview(includedColumns);
                    fillImportDefines();
                    importDefines.ContinuousMDRecodeValue(
                        datasetNode->GetProperty(L"continuous-md-recode-value")
                            ->AsDouble(std::numeric_limits<double>::quiet_NaN()));
                    // only honor the ID column if it's still present (and included) in the file
                    if (const auto* idCol = findColumn(idColumn);
                        !idColumn.empty() && idCol != nullptr && !idCol->m_excluded)
                        {
                        importDefines.IdColumn(idColumn);
                        }

                    // re-apply the precise parser/format for explicitly-overridden date columns
                    if (!dateInfo.empty())
                        {
                        std::vector<Data::ImportInfo::DateImportInfo> mergedDateInfo;
                        mergedDateInfo.reserve(importDefines.GetDateColumns().size());
                        for (const auto& di : importDefines.GetDateColumns())
                            {
                            const auto overrideIt = std::ranges::find_if(
                                dateInfo, [&di](const auto& override_)
                                { return override_.m_columnName.CmpNoCase(di.m_columnName) == 0; });
                            mergedDateInfo.push_back(overrideIt != dateInfo.cend() ? *overrideIt :
                                                                                     di);
                            }
                        importDefines.DateColumns(mergedDateInfo);
                        }

                    // import using the user-provided parser or deduce from the file extension
                    const auto fileExt(wxFileName{ path }.GetExt());
                    if (importer.CmpNoCase(L"csv") == 0 || fileExt.CmpNoCase(L"csv") == 0)
                        {
                        dataset->ImportCSV(path, importDefines);
                        }
                    else if (importer.CmpNoCase(L"tsv") == 0 || fileExt.CmpNoCase(L"tsv") == 0 ||
                             fileExt.CmpNoCase(L"txt") == 0)
                        {
                        dataset->ImportTSV(path, importDefines);
                        }
                    else if (importer.CmpNoCase(L"xlsx") == 0 || fileExt.CmpNoCase(L"xlsx") == 0)
                        {
                        dataset->ImportExcel(path, worksheet, importDefines);
                        }
                    else if (importer.CmpNoCase(L"ods") == 0 || fileExt.CmpNoCase(L"ods") == 0)
                        {
                        dataset->ImportOds(path, worksheet, importDefines);
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a valid importer specified.")).ToUTF8());
                        }

                    if (m_datasets.contains(dsName))
                        {
                        wxLogWarning(L"Dataset '%s' already exists and will be overwritten.",
                                     dsName);
                        }
                    AddDataset(dsName, dataset,
                               DatasetImportOptions{ path, importer, worksheet, columnPreviewInfo,
                                                     importDefines });
                    if (pathResolved)
                        {
                        anyPathResolved = true;
                        }
                    // recode values, build subsets and pivots, etc.
                    LoadDatasetTransformations(datasetNode, dataset);
                    // update column preview info to reflect any renames
                    const auto txIt = m_datasetTransformOptions.find(dsName);
                    if (txIt != m_datasetTransformOptions.cend())
                        {
                        auto& cpInfo = m_datasetImportOptions[dsName].m_columnPreviewInfo;
                        for (const auto& rename : txIt->second.m_columnRenames)
                            {
                            if (!rename.m_name.empty() && !rename.m_newName.empty())
                                {
                                for (auto& col : cpInfo)
                                    {
                                    if (col.m_name.CmpNoCase(rename.m_name) == 0)
                                        {
                                        col.m_name = rename.m_newName;
                                        break;
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            // only committed if every dataset loaded without throwing
            m_resolvedMissingDatasets = anyPathResolved;
            }
        }

    //---------------------------------------------------
    std::unique_ptr<GraphItems::FillableShape>
    ReportBuilder::LoadFillableShape(const wxSimpleJSON::Ptr_t& shapeNode) const
        {
        const auto loadedShape =
            ReportEnumConvert::ConvertIcon(shapeNode->GetProperty(L"icon")->AsString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(wxString::Format(_(L"%s: unknown icon for fillable shape."),
                                                      shapeNode->GetProperty(L"icon")->AsString())
                                         .ToUTF8());
            }

        wxSize sz(32, 32);
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->AsDouble(32);
            sz.y = sizeNode->GetProperty(L"height")->AsDouble(32);
            }

        wxPen pen(Colors::ColorBrewer::GetColor(Colors::Color::Black));
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(Colors::ColorBrewer::GetColor(Colors::Color::Black));
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        double fillPercent{ math_constants::empty };
        const auto fillPercentNode = shapeNode->GetProperty(L"fill-percent");
        if (fillPercentNode->IsOk())
            {
            if (fillPercentNode->IsValueNumber())
                {
                fillPercent = fillPercentNode->AsDouble(fillPercent);
                }
            else if (fillPercentNode->IsValueString())
                {
                if (const auto numberVal = ExpandNumericConstant(fillPercentNode->AsString()))
                    {
                    fillPercent = numberVal.value();
                    }
                }
            }

        auto shapeLabel = LoadLabel(shapeNode->GetProperty(L"label"), GraphItems::Label{});

        auto sh = std::make_unique<GraphItems::FillableShape>(
            GraphItems::GraphItemInfo{
                (shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}) }
                .Anchoring(Anchoring::TopLeftCorner)
                .Pen(pen)
                .Brush(brush)
                .FontColor((shapeLabel != nullptr ?
                                shapeLabel->GetFontColor() :
                                Colors::ColorBrewer::GetColor(Colors::Color::Black))),
            loadedShape.value(), sz, fillPercent);

        LoadItem(shapeNode, *sh);

        // cache size for round-tripping
        if (sizeNode->IsOk())
            {
            if (sizeNode->HasProperty(L"width"))
                {
                sh->SetPropertyTemplate(L"size.width", std::to_wstring(sz.x));
                }
            if (sizeNode->HasProperty(L"height"))
                {
                sh->SetPropertyTemplate(L"size.height", std::to_wstring(sz.y));
                }
            }
        // cache fill-percent template for round-tripping
        if (fillPercentNode->IsOk() && fillPercentNode->IsValueString())
            {
            sh->SetPropertyTemplate(L"fill-percent", fillPercentNode->AsString());
            }
        // cache label templates for round-tripping
        if (shapeLabel != nullptr)
            {
            const auto textTmpl = shapeLabel->GetPropertyTemplate(L"text");
            if (!textTmpl.empty())
                {
                sh->SetPropertyTemplate(L"label.text", textTmpl);
                }
            const auto colorTmpl = shapeLabel->GetPropertyTemplate(L"color");
            if (!colorTmpl.empty())
                {
                sh->SetPropertyTemplate(L"label.color", colorTmpl);
                }
            }

        // fit the column area to this shape
        sh->SetFixedWidthOnCanvas(true);
        return sh;
        }

    //---------------------------------------------------
    GraphItems::ShapeInfo ReportBuilder::LoadShapeInfo(const wxSimpleJSON::Ptr_t& shapeNode) const
        {
        const auto loadedShape =
            ReportEnumConvert::ConvertIcon(shapeNode->GetProperty(L"icon")->AsString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(wxString::Format(_(L"%s: unknown icon for shape."),
                                                      shapeNode->GetProperty(L"icon")->AsString())
                                         .ToUTF8());
            }

        wxSize sz{ 32, 32 };
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->AsDouble(32);
            sz.y = sizeNode->GetProperty(L"height")->AsDouble(32);
            }

        wxPen pen(Colors::ColorBrewer::GetColor(Colors::Color::Black));
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(Colors::ColorBrewer::GetColor(Colors::Color::White));
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        auto shapeLabel = LoadLabel(shapeNode->GetProperty(L"label"), GraphItems::Label{});

        double fillPercent{ math_constants::full };
        const auto fillPercentNode = shapeNode->GetProperty(L"fill-percent");
        if (fillPercentNode->IsOk())
            {
            if (fillPercentNode->IsValueNumber())
                {
                fillPercent = fillPercentNode->AsDouble(fillPercent);
                }
            else if (fillPercentNode->IsValueString())
                {
                if (const auto numberVal = ExpandNumericConstant(fillPercentNode->AsString()))
                    {
                    fillPercent = numberVal.value();
                    }
                }
            }

        double repeat{ 1.0 };
        const auto repeatNode = shapeNode->GetProperty(L"repeat");
        if (repeatNode->IsOk())
            {
            if (repeatNode->IsValueNumber())
                {
                repeat = repeatNode->AsDouble(repeat);
                }
            else if (repeatNode->IsValueString())
                {
                if (const auto numberVal = ExpandNumericConstant(repeatNode->AsString()))
                    {
                    repeat = numberVal.value();
                    }
                }
            }

        auto result = GraphItems::ShapeInfo{}
                          .Shape(loadedShape.value())
                          .Size(sz)
                          .Pen(pen)
                          .Brush(brush)
                          .Text((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}))
                          .Repeat(wxRound(repeat))
                          .FillPercent(fillPercent);

        // cache the raw string if it was a constant reference
        if (repeatNode->IsOk() && repeatNode->IsValueString())
            {
            result.SetPropertyTemplate(L"repeat", repeatNode->AsString());
            }
        if (fillPercentNode->IsOk() && fillPercentNode->IsValueString())
            {
            result.SetPropertyTemplate(L"fill-percent", fillPercentNode->AsString());
            }

        return result;
        }

    //---------------------------------------------------
    std::unique_ptr<GraphItems::Shape>
    ReportBuilder::LoadShape(const wxSimpleJSON::Ptr_t& shapeNode) const
        {
        const auto loadedShape =
            ReportEnumConvert::ConvertIcon(shapeNode->GetProperty(L"icon")->AsString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(wxString::Format(_(L"%s: unknown icon for shape."),
                                                      shapeNode->GetProperty(L"icon")->AsString())
                                         .ToUTF8());
            }

        wxSize sz{ 32, 32 };
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->AsDouble(32);
            sz.y = sizeNode->GetProperty(L"height")->AsDouble(32);
            }

        wxPen pen(Colors::ColorBrewer::GetColor(Colors::Color::Black));
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(Colors::ColorBrewer::GetColor(Colors::Color::White));
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        auto shapeLabel = LoadLabel(shapeNode->GetProperty(L"label"), GraphItems::Label{});

        auto sh = std::make_unique<GraphItems::Shape>(
            GraphItems::GraphItemInfo{
                (shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}) }
                .Anchoring(Anchoring::TopLeftCorner)
                .Pen(pen)
                .Brush(brush)
                .FontColor((shapeLabel != nullptr ?
                                shapeLabel->GetFontColor() :
                                Colors::ColorBrewer::GetColor(Colors::Color::Black))),
            loadedShape.value(), sz);

        LoadItem(shapeNode, *sh);

        // cache size for round-tripping
        if (sizeNode->IsOk())
            {
            if (sizeNode->HasProperty(L"width"))
                {
                sh->SetPropertyTemplate(L"size.width", std::to_wstring(sz.x));
                }
            if (sizeNode->HasProperty(L"height"))
                {
                sh->SetPropertyTemplate(L"size.height", std::to_wstring(sz.y));
                }
            }
        // cache label templates for round-tripping
        if (shapeLabel != nullptr)
            {
            const auto textTmpl = shapeLabel->GetPropertyTemplate(L"text");
            if (!textTmpl.empty())
                {
                sh->SetPropertyTemplate(L"label.text", textTmpl);
                }
            const auto colorTmpl = shapeLabel->GetPropertyTemplate(L"color");
            if (!colorTmpl.empty())
                {
                sh->SetPropertyTemplate(L"label.color", colorTmpl);
                }
            }

        // fit the column area to this shape
        sh->SetFixedWidthOnCanvas(true);
        return sh;
        }

    //---------------------------------------------------
    void ReportBuilder::ApplyTableFeatures(std::shared_ptr<Graphs::Table>& table) const
        {
        const ReportTableLoader loader(*this);
        loader.ApplyTableFeatures(table);
        }

    //---------------------------------------------------
    std::shared_ptr<Brushes::Schemes::BrushScheme>
    ReportBuilder::LoadBrushScheme(const wxSimpleJSON::Ptr_t& brushSchemeNode) const
        {
        const auto brushStylesNode = brushSchemeNode->GetProperty(L"brush-styles");
        if (brushStylesNode->IsOk() && brushStylesNode->IsValueArray())
            {
            std::vector<wxBrushStyle> brushStyles;
            const auto brushStylesVals = brushStylesNode->AsStrings();
            for (const auto& brushStylesVal : brushStylesVals)
                {
                if (const auto bStyle = ReportEnumConvert::ConvertBrushStyle(brushStylesVal))
                    {
                    brushStyles.push_back(bStyle.value());
                    }
                }
            if (const auto colorScheme =
                    LoadColorScheme(brushSchemeNode->GetProperty(L"color-scheme")))
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(brushStyles, *colorScheme);
                }
            }
        // object with just a color-scheme array (no brush-styles means solid)
        else if (const auto colorScheme =
                     LoadColorScheme(brushSchemeNode->GetProperty(L"color-scheme")))
            {
            return std::make_shared<Brushes::Schemes::BrushScheme>(*colorScheme);
            }
        // just a named color scheme
        else if (brushSchemeNode->IsValueString())
            {
            if (const auto namedColorScheme = LoadColorScheme(brushSchemeNode))
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(*namedColorScheme);
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Colors::Schemes::ColorScheme>
    ReportBuilder::LoadGraphColorScheme(const wxSimpleJSON::Ptr_t& graphNode) const
        {
        auto colorScheme = LoadColorScheme(graphNode->GetProperty(L"color-scheme"));
        if (colorScheme == nullptr)
            {
            const auto brushSchemeNode = graphNode->GetProperty(L"brush-scheme");
            if (brushSchemeNode->IsOk())
                {
                colorScheme = LoadColorScheme(brushSchemeNode->GetProperty(L"color-scheme"));
                }
            }
        return colorScheme;
        }

    //---------------------------------------------------
    std::shared_ptr<Colors::Schemes::ColorScheme>
    ReportBuilder::LoadColorScheme(const wxSimpleJSON::Ptr_t& colorSchemeNode) const
        {
        if (!colorSchemeNode->IsOk())
            {
            return nullptr;
            }
        if (colorSchemeNode->IsValueArray())
            {
            std::vector<wxColour> colors;
            const auto colorValues = colorSchemeNode->AsStrings();
            if (colorValues.empty())
                {
                return nullptr;
                }
            colors.reserve(colorValues.size());
            for (const auto& color : colorValues)
                {
                colors.push_back(ConvertColor(color));
                }
            return std::make_shared<Colors::Schemes::ColorScheme>(colors);
            }
        if (colorSchemeNode->IsValueString())
            {
            return ReportEnumConvert::ConvertColorScheme(colorSchemeNode->AsString());
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<LineStyleScheme>
    ReportBuilder::LoadLineStyleScheme(const wxSimpleJSON::Ptr_t& lineStyleSchemeNode) const
        {
        if (!lineStyleSchemeNode->IsOk())
            {
            return nullptr;
            }
        // a list of icons
        if (lineStyleSchemeNode->IsValueArray())
            {
            std::vector<std::pair<wxPenStyle, LineStyle>> lineStyles;
            const auto lineStyleValues = lineStyleSchemeNode->AsNodes();
            for (const auto& lineStyle : lineStyleValues)
                {
                wxPen pn(Colors::ColorBrewer::GetColor(Colors::Color::Black), 1,
                         wxPenStyle::wxPENSTYLE_SOLID);
                LoadPen(lineStyle->GetProperty(L"pen-style"), pn);
                const auto foundLineStyle = ReportEnumConvert::ConvertLineStyle(
                    lineStyle->GetProperty(L"line-style")->AsString());
                if (foundLineStyle.has_value())
                    {
                    lineStyles.emplace_back(pn.GetStyle(), foundLineStyle.value());
                    }
                }
            if (lineStyles.empty())
                {
                return nullptr;
                }
            return std::make_shared<LineStyleScheme>(lineStyles);
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Icons::Schemes::IconScheme>
    ReportBuilder::LoadIconScheme(const wxSimpleJSON::Ptr_t& iconSchemeNode)
        {
        static const std::map<std::wstring_view, std::shared_ptr<Icons::Schemes::IconScheme>>
            iconSchemes = { { L"standard-shapes",
                              std::make_shared<Icons::Schemes::StandardShapes>() },
                            { L"semesters", std::make_shared<Icons::Schemes::Semesters>() } };

        if (!iconSchemeNode->IsOk())
            {
            return nullptr;
            }
        // a list of icons
        if (iconSchemeNode->IsValueArray())
            {
            std::vector<Icons::IconShape> icons;
            const auto iconValues = iconSchemeNode->AsStrings();
            if (iconValues.empty())
                {
                return nullptr;
                }
            for (const auto& icon : iconValues)
                {
                const auto iconValue = ReportEnumConvert::ConvertIcon(icon);
                if (iconValue.has_value())
                    {
                    icons.push_back(iconValue.value());
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for icon scheme."), icon).ToUTF8());
                    }
                }
            if (icons.empty())
                {
                return nullptr;
                }
            return std::make_shared<Icons::Schemes::IconScheme>(icons);
            }
        // a pre-defined icon scheme
        if (iconSchemeNode->IsValueString())
            {
            const auto foundPos = iconSchemes.find(
                std::wstring_view(iconSchemeNode->AsString().MakeLower().wc_str()));
            if (foundPos != iconSchemes.cend())
                {
                return foundPos->second;
                }
            // a single icon that should be recycled
            const auto iconValue = ReportEnumConvert::ConvertIcon(iconSchemeNode->AsString());
            if (iconValue.has_value())
                {
                return std::make_shared<Icons::Schemes::IconScheme>(
                    std::vector<Wisteria::Icons::IconShape>{ iconValue.value() });
                }

            throw std::runtime_error(wxString::Format(_(L"%s: unknown icon for icon scheme."),
                                                      iconSchemeNode->AsString())
                                         .ToUTF8());
            }

        return nullptr;
        }

    //---------------------------------------------------
    wxString ReportBuilder::NormalizeFilePath(const wxString& path) const
        {
        wxString expandedPath{ path };
        if (expandedPath.empty())
            {
            throw std::runtime_error(_(L"Filepath is empty.").ToUTF8());
            }
        if (!wxFileName::FileExists(expandedPath))
            {
            expandedPath = wxFileName{ m_configFilePath }.GetPathWithSep() + expandedPath;
            if (!wxFileName::FileExists(expandedPath))
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: file not found."), expandedPath).ToUTF8());
                }
            }
        return expandedPath;
        }

    //---------------------------------------------------
    std::unique_ptr<GraphItems::Image>
    ReportBuilder::LoadImage(const wxSimpleJSON::Ptr_t& imageNode) const
        {
        const auto importNode = imageNode->GetProperty(L"image-import");
        const auto bmp = LoadImageFile(importNode);
        auto image = std::make_unique<GraphItems::Image>(bmp.ConvertToImage());
        if (image->IsOk())
            {
            // cache the import path for round-tripping
            if (importNode->IsValueString())
                {
                image->SetPropertyTemplate(L"image-import.path", importNode->AsString());
                }
            else if (importNode->IsOk())
                {
                const auto pathStr = importNode->GetProperty(L"path")->AsString();
                if (!pathStr.empty())
                    {
                    image->SetPropertyTemplate(L"image-import.path", pathStr);
                    }
                // cache multiple paths as tab-separated string
                const auto pathsArr = importNode->GetProperty(L"paths")->AsStrings();
                if (!pathsArr.empty())
                    {
                    wxString joined;
                    for (size_t i = 0; i < pathsArr.size(); ++i)
                        {
                        if (i > 0)
                            {
                            joined += L"\t";
                            }
                        joined += pathsArr[i];
                        }
                    image->SetPropertyTemplate(L"image-import.paths", joined);
                    }
                // cache stitch direction
                const auto stitchStr = importNode->GetProperty(L"stitch")->AsString();
                if (!stitchStr.empty())
                    {
                    image->SetPropertyTemplate(L"image-import.stitch", stitchStr);
                    }
                // cache effect
                const auto effectStr = importNode->GetProperty(L"effect")->AsString();
                if (!effectStr.empty())
                    {
                    image->SetPropertyTemplate(L"image-import.effect", effectStr);
                    }
                }

            wxSize sz{ 32, 32 };
            const auto sizeNode = imageNode->GetProperty(L"size");
            if (sizeNode->IsOk())
                {
                const bool hasWidth = sizeNode->HasProperty(L"width");
                const bool hasHeight = sizeNode->HasProperty(L"height");
                sz.x = sizeNode->GetProperty(L"width")->AsDouble(bmp.GetScaledWidth());
                sz.y = sizeNode->GetProperty(L"height")->AsDouble(bmp.GetScaledHeight());

                image->SetSize(GraphItems::Image::ToBestSize(bmp.GetSize(), sz));

                // cache original size values for round-tripping
                if (hasWidth)
                    {
                    image->SetPropertyTemplate(L"size.width", std::to_wstring(sz.x));
                    }
                if (hasHeight)
                    {
                    image->SetPropertyTemplate(L"size.height", std::to_wstring(sz.y));
                    }
                }

            const auto foundResize = ReportEnumConvert::ConvertResizeMethod(
                imageNode->GetProperty(L"resize-method")->AsString());
            if (foundResize.has_value())
                {
                image->SetResizeMethod(foundResize.value());
                }
            LoadItem(imageNode, *image);

            image->SetFixedWidthOnCanvas(true);
            return image;
            }
        return nullptr;
        }

    //---------------------------------------------------
    wxBitmap ReportBuilder::LoadImageFile(const wxSimpleJSON::Ptr_t& bmpNode) const
        {
        // if simply a file path, then load that and return
        if (bmpNode->IsValueString())
            {
            const auto path = NormalizeFilePath(bmpNode->AsString());
            return GraphItems::Image::LoadFile(path);
            }

        // otherwise, load as all images and apply effects to them
        std::vector<wxBitmap> bmps;
        auto paths = bmpNode->GetProperty(L"paths")->AsStrings();
        for (auto& path : paths)
            {
            path = NormalizeFilePath(path);
            bmps.emplace_back(GraphItems::Image::LoadFile(path));
            }

        // single image
        const auto path = bmpNode->GetProperty(L"path")->AsString();
        if (!path.empty())
            {
            bmps.emplace_back(GraphItems::Image::LoadFile(NormalizeFilePath(path)));
            }

        if (bmps.empty())
            {
            return wxNullBitmap;
            }

        wxBitmap bmp{ bmps[0] };

        if (bmps.size() > 1)
            {
            const auto stitch = bmpNode->GetProperty(L"stitch")->AsString();
            if (stitch.CmpNoCase(L"vertical") == 0)
                {
                bmp = GraphItems::Image::StitchVertically(bmps);
                }
            else
                {
                bmp = GraphItems::Image::StitchHorizontally(bmps);
                }
            }

        if (bmpNode->HasProperty(L"color-filter"))
            {
            auto color = ConvertColor(bmpNode->GetProperty(L"color-filter"));
            if (color.IsOk())
                {
                bmp = GraphItems::Image::CreateColorFilteredImage(bmp.ConvertToImage(), color);
                }
            }

        if (bmpNode->GetProperty(L"opacity")->IsValueNumber())
            {
            GraphItems::Image::SetOpacity(
                bmp, bmpNode->GetProperty(L"opacity")->AsDouble(wxALPHA_OPAQUE), false);
            }

        if (bmpNode->HasProperty(L"effect"))
            {
            const auto imgEffect =
                ReportEnumConvert::ConvertImageEffect(bmpNode->GetProperty(L"effect")->AsString());
            if (imgEffect)
                {
                bmp = GraphItems::Image::ApplyEffect(imgEffect.value(), bmp.ConvertToImage());
                }
            }

        return bmp;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
                                 GraphItems::GraphItemBase& item) const
        {
        if (!itemNode->IsOk())
            {
            return;
            }

        item.SetDPIScaleFactor(m_dpiScaleFactor);

        // ID
        item.SetId(itemNode->GetProperty(L"id")->AsDouble(wxID_ANY));

        // anchoring
        const auto foundAnchoring =
            ReportEnumConvert::ConvertAnchoring(itemNode->GetProperty(L"anchoring")->AsString());
        if (foundAnchoring.has_value())
            {
            item.SetAnchoring(foundAnchoring.value());
            }

        // outline
        const auto outlineFlagsNode = itemNode->GetProperty(L"outline");
        if (outlineFlagsNode->IsOk() && outlineFlagsNode->IsValueArray())
            {
            const auto outlineFlags = outlineFlagsNode->AsBools();
            item.GetGraphItemInfo().Outline((!outlineFlags.empty() ? outlineFlags[0] : false),
                                            (outlineFlags.size() > 1 ? outlineFlags[1] : false),
                                            (outlineFlags.size() > 2 ? outlineFlags[2] : false),
                                            (outlineFlags.size() > 3 ? outlineFlags[3] : false));
            }

        // child-alignment
        const auto childPlacement = itemNode->GetProperty(L"relative-alignment")->AsString();
        if (childPlacement.CmpNoCase(L"flush-left") == 0)
            {
            item.SetRelativeAlignment(RelativeAlignment::FlushLeft);
            }
        else if (childPlacement.CmpNoCase(L"flush-right") == 0)
            {
            item.SetRelativeAlignment(RelativeAlignment::FlushRight);
            }
        else if (childPlacement.CmpNoCase(L"flush-top") == 0)
            {
            item.SetRelativeAlignment(RelativeAlignment::FlushTop);
            }
        else if (childPlacement.CmpNoCase(L"flush-bottom") == 0)
            {
            item.SetRelativeAlignment(RelativeAlignment::FlushBottom);
            }
        else if (childPlacement.CmpNoCase(L"centered") == 0)
            {
            item.SetRelativeAlignment(RelativeAlignment::Centered);
            }

        // padding (going clockwise)
        const auto paddingSpec = itemNode->GetProperty(L"padding")->AsDoubles();
        if (!paddingSpec.empty())
            {
            item.SetTopPadding(paddingSpec.at(0));
            }
        if (paddingSpec.size() > 1)
            {
            item.SetRightPadding(paddingSpec.at(1));
            }
        if (paddingSpec.size() > 2)
            {
            item.SetBottomPadding(paddingSpec.at(2));
            }
        if (paddingSpec.size() > 3)
            {
            item.SetLeftPadding(paddingSpec.at(3));
            }

        // canvas padding (going clockwise)
        const auto canvasPaddingSpec = itemNode->GetProperty(L"canvas-margins")->AsDoubles();
        if (!canvasPaddingSpec.empty())
            {
            item.SetTopCanvasMargin(canvasPaddingSpec.at(0));
            }
        if (canvasPaddingSpec.size() > 1)
            {
            item.SetRightCanvasMargin(canvasPaddingSpec.at(1));
            }
        if (canvasPaddingSpec.size() > 2)
            {
            item.SetBottomCanvasMargin(canvasPaddingSpec.at(2));
            }
        if (canvasPaddingSpec.size() > 3)
            {
            item.SetLeftCanvasMargin(canvasPaddingSpec.at(3));
            }

        // horizontal page alignment
        const auto hPageAlignment = itemNode->GetProperty(L"horizontal-page-alignment")->AsString();
        if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
            {
            item.SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned);
            }
        else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
            {
            item.SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned);
            }
        else if (hPageAlignment.CmpNoCase(L"centered") == 0)
            {
            item.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            }

        // vertical page alignment
        const auto vPageAlignment = itemNode->GetProperty(L"vertical-page-alignment")->AsString();
        if (vPageAlignment.CmpNoCase(L"top-aligned") == 0)
            {
            item.SetPageVerticalAlignment(PageVerticalAlignment::TopAligned);
            }
        else if (vPageAlignment.CmpNoCase(L"bottom-aligned") == 0)
            {
            item.SetPageVerticalAlignment(PageVerticalAlignment::BottomAligned);
            }
        else if (vPageAlignment.CmpNoCase(L"centered") == 0)
            {
            item.SetPageVerticalAlignment(PageVerticalAlignment::Centered);
            }

        // should the item be shown
        item.Show(itemNode->GetProperty(L"show")->AsBool(true));

        item.SetScaling(itemNode->GetProperty(L"scaling")->AsDouble(1));

        LoadPen(itemNode->GetProperty(L"pen"), item.GetPen());

        item.SetFixedWidthOnCanvas(itemNode->GetProperty(L"fixed-width")->AsBool());
        item.FitCanvasRowHeightToContent(itemNode->GetProperty(L"fit-row-to-content")->AsBool());

        // accessibility
        if (const auto accessNode = itemNode->GetProperty(L"accessibility"); accessNode->IsOk())
            {
            wxSVGAttributes attrs;

            const auto ariaLabel =
                ExpandAndCache(&item, L"accessibility.aria-label",
                               accessNode->GetProperty(L"aria-label")->AsString());
            if (!ariaLabel.empty())
                {
                attrs.AriaLabel(ariaLabel);
                }

            const auto role = ExpandAndCache(&item, L"accessibility.role",
                                             accessNode->GetProperty(L"role")->AsString());
            if (!role.empty())
                {
                attrs.Role(role);
                }

            if (accessNode->GetProperty(L"aria-hidden")->AsBool())
                {
                attrs.AriaHidden();
                item.SetPropertyTemplate(L"accessibility.aria-hidden", L"true");
                }

            if (accessNode->GetProperty(L"auto")->AsBool())
                {
                item.SetAutoAccessibility(true);
                item.SetPropertyTemplate(L"accessibility.auto", L"true");
                }

            if (!attrs.IsEmpty())
                {
                item.GetAccessibilityAttributes() = std::move(attrs);
                }
            }
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::FindAxisPosition(const GraphItems::Axis& axis,
                                    const wxSimpleJSON::Ptr_t& positionNode) const
        {
        std::optional<double> axisPos;
        if (positionNode->IsOk() && positionNode->IsValueString())
            {
            // see if it's a date
            wxDateTime dt;
            if (dt.ParseDateTime(positionNode->AsString()) ||
                dt.ParseDate(positionNode->AsString()))
                {
                axisPos = axis.FindDatePosition(dt);
                // looks like a date, but couldn't be found on the axis,
                // so just show it as a string
                if (!axisPos.has_value())
                    {
                    axisPos = axis.FindCustomLabelPosition(positionNode->AsString());
                    }
                }
            else
                {
                axisPos = axis.FindCustomLabelPosition(ExpandConstants(positionNode->AsString()));
                }
            }
        else if (positionNode->IsOk() && positionNode->IsValueNumber())
            {
            axisPos = positionNode->AsDouble();
            }
        return axisPos;
        }
    } // namespace Wisteria
