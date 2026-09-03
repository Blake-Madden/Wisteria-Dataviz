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

#include <limits>
#include <map>
#include <set>
#include <vector>
#include <wx/string.h>

class wxXmlNode;

namespace Wisteria::Data
    {
    /// @brief A geographic coordinate (WGS 84), as read from a KML file.
    /// @details Coordinates are stored exactly as they appear in the file;
    ///     no map projection is applied. The altitude component (if any) is discarded.
    struct GeoCoordinate
        {
        /// @brief The longitude, in decimal degrees (the @c x value, -180 to 180).
        double m_longitude{ 0.0 };
        /// @brief The latitude, in decimal degrees (the @c y value, -90 to 90).
        double m_latitude{ 0.0 };
        };

    /// @brief An axis-aligned geographic extent, in decimal degrees.
    /// @details A default-constructed box is empty (see IsOk()).
    ///     Feeding coordinates or other boxes into Encompass() grows it to fit them.
    struct GeoBoundingBox
        {
        /// @brief The western edge, in decimal degrees.
        double m_minLongitude{ std::numeric_limits<double>::max() };
        /// @brief The eastern edge, in decimal degrees.
        double m_maxLongitude{ std::numeric_limits<double>::lowest() };
        /// @brief The southern edge, in decimal degrees.
        double m_minLatitude{ std::numeric_limits<double>::max() };
        /// @brief The northern edge, in decimal degrees.
        double m_maxLatitude{ std::numeric_limits<double>::lowest() };

        /// @returns @c true if the box holds a valid, non-empty extent.
        [[nodiscard]]
        bool IsOk() const noexcept;

        /// @brief Grows the box so that it contains @c coordinate.
        /// @param coordinate The coordinate to fit inside the box.
        void Encompass(const GeoCoordinate& coordinate) noexcept;

        /// @brief Grows the box so that it contains @c box.
        /// @param box The other box to fit inside this one. An empty box is ignored.
        void Encompass(const GeoBoundingBox& box) noexcept;

        /// @returns The width of the box, in decimal degrees, or @c 0 if the box is empty.
        [[nodiscard]]
        double GetWidth() const noexcept;

        /// @returns The height of the box, in decimal degrees, or @c 0 if the box is empty.
        [[nodiscard]]
        double GetHeight() const noexcept;

        /// @returns The coordinate at the center of the box.
        /// @note The return value is meaningless if the box is empty.
        [[nodiscard]]
        GeoCoordinate GetCenter() const noexcept;
        };

    /// @brief A closed ring of coordinates (a KML @c LinearRing).
    using GeoLinearRing = std::vector<GeoCoordinate>;

    /// @brief A single polygon: one outer ring, plus zero or more inner rings (holes).
    struct GeoPolygon
        {
        /// @brief The outer boundary of the polygon.
        GeoLinearRing m_outerBoundary;
        /// @brief The inner boundaries (holes) cut out of the polygon.
        std::vector<GeoLinearRing> m_innerBoundaries;
        /// @brief The extent of every ring in the polygon.
        GeoBoundingBox m_boundingBox;
        };

    /// @brief A named region loaded from a KML @c Placemark.
    /// @details A region carries a label, any tabular attributes attached to the
    ///     placemark, and the polygon(s) that make up its shape. A placemark with a
    ///     @c MultiGeometry (e.g., a mainland plus its islands) yields several
    ///     polygons in @c m_polygons.
    struct GeoRegion
        {
        /// @brief The label of the region (the placemark's @c name).
        wxString m_name;
        /// @brief The attributes attached to the placemark, keyed by field name.
        /// @details These come from the @c SchemaData / @c SimpleData fields in the
        ///     placemark's @c ExtendedData. They are the values a choropleth is
        ///     typically colored by.
        std::map<wxString, wxString> m_attributes;
        /// @brief The polygons that make up the region's shape.
        std::vector<GeoPolygon> m_polygons;
        /// @brief The extent of every polygon in the region.
        GeoBoundingBox m_boundingBox;

        /// @brief Looks up an attribute by field name.
        /// @param fieldName The name of the field to look up.
        /// @param defaultValue The value to return if the field is not present.
        /// @returns The attribute's value, or @c defaultValue if it was not found.
        [[nodiscard]]
        wxString GetAttribute(const wxString& fieldName,
                              const wxString& defaultValue = wxString{}) const;
        };

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
    class KmlReader
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

        /// @returns @c true if the last load succeeded and produced at least one region.
        [[nodiscard]]
        bool IsOk() const noexcept
            {
            return m_lastError.empty() && !m_regions.empty();
            }

        /// @returns A description of why the last load failed, or an empty string
        ///     if it succeeded.
        [[nodiscard]]
        const wxString& GetLastError() const noexcept
            {
            return m_lastError;
            }

        /// @returns The regions read from the file.
        [[nodiscard]]
        const std::vector<GeoRegion>& GetRegions() const noexcept
            {
            return m_regions;
            }

        /// @returns The name of the KML @c Document, or an empty string if it had none.
        [[nodiscard]]
        const wxString& GetName() const noexcept
            {
            return m_name;
            }

        /// @returns The combined extent of every region that was read.
        [[nodiscard]]
        const GeoBoundingBox& GetBoundingBox() const noexcept
            {
            return m_boundingBox;
            }

        /// @brief Looks up a region by its label.
        /// @param name The label to search for (a case-sensitive, exact match).
        /// @returns A pointer to the region, or @c nullptr if no region has that label.
        [[nodiscard]]
        const GeoRegion* FindRegion(const wxString& name) const;

      private:
        // KML is untrusted input, so the reader keeps a few structural limits that a
        // malformed or hostile file cannot push past. They are well above anything a
        // real region file needs.

        // deepest folder or geometry nesting the tree walkers will descend into
        constexpr static int maxTraversalDepth{ 256 };
        // most coordinates kept for a single ring
        constexpr static size_t maxRingVertices{ 2'000'000 };
        // most placemarks turned into regions from one file
        constexpr static size_t maxRegionCount{ 500'000 };
        // largest KML file the reader will hand to the XML parser
        constexpr static wxULongLong_t maxKmlFileBytes{ 512ULL * 1024 * 1024 };

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

        std::vector<GeoRegion> m_regions;
        GeoBoundingBox m_boundingBox;
        wxString m_name;
        wxString m_lastError;
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_KML_H
