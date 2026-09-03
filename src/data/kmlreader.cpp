///////////////////////////////////////////////////////////////////////////////
// Name:        kmlreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "kmlreader.h"
#include "../../util/donttranslate.h"
#include <algorithm>
#include <cmath>
#include <wx/filename.h>
#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>
#include <wx/xml/xml.h>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    bool GeoBoundingBox::IsOk() const noexcept
        {
        return std::isfinite(m_minLongitude) && std::isfinite(m_maxLongitude) &&
               std::isfinite(m_minLatitude) && std::isfinite(m_maxLatitude) &&
               m_minLongitude <= m_maxLongitude && m_minLatitude <= m_maxLatitude;
        }

    //---------------------------------------------------
    void GeoBoundingBox::Encompass(const GeoCoordinate& coordinate) noexcept
        {
        if (!std::isfinite(coordinate.m_longitude) || !std::isfinite(coordinate.m_latitude))
            {
            return;
            }
        m_minLongitude = std::min(m_minLongitude, coordinate.m_longitude);
        m_maxLongitude = std::max(m_maxLongitude, coordinate.m_longitude);
        m_minLatitude = std::min(m_minLatitude, coordinate.m_latitude);
        m_maxLatitude = std::max(m_maxLatitude, coordinate.m_latitude);
        }

    //---------------------------------------------------
    void GeoBoundingBox::Encompass(const GeoBoundingBox& box) noexcept
        {
        if (!box.IsOk())
            {
            return;
            }
        Encompass(GeoCoordinate{ box.m_minLongitude, box.m_minLatitude });
        Encompass(GeoCoordinate{ box.m_maxLongitude, box.m_maxLatitude });
        }

    //---------------------------------------------------
    double GeoBoundingBox::GetWidth() const noexcept
        {
        return IsOk() ? (m_maxLongitude - m_minLongitude) : 0.0;
        }

    //---------------------------------------------------
    double GeoBoundingBox::GetHeight() const noexcept
        {
        return IsOk() ? (m_maxLatitude - m_minLatitude) : 0.0;
        }

    //---------------------------------------------------
    GeoCoordinate GeoBoundingBox::GetCenter() const noexcept
        {
        return GeoCoordinate{ (m_minLongitude + m_maxLongitude) / 2.0,
                              (m_minLatitude + m_maxLatitude) / 2.0 };
        }

    //---------------------------------------------------
    wxString GeoRegion::GetAttribute(const wxString& fieldName, const wxString& defaultValue) const
        {
        const auto foundAttribute = m_attributes.find(fieldName);
        return (foundAttribute != m_attributes.cend()) ? foundAttribute->second : defaultValue;
        }

    //---------------------------------------------------
    bool KmlReader::LoadFile(const wxString& filePath)
        {
        m_regions.clear();
        m_boundingBox = GeoBoundingBox{};
        m_name.clear();
        m_lastError.clear();

        if (const wxULongLong fileSize = wxFileName::GetSize(filePath);
            fileSize != wxInvalidSize && fileSize.GetValue() > maxKmlFileBytes)
            {
            m_lastError = wxString::Format(_(L"'%s': KML file is too large to read."), filePath);
            return false;
            }

        wxXmlDocument doc;
        if (!doc.Load(filePath))
            {
            m_lastError = wxString::Format(_(L"'%s': unable to read KML file."), filePath);
            return false;
            }
        return ReadDocument(doc.GetRoot());
        }

    //---------------------------------------------------
    bool KmlReader::LoadText(const wxString& kmlText)
        {
        m_regions.clear();
        m_boundingBox = GeoBoundingBox{};
        m_name.clear();
        m_lastError.clear();

        wxStringInputStream textStream(kmlText);
        wxXmlDocument doc;
        if (!doc.Load(textStream))
            {
            m_lastError = _(L"Unable to parse KML content.");
            return false;
            }
        return ReadDocument(doc.GetRoot());
        }

    //---------------------------------------------------
    std::vector<wxString> KmlReader::ReadFieldNames(const wxString& filePath)
        {
        if (const wxULongLong fileSize = wxFileName::GetSize(filePath);
            fileSize != wxInvalidSize && fileSize.GetValue() > maxKmlFileBytes)
            {
            return {};
            }

        wxXmlDocument doc;
        if (!doc.Load(filePath))
            {
            return {};
            }
        std::set<wxString> fieldNames;
        CollectFieldNames(doc.GetRoot(), fieldNames, 0);
        return { fieldNames.cbegin(), fieldNames.cend() };
        }

    //---------------------------------------------------
    void KmlReader::CollectFieldNames(const wxXmlNode* parent, std::set<wxString>& fieldNames,
                                      const int depth)
        {
        if (depth > maxTraversalDepth)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            if (childName.IsSameAs(L"SimpleField", false) ||
                childName.IsSameAs(L"SimpleData", false) || childName.IsSameAs(_DT(L"Data"), false))
                {
                const wxString fieldName = child->GetAttribute(_DT(L"name")).Strip(wxString::both);
                if (!fieldName.empty())
                    {
                    fieldNames.insert(fieldName);
                    }
                }
            CollectFieldNames(child, fieldNames, depth + 1);
            }
        }

    //---------------------------------------------------
    bool KmlReader::ReadDocument(const wxXmlNode* rootNode)
        {
        if (rootNode == nullptr)
            {
            m_lastError = _(L"KML file has no root node.");
            return false;
            }

        // the document name lives just below <Document> (or the root, for terse files)
        const wxXmlNode* documentNode = FindChildElement(rootNode, _DT(L"Document"));
        const wxXmlNode* nameParent = (documentNode != nullptr) ? documentNode : rootNode;
        if (const wxXmlNode* nameNode = FindChildElement(nameParent, _DT(L"name"));
            nameNode != nullptr)
            {
            m_name = nameNode->GetNodeContent().Strip(wxString::both);
            }

        // gather every <Placemark>, wherever it sits in the folder hierarchy
        std::vector<const wxXmlNode*> placemarkNodes;
        CollectPlacemarkNodes(rootNode, placemarkNodes, 0);

        if (placemarkNodes.size() > maxRegionCount)
            {
            wxLogWarning(L"KML file holds %zu placemarks; only the first %zu will be read.",
                         placemarkNodes.size(), maxRegionCount);
            placemarkNodes.resize(maxRegionCount);
            }

        m_regions.reserve(placemarkNodes.size());
        for (const wxXmlNode* placemarkNode : placemarkNodes)
            {
            ReadPlacemark(placemarkNode);
            }

        if (m_regions.empty())
            {
            m_lastError = _(L"No mappable regions were found in the KML file.");
            return false;
            }
        return true;
        }

    //---------------------------------------------------
    void KmlReader::ReadPlacemark(const wxXmlNode* placemarkNode)
        {
        if (placemarkNode == nullptr)
            {
            return;
            }

        GeoRegion region;
        if (const wxXmlNode* nameNode = FindChildElement(placemarkNode, _DT(L"name"));
            nameNode != nullptr)
            {
            region.m_name = nameNode->GetNodeContent().Strip(wxString::both);
            }
        ReadAttributes(placemarkNode, region);

        std::vector<const wxXmlNode*> polygonNodes;
        CollectPolygonNodes(placemarkNode, polygonNodes, 0);
        if (polygonNodes.empty())
            {
            // A placemark with only a Point or LineString has no fillable area.
            // Skip it rather than adding an empty region.
            return;
            }

        region.m_polygons.reserve(polygonNodes.size());
        for (const wxXmlNode* polygonNode : polygonNodes)
            {
            GeoPolygon polygon = ReadPolygon(polygonNode);
            if (polygon.m_outerBoundary.empty())
                {
                continue;
                }
            region.m_boundingBox.Encompass(polygon.m_boundingBox);
            region.m_polygons.push_back(std::move(polygon));
            }

        if (region.m_polygons.empty())
            {
            return;
            }
        m_boundingBox.Encompass(region.m_boundingBox);
        m_regions.push_back(std::move(region));
        }

    //---------------------------------------------------
    void KmlReader::ReadAttributes(const wxXmlNode* placemarkNode, GeoRegion& region)
        {
        const wxXmlNode* extendedDataNode = FindChildElement(placemarkNode, L"ExtendedData");
        if (extendedDataNode == nullptr)
            {
            return;
            }

        for (const wxXmlNode* child = extendedDataNode->GetChildren(); child != nullptr;
             child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            // <SchemaData><SimpleData name="FIELD">value</SimpleData></SchemaData>
            if (childName.IsSameAs(L"SchemaData", false))
                {
                for (const wxXmlNode* fieldNode = child->GetChildren(); fieldNode != nullptr;
                     fieldNode = fieldNode->GetNext())
                    {
                    if (fieldNode->GetName().AfterLast(L':').IsSameAs(L"SimpleData", false))
                        {
                        const wxString fieldName = fieldNode->GetAttribute(_DT(L"name"));
                        if (!fieldName.empty())
                            {
                            region.m_attributes.insert_or_assign(
                                fieldName, fieldNode->GetNodeContent().Strip(wxString::both));
                            }
                        }
                    }
                }
            // <Data name="FIELD"><value>value</value></Data>
            else if (childName.IsSameAs(L"Data", false))
                {
                const wxString fieldName = child->GetAttribute(_DT(L"name"));
                const wxXmlNode* valueNode = FindChildElement(child, _DT(L"value"));
                if (!fieldName.empty() && valueNode != nullptr)
                    {
                    region.m_attributes.insert_or_assign(
                        fieldName, valueNode->GetNodeContent().Strip(wxString::both));
                    }
                }
            }
        }

    //---------------------------------------------------
    GeoPolygon KmlReader::ReadPolygon(const wxXmlNode* polygonNode)
        {
        GeoPolygon polygon;
        if (polygonNode == nullptr)
            {
            return polygon;
            }

        const auto readRing = [](const wxXmlNode* boundaryNode) -> GeoLinearRing
        {
            if (boundaryNode == nullptr)
                {
                return {};
                }
            const wxXmlNode* ringNode = FindChildElement(boundaryNode, L"LinearRing");
            if (ringNode == nullptr)
                {
                return {};
                }
            const wxXmlNode* coordinatesNode = FindChildElement(ringNode, _DT(L"coordinates"));
            return (coordinatesNode != nullptr) ?
                       ParseCoordinates(coordinatesNode->GetNodeContent()) :
                       GeoLinearRing{};
        };

        for (const wxXmlNode* child = polygonNode->GetChildren(); child != nullptr;
             child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            if (childName.IsSameAs(L"outerBoundaryIs", false))
                {
                polygon.m_outerBoundary = readRing(child);
                }
            else if (childName.IsSameAs(L"innerBoundaryIs", false))
                {
                GeoLinearRing innerRing = readRing(child);
                if (!innerRing.empty())
                    {
                    polygon.m_innerBoundaries.push_back(std::move(innerRing));
                    }
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

        return polygon;
        }

    //---------------------------------------------------
    GeoLinearRing KmlReader::ParseCoordinates(const wxString& coordinateText)
        {
        GeoLinearRing ring;
        // Tuples are separated by whitespace. Each tuple is "lon,lat" or "lon,lat,alt".
        wxStringTokenizer tupleTokenizer(coordinateText, L" \t\r\n");
        while (tupleTokenizer.HasMoreTokens())
            {
            if (ring.size() >= maxRingVertices)
                {
                wxLogWarning(L"KML ring has more than %zu vertices; the rest were dropped.",
                             maxRingVertices);
                break;
                }
            const wxString tuple = tupleTokenizer.GetNextToken();
            if (tuple.empty())
                {
                continue;
                }
            const wxString longitudeStr = tuple.BeforeFirst(L',');
            const wxString latitudeStr = tuple.AfterFirst(L',').BeforeFirst(L',');

            GeoCoordinate coordinate;
            if (longitudeStr.ToCDouble(&coordinate.m_longitude) &&
                latitudeStr.ToCDouble(&coordinate.m_latitude) &&
                std::isfinite(coordinate.m_longitude) && std::isfinite(coordinate.m_latitude))
                {
                // A KML coordinate is WGS 84 decimal degrees. Hold anything outside
                // that range to the edge of the globe so a wild value cannot escape
                // the map projection later.
                coordinate.m_longitude = std::clamp(coordinate.m_longitude, -180.0, 180.0);
                coordinate.m_latitude = std::clamp(coordinate.m_latitude, -90.0, 90.0);
                ring.push_back(coordinate);
                }
            }
        return ring;
        }

    //---------------------------------------------------
    void KmlReader::CollectPlacemarkNodes(const wxXmlNode* parent,
                                          std::vector<const wxXmlNode*>& placemarkNodes,
                                          const int depth)
        {
        if (depth > maxTraversalDepth)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            if (child->GetName().AfterLast(L':').IsSameAs(_DT(L"Placemark"), false))
                {
                placemarkNodes.push_back(child);
                }
            else
                {
                CollectPlacemarkNodes(child, placemarkNodes, depth + 1);
                }
            }
        }

    //---------------------------------------------------
    void KmlReader::CollectPolygonNodes(const wxXmlNode* parent,
                                        std::vector<const wxXmlNode*>& polygonNodes,
                                        const int depth)
        {
        if (depth > maxTraversalDepth)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            if (childName.IsSameAs(_DT(L"Polygon"), false))
                {
                polygonNodes.push_back(child);
                }
            else if (childName.IsSameAs(L"MultiGeometry", false) ||
                     childName.IsSameAs(L"MultiPolygon", false))
                {
                CollectPolygonNodes(child, polygonNodes, depth + 1);
                }
            }
        }

    //---------------------------------------------------
    const wxXmlNode* KmlReader::FindChildElement(const wxXmlNode* parent, const wxString& name)
        {
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            if (child->GetName().AfterLast(L':').IsSameAs(name, false))
                {
                return child;
                }
            }
        return nullptr;
        }

    //---------------------------------------------------
    const GeoRegion* KmlReader::FindRegion(const wxString& name) const
        {
        const auto foundRegion = std::ranges::find_if(m_regions, [&name](const auto& region)
                                                      { return region.m_name == name; });
        return (foundRegion != m_regions.cend()) ? &(*foundRegion) : nullptr;
        }
    } // namespace Wisteria::Data
