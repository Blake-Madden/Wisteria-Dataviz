#include "reportbuilder.h"

using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;
using namespace Wisteria::Colors;
using namespace Wisteria::Icons;
using namespace Wisteria::Icons::Schemes;

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
        std::vector<std::shared_ptr<Wisteria::Graphs::Graph2D>> embeddedGraphs;

        assert(parent && L"Parent window must not be null when building a canvas!");
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
            m_name = reportNameNode->GetValueString();
            }

        // print settings
        wxPrintData reportPrintSettings;
        const auto printNode = json->GetProperty(L"print");
        if (printNode->IsOk())
            {
            const auto orientation = printNode->GetProperty(L"orientation")->GetValueString();
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
                printNode->GetProperty(L"paper-size")->GetValueString(L"paper-letter"));
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
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Datasets Section Error"), wxOK | wxICON_WARNING | wxCENTRE);
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

        // start loading the pages
        const auto pagesProperty = json->GetProperty(L"pages");
        if (pagesProperty->IsOk())
            {
            const auto pages = pagesProperty->GetValueArrayObject();
            for (const auto& page : pages)
                {
                // common axes are per page, where they must reference child graphs on the same page
                m_commonAxesPlaceholders.clear();
                if (page->IsOk())
                    {
                    // create the canvas used for the page
                    auto canvas = new Canvas(parent);
                    canvas->SetLabel(page->GetProperty(_DT(L"name"))->GetValueString());

                    // page numbering
                    if (page->HasProperty(L"page-numbering"))
                        {
                        m_pageNumber = 1;
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
                        canvas->SetBackgroundImage(
                            wxBitmapBundle(LoadImageFile(page->GetProperty(L"background-image"))));
                        }

                    // copy print settings from report
                    canvas->GetPrinterSettings().SetOrientation(
                        reportPrintSettings.GetOrientation());

                    size_t rowCount{ 0 };
                    const auto rowsProperty = page->GetProperty(L"rows");
                    if (rowsProperty->IsOk())
                        {
                        size_t currentRow{ 0 }, currentColumn{ 0 };
                        const auto rows = rowsProperty->GetValueArrayObject();
                        rowCount = rows.size();
                        // Empty page? Go to next one.
                        if (rows.size() == 0)
                            {
                            continue;
                            }
                        canvas->SetFixedObjectsGridSize(rows.size(), 1);
                        for (const auto& row : rows)
                            {
                            const auto itemsProperty = row->GetProperty(L"items");
                            if (itemsProperty->IsOk())
                                {
                                currentColumn = 0;
                                auto items = itemsProperty->GetValueArrayObject();
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
                                            if (typeProperty->GetValueString().CmpNoCase(
                                                    L"line-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadLinePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"heatmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadHeatMap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"gantt-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadGanttChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"candlestick-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadCandlestickPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"w-curve-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWCurvePlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"likert-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadLikertChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"linear-regression-roadmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadLRRoadmap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"pro-con-roadmap") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadProConRoadmap(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"word-cloud") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadWordCloud(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"sankey-diagram") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadSankeyDiagram(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"box-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadBoxPlot(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"pie-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadPieChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"histogram") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadHistogram(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"categorical-bar-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(LoadCategoricalBarChart(
                                                    item, canvas, currentRow, currentColumn));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"label") == 0)
                                                {
                                                canvas->SetFixedObject(
                                                    currentRow, currentColumn,
                                                    LoadLabel(item, GraphItems::Label()));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"image") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadImage(item));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"table") == 0)
                                                {
                                                LoadTable(item, canvas, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
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
                                            else if (typeProperty->GetValueString().CmpNoCase(
                                                         L"shape") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                                       LoadShape(item));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(
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
                    if (m_commonAxesPlaceholders.size())
                        {
                        std::vector<std::shared_ptr<Graphs::Graph2D>> childGraphs;
                        for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                            {
                            for (const auto& childId : commonAxisInfo.m_childrenIds)
                                {
                                auto childGraph = std::find_if(
                                    embeddedGraphs.cbegin(), embeddedGraphs.cend(),
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

        for (auto& tlink : m_tableLinks)
            {
            tlink.SyncTableSizes();
            }

        return reportPages;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBrush(const wxSimpleJSON::Ptr_t& brushNode, wxBrush& brush)
        {
        if (brushNode->IsOk())
            {
            if (brushNode->IsValueNull())
                { brush = wxNullBrush; }
            // just a color string
            else if (brushNode->IsValueString())
                {
                const wxColour brushColor(
                    ConvertColor(brushNode->GetValueString()));
                if (brushColor.IsOk())
                    { brush.SetColour(brushColor); }
                }
            // or a full definition
            else
                {
                const wxColour brushColor(
                    ConvertColor(brushNode->GetProperty(L"color")));
                if (brushColor.IsOk())
                    { brush.SetColour(brushColor); }

                const auto foundStyle =
                    ReportEnumConvert::ConvertBrushStyle(brushNode->GetProperty(L"style")->GetValueString());
                if (foundStyle)
                    { brush.SetStyle(foundStyle.value()); }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen)
        {
        static const std::map<std::wstring, wxPenStyle> styleValues =
            {
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
                { pen = wxNullPen; }
            else
                {
                const wxColour penColor(
                    ConvertColor(penNode->GetProperty(L"color")));
                if (penColor.IsOk())
                    { pen.SetColour(penColor); }

                if (penNode->HasProperty(L"width"))
                    { pen.SetWidth(penNode->GetProperty(L"width")->GetValueNumber(1)); }

                const auto style = styleValues.find(
                    penNode->GetProperty(L"style")->GetValueString().Lower().ToStdWstring());
                if (style != styleValues.cend())
                    { pen.SetStyle(style->second); }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis)
        {
        static const std::map<std::wstring_view, Axis::TickMark::DisplayType> tickmarkValues =
            {
            { L"inner", Axis::TickMark::DisplayType::Inner },
            { L"outer", Axis::TickMark::DisplayType::Outer },
            { L"crossed", Axis::TickMark::DisplayType::Crossed },
            { L"no-display", Axis::TickMark::DisplayType::NoDisplay }
            };

        static const std::map<std::wstring_view, AxisLabelDisplay> labelDisplayValues =
            {
            { L"custom-labels-or-values", AxisLabelDisplay::DisplayCustomLabelsOrValues },
            { L"only-custom-labels", AxisLabelDisplay::DisplayOnlyCustomLabels },
            { L"custom-labels-and-values", AxisLabelDisplay::DisplayCustomLabelsAndValues },
            { L"no-display", AxisLabelDisplay::NoDisplay },
            { L"values", AxisLabelDisplay::DisplayValues }
            };

        static const std::map<std::wstring_view, BracketLineStyle> bracketLineValues =
            {
            { L"arrow", BracketLineStyle::Arrow },
            { L"lines", BracketLineStyle::Lines },
            { L"reverse-arrow", BracketLineStyle::ReverseArrow },
            { L"curly-braces", BracketLineStyle::CurlyBraces },
            { L"no-connection-lines", BracketLineStyle::NoConnectionLines },
            };

        const auto titleProperty = axisNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty, GraphItems::Label());
            if (titleLabel != nullptr)
                { axis.GetTitle() = *titleLabel; }
            }
        const auto tickmarksProperty = axisNode->GetProperty(L"tickmarks");
        if (tickmarksProperty->IsOk())
            {
            const auto display = tickmarksProperty->GetProperty(L"display")->GetValueString().Lower();
            auto foundPos = tickmarkValues.find(std::wstring_view(display.wc_str()));
            if (foundPos != tickmarkValues.cend())
                { axis.SetTickMarkDisplay(foundPos->second); }
            }
        const auto display = axisNode->GetProperty(L"label-display")->GetValueString().Lower();
        const auto foundPos = labelDisplayValues.find(std::wstring_view(display.wc_str()));
        if (foundPos != labelDisplayValues.cend())
            { axis.SetLabelDisplay(foundPos->second); }

        const auto numDisplay = ReportEnumConvert::ConvertNumberDisplay(
            axisNode->GetProperty(_DT(L"number-display"))->GetValueString());
        if (numDisplay)
            { axis.SetNumerDisplay(numDisplay.value()); }

        axis.SetDoubleSidedAxisLabels(axisNode->GetProperty(L"double-sided-labels")->GetValueBool());

        // pens
        LoadPen(axisNode->GetProperty(L"axis-pen"), axis.GetAxisLinePen());
        LoadPen(axisNode->GetProperty(L"gridline-pen"), axis.GetGridlinePen());

        // max line length
        if (axisNode->HasProperty(L"label-length"))
            {
            axis.SetLabelLineLength(axisNode->GetProperty(L"label-length")->
                GetValueNumber(axis.GetLabelLineLength()));
            }

        if (axisNode->GetProperty(L"label-length-auto")->GetValueBool())
            { axis.SetLabelLengthAuto(); }

        const auto rangeNode = axisNode->GetProperty(_DT(L"range"));
        if (rangeNode->IsOk())
            {
            auto [rangeStart, rangeEnd] = axis.GetRange();
            rangeStart = rangeNode->GetProperty(_DT(L"start"))->GetValueNumber(rangeStart);
            rangeEnd = rangeNode->GetProperty(_DT(L"end"))->GetValueNumber(rangeEnd);

            auto precision = rangeNode->GetProperty(_DT(L"precision"))->GetValueNumber(axis.GetPrecision());
            auto interval = rangeNode->GetProperty(_DT(L"interval"))->GetValueNumber(axis.GetInterval());
            auto displayInterval =
                rangeNode->GetProperty(_DT(L"display-interval"))->GetValueNumber(axis.GetDisplayInterval());

            axis.SetRange(rangeStart, rangeEnd,
                precision, interval, displayInterval);
            }

        // custom labels
        const auto customLabelsNode = axisNode->GetProperty(L"custom-labels");
        if (customLabelsNode->IsOk() && customLabelsNode->IsValueArray())
            {
            const auto customLabels = customLabelsNode->GetValueArrayObject();
            for (const auto& customLabel : customLabels)
                {
                if (customLabel->HasProperty(L"value"))
                    {
                    axis.SetCustomLabel(customLabel->GetProperty(L"value")->GetValueNumber(),
                        *LoadLabel(customLabel->GetProperty(L"label"), GraphItems::Label()));
                    }
                }
            }

        // brackets
        const auto bracketsNode = axisNode->GetProperty(L"brackets");
        if (bracketsNode->IsOk())
            {
            wxPen bracketPen{ wxPenInfo(*wxBLACK, 2) };
            // individually defined brackets
            if (bracketsNode->IsValueArray())
                {
                const auto brackets = bracketsNode->GetValueArrayObject();
                for (const auto& bracket : brackets)
                    {
                    LoadPen(bracket->GetProperty(L"pen"), bracketPen);

                    const auto foundBracketStyle = bracketLineValues.find(std::wstring_view(
                        bracket->GetProperty(L"style")->GetValueString().wc_str()));

                    const std::optional<double> axisPos1 =
                        FindAxisPosition(axis, bracket->GetProperty(L"start"));
                    const std::optional<double> axisPos2 =
                        FindAxisPosition(axis, bracket->GetProperty(L"end"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        axis.AddBracket(
                            Axis::AxisBracket(axisPos1.value(), axisPos2.value(),
                                safe_divide<double>(axisPos1.value() + axisPos2.value(), 2),
                                bracket->GetProperty(L"label")->GetValueString(),
                                bracketPen,
                                (foundBracketStyle != bracketLineValues.cend()) ?
                                    foundBracketStyle->second : BracketLineStyle::CurlyBraces));
                        }
                    }
                }
            // or build a series of brackets from a dataset
            else
                {
                LoadPen(bracketsNode->GetProperty(L"pen"), bracketPen);
                const auto foundBracketStyle = bracketLineValues.find(std::wstring_view(
                    bracketsNode->GetProperty(L"style")->GetValueString().wc_str()));
                // if loading brackets based on the dataset
                if (bracketsNode->HasProperty(L"dataset"))
                    {
                    const wxString dsName = bracketsNode->GetProperty(L"dataset")->GetValueString();
                    const auto foundDataset = m_datasets.find(dsName);
                    if (foundDataset == m_datasets.cend() ||
                        foundDataset->second == nullptr)
                        {
                        throw std::runtime_error(
                            wxString::Format(
                                _(L"%s: dataset not found for axis brackets."), dsName).ToUTF8());
                        }

                    const auto variablesNode = bracketsNode->GetProperty(L"variables");
                    if (variablesNode->IsOk())
                        {
                        const auto labelVarName =
                            ExpandConstants(variablesNode->GetProperty(L"label")->GetValueString());
                        const auto valueVarName =
                            ExpandConstants(variablesNode->GetProperty(L"value")->GetValueString());

                        axis.AddBrackets(foundDataset->second, labelVarName, valueVarName);
                        if (bracketPen.IsOk())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                { bracket.GetLinePen() = bracketPen; }
                            }
                        if (foundBracketStyle != bracketLineValues.cend())
                            {
                            for (auto& bracket : axis.GetBrackets())
                                { bracket.SetBracketLineStyle(foundBracketStyle->second); }
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
                           "start and end points instead?").ToUTF8());
                    }
                }

            if (bracketsNode->GetProperty(L"simplify")->GetValueBool())
                { axis.SimplifyBrackets(); }
            }

        // show options
        axis.Show(axisNode->GetProperty(L"show")->GetValueBool(true));
        axis.ShowOuterLabels(axisNode->GetProperty(L"show-outer-labels")->GetValueBool(true));
        }

    //---------------------------------------------------
    void ReportBuilder::LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                                       const size_t currentRow, const size_t currentColumn)
        {
        const auto axisType = ReportEnumConvert::ConvertAxisType(
            commonAxisNode->GetProperty(L"axis-type")->GetValueString());
        if (axisType.has_value())
            {
            m_commonAxesPlaceholders.push_back(
                {
                axisType.value(),
                std::make_pair(currentRow, currentColumn),
                commonAxisNode->GetProperty(L"child-ids")->GetValueArrayNumber(),
                commonAxisNode->GetProperty(L"common-perpendicular-axis")->GetValueBool(),
                commonAxisNode
                });
            }
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadLabel(
        const wxSimpleJSON::Ptr_t& labelNode, const GraphItems::Label& labelTemplate)
        {
        // just a string
        if (labelNode->IsValueString())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetText(ExpandConstants(labelNode->GetValueString()));
            label->GetPen() = wxNullPen;

            return label;
            }
        // a fully defined label
        else if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetText(ExpandConstants(labelNode->GetProperty(L"text")->GetValueString()));
            label->GetPen() = wxNullPen;

            const wxColour bgcolor(
                ConvertColor(labelNode->GetProperty(L"background")));
            if (bgcolor.IsOk())
                { label->SetFontBackgroundColor(bgcolor); }
            const wxColour color(
                ConvertColor(labelNode->GetProperty(L"color")));
            if (color.IsOk())
                { label->SetFontColor(color); }

            // an image to the left side of it
            if (const auto imgNode = labelNode->GetProperty(L"left-image");
                imgNode->IsOk())
                {
                label->SetLeftImage(LoadImageFile(imgNode->GetProperty(L"image-import")));
                }
            // top image
            if (const auto imgNode = labelNode->GetProperty(L"top-image");
                imgNode->IsOk())
                {
                label->SetTopImage(LoadImageFile(imgNode->GetProperty(L"image-import")),
                    imgNode->GetProperty(L"offset")->GetValueNumber(0));
                }

            const auto orientation = labelNode->GetProperty(L"orientation")->GetValueString();
            if (orientation.CmpNoCase(L"horizontal") == 0)
                { label->SetTextOrientation(Orientation::Horizontal); }
            else if (orientation.CmpNoCase(L"vertical") == 0)
                { label->SetTextOrientation(Orientation::Vertical); }

            label->SetLineSpacing(labelNode->GetProperty(L"line-spacing")->GetValueNumber(1));

            // font attributes
            if (labelNode->HasProperty(L"bold"))
                {
                if (labelNode->GetProperty(L"bold")->GetValueBool())
                    { label->GetFont().MakeBold(); }
                else
                    { label->GetFont().SetWeight(wxFONTWEIGHT_NORMAL); }
                }

            const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                labelNode->GetProperty(L"text-alignment")->GetValueString());
            if (textAlignment.has_value())
                { label->SetTextAlignment(textAlignment.value()); }

            // header info
            auto headerNode = labelNode->GetProperty(L"header");
            if (headerNode->IsOk())
                {
                label->GetHeaderInfo().Enable(true);
                if (headerNode->HasProperty(L"bold"))
                    {
                    if (headerNode->GetProperty(L"bold")->GetValueBool())
                        { label->GetHeaderInfo().GetFont().MakeBold(); }
                    else
                        { label->GetHeaderInfo().GetFont().SetWeight(wxFONTWEIGHT_NORMAL); }
                    }
                const wxColour headerColor(
                    ConvertColor(headerNode->GetProperty(L"color")));
                if (headerColor.IsOk())
                    { label->GetHeaderInfo().FontColor(headerColor); }

                label->GetHeaderInfo().RelativeScaling(
                    headerNode->GetProperty(L"relative-scaling")->GetValueNumber(1));

                const auto headerTextAlignment = ReportEnumConvert::ConvertTextAlignment(
                    headerNode->GetProperty(L"text-alignment")->GetValueString());
                if (headerTextAlignment.has_value())
                    { label->GetHeaderInfo().LabelAlignment(headerTextAlignment.value()); }
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
            const auto values = constantsNode->GetValueArrayObject();
            for (const auto& value : values)
                {
                if (value->IsOk())
                    {
                    const wxString vName = value->GetProperty(_DT(L"name"))->GetValueString();
                    m_values.insert_or_assign(vName,
                        value->GetProperty(L"value")->IsValueString() ?
                        ValuesType(value->GetProperty(L"value")->GetValueString()) :
                        ValuesType(value->GetProperty(L"value")->GetValueNumber()) );
                    }
                }
            }
        }

    //---------------------------------------------------
    std::optional<std::vector<wxString>> ReportBuilder::ExpandColumnSelections(wxString var,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (var.starts_with(L"{{") && var.ends_with(L"}}"))
            { var = var.substr(2, var.length() - 4); }
        else
            { return std::nullopt; }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(everything|everythingexcept|matches)" +
                         OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(var))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount < 2)
                { return std::nullopt; }

            const wxString funcName = re.GetMatch(var, 1).MakeLower();
            std::vector<wxString> columns;

            if (funcName.CmpNoCase(L"everything") == 0)
                {
                if (dataset->GetIdColumn().GetName().length())
                    {
                    columns.push_back(dataset->GetIdColumn().GetName());
                    }
                const auto catCols{ dataset->GetCategoricalColumnNames() };
                const auto contCols{ dataset->GetContinuousColumnNames() };
                const auto dateCols{ dataset->GetDateColumnNames() };
                std::copy(catCols.cbegin(), catCols.cend(), std::back_inserter(columns));
                std::copy(contCols.cbegin(), contCols.cend(), std::back_inserter(columns));
                std::copy(dateCols.cbegin(), dateCols.cend(), std::back_inserter(columns));

                return columns;
                }
            else if (paramPartsCount >= 3)
                {
                const wxString columnPattern =
                    ConvertColumnOrGroupParameter(re.GetMatch(var, 2), dataset);

                if (funcName.CmpNoCase(L"matches") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that contain the string
                        if (columnRE.Matches(dataset->GetIdColumn().GetName()) )
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
                    else
                        { return std::nullopt; }
                    }
                else if (funcName.CmpNoCase(L"everythingexcept") == 0)
                    {
                    const wxRegEx columnRE(columnPattern);
                    if (columnRE.IsValid())
                        {
                        // get columns that DON'T contain the string
                        if (dataset->GetIdColumn().GetName().length() &&
                            !columnRE.Matches(dataset->GetIdColumn().GetName()) )
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
                    else
                        { return std::nullopt; }
                    }
                else
                    { return std::nullopt; }
                }
            else
                { return std::nullopt; }
            }
        else
            { return std::nullopt; }
        }

    //---------------------------------------------------
    void ReportBuilder::CalcFormulas(const wxSimpleJSON::Ptr_t& formulasNode,
        const std::shared_ptr<const Data::Dataset>& dataset)
        {
        if (formulasNode->IsOk())
            {
            const auto formulas = formulasNode->GetValueArrayObject();
            for (const auto& formula : formulas)
                {
                if (formula->IsOk())
                    {
                    const wxString vName = formula->GetProperty(_DT(L"name"))->GetValueString();
                    if (formula->GetProperty(L"value")->IsValueString())
                        {
                        m_values.insert_or_assign(vName,
                            CalcFormula(formula->GetProperty(L"value")->GetValueString(), dataset) );
                        }
                    else if (formula->GetProperty(L"value")->IsValueNumber())
                        {
                        m_values.insert_or_assign(vName,
                            formula->GetProperty(L"value")->GetValueNumber() );
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType ReportBuilder::CalcFormula(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        const wxRegEx re(FunctionStartRegEx() +
            LR"((min|max|n|total|grandtotal|groupcount|grouppercentdecimal|grouppercent|continuouscolumn|now|pagenumber|reportname))" +
            OpeningParenthesisRegEx());
        if (re.Matches(formula))
            {
            const wxString funcName = re.GetMatch(formula, 1).MakeLower();
            if (funcName.CmpNoCase(L"min") == 0 ||
                funcName.CmpNoCase(L"max") == 0)
                { return CalcMinMax(formula, dataset); }
            else if (funcName.CmpNoCase(L"n") == 0)
                {
                const auto calcValue = CalcValidN(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"total") == 0)
                {
                const auto calcValue = CalcTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grandtotal") == 0)
                {
                const auto calcValue = CalcGrandTotal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"groupcount") == 0)
                {
                const auto calcValue = CalcGroupCount(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grouppercentdecimal") == 0)
                {
                const auto calcValue = CalcGroupPercentDecimal(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"grouppercent") == 0)
                {
                const auto calcValue = CalcGroupPercent(formula, dataset);
                return (calcValue ? ValuesType(calcValue.value()) : ValuesType(formula));
                }
            else if (funcName.CmpNoCase(L"continuouscolumn") == 0)
                { return ExpandColumnSelection(formula, dataset); }
            else if (funcName.CmpNoCase(L"now") == 0)
                { return CalcNow(formula); }
            else if (funcName.CmpNoCase(L"pagenumber") == 0)
                { return FormatPageNumber(formula); }
            else if (funcName.CmpNoCase(L"reportname") == 0)
                { return m_name; }
            }
        // note that formula may just be a constant (e.g., color string),
        // so return that if it can't be calculated into something else
        return formula;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ExpandColumnSelection(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        const wxRegEx re(FunctionStartRegEx() +
            L"(continuouscolumn)" + OpeningParenthesisRegEx() +
            NumberOrStringRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            if (dataset->GetContinuousColumns().size() == 0)
                {
                throw std::runtime_error(
                    wxString(
                        _(L"ContinuousColumn() failed. "
                           "There are no continuous columns in the dataset.")).ToUTF8());
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
                        { columnIndex = dataset->GetContinuousColumns().size() - 1; }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(
                                _(L"'%s': unknown constant for continuous column index."),
                                columnIndexStr).ToUTF8());
                        }
                    }
                else if (columnIndexStr.ToULong(&columnIndex))
                    {
                    if (columnIndex >= dataset->GetContinuousColumns().size())
                        {
                        throw std::runtime_error(
                        wxString::Format(
                            _(L"%lu: invalid continuous column index."), columnIndex).ToUTF8());
                        }
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                            _(L"'%s': unable to convert value for continuous column index."),
                            columnIndexStr).ToUTF8());
                    }
                return dataset->GetContinuousColumn(columnIndex).GetName();
                }
            }

        // can't get the name of the column, just return the original text
        return formula;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupCount(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                                 L"(groupcount)" + OpeningParenthesisRegEx() +
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
                        wxString::Format(_(L"%s: group column not found."),
                                            groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."),
                                            groupValue).ToUTF8());
                    }

                return dataset->GetCategoricalColumnValidN(groupName, groupName,
                                                           groupID.value());
                }
            // dataset or something missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGroupPercentDecimal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() +
                         L"(grouppercentdecimal)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercentdecimal)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"groupcount");
                const auto groupTotal = CalcGroupCount(countFormula, dataset);
                if (groupTotal)
                    {
                    return safe_divide<double>(groupTotal.value(), dataset->GetRowCount());
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<wxString> ReportBuilder::CalcGroupPercent(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }

        const wxRegEx re(FunctionStartRegEx() +
                         L"(grouppercent)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ParamSeparatorRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());

        if (re.Matches(formula))
            {
            wxString countFormula(formula);
            const wxRegEx reFunctionRename(L"(?i)(grouppercent)");
            if (reFunctionRename.Matches(countFormula))
                {
                reFunctionRename.ReplaceFirst(&countFormula, L"grouppercentdecimal");
                const auto percDec = CalcGroupPercentDecimal(countFormula, dataset);
                if (percDec)
                    {
                    return wxNumberFormatter::ToString(percDec.value() * 100, 0,
                                wxNumberFormatter::Style::Style_NoTrailingZeroes) + L"%";
                    }
                }
            }

        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcValidN(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx reSimple(FunctionStartRegEx() +
                               L"(n)" + OpeningParenthesisRegEx() +
                               ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        const wxRegEx reExtended(FunctionStartRegEx() +
                                 L"(n)" + OpeningParenthesisRegEx() +
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
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName);
                    }
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
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
                        wxString::Format(_(L"%s: group column not found."),
                                            groupName).ToUTF8());
                    }
                const auto groupID = groupColumn->GetIDFromLabel(groupValue);
                if (!groupID)
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"Group ID for '%s' not found."),
                                            groupValue).ToUTF8());
                    }
                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    return dataset->GetCategoricalColumnValidN(columnName, groupName,
                                                                        groupID.value());
                    }
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousColumnValidN(columnName, groupName,
                                                                    groupID.value());
                    }
                }
            // dataset or something missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    wxString ReportBuilder::ConvertColumnOrGroupParameter(wxString columnStr,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (columnStr.starts_with(L"`") && columnStr.ends_with(L"`"))
            { columnStr = columnStr.substr(1, columnStr.length() - 2); }
        else if (columnStr.starts_with(L"{{") && columnStr.ends_with(L"}}"))
            {
            columnStr = columnStr.substr(2, columnStr.length() - 4);
            columnStr = ExpandConstants(columnStr);
            const auto calcStr = CalcFormula(columnStr, dataset);
            if (const auto strVal{ std::get_if<wxString>(&calcStr) };
                strVal != nullptr)
                { return *strVal; }
            else
                { return wxEmptyString; }
            }

        return columnStr;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcGrandTotal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(grandtotal)" + OpeningParenthesisRegEx() +
                         ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                // only continuous can be totaled
                double total{ 0 };
                for (size_t i = 0; i < dataset->GetContinuousColumns().size(); ++i)
                    { total += dataset->GetContinuousTotal(i); }
                return total;
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::CalcTotal(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(total)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString columnName = ConvertColumnOrGroupParameter(
                                                re.GetMatch(formula, 2), dataset);
                // only continuous can be totaled
                if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    return dataset->GetContinuousTotal(columnName);
                    }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                        _(L"%s: column must be continuous when totaling."), columnName).ToUTF8());
                    }
                }
            // dataset or column name missing
            else
                { return std::nullopt; }
            }
        return std::nullopt;
        }

    //---------------------------------------------------
    ReportBuilder::ValuesType ReportBuilder::CalcMinMax(const wxString& formula,
        const std::shared_ptr<const Data::Dataset>& dataset) const
        {
        if (dataset == nullptr)
            {
            throw std::runtime_error(
                    wxString::Format(
                        _(L"%s: invalid dataset when calculating formula."), formula).ToUTF8());
            }
        const wxRegEx re(FunctionStartRegEx() +
                         L"(min|max)" + OpeningParenthesisRegEx() +
                         ColumnNameOrFormulaRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 3)
                {
                const wxString funcName = re.GetMatch(formula, 1).MakeLower();
                const wxString columnName = ConvertColumnOrGroupParameter(
                                                re.GetMatch(formula, 2), dataset);

                if (dataset->GetCategoricalColumn(columnName) !=
                    dataset->GetCategoricalColumns().cend())
                    {
                    const auto [minVal, maxVal] =
                        dataset->GetCategoricalMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                else if (dataset->GetContinuousColumn(columnName) !=
                    dataset->GetContinuousColumns().cend())
                    {
                    const auto [minVal, maxVal] =
                        dataset->GetContinuousMinMax(columnName);
                    return (funcName.CmpNoCase(L"min") == 0 ? minVal : maxVal);
                    }
                }
            // dataset or column name missing
            else
                { return formula; }
            }
        return formula;
        }

    //---------------------------------------------------
    wxString ReportBuilder::FormatPageNumber(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() +
            L"(pagenumber)" + OpeningParenthesisRegEx() +
            StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        return re.Matches(formula) ?
            wxNumberFormatter::ToString(m_pageNumber, 0,
                wxNumberFormatter::Style::Style_WithThousandsSep) :
                   wxString{};
        }

    //---------------------------------------------------
    wxString ReportBuilder::CalcNow(const wxString& formula) const
        {
        const wxRegEx re(FunctionStartRegEx() +
                         L"(now)" + OpeningParenthesisRegEx() +
                         StringOrEmptyRegEx() + ClosingParenthesisRegEx());
        if (re.Matches(formula))
            {
            const auto paramPartsCount = re.GetMatchCount();
            if (paramPartsCount >= 2)
                {
                wxString paramValue = re.GetMatch(formula, 2).MakeLower();
                if (paramValue.starts_with(L"`") && paramValue.ends_with(L"`"))
                    { paramValue = paramValue.substr(1, paramValue.length() - 2); }

                if (paramValue.empty())
                    { return wxDateTime::Now().FormatDate(); }
                else if (paramValue == L"fancy")
                    { return wxDateTime::Now().Format(L"%B %d, %Y"); }
                // which part of the date is requested?
                else if (paramValue == L"year")
                    { return std::to_wstring(wxDateTime::Now().GetYear()); }
                else if (paramValue == L"month")
                    { return std::to_wstring(wxDateTime::Now().GetMonth()); }
                else if (paramValue == L"monthname")
                    { return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth()); }
                else if (paramValue == L"monthshortname")
                    { return wxDateTime::GetMonthName(wxDateTime::Now().GetMonth(), wxDateTime::Name_Abbr); }
                else if (paramValue == L"day")
                    { return std::to_wstring(wxDateTime::Now().GetDay()); }
                else if (paramValue == L"dayname")
                    { return wxDateTime::GetWeekDayName(wxDateTime::Now().GetWeekDay()); }
                else
                    {
                    throw std::runtime_error(
                        wxString::Format(
                        _(L"%s: unknown parameter passed to Now()."), paramValue).ToUTF8());
                    }
                }
            // no param, just return full date
            else
                { return wxDateTime::Now().Format(); }
            }
        return wxDateTime::Now().Format();
        }

    //---------------------------------------------------
    void ReportBuilder::LoadMerges(const wxSimpleJSON::Ptr_t& mergesNode,
                                   const std::shared_ptr<const Data::Dataset>& datasetToMerge)
        {
        if (mergesNode->IsOk())
            {
            auto merges = mergesNode->GetValueArrayObject();
            for (const auto& merge : merges)
                {
                if (merge->IsOk())
                    {
                    const wxString dsName = merge->GetProperty(L"other-dataset")->GetValueString();
                    const auto foundPos = m_datasets.find(dsName);
                    if (foundPos == m_datasets.cend() ||
                        foundPos->second == nullptr)
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: dataset not found for dataset merging."), dsName).ToUTF8());
                        }

                    const auto mergeType = merge->GetProperty(L"type")->GetValueString(L"left-join-unique");
                    if (mergeType.CmpNoCase(L"left-join-unique") == 0)
                        {
                        std::vector<std::pair<wxString, wxString>> bys;
                        const auto byCols = merge->GetProperty(L"by")->GetValueArrayObject();
                        for (const auto& byCol : byCols)
                            {
                            bys.push_back(std::make_pair(byCol->GetProperty(L"left-column")->GetValueString(),
                                                         byCol->GetProperty(L"right-column")->GetValueString()));
                            }
                        auto mergedData = DatasetJoin::LeftJoinUnique(datasetToMerge, foundPos->second,
                            bys, merge->GetProperty(L"suffix")->GetValueString(L".x"));

                        if (mergedData)
                            {
                            m_datasets.insert_or_assign(
                                merge->GetProperty(_DT(L"name"))->GetValueString(), mergedData);
                            LoadDatasetTransformations(merge, mergedData);
                            }
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: unrecognized dataset merging method."), mergeType).ToUTF8());
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
            auto pivots = pivotsNode->GetValueArrayObject();
            for (const auto& pivot : pivots)
                {
                if (pivot->IsOk())
                    {
                    Pivot pw;
                    const auto pivotType = pivot->GetProperty(L"type")->GetValueString(L"wider");
                    if (pivotType.CmpNoCase(L"wider") == 0)
                        {
                        auto pivotedData = pw.PivotWider(parentToPivot,
                            pivot->GetProperty(L"id-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-from-column")->GetValueString(),
                            pivot->GetProperty(L"values-from-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-separator")->GetValueString(L"_"),
                            pivot->GetProperty(L"names-prefix")->GetValueString(),
                            pivot->GetProperty(L"fill-value")->GetValueNumber(
                                std::numeric_limits<double>::quiet_NaN()));

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(_DT(L"name"))->GetValueString(), pivotedData);
                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else if (pivotType.CmpNoCase(L"longer") == 0)
                        {
                        auto pivotedData = pw.PivotLonger(parentToPivot,
                            pivot->GetProperty(L"columns-to-keep")->GetValueStringVector(),
                            pivot->GetProperty(L"from-columns")->GetValueStringVector(),
                            pivot->GetProperty(L"names-to")->GetValueStringVector(),
                            pivot->GetProperty(L"values-to")->GetValueString(),
                            pivot->GetProperty(L"names-pattern")->GetValueString());

                        if (pivotedData)
                            {
                            m_datasets.insert_or_assign(
                                pivot->GetProperty(_DT(L"name"))->GetValueString(), pivotedData);
                            LoadDatasetTransformations(pivot, pivotedData);
                            }
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString::Format(_(L"%s: unrecognized pivot method."), pivotType).ToUTF8());
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadSubsets(const wxSimpleJSON::Ptr_t& subsetsNode,
                                    const std::shared_ptr<const Data::Dataset>& parentToSubset)
        {
        static const std::map<std::wstring_view, Comparison> cmpOperators =
            {
            { L"=", Comparison::Equals },
            { L"==", Comparison::Equals },
            { L"!=", Comparison::NotEquals },
            { L"<>", Comparison::NotEquals },
            { L"<", Comparison::LessThan },
            { L"<=", Comparison::LessThanOrEqualTo },
            { L">", Comparison::GreaterThan },
            { L">=", Comparison::GreaterThanOrEqualTo }
            };

        const auto loadColumnFilter = [this, &parentToSubset](const auto& filterNode)
            {
            const auto foundPos = cmpOperators.find(std::wstring_view(
                filterNode->GetProperty(L"operator")->
                GetValueString().MakeLower().wc_str()));
            Comparison cmp = (foundPos != cmpOperators.cend() ?
                foundPos->second : Comparison::Equals);

            const auto valuesNode = filterNode->GetProperty(L"values");

            if (valuesNode->IsOk())
                {
                ColumnFilterInfo cFilter
                    {
                    filterNode->GetProperty(L"column")->GetValueString(),
                    cmp,
                    std::vector<DatasetValueType>()
                    };
                if (!parentToSubset->ContainsColumn(cFilter.m_columnName) &&
                    parentToSubset->GetContinuousColumns().size() > 0 &&
                    cFilter.m_columnName.CmpNoCase(L"last-continuous-column") == 0)
                    {
                    cFilter.m_columnName = parentToSubset->GetContinuousColumnNames().back();
                    }
                const auto filterValues = valuesNode->GetValueArrayObject();
                if (filterValues.empty())
                    {
                    throw std::runtime_error(
                        _(L"No values were provided for subset filtering.").ToUTF8());
                    }
                for (const auto& filterValue : filterValues)
                    {
                    wxDateTime dt;
                    const bool isDate =
                        (filterValue->IsValueString() &&
                            (dt.ParseDateTime(filterValue->GetValueString()) ||
                                dt.ParseDate(filterValue->GetValueString())));
                    cFilter.m_values.push_back(isDate ?
                        DatasetValueType(dt) :
                        filterValue->IsValueString() ?
                        DatasetValueType(ExpandConstants(filterValue->GetValueString())) :
                        DatasetValueType(filterValue->GetValueNumber()));
                    }

                return cFilter;
                }
            else
                {
                throw std::runtime_error(
                    _(L"Comparison value for subset filter missing.").ToUTF8());
                }
            };

        if (subsetsNode->IsOk())
            {
            auto subsets = subsetsNode->GetValueArrayObject();
            for (const auto& subset : subsets)
                {
                if (subset->IsOk())
                    {
                    const auto sectionNode = subset->GetProperty(L"section");
                    const auto filterNode = subset->GetProperty(L"filter");
                    const auto filterAndNode = subset->GetProperty(L"filter-and");
                    const auto filterOrNode = subset->GetProperty(L"filter-or");
                    const auto validFilterTypeNodes =
                        (filterNode->IsOk() ? 1 : 0) +
                        (filterAndNode->IsOk() ? 1 : 0) +
                        (filterOrNode->IsOk() ? 1 : 0);
                    if (validFilterTypeNodes > 1)
                        {
                        throw std::runtime_error(
                            _(L"Only one filter type allowed for a subset.").ToUTF8());
                        }
                    else if (validFilterTypeNodes == 0 && sectionNode->IsNull())
                        {
                        throw std::runtime_error(
                            _(L"Subset missing filters or section definition.").ToUTF8());
                        }

                    Subset dataSubsetter;
                    std::shared_ptr<Data::Dataset> subsettedDataset{ nullptr };
                    // single column filter
                    if (filterNode->IsOk())
                        {
                        subsettedDataset = dataSubsetter.SubsetSimple(
                                parentToSubset, loadColumnFilter(filterNode));
                        }
                    // ANDed filters
                    else if (filterAndNode->IsOk())
                        {
                        std::vector<ColumnFilterInfo> cf;
                        const auto filterAndNodes = filterAndNode->GetValueArrayObject();
                        if (filterAndNodes.size() == 0)
                            {
                            throw std::runtime_error(
                                _(L"Subset missing filters.").ToUTF8());
                            }
                        for (const auto& filternode : filterAndNodes)
                            {
                            cf.push_back(loadColumnFilter(filternode));
                            }

                        subsettedDataset = dataSubsetter.SubsetAnd(parentToSubset, cf);
                        }
                    // ORed filters
                    else if (filterOrNode->IsOk())
                        {
                        std::vector<ColumnFilterInfo> cf;
                        const auto filterOrNodes = filterOrNode->GetValueArrayObject();
                        if (filterOrNodes.size() == 0)
                            {
                            throw std::runtime_error(
                                _(L"Subset missing filters.").ToUTF8());
                            }
                        for (const auto& filternode : filterOrNodes)
                            {
                            cf.push_back(loadColumnFilter(filternode));
                            }

                        subsettedDataset = dataSubsetter.SubsetOr(parentToSubset, cf);
                        }
                    else if (sectionNode->IsOk())
                        {
                        subsettedDataset = dataSubsetter.SubsetSection(
                                parentToSubset, sectionNode->GetProperty(_DT(L"column"))->GetValueString(),
                                sectionNode->GetProperty(L"start-label")->GetValueString(),
                                sectionNode->GetProperty(L"end-label")->GetValueString(),
                                sectionNode->GetProperty(L"include-sentinel-labels")->GetValueBool(true));
                        }

                    if (subsettedDataset)
                        {
                        m_datasets.insert_or_assign(
                            subset->GetProperty(_DT(L"name"))->GetValueString(), subsettedDataset);
                        LoadDatasetTransformations(subset, subsettedDataset);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasetTransformations(const wxSimpleJSON::Ptr_t& dsNode,
        std::shared_ptr<Data::Dataset>& dataset)
        {
        if (dsNode->IsOk())
            {
            // column renaming
            auto colRenames = dsNode->GetProperty(L"columns-rename")->GetValueArrayObject();
            for (const auto& colRename : colRenames)
                {
                if (colRename->HasProperty(_DT(L"name")))
                    {
                    dataset->RenameColumn(
                        colRename->GetProperty(_DT(L"name"))->GetValueString(),
                        colRename->GetProperty(L"new-name")->GetValueString());
                    }
                if (colRename->HasProperty(L"name-re"))
                    {
                    dataset->RenameColumnRE(
                        colRename->GetProperty(L"name-re")->GetValueString(),
                        colRename->GetProperty(L"new-name-re")->GetValueString());
                    }
                }

            // column mutations
            auto mutateCats = dsNode->GetProperty(L"mutate-categorical-columns")->GetValueArrayObject();
            for (const auto& mutateCat : mutateCats)
                {
                RegExMap reMap;
                const auto replacements = mutateCat->GetProperty(L"replacements")->GetValueArrayObject();
                for (const auto& replacement : replacements)
                    {
                    reMap.push_back(
                        std::make_pair(std::make_unique<wxRegEx>(
                                           replacement->GetProperty(L"pattern")->GetValueString()),
                                       replacement->GetProperty(L"replacement")->GetValueString()));
                    }

                dataset->MutateCategoricalColumn(
                    mutateCat->GetProperty(L"source-column")->GetValueString(),
                    mutateCat->GetProperty(L"target-column")->GetValueString(),
                    reMap);
                }

            // column SELECT
            auto selectPattern = dsNode->GetProperty(L"columns-select")->GetValueString();
            if (selectPattern.length())
                { dataset->SelectColumnsRE(selectPattern); }

            // label recoding
            auto recodeREs = dsNode->GetProperty(L"recode-re")->GetValueArrayObject();
            for (const auto& recodeRE : recodeREs)
                {
                dataset->RecodeRE(
                    recodeRE->GetProperty(L"column")->GetValueString(),
                    recodeRE->GetProperty(L"pattern")->GetValueString(),
                    recodeRE->GetProperty(L"replacement")->GetValueString());
                }

            // category collapsing (min)
            auto collapseMins = dsNode->GetProperty(L"collapse-min")->GetValueArrayObject();
            for (const auto& collapseMin : collapseMins)
                {
                dataset->CollapseMin(
                    collapseMin->GetProperty(L"column")->GetValueString(),
                    collapseMin->GetProperty(L"min")->GetValueNumber(2),
                    collapseMin->GetProperty(L"other-label")->GetValueString(_(L"Other")));
                }

            // category collapsing (except)
            auto collapseExcepts = dsNode->GetProperty(L"collapse-except")->GetValueArrayObject();
            for (const auto& collapseExcept : collapseExcepts)
                {
                dataset->CollapseExcept(
                    collapseExcept->GetProperty(L"column")->GetValueString(),
                    collapseExcept->GetProperty(L"labels-to-keep")->GetValueStringVector(),
                    collapseExcept->GetProperty(L"other-label")->GetValueString(_(L"Other")));
                }

            // load any constants defined with this dataset
            CalcFormulas(dsNode->GetProperty(L"formulas"), dataset);

            // load any subsets of this dataset
            LoadSubsets(dsNode->GetProperty(L"subsets"), dataset);

            // load any pivots of this dataset
            LoadPivots(dsNode->GetProperty(L"pivots"), dataset);

            // load any merges of this dataset
            LoadMerges(dsNode->GetProperty(L"merges"), dataset);

            if (dsNode->GetProperty(L"column-names-sort")->GetValueBool())
                { dataset->SortColumnNames(); }

            const auto exportPath =
                    dsNode->GetProperty(L"export-path")->GetValueString();
            // A project silently writing to an arbitrary file is
            // a security threat vector, so only allow that for builds
            // with DEBUG_FILE_IO explicitly set.
            // This should only be used for reviewing the output from a pivot operation
            // when designing a project (in release build).
            if constexpr(Settings::IsDebugFlagEnabled(DebugSettings::AllowFileIO))
                {
                if (exportPath.length())
                    {
                    wxFileName fn(exportPath);
                    if (fn.GetPath().empty())
                        { fn = wxFileName(m_configFilePath).GetPathWithSep() + exportPath; }
                    if (fn.GetExt().CmpNoCase(L"csv") == 0)
                        { dataset->ExportCSV(fn.GetFullPath()); }
                    else
                        { dataset->ExportTSV(fn.GetFullPath()); }
                    }
                }
            else if (exportPath.length())
                {
                // just log this (don't throw)
                wxLogWarning(
                        wxString::Format(L"Dataset '%s' cannot be exported "
                            "because debug file IO is not enabled.",
                            dataset->GetName()));
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasets(const wxSimpleJSON::Ptr_t& datasetsNode)
        {
        if (datasetsNode->IsOk())
            {
            auto datasets = datasetsNode->GetValueArrayObject();
            for (const auto& datasetNode : datasets)
                {
                if (datasetNode->IsOk())
                    {
                    wxString path = datasetNode->GetProperty(L"path")->GetValueString();
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
                    wxString dsName = datasetNode->GetProperty(_DT(L"name"))->GetValueString();
                    if (dsName.empty())
                        { dsName = wxFileName(path).GetName(); }

                    const wxString importer = datasetNode->GetProperty(L"importer")->GetValueString();
                    // read the variables info
                    //------------------------
                    // ID column
                    const wxString idColumn = datasetNode->GetProperty(L"id-column")->GetValueString();
                    // date columns
                    std::vector<Data::ImportInfo::DateImportInfo> dateInfo;
                    const auto dateProperty = datasetNode->GetProperty(L"date-columns");
                    if (dateProperty->IsOk())
                        {
                        const auto dateVars = dateProperty->GetValueArrayObject();
                        for (const auto& dateVar : dateVars)
                            {
                            if (dateVar->IsOk())
                                {
                                // get the date column's name and how to load it
                                const wxString dateName =
                                    dateVar->GetProperty(_DT(L"name"))->GetValueString();
                                if (dateName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Date column must have a name.")).ToUTF8());
                                    }
                                const wxString dateParser =
                                    dateVar->GetProperty(L"parser")->GetValueString();
                                const wxString dateFormat =
                                    dateVar->GetProperty(L"format")->GetValueString();
                                dateInfo.push_back(
                                    {
                                    dateName,
                                    (dateParser.CmpNoCase(L"iso-date") == 0 ?
                                        DateImportMethod::IsoDate :
                                     dateParser.CmpNoCase(L"time") == 0 ?
                                        DateImportMethod::Time :
                                     dateParser.CmpNoCase(L"iso-combined") == 0 ?
                                        DateImportMethod::IsoCombined :
                                     dateParser.CmpNoCase(L"strptime-format") == 0 ?
                                        DateImportMethod::StrptimeFormatString :
                                     dateParser.CmpNoCase(L"rfc822") == 0 ?
                                        DateImportMethod::Rfc822 :
                                     DateImportMethod::Automatic),
                                    dateFormat
                                    }
                                    );
                                }
                            }
                        }
                    // continuous columns
                    std::vector<wxString> continuousVars =
                        datasetNode->GetProperty(L"continuous-columns")->GetValueStringVector();
                    // categorical columns
                    std::vector<Data::ImportInfo::CategoricalImportInfo> catInfo;
                    const auto catProperty = datasetNode->GetProperty(L"categorical-columns");
                    if (catProperty->IsOk())
                        {
                        const auto catVars = catProperty->GetValueArrayObject();
                        for (const auto& catVar : catVars)
                            {
                            if (catVar->IsOk())
                                {
                                // get the cat column's name and how to load it
                                const wxString catName = catVar->GetProperty(_DT(L"name"))->GetValueString();
                                if (catName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Categorical column must have a name.")).ToUTF8());
                                    }
                                const wxString catParser =
                                    catVar->GetProperty(L"parser")->GetValueString();
                                catInfo.push_back(
                                    {
                                    catName,
                                    (catParser.CmpNoCase(L"as-integers") == 0 ?
                                      CategoricalImportMethod::ReadAsIntegers :
                                      CategoricalImportMethod::ReadAsStrings)
                                    }
                                    );
                                }
                            }
                        }

                    // create the dataset
                    auto dataset = std::make_shared<Data::Dataset>();

                    ImportInfo importDefines;
                    const auto fillImportDefines = [&importDefines,&datasetNode]()
                        {
                        if (datasetNode->HasProperty(L"skip-rows"))
                            {
                            importDefines.SkipRows(
                                datasetNode->GetProperty(L"skip-rows")->GetValueNumber(0));
                            }
                        if (datasetNode->HasProperty(L"md-codes"))
                            {
                            importDefines.MDCodes(
                                wxStringVectorToWstringVector(
                                    datasetNode->GetProperty(L"md-codes")->GetValueStringVector()));
                            }
                        if (datasetNode->HasProperty(L"treat-leading-zeros-as-text"))
                            {
                            importDefines.TreatLeadingZerosAsText(
                                datasetNode->GetProperty(L"treat-leading-zeros-as-text")->GetValueBool());
                            }
                        if (datasetNode->HasProperty(L"treat-years-as-text"))
                            {
                            importDefines.TreatYearsAsText(
                                datasetNode->GetProperty(L"treat-years-as-text")->GetValueBool());
                            }
                        if (datasetNode->HasProperty(L"max-discrete-value"))
                            {
                            importDefines.MaxDiscreteValue(
                                datasetNode->GetProperty(L"max-discrete-value")->GetValueNumber());
                            }
                        };
                    fillImportDefines();

                    const std::variant<wxString, size_t> worksheet =
                    datasetNode->GetProperty(L"worksheet")->IsValueNumber() ?
                        std::variant<wxString, size_t>(
                            static_cast<size_t>(
                                datasetNode->GetProperty(L"worksheet")->GetValueNumber())) :
                        std::variant<wxString, size_t>(
                            datasetNode->GetProperty(L"worksheet")->GetValueString());
                    // if no columns are defined, then deduce them ourselves
                    if (!datasetNode->HasProperty(L"id-column") &&
                        !datasetNode->HasProperty(L"date-columns") &&
                        !datasetNode->HasProperty(L"continuous-columns") &&
                        !datasetNode->HasProperty(L"categorical-columns"))
                        {
                        importDefines = Dataset::ImportInfoFromPreview(
                            Dataset::ReadColumnInfo(path, importDefines, std::nullopt, worksheet));
                        fillImportDefines();
                        }
                    else
                        {
                        importDefines.ContinuousMDRecodeValue(
                                datasetNode->GetProperty(L"continuous-md-recode-value")->
                                    GetValueNumber(std::numeric_limits<double>::quiet_NaN())).
                            IdColumn(idColumn).
                            DateColumns(dateInfo).
                            ContinuousColumns(continuousVars).
                            CategoricalColumns(catInfo);
                        }

                    // import using the user-provided parser or deduce from the file extension
                    const auto fileExt(wxFileName(path).GetExt());
                    if (importer.CmpNoCase(L"csv") == 0 ||
                        fileExt.CmpNoCase(L"csv") == 0)
                        { dataset->ImportCSV(path, importDefines); }
                    else if (importer.CmpNoCase(L"tsv") == 0 ||
                        fileExt.CmpNoCase(L"tsv") == 0 ||
                        fileExt.CmpNoCase(L"txt") == 0)
                        { dataset->ImportTSV(path, importDefines); }
                    else if (importer.CmpNoCase(L"xlsx") == 0 ||
                        fileExt.CmpNoCase(L"xlsx") == 0)
                        {
                        dataset->ImportExcel(path, worksheet, importDefines);
                        }
                    else
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a valid importer specified.")).
                                    ToUTF8());
                        }

                    m_datasets.insert_or_assign(dsName, dataset);
                    // recode values, build subsets and pivots, etc.
                    LoadDatasetTransformations(datasetNode, dataset);
                    }
                }
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadProConRoadmap(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Pro & Con Roadmap."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto pcRoadmap = std::make_shared<ProConRoadmap>(canvas);
            pcRoadmap->SetData(foundPos->second,
                variablesNode->GetProperty(L"positive")->GetValueString(),
                (variablesNode->HasProperty(L"positive-aggregate") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"positive-aggregate")->GetValueString())) :
                    std::nullopt),
                variablesNode->GetProperty(L"negative")->GetValueString(),
                (variablesNode->HasProperty(L"negative-aggregate") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"negative-aggregate")->GetValueString())) :
                    std::nullopt),
                (graphNode->GetProperty(L"minimum-count")->IsValueNumber() ?
                    std::optional<double>(graphNode->GetProperty(L"minimum-count")->
                                          GetValueNumber(1)) :
                    std::nullopt));

            if (graphNode->GetProperty(L"positive-legend-label")->IsValueString())
                {
                pcRoadmap->SetPositiveLegendLabel(
                    graphNode->GetProperty(L"positive-legend-label")->GetValueString());
                }
            if (graphNode->GetProperty(L"negative-legend-label")->IsValueString())
                {
                pcRoadmap->SetNegativeLegendLabel(
                    graphNode->GetProperty(L"negative-legend-label")->GetValueString());
                }

            LoadPen(graphNode->GetProperty(L"road-pen"), pcRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"), pcRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement =
                ReportEnumConvert::ConvertLabelPlacement(
                    graphNode->GetProperty(L"label-placement")->GetValueString());
            if (labelPlacement.has_value())
                { pcRoadmap->SetLabelPlacement(labelPlacement.value()); }

            const auto laneSepStyle =
                ReportEnumConvert::ConvertLaneSeparatorStyle(
                    graphNode->GetProperty(L"lane-separator-style")->GetValueString());
            if (laneSepStyle.has_value())
                { pcRoadmap->SetLaneSeparatorStyle(laneSepStyle.value()); }

            const auto roadStopTheme =
                ReportEnumConvert::ConvertRoadStopTheme(
                    graphNode->GetProperty(L"road-stop-theme")->GetValueString());
            if (roadStopTheme.has_value())
                { pcRoadmap->SetRoadStopTheme(roadStopTheme.value()); }

            const auto markerLabelDisplay =
                ReportEnumConvert::ConvertMarkerLabelDisplay(
                    graphNode->GetProperty(L"marker-label-display")->GetValueString());
            if (markerLabelDisplay.has_value())
                { pcRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value()); }

            if (graphNode->GetProperty(L"default-caption")->GetValueBool())
                { pcRoadmap->AddDefaultCaption(); }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pcRoadmap);
            return pcRoadmap;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for Pro & Con Roadmap.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadGanttChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Gantt chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto dateInterval =
                ReportEnumConvert::ConvertDateInterval(graphNode->GetProperty(L"date-interval")->GetValueString());
            const auto fyType =
                ReportEnumConvert::ConvertFiscalYear(graphNode->GetProperty(L"fy-type")->GetValueString());

            auto ganttChart = std::make_shared<GanttChart>(canvas);
            ganttChart->SetData(foundPos->second,
                dateInterval.has_value() ? dateInterval.value() : DateInterval::FiscalQuarterly,
                fyType.has_value() ? fyType.value() : FiscalYear::USBusiness,
                ExpandConstants(variablesNode->GetProperty(L"task")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"start-date")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"end-date")->GetValueString()),
                (variablesNode->HasProperty(L"resource") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"resource")->GetValueString())) :
                    std::nullopt),
                (variablesNode->HasProperty(L"description") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"description")->GetValueString())) :
                    std::nullopt),
                (variablesNode->HasProperty(L"completion") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"completion")->GetValueString())) :
                    std::nullopt),
                (variablesNode->HasProperty(L"group") ?
                    std::optional<wxString>(
                        ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString())) :
                    std::nullopt));

            const auto taskLabelDisplay =
                ReportEnumConvert::ConvertTaskLabelDisplay(
                    graphNode->GetProperty(L"task-label-display")->GetValueString());
            if (taskLabelDisplay.has_value())
                { ganttChart->SetLabelDisplay(taskLabelDisplay.value()); }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, ganttChart);
            return ganttChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for Gannt chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadLRRoadmap(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(_DT(L"dataset"))->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Linear Regression Roadmap."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto pValueColumn =
                ExpandConstants(variablesNode->GetProperty(L"p-value")->GetValueString());
            const auto dvName =
                ExpandConstants(variablesNode->GetProperty(L"dependent-variable-name")->GetValueString());

            int lrPredictors{ 0 };
            if (graphNode->HasProperty(L"predictors-to-include"))
                {
                const auto preds = graphNode->GetProperty(L"predictors-to-include")->GetValueStringVector();
                for (const auto& pred : preds)
                    {
                    if (pred.CmpNoCase(_DT(L"positive")) == 0)
                        { lrPredictors |= Influence::InfluencePositive; }
                    else if (pred.CmpNoCase(_DT(L"negative")) == 0)
                        { lrPredictors |= Influence::InfluenceNegative; }
                    else if (pred.CmpNoCase(_DT(L"neutral")) == 0)
                        { lrPredictors |= Influence::InfluenceNeutral; }
                    else if (pred.CmpNoCase(_DT(L"all")) == 0)
                        { lrPredictors |= Influence::InfluenceAll; }
                    }
                }

            auto lrRoadmap = std::make_shared<LRRoadmap>(canvas);
            lrRoadmap->SetData(foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"predictor")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"coefficient")->GetValueString()),
                (pValueColumn.length() ? std::optional<wxString>(pValueColumn) : std::nullopt),
                (graphNode->GetProperty(L"p-value-threshold")->IsValueNumber() ?
                    std::optional<double>(graphNode->GetProperty(L"p-value-threshold")->
                                          GetValueNumber(0.05)) :
                    std::nullopt),
                (lrPredictors == 0 ?
                    std::nullopt :
                    std::optional<Influence>(static_cast<Influence>(lrPredictors)) ),
                (dvName.length() ? std::optional<wxString>(dvName) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"road-pen"), lrRoadmap->GetRoadPen());
            LoadPen(graphNode->GetProperty(L"lane-separator-pen"), lrRoadmap->GetLaneSeparatorPen());

            const auto labelPlacement =
                ReportEnumConvert::ConvertLabelPlacement(
                    graphNode->GetProperty(L"label-placement")->GetValueString());
            if (labelPlacement.has_value())
                { lrRoadmap->SetLabelPlacement(labelPlacement.value()); }

            const auto laneSepStyle =
                ReportEnumConvert::ConvertLaneSeparatorStyle(
                    graphNode->GetProperty(L"lane-separator-style")->GetValueString());
            if (laneSepStyle.has_value())
                { lrRoadmap->SetLaneSeparatorStyle(laneSepStyle.value()); }

            const auto roadStopTheme =
                ReportEnumConvert::ConvertRoadStopTheme(
                    graphNode->GetProperty(L"road-stop-theme")->GetValueString());
            if (roadStopTheme.has_value())
                { lrRoadmap->SetRoadStopTheme(roadStopTheme.value()); }

            const auto markerLabelDisplay =
                ReportEnumConvert::ConvertMarkerLabelDisplay(
                    graphNode->GetProperty(L"marker-label-display")->GetValueString());
            if (markerLabelDisplay.has_value())
                { lrRoadmap->SetMarkerLabelDisplay(markerLabelDisplay.value()); }

            if (graphNode->GetProperty(L"default-caption")->GetValueBool())
                { lrRoadmap->AddDefaultCaption(); }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, lrRoadmap);
            return lrRoadmap;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for Linear Regression Roadmap.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadLikertChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Likert chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            std::vector<wxString> questions;
            const auto questionVars =
                ExpandConstants(variablesNode->GetProperty(L"questions")->GetValueStringVector());
            for (const auto& questionVar : questionVars)
                {
                auto convertedVars = ExpandColumnSelections(questionVar, foundPos->second);
                if (convertedVars)
                    {
                    questions.insert(questions.cend(),
                        convertedVars.value().cbegin(), convertedVars.value().cend());
                    }
                else
                    { questions.push_back(questionVar); }
                }
            const auto groupVarName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());

            // get the survey format
            auto surveyFormat =
                ReportEnumConvert::ConvertLikertSurveyQuestionFormat(
                    graphNode->GetProperty(L"survey-format")->GetValueString());
            if (!surveyFormat.has_value())
                {
                surveyFormat = LikertChart::DeduceScale(foundPos->second, questions, groupVarName);
                }

            if (graphNode->GetProperty(L"simplify")->GetValueBool())
                {
                surveyFormat =
                    LikertChart::Simplify(foundPos->second, questions, surveyFormat.value());
                }

            if (graphNode->GetProperty(L"apply-default-labels")->GetValueBool())
                {
                LikertChart::SetLabels(foundPos->second, questions,
                    LikertChart::CreateLabels(surveyFormat.value()));
                }

            auto likertChart = std::make_shared<LikertChart>(canvas,
                surveyFormat.value(),
                ConvertColor(graphNode->GetProperty(L"negative-color")),
                ConvertColor(graphNode->GetProperty(L"positive-color")),
                ConvertColor(graphNode->GetProperty(L"neutral-color")),
                ConvertColor(graphNode->GetProperty(L"no-response-color")));

            likertChart->SetData(foundPos->second,
                questions,
                (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt));

            likertChart->ShowResponseCounts(
                graphNode->GetProperty(L"show-response-counts")->GetValueBool(false));
            likertChart->ShowPercentages(
                graphNode->GetProperty(L"show-percentages")->GetValueBool(true));
            likertChart->ShowSectionHeaders(
                graphNode->GetProperty(L"show-section-headers")->GetValueBool(true));
            likertChart->SetBarSizesToRespondentSize(
                graphNode->GetProperty(L"adjust-bar-widths-to-respondent-size")->GetValueBool(false));

            if (graphNode->HasProperty(L"positive-label"))
                {
                likertChart->SetPositiveHeader(
                    graphNode->GetProperty(L"positive-label")->GetValueString());
                }
            if (graphNode->HasProperty(L"negative-label"))
                {
                likertChart->SetNegativeHeader(
                    graphNode->GetProperty(L"negative-label")->GetValueString());
                }
            if (graphNode->HasProperty(L"no-response-label"))
                {
                likertChart->SetNoResponseHeader(
                    graphNode->GetProperty(L"no-response-label")->GetValueString());
                }

            const auto questionBracketNodes =
                graphNode->GetProperty(L"question-brackets")->GetValueArrayObject();
            for (const auto& questionBracketNode : questionBracketNodes)
                {
                likertChart->AddQuestionsBracket(
                    {
                    questionBracketNode->GetProperty(L"start")->GetValueString(),
                    questionBracketNode->GetProperty(L"end")->GetValueString(),
                    questionBracketNode->GetProperty(L"title")->GetValueString()
                    }
                    );
                }

            LoadGraph(graphNode, canvas, currentRow, currentColumn, likertChart);
            return likertChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for Likert chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadWCurvePlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for W-curve plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());

            auto wcurvePlot = std::make_shared<WCurvePlot>(canvas,
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            wcurvePlot->SetData(foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"y")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"x")->GetValueString()),
                (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt));
            if (graphNode->HasProperty(L"time-interval-label"))
                {
                wcurvePlot->SetTimeIntervalLabel(
                    graphNode->GetProperty(L"time-interval-label")->GetValueString());
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, wcurvePlot);
            return wcurvePlot;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for W-curve plot.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadCandlestickPlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for candlestick plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            auto candlestickPlot = std::make_shared<CandlestickPlot>(canvas);
            candlestickPlot->SetData(foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"date")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"open")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"high")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"low")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"close")->GetValueString()));

            const auto plotType =
                ReportEnumConvert::ConvertCandlestickPlotType(graphNode->GetProperty(L"plot-type")->GetValueString());
            if (plotType.has_value())
                { candlestickPlot->SetPlotType(plotType.value()); }

            LoadBrush(graphNode->GetProperty(L"gain-brush"), candlestickPlot->GetGainBrush());
            LoadBrush(graphNode->GetProperty(L"loss-brush"), candlestickPlot->GetLossBrush());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, candlestickPlot);
            return candlestickPlot;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for candlestick plot.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadLinePlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for line plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());

            auto linePlot = std::make_shared<LinePlot>(canvas,
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")),
                LoadLineStyleScheme(graphNode->GetProperty(L"line-scheme")));
            linePlot->SetData(foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"y")->GetValueString()),
                ExpandConstants(variablesNode->GetProperty(L"x")->GetValueString()),
                (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt));
            LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
            return linePlot;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for line plot.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadHeatMap(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for heatmap."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto groupVarName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());

            auto heatmap = std::make_shared<HeatMap>(canvas,
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            heatmap->SetData(foundPos->second,
                ExpandConstants(variablesNode->GetProperty(L"continuous")->GetValueString()),
                (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt),
                graphNode->GetProperty(L"group-column-count")->GetValueNumber(5));

            heatmap->ShowGroupHeaders(graphNode->GetProperty(L"show-group-header")->GetValueBool(true));
            heatmap->SetGroupHeaderPrefix(graphNode->GetProperty(L"group-header-prefix")->GetValueString());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, heatmap);
            return heatmap;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for heatmap.").ToUTF8());
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadBarChart(const wxSimpleJSON::Ptr_t& graphNode,
        std::shared_ptr<Graphs::BarChart> barChart)
        {
        const auto boxEffect = ReportEnumConvert::ConvertBoxEffect(
                graphNode->GetProperty(L"box-effect")->GetValueString());
        if (boxEffect)
            { barChart->SetBarEffect(boxEffect.value()); }

        // sorting
        const auto sortNode = graphNode->GetProperty(L"bar-sort");
        if (sortNode->IsOk())
            {
            const auto sortDirection =
                sortNode->GetProperty(L"direction")->GetValueString().CmpNoCase(_DT(L"ascending")) == 0 ?
                SortDirection::SortAscending : SortDirection::SortDescending;
            const auto byNode = sortNode->GetProperty(L"by");
            if (byNode->IsOk())
                {
                const auto sortBy = (byNode->GetValueString().CmpNoCase(L"length") == 0 ?
                    std::optional<BarChart::BarSortComparison>(BarChart::BarSortComparison::SortByBarLength) :
                    byNode->GetValueString().CmpNoCase(L"label") == 0 ?
                    std::optional<BarChart::BarSortComparison>(BarChart::BarSortComparison::SortByAxisLabel) :
                    std::nullopt);
                if (!sortBy.has_value())
                    {
                    throw std::runtime_error(wxString::Format(
                        _(L"'%s': invalid bar sorting 'by' method."), byNode->GetValueString()).ToUTF8());
                    }
                barChart->SortBars(sortBy.value(), sortDirection);
                }
            // or is sorting by a list of labels with a custom order
            else if (const auto labelsNode = sortNode->GetProperty(L"labels");
                labelsNode->IsOk() && labelsNode->IsValueArray())
                { barChart->SortBars(labelsNode->GetValueStringVector(), sortDirection); }
            else
                {
                throw std::runtime_error(
                    _(L"Sorting method not defined for bar sort.").ToUTF8());
                }
            }

        if (graphNode->HasProperty(L"ghost-opacity"))
            { barChart->SetGhostOpacity(graphNode->GetProperty(L"ghost-opacity")->GetValueNumber(32)); }

        // showcasing
        if (const auto showcaseNode = graphNode->GetProperty(L"showcase-bars");
            showcaseNode->IsOk() && showcaseNode->IsValueArray())
            { barChart->ShowcaseBars(showcaseNode->GetValueStringVector()); }

        // decals to add to the bars
        const auto decalsNode = graphNode->GetProperty(L"decals");
        if (decalsNode->IsOk() && decalsNode->IsValueArray())
            {
            const auto decals = decalsNode->GetValueArrayObject();
            for (const auto& decal : decals)
                {
                const auto barPos = barChart->FindBar(decal->GetProperty(L"bar")->GetValueString());
                if (barPos.has_value())
                    {
                    const auto blockIndex = decal->GetProperty(L"block")->GetValueNumber(0);
                    const auto decalLabel = LoadLabel(decal->GetProperty(L"decal"), GraphItems::Label());
                    if (decalLabel != nullptr &&
                        blockIndex < barChart->GetBars().at(barPos.value()).GetBlocks().size())
                        { barChart->GetBars().at(barPos.value()).GetBlocks().at(blockIndex).SetDecal(*decalLabel); }
                    }
                }
            }

        // bar groups
        const auto barGroupPlacement =
        ReportEnumConvert::ConvertLabelPlacement(graphNode->GetProperty(L"bar-group-placement")->GetValueString());
        if (barGroupPlacement.has_value())
            { barChart->SetBarGroupPlacement(barGroupPlacement.value()); }

        const auto barGroupsNode = graphNode->GetProperty(L"bar-groups");
        if (barGroupsNode->IsOk() && barGroupsNode->IsValueArray())
            {
            const auto barGroups = barGroupsNode->GetValueArrayObject();
            for (const auto& barGroup : barGroups)
                {
                if (barGroup->IsOk())
                    {
                    BarChart::BarGroup bGroup;
                    bGroup.m_barColor = (
                        ConvertColor(barGroup->GetProperty(L"color")));
                    if (!bGroup.m_barColor.IsOk() && barChart->GetColorScheme() != nullptr)
                        { bGroup.m_barColor = barChart->GetColorScheme()->GetColor(0); }
                    LoadBrush(barGroup->GetProperty(L"brush"), bGroup.m_barBrush);
                    if (!bGroup.m_barBrush.IsOk() && barChart->GetBrushScheme() != nullptr)
                        { bGroup.m_barBrush = barChart->GetBrushScheme()->GetBrush(0); }
                    bGroup.m_barDecal = barGroup->GetProperty(L"decal")->GetValueString();

                    if (barGroup->GetProperty(L"start")->IsValueNumber())
                        {
                        bGroup.m_barPositions.first = barGroup->GetProperty(L"start")->GetValueNumber();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"start")->GetValueString());
                        if (foundBar.has_value())
                            { bGroup.m_barPositions.first = foundBar.value(); }
                        else
                            {
                            throw std::runtime_error(wxString::Format(
                                _(L"'%s': bar label not found when adding bar group."),
                                barGroup->GetProperty(L"start")->GetValueString()).ToUTF8());
                            }
                        }
                    if (barGroup->GetProperty(L"end")->IsValueNumber())
                        {
                        bGroup.m_barPositions.second = barGroup->GetProperty(L"end")->GetValueNumber();
                        }
                    else
                        {
                        const auto foundBar =
                            barChart->FindBar(barGroup->GetProperty(L"end")->GetValueString());
                        if (foundBar.has_value())
                            { bGroup.m_barPositions.second = foundBar.value(); }
                        else
                            {
                            throw std::runtime_error(wxString::Format(
                                _(L"'%s': bar label not found when adding bar group."),
                                barGroup->GetProperty(L"end")->GetValueString()).ToUTF8());
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
            const auto barBrackets = barBracketsNode->GetValueArrayObject();
            for (const auto& barBracket : barBrackets)
                {
                if (barBracket->HasProperty(L"start-block-re") &&
                    barBracket->HasProperty(L"end-block-re"))
                    {
                    barChart->AddFirstBarBracketRE(
                        barBracket->GetProperty(L"start-block-re")->GetValueString(),
                        barBracket->GetProperty(L"end-block-re")->GetValueString(),
                        barBracket->GetProperty(L"label")->GetValueString());
                    }
                else
                    {
                    barChart->AddFirstBarBracket(
                        barBracket->GetProperty(L"start-block")->GetValueString(),
                        barBracket->GetProperty(L"end-block")->GetValueString(),
                        barBracket->GetProperty(L"label")->GetValueString());
                    }
                }
            }
        if (const auto barBracketsNode = graphNode->GetProperty(L"last-bar-brackets");
            barBracketsNode->IsOk() && barBracketsNode->IsValueArray())
            {
            const auto barBrackets = barBracketsNode->GetValueArrayObject();
            for (const auto& barBracket : barBrackets)
                {
                if (barBracket->HasProperty(L"start-block-re") &&
                    barBracket->HasProperty(L"end-block-re"))
                    {
                    barChart->AddLastBarBracketRE(
                        barBracket->GetProperty(L"start-block-re")->GetValueString(),
                        barBracket->GetProperty(L"end-block-re")->GetValueString(),
                        barBracket->GetProperty(L"label")->GetValueString());
                    }
                else
                    {
                    barChart->AddLastBarBracket(
                        barBracket->GetProperty(L"start-block")->GetValueString(),
                        barBracket->GetProperty(L"end-block")->GetValueString(),
                        barBracket->GetProperty(L"label")->GetValueString());
                    }
                }
            }

        const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
            graphNode->GetProperty(L"bar-label-display")->GetValueString());
        if (binLabel.has_value())
            { barChart->SetBinLabelDisplay(binLabel.value()); }

        barChart->SetBinLabelSuffix(graphNode->GetProperty(L"bar-label-suffix")->GetValueString());

        // bar icons
        const auto barIconsNode = graphNode->GetProperty(L"bar-icons");
        if (barIconsNode->IsOk() && barIconsNode->IsValueArray())
            {
            const auto barIcons = barIconsNode->GetValueArrayObject();
            for (const auto& barIcon : barIcons)
                {
                if (barIcon->IsOk())
                    {
                    auto path = barIcon->GetProperty(L"image")->GetValueString();
                    if (path.length())
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

                    barChart->AddBarIcon(
                        barIcon->GetProperty("label")->GetValueString(), Image::LoadFile(path));
                    }
                }
            }

        barChart->IncludeSpacesBetweenBars(graphNode->GetProperty(L"include-spaces-between-bars")->GetValueBool(true));

        if (graphNode->HasProperty(L"constrain-scaling-axis-to-bars") &&
            graphNode->GetProperty(L"constrain-scaling-axis-to-bars")->GetValueBool())
            { barChart->ConstrainScalingAxisToBars(); }

        if (graphNode->GetProperty(L"apply-brushes-to-ungrouped-bars")->GetValueBool() &&
            !barChart->IsUsingGrouping() && barChart->GetBrushScheme() &&
            barChart->GetBrushScheme()->GetBrushes().size())
            {
            if (barChart->GetBarOrientation() == Orientation::Vertical)
                {
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (blocks.size())
                        { blocks.front().GetBrush() = barChart->GetBrushScheme()->GetBrush(i); }
                    }
                }
            else
                {
                // apply brush in reverse because the origin in going upward, but the client
                // sees the bars as going downward
                for (size_t i = 0; i < barChart->GetBars().size(); ++i)
                    {
                    auto& blocks = barChart->GetBars()[i].GetBlocks();
                    if (blocks.size())
                        {
                        assert(
                            barChart->GetBrushScheme()->GetBrushes().size() >= (1 + i) &&
                            L"Bad brush mapping for bar chart!");
                        blocks.front().GetBrush() =
                            barChart->GetBrushScheme()->GetBrush(
                                barChart->GetBrushScheme()->GetBrushes().size() - 1 - i);
                        }
                    }
                }
            }
        }

    //---------------------------------------------------
    std::unique_ptr<GraphItems::FillableShape>
        ReportBuilder::LoadFillableShape(const wxSimpleJSON::Ptr_t& shapeNode)
        {
        const auto loadedShape = ReportEnumConvert::ConvertIcon(shapeNode->GetProperty(L"icon")->GetValueString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: unknown icon for fillable shape."),
                    shapeNode->GetProperty(L"icon")->GetValueString()).ToUTF8());
            }

        wxSize sz(32, 32);
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->GetValueNumber(32);
            sz.y = sizeNode->GetProperty(L"height")->GetValueNumber(32);
            }

        wxPen pen(*wxBLACK_PEN);
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(*wxBLACK_BRUSH);
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        double fillPercent{ math_constants::empty };
        const auto fillPercentNode = shapeNode->GetProperty(L"fill-percent");
        if (fillPercentNode->IsOk())
            {
            if (fillPercentNode->IsValueNumber())
                {
                fillPercent = fillPercentNode->GetValueNumber(fillPercent);
                }
            else if (fillPercentNode->IsValueString())
                {
                const auto numberVal = ExpandNumericConstant(fillPercentNode->GetValueString());
                if (numberVal)
                    { fillPercent = numberVal.value(); }
                }
            }

        auto shapeLabel = LoadLabel(shapeNode->GetProperty(L"text"), GraphItems::Label());

        auto sh = std::make_unique<FillableShape>(
            GraphItemInfo((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{})).
            Anchoring(Anchoring::TopLeftCorner).
            Pen(pen).Brush(brush).
            FontColor((shapeLabel != nullptr ? shapeLabel->GetFontColor() : *wxBLACK)),
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
    std::unique_ptr<GraphItems::Shape>
    ReportBuilder::LoadShape(const wxSimpleJSON::Ptr_t& shapeNode)
        {
        const auto loadedShape = ReportEnumConvert::ConvertIcon(shapeNode->GetProperty(L"icon")->GetValueString());
        if (!loadedShape.has_value())
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: unknown icon for shape."),
                    shapeNode->GetProperty(L"icon")->GetValueString()).ToUTF8());
            }

        wxSize sz(32, 32);
        const auto sizeNode = shapeNode->GetProperty(L"size");
        if (sizeNode->IsOk())
            {
            sz.x = sizeNode->GetProperty(L"width")->GetValueNumber(32);
            sz.y = sizeNode->GetProperty(L"height")->GetValueNumber(32);
            }

        wxPen pen(*wxBLACK_PEN);
        LoadPen(shapeNode->GetProperty(L"pen"), pen);

        wxBrush brush(*wxWHITE_BRUSH);
        LoadBrush(shapeNode->GetProperty(L"brush"), brush);

        auto shapeLabel = LoadLabel(shapeNode->GetProperty(L"text"), GraphItems::Label());

        auto sh = std::make_unique<Shape>(
            GraphItemInfo((shapeLabel != nullptr ? shapeLabel->GetText() : wxString{})).
            Anchoring(Anchoring::TopLeftCorner).
            Pen(pen).Brush(brush).
            FontColor((shapeLabel != nullptr ? shapeLabel->GetFontColor() : *wxBLACK)),
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
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadHistogram(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for histogram."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto contVarName = ExpandConstants(variablesNode->GetProperty(L"aggregate")->GetValueString());
            const auto groupName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());

            const auto binMethod = ReportEnumConvert::ConvertBinningMethod(
                graphNode->GetProperty(L"binning-method")->GetValueString());

            const auto binIntervalDisplay = ReportEnumConvert::ConvertIntervalDisplay(
                graphNode->GetProperty(L"interval-display")->GetValueString());

            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->GetValueString());

            const auto rounding = ReportEnumConvert::ConvertRoundingMethod(
                graphNode->GetProperty(L"rounding")->GetValueString());

            const std::optional<double> startBinsValue = graphNode->HasProperty(L"bins-start") ?
                std::optional<double>(graphNode->GetProperty(L"bins-start")->GetValueNumber()) :
                std::nullopt;
            const std::optional<size_t> suggestedBinCount = graphNode->HasProperty(L"suggested-bin-count") ?
                std::optional<double>(graphNode->GetProperty(L"suggested-bin-count")->GetValueNumber()) :
                std::nullopt;
            const std::optional<size_t> maxBinCount = graphNode->HasProperty(L"max-bin-count") ?
                std::optional<double>(graphNode->GetProperty(L"max-bin-count")->GetValueNumber()) :
                std::nullopt;

            auto histo = std::make_shared<Histogram>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")) );

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->GetValueString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                { histo->SetBarOrientation(Orientation::Horizontal); }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                { histo->SetBarOrientation(Orientation::Vertical); }

            histo->SetData(foundPos->second, contVarName,
                (groupName.length() ? std::optional<wxString>(groupName) : std::nullopt),
                binMethod.value_or(Histogram::BinningMethod::BinByIntegerRange),
                rounding.value_or(RoundingMethod::NoRounding),
                binIntervalDisplay.value_or(Histogram::IntervalDisplay::Cutpoints),
                binLabel.value_or(BinLabelDisplay::BinValue),
                graphNode->GetProperty(L"show-full-range")->GetValueBool(true),
                startBinsValue, std::make_pair(suggestedBinCount, maxBinCount) );

            LoadBarChart(graphNode, histo);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, histo);
            return histo;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for histogram.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadCategoricalBarChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for bar chart."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = ExpandConstants(variablesNode->GetProperty(L"aggregate")->GetValueString());
            const auto groupName = ExpandConstants(variablesNode->GetProperty(L"group")->GetValueString());
            const auto categoryName = ExpandConstants(variablesNode->GetProperty(L"category")->GetValueString());
            const auto binLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"bar-label-display")->GetValueString());

            auto barChart = std::make_shared<CategoricalBarChart>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")) );

            const auto bOrientation = graphNode->GetProperty(L"bar-orientation")->GetValueString();
            if (bOrientation.CmpNoCase(L"horizontal") == 0)
                { barChart->SetBarOrientation(Orientation::Horizontal); }
            else if (bOrientation.CmpNoCase(L"vertical") == 0)
                { barChart->SetBarOrientation(Orientation::Vertical); }

            barChart->SetData(foundPos->second, categoryName,
                (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt),
                (groupName.length() ? std::optional<wxString>(groupName) : std::nullopt),
                binLabel.has_value() ? binLabel.value() : BinLabelDisplay::BinValue);

            LoadBarChart(graphNode, barChart);
            LoadGraph(graphNode, canvas, currentRow, currentColumn, barChart);
            return barChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for bar chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadSankeyDiagram(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Sankey diagram."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto fromVarName = ExpandConstants(variablesNode->GetProperty(L"from")->GetValueString());
            const auto toColName = ExpandConstants(variablesNode->GetProperty(L"to")->GetValueString());

            const auto fromWeightVarName = ExpandConstants(
                variablesNode->GetProperty(L"from-weight")->GetValueString());
            const auto toWeightColName = ExpandConstants(
                variablesNode->GetProperty(L"to-weight")->GetValueString());

            const auto fromGroupVarName = ExpandConstants(
                variablesNode->GetProperty(L"from-group")->GetValueString());

            auto sankey = std::make_shared<SankeyDiagram>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")) );

            const auto groupLabelDisplay = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"group-label-display")->GetValueString());
            if (groupLabelDisplay)
                { sankey->SetGroupLabelDisplay(groupLabelDisplay.value()); }

            const auto groupHeaderDisplay = ReportEnumConvert::ConvertGraphColumnHeader(
                graphNode->GetProperty(L"group-header-display")->GetValueString());
            if (groupHeaderDisplay)
                { sankey->SetColumnHeaderDisplay(groupHeaderDisplay.value()); }

            if (graphNode->HasProperty(L"column-headers"))
                {
                std::vector<wxString> columnHeader = graphNode->GetProperty(L"column-headers")->GetValueStringVector();
                std::transform(columnHeader.begin(), columnHeader.end(), columnHeader.begin(),
                    [this](const auto& val)
                    { return ExpandConstants(val); });
                sankey->SetColumnHeaders(columnHeader);
                }

            const auto flowShape = ReportEnumConvert::ConvertFlowShape(
                graphNode->GetProperty(L"flow-shape")->GetValueString());
            if (flowShape)
                { sankey->SetFlowShape(flowShape.value()); }

            sankey->SetData(foundPos->second, fromVarName, toColName,
                fromWeightVarName.length() ? std::optional<wxString>(fromWeightVarName) : std::nullopt,
                toWeightColName.length() ? std::optional<wxString>(toWeightColName) : std::nullopt,
                fromGroupVarName.length() ? std::optional<wxString>(fromGroupVarName) : std::nullopt);

            LoadGraph(graphNode, canvas, currentRow, currentColumn, sankey);
            return sankey;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for Sankey diagram.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadWordCloud(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for word cloud."),
                                 dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = ExpandConstants(variablesNode->GetProperty(L"aggregate")->GetValueString());
            const auto wordColName = ExpandConstants(variablesNode->GetProperty(L"words")->GetValueString());

            auto wordCloud = std::make_shared<WordCloud>(canvas,
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")) );

            wordCloud->SetData(foundPos->second, wordColName,
                (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt));

            LoadGraph(graphNode, canvas, currentRow, currentColumn, wordCloud);
            return wordCloud;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for word cloud.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadBoxPlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for box plot."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = ExpandConstants(variablesNode->GetProperty(L"aggregate")->GetValueString());
            const auto groupVar1Name = ExpandConstants(variablesNode->GetProperty(L"group-1")->GetValueString());

            auto boxPlot = std::make_shared<BoxPlot>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
            boxPlot->SetData(foundPos->second,
                aggVarName,
                (groupVar1Name.length() ? std::optional<wxString>(groupVar1Name) : std::nullopt));

            const auto boxEffect = ReportEnumConvert::ConvertBoxEffect(
                graphNode->GetProperty(L"box-effect")->GetValueString());
            if (boxEffect)
                { boxPlot->SetBoxEffect(boxEffect.value()); }

            boxPlot->ShowAllPoints(graphNode->GetProperty(L"show-all-points")->GetValueBool());
            boxPlot->ShowLabels(graphNode->GetProperty(L"show-labels")->GetValueBool());

            LoadGraph(graphNode, canvas, currentRow, currentColumn, boxPlot);
            return boxPlot;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for box plot.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadPieChart(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for pie chart."), dsName).ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk())
            {
            const auto aggVarName = ExpandConstants(variablesNode->GetProperty(L"aggregate")->GetValueString());
            const auto groupVar1Name = ExpandConstants(variablesNode->GetProperty(L"group-1")->GetValueString());
            const auto groupVar2Name = ExpandConstants(variablesNode->GetProperty(L"group-2")->GetValueString());

            auto pieChart = std::make_shared<PieChart>(canvas,
                LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
                LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
            pieChart->SetData(foundPos->second,
                (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt),
                groupVar1Name,
                (groupVar2Name.length() ? std::optional<wxString>(groupVar2Name) : std::nullopt));

            LoadPen(graphNode->GetProperty(L"inner-pie-line-pen"),
                    pieChart->GetInnerPieConnectionLinePen());

            const auto labelPlacement =
                ReportEnumConvert::ConvertLabelPlacement(graphNode->GetProperty(L"label-placement")->GetValueString());
            if (labelPlacement.has_value())
                { pieChart->SetLabelPlacement(labelPlacement.value()); }

            const auto outerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-pie-midpoint-label-display")->GetValueString());
            if (outerPieMidLabel.has_value())
                { pieChart->SetOuterPieMidPointLabelDisplay(outerPieMidLabel.value()); }

            const auto innerPieMidLabel = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"inner-pie-midpoint-label-display")->GetValueString());
            if (innerPieMidLabel.has_value())
                { pieChart->SetInnerPieMidPointLabelDisplay(innerPieMidLabel.value()); }

            const auto outerLabelDisplay = ReportEnumConvert::ConvertBinLabelDisplay(
                graphNode->GetProperty(L"outer-label-display")->GetValueString());
            if (outerLabelDisplay.has_value())
                { pieChart->SetOuterLabelDisplay(outerLabelDisplay.value()); }

            if (graphNode->HasProperty(L"include-inner-pie-labels"))
                {
                pieChart->ShowInnerPieLabels(
                    graphNode->GetProperty(L"include-inner-pie-labels")->GetValueBool());
                }

            if (graphNode->HasProperty(L"include-outer-pie-labels"))
                {
                pieChart->ShowOuterPieLabels(
                    graphNode->GetProperty(L"include-outer-pie-labels")->GetValueBool());
                }

            if (graphNode->HasProperty(L"color-labels"))
                {
                pieChart->UseColorLabels(
                    graphNode->GetProperty(L"color-labels")->GetValueBool());
                }

            if (graphNode->HasProperty(L"ghost-opacity"))
                {
                pieChart->SetGhostOpacity(
                    graphNode->GetProperty(L"ghost-opacity")->GetValueNumber(32));
                }

            // margin notes
            if (graphNode->HasProperty(L"left-margin-note"))
                {
                auto marginLabel = LoadLabel(graphNode->GetProperty(L"left-margin-note"),
                                             GraphItems::Label());
                if (marginLabel != nullptr)
                    { pieChart->GetLeftMarginNote() = *marginLabel; }
                }
            if (graphNode->HasProperty(L"right-margin-note"))
                {
                auto marginLabel = LoadLabel(graphNode->GetProperty(L"right-margin-note"),
                                             GraphItems::Label());
                if (marginLabel != nullptr)
                    { pieChart->GetRightMarginNote() = *marginLabel; }
                }

            if (const auto pieEffect =
                ReportEnumConvert::ConvertPieSliceEffect(
                    graphNode->GetProperty(L"pie-slice-effect")->GetValueString());
                pieEffect.has_value())
                { pieChart->SetPieSliceEffect(pieEffect.value()); }

            pieChart->SetDynamicMargins(graphNode->GetProperty(L"dynamic-margins")->GetValueBool());

            // showcase the slices
            const auto showcaseNode = graphNode->GetProperty(L"showcase-slices");
            if (showcaseNode->IsValueArray())
                {
                const auto peri =
                    ReportEnumConvert::ConvertPerimeter(
                        graphNode->GetProperty(L"showcased-ring-labels")->GetValueString());
                pieChart->ShowcaseOuterPieSlices(
                    showcaseNode->GetValueStringVector(),
                    peri.has_value() ? peri.value() : Perimeter::Outer);
                }
            else if (showcaseNode->IsOk())
                {
                const auto pieType = showcaseNode->GetProperty(L"pie")->GetValueString();
                const auto categoryType = showcaseNode->GetProperty(L"category")->GetValueString();
                const auto peri =
                    ReportEnumConvert::ConvertPerimeter(
                        graphNode->GetProperty(L"showcased-ring-labels")->GetValueString());
                if (pieType.CmpNoCase(L"inner") == 0)
                    {
                    if (categoryType.CmpNoCase(L"smallest") == 0)
                        {
                        pieChart->ShowcaseSmallestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->GetValueBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->GetValueBool() );
                        }
                    else if (categoryType.CmpNoCase(L"largest") == 0)
                        {
                        pieChart->ShowcaseLargestInnerPieSlices(
                            showcaseNode->GetProperty(L"by-group")->GetValueBool(),
                            showcaseNode->GetProperty(L"show-outer-pie-midpoint-labels")->GetValueBool() );
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
                        { pieChart->GetDonutHoleLabel() = *holeLabel; }
                    }
                const auto propNode = donutHoleNode->GetProperty(L"proportion");
                if (propNode->IsOk())
                    { pieChart->SetDonutHoleProportion(propNode->GetValueNumber()); }
                const wxColour color(
                    ConvertColor(donutHoleNode->GetProperty(L"color")));
                if (color.IsOk())
                    { pieChart->SetDonutHoleColor(color); }
                }
            LoadGraph(graphNode, canvas, currentRow, currentColumn, pieChart);
            return pieChart;
            }
        else
            {
            throw std::runtime_error(
                _(L"Variables not defined for pie chart.").ToUTF8());
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadTable(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() ||
            foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for table."), dsName).ToUTF8());
            }

        std::vector<wxString> variables;
        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (variablesNode->IsOk() && variablesNode->IsValueString())
            {
            auto convertedVars =
                ExpandColumnSelections(variablesNode->GetValueString(), foundPos->second);
            if (convertedVars)
                {
                variables.insert(variables.cend(),
                    convertedVars.value().cbegin(), convertedVars.value().cend());
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown variable selection formula for table."),
                        variablesNode->GetValueString()).ToUTF8());
                }
            }
        else if (variablesNode->IsOk() && variablesNode->IsValueArray())
            {
            const auto readVariables = variablesNode->GetValueStringVector();
            for (const auto& readVar : readVariables)
                {
                auto convertedVars = ExpandColumnSelections(readVar, foundPos->second);
                if (convertedVars)
                    {
                    variables.insert(variables.cend(),
                        convertedVars.value().cbegin(), convertedVars.value().cend());
                    }
                else
                    { variables.push_back(readVar); }
                }
            }

        auto table = std::make_shared<Graphs::Table>(canvas);

        // load table defaults
        // change columns' borders
        const auto borderDefaults =
            graphNode->GetProperty(L"default-borders")->GetValueArrayBool();
        table->SetDefaultBorders(
            (borderDefaults.size() > 0 ? borderDefaults[0] : true),
            (borderDefaults.size() > 1 ? borderDefaults[1] : true),
            (borderDefaults.size() > 2 ? borderDefaults[2] : true),
            (borderDefaults.size() > 3 ? borderDefaults[3] : true));

        table->SetData(foundPos->second, variables,
            graphNode->GetProperty(L"transpose")->GetValueBool());

        // sorting
        const auto sortNode = graphNode->GetProperty(L"row-sort");
        if (sortNode->IsOk())
            {
            const auto sortDirection =
                sortNode->GetProperty(L"direction")->GetValueString().CmpNoCase(_DT(L"descending")) == 0 ?
                SortDirection::SortDescending : SortDirection::SortAscending;
            const std::optional<size_t> sortColumn =
                LoadTablePosition(sortNode->GetProperty(L"column"), table);

            // if sorting by a list of labels with a custom order
            if (sortColumn)
                {
                if (const auto labelsNode = sortNode->GetProperty(L"labels");
                    labelsNode->IsOk() && labelsNode->IsValueArray())
                    { table->Sort(sortColumn.value(), labelsNode->GetValueStringVector(), sortDirection); }
                else
                    { table->Sort(sortColumn.value(), sortDirection); }
                }
            }

        if (graphNode->HasProperty(L"link-id"))
            {
            const auto linkId = ConvertNumber(graphNode->GetProperty(L"link-id"));
            if (linkId)
                {
                auto foundPosTLink = std::find_if(m_tableLinks.begin(), m_tableLinks.end(),
                    [&linkId](const auto& tLink)
                    { return tLink.GetId() == static_cast<size_t>(linkId.value()); });
                if (foundPosTLink != m_tableLinks.end())
                    { foundPosTLink->AddTable(table); }
                else
                    {
                    TableLink tLink{ static_cast<size_t>(linkId.value()) };
                    tLink.AddTable(table);
                    m_tableLinks.push_back(std::move(tLink));
                    }
                }
            }

        table->ClearTrailingRowFormatting(graphNode->GetProperty(L"clear-trailing-row-formatting")->GetValueBool());

        const auto minWidthProp = graphNode->GetProperty(L"min-width-proportion");
        if (minWidthProp->IsOk())
            { table->SetMinWidthProportion(minWidthProp->GetValueNumber()); }
        const auto minHeightProp = graphNode->GetProperty(L"min-height-proportion");
        if (minHeightProp->IsOk())
            { table->SetMinHeightProportion(minHeightProp->GetValueNumber()); }

        LoadPen(graphNode->GetProperty(L"highlight-pen"), table->GetHighlightPen());

        // reads a single position and range of positions (start and end)
        const auto readPositions =
            [&table, this]
            (const wxSimpleJSON::Ptr_t& theNode)
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
        const auto loadStops = [this, &table](const auto& stopsNode)
            {
            std::set<size_t> rowOrColumnStops;
            const auto stops = stopsNode->GetValueArrayObject();
            if (stops.size())
                {
                for (const auto& stop : stops)
                    {
                    const std::optional<size_t> stopPosition =
                        LoadTablePosition(stop->GetProperty(L"position"), table);
                    if (stopPosition.has_value())
                        { rowOrColumnStops.insert(stopPosition.value()); }
                    }
                }
            return rowOrColumnStops;
            };

        if (graphNode->HasProperty(L"insert-group-header"))
            { table->InsertGroupHeader(graphNode->GetProperty(L"insert-group-header")->GetValueStringVector()); }

        // group the rows
        const auto rowGroupings = graphNode->GetProperty(L"row-group")->GetValueArrayNumber();
        for (const auto& rowGrouping : rowGroupings)
            { table->GroupRow(rowGrouping); }

        // group the columns
        const auto columnGroupings = graphNode->GetProperty(L"column-group")->GetValueArrayNumber();
        for (const auto& columnGrouping : columnGroupings)
            { table->GroupColumn(columnGrouping); }

        // apply zebra stripes to loaded data before we just adding custom rows/columns, manually changing
        // row/column colors, etc.
        if (graphNode->HasProperty(L"alternate-row-color"))
            {
            const auto altRowColorNode = graphNode->GetProperty(L"alternate-row-color");
            const auto startRow = LoadTablePosition(altRowColorNode->GetProperty(L"start"), table);
            const std::set<size_t> colStops =
                loadStops(altRowColorNode->GetProperty(L"stops"));
            auto rowColor{ ConvertColor(altRowColorNode->GetProperty(L"color")) };
            if (!rowColor.IsOk())
                { rowColor = *wxWHITE; }
            table->ApplyAlternateRowColors(rowColor, startRow.value_or(0), colStops);
            }

        // add rows
        auto rowAddCommands = graphNode->GetProperty(L"row-add")->GetValueArrayObject();
        if (rowAddCommands.size())
            {
            for (const auto& rowAddCommand : rowAddCommands)
                {
                const std::optional<size_t> position =
                    LoadTablePosition(rowAddCommand->GetProperty(L"position"), table);
                if (!position.has_value())
                    { continue; }
                table->InsertRow(position.value());
                // fill the values across the row
                const auto values = rowAddCommand->GetProperty(L"values")->GetValueStringVector();
                for (size_t i = 0; i < values.size(); ++i)
                    { table->GetCell(position.value(), i).SetValue(values[i]); }
                const wxColour bgcolor(
                    ConvertColor(rowAddCommand->GetProperty(L"background")));
                if (bgcolor.IsOk())
                    { table->SetRowBackgroundColor(position.value(), bgcolor, std::nullopt); }
                }
            }

        // change the rows' suppression
        const auto rowSuppressionCommands = graphNode->GetProperty(L"row-suppression")->GetValueArrayObject();
        if (rowSuppressionCommands.size())
            {
            for (const auto& rowSuppressionCommand : rowSuppressionCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowSuppressionCommand);
                const auto threshold =
                    ConvertNumber(rowSuppressionCommand->GetProperty(L"threshold"));
                const auto suppressionLabel =
                    ExpandConstants(rowSuppressionCommand->GetProperty(L"label")->GetValueString());

                const std::set<size_t> colStops =
                    loadStops(rowSuppressionCommand->GetProperty(L"stops"));
                if (threshold.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetRowSuppression(position.value(), threshold.value(),
                            suppressionLabel.length() ? std::optional<wxString>(suppressionLabel) : std::nullopt,
                            colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowSuppression(i, threshold.value(),
                                suppressionLabel.length() ? std::optional<wxString>(suppressionLabel) : std::nullopt,
                                colStops);
                            }
                        }
                    }
                }
            }

        // change the rows' formatting
        const auto rowFormattingCommands = graphNode->GetProperty(L"row-formatting")->GetValueArrayObject();
        if (rowFormattingCommands.size())
            {
            for (const auto& rowFormattingCommand : rowFormattingCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowFormattingCommand);
                const auto formatValue =
                    ReportEnumConvert::ConvertTableCellFormat(
                        rowFormattingCommand->GetProperty(L"format")->GetValueString());

                const std::set<size_t> colStops =
                    loadStops(rowFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        { table->SetRowFormat(position.value(), formatValue.value(), colStops); }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            { table->SetRowFormat(i, formatValue.value(), colStops); }
                        }
                    }
                }
            }

        // color the rows
        const auto rowColorCommands = graphNode->GetProperty(L"row-color")->GetValueArrayObject();
        if (rowColorCommands.size())
            {
            for (const auto& rowColorCommand : rowColorCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowColorCommand);
                const wxColour bgcolor(
                    ConvertColor(rowColorCommand->GetProperty(L"background")));
                const std::set<size_t> colStops =
                    loadStops(rowColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    // single column
                    if (position.has_value())
                        { table->SetRowBackgroundColor(position.value(), bgcolor, colStops); }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            { table->SetRowBackgroundColor(i, bgcolor, colStops); }
                        }
                    }
                }
            }

        // bold the rows
        const auto rowBoldCommands = graphNode->GetProperty(L"row-bold")->GetValueArrayObject();
        if (rowBoldCommands.size())
            {
            for (const auto& rowBoldCommand : rowBoldCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowBoldCommand);
                const std::set<size_t> colStops =
                    loadStops(rowBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    { table->BoldRow(position.value(), colStops); }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        { table->BoldRow(i, colStops); }
                    }
                }
            }

        // change rows' borders
        const auto rowBordersCommands =
            graphNode->GetProperty(L"row-borders")->GetValueArrayObject();
        if (rowBordersCommands.size())
            {
            for (const auto& rowBordersCommand : rowBordersCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowBordersCommand);
                const auto borderFlags =
                    rowBordersCommand->GetProperty(L"borders")->GetValueArrayBool();

                const std::set<size_t> rowStops =
                    loadStops(rowBordersCommand->GetProperty(L"stops"));
                if (borderFlags.size() > 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                            (borderFlags.size() > 0 ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] : table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] : table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] : table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                (borderFlags.size() > 0 ? borderFlags[0] : table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] : table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] : table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] : table->IsShowingLeftBorder()),
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
                            rowBordersCommand->GetProperty(L"top-border")->
                                GetValueBool(table->IsShowingTopBorder()),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                rowBordersCommand->GetProperty(L"top-border")->
                                GetValueBool(table->IsShowingTopBorder()),
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                            std::nullopt,
                            rowBordersCommand->GetProperty(L"right-border")->
                                GetValueBool(table->IsShowingRightBorder()),
                            std::nullopt,
                            std::nullopt,
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                std::nullopt,
                                rowBordersCommand->GetProperty(L"right-border")->
                                GetValueBool(table->IsShowingRightBorder()),
                                std::nullopt,
                                std::nullopt,
                                rowStops);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                            std::nullopt,
                            std::nullopt,
                            rowBordersCommand->GetProperty(L"bottom-border")->
                                GetValueBool(table->IsShowingBottomBorder()),
                            std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                std::nullopt,
                                std::nullopt,
                                rowBordersCommand->GetProperty(L"bottom-border")->
                                GetValueBool(table->IsShowingBottomBorder()),
                                std::nullopt);
                            }
                        }
                    }
                if (rowBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetRowBorders(position.value(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            rowBordersCommand->GetProperty(L"left-border")->
                                GetValueBool(table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowBorders(i,
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                rowBordersCommand->GetProperty(L"left-border")->
                                GetValueBool(table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }
                }
            }

        // change rows' content alignment
        const auto rowContentCommands =
            graphNode->GetProperty(L"row-content-align")->GetValueArrayObject();
        if (rowContentCommands.size())
            {
            for (const auto& rowContentCommand : rowContentCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(rowContentCommand);
                const auto hPageAlignment =
                    rowContentCommand->GetProperty(L"horizontal-page-alignment")->GetValueString();
                const std::set<size_t> colStops =
                    loadStops(rowContentCommand->GetProperty(L"stops"));
                if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::LeftAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(i,
                                PageHorizontalAlignment::LeftAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::RightAligned, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(i,
                                PageHorizontalAlignment::RightAligned, colStops);
                            }
                        }
                    }
                else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                    {
                    if (position.has_value())
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::Centered, colStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetRowHorizontalPageAlignment(i,
                                PageHorizontalAlignment::Centered, colStops);
                            }
                        }
                    }
                }
            }

        // change the columns' suppression
        const auto columnSuppressionCommands = graphNode->GetProperty(L"column-suppression")->GetValueArrayObject();
        if (columnSuppressionCommands.size())
            {
            for (const auto& columnSuppressionCommand : columnSuppressionCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(columnSuppressionCommand);
                const auto threshold =
                    ConvertNumber(columnSuppressionCommand->GetProperty(L"threshold"));
                const auto suppressionLabel =
                    ExpandConstants(columnSuppressionCommand->GetProperty(L"label")->GetValueString());

                const std::set<size_t> rowStops =
                    loadStops(columnSuppressionCommand->GetProperty(L"stops"));
                if (threshold.has_value())
                    {
                    // single column
                    if (position.has_value())
                        {
                        table->SetColumnSuppression(position.value(), threshold.value(),
                            suppressionLabel.length() ? std::optional<wxString>(suppressionLabel) : std::nullopt,
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnSuppression(i, threshold.value(),
                                suppressionLabel.length() ? std::optional<wxString>(suppressionLabel) : std::nullopt,
                                rowStops);
                            }
                        }
                    }
                }
            }

        // change columns' cell formatting
        const auto columnFormattingCommands =
            graphNode->GetProperty(L"column-formatting")->GetValueArrayObject();
        if (columnFormattingCommands.size())
            {
            for (const auto& columnFormattingCommand : columnFormattingCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(columnFormattingCommand);
                const auto formatValue =
                    ReportEnumConvert::ConvertTableCellFormat(
                        columnFormattingCommand->GetProperty(L"format")->GetValueString());

                const std::set<size_t> rowStops =
                    loadStops(columnFormattingCommand->GetProperty(L"stops"));
                if (formatValue.has_value())
                    {
                    // single column
                    if (position.has_value())
                        { table->SetColumnFormat(position.value(), formatValue.value(), rowStops); }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            { table->SetColumnFormat(i, formatValue.value(), rowStops); }
                        }
                    }
                }
            }

        // color the columns
        const auto colColorCommands = graphNode->GetProperty(L"column-color")->GetValueArrayObject();
        if (colColorCommands.size())
            {
            for (const auto& colColorCommand : colColorCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(colColorCommand);
                const wxColour bgcolor(
                    ConvertColor(colColorCommand->GetProperty(L"background")));
                const std::set<size_t> rowStops =
                    loadStops(colColorCommand->GetProperty(L"stops"));
                if (bgcolor.IsOk())
                    {
                    if (position.has_value())
                        { table->SetColumnBackgroundColor(position.value(), bgcolor, rowStops); }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            { table->SetColumnBackgroundColor(i, bgcolor, rowStops); }
                        }
                    }
                }
            }

        // bold the columns
        const auto colBoldCommands = graphNode->GetProperty(L"column-bold")->GetValueArrayObject();
        if (colBoldCommands.size())
            {
            for (const auto& colBoldCommand : colBoldCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(colBoldCommand);
                const std::set<size_t> rowStops =
                    loadStops(colBoldCommand->GetProperty(L"stops"));
                if (position.has_value())
                    { table->BoldColumn(position.value(), rowStops); }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        { table->BoldColumn(i, rowStops); }
                    }
                }
            }

        // change columns' borders
        const auto columnBordersCommands =
            graphNode->GetProperty(L"column-borders")->GetValueArrayObject();
        if (columnBordersCommands.size())
            {
            for (const auto& columnBordersCommand : columnBordersCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(columnBordersCommand);
                const auto borderFlags =
                    columnBordersCommand->GetProperty(L"borders")->GetValueArrayBool();

                const std::set<size_t> rowStops =
                    loadStops(columnBordersCommand->GetProperty(L"stops"));
                if (borderFlags.size() > 0)
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                            (borderFlags.size() > 0 ? borderFlags[0] : table->IsShowingTopBorder()),
                            (borderFlags.size() > 1 ? borderFlags[1] : table->IsShowingRightBorder()),
                            (borderFlags.size() > 2 ? borderFlags[2] : table->IsShowingBottomBorder()),
                            (borderFlags.size() > 3 ? borderFlags[3] : table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                (borderFlags.size() > 0 ? borderFlags[0] : table->IsShowingTopBorder()),
                                (borderFlags.size() > 1 ? borderFlags[1] : table->IsShowingRightBorder()),
                                (borderFlags.size() > 2 ? borderFlags[2] : table->IsShowingBottomBorder()),
                                (borderFlags.size() > 3 ? borderFlags[3] : table->IsShowingLeftBorder()),
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
                            columnBordersCommand->GetProperty(L"top-border")->
                                GetValueBool(table->IsShowingTopBorder()),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                columnBordersCommand->GetProperty(L"top-border")->
                                GetValueBool(table->IsShowingTopBorder()),
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"right-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                            std::nullopt,
                            columnBordersCommand->GetProperty(L"right-border")->
                                GetValueBool(table->IsShowingRightBorder()),
                            std::nullopt,
                            std::nullopt,
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                std::nullopt,
                                columnBordersCommand->GetProperty(L"right-border")->
                                GetValueBool(table->IsShowingRightBorder()),
                                std::nullopt,
                                std::nullopt,
                                rowStops);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"bottom-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                            std::nullopt,
                            std::nullopt,
                            columnBordersCommand->GetProperty(L"bottom-border")->
                                GetValueBool(table->IsShowingBottomBorder()),
                            std::nullopt);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                std::nullopt,
                                std::nullopt,
                                columnBordersCommand->GetProperty(L"bottom-border")->
                                GetValueBool(table->IsShowingBottomBorder()),
                                std::nullopt);
                            }
                        }
                    }
                if (columnBordersCommand->HasProperty(L"left-border"))
                    {
                    if (position.has_value())
                        {
                        table->SetColumnBorders(position.value(),
                            std::nullopt,
                            std::nullopt,
                            std::nullopt,
                            columnBordersCommand->GetProperty(L"left-border")->
                                GetValueBool(table->IsShowingLeftBorder()),
                            rowStops);
                        }
                    // range
                    if (startPosition.has_value() && endPosition.has_value())
                        {
                        for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                            {
                            table->SetColumnBorders(i,
                                std::nullopt,
                                std::nullopt,
                                std::nullopt,
                                columnBordersCommand->GetProperty(L"left-border")->
                                GetValueBool(table->IsShowingLeftBorder()),
                                rowStops);
                            }
                        }
                    }
                }
            }

        // highlight cells down a column
        const auto columnHighlightsCommands =
            graphNode->GetProperty(L"column-highlight")->GetValueArrayObject();
        if (columnHighlightsCommands.size())
            {
            for (const auto& columnHighlightsCommand : columnHighlightsCommands)
                {
                const auto [position, startPosition, endPosition] = readPositions(columnHighlightsCommand);

                std::set<size_t> rowStops =
                    loadStops(columnHighlightsCommand->GetProperty(L"stops"));
                if (position.has_value())
                    { table->HighlightColumn(position.value(), rowStops); }
                // range
                if (startPosition.has_value() && endPosition.has_value())
                    {
                    for (auto i = startPosition.value(); i <= endPosition.value(); ++i)
                        { table->HighlightColumn(i, rowStops); }
                    }
                }
            }

        // column/row aggregates
        const auto columnRowAggregates =
            graphNode->GetProperty(L"aggregates")->GetValueArrayObject();
        if (columnRowAggregates.size())
            {
            for (const auto& columnRowAggregate : columnRowAggregates)
                {
                const auto aggName = columnRowAggregate->GetProperty(_DT(L"name"))->GetValueString();
                const auto whereType = columnRowAggregate->GetProperty(L"type")->GetValueString();
                const auto aggType = columnRowAggregate->GetProperty(L"aggregate-type")->GetValueString();

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
                const auto cellBorders =
                    columnRowAggregate->GetProperty(L"borders")->GetValueArrayBool();
                std::bitset<4> borders{ 0 };
                borders[0] = (cellBorders.size() > 0 ? cellBorders[0] : table->IsShowingTopBorder());
                borders[1] = (cellBorders.size() > 1 ? cellBorders[1] : table->IsShowingRightBorder());
                borders[2] = (cellBorders.size() > 2 ? cellBorders[2] : table->IsShowingBottomBorder());
                borders[3] = (cellBorders.size() > 3 ? cellBorders[3] : table->IsShowingLeftBorder());

                const wxColour bkColor(
                    ConvertColor(columnRowAggregate->GetProperty(L"background")));

                Table::AggregateInfo aggInfo;
                if (startColumn.has_value())
                    { aggInfo.FirstCell(startColumn.value()); }
                if (endingColumn.has_value())
                    { aggInfo.LastCell(endingColumn.value()); }

                if (aggType.CmpNoCase(L"percent-change") == 0)
                    { aggInfo.Type(AggregateType::ChangePercent); }
                else if (aggType.CmpNoCase(L"change") == 0)
                    { aggInfo.Type(AggregateType::Change); }
                else if (aggType.CmpNoCase(L"total") == 0)
                    { aggInfo.Type(AggregateType::Total); }
                else if (aggType.CmpNoCase(L"ratio") == 0)
                    { aggInfo.Type(AggregateType::Ratio); }
                // invalid agg column type
                else
                    { continue; }

                if (whereType.CmpNoCase(L"column") == 0)
                    {
                    table->InsertAggregateColumn(aggInfo, aggName,
                        insertionPosition,
                        columnRowAggregate->GetProperty(L"use-adjacent-color")->GetValueBool(),
                        (bkColor.IsOk() ?
                            std::optional<wxColour>(bkColor) : std::nullopt),
                        (cellBorders.size() ? std::optional<std::bitset<4>>(borders) : std::nullopt));
                    }
                else if (whereType.CmpNoCase(L"row") == 0)
                    {
                    table->InsertAggregateRow(aggInfo, aggName,
                        insertionPosition, (bkColor.IsOk() ?
                                       std::optional<wxColour>(bkColor) : std::nullopt),
                        (cellBorders.size() ? std::optional<std::bitset<4>>(borders) : std::nullopt));
                    }
                }
            }

        // row totals
        const auto rowTotals =
            graphNode->GetProperty(L"row-totals");
        if (rowTotals->IsOk())
            {
            const wxColour bkColor(
                    ConvertColor(rowTotals->GetProperty(L"background")));
            table->InsertRowTotals(bkColor.IsOk() ?
                                    std::optional<wxColour>(bkColor) : std::nullopt);
            }

        // cell updating
        const auto cellUpdates =
            graphNode->GetProperty(L"cell-update")->GetValueArrayObject();
        if (cellUpdates.size())
            {
            for (const auto& cellUpdate : cellUpdates)
                {
                // last column and row will be the last aggregates at this point
                // (if applicable)
                const std::optional<size_t> rowPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"row"), table);
                const std::optional<size_t> columnPosition =
                    LoadTablePosition(cellUpdate->GetProperty(L"column"), table);
                Table::TableCell* currentCell =
                    ((rowPosition.has_value() && columnPosition.has_value() &&
                      rowPosition.value() < table->GetRowCount() &&
                      columnPosition.value() < table->GetColumnCount()) ?
                    &table->GetCell(rowPosition.value(), columnPosition.value()) :
                    table->FindCell(cellUpdate->GetProperty(L"value-to-find")->GetValueString()) );

                if (currentCell)
                    {
                    // column count
                    const auto columnCountProperty =
                        cellUpdate->GetProperty(L"column-count");
                    if (columnCountProperty->IsOk())
                        {
                        if (columnCountProperty->IsValueString() &&
                            columnCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetColumnCount(table->GetColumnCount());
                            }
                        else if (columnCountProperty->IsValueNumber())
                            {
                            currentCell->SetColumnCount(columnCountProperty->GetValueNumber());
                            }
                        }
                    // row count
                    const auto rowCountProperty =
                        cellUpdate->GetProperty(L"row-count");
                    if (rowCountProperty->IsOk())
                        {
                        if (rowCountProperty->IsValueString() &&
                            rowCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                            {
                            currentCell->SetRowCount(table->GetRowCount());
                            }
                        else if (rowCountProperty->IsValueNumber())
                            {
                            currentCell->SetRowCount(rowCountProperty->GetValueNumber());
                            }
                        }
                    // value
                    const auto valueProperty = cellUpdate->GetProperty(L"value");
                    if (valueProperty->IsOk())
                        {
                        if (valueProperty->IsValueString())
                            {
                            currentCell->SetValue(valueProperty->GetValueString());
                            }
                        else if (valueProperty->IsValueNumber())
                            { currentCell->SetValue(valueProperty->GetValueNumber()); }
                        else if (valueProperty->IsValueNull())
                            { currentCell->SetValue(wxEmptyString); }
                        }
                    // background color
                    const wxColour bgcolor(
                        ConvertColor(cellUpdate->GetProperty(L"background")));
                    if (bgcolor.IsOk())
                        { currentCell->SetBackgroundColor(bgcolor); }

                    // an image to the left side of it
                    const auto leftSideNode = cellUpdate->GetProperty(L"left-image");
                    if (leftSideNode->IsOk())
                        {
                        auto path = leftSideNode->GetProperty(L"path")->GetValueString();
                        if (path.length())
                            {
                            if (!wxFileName::FileExists(path))
                                {
                                path = wxFileName(m_configFilePath).GetPathWithSep() + path;
                                if (!wxFileName::FileExists(path))
                                    {
                                    throw std::runtime_error(
                                        wxString::Format(_(L"%s: label side image not found."), path).ToUTF8());
                                    }
                                }
                            currentCell->SetLeftImage(Image::LoadFile(path));
                            }
                        }

                    // prefix
                    if (cellUpdate->HasProperty(L"prefix"))
                        {
                        currentCell->SetPrefix(
                            cellUpdate->GetProperty(L"prefix")->GetValueString());
                        }

                    // is it highlighted
                    if (cellUpdate->HasProperty(L"highlight"))
                        {
                        currentCell->Highlight(
                            cellUpdate->GetProperty(L"highlight")->GetValueBool());
                        }

                    // font attributes
                    if (cellUpdate->HasProperty(L"bold"))
                        {
                        if (cellUpdate->GetProperty(L"bold")->GetValueBool())
                            { currentCell->GetFont().MakeBold(); }
                        else
                            { currentCell->GetFont().SetWeight(wxFONTWEIGHT_NORMAL); }
                        }

                    // outer border toggles
                    const auto outerBorderToggles =
                        cellUpdate->GetProperty(L"show-borders")->GetValueArrayBool();
                    if (outerBorderToggles.size() >= 1)
                        { currentCell->ShowTopBorder(outerBorderToggles[0]); }
                    if (outerBorderToggles.size() >= 2)
                        { currentCell->ShowRightBorder(outerBorderToggles[1]); }
                    if (outerBorderToggles.size() >= 3)
                        { currentCell->ShowBottomBorder(outerBorderToggles[2]); }
                    if (outerBorderToggles.size() >= 4)
                        { currentCell->ShowLeftBorder(outerBorderToggles[3]); }

                    const auto textAlignment = ReportEnumConvert::ConvertTextAlignment(
                        cellUpdate->GetProperty(L"text-alignment")->GetValueString());
                    if (textAlignment.has_value())
                        { currentCell->SetTextAlignment(textAlignment.value()); }

                    // horizontal page alignment
                    const auto hPageAlignment =
                        cellUpdate->GetProperty(L"horizontal-page-alignment")->GetValueString();
                    if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
                    else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
                    else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                        { currentCell->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }
                    }
                }
            }

        const auto annotationsNode = graphNode->GetProperty(L"cell-annotations")->GetValueArrayObject();
        if (annotationsNode.size())
            {
            for (const auto& annotation : annotationsNode)
                {
                Table::CellAnnotation cellAnnotation
                    {
                    annotation->GetProperty(L"value")->GetValueString(),
                    std::vector<Table::CellPosition>(), Side::Right, std::nullopt, wxColour()
                    };
                if (annotation->HasProperty(L"side"))
                    {
                    cellAnnotation.m_side =
                        (annotation->GetProperty(L"side")->GetValueString().CmpNoCase(L"left") == 0) ?
                        Side::Left : Side::Right;
                    }
                cellAnnotation.m_connectionLinePen = table->GetHighlightPen();
                LoadPen(annotation->GetProperty(L"pen"), cellAnnotation.m_connectionLinePen.value());
                cellAnnotation.m_bgColor = ConvertColor(annotation->GetProperty(L"background"));

                const auto cellsNode = annotation->GetProperty(L"cells");
                if (cellsNode->IsOk() && cellsNode->IsValueObject())
                    {
                    const auto outliersNode = cellsNode->GetProperty(L"column-outliers");
                    const auto topNNode = cellsNode->GetProperty(L"column-top-n");
                    if (outliersNode->IsOk() && outliersNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(outliersNode->GetValueString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetOutliers(colIndex.value());
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    else if (topNNode->IsOk() && topNNode->IsValueString())
                        {
                        const auto colIndex = table->FindColumnIndex(topNNode->GetValueString());
                        if (colIndex.has_value())
                            {
                            cellAnnotation.m_cells = table->GetTopN(colIndex.value(),
                                                                    cellsNode->GetProperty(L"n")->GetValueNumber(1));
                            table->AddCellAnnotation(cellAnnotation);
                            }
                        }
                    }
                }
            }

        // assign footnotes after all cells have been updated
        const auto footnotesNode = graphNode->GetProperty(L"footnotes")->GetValueArrayObject();
        if (footnotesNode.size())
            {
            for (const auto& ftNode : footnotesNode)
                {
                table->AddFootnote(
                    ExpandConstants(ftNode->GetProperty(L"value")->GetValueString()),
                    ExpandConstants(ftNode->GetProperty(L"footnote")->GetValueString()));
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, table);
        return table;
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::ExpandNumericConstant(wxString str) const
        {
        if (str.starts_with(L"{{") && str.ends_with(L"}}"))
            { str = str.substr(2, str.length() - 4); }
        const auto foundVal = m_values.find(str);
        if (foundVal != m_values.cend())
            {
            if (const auto dVal{ std::get_if<double>(&foundVal->second) };
                dVal != nullptr && !std::isnan(*dVal))
                { return *dVal; }
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
            [[maybe_unused]] const auto fullMatchResult = re.GetMatch(&start, &len, 0);
            const auto foundVal = m_values.find(
                re.GetMatch(processText.data(), 1));
            if (foundVal != m_values.cend())
                {
                if (const auto strVal{ std::get_if<wxString>(&foundVal->second) };
                    strVal != nullptr)
                    {
                    replacements.insert_or_assign(
                        std::wstring(processText.substr(start, len)), *strVal);
                    }
                else if (const auto dVal{ std::get_if<double>(&foundVal->second) };
                         dVal != nullptr)
                    {
                    if (std::isnan(*dVal))
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)), wxEmptyString);
                        }
                    else
                        {
                        replacements.insert_or_assign(
                            std::wstring(processText.substr(start, len)),
                            wxNumberFormatter::ToString(*dVal, 2,
                                wxNumberFormatter::Style::Style_WithThousandsSep|
                                wxNumberFormatter::Style::Style_NoTrailingZeroes));
                        }
                    }
                }
            // or a function like "Now()" that doesn't require a dataset
            else
                {
                const auto calcStr = CalcFormula(re.GetMatch(processText.data(), 1), nullptr);
                if (const auto strVal{ std::get_if<wxString>(&calcStr) };
                    strVal != nullptr)
                    {
                    replacements.insert_or_assign(
                        std::wstring(processText.substr(start, len)),
                        *strVal);
                    }
                }
            processText = processText.substr(start + len);
            }

        // now, replace the placeholders with the user-defined values mapped to them
        for (const auto& rep : replacements)
            { str.Replace(rep.first, rep.second); }

        return str;
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(const wxSimpleJSON::Ptr_t& colorNode)
        {
        if (!colorNode->IsOk())
            {
            return wxNullColour;
            }

        return (colorNode->IsValueNull() ?
                    // using null in JSON for a color implies that we want
                    // a legit color that is transparent
                    wxTransparentColour :
                    ConvertColor(colorNode->GetValueString()));
        }

    //---------------------------------------------------
    wxColour ReportBuilder::ConvertColor(wxString colorStr)
        {
        // in case the color is a user-defined constant in the file
        colorStr = ExpandConstants(colorStr);

        // see if it is one of our defined colors
        auto foundPos = m_colorMap.find(std::wstring_view(colorStr.MakeLower().wc_str()));
        if (foundPos != m_colorMap.cend())
            {
            return Colors::ColorBrewer::GetColor(foundPos->second);
            }

        return wxColour(colorStr);
        }

    //---------------------------------------------------
    std::optional<size_t> ReportBuilder::LoadTablePosition(const wxSimpleJSON::Ptr_t& positionNode,
        std::shared_ptr<Graphs::Table> table)
        {
        if (!positionNode->IsOk())
            { return std::nullopt; }

        std::optional<size_t> position;

        const auto loadStringToPosition = [&](const auto& originStr)
            {
            if (originStr.CmpNoCase(L"last-column") == 0)
                { position = table->GetLastDataColumn(); }
            else if (originStr.CmpNoCase(L"last-row") == 0)
                { position = table->GetLastDataRow(); }
            else if (originStr.StartsWith(L"column:"))
                {
                if (const auto colPos =
                        (table ? table->FindColumnIndex(originStr.substr(7)) : std::nullopt);
                     colPos.has_value())
                    { position = colPos; }
                }
            else if (originStr.StartsWith(L"row:"))
                {
                if (const auto colPos =
                        (table ? table->FindRowIndex(originStr.substr(4)) : std::nullopt);
                     colPos.has_value())
                    { position = colPos; }
                }
            else
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: unknown table position origin value."), originStr).ToUTF8());
                }
            };

        const auto origin = positionNode->GetProperty(L"origin");
        if (origin->IsOk())
            {
            if (origin->IsValueString())
                {
                loadStringToPosition(origin->GetValueString());
                }
            else if (origin->IsValueNumber())
                {
                position = origin->GetValueNumber();
                }
            }
        else if (positionNode->IsValueString())
            {
            loadStringToPosition(positionNode->GetValueString());
            }
        else if (positionNode->IsValueNumber())
            {
            position = positionNode->GetValueNumber();
            }
        std::optional<double> doubleStartOffset =
            positionNode->HasProperty(L"offset") ?
                std::optional<double>(positionNode->GetProperty(L"offset")->GetValueNumber()) :
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
    ReportBuilder::LoadBrushScheme(const wxSimpleJSON::Ptr_t& brushSchemeNode)
        {
        const auto brushStylesNode = brushSchemeNode->GetProperty(L"brush-styles");
        if (brushStylesNode->IsOk() && brushStylesNode->IsValueArray())
            {
            std::vector<wxBrushStyle> brushStyles;
            const auto brushStylesVals = brushStylesNode->GetValueArrayString();
            for (const auto& brushStylesVal : brushStylesVals)
                {
                const auto bStyle = ReportEnumConvert::ConvertBrushStyle(brushStylesVal);
                if (bStyle)
                    {
                    brushStyles.push_back(bStyle.value());
                    }
                }
            const auto colorScheme = LoadColorScheme(brushSchemeNode->GetProperty(L"color-scheme"));
            if (colorScheme)
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(brushStyles, *colorScheme);
                }
            }
        // just a named color scheme
        else if (brushSchemeNode->IsValueString())
            {
            const auto colorScheme = LoadColorScheme(brushSchemeNode);
            if (colorScheme)
                {
                return std::make_shared<Brushes::Schemes::BrushScheme>(*colorScheme);
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Colors::Schemes::ColorScheme>
    ReportBuilder::LoadColorScheme(const wxSimpleJSON::Ptr_t& colorSchemeNode)
        {
        if (!colorSchemeNode->IsOk())
            {
            return nullptr;
            }
        else if (colorSchemeNode->IsValueArray())
            {
            std::vector<wxColour> colors;
            const auto colorValues = colorSchemeNode->GetValueStringVector();
            if (colorValues.size() == 0)
                {
                return nullptr;
                }
            for (const auto& color : colorValues)
                {
                colors.push_back(ConvertColor(color));
                }
            return std::make_shared<Colors::Schemes::ColorScheme>(colors);
            }
        else if (colorSchemeNode->IsValueString())
            {
            return ReportEnumConvert::ConvertColorScheme(colorSchemeNode->GetValueString());
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Wisteria::LineStyleScheme> ReportBuilder::LoadLineStyleScheme(
        const wxSimpleJSON::Ptr_t& lineStyleSchemeNode)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring_view, LineStyle> lineStyleEnsums =
            {
            { L"arrows", LineStyle::Arrows },
            { L"lines", LineStyle::Lines },
            { L"spline", LineStyle::Spline }
            };

        if (!lineStyleSchemeNode->IsOk())
            { return nullptr; }
        // a list of icons
        else if (lineStyleSchemeNode->IsValueArray())
            {
            std::vector<std::pair<wxPenStyle, LineStyle>> lineStyles;
            const auto lineStyleValues = lineStyleSchemeNode->GetValueArrayObject();
            for (const auto& lineStyle : lineStyleValues)
                {
                wxPen pn(*wxBLACK, 1, wxPenStyle::wxPENSTYLE_SOLID);
                LoadPen(lineStyle->GetProperty(L"pen-style"), pn);
                const auto foundPos = lineStyleEnsums.find(
                    std::wstring_view(lineStyle->GetProperty(L"line-style")->
                        GetValueString().MakeLower().wc_str()));
                if (foundPos != lineStyleEnsums.cend())
                    { lineStyles.emplace_back(pn.GetStyle(), foundPos->second); }
                }
            if (lineStyles.size() == 0)
                { return nullptr; }
            return std::make_shared<LineStyleScheme>(lineStyles);
            }

        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Wisteria::Icons::Schemes::IconScheme> ReportBuilder::LoadIconScheme(
        const wxSimpleJSON::Ptr_t& iconSchemeNode)
        {
        static const std::map<std::wstring_view, std::shared_ptr<IconScheme>> iconSchemes =
            {
            { L"standard-shapes", std::make_shared<StandardShapes>() },
            { L"semesters", std::make_shared<Semesters>() }
            };

        if (!iconSchemeNode->IsOk())
            { return nullptr; }
        // a list of icons
        else if (iconSchemeNode->IsValueArray())
            {
            std::vector<IconShape> icons;
            const auto iconValues = iconSchemeNode->GetValueStringVector();
            if (iconValues.size() == 0)
                { return nullptr; }
            for (const auto& icon : iconValues)
                {
                const auto iconValue = ReportEnumConvert::ConvertIcon(icon);
                if (iconValue.has_value())
                    {
                    icons.push_back(iconValue.value());
                    }
                }
            if (icons.size() == 0)
                { return nullptr; }
            return std::make_shared<IconScheme>(icons);
            }
        // a pre-defined icon scheme
        else if (iconSchemeNode->IsValueString())
            {
            const auto foundPos = iconSchemes.find(
                std::wstring_view(iconSchemeNode->GetValueString().MakeLower().wc_str()));
            if (foundPos != iconSchemes.cend())
                { return foundPos->second; }
            }

        return nullptr;
        }

    //---------------------------------------------------
    wxString ReportBuilder::NormalizeFilePath(const wxString& path)
        {
        wxString expandedPath{ path };
        if (expandedPath.empty())
            {
            throw std::runtime_error(
                wxString(_(L"Filepath is empty.")).ToUTF8());
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
    ReportBuilder::LoadImage(
        const wxSimpleJSON::Ptr_t& imageNode)
        {
        static const std::map<std::wstring_view, ResizeMethod> resizeValues =
            {
            { L"downscale-only", ResizeMethod::DownscaleOnly },
            { L"downscale-or-upscale", ResizeMethod::DownscaleOrUpscale },
            { L"upscale-only", ResizeMethod::UpscaleOnly },
            { L"no-resize", ResizeMethod::NoResize },
            };

        const auto bmp = LoadImageFile(imageNode->GetProperty(L"image-import"));
        auto image = std::make_unique<GraphItems::Image>(bmp.ConvertToImage());
        if (image->IsOk())
            {
            // center by default, but allow LoadItems (below) to override that
            // if client asked for something else
            image->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered);
            image->SetPageVerticalAlignment(PageVerticalAlignment::Centered);

            auto foundPos = resizeValues.find(std::wstring_view(
                imageNode->GetProperty(L"resize-method")->GetValueString().MakeLower().wc_str()));
            if (foundPos != resizeValues.cend())
                { image->SetResizeMethod(foundPos->second); }
            LoadItem(imageNode, *image);

            image->SetFixedWidthOnCanvas(true);
            return image;
            }
        else
            { return nullptr; }
        }

    //---------------------------------------------------
    wxBitmap ReportBuilder::LoadImageFile(const wxSimpleJSON::Ptr_t& bmpNode)
        {
        // if simply a file path, then load that and return
        if (bmpNode->IsValueString())
            {
            const auto path = NormalizeFilePath(bmpNode->GetValueString());
            return Image::LoadFile(path);
            }

        // otherwise, load as all images and apply effects to them
        std::vector<wxBitmap> bmps;
        auto paths = bmpNode->GetProperty(L"paths")->GetValueStringVector();
        for (auto& path : paths)
            {
            path = NormalizeFilePath(path);
            bmps.push_back(Image::LoadFile(path));
            }

        // single image
        const auto path = bmpNode->GetProperty(L"path")->GetValueString();
        if (path.length())
            { bmps.push_back(Image::LoadFile(NormalizeFilePath(path))); }

        if (bmps.empty())
            { return wxNullBitmap; }

        wxBitmap bmp = bmps[0];

        if (bmps.size() > 1)
            {
            const auto stitch = bmpNode->GetProperty(L"stitch")->GetValueString();
            if (stitch.CmpNoCase(L"vertical") == 0)
                { bmp = Image::StitchVertically(bmps); }
            else
                { bmp = Image::StitchHorizontally(bmps); }
            }

        if (bmpNode->HasProperty(L"color-filter"))
            {
            auto color = ConvertColor(bmpNode->GetProperty(L"color-filter"));
            if (color.IsOk())
                {
                bmp = Image::CreateColorFilteredImage(bmp.ConvertToImage(), color);
                }
            }

        if (bmpNode->GetProperty(L"opacity")->IsValueNumber())
            {
            Image::SetOpacity(bmp,
                bmpNode->GetProperty(L"opacity")->GetValueNumber(wxALPHA_OPAQUE));
            }

        return bmp;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
                                 GraphItems::GraphItemBase& item)
        {
        if (!itemNode->IsOk())
            { return; }

        static const std::map<std::wstring_view, Anchoring> anchoringValues =
            {
            { L"bottom-left-corner", Anchoring::BottomLeftCorner },
            { L"bottom-right-corner", Anchoring::BottomRightCorner },
            { L"center", Anchoring::Center },
            { L"top-left-corner", Anchoring::TopLeftCorner },
            { L"top-right-corner", Anchoring::TopRightCorner },
            };

        item.SetDPIScaleFactor(m_dpiScaleFactor);

        // ID
        item.SetId(itemNode->GetProperty(L"id")->GetValueNumber(wxID_ANY));

        // anchoring
        const auto foundPos = anchoringValues.find(std::wstring_view(
            itemNode->GetProperty(L"anchoring")->GetValueString().MakeLower().wc_str()));
        if (foundPos != anchoringValues.cend())
            { item.SetAnchoring(foundPos->second); }

        // outline
        const auto outlineFlagsNode = itemNode->GetProperty(L"outline");
        if (outlineFlagsNode->IsOk() && outlineFlagsNode->IsValueArray())
            {
            const auto outlineFlags = outlineFlagsNode->GetValueArrayBool();
            item.GetGraphItemInfo().Outline(
                (outlineFlags.size() > 0 ? outlineFlags[0] : false),
                (outlineFlags.size() > 1 ? outlineFlags[1] : false),
                (outlineFlags.size() > 2 ? outlineFlags[2] : false),
                (outlineFlags.size() > 3 ? outlineFlags[3] : false));
            }

        // child-alignment
        const auto childPlacement = itemNode->GetProperty(L"relative-alignment")->GetValueString();
        if (childPlacement.CmpNoCase(L"flush-left") == 0)
            { item.SetRelativeAlignment(RelativeAlignment::FlushLeft); }
        else if (childPlacement.CmpNoCase(L"flush-right") == 0)
            { item.SetRelativeAlignment(RelativeAlignment::FlushRight); }
        else if (childPlacement.CmpNoCase(L"flush-top") == 0)
            { item.SetRelativeAlignment(RelativeAlignment::FlushTop); }
        else if (childPlacement.CmpNoCase(L"flush-bottom") == 0)
            { item.SetRelativeAlignment(RelativeAlignment::FlushBottom); }
        else if (childPlacement.CmpNoCase(L"centered") == 0)
            { item.SetRelativeAlignment(RelativeAlignment::Centered); }

        // padding (going clockwise)
        const auto paddingSpec = itemNode->GetProperty(L"padding")->GetValueArrayNumber();
        if (paddingSpec.size() > 0)
            { item.SetTopPadding(paddingSpec.at(0)); }
        if (paddingSpec.size() > 1)
            { item.SetRightPadding(paddingSpec.at(1)); }
        if (paddingSpec.size() > 2)
            { item.SetBottomPadding(paddingSpec.at(2)); }
        if (paddingSpec.size() > 3)
            { item.SetLeftPadding(paddingSpec.at(3)); }

        // canvas padding (going clockwise)
        const auto canvasPaddingSpec = itemNode->GetProperty(L"canvas-margins")->GetValueArrayNumber();
        if (canvasPaddingSpec.size() > 0)
            { item.SetTopCanvasMargin(canvasPaddingSpec.at(0)); }
        if (canvasPaddingSpec.size() > 1)
            { item.SetRightCanvasMargin(canvasPaddingSpec.at(1)); }
        if (canvasPaddingSpec.size() > 2)
            { item.SetBottomCanvasMargin(canvasPaddingSpec.at(2)); }
        if (canvasPaddingSpec.size() > 3)
            { item.SetLeftCanvasMargin(canvasPaddingSpec.at(3)); }

        // horizontal page alignment
        const auto hPageAlignment = itemNode->GetProperty(L"horizontal-page-alignment")->GetValueString();
        if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
            { item.SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
        else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
            { item.SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
        else if (hPageAlignment.CmpNoCase(L"centered") == 0)
            { item.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }

        // vertical page alignment
        const auto vPageAlignment = itemNode->GetProperty(L"vertical-page-alignment")->GetValueString();
        if (vPageAlignment.CmpNoCase(L"top-aligned") == 0)
            { item.SetPageVerticalAlignment(PageVerticalAlignment::TopAligned); }
        else if (vPageAlignment.CmpNoCase(L"bottom-aligned") == 0)
            { item.SetPageVerticalAlignment(PageVerticalAlignment::BottomAligned); }
        else if (vPageAlignment.CmpNoCase(L"centered") == 0)
            { item.SetPageVerticalAlignment(PageVerticalAlignment::Centered); }

        // should the item be shown
        item.Show(itemNode->GetProperty(L"show")->GetValueBool(true));

        item.SetScaling(itemNode->GetProperty(L"scaling")->GetValueNumber(1));

        LoadPen(itemNode->GetProperty(L"pen"), item.GetPen());

        item.SetFixedWidthOnCanvas(
            itemNode->GetProperty(L"fixed-width")->GetValueBool());
        item.FitCanvasRowHeightToContent(
            itemNode->GetProperty(L"fit-row-to-content")->GetValueBool());
        }

    //---------------------------------------------------
    std::optional<double> ReportBuilder::FindAxisPosition(const GraphItems::Axis& axis,
        const wxSimpleJSON::Ptr_t& positionNode) const
        {
        std::optional<double> axisPos;
        if (positionNode->IsOk() && positionNode->IsValueString())
            {
            // see if it's a date
            wxDateTime dt;
            if (dt.ParseDateTime(positionNode->GetValueString()) ||
                dt.ParseDate(positionNode->GetValueString()))
                {
                axisPos = axis.FindDatePosition(dt);
                // looks like a date, but couldn't be found on the axis,
                // so just show it as a string
                if (!axisPos.has_value())
                    {
                    axisPos = axis.FindCustomLabelPosition(positionNode->GetValueString());
                    }
                }
            else
                {
                axisPos = axis.FindCustomLabelPosition(positionNode->GetValueString());
                }
            }
        else if (positionNode->IsOk() && positionNode->IsValueNumber())
            {
            axisPos = positionNode->GetValueNumber();
            }
        return axisPos;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                                  Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                                  std::shared_ptr<Graphs::Graph2D> graph)
        {
        LoadItem(graphNode, *graph);

        // title information
        const auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            const auto titleLabel = LoadLabel(titleProperty, graph->GetTitle());
            if (titleLabel != nullptr)
                { graph->GetTitle() = *titleLabel; }
            }

        // subtitle information
        const auto subtitleProperty = graphNode->GetProperty(L"sub-title");
        if (subtitleProperty->IsOk())
            {
            const auto subtitleLabel = LoadLabel(subtitleProperty, graph->GetSubtitle());
            if (subtitleLabel != nullptr)
                { graph->GetSubtitle() = *subtitleLabel; }
            }

        // caption information
        const auto captionProperty = graphNode->GetProperty(L"caption");
        if (captionProperty->IsOk())
            {
            const auto captionLabel = LoadLabel(captionProperty, graph->GetCaption());
            if (captionLabel != nullptr)
                { graph->GetCaption() = *captionLabel; }
            }

        // background color
        const auto bgColor = ConvertColor(graphNode->GetProperty(L"background-color"));
        if (bgColor.IsOk())
            { graph->SetPlotBackgroundColor(bgColor); }

        // image scheme
        const auto imageSchemeNode = graphNode->GetProperty(L"image-scheme");
        if (imageSchemeNode->IsOk() && imageSchemeNode->IsValueArray())
            {
            std::vector<wxBitmapBundle> images;
            const auto imgNodes = imageSchemeNode->GetValueArrayObject();
            for (const auto& imgNode : imgNodes)
                {
                images.push_back(LoadImageFile(imgNode));
                }
            graph->SetImageScheme(
                std::make_shared<Images::Schemes::ImageScheme>(std::move(images)));
            }

        // common image outline used for bar charts/box plots
        if (graphNode->HasProperty(L"common-box-image-outline"))
            {
            graph->SetCommonBoxImageOutlineColor(
                ConvertColor(graphNode->GetProperty(L"common-box-image-outline")) );
            }

        // stipple brush used for bar charts/box plots
        if (const auto stippleImgNode = graphNode->GetProperty(L"stipple-image");
            stippleImgNode->IsOk())
            { graph->SetStippleBrush(LoadImageFile(stippleImgNode)); }

        if (const auto stippleShapeNode = graphNode->GetProperty(L"stipple-shape");
            stippleShapeNode->IsOk())
            {
            if (stippleShapeNode->IsValueString())
                {
                const auto iconValue = ReportEnumConvert::ConvertIcon(stippleShapeNode->GetValueString());
                if (iconValue.has_value())
                    { graph->SetStippleShape(iconValue.value()); }
                }
            else
                {
                const auto iconValue =
                    ReportEnumConvert::ConvertIcon(stippleShapeNode->GetProperty(L"icon")->GetValueString());
                if (iconValue.has_value())
                    { graph->SetStippleShape(iconValue.value()); }
                if (const auto stippleShapeColor = ConvertColor(stippleShapeNode->GetProperty(L"color"));
                    stippleShapeColor.IsOk())
                    { graph->SetStippleShapeColor(stippleShapeColor); }
                }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->GetValueArrayObject();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    axisNode->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    if (axisType.value() == AxisType::LeftYAxis)
                        { LoadAxis(axisNode, graph->GetLeftYAxis()); }
                    else if (axisType.value() == AxisType::RightYAxis)
                        { LoadAxis(axisNode, graph->GetRightYAxis()); }
                    else if (axisType.value() == AxisType::BottomXAxis)
                        { LoadAxis(axisNode, graph->GetBottomXAxis()); }
                    else if (axisType.value() == AxisType::TopXAxis)
                        { LoadAxis(axisNode, graph->GetTopXAxis()); }
                    }
                }
            }

        // annotations embedded on the plot
        const auto annotationNode = graphNode->GetProperty(L"annotations");
        if (annotationNode->IsOk())
            {
            const auto annotations = annotationNode->GetValueArrayObject();
            for (const auto& annotation : annotations)
                {
                auto label = LoadLabel(annotation->GetProperty(L"label"),
                                       GraphItems::Label());
                if (!label)
                    { continue; }
                // add outline and background color if not provided in config file
                if (!label->GetPen().IsOk())
                    { label->GetPen() = *wxBLACK_PEN; }
                if (!label->GetFontBackgroundColor().IsOk())
                    {
                    label->SetFontBackgroundColor(
                        ColorContrast::BlackOrWhiteContrast(label->GetFontColor()));
                    }
                label->SetPadding(5, 5, 5, 5);

                const auto interestPointsNode = annotation->GetProperty(L"interest-points");
                if (interestPointsNode->IsOk())
                    {
                    // get all the points on the plot that the note is pointing at
                    std::vector<wxPoint> interestPointPostions;
                    const auto interestPoints = interestPointsNode->GetValueArrayObject();
                    for (const auto& interestPoint : interestPoints)
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           interestPoint->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           interestPoint->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            interestPointPostions.push_back(
                                wxPoint(xPos.value(), yPos.value()));
                            }
                        }
                    wxPoint anchorPt;
                    const auto anchorNode = annotation->GetProperty(L"anchor");
                    if (anchorNode->IsOk())
                        {
                        const auto xPos = FindAxisPosition(graph->GetBottomXAxis(),
                                                           anchorNode->GetProperty(L"x"));
                        const auto yPos = FindAxisPosition(graph->GetLeftYAxis(),
                                                           anchorNode->GetProperty(L"y"));
                        if (xPos.has_value() && yPos.has_value())
                            {
                            anchorPt.x = xPos.value();
                            anchorPt.y = yPos.value();
                            }
                        }
                    // if no anchor point specified, then use the middle point of the interest points
                    /// @todo try to add even better logic in here, like how ggrepel works
                    else if (interestPointPostions.size())
                        {
                        const auto [minX, maxX] = std::minmax_element(
                            interestPointPostions.cbegin(),
                            interestPointPostions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.x < rhv.x; });
                        const auto [minY, maxY] = std::minmax_element(
                            interestPointPostions.cbegin(),
                            interestPointPostions.cend(),
                            [](const auto& lhv, const auto& rhv) noexcept
                            { return lhv.y < rhv.y; });
                        anchorPt.x = safe_divide(maxX->x - minX->x, 2) + minX->x;
                        anchorPt.y = safe_divide(maxY->y - minY->y, 2) + minY->y;
                        }
                    if (anchorPt.IsFullySpecified())
                        { graph->AddAnnotation(label, anchorPt, interestPointPostions); }
                    }
                }
            }

        // reference lines
        const auto referenceLinesNode = graphNode->GetProperty(L"reference-lines");
        if (referenceLinesNode->IsOk())
            {
            const auto refLines = referenceLinesNode->GetValueArrayObject();
            for (const auto& refLine : refLines)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refLine->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    wxPen pen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH);
                    LoadPen(refLine->GetProperty(L"pen"), pen);

                    auto& axis = (axisType == AxisType::BottomXAxis ?
                        graph->GetBottomXAxis() :
                        axisType == AxisType::TopXAxis ?
                        graph->GetTopXAxis() :
                        axisType == AxisType::LeftYAxis ?
                        graph->GetLeftYAxis() :
                        axisType == AxisType::RightYAxis ?
                        graph->GetRightYAxis() :
                        graph->GetBottomXAxis());
                    const auto axisPos =
                        FindAxisPosition(axis, refLine->GetProperty(L"position"));

                    if (axisPos.has_value())
                        {
                        graph->AddReferenceLine(
                            ReferenceLine(axisType.value(), axisPos.value(),
                                refLine->GetProperty(L"label")->GetValueString(),
                                pen));
                        }
                    }
                }
            }

        // reference areas
        static const std::map<std::wstring_view, ReferenceAreaStyle> refAreaValues =
            {
            { L"fade-from-left-to-right", ReferenceAreaStyle::FadeFromLeftToRight },
            { L"fade-from-right-to-left", ReferenceAreaStyle::FadeFromRightToLeft },
            { L"solid", ReferenceAreaStyle::Solid },
            };

        const auto referenceAreasNode = graphNode->GetProperty(L"reference-areas");
        if (referenceAreasNode->IsOk())
            {
            const auto refAreas = referenceAreasNode->GetValueArrayObject();
            for (const auto& refArea : refAreas)
                {
                const auto axisType = ReportEnumConvert::ConvertAxisType(
                    refArea->GetProperty(L"axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    wxPen pen(*wxLIGHT_GREY, 1, wxPenStyle::wxPENSTYLE_LONG_DASH);
                    LoadPen(refArea->GetProperty(L"pen"), pen);

                    auto& axis = (axisType == AxisType::BottomXAxis ?
                        graph->GetBottomXAxis() :
                        axisType == AxisType::TopXAxis ?
                        graph->GetTopXAxis() :
                        axisType == AxisType::LeftYAxis ?
                        graph->GetLeftYAxis() :
                        axisType == AxisType::RightYAxis ?
                        graph->GetRightYAxis() :
                        graph->GetBottomXAxis());

                    ReferenceAreaStyle areaStyle{ ReferenceAreaStyle::Solid };
                    const auto foundPos = refAreaValues.find(
                        std::wstring_view(refArea->GetProperty(L"style")->
                            GetValueString().MakeLower().wc_str()));
                    if (foundPos != refAreaValues.cend())
                        { areaStyle = foundPos->second; }

                    const auto axisPos1 =
                        FindAxisPosition(axis, refArea->GetProperty(L"start"));

                    const auto axisPos2 =
                        FindAxisPosition(axis, refArea->GetProperty(L"end"));

                    if (axisPos1.has_value() && axisPos2.has_value())
                        {
                        graph->AddReferenceArea(
                            ReferenceArea(axisType.value(), axisPos1.value(), axisPos2.value(),
                                refArea->GetProperty(L"label")->GetValueString(),
                                pen, areaStyle));
                        }
                    }
                }
            }

        // is there a legend?
        const auto legendNode = graphNode->GetProperty(L"legend");
        if (legendNode->IsOk())
            {
            const auto ringPerimeterStr = legendNode->GetProperty(L"ring")->GetValueString();
            const auto ringPerimeter = (ringPerimeterStr.CmpNoCase(L"inner") == 0 ?
                                        Perimeter::Inner :
                                        Perimeter::Outer);
            const auto includeHeader = legendNode->GetProperty(L"include-header")->GetValueBool(true);
            const auto headerLabel = legendNode->GetProperty(L"title")->GetValueString();
            const auto placement = legendNode->GetProperty(L"placement")->GetValueString();
            if (placement.CmpNoCase(L"left") == 0)
                {
                auto legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::LeftOfGraph));
                // update title
                if (headerLabel.length())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow, currentColumn+1, graph);
                canvas->SetFixedObject(currentRow, currentColumn++, std::move(legend));
                }
            else if (placement.CmpNoCase(L"bottom") == 0)
                {
                auto legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                if (headerLabel.length())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(++currentRow, currentColumn, std::move(legend));
                }
            else if (placement.CmpNoCase(L"top") == 0)
                {
                auto legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph));
                if (headerLabel.length())
                    {
                    legend->SetLine(0, headerLabel);
                    }
                canvas->SetFixedObject(currentRow+1, currentColumn, graph);
                canvas->SetFixedObject(currentRow++, currentColumn, std::move(legend));
                }
            else // right, the default
                {
                auto legend = graph->CreateLegend(
                    LegendOptions().
                    RingPerimeter(ringPerimeter).
                    IncludeHeader(includeHeader).
                    PlacementHint(LegendCanvasPlacementHint::RightOfGraph));
                if (headerLabel.length())
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
    }
