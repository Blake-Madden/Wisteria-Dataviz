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
                                            else if (typeProperty->GetValueString().CmpNoCase(L"label") == 0)
                                                {
                                                canvas->SetFixedObject(currentRow, currentColumn,
                                                    LoadLabel(item, GraphItems::Label()));
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"image") == 0)
                                                {
                                                LoadImage(item, canvas, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"table") == 0)
                                                {
                                                LoadTable(item, canvas, currentRow, currentColumn);
                                                }
                                            else if (typeProperty->GetValueString().CmpNoCase(L"common-axis") == 0)
                                                {
                                                // Common axis cannot be created until we know all its children
                                                // have been created. Add a placeholder for now and circle back
                                                // after all other items have been added to the grid.
                                                canvas->SetFixedObject(currentRow, currentColumn, nullptr);
                                                LoadCommonAxis(item, currentRow, currentColumn);
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
            auto titleLabel = LoadLabel(titleProperty, GraphItems::Label());
            if (titleLabel != nullptr)
                { axis.GetTitle() = *titleLabel; }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadCommonAxis(const wxSimpleJSON::Ptr_t& commonAxisNode,
                                       const size_t currentRow, const size_t currentColumn)
        {
        const auto axisType = ConvertAxisType(
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
        const wxSimpleJSON::Ptr_t& labelNode, const GraphItems::Label labelTemplate)
        {
        if (labelNode->IsOk())
            {
            auto label = std::make_shared<GraphItems::Label>(labelTemplate);
            label->SetText(labelNode->GetProperty(L"text")->GetValueString());
            label->GetPen() = wxNullPen;
            label->SetDPIScaleFactor(m_dpiScaleFactor);

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
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadTable(
        const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
        size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"datasource")->GetValueString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos != m_datasets.cend() &&
            foundPos->second != nullptr)
            {
            auto variables = graphNode->GetProperty(L"variables")->GetValueStringVector();

            auto table = std::make_shared<Graphs::Table>(canvas);
            table->SetData(foundPos->second, variables,
                graphNode->GetProperty(L"transpose")->GetValueBool());

            const auto& minWidthProp = graphNode->GetProperty(L"min-width-proportion");
            if (minWidthProp->IsOk())
                { table->SetMinWidthProportion(minWidthProp->GetValueNumber()); }
            const auto& minHeightProp = graphNode->GetProperty(L"min-height-proportion");
            if (minHeightProp->IsOk())
                { table->SetMinHeightProportion(minHeightProp->GetValueNumber()); }

            const size_t originalColumnCount = table->GetColumnCount();
            const size_t originalRowCount = table->GetRowCount();

            // add rows
            auto rowAddCommands = graphNode->GetProperty(L"rows-add")->GetValueArrayObject();
            if (rowAddCommands.size())
                {
                for (const auto& rowAddCommand : rowAddCommands)
                    {
                    const std::optional<size_t> position =
                        LoadPosition(rowAddCommand->GetProperty(L"position"),
                            originalColumnCount, originalRowCount);
                    if (!position.has_value())
                        { continue; }
                    table->InsertRow(position.value());
                    // fill the values across the row
                    const auto values = rowAddCommand->GetProperty(L"values")->GetValueStringVector();
                    for (size_t i = 0; i < values.size(); ++i)
                        { table->GetCell(position.value(), i).SetValue(values[i]); }
                    const wxColour bgcolor(rowAddCommand->GetProperty(L"background")->GetValueString());
                    if (bgcolor.IsOk())
                        { table->SetRowBackgroundColor(position.value(), bgcolor); }
                    }
                }
            // group the rows
            auto rowGroupings = graphNode->GetProperty(L"rows-group")->GetValueArrayNumber();
            for (const auto& rowGrouping : rowGroupings)
                { table->GroupRow(rowGrouping); }

            // color the rows
            auto rowColorCommands = graphNode->GetProperty(L"rows-color")->GetValueArrayObject();
            if (rowColorCommands.size())
                {
                for (const auto& rowColorCommand : rowColorCommands)
                    {
                    const std::optional<size_t> position =
                        LoadPosition(rowColorCommand->GetProperty(L"position"),
                            originalColumnCount, originalRowCount);
                    const wxColour bgcolor(rowColorCommand->GetProperty(L"background")->GetValueString());
                    if (position.has_value() && bgcolor.IsOk())
                        { table->SetRowBackgroundColor(position.value(), bgcolor); }
                    }
                }

            // change rows' content alignent
            auto rowContentCommands =
                graphNode->GetProperty(L"rows-content-align")->GetValueArrayObject();
            if (rowContentCommands.size())
                {
                for (const auto& rowContentCommand : rowContentCommands)
                    {
                    const std::optional<size_t> position =
                        LoadPosition(rowContentCommand->GetProperty(L"position"),
                            originalColumnCount, originalRowCount);
                    if (!position.has_value())
                        { continue; }
                    const auto hPageAlignment =
                        rowContentCommand->GetProperty(L"horizontal-page-alignment")->GetValueString();
                    if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::LeftAligned);
                        }
                    else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::RightAligned);
                        }
                    else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                        {
                        table->SetRowHorizontalPageAlignment(position.value(),
                            PageHorizontalAlignment::Centered);
                        }
                    }
                }

            // column aggregates
            auto columnAggregates = graphNode->GetProperty(L"columns-add-aggregates")->GetValueArrayObject();
            if (columnAggregates.size())
                {
                for (const auto& columnAggregate : columnAggregates)
                    {
                    const auto aggName = columnAggregate->GetProperty(L"name")->GetValueString();
                    const auto aggType = columnAggregate->GetProperty(L"type")->GetValueString();

                    // starting column
                    const std::optional<size_t> startColumn =
                        LoadPosition(columnAggregate->GetProperty(L"start"),
                            originalColumnCount, originalRowCount);
                    // ending column
                    const std::optional<size_t> endingColumn =
                        LoadPosition(columnAggregate->GetProperty(L"end"),
                            originalColumnCount, originalRowCount);

                    Table::AggregateInfo aggInfo;
                    if (aggType.CmpNoCase(L"percent-change") == 0)
                        { aggInfo.Type(Table::AggregateType::ChangePercent); }
                    if (startColumn.has_value())
                        { aggInfo.FirstCell(startColumn.value()); }
                    if (endingColumn.has_value())
                        { aggInfo.LastCell(endingColumn.value()); }
                    // invalid agg column type
                    else
                        { continue; }
                    table->InsertAggregateColumn(aggInfo, aggName);
                    }
                }

            // cell updating
            auto cellUpdates = graphNode->GetProperty(L"cells-update")->GetValueArrayObject();
            if (cellUpdates.size())
                {
                for (const auto& cellUpdate : cellUpdates)
                    {
                    // last column and row will be the last aggregates at this point
                    // (if applicable)
                    const std::optional<size_t> rowPosition =
                        LoadPosition(cellUpdate->GetProperty(L"row"),
                            table->GetColumnCount(),
                            table->GetRowCount());
                    const std::optional<size_t> columnPosition =
                        LoadPosition(cellUpdate->GetProperty(L"column"),
                            table->GetColumnCount(),
                            table->GetRowCount());
                    if (rowPosition.has_value() && columnPosition.has_value() &&
                        rowPosition.value() < table->GetRowCount() &&
                        columnPosition.value() < table->GetColumnCount())
                        {
                        auto& currentCell = table->GetCell(rowPosition.value(), columnPosition.value());
                        // column count
                        const auto& columnCountProperty =
                            cellUpdate->GetProperty(L"column-count");
                        if (columnCountProperty->IsOk())
                            {
                            if (columnCountProperty->GetType() == wxSimpleJSON::JSONType::IS_STRING &&
                                columnCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                                {
                                currentCell.SetColumnCount(table->GetColumnCount());
                                }
                            else if (columnCountProperty->GetType() == wxSimpleJSON::JSONType::IS_NUMBER)
                                {
                                currentCell.SetColumnCount(columnCountProperty->GetValueNumber());
                                }
                            }
                        // row count
                        const auto& rowCountProperty =
                            cellUpdate->GetProperty(L"row-count");
                        if (rowCountProperty->IsOk())
                            {
                            if (rowCountProperty->GetType() == wxSimpleJSON::JSONType::IS_STRING &&
                                rowCountProperty->GetValueString().CmpNoCase(L"all") == 0)
                                {
                                currentCell.SetRowCount(table->GetRowCount());
                                }
                            else if (rowCountProperty->GetType() == wxSimpleJSON::JSONType::IS_NUMBER)
                                {
                                currentCell.SetRowCount(rowCountProperty->GetValueNumber());
                                }
                            }
                        // value
                        const auto& valueProperty = cellUpdate->GetProperty(L"value");
                        if (valueProperty->IsOk())
                            {
                            if (valueProperty->GetType() == wxSimpleJSON::JSONType::IS_STRING)
                                {
                                currentCell.SetValue(valueProperty->GetValueString());
                                }
                            else if (valueProperty->GetType() == wxSimpleJSON::JSONType::IS_NUMBER)
                                { currentCell.SetValue(valueProperty->GetValueNumber()); }
                            else if (valueProperty->GetType() == wxSimpleJSON::JSONType::IS_NULL)
                                { currentCell.SetValue(wxEmptyString); }
                            }
                        // background color
                        const wxColour bgcolor(cellUpdate->GetProperty(L"background")->GetValueString());
                        if (bgcolor.IsOk())
                            { currentCell.SetBackgroundColor(bgcolor); }
                        // outer border toggles
                        const auto outerBorderToggles =
                            cellUpdate->GetProperty(L"show-borders")->GetValueArrayBool();
                        if (outerBorderToggles.size() >= 1)
                            { currentCell.ShowTopBorder(outerBorderToggles[0]); }
                        if (outerBorderToggles.size() >= 2)
                            { currentCell.ShowRightBorder(outerBorderToggles[1]); }
                        if (outerBorderToggles.size() >= 3)
                            { currentCell.ShowBottomBorder(outerBorderToggles[2]); }
                        if (outerBorderToggles.size() >= 4)
                            { currentCell.ShowLeftBorder(outerBorderToggles[3]); }

                        // horizontal page alignment
                        const auto hPageAlignment =
                            cellUpdate->GetProperty(L"horizontal-page-alignment")->GetValueString();
                        if (hPageAlignment.CmpNoCase(L"left-aligned") == 0)
                            { currentCell.SetPageHorizontalAlignment(PageHorizontalAlignment::LeftAligned); }
                        else if (hPageAlignment.CmpNoCase(L"right-aligned") == 0)
                            { currentCell.SetPageHorizontalAlignment(PageHorizontalAlignment::RightAligned); }
                        else if (hPageAlignment.CmpNoCase(L"centered") == 0)
                            { currentCell.SetPageHorizontalAlignment(PageHorizontalAlignment::Centered); }
                        }
                    }
                }

            return LoadGraph(graphNode, canvas, currentRow, currentColumn, table);
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::optional<size_t> ReportBuilder::LoadPosition(const wxSimpleJSON::Ptr_t& positionNode,
        const size_t columnCount, const size_t columnRow)
        {
        std::optional<size_t> position;
        const auto origin = positionNode->GetProperty(L"origin");
        if (origin->IsOk())
            {
            if (origin->GetType() == wxSimpleJSON::JSONType::IS_STRING)
                {
                if (origin->GetValueString().CmpNoCase(L"last-column") == 0)
                    { position = columnCount-1; }
                else if (origin->GetValueString().CmpNoCase(L"last-row") == 0)
                    { position = columnRow-1; }
                }
            else if (origin->GetType() == wxSimpleJSON::JSONType::IS_NUMBER)
                { position = origin->GetValueNumber(); }
            }
        std::optional<double> doubleStartOffset =
            positionNode->GetProperty(L"offset")->IsOk() ?
            std::optional<double>(positionNode->GetProperty(L"offset")->GetValueNumber()) :
            std::nullopt;
        if (position.has_value() && doubleStartOffset.has_value())
            { position.value() += doubleStartOffset.value(); }

        return position;
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Image> ReportBuilder::LoadImage(
        const wxSimpleJSON::Ptr_t& imageNode,
        Canvas* canvas, size_t& currentRow, size_t& currentColumn)
        {
        auto image = std::make_shared<GraphItems::Image>(
            imageNode->GetProperty(L"path")->GetValueString());
        if (image->IsOk())
            {
            LoadItem(imageNode, image);
            canvas->SetFixedObject(currentRow, currentColumn, image);
            return image;
            }
        else
            { return nullptr; }
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

        const auto penNode = itemNode->GetProperty(L"pen");
        if (penNode->IsOk())
            {
            const wxColour penColor(penNode->GetProperty(L"color")->GetValueString());
            if (penColor.IsOk())
                { item->GetPen().SetColour(penColor); }
            }

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
            auto titleLabel = LoadLabel(titleProperty, graph->GetTitle());
            if (titleLabel != nullptr)
                { graph->GetTitle() = *titleLabel; }
            }

        // subtitle information
        const auto subtitleProperty = graphNode->GetProperty(L"sub-title");
        if (subtitleProperty->IsOk())
            {
            auto subtitleLabel = LoadLabel(subtitleProperty, graph->GetSubtitle());
            if (subtitleLabel != nullptr)
                { graph->GetSubtitle() = *subtitleLabel; }
            }

        // caption information
        const auto captionProperty = graphNode->GetProperty(L"caption");
        if (captionProperty->IsOk())
            {
            auto captionLabel = LoadLabel(captionProperty, graph->GetCaption());
            if (captionLabel != nullptr)
                { graph->GetCaption() = *captionLabel; }
            }

        // axes
        const auto axesProperty = graphNode->GetProperty(L"axes");
        if (axesProperty->IsOk())
            {
            const auto axesNodes = axesProperty->GetValueArrayObject();
            for (const auto& axisNode : axesNodes)
                {
                const auto axisType = ConvertAxisType(
                    axisNode->GetProperty(L"axis-type")->GetValueString());
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