///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_basic_graphs.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"
#include <cmath>

namespace Wisteria
    {
    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadChoroplethMap(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                     size_t& currentRow, size_t& currentColumn)
        {
        const wxString kmlFileRaw = graphNode->GetProperty(_DT(L"kml-file"))->AsString();
        if (kmlFileRaw.empty())
            {
            throw std::runtime_error(
                _(L"A KML file must be specified for a choropleth map.").ToUTF8());
            }
        // don't allow trying to load external paths
        if (kmlFileRaw.StartsWith(L"\\\\") || kmlFileRaw.StartsWith(L"//"))
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': network paths are not allowed for KML files."),
                                 kmlFileRaw)
                    .ToUTF8());
            }
        const wxString kmlFile = NormalizeFilePath(kmlFileRaw);
        const wxString kmlIdField = graphNode->GetProperty(_DT(L"kml-id-field"))->AsString();

        auto geoData = std::make_shared<Data::GeoDataset>();
        if (!geoData->ImportKML(kmlFile, Data::GeoImportInfo().IdField(kmlIdField)))
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': %s"), kmlFile, geoData->GetLastError()).ToUTF8());
            }

        // optional dataset merged in for shading
        wxString dataSourceName;
        wxString dataSourceKeyColumn;
        wxString valueColumn;
        wxString categoryColumn;
        wxString symbolColumn;
        if (const auto dataSourceNode = graphNode->GetProperty(_DT(L"data-source"));
            dataSourceNode->IsOk())
            {
            dataSourceName = dataSourceNode->GetProperty(_DT(L"dataset"))->AsString();
            dataSourceKeyColumn = dataSourceNode->GetProperty(_DT(L"key-column"))->AsString();
            valueColumn = dataSourceNode->GetProperty(_DT(L"value-column"))->AsString();
            categoryColumn = dataSourceNode->GetProperty(_DT(L"category-column"))->AsString();
            symbolColumn = dataSourceNode->GetProperty(_DT(L"symbol-column"))->AsString();

            const auto foundSource = m_datasets.find(dataSourceName);
            if (foundSource == m_datasets.cend() || foundSource->second == nullptr)
                {
                throw std::runtime_error(
                    wxString::Format(_(L"%s: dataset not found for choropleth map."),
                                     dataSourceName)
                        .ToUTF8());
                }
            if (!categoryColumn.empty())
                {
                geoData->CopyCategoricalColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                   categoryColumn, categoryColumn);
                }
            else if (!valueColumn.empty())
                {
                geoData->CopyContinuousColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                  valueColumn, valueColumn);
                }
            if (!symbolColumn.empty() && symbolColumn != valueColumn &&
                symbolColumn != categoryColumn)
                {
                geoData->CopyContinuousColumnFrom(*foundSource->second, dataSourceKeyColumn,
                                                  symbolColumn, symbolColumn);
                }
            }

        const wxString shadingColumn = !categoryColumn.empty() ? categoryColumn : valueColumn;

        auto choroplethMap =
            std::make_shared<Graphs::ChoroplethMap>(canvas, LoadGraphColorScheme(graphNode));

        // classification of the value column, applied before SetData() computes the class colors
        if (const auto classMethodNode = graphNode->GetProperty(_DT(L"classification-method"));
            classMethodNode->IsOk() &&
            classMethodNode->AsString().CmpNoCase(L"jenks-natural-breaks") == 0)
            {
            choroplethMap->SetClassificationMethod(
                Graphs::ChoroplethMap::ClassificationMethod::JenksNaturalBreaks);
            }
        if (const auto classCountNode = graphNode->GetProperty(_DT(L"classification-count"));
            classCountNode->IsOk())
            {
            // screen the JSON value first to avoid a wild or non-finite number
            if (const double rawClassCount = classCountNode->AsDouble(5);
                std::isfinite(rawClassCount) && rawClassCount >= 2.0 && rawClassCount <= 12.0)
                {
                choroplethMap->SetClassCount(static_cast<size_t>(rawClassCount));
                }
            }

        choroplethMap->SetData(
            geoData, shadingColumn.empty() ? std::nullopt : std::optional<wxString>(shadingColumn));
        choroplethMap->SetSourceInfo(kmlFile, kmlIdField, dataSourceName, dataSourceKeyColumn);

        if (!symbolColumn.empty())
            {
            choroplethMap->SetProportionalSymbolColumn(symbolColumn);
            }

        if (const auto labelsNode = graphNode->GetProperty(_DT(L"show-region-labels"));
            labelsNode->IsOk())
            {
            choroplethMap->ShowRegionLabels(labelsNode->AsBool());
            }

        if (const auto graticuleNode = graphNode->GetProperty(_DT(L"show-graticule"));
            graticuleNode->IsOk())
            {
            choroplethMap->ShowGraticule(graticuleNode->AsBool());
            }

        if (const auto regionLabelDisplayNode =
                graphNode->GetProperty(_DT(L"region-label-display"));
            regionLabelDisplayNode->IsOk())
            {
            if (const auto labelDisplay =
                    ReportEnumConvert::ConvertBinLabelDisplay(regionLabelDisplayNode->AsString());
                labelDisplay.has_value())
                {
                choroplethMap->SetLabelDisplay(labelDisplay.value());
                }
            }

        const wxString projectionStr = graphNode->GetProperty(_DT(L"projection"))->AsString();
        if (projectionStr.CmpNoCase(L"equirectangular") == 0)
            {
            choroplethMap->SetMapProjection(Graphs::ChoroplethMap::MapProjection::Equirectangular);
            }
        else if (projectionStr.CmpNoCase(L"albers-equal-area-conic") == 0)
            {
            choroplethMap->SetMapProjection(
                Graphs::ChoroplethMap::MapProjection::AlbersEqualAreaConic);
            }
        else if (projectionStr.CmpNoCase(L"equal-earth") == 0)
            {
            choroplethMap->SetMapProjection(Graphs::ChoroplethMap::MapProjection::EqualEarth);
            }

        if (const wxColour noDataColor(ConvertColor(graphNode->GetProperty(_DT(L"no-data-color"))));
            noDataColor.IsOk())
            {
            choroplethMap->SetNoDataColor(noDataColor);
            }

        if (const wxColour symbolColor(ConvertColor(graphNode->GetProperty(_DT(L"symbol-color"))));
            symbolColor.IsOk())
            {
            choroplethMap->SetProportionalSymbolColor(symbolColor);
            }

        if (const auto noDataFillStyleNode = graphNode->GetProperty(_DT(L"no-data-fill-style"));
            noDataFillStyleNode->IsOk())
            {
            if (const auto fillStyle =
                    ReportEnumConvert::ConvertBrushStyle(noDataFillStyleNode->AsString());
                fillStyle.has_value())
                {
                choroplethMap->SetNoDataFillStyle(fillStyle.value());
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, choroplethMap);
        return choroplethMap;
        }
    } // namespace Wisteria
