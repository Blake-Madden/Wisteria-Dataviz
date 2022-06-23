#include "reportbuilder.h"

using namespace Wisteria::Data;
using namespace Wisteria::Graphs;

namespace Wisteria
    {
    //---------------------------------------------------
    std::vector<Canvas*> ReportBuilder::LoadConfigurationFile(const wxString& filePath,
                                                              wxWindow* parent)
        {
        std::vector<Canvas*> reportPages;

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
                                    if (typeProperty->IsOk())
                                        {
                                        try
                                            {
                                            if (typeProperty->GetValueString().CmpNoCase(L"line-plot") == 0)
                                                {
                                                LoadLinePlot(item, canvas, currentRow, currentColumn);
                                                }
                                            }
                                        // show error, but OK to keep going
                                        catch (const std::exception& err)
                                            {
                                            wxMessageBox(wxString::FromUTF8(wxString::FromUTF8(err.what())),
                                                         _(L"Canvas Item Error"), wxOK|wxICON_ERROR|wxCENTRE);
                                            }
                                        }
                                    ++currentColumn;
                                    }
                                ++currentRow;
                                }
                            }
                        }
                    canvas->CalcRowDimensions();
                    reportPages.push_back(canvas);
                    }
                }
            }

        return reportPages;
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
    void ReportBuilder::LoadLinePlot(
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
                LoadGraph(graphNode, canvas, currentRow, currentColumn, linePlot);
                }
            }
        }

    //---------------------------------------------------
    void ReportBuilder::LoadGraph(const wxSimpleJSON::Ptr_t& graphNode,
                                  Canvas* canvas, size_t& currentRow, size_t& currentColumn,
                                  std::shared_ptr<Graphs::Graph2D> graph)
        {
        // title information
        auto titleProperty = graphNode->GetProperty(L"title");
        if (titleProperty->IsOk())
            {
            graph->GetTitle().SetText(titleProperty->GetProperty(L"text")->GetValueString());
            }

        // is there a legend?
        auto legendNode = graphNode->GetProperty(L"legend");
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
        }
    }