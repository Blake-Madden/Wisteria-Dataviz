/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_BOX_PLOT_H__
#define __WISTERIA_BOX_PLOT_H__

#include "graph2d.h"
#include "colorbrewer.h"
#include "jitter.h"

namespace Wisteria::Graphs
    {
    /** @brief Box & Whisker plot, used for displaying the dispersion of data and
         for detecting outliers.

         %Data can either be plotted as a single box or split into grouped boxes.

         | Single Series   | Grouped |
         | :-------------- | :-------------------------------- |
         | @image html BoxPlot.svg width=90% | @image html GroupedBoxPlot.svg width=90% |

         Outliers are always displayed, and non-outlier points can be optionally
         displayed as well. If non-outlier points overlap, then bee-swarm jittering
         is used to show their distribution.

        @note For multi-box plots, set the pen (via GetPen()) to enable and customize
         the line connecting the boxes' midpoints. (This is turned off by default.)
        @par %Data:
         This plot accepts a Data::Dataset, where the a continuous column is the
         dependent measurement. A categorical column can optionally be used to create
         separate boxes for different groups in the data.
        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog, "canvas"
         // is a scrolled window derived object that will hold the box plot
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 2);

         // import the dataset (this is available in the "datasets" folder)
         auto quarterlyPerformanceData = std::make_shared<Data::Dataset>();
         quarterlyPerformanceData->ImportCSV(L"Performance Reviews.csv",
            ImportInfo().ContinuousColumns({ L"PERF" }).
            CategoricalColumns({ { L"QUARTER", CategoricalImportMethod::ReadAsStrings } }));
         auto plot = std::make_shared<BoxPlot>(canvas,
            // use a non-default color scheme
            std::make_shared<Colors::Schemes::October>());

         plot->SetData(quarterlyPerformanceData, L"PERF, L"QUARTER", 25);
         // customize a box's appearance
         plot->GetBox(1).SetBoxEffect(BoxEffect::Glassy);
         // Show all points (not just outliers).
         // The points within the boxes and whiskers will be
         // bee swarm jittering to visualize the distribution.
         plot->ShowAllPoints(true);

         canvas->SetFixedObject(0, 0, plot);
         canvas->SetFixedObject(0, 1, plot->CreateLegend(
                                LegendCanvasPlacementHint::RightOrLeftOfGraph));
        @endcode
        @todo Add notch support.*/
    class BoxPlot final : public Graph2D
        {
    public:
        /** @brief A box & whisker used for displaying distribution (and ranges) of data.*/
        class BoxAndWhisker
            {
            /// BoxPlot needs access to values the that client constructing the box doesn't need.
            friend class BoxPlot;
        public:
            /// @private
            BoxAndWhisker() = default;
            /** @brief Constructor.
                @param boxColor The color of the boxes.
                @param effect The effect to display across the boxes.
                @param boxCorners The corner display to use.
                @param opacity The boxes' opacity.*/
            BoxAndWhisker(const wxColor& boxColor, const BoxEffect effect,
                          const BoxCorners boxCorners,
                          const uint8_t opacity = wxALPHA_OPAQUE)
                    : m_boxColor(boxColor), m_opacity(opacity),
                      m_boxEffect(effect), m_boxCorners(boxCorners)
                {}

            /// @name Statistics Functions
            /// @brief Functions relating to the calculated statistics.
            /// @{

            /// @returns The middle line of the box (usually the median of the data).
            [[nodiscard]] double GetMiddlePoint() const noexcept
                { return m_middlePoint; }
            /// @returns The lower side of the box.
            [[nodiscard]] double GetLowerControlLimit() const noexcept
                { return m_lowerControlLimit; }
            /// @returns The upper side of the box.
            [[nodiscard]] double GetUpperControlLimit() const noexcept
                { return m_upperControlLimit; }
            /// @returns The value of the lower whisker (non-outlier range).
            //  Any value lower than this is an outlier.
            [[nodiscard]] double GetLowerWhisker() const noexcept
                { return m_lowerWhisker; }
            /// @returns The value of the upper whisker (non-outlier range).
            ///  Any value larger than this is an outlier.
            [[nodiscard]] double GetUpperWhisker() const noexcept
                { return m_upperWhisker; }
            /// @}

            /// @name Box Display Functions
            /// @brief Functions relating to the visual display of the boxes.
            /// @{

            /// @returns The box(es) color.
            [[nodiscard]] wxColour GetBoxColor() const noexcept
                { return m_boxColor; }
            /// @brief Sets the color for the box(es).
            /// @param color The color to set the box.
            void SetBoxColor(const wxColour color) noexcept
                { m_boxColor = color; }

            /// @returns The opacity (how opaque or translucent) the box is.
            [[nodiscard]] uint8_t GetOpacity() const noexcept
                { return m_opacity; }
            /** @brief Sets he opacity (how opaque or translucent) the box is.
                @param opacity The opacity level.*/
            void SetOpacity(const uint8_t opacity) noexcept
                { m_opacity = opacity; }

            /// @returns The effect (e.g., color gradient) displayed across the box(es).
            [[nodiscard]] BoxEffect GetBoxEffect() const noexcept
                { return m_boxEffect; }
            /** @brief Sets the effect displayed on the boxes.
                @param boxEffect The effect to use.*/
            void SetBoxEffect(const BoxEffect boxEffect) noexcept
                { m_boxEffect = boxEffect; }
            /// @returns How the corners of the boxes are drawn.
            [[nodiscard]] BoxCorners GetBoxCorners() const noexcept
                { return m_boxCorners; }
            /** @brief Sets how the corners of the boxes are drawn.
                @param boxCorners The corner display to use.*/
            void SetBoxCorners(const BoxCorners boxCorners) noexcept
                { m_boxCorners = boxCorners; }
            /// @}

            /// @name Label & Point Functions
            /// @brief Functions relating to how labels and points are displayed.
            /// @{

            /// @returns `true` if displaying labels on the hinges, midpoint, and outliers.
            [[nodiscard]] bool IsShowingLabels() const noexcept
                { return m_displayLabels; }
            /// @brief Sets whether to display labels on the hinges, midpoint, and outliers.
            /// @param display Whether to display the labels.
            void ShowLabels(const bool display = true) noexcept
                { m_displayLabels = display; }

            /// @returns `true` if all data points are being displayed.
            [[nodiscard]] bool IsShowingAllPoints() const noexcept
                { return m_showAllPoints; }
            /// @brief Specifies whether to display all data points on the boxes and whiskers.
            /// @param display Whether to display the points.
            /// @note Outliers are always displayed as points.
            void ShowAllPoints(const bool display = true) noexcept
                { m_showAllPoints = display; }
            /// @}
        private:
            /// @name Data Functions
            /// @brief Functions relating to setting up the data and how the statistics are calculated.
            /// @{

            /** @brief Sets the data for the box.
                @details Also sets the percentiles used for the box range (must be between 1 and 49)
                 This would usually be 25, giving you the standard quartiles range.
                @param data The data to assign to the box.
                @param useGrouping Whether to filter the data to a specific group ID for this box.
                @param groupId The group ID for this box. Data points from @c data will only be used for
                 his box if their group ID is @c groupId. Has no effect if @c useGrouping if `false`.
                @param percentileCoefficient The percentile coefficient to use.
                @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
            void SetData(std::shared_ptr<const Data::Dataset> data,
                         const wxString& continuousColumnName,
                         std::optional<const wxString> groupColumnName,
                         const Data::GroupIdType groupId,
                         uint8_t percentileCoefficient);

            /** @returns The data connected to the box.*/
            [[nodiscard]] const std::shared_ptr<const Data::Dataset>& GetData() const noexcept
                { return m_data; }
            /// @}

            /// @name Axis Functions
            /// @brief Functions relating to connecting the box to the parent plot.
            /// @{

            /// @returns The position on the X axis (on the parent plot) that the box
            //   is aligned with.
            [[nodiscard]] double GetXAxisPosition() const noexcept
                { return m_xAxisPosition; }
            /** @brief Sets where the box should be aligned on the X axis.
                @param position The position on the X axis to place the box.
                @note The parent box plot will manage this value, you do not need to call
                 them when creating a box.*/
            void SetXAxisPosition(const double position) noexcept
                { m_xAxisPosition = position; }
            /// @}

            /// @returns The percentile coefficient.
            /// @note To change this, call SetData() with the new coefficient and it will
            ///  recalculate everything.
            [[nodiscard]] double GetPercentileCoefficient() const noexcept
                { return m_percentileCoefficient; }
            /// @brief Calculates the outlier and box ranges.
            void Calculate();

            bool m_displayLabels{ false };
            bool m_showAllPoints{ false };

            wxColour m_boxColor{ *wxGREEN };
            uint8_t m_opacity{ wxALPHA_OPAQUE };
            BoxEffect m_boxEffect{ BoxEffect::Solid };
            BoxCorners m_boxCorners{ BoxCorners::Straight };

            std::shared_ptr<const Data::Dataset> m_data;
            std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
            std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;
            wxString m_continuousColumnName;
            std::optional<wxString> m_groupColumnName;

            Data::Jitter m_jitter{ AxisType::LeftYAxis };
            Data::GroupIdType m_groupId{ 0 };
            bool m_useGrouping{ false };

            double m_xAxisPosition{ 2 };
            double m_middlePoint{ 0 };
            double m_lowerControlLimit{ 0 };
            double m_upperControlLimit{ 0 };
            double m_lowerWhisker{ 0 };
            double m_upperWhisker{ 0 };
            double m_percentileCoefficient{ .25f };

            // drawing coordinates used by parent BoxPlot
            wxPoint m_lowerOutlierRangeCoordinate;
            wxPoint m_upperOutlierRangeCoordinate;
            wxPoint m_middleCoordinate;
            wxPoint m_lowerQuartileCoordinate;
            wxPoint m_upperQuartileCoordinate;
            wxRect m_boxRect;
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param colors The color scheme to apply to the points. Leave as null to use the default theme.
            @param shapes The shape scheme to use for the points. Leave as null to use the standard shapes.*/
        explicit BoxPlot(Canvas* canvas,
            std::shared_ptr<Colors::Schemes::ColorScheme> colors = nullptr,
            std::shared_ptr<IconShapeScheme> shapes = nullptr);

        /** @brief Sets the data.
            @details Also sets the percentiles used for the box range (must be between 1 and 49)
             This would usually be 25, giving you the standard quartiles range.

             If multiple groups are found in the data's grouping column, then separate boxes will
             be created for each group.
            @param data The data to use for the plot.
            @param continuousColumnName The column from the dataset to analyze.
            @param groupColumnName The group column to split the data into (this is optional).
            @param percentileCoefficient The percentile coefficient to use.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.*/
        void SetData(std::shared_ptr<const Data::Dataset> data,
                     const wxString& continuousColumnName,
                     std::optional<const wxString> groupColumnName = std::nullopt,
                     uint8_t percentileCoefficient = 25);

        /// @name Box Functions
        /// @brief Functions relating to the boxes.
        /// @{

        /// @returns The box at given index.
        /// @param index The index of the box to access.
        [[nodiscard]] BoxAndWhisker& GetBox(const size_t index)
            {
            wxASSERT_LEVEL_2_MSG(index < m_boxes.size(), L"Invalid index in GetBox()!");
            return m_boxes.at(index);
            }
        /// @returns The number of boxes (i.e., data distributions) being plotted.
        [[nodiscard]] size_t GetBoxCount() const noexcept
            { return m_boxes.size(); }
        /// @}

        /// @name Label Functions
        /// @brief Functions relating to labels drawn on the plot.
        /// @{

        /// @returns The numeric precision displayed on the labels.
        [[nodiscard]] uint8_t GetLabelPrecision() const noexcept
            { return m_labelPrecision; }
        /// @brief Sets the numeric precision for labels in the plot. (Default is 1.)
        /// @param precision The precision to use for the labels.
        void SetLabelPrecision(const uint8_t precision) noexcept
            { m_labelPrecision = precision; }
        /// @}

        /// @name Legend Functions
        /// @brief Functions relating to drawing a legend.
        /// @{

        /// @returns Whether a legend is included directly on the plot (when showing just one box).
        [[nodiscard]] bool IsOverlayingLegend() const noexcept
            { return m_overlayLegend; }
        /** @brief Overlays a legend on top of the plot.
            @param overlay `true` to include the legend on the plot.
            @note This only applies when a displaying single-series box.*/
        void IncludedOverlayingLegend(const bool overlay) noexcept
            { m_overlayLegend = overlay; }
        /** @brief Builds and returns a legend using the current colors and labels.
             @details This can be then be managed by the parent canvas and placed next to the plot.
             @param hint A hint about where the legend will be placed after construction. This is used
              for defining the legend's padding, outlining, canvas proportions, etc.
             @returns The legend for the plot.
             @sa IncludedOverlayingLegend().
             @note By default, this legend will be created and laid on top of the plot if a single box plot.

              For multi-group plots, the legend will use colored boxes if the color scheme is
              using more than one column (meaning that the boxes are different colors).
              If the boxes are all the same color, then the groups' point shapes are used on legend instead.*/
        [[nodiscard]] std::shared_ptr<GraphItems::Label> CreateLegend(
            const LegendCanvasPlacementHint hint) const;
        /// @}

        /// @name Box Effect Functions
        /// @brief Functions relating to the effects used to draw the boxes.
        /// @note Use the color and shape schemes in the constructor to control the color
        ///  and shapes of the boxes and its points. GetBox() can also be used after a call to
        ///  SetData() to customize the appearance of a box.
        /// @{

        /// @returns The opacity (how opaque or translucent) the box is.
        [[nodiscard]] uint8_t GetOpacity() const noexcept
            { return m_opacity; }
        /** @brief Sets he opacity (how opaque or translucent) the box is.
            @param opacity The opacity level.*/
        void SetOpacity(const uint8_t opacity) noexcept
            {
            for (auto& box : m_boxes)
                { box.SetOpacity(opacity); }
            m_opacity = opacity;
            }

        /// @returns The effect (e.g., color gradient) displayed across the box(es).
        [[nodiscard]] BoxEffect GetBoxEffect() const noexcept
            { return m_boxEffect; }
        /** @brief Sets the effect displayed on the boxes.
            @param boxEffect The effect to use.*/
        void SetBoxEffect(const BoxEffect boxEffect) noexcept
            {
            for (auto& box : m_boxes)
                { box.SetBoxEffect(boxEffect); }
            m_boxEffect = boxEffect;
            }

        /// @brief Sets a common image to be drawn just within the box areas.
        /// @param boxesImage The image to draw across the boxes.
        /// @param outlineColor The outline color of the boxes.
        /// @note This effect will only apply to bars using the @c CommonImage effect. 
        void SetCommonBoxImage(std::shared_ptr<const wxImage> boxesImage,
                               const wxColour& outlineColor) noexcept
            {
            m_boxesImage = boxesImage;
            m_imageOutlineColor = outlineColor;
            }

        /// @returns How the corners of the boxes are drawn.
        [[nodiscard]] BoxCorners GetBoxCorners() const noexcept
            { return m_boxCorners; }
        /** @brief Sets how the corners of the boxes are drawn.
            @param boxCorners The corner display to use.
            @note Rounded corners are only applied if drawing with BoxEffect::Solid.*/
        void SetBoxCorners(const BoxCorners boxCorners) noexcept
            { m_boxCorners = boxCorners; }
        /// @}

        /// @name Label & Point Functions
        /// @brief Functions relating to how labels and points are displayed.
        /// @{

        /// @returns `true` if displaying labels on the hinges, midpoint, and outliers.
        [[nodiscard]] bool IsShowingLabels() const noexcept
            { return m_displayLabels; }
        /// @brief Sets whether to display labels on the hinges, midpoint, and outliers.
        /// @param display Whether to display the labels.
        void ShowLabels(const bool display = true) noexcept
            {
            for (auto& box : m_boxes)
                { box.ShowLabels(display); }
            m_displayLabels = display;
            }

        /// @returns `true` if all data points are being displayed.
        [[nodiscard]] bool IsShowingAllPoints() const noexcept
            { return m_showAllPoints; }
        /// @brief Specifies whether to display all data points on the boxes and whiskers.
        /// @param display Whether to display the points.
        /// @note Outliers are always displayed as points.
        void ShowAllPoints(const bool display = true) noexcept
            {
            for (auto& box : m_boxes)
                { box.ShowAllPoints(display); }
            m_showAllPoints = display;
            }

        /// @returns Access to the brush used to draw the outliers.
        [[nodiscard]] wxBrush& GetOutlierPointsBrush() noexcept
            { return m_outlierPointsBrush; }
        /// @returns Access to the brush used to draw the points.
        [[nodiscard]] wxBrush& GetPointsBrush() noexcept
            { return m_pointsBrush; }
        /// @}

        /// @private
        [[nodiscard]] const BoxAndWhisker& GetBox(const size_t index) const
            { return m_boxes[index]; }
    private:
        /// @returns The image drawn across all bars.
        [[nodiscard]] const std::shared_ptr<const wxImage>& GetCommonBoxImage() const noexcept
            { return m_boxesImage; }
        /** @brief Adds a box to the plot.
            @param box The box to draw.
            @note If only one box is on the plot, then no labels will be shown on the bottom X axis
             (even if a custom label is provided for where the box is). To override this,
             call GetBottomXAxis().ShowAxisLabels(true) after adding the box and its custom label.*/
        void AddBox(const BoxAndWhisker& box);
        void RecalcSizes() final;

        /// @brief Get the shape scheme used for the points.
        /// @returns The shape scheme used for the points.
        [[nodiscard]] std::shared_ptr<IconShapeScheme>& GetShapeScheme() noexcept
            { return m_shapeScheme; }
        /// @private
        [[nodiscard]] const std::shared_ptr<IconShapeScheme>& GetShapeScheme() const noexcept
            { return m_shapeScheme; }

        /// @brief Get the color scheme used for the points.
        /// @returns The color scheme used for the points.
        [[nodiscard]] std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() noexcept
            { return m_colorScheme; }
        /// @private
        [[nodiscard]] const std::shared_ptr<Colors::Schemes::ColorScheme>& GetColorScheme() const noexcept
            { return m_colorScheme; }

        std::vector<BoxAndWhisker> m_boxes;
        bool m_overlayLegend{ true };
        uint8_t m_labelPrecision{ 1 };

        std::shared_ptr<const Data::Dataset> m_data;
        std::vector<Wisteria::Data::ColumnWithStringTable>::const_iterator m_groupColumn;
        std::vector<Wisteria::Data::Column<double>>::const_iterator m_continuousColumn;

        std::shared_ptr<Colors::Schemes::ColorScheme> m_colorScheme;
        std::shared_ptr<IconShapeScheme> m_shapeScheme;
        wxBrush m_outlierPointsBrush{
            Colors::ColorBrewer::GetColor(Colors::Color::Red) };
        wxBrush m_pointsBrush{
            Colors::ColorBrewer::GetColor(Colors::Color::CarolinaBlue) };
        uint8_t m_opacity{ wxALPHA_OPAQUE };
        BoxEffect m_boxEffect{ BoxEffect::Solid };
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        bool m_displayLabels{ false };
        bool m_showAllPoints{ false };
        std::shared_ptr<const wxImage> m_boxesImage{ nullptr };
        wxColour m_imageOutlineColor{ *wxBLACK };
        };
    }

/** @}*/

#endif //__WISTERIA_BOX_PLOT_H__
