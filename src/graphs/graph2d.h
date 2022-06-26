/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_PLOT2D_H__
#define __WISTERIA_PLOT2D_H__

#include "../base/points.h"
#include "../base/polygon.h"
#include "../base/canvas.h"
#include "../base/colorbrewer.h"
#include "../data/dataset.h"
#include "../base/axis.h"
#include "../base/lines.h"
#include "../math/mathematics.h"

/// @brief Classes for presenting data graphically.
namespace Wisteria::Graphs
    {
    /// @brief Options for building a legend.
    class LegendOptions
        {
    public:
        /// @brief Sets whether to show a header on top of the legend
        ///     (which is usually the grouping variable name).
        /// @param includeHeader @c true to include headers.
        /// @returns A self reference.
        LegendOptions& IncludeHeader(bool includeHeader) noexcept
            {
            m_includeHeader = includeHeader;
            return *this;
            }
        /// @returns @c true if including a header on the legend.
        [[nodiscard]] bool IsIncludingHeader() const noexcept
            { return m_includeHeader; }
        /// @brief Provides a hint about where the legend will be placed
        ///     relative to its parent graph.
        /// @param hint A hint about where the legend will be placed
        ///     after construction.\n
        ///     This is used for defining the legend's padding, outlining,
        ///     canvas proportions, etc.
        /// @returns A self reference.
        LegendOptions& PlacementHint(LegendCanvasPlacementHint hint) noexcept
            {
            m_hint = hint;
            return *this;
            }
        /// @returns How the legend is being placed relative to its graph.
        LegendCanvasPlacementHint GetPlacementHint() const noexcept
            { return m_hint; }
        /// @brief Which ring of a pie-like chart is the legend referring to.
        /// @param peri Whether the legend is the outer or inner ring of the chart.
        /// @returns A self reference.
        LegendOptions& RingPerimeter(Perimeter peri) noexcept
            {
            m_perimeter = peri;
            return *this;
            }
        /// @returns Which ring of a pie-like chart is the legend referring to.
        Perimeter GetRingPerimeter() const noexcept
            { return m_perimeter; }
    private:
        bool m_includeHeader{ false };
        LegendCanvasPlacementHint m_hint{ LegendCanvasPlacementHint::RightOfGraph };
        Perimeter m_perimeter{ Perimeter::Outer };
        };

    /// @brief Base class for plotting 2D data.
    class Graph2D : public GraphItems::GraphItemBase
        {
    public:
        /** @brief Constructor.
            @param canvas The parent canvas that the plot is being drawn on.*/
        explicit Graph2D(Canvas* canvas);
        /// @private
        Graph2D(const Graph2D&) = delete;
        /// @private
        Graph2D(Graph2D&&) = delete;
        /// @private
        Graph2D& operator=(const Graph2D&) = delete;
        /// @private
        Graph2D& operator=(Graph2D&&) = delete;

        /** @brief Embeds an object onto the plot.
            @param object The object (e.g., a text note or image) to embed onto the plot.
            @param pt The X and Y coordinates of the object. These coordinates are relative to the
                plot's X and Y axes, not physical coordinates on the canvas.\n
                Note that if one (or both) of the axes are date-based, you can call
                @c GetPointFromDate() to find its point to use here.
            @param interestPts An optional collection of points on the plot to draw a line from
                this object's anchor point to.
                For example, this can draw a line from a data point to an annotation.
            @note This is intended as a way for the client to add a custom object on top of the plot.
                An example would be inserting a @c Label as a sticky note.*/
        void AddEmbeddedObject(std::shared_ptr<GraphItems::GraphItemBase> object,
                               const wxPoint pt,
                               const std::vector<wxPoint> interestPts = std::vector<wxPoint>())
            {
            if (object != nullptr)
                {
                object->SetId(m_currentAssignedId++);
                object->SetDPIScaleFactor(GetDPIScaleFactor());
                m_embeddedObjects.push_back({ object, pt, interestPts });
                }
            }

        /** @name Title Functions
            @brief Functions related to the titles and caption.*/
        /// @{

        /** @brief Sets/gets the graph's title.
            @note The title's relative alignment controls where the title is aligned against the plot
                (i.e., whether it is center, right aligned, etc.).\n
                Its display info controls its font, color, and other formatting settings.\n
                As a special note, changing its background color will stretch the title across
                the graph area, making it appear as a banner.
            @returns The title.*/
        [[nodiscard]] GraphItems::Label& GetTitle() noexcept
            { return m_title; }

        /** @brief Sets/gets the graph's subtitle.
            @note The subtitle's relative alignment controls where the subtitle is aligned against the plot.\n
                Its display info controls its font, color, and other formatting settings.
            @returns The subtitle.*/
        [[nodiscard]] GraphItems::Label& GetSubtitle() noexcept
            { return m_subtitle; }

        /** @brief Sets/gets the graph's caption.
            @note The caption's relative alignment controls where the caption is aligned against the plot.\n
                Its display info controls its font, color, and other formatting settings.
            @returns The caption.*/
        [[nodiscard]] GraphItems::Label& GetCaption() noexcept
            { return m_caption; }
        /// @}

        /** @name Axis Functions
            @brief Functions related to the axes.*/
        /// @{

        /// @returns The bottom X axis.
        [[nodiscard]] Wisteria::GraphItems::Axis& GetBottomXAxis() noexcept
            { return m_bottomXAxis; }
        /// @returns The top X axis.
        [[nodiscard]] Wisteria::GraphItems::Axis& GetTopXAxis() noexcept
            { return m_topXAxis; }
        /// @returns The left Y axis.
        [[nodiscard]] Wisteria::GraphItems::Axis& GetLeftYAxis() noexcept
            { return m_leftYAxis; }
        /// @returns The right Y axis.
        [[nodiscard]] Wisteria::GraphItems::Axis& GetRightYAxis() noexcept
            { return m_rightYAxis; }
        /// @returns The custom axes.
        [[nodiscard]] std::vector<Wisteria::GraphItems::Axis>& GetCustomAxes() noexcept
            { return m_customAxes; }
        /// @returns The reference lines.
        [[nodiscard]] std::vector<Wisteria::GraphItems::ReferenceLine>& GetReferenceLines() noexcept
            { return m_referenceLines; }
        /// @returns The reference areas.
        [[nodiscard]] std::vector<Wisteria::GraphItems::ReferenceArea>& GetReferenceAreas() noexcept
            { return m_referenceAreas; }

        /// @returns `true` if a copy of the bottom X axis is being displayed on the top of the graph.
        [[nodiscard]] bool IsXAxisMirrored() const noexcept
            { return m_mirrorXAxis; }
        /** @brief Sets whether to display a copy of the bottom X axis on the top of the graph.
            @param mirror Whether to display the mirrored axis.*/
        void MirrorXAxis(const bool mirror) noexcept
            { m_mirrorXAxis = mirror; }

        /// @returns `true` if a copy of the left Y axis is being displayed on the right side of the graph.
        [[nodiscard]] bool IsYAxisMirrored() const noexcept
            { return m_mirrorYAxis; }
        /** @brief Sets whether to display a copy of the left Y axis on the right side of the graph.
            @param mirror Whether to display the mirrored axis.*/
        void MirrorYAxis(const bool mirror) noexcept
            { m_mirrorYAxis = mirror; }

        /** @brief Adds a custom axis that would be in addition to the regular X and Y
                axes around the graph.
            @param axis The custom axis to add.*/
        void AddCustomAxis(Wisteria::GraphItems::Axis& axis)
            {
            m_customAxes.push_back(axis);
            }

        /** @brief Adds a reference line to draw across the graph.
            @details The parent axis and starting point are specified in the @c ReferenceLine,
                and the graph will set the length of the line to be the full length of the
                parallel axis for you.
            @param refLine The reference line to add.*/
        void AddReferenceLine(const Wisteria::GraphItems::ReferenceLine& refLine)
            { m_referenceLines.push_back(refLine); }

        /** @brief Adds a reference area to draw across the graph.
            @details The parent axis and starting points are specified in the composite
                reference lines, and the graph will set the length of the lines to be the
                full length of the parallel axis for you.
            @param refArea The reference area to add.
            @note Duplicate reference areas will be combined into one on the legend;
                This is useful for instances of related events on a plot (e.g., recessions).
                In this context, 'duplicate' means the areas have the same label and area color.*/
        void AddReferenceArea(const Wisteria::GraphItems::ReferenceArea& refArea)
            { m_referenceAreas.push_back(refArea); }
        /// @}

        /** @name Visual Effect Functions
            @brief Functions related to visual effects (e.g., background color).*/
        /// @{

        /** @brief Sets the image brush to paint with.
            @details This is used by derived classes that use stipple painting for its objects
                (e.g., barcharts).
            @param image The image to paint with.*/
        void SetStippleBrush(const wxBitmapBundle& image) noexcept
            { m_stipple = image; }
        /// @brief Gets the stipple brush.
        /// @returns The image that is being used to paint with, or null if one hasn't been specified.
        [[nodiscard]] const wxBitmapBundle& GetStippleBrush() const noexcept
            { return m_stipple; }

        /// @returns The background color of the plotting area
        ///     (i.e., the area inside the X and Y axes).
        /// @note By default, this color is invalid and not used, which will allow the parent
        ///     canvas's background to show through. To see which color is actually being shown
        ///     in the plotting area, call GetPlotOrCanvasColor().
        [[nodiscard]] wxColour GetBackgroundColor() const noexcept
            { return m_bgColor; }
        /** @brief Sets the background color of the plot.
                This is the color of the plotting area (inside the main axes).
                This is invalid by default (normally, the canvas background will show through).
            @param color The color to paint with.*/
        void SetBackgroundColor(const wxColour& color)
            { m_bgColor = color; }

        /// @returns The plot background color, if it is valid and not transparent;
        ///     otherwise, returns the canvas's background.
        [[nodiscard]] wxColour GetPlotOrCanvasColor() const noexcept
            {
            return (m_bgColor.IsOk() && m_bgColor.GetAlpha() != wxALPHA_TRANSPARENT) ?
                m_bgColor : GetCanvas()->GetBackgroundColor();
            }

        /// @brief Sets a common image to be drawn just within the bars' (or boxes') areas.
        /// @details This only applies to graphs which use boxes to visualized data
        ///     (e.g., bar charts, box plots).
        /// @param boxImage The image to draw across the bars/boxes.
        /// @param outlineColor The outline color of the bars/boxes.
        /// @note This effect will only apply to bars/boxes using the @c CommonImage effect.\n
        ///     If the image is smaller than the plot area, then it will not be used
        ///     and the bars/boxes will fall back to using a solid color.
        void SetCommonBoxImage(const wxBitmapBundle& boxImage,
                               const wxColour& outlineColor) noexcept
            {
            m_commonBoxImage = boxImage;
            m_imageOutlineColor = outlineColor;
            }
        /// @}

        /** @name Property Functions
            @brief Functions related to adding custom properties to a plot.
            @details This is a useful way to add extended information to a plot
                that can be application specific.*/
        /// @{

        /// @brief Clears all custom properties.
        void ClearProperties() noexcept
            { m_properties.clear(); }
        /// @brief Adds (or updates an existing) custom property.
        /// @param key The property key to update.
        /// @param val The new value for the property.
        void AddProperty(const wxString& key, const wxVariant& val)
            { m_properties[key] = val; }
        /// @returns Whether a property is in the canvas.
        /// @param key The property key to search for.
        [[nodiscard]] bool HasProperty(const wxString& key) const
            { return m_properties.find(key) != m_properties.end(); }
        /// @returns The value of the specified property, or an empty variant if not found.
        /// @param key The property key to search for.
        /// @returns The property if found, or an empty wxVariant otherwise.
        [[nodiscard]] wxVariant GetPropertyValue(const wxString& key) const
            { return HasProperty(key) ? m_properties.find(key)->second : wxVariant(); }
        /// @}

        /** @brief Builds and returns a legend.
            @details This is graph-type specific and must be defined in derived graph classes.
            @param options Options for how to build the legend.
            @returns The legend for the plot.*/
        virtual std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendOptions& options) = 0;

        // Just hiding these from Doxygen. If these are included inside of groupings,
        // then the "private" tag will break the group in the generated help.
        /// @private
        void SetCommonBoxImage(wxBitmapBundle&& boxImage,
                               const wxColour& outlineColor) noexcept
            {
            m_commonBoxImage = std::move(boxImage);
            m_imageOutlineColor = outlineColor;
            }
        /// @private
        void SetStippleBrush(wxBitmapBundle&& image) noexcept
            { m_stipple = std::move(image); }
        /// @private
        [[nodiscard]] const Wisteria::GraphItems::Axis& GetBottomXAxis() const noexcept
            { return m_bottomXAxis; }
        /// @private
        [[nodiscard]] const Wisteria::GraphItems::Axis& GetTopXAxis() const noexcept
            { return m_topXAxis; }
        /// @private
        [[nodiscard]] const Wisteria::GraphItems::Axis& GetLeftYAxis() const noexcept
            { return m_leftYAxis; }
        /// @private
        [[nodiscard]] const Wisteria::GraphItems::Axis& GetRightYAxis() const noexcept
            { return m_rightYAxis; }
        /// @private
        [[nodiscard]] const std::vector<Wisteria::GraphItems::Axis>& GetCustomAxes() const noexcept
            { return m_customAxes; }
        /// @private
        [[nodiscard]] const std::vector<Wisteria::GraphItems::ReferenceLine>& GetReferenceLines() const noexcept
            { return m_referenceLines; }
        /// @private
        void AddReferenceLine(Wisteria::GraphItems::ReferenceLine&& refLine)
            { m_referenceLines.emplace_back(refLine); }
        /// @private
        [[nodiscard]] const std::vector<Wisteria::GraphItems::ReferenceArea>& GetReferenceAreas() const noexcept
            { return m_referenceAreas; }
        /// @private
        void AddReferenceArea(Wisteria::GraphItems::ReferenceArea&& refArea)
            { m_referenceAreas.emplace_back(refArea); }
        /// @private
        [[nodiscard]] const GraphItems::Label& GetTitle() const noexcept
            { return m_title; }
        /// @private
        [[nodiscard]] const GraphItems::Label& GetSubtitle() const noexcept
            { return m_subtitle; }
        /// @private
        [[nodiscard]] const GraphItems::Label& GetCaption() const noexcept
            { return m_caption; }
        /// @private
        [[nodiscard]] const Wisteria::Canvas* GetCanvas() const noexcept
            { return m_parentCanvas; }
    protected:
        /// @returns The image drawn across all bars/boxes.
        [[nodiscard]] const wxBitmapBundle& GetCommonBoxImage() const noexcept
            { return m_commonBoxImage; }
        /// @returns The color used to outline images used for bars/boxes.
        [[nodiscard]] wxColour GetImageOulineColor() const noexcept
            { return m_imageOutlineColor; }
        /** @brief Updates the settings for a legend based on the provided hints.
                This should be called on a legend after it is constructed by
                a derived graph type.
            @param[in,out] legend The legend to adjust.
            @param hint A hint about where the legend will be placed.*/
        void AdjustLegendSettings(std::shared_ptr<GraphItems::Label>& legend,
                                  const LegendCanvasPlacementHint hint);
        /** @brief Adds information about any reference lines/areas in the graph onto the legend.
            @details This will be a separate section added to the bottom of the legend,
                with a separator line above it.
            @param[in,out] legend The legend to add the information to. This should already be
                constructed with any other text and icons it needs in it.
            @note Duplicate reference areas will be combined into one;
                This is useful for multiple instances of the same event on a plot
                (e.g., recessions).*/
        void AddReferenceLinesAndAreasToLegend(std::shared_ptr<GraphItems::Label>& legend) const;
        /// @returns A non-const version of the parent canvas.
        /// @details This should be used in derived classes when needing to call
        ///     `Canvas::CalcAllSizes()` or `Canvas::SetCanvasMinHeightDIPs()`.
        [[nodiscard]] Wisteria::Canvas* GetCanvas() noexcept
            { return m_parentCanvas; }
        /** @brief Adds an object (e.g., a polygon) to the plot to be rendered.
            @param object The object to add to the plot.
            @note The canvas of @c object is set to the plot's, but its scaling
                is preserved as objects may be using the parent's scaling or a different
                one, depending on how it was constructed.*/
        void AddObject(std::shared_ptr<GraphItems::GraphItemBase> object)
            {
            if (object != nullptr)
                {
                object->SetId(m_currentAssignedId++);
                object->SetDPIScaleFactor(GetDPIScaleFactor());
                m_plotObjects.push_back(object);
                }
            }
        /** @brief Sets the DPI scaling.
            @param scaling The DPI scaling.*/
        void SetDPIScaleFactor(const double scaling) override;

        /** @brief Draws the plot.
            @param dc The DC to draw to.
            @returns The bounding box of the plot.*/
        wxRect Draw(wxDC& dc) const override;
        /// @returns The rectangle on the canvas where the point would fit in.
        /// @param dc Measurement DC, which is not used in this implementation.
        [[nodiscard]] wxRect GetBoundingBox([[maybe_unused]] wxDC& dc) const final
            { return m_rect; }
        /// @returns The rectangular area of the plot area. This is relative to its parent canvas.
        [[nodiscard]] wxRect GetPlotAreaBoundingBox() const noexcept
            { return m_plotRect; }
        /// @returns The rectangular area of the plot area. This is relative to its parent canvas.
        [[nodiscard]] wxRect GetContentRect() const noexcept override
            { return GetPlotAreaBoundingBox(); }

        /** @brief Sets the rectangular area of the entire graph area.
            @param rect The rectangle to bound the entire plot to. This is relative to its parent canvas.
            @param dc Measurement DC, which is not used in this implementation.
            @param parentScaling This parameter is ignored.
            @note Derived classes should not need to call this except for special situations.\n
                For example, Table uses it to scale itself down so that it doesn't consume
                more space than needed.*/
        void SetBoundingBox(const wxRect& rect,
                            [[maybe_unused]] wxDC& dc,
                            [[maybe_unused]] const double parentScaling) noexcept final
            { m_rect = rect; }

        /** @brief Retrieves the coordinates on the canvas where the given point is at.
            @param xValue The x value of the point.
            @param yValue The y value of the point.
            @param[out] resultPt The coordinate on the canvas where the point is at. If the point is not
                within the plot (i.e., the x or y value is outside of the axes), then this will be (-1,-1).
            @returns `true` if the point could be found within the plot.*/
        bool GetPhyscialCoordinates(const double xValue, const double yValue, wxPoint& resultPt) const
            {
            if (GetBottomXAxis().GetPhysicalCoordinate(xValue, resultPt.x) &&
                GetLeftYAxis().GetPhysicalCoordinate(yValue, resultPt.y))
                { return true; }
            else
                {
                resultPt.x = -1;
                resultPt.y = -1;
                return false;
                }
            }
        /** @brief Retrieves the coordinates on the canvas where the given point is at.
            @param point The point to search for.
            @param[out] resultPt The coordinate on the canvas where the point is at. If the point is not
                within the plot (i.e., the x or y value is outside of the axes), then this will be (-1,-1).
            @returns `true` if the point could be found within the plot.*/
        bool GetPhyscialCoordinates(const wxPoint& point, wxPoint& resultPt) const
            { return GetPhyscialCoordinates(point.x, point.y, resultPt); };

        /** @brief Override this to perform plotting logic.
            @details The is the main interface for constructing the layout and object positioning
                for a plot. All plots should override this.
            @param dc The DC to measure content with.
            @note The base version of this should be called first in derived overrides
                so that the axis and gridlines are drawn.*/
        void RecalcSizes(wxDC& dc) override;

        void UpdateSelectedItems() final;

        /// @brief Additional info to show when selecting a plot in debug mode.
        wxString m_debugDrawInfoLabel;
    private:
        /// @brief Sets a non-const pointer to the parent canvas.
        /// @param canvas The parent canvas.
        void SetCanvas(Wisteria::Canvas* canvas) noexcept
            { m_parentCanvas = canvas; }

        struct EmbeddedObject
            {
            std::shared_ptr<GraphItems::GraphItemBase> m_object;
            wxPoint m_anchorPt;
            std::vector<wxPoint> m_interestPts;
            };

        /** @brief Moves the points by the specified x and y values.
            @param xToMove The amount to move horizontally.
            @param yToMove The amount to move vertically.*/
        void Offset(const int xToMove, const int yToMove) final
            {
            for (auto& object : m_plotObjects)
                { object->Offset(xToMove,yToMove); }
            for (auto& object : m_embeddedObjects)
                {
                if (object.m_object != nullptr)
                    { object.m_object->Offset(xToMove,yToMove); }
                }
            m_rect.Offset(wxPoint(xToMove, yToMove));
            m_plotRect.Offset(wxPoint(xToMove, yToMove));
            }
        /** @brief Draws the items label (if it has one) in the middle of the item if it is selected.
            @param dc The canvas to draw the item on.
            @param scaling This parameter is ignored.
            @param boundingBox This parameter is ignored.*/
        void DrawSelectionLabel(wxDC& dc, [[maybe_unused]] const double scaling,
                                [[maybe_unused]] const wxRect boundingBox) const final;
        /// @brief Unselects all objects on the plot.
        void ClearSelections() final
            {
            GraphItemBase::SetSelected(false);
            for (auto& object : m_plotObjects)
                {
                if (object->IsSelected())
                    { object->SetSelected(false); }
                }
            for (auto& object : m_embeddedObjects)
                {
                if (object.m_object != nullptr &&
                    object.m_object->IsSelected())
                    { object.m_object->SetSelected(false); }
                }
            }
        /** @brief Sets whether the plot is selected.
            @param selected Whether the last hit subobject should be selected.*/
        void SetSelected(const bool selected) final
            {
            GraphItemBase::SetSelected(selected);
            if (m_lastHitPointIndex < m_plotObjects.size())
                { m_plotObjects.at(m_lastHitPointIndex)->SetSelected(selected); }
            if (m_lastHitPointEmbeddedObjectIndex < m_embeddedObjects.size())
                { m_embeddedObjects.at(m_lastHitPointEmbeddedObjectIndex).m_object->SetSelected(selected); }
            }
        /** @returns `true` if @c pt is inside of plot area.
            @param pt The point to see that is in the plot.*/
        [[nodiscard]] bool HitTest(const wxPoint pt, wxDC& dc) const final
            { return GetBoundingBox(dc).Contains(pt); }
        /** @brief Selects the object at the given point (relative to the parent canvas),
                if there is an object at that point.
            @param pt The point to hit test.
            @returns `true` if something was selected at the given point.
            @note This will toggle the selection of an object, if it was selected before
                then it will become unselected.*/
        [[nodiscard]] bool SelectObjectAtPoint(const wxPoint& pt, wxDC& dc) final;
        /// @brief Calculates how much outer axis labels and headers go outside of the
        ///     axes' widths and heights (used to adjust the margins of the plot area).
        void GetAxesOverhang(long& leftMargin, long& rightMargin, long& topMargin, long& bottomMargin,
                             wxDC& dc) const;
        /// @brief Calculates how much space is needed around the plot to fit everything
        ///     (e.g., axes outer content, captions, etc.), resizes the plot area, and finally
        ///     recalculates the axes' points' positions.
        void AdjustPlotArea(wxDC& dc);

        wxRect m_rect;
        wxRect m_plotRect;
        bool m_mirrorXAxis{ false };
        bool m_mirrorYAxis{ false };
        std::vector<std::shared_ptr<GraphItems::GraphItemBase>> m_plotObjects;
        std::vector<EmbeddedObject> m_embeddedObjects;
        GraphItems::Label m_title;
        GraphItems::Label m_subtitle;
        GraphItems::Label m_caption;
        wxBitmapBundle m_stipple;

        std::map<wxString, wxVariant> m_properties;

        // not used by default, keep invalid
        wxColour m_bgColor;

        wxBitmapBundle m_commonBoxImage;
        wxColour m_imageOutlineColor{ *wxBLACK };

        Wisteria::Canvas* m_parentCanvas{ nullptr };
        Wisteria::GraphItems::Axis m_bottomXAxis{ AxisType::BottomXAxis };
        Wisteria::GraphItems::Axis m_topXAxis{ AxisType::TopXAxis };
        Wisteria::GraphItems::Axis m_leftYAxis{ AxisType::LeftYAxis };
        Wisteria::GraphItems::Axis m_rightYAxis{ AxisType::RightYAxis };
        std::vector<Wisteria::GraphItems::Axis> m_customAxes;
        std::vector<Wisteria::GraphItems::ReferenceLine> m_referenceLines;
        std::vector<Wisteria::GraphItems::ReferenceArea> m_referenceAreas;
        mutable size_t m_lastHitPointIndex{ static_cast<size_t>(-1) };
        mutable size_t m_lastHitPointEmbeddedObjectIndex{ static_cast<size_t>(-1) };

        // cached values
        long m_calculatedTopPadding{ 0 };
        long m_calculatedRightPadding{ 0 };
        long m_calculatedBottomPadding{ 0 };
        long m_calculatedLeftPadding{ 0 };

        long m_currentAssignedId{ 0 };
        std::map<long, std::set<long>> m_selectedItemsWithSubitems;
        };
    }

/** @}*/

#endif //__WISTERIA_PLOT2D_H__

