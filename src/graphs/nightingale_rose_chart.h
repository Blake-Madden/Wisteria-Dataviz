/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_NIGHTINGALE_ROSE_CHART_H
#define WISTERIA_NIGHTINGALE_ROSE_CHART_H

#include "../base/settings.h"
#include "groupgraph2d.h"
#include <cstdint>
#include <map>
#include <optional>
#include <utility>
#include <vector>

namespace Wisteria::Graphs
    {
    // clang-format off
    /** @brief A Nightingale rose chart (also known as a polar-area diagram or coxcomb chart).
        @details Categories are equal-width angular slices around a common center. Unlike a
            pie chart, the value is encoded by how far the wedge reaches out, not by its
            angle. By default, the wedge <b>area</b> is proportional to the value (the
            method Florence Nightingale used).

            An optional grouping column places several series in each angular slice.
            The series can be drawn two ways:

            | Overlaid                              | Stacked                               |
            | :------------------------------------ | :------------------------------------ |
            | All wedges start at the center, largest drawn first. (Nightingale's 1858 layout.) | Series stack as annular bands. The outer edge is the slice total. |

        @note Slices sweep clockwise from the start angle, which defaults to 6 o'clock
            (see SetStartAngle()). They follow category code order (not alphabetical) so
            months stay chronological.

        @par %Data:
            This graph accepts a Data::Dataset with a categorical column for the slice
            categories and (optionally):
            - A continuous column of values to aggregate into each slice.
              If not provided, the frequency counts of the categories are used.
            - A second categorical column that splits each slice into series.

            | Month    | Cause             | Deaths |
            | :--      | :--               | --:    |
            | Apr 1854 | Zymotic diseases  | 1      |
            | Apr 1854 | Wounds & injuries | 0      |
            | Apr 1854 | All other causes  | 5      |
            | May 1854 | Zymotic diseases  | 12     |
            | ...      | ...               | ...    |

            With the data above, the categorical column is `Month`, the aggregate column is
            `Deaths`, and the grouping column is `Cause`.

        @par Missing Data:
            - Missing data in the categorical column will be shown as an empty slice label.
            - Non-finite or negative values in the aggregate column are ignored
              (pairwise deletion). A value of zero is plotted as a wedge with no radius.

        @par Citation:
            Nightingale's original figure, the "Diagram of the Causes of Mortality in the
            Army in the East," appeared in <i>Notes on Matters Affecting the Health,
            Efficiency, and Hospital Administration of the British Army</i> (Florence
            Nightingale, London, 1858). It plots army mortality for April 1854 through
            March 1856 as two polar-area figures.

            This chart type is often called a "coxcomb." Nightingale used that word for the
            booklet the diagrams were bound in, not for the figure itself. For that
            distinction, and a history of the diagrams, see Hugh Small, "Florence
            Nightingale's Statistical Diagrams" (Florence Nightingale Museum research
            conference, 1998).

            The mortality data used in the example is the digitized @c Nightingale dataset
            from the @c HistData R package by Michael Friendly.

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog,
         // "canvas" is a scrolled window derived object that will hold the plot
         auto canvas = new Wisteria::Canvas{ this };
         canvas->SetFixedObjectsGridSize(1, 2);

         // data is in wide format (one row per month, a column per cause),
         // so import it and reshape it to one row per month and cause
         auto wideData = std::make_shared<Data::Dataset>();
         try
            {
            wideData->ImportCSV(L"datasets/historical/Nightingale.csv",
                ImportInfo().
                ContinuousColumns({ L"Disease", L"Wounds", L"Other" }).
                CategoricalColumns({
                  { L"Month", CategoricalImportMethod::ReadAsStrings }
                  }));
            }
         catch (const std::exception& err)
            {
            wxMessageBox(wxString::FromUTF8(err.what()), _(L"Import Error"),
                         wxOK | wxICON_ERROR | wxCENTRE);
            return;
            }
         auto roseData = Data::Pivot::PivotLonger(wideData, { L"Month" },
             { L"Disease", L"Wounds", L"Other" }, { L"Cause" }, L"Deaths");

         auto plot = std::make_shared<NightingaleRoseChart>(canvas);
         plot->SetData(roseData, L"Deaths", L"Month", L"Cause");

         canvas->SetFixedObject(0, 0, plot);
         canvas->SetFixedObject(0, 1,
             plot->CreateLegend(
                 LegendOptions{}.IncludeHeader(true).
                     PlacementHint(LegendCanvasPlacementHint::RightOfGraph)));
        @endcode*/
    // clang-format on

    class NightingaleRoseChart final : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(NightingaleRoseChart);
        NightingaleRoseChart() = default;

      public:
        /// @brief How a wedge's value maps to its radius.
        enum class RadialScaling
            {
            AreaProportional,  /*!< The wedge area is proportional to the value
                                    (radius scales with the square root of the value).*/
            RadiusProportional /*!< The wedge radius is proportional to the value.*/
            };

        /// @brief How the series within an angular slice are arranged.
        enum class SeriesDisplay
            {
            Overlaid, /*!< All wedges start at the center. Larger ones sit behind smaller ones.*/
            Stacked   /*!< Each series is an annular band on the previous one.*/
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the chart on.
            @param brushes The brush scheme, which will contain the color and brush patterns
                to render the wedges with.
            @param colors An optional solid color scheme drawn under the wedge brushes.
                Useful behind a hatched brush. Leave as @c nullptr to use only the brush scheme.*/
        explicit NightingaleRoseChart(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr);

        /** @brief Sets the data for the chart.
            @param data The data to use.
            @param aggregateColumnName The continuous column to aggregate into each slice.
                Pass @c std::nullopt to use the frequency counts of the categories.
            @param categoryColumnName The categorical column containing the slice categories.
            @param groupColumnName The optional categorical column that splits each slice
                into series.
            @note Call the parent canvas's @c CalcAllSizes() after setting a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const std::optional<wxString>& aggregateColumnName,
                     const wxString& categoryColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @returns The name of the categorical column that the slice categories came from.
        [[nodiscard]]
        const wxString& GetCategoryColumnName() const noexcept
            {
            return m_categoryColumnName;
            }

        /// @returns The name of the continuous column that the slice values came from, or an
        ///     empty string if frequency counts are being used.
        [[nodiscard]]
        const wxString& GetAggregateColumnName() const noexcept
            {
            return m_aggregateColumnName;
            }

        /// @returns How a wedge's value maps to its radius.
        [[nodiscard]]
        RadialScaling GetRadialScaling() const noexcept
            {
            return m_radialScaling;
            }

        /// @brief Sets how a wedge's value maps to its radius.
        /// @param scaling The scaling to use (default is @c RadialScaling::AreaProportional).
        void SetRadialScaling(const RadialScaling scaling) noexcept { m_radialScaling = scaling; }

        /// @returns How the series within an angular slice are arranged.
        [[nodiscard]]
        SeriesDisplay GetSeriesDisplay() const noexcept
            {
            return m_seriesDisplay;
            }

        /// @brief Sets how the series within an angular slice are arranged.
        /// @param display The arrangement to use (default is @c SeriesDisplay::Overlaid).
        void SetSeriesDisplay(const SeriesDisplay display) noexcept { m_seriesDisplay = display; }

        /// @returns The angle (in degrees) where the first slice starts.
        [[nodiscard]]
        double GetStartAngle() const noexcept
            {
            return m_startAngle;
            }

        /// @brief Sets the angle where the first slice starts.
        /// @param angleDeg The start angle in degrees (0 = 3 o'clock, 90 = 6 o'clock, etc.).
        ///     The value is wrapped into the [0, 360) range. A non-finite value is
        ///     ignored and the start angle is left at its default of 90 degrees.
        void SetStartAngle(double angleDeg) noexcept;

        /// @returns @c true if the chart is showing category labels around the perimeter.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_showLabels;
            }

        /// @brief Whether to show category labels around the perimeter.
        /// @param show @c true to show labels.
        void ShowLabels(const bool show) noexcept { m_showLabels = show; }

        /// @returns The opacity (0-255) that ghosted wedges are drawn with.
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /// @brief Sets the opacity that ghosted wedges are drawn with.
        /// @param opacity 0 (fully transparent) to 255 (opaque).
        void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

        /** @brief Draws a wedge translucent instead of solid.
            @details This de-emphasizes a series within a slice without removing it,
                like the boundary line Nightingale drew across the November 1854 wedge.
                The wedge outline still shows how far the series reaches.
            @param groupLabel The series (group) label to ghost, as shown in the legend.
            @param categoryLabel The slice (category) label to ghost it in (e.g., a month).
                Leave empty to ghost the series in every slice.
            @note Ghosting applies only when a grouping column is set (see SetData()).
                Without one, each slice is a single wedge and ghost specifications have
                no effect.*/
        void GhostWedge(const wxString& groupLabel, const wxString& categoryLabel = wxString{});

        /// @brief Clears every wedge ghosted by GhostWedge().
        void ClearGhostedWedges() noexcept { m_ghostedWedges.clear(); }

        /// @returns The wedges ghosted by GhostWedge(), as {group label, category label}
        ///     pairs. An empty category label means every slice.
        [[nodiscard]]
        const std::vector<std::pair<wxString, wxString>>& GetGhostedWedges() const noexcept
            {
            return m_ghostedWedges;
            }

        /// @returns The number of angular slices (categories) in the chart.
        [[nodiscard]]
        size_t GetSliceCount() const noexcept
            {
            return m_slices.size();
            }

        /// @returns The number of wedges produced by the last layout pass.
        [[nodiscard]]
        size_t GetWedgeCount() const noexcept
            {
            return m_wedgeCount;
            }

      private:
        void RecalcSizes(wxDC& dc) final;
        void SetAutoAccessibilityAttributes() final;

        /// @returns @c true if the wedge for @p groupLabel within @p categoryLabel has been
        /// ghosted.
        [[nodiscard]]
        bool IsWedgeGhosted(const wxString& categoryLabel, const wxString& groupLabel) const;

        /** @brief A single wedge, a filled circular sector between an inner and outer radius.
            @details A zero inner radius draws a full wedge from the center. A positive
                inner radius draws an annular band for a stacked series segment.*/
        class RoseWedge final : public GraphItems::GraphItemBase
            {
          public:
            /** @brief Constructor.
                @param center The center of the chart.
                @param startAngleDeg The angle (in degrees) that the wedge starts at.
                @param endAngleDeg The angle (in degrees) that the wedge ends at.
                @param innerRadius The radius that the wedge starts at (0 for a full wedge).
                @param outerRadius The radius that the wedge reaches out to.
                @param brush The brush to fill the wedge with.
                @param pen The pen to outline the wedge with.*/
            RoseWedge(const wxPoint& center, double startAngleDeg, double endAngleDeg,
                      double innerRadius, double outerRadius, const wxBrush& brush,
                      const wxPen& pen);

            /// @private
            [[nodiscard]]
            wxRect GetBoundingBox(wxDC& dc) const final;

          private:
            wxRect Draw(wxDC& dc) const final;

            [[nodiscard]]
            bool HitTest(wxPoint pt, wxDC& dc) const final;

            void Offset(int xOffset, int yOffset) final;

            void SetBoundingBox(const wxRect& rect, wxDC& dc, double scaling) final;

            /** @returns The wedge outline as canvas points, approximating its two arcs
                    with chords.
                @note Used for hit-testing and the selection outline only. The visible
                    fill is built from true circular arcs (see Draw()).*/
            [[nodiscard]]
            std::vector<wxPoint> GetPolygon() const;

            wxPoint m_center;
            double m_startAngle{ 0 };
            double m_endAngle{ 0 };
            double m_innerRadius{ 0 };
            double m_outerRadius{ 0 };
            };

        /// @brief One angular slice and its series values.
        struct SliceInfo
            {
            wxString m_label;
            Data::GroupIdType m_categoryId{ 0 };
            // Series ID and its aggregated value, ordered by GetGroupIds().
            // One entry keyed on 0 when grouping is off.
            std::vector<std::pair<Data::GroupIdType, double>> m_seriesValues;

            /// @returns The sum of every series value in this slice.
            [[nodiscard]]
            double GetTotal() const noexcept
                {
                double total{ 0 };
                for (const auto& seriesValue : m_seriesValues)
                    {
                    total += seriesValue.second;
                    }
                return total;
                }
            };

        std::vector<SliceInfo> m_slices;
        wxString m_aggregateColumnName;
        wxString m_categoryColumnName;
        RadialScaling m_radialScaling{ RadialScaling::AreaProportional };
        SeriesDisplay m_seriesDisplay{ SeriesDisplay::Overlaid };
        double m_startAngle{ 90.0 }; // 6 o'clock in screen coordinates
        bool m_showLabels{ true };
        size_t m_wedgeCount{ 0 };

        std::vector<std::pair<wxString, wxString>> m_ghostedWedges;
        uint8_t m_ghostOpacity{ Wisteria::Settings::GHOST_OPACITY };

        // proportion of the plot area's shorter side used for the outer radius
        constexpr static double m_plainRadiusProportion{ 0.45 };
        constexpr static double m_labeledRadiusProportion{ 0.40 };
        // gap in DIPs between a label and the radius it is anchored against
        constexpr static double m_labelGapDIPs{ 8.0 };
        // slice labels are scaled down so they read as captions, not competing with the wedge
        constexpr static double m_labelScaling{ 0.7 };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_NIGHTINGALE_ROSE_CHART_H
