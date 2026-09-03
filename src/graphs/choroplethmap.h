/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_CHOROPLETH_MAP_H
#define WISTERIA_CHOROPLETH_MAP_H

#include "../data/geodataset.h"
#include "graph2d.h"
#include <map>
#include <utility>
#include <vector>
#include <wx/brush.h>

namespace Wisteria::Graphs
    {
    /// @private
    /// @brief One region of a ChoroplethMap. Its screen-space rings, drawn as a
    ///     filled outline and hit-tested with a robust point-in-polygon test.
    /// @details A plain GraphItems::Polygon is not used because its hit test treats
    ///     borders as "inside" and uses integer math, which produces false positives
    ///     when hundreds of dense adjacent polygons are on the canvas.
    class ChoroplethRegion final : public GraphItems::GraphItemBase
        {
      public:
        /// @brief Constructor.
        /// @param itemInfo Base information (pen, brush, text, selectability).
        explicit ChoroplethRegion(GraphItems::GraphItemInfo itemInfo)
            : GraphItemBase(std::move(itemInfo))
            {
            }

        /// @brief Adds a filled outer ring (screen coordinates).
        /// @param ring The ring's points.
        void AddOuterRing(std::vector<wxPoint> ring);

        /// @brief Adds a hole ring (screen coordinates) cut out of the region.
        /// @param ring The ring's points.
        void AddHoleRing(std::vector<wxPoint> ring) { m_holeRings.push_back(std::move(ring)); }

        /// @returns @c true if the region has at least one drawable outer ring.
        [[nodiscard]]
        bool HasRings() const noexcept
            {
            return !m_outerRings.empty();
            }

      private:
        wxRect Draw(wxDC& dc) const final;
        // draws the region's label on selection, a step smaller than the default
        // selection label
        void DrawSelectionLabel(wxDC& dc, double scaling, wxRect boundingBox) const final;
        [[nodiscard]]
        bool HitTest(wxPoint pt, wxDC& dc) const final;

        [[nodiscard]]
        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            {
            return m_boundingBox;
            }

        void SetBoundingBox([[maybe_unused]] const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] double parentScaling) final
            {
            }

        void Offset(int xToMove, int yToMove) final;

        std::vector<std::vector<wxPoint>> m_outerRings;
        std::vector<std::vector<wxPoint>> m_holeRings;
        wxRect m_boundingBox;
        };

    /// @private
    /// @brief The legend for a ChoroplethMap. The value scale (a color ramp or
    ///     category swatches) and, beneath it, a graduated-circle size key for the
    ///     proportional layer.
    class ChoroplethLegend final : public GraphItems::GraphItemBase
        {
        wxDECLARE_DYNAMIC_CLASS(ChoroplethLegend);

      public:
        /// @private
        ChoroplethLegend() = default;

        /// @brief Constructor.
        /// @param itemInfo Base information (scaling, DPI scaling).
        explicit ChoroplethLegend(const GraphItems::GraphItemInfo& itemInfo)
            : GraphItemBase(itemInfo)
            {
            }

        /// @private
        ChoroplethLegend(const ChoroplethLegend&) = delete;
        /// @private
        ChoroplethLegend& operator=(const ChoroplethLegend&) = delete;

        /// @brief One ring of the size key.
        struct Entry
            {
            /// @brief The value the ring stands for.
            double m_value{ 0.0 };
            /// @brief The value formatted for display.
            wxString m_label;
            };

        /// @brief Sets the value-scale legend shown above the size key.
        /// @param scaleLegend The color-ramp or swatch legend, or @c nullptr for none.
        void SetScaleLegend(std::unique_ptr<GraphItems::Label> scaleLegend)
            {
            m_scaleLegend = std::move(scaleLegend);
            if (m_scaleLegend != nullptr)
                {
                m_scaleLegend->SetScaling(GetScaling());
                m_scaleLegend->SetDPIScaleFactor(GetDPIScaleFactor());
                }
            }

        /// @brief Sets the size-key rings, largest value first.
        /// @param entries The rings.
        void SetEntries(std::vector<Entry> entries) { m_entries = std::move(entries); }

        /// @brief Sets the heading drawn above the size key.
        /// @param heading The heading text, usually the symbol column name.
        void SetHeading(wxString heading) { m_heading = std::move(heading); }

        /// @brief Sets the ring outline color.
        /// @param color The outline color.
        void SetOutlineColor(const wxColour& color) { m_outlineColor = color; }

        /// @brief Sets the color of the size-key heading and labels.
        /// @param color The text color.
        void SetTextColor(const wxColour& color) { m_textColor = color; }

        /// @brief Sets the scaling, propagating it to the embedded scale legend.
        /// @param scaling The scaling to use.
        void SetScaling(const double scaling) final
            {
            GraphItemBase::SetScaling(scaling);
            if (m_scaleLegend != nullptr)
                {
                m_scaleLegend->SetScaling(scaling);
                }
            }

        /// @brief Sets the DPI scale factor, propagating it to the embedded scale legend.
        /// @param scaling The DPI scale factor.
        void SetDPIScaleFactor(const double scaling) noexcept final
            {
            GraphItemBase::SetDPIScaleFactor(scaling);
            if (m_scaleLegend != nullptr)
                {
                m_scaleLegend->SetDPIScaleFactor(scaling);
                }
            }

      private:
        wxRect Draw(wxDC& dc) const final;
        void RecalcSizes(wxDC& dc) final;

        [[nodiscard]]
        wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            {
            return m_rect;
            }

        void SetBoundingBox(const wxRect& rect, [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] double parentScaling) final
            {
            m_rect = rect;
            }

        [[nodiscard]]
        bool HitTest(wxPoint pt, [[maybe_unused]] wxDC& dc) const final
            {
            return m_rect.Contains(pt);
            }

        void Offset(const int xToMove, const int yToMove) final { m_rect.Offset(xToMove, yToMove); }

        /// @returns The screen radius for a ring standing for @p value.
        [[nodiscard]]
        double RingRadius(double value) const;

        /// @brief Measures, and optionally draws, the size key with its top-left at @p origin.
        /// @param dc The device context.
        /// @param origin The top-left corner of the key.
        /// @param draw @c true to render, @c false to only measure.
        /// @returns The size the key occupies.
        wxSize LayOutKey(wxDC& dc, const wxPoint& origin, bool draw) const;

        std::unique_ptr<GraphItems::Label> m_scaleLegend;
        std::vector<Entry> m_entries;
        wxString m_heading;
        wxColour m_outlineColor{ *wxBLACK };
        wxColour m_textColor{ *wxBLACK };
        wxRect m_rect;
        };

    /** @brief A map that shades geographic regions by a data value.
        @details The regions and their labels come from a Data::GeoDataset (built from a
            KML file). An optional continuous column supplies the value each region is
            shaded by; without one, every region is filled with a single color.

        @par %Data:
         This graph accepts a Data::GeoDataset. One row per region, with the polygon
         geometry carried alongside the rows. The column named in SetData() is mapped
         onto the graph's color scheme, from the scheme's first color (the minimum
         value) to its last color (the maximum). Regions whose value is missing are
         filled with GetNoDataColor().

        @par Missing Data:
         - A region with a missing value is filled with the "no data" color.
           Its fill style can be switched to a hatch pattern with SetNoDataFillStyle().
         - A region with no usable polygon is skipped.

        @par Projection:
         The map is projected before it is drawn. By default, the projection is chosen
         from the extent of the data. An equal-area conic for a country or continent,
         an equal-area pseudocylindrical for hemispheric or global data,
         and a latitude-corrected equirectangular for a small area.
         The projection can also be set explicitly with SetMapProjection().

        @par Proportional Symbols:
         A second continuous column can be shown as a circle at each region's center,
         its area proportional to the value.*/
    class ChoroplethMap final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(ChoroplethMap);
        ChoroplethMap() = default;

      public:
        /// @brief How longitude/latitude are mapped to the plot area.
        enum class MapProjection
            {
            /// @brief Pick a projection from the extent of the data.
            Automatic,
            /// @brief Equirectangular, with longitude scaled by the cosine of the center
            ///     latitude. Cheap and accurate for a small area; distorts over a
            ///     continent.
            Equirectangular,
            /// @brief Albers equal-area conic, with standard parallels taken from the
            ///     data extent. The usual choice for a mid-latitude country or continent.
            AlbersEqualAreaConic,
            /// @brief The Equal Earth projection (Savric, Patterson, Jenny 2018), an
            ///     equal-area pseudocylindrical projection suited to hemispheric or
            ///     world maps.
            EqualEarth
            };

        /// @brief How a continuous shading column is divided into classes.
        enum class ClassificationMethod
            {
            /// @brief No classification. The value is mapped onto a continuous color ramp.
            Unclassed,
            /// @brief Jenks natural breaks. The class boundaries are chosen to keep the
            ///     values within each class close together and the classes well separated.
            JenksNaturalBreaks
            };

        /** @brief Constructor.
            @param canvas The canvas that the map is drawn on.
            @param colors The color scheme applied to the value column. The first color
                is the low end of the range, the last color the high end.*/
        explicit ChoroplethMap(
            Wisteria::Canvas* canvas,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the map.
            @param data The GeoDataset holding the regions and their geometry.
            @param valueColumnName The continuous column to shade the regions by. If not
                provided, every region is filled with a single color.
            @note Call the parent canvas's @c CalcAllSizes() after setting new data to
                re-plot.
            @throws std::runtime_error If @p valueColumnName is provided but not found,
                throws an exception. The @c what() message is UTF-8 encoded.*/
        void SetData(const std::shared_ptr<const Data::GeoDataset>& data,
                     const std::optional<wxString>& valueColumnName = std::nullopt);

        /// @returns The name of the shading column, or empty if the map is not data-shaded.
        [[nodiscard]]
        const wxString& GetValueColumnName() const noexcept
            {
            return m_valueColumnName;
            }

        /// @returns @c true if the shading column is categorical (each distinct value
        ///     drawn in its own color) rather than a continuous value ramp.
        [[nodiscard]]
        bool IsCategoricalShading() const noexcept
            {
            return m_isCategorical;
            }

        /** @brief Sets how a continuous shading column is split into classes.
            @param method The classification method, or ClassificationMethod::Unclassed
                for a continuous color ramp.
            @details When a method is set, each class gets one color from the scheme
                and the legend shows the class ranges instead of a ramp. This has no
                effect on a categorical shading column. Call SetData() again for a
                change to take effect.*/
        void SetClassificationMethod(const ClassificationMethod method) noexcept
            {
            m_classificationMethod = method;
            }

        /// @returns The classification method for a continuous shading column.
        [[nodiscard]]
        ClassificationMethod GetClassificationMethod() const noexcept
            {
            return m_classificationMethod;
            }

        /** @brief Sets the number of classes to split a continuous shading column into.
            @param count The class count. Values outside the 2 to 12 range are clamped
                when the classification is computed.
            @details Only used when GetClassificationMethod() is not
                ClassificationMethod::Unclassed. Call SetData() again for a change to
                take effect.*/
        void SetClassCount(const size_t count) noexcept { m_classCount = count; }

        /// @returns The requested number of classes for a continuous shading column.
        [[nodiscard]]
        size_t GetClassCount() const noexcept
            {
            return m_classCount;
            }

        /// @returns @c true if the continuous shading column was split into classes.
        [[nodiscard]]
        bool IsClassified() const noexcept
            {
            return m_isClassified;
            }

        /// @returns The class boundaries, lowest first, when IsClassified() is @c true.
        ///     There is one more boundary than there are classes.
        [[nodiscard]]
        const std::vector<double>& GetClassBreaks() const noexcept
            {
            return m_classBreaks;
            }

        /** @brief Records where the map's regions came from, for serialization and editing.
            @details This does not load anything. It just stores the strings so that
                saving the project (or re-opening the editor) can recreate the same map.
            @param kmlFilePath The path to the KML file the regions were read from.
            @param kmlIdField The @c ExtendedData field used as the region key
                (empty for the placemark name).
            @param dataSourceName The name of the project dataset whose column was
                merged in for shading (empty if none).
            @param dataSourceKeyColumn The column in that dataset matched against the
                region key (empty if none).*/
        void SetSourceInfo(wxString kmlFilePath, wxString kmlIdField,
                           wxString dataSourceName = wxString{},
                           wxString dataSourceKeyColumn = wxString{})
            {
            m_kmlFilePath = std::move(kmlFilePath);
            m_kmlIdField = std::move(kmlIdField);
            m_dataSourceName = std::move(dataSourceName);
            m_dataSourceKeyColumn = std::move(dataSourceKeyColumn);
            }

        /// @returns The path to the KML file the regions were read from.
        [[nodiscard]]
        const wxString& GetKMLFilePath() const noexcept
            {
            return m_kmlFilePath;
            }

        /// @returns The @c ExtendedData field used as the region key, or empty for the
        ///     placemark name.
        [[nodiscard]]
        const wxString& GetKMLIdField() const noexcept
            {
            return m_kmlIdField;
            }

        /// @returns The project dataset merged in for shading, or empty if none.
        [[nodiscard]]
        const wxString& GetDataSourceName() const noexcept
            {
            return m_dataSourceName;
            }

        /// @returns The dataset column matched against the region key, or empty if none.
        [[nodiscard]]
        const wxString& GetDataSourceKeyColumn() const noexcept
            {
            return m_dataSourceKeyColumn;
            }

        /// @returns The GeoDataset the map was built from (regions, geometry, and any
        ///     merged-in shading column), or @c nullptr if SetData() was never called.
        [[nodiscard]]
        const std::shared_ptr<const Data::GeoDataset>& GetGeoDataset() const noexcept
            {
            return m_geoData;
            }

        /// @brief Sets whether each region's label is drawn at its center.
        /// @param show @c true to draw region labels.
        void ShowRegionLabels(const bool show) noexcept { m_showLabels = show; }

        /// @returns @c true if region labels are drawn.
        [[nodiscard]]
        bool IsShowingRegionLabels() const noexcept
            {
            return m_showLabels;
            }

        /** @brief Sets what a region's label shows: its name, its mapped value, or both.
            @param display The label content.
            @details This applies both to the labels drawn on the map (see
                ShowRegionLabels()) and to the label shown when a region is selected.
                A region with no mapped value falls back to its name. The percentage
                forms use each region's share of the value column's total (or, for a
                categorical column, the share of regions in that category).*/
        void SetLabelDisplay(const BinLabelDisplay display) noexcept { m_labelDisplay = display; }

        /// @returns What a region's label shows.
        [[nodiscard]]
        BinLabelDisplay GetLabelDisplay() const noexcept
            {
            return m_labelDisplay;
            }

        /// @brief Sets whether a faint latitude/longitude grid is drawn over the map.
        /// @param show @c true to draw the graticule.
        /// @details Each grid line is labeled with its coordinate at the map's north
        ///     or west edge.
        void ShowGraticule(const bool show) noexcept { m_showGraticule = show; }

        /// @returns @c true if the latitude/longitude graticule is drawn.
        [[nodiscard]]
        bool IsShowingGraticule() const noexcept
            {
            return m_showGraticule;
            }

        /// @brief Sets the color used for regions whose value is missing.
        /// @param color The "no data" fill color.
        void SetNoDataColor(const wxColour& color) { m_noDataColor = color; }

        /// @returns The color used for regions whose value is missing.
        [[nodiscard]]
        const wxColour& GetNoDataColor() const noexcept
            {
            return m_noDataColor;
            }

        /** @brief Sets the brush style used to fill regions whose value is missing.
            @param style The fill style. @c wxBRUSHSTYLE_SOLID (the default) fills a
                no-data region flat with GetNoDataColor(). A hatch style instead draws
                GetNoDataColor() hatching over a transparent background, so a no-data
                region reads differently from a shaded one.*/
        void SetNoDataFillStyle(const wxBrushStyle style) noexcept { m_noDataFillStyle = style; }

        /// @returns The brush style used to fill regions whose value is missing.
        [[nodiscard]]
        wxBrushStyle GetNoDataFillStyle() const noexcept
            {
            return m_noDataFillStyle;
            }

        /** @brief Sets a continuous column drawn as a scaled circle at each region's
                center, on top of any region shading.
            @param columnName The column that sizes the circles, or @c std::nullopt
                for none.
            @details The circle area is proportional to the value. A region needs a
                finite value above zero to get a circle. This runs as a second
                variable over a shaded map, or on its own for a plain
                proportional map.*/
        void SetProportionalSymbolColumn(const std::optional<wxString>& columnName)
            {
            m_symbolColumnName = columnName.value_or(wxString{});
            }

        /// @returns The name of the proportional column, or empty if none.
        [[nodiscard]]
        const wxString& GetProportionalSymbolColumnName() const noexcept
            {
            return m_symbolColumnName;
            }

        /// @brief Sets the fill color of the proportional symbols.
        /// @param color The symbol color. A translucent form of it is used for the fill.
        void SetProportionalSymbolColor(const wxColour& color)
            {
            if (color.IsOk())
                {
                m_symbolColor = color;
                }
            }

        /// @returns The fill color of the proportional symbols.
        [[nodiscard]]
        const wxColour& GetProportionalSymbolColor() const noexcept
            {
            return m_symbolColor;
            }

        /// @brief Sets the projection used to draw the map.
        /// @param projection The projection, or MapProjection::Automatic to choose one
        ///     from the data extent.
        void SetMapProjection(const MapProjection projection) noexcept
            {
            m_projection = projection;
            }

        /// @returns The projection setting (which may be MapProjection::Automatic).
        /// @sa GetEffectiveMapProjection().
        [[nodiscard]]
        MapProjection GetMapProjection() const noexcept
            {
            return m_projection;
            }

        /// @returns The projection actually in use. This differs from GetMapProjection()
        ///     only when that is MapProjection::Automatic, and is not resolved until the
        ///     map has been laid out at least once.
        [[nodiscard]]
        MapProjection GetEffectiveMapProjection() const noexcept
            {
            return m_effectiveProjection;
            }

        /** @brief Builds a gradient legend for the value column.
            @param options The legend options.
            @returns The legend, or @c nullptr if the map is not shaded by a value.*/
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) override;

        /** @brief Builds the combined legend for a map that has a concentric symbol.
            @param options The legend options.
            @returns The combined legend, or @c nullptr if there is no
                proportional column.*/
        std::unique_ptr<ChoroplethLegend> CreateChoroplethLegend(const LegendOptions& options);

      private:
        void RecalcSizes(wxDC& dc) final;

        void SetAutoAccessibilityAttributes() final;

        /// @brief Resolves MapProjection::Automatic and computes the projection's
        ///     parameters from the current data extent.
        void PrepareProjection();

        /// @brief Assigns a color to each category in a categorical shading column and
        ///     fills m_regionColors and m_categoryLegend from it.
        /// @param column The categorical column to shade by.
        void BuildCategoricalColors(const Data::ColumnWithStringTable& column);

        /// @brief Splits a continuous shading column into classes and fills
        ///     m_classBreaks, m_classColors, and m_regionColors from it.
        /// @param values The column's values.
        /// @returns @c true if a usable classification was produced, @c false to fall
        ///     back to the continuous color ramp.
        [[nodiscard]]
        bool BuildClassifiedColors(const std::vector<double>& values);

        /// @brief Builds the swatch legend for a categorical shading column.
        /// @param options The legend options.
        /// @returns The legend, or @c nullptr if there are no categories.
        std::unique_ptr<GraphItems::Label> CreateCategoricalLegend(const LegendOptions& options);

        /// @brief Builds the labeled-range legend for a classified shading column.
        /// @param options The legend options.
        /// @returns The legend, or @c nullptr if there are no classes.
        std::unique_ptr<GraphItems::Label> CreateClassifiedLegend(const LegendOptions& options);

        /// @brief Formats a degree value with a hemisphere suffix (e.g., "84.5°W").
        /// @param degrees The signed degree value.
        /// @param negSuffix The suffix for negative values (e.g., "W" or "S").
        /// @param posSuffix The suffix for positive values (e.g., "E" or "N").
        /// @param wraps @c true for longitude, so +/-180 gets no suffix.
        /// @returns The formatted label.
        [[nodiscard]]
        static wxString GraticuleDegreeText(double degrees, const wchar_t* negSuffix,
                                            const wchar_t* posSuffix, bool wraps);

        /// @brief Returns the largest 1, 2, or 5 times a power of ten that is not
        ///     greater than @c value.
        /// @param value The value to round down.
        /// @returns The nice number, or @c 0 if @c value is not finite and positive.
        [[nodiscard]]
        static double NiceNumberFloor(double value);

        /// @brief Partitions @c values into @c classCount classes so that the squared
        ///     deviation from the class means is as small as possible.
        /// @param values The values to classify.
        /// @param classCount The number of classes to produce.
        /// @returns @c classCount + 1 boundaries, lowest first, or an empty vector
        ///     when the data cannot be partitioned.
        [[nodiscard]]
        static std::vector<double> JenksNaturalBreaks(std::vector<double> values,
                                                      size_t classCount);

        /// @brief Composes the label text for a region, honoring GetLabelDisplay().
        /// @param row The region's row.
        /// @returns The label text, empty for BinLabelDisplay::NoDisplay.
        [[nodiscard]]
        wxString BuildRegionLabel(size_t row) const;

        /// @brief Measures the strip the graticule's coordinate labels need.
        /// @param dc The device context to measure text with.
        /// @returns The width to reserve at the left and the height at the top.
        [[nodiscard]]
        wxSize MeasureGraticuleGutter(wxDC& dc) const;

        /// @brief Adds the latitude/longitude grid lines and their labels.
        /// @param mapRect The rectangle the map is drawn in (the plot area minus the
        ///     label gutter).
        /// @param gutterSize The strip reserved at the top and left for the labels.
        void AddGraticule(const wxRect& mapRect, const wxSize& gutterSize);

        /// @brief Adds a scaled circle at each region's center, its area set by the
        ///     proportional column.
        /// @param mapRect The rectangle the map is drawn in, used to size the
        ///     largest circle.
        void AddProportionalSymbols(const wxRect& mapRect);

        /// @brief Projects a geographic coordinate into the projection plane.
        /// @param coord The coordinate to project.
        /// @returns The point in projection-plane units (y increasing northward).
        [[nodiscard]]
        std::pair<double, double> Project(const Data::GeoCoordinate& coord) const;

        /// @brief Maps a geographic coordinate to a pixel in the plot area.
        /// @param coord The coordinate to map.
        /// @returns The pixel position, using the transform computed in RecalcSizes().
        [[nodiscard]]
        wxPoint GeoToScreen(const Data::GeoCoordinate& coord) const;

        // Fisher-Jenks runs in quadratic time, so it is only attempted for inputs at
        // or below this size. Larger value columns fall back to the continuous color ramp.
        constexpr static size_t MAX_JENKS_VALUE_COUNT{ 8'000 };

        std::shared_ptr<const Data::GeoDataset> m_geoData;
        wxString m_valueColumnName;

        // where the regions came from (stored for serialization / editing, not used to draw)
        wxString m_kmlFilePath;
        wxString m_kmlIdField;
        wxString m_dataSourceName;
        wxString m_dataSourceKeyColumn;

        // one entry per region row; only valid when m_hasValues is true
        std::vector<wxColour> m_regionColors;
        std::vector<wxColour> m_colorSpectrum;
        std::pair<double, double> m_valueRange{ 0.0, 0.0 };
        bool m_hasValues{ false };
        // sum of the finite values in a continuous shading column, for percentage labels
        double m_valueTotal{ 0.0 };

        // set when the shading column is categorical rather than a value ramp
        bool m_isCategorical{ false };
        // ordered (category label, swatch color) pairs for the categorical legend
        std::vector<std::pair<wxString, wxColour>> m_categoryLegend;

        // classification of a continuous shading column; the method and count are
        // user settings, the rest is recomputed by SetData()
        ClassificationMethod m_classificationMethod{ ClassificationMethod::Unclassed };
        size_t m_classCount{ 5 };
        bool m_isClassified{ false };
        std::vector<double> m_classBreaks;
        std::vector<wxColour> m_classColors;
        // region count per category code, and the number of regions that have a
        // category, for percentage labels on a categorical map
        std::map<Data::GroupIdType, size_t> m_categoryRowCounts;
        size_t m_categorizedRegionCount{ 0 };

        bool m_showLabels{ false };
        bool m_showGraticule{ false };
        BinLabelDisplay m_labelDisplay{ BinLabelDisplay::BinName };
        wxColour m_noDataColor{ L"#DDDDDD" };
        wxBrushStyle m_noDataFillStyle{ wxBRUSHSTYLE_SOLID };

        wxString m_symbolColumnName;
        wxColour m_symbolColor{ L"#2F6F8F" };

        MapProjection m_projection{ MapProjection::Automatic };
        MapProjection m_effectiveProjection{ MapProjection::Equirectangular };

        // projection parameters (radians), recomputed by PrepareProjection()
        Data::GeoBoundingBox m_dataBounds;
        double m_projLon0{ 0.0 };
        double m_projLat0{ 0.0 };
        // Albers conic constants
        double m_albersN{ 1.0 };
        double m_albersC{ 0.0 };
        double m_albersRho0{ 0.0 };
        // equirectangular longitude compression, cos(center latitude)
        double m_lonScale{ 1.0 };

        // plane-to-pixel transform, recomputed each RecalcSizes()
        double m_geoScale{ 1.0 };
        wxPoint m_geoOrigin;
        std::pair<double, double> m_planeMin{ 0.0, 0.0 };
        std::pair<double, double> m_planeMax{ 0.0, 0.0 };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_CHOROPLETH_MAP_H
