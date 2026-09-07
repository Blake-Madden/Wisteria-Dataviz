///////////////////////////////////////////////////////////////////////////////
// Name:        geojsonreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "geojsonreader.h"
#include "../../util/donttranslate.h"
#include "../../util/fileutil.h"
#include "../wxSimpleJSON/src/wxSimpleJSON.h"
#include <algorithm>
#include <array>
#include <cmath>
#include <set>
#include <string>
#include <utility>
#include <wx/log.h>
#include <wx/translation.h>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    // Renders a JSON number as text. A whole number is written without a fractional
    // part, so that a numeric key (a GEOID, say) still matches a string key in a
    // dataset it is merged with.
    static wxString FormatJsonNumber(const double value)
        {
        if (std::isfinite(value) && std::floor(value) == value && std::fabs(value) < 1e15)
            {
            return std::to_wstring(static_cast<long long>(value));
            }
        return wxString::FromCDouble(value);
        }

    //---------------------------------------------------
    bool GeoJsonReader::LoadFile(const wxString& filePath)
        {
        ResetFeatures();

        switch (CheckFileSizeLimit(filePath, MAX_GEOJSON_FILE_BYTES))
            {
        case FileSizeCheckResult::TooLarge:
            m_lastError =
                wxString::Format(_(L"'%s': GeoJSON file is too large to read."), filePath);
            return false;
        case FileSizeCheckResult::Unreadable:
            m_lastError = wxString::Format(_(L"'%s': unable to read GeoJSON file."), filePath);
            return false;
        case FileSizeCheckResult::WithinLimit:
            break;
            }

        const wxSimpleJSON::Ptr_t rootNode = wxSimpleJSON::LoadFile(filePath);
        if (!rootNode || !rootNode->IsOk())
            {
            m_lastError = wxString::Format(_(L"'%s': unable to parse GeoJSON file."), filePath);
            return false;
            }
        return ReadDocument(*rootNode);
        }

    //---------------------------------------------------
    bool GeoJsonReader::LoadText(const wxString& geoJsonText)
        {
        ResetFeatures();

        const wxSimpleJSON::Ptr_t rootNode = wxSimpleJSON::Create(geoJsonText, true);
        if (!rootNode || !rootNode->IsOk())
            {
            m_lastError = _(L"Unable to parse GeoJSON content.");
            return false;
            }
        return ReadDocument(*rootNode);
        }

    //---------------------------------------------------
    std::vector<wxString> GeoJsonReader::ReadFieldNames(const wxString& filePath)
        {
        if (CheckFileSizeLimit(filePath, MAX_GEOJSON_FILE_BYTES) !=
            FileSizeCheckResult::WithinLimit)
            {
            return {};
            }

        const wxSimpleJSON::Ptr_t rootNode = wxSimpleJSON::LoadFile(filePath);
        if (!rootNode || !rootNode->IsOk() || !rootNode->IsValueObject())
            {
            return {};
            }

        std::set<wxString> fieldNames;
        const auto collectFromFeature = [&fieldNames](const wxSimpleJSON::Ptr_t& featureNode)
        {
            if (!featureNode || !featureNode->IsOk())
                {
                return;
                }
            const auto propertiesNode = featureNode->GetProperty(_DT(L"properties"));
            if (propertiesNode->IsOk() && propertiesNode->IsValueObject())
                {
                for (const auto& fieldName : propertiesNode->GetObjectKeys())
                    {
                    fieldNames.insert(fieldName);
                    }
                }
        };

        const wxString rootType = rootNode->GetProperty(_DT(L"type"))->AsString();
        if (rootType.CmpNoCase(_DT(L"FeatureCollection")) == 0)
            {
            if (const auto featuresNode = rootNode->GetProperty(_DT(L"features"));
                featuresNode->IsOk() && featuresNode->IsValueArray())
                {
                const auto featureNodes = featuresNode->AsNodes();
                // sample only up to the region cap, so a hostile file cannot make
                // this scan every feature in a huge array
                const size_t featureCount = std::min(featureNodes.size(), MAX_REGION_COUNT);
                for (size_t featureIndex = 0; featureIndex < featureCount; ++featureIndex)
                    {
                    collectFromFeature(featureNodes[featureIndex]);
                    }
                }
            }
        else if (rootType.CmpNoCase(_DT(L"Feature")) == 0)
            {
            collectFromFeature(rootNode);
            }

        return { fieldNames.cbegin(), fieldNames.cend() };
        }

    //---------------------------------------------------
    bool GeoJsonReader::ReadDocument(wxSimpleJSON& rootNode)
        {
        if (!rootNode.IsValueObject())
            {
            m_lastError = _(L"The GeoJSON document's root is not an object.");
            return false;
            }

        // "name" is not part of RFC 7946, but ogr2ogr and other exporters write one
        if (const auto nameNode = rootNode.GetProperty(_DT(L"name"));
            nameNode->IsOk() && nameNode->IsValueString())
            {
            m_name = nameNode->AsString();
            }

        const wxString rootType = rootNode.GetProperty(_DT(L"type"))->AsString();
        if (rootType.CmpNoCase(_DT(L"FeatureCollection")) == 0)
            {
            const auto featuresNode = rootNode.GetProperty(_DT(L"features"));
            if (!featuresNode->IsOk() || !featuresNode->IsValueArray())
                {
                m_lastError = _(L"The GeoJSON FeatureCollection has no \"features\" array.");
                return false;
                }
            auto featureNodes = featuresNode->AsNodes();
            if (featureNodes.size() > MAX_REGION_COUNT)
                {
                wxLogWarning(L"GeoJSON file holds %zu features; only the first %zu will be read.",
                             featureNodes.size(), MAX_REGION_COUNT);
                featureNodes.resize(MAX_REGION_COUNT);
                }
            m_regions.reserve(featureNodes.size());
            for (const auto& featureNode : featureNodes)
                {
                if (featureNode && featureNode->IsOk())
                    {
                    ReadFeature(*featureNode);
                    }
                }
            }
        else if (rootType.CmpNoCase(_DT(L"Feature")) == 0)
            {
            ReadFeature(rootNode);
            }
        else if (rootType.CmpNoCase(_DT(L"Polygon")) == 0 ||
                 rootType.CmpNoCase(_DT(L"MultiPolygon")) == 0 ||
                 rootType.CmpNoCase(_DT(L"GeometryCollection")) == 0 ||
                 rootType.CmpNoCase(_DT(L"Point")) == 0 ||
                 rootType.CmpNoCase(_DT(L"MultiPoint")) == 0 ||
                 rootType.CmpNoCase(_DT(L"LineString")) == 0 ||
                 rootType.CmpNoCase(_DT(L"MultiLineString")) == 0)
            {
            ReadBareGeometry(rootNode);
            }
        else
            {
            m_lastError = _(L"The GeoJSON document has an unrecognized \"type\".");
            return false;
            }

        if (m_regions.empty())
            {
            if (m_lastError.empty())
                {
                m_lastError = _(L"No mappable regions were found in the GeoJSON file.");
                }
            return false;
            }
        return true;
        }

    //---------------------------------------------------
    void GeoJsonReader::ReadFeature(wxSimpleJSON& featureNode)
        {
        GeoRegion region;

        if (const auto propertiesNode = featureNode.GetProperty(_DT(L"properties"));
            propertiesNode->IsOk() && propertiesNode->IsValueObject())
            {
            ReadProperties(*propertiesNode, region);
            }

        AssignRegionName(featureNode, region);

        if (const auto geometryNode = featureNode.GetProperty(_DT(L"geometry"));
            geometryNode->IsOk() && geometryNode->IsValueObject())
            {
            ReadGeometry(*geometryNode, region, 0);
            }

        if (region.m_polygons.empty())
            {
            // a Point or LineString feature, or one with a null geometry, has no
            // fillable area, so drop it rather than adding an empty region
            return;
            }
        m_boundingBox.Encompass(region.m_boundingBox);
        m_regions.push_back(std::move(region));
        }

    //---------------------------------------------------
    void GeoJsonReader::ReadBareGeometry(wxSimpleJSON& geometryNode)
        {
        GeoRegion region;
        ReadGeometry(geometryNode, region, 0);
        if (region.m_polygons.empty())
            {
            return;
            }
        m_boundingBox.Encompass(region.m_boundingBox);
        m_regions.push_back(std::move(region));
        }

    //---------------------------------------------------
    void GeoJsonReader::ReadGeometry(wxSimpleJSON& geometryNode, GeoRegion& region,
                                     const int depth) const
        {
        if (depth > MAX_TRAVERSAL_DEPTH || !geometryNode.IsValueObject())
            {
            return;
            }

        const wxString geometryType = geometryNode.GetProperty(_DT(L"type"))->AsString();
        if (geometryType.CmpNoCase(_DT(L"Polygon")) == 0)
            {
            if (const auto coordinatesNode = geometryNode.GetProperty(_DT(L"coordinates"));
                coordinatesNode->IsOk() && coordinatesNode->IsValueArray())
                {
                ReadPolygon(*coordinatesNode, region);
                }
            }
        else if (geometryType.CmpNoCase(_DT(L"MultiPolygon")) == 0)
            {
            if (const auto coordinatesNode = geometryNode.GetProperty(_DT(L"coordinates"));
                coordinatesNode->IsOk() && coordinatesNode->IsValueArray())
                {
                for (const auto& polygonNode : coordinatesNode->AsNodes())
                    {
                    if (polygonNode && polygonNode->IsOk() && polygonNode->IsValueArray())
                        {
                        ReadPolygon(*polygonNode, region);
                        }
                    }
                }
            }
        else if (geometryType.CmpNoCase(_DT(L"GeometryCollection")) == 0)
            {
            if (const auto geometriesNode = geometryNode.GetProperty(_DT(L"geometries"));
                geometriesNode->IsOk() && geometriesNode->IsValueArray())
                {
                for (const auto& childGeometryNode : geometriesNode->AsNodes())
                    {
                    if (childGeometryNode && childGeometryNode->IsOk())
                        {
                        ReadGeometry(*childGeometryNode, region, depth + 1);
                        }
                    }
                }
            }
        // Point, MultiPoint, LineString, and MultiLineString have no fillable area
        }

    //---------------------------------------------------
    void GeoJsonReader::ReadPolygon(wxSimpleJSON& ringsNode, GeoRegion& region) const
        {
        const auto ringNodes = ringsNode.AsNodes();
        if (ringNodes.empty())
            {
            return;
            }

        GeoPolygon polygon;
        if (ringNodes.front() && ringNodes.front()->IsOk())
            {
            polygon.m_outerBoundary = ReadRing(*ringNodes.front());
            }
        if (polygon.m_outerBoundary.empty())
            {
            return;
            }

        for (size_t ringIndex = 1; ringIndex < ringNodes.size(); ++ringIndex)
            {
            if (!ringNodes[ringIndex] || !ringNodes[ringIndex]->IsOk())
                {
                continue;
                }
            GeoLinearRing innerRing = ReadRing(*ringNodes[ringIndex]);
            if (!innerRing.empty())
                {
                polygon.m_innerBoundaries.push_back(std::move(innerRing));
                }
            }

        for (const auto& coordinate : polygon.m_outerBoundary)
            {
            polygon.m_boundingBox.Encompass(coordinate);
            }
        for (const auto& innerRing : polygon.m_innerBoundaries)
            {
            for (const auto& coordinate : innerRing)
                {
                polygon.m_boundingBox.Encompass(coordinate);
                }
            }

        region.m_boundingBox.Encompass(polygon.m_boundingBox);
        region.m_polygons.push_back(std::move(polygon));
        }

    //---------------------------------------------------
    GeoLinearRing GeoJsonReader::ReadRing(wxSimpleJSON& ringNode) const
        {
        GeoLinearRing ring;
        const auto positionNodes = ringNode.AsNodes();
        ring.reserve(std::min<size_t>(positionNodes.size(), MAX_RING_VERTICES));
        for (const auto& positionNode : positionNodes)
            {
            if (ring.size() >= MAX_RING_VERTICES)
                {
                wxLogWarning(L"GeoJSON ring has more than %zu vertices; the rest were dropped.",
                             MAX_RING_VERTICES);
                break;
                }
            if (!positionNode || !positionNode->IsOk() || !positionNode->IsValueArray())
                {
                continue;
                }
            const auto componentNodes = positionNode->AsNodes();
            if (componentNodes.size() < 2 || !componentNodes[0] || !componentNodes[1] ||
                !componentNodes[0]->IsValueNumber() || !componentNodes[1]->IsValueNumber())
                {
                continue;
                }
            GeoCoordinate coordinate;
            if (MakeGeoCoordinate(componentNodes[0]->AsDouble(), componentNodes[1]->AsDouble(),
                                  coordinate))
                {
                ring.push_back(coordinate);
                }
            }
        return ring;
        }

    //---------------------------------------------------
    void GeoJsonReader::ReadProperties(wxSimpleJSON& propertiesNode, GeoRegion& region)
        {
        for (const auto& fieldName : propertiesNode.GetObjectKeys())
            {
            const auto valueNode = propertiesNode.GetProperty(fieldName);
            if (!valueNode->IsOk())
                {
                continue;
                }
            wxString fieldValue;
            switch (valueNode->GetType())
                {
            case wxSimpleJSON::JSONType::IS_STRING:
                fieldValue = valueNode->AsString();
                break;
            case wxSimpleJSON::JSONType::IS_NUMBER:
                fieldValue = FormatJsonNumber(valueNode->AsDouble());
                break;
            case wxSimpleJSON::JSONType::IS_TRUE:
                fieldValue = _DT(L"true");
                break;
            case wxSimpleJSON::JSONType::IS_FALSE:
                fieldValue = _DT(L"false");
                break;
            default:
                // a null, a nested object, or an array is not a scalar attribute
                continue;
                }
            region.m_attributes.insert_or_assign(fieldName, fieldValue);
            }
        }

    //---------------------------------------------------
    void GeoJsonReader::AssignRegionName(wxSimpleJSON& featureNode, GeoRegion& region) const
        {
        // an explicit name field wins
        if (!m_nameField.empty())
            {
            const auto foundExplicit = region.m_attributes.find(m_nameField);
            if (foundExplicit != region.m_attributes.cend())
                {
                region.m_name = foundExplicit->second;
                return;
                }
            }

        // otherwise fall back to the keys a region label is usually stored under
        static const std::array<const wchar_t*, 9> nameKeys = {
            L"name",      L"NAME",     L"Name",  L"name_en", L"NAME_EN",
            L"NAME_LONG", L"NAMELSAD", L"admin", L"ADMIN"
        };
        for (const auto* candidateKey : nameKeys)
            {
            if (const auto foundCandidate = region.m_attributes.find(candidateKey);
                foundCandidate != region.m_attributes.cend() && !foundCandidate->second.empty())
                {
                region.m_name = foundCandidate->second;
                break;
                }
            }

        // a top-level feature "id" is a good last resort, and is worth keeping as an
        // attribute so that it can be named as the ID field of a GeoDataset
        const auto idNode = featureNode.GetProperty(_DT(L"id"));
        if (idNode->IsOk())
            {
            wxString idValue;
            if (idNode->IsValueString())
                {
                idValue = idNode->AsString();
                }
            else if (idNode->IsValueNumber())
                {
                idValue = FormatJsonNumber(idNode->AsDouble());
                }
            if (!idValue.empty())
                {
                if (region.m_name.empty())
                    {
                    region.m_name = idValue;
                    }
                region.m_attributes.emplace(_DT(L"id"), idValue);
                }
            }
        }
    } // namespace Wisteria::Data
