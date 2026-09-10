/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_GEOJSON_H
#define WISTERIA_GEOJSON_H

#include "geofeature.h"
#include <array>
#include <string_view>
#include <utility>
#include <vector>
#include <wx/string.h>

// forward declaration, so the header does not pull in the JSON library
class wxSimpleJSON;

namespace Wisteria::Data
    {
    /// @brief Reads region geometry and labels from a GeoJSON file (RFC 7946).
    /// @details Coordinates are read as WGS 84 longitude/latitude, the order GeoJSON
    ///     uses. A @c FeatureCollection, a bare @c Feature, and a bare geometry object
    ///     are all accepted. Each @c Feature becomes a GeoRegion holding its label
    ///     (taken from a @c properties member), the rest of its @c properties as
    ///     attributes, and its polygon rings. @c Polygon and @c MultiPolygon geometry
    ///     is kept. @c Point, @c LineString, and their multi-part forms are skipped
    ///     because they have no fillable area. A @c GeometryCollection is flattened.
    /// @code
    ///     Wisteria::Data::GeoJsonReader geojson{ L"ohio-counties.geojson" };
    ///     if (geojson.IsOk())
    ///         {
    ///         for (const auto& region : geojson.GetRegions())
    ///             {
    ///             // region.m_name, region.m_attributes, region.m_polygons ...
    ///             }
    ///         }
    /// @endcode
    class GeoJsonReader final : public GeoFeatureReader
        {
      public:
        /// @brief Constructor.
        GeoJsonReader() = default;

        /// @brief Constructor which loads a GeoJSON file.
        /// @param filePath The path to the GeoJSON file to load.
        /// @note Call IsOk() to see whether the load succeeded.
        explicit GeoJsonReader(const wxString& filePath) { LoadFile(filePath); }

        /// @brief Sets which feature @c properties member supplies a region's label.
        /// @param fieldName The property name, or an empty string to fall back to a
        ///     set of common name keys (e.g., @c "name", @c "NAME", @c "NAMELSAD").
        /// @note Call this before LoadFile() or LoadText().
        void SetNameField(wxString fieldName) { m_nameField = std::move(fieldName); }

        /// @returns The property name used for a region's label, or an empty string
        ///     if the common name keys are being tried.
        [[nodiscard]]
        const wxString& GetNameField() const noexcept
            {
            return m_nameField;
            }

        /// @brief Loads a GeoJSON file, replacing any regions already read.
        /// @param filePath The path to the GeoJSON file to load.
        /// @returns @c true on success. On failure, GetLastError() explains why.
        bool LoadFile(const wxString& filePath);

        /// @brief Parses GeoJSON content held in a string, replacing any regions
        ///     already read.
        /// @param geoJsonText The GeoJSON text to parse.
        /// @returns @c true on success. On failure, GetLastError() explains why.
        bool LoadText(const wxString& geoJsonText);

        /// @brief Reads just the feature-property field names from a GeoJSON file.
        /// @param filePath The path to the GeoJSON file.
        /// @returns The property field names, sorted and de-duplicated. Empty if the
        ///     file cannot be read or its features carry no properties.
        [[nodiscard]]
        static std::vector<wxString> ReadFieldNames(const wxString& filePath);

        /// @brief Checks whether a field name is one of the feature properties a
        ///     region label is looked for under when no explicit name field is set.
        /// @param fieldName The field name to test.
        /// @returns @c true if the field is one of the common label properties.
        [[nodiscard]]
        static bool IsCommonNameField(const wxString& fieldName);

      private:
        // the feature properties a region label is looked for under, in priority order,
        // when SetNameField() was not called
        constexpr static std::array<std::wstring_view, 9> COMMON_NAME_FIELDS = {
            L"name",      L"NAME",     L"Name",  L"name_en", L"NAME_EN",
            L"NAME_LONG", L"NAMELSAD", L"admin", L"ADMIN"
        };

        // GeoJSON is untrusted input, so the reader keeps a few structural limits that
        // a malformed or hostile file cannot push past. They are well above anything a
        // real region file needs.

        // deepest GeometryCollection nesting the geometry walker will descend into
        constexpr static int MAX_TRAVERSAL_DEPTH{ 256 };
        // most coordinates kept for a single ring
        constexpr static size_t MAX_RING_VERTICES{ 2'000'000 };
        // most features turned into regions from one file
        constexpr static size_t MAX_REGION_COUNT{ 500'000 };
        // largest GeoJSON file the reader will hand to the JSON parser
        constexpr static wxULongLong_t MAX_GEOJSON_FILE_BYTES{ 256ULL * 1024 * 1024 };

        /// @brief Walks a parsed GeoJSON document and fills in the region list.
        /// @param rootNode The document's root node.
        /// @returns @c true on success.
        bool ReadDocument(wxSimpleJSON& rootNode);

        /// @brief Reads a single @c Feature node into a region and appends it.
        /// @param featureNode The @c Feature node to read.
        void ReadFeature(wxSimpleJSON& featureNode);

        /// @brief Reads a bare geometry object (no enclosing @c Feature) into a region
        ///     and appends it.
        /// @param geometryNode The geometry node to read.
        void ReadBareGeometry(wxSimpleJSON& geometryNode);

        /// @brief Adds the polygons of a geometry node to a region.
        /// @param geometryNode The geometry node (@c Polygon, @c MultiPolygon, or
        ///     @c GeometryCollection).
        /// @param[in,out] region The region to add the polygons to.
        /// @param depth The current @c GeometryCollection nesting level.
        void ReadGeometry(wxSimpleJSON& geometryNode, GeoRegion& region, int depth) const;

        /// @brief Reads one polygon (an array of rings, the first being the outer ring)
        ///     and adds it to a region.
        /// @param ringsNode The array-of-rings node.
        /// @param[in,out] region The region to add the polygon to.
        void ReadPolygon(wxSimpleJSON& ringsNode, GeoRegion& region) const;

        /// @brief Parses one ring (an array of @c [lon,lat] positions).
        /// @param ringNode The array-of-positions node.
        /// @returns The parsed coordinates.
        GeoLinearRing ReadRing(wxSimpleJSON& ringNode) const;

        /// @brief Copies a feature's @c properties members into a region's attributes.
        /// @param propertiesNode The @c properties node.
        /// @param[in,out] region The region to fill.
        static void ReadProperties(wxSimpleJSON& propertiesNode, GeoRegion& region);

        /// @brief Chooses a region's label from its attributes (and the feature @c id).
        /// @param featureNode The @c Feature node, for its top-level @c id.
        /// @param[in,out] region The region whose attributes are already filled.
        void AssignRegionName(wxSimpleJSON& featureNode, GeoRegion& region) const;

        wxString m_nameField;
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_GEOJSON_H
