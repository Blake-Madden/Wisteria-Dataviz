///////////////////////////////////////////////////////////////////////////////
// Name:        reportbuilder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"
#include "../data/pivot.h"
#include "../data/subset.h"
#include "../graphs/candlestickplot.h"
#include "../graphs/categoricalbarchart.h"
#include "../graphs/heatmap.h"
#include "../graphs/histogram.h"
#include "../graphs/likertchart.h"
#include "../graphs/lrroadmap.h"
#include "../graphs/multi_series_lineplot.h"
#include "../graphs/piechart.h"
#include "../graphs/proconroadmap.h"
#include "../graphs/sankeydiagram.h"
#include "../graphs/waffle_chart.h"
#include "../graphs/wcurveplot.h"
#include "../graphs/win_loss_sparkline.h"
#include "../graphs/wordcloud.h"
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
            wxMessageBox(json->GetLastError(), _(L"Configuration File Parsing Error"),
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
                reportPrintSettings.SetOrientation(wxPrintOrientation::wxLANDSCAPE);
                }
            else if (orientation.CmpNoCase(L"vertical") == 0 ||
                     orientation.CmpNoCase(L"portrait") == 0)
                {
                reportPrintSettings.SetOrientation(wxPrintOrientation::wxPORTRAIT);
                }

            const auto paperSize = ReportEnumConvert::ConvertPaperSize(
                printNode->GetProperty(L"paper-size")->AsString(L"paper-letter"));
            if (paperSize.has_value())
                {
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
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Datasets Section Error"),
                         wxOK | wxICON_WARNING | wxCENTRE);
            return reportPages;
            }

        try
            {
            LoadConstants(json->GetProperty(L"constants"));
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Constants Section Error"), wxOK | wxICON_WARNING | wxCENTRE);
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
                    canvas->SetLabel(ExpandConstants(page->GetProperty(_DT(L"name"))->AsString()));

                    // page numbering
                    if (page->HasProperty(L"page-numbering"))
                        {
                        m_pageNumber = 1;
                        }

                    // watermark (overrides report-level watermark)
                    const auto watermarkPageProperty = page->GetProperty(L"watermark");
                    if (watermarkPageProperty->IsOk())
                        {
                        canvas->SetWatermark(ExpandConstants(
                            watermarkPageProperty->GetProperty(L"label")->AsString()));
                        canvas->SetWatermarkColor(
                            ConvertColor(watermarkPageProperty->GetProperty(L"color")));
                        }
                    else
                        {
                        if (!reportWatermark.empty())
                            {
                            canvas->SetWatermark(reportWatermark);
                            }
                        if (reportWatermarkColor.IsOk())
                            {
                            canvas->SetWatermarkColor(reportWatermarkColor);
                            }
                        }

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
                        // Empty page? Go to next one.
                        if (rows.empty())
                            {
                            delete canvas;
                            continue;
                            }
                        canvas->SetFixedObjectsGridSize(rows.size(), 1);
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
                                                embeddedGraphs.push_back(WinLossSparkline(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(
                                                         L"waffle-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(WaffleChart(
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
                                                         L"categorical-bar-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadCategoricalBarChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->AsString().CmpNoCase(L"label") ==
                                                     0)
                                                {
                                                canvas->SetFixedObject(
                                                    currentRow, currentColumn,
                                                    LoadLabel(item, GraphItems::Label()));
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
                                            wxMessageBox(
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
                                LoadAxis(commonAxisInfo.m_node, *commonAxis);
                                LoadItem(commonAxisInfo.m_node, *commonAxis);
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
    void ReportBuilder::LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush) const
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
                const wxColour brushColor(ConvertColor(brushNode->AsString()));
                if (brushColor.IsOk())
                    {
                    brush.SetColour(brushColor);
                    }
                }
            // or a full definition
            else
                {
                const wxColour brushColor(ConvertColor(brushNode->GetProperty(L"color")));
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
    void ReportBuilder::LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen) const
        {
        static const std::map<std::wstring, wxPenStyle> styleValues = {
            { L"dot", wxPenStyle::wxPENSTYLE_DOT },
            { L"dot-dash", wxPenStyle::wxPENSTYLE_DOT_DASH },
            { L"long-dash", wxPenStyle::wxPENSTYLE_LONG_DASH },
            { L"short-dash", wxPenStyle::wxPENSTYLE_SHORT_DASH },
            { L"solid", wxPenStyle::wxPENSTYLE_SOLID },
            { L"cross-hatch", wxPenStyle::wxPENSTYLE_CROSS_HATCH },
            { L"horizontal-hatch", wxPenStyle::wxPENSTYLE_HORIZONTAL_HATCH },
            { L"vertical-hatch", wxPenStyle::wxPENSTYLE_VERTICAL_HATCH }
        };

        if (penNode->IsOk())
            {
            if (penNode->IsValueNull())
                {
                pen = wxNullPen;
                }
            else
                {
                const wxColour penColor(ConvertColor(penNode->GetProperty(L"color")));
                if (penColor.IsOk())
                    {
                    pen.SetColour(penColor);
                    }

                if (penNode->HasProperty(L"width"))
                    {
                    pen.SetWidth(penNode->GetProperty(L"width")->AsDouble(1));
                    }

                const auto styleStr{ penNode->GetProperty(L"style")->AsString() };
                const auto style = styleValues.find(styleStr.Lower().ToStdWstring());
                if (style != styleValues.cend())
                    {
                    pen.SetStyle(style->second);
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
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis)
        {
        static const std::map<std::wstring_view, GraphItems::Axis::TickMark::DisplayType>
            tickmarkValues = { { L"inner", GraphItems::Axis::TickMark::DisplayType::Inner },
                               { L"outer", GraphItems::Axis::TickMark::DisplayType::Outer },
                               { L"crossed", GraphItems::Axis::TickMark::DisplayType::Crossed },
                               { L"no-display",
                                 GraphItems::Axis::TickMark::DisplayType::NoDisplay } };

        static const std::map<std::wstring_view, AxisLabelDisplay> labelDisplayValues = {
            { L"custom-labels-or-values", AxisLabelDisplay::DisplayCustomLabelsOrValues },
            { L"only-custom-labels", AxisLabelDisplay::DisplayOnlyCustomLabels },
            { L"custom-labels-and-values", AxisLabelDisplay::DisplayCustomLabelsAndValues },
            { L"no-display", AxisLabelDisplay::NoDisplay },
            { L"values", AxisLabelDisplay::DisplayValues }
        };

        static const std::map<std::wstring_view, BracketLineStyle> bracketLineValues = {
            { L"arrow", BracketLineStyle::Arrow },
            { L"lines", BracketLineStyle::Lines },
            { L"reverse-arrow", BracketLineStyle::ReverseArrow },
            { L"curly-braces", BracketLineStyle::CurlyBraces },
            { L"no-connection-lines", BracketLineStyle::NoConnectionLines }
        };

        const auto titleProperty = axisNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty, GraphItems::Label());
            if (titleLabel != nullptr)
                {
                axis.GetTitle() = *titleLabel;
                }
            }
        const auto tickmarksProperty = axisNode->GetProperty(L"tickmarks");
        if (tickmarksProperty->IsOk())
            {
            const auto display = tickmarksProperty->GetProperty(L"display")->AsString().Lower();
            auto foundPos = tickmarkValues.find(std::wstring_view(display.wc_str()));
            if (foundPos != tickmarkValues.cend())
                {
                axis.SetTickMarkDisplay(foundPos->second);
                }
            }
        const auto display = axisNode->GetProperty(L"label-display")->AsString().Lower();
        const auto foundPos = labelDisplayValues.find(std::wstring_view(display.wc_str()));
        if (foundPos != labelDisplayValues.cend())
            {
            axis.SetLabelDisplay(foundPos->second);
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

                    const auto foundBracketStyle = bracketLineValues.find(
                        std::wstring_view(bracket->GetProperty(L"style")->AsString().wc_str()));

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
                            (foundBracketStyle != bracketLineValues.cend()) ?
                                foundBracketStyle->second :
                                BracketLineStyle::CurlyBraces));
                        }
                    }
                }
            // or build a series of brackets from a dataset
            else
                {
                LoadPen(bracketsNode->GetProperty(L"pen"), bracketPen);
                const auto foundBracketStyle = bracketLineValues.find(
                    std::wstring_view(bracketsNode->GetProperty(L"style")->AsString().wc_str()));
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

                    const auto variablesNode = bracketsNode->GetProperty(L"variables");
                    if (variablesNode->IsOk())
                        {
                        const auto labelVarName =
                            ExpandConstants(variablesNode->GetProperty(L"label")->AsString());
                        const auto valueVarName =
                            ExpandConstants(variablesNode->GetProperty(L"value")->AsString());

                        axis.AddBrackets(foundDataset->second, labelVarName, valueVarName);
                        if (bracketPen.IsOk())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                {
                                bracket.GetLinePen() = bracketPen;
                                }
                            }
                        if (foundBracketStyle != bracketLineValues.cend())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                {
                                bracket.SetBracketLineStyle(foundBracketStyle->second);
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
            GraphItems::GraphItemInfo().DPIScaling(m_dpiScaleFactor).Scaling(1.0).Show(false));
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadEmptySpacer() const
        {
        return std::make_shared<GraphItems::Label>(GraphItems::GraphItemInfo()
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
            label->SetText(ExpandConstants(labelNode->AsString()));
            label->GetPen() = wxNullPen;

            return label;
            }
        // a fully defined label
        if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetText(ExpandConstants(labelNode->GetProperty(L"text")->AsString()));
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
                if (const auto img = LoadImageFile(imgNode->GetProperty(L"image-import"));
                    img.IsOk())
                    {
                    label->SetLeftImage(img);
                    }
                }
            // top image
            if (const auto imgNode = labelNode->GetProperty(L"top-image"); imgNode->IsOk())
                {
                label->SetTopImage(LoadImageFile(imgNode->GetProperty(L"image-import")),
                                   imgNode->GetProperty(L"offset")->AsDouble(0));
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
                    }
                }
            }
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

            return wxEmptyString;
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
                             "A continuous or categorical column was expected.");
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
                    const wxString prefix = firstValue.substr(0, firstNumber);
                    double firstDouble{ 0 };
                    if (firstValue.substr(firstNumber).ToCDouble(&firstDouble))
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
            auto merges = mergesNode->AsNodes();
            for (const auto& merge : merges)
                {
                if (merge->IsOk())
                    {
                    const wxString dsName = merge->GetProperty(L"other-dataset")->AsString();
                    const auto foundPos = m_datasets.find(dsName);
                    if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: dataset not found for dataset merging."),
                                             dsName)
                                .ToUTF8());
                        }

                    const auto mergeType =
                        merge->GetProperty(L"type")->AsString(L"left-join-unique");
                    if (mergeType.CmpNoCase(L"left-join-unique") == 0)
                        {
                        std::vector<std::pair<wxString, wxString>> bys;
                        const auto byCols = merge->GetProperty(L"by")->AsNodes();
                        bys.reserve(byCols.size());
                        for (const auto& byCol : byCols)
                            {
                            bys.emplace_back(byCol->GetProperty(L"left-column")->AsString(),
                                             byCol->GetProperty(L"right-column")->AsString());
                            }
                        auto mergedData = Data::DatasetJoin::LeftJoinUnique(
                            datasetToMerge, foundPos->second, bys,
                            merge->GetProperty(L"suffix")->AsString(L".x"));

                        if (mergedData)
                            {
                            m_datasets.insert_or_assign(
                                merge->GetProperty(_DT(L"name"))->AsString(), mergedData);
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
                                   const std::shared_ptr<const Data::Dataset>& parentToPivot)
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
                        auto pivotedData = Data::Pivot::PivotWider(
                            parentToPivot, pivot->GetProperty(L"id-columns")->AsStrings(),
                            pivot->GetProperty(L"names-from-column")->AsString(),
                            pivot->GetProperty(L"values-from-columns")->AsStrings(),
                            pivot->GetProperty(L"names-separator")->AsString(L"_"),
                            pivot->GetProperty(L"names-prefix")->AsString(),
                            pivot->GetProperty(L"fill-value")
                                ->AsDouble(std::numeric_limits<double>::quiet_NaN()));

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(_DT(L"name"))->AsString(), pivotedData);
                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else if (pivotType.CmpNoCase(L"longer") == 0)
                        {
                        auto pivotedData = Data::Pivot::PivotLonger(
                            parentToPivot, pivot->GetProperty(L"columns-to-keep")->AsStrings(),
                            pivot->GetProperty(L"from-columns")->AsStrings(),
                            pivot->GetProperty(L"names-to")->AsStrings(),
                            pivot->GetProperty(L"values-to")->AsString(),
                            pivot->GetProperty(L"names-pattern")->AsString());

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(_DT(L"name"))->AsString(), pivotedData);
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

                    Data::Subset dataSubsetter;
                    std::shared_ptr<Data::Dataset> subsettedDataset{ nullptr };
                    // single column filter
                    if (filterNode->IsOk())
                        {
                        subsettedDataset = dataSubsetter.SubsetSimple(parentToSubset,
                                                                      loadColumnFilter(filterNode));
                        }
                    // ANDed filters
                    else if (filterAndNode->IsOk())
                        {
                        std::vector<Data::ColumnFilterInfo> cf;
                        const auto filterAndNodes = filterAndNode->AsNodes();
                        if (filterAndNodes.empty())
                            {
                            throw std::runtime_error(_(L"Subset missing filters.").ToUTF8());
                            }
                        cf.reserve(filterAndNodes.size());
                        for (const auto& fAndNode : filterAndNodes)
                            {
                            cf.push_back(loadColumnFilter(fAndNode));
                            }

                        subsettedDataset = dataSubsetter.SubsetAnd(parentToSubset, cf);
                        }
                    // ORed filters
                    else if (filterOrNode->IsOk())
                        {
                        std::vector<Data::ColumnFilterInfo> cf;
                        const auto filterOrNodes = filterOrNode->AsNodes();
                        if (filterOrNodes.empty())
                            {
                            throw std::runtime_error(_(L"Subset missing filters.").ToUTF8());
                            }
                        cf.reserve(filterOrNodes.size());
                        for (const auto& fOrNode : filterOrNodes)
                            {
                            cf.push_back(loadColumnFilter(fOrNode));
                            }

                        subsettedDataset = dataSubsetter.SubsetOr(parentToSubset, cf);
                        }
                    else if (sectionNode->IsOk())
                        {
                        subsettedDataset = dataSubsetter.SubsetSection(
                            parentToSubset, sectionNode->GetProperty(_DT(L"column"))->AsString(),
                            sectionNode->GetProperty(L"start-label")->AsString(),
                            sectionNode->GetProperty(L"end-label")->AsString(),
                            sectionNode->GetProperty(L"include-sentinel-labels")->AsBool(true));
                        }

                    if (subsettedDataset)
                        {
                        m_datasets.insert_or_assign(subset->GetProperty(_DT(L"name"))->AsString(),
                                                    subsettedDataset);
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
            // column renaming
            auto colRenames = dsNode->GetProperty(L"columns-rename")->AsNodes();
            for (const auto& colRename : colRenames)
                {
                if (colRename->HasProperty(_DT(L"name")))
                    {
                    dataset->RenameColumn(colRename->GetProperty(_DT(L"name"))->AsString(),
                                          colRename->GetProperty(L"new-name")->AsString());
                    }
                if (colRename->HasProperty(L"name-re"))
                    {
                    dataset->RenameColumnRE(colRename->GetProperty(L"name-re")->AsString(),
                                            colRename->GetProperty(L"new-name-re")->AsString());
                    }
                }

            // column mutations
            auto mutateCats = dsNode->GetProperty(L"mutate-categorical-columns")->AsNodes();
            for (const auto& mutateCat : mutateCats)
                {
                Data::RegExMap reMap;
                const auto replacements = mutateCat->GetProperty(L"replacements")->AsNodes();
                for (const auto& replacement : replacements)
                    {
                    reMap.emplace_back(
                        std::make_unique<wxRegEx>(replacement->GetProperty(L"pattern")->AsString()),
                        replacement->GetProperty(L"replacement")->AsString());
                    }

                dataset->MutateCategoricalColumn(
                    mutateCat->GetProperty(L"source-column")->AsString(),
                    mutateCat->GetProperty(L"target-column")->AsString(), reMap);
                }

            // column SELECT
            auto selectPattern = dsNode->GetProperty(L"columns-select")->AsString();
            if (!selectPattern.empty())
                {
                dataset->SelectColumnsRE(selectPattern);
                }

            // label recoding
            auto recodeREs = dsNode->GetProperty(L"recode-re")->AsNodes();
            for (const auto& recodeRE : recodeREs)
                {
                dataset->RecodeRE(recodeRE->GetProperty(L"column")->AsString(),
                                  recodeRE->GetProperty(L"pattern")->AsString(),
                                  recodeRE->GetProperty(L"replacement")->AsString());
                }

            // category collapsing (min)
            auto collapseMins = dsNode->GetProperty(L"collapse-min")->AsNodes();
            for (const auto& collapseMin : collapseMins)
                {
                dataset->CollapseMin(
                    collapseMin->GetProperty(L"column")->AsString(),
                    collapseMin->GetProperty(L"min")->AsDouble(2),
                    collapseMin->GetProperty(L"other-label")->AsString(_(L"Other")));
                }

            // category collapsing (except)
            auto collapseExcepts = dsNode->GetProperty(L"collapse-except")->AsNodes();
            for (const auto& collapseExcept : collapseExcepts)
                {
                dataset->CollapseExcept(
                    collapseExcept->GetProperty(L"column")->AsString(),
                    collapseExcept->GetProperty(L"labels-to-keep")->AsStrings(),
                    collapseExcept->GetProperty(L"other-label")->AsString(_(L"Other")));
                }

            // load any constants defined with this dataset
            CalcFormulas(dsNode->GetProperty(L"formulas"), dataset);

            // load any subsets of this dataset
            LoadSubsets(dsNode->GetProperty(L"subsets"), dataset);

            // load any pivots of this dataset
            LoadPivots(dsNode->GetProperty(L"pivots"), dataset);

            // load any merges of this dataset
            LoadMerges(dsNode->GetProperty(L"merges"), dataset);

            if (dsNode->GetProperty(L"column-names-sort")->AsBool())
                {
                dataset->SortColumnNames();
                }

            const auto exportPath = dsNode->GetProperty(L"export-path")->AsString();
            // A project silently writing to an arbitrary file is
            // a security threat vector, so only allow that for builds
            // with DEBUG_FILE_IO explicitly set.
            // This should only be used for reviewing the output from a pivot operation
            // when designing a project (in release build).
            if constexpr (Settings::IsDebugFlagEnabled(DebugSettings::AllowFileIO))
                {
                if (!exportPath.empty())
                    {
                    wxFileName fn(exportPath);
                    if (fn.GetPath().empty())
                        {
                        fn = wxFileName(m_configFilePath).GetPathWithSep() + exportPath;
                        }
                    if (fn.GetExt().CmpNoCase(L"csv") == 0)
                        {
                        dataset->ExportCSV(fn.GetFullPath());
                        }
                    else
                        {
                        dataset->ExportTSV(fn.GetFullPath());
                        }
                    }
                }
            else if (!exportPath.empty())
                {
                // just log this (don't throw)
                wxLogWarning(L"Dataset '%s' cannot be exported "
                             "because debug file IO is not enabled.",
                             dataset->GetName());
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
                        path = wxFileName(m_configFilePath).GetPathWithSep() + path;
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
                        dsName = wxFileName(path).GetName();
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

                    const std::variant<wxString, size_t> worksheet =
                        datasetNode->GetProperty(L"worksheet")->IsValueNumber() ?
                            std::variant<wxString, size_t>(static_cast<size_t>(
                                datasetNode->GetProperty(L"worksheet")->AsDouble())) :
                            std::variant<wxString, size_t>(
                                datasetNode->GetProperty(L"worksheet")->AsString());
                    // if no columns are defined, then deduce them ourselves
                    if (!datasetNode->HasProperty(L"id-column") &&
                        !datasetNode->HasProperty(L"date-columns") &&
                        !datasetNode->HasProperty(L"continuous-columns") &&
                        !datasetNode->HasProperty(L"categorical-columns"))
                        {
                        importDefines =
                            Data::Dataset::ImportInfoFromPreview(Data::Dataset::ReadColumnInfo(
                                path, importDefines, std::nullopt, worksheet));
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
                        }

                    // import using the user-provided parser or deduce from the file extension
                    const auto fileExt(wxFileName(path).GetExt());
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
                    else
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a valid importer specified.")).ToUTF8());
                        }

                    m_datasets.insert_or_assign(dsName, dataset);
                    // recode values, build subsets and pivots, etc.
                    LoadDatasetTransformations(datasetNode, dataset);
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
                     std::optional<wxString>(ExpandConstants(
                         variablesNode->GetProperty(L"positive-aggregate")->AsString())) :
                     std::nullopt),
                variablesNode->GetProperty(L"negative")->AsString(),
                (variablesNode->HasProperty(L"negative-aggregate") ?
                     std::optional<wxString>(ExpandConstants(
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
                ExpandConstants(variablesNode->GetProperty(L"task")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"start-date")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"end-date")->AsString()),
                (variablesNode->HasProperty(L"resource") ?
                     std::optional<wxString>(
                         ExpandConstants(variablesNode->GetProperty(L"resource")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"description") ?
                     std::optional<wxString>(
                         ExpandConstants(variablesNode->GetProperty(L"description")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"completion") ?
                     std::optional<wxString>(
                         ExpandConstants(variablesNode->GetProperty(L"completion")->AsString())) :
                     std::nullopt),
                (variablesNode->HasProperty(L"group") ?
                     std::optional<wxString>(
                         ExpandConstants(variablesNode->GetProperty(L"group")->AsString())) :
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
            const auto pValueColumn =
                ExpandConstants(variablesNode->GetProperty(L"p-value")->AsString());
            const auto dvName =
                ExpandConstants(variablesNode->GetProperty(L"dependent-variable-name")->AsString());

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
            lrRoadmap->SetData(
                foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"predictor")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"coefficient")->AsString()),
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
            const auto questionVars =
                ExpandConstants(variablesNode->GetProperty(L"questions")->AsStrings());
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
            const auto groupVarName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());

            // get the survey format
            auto surveyFormat = ReportEnumConvert::ConvertLikertSurveyQuestionFormat(
                graphNode->GetProperty(L"survey-format")->AsString());
            if (!surveyFormat.has_value())
                {
                surveyFormat =
                    Graphs::LikertChart::DeduceScale(foundPos->second, questions, groupVarName);
                }

            if (graphNode->GetProperty(L"simplify")->AsBool())
                {
                surveyFormat = Graphs::LikertChart::Simplify(foundPos->second, questions,
                                                             surveyFormat.value());
                }

            if (graphNode->GetProperty(L"apply-default-labels")->AsBool())
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
            const auto groupVarName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());

            auto wcurvePlot = std::make_shared<Graphs::WCurvePlot>(
                canvas, LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            wcurvePlot->SetData(
                foundPos->second, ExpandConstants(variablesNode->GetProperty(L"y")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"x")->AsString()),
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
                foundPos->second, ExpandConstants(variablesNode->GetProperty(L"date")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"open")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"high")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"low")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"close")->AsString()));

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
            const auto groupVarName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());

            auto linePlot = std::make_shared<Graphs::LinePlot>(
                canvas, LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(
                foundPos->second, ExpandConstants(variablesNode->GetProperty(L"y")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"x")->AsString()),
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
            linePlot->ShowcaseLines(ExpandConstants(showcaseNode->AsStrings()));
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
                canvas, LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(foundPos->second,
                              ExpandConstants(variablesNode->GetProperty(L"y")->AsStrings()),
                              ExpandConstants(variablesNode->GetProperty(L"x")->AsString()));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            LoadLinePlotBaseOptions(graphNode, linePlot.get());

            return linePlot;
            }

        throw std::runtime_error(_(L"Variables not defined for multi-series line plot.").ToUTF8());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::WaffleChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
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
    ReportBuilder::WinLossSparkline(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
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
            const auto postSeason =
                ExpandConstants(variablesNode->GetProperty(L"postseason")->AsString());
            auto wlSparkline = std::make_shared<Graphs::WinLossSparkline>(canvas);
            wlSparkline->SetData(
                foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"season")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"won")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"shutout")->AsString()),
                ExpandConstants(variablesNode->GetProperty(L"home-game")->AsString()),
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
            const auto groupVarName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());

            auto heatmap = std::make_shared<Graphs::HeatMap>(
                canvas, LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            heatmap->SetData(
                foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"continuous")->AsString()),
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
            barChart->ShowcaseBars(ExpandConstants(showcaseNode->AsStrings()), hideGhostedLabels);
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
                        LoadLabel(decal->GetProperty(L"decal"), GraphItems::Label());
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
            for (const auto& barBracket : barBrackets)
                {
                // just log any missing bracket requests and then skip over them
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        barChart->AddFirstBarBracketRE(
                            barBracket->GetProperty(L"start-block-re")->AsString(),
                            barBracket->GetProperty(L"end-block-re")->AsString(),
                            barBracket->GetProperty(L"label")->AsString());
                        }
                    else
                        {
                        barChart->AddFirstBarBracket(
                            barBracket->GetProperty(L"start-block")->AsString(),
                            barBracket->GetProperty(L"end-block")->AsString(),
                            barBracket->GetProperty(L"label")->AsString());
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
            for (const auto& barBracket : barBrackets)
                {
                try
                    {
                    if (barBracket->HasProperty(L"start-block-re") &&
                        barBracket->HasProperty(L"end-block-re"))
                        {
                        barChart->AddLastBarBracketRE(
                            barBracket->GetProperty(L"start-block-re")->AsString(),
                            barBracket->GetProperty(L"end-block-re")->AsString(),
                            barBracket->GetProperty(L"label")->AsString());
                        }
                    else
                        {
                        barChart->AddLastBarBracket(
                            barBracket->GetProperty(L"start-block")->AsString(),
                            barBracket->GetProperty(L"end-block")->AsString(),
                            barBracket->GetProperty(L"label")->AsString());
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
                            path = wxFileName(m_configFilePath).GetPathWithSep() + path;
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
            GraphItems::GraphItemInfo((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}))
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

        return GraphItems::ShapeInfo{}
            .Shape(loadedShape.value())
            .Size(sz)
            .Pen(pen)
            .Brush(brush)
            .Text((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}))
            .Repeat(wxRound(repeat))
            .FillPercent(fillPercent);
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
            GraphItems::GraphItemInfo((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{}))
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
            const auto contVarName =
                ExpandConstants(variablesNode->GetProperty(L"aggregate")->AsString());
            const auto groupName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());

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
            const auto aggVarName =
                ExpandConstants(variablesNode->GetProperty(L"aggregate")->AsString());
            const auto groupName =
                ExpandConstants(variablesNode->GetProperty(L"group")->AsString());
            const auto categoryName =
                ExpandConstants(variablesNode->GetProperty(L"category")->AsString());
            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->AsString());

            auto barChart = std::make_shared<Graphs::CategoricalBarChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));

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
            const auto fromVarName =
                ExpandConstants(variablesNode->GetProperty(L"from")->AsString());
            const auto toColName = ExpandConstants(variablesNode->GetProperty(L"to")->AsString());

            const auto fromWeightVarName =
                ExpandConstants(variablesNode->GetProperty(L"from-weight")->AsString());
            const auto toWeightColName =
                ExpandConstants(variablesNode->GetProperty(L"to-weight")->AsString());

            const auto fromGroupVarName =
                ExpandConstants(variablesNode->GetProperty(L"from-group")->AsString());

            auto sankey = std::make_shared<Graphs::SankeyDiagram>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")));

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
                std::vector<wxString> columnHeader =
                    graphNode->GetProperty(L"column-headers")->AsStrings();
                std::ranges::transform(columnHeader, columnHeader.begin(),
                                       [this](const auto& val) { return ExpandConstants(val); });
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
            const auto aggVarName =
                ExpandConstants(variablesNode->GetProperty(L"aggregate")->AsString());
            const auto wordColName =
                ExpandConstants(variablesNode->GetProperty(L"words")->AsString());

            auto wordCloud = std::make_shared<Graphs::WordCloud>(
                canvas, LoadColorScheme(graphNode->GetProperty(L"color-scheme")));

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
            const auto aggVarName =
                ExpandConstants(variablesNode->GetProperty(L"aggregate")->AsString());
            const auto groupVar1Name =
                ExpandConstants(variablesNode->GetProperty(L"group-1")->AsString());

            auto boxPlot = std::make_shared<Graphs::BoxPlot>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
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
            const auto aggVarName =
                ExpandConstants(variablesNode->GetProperty(L"aggregate")->AsString());
            const auto groupVar1Name =
                ExpandConstants(variablesNode->GetProperty(L"group-1")->AsString());
            const auto groupVar2Name =
                ExpandConstants(variablesNode->GetProperty(L"group-2")->AsString());

            auto pieChart = std::make_shared<Graphs::PieChart>(
                canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
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
                    LoadLabel(graphNode->GetProperty(L"left-margin-note"), GraphItems::Label());
                if (marginLabel != nullptr)
                    {
                    pieChart->GetLeftMarginNote() = *marginLabel;
                    }
                }
            if (graphNode->HasProperty(L"right-margin-note"))
                {
                auto marginLabel =
                    LoadLabel(graphNode->GetProperty(L"right-margin-note"), GraphItems::Label());
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

            pieChart->SetDynamicMargins(graphNode->GetProperty(L"dynamic-margins")->AsBool());

            // showcase the slices
            const auto showcaseNode = graphNode->GetProperty(L"showcase-slices");
            if (showcaseNode->IsValueArray())
                {
                const auto peri = ReportEnumConvert::ConvertPerimeter(
                    graphNode->GetProperty(L"showcased-ring-labels")->AsString());
                pieChart->ShowcaseOuterPieSlices(ExpandConstants(showcaseNode->AsStrings()),
                                                 peri.has_value() ? peri.value() :
                                                                    Perimeter::Outer);
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
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadTable(const wxSimpleJSON::Ptr_t& graphNode,
                                                              Canvas* canvas, size_t& currentRow,
                                                              size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for table."), dsName).ToUTF8());
            }

        std::vector<wxString> variables;
        const auto variablesNode = graphNode->GetProperty(L"variables");
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
        const auto borderDefaults = graphNode->GetProperty(L"default-borders")->AsBools();
        table->SetDefaultBorders((!borderDefaults.empty() ? borderDefaults[0] : true),
                                 (borderDefaults.size() > 1 ? borderDefaults[1] : true),
                                 (borderDefaults.size() > 2 ? borderDefaults[2] : true),
                                 (borderDefaults.size() > 3 ? borderDefaults[3] : true));

        table->SetData(foundPos->second, variables, graphNode->GetProperty(L"transpose")->AsBool());

        // sorting
        const auto sortNode = graphNode->GetProperty(L"row-sort");
        if (sortNode->IsOk())
            {
            const auto sortDirection =
                sortNode->GetProperty(L"direction")->AsString().CmpNoCase(_DT(L"descending")) == 0 ?
                    SortDirection::SortDescending :
                    SortDirection::SortAscending;
            const std::optional<size_t> sortColumn =
                LoadTablePosition(sortNode->GetProperty(L"column"), table);

            // if sorting by a list of labels with a custom order
            if (sortColumn)
                {
                if (const auto labelsNode = sortNode->GetProperty(L"labels");
                    labelsNode->IsOk() && labelsNode->IsValueArray())
                    {
                    table->Sort(sortColumn.value(), labelsNode->AsStrings(), sortDirection);
                    }
                else
                    {
                    table->Sort(sortColumn.value(), sortDirection);
                    }
                }
            }

        if (graphNode->HasProperty(L"link-id"))
            {
            if (const auto linkId = ConvertNumber(graphNode->GetProperty(L"link-id")))
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
                }
            }

        table->ClearTrailingRowFormatting(
            graphNode->GetProperty(L"clear-trailing-row-formatting")->AsBool());

        const auto minWidthProp = graphNode->GetProperty(L"min-width-proportion");
        if (minWidthProp->IsOk())
            {
            table->SetMinWidthProportion(minWidthProp->AsDouble());
            }
        const auto minHeightProp = graphNode->GetProperty(L"min-height-proportion");
        if (minHeightProp->IsOk())
            {
            table->SetMinHeightProportion(minHeightProp->AsDouble());
            }

        LoadPen(graphNode->GetProperty(L"highlight-pen"), table->GetHighlightPen());

        // reads a single position and range of positions (start and end)
        const auto readPositions = [&table](const wxSimpleJSON::Ptr_t& theNode)
        {
            const std::optional<size_t> position =
                LoadTablePosition(theNode->GetProperty(L"position"), table);
            const std::optional<size_t> startPosition =
                LoadTablePosition(theNode->GetProperty(L"start"), table);
            const std::optional<size_t> endPosition =
                LoadTablePosition(theNode->GetProperty(L"end"), table);
            return std::tuple(position, startPosition, endPosition);
        };

        // loads the positions from a row or column stops array
        const auto loadStops = [&table](const auto& stopsNode)
        {
            std::set<size_t> rowOrColumnStops;
            const auto stops = stopsNode->AsNodes();
            if (stops.size())
                {
                for (const auto& stop : stops)
                    {
                    const std::optional<size_t> stopPosition =
                        LoadTablePosition(stop->GetProperty(L"position"), table);
                    if (stopPosition.has_value())
                        {
                        rowOrColumnStops.insert(stopPosition.value());
                        }
                    }
                }
            return rowOrColumnStops;
        };

        if (graphNode->HasProperty(L"insert-group-header"))
            {
            table->InsertGroupHeader(graphNode->GetProperty(L"insert-group-header")->AsStrings());
            }

        // group the rows
        const auto rowGroupings = graphNode->GetProperty(L"row-group")->AsDoubles();
        for (const auto& rowGrouping : rowGroupings)
            {
            table->GroupRow(rowGrouping);
            }

        // group the columns
        const auto columnGroupings = graphNode->GetProperty(L"column-group")->AsDoubles();
        for (const auto& columnGrouping : columnGroupings)
            {
            table->GroupColumn(columnGrouping);
            }

        // apply zebra stripes to loaded data before we start adding custom rows/columns, manually
        // changing row/column colors, etc.
        if (graphNode->HasProperty(L"alternate-row-color"))
            {
            const auto altRowColorNode = graphNode->GetProperty(L"alternate-row-color");
            const auto startRow = LoadTablePosition(altRowColorNode->GetProperty(L"start"), table);
            const std::set<size_t> colStops = loadStops(altRowColorNode->GetProperty(L"stops"));
            auto rowColor{ ConvertColor(altRowColorNode->GetProperty(L"color")) };
            if (!rowColor.IsOk())
                {
                rowColor = Colors::ColorBrewer::GetColor(Colors::Color::White);
                }
            table->ApplyAlternateRowColors(rowColor, startRow.value_or(0), colStops);
            }

        // add rows
        auto rowAddCommands = graphNode->GetProperty(L"row-add")->AsNodes();
        if (!rowAddCommands.empty())
            {
            for (const auto& rowAddCommand : rowAddCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(rowAddCommand->GetProperty(L"position"), table);
                if (!position.has_value())
                    {
                    continue;
                    }
                table->InsertRow(position.value());
                // fill the values across the row
                const auto values = rowAddCommand->GetProperty(L"values")->AsStrings();
                for (size_t i = 0; i < values.size(); ++i)
                    {
                    table->GetCell(position.value(), i).SetValue(values[i]);
                    }
                const wxColour bgcolor(ConvertColor(rowAddCommand->GetProperty(L"background")));
                if (bgcolor.IsOk())
                    {
                    table->SetRowBackgroundColor(position.value(), bgcolor, std::nullopt);
                    }
                }
            }

        // change the rows' suppression
        const auto rowSuppressionCommands = graphNode->GetProperty(L"row-suppression")->AsNodes();
        if (!rowSuppressionCommands.empty())
            {
            for (const auto& rowSuppressionCommand : rowSuppressionCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(rowSuppressionCommand);
                const auto threshold =
                    ConvertNumber(rowSuppressionCommand->GetProperty(L"threshold"));
                const auto suppressionLabel =
                    ExpandConstants(rowSuppressionCommand->GetProperty(L"label")->AsString());

                const std::set<size_t> colStops =
                    loadStops(rowSuppressionCommand->GetProperty(L"stops"));
                if (threshold.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowSuppression(position.value(), threshold,
                                                 !suppressionLabel.empty() ?
                                                     std::optional<wxString>(suppressionLabel) :
                                                     std::nullopt,
                                                 colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowSuppression(i, threshold,
                                                     !suppressionLabel.empty() ?
                                                         std::optional<wxString>(suppressionLabel) :
                                                         std::nullopt,
                                                     colStops);
                            }
                        }
                    }
                }
            }

        // change the rows' formatting
        const auto rowFormattingCommands = graphNode->GetProperty(L"row-formatting")->AsNodes();
        if (!rowFormattingCommands.empty())
            {
            for (const auto& rowFormattingCommand : rowFormattingCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(rowFormattingCommand);
                const auto formatValue = ReportEnumConvert::ConvertTableCellFormat(
                    rowFormattingCommand->GetProperty(L"format")->AsString());

                const std::set<size_t> colStops =
                    loadStops(rowFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowFormat(position.value(), formatValue.value(), colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowFormat(i, formatValue.value(), colStops);
                            }
                        }
                    }
                }
            }

        // color the rows
        const auto rowColorCommands = graphNode->GetProperty(L"row-color")->AsNodes();
        if (!rowColorCommands.empty())
            {
            for (const auto& rowColorCommand : rowColorCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowColorCommand);
                const wxColour bgcolor(ConvertColor(rowColorCommand->GetProperty(L"background")));
                const std::set<size_t> colStops = loadStops(rowColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowBackgroundColor(position.value(), bgcolor, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBackgroundColor(i, bgcolor, colStops);
                            }
                        }
                    }
                }
            }

        // bold the rows
        const auto rowBoldCommands = graphNode->GetProperty(L"row-bold")->AsNodes();
        if (!rowBoldCommands.empty())
            {
            for (const auto& rowBoldCommand : rowBoldCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowBoldCommand);
                const std::set<size_t> colStops = loadStops(rowBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    {
                    table->BoldRow(position.value(), colStops);
                    }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->BoldRow(i, colStops);
                        }
                    }
                }
            }

        // change rows' borders
        const auto rowBordersCommands = graphNode->GetProperty(L"row-borders")->AsNodes();
        if (!rowBordersCommands.empty())
            {
            for (const auto& rowBordersCommand : rowBordersCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(rowBordersCommand);
                const auto borderFlags = rowBordersCommand->GetProperty(L"borders")->AsBools();

                const std::set<size_t> rowStops =
                    loadStops(rowBordersCommand->GetProperty(L"stops"));
                if (!borderFlags.empty())
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(
                            position.value(),
                            (!borderFlags.empty() ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] :
                                                      table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] :
                                                      table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] :
                                                      table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(
                                i,
                                (!borderFlags.empty() ? borderFlags[0] :
                                                        table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] :
                                                          table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] :
                                                          table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] :
                                                          table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }

                // borders specified individually
                if (rowBordersCommand->HasProperty(L"top-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                                             rowBordersCommand->GetProperty(L"top-border")
                                                 ->AsBool(table->IsShowingTopBorder()),
                                             std::nullopt, std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                                 rowBordersCommand->GetProperty(L"top-border")
                                                     ->AsBool(table->IsShowingTopBorder()),
                                                 std::nullopt, std::nullopt, std::nullopt,
                                                 rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt,
                                             rowBordersCommand->GetProperty(L"right-border")
                                                 ->AsBool(table->IsShowingRightBorder()),
                                             std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"right-border")
                                                     ->AsBool(table->IsShowingRightBorder()),
                                                 std::nullopt, std::nullopt, rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt, std::nullopt,
                                             rowBordersCommand->GetProperty(L"bottom-border")
                                                 ->AsBool(table->IsShowingBottomBorder()),
                                             std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"bottom-border")
                                                     ->AsBool(table->IsShowingBottomBorder()),
                                                 std::nullopt);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(), std::nullopt, std::nullopt,
                                             std::nullopt,
                                             rowBordersCommand->GetProperty(L"left-border")
                                                 ->AsBool(table->IsShowingLeftBorder()),
                                             rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i, std::nullopt, std::nullopt, std::nullopt,
                                                 rowBordersCommand->GetProperty(L"left-border")
                                                     ->AsBool(table->IsShowingLeftBorder()),
                                                 rowStops);
                            }
                        }
                    }
                }
            }

        // change rows' content alignment
        const auto rowContentCommands = graphNode->GetProperty(L"row-content-align")->AsNodes();
        if (!rowContentCommands.empty())
            {
            for (const auto& rowContentCommand : rowContentCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(rowContentCommand);
                const auto hPageAlignment =
                    rowContentCommand->GetProperty(L"horizontal-page-alignment")->AsString();
                const std::set<size_t> colStops =
                    loadStops(rowContentCommand->GetProperty(L"stops"));
                if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::LeftAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::LeftAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::RightAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::RightAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(
                            position.value(), PageHorizontalAlignment::Centered, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(
                                i, PageHorizontalAlignment::Centered, colStops);
                            }
                        }
                    }
                }
            }

        // change the columns' suppression
        const auto columnSuppressionCommands =
            graphNode->GetProperty(L"column-suppression")->AsNodes();
        if (!columnSuppressionCommands.empty())
            {
            for (const auto& columnSuppressionCommand : columnSuppressionCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(columnSuppressionCommand);
                const auto threshold =
                    ConvertNumber(columnSuppressionCommand->GetProperty(L"threshold"));
                const auto suppressionLabel =
                    ExpandConstants(columnSuppressionCommand->GetProperty(L"label")->AsString());

                const std::set<size_t> rowStops =
                    loadStops(columnSuppressionCommand->GetProperty(L"stops"));
                if (threshold.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetColumnSuppression(position.value(), threshold,
                                                    !suppressionLabel.empty() ?
                                                        std::optional<wxString>(suppressionLabel) :
                                                        std::nullopt,
                                                    rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnSuppression(
                                i, threshold,
                                !suppressionLabel.empty() ?
                                    std::optional<wxString>(suppressionLabel) :
                                    std::nullopt,
                                rowStops);
                            }
                        }
                    }
                }
            }

        // change columns' cell formatting
        const auto columnFormattingCommands =
            graphNode->GetProperty(L"column-formatting")->AsNodes();
        if (!columnFormattingCommands.empty())
            {
            for (const auto& columnFormattingCommand : columnFormattingCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(columnFormattingCommand);
                const auto formatValue = ReportEnumConvert::ConvertTableCellFormat(
                    columnFormattingCommand->GetProperty(L"format")->AsString());

                const std::set<size_t> rowStops =
                    loadStops(columnFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetColumnFormat(position.value(), formatValue.value(), rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnFormat(i, formatValue.value(), rowStops);
                            }
                        }
                    }
                }
            }

        // color the columns
        const auto colColorCommands = graphNode->GetProperty(L"column-color")->AsNodes();
        if (!colColorCommands.empty())
            {
            for (const auto& colColorCommand : colColorCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(colColorCommand);
                const wxColour bgcolor(ConvertColor(colColorCommand->GetProperty(L"background")));
                const std::set<size_t> rowStops = loadStops(colColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBackgroundColor(position.value(), bgcolor, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBackgroundColor(i, bgcolor, rowStops);
                            }
                        }
                    }
                }
            }

        // bold the columns
        const auto colBoldCommands = graphNode->GetProperty(L"column-bold")->AsNodes();
        if (!colBoldCommands.empty())
            {
            for (const auto& colBoldCommand : colBoldCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(colBoldCommand);
                const std::set<size_t> rowStops = loadStops(colBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    {
                    table->BoldColumn(position.value(), rowStops);
                    }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->BoldColumn(i, rowStops);
                        }
                    }
                }
            }

        // change columns' borders
        const auto columnBordersCommands = graphNode->GetProperty(L"column-borders")->AsNodes();
        if (!columnBordersCommands.empty())
            {
            for (const auto& columnBordersCommand : columnBordersCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(columnBordersCommand);
                const auto borderFlags = columnBordersCommand->GetProperty(L"borders")->AsBools();

                const std::set<size_t> rowStops =
                    loadStops(columnBordersCommand->GetProperty(L"stops"));
                if (!borderFlags.empty())
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(
                            position.value(),
                            (!borderFlags.empty() ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] :
                                                      table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] :
                                                      table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] :
                                                      table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i,
                                (!borderFlags.empty() ? borderFlags[0] :
                                                        table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] :
                                                          table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] :
                                                          table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] :
                                                          table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }

                // borders specified individually
                if (columnBordersCommand->HasProperty(L"top-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                                                columnBordersCommand->GetProperty(L"top-border")
                                                    ->AsBool(table->IsShowingTopBorder()),
                                                std::nullopt, std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                                    columnBordersCommand->GetProperty(L"top-border")
                                                        ->AsBool(table->IsShowingTopBorder()),
                                                    std::nullopt, std::nullopt, std::nullopt,
                                                    rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt,
                                                columnBordersCommand->GetProperty(L"right-border")
                                                    ->AsBool(table->IsShowingRightBorder()),
                                                std::nullopt, std::nullopt, rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt,
                                columnBordersCommand->GetProperty(L"right-border")
                                    ->AsBool(table->IsShowingRightBorder()),
                                std::nullopt, std::nullopt, rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt, std::nullopt,
                                                columnBordersCommand->GetProperty(L"bottom-border")
                                                    ->AsBool(table->IsShowingBottomBorder()),
                                                std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt, std::nullopt,
                                columnBordersCommand->GetProperty(L"bottom-border")
                                    ->AsBool(table->IsShowingBottomBorder()),
                                std::nullopt);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(), std::nullopt, std::nullopt,
                                                std::nullopt,
                                                columnBordersCommand->GetProperty(L"left-border")
                                                    ->AsBool(table->IsShowingLeftBorder()),
                                                rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(
                                i, std::nullopt, std::nullopt, std::nullopt,
                                columnBordersCommand->GetProperty(L"left-border")
                                    ->AsBool(table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }
                }
            }

        // highlight cells down a column
        const auto columnHighlightsCommands =
            graphNode->GetProperty(L"column-highlight")->AsNodes();
        if (!columnHighlightsCommands.empty())
            {
            for (const auto& columnHighlightsCommand : columnHighlightsCommands)
                {
                const auto [position, startPosition, endPosition] =
                    readPositions(columnHighlightsCommand);

                std::set<size_t> rowStops =
                    loadStops(columnHighlightsCommand->GetProperty(L"stops"));
                if (position.has_value())
                    {
                    table->HighlightColumn(position.value(), rowStops);
                    }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        {
                        table->HighlightColumn(i, rowStops);
                        }
                    }
                }
            }

        // column/row aggregates
        const auto columnRowAggregates = graphNode->GetProperty(L"aggregates")->AsNodes();
        if (!columnRowAggregates.empty())
            {
            for (const auto& columnRowAggregate : columnRowAggregates)
                {
                const auto aggName = columnRowAggregate->GetProperty(_DT(L"name"))->AsString();
                const auto whereType = columnRowAggregate->GetProperty(L"type")->AsString();
                const auto aggType = columnRowAggregate->GetProperty(L"aggregate-type")->AsString();

                // starting column/row
                const std::optional<size_t> startColumn =
                    LoadTablePosition(columnRowAggregate->GetProperty(L"start"), table);
                // ending column/row
                const std::optional<size_t> endingColumn =
                    LoadTablePosition(columnRowAggregate->GetProperty(L"end"), table);
                // position
                const std::optional<size_t> insertionPosition =
                    LoadTablePosition(columnRowAggregate->GetProperty(L"position"), table);
                // (optional) overriding cell borders
                const auto cellBorders = columnRowAggregate->GetProperty(L"borders")->AsBools();
                std::bitset<4> borders{ 0 };
                borders[0] = (!cellBorders.empty() ? cellBorders[0] : table->IsShowingTopBorder());
                borders[1] =
                    (cellBorders.size() > 1 ? cellBorders[1] : table->IsShowingRightBorder());
                borders[2] =
                    (cellBorders.size() > 2 ? cellBorders[2] : table->IsShowingBottomBorder());
                borders[3] =
                    (cellBorders.size() > 3 ? cellBorders[3] : table->IsShowingLeftBorder());

                const wxColour bkColor(
                    ConvertColor(columnRowAggregate->GetProperty(L"background")));

                Graphs::Table::AggregateInfo aggInfo;
                if (startColumn.has_value())
                    {
                    aggInfo.FirstCell(startColumn.value());
                    }
                if (endingColumn.has_value())
                    {
                    aggInfo.LastCell(endingColumn.value());
                    }

                if (aggType.CmpNoCase(L"percent-change") == 0)
                    {
                    aggInfo.Type(AggregateType::ChangePercent);
                    }
                else if (aggType.CmpNoCase(L"change") == 0)
                    {
                    aggInfo.Type(AggregateType::Change);
                    }
                else if (aggType.CmpNoCase(L"total") == 0)
                    {
                    aggInfo.Type(AggregateType::Total);
                    }
                else if (aggType.CmpNoCase(L"ratio") == 0)
                    {
                    aggInfo.Type(AggregateType::Ratio);
                    }
                // invalid agg column type
                else
                    {
                    continue;
                    }

                if (whereType.CmpNoCase(L"column") == 0)
                    {
                    table->InsertAggregateColumn(
                        aggInfo, aggName, insertionPosition,
                        columnRowAggregate->GetProperty(L"use-adjacent-color")->AsBool(),
                        (bkColor.IsOk() ? std::optional<wxColour>(bkColor) : std::nullopt),
                        (!cellBorders.empty() ? std::optional<std::bitset<4>>(borders) :
                                                std::nullopt));
                    }
                else if (whereType.CmpNoCase(L"row") == 0)
                    {
                    table->InsertAggregateRow(
                        aggInfo, aggName, insertionPosition,
                        (bkColor.IsOk() ? std::optional<wxColour>(bkColor) : std::nullopt),
                        (!cellBorders.empty() ? std::optional<std::bitset<4>>(borders) :
                                                std::nullopt));
                    }
                }
            }

        // row totals
        const auto rowTotals = graphNode->GetProperty(L"row-totals");
        if (rowTotals->IsOk())
            {
            const wxColour bkColor(ConvertColor(rowTotals->GetProperty(L"background")));
            table->InsertRowTotals(bkColor.IsOk() ? std::optional<wxColour>(bkColor) :
                                                    std::nullopt);
            }

        // cell updating
        const auto cellUpdates = graphNode->GetProperty(L"cell-update")->AsNodes();
        if (!cellUpdates.empty())
            {
            for (const auto& cellUpdate : cellUpdates)
                {
                // last column and row will be the last aggregates at this point
                // (if applicable)
                const std::optional<size_t> rowPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"row"), table);
                const std::optional<size_t> columnPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"column"), table);
                Graphs::Table::TableCell* currentCell =
                    ((rowPosition.has_value() && columnPosition.has_value() &&
                      rowPosition.value() < table->GetRowCount() &&
                      columnPosition.value() < table->GetColumnCount()) ?
                         &table->GetCell(rowPosition.value(), columnPosition.value()) :
                         table->FindCell(cellUpdate->GetProperty(L"value-to-find")->AsString()));

                if (currentCell != nullptr)
                    {
                    // column count
                    const auto columnCountProperty = cellUpdate->GetProperty(L"column-count");
                    if (columnCountProperty->IsOk())
                        {
                        if (columnCountProperty->IsValueString() &&
                            columnCountProperty->AsString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetColumnCount(table->GetColumnCount());
                            }
                        else if (columnCountProperty->IsValueNumber())
                            {
                            currentCell->SetColumnCount(columnCountProperty->AsDouble());
                            }
                        }
                    // row count
                    const auto rowCountProperty = cellUpdate->GetProperty(L"row-count");
                    if (rowCountProperty->IsOk())
                        {
                        if (rowCountProperty->IsValueString() &&
                            rowCountProperty->AsString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetRowCount(table->GetRowCount());
                            }
                        else if (rowCountProperty->IsValueNumber())
                            {
                            currentCell->SetRowCount(rowCountProperty->AsDouble());
                            }
                        }
                    // value
                    const auto valueProperty = cellUpdate->GetProperty(L"value");
                    if (valueProperty->IsOk())
                        {
                        if (valueProperty->IsValueString())
                            {
                            currentCell->SetValue(valueProperty->AsString());
                            }
                        else if (valueProperty->IsValueNumber())
                            {
                            currentCell->SetValue(valueProperty->AsDouble());
                            }
                        else if (valueProperty->IsValueNull())
                            {
                            currentCell->SetValue(wxEmptyString);
                            }
                        }
                    // background color
                    const wxColour bgcolor(ConvertColor(cellUpdate->GetProperty(L"background")));
                    if (bgcolor.IsOk())
                        {
                        currentCell->SetBackgroundColor(bgcolor);
                        }

                    // an image to the left side of it
                    const auto leftSideNode = cellUpdate->GetProperty(L"left-image");
                    if (leftSideNode->IsOk())
                        {
                        auto path = leftSideNode->GetProperty(L"path")->AsString();
                        if (!path.empty())
                            {
                            if (!wxFileName::FileExists(path))
                                {
                                path = wxFileName(m_configFilePath).GetPathWithSep() + path;
                                if (!wxFileName::FileExists(path))
                                    {
                                    throw std::runtime_error(
                                        wxString::Format(_(L"%s: label side image not found."),
                                                         path)
                                            .ToUTF8());
                                    }
                                }
                            currentCell->SetLeftImage(GraphItems::Image::LoadFile(path));
                            }
                        }

                    // prefix
                    if (cellUpdate->HasProperty(L"prefix"))
                        {
                        currentCell->SetPrefix(cellUpdate->GetProperty(L"prefix")->AsString());
                        }

                    // is it highlighted
                    if (cellUpdate->HasProperty(L"highlight"))
                        {
                        currentCell->Highlight(cellUpdate->GetProperty(L"highlight")->AsBool());
                        }

                    // font attributes
                    if (cellUpdate->HasProperty(L"bold"))
                        {
                        if (cellUpdate->GetProperty(L"bold")->AsBool())
                            {
                            currentCell->GetFont().MakeBold();
                            }
                        else
                            {
                            currentCell->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                            }
                        }

                    // outer border toggles
                    const auto outerBorderToggles =
                        cellUpdate->GetProperty(L"show-borders")->AsBools();
                    if (!outerBorderToggles.empty())
                        {
                        currentCell->ShowTopBorder(outerBorderToggles[0]);
                        }
                    if (outerBorderToggles.size() >= 2)
                        {
                        currentCell->ShowRightBorder(outerBorderToggles[1]);
                        }
                    if (outerBorderToggles.size() >= 3)
                        {
                        currentCell->ShowBottomBorder(outerBorderToggles[2]);
                        }
                    if (outerBorderToggles.size() >= 4)
                        {
                        currentCell->ShowLeftBorder(outerBorderToggles[3]);
                        }

                    const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                        cellUpdate->GetProperty(L"text-alignment")->AsString());
                    if (textAlignment.has_value())
                        {
                        currentCell->SetTextAlignment(textAlignment.value());
                        }

                    // horizontal page alignment
                    const auto hPageAlignment =
                        cellUpdate->GetProperty(L"horizontal-page-alignment")->AsString();
                    if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                        {
                        currentCell->SetPageHorizontalAlignment(
                            PageHorizontalAlignment::LeftAligned);
                        }
                    else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                        {
                        currentCell->SetPageHorizontalAlignment(
                            PageHorizontalAlignment::RightAligned);
                        }
                    else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                        {
                        currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
                        }
                    }
                }
            }

        const auto annotationsNode = graphNode->GetProperty(L"cell-annotations")->AsNodes();
        if (!annotationsNode.empty())
            {
            for (const auto& annotation : annotationsNode)
                {
                Graphs::Table::CellAnnotation cellAnnotation{
                    annotation->GetProperty(L"value")->AsString(),
                    std::vector<Graphs::Table::CellPosition>{}, Side::Right, std::nullopt,
                    wxColour{}
                };
                if (annotation->HasProperty(L"side"))
                    {
                    cellAnnotation.m_side =
                        (annotation->GetProperty(L"side")->AsString().CmpNoCase(L"left") == 0) ?
                            Side::Left :
                            Side::Right;
                    }
                cellAnnotation.m_connectionLinePen = table->GetHighlightPen();
                LoadPen(annotation->GetProperty(L"pen"),
                        cellAnnotation.m_connectionLinePen.value());
                cellAnnotation.m_bgColor = ConvertColor(annotation->GetProperty(L"background"));

                const auto cellsNode = annotation->GetProperty(L"cells");
                if (cellsNode->IsOk() && cellsNode->IsValueObject())
                    {
                    const auto outliersNode = cellsNode->GetProperty(L"column-outliers");
                    const auto topNNode = cellsNode->GetProperty(L"column-top-n");
                    const auto rangeNode = cellsNode->GetProperty(L"range");
                    if (outliersNode->IsOk() && outliersNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(outliersNode->AsString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetOutliers(colIndex.value());
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    else if (topNNode->IsOk() && topNNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(topNNode->AsString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetTopN(
                                colIndex.value(), cellsNode->GetProperty(L"n")->AsDouble(1));
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    else if (rangeNode->IsOk() && rangeNode->HasProperty(L"start") &&
                             rangeNode->HasProperty(L"end"))
                        {
                        const auto startCell =
                            table->FindCellPosition(rangeNode->GetProperty(L"start")->AsString());
                        const auto endCell =
                            table->FindCellPosition(rangeNode->GetProperty(L"end")->AsString());
                        if (startCell && endCell)
                            {
                            cellAnnotation.m_cells = { startCell.value(), endCell.value() };
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    }
                }
            }

        // assign footnotes after all cells have been updated
        const auto footnotesNode = graphNode->GetProperty(L"footnotes")->AsNodes();
        if (!footnotesNode.empty())
            {
            for (const auto& ftNode : footnotesNode)
                {
                table->AddFootnote(ExpandConstants(ftNode->GetProperty(L"value")->AsString()),
                                   ExpandConstants(ftNode->GetProperty(L"footnote")->AsString()));
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, table);
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
                                                      wxEmptyString);
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
    wxColour ReportBuilder::ConvertColor(wxString colorStr) const
        {
        long opacity{ wxALPHA_OPAQUE };
        if (const auto colonPos = colorStr.find(L':'); colonPos != wxString::npos)
            {
            colorStr.substr(colonPos + 1).ToLong(&opacity);
            colorStr = colorStr.substr(0, colonPos);
            }
        // in case the color is a user-defined constant in the file
        colorStr = ExpandConstants(colorStr);

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
    std::optional<size_t> ReportBuilder::LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
                                                           std::shared_ptr<Graphs::Table> table)
        {
        if (!positionNode->IsOk())
            {
            return std::nullopt;
            }

        std::optional<size_t> position;

        const auto loadStringToPosition = [&](const auto& originStr)
        {
            if (originStr.CmpNoCase(L"last-column") == 0)
                {
                position = table->GetLastDataColumn();
                }
            else if (originStr.CmpNoCase(L"last-row") == 0)
                {
                position = table->GetLastDataRow();
                }
            else if (originStr.StartsWith(L"column:"))
                {
                if (const auto colPos =
                        (table ? table->FindColumnIndex(originStr.substr(7)) : std::nullopt);
                    colPos.has_value())
                    {
                    position = colPos;
                    }
                }
            else if (originStr.StartsWith(L"row:"))
                {
                if (const auto colPos =
                        (table ? table->FindRowIndex(originStr.substr(4)) : std::nullopt);
                    colPos.has_value())
                    {
                    position = colPos;
                    }
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown table position origin value."), originStr)
                        .ToUTF8());
                }
        };

        const auto origin = positionNode->GetProperty(L"origin");
        if (origin->IsOk())
            {
            if (origin->IsValueString())
                {
                loadStringToPosition(origin->AsString());
                }
            else if (origin->IsValueNumber())
                {
                position = origin->AsDouble();
                }
            }
        else if (positionNode->IsValueString())
            {
            loadStringToPosition(positionNode->AsString());
            }
        else if (positionNode->IsValueNumber())
            {
            position = positionNode->AsDouble();
            }
        const std::optional<double> doubleStartOffset =
            positionNode->HasProperty(L"offset") ?
                std::optional<double>(positionNode->GetProperty(L"offset")->AsDouble()) :
                std::nullopt;
        if (position.has_value() && doubleStartOffset.has_value())
            {
            // value() should work here, but GCC throws an
            // 'uninitialized value' false positive warning
            position = position.value_or(0) + doubleStartOffset.value_or(0);
            }

        return position;
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
        // just a named color scheme
        else if (brushSchemeNode->IsValueString())
            {
            if (const auto colorScheme = LoadColorScheme(brushSchemeNode))
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(*colorScheme);
                }
            }
        return nullptr;
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
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring_view, LineStyle> lineStyleEnums = {
            { L"arrows", LineStyle::Arrows },
            { L"lines", LineStyle::Lines },
            { L"spline", LineStyle::Spline }
        };

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
                const auto foundPos = lineStyleEnums.find(std::wstring_view(
                    lineStyle->GetProperty(L"line-style")->AsString().MakeLower().wc_str()));
                if (foundPos != lineStyleEnums.cend())
                    {
                    lineStyles.emplace_back(pn.GetStyle(), foundPos->second);
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
            expandedPath = wxFileName(m_configFilePath).GetPathWithSep() + expandedPath;
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
        static const std::map<std::wstring_view, ResizeMethod> resizeValues = {
            { L"downscale-only", ResizeMethod::DownscaleOnly },
            { L"downscale-or-upscale", ResizeMethod::DownscaleOrUpscale },
            { L"upscale-only", ResizeMethod::UpscaleOnly },
            { L"no-resize", ResizeMethod::NoResize },
        };

        const auto bmp = LoadImageFile(imageNode->GetProperty(L"image-import"));
        auto image = std::make_unique<GraphItems::Image>(bmp.ConvertToImage());
        if (image->IsOk())
            {
            wxSize sz{ 32, 32 };
            const auto sizeNode = imageNode->GetProperty(L"size");
            if (sizeNode->IsOk())
                {
                sz.x = sizeNode->GetProperty(L"width")->AsDouble(bmp.GetScaledWidth());
                sz.y = sizeNode->GetProperty(L"height")->AsDouble(bmp.GetScaledHeight());

                image->SetSize(GraphItems::Image::ToBestSize(bmp.GetSize(), sz));
                }

            // center by default, but allow LoadItems (below) to override that
            // if client asked for something else
            image->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            image->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

            auto foundPos = resizeValues.find(std::wstring_view(
                imageNode->GetProperty(L"resize-method")->AsString().MakeLower().wc_str()));
            if (foundPos != resizeValues.cend())
                {
                image->SetResizeMethod(foundPos->second);
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

        static const std::map<std::wstring_view, Anchoring> anchoringValues = {
            { L"bottom-left-corner", Anchoring::BottomLeftCorner },
            { L"bottom-right-corner", Anchoring::BottomRightCorner },
            { L"center", Anchoring::Center },
            { L"top-left-corner", Anchoring::TopLeftCorner },
            { L"top-right-corner", Anchoring::TopRightCorner },
        };

        item.SetDPIScaleFactor(m_dpiScaleFactor);

        // ID
        item.SetId(itemNode->GetProperty(L"id")->AsDouble(wxID_ANY));

        // anchoring
        const auto foundPos = anchoringValues.find(std::wstring_view(
            itemNode->GetProperty(L"anchoring")->AsString().MakeLower().wc_str()));
        if (foundPos != anchoringValues.cend())
            {
            item.SetAnchoring(foundPos->second);
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

        // image scheme
        const auto imageSchemeNode = graphNode->GetProperty(L"image-scheme");
        if (imageSchemeNode->IsOk() && imageSchemeNode->IsValueArray())
            {
            std::vector<wxBitmapBundle> images;
            const auto imgNodes = imageSchemeNode->AsNodes();
            images.reserve(imgNodes.size());
            for (const auto& imgNode : imgNodes)
                {
                images.emplace_back(LoadImageFile(imgNode));
                }
            graph->SetImageScheme(
                std::make_shared<Images::Schemes::ImageScheme>(std::move(images)));
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
                const auto iconValue = ReportEnumConvert::ConvertIcon(
                    stippleShapeNode->GetProperty(L"icon")->AsString());
                if (iconValue.has_value())
                    {
                    graph->SetStippleShape(iconValue.value());
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"%s: unknown icon for graph stipple shape."),
                                         stippleShapeNode->GetProperty(L"icon")->AsString())
                            .ToUTF8());
                    }
                if (const auto stippleShapeColor =
                        ConvertColor(stippleShapeNode->GetProperty(L"color"));
                    stippleShapeColor.IsOk())
                    {
                    graph->SetStippleShapeColor(stippleShapeColor);
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
                auto label = LoadLabel(annotation->GetProperty(L"label"), GraphItems::Label());
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
        static const std::map<std::wstring_view, ReferenceAreaStyle> refAreaValues = {
            { L"fade-from-left-to-right", ReferenceAreaStyle::FadeFromLeftToRight },
            { L"fade-from-right-to-left", ReferenceAreaStyle::FadeFromRightToLeft },
            { L"solid", ReferenceAreaStyle::Solid },
        };

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

                    ReferenceAreaStyle areaStyle{ ReferenceAreaStyle::Solid };
                    const auto foundPos = refAreaValues.find(std::wstring_view(
                        refArea->GetProperty(L"style")->AsString().MakeLower().wc_str()));
                    if (foundPos != refAreaValues.cend())
                        {
                        areaStyle = foundPos->second;
                        }

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
            const auto ringPerimeterStr = legendNode->GetProperty(L"ring")->AsString();
            const auto ringPerimeter =
                (ringPerimeterStr.CmpNoCase(L"inner") == 0 ? Perimeter::Inner : Perimeter::Outer);
            const auto includeHeader = legendNode->GetProperty(L"include-header")->AsBool(true);
            const auto headerLabel = legendNode->GetProperty(L"title")->AsString();
            const auto placement = legendNode->GetProperty(L"placement")->AsString();
            if (placement.CmpNoCase(L"left") == 0)
                {
                auto legend =
                    graph->CreateLegend(Graphs::LegendOptions()
                                            .RingPerimeter(ringPerimeter)
                                            .IncludeHeader(includeHeader)
                                            .PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                // update title
                if (!headerLabel.empty())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow, currentColumn + 1, graph);
                canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                }
            else if (placement.CmpNoCase(L"bottom") == 0)
                {
                auto legend = graph->CreateLegend(
                    Graphs::LegendOptions()
                        .RingPerimeter(ringPerimeter)
                        .IncludeHeader(includeHeader)
                        .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                if (!headerLabel.empty())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                }
            else if (placement.CmpNoCase(L"top") == 0)
                {
                auto legend = graph->CreateLegend(
                    Graphs::LegendOptions()
                        .RingPerimeter(ringPerimeter)
                        .IncludeHeader(includeHeader)
                        .PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                if (!headerLabel.empty())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow + 1, currentColumn, graph);
                canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                }
            else // right, the default
                {
                auto legend = graph->CreateLegend(
                    Graphs::LegendOptions()
                        .RingPerimeter(ringPerimeter)
                        .IncludeHeader(includeHeader)
                        .PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                if (!headerLabel.empty())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(currentRow, ++currentColumn, std::move(legend));
                }
            }
        // no legend, so just add the graph
        else
            {
            canvas->SetFixedObject(currentRow, currentColumn, graph);
            }
        }
    } // namespace Wisteria
