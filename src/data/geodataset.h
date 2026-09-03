/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_GEODATASET_H
#define WISTERIA_GEODATASET_H

#include "dataset.h"
#include "kmlreader.h"
#include <optional>
#include <utility>
#include <vector>
#include <wx/string.h>

namespace Wisteria::Data
    {
    /// @brief Options controlling how a KML file is turned into a GeoDataset.
    /// @sa GeoDataset::ImportKML().
    class GeoImportInfo
        {
      public:
        /** @brief Sets which @c ExtendedData field supplies the dataset's ID column.
            @details The ID column is what a later join (or CopyContinuousColumnFrom())
                matches on, so this is usually a stable key such as @c "GEOID".
                If empty, the placemark's @c name is used as the ID instead.
            @param fieldName The field name.
            @returns A self reference.*/
        GeoImportInfo& IdField(wxString fieldName)
            {
            m_idField = std::move(fieldName);
            return *this;
            }

        /** @brief Forces the named @c ExtendedData fields to be imported as continuous columns.
            @param fieldNames The field names.
            @returns A self reference.*/
        GeoImportInfo& ContinuousFields(std::vector<wxString> fieldNames)
            {
            m_continuousFields = std::move(fieldNames);
            return *this;
            }

        /** @brief Forces the named @c ExtendedData fields to be imported as categorical columns.
            @param fieldNames The field names.
            @returns A self reference.*/
        GeoImportInfo& CategoricalFields(std::vector<wxString> fieldNames)
            {
            m_categoricalFields = std::move(fieldNames);
            return *this;
            }

        /** @brief Whether to add @c "Centroid Longitude" and @c "Centroid Latitude"
                continuous columns containing each region's bounding-box center.
            @param includeCentroids @c true to add the columns.
            @returns A self reference.*/
        GeoImportInfo& ImportCentroids(const bool includeCentroids)
            {
            m_importCentroids = includeCentroids;
            return *this;
            }

        /// @private
        [[nodiscard]]
        const wxString& GetIdField() const noexcept
            {
            return m_idField;
            }

        /// @private
        [[nodiscard]]
        const std::vector<wxString>& GetContinuousFields() const noexcept
            {
            return m_continuousFields;
            }

        /// @private
        [[nodiscard]]
        const std::vector<wxString>& GetCategoricalFields() const noexcept
            {
            return m_categoricalFields;
            }

        /// @private
        [[nodiscard]]
        bool IsImportingCentroids() const noexcept
            {
            return m_importCentroids;
            }

      private:
        wxString m_idField;
        std::vector<wxString> m_continuousFields;
        std::vector<wxString> m_categoricalFields;
        bool m_importCentroids{ true };
        };

    /// @brief A dataset with one row per geographic region, plus the polygon
    ///     geometry needed to draw each region.
    /// @details This is the input to a choropleth (shaded-region) map. The tabular
    ///     side is an ordinary Dataset. The ID column holds a region key, and the
    ///     KML @c ExtendedData fields become categorical or continuous columns. The
    ///     geometry side is a parallel array of GeoRegion objects, one per row and
    ///     in the same order, reachable through GetRegionGeometry().
    ///
    ///     The value a map is shaded by is rarely in the KML itself. Add it either
    ///     with the normal Dataset column API, or by pulling it from another dataset
    ///     with CopyContinuousColumnFrom(), matched on the ID column.
    /// @code
    ///     auto geoData = std::make_shared<GeoDataset>();
    ///     geoData->ImportKML(L"ohio-counties.kml",
    ///                        GeoImportInfo().IdField(L"GEOID"));
    ///
    ///     // bring in a metric to shade by, matched on GEOID
    ///     geoData->CopyContinuousColumnFrom(*scoresData, L"county_id", L"grad_rate",
    ///                                       L"Graduation Rate");
    ///
    ///     for (size_t row = 0; row < geoData->GetRowCount(); ++row)
    ///         {
    ///         const auto& geometry = geoData->GetRegionGeometry(row);
    ///         // shade geometry.m_polygons using the "Graduation Rate" column value
    ///         }
    /// @endcode
    class GeoDataset final : public Dataset
        {
      public:
        /// @brief Constructor.
        GeoDataset() = default;

        /** @brief Reads a KML file into this dataset, replacing any existing content.
            @param filePath The path to the KML file to load.
            @param info Options controlling the ID column and column typing.
            @returns @c true on success. On failure, GetLastError() explains why and
                the dataset is left empty.*/
        bool ImportKML(const wxString& filePath, const GeoImportInfo& info = GeoImportInfo{});

        /** @brief Reads KML content held in a string into this dataset, replacing any
                existing content.
            @param kmlText The KML markup to parse.
            @param info Options controlling the ID column and column typing.
            @returns @c true on success. On failure, GetLastError() explains why.*/
        bool ImportRegionsFromText(const wxString& kmlText,
                                   const GeoImportInfo& info = GeoImportInfo{});

        /** @brief Builds this dataset from regions that have already been parsed.
            @param reader A KmlReader holding the regions to import.
            @param info Options controlling the ID column and column typing.
            @returns @c true on success.*/
        bool ImportRegions(const KmlReader& reader, const GeoImportInfo& info = GeoImportInfo{});

        /// @returns A description of why the last import failed, or an empty string
        ///     if it succeeded.
        [[nodiscard]]
        const wxString& GetLastError() const noexcept
            {
            return m_lastError;
            }

        /// @returns The geometry for the region on a given row.
        /// @param row The row index (parallel to GetRowCount()).
        [[nodiscard]]
        const GeoRegion& GetRegionGeometry(const size_t row) const
            {
            wxASSERT_MSG(row < m_geometries.size(), L"Invalid row in call to GetRegionGeometry()!");
            return m_geometries.at(row);
            }

        /// @returns The geometry for every region, in row order.
        [[nodiscard]]
        const std::vector<GeoRegion>& GetGeometries() const noexcept
            {
            return m_geometries;
            }

        /// @returns The combined extent of every region in the dataset.
        [[nodiscard]]
        const GeoBoundingBox& GetGeoBoundingBox() const noexcept
            {
            return m_boundingBox;
            }

        /// @brief Finds the row whose ID column matches @c idValue.
        /// @param idValue The ID value to search for (case-sensitive, exact match).
        /// @returns The row index, or @c std::nullopt if no row has that ID.
        [[nodiscard]]
        std::optional<size_t> FindRegionRow(const wxString& idValue) const;

        /** @brief Copies a continuous column from another dataset, matched on this
                dataset's ID column.
            @details This is the usual way to attach a metric to shade the map by.
                Rows with no match are left as missing data (NaN).
            @param source The dataset to copy values from.
            @param sourceKeyColumn The name of the column in @c source to match against
                this dataset's ID column. May be @c source's ID column or one of its
                categorical columns.
            @param sourceValueColumn The name of the continuous column in @c source
                to copy.
            @param targetColumnName The name for the new column in this dataset. If
                empty, @c sourceValueColumn is used.
            @returns @c true on success. Returns @c false (and adds nothing) if any of
                the named columns cannot be found.*/
        bool CopyContinuousColumnFrom(const Dataset& source, const wxString& sourceKeyColumn,
                                      const wxString& sourceValueColumn,
                                      const wxString& targetColumnName = wxString{});

        /** @brief Copies a categorical column from another dataset, matched on this
                dataset's ID column.
            @details Use this to attach a category to shade the map by.
                The source dataset has one row per region keyed by the same identifier
                used for this dataset's ID column.
                Rows with no match are left as missing data.
            @param source The dataset to copy values from.
            @param sourceKeyColumn The name of the column in @c source to match against
                this dataset's ID column. May be @c source's ID column or one of its
                categorical columns.
            @param sourceLabelColumn The name of the categorical column in @c source
                to copy.
            @param targetColumnName The name for the new column in this dataset. If
                empty, @c sourceLabelColumn is used.
            @returns @c true on success. Returns @c false (and adds nothing) if any of
                the named columns cannot be found.*/
        bool CopyCategoricalColumnFrom(const Dataset& source, const wxString& sourceKeyColumn,
                                       const wxString& sourceLabelColumn,
                                       const wxString& targetColumnName = wxString{});

      private:
        /// @brief Decides whether an attribute field should be a continuous column.
        /// @param fieldName The field name.
        /// @param reader The reader holding the regions to inspect.
        /// @param info The import options.
        /// @returns @c true if the field should be continuous, @c false for categorical.
        static bool IsContinuousField(const wxString& fieldName, const KmlReader& reader,
                                      const GeoImportInfo& info);

        std::vector<GeoRegion> m_geometries;
        GeoBoundingBox m_boundingBox;
        wxString m_lastError;
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_GEODATASET_H
