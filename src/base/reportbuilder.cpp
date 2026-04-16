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
            wxLogError(json->GetLastError(), _(L"Configuration File Parsing Error"),
                       wxOK | wxICON_WARNING | wxCENTRE);
            return reportPages;
            }

        const auto reportNameNode = json->GetProperty(_DT(L"name"));
        if (reportNameNode->IsOk())
            {
            m_name = reportNameNode->AsString();
            }

        // print settings
        wxPrintData reportPrintSettings;
        const auto printNode = json->GetProperty(L"print");
        if (printNode->IsOk())
            {
            const auto orientation = printNode->GetProperty(L"orientation")->AsString();
            if (orientation.CmpNoCase(L"horizontal") == 0 ||
                orientation.CmpNoCase(L"landscape") == 0)
                {
                m_printOrientation = wxPrintOrientation::wxLANDSCAPE;
                reportPrintSettings.SetOrientation(wxPrintOrientation::wxLANDSCAPE);
                }
            else if (orientation.CmpNoCase(L"vertical") == 0 ||
                     orientation.CmpNoCase(L"portrait") == 0)
                {
                m_printOrientation = wxPrintOrientation::wxPORTRAIT;
                reportPrintSettings.SetOrientation(wxPrintOrientation::wxPORTRAIT);
                }

            const auto paperSize = ReportEnumConvert::ConvertPaperSize(
                printNode->GetProperty(L"paper-size")->AsString(L"paper-letter"));
            if (paperSize.has_value())
                {
                m_paperSize = paperSize.value();
                reportPrintSettings.SetPaperId(paperSize.value());
                }
            }

        const auto datasetsNode = json->GetProperty(L"datasets");
        try
            {
            LoadDatasets(datasetsNode);
            }
        catch (const std::exception& err)
            {
            wxLogError(wxString::FromUTF8(err.what()), _(L"Datasets Section Error"),
                       wxOK | wxICON_WARNING | wxCENTRE);
            return reportPages;
            }

        try
            {
            LoadConstants(json->GetProperty(L"constants"));
            }
        catch (const std::exception& err)
            {
            wxLogError(wxString::FromUTF8(err.what()), _(L"Constants Section Error"),
                       wxOK | wxICON_WARNING | wxCENTRE);
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

                    // page numbering
                    if (page->HasProperty(L"page-numbering"))
                        {
                        m_pageNumber = 1;
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
                        const auto bmp = LoadImageFile(page->GetProperty(L"background-image"));
                        if (bmp.IsOk())
                            {
                            canvas->SetBackgroundImage(wxBitmapBundle(bmp));
                            }
                        }

                    // copy print settings from report
                    canvas->GetPrinterSettings().SetOrientation(
                        reportPrintSettings.GetOrientation());

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
                                            wxLogError(
                                                wxString::FromUTF8(wxString::FromUTF8(err.what())),
                                                _(L"Canvas Item Error"),
                                                wxOK | wxICON_WARNING | wxCENTRE);
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
            auto [rangeStart, rangeEnd] = axis.GetRange();
            rangeStart = rangeNode->GetProperty(_DT(L"start"))->AsDouble(rangeStart);
            rangeEnd = rangeNode->GetProperty(_DT(L"end"))->AsDouble(rangeEnd);

            auto precision =
                rangeNode->GetProperty(_DT(L"precision"))->AsDouble(axis.GetPrecision());
            auto interval = rangeNode->GetProperty(_DT(L"interval"))->AsDouble(axis.GetInterval());
            auto displayInterval = rangeNode->GetProperty(_DT(L"display-interval"))
                                       ->AsDouble(axis.GetDisplayInterval());

            if (rangeStart > rangeEnd)
                {
                wxLogWarning(_("Invalid axis range (%f to %f) provided."), rangeStart, rangeEnd);
                }
            else
                {
                axis.SetRange(rangeStart, rangeEnd, precision, interval, displayInterval);
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
                    axis.SetCustomLabel(
                        customLabel->GetProperty(L"value")->AsDouble(),
                        *LoadLabel(customLabel->GetProperty(L"label"), GraphItems::Label{}));
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

            const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                labelNode->GetProperty(L"text-alignment")->AsString());
            if (textAlignment.has_value())
                {
                label->SetTextAlignment(textAlignment.value());
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
    void ReportBuilder::LoadConstants(const wxSimpleJSON::Ptr_t& constantsNode)
        {
        if (constantsNode->IsOk())
            {
            const auto values = constantsNode->AsNodes();
            for (const auto& value : values)
                {
                if (value->IsOk())
                    {
                    const wxString vName = value->GetProperty(_DT(L"name"))->AsString();
                    m_values.insert_or_assign(
                        vName, value->GetProperty(L"value")->IsValueString() ?
                                   ValuesType(value->GetProperty(L"value")->AsString()) :
                                   ValuesType(value->GetProperty(L"value")->AsDouble()));

                    DatasetFormulaInfo constInfo;
                    constInfo.m_name = vName;
                    if (value->GetProperty(L"value")->IsValueString())
                        {
                        constInfo.m_value = value->GetProperty(L"value")->AsString();
                        }
                    else if (value->GetProperty(L"value")->IsValueNumber())
                        {
                        constInfo.m_value =
                            std::to_wstring(value->GetProperty(L"value")->AsDouble());
                        }
                    m_constants.insert(std::move(constInfo));
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::RecalcFormula(const wxString& formulaName, const wxString& formulaValue,
                                      const wxString& datasetName)
        {
        const auto dsIt = m_datasets.find(datasetName);
        if (dsIt == m_datasets.cend() || dsIt->second == nullptr)
            {
            return;
            }
        m_values.insert_or_assign(formulaName, CalcFormula(formulaValue, dsIt->second));
        }

    //---------------------------------------------------
    std::optional<std::vector<wxString>>
    ReportBuilder::ExpandColumnSelections(wxString var,
                                          const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (var.starts_with(L"{{") && var.ends_with(L"}}"))
            {
            var = var.substr(2, var.length() - 4);
            }
        else
            {
            return std::nullopt;
            }
        const wxRegEx re(FunctionStartRegEx() + L"(everything|everythingexcept|matches)" +
                         OpeningParenthesisRegEx() + ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(var))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount < 2)
                {
                return std::nullopt;
                }

            const wxString funcName = re.GetMatch(var, 1).MakeLower();
            std::vector<wxString> columns;

            if (funcName.CmpNoCase(L"everything") == 0)
                {
                if (!dataset->GetIdColumn().GetName().empty())
                    {
                    columns.push_back(dataset->GetIdColumn().GetName());
                    }
                const auto catCols{ dataset->GetCategoricalColumnNames() };
                const auto contCols{ dataset->GetContinuousColumnNames() };
                const auto dateCols{ dataset->GetDateColumnNames() };
                std::ranges::copy(catCols, std::back_inserter(columns));
                std::ranges::copy(contCols, std::back_inserter(columns));
                std::ranges::copy(dateCols, std::back_inserter(columns));

                return columns;
                }
            if (paramPartsCount >= 3)
                {
                const wxString columnPattern =
                    ConvertColumnOrGroupParameter(re.GetMatch(var, 2), dataset);

                if (funcName.CmpNoCase(L"matches") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that contain the string
                        if (columnRE.Matches(dataset->GetIdColumn().GetName()))
                            {
                            columns.push_back(dataset->GetIdColumn().GetName());
                            }
                        for (const auto& col : dataset->GetCategoricalColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetContinuousColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetDateColumns())
                            {
                            if (columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        return columns;
                        }
                    return std::nullopt;
                    }
                if (funcName.CmpNoCase(L"everythingexcept") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that DON'T contain the string
                        if (!dataset->GetIdColumn().GetName().empty() &&
                            !columnRE.Matches(dataset->GetIdColumn().GetName()))
                            {
                            columns.push_back(dataset->GetIdColumn().GetName());
                            }
                        for (const auto& col : dataset->GetCategoricalColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetContinuousColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        for (const auto& col : dataset->GetDateColumns())
                            {
                            if (!columnRE.Matches(col.GetName()))
                                {
                                columns.push_back(col.GetName());
                                }
                            }
                        return columns;
                        }
                    return std::nullopt;
                    }

                return std::nullopt;
                }

            return std::nullopt;
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    void ReportBuilder::CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
                                     const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (formulasNode->IsOk())
            {
            const auto formulas = formulasNode->AsNodes();
            for (const auto& formula : formulas)
                {
                if (formula->IsOk())
                    {
                    const wxString vName = formula->GetProperty(_DT(L"name"))->AsString();
                    if (formula->GetProperty(L"value")->IsValueString())
                        {
                        m_values.insert_or_assign(
                            vName,
                            CalcFormula(formula->GetProperty(L"value")->AsString(), dataset));
                        }
                    else if (formula->GetProperty(L"value")->IsValueNumber())
                        {
                        m_values.insert_or_assign(vName,
                                                  formula->GetProperty(L"value")->AsDouble());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType
    ReportBuilder::CalcFormula(const wxString& formula,
                               const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        const wxRegEx re(
            FunctionStartRegEx() +
            LR"((min|max|median|n|total|grandtotal|groupcount|grouppercentdecimal|grouppercent|continuouscolumn|now|pagenumber|reportname|add))" +
            OpeningParenthesisRegEx());
        if (re.Matches(formula))
            {
            const wxString funcName = re.GetMatch(formula, 1).MakeLower();
            if (funcName.CmpNoCase(L"min") == 0 || funcName.CmpNoCase(L"max") == 0)
                {
                return CalcMinMax(formula, dataset);
                }
            if (funcName.CmpNoCase(L"median") == 0)
                {
                const auto calcValue = CalcMedian(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"n") == 0)
                {
                const auto calcValue = CalcValidN(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"total") == 0)
                {
                const auto calcValue = CalcTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grandtotal") == 0)
                {
                const auto calcValue = CalcGrandTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"groupcount") == 0)
                {
                const auto calcValue = CalcGroupCount(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grouppercentdecimal") == 0)
                {
                const auto calcValue = CalcGroupPercentDecimal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"grouppercent") == 0)
                {
                const auto calcValue = CalcGroupPercent(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            if (funcName.CmpNoCase(L"continuouscolumn") == 0)
                {
                return ExpandColumnSelection(formula, dataset);
                }
            if (funcName.CmpNoCase(L"now") == 0)
                {
                return CalcNow(formula);
                }
            if (funcName.CmpNoCase(L"add") == 0)
                {
                return CalcAdd(formula);
                }
            if (funcName.CmpNoCase(L"pagenumber") == 0)
                {
                return FormatPageNumber(formula);
                }
            if (funcName.CmpNoCase(L"reportname") == 0)
                {
                return m_name;
                }
            }
        // note that formula may just be a constant (e.g., color string),
        // so return that if it can't be calculated into something else
        return formula;
        }

    //---------------------------------------------------
    wxString
    ReportBuilder::ExpandColumnSelection(const wxString& formula,
                                         const std::shared_ptr<const Data::Dataset>& dataset)
        {
        const wxRegEx re(FunctionStartRegEx() + L"(continuouscolumn)" + OpeningParenthesisRegEx() +
                         NumberOrStringRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            if (dataset->GetContinuousColumns().empty())
                {
                throw std::runtime_error(
                    wxString(_(L"ContinuousColumn() failed. "
                               "There are no continuous columns in the dataset."))
                        .ToUTF8());
                }
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                unsigned long columnIndex{ 0 };
                wxString columnIndexStr = re.GetMatch(formula, 2);
                if (columnIndexStr.starts_with(L"`") && columnIndexStr.ends_with(L"`"))
                    {
                    columnIndexStr = columnIndexStr.substr(1, columnIndexStr.length() - 2);
                    if (columnIndexStr.CmpNoCase(L"last") == 0)
                        {
                        columnIndex = dataset->GetContinuousColumns().size() - 1;
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(
                                _(L"'%s': unknown constant for continuous column index."),
                                columnIndexStr)
                                .ToUTF8());
                        }
                    }
                else if (columnIndexStr.ToULong(&columnIndex))
                    {
                    if (columnIndex >= dataset->GetContinuousColumns().size())
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%lu: invalid continuous column index."),
                                             columnIndex)
                                .ToUTF8());
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': unable to convert value for continuous column index."),
                            columnIndexStr)
                            .ToUTF8());
                    }
                return dataset->GetContinuousColumn(columnIndex).GetName();
                }
            }

        // can't get the name of the column, just return the original text
        return formula;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGroupCount(const wxString& formula,
                                  const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(groupcount)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 4)
                {
                const wxString groupName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 3), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }

                return dataset->GetCategoricalColumnValidN(groupName, groupName, groupID);
                }
            // dataset or something missing
            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupPercentDecimal(
        const wxString& formula, const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() + L"(grouppercentdecimal)" +
                         OpeningParenthesisRegEx() + ColumnNameOrFormulaRegEx() +
                         ParamSeparatorRegEx() + ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercentdecimal)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"groupcount");
                if (const auto groupTotal = CalcGroupCount(countFormula, dataset))
                    {
                    return safe_divide<double>(groupTotal.value(), dataset->GetRowCount());
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGroupPercent(const wxString& formula,
                                    const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() + L"(grouppercent)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            if (const wxRegEx reFunctionRename(L"(?i)(grouppercent)");
                reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"grouppercentdecimal");
                if (const auto percDec = CalcGroupPercentDecimal(countFormula, dataset))
                    {
                    return wxRound(percDec.value() * 100);
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcMedian(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() + L"(median)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() + L"(median)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (reSimple.Matches(formula))
            {
            const auto paramPartsCount = reSimple.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reSimple.GetMatch(formula, 2), dataset);
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousMedian(columnName);
                    }
                }
            // dataset or column name missing
            else
                {
                return std::nullopt;
                }
            }
        else if (reExtended.Matches(formula))
            {
            const auto paramPartsCount = reExtended.GetMatchCount();
            if (paramPartsCount >= 5)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 2), dataset);
                const wxString groupName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 3), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 4), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousMedian(columnName, groupName, groupID);
                    }
                }
            // dataset or something missing
            else
                {
                return std::nullopt;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcValidN(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() + L"(n)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() + L"(n)" + OpeningParenthesisRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                                 ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (reSimple.Matches(formula))
            {
            const auto paramPartsCount = reSimple.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reSimple.GetMatch(formula, 2), dataset);
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName);
                    }
                }
            // dataset or column name missing
            else
                {
                return std::nullopt;
                }
            }
        else if (reExtended.Matches(formula))
            {
            const auto paramPartsCount = reExtended.GetMatchCount();
            if (paramPartsCount >= 5)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 2), dataset);
                const wxString groupName =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 3), dataset);
                // if the group value is an embedded formula, then calculate it
                const wxString groupValue =
                    ConvertColumnOrGroupParameter(reExtended.GetMatch(formula, 4), dataset);
                // get the group column and the numeric code for the value
                const auto groupColumn = dataset->GetCategoricalColumn(groupName);
                if (groupColumn == dataset->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: group column not found."), groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."), groupValue).ToUTF8());
                    }
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName, groupName, groupID);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName, groupName, groupID);
                    }
                }
            // dataset or something missing
            else
                {
                return std::nullopt;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ConvertColumnOrGroupParameter(
        wxString columnStr, const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (columnStr.starts_with(L"`") && columnStr.ends_with(L"`"))
            {
            columnStr = columnStr.substr(1, columnStr.length() - 2);
            }
        else if (columnStr.starts_with(L"{{") && columnStr.ends_with(L"}}"))
            {
            columnStr = columnStr.substr(2, columnStr.length() - 4);
            columnStr = ExpandConstants(columnStr);
            const auto calcStr = CalcFormula(columnStr, dataset);
            if (const auto* const strVal{ std::get_if<wxString>(&calcStr) }; strVal != nullptr)
                {
                return *strVal;
                }

            return {};
            }

        return columnStr;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcGrandTotal(const wxString& formula,
                                  const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(grandtotal)" + OpeningParenthesisRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                // only continuous can be totaled
                double total{ 0 };
                for (size_t i = 0; i < dataset->GetContinuousColumns().size(); ++i)
                    {
                    total += dataset->GetContinuousTotal(i);
                    }
                return total;
                }
            // dataset or column name missing

            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double>
    ReportBuilder::CalcTotal(const wxString& formula,
                             const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(total)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);
                // only continuous can be totaled
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousTotal(columnName);
                    }

                throw std::runtime_error(
                    wxString::Format(_(L"%s: column must be continuous when totaling."), columnName)
                        .ToUTF8());
                }
            // dataset or column name missing

            return std::nullopt;
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType
    ReportBuilder::CalcMinMax(const wxString& formula,
                              const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: invalid dataset when calculating formula."), formula)
                    .ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() + L"(min|max)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnName =
                    ConvertColumnOrGroupParameter(re.GetMatch(formula, 2), dataset);

                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    const auto [minVal, maxVal] = dataset->GetCategoricalMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    const auto [minVal, maxVal] = dataset->GetContinuousMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                wxLogWarning(L"'%s' column not found in call to MIN or MAX. "
                             "A continuous or categorical column was expected.",
                             columnName);
                }
            // dataset or column name missing
            else
                {
                return formula;
                }
            }
        return formula;
        }

    //---------------------------------------------------
    wxString ReportBuilder::FormatPageNumber(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() + L"(pagenumber)" + OpeningParenthesisRegEx() +
                         StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        return re.Matches(formula) ?
                   wxNumberFormatter::ToString(m_pageNumber, 0,
                                               wxNumberFormatter::Style::Style_WithThousandsSep) :
                   wxString{};
        }

    //---------------------------------------------------
    wxString ReportBuilder::CalcAdd(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() + L"(add)" + OpeningParenthesisRegEx() +
                         NumberOrStringRegEx() + ParamSeparatorRegEx() + NumberOrStringRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            if (re.GetMatchCount() >= 3)
                {
                wxString firstValue = re.GetMatch(formula, 2);
                if (firstValue.starts_with(L"`") && firstValue.ends_with(L"`"))
                    {
                    firstValue = ExpandConstants(firstValue.substr(1, firstValue.length() - 2));
                    }
                wxString secondValue = re.GetMatch(formula, 3);
                if (secondValue.starts_with(L"`") && secondValue.ends_with(L"`"))
                    {
                    secondValue = ExpandConstants(secondValue.substr(1, secondValue.length() - 2));
                    }
                double secondDouble{ 0 };
                if (secondValue.ToCDouble(&secondDouble))
                    {
                    const auto firstNumber = firstValue.find_first_of(L"0123456789");
                    if (firstNumber == wxString::npos)
                        {
                        return {};
                        }
                    const auto endOfNumber =
                        firstValue.find_first_not_of(L"0123456789.", firstNumber);
                    const wxString prefix = firstValue.substr(0, firstNumber);
                    double firstDouble{ 0 };
                    if (firstValue
                            .substr(firstNumber, endOfNumber == wxString::npos ?
                                                     wxString::npos :
                                                     endOfNumber - firstNumber)
                            .ToCDouble(&firstDouble))
                        {
                        return prefix + wxNumberFormatter::ToString(
                                            (firstDouble + secondDouble), 2,
                                            wxNumberFormatter::Style::Style_NoTrailingZeroes);
                        }
                    }
                }
            }
        return {};
        }

    //---------------------------------------------------
    wxString ReportBuilder::CalcNow(const wxString& formula)
        {
        const wxRegEx re(FunctionStartRegEx() + L"(now)" + OpeningParenthesisRegEx() +
                         StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                wxString paramValue = re.GetMatch(formula, 2).MakeLower();
                if (paramValue.starts_with(L"`") && paramValue.ends_with(L"`"))
                    {
                    paramValue = paramValue.substr(1, paramValue.length() - 2);
                    }

                if (paramValue.empty())
                    {
                    return wxDateTime::Now().FormatDate();
                    }
                if (paramValue == L"fancy")
                    {
                    return wxDateTime::Now().Format(L"%B %d, %Y");
                    }
                // which part of the date is requested?
                if (paramValue == L"year")
                    {
                    return std::to_wstring(wxDateTime::Now().GetYear());
                    }
                if (paramValue == L"month")
                    {
                    return std::to_wstring(wxDateTime::Now().GetMonth());
                    }
                if (paramValue == L"monthname")
                    {
                    return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth());
                    }
                if (paramValue == L"monthshortname")
                    {
                    return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth(),
                                                    wxDateTime::Name_Abbr);
                    }
                if (paramValue == L"day")
                    {
                    return std::to_wstring(wxDateTime::Now().GetDay());
                    }
                if (paramValue == L"dayname")
                    {
                    return wxDateTime::GetWeekDayName(wxDateTime::Now().GetWeekDay());
                    }

                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown parameter passed to Now()."), paramValue)
                        .ToUTF8());
                }
            // no param, just return full date
            return wxDateTime::Now().Format();
            }
        return wxDateTime::Now().Format();
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
                    info.m_values.push_back(std::to_wstring(filterValue->AsDouble()));
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
                    cFilter.m_values.push_back(
                        isDate ? Data::DatasetValueType(dt) :
                        filterValue->IsValueString() ?
                                 Data::DatasetValueType(ExpandConstants(filterValue->AsString())) :
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
                            formulaOpt.m_value =
                                std::to_wstring(formula->GetProperty(L"value")->AsDouble());
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
                    if (!wxFileName::FileExists(path))
                        {
                        path = wxFileName{ m_configFilePath }.GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            throw std::runtime_error(
                                wxString::Format(_(L"'%s': dataset not found."), path).ToUTF8());
                            }
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
                    // date columns
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
                    // if no columns are defined, then deduce them ourselves
                    Data::Dataset::ColumnPreviewInfo columnPreviewInfo;
                    if (!datasetNode->HasProperty(L"id-column") &&
                        !datasetNode->HasProperty(L"date-columns") &&
                        !datasetNode->HasProperty(L"continuous-columns") &&
                        !datasetNode->HasProperty(L"categorical-columns"))
                        {
                        columnPreviewInfo = Data::Dataset::ReadColumnInfo(path, importDefines,
                                                                          std::nullopt, worksheet);
                        importDefines = Data::Dataset::ImportInfoFromPreview(columnPreviewInfo);
                        fillImportDefines();
                        }
                    else
                        {
                        importDefines
                            .ContinuousMDRecodeValue(
                                datasetNode->GetProperty(L"continuous-md-recode-value")
                                    ->AsDouble(std::numeric_limits<double>::quiet_NaN()))
                            .IdColumn(idColumn)
                            .DateColumns(dateInfo)
                            .ContinuousColumns(continuousVars)
                            .CategoricalColumns(catInfo);

                        // build ColumnPreviewInfo from explicit column definitions
                        // (mark as user-overridden since the user specified
                        //  these types in the JSON)
                        if (!idColumn.empty())
                            {
                            columnPreviewInfo.emplace_back(idColumn,
                                                           Data::Dataset::ColumnImportType::String,
                                                           wxString{}, false, true);
                            }
                        for (const auto& di : dateInfo)
                            {
                            columnPreviewInfo.emplace_back(di.m_columnName,
                                                           Data::Dataset::ColumnImportType::Date,
                                                           wxString{}, false, true);
                            }
                        for (const auto& cv : continuousVars)
                            {
                            columnPreviewInfo.emplace_back(cv,
                                                           Data::Dataset::ColumnImportType::Numeric,
                                                           wxString{}, false, true);
                            }
                        for (const auto& ci : catInfo)
                            {
                            columnPreviewInfo.emplace_back(
                                ci.m_columnName,
                                (ci.m_importMethod ==
                                         Data::CategoricalImportMethod::ReadAsIntegers ?
                                     Data::Dataset::ColumnImportType::Discrete :
                                     Data::Dataset::ColumnImportType::String),
                                wxString{}, false, true);
                            }
                        }

                    // reorder ColumnPreviewInfo to match original data file order
                    auto colOrder = datasetNode->GetProperty(L"columns-order")->AsStrings();
                    // if no explicit order, read from the file itself
                    if (colOrder.empty())
                        {
                        Data::ImportInfo headerInfo;
                        headerInfo.SkipRows(importDefines.GetSkipRows());
                        const auto fileColumnInfo =
                            Data::Dataset::ReadColumnInfo(path, headerInfo, 1, worksheet);
                        colOrder.reserve(fileColumnInfo.size());
                        for (const auto& fc : fileColumnInfo)
                            {
                            colOrder.push_back(fc.m_name);
                            }
                        }
                    if (!colOrder.empty())
                        {
                        Data::Dataset::ColumnPreviewInfo reordered;
                        for (const auto& name : colOrder)
                            {
                            const auto it =
                                std::find_if(columnPreviewInfo.cbegin(), columnPreviewInfo.cend(),
                                             [&name](const auto& cp) { return cp.m_name == name; });
                            if (it != columnPreviewInfo.cend())
                                {
                                reordered.push_back(*it);
                                }
                            }
                        // append any columns not in the order list
                        // (shouldn't happen, but be safe)
                        for (const auto& cp : columnPreviewInfo)
                            {
                            const auto it = std::find_if(reordered.cbegin(), reordered.cend(),
                                                         [&cp](const auto& r)
                                                         { return r.m_name == cp.m_name; });
                            if (it == reordered.cend())
                                {
                                reordered.push_back(cp);
                                }
                            }
                        columnPreviewInfo = std::move(reordered);
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
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadProConRoadmap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Pro & Con Roadmap."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto pcRoadmap = std::make_shared<Graphs::ProConRoadmap>(canvas);
            pcRoadmap->SetData(
                foundPos->second, variablesNode->GetProperty(L"positive")->AsString(),
                (variablesNode->HasProperty(L"positive-aggregate") ?
                     std::optional<wxString>(ExpandAndCache(
                         pcRoadmap.get(), L"variables.positive-aggregate",
                         variablesNode->GetProperty(L"positive-aggregate")->AsString())) :
                     std::nullopt),
                variablesNode->GetProperty(L"negative")->AsString(),
                (variablesNode->HasProperty(L"negative-aggregate") ?
                     std::optional<wxString>(ExpandAndCache(
                         pcRoadmap.get(), L"variables.negative-aggregate",
                         variablesNode->GetProperty(L"negative-aggregate")->AsString())) :
                     std::nullopt),
                (graphNode->GetProperty(L"minimum-count")->IsValueNumber() ?
                     std::optional<double>(graphNode->GetProperty(L"minimum-count")->AsDouble(1)) :
                     std::nullopt));

            if (graphNode->GetProperty(L"positive-legend-label")->IsValueString())
                {
                pcRoadmap->SetPositiveLegendLabel(
                    graphNode->GetProperty(L"positive-legend-label")->AsString());
                }
            if (graphNode->GetProperty(L"negative-legend-label")->IsValueString())
                {
                pcRoadmap->SetNegativeLegendLabel(
                    graphNode->GetProperty(L"negative-legend-label")->AsString());
                }

            LoadPen(graphNode->GetProperty(L"road-pen"), pcRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"),
                    pcRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                pcRoadmap->SetLabelPlacement(labelPlacement.value());
                }

            const auto laneSepStyle = ReportEnumConvert::ConvertLaneSeparatorStyle(
                graphNode->GetProperty(L"lane-separator-style")->AsString());
            if (laneSepStyle.has_value())
                {
                pcRoadmap->SetLaneSeparatorStyle(laneSepStyle.value());
                }

            const auto roadStopTheme = ReportEnumConvert::ConvertRoadStopTheme(
                graphNode->GetProperty(L"road-stop-theme")->AsString());
            if (roadStopTheme.has_value())
                {
                pcRoadmap->SetRoadStopTheme(roadStopTheme.value());
                }

            const auto markerLabelDisplay = ReportEnumConvert::ConvertMarkerLabelDisplay(
                graphNode->GetProperty(L"marker-label-display")->AsString());
            if (markerLabelDisplay.has_value())
                {
                pcRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value());
                }

            if (graphNode->GetProperty(L"default-caption")->AsBool())
                {
                pcRoadmap->AddDefaultCaption();
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pcRoadmap);
            return pcRoadmap;
            }

        throw std::runtime_error(_(L"Variables not defined for Pro & Con Roadmap.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadGanttChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Gantt chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto dateInterval = ReportEnumConvert::ConvertDateInterval(
                graphNode->GetProperty(L"date-interval")->AsString());
            const auto fyType = ReportEnumConvert::ConvertFiscalYear(
                graphNode->GetProperty(L"fy-type")->AsString());

            auto ganttChart = std::make_shared<Graphs::GanttChart>(canvas);
            ganttChart->SetData(
                foundPos->second,
                dateInterval.has_value() ? dateInterval.value() : DateInterval::FiscalQuarterly,
                fyType.has_value() ? fyType.value() : FiscalYear::USBusiness,
                ExpandAndCache(ganttChart.get(), L"variables.task",
                               variablesNode->GetProperty(L"task")->AsString()),
                ExpandAndCache(ganttChart.get(), L"variables.start-date",
                               variablesNode->GetProperty(L"start-date")->AsString()),
                ExpandAndCache(ganttChart.get(), L"variables.end-date",
                               variablesNode->GetProperty(L"end-date")->AsString()),
                (variablesNode->HasProperty(L"resource") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.resource",
                                        variablesNode->GetProperty(L"resource")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"description") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.description",
                                        variablesNode->GetProperty(L"description")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"completion") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.completion",
                                        variablesNode->GetProperty(L"completion")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"group") ?
                     std::optional<wxString>(
                         ExpandAndCache(ganttChart.get(), L"variables.group",
                                        variablesNode->GetProperty(L"group")->AsString())) :
                     std::nullopt));

            const auto taskLabelDisplay = ReportEnumConvert::ConvertTaskLabelDisplay(
                graphNode->GetProperty(L"task-label-display")->AsString());
            if (taskLabelDisplay.has_value())
                {
                ganttChart->SetLabelDisplay(taskLabelDisplay.value());
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, ganttChart);
            return ganttChart;
            }

        throw std::runtime_error(_(L"Variables not defined for Gantt chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLRRoadmap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Linear Regression Roadmap."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            // lrRoadmap created below; cache after creation
            const auto pValueColumnRaw = variablesNode->GetProperty(L"p-value")->AsString();
            const auto pValueColumn = ExpandConstants(pValueColumnRaw);
            const auto dvNameRaw =
                variablesNode->GetProperty(L"dependent-variable-name")->AsString();
            const auto dvName = ExpandConstants(dvNameRaw);

            int lrPredictors{ 0 };
            if (graphNode->HasProperty(L"predictors-to-include"))
                {
                const auto preds = graphNode->GetProperty(L"predictors-to-include")->AsStrings();
                for (const auto& pred : preds)
                    {
                    if (pred.CmpNoCase(_DT(L"positive")) == 0)
                        {
                        lrPredictors |= Influence::InfluencePositive;
                        }
                    else if (pred.CmpNoCase(_DT(L"negative")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceNegative;
                        }
                    else if (pred.CmpNoCase(_DT(L"neutral")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceNeutral;
                        }
                    else if (pred.CmpNoCase(_DT(L"all")) == 0)
                        {
                        lrPredictors |= Influence::InfluenceAll;
                        }
                    }
                }

            auto lrRoadmap = std::make_shared<Graphs::LRRoadmap>(canvas);
            // cache templates for variables expanded before lrRoadmap was created
            if (!pValueColumnRaw.empty())
                {
                lrRoadmap->SetPropertyTemplate(L"variables.p-value", pValueColumnRaw);
                }
            if (!dvNameRaw.empty())
                {
                lrRoadmap->SetPropertyTemplate(L"variables.dependent-variable-name", dvNameRaw);
                }
            lrRoadmap->SetData(
                foundPos->second,
                ExpandAndCache(lrRoadmap.get(), L"variables.predictor",
                               variablesNode->GetProperty(L"predictor")->AsString()),
                ExpandAndCache(lrRoadmap.get(), L"variables.coefficient",
                               variablesNode->GetProperty(L"coefficient")->AsString()),
                (!pValueColumn.empty() ? std::optional<wxString>(pValueColumn) : std::nullopt),
                (graphNode->GetProperty(L"p-value-threshold")->IsValueNumber() ?
                     std::optional<double>(
                         graphNode->GetProperty(L"p-value-threshold")->AsDouble(0.05)) :
                     std::nullopt),
                (lrPredictors == 0 ?
                     std::nullopt :
                     std::optional<Influence>(static_cast<Influence>(lrPredictors))),
                (!dvName.empty() ? std::optional<wxString>(dvName) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"road-pen"), lrRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"),
                    lrRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                lrRoadmap->SetLabelPlacement(labelPlacement.value());
                }

            const auto laneSepStyle = ReportEnumConvert::ConvertLaneSeparatorStyle(
                graphNode->GetProperty(L"lane-separator-style")->AsString());
            if (laneSepStyle.has_value())
                {
                lrRoadmap->SetLaneSeparatorStyle(laneSepStyle.value());
                }

            const auto roadStopTheme = ReportEnumConvert::ConvertRoadStopTheme(
                graphNode->GetProperty(L"road-stop-theme")->AsString());
            if (roadStopTheme.has_value())
                {
                lrRoadmap->SetRoadStopTheme(roadStopTheme.value());
                }

            const auto markerLabelDisplay = ReportEnumConvert::ConvertMarkerLabelDisplay(
                graphNode->GetProperty(L"marker-label-display")->AsString());
            if (markerLabelDisplay.has_value())
                {
                lrRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value());
                }

            if (graphNode->GetProperty(L"default-caption")->AsBool())
                {
                lrRoadmap->AddDefaultCaption();
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, lrRoadmap);
            return lrRoadmap;
            }

        throw std::runtime_error(
            _(L"Variables not defined for Linear Regression Roadmap.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLikertChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Likert chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            std::vector<wxString> questions;
            const auto questionVarsRaw = variablesNode->GetProperty(L"questions")->AsStrings();
            const auto questionVars = ExpandConstants(questionVarsRaw);
            for (const auto& questionVar : questionVars)
                {
                if (auto convertedVars = ExpandColumnSelections(questionVar, foundPos->second))
                    {
                    questions.insert(questions.cend(), convertedVars.value().cbegin(),
                                     convertedVars.value().cend());
                    }
                else
                    {
                    questions.push_back(questionVar);
                    }
                }
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            // get the survey format
            auto surveyFormat = ReportEnumConvert::ConvertLikertSurveyQuestionFormat(
                graphNode->GetProperty(L"survey-format")->AsString());
            if (!surveyFormat.has_value())
                {
                surveyFormat =
                    Graphs::LikertChart::DeduceScale(foundPos->second, questions, groupVarName);
                }

            const bool doSimplify = graphNode->GetProperty(L"simplify")->AsBool();
            if (doSimplify)
                {
                surveyFormat = Graphs::LikertChart::Simplify(foundPos->second, questions,
                                                             surveyFormat.value());
                }

            const bool doApplyLabels = graphNode->GetProperty(L"apply-default-labels")->AsBool();
            if (doApplyLabels)
                {
                Graphs::LikertChart::SetLabels(
                    foundPos->second, questions,
                    Graphs::LikertChart::CreateLabels(surveyFormat.value()));
                }

            auto likertChart = std::make_shared<Graphs::LikertChart>(
                canvas, surveyFormat.value(),
                ConvertColor(graphNode->GetProperty(L"negative-color")),
                ConvertColor(graphNode->GetProperty(L"positive-color")),
                ConvertColor(graphNode->GetProperty(L"neutral-color")),
                ConvertColor(graphNode->GetProperty(L"no-response-color")));

            // cache templates for variables expanded before likertChart was created
            for (size_t i = 0; i < questionVarsRaw.size(); ++i)
                {
                if (!questionVarsRaw[i].empty())
                    {
                    likertChart->SetPropertyTemplate(
                        L"variables.questions[" + std::to_wstring(i) + L"]", questionVarsRaw[i]);
                    }
                }
            if (!groupVarNameRaw.empty())
                {
                likertChart->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            if (doSimplify)
                {
                likertChart->SetPropertyTemplate(L"simplify", L"true");
                }
            if (doApplyLabels)
                {
                likertChart->SetPropertyTemplate(L"apply-default-labels", L"true");
                }

            likertChart->SetData(
                foundPos->second, questions,
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            likertChart->ShowResponseCounts(
                graphNode->GetProperty(L"show-response-counts")->AsBool(false));
            likertChart->ShowPercentages(graphNode->GetProperty(L"show-percentages")->AsBool(true));
            likertChart->ShowSectionHeaders(
                graphNode->GetProperty(L"show-section-headers")->AsBool(true));
            likertChart->SetBarSizesToRespondentSize(
                graphNode->GetProperty(L"adjust-bar-widths-to-respondent-size")->AsBool(false));

            if (graphNode->HasProperty(L"positive-label"))
                {
                likertChart->SetPositiveHeader(
                    graphNode->GetProperty(L"positive-label")->AsString());
                }
            if (graphNode->HasProperty(L"negative-label"))
                {
                likertChart->SetNegativeHeader(
                    graphNode->GetProperty(L"negative-label")->AsString());
                }
            if (graphNode->HasProperty(L"no-response-label"))
                {
                likertChart->SetNoResponseHeader(
                    graphNode->GetProperty(L"no-response-label")->AsString());
                }

            const auto questionBracketNodes =
                graphNode->GetProperty(L"question-brackets")->AsNodes();
            for (const auto& questionBracketNode : questionBracketNodes)
                {
                likertChart->AddQuestionsBracket(
                    { questionBracketNode->GetProperty(L"start")->AsString(),
                      questionBracketNode->GetProperty(L"end")->AsString(),
                      questionBracketNode->GetProperty(L"title")->AsString() });
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, likertChart);
            return likertChart;
            }

        throw std::runtime_error(_(L"Variables not defined for Likert chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWCurvePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for W-curve plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto wcurvePlot = std::make_shared<Graphs::WCurvePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                wcurvePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            wcurvePlot->SetData(
                foundPos->second,
                ExpandAndCache(wcurvePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(wcurvePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));
            if (graphNode->HasProperty(L"time-interval-label"))
                {
                wcurvePlot->SetTimeIntervalLabel(
                    graphNode->GetProperty(L"time-interval-label")->AsString());
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, wcurvePlot);
            LoadLinePlotBaseOptions(graphNode, wcurvePlot.get());

            return wcurvePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for W-curve plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadCandlestickPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                       size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for candlestick plot."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto candlestickPlot = std::make_shared<Graphs::CandlestickPlot>(canvas);
            candlestickPlot->SetData(
                foundPos->second,
                ExpandAndCache(candlestickPlot.get(), L"variables.date",
                               variablesNode->GetProperty(L"date")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.open",
                               variablesNode->GetProperty(L"open")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.high",
                               variablesNode->GetProperty(L"high")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.low",
                               variablesNode->GetProperty(L"low")->AsString()),
                ExpandAndCache(candlestickPlot.get(), L"variables.close",
                               variablesNode->GetProperty(L"close")->AsString()));

            const auto plotType = ReportEnumConvert::ConvertCandlestickPlotType(
                graphNode->GetProperty(L"plot-type")->AsString());
            if (plotType.has_value())
                {
                candlestickPlot->SetPlotType(plotType.value());
                }

            LoadBrush(graphNode->GetProperty(L"gain-brush"), candlestickPlot->GetGainBrush());
            LoadBrush(graphNode->GetProperty(L"loss-brush"), candlestickPlot->GetLossBrush());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, candlestickPlot);
            return candlestickPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for candlestick plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadLinePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for line plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto linePlot = std::make_shared<Graphs::LinePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                linePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            linePlot->SetData(
                foundPos->second,
                ExpandAndCache(linePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(linePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            LoadLinePlotBaseOptions(graphNode, linePlot.get());

            return linePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for line plot.").ToUTF8());
        }

    //---------------------------------------------------
    void ReportBuilder::LoadLinePlotBaseOptions(const wxSimpleJSON::Ptr_t& graphNode,
                                                Graphs::LinePlot* linePlot) const
        {
        // showcasing
        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            linePlot->SetGhostOpacity(
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
            }

        if (const auto showcaseNode = graphNode->GetProperty(L"showcase-lines");
            showcaseNode->IsOk() && showcaseNode->IsValueArray())
            {
            linePlot->ShowcaseLines(
                ExpandAndCache(linePlot, L"showcase-lines", showcaseNode->AsStrings()));
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadMultiSeriesLinePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                           size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for multi-series line plot."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto linePlot = std::make_shared<Graphs::MultiSeriesLinePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(foundPos->second,
                              ExpandAndCache(linePlot.get(), L"variables.y",
                                             variablesNode->GetProperty(L"y")->AsStrings()),
                              ExpandAndCache(linePlot.get(), L"variables.x",
                                             variablesNode->GetProperty(L"x")->AsString()));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            LoadLinePlotBaseOptions(graphNode, linePlot.get());

            return linePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for multi-series line plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadScatterPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for scatter plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(_DT(L"group"))->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto scatterPlot = std::make_shared<Graphs::ScatterPlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"regression-line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                scatterPlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            scatterPlot->SetData(
                foundPos->second,
                ExpandAndCache(scatterPlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(scatterPlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            // regression line options
            if (graphNode->HasProperty(L"show-regression-lines"))
                {
                scatterPlot->ShowRegressionLines(
                    graphNode->GetProperty(L"show-regression-lines")->AsBool(true));
                }
            if (graphNode->HasProperty(L"show-confidence-bands"))
                {
                scatterPlot->ShowConfidenceBands(
                    graphNode->GetProperty(L"show-confidence-bands")->AsBool(true));
                }
            if (graphNode->HasProperty(L"confidence-level"))
                {
                scatterPlot->SetConfidenceLevel(
                    graphNode->GetProperty(L"confidence-level")->AsDouble(0.95));
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, scatterPlot);
            return scatterPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for scatter plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadBubblePlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bubble plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(_DT(L"group"))->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto bubblePlot = std::make_shared<Graphs::BubblePlot>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"regression-line-scheme")));
            if (!groupVarNameRaw.empty())
                {
                bubblePlot->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            bubblePlot->SetData(
                foundPos->second,
                ExpandAndCache(bubblePlot.get(), L"variables.y",
                               variablesNode->GetProperty(L"y")->AsString()),
                ExpandAndCache(bubblePlot.get(), L"variables.x",
                               variablesNode->GetProperty(L"x")->AsString()),
                ExpandAndCache(bubblePlot.get(), L"variables.size",
                               variablesNode->GetProperty(_DT(L"size"))->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

            // bubble size options
            if (graphNode->HasProperty(L"min-bubble-radius"))
                {
                bubblePlot->SetMinBubbleRadius(
                    static_cast<size_t>(graphNode->GetProperty(L"min-bubble-radius")->AsDouble(4)));
                }
            if (graphNode->HasProperty(L"max-bubble-radius"))
                {
                bubblePlot->SetMaxBubbleRadius(static_cast<size_t>(
                    graphNode->GetProperty(L"max-bubble-radius")->AsDouble(30)));
                }

            // regression line options (inherited from ScatterPlot)
            if (graphNode->HasProperty(L"show-regression-lines"))
                {
                bubblePlot->ShowRegressionLines(
                    graphNode->GetProperty(L"show-regression-lines")->AsBool(true));
                }
            if (graphNode->HasProperty(L"show-confidence-bands"))
                {
                bubblePlot->ShowConfidenceBands(
                    graphNode->GetProperty(L"show-confidence-bands")->AsBool(true));
                }
            if (graphNode->HasProperty(L"confidence-level"))
                {
                bubblePlot->SetConfidenceLevel(
                    graphNode->GetProperty(L"confidence-level")->AsDouble(0.95));
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, bubblePlot);
            return bubblePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for bubble plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadChernoffFaces(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Chernoff faces."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(_DT(L"variables"));
        if (variablesNode->IsOk())
            {
            // chernoffPlot created below; lambda captures pointer after creation
            std::shared_ptr<Graphs::ChernoffFacesPlot> chernoffPlot;
            const auto getOptionalColumn = [this, &variablesNode, &chernoffPlot](
                                               const wxString& propName) -> std::optional<wxString>
            {
                const wxString rawValue = variablesNode->GetProperty(propName)->AsString();
                const wxString colName = ExpandConstants(rawValue);
                if (!rawValue.empty() && chernoffPlot)
                    {
                    chernoffPlot->SetPropertyTemplate(L"variables." + propName, rawValue);
                    }
                return (!colName.empty() ? std::optional<wxString>(colName) : std::nullopt);
            };

            // face color (optional)
            wxColour faceColor{ 255, 224, 189 }; // default flesh tone
            if (graphNode->HasProperty(L"face-color"))
                {
                faceColor = ConvertColor(graphNode->GetProperty(L"face-color"));
                }

            chernoffPlot = std::make_shared<Graphs::ChernoffFacesPlot>(canvas, faceColor);

            chernoffPlot->SetData(
                foundPos->second,
                ExpandAndCache(chernoffPlot.get(), L"variables.face-width",
                               variablesNode->GetProperty(L"face-width")->AsString()),
                getOptionalColumn(L"face-height"), getOptionalColumn(L"eye-size"),
                getOptionalColumn(L"eye-position"), getOptionalColumn(L"eyebrow-slant"),
                getOptionalColumn(L"pupil-position"), getOptionalColumn(L"nose-size"),
                getOptionalColumn(L"mouth-width"), getOptionalColumn(L"mouth-curvature"),
                getOptionalColumn(L"face-saturation"), getOptionalColumn(L"ear-size"));

            // appearance options
            if (graphNode->HasProperty(L"show-labels"))
                {
                chernoffPlot->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool(true));
                }
            if (graphNode->HasProperty(L"outline-color"))
                {
                chernoffPlot->SetOutlineColor(
                    ConvertColor(graphNode->GetProperty(L"outline-color")));
                }
            if (graphNode->HasProperty(L"gender"))
                {
                const auto gender =
                    ReportEnumConvert::ConvertGender(graphNode->GetProperty(L"gender")->AsString());
                if (gender.has_value())
                    {
                    chernoffPlot->SetGender(gender.value());
                    }
                }
            if (graphNode->HasProperty(L"eye-color"))
                {
                chernoffPlot->SetEyeColor(ConvertColor(graphNode->GetProperty(L"eye-color")));
                }
            if (graphNode->HasProperty(L"hair-color"))
                {
                chernoffPlot->SetHairColor(ConvertColor(graphNode->GetProperty(L"hair-color")));
                }
            if (graphNode->HasProperty(L"lipstick-color"))
                {
                chernoffPlot->SetLipstickColor(
                    ConvertColor(graphNode->GetProperty(L"lipstick-color")));
                }
            if (graphNode->HasProperty(L"hair-style"))
                {
                const auto hairStyle = ReportEnumConvert::ConvertHairStyle(
                    graphNode->GetProperty(L"hair-style")->AsString());
                if (hairStyle.has_value())
                    {
                    chernoffPlot->SetHairStyle(hairStyle.value());
                    }
                }
            if (graphNode->HasProperty(L"facial-hair"))
                {
                const auto facialHair = ReportEnumConvert::ConvertFacialHair(
                    graphNode->GetProperty(L"facial-hair")->AsString());
                if (facialHair.has_value())
                    {
                    chernoffPlot->SetFacialHair(facialHair.value());
                    }
                }

            chernoffPlot->SetPropertyTemplate(
                L"enhanced-legend",
                graphNode->GetProperty(L"enhanced-legend")->AsBool(true) ? L"true" : L"false");

            LoadGraph(graphNode, canvas, currentRow, currentColumn, chernoffPlot);
            return chernoffPlot;
            }

        throw std::runtime_error(_(L"Variables not defined for Chernoff faces.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWaffleChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                   size_t& currentRow, size_t& currentColumn)
        {
        std::vector<GraphItems::ShapeInfo> shapes;

        if (const auto shapesNode = graphNode->GetProperty(L"shapes"); shapesNode->IsOk())
            {
            if (shapesNode->IsValueArray())
                {
                auto nodes = shapesNode->AsNodes();
                for (const auto& shpNode : nodes)
                    {
                    shapes.push_back(LoadShapeInfo(shpNode));
                    }
                }
            }
        else
            {
            throw std::runtime_error(_(L"No shapes provided for waffle chart.").ToUTF8());
            }

        std::optional<Graphs::WaffleChart::GridRounding> gridRound{ std::nullopt };
        if (graphNode->HasProperty(L"grid-round"))
            {
            if (graphNode->GetProperty(L"grid-round")->HasProperty(L"cell-count") &&
                graphNode->GetProperty(L"grid-round")->HasProperty(L"shape-index"))
                {
                gridRound = Graphs::WaffleChart::GridRounding{
                    static_cast<size_t>(graphNode->GetProperty(L"grid-round")
                                            ->GetProperty(L"cell-count")
                                            ->AsDouble(100)),
                    static_cast<size_t>(graphNode->GetProperty(L"grid-round")
                                            ->GetProperty(L"shape-index")
                                            ->AsDouble(100))
                };
                }
            }

        auto waffleChart = std::make_shared<Graphs::WaffleChart>(
            canvas, shapes, gridRound,
            graphNode->HasProperty(L"row-count") ?
                std::optional<size_t>(graphNode->GetProperty(L"row-count")->AsDouble()) :
                std::nullopt);

        LoadGraph(graphNode, canvas, currentRow, currentColumn, waffleChart);
        return waffleChart;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWinLossSparkline(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for sparkline."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto postSeasonRaw = variablesNode->GetProperty(L"postseason")->AsString();
            const auto postSeason = ExpandConstants(postSeasonRaw);
            auto wlSparkline = std::make_shared<Graphs::WinLossSparkline>(canvas);
            if (!postSeasonRaw.empty())
                {
                wlSparkline->SetPropertyTemplate(L"variables.postseason", postSeasonRaw);
                }
            wlSparkline->SetData(
                foundPos->second,
                ExpandAndCache(wlSparkline.get(), L"variables.season",
                               variablesNode->GetProperty(L"season")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.won",
                               variablesNode->GetProperty(L"won")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.shutout",
                               variablesNode->GetProperty(L"shutout")->AsString()),
                ExpandAndCache(wlSparkline.get(), L"variables.home-game",
                               variablesNode->GetProperty(L"home-game")->AsString()),
                (postSeason.empty() ? std::nullopt : std::optional<wxString>(postSeason)));

            wlSparkline->HighlightBestRecords(
                graphNode->GetProperty(L"highlight-best-records")->AsBool(true));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, wlSparkline);
            return wlSparkline;
            }

        throw std::runtime_error(_(L"Variables not defined for sparkline.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadHeatMap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                               size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for heatmap."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupVarName = ExpandConstants(groupVarNameRaw);

            auto heatmap =
                std::make_shared<Graphs::HeatMap>(canvas, LoadGraphColorScheme(graphNode));
            if (!groupVarNameRaw.empty())
                {
                heatmap->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
                }
            heatmap->SetData(
                foundPos->second,
                ExpandAndCache(heatmap.get(), L"variables.continuous",
                               variablesNode->GetProperty(L"continuous")->AsString()),
                (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt),
                graphNode->GetProperty(L"group-column-count")->AsDouble(5));

            heatmap->ShowGroupHeaders(graphNode->GetProperty(L"show-group-header")->AsBool(true));
            heatmap->SetGroupHeaderPrefix(
                graphNode->GetProperty(L"group-header-prefix")->AsString());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, heatmap);
            return heatmap;
            }

        throw std::runtime_error(_(L"Variables not defined for heatmap.").ToUTF8());
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
                                     const std::shared_ptr<Graphs::BarChart>& barChart) const
        {
        if (barChart == nullptr)
            {
            return;
            }
        const auto boxEffect =
            ReportEnumConvert::ConvertBoxEffect(graphNode->GetProperty(L"box-effect")->AsString());
        if (boxEffect)
            {
            barChart->SetBarEffect(boxEffect.value());
            }

        // sorting
        const auto sortNode = graphNode->GetProperty(L"bar-sort");
        if (sortNode->IsOk())
            {
            // cache that bar-sort was explicitly specified
            barChart->SetPropertyTemplate(L"bar-sort", L"true");
            const auto sortDirection =
                sortNode->GetProperty(L"direction")->AsString().CmpNoCase(_DT(L"ascending")) == 0 ?
                    SortDirection::SortAscending :
                    SortDirection::SortDescending;
            const auto byNode = sortNode->GetProperty(L"by");
            if (byNode->IsOk())
                {
                const auto sortBy =
                    (byNode->AsString().CmpNoCase(L"length") == 0 ?
                         std::optional(Graphs::BarChart::BarSortComparison::SortByBarLength) :
                     byNode->AsString().CmpNoCase(L"label") == 0 ?
                         std::optional(Graphs::BarChart::BarSortComparison::SortByAxisLabel) :
                         std::nullopt);
                if (!sortBy.has_value())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': invalid bar sorting 'by' method."),
                                         byNode->AsString())
                            .ToUTF8());
                    }
                barChart->SortBars(sortBy.value(), sortDirection);
                }
            // or is sorting by a list of labels with a custom order
            else if (const auto labelsNode = sortNode->GetProperty(L"labels");
                     labelsNode->IsOk() && labelsNode->IsValueArray())
                {
                barChart->SortBars(labelsNode->AsStrings(), sortDirection);
                }
            else
                {
                throw std::runtime_error(_(L"Sorting method not defined for bar sort.").ToUTF8());
                }
            }

        const auto numDisplay = ReportEnumConvert::ConvertNumberDisplay(
            graphNode->GetProperty(_DT(L"number-display"))->AsString());
        if (numDisplay)
            {
            barChart->SetNumberDisplay(numDisplay.value());
            }

        // bar shapes
        const auto barShapes = graphNode->GetProperty(L"bar-shapes");
        // if applying one shape to all bars
        if (barShapes->IsOk() && !barShapes->IsValueArray())
            {
            const auto barShape = ReportEnumConvert::ConvertBarShape(barShapes->AsString());
            if (barShape.has_value())
                {
                for (auto& bar : barChart->GetBars())
                    {
                    bar.SetShape(barShape.value());
                    }
                }
            }
        /// @todo Add support for assigning shapes to different bars, based on axis label

        // showcasing
        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            barChart->SetGhostOpacity(
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
            }
        if (const auto showcaseNode = graphNode->GetProperty(L"showcase-bars");
            showcaseNode->IsOk() && showcaseNode->IsValueArray())
            {
            const bool hideGhostedLabels =
                graphNode->GetProperty(L"hide-ghosted-labels")->AsBool(true);
            barChart->ShowcaseBars(
                ExpandAndCache(barChart.get(), L"showcase-bars", showcaseNode->AsStrings()),
                hideGhostedLabels);
            }

        // decals to add to the bars
        const auto decalsNode = graphNode->GetProperty(L"decals");
        if (decalsNode->IsOk() && decalsNode->IsValueArray())
            {
            const auto decals = decalsNode->AsNodes();
            for (const auto& decal : decals)
                {
                const auto barPos = barChart->FindBar(decal->GetProperty(L"bar")->AsString());
                if (barPos.has_value())
                    {
                    const auto blockIndex = decal->GetProperty(L"block")->AsDouble(0);
                    const auto decalLabel =
                        LoadLabel(decal->GetProperty(L"decal"), GraphItems::Label{});
                    if (decalLabel != nullptr &&
                        blockIndex < barChart->GetBars().at(barPos.value()).GetBlocks().size())
                        {
                        barChart->GetBars()
                            .at(barPos.value())
                            .GetBlocks()
                            .at(blockIndex)
                            .SetDecal(*decalLabel);
                        }
                    }
                }
            }

        // bar groups
        const auto barGroupPlacement = ReportEnumConvert::ConvertLabelPlacement(
            graphNode->GetProperty(L"bar-group-placement")->AsString());
        if (barGroupPlacement.has_value())
            {
            barChart->SetBarGroupPlacement(barGroupPlacement.value());
            }

        const auto barGroupsNode = graphNode->GetProperty(L"bar-groups");
        if (barGroupsNode->IsOk() && barGroupsNode->IsValueArray())
            {
            const auto barGroups = barGroupsNode->AsNodes();
            for (const auto& barGroup : barGroups)
                {
                if (barGroup->IsOk())
                    {
                    Graphs::BarChart::BarGroup bGroup;
                    bGroup.m_barColor = (ConvertColor(barGroup->GetProperty(L"color")));
                    if (!bGroup.m_barColor.IsOk() && barChart->GetColorScheme() != nullptr)
                        {
                        bGroup.m_barColor = barChart->GetColorScheme()->GetColor(0);
                        }
                    LoadBrush(barGroup->GetProperty(L"brush"), bGroup.m_barBrush);
                    if (!bGroup.m_barBrush.IsOk() && barChart->GetBrushScheme() != nullptr)
                        {
                        bGroup.m_barBrush = barChart->GetBrushScheme()->GetBrush(0);
                        }
                    bGroup.m_barDecal = barGroup->GetProperty(L"decal")->AsString();

                    if (barGroup->GetProperty(L"start")->IsValueNumber())
                        {
                        bGroup.m_barPositions.first = barGroup->GetProperty(L"start")->AsDouble();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"start")->AsString());
                        if (foundBar.has_value())
                            {
                            bGroup.m_barPositions.first = foundBar.value();
                            }
                        else
                            {
                            throw std::runtime_error(
                                wxString::Format(
                                    _(L"'%s': bar label not found when adding bar group."),
                                    barGroup->GetProperty(L"start")->AsString())
                                    .ToUTF8());
                            }
                        }
                    if (barGroup->GetProperty(L"end")->IsValueNumber())
                        {
                        bGroup.m_barPositions.second = barGroup->GetProperty(L"end")->AsDouble();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"end")->AsString());
                        if (foundBar.has_value())
                            {
                            bGroup.m_barPositions.second = foundBar.value();
                            }
                        else
                            {
                            throw std::runtime_error(
                                wxString::Format(
                                    _(L"'%s': bar label not found when adding bar group."),
                                    barGroup->GetProperty(L"end")->AsString())
                                    .ToUTF8());
                            }
                        }

                    barChart->AddBarGroup(bGroup);
                    }
                }
            }

        // bar brackets
        if (const auto barBracketsNode = graphNode->GetProperty(L"first-bar-brackets");
            barBracketsNode->IsOk() && barBracketsNode->IsValueArray())
            {
            const auto barBrackets = barBracketsNode->AsNodes();
            for (size_t bbi = 0; bbi < barBrackets.size(); ++bbi)
                {
                const auto& barBracket = barBrackets[bbi];
                const auto idx = std::to_wstring(bbi);
                // just log any missing bracket requests and then skip over them
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        const auto startVal =
                            barBracket->GetProperty(L"start-block-re")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block-re")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddFirstBarBracketRE(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].start-block-re", startVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].end-block-re", endVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    else
                        {
                        const auto startVal = barBracket->GetProperty(L"start-block")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddFirstBarBracket(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"first-bar-brackets[" + idx + L"].start-block", startVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].end-block",
                                                      endVal);
                        barChart->SetPropertyTemplate(L"first-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    }
                catch (const std::exception& err)
                    {
                    wxLogWarning(wxString::FromUTF8(err.what()));
                    }
                }
            }
        if (const auto barBracketsNode = graphNode->GetProperty(L"last-bar-brackets");
            barBracketsNode->IsOk() && barBracketsNode->IsValueArray())
            {
            const auto barBrackets = barBracketsNode->AsNodes();
            for (size_t bbi = 0; bbi < barBrackets.size(); ++bbi)
                {
                const auto& barBracket = barBrackets[bbi];
                const auto idx = std::to_wstring(bbi);
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        const auto startVal =
                            barBracket->GetProperty(L"start-block-re")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block-re")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddLastBarBracketRE(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].start-block-re", startVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].end-block-re", endVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    else
                        {
                        const auto startVal = barBracket->GetProperty(L"start-block")->AsString();
                        const auto endVal = barBracket->GetProperty(L"end-block")->AsString();
                        const auto labelVal = barBracket->GetProperty(L"label")->AsString();
                        barChart->AddLastBarBracket(startVal, endVal, labelVal);
                        barChart->SetPropertyTemplate(
                            L"last-bar-brackets[" + idx + L"].start-block", startVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].end-block",
                                                      endVal);
                        barChart->SetPropertyTemplate(L"last-bar-brackets[" + idx + L"].label",
                                                      labelVal);
                        }
                    }
                catch (const std::exception& err)
                    {
                    wxLogWarning(wxString::FromUTF8(err.what()));
                    }
                }
            }

        const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
            graphNode->GetProperty(L"bar-label-display")->AsString());
        if (binLabel.has_value())
            {
            barChart->SetBinLabelDisplay(binLabel.value());
            }

        barChart->SetBinLabelSuffix(graphNode->GetProperty(L"bar-label-suffix")->AsString());

        // bar icons
        const auto barIconsNode = graphNode->GetProperty(L"bar-icons");
        if (barIconsNode->IsOk() && barIconsNode->IsValueArray())
            {
            const auto barIcons = barIconsNode->AsNodes();
            for (const auto& barIcon : barIcons)
                {
                if (barIcon->IsOk())
                    {
                    auto path = barIcon->GetProperty(L"image")->AsString();
                    if (!path.empty())
                        {
                        if (!wxFileName::FileExists(path))
                            {
                            path = wxFileName{ m_configFilePath }.GetPathWithSep() + path;
                            if (!wxFileName::FileExists(path))
                                {
                                throw std::runtime_error(
                                    wxString::Format(_(L"%s: image not found."), path).ToUTF8());
                                }
                            }
                        }

                    barChart->AddBarIcon(barIcon->GetProperty("label")->AsString(),
                                         GraphItems::Image::LoadFile(path));
                    }
                }
            }

        barChart->IncludeSpacesBetweenBars(
            graphNode->GetProperty(L"include-spaces-between-bars")->AsBool(true));

        if (graphNode->HasProperty(L"constrain-scaling-axis-to-bars") &&
            graphNode->GetProperty(L"constrain-scaling-axis-to-bars")->AsBool())
            {
            barChart->ConstrainScalingAxisToBars();
            }

        if (graphNode->GetProperty(L"apply-brushes-to-ungrouped-bars")->AsBool() &&
            !barChart->IsUsingGrouping() && barChart->GetBrushScheme() &&
            !barChart->GetBrushScheme()->GetBrushes().empty())
            {
            barChart->SetApplyBrushesToUngroupedBars(true);
            if (barChart->GetBarOrientation() == Orientation::Vertical)
                {
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (!blocks.empty())
                        {
                        blocks.front().GetBrush() = barChart->GetBrushScheme()->GetBrush(i);
                        }
                    }
                }
            else
                {
                // apply brush in reverse because the origin in going upward, but the client
                // sees the bars as going downward
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (!blocks.empty())
                        {
                        wxASSERT_MSG(barChart->GetBrushScheme()->GetBrushes().size() >= (1 + i),
                                     L"Bad brush mapping for bar chart!");
                        blocks.front().GetBrush() = barChart->GetBrushScheme()->GetBrush(
                            barChart->GetBrushScheme()->GetBrushes().size() - 1 - i);
                        }
                    }
                }
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
        // center by default, but allow LoadItems (below) to override that
        // if client asked for something else
        sh->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        sh->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

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
        // center by default, but allow LoadItems (below) to override that
        // if client asked for something else
        sh->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
        sh->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

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
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadHistogram(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for histogram."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto contVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto contVarName = ExpandConstants(contVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);

            const auto binMethod = ReportEnumConvert::ConvertBinningMethod(
                graphNode->GetProperty(L"binning-method")->AsString());

            const auto binIntervalDisplay = ReportEnumConvert::ConvertIntervalDisplay(
                graphNode->GetProperty(L"interval-display")->AsString());

            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->AsString());

            const auto rounding = ReportEnumConvert::ConvertRoundingMethod(
                graphNode->GetProperty(L"rounding")->AsString());

            const std::optional<double> startBinsValue =
                graphNode->HasProperty(L"bins-start") ?
                    std::optional<double>(graphNode->GetProperty(L"bins-start")->AsDouble()) :
                    std::nullopt;
            const std::optional<size_t> suggestedBinCount =
                graphNode->HasProperty(L"suggested-bin-count") ?
                    std::optional<double>(
                        graphNode->GetProperty(L"suggested-bin-count")->AsDouble()) :
                    std::nullopt;
            const std::optional<size_t> maxBinCount =
                graphNode->HasProperty(L"max-bin-count") ?
                    std::optional<double>(graphNode->GetProperty(L"max-bin-count")->AsDouble()) :
                    std::nullopt;

            auto histo = std::make_shared<Graphs::Histogram>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!contVarNameRaw.empty())
                {
                histo->SetPropertyTemplate(L"variables.aggregate", contVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                histo->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->AsString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                {
                histo->SetBarOrientation(Orientation::Horizontal);
                }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                {
                histo->SetBarOrientation(Orientation::Vertical);
                }

            histo->SetData(
                foundPos->second, contVarName,
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt),
                binMethod.value_or(Graphs::Histogram::BinningMethod::BinByIntegerRange),
                rounding.value_or(RoundingMethod::NoRounding),
                binIntervalDisplay.value_or(Graphs::Histogram::IntervalDisplay::Cutpoints),
                binLabel.value_or(BinLabelDisplay::BinValue),
                graphNode->GetProperty(L"show-full-range")->AsBool(true), startBinsValue,
                std::make_pair(suggestedBinCount, maxBinCount));

            LoadBarChart(graphNode, histo);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, histo);
            return histo;
            }

        throw std::runtime_error(_(L"Variables not defined for histogram.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadScaleChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for scale chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto scoreVarNameRaw = variablesNode->GetProperty(L"score")->AsString();
            const auto scoreVarName = ExpandConstants(scoreVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);

            auto chart = std::make_shared<Graphs::ScaleChart>(
                canvas, LoadGraphColorScheme(graphNode),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
            if (!scoreVarNameRaw.empty())
                {
                chart->SetPropertyTemplate(L"variables.score", scoreVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                chart->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }

            if (graphNode->GetProperty(L"showcase-score")->AsBool(false))
                {
                chart->ShowcaseScore(true);
                }

            // load scales
            const auto scalesNode = graphNode->GetProperty(L"scales");
            if (scalesNode->IsOk())
                {
                const auto scales = scalesNode->AsNodes();
                for (const auto& scaleNode : scales)
                    {
                    std::vector<Graphs::BarChart::BarBlock> blocks;
                    const auto blocksNode = scaleNode->GetProperty(L"blocks");
                    if (blocksNode->IsOk())
                        {
                        const auto blockNodes = blocksNode->AsNodes();
                        blocks.reserve(blockNodes.size());
                        for (const auto& blockNode : blockNodes)
                            {
                            const auto length = blockNode->GetProperty(L"length")->AsDouble(10);
                            const auto label = blockNode->GetProperty(L"label")->AsString();
                            const auto color = ConvertColor(blockNode->GetProperty(L"color"));
                            blocks.emplace_back(
                                Graphs::BarChart::BarBlockInfo(length)
                                    .Brush(color.IsOk() ? wxBrush(color) : *wxGREEN_BRUSH)
                                    .Decal(GraphItems::Label(
                                        GraphItems::GraphItemInfo{ label }.LabelFitting(
                                            LabelFit::ScaleFontToFit))));
                            }
                        }
                    const auto header =
                        ExpandConstants(scaleNode->GetProperty(L"header")->AsString());
                    const auto startNode = scaleNode->GetProperty(L"start");
                    const std::optional<double> startPos =
                        startNode->IsOk() ? std::optional<double>(startNode->AsDouble(0)) :
                                            std::nullopt;
                    chart->AddScale(blocks, startPos, header);
                    }
                }

            // main scale values and precision
            const auto mainValsNode = graphNode->GetProperty(L"main-scale-values");
            if (mainValsNode->IsOk())
                {
                const auto mainVals = mainValsNode->AsDoubles();
                const auto precision = static_cast<uint8_t>(
                    graphNode->GetProperty(L"main-scale-precision")->AsDouble(0));
                chart->SetMainScaleValues(mainVals, precision);
                }

            chart->SetData(
                foundPos->second, scoreVarName,
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt));

            // column headers (must be set after AddScale/SetData)
            chart->SetMainScaleColumnHeader(
                ExpandConstants(graphNode->GetProperty(L"main-scale-header")->AsString()));
            chart->SetDataColumnHeader(
                ExpandConstants(graphNode->GetProperty(L"data-column-header")->AsString()));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, chart);
            return chart;
            }

        throw std::runtime_error(_(L"Variables not defined for scale chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadCategoricalBarChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                           size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bar chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupNameRaw = variablesNode->GetProperty(L"group")->AsString();
            const auto groupName = ExpandConstants(groupNameRaw);
            const auto categoryNameRaw = variablesNode->GetProperty(L"category")->AsString();
            const auto categoryName = ExpandConstants(categoryNameRaw);
            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->AsString());

            auto barChart = std::make_shared<Graphs::CategoricalBarChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!aggVarNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.group", groupNameRaw);
                }
            if (!categoryNameRaw.empty())
                {
                barChart->SetPropertyTemplate(L"variables.category", categoryNameRaw);
                }

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->AsString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                {
                barChart->SetBarOrientation(Orientation::Horizontal);
                }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                {
                barChart->SetBarOrientation(Orientation::Vertical);
                }

            barChart->SetData(
                foundPos->second, categoryName,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
                (!groupName.empty() ? std::optional<wxString>(groupName) : std::nullopt),
                binLabel.has_value() ? binLabel.value() : BinLabelDisplay::BinValue);

            LoadBarChart(graphNode, barChart);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, barChart);
            return barChart;
            }
        throw std::runtime_error(_(L"Variables not defined for bar chart.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadSankeyDiagram(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Sankey diagram."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto fromVarNameRaw = variablesNode->GetProperty(L"from")->AsString();
            const auto fromVarName = ExpandConstants(fromVarNameRaw);
            const auto toColNameRaw = variablesNode->GetProperty(L"to")->AsString();
            const auto toColName = ExpandConstants(toColNameRaw);

            const auto fromWeightVarNameRaw =
                variablesNode->GetProperty(L"from-weight")->AsString();
            const auto fromWeightVarName = ExpandConstants(fromWeightVarNameRaw);
            const auto toWeightColNameRaw = variablesNode->GetProperty(L"to-weight")->AsString();
            const auto toWeightColName = ExpandConstants(toWeightColNameRaw);

            const auto fromGroupVarNameRaw = variablesNode->GetProperty(L"from-group")->AsString();
            const auto fromGroupVarName = ExpandConstants(fromGroupVarNameRaw);

            auto sankey = std::make_shared<Graphs::SankeyDiagram>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")));
            if (!fromVarNameRaw.empty())
                {
                sankey->SetPropertyTemplate(L"variables.from", fromVarNameRaw);
                }
            if (!toColNameRaw.empty())
                {
                sankey->SetPropertyTemplate(L"variables.to", toColNameRaw);
                }
            if (!fromWeightVarNameRaw.empty())
                {
                sankey->SetPropertyTemplate(L"variables.from-weight", fromWeightVarNameRaw);
                }
            if (!toWeightColNameRaw.empty())
                {
                sankey->SetPropertyTemplate(L"variables.to-weight", toWeightColNameRaw);
                }
            if (!fromGroupVarNameRaw.empty())
                {
                sankey->SetPropertyTemplate(L"variables.from-group", fromGroupVarNameRaw);
                }

            const auto groupLabelDisplay = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"group-label-display")->AsString());
            if (groupLabelDisplay)
                {
                sankey->SetGroupLabelDisplay(groupLabelDisplay.value());
                }

            const auto groupHeaderDisplay = ReportEnumConvert::ConvertGraphColumnHeader(
                graphNode->GetProperty(L"group-header-display")->AsString());
            if (groupHeaderDisplay)
                {
                sankey->SetColumnHeaderDisplay(groupHeaderDisplay.value());
                }

            if (graphNode->HasProperty(L"column-headers"))
                {
                const auto columnHeaderRaw = graphNode->GetProperty(L"column-headers")->AsStrings();
                auto columnHeader =
                    ExpandAndCache(sankey.get(), L"column-headers", columnHeaderRaw);
                sankey->SetColumnHeaders(columnHeader);
                }

            const auto flowShape = ReportEnumConvert::ConvertFlowShape(
                graphNode->GetProperty(L"flow-shape")->AsString());
            if (flowShape)
                {
                sankey->SetFlowShape(flowShape.value());
                }

            sankey->SetData(
                foundPos->second, fromVarName, toColName,
                !fromWeightVarName.empty() ? std::optional<wxString>(fromWeightVarName) :
                                             std::nullopt,
                !toWeightColName.empty() ? std::optional<wxString>(toWeightColName) : std::nullopt,
                !fromGroupVarName.empty() ? std::optional<wxString>(fromGroupVarName) :
                                            std::nullopt);

            LoadGraph(graphNode, canvas, currentRow, currentColumn, sankey);
            return sankey;
            }
        throw std::runtime_error(_(L"Variables not defined for Sankey diagram.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadWordCloud(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                 size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for word cloud."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto wordColNameRaw = variablesNode->GetProperty(L"words")->AsString();
            const auto wordColName = ExpandConstants(wordColNameRaw);

            auto wordCloud =
                std::make_shared<Graphs::WordCloud>(canvas, LoadGraphColorScheme(graphNode));
            if (!aggVarNameRaw.empty())
                {
                wordCloud->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!wordColNameRaw.empty())
                {
                wordCloud->SetPropertyTemplate(L"variables.words", wordColNameRaw);
                }

            wordCloud->SetData(
                foundPos->second, wordColName,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, wordCloud);
            return wordCloud;
            }
        throw std::runtime_error(_(L"Variables not defined for word cloud.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadBoxPlot(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                               size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for box plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupVar1NameRaw = variablesNode->GetProperty(L"group-1")->AsString();
            const auto groupVar1Name = ExpandConstants(groupVar1NameRaw);

            auto boxPlot = std::make_shared<Graphs::BoxPlot>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
            if (!aggVarNameRaw.empty())
                {
                boxPlot->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupVar1NameRaw.empty())
                {
                boxPlot->SetPropertyTemplate(L"variables.group-1", groupVar1NameRaw);
                }
            boxPlot->SetData(
                foundPos->second, aggVarName,
                (!groupVar1Name.empty() ? std::optional<wxString>(groupVar1Name) : std::nullopt));

            const auto boxEffect = ReportEnumConvert::ConvertBoxEffect(
                graphNode->GetProperty(L"box-effect")->AsString());
            if (boxEffect)
                {
                boxPlot->SetBoxEffect(boxEffect.value());
                }

            boxPlot->ShowAllPoints(graphNode->GetProperty(L"show-all-points")->AsBool());
            boxPlot->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool());
            const auto showMidpointNode = graphNode->GetProperty(L"show-midpoint-connection");
            if (showMidpointNode->IsOk())
                {
                boxPlot->ShowMidpointConnection(showMidpointNode->AsBool());
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, boxPlot);
            return boxPlot;
            }
        throw std::runtime_error(_(L"Variables not defined for box plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadPieChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for pie chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
            const auto aggVarName = ExpandConstants(aggVarNameRaw);
            const auto groupVar1NameRaw = variablesNode->GetProperty(L"group-1")->AsString();
            const auto groupVar1Name = ExpandConstants(groupVar1NameRaw);
            const auto groupVar2NameRaw = variablesNode->GetProperty(L"group-2")->AsString();
            const auto groupVar2Name = ExpandConstants(groupVar2NameRaw);

            auto pieChart = std::make_shared<Graphs::PieChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            if (!aggVarNameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
                }
            if (!groupVar1NameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.group-1", groupVar1NameRaw);
                }
            if (!groupVar2NameRaw.empty())
                {
                pieChart->SetPropertyTemplate(L"variables.group-2", groupVar2NameRaw);
                }
            if (groupVar1Name.empty())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: group variable not specified for pie chart."), dsName)
                        .ToUTF8());
                }
            pieChart->SetData(
                foundPos->second,
                (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
                groupVar1Name,
                (!groupVar2Name.empty() ? std::optional<wxString>(groupVar2Name) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"inner-pie-line-pen"),
                    pieChart->GetInnerPieConnectionLinePen());

            const auto labelPlacement = ReportEnumConvert::ConvertLabelPlacement(
                graphNode->GetProperty(L"label-placement")->AsString());
            if (labelPlacement.has_value())
                {
                pieChart->SetLabelPlacement(labelPlacement.value());
                }

            const auto outerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-pie-midpoint-label-display")->AsString());
            if (outerPieMidLabel.has_value())
                {
                pieChart->SetOuterPieMidPointLabelDisplay(outerPieMidLabel.value());
                }

            const auto innerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"inner-pie-midpoint-label-display")->AsString());
            if (innerPieMidLabel.has_value())
                {
                pieChart->SetInnerPieMidPointLabelDisplay(innerPieMidLabel.value());
                }

            const auto outerLabelDisplay = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-label-display")->AsString());
            if (outerLabelDisplay.has_value())
                {
                pieChart->SetOuterLabelDisplay(outerLabelDisplay.value());
                }

            if (graphNode->HasProperty(L"include-inner-pie-labels"))
                {
                pieChart->ShowInnerPieLabels(
                    graphNode->GetProperty(L"include-inner-pie-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"include-outer-pie-labels"))
                {
                pieChart->ShowOuterPieLabels(
                    graphNode->GetProperty(L"include-outer-pie-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"color-labels"))
                {
                pieChart->UseColorLabels(graphNode->GetProperty(L"color-labels")->AsBool());
                }

            if (graphNode->HasProperty(L"ghost-opacity"))
                {
                pieChart->SetGhostOpacity(
                    graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY));
                }

            // margin notes
            if (graphNode->HasProperty(L"left-margin-note"))
                {
                auto marginLabel =
                    LoadLabel(graphNode->GetProperty(L"left-margin-note"), GraphItems::Label{});
                if (marginLabel != nullptr)
                    {
                    pieChart->GetLeftMarginNote() = *marginLabel;
                    }
                }
            if (graphNode->HasProperty(L"right-margin-note"))
                {
                auto marginLabel =
                    LoadLabel(graphNode->GetProperty(L"right-margin-note"), GraphItems::Label{});
                if (marginLabel != nullptr)
                    {
                    pieChart->GetRightMarginNote() = *marginLabel;
                    }
                }

            if (const auto pieEffect = ReportEnumConvert::ConvertPieSliceEffect(
                    graphNode->GetProperty(L"pie-slice-effect")->AsString());
                pieEffect.has_value())
                {
                pieChart->SetPieSliceEffect(pieEffect.value());
                }

            if (const auto pieStyle = ReportEnumConvert::ConvertPieStyle(
                    graphNode->GetProperty(L"pie-style")->AsString());
                pieStyle.has_value())
                {
                pieChart->SetPieStyle(pieStyle.value());
                }

            pieChart->SetDynamicMargins(graphNode->GetProperty(L"dynamic-margins")->AsBool());

            // showcase the slices
            const auto showcaseNode = graphNode->GetProperty(L"showcase-slices");
            if (showcaseNode->IsValueArray())
                {
                const auto peri = ReportEnumConvert::ConvertPerimeter(
                    graphNode->GetProperty(L"showcased-ring-labels")->AsString());
                pieChart->ShowcaseOuterPieSlices(
                    ExpandAndCache(pieChart.get(), L"showcase-slices", showcaseNode->AsStrings()),
                    peri.has_value() ? peri.value() : Perimeter::Outer);
                }
            else if (showcaseNode->IsOk())
                {
                const auto pieType = showcaseNode->GetProperty(L"pie")->AsString();
                const auto categoryType = showcaseNode->GetProperty(L"category")->AsString();
                const auto peri = ReportEnumConvert::ConvertPerimeter(
                    graphNode->GetProperty(L"showcased-ring-labels")->AsString());
                if (pieType.CmpNoCase(L"inner") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->AsBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->AsBool());
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->AsBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->AsBool());
                        }
                    }
                if (pieType.CmpNoCase(L"outer") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestOuterPieSlices(
                            peri.has_value() ? peri.value() : Perimeter::Outer);
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestOuterPieSlices(
                            peri.has_value() ? peri.value() : Perimeter::Outer);
                        }
                    }
                }

            // donut hole info
            const auto donutHoleNode = graphNode->GetProperty(L"donut-hole");
            if (donutHoleNode->IsOk())
                {
                pieChart->IncludeDonutHole(true);
                const auto labelProperty = donutHoleNode->GetProperty(L"label");
                if (labelProperty->IsOk())
                    {
                    auto holeLabel = LoadLabel(labelProperty, pieChart->GetDonutHoleLabel());
                    if (holeLabel != nullptr)
                        {
                        pieChart->GetDonutHoleLabel() = *holeLabel;
                        }
                    }
                const auto propNode = donutHoleNode->GetProperty(L"proportion");
                if (propNode->IsOk())
                    {
                    pieChart->SetDonutHoleProportion(propNode->AsDouble());
                    }
                const wxColour color(ConvertColor(donutHoleNode->GetProperty(L"color")));
                if (color.IsOk())
                    {
                    pieChart->SetDonutHoleColor(color);
                    }
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pieChart);
            return pieChart;
            }
        throw std::runtime_error(_(L"Variables not defined for pie chart.").ToUTF8());
        }

    //---------------------------------------------------
    void ReportBuilder::ApplyTableFeatures(std::shared_ptr<Graphs::Table>& table)
        {
        ReportNodeLoader loader(*this);
        loader.ApplyTableFeatures(table);
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadTable(const wxSimpleJSON::Ptr_t& tableNode,
                                                              Canvas* canvas, size_t& currentRow,
                                                              size_t& currentColumn)
        {
        ReportNodeLoader nodeLoader(*this);

        const wxString dsName = tableNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for table."), dsName).ToUTF8());
            }

        std::vector<wxString> variables;
        const auto variablesNode = tableNode->GetProperty(L"variables");
        if (variablesNode->IsOk() && variablesNode->IsValueString())
            {
            auto convertedVars =
                ExpandColumnSelections(variablesNode->AsString(), foundPos->second);
            if (convertedVars)
                {
                variables.insert(variables.cend(), convertedVars.value().cbegin(),
                                 convertedVars.value().cend());
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown variable selection formula for table."),
                                     variablesNode->AsString())
                        .ToUTF8());
                }
            }
        else if (variablesNode->IsOk() && variablesNode->IsValueArray())
            {
            const auto readVariables = variablesNode->AsStrings();
            for (const auto& readVar : readVariables)
                {
                if (auto convertedVars = ExpandColumnSelections(readVar, foundPos->second))
                    {
                    variables.insert(variables.cend(), convertedVars.value().cbegin(),
                                     convertedVars.value().cend());
                    }
                else
                    {
                    variables.push_back(readVar);
                    }
                }
            }

        auto table = std::make_shared<Graphs::Table>(canvas);

        // load table defaults
        // change columns' borders
        const auto borderDefaults = tableNode->GetProperty(L"default-borders")->AsBools();
        table->SetDefaultBorders((!borderDefaults.empty() ? borderDefaults[0] : true),
                                 (borderDefaults.size() > 1 ? borderDefaults[1] : true),
                                 (borderDefaults.size() > 2 ? borderDefaults[2] : true),
                                 (borderDefaults.size() > 3 ? borderDefaults[3] : true));

        table->SetData(foundPos->second, variables, tableNode->GetProperty(L"transpose")->AsBool());

        // cache raw JSON for round-trip serialization
        if (variablesNode->IsOk())
            {
            table->SetPropertyTemplate(L"variables", variablesNode->Print(false));
            }
        if (tableNode->GetProperty(L"transpose")->AsBool())
            {
            table->SetPropertyTemplate(L"transpose", L"true");
            }

        // sorting
        const auto sortNode = tableNode->GetProperty(L"row-sort");
        if (sortNode->IsOk())
            {
            nodeLoader.ApplyTableSort(table, sortNode);
            table->SetPropertyTemplate(L"row-sort", sortNode->Print(false));
            }

        if (tableNode->HasProperty(L"link-id"))
            {
            if (const auto linkId = ConvertNumber(tableNode->GetProperty(L"link-id")))
                {
                auto foundPosTLink = std::ranges::find_if(
                    m_tableLinks, [&linkId](const auto& tLink)
                    { return tLink.GetId() == static_cast<size_t>(linkId.value()); });
                if (foundPosTLink != m_tableLinks.end())
                    {
                    foundPosTLink->AddTable(table);
                    }
                else
                    {
                    TableLink tLink{ static_cast<size_t>(linkId.value()) };
                    tLink.AddTable(table);
                    m_tableLinks.push_back(std::move(tLink));
                    }
                table->SetPropertyTemplate(L"link-id", std::to_wstring(linkId.value()));
                }
            }

        table->ClearTrailingRowFormatting(
            tableNode->GetProperty(L"clear-trailing-row-formatting")->AsBool());

        const auto minWidthProp = tableNode->GetProperty(L"min-width-proportion");
        if (minWidthProp->IsOk())
            {
            table->SetMinWidthProportion(minWidthProp->AsDouble());
            }
        const auto minHeightProp = tableNode->GetProperty(L"min-height-proportion");
        if (minHeightProp->IsOk())
            {
            table->SetMinHeightProportion(minHeightProp->AsDouble());
            }

        LoadPen(tableNode->GetProperty(L"highlight-pen"), table->GetHighlightPen());

        if (tableNode->HasProperty(L"insert-group-header"))
            {
            const auto groupHeaderNode = tableNode->GetProperty(L"insert-group-header");
            nodeLoader.ApplyTableGroupHeader(table, groupHeaderNode);
            table->SetPropertyTemplate(L"insert-group-header", groupHeaderNode->Print(false));
            }

        // group the rows
        const auto rowGroupNode = tableNode->GetProperty(L"row-group");
        if (rowGroupNode->IsOk() && !rowGroupNode->AsDoubles().empty())
            {
            nodeLoader.ApplyTableRowGrouping(table, rowGroupNode);
            table->SetPropertyTemplate(L"row-group", rowGroupNode->Print(false));
            }

        // group the columns
        const auto columnGroupNode = tableNode->GetProperty(L"column-group");
        if (columnGroupNode->IsOk() && !columnGroupNode->AsDoubles().empty())
            {
            nodeLoader.ApplyTableColumnGrouping(table, columnGroupNode);
            table->SetPropertyTemplate(L"column-group", columnGroupNode->Print(false));
            }

        // apply zebra stripes to loaded data before we start adding custom rows/columns,
        // manually changing row/column colors, etc.
        if (tableNode->HasProperty(L"alternate-row-color"))
            {
            const auto altRowColorNode = tableNode->GetProperty(L"alternate-row-color");
            nodeLoader.ApplyTableAlternateRowColor(table, altRowColorNode);
            table->SetPropertyTemplate(L"alternate-row-color", altRowColorNode->Print(false));
            }

        // add rows
        const auto rowAddNode = tableNode->GetProperty(L"row-add");
        if (!rowAddNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableRowAdditions(table, rowAddNode);
            table->SetPropertyTemplate(L"row-add", rowAddNode->Print(false));
            }

        // change the rows' suppression
        const auto rowSuppressionNode = tableNode->GetProperty(L"row-suppression");
        if (!rowSuppressionNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableRowSuppression(table, rowSuppressionNode);
            table->SetPropertyTemplate(L"row-suppression", rowSuppressionNode->Print(false));
            }

        nodeLoader.ApplyTableRowFormatting(table, tableNode->GetProperty(L"row-formatting"));
        nodeLoader.ApplyTableRowColor(table, tableNode->GetProperty(L"row-color"));
        nodeLoader.ApplyTableRowBold(table, tableNode->GetProperty(L"row-bold"));
        nodeLoader.ApplyTableRowBorders(table, tableNode->GetProperty(L"row-borders"));
        nodeLoader.ApplyTableRowContentAlignment(table,
                                                 tableNode->GetProperty(L"row-content-align"));

        // cache row formatting properties for round-trip serialization
        for (const auto& prop :
             { L"row-formatting", L"row-color", L"row-bold", L"row-borders", L"row-content-align" })
            {
            const auto node = tableNode->GetProperty(prop);
            if (node->IsOk() && !node->AsNodes().empty())
                {
                table->SetPropertyTemplate(prop, node->Print(false));
                }
            }

        // change the columns' suppression
        const auto columnSuppressionNode = tableNode->GetProperty(L"column-suppression");
        if (!columnSuppressionNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableColumnSuppression(table, columnSuppressionNode);
            table->SetPropertyTemplate(L"column-suppression", columnSuppressionNode->Print(false));
            }

        nodeLoader.ApplyTableColumnFormatting(table, tableNode->GetProperty(L"column-formatting"));
        nodeLoader.ApplyTableColumnColor(table, tableNode->GetProperty(L"column-color"));
        nodeLoader.ApplyTableColumnBold(table, tableNode->GetProperty(L"column-bold"));
        nodeLoader.ApplyTableColumnBorders(table, tableNode->GetProperty(L"column-borders"));

        // cache column formatting properties for round-trip serialization
        for (const auto& prop :
             { L"column-formatting", L"column-color", L"column-bold", L"column-borders" })
            {
            const auto node = tableNode->GetProperty(prop);
            if (node->IsOk() && !node->AsNodes().empty())
                {
                table->SetPropertyTemplate(prop, node->Print(false));
                }
            }

        // highlight cells down a column
        const auto columnHighlightNode = tableNode->GetProperty(L"column-highlight");
        if (!columnHighlightNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableColumnHighlight(table, columnHighlightNode);
            table->SetPropertyTemplate(L"column-highlight", columnHighlightNode->Print(false));
            }

        // column/row aggregates
        const auto aggregatesNode = tableNode->GetProperty(L"aggregates");
        if (!aggregatesNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableAggregates(table, aggregatesNode);
            table->SetPropertyTemplate(L"aggregates", aggregatesNode->Print(false));
            }

        // row totals
        const auto rowTotalsNode = tableNode->GetProperty(L"row-totals");
        if (rowTotalsNode->IsOk())
            {
            nodeLoader.ApplyTableRowTotals(table, rowTotalsNode);
            table->SetPropertyTemplate(L"row-totals", rowTotalsNode->Print(false));
            }

        // cell updating
        const auto cellUpdateNode = tableNode->GetProperty(L"cell-update");
        if (!cellUpdateNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableCellUpdates(table, cellUpdateNode);
            table->SetPropertyTemplate(L"cell-update", cellUpdateNode->Print(false));
            }

        const auto cellAnnotationsNode = tableNode->GetProperty(L"cell-annotations");
        if (!cellAnnotationsNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableAnnotations(table, cellAnnotationsNode);
            table->SetPropertyTemplate(L"cell-annotations", cellAnnotationsNode->Print(false));
            }

        // assign footnotes after all cells have been updated
        const auto footnotesJsonNode = tableNode->GetProperty(L"footnotes");
        if (!footnotesJsonNode->AsNodes().empty())
            {
            nodeLoader.ApplyTableFootnotes(table, footnotesJsonNode);
            table->SetPropertyTemplate(L"footnotes", footnotesJsonNode->Print(false));
            }

        // UI-only formatting options (used by InsertTableDlg for round-tripping)
        const auto boldHeaderNode = tableNode->GetProperty(L"ui.bold-header-row");
        if (boldHeaderNode->IsOk())
            {
            const bool val = boldHeaderNode->AsBool();
            table->SetPropertyTemplate(L"ui.bold-header-row", val ? L"true" : L"false");
            if (val)
                {
                table->BoldRow(0);
                }
            }
        const auto centerHeaderNode = tableNode->GetProperty(L"ui.center-header-row");
        if (centerHeaderNode->IsOk())
            {
            const bool val = centerHeaderNode->AsBool();
            table->SetPropertyTemplate(L"ui.center-header-row", val ? L"true" : L"false");
            if (val)
                {
                table->SetRowHorizontalPageAlignment(0, PageHorizontalAlignment::Centered);
                }
            }
        const auto boldFirstColNode = tableNode->GetProperty(L"ui.bold-first-column");
        if (boldFirstColNode->IsOk())
            {
            const bool val = boldFirstColNode->AsBool();
            table->SetPropertyTemplate(L"ui.bold-first-column", val ? L"true" : L"false");
            if (val)
                {
                table->BoldColumn(0);
                }
            }

        LoadGraph(tableNode, canvas, currentRow, currentColumn, table);
        return table;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::ExpandNumericConstant(wxString str) const
        {
        if (str.starts_with(L"{{") && str.ends_with(L"}}"))
            {
            str = str.substr(2, str.length() - 4);
            }
        const auto foundVal = m_values.find(str);
        if (foundVal != m_values.cend())
            {
            if (const auto* const dVal{ std::get_if<double>(&foundVal->second) };
                dVal != nullptr && !std::isnan(*dVal))
                {
                return *dVal;
                }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ExpandConstants(wxString str) const
        {
        const wxRegEx re(L"{{([^}]+)}}");
        size_t start{ 0 }, len{ 0 };
        std::wstring_view processText(str.wc_str());
        std::map<wxString, wxString> replacements;
        while (re.Matches(processText.data()))
            {
            // catalog all the placeholders and their replacements
            [[maybe_unused]]
            const auto fullMatchResult = re.GetMatch(&start, &len, 0);
            const auto foundVal = m_values.find(re.GetMatch(processText.data(), 1));
            if (foundVal != m_values.cend())
                {
                if (const auto* const strVal{ std::get_if<wxString>(&foundVal->second) };
                    strVal != nullptr)
                    {
                    replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                  *strVal);
                    }
                else if (const auto* const dVal{ std::get_if<double>(&foundVal->second) };
                         dVal != nullptr)
                    {
                    if (std::isnan(*dVal))
                        {
                        replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                      wxString{});
                        }
                    else
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)),
                            wxNumberFormatter::ToString(
                                *dVal, 2,
                                wxNumberFormatter::Style::Style_WithThousandsSep |
                                    wxNumberFormatter::Style::Style_NoTrailingZeroes));
                        }
                    }
                }
            // or a function like "Now()" that doesn't require a dataset
            else
                {
                const auto calcStr = CalcFormula(re.GetMatch(processText.data(), 1), nullptr);
                if (const auto* const strVal{ std::get_if<wxString>(&calcStr) }; strVal != nullptr)
                    {
                    replacements.insert_or_assign(std::wstring(processText.substr(start, len)),
                                                  *strVal);
                    }
                }
            processText = processText.substr(start + len);
            }

        // now, replace the placeholders with the user-defined values mapped to them
        for (const auto& rep : replacements)
            {
            str.Replace(rep.first, rep.second);
            }

        return str;
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(const wxSimpleJSON::Ptr_t& colorNode) const
        {
        if (!colorNode->IsOk())
            {
            return wxNullColour;
            }

        return (colorNode->IsValueNull() ?
                    // using null in JSON for a color implies that we want
                    // a legit color that is transparent
                    wxTransparentColour :
                    ConvertColor(colorNode->AsString()));
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(wxString colorStr,
                                         GraphItems::GraphItemBase* item /*= nullptr*/,
                                         const wxString& property /*= wxString{}*/) const
        {
        long opacity{ wxALPHA_OPAQUE };
        if (const auto colonPos = colorStr.find(L':'); colonPos != wxString::npos)
            {
            colorStr.substr(colonPos + 1).ToLong(&opacity);
            colorStr = colorStr.substr(0, colonPos);
            }
        // in case the color is a user-defined constant in the file
        const wxString rawColorStr = colorStr;
        colorStr = ExpandConstants(colorStr);
        if (!rawColorStr.empty() && item != nullptr && !property.empty())
            {
            item->SetPropertyTemplate(property, rawColorStr);
            }

        wxColour retColor;
        // see if it is one of our defined colors
        auto foundPos = m_colorMap.find(std::wstring_view(colorStr.MakeLower().wc_str()));
        if (foundPos != m_colorMap.cend())
            {
            retColor = Colors::ColorBrewer::GetColor(foundPos->second);
            }
        else if (colorStr == L"transparent")
            {
            retColor = wxTransparentColour;
            }
        // may be a hex string or RGB string
        else
            {
            retColor = wxColour{ colorStr };
            }

        if (std::cmp_not_equal(opacity, wxALPHA_OPAQUE))
            {
            retColor = Colors::ColorContrast::ChangeOpacity(retColor, opacity);
            }
        return retColor;
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

            // center by default, but allow LoadItems (below) to override that
            // if client asked for something else
            image->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            image->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

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

        wxBitmap bmp = bmps[0];

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

    //---------------------------------------------------
    void ReportBuilder::LoadGraph(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                  size_t& currentRow, size_t& currentColumn,
                                  const std::shared_ptr<Graphs::Graph2D>& graph)
        {
        LoadItem(graphNode, *graph);

        // cache dataset name for round-tripping
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        if (!dsName.empty())
            {
            graph->SetPropertyTemplate(L"dataset", dsName);
            }

        // title information
        const auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            const auto titleLabel = LoadLabel(titleProperty, graph->GetTitle());
            if (titleLabel != nullptr)
                {
                graph->GetTitle() = *titleLabel;
                }
            }

        // subtitle information
        const auto subtitleProperty = graphNode->GetProperty(L"sub-title");
        if (subtitleProperty->IsOk())
            {
            const auto subtitleLabel = LoadLabel(subtitleProperty, graph->GetSubtitle());
            if (subtitleLabel != nullptr)
                {
                graph->GetSubtitle() = *subtitleLabel;
                }
            }

        // caption information
        const auto captionProperty = graphNode->GetProperty(L"caption");
        if (captionProperty->IsOk())
            {
            const auto captionLabel = LoadLabel(captionProperty, graph->GetCaption());
            if (captionLabel != nullptr)
                {
                graph->GetCaption() = *captionLabel;
                }
            }

        // background color
        const auto bgColor = ConvertColor(graphNode->GetProperty(L"background-color"));
        if (bgColor.IsOk())
            {
            graph->SetPlotBackgroundColor(bgColor);
            }

        // background image
        if (const auto bgImgNode = graphNode->GetProperty(L"background-image"); bgImgNode->IsOk())
            {
            const auto importNode = bgImgNode->GetProperty(L"image-import");
            const auto bmp = LoadImageFile(importNode);
            if (bmp.IsOk())
                {
                const auto opacity = static_cast<uint8_t>(
                    bgImgNode->GetProperty(L"opacity")->AsDouble(wxALPHA_OPAQUE));
                graph->SetPlotBackgroundImage(wxBitmapBundle(bmp), opacity);

                // image fit
                const auto fitStr = bgImgNode->GetProperty(L"image-fit")->AsString();
                if (!fitStr.empty())
                    {
                    const auto fit = ReportEnumConvert::ConvertImageFit(fitStr);
                    if (fit.has_value())
                        {
                        graph->SetPlotBackgroundImageFit(fit.value());
                        }
                    }

                // cache property templates for round-tripping
                if (importNode->IsValueString())
                    {
                    graph->SetPropertyTemplate(L"image-import.path", importNode->AsString());
                    }
                else if (importNode->IsOk())
                    {
                    const auto pathStr = importNode->GetProperty(L"path")->AsString();
                    if (!pathStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.path", pathStr);
                        }
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
                        graph->SetPropertyTemplate(L"image-import.paths", joined);
                        }
                    const auto stitchStr = importNode->GetProperty(L"stitch")->AsString();
                    if (!stitchStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.stitch", stitchStr);
                        }
                    const auto effectStr = importNode->GetProperty(L"effect")->AsString();
                    if (!effectStr.empty())
                        {
                        graph->SetPropertyTemplate(L"image-import.effect", effectStr);
                        }
                    }
                // cache size
                const auto sizeNode = bgImgNode->GetProperty(L"size");
                if (sizeNode->IsOk())
                    {
                    const auto widthVal = sizeNode->GetProperty(L"width")->AsString();
                    if (!widthVal.empty())
                        {
                        graph->SetPropertyTemplate(L"size.width", widthVal);
                        }
                    const auto heightVal = sizeNode->GetProperty(L"height")->AsString();
                    if (!heightVal.empty())
                        {
                        graph->SetPropertyTemplate(L"size.height", heightVal);
                        }
                    }
                }
            }

        // image scheme
        const auto imageSchemeNode = graphNode->GetProperty(L"image-scheme");
        if (imageSchemeNode->IsOk() && imageSchemeNode->IsValueArray())
            {
            std::vector<wxBitmapBundle> images;
            const auto imgNodes = imageSchemeNode->AsNodes();
            images.reserve(imgNodes.size());
            wxString pathsJoined;
            wxString effectStr;
            for (const auto& imgNode : imgNodes)
                {
                images.emplace_back(LoadImageFile(imgNode));
                // cache file paths for round-tripping
                if (!pathsJoined.empty())
                    {
                    pathsJoined += L"\t";
                    }
                if (imgNode->IsValueString())
                    {
                    pathsJoined += imgNode->AsString();
                    }
                else
                    {
                    const auto nodePath = imgNode->GetProperty(L"path")->AsString();
                    if (!nodePath.empty())
                        {
                        pathsJoined += nodePath;
                        }
                    if (effectStr.empty())
                        {
                        effectStr = imgNode->GetProperty(L"effect")->AsString();
                        }
                    }
                }
            graph->SetImageScheme(
                std::make_shared<Images::Schemes::ImageScheme>(std::move(images)));
            if (!pathsJoined.empty())
                {
                graph->SetPropertyTemplate(L"image-paths", pathsJoined);
                }
            if (!effectStr.empty())
                {
                graph->SetPropertyTemplate(L"image-effect", effectStr);
                }
            }

        // common image outline used for bar charts/box plots
        if (graphNode->HasProperty(L"common-box-image-outline"))
            {
            graph->SetCommonBoxImageOutlineColor(
                ConvertColor(graphNode->GetProperty(L"common-box-image-outline")));
            }

        // stipple brush used for bar charts/box plots
        if (const auto stippleImgNode = graphNode->GetProperty(L"stipple-image");
            stippleImgNode->IsOk())
            {
            graph->SetStippleBrush(LoadImageFile(stippleImgNode));
            // cache file path and effect for round-tripping
            wxString stipplePath;
            wxString stippleEffect;
            if (stippleImgNode->IsValueString())
                {
                stipplePath = stippleImgNode->AsString();
                }
            else
                {
                stipplePath = stippleImgNode->GetProperty(L"path")->AsString();
                stippleEffect = stippleImgNode->GetProperty(L"effect")->AsString();
                }
            if (!stipplePath.empty())
                {
                graph->SetPropertyTemplate(L"image-paths", stipplePath);
                }
            if (!stippleEffect.empty())
                {
                graph->SetPropertyTemplate(L"image-effect", stippleEffect);
                }
            }

        if (const auto stippleShapeNode = graphNode->GetProperty(L"stipple-shape");
            stippleShapeNode->IsOk())
            {
            if (stippleShapeNode->IsValueString())
                {
                const auto iconValue = ReportEnumConvert::ConvertIcon(stippleShapeNode->AsString());
                if (iconValue.has_value())
                    {
                    graph->SetStippleShape(iconValue.value());
                    graph->SetPropertyTemplate(L"stipple-shape", stippleShapeNode->AsString());
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for graph stipple shape."),
                                         stippleShapeNode->AsString())
                            .ToUTF8());
                    }
                }
            else
                {
                const auto iconStr = stippleShapeNode->GetProperty(L"icon")->AsString();
                const auto iconValue = ReportEnumConvert::ConvertIcon(iconStr);
                if (iconValue.has_value())
                    {
                    graph->SetStippleShape(iconValue.value());
                    graph->SetPropertyTemplate(L"stipple-shape", iconStr);
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for graph stipple shape."), iconStr)
                            .ToUTF8());
                    }
                if (const auto stippleShapeColor =
                        ConvertColor(stippleShapeNode->GetProperty(L"color"));
                    stippleShapeColor.IsOk())
                    {
                    graph->SetStippleShapeColor(stippleShapeColor);
                    graph->SetPropertyTemplate(L"stipple-shape-color",
                                               stippleShapeColor.GetAsString(wxC2S_HTML_SYNTAX));
                    }
                }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->AsNodes();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    axisNode->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    if (axisType.value() == AxisType::LeftYAxis)
                        {
                        LoadAxis(axisNode, graph->GetLeftYAxis());
                        }
                    else if (axisType.value() == AxisType::RightYAxis)
                        {
                        LoadAxis(axisNode, graph->GetRightYAxis());
                        }
                    else if (axisType.value() == AxisType::BottomXAxis)
                        {
                        LoadAxis(axisNode, graph->GetBottomXAxis());
                        }
                    else if (axisType.value() == AxisType::TopXAxis)
                        {
                        LoadAxis(axisNode, graph->GetTopXAxis());
                        }
                    }
                }
            }

        // annotations embedded on the plot
        const auto annotationNode = graphNode->GetProperty(L"annotations");
        if (annotationNode->IsOk())
            {
            const auto annotations = annotationNode->AsNodes();
            for (const auto& annotation : annotations)
                {
                auto label = LoadLabel(annotation->GetProperty(L"label"), GraphItems::Label{});
                if (!label)
                    {
                    continue;
                    }
                // add outline and background color if not provided in config file
                if (!label->GetPen().IsOk())
                    {
                    label->GetPen() = wxPen{ Colors::ColorBrewer::GetColor(Colors::Color::Black) };
                    }
                if (!label->GetFontBackgroundColor().IsOk())
                    {
                    label->SetFontBackgroundColor(
                        Colors::ColorContrast::BlackOrWhiteContrast(label->GetFontColor()));
                    }
                label->SetPadding(5, 5, 5, 5);

                const auto interestPointsNode = annotation->GetProperty(L"interest-points");
                if (interestPointsNode->IsOk())
                    {
                    // get all the points on the plot that the note is pointing at
                    std::vector<wxPoint2DDouble> interestPointPositions;
                    const auto interestPoints = interestPointsNode->AsNodes();
                    for (const auto& interestPoint : interestPoints)
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           interestPoint->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           interestPoint->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            interestPointPositions.emplace_back(xPos.value(), yPos.value());
                            }
                        }
                    wxPoint2DDouble anchorPt;
                    const auto anchorNode = annotation->GetProperty(L"anchor");
                    if (anchorNode->IsOk())
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           anchorNode->GetProperty(L"x"));
                        const auto yPos =
                            FindAxisPosition(graph->GetLeftYAxis(), anchorNode->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            anchorPt.m_x = xPos.value();
                            anchorPt.m_y = yPos.value();
                            }
                        }
                    // if no anchor point specified, then use the middle point
                    // of the interest points
                    /// @todo try to add even better logic in here, like how ggrepel works
                    else if (!interestPointPositions.empty())
                        {
                        const auto [minX, maxX] = std::minmax_element(
                            interestPointPositions.cbegin(), interestPointPositions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.m_x < rhv.m_x; });
                        const auto [minY, maxY] = std::minmax_element(
                            interestPointPositions.cbegin(), interestPointPositions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.m_y < rhv.m_y; });
                        anchorPt.m_x = safe_divide(maxX->m_x - minX->m_x, 2.0) + minX->m_x;
                        anchorPt.m_y = safe_divide(maxY->m_y - minY->m_y, 2.0) + minY->m_y;
                        }
                    graph->AddAnnotation(label, anchorPt, interestPointPositions);
                    }
                }
            }

        // reference lines
        const auto referenceLinesNode = graphNode->GetProperty(L"reference-lines");
        if (referenceLinesNode->IsOk())
            {
            const auto refLines = referenceLinesNode->AsNodes();
            for (const auto& refLine : refLines)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refLine->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    wxPen pen{ Colors::ColorBrewer::GetColor(Colors::Color::LightGray), 1,
                               wxPenStyle::wxPENSTYLE_LONG_DASH };
                    LoadPen(refLine->GetProperty(L"pen"), pen);

                    const auto axisPos =
                        FindAxisPosition(graph->GetAxis(axisType.value_or(AxisType::BottomXAxis)),
                                         refLine->GetProperty(L"position"));

                    if (axisPos.has_value())
                        {
                        const auto labelPlacement =
                            ReportEnumConvert::ConvertReferenceLabelPlacement(
                                refLine->GetProperty(L"reference-label-placement")->AsString());
                        graph->AddReferenceLine(GraphItems::ReferenceLine(
                            axisType.value(), axisPos.value(),
                            refLine->GetProperty(L"label")->AsString(), pen,
                            labelPlacement.value_or(ReferenceLabelPlacement::Legend)));
                        }
                    }
                }
            }

        // reference areas
        const auto referenceAreasNode = graphNode->GetProperty(L"reference-areas");
        if (referenceAreasNode->IsOk())
            {
            const auto refAreas = referenceAreasNode->AsNodes();
            for (const auto& refArea : refAreas)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refArea->GetProperty(L"axis-type")->AsString());
                if (axisType.has_value())
                    {
                    wxPen pen{ Colors::ColorBrewer::GetColor(Colors::Color::LightGray), 1,
                               wxPenStyle::wxPENSTYLE_LONG_DASH };
                    LoadPen(refArea->GetProperty(L"pen"), pen);

                    auto& axis = graph->GetAxis(axisType.value_or(AxisType::BottomXAxis));

                    const ReferenceAreaStyle areaStyle =
                        ReportEnumConvert::ConvertReferenceAreaStyle(
                            refArea->GetProperty(L"style")->AsString())
                            .value_or(ReferenceAreaStyle::Solid);

                    const auto axisPos1 = FindAxisPosition(axis, refArea->GetProperty(L"start"));

                    const auto axisPos2 = FindAxisPosition(axis, refArea->GetProperty(L"end"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        graph->AddReferenceArea(GraphItems::ReferenceArea(
                            axisType.value(), axisPos1.value(), axisPos2.value(),
                            refArea->GetProperty(L"label")->AsString(), pen, areaStyle));
                        }
                    }
                }
            }

        // is there a legend?
        const auto legendNode = graphNode->GetProperty(L"legend");
        if (legendNode->IsOk())
            {
            const bool useEnhancedChernoffLegend =
                (graph->IsKindOf(wxCLASSINFO(Graphs::ChernoffFacesPlot)) &&
                 graph->GetPropertyTemplate(L"enhanced-legend") == L"true");
            auto* chernoffPlot = useEnhancedChernoffLegend ?
                                     dynamic_cast<Graphs::ChernoffFacesPlot*>(graph.get()) :
                                     nullptr;
            const auto ringPerimeterStr = legendNode->GetProperty(L"ring")->AsString();
            const auto ringPerimeter =
                (ringPerimeterStr.CmpNoCase(L"inner") == 0 ? Perimeter::Inner : Perimeter::Outer);
            const auto includeHeader = legendNode->GetProperty(L"include-header")->AsBool(true);
            const auto headerLabel = legendNode->GetProperty(L"title")->AsString();
            const auto placement = legendNode->GetProperty(L"placement")->AsString();

            if (useEnhancedChernoffLegend && chernoffPlot != nullptr)
                {
                if (placement.CmpNoCase(L"left") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Left)
                            .PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                        canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"bottom") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Bottom)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"top") == 0)
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}.Placement(Side::Top).PlacementHint(
                            LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                        canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else // right, the default
                    {
                    auto legend = chernoffPlot->CreateEnhancedLegend(
                        Graphs::LegendOptions{}
                            .Placement(Side::Right)
                            .PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                    if (legend != nullptr)
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                }
            else
                {
                if (placement.CmpNoCase(L"left") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Left)
                            .PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                        canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"bottom") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Bottom)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else if (placement.CmpNoCase(L"top") == 0)
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Top)
                            .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                        canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                else // right, the default
                    {
                    auto legend = graph->CreateLegend(
                        Graphs::LegendOptions{}
                            .RingPerimeter(ringPerimeter)
                            .IncludeHeader(includeHeader)
                            .Title(headerLabel)
                            .Placement(Side::Right)
                            .PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                    if (legend != nullptr)
                        {
                        if (!headerLabel.empty())
                            {
                            legend->SetLine(0, headerLabel);
                            }
                        legend->SetIsLegend(true);
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legend));
                        }
                    else
                        {
                        canvas->SetFixedObject(currentRow, currentColumn, graph);
                        }
                    }
                }
            }
        // no legend, so just add the graph
        else
            {
            canvas->SetFixedObject(currentRow, currentColumn, graph);
            }
        }
    } // namespace Wisteria
