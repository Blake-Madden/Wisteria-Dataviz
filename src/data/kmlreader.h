/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_KML_H
#define WISTERIA_KML_H

#include "geofeature.h"
#include <set>
#include <vector>
#include <wx/string.h>

class wxXmlNode;

namespace Wisteria::Data
    {
    /// @brief Reads region geometry and labels from a KML file.
    /// @details KML is an XML dialect, so this is a thin reader over @c wxXmlDocument.
    ///     Only the parts needed to draw filled regions are kept. Each @c Placemark
    ///     becomes a GeoRegion holding its label, its @c ExtendedData attributes, and
    ///     its polygon rings. @c Point and @c LineString geometry is skipped.
    /// @code
    ///     Wisteria::Data::KmlReader kml{ L"ohio-counties.kml" };
    ///     if (kml.IsOk())
    ///         {
    ///         for (const auto& region : kml.GetRegions())
    ///             {
    ///             // region.m_name, region.m_attributes, region.m_polygons ...
    ///             }
    ///         }
    /// @endcode
    class KmlReader final : public GeoFeatureReader
        {
      public:
        /// @brief Constructor.
        KmlReader() = default;

        /// @brief Constructor which loads a KML file.
        /// @param filePath The path to the KML file to load.
        /// @note Call IsOk() to see whether the load succeeded.
        explicit KmlReader(const wxString& filePath) { LoadFile(filePath); }

        /// @brief Loads a KML file, replacing any regions already read.
        /// @param filePath The path to the KML file to load.
        /// @returns @c true on success. On failure, GetLastError() explains why.
        bool LoadFile(const wxString& filePath);

        /// @brief Parses KML content held in a string, replacing any regions already read.
        /// @param kmlText The KML markup to parse.
        /// @returns @c true on success. On failure, GetLastError() explains why.
        bool LoadText(const wxString& kmlText);

        /// @brief Reads just the attribute field names from a KML file.
        /// @param filePath The path to the KML file.
        /// @returns The attribute field names, sorted and de-duplicated. Empty if the
        ///     file cannot be read or declares no attribute fields.
        [[nodiscard]]
        static std::vector<wxString> ReadFieldNames(const wxString& filePath);

      private:
        // KML is untrusted input, so the reader keeps a few structural limits that a
        // malformed or hostile file cannot push past. They are well above anything a
        // real region file needs.

        // deepest folder or geometry nesting the tree walkers will descend into
        constexpr static int MAX_TRAVERSAL_DEPTH{ 256 };
        // most coordinates kept for a single ring
        constexpr static size_t MAX_RING_VERTICES{ 2'000'000 };
        // most placemarks turned into regions from one file
        constexpr static size_t MAX_REGION_COUNT{ 500'000 };
        // largest KML file the reader will hand to the XML parser
        constexpr static wxULongLong_t MAX_KML_FILE_BYTES{ 256ULL * 1024 * 1024 };

        /// @brief Walks a parsed KML document and fills in the region list.
        /// @param rootNode The document's root (@c kml) node.
        /// @returns @c true on success.
        bool ReadDocument(const wxXmlNode* rootNode);

        /// @brief Reads a single @c Placemark node into a region and appends it.
        /// @param placemarkNode The @c Placemark node to read.
        void ReadPlacemark(const wxXmlNode* placemarkNode);

        /// @brief Reads the @c SchemaData / @c SimpleData attributes of a placemark.
        /// @param placemarkNode The @c Placemark node to read.
        /// @param[out] region The region to add the attributes to.
        static void ReadAttributes(const wxXmlNode* placemarkNode, GeoRegion& region);

        /// @brief Reads one @c Polygon node (its outer ring and any holes).
        /// @param polygonNode The @c Polygon node to read.
        /// @returns The polygon, with its bounding box computed.
        static GeoPolygon ReadPolygon(const wxXmlNode* polygonNode);

        /// @brief Parses a KML @c coordinates string into a ring.
        /// @param coordinateText Whitespace-separated @c lon,lat,alt tuples.
        /// @returns The parsed coordinates.
        static GeoLinearRing ParseCoordinates(const wxString& coordinateText);

        /// @brief Collects every descendant @c Placemark node under @c parent.
        /// @param parent The node to search below.
        /// @param[out] placemarkNodes The list to append matching nodes to.
        /// @param depth The current nesting level. Descent stops past a fixed limit,
        ///     so a pathologically deep document cannot overflow the stack.
        static void CollectPlacemarkNodes(const wxXmlNode* parent,
                                          std::vector<const wxXmlNode*>& placemarkNodes, int depth);

        /// @brief Collects every descendant @c Polygon node under @c parent.
        /// @details This flattens a bare @c Polygon, a @c MultiGeometry, and any
        ///     nesting of the two into a single list.
        /// @param parent The node to search below.
        /// @param[out] polygonNodes The list to append matching nodes to.
        /// @param depth The current nesting level. Descent stops past a fixed limit.
        static void CollectPolygonNodes(const wxXmlNode* parent,
                                        std::vector<const wxXmlNode*>& polygonNodes, int depth);

        /// @brief Finds the first direct child of @c parent with a given name.
        /// @param parent The node whose children to scan.
        /// @param name The element name to match.
        /// @returns The child node, or @c nullptr if @c parent has no such child.
        static const wxXmlNode* FindChildElement(const wxXmlNode* parent, const wxString& name);

        /// @brief Recursively collects the @c name attribute of every attribute-field
        ///     element (@c SimpleField, @c SimpleData, or @c Data) below @c parent.
        /// @param parent The node to search below.
        /// @param[out] fieldNames The set to add field names to.
        /// @param depth The current nesting level. Descent stops past a fixed limit.
        static void CollectFieldNames(const wxXmlNode* parent, std::set<wxString>& fieldNames,
                                      int depth);
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_KML_H
