///////////////////////////////////////////////////////////////////////////////
// Name:        kmlreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "kmlreader.h"
#include "../../util/donttranslate.h"
#include "../../util/fileutil.h"
#include <utility>
#include <wx/log.h>
#include <wx/sstream.h>
#include <wx/tokenzr.h>
#include <wx/translation.h>
#include <wx/xml/xml.h>

namespace Wisteria::Data
    {
    //---------------------------------------------------
    bool KmlReader::LoadFile(const wxString& filePath)
        {
        ResetFeatures();

        switch (CheckFileSizeLimit(filePath, MAX_KML_FILE_BYTES))
            {
        case FileSizeCheckResult::TooLarge:
            m_lastError = wxString::Format(_(L"'%s': KML file is too large to read."), filePath);
            return false;
        case FileSizeCheckResult::Unreadable:
            m_lastError = wxString::Format(_(L"'%s': unable to read KML file."), filePath);
            return false;
        case FileSizeCheckResult::WithinLimit:
            break;
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
        ResetFeatures();

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
        if (CheckFileSizeLimit(filePath, MAX_KML_FILE_BYTES) != FileSizeCheckResult::WithinLimit)
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
        if (depth > MAX_TRAVERSAL_DEPTH)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            if (childName.CmpNoCase(L"SimpleField") == 0 ||
                childName.CmpNoCase(L"SimpleData") == 0 || childName.CmpNoCase(_DT(L"Data")) == 0)
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

        if (placemarkNodes.size() > MAX_REGION_COUNT)
            {
            wxLogWarning(L"KML file holds %zu placemarks; only the first %zu will be read.",
                         placemarkNodes.size(), MAX_REGION_COUNT);
            placemarkNodes.resize(MAX_REGION_COUNT);
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
            if (childName.CmpNoCase(L"SchemaData") == 0)
                {
                for (const wxXmlNode* fieldNode = child->GetChildren(); fieldNode != nullptr;
                     fieldNode = fieldNode->GetNext())
                    {
                    if (fieldNode->GetName().AfterLast(L':').CmpNoCase(L"SimpleData") == 0)
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
            else if (childName.CmpNoCase(L"Data") == 0)
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
            if (childName.CmpNoCase(L"outerBoundaryIs") == 0)
                {
                polygon.m_outerBoundary = readRing(child);
                }
            else if (childName.CmpNoCase(L"innerBoundaryIs") == 0)
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
            if (ring.size() >= MAX_RING_VERTICES)
                {
                wxLogWarning(L"KML ring has more than %zu vertices; the rest were dropped.",
                             MAX_RING_VERTICES);
                break;
                }
            const wxString tuple = tupleTokenizer.GetNextToken();
            if (tuple.empty())
                {
                continue;
                }
            const wxString longitudeStr = tuple.BeforeFirst(L',');
            const wxString latitudeStr = tuple.AfterFirst(L',').BeforeFirst(L',');

            double longitude{ 0.0 };
            double latitude{ 0.0 };
            GeoCoordinate coordinate;
            if (longitudeStr.ToCDouble(&longitude) && latitudeStr.ToCDouble(&latitude) &&
                MakeGeoCoordinate(longitude, latitude, coordinate))
                {
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
        if (depth > MAX_TRAVERSAL_DEPTH)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            if (child->GetName().AfterLast(L':').CmpNoCase(_DT(L"Placemark")) == 0)
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
        if (depth > MAX_TRAVERSAL_DEPTH)
            {
            return;
            }
        for (const wxXmlNode* child = (parent != nullptr) ? parent->GetChildren() : nullptr;
             child != nullptr; child = child->GetNext())
            {
            const wxString childName = child->GetName().AfterLast(L':');
            if (childName.CmpNoCase(_DT(L"Polygon")) == 0)
                {
                polygonNodes.push_back(child);
                }
            else if (childName.CmpNoCase(L"MultiGeometry") == 0 ||
                     childName.CmpNoCase(L"MultiPolygon") == 0)
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
            if (child->GetName().AfterLast(L':').CmpNoCase(name) == 0)
                {
                return child;
                }
            }
        return nullptr;
        }
    } // namespace Wisteria::Data
