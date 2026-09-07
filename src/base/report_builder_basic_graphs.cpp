///////////////////////////////////////////////////////////////////////////////
// Name:        report_builder_basic_graphs.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "reportbuilder.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

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
                wxString::Format(L"'%s': %s", kmlFile, geoData->GetLastError()).ToUTF8());
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

    //---------------------------------------------------
    std::shared_ptr<Graphs::Graph2D>
    ReportBuilder::LoadNightingaleRoseChart(const wxSimpleJSON::Ptr_t& graphNode, Canvas* canvas,
                                            size_t& currentRow, size_t& currentColumn)
        {
        const wxString dsName = graphNode->GetProperty(L"dataset")->AsString();
        const auto foundPos = m_datasets.find(dsName);
        if (foundPos == m_datasets.cend() || foundPos->second == nullptr)
            {
            throw std::runtime_error(
                wxString::Format(_(L"%s: dataset not found for Nightingale rose chart."), dsName)
                    .ToUTF8());
            }

        const auto variablesNode = graphNode->GetProperty(L"variables");
        if (!variablesNode->IsOk())
            {
            throw std::runtime_error(
                _(L"Variables not defined for Nightingale rose chart.").ToUTF8());
            }

        const auto aggVarNameRaw = variablesNode->GetProperty(L"aggregate")->AsString();
        const auto aggVarName = ExpandConstants(aggVarNameRaw);
        const auto categoryVarNameRaw = variablesNode->GetProperty(L"category")->AsString();
        const auto categoryVarName = ExpandConstants(categoryVarNameRaw);
        const auto groupVarNameRaw = variablesNode->GetProperty(L"group")->AsString();
        const auto groupVarName = ExpandConstants(groupVarNameRaw);

        if (categoryVarName.empty())
            {
            throw std::runtime_error(
                wxString::Format(
                    _(L"%s: category variable not specified for Nightingale rose chart."), dsName)
                    .ToUTF8());
            }

        auto roseChart = std::make_shared<Graphs::NightingaleRoseChart>(
            canvas, LoadBrushScheme(graphNode->GetProperty(L"brush-scheme")),
            LoadGraphColorScheme(graphNode));
        if (!aggVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.aggregate", aggVarNameRaw);
            }
        if (!categoryVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.category", categoryVarNameRaw);
            }
        if (!groupVarNameRaw.empty())
            {
            roseChart->SetPropertyTemplate(L"variables.group", groupVarNameRaw);
            }

        roseChart->SetData(
            foundPos->second,
            (!aggVarName.empty() ? std::optional<wxString>(aggVarName) : std::nullopt),
            categoryVarName,
            (!groupVarName.empty() ? std::optional<wxString>(groupVarName) : std::nullopt));

        if (const auto radialScaling = ReportEnumConvert::ConvertNightingaleRoseRadialScaling(
                graphNode->GetProperty(L"radial-scaling")->AsString());
            radialScaling.has_value())
            {
            roseChart->SetRadialScaling(radialScaling.value());
            }

        if (const auto seriesDisplay = ReportEnumConvert::ConvertNightingaleRoseSeriesDisplay(
                graphNode->GetProperty(L"series-display")->AsString());
            seriesDisplay.has_value())
            {
            roseChart->SetSeriesDisplay(seriesDisplay.value());
            }

        if (graphNode->HasProperty(L"start-angle"))
            {
            roseChart->SetStartAngle(graphNode->GetProperty(L"start-angle")->AsDouble(90.0));
            }

        if (graphNode->HasProperty(L"show-labels"))
            {
            roseChart->ShowLabels(graphNode->GetProperty(L"show-labels")->AsBool());
            }

        if (graphNode->HasProperty(L"ghost-opacity"))
            {
            const double rawGhostOpacity =
                graphNode->GetProperty(L"ghost-opacity")->AsDouble(Settings::GHOST_OPACITY);
            roseChart->SetGhostOpacity(static_cast<uint8_t>(std::clamp(
                std::isfinite(rawGhostOpacity) ? rawGhostOpacity :
                                                 static_cast<double>(Settings::GHOST_OPACITY),
                0.0, 255.0)));
            }

        if (const auto ghostedWedgesNode = graphNode->GetProperty(L"ghosted-wedges");
            ghostedWedgesNode->IsOk() && ghostedWedgesNode->IsValueArray())
            {
            for (const auto& ghostNode : ghostedWedgesNode->AsNodes())
                {
                const auto groupLabel =
                    ExpandConstants(ghostNode->GetProperty(L"group")->AsString());
                if (groupLabel.empty())
                    {
                    continue;
                    }
                roseChart->GhostWedge(
                    groupLabel, ExpandConstants(ghostNode->GetProperty(L"category")->AsString()));
                }
            }

        LoadGraph(graphNode, canvas, currentRow, currentColumn, roseChart);
        return roseChart;
        }
    } // namespace Wisteria
