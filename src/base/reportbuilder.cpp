#include "reportbuilder.h"

using namespace Wisteria::Data;
using namespace Wisteria::Graphs;
using namespace Wisteria::GraphItems;

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

        m_configFilePath = filePath;

        std::vector<Canvas*> reportPages;
        std::vector<std::shared_ptr<Wisteria::Graphs::Graph2D>> embeddedGraphs;

        wxASSERT_MSG(parent, L"Parent window must not be null when building a canvas!");
        if (parent == nullptr)
            { return reportPages; }
        auto json = wxSimpleJSON::LoadFile(m_configFilePath);
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
                                            else if (typeProperty->GetValueString().CmpNoCase(L"pie-chart") == 0)
                                                {
                                                embeddedGraphs.push_back(
                                                    LoadPieChart(item, canvas, currentRow, currentColumn));
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
                                    LoadAxis(commonAxisInfo.m_node, *commonAxis);
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
                    canvas->SetSizeFromPaperSize();
                    reportPages.push_back(canvas);
                    }
                }
            }

        return reportPages;
        }

    //---------------------------------------------------
    std::optional<LabelPlacement> ReportBuilder::ConvertLabelPlacement(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, LabelPlacement> values =
            {
            { L"next-to-parent", LabelPlacement::NextToParent },
            { L"flush", LabelPlacement::Flush }
            };

        const auto foundValue = values.find(value.Lower().ToStdWstring());
        return ((foundValue != values.cend()) ?
            std::optional<LabelPlacement>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<BinLabelDisplay> ReportBuilder::ConvertBinLabelDisplay(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, BinLabelDisplay> values =
            {
            { L"percentage", BinLabelDisplay::BinPercentage },
            { L"value", BinLabelDisplay::BinValue },
            { L"value-and-percentage", BinLabelDisplay::BinValueAndPercentage },
            { L"no-display", BinLabelDisplay::NoDisplay }
            };

        const auto foundValue = values.find(value.Lower().ToStdWstring());
        return ((foundValue != values.cend()) ?
            std::optional<BinLabelDisplay>(foundValue->second) :
            std::nullopt);
        }

    //---------------------------------------------------
    std::optional<AxisType> ReportBuilder::ConvertAxisType(const wxString& value)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring, AxisType> values =
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
    void ReportBuilder::LoadPen(const wxSimpleJSON::Ptr_t& penNode, wxPen& pen)
        {
        if (penNode->IsOk())
            {
            const wxColour penColor(
                ConvertColor(penNode->GetProperty(L"color")->GetValueString()));
            if (penColor.IsOk())
                { pen.SetColour(penColor); }
            if (penNode->GetProperty(L"width")->IsOk())
                {
                pen.SetWidth(penNode->GetProperty(L"width")->GetValueNumber(1));
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadAxis(const wxSimpleJSON::Ptr_t& axisNode, GraphItems::Axis& axis)
        {
        static const std::map<std::wstring_view, Axis::TickMark::DisplayType> values =
            {
            { L"inner", Axis::TickMark::DisplayType::Inner },
            { L"outer", Axis::TickMark::DisplayType::Outer },
            { L"crossed", Axis::TickMark::DisplayType::Crossed },
            { L"no-display", Axis::TickMark::DisplayType::NoDisplay }
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
            auto foundPos = values.find(std::wstring_view(display.wc_str()));
            if (foundPos != values.cend())
                { axis.SetTickMarkDisplay(foundPos->second); }
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

            const wxColour bgcolor(
                ConvertColor(labelNode->GetProperty(L"background")->GetValueString()));
            if (bgcolor.IsOk())
                { label->SetFontBackgroundColor(bgcolor); }
            const wxColour color(
                ConvertColor(labelNode->GetProperty(L"color")->GetValueString()));
            if (color.IsOk())
                { label->SetFontColor(color); }

            // font attributes
            if (labelNode->GetProperty(L"bold")->IsOk())
                {
                if (labelNode->GetProperty(L"bold")->IsOk())
                    {
                    labelNode->GetProperty(L"bold")->GetValueBool() ?
                        label->GetFont().MakeBold() :
                        label->GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                    }
                }

            auto textAlignment = labelNode->GetProperty("text-alignment")->GetValueString();
            if (textAlignment.CmpNoCase(L"flush-left") == 0 ||
                textAlignment.CmpNoCase(L"ragged-right") == 0)
                { label->SetTextAlignment(TextAlignment::FlushLeft); }
            else if (textAlignment.CmpNoCase(L"flush-right") == 0 ||
                textAlignment.CmpNoCase(L"ragged-left") == 0)
                { label->SetTextAlignment(TextAlignment::FlushRight); }
            else if (textAlignment.CmpNoCase(L"centered") == 0)
                { label->SetTextAlignment(TextAlignment::Centered); }
            else if (textAlignment.CmpNoCase(L"justified") == 0)
                { label->SetTextAlignment(TextAlignment::Justified); }

            // header info
            auto headerNode = labelNode->GetProperty(L"header");
            if (headerNode->IsOk())
                {
                label->GetHeaderInfo().Enable(true);
                if (headerNode->GetProperty(L"bold")->IsOk())
                    {
                    headerNode->GetProperty(L"bold")->GetValueBool() ?
                        label->GetHeaderInfo().GetFont().MakeBold() :
                        label->GetHeaderInfo().GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                    }
                const wxColour color(
                    ConvertColor(headerNode->GetProperty(L"color")->GetValueString()));
                if (color.IsOk())
                    { label->GetHeaderInfo().FontColor(color); }
                // not actually setting the scaling of the header (since the label
                // that it is part of has its own scaling), but instead set the scaling
                // of the header's font size.
                label->GetHeaderInfo().GetFont().SetFractionalPointSize(
                    label->GetHeaderInfo().GetFont().GetFractionalPointSize() *
                    headerNode->GetProperty(L"scaling")->GetValueNumber(1));

                const auto textAlignment = headerNode->GetProperty("text-alignment")->GetValueString();
                if (textAlignment.CmpNoCase(L"flush-left") == 0 ||
                    textAlignment.CmpNoCase(L"ragged-right") == 0)
                    { label->GetHeaderInfo().LabelAlignment(TextAlignment::FlushLeft); }
                else if (textAlignment.CmpNoCase(L"flush-right") == 0 ||
                    textAlignment.CmpNoCase(L"ragged-left") == 0)
                    { label->GetHeaderInfo().LabelAlignment(TextAlignment::FlushRight); }
                else if (textAlignment.CmpNoCase(L"centered") == 0)
                    { label->GetHeaderInfo().LabelAlignment(TextAlignment::Centered); }
                else if (textAlignment.CmpNoCase(L"justified") == 0)
                    { label->GetHeaderInfo().LabelAlignment(TextAlignment::Justified); }
                }

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
                    wxString path = datasource->GetProperty(L"path")->GetValueString();
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
                    if (!wxFileName::FileExists(path))
                        {
                        path = wxFileName(m_configFilePath).GetPathWithSep() + path;
                        if (!wxFileName::FileExists(path))
                            {
                            throw std::runtime_error(
                                wxString(_(L"Dataset not found.")).ToUTF8());
                            }
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

                auto linePlot = std::make_shared<LinePlot>(canvas,
                    LoadColorScheme(graphNode->GetProperty(L"color-scheme")),
                    LoadIconScheme(graphNode->GetProperty(L"icon-scheme")));
                linePlot->SetData(foundPos->second,
                    variablesNode->GetProperty(L"y")->GetValueString(),
                    variablesNode->GetProperty(L"x")->GetValueString(),
                    (groupVarName.length() ? std::optional<wxString>(groupVarName) : std::nullopt));
                return LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D> ReportBuilder::LoadPieChart(
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
                const auto aggVarName = variablesNode->GetProperty(L"aggregate")->GetValueString();
                const auto groupVar1Name = variablesNode->GetProperty(L"group-1")->GetValueString();
                const auto groupVar2Name = variablesNode->GetProperty(L"group-2")->GetValueString();

                auto pieChart = std::make_shared<PieChart>(canvas,
                    LoadColorScheme(graphNode->GetProperty(L"color-scheme")));
                pieChart->SetData(foundPos->second,
                    (aggVarName.length() ? std::optional<wxString>(aggVarName) : std::nullopt),
                    groupVar1Name,
                    (groupVar2Name.length() ?std::optional<wxString>(groupVar2Name) : std::nullopt));

                const auto labelPlacement =
                    ConvertLabelPlacement(graphNode->GetProperty(L"label-placement")->GetValueString());
                if (labelPlacement.has_value())
                    { pieChart->SetLabelPlacement(labelPlacement.value()); }

                const auto outerPieMidLabel = ConvertBinLabelDisplay(
                    graphNode->GetProperty(L"outer-pie-midpoint-label-display")->GetValueString());
                if (outerPieMidLabel.has_value())
                    { pieChart->SetOuterPieMidPointLabelDisplay(outerPieMidLabel.value()); }

                const auto innerPieMidLabel = ConvertBinLabelDisplay(
                    graphNode->GetProperty(L"inner-pie-midpoint-label-display")->GetValueString());
                if (innerPieMidLabel.has_value())
                    { pieChart->SetInnerPieMidPointLabelDisplay(innerPieMidLabel.value()); }

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
                        ConvertColor(donutHoleNode->GetProperty(L"color")->GetValueString()));
                    if (color.IsOk())
                        { pieChart->SetDonutHoleColor(color); }
                    }
                return LoadGraph(graphNode, canvas, currentRow, currentColumn, pieChart);
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

            LoadPen(graphNode->GetProperty(L"highlight-pen"), table->GetHighlightPen());

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
                    const wxColour bgcolor(
                        ConvertColor(rowAddCommand->GetProperty(L"background")->GetValueString()));
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
                    const wxColour bgcolor(
                        ConvertColor(rowColorCommand->GetProperty(L"background")->GetValueString()));
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
                        const wxColour bgcolor(
                            ConvertColor(cellUpdate->GetProperty(L"background")->GetValueString()));
                        if (bgcolor.IsOk())
                            { currentCell.SetBackgroundColor(bgcolor); }

                        // is it highlighted
                        if (cellUpdate->GetProperty(L"highlight")->IsOk())
                            {
                            currentCell.Highlight(
                                cellUpdate->GetProperty(L"highlight")->GetValueBool());
                            }

                        // font attributes
                        if (cellUpdate->GetProperty(L"bold")->IsOk())
                            {
                            cellUpdate->GetProperty(L"bold")->GetValueBool() ?
                                currentCell.GetFont().MakeBold() :
                                currentCell.GetFont().SetWeight(wxFONTWEIGHT_NORMAL);
                            }

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
    wxColour ReportBuilder::ConvertColor(wxString colorStr)
        {
        // NOTE: do not edit this. This is generated from the file "tools/Build Color List.R"
        // Refer to the instructions in that file for updating this map.
        static const std::map<std::wstring_view, Colors::Color> values =
            {
            { L"afternoon", Colors::Color::Afternoon },
            { L"airforceblue", Colors::Color::AirForceBlue },
            { L"alexandrite", Colors::Color::Alexandrite },
            { L"aliceblue", Colors::Color::AliceBlue },
            { L"almond", Colors::Color::Almond },
            { L"amaranth", Colors::Color::Amaranth },
            { L"amber", Colors::Color::Amber },
            { L"amberwave", Colors::Color::AmberWave },
            { L"amethyst", Colors::Color::Amethyst },
            { L"androidgreen", Colors::Color::AndroidGreen },
            { L"antiquebrass", Colors::Color::AntiqueBrass },
            { L"antiquefuchsia", Colors::Color::AntiqueFuchsia },
            { L"antiquewhite", Colors::Color::AntiqueWhite },
            { L"ao", Colors::Color::Ao },
            { L"appleblossom", Colors::Color::Appleblossom },
            { L"applegreen", Colors::Color::AppleGreen },
            { L"apricot", Colors::Color::Apricot },
            { L"aqua", Colors::Color::Aqua },
            { L"aquamarine", Colors::Color::Aquamarine },
            { L"aquitaine", Colors::Color::Aquitaine },
            { L"armygreen", Colors::Color::ArmyGreen },
            { L"ashgrey", Colors::Color::AshGrey },
            { L"asparagus", Colors::Color::Asparagus },
            { L"auburn", Colors::Color::Auburn },
            { L"aureolin", Colors::Color::Aureolin },
            { L"avocado", Colors::Color::Avocado },
            { L"azure", Colors::Color::Azure },
            { L"azuremist", Colors::Color::AzureMist },
            { L"babyblue", Colors::Color::BabyBlue },
            { L"babypink", Colors::Color::BabyPink },
            { L"bananayellow", Colors::Color::BananaYellow },
            { L"bark", Colors::Color::Bark },
            { L"basketbeige", Colors::Color::BasketBeige },
            { L"battleshipgrey", Colors::Color::BattleshipGrey },
            { L"bazaar", Colors::Color::Bazaar },
            { L"beaublue", Colors::Color::BeauBlue },
            { L"beige", Colors::Color::Beige },
            { L"belvederecream", Colors::Color::BelvedereCream },
            { L"bistre", Colors::Color::Bistre },
            { L"black", Colors::Color::Black },
            { L"blanchedalmond", Colors::Color::BlanchedAlmond },
            { L"blizzardblue", Colors::Color::BlizzardBlue },
            { L"blond", Colors::Color::Blond },
            { L"blue", Colors::Color::Blue },
            { L"bluebell", Colors::Color::BlueBell },
            { L"blueberry", Colors::Color::Blueberry },
            { L"bluesky", Colors::Color::BlueSky },
            { L"blush", Colors::Color::Blush },
            { L"bondiblue", Colors::Color::BondiBlue },
            { L"boysenberry", Colors::Color::Boysenberry },
            { L"brass", Colors::Color::Brass },
            { L"brickred", Colors::Color::BrickRed },
            { L"britishracinggreen", Colors::Color::BritishRacingGreen },
            { L"bronze", Colors::Color::Bronze },
            { L"brown", Colors::Color::Brown },
            { L"brownstone", Colors::Color::Brownstone },
            { L"bubblegum", Colors::Color::BubbleGum },
            { L"bubbles", Colors::Color::Bubbles },
            { L"bungalowbeige", Colors::Color::BungalowBeige },
            { L"burgundy", Colors::Color::Burgundy },
            { L"burlywood", Colors::Color::Burlywood },
            { L"burntsienna", Colors::Color::BurntSienna },
            { L"burntumber", Colors::Color::BurntUmber },
            { L"byzantium", Colors::Color::Byzantium },
            { L"cadet", Colors::Color::Cadet },
            { L"cadetblue", Colors::Color::CadetBlue },
            { L"cadetgrey", Colors::Color::CadetGrey },
            { L"cafeaulait", Colors::Color::CafeAuLait },
            { L"cafenoir", Colors::Color::CafeNoir },
            { L"camouflagegreen", Colors::Color::CamouflageGreen },
            { L"canary", Colors::Color::Canary },
            { L"candyapple", Colors::Color::CandyApple },
            { L"candypink", Colors::Color::CandyPink },
            { L"capri", Colors::Color::Capri },
            { L"caputmortuum", Colors::Color::CaputMortuum },
            { L"caramel", Colors::Color::Caramel },
            { L"cardinal", Colors::Color::Cardinal },
            { L"caribbeangreen", Colors::Color::CaribbeanGreen },
            { L"carmine", Colors::Color::Carmine },
            { L"carnationpink", Colors::Color::CarnationPink },
            { L"carnelian", Colors::Color::Carnelian },
            { L"carrotorange", Colors::Color::CarrotOrange },
            { L"cascadegreen", Colors::Color::CascadeGreen },
            { L"cayenne", Colors::Color::Cayenne },
            { L"celadon", Colors::Color::Celadon },
            { L"celeste", Colors::Color::Celeste },
            { L"celestialblue", Colors::Color::CelestialBlue },
            { L"ceramic", Colors::Color::Ceramic },
            { L"cerise", Colors::Color::Cerise },
            { L"cerulean", Colors::Color::Cerulean },
            { L"chamoisee", Colors::Color::Chamoisee },
            { L"champagne", Colors::Color::Champagne },
            { L"charcoal", Colors::Color::Charcoal },
            { L"cherry", Colors::Color::Cherry },
            { L"cherryblossompink", Colors::Color::CherryBlossomPink },
            { L"chestnut", Colors::Color::Chestnut },
            { L"chinesered", Colors::Color::ChineseRed },
            { L"chocolate", Colors::Color::Chocolate },
            { L"cinereous", Colors::Color::Cinereous },
            { L"cinnamon", Colors::Color::Cinnamon },
            { L"citrine", Colors::Color::Citrine },
            { L"classicfrenchgray", Colors::Color::ClassicFrenchGray },
            { L"classicrose", Colors::Color::ClassicRose },
            { L"cobalt", Colors::Color::Cobalt },
            { L"coffee", Colors::Color::Coffee },
            { L"cooledblue", Colors::Color::CooledBlue },
            { L"coolgrey", Colors::Color::CoolGrey },
            { L"copper", Colors::Color::Copper },
            { L"copperrose", Colors::Color::CopperRose },
            { L"coral", Colors::Color::Coral },
            { L"coralpink", Colors::Color::CoralPink },
            { L"cordovan", Colors::Color::Cordovan },
            { L"corn", Colors::Color::Corn },
            { L"cornflower", Colors::Color::Cornflower },
            { L"cornflowerblue", Colors::Color::CornflowerBlue },
            { L"cottoncandy", Colors::Color::CottonCandy },
            { L"cream", Colors::Color::Cream },
            { L"crimson", Colors::Color::Crimson },
            { L"cyan", Colors::Color::Cyan },
            { L"daffodil", Colors::Color::Daffodil },
            { L"daisy", Colors::Color::Daisy },
            { L"dandelion", Colors::Color::Dandelion },
            { L"darkblue", Colors::Color::DarkBlue },
            { L"darkbrown", Colors::Color::DarkBrown },
            { L"darkgray", Colors::Color::DarkGray },
            { L"darkgreen", Colors::Color::DarkGreen },
            { L"davygrey", Colors::Color::DavyGrey },
            { L"denim", Colors::Color::Denim },
            { L"desert", Colors::Color::Desert },
            { L"desertsand", Colors::Color::DesertSand },
            { L"dimgray", Colors::Color::DimGray },
            { L"dodgerblue", Colors::Color::DodgerBlue },
            { L"dollarbill", Colors::Color::DollarBill },
            { L"doverwhite", Colors::Color::DoverWhite },
            { L"drab", Colors::Color::Drab },
            { L"dressyrose", Colors::Color::DressyRose },
            { L"earthyellow", Colors::Color::EarthYellow },
            { L"edgygold", Colors::Color::EdgyGold },
            { L"eggplant", Colors::Color::Eggplant },
            { L"eggshell", Colors::Color::Eggshell },
            { L"egyptianblue", Colors::Color::EgyptianBlue },
            { L"electricblue", Colors::Color::ElectricBlue },
            { L"emerald", Colors::Color::Emerald },
            { L"evergreenfog", Colors::Color::EvergreenFog },
            { L"exuberantpink", Colors::Color::ExuberantPink },
            { L"fallow", Colors::Color::Fallow },
            { L"falured", Colors::Color::FaluRed },
            { L"famous", Colors::Color::Famous },
            { L"favoritejeans", Colors::Color::FavoriteJeans },
            { L"fawn", Colors::Color::Fawn },
            { L"feldgrau", Colors::Color::Feldgrau },
            { L"fern", Colors::Color::Fern },
            { L"ferngreen", Colors::Color::FernGreen },
            { L"fielddrab", Colors::Color::FieldDrab },
            { L"firebrick", Colors::Color::Firebrick },
            { L"fireenginered", Colors::Color::FireEngineRed },
            { L"fireweed", Colors::Color::Fireweed },
            { L"firework", Colors::Color::Firework },
            { L"flamingopink", Colors::Color::FlamingoPink },
            { L"flatteringpeach", Colors::Color::FlatteringPeach },
            { L"flax", Colors::Color::Flax },
            { L"fluorescentorange", Colors::Color::FluorescentOrange },
            { L"fluorescentpink", Colors::Color::FluorescentPink },
            { L"folksygold", Colors::Color::FolksyGold },
            { L"forestgreen", Colors::Color::ForestGreen },
            { L"forgetmenot", Colors::Color::ForgetMeNot },
            { L"frenchbeige", Colors::Color::FrenchBeige },
            { L"frenchblue", Colors::Color::FrenchBlue },
            { L"frenchlilac", Colors::Color::FrenchLilac },
            { L"frenchrose", Colors::Color::FrenchRose },
            { L"frolic", Colors::Color::Frolic },
            { L"frosting", Colors::Color::Frosting },
            { L"frostwork", Colors::Color::Frostwork },
            { L"fuchsia", Colors::Color::Fuchsia },
            { L"fulvous", Colors::Color::Fulvous },
            { L"fuzzywuzzy", Colors::Color::FuzzyWuzzy },
            { L"gamboge", Colors::Color::Gamboge },
            { L"ghostwhite", Colors::Color::GhostWhite },
            { L"ginger", Colors::Color::Ginger },
            { L"glacierblue", Colors::Color::GlacierBlue },
            { L"glaucous", Colors::Color::Glaucous },
            { L"gold", Colors::Color::Gold },
            { L"goldenbrown", Colors::Color::GoldenBrown },
            { L"goldenrod", Colors::Color::Goldenrod },
            { L"goldenyellow", Colors::Color::GoldenYellow },
            { L"goldleaf", Colors::Color::GoldLeaf },
            { L"grannysmithapple", Colors::Color::GrannySmithApple },
            { L"grass", Colors::Color::Grass },
            { L"gray", Colors::Color::Gray },
            { L"grayasparagus", Colors::Color::GrayAsparagus },
            { L"green", Colors::Color::Green },
            { L"grullo", Colors::Color::Grullo },
            { L"halayaube", Colors::Color::HalayaUbe },
            { L"harlequin", Colors::Color::Harlequin },
            { L"harvestgold", Colors::Color::HarvestGold },
            { L"heartgold", Colors::Color::HeartGold },
            { L"heliotrope", Colors::Color::Heliotrope },
            { L"hickorysmoke", Colors::Color::HickorySmoke },
            { L"honeydew", Colors::Color::Honeydew },
            { L"hookergreen", Colors::Color::HookerGreen },
            { L"hotmagenta", Colors::Color::HotMagenta },
            { L"hotpink", Colors::Color::HotPink },
            { L"huntergreen", Colors::Color::HunterGreen },
            { L"ice", Colors::Color::Ice },
            { L"icterine", Colors::Color::Icterine },
            { L"inchworm", Colors::Color::Inchworm },
            { L"indigo", Colors::Color::Indigo },
            { L"iris", Colors::Color::Iris },
            { L"ivory", Colors::Color::Ivory },
            { L"jade", Colors::Color::Jade },
            { L"jasmine", Colors::Color::Jasmine },
            { L"jasper", Colors::Color::Jasper },
            { L"jazzagecoral", Colors::Color::JazzAgeCoral },
            { L"jazzberryjam", Colors::Color::JazzberryJam },
            { L"jonquil", Colors::Color::Jonquil },
            { L"junglegreen", Colors::Color::JungleGreen },
            { L"jutebrown", Colors::Color::JuteBrown },
            { L"kellygreen", Colors::Color::KellyGreen },
            { L"khaki", Colors::Color::Khaki },
            { L"latte", Colors::Color::Latte },
            { L"laurelgreen", Colors::Color::LaurelGreen },
            { L"lavender", Colors::Color::Lavender },
            { L"leaves", Colors::Color::Leaves },
            { L"lemon", Colors::Color::Lemon },
            { L"lemonchiffon", Colors::Color::LemonChiffon },
            { L"lemonlime", Colors::Color::LemonLime },
            { L"lemonyellow", Colors::Color::LemonYellow },
            { L"lightapricot", Colors::Color::LightApricot },
            { L"lightblue", Colors::Color::LightBlue },
            { L"lightbrown", Colors::Color::LightBrown },
            { L"lightcarminepink", Colors::Color::LightCarminePink },
            { L"lightcoral", Colors::Color::LightCoral },
            { L"lightcornflowerblue", Colors::Color::LightCornflowerBlue },
            { L"lightgray", Colors::Color::LightGray },
            { L"lightseafoam", Colors::Color::LightSeafoam },
            { L"lilac", Colors::Color::Lilac },
            { L"lime", Colors::Color::Lime },
            { L"limegreen", Colors::Color::LimeGreen },
            { L"lincolngreen", Colors::Color::LincolnGreen },
            { L"linen", Colors::Color::Linen },
            { L"lion", Colors::Color::Lion },
            { L"lust", Colors::Color::Lust },
            { L"macaroniandcheese", Colors::Color::MacaroniAndCheese },
            { L"magenta", Colors::Color::Magenta },
            { L"magicmint", Colors::Color::MagicMint },
            { L"magnolia", Colors::Color::Magnolia },
            { L"mahogany", Colors::Color::Mahogany },
            { L"maize", Colors::Color::Maize },
            { L"majorelleblue", Colors::Color::MajorelleBlue },
            { L"malachite", Colors::Color::Malachite },
            { L"manatee", Colors::Color::Manatee },
            { L"mangotango", Colors::Color::MangoTango },
            { L"marble", Colors::Color::Marble },
            { L"maroon", Colors::Color::Maroon },
            { L"mauve", Colors::Color::Mauve },
            { L"mauvelous", Colors::Color::Mauvelous },
            { L"mauvetaupe", Colors::Color::MauveTaupe },
            { L"meadow", Colors::Color::Meadow },
            { L"melon", Colors::Color::Melon },
            { L"metal", Colors::Color::Metal },
            { L"mint", Colors::Color::Mint },
            { L"mintcream", Colors::Color::MintCream },
            { L"mintgreen", Colors::Color::MintGreen },
            { L"moccasin", Colors::Color::Moccasin },
            { L"modebeige", Colors::Color::ModeBeige },
            { L"moonstoneblue", Colors::Color::MoonstoneBlue },
            { L"moss", Colors::Color::Moss },
            { L"mossgreen", Colors::Color::MossGreen },
            { L"mountainmeadow", Colors::Color::MountainMeadow },
            { L"mountbattenpink", Colors::Color::MountbattenPink },
            { L"mulberry", Colors::Color::Mulberry },
            { L"munsell", Colors::Color::Munsell },
            { L"mustard", Colors::Color::Mustard },
            { L"myrtle", Colors::Color::Myrtle },
            { L"nadeshikopink", Colors::Color::NadeshikoPink },
            { L"napiergreen", Colors::Color::NapierGreen },
            { L"naplesyellow", Colors::Color::NaplesYellow },
            { L"navajowhite", Colors::Color::NavajoWhite },
            { L"navel", Colors::Color::Navel },
            { L"navy", Colors::Color::Navy },
            { L"navyblue", Colors::Color::NavyBlue },
            { L"neoncarrot", Colors::Color::NeonCarrot },
            { L"neonfuchsia", Colors::Color::NeonFuchsia },
            { L"neongreen", Colors::Color::NeonGreen },
            { L"newsprint", Colors::Color::Newsprint },
            { L"ocean", Colors::Color::Ocean },
            { L"oceanboatblue", Colors::Color::OceanBoatBlue },
            { L"oceanic", Colors::Color::Oceanic },
            { L"ochre", Colors::Color::Ochre },
            { L"octobermist", Colors::Color::OctoberMist },
            { L"officegreen", Colors::Color::OfficeGreen },
            { L"oldgold", Colors::Color::OldGold },
            { L"oldlace", Colors::Color::OldLace },
            { L"olive", Colors::Color::Olive },
            { L"olivedrab", Colors::Color::OliveDrab },
            { L"olivegreen", Colors::Color::OliveGreen },
            { L"olivine", Colors::Color::Olivine },
            { L"onyx", Colors::Color::Onyx },
            { L"operamauve", Colors::Color::OperaMauve },
            { L"orange", Colors::Color::Orange },
            { L"orangered", Colors::Color::OrangeRed },
            { L"orangeyellow", Colors::Color::OrangeYellow },
            { L"orchid", Colors::Color::Orchid },
            { L"origamiwhite", Colors::Color::OrigamiWhite },
            { L"otterbrown", Colors::Color::OtterBrown },
            { L"outerspace", Colors::Color::OuterSpace },
            { L"outrageousorange", Colors::Color::OutrageousOrange },
            { L"overcast", Colors::Color::Overcast },
            { L"oxfordblue", Colors::Color::OxfordBlue },
            { L"pacificblue", Colors::Color::PacificBlue },
            { L"pansypurple", Colors::Color::PansyPurple },
            { L"paper", Colors::Color::Paper },
            { L"parisgreen", Colors::Color::ParisGreen },
            { L"pastelblue", Colors::Color::PastelBlue },
            { L"pastelbrown", Colors::Color::PastelBrown },
            { L"pastelgray", Colors::Color::PastelGray },
            { L"pastelgreen", Colors::Color::PastelGreen },
            { L"pastelmagenta", Colors::Color::PastelMagenta },
            { L"pastelorange", Colors::Color::PastelOrange },
            { L"pastelpink", Colors::Color::PastelPink },
            { L"pastelpurple", Colors::Color::PastelPurple },
            { L"pastelred", Colors::Color::PastelRed },
            { L"pastelviolet", Colors::Color::PastelViolet },
            { L"pastelyellow", Colors::Color::PastelYellow },
            { L"patriarch", Colors::Color::Patriarch },
            { L"paynegrey", Colors::Color::PayneGrey },
            { L"peach", Colors::Color::Peach },
            { L"peacockblue", Colors::Color::PeacockBlue },
            { L"pear", Colors::Color::Pear },
            { L"pearl", Colors::Color::Pearl },
            { L"pencilyellow", Colors::Color::PencilYellow },
            { L"peridot", Colors::Color::Peridot },
            { L"periwinkle", Colors::Color::Periwinkle },
            { L"petal", Colors::Color::Petal },
            { L"pewter", Colors::Color::Pewter },
            { L"phlox", Colors::Color::Phlox },
            { L"phthaloblue", Colors::Color::PhthaloBlue },
            { L"phthalogreen", Colors::Color::PhthaloGreen },
            { L"piggypink", Colors::Color::PiggyPink },
            { L"pinegreen", Colors::Color::PineGreen },
            { L"pink", Colors::Color::Pink },
            { L"pinkeraser", Colors::Color::PinkEraser },
            { L"pinkflamingo", Colors::Color::PinkFlamingo },
            { L"pinkpearl", Colors::Color::PinkPearl },
            { L"pinkshadow", Colors::Color::PinkShadow },
            { L"pinksherbet", Colors::Color::PinkSherbet },
            { L"pinktulip", Colors::Color::PinkTulip },
            { L"pinkybeige", Colors::Color::PinkyBeige },
            { L"pistachio", Colors::Color::Pistachio },
            { L"platinum", Colors::Color::Platinum },
            { L"plum", Colors::Color::Plum },
            { L"poppy", Colors::Color::Poppy },
            { L"powderblue", Colors::Color::PowderBlue },
            { L"practicalbeige", Colors::Color::PracticalBeige },
            { L"prussianblue", Colors::Color::PrussianBlue },
            { L"psychedelicpurple", Colors::Color::PsychedelicPurple },
            { L"puce", Colors::Color::Puce },
            { L"pumpkin", Colors::Color::Pumpkin },
            { L"purewhite", Colors::Color::PureWhite },
            { L"purple", Colors::Color::Purple },
            { L"purpleheart", Colors::Color::PurpleHeart },
            { L"purplemountainmajesty", Colors::Color::PurpleMountainMajesty },
            { L"purplepizzazz", Colors::Color::PurplePizzazz },
            { L"purpletaupe", Colors::Color::PurpleTaupe },
            { L"rackley", Colors::Color::Rackley },
            { L"radiantlilac", Colors::Color::RadiantLilac },
            { L"rain", Colors::Color::Rain },
            { L"raspberry", Colors::Color::Raspberry },
            { L"raspberryglace", Colors::Color::RaspberryGlace },
            { L"raspberrypink", Colors::Color::RaspberryPink },
            { L"razzledazzlerose", Colors::Color::RazzleDazzleRose },
            { L"razzmatazz", Colors::Color::Razzmatazz },
            { L"red", Colors::Color::Red },
            { L"redtomato", Colors::Color::RedTomato },
            { L"robineggblue", Colors::Color::RobinEggBlue },
            { L"rose", Colors::Color::Rose },
            { L"rosegold", Colors::Color::RoseGold },
            { L"rosemadder", Colors::Color::RoseMadder },
            { L"rosepink", Colors::Color::RosePink },
            { L"rosequartz", Colors::Color::RoseQuartz },
            { L"rosetan", Colors::Color::RoseTan },
            { L"rosetaupe", Colors::Color::RoseTaupe },
            { L"rosevale", Colors::Color::RoseVale },
            { L"rosewood", Colors::Color::Rosewood },
            { L"rossocorsa", Colors::Color::RossoCorsa },
            { L"rosybrown", Colors::Color::RosyBrown },
            { L"rosyoutlook", Colors::Color::RosyOutlook },
            { L"royalblue", Colors::Color::RoyalBlue },
            { L"royalfuchsia", Colors::Color::RoyalFuchsia },
            { L"royalpurple", Colors::Color::RoyalPurple },
            { L"ruby", Colors::Color::Ruby },
            { L"ruddybrown", Colors::Color::RuddyBrown },
            { L"rust", Colors::Color::Rust },
            { L"saddlebrown", Colors::Color::SaddleBrown },
            { L"safetyorange", Colors::Color::SafetyOrange },
            { L"saffron", Colors::Color::Saffron },
            { L"salmon", Colors::Color::Salmon },
            { L"salmonpink", Colors::Color::SalmonPink },
            { L"salonrose", Colors::Color::SalonRose },
            { L"sand", Colors::Color::Sand },
            { L"sanddune", Colors::Color::SandDune },
            { L"sandstorm", Colors::Color::Sandstorm },
            { L"sandybrown", Colors::Color::SandyBrown },
            { L"sapgreen", Colors::Color::SapGreen },
            { L"sapphire", Colors::Color::Sapphire },
            { L"satinsheengold", Colors::Color::SatinSheenGold },
            { L"scarlet", Colors::Color::Scarlet },
            { L"schoolbusyellow", Colors::Color::SchoolBusYellow },
            { L"seablue", Colors::Color::SeaBlue },
            { L"seagreen", Colors::Color::SeaGreen },
            { L"sealbrown", Colors::Color::SealBrown },
            { L"seashell", Colors::Color::Seashell },
            { L"sepia", Colors::Color::Sepia },
            { L"serenity", Colors::Color::Serenity },
            { L"shadow", Colors::Color::Shadow },
            { L"shamrock", Colors::Color::Shamrock },
            { L"sienna", Colors::Color::Sienna },
            { L"silver", Colors::Color::Silver },
            { L"sinopia", Colors::Color::Sinopia },
            { L"skobeloff", Colors::Color::Skobeloff },
            { L"sky", Colors::Color::Sky },
            { L"skyblue", Colors::Color::SkyBlue },
            { L"slate", Colors::Color::Slate },
            { L"slateblue", Colors::Color::SlateBlue },
            { L"slategray", Colors::Color::SlateGray },
            { L"slytherin1", Colors::Color::Slytherin1 },
            { L"slytherin2", Colors::Color::Slytherin2 },
            { L"slytherin3", Colors::Color::Slytherin3 },
            { L"slytherin4", Colors::Color::Slytherin4 },
            { L"smalt", Colors::Color::Smalt },
            { L"smokeytopaz", Colors::Color::SmokeyTopaz },
            { L"smokyblack", Colors::Color::SmokyBlack },
            { L"smokysalmon", Colors::Color::SmokySalmon },
            { L"snow", Colors::Color::Snow },
            { L"spicedcider", Colors::Color::SpicedCider },
            { L"spirodiscoball", Colors::Color::SpiroDiscoBall },
            { L"springbud", Colors::Color::SpringBud },
            { L"springgreen", Colors::Color::SpringGreen },
            { L"steel", Colors::Color::Steel },
            { L"steelblue", Colors::Color::SteelBlue },
            { L"stem", Colors::Color::Stem },
            { L"stizza", Colors::Color::Stizza },
            { L"stormcloud", Colors::Color::Stormcloud },
            { L"straw", Colors::Color::Straw },
            { L"strawberry", Colors::Color::Strawberry },
            { L"studiomauve", Colors::Color::StudioMauve },
            { L"sunbeamyellow", Colors::Color::SunbeamYellow },
            { L"sunflower", Colors::Color::Sunflower },
            { L"sunglow", Colors::Color::Sunglow },
            { L"sunset", Colors::Color::Sunset },
            { L"sunsetorange", Colors::Color::SunsetOrange },
            { L"sveltesage", Colors::Color::SvelteSage },
            { L"tan", Colors::Color::Tan },
            { L"tangelo", Colors::Color::Tangelo },
            { L"tangerine", Colors::Color::Tangerine },
            { L"taupe", Colors::Color::Taupe },
            { L"tawny", Colors::Color::Tawny },
            { L"teagreen", Colors::Color::TeaGreen },
            { L"teal", Colors::Color::Teal },
            { L"tearose", Colors::Color::TeaRose },
            { L"terracotta", Colors::Color::TerraCotta },
            { L"thistle", Colors::Color::Thistle },
            { L"thulianpink", Colors::Color::ThulianPink },
            { L"thundercloud", Colors::Color::ThunderCloud },
            { L"tiffanyblue", Colors::Color::TiffanyBlue },
            { L"tigereye", Colors::Color::TigerEye },
            { L"timberwolf", Colors::Color::Timberwolf },
            { L"titaniumyellow", Colors::Color::TitaniumYellow },
            { L"toffee", Colors::Color::Toffee },
            { L"tomato", Colors::Color::Tomato },
            { L"toolbox", Colors::Color::Toolbox },
            { L"topaz", Colors::Color::Topaz },
            { L"tractorred", Colors::Color::TractorRed },
            { L"tradewind", Colors::Color::Tradewind },
            { L"tricornblack", Colors::Color::TricornBlack },
            { L"trolleygrey", Colors::Color::TrolleyGrey },
            { L"tropicalrainforest", Colors::Color::TropicalRainForest },
            { L"tumbleweed", Colors::Color::Tumbleweed },
            { L"turkishrose", Colors::Color::TurkishRose },
            { L"turquoise", Colors::Color::Turquoise },
            { L"twilightlavender", Colors::Color::TwilightLavender },
            { L"tyrianpurple", Colors::Color::TyrianPurple },
            { L"umber", Colors::Color::Umber },
            { L"unmellowyellow", Colors::Color::UnmellowYellow },
            { L"urbanputty", Colors::Color::UrbanPutty },
            { L"urobilin", Colors::Color::Urobilin },
            { L"vanilla", Colors::Color::Vanilla },
            { L"vegasgold", Colors::Color::VegasGold },
            { L"venetianred", Colors::Color::VenetianRed },
            { L"verdigris", Colors::Color::Verdigris },
            { L"vermilion", Colors::Color::Vermilion },
            { L"veronica", Colors::Color::Veronica },
            { L"violet", Colors::Color::Violet },
            { L"viridian", Colors::Color::Viridian },
            { L"vividauburn", Colors::Color::VividAuburn },
            { L"vividburgundy", Colors::Color::VividBurgundy },
            { L"vividcerise", Colors::Color::VividCerise },
            { L"vividtangerine", Colors::Color::VividTangerine },
            { L"vividviolet", Colors::Color::VividViolet },
            { L"voguegreen", Colors::Color::VogueGreen },
            { L"warmblack", Colors::Color::WarmBlack },
            { L"warmgray", Colors::Color::WarmGray },
            { L"waterfall", Colors::Color::Waterfall },
            { L"waterspout", Colors::Color::Waterspout },
            { L"watery", Colors::Color::Watery },
            { L"wave", Colors::Color::Wave },
            { L"wenge", Colors::Color::Wenge },
            { L"wheat", Colors::Color::Wheat },
            { L"white", Colors::Color::White },
            { L"wholewheat", Colors::Color::WholeWheat },
            { L"wildblueyonder", Colors::Color::WildBlueYonder },
            { L"wildstrawberry", Colors::Color::WildStrawberry },
            { L"wildwatermelon", Colors::Color::WildWatermelon },
            { L"wine", Colors::Color::Wine },
            { L"wisteria", Colors::Color::Wisteria },
            { L"wood", Colors::Color::Wood },
            { L"xanadu", Colors::Color::Xanadu },
            { L"yellow", Colors::Color::Yellow },
            { L"yellowpepper", Colors::Color::YellowPepper },
            { L"zinnwalditebrown", Colors::Color::ZinnwalditeBrown }
            };

        // see if it is one of our defined colors
        auto foundPos = values.find(std::wstring_view(colorStr.MakeLower().wc_str()));
        if (foundPos != values.cend())
            {
            return Colors::ColorBrewer::GetColor(foundPos->second);
            }

        // otherwise, load it with wx (which will support hex coded values and other names)
        return wxColour(colorStr);
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
    std::shared_ptr<Colors::Schemes::ColorScheme> ReportBuilder::LoadColorScheme(
        const wxSimpleJSON::Ptr_t& colorSchemeNode)
        {
        std::vector<wxColour> colors;
        const auto colorValues = colorSchemeNode->GetValueStringVector();
        if (colorValues.size() == 0)
            { return nullptr; }
        for (auto& color : colorValues)
            { colors.emplace_back(ConvertColor(color)); }
        return std::make_shared<Colors::Schemes::ColorScheme>(colors);
        }

    //---------------------------------------------------
    std::shared_ptr<IconShapeScheme> ReportBuilder::LoadIconScheme(
        const wxSimpleJSON::Ptr_t& iconSchemeNode)
        {
        // use standard string, wxString should not be constructed globally
        static const std::map<std::wstring_view, IconShape> values =
            {
            { L"blank-icon", IconShape::BlankIcon },
            { L"horizontal-line-icon", IconShape::HorizontalLineIcon },
            { L"arrow-right-icon", IconShape::ArrowRightIcon },
            { L"circle-icon", IconShape::CircleIcon },
            { L"image-icon", IconShape::ImageIcon },
            { L"horizontal-separator", IconShape::HorizontalSeparator },
            { L"horizontal-arrow-right-separator", IconShape::HorizontalArrowRightSeparator },
            { L"image-wholelegend", IconShape::ImageWholeLegend },
            { L"color-gradient-icon", IconShape::ColorGradientIcon },
            { L"square-icon", IconShape::SquareIcon },
            { L"triangle-upward-icon", IconShape::TriangleUpwardIcon },
            { L"triangle-downward-icon", IconShape::TriangleDownwardIcon },
            { L"triangle-right-icon", IconShape::TriangleRightIcon },
            { L"triangle-left-icon", IconShape::TriangleLeftIcon },
            { L"diamond-icon", IconShape::DiamondIcon },
            { L"cross-icon", IconShape::CrossIcon },
            { L"asterisk-icon", IconShape::AsteriskIcon },
            { L"hexagon-icon", IconShape::HexagonIcon },
            { L"box-plot-icon", IconShape::BoxPlotIcon },
            { L"location-marker", IconShape::LocationMarker },
            { L"go-road-sign", IconShape::GoRoadSign },
            { L"warning-road-sign", IconShape::WarningRoadSign }
            };

        std::vector<IconShape> icons;
        auto iconValues = iconSchemeNode->GetValueStringVector();
        if (iconValues.size() == 0)
            { return nullptr; }
        for (auto& icon : iconValues)
            {
            auto foundPos = values.find(std::wstring_view(icon.MakeLower().wc_str()));
            if (foundPos != values.cend())
                { icons.emplace_back(foundPos->second); }
            }
        if (icons.size() == 0)
            { return nullptr; }
        return std::make_shared<IconShapeScheme>(icons);
        }

    //---------------------------------------------------
    std::shared_ptr<GraphItems::Image> ReportBuilder::LoadImage(
        const wxSimpleJSON::Ptr_t& imageNode,
        Canvas* canvas, size_t& currentRow, size_t& currentColumn)
        {
        auto path = imageNode->GetProperty(L"path")->GetValueString();
        if (path.empty())
            {
            throw std::runtime_error(
                wxString(_(L"image must have a filepath.")).ToUTF8());
            }
        if (!wxFileName::FileExists(path))
            {
            path = wxFileName(m_configFilePath).GetPathWithSep() + path;
            if (!wxFileName::FileExists(path))
                {
                throw std::runtime_error(
                    wxString(_(L"image not found.")).ToUTF8());
                }
            }
        auto image = std::make_shared<GraphItems::Image>(path);
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

        item->SetScaling(itemNode->GetProperty(L"scaling")->GetValueNumber(1));

        LoadPen(itemNode->GetProperty(L"pen"), item->GetPen());

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