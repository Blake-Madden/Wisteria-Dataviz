/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_BOX_PLOT_H
#define WISTERIA_BOX_PLOT_H

#include "../data/jitter.h"
#include "graph2d.h"

namespace Wisteria::Graphs
    {
    /** @brief Box & Whisker plot, used for displaying data dispersion and outlier detection.

         %Data can either be plotted as a single box or split into grouped boxes.

         | Single Series   | Grouped |
         | :-------------- | :-------------------------------- |
         | @image html BoxPlot.svg width=90% | @image html GroupedBoxPlot.svg width=90% |

         Outliers are always displayed, and non-outlier points can be optionally
         displayed as well. If any points overlap, then bee-swarm jittering
         is used to show their distribution.

        @note For multi-box plots, set the pen (via GetPen()) to enable and customize
         the line connecting the boxes' midpoints. (This is turned off by default.)

        @par %Data:
         This plot accepts a Data::Dataset, where a continuous column is the
         dependent measurement. A categorical column can optionally be used to create
         separate boxes for different groups in the data.

        @par Missing Data:
         - Missing data in the group column will be shown as an empty label.
         - Missing data in the value column will be ignored (listwise deletion).

        @par Example:
        @code
         // "this" will be a parent wxWidgets frame or dialog, "canvas"
         // is a scrolled window derived object that will hold the box plot
         auto canvas = new Wisteria::Canvas(this);
         canvas->SetFixedObjectsGridSize(1, 1);

         // import the dataset (this is available in the "datasets" folder)
         auto mpgData = std::make_shared<Data::Dataset>();
         mpgData->ImportCSV(L"/home/daphne/data/mpg.csv",
            ImportInfo().ContinuousColumns({ L"hwy" }).
            CategoricalColumns({ { L"class", CategoricalImportMethod::ReadAsStrings } }));
         auto plot = std::make_shared<BoxPlot>(canvas);

         plot->SetData(mpgData, L"hwy", L"class");

         // Show all points (not just outliers).
         // The points within the boxes and whiskers will be
         // bee swarm jittered to visualize the distribution.
         plot->ShowAllPoints(true);

         canvas->SetFixedObject(0, 0, plot);
        @endcode
        @todo Add notch support.*/
    class BoxPlot final : public Graph2D
        {
        wxDECLARE_DYNAMIC_CLASS(BoxPlot);
        BoxPlot() = default;

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
                @param effect The effect to display across the box.
                @param boxCorners The corner display to use.
                @param opacity The box's opacity.*/
            BoxAndWhisker(const BoxEffect effect, const BoxCorners boxCorners,
                          const uint8_t opacity = wxALPHA_OPAQUE)
                : m_opacity(opacity), m_boxEffect(effect), m_boxCorners(boxCorners)
                {
                }

            /// @name Statistical Functions
            /// @brief Functions relating to the calculated statistics.
            /// @{

            /// @returns The middle line of the box (usually the median of the data).
            [[nodiscard]]
            double GetMiddlePoint() const noexcept
                {
                return m_middlePoint;
                }

            /// @returns The lower side of the box.
            [[nodiscard]]
            double GetLowerControlLimit() const noexcept
                {
                return m_lowerControlLimit;
                }

            /// @returns The upper side of the box.
            [[nodiscard]]
            double GetUpperControlLimit() const noexcept
                {
                return m_upperControlLimit;
                }

            /// @returns The value of the lower whisker (non-outlier range).
            ///     Any value lower than this is an outlier.
            [[nodiscard]]
            double GetLowerWhisker() const noexcept
                {
                return m_lowerWhisker;
                }

            /// @returns The value of the upper whisker (non-outlier range).
            ///     Any value larger than this is an outlier.
            [[nodiscard]]
            double GetUpperWhisker() const noexcept
                {
                return m_upperWhisker;
                }

            /// @}

            /// @name Box Display Functions
            /// @brief Functions relating to the visual display of the boxes.
            /// @{

            /// @returns The opacity (how opaque or translucent) the box is.
            [[nodiscard]]
            uint8_t GetOpacity() const noexcept
                {
                return m_opacity;
                }

            /** @brief Sets the opacity (how opaque or translucent) the box is.
                @param opacity The opacity level.*/
            void SetOpacity(const uint8_t opacity) noexcept { m_opacity = opacity; }

            /// @returns The effect (e.g., color gradient) displayed across the box(es).
            [[nodiscard]]
            BoxEffect GetBoxEffect() const noexcept
                {
                return m_boxEffect;
                }

            /** @brief Sets the effect displayed on the boxes.
                @param boxEffect The effect to use.
                @sa SetImageScheme().*/
            void SetBoxEffect(const BoxEffect boxEffect) noexcept { m_boxEffect = boxEffect; }

            /// @returns How the corners of the boxes are drawn.
            [[nodiscard]]
            BoxCorners GetBoxCorners() const noexcept
                {
                return m_boxCorners;
                }

            /** @brief Sets how the corners of the boxes are drawn.
                @param boxCorners The corner display to use.*/
            void SetBoxCorners(const BoxCorners boxCorners) noexcept { m_boxCorners = boxCorners; }

            /// @}

            /// @name Label & Point Functions
            /// @brief Functions relating to how labels and points are displayed.
            /// @{

            /// @returns @c true if displaying labels on the hinges, midpoint, and outliers.
            [[nodiscard]]
            bool IsShowingLabels() const noexcept
                {
                return m_displayLabels;
                }

            /// @brief Sets whether to display labels on the hinges, midpoint, and outliers.
            /// @param display Whether to display the labels.
            void ShowLabels(const bool display = true) noexcept { m_displayLabels = display; }

            /// @returns @c true if all data points are being displayed.
            [[nodiscard]]
            bool IsShowingAllPoints() const noexcept
                {
                return m_showAllPoints;
                }

            /// @brief Specifies whether to display all data points on the boxes and whiskers.
            /// @param display Whether to display the points.
            /// @note Outliers are always displayed as points.
            void ShowAllPoints(const bool display = true) noexcept { m_showAllPoints = display; }

            /// @}

          private:
            /// @name Data Functions
            /// @brief Functions relating to setting up the data and how the
            /// statistics are calculated.
            /// @{

            /** @brief Sets the data for the box.
                @details Also sets the percentiles used for the box range (must be between 1 and 49)
                    This would usually be 25, giving you the standard quartiles range.
                @param data The data to assign to the box.
                @param continuousColumnName The data column.
                @param groupColumnName  An optional categorical column to group the points by.
                @param groupId The group ID for this box. Data points from @c data will only be used
                    for his box if their group ID is @c groupId.
                    Has no effect if @c useGrouping is @c false.
                @param schemeIndex The index into the icon/color/brush schemes.
                @throws std::runtime_error If any columns can't be found by name, throws an
                    exception.\n The exception's @c what() message is UTF-8 encoded,
                    so pass it to @c wxString::FromUTF8() when formatting it for
                    an error message.*/
            void SetData(const std::shared_ptr<const Data::Dataset>& data,
                         const wxString& continuousColumnName,
                         const std::optional<wxString>& groupColumnName, Data::GroupIdType groupId,
                         size_t schemeIndex);

            /** @returns The data connected to the box.*/
            [[nodiscard]]
            const std::shared_ptr<const Data::Dataset>& GetDataset() const noexcept
                {
                return m_data;
                }

            /// @}

            /// @name Axis Functions
            /// @brief Functions relating to connecting the box to the parent plot.
            /// @{

            /// @returns The position on the x-axis (on the parent plot) that the box
            ///      is aligned with.
            [[nodiscard]]
            double GetXAxisPosition() const noexcept
                {
                return m_xAxisPosition;
                }

            /** @brief Sets where the box should be aligned on the x-axis.
                @param position The position on the x-axis to place the box.
                @note The parent box plot will manage this value, you do not need to call
                    them when creating a box.*/
            void SetXAxisPosition(const double position) noexcept { m_xAxisPosition = position; }

            /// @}

            /// @returns The box's index into the icon/color/brush schemes.
            [[nodiscard]]
            size_t GetSchemeIndex() const noexcept
                {
                return m_schemeIndex;
                }

            /// @brief Calculates the outlier and box ranges.
            void Calculate();

            [[nodiscard]]
            auto GetContinuousColumn(const wxString& continuousColumnName) const
                {
                auto continuousColumn = GetDataset()->GetContinuousColumn(continuousColumnName);
                if (continuousColumn == GetDataset()->GetContinuousColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': continuous column not found for box plot."),
                                         continuousColumnName)
                            .ToUTF8());
                    }
                return continuousColumn;
                }

            [[nodiscard]]
            auto GetGroupColumn(const std::optional<wxString>& groupColumnName) const
                {
                auto groupColumn =
                    (groupColumnName ? GetDataset()->GetCategoricalColumn(groupColumnName.value()) :
                                       GetDataset()->GetCategoricalColumns().cend());
                if (groupColumnName && groupColumn == GetDataset()->GetCategoricalColumns().cend())
                    {
                    throw std::runtime_error(
                        wxString::Format(_(L"'%s': group column not found for box plot."),
                                         groupColumnName.value())
                            .ToUTF8());
                    }
                return groupColumn;
                }

            bool m_displayLabels{ false };
            bool m_showAllPoints{ false };

            uint8_t m_opacity{ wxALPHA_OPAQUE };
            BoxEffect m_boxEffect{ BoxEffect::Solid };
            BoxCorners m_boxCorners{ BoxCorners::Straight };

            std::shared_ptr<const Data::Dataset> m_data{ nullptr };
            wxString m_continuousColumnName;
            std::optional<wxString> m_groupColumnName;

            Data::Jitter m_jitter{ AxisType::LeftYAxis };
            Data::GroupIdType m_groupId{ 0 };
            bool m_useGrouping{ false };
            size_t m_schemeIndex{ 0 };

            double m_xAxisPosition{ 2 };
            double m_middlePoint{ 0 };
            double m_lowerControlLimit{ 0 };
            double m_upperControlLimit{ 0 };
            double m_lowerWhisker{ 0 };
            double m_upperWhisker{ 0 };

            // drawing coordinates used by parent BoxPlot
            wxPoint m_lowerOutlierRangeCoordinate;
            wxPoint m_upperOutlierRangeCoordinate;
            wxPoint m_middleCoordinate;
            wxPoint m_lowerQuartileCoordinate;
            wxPoint m_upperQuartileCoordinate;
            wxRect m_boxRect;

          public:
            /// @private
            [[nodiscard]]
            bool operator<(const BoxAndWhisker& that) const
                {
                if (!m_useGrouping || !that.m_useGrouping)
                    {
                    return false;
                    }
                auto groupColumn = GetGroupColumn(m_groupColumnName);
                auto thatGroupColumn = that.GetGroupColumn(that.m_groupColumnName);
                return wxUILocale::GetCurrent().CompareStrings(
                           groupColumn->GetLabelFromID(m_groupId),
                           thatGroupColumn->GetLabelFromID(that.m_groupId),
                           wxCompare_CaseInsensitive) < 0;
                }
            };

        /** @brief Constructor.
            @param canvas The canvas to draw the plot on.
            @param brushes The brushes to paint the boxes with.
            @param colors The base color scheme to paint under the boxes' brushes.\n
                This will only have a noticeable effect if the brush is non-solid (e.g., hatched).
            @param shapes The shape scheme to use for the points.\n
                Leave as @c nullptr to use the standard shapes.*/
        explicit BoxPlot(
            Canvas* canvas, const std::shared_ptr<Brushes::Schemes::BrushScheme>& brushes = nullptr,
            const std::shared_ptr<Colors::Schemes::ColorScheme>& colors = nullptr,
            const std::shared_ptr<Wisteria::Icons::Schemes::IconScheme>& shapes = nullptr);

        /** @brief Sets the data.
            @details Also sets the percentiles used for the box range (must be between 1 and 49)
                This would usually be 25, giving you the standard quartiles range.\n
                If multiple groups are found in the data's grouping column, then separate boxes will
                be created for each group.
            @param data The data to use for the plot.
            @param continuousColumnName The column from the dataset to analyze.
            @param groupColumnName The group column to split the data into (this is optional).
            @note Call the parent canvas's `CalcAllSizes()` when setting to a new dataset to
                re-plot the data.
            @throws std::runtime_error If any columns can't be found by name, throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to @c
                wxString::FromUTF8() when formatting it for an error message.*/
        void SetData(const std::shared_ptr<const Data::Dataset>& data,
                     const wxString& continuousColumnName,
                     const std::optional<wxString>& groupColumnName = std::nullopt);

        /// @name Box Functions
        /// @brief Functions relating to the boxes.
        /// @{

        /// @returns The box at given index.
        /// @param index The index of the box to access.
        [[nodiscard]]
        BoxAndWhisker& GetBox(const size_t index)
            {
            assert(index < m_boxes.size() && L"Invalid index in GetBox()!");
            return m_boxes.at(index);
            }

        /// @returns The number of boxes (i.e., data distributions) being plotted.
        [[nodiscard]]
        size_t GetBoxCount() const noexcept
            {
            return m_boxes.size();
            }

        /// @}

        /// @name Label Functions
        /// @brief Functions relating to labels drawn on the plot.
        /// @{

        /// @returns The numeric precision displayed on the labels.
        [[nodiscard]]
        uint8_t GetLabelPrecision() const noexcept
            {
            return m_labelPrecision;
            }

        /// @brief Sets the numeric precision for labels in the plot. (Default is `1`.)
        /// @param precision The precision to use for the labels.
        void SetLabelPrecision(const uint8_t precision) noexcept { m_labelPrecision = precision; }

        /// @}

        /// @name Legend Functions
        /// @brief Functions relating to drawing a legend.
        /// @{

        /// @returns Whether a legend is included directly on the plot (when showing just one box).
        [[nodiscard]]
        bool IsOverlayingLegend() const noexcept
            {
            return m_overlayLegend;
            }

        /** @brief Overlays a legend on top of the plot.
            @param overlay @c true to include the legend on the plot.
            @note This only applies when a displaying single-series box.*/
        void IncludeOverlayingLegend(const bool overlay) noexcept { m_overlayLegend = overlay; }

        /** @brief Builds and returns a legend for single-box plots,
                showing the various statistics.
            @details This can then be managed by the parent canvas and placed next to the plot.
            @param options The options for how to build the legend.\n
                Legend placement is the only field used here.
            @returns The legend for the plot.
            @sa IncludeOverlayingLegend().
            @note By default, this legend will be created and laid on top of the plot
                if a single box plot.\n
                For multi-group plots, null will be returned.*/
        [[nodiscard]]
        std::unique_ptr<GraphItems::Label> CreateLegend(const LegendOptions& options) final;

        /// @}

        /// @name Box Effect Functions
        /// @brief Functions relating to the effects used to draw the boxes.
        /// @note GetBox() can also be used after a call to SetData() to
        ///     customize the effect of a specific box.
        /// @{

        /// @returns The opacity (how opaque or translucent) the box is.
        [[nodiscard]]
        uint8_t GetOpacity() const noexcept
            {
            return m_opacity;
            }

        /** @brief Sets the opacity (how opaque or translucent) the box is.
            @param opacity The opacity level.*/
        void SetOpacity(const uint8_t opacity) noexcept
            {
            for (auto& box : m_boxes)
                {
                box.SetOpacity(opacity);
                }
            m_opacity = opacity;
            }

        /// @returns The effect (e.g., color gradient) displayed across the box(es).
        [[nodiscard]]
        BoxEffect GetBoxEffect() const noexcept
            {
            return m_boxEffect;
            }

        /** @brief Sets the effect displayed on the boxes.
            @param boxEffect The effect to use.*/
        void SetBoxEffect(const BoxEffect boxEffect) noexcept
            {
            for (auto& box : m_boxes)
                {
                box.SetBoxEffect(boxEffect);
                }
            m_boxEffect = boxEffect;
            }

        /// @returns How the corners of the boxes are drawn.
        [[nodiscard]]
        BoxCorners GetBoxCorners() const noexcept
            {
            return m_boxCorners;
            }

        /** @brief Sets how the corners of the boxes are drawn.
            @param boxCorners The corner display to use.
            @note Rounded corners are only applied if drawing with BoxEffect::Solid.*/
        void SetBoxCorners(const BoxCorners boxCorners) noexcept { m_boxCorners = boxCorners; }

        /// @}

        /// @name Label & Point Functions
        /// @brief Functions relating to how labels and points are displayed.
        /// @{

        /// @returns @c true if displaying labels on the hinges, midpoint, and outliers.
        [[nodiscard]]
        bool IsShowingLabels() const noexcept
            {
            return m_displayLabels;
            }

        /// @brief Sets whether to display labels on the hinges, midpoint, and outliers.
        /// @param display Whether to display the labels.
        void ShowLabels(const bool display = true) noexcept
            {
            for (auto& box : m_boxes)
                {
                box.ShowLabels(display);
                }
            m_displayLabels = display;
            }

        /// @returns @c true if all data points are being displayed.
        [[nodiscard]]
        bool IsShowingAllPoints() const noexcept
            {
            return m_showAllPoints;
            }

        /// @brief Specifies whether to display all data points on the boxes and whiskers.
        /// @param display Whether to display the points.
        /// @note Outliers are always displayed as points.
        void ShowAllPoints(const bool display = true) noexcept
            {
            for (auto& box : m_boxes)
                {
                box.ShowAllPoints(display);
                }
            m_showAllPoints = display;
            }

        /// @returns The default color of the points.
        [[nodiscard]]
        wxColour GetPointColor() const noexcept
            {
            return m_pointColour;
            }

        /** @brief Sets the default color of the points.
            @param color The color to use.*/
        void SetPointColor(const wxColour& color) noexcept
            {
            if (color.IsOk())
                {
                m_pointColour = color;
                }
            }

        /// @}

        /// @private
        [[nodiscard]]
        const BoxAndWhisker& GetBox(const size_t index) const
            {
            return m_boxes[index];
            }

      private:
        /** @brief Adds a box to the plot.
            @param box The box to draw.
            @note If only one box is on the plot, then no labels will be shown on the bottom x-axis
                (even if a custom label is provided for where the box is). To override this,
                call `GetBottomXAxis().SetLabelDisplay(...)` after adding the box and
                its custom label.*/
        void AddBox(const BoxAndWhisker& box);
        void RecalcSizes(wxDC& dc) final;

        [[nodiscard]]
        auto GetGroupColumn(const std::optional<wxString>& groupColumnName) const
            {
            auto groupColumn =
                (groupColumnName ? GetDataset()->GetCategoricalColumn(groupColumnName.value()) :
                                   GetDataset()->GetCategoricalColumns().cend());
            if (groupColumnName && groupColumn == GetDataset()->GetCategoricalColumns().cend())
                {
                throw std::runtime_error(
                    wxString::Format(_(L"'%s': group column not found for box plot."),
                                     groupColumnName.value())
                        .ToUTF8());
                }
            return groupColumn;
            }

        std::vector<BoxAndWhisker> m_boxes;
        bool m_overlayLegend{ true };
        uint8_t m_labelPrecision{ 1 };

        std::optional<wxString> m_groupColumn;
        wxString m_continuousColumn;

        uint8_t m_opacity{ wxALPHA_OPAQUE };
        BoxEffect m_boxEffect{ BoxEffect::Solid };

        wxColour m_pointColour{ Colors::ColorBrewer::GetColor(Colors::Color::CelestialBlue) };
        BoxCorners m_boxCorners{ BoxCorners::Straight };
        bool m_displayLabels{ false };
        bool m_showAllPoints{ false };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_BOX_PLOT_H
