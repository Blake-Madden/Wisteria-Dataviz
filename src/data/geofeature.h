/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_GEOFEATURE_H
#define WISTERIA_GEOFEATURE_H

#include <limits>
#include <map>
#include <optional>
#include <vector>
#include <wx/string.h>

namespace Wisteria::Data
    {
    /// @brief A geographic coordinate (WGS 84), as read from a region file.
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

    /// @brief A closed ring of coordinates.
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

    /// @brief A named region loaded from a placemark (KML) or feature (GeoJSON).
    /// @details A region carries a label, any tabular attributes attached to the
    ///     placemark or feature, and the polygon(s) that make up its shape. A source
    ///     with multi-part geometry (e.g., a mainland plus its islands) yields several
    ///     polygons in @c m_polygons.
    struct GeoRegion
        {
        /// @brief The label of the region (a KML placemark's @c name, or a GeoJSON
        ///     feature property).
        wxString m_name;
        /// @brief The attributes attached to the region, keyed by field name.
        /// @details For KML these come from the @c SchemaData / @c SimpleData fields in
        ///     the placemark's @c ExtendedData. For GeoJSON they are the members of the
        ///     feature's @c properties object. They are the values a choropleth is
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

    /// @brief Builds a GeoCoordinate from a longitude/latitude pair, holding each
    ///     component to the valid WGS 84 range.
    /// @details A value outside the range is clamped to the edge of the globe so a
    ///     wild coordinate cannot escape the map projection later.
    /// @param longitude The longitude, in decimal degrees.
    /// @param latitude The latitude, in decimal degrees.
    /// @param[out] coordinateOut The coordinate to fill in.
    /// @returns @c true if both components were finite (and @p coordinateOut was set),
    ///     @c false otherwise (and @p coordinateOut is left untouched).
    [[nodiscard]]
    bool MakeGeoCoordinate(double longitude, double latitude,
                           GeoCoordinate& coordinateOut) noexcept;

    /// @brief The parsed regions of a KML or GeoJSON file, plus the load status.
    /// @details KmlReader and GeoJsonReader derive from this and only add the parsing.
    ///     The region list, bounding box, name, and error string, and every query over
    ///     them, live here so GeoDataset can be filled from either reader without
    ///     caring which format the regions came from.
    class GeoFeatureReader
        {
      public:
        /// @private
        virtual ~GeoFeatureReader() = default;

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

        /// @returns The name of the region collection, or an empty string if it had none.
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
        /// @returns The index of the region within GetRegions(), or @c std::nullopt
        ///     if no region has that label.
        [[nodiscard]]
        std::optional<size_t> FindRegion(const wxString& name) const;

      protected:
        /// @private
        GeoFeatureReader() = default;
        /// @private
        GeoFeatureReader(const GeoFeatureReader&) = default;
        /// @private
        GeoFeatureReader(GeoFeatureReader&&) = default;
        /// @private
        GeoFeatureReader& operator=(const GeoFeatureReader&) = default;
        /// @private
        GeoFeatureReader& operator=(GeoFeatureReader&&) = default;

        /// @brief Clears the region list and load status, ready for a fresh parse.
        void ResetFeatures() noexcept
            {
            m_regions.clear();
            m_boundingBox = GeoBoundingBox{};
            m_name.clear();
            m_lastError.clear();
            }

        /// @brief The regions read from the file.
        std::vector<GeoRegion> m_regions;
        /// @brief The combined extent of every region that was read.
        GeoBoundingBox m_boundingBox;
        /// @brief The name of the region collection.
        wxString m_name;
        /// @brief A description of why the last import failed, or empty if it succeeded.
        wxString m_lastError;
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_GEOFEATURE_H
