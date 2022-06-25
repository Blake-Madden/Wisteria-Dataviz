#include "reportbuilder.h"

using namespace Wisteria::Data;
using namespace Wisteria::Graphs;

namespace Wisteria
    {
    //---------------------------------------------------
    std::vector<Canvas*> ReportBuilder::LoadConfigurationFile(const wxString& filePath,
                                                              wxWindow* parent)
        {
        // reset from previous calls
        m_commonAxesPlaceholders.clear();
        m_name.clear();
        m_datasets.clear();
        m_dpiScaleFactor = parent->GetDPIScaleFactor();

        std::vector<Canvas*> reportPages;
        std::vector<std::shared_ptr<Wisteria::Graphs::Graph2D>> embeddedGraphs;

        wxASSERT_MSG(parent, L"Parent window must not be null when building a canvas!");
        if (parent == nullptr)
            { return reportPages; }
        auto json = wxSimpleJSON::LoadFile(filePath);
        if (!json->IsOk())
            { return reportPages; }

        auto reportNameNode = json->GetProperty(L"name");
        if (reportNameNode->IsOk())
            { m_name = reportNameNode->GetValueString(); }

        auto datasourcesNode = json->GetProperty(L"datasources");
        try
            {
            LoadDatasources(datasourcesNode);
            }
        catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                         _(L"Datasource Error"), wxOK|wxICON_ERROR|wxCENTRE);
            return reportPages;
            }

        // start loading the pages
        auto pagesProperty = json->GetProperty(L"pages");
        if (pagesProperty->IsOk())
            {
            auto pages = pagesProperty->GetValueArrayObject();
            for (const auto& page : pages)
                {
                if (page->IsOk())
                    {
                    // create the canvas used for the page
                    auto canvas = new Canvas(parent);
                    canvas->SetLabel(page->GetProperty(L"name")->GetValueString());

                    auto rowsProperty = page->GetProperty(L"rows");
                    if (rowsProperty->IsOk())
                        {
                        size_t currentRow{ 0 }, currentColumn{ 0 };
                        auto rows = rowsProperty->GetValueArrayObject();
                        // Empty page? Go to next one.
                        if (rows.size() == 0)
                            { continue; }
                        canvas->SetFixedObjectsGridSize(rows.size(), 1);
                        for (const auto& row : rows)
                            {
                            auto itemsProperty = row->GetProperty(L"items");
                            if (itemsProperty->IsOk())
                                {
                                currentColumn = 0;
                                auto items = itemsProperty->GetValueArrayObject();
                                for (const auto& item : items)
                                    {
                                    const auto typeProperty = item->GetProperty(L"type");
                                    // load the item into the grid cell(s)
                                    if (typeProperty->IsOk())
                                        {
                                        try
                                            {
                                            if (typeProperty->GetValueString().CmpNoCase(L"line-plot") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadLinePlot(item, canvas, currentRow, currentColumn));
                                                }
                                            if (typeProperty->GetValueString().CmpNoCase(L"label") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadLabel(item));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"common-axis") == 0)
                                                {
                                                // Common axis cannot be created until we know all its children
                                                // have been created. Add a placeholder for now and circle back
                                                // after all other items have been added to the grid.
                                                canvas->SetFixedObject(currentRow, currentColumn, nullptr);
                                                LoadCommonAxis(item, currentRow, currentColumn);
                                                }
                                            }
                                        // show error, but OK to keep going
                                        catch (const std::exception& err)
                                            {
                                            wxMessageBox(wxString::FromUTF8(
                                                         wxString::FromUTF8(err.what())),
                                                         _(L"Canvas Item Error"), wxOK|wxICON_ERROR|wxCENTRE);
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
                        for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                            {
                            std::vector<std::shared_ptr<Graphs::Graph2D>> childGraphs;
                            for (const auto& commonAxisInfo : m_commonAxesPlaceholders)
                                {
                                for (const auto& childId : commonAxisInfo.m_childrenIds)
                                    {
                                    auto childGraph = std::find_if(embeddedGraphs.begin(), embeddedGraphs.end(),
                                        [&childId](const auto& graph) noexcept
                                          {
                                          return graph->GetId() == static_cast<long>(childId);
                                          });
                                    if (childGraph != embeddedGraphs.end() &&
                                        (*childGraph) != nullptr)
                                        { childGraphs.push_back(*childGraph); }
                                    }
                                if (childGraphs.size())
                                    {
                                    auto commonAxis = (commonAxisInfo.m_axisType == AxisType::BottomXAxis) ?
                                        CommonAxisBuilder::BuildBottomAxis(canvas,
                                            childGraphs,
                                            commonAxisInfo.m_commonPerpendicularAxis) :
                                        CommonAxisBuilder::BuildRightAxis(canvas,
                                            childGraphs);
                                    LoadItem(commonAxisInfo.m_node, commonAxis);
                                    // force the row to its height and no more
                                    commonAxis->FitCanvasHeightToContent(true);
                                    canvas->SetFixedObject(
                                        commonAxisInfo.m_gridPosition.first,
                                        commonAxisInfo.m_gridPosition.second,
                                        commonAxis);
                                    }
                                }
                            }
                        }
                    canvas->CalcRowDimensions();
                    canvas->FitToPageWhenPrinting(true);
                    reportPages.push_back(canvas);
                    }
                }
            }

        return reportPages;
        }

    //---------------------------------------------------
    std::optional<AxisType> ReportBuilder::ConvertAxisType(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static std::map<std::wstring, AxisType> values =
            {
            { L"bottom-x", AxisType::BottomXAxis },
            { L"top-x", AxisType::TopXAxis },
            { L"left-y", AxisType::LeftYAxis },
            { L"right-y", AxisType::RightYAxis }
            };

        const auto foundValue = values.find(value.Lower().ToStdWstring());
        return ((foundValue != values.cend()) ?
            std::optional<AxisType>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis)
        {
        const auto titleProperty = axisNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty);
            if (titleLabel != nullptr)
                { axis.GetTitle() = *titleLabel; }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                                       const size_t currentRow, const size_t currentColumn)
        {
        const auto axisType = ConvertAxisType(
            commonAxisNode->GetProperty("axis-type")->GetValueString());
        if (axisType.has_value())
            {
            m_commonAxesPlaceholders.push_back(
                {
                axisType.value(),
                std::make_pair(currentRow, currentColumn),
                commonAxisNode->GetProperty("child-ids")->GetValueArrayNumber(),
                commonAxisNode->GetProperty("common-perpendicular-axis")->GetValueBool(),
                commonAxisNode
                });
            }
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Label> ReportBuilder::LoadLabel(const wxSimpleJSON::Ptr_t& labelNode)
        {
        if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(
                GraphItems::GraphItemInfo(labelNode->GetProperty(L"text")->GetValueString()).
                Pen(wxNullPen).DPIScaling(m_dpiScaleFactor));

            const wxColour bgcolor(labelNode->GetProperty(L"background")->GetValueString());
            if (bgcolor.IsOk())
                { label->SetFontBackgroundColor(bgcolor); }
            const wxColour color(labelNode->GetProperty(L"color")->GetValueString());
            if (color.IsOk())
                { label->SetFontColor(color); }

            // font attributes
            if (labelNode->GetProperty(L"bold")->GetValueBool())
                { label->GetFont().MakeBold(); }

            LoadItem(labelNode, label);
            return label;
            }
        return nullptr;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadDatasources(const wxSimpleJSON::Ptr_t& datasourcesNode)
        {
        if (datasourcesNode->IsOk())
            {
            auto datasources = datasourcesNode->GetValueArrayObject();
            for (const auto& datasource : datasources)
                {
                if (datasource->IsOk())
                    {
                    const wxString dsName = datasource->GetProperty(L"name")->GetValueString();
                    const wxString path = datasource->GetProperty(L"path")->GetValueString();
                    const wxString parser = datasource->GetProperty(L"parser")->GetValueString();
                    // read the variables info
                    //------------------------
                    // ID column
                    const wxString idColumn = datasource->GetProperty(L"id-column")->GetValueString();
                    // date columns
                    std::vector<Data::ImportInfo::DateImportInfo> dateInfo;
                    const auto dateProperty = datasource->GetProperty(L"date-columns");
                    if (dateProperty->IsOk())
                        {
                        const auto dateVars = dateProperty->GetValueArrayObject();
                        for (const auto& dateVar : dateVars)
                            {
                            if (dateVar->IsOk())
                                {
                                // get the date column's name and how to load it
                                const wxString dateName = dateVar->GetProperty(L"name")->GetValueString();
                                if (dateName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Date column must have a name.")).ToUTF8());
                                    }
                                const wxString dateParser = dateVar->GetProperty(L"parser")->GetValueString();
                                const wxString dateFormat = dateVar->GetProperty(L"format")->GetValueString();
                                dateInfo.push_back(
                                    {
                                    dateName,
                                    (dateParser.CmpNoCase(L"iso-date") == 0 ?
                                      DateImportMethod::IsoDate :
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
                        datasource->GetProperty(L"continuous-columns")->GetValueStringVector();
                    // categorical columns
                    std::vector<Data::ImportInfo::CategoricalImportInfo> catInfo;
                    const auto catProperty = datasource->GetProperty(L"categorical-columns");
                    if (catProperty->IsOk())
                        {
                        const auto catVars = catProperty->GetValueArrayObject();
                        for (const auto& catVar : catVars)
                            {
                            if (catVar->IsOk())
                                {
                                // get the cat column's name and how to load it
                                const wxString catName = catVar->GetProperty(L"name")->GetValueString();
                                if (catName.empty())
                                    {
                                    throw std::runtime_error(
                                        wxString(_(L"Categorical column must have a name.")).ToUTF8());
                                    }
                                const wxString catParser = catVar->GetProperty(L"parser")->GetValueString();
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
                    // validate settings
                    if (dsName.empty())
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a name.")).ToUTF8());
                        }
                    if (path.empty())
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a filepath.")).ToUTF8());
                        }
                    if (parser.empty() ||
                        !(parser.CmpNoCase(L"tsv") == 0 ||
                            parser.CmpNoCase(L"csv") == 0))
                        {
                        throw std::runtime_error(
                            wxString(_(L"Dataset must have a valid parser type specified.")).
                                    ToUTF8());
                        }
                    if (parser.CmpNoCase(L"csv") == 0)
                        {
                        dataset->ImportCSV(path,
                            ImportInfo().
                            IdColumn(idColumn).
                            DateColumns(dateInfo).
                            ContinuousColumns(continuousVars).
                            CategoricalColumns(catInfo));
                        }
                    else if (parser.CmpNoCase(L"tsv") == 0)
                        {
                        dataset->ImportTSV(path,
                            ImportInfo().
                            IdColumn(idColumn).
                            DateColumns(dateInfo).
                            ContinuousColumns(continuousVars).
                            CategoricalColumns(catInfo));
                        }
                    
                    m_datasets.insert(std::make_pair(dsName, dataset));
                    }
                }
            }
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadLinePlot(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"datasource")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos != m_datasets.cend() &&
            foundPos->second != nullptr)
            {
            auto variablesNode = graphNode->GetProperty(L"variables");
            if (variablesNode->IsOk())
                {
                auto groupVarName = variablesNode->GetProperty(L"group")->GetValueString();
                auto linePlot = std::make_shared<LinePlot>(canvas);
                linePlot->SetData(foundPos->second,
                    variablesNode->GetProperty(L"y")->GetValueString(),
                    variablesNode->GetProperty(L"x")->GetValueString(),
                    (groupVarName.empty() ? std::nullopt : std::optional<wxString>(groupVarName)));
                return LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    void ReportBuilder::LoadItem(const wxSimpleJSON::Ptr_t& itemNode,
                                 std::shared_ptr<GraphItems::GraphItemBase> item)
        {
        if (!itemNode->IsOk())
            { return; }

        // ID
        item->SetId(itemNode->GetProperty(L"id")->GetValueNumber(wxID_ANY));

        // child-alignment
        const auto childPlacement = itemNode->GetProperty(L"relative-alignment")->GetValueString();
        if (childPlacement.CmpNoCase(L"flush-left") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushLeft); }
        else if (childPlacement.CmpNoCase(L"flush-right") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushRight); }
        else if (childPlacement.CmpNoCase(L"flush-top") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushTop); }
        else if (childPlacement.CmpNoCase(L"flush-bottom") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::FlushBottom); }
        else if (childPlacement.CmpNoCase(L"centered") == 0)
            { item->SetRelativeAlignment(RelativeAlignment::Centered); }

        // padding (going clockwise)
        const auto paddingSpec = itemNode->GetProperty(L"padding")->GetValueArrayNumber();
        if (paddingSpec.size() > 0)
            { item->SetTopPadding(paddingSpec.at(0)); }
        if (paddingSpec.size() > 1)
            { item->SetRightPadding(paddingSpec.at(1)); }
        if (paddingSpec.size() > 2)
            { item->SetBottomPadding(paddingSpec.at(2)); }
        if (paddingSpec.size() > 3)
            { item->SetLeftPadding(paddingSpec.at(3)); }

        // canvas padding (going clockwise)
        const auto canvasPaddingSpec = itemNode->GetProperty(L"canvas-margin")->GetValueArrayNumber();
        if (canvasPaddingSpec.size() > 0)
            { item->SetTopCanvasMargin(canvasPaddingSpec.at(0)); }
        if (canvasPaddingSpec.size() > 1)
            { item->SetRightCanvasMargin(canvasPaddingSpec.at(1)); }
        if (canvasPaddingSpec.size() > 2)
            { item->SetBottomCanvasMargin(canvasPaddingSpec.at(2)); }
        if (canvasPaddingSpec.size() > 3)
            { item->SetLeftCanvasMargin(canvasPaddingSpec.at(3)); }

        // horizontal page alignment
        const auto hPageAlignment = itemNode->GetProperty(L"horizontal-page-alignment")->GetValueString();
        if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
        else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
        else if (hPageAlignment.CmpNoCase(L"centered") == 0)
            { item->SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }

        // vertical page alignment
        const auto vPageAlignment = itemNode->GetProperty(L"vertical-page-alignment")->GetValueString();
        if (vPageAlignment.CmpNoCase(L"top-aligned") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::TopAligned); }
        else if (vPageAlignment.CmpNoCase(L"bottom-aligned") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::BottomAligned); }
        else if (vPageAlignment.CmpNoCase(L"centered") == 0)
            { item->SetPageVerticalAlignment(PageVerticalAlignment::Centered); }

        item->FitContentWidthToCanvas(
            itemNode->GetProperty(L"fit-to-content-width")->GetValueBool());
        item->FitCanvasHeightToContent(
            itemNode->GetProperty(L"fit-row-to-content")->GetValueBool());
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadGraph(
                                  const wxSimpleJSON::Ptr_t& graphNode,
                                  Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                                  std::shared_ptr<Graphs::Graph2D> graph)
        {
        LoadItem(graphNode, graph);

        // title information
        const auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            auto titleLabel = LoadLabel(titleProperty);
            if (titleLabel != nullptr)
                { graph->GetTitle() = *titleLabel; }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->GetValueArrayObject();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ConvertAxisType(
                    axisNode->GetProperty("axis-type")->GetValueString());
                if (axisType.has_value())
                    {
                    if (axisType.value() == AxisType::LeftYAxis)
                        {
                        LoadAxis(axisNode, graph->GetLeftYAxis());
                        }
                    }
                }
            }
        

        // is there a legend?
        const auto legendNode = graphNode->GetProperty(L"legend");
        if (legendNode->IsOk())
            {
            auto placement = legendNode->GetProperty(L"placement")->GetValueString();
            if (placement.CmpNoCase(L"right") == 0)
                {
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(currentRow, ++currentColumn,
                    graph->CreateLegend(
                        LegendOptions().
                            IncludeHeader(true).
                            PlacementHint(LegendCanvasPlacementHint::RightOfGraph)) );
                }
            else if (placement.CmpNoCase(L"left") == 0)
                {
                canvas->SetFixedObject(currentRow, currentColumn+1, graph);
                canvas->SetFixedObject(currentRow, currentColumn++,
                    graph->CreateLegend(
                        LegendOptions().
                            IncludeHeader(true).
                            PlacementHint(LegendCanvasPlacementHint::LeftOfGraph)) );
                }
            else if (placement.CmpNoCase(L"bottom") == 0)
                {
                canvas->SetFixedObject(currentRow, currentColumn, graph);
                canvas->SetFixedObject(++currentRow, currentColumn,
                    graph->CreateLegend(
                        LegendOptions().
                            IncludeHeader(true).
                            PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph)) );
                }
            else if (placement.CmpNoCase(L"top") == 0)
                {
                canvas->SetFixedObject(currentRow+1, currentColumn, graph);
                canvas->SetFixedObject(currentRow++, currentColumn,
                    graph->CreateLegend(
                        LegendOptions().
                            IncludeHeader(true).
                            PlacementHint(LegendCanvasPlacementHint::AboveOrBeneathGraph)) );
                }
            }
        // no legend, so just add the graph
        else
            {
            canvas->SetFixedObject(currentRow, currentColumn, graph);
            }
        return graph;
        }
    }