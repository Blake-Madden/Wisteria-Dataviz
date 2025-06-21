/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_BARCHART_H
#define WISTERIA_BARCHART_H

#include "groupgraph2d.h"
#include <numeric>
#include <optional>

namespace Wisteria::Graphs
    {
    /** @brief %Bar chart, which charts data as bars horizontally or vertically along a 2D axis.

         | Regular   | Stylized |
         | :-------------- | :-------------------------------- |
         | @image html BarChart.svg width=90% | @image html BarChartStylized.svg width=90% |

         | Image Background   |
         | :-------------- |
         | @image html BarChartImage.svg width=90% |

        @note This class is a base-level, generic interface for constructing a bar chart,
         which does not have an interface for datasets.\n
         \n
         If you want to create a bar chart that aggregates the counts of discrete values
         from a continuous variable, then histograms offer this ability. Refer to the
         @c BinUniqueValues binning method in the Histogram documentation to learn more.\n
         \n
         Likewise, to create a bar chart that aggregates counts of labels from a categorical
         variable, then categorical bar charts offer this ability as well.
         Refer to CategoricalBarChart for further details.\n
         \n
         Both Histogram and CategoricalBarChart have @c SetData() methods for working
         with a dataset, which will perform the aggregations for you.

        @par Missing Data:
         Because this class does not work with datasets, MD handling is not applicable.

        @par Example:
        @code
         auto plot = std::make_shared<BarChart>(canvas);

         // make it a horizontal barchart
         plot->SetBarOrientation(Orientation::Horizontal);

         auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

         plot->AddBar(BarChart::Bar(1,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(92).Brush(barColor))
            },
            wxString{}, Label(_(L"Bugs")), BoxEffect::Solid) );

         plot->AddBar(BarChart::Bar(2,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor))
            },
            wxString{}, Label(_(L"Pending feature requests")), BoxEffect::Solid));

         plot->AddBar(BarChart::Bar(3,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor))
            },
            wxString{}, Label(_(L"Unfinished help topics")), BoxEffect::Solid));

         plot->AddBar(BarChart::Bar(4,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor))
            },
            wxString{}, Label(_(L"Missing unit tests")), BoxEffect::Solid));

        plot->IncludeSpacesBetweenBars();

         // only show the labels on the axis
         plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);

         plot->GetBarAxis().GetTitle().GetGraphItemInfo().Text(L"ISSUES");

         canvas->SetFixedObject(0, 0, plot);
        @endcode

        @par Stylized Example:
        @code
         auto plot = std::make_shared<BarChart>(canvas);

         // make it a horizontal barchart
         plot->SetBarOrientation(Orientation::Horizontal);

         auto barColor = ColorBrewer::GetColor(Color::OceanBoatBlue);

         plot->AddBar(BarChart::Bar(1,
            {
            // this bar will have two sections to it, where a red section
            // refers to the more critical bugs
            BarChart::BarBlock(BarChart::BarBlockInfo(22).Brush(*wxRED)),
            BarChart::BarBlock(BarChart::BarBlockInfo(72).Brush(barColor))
            },
            wxString{}, Label(_(L"Bugs")), BoxEffect::Glassy,
            // we will make the width of the bar twice as wide as the others
            // to show how important it is
            wxALPHA_OPAQUE, 2) );

         // Note that because the first bar has an unusual width, this will offset
         // the positions of the following bars. Therefore, we need to place them
         // at positions like 2.5, 3.5, etc. Normally, they would just go on points like 2 or 3.
         plot->AddBar(BarChart::Bar(2.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(32).Brush(barColor))
            },
            wxString{}, Label(_(L"Pending feature requests")), BoxEffect::Glassy,
            // this bar will be translucent
            75, 1));

         plot->AddBar(BarChart::Bar(3.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(12).Brush(barColor))
            },
            wxString{}, Label(_(L"Unfinished help topics")), BoxEffect::Glassy,
            wxALPHA_OPAQUE, 1));

         plot->AddBar(BarChart::Bar(4.5,
            {
            BarChart::BarBlock(BarChart::BarBlockInfo(107).Brush(barColor))
            },
            wxString{}, Label(_(L"Missing unit tests")), BoxEffect::Glassy,
            wxALPHA_OPAQUE, 1));

         // only show the labels on the axis
         plot->GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
         // force the custom labels set at points like 2.5 to be shown
         const auto [rangeStart, rangeEnd] = plot->GetBarAxis().GetRange();
         plot->GetBarAxis().SetRange(rangeStart, rangeEnd, 1, 0.5, 1);

         // make the left axis title more stylized
         plot->GetBarAxis().GetTitle().GetGraphItemInfo().
            Text(L"ISSUES").Orient(Orientation::Horizontal).Padding(5, 10, 0, 0).
            LabelAlignment(TextAlignment::Centered);
         plot->GetBarAxis().GetTitle().SplitTextByCharacter();

         // align the axis labels over to the left
         plot->GetBarAxis().SetPerpendicularLabelAxisAlignment(
            AxisLabelAlignment::AlignWithBoundary);

         canvas->SetFixedObject(0, 0, plot);
        @endcode
        @sa The [bar chart](../../BarChart.md) overview for more information.
    */
    class BarChart : public GroupGraph2D
        {
        wxDECLARE_DYNAMIC_CLASS(BarChart);
        BarChart() = default;

      public:
        /// @brief Ways to compare bars.
        enum class BarSortComparison
            {
            SortByBarLength, /*!< Compare bars based on length.*/
            SortByAxisLabel  /*!< Compare bars lexicographically on their labels.*/
            };

        /// @brief Shapes for the bars.
        enum class BarShape
            {
            Rectangle, /*!< A rectangle.*/
            Arrow,     /*!< An arrow.*/
            /// @private
            BARSHAPE_COUNT
            };

        class BarBlock; // forward declare

        /// @brief Helper class for constructing a BarBlock.
        /// @details This class can chain parameters together and then be passed
        ///    to a BarBlock constructor.
        class BarBlockInfo
            {
            friend class BarBlock;

          public:
            /// @private
            BarBlockInfo() = default;

            /// @brief Constructor which takes a bar block length (along the bar axis).
            /// @param len The length of the block.
            explicit BarBlockInfo(const double len) : m_length(len) {}

            /// @brief Sets the block's brush.
            /// @param brush The brush of the block.
            /// @returns A self reference.
            BarBlockInfo& Brush(const wxBrush brush)
                {
                m_brush = brush;
                return *this;
                }

            /// @brief Sets the block's background color.
            /// @details This is invalid and not used by default, as the brush is what
            ///    is normally used. However, if this set to a valid color, then that
            ///    color will be filled in first, and then the brush will be painted
            ///    on top. This is useful if the brush is a pattern (e.g., hatch),
            ///    rather than a solid color.
            /// @note This is only used if the bar block's painting effect is solid.
            ///    In other works, this color is ignored for fades, glossy effects, stipples, etc.
            /// @param color The background color of the block.
            /// @returns A self reference.
            BarBlockInfo& Color(const wxColour color)
                {
                m_color = color;
                return *this;
                }

            /// @brief Explicitly sets the outline of the bar block.
            /// @param pen The pen to use for outlining.
            /// @note If this is not set, then the parent bar chart will deduce the
            ///    best outline color.
            /// @returns A self reference.
            BarBlockInfo& OutlinePen(const wxPen pen)
                {
                m_outlinePen = pen;
                return *this;
                }

            /// @brief Sets the block's length.
            /// @param len The length of the bar block.
            /// @returns A self reference.
            BarBlockInfo& Length(const double len) noexcept
                {
                m_length = len;
                return *this;
                }

            /// @brief Sets the block's label when selected.
            /// @param label The selection label of the bar block.
            /// @returns A self reference.
            BarBlockInfo& SelectionLabel(const Wisteria::GraphItems::Label& label)
                {
                m_selectionLabel = label;
                return *this;
                }

            /// @brief Sets the block's decal (i.e., text written on the block).
            /// @param label The decal of the bar block.
            /// @returns A self reference.
            BarBlockInfo& Decal(const Wisteria::GraphItems::Label& label)
                {
                m_decal = label;
                return *this;
                }

            /// @brief Sets whether the block should be displayed.
            /// @param show @c true to show the block.
            /// @returns A self reference.
            BarBlockInfo& Show(const bool show) noexcept
                {
                m_show = show;
                return *this;
                }

            /// @brief Sets a user-defined string to associate with the block.
            /// @details Client code determines what this value means.
            /// @param tag A user-defined string to connect to the block.
            /// @returns A self reference.
            BarBlockInfo& Tag(const wxString& tag)
                {
                m_tag = tag;
                return *this;
                }

          private:
            wxBrush m_brush{ *wxGREEN_BRUSH };
            wxPen m_outlinePen{ wxNullPen };
            wxColour m_color;
            double m_length{ 0 };
            Wisteria::GraphItems::Label m_selectionLabel;
            Wisteria::GraphItems::Label m_decal;
            bool m_show{ true };
            wxString m_tag;
            };

        /// @brief The smaller sections of a bar.
        /// @details Multiple blocks are stacked to make the final bar
        ///    show grouping inside of the bar.
        class BarBlock
            {
          public:
            /// @private
            BarBlock() = default;

            /// @brief Constructor
            /// @param info A chainable set of fields to assign to the bar block.
            explicit BarBlock(const BarBlockInfo& info)
                : m_brush(info.m_brush), m_outlinePen(info.m_outlinePen), m_color(info.m_color),
                  m_length(info.m_length), m_selectionLabel(info.m_selectionLabel),
                  m_decal(info.m_decal), m_show(info.m_show), m_tag(info.m_tag)
                {
                }

            /// @returns The block's brush.
            [[nodiscard]]
            const wxBrush& GetBrush() const noexcept
                {
                return m_brush;
                }

            /// @private
            [[nodiscard]]
            wxBrush& GetBrush() noexcept
                {
                return m_brush;
                }

            /// @returns The block's outline pen.
            [[nodiscard]]
            const wxPen& GetOutlinePen() const noexcept
                {
                return m_outlinePen;
                }

            /// @returns The block's background color.
            /// @details This is invalid and not used by default, as the brush is what
            ///    is normally used. However, if this set to a valid color, then that
            ///    color will be filled in first, and then the brush will be painted
            ///    on top. This is useful if the brush is a pattern (e.g., hatch),
            ///    rather than a solid color.
            /// @note This is only used if the bar block's painting effect is solid.
            ///    In other works, this color is ignored for fades, glossy effects, stipples, etc.
            [[nodiscard]]
            const wxColour& GetColor() const noexcept
                {
                return m_color;
                }

            /// @returns A lightened variation of the block color.
            [[nodiscard]]
            wxColour GetLightenedColor() const
                {
                return m_brush.GetColour().ChangeLightness(160);
                }

            /** @brief Sets the decal to draw across the bar.
                @param decal The decal to draw.*/
            void SetDecal(const Wisteria::GraphItems::Label& decal) { m_decal = decal; }

            /// @private
            [[nodiscard]]
            const Wisteria::GraphItems::Label& GetDecal() const noexcept
                {
                return m_decal;
                }

            /// @returns The decal being drawn across the block.
            [[nodiscard]]
            Wisteria::GraphItems::Label& GetDecal() noexcept
                {
                return m_decal;
                }

            /// @returns The decal label of the block, with constants expanded in it.\n
            ///     The constants supported are:\n
            ///     `@COUNT@`: The length of the block.
            [[nodiscard]]
            wxString ExpandDecalLabel() const
                {
                wxString expandedStr = GetDecal().GetText();
                expandedStr.Replace(
                    L"@COUNT@",
                    wxNumberFormatter::ToString(GetLength(), 0,
                                                wxNumberFormatter::Style::Style_WithThousandsSep));
                return expandedStr;
                }

            /// @private
            [[nodiscard]]
            const Wisteria::GraphItems::Label& GetSelectionLabel() const noexcept
                {
                return m_selectionLabel;
                }

            /// @returns The text displayed on the bar when it is selected.
            [[nodiscard]]
            Wisteria::GraphItems::Label& GetSelectionLabel() noexcept
                {
                return m_selectionLabel;
                }

            /// @brief Whether the block is being drawn.
            /// @details Set to @c false to treat the block like a placeholder
            ///    in the parent bar.
            /// @returns @c true if being displayed.
            [[nodiscard]]
            bool IsShown() const noexcept
                {
                return m_show;
                }

            /// @brief Sets whether the block is being drawn.
            /// @param show Set to @c false to treat the block like a placeholder
            ///    in the parent bar.
            void Show(const bool show) noexcept { m_show = show; }

            /// @returns The user-defined tag.
            [[nodiscard]]
            wxString GetTag() const noexcept
                {
                return m_tag;
                }

            /// @returns The length of the block along the scaling axis.
            [[nodiscard]]
            double GetLength() const noexcept
                {
                return m_length;
                }

            /// @brief Sets the custom width of the block.
            /// @details This will be used first when drawing the block.
            ///    If invalid, then the parent bar's custom width will be used.
            ///    If that is invalid, then bars and blocks will have their widths
            ///    calculated by the chart (the default).
            /// @param width The width of the block (in terms of units along the bar axis).
            ///    For example, if the bar axis range is 0-100 and you set 25 here, then the
            ///    block will consume 25% of the width of the axis
            ///    (regardless of how wide the other bars are).
            /// @note You can mix and match custom-width at the bar and bar-block levels,
            ///    although that normally wouldn't be recommend.\n
            ///    Prefer just setting custom widths at the bar level,
            ///    unless you have a special need.
            void SetCustomWidth(const std::optional<double> width) noexcept
                {
                m_customWidth = width;
                if (m_customWidth.has_value() &&
                    (m_customWidth <= 0 || std::isnan(m_customWidth.value())))
                    {
                    m_customWidth = std::nullopt;
                    }
                }

            /** @brief The custom width used for the block along the bar axis.
                @details Not normally used, usually the custom width is handled on the bar level.
                @returns The custom width (will be @c std::nullopt if invalid).*/
            [[nodiscard]]
            std::optional<double> GetCustomWidth() const noexcept
                {
                return m_customWidth;
                }

          private:
            /// @brief The brush (color and pattern) of the block.
            /// @note The bar block's opacity will override the parent bar's opacity
            ///    if different from the default (i.e., fully opaque).
            wxBrush m_brush{ *wxGREEN_BRUSH };
            /// @brief Explicitly sets the outline of the block. Normally, the
            ///    parent chart will determine the best outline color.
            wxPen m_outlinePen{ wxNullPen };
            /// @brief An optional background color to use in conjunction with the brush.
            ///    Will be invalid by default, so that the brush is what is used exclusively.
            wxColour m_color;
            /// The length of the block (i.e., how much of the scaling axis the block consumes).
            double m_length{ 0 };
            /// The label shown on the middle of the bar when it is selected by the mouse.
            Wisteria::GraphItems::Label m_selectionLabel;
            /// The decal being drawn across the bar.
            Wisteria::GraphItems::Label m_decal;
            /// Whether the block should be display. If @c false, then it acts like a placeholder.
            bool m_show{ true };
            /// An optional tag to identify or classify the block.
            wxString m_tag;
            /// only used if a bar must be a specific width
            std::optional<double> m_customWidth{ std::nullopt };
            };

        /// @brief A bar shown along an axis.
        class Bar
            {
            friend BarChart;

          public:
            /// @private
            Bar() = default;

            /** @brief Constructor.
                @param axisPosition The position on the parent axis to anchor this bar.
                @param blocks The blocks used to build the bar.
                @param barLabel The label shown on top of the bar
                    (useful for showing the item count in the bar, for example).
                @param axisLabel The label to display beneath the bar on the parent axis.
                @param effect The effect to display on the bar (e.g., a color gradient).
                @param opacity The opacity of the bar.
                @param customWidth How wide to show the bar. If provided, this will override the
                    calculated width (which would show all bars with a uniform width).*/
            Bar(const double axisPosition, const std::vector<BarBlock>& blocks,
                const wxString& barLabel, const Wisteria::GraphItems::Label& axisLabel,
                const BoxEffect effect, const uint8_t opacity = wxALPHA_OPAQUE,
                const std::optional<double> customWidth = std::nullopt)
                : m_blocks(blocks), m_opacity(opacity), m_barEffect(effect), m_axisLabel(axisLabel),
                  m_barLabel(Wisteria::GraphItems::GraphItemInfo(barLabel).Pen(wxNullPen)),
                  m_customWidth(customWidth), m_axisPosition(axisPosition)
                {
                // set to sane value
                if (m_customWidth.has_value() &&
                    (m_customWidth <= 0 || std::isnan(m_customWidth.value())))
                    {
                    m_customWidth = std::nullopt;
                    }
                m_length = std::accumulate(m_blocks.cbegin(), m_blocks.cend(), 0.0f,
                                           [](const auto initVal, const auto& block) noexcept
                                           { return initVal + block.GetLength(); });
                }

            /// @private
            [[nodiscard]]
            bool operator<(const Bar& that) const noexcept
                {
                return GetLength() < that.GetLength();
                }

            /** @name Visual Effect Functions
                @brief Functions related to the bar's visual effects.*/
            /// @{

            /// @returns The opacity of the bar.
            [[nodiscard]]
            uint8_t GetOpacity() const noexcept
                {
                return m_opacity;
                }

            /// @brief Sets the box's opacity.
            /// @param opacity The opacity to apply.
            void SetOpacity(const uint8_t opacity) noexcept { m_opacity = opacity; }

            /// @returns The effect drawn across the bar.
            [[nodiscard]]
            BoxEffect GetEffect() const noexcept
                {
                return m_barEffect;
                }

            /// @brief Sets the effect drawn across the bar.
            /// @param effect The box effect to use.
            /// @note If using the arrow shape, some effects (glassy, stipple) are ignored.
            /// @sa SetShape().
            void SetEffect(const BoxEffect effect) noexcept { m_barEffect = effect; }

            /** @returns The shape of the bar.
                @note Image-based bar effects and drop shadows will only work with
                    rectangular shapes.
                @todo Add support for drop shadows for arrows.*/
            [[nodiscard]]
            BarShape GetShape() const noexcept
                {
                return m_barShape;
                }

            /** @brief Sets the shape to draw the bar as.
                @param shape The shape to draw for the bar.
                @note If using the arrow shape, some effects (glassy, stipple) are ignored.
                @sa SetEffect().*/
            void SetShape(const BarShape shape) noexcept { m_barShape = shape; }

            /// @}

            /** @name %Label Functions
                @brief Functions related to the labelling of the bar.
                @note To draw text directly on a bar, use the built-in decals on the bar blocks.*/
            /// @{

            /// @brief Gets/sets the text displayed on the axis beneath the bar.
            /// @details Usually, this would be the observation or category label.
            /// @returns The axis label.
            [[nodiscard]]
            Wisteria::GraphItems::Label& GetAxisLabel() noexcept
                {
                return m_axisLabel;
                }

            /// @brief Gets/sets the label shown on top (or to the right) of the bar
            ///    (useful for showing the item count in the bar, for example).
            /// @returns The bar label.
            [[nodiscard]]
            Wisteria::GraphItems::Label& GetLabel() noexcept
                {
                return m_barLabel;
                }

            /// @}

            /** @name Size Functions
                @brief Functions related to the bar's height and width
                 (in terms of its parent axis).*/
            /// @{

            /// @brief The length/height of the bar along the scaling axis
            ///    (i.e., how tall or long the bar is).
            /// @note This is the summation of the lengths of all blocks (i.e., groups) in the bar.
            ///    Control of the bar's length is done through the constituent blocks in the bar.
            /// @returns The bar's length along the scaling axis.
            [[nodiscard]]
            double GetLength() const noexcept
                {
                return m_length;
                }

            /// @brief Sets the custom width of the bar.
            /// @param width The width of the bar (in terms of units along the bar axis).\n
            ///    For example, if the bar axis range is 0-100 and you set 25 here, then the bar
            ///    will consume 25% of the width of the axis (regardless of how wide the other
            ///    bars are).
            /// @note You can mix and match custom-width and auto-fitted bars in the same barchart;
            ///    just don't set the custom width for bars that you wish to be auto-fitted.
            void SetCustomWidth(const std::optional<double> width) noexcept
                {
                m_customWidth = width;
                // sanity checks
                if (m_customWidth.has_value() &&
                    (m_customWidth.value() <= 0 || std::isnan(m_customWidth.value())))
                    {
                    m_customWidth = std::nullopt;
                    }
                }

            /** @brief The custom width used for the bar along the bar axis.
                @details Not normally used, this is usually meant for situations where the
                    bars must fit together a very specific way (e.g., ranges on a histogram).
                @returns The custom width (will be @c std::nullopt if invalid).*/
            [[nodiscard]]
            std::optional<double> GetCustomWidth() const noexcept
                {
                return m_customWidth;
                }

            /// @}

            /** @name Block Functions
                @brief Functions related to the blocks that make up the bar
                @details Most bars will consist of a single block, but you can construct
                    a bar out of multiple blocks that stack up on each other as well. This is
                    useful for grouping or showing ranges inside the bar.*/
            /// @{

            /** @brief Adds a block to the bar along the scaling axis (i.e., how "tall" the bar is).
                @param block The to add to the bar.
                @note If calling this, be sure to adjust the range of the scaling axis if need be.
                    Normally, it's preferred to let AddBar() handle this.*/
            void AddBlock(const BarBlock& block) noexcept
                {
                m_blocks.push_back(block);
                m_length = std::accumulate(m_blocks.cbegin(), m_blocks.cend(), 0.0f,
                                           [](const auto initVal, const auto& bBlock)
                                           { return initVal + bBlock.GetLength(); });
                }

            /// @brief Removes the blocks constituting the bar.
            void ClearBlocks() noexcept { m_blocks.clear(); }

            /// @brief Gets/sets The blocks that make up the bar.
            /// @returns The bar's blocks.
            std::vector<BarBlock>& GetBlocks() noexcept { return m_blocks; }

            /** @brief Searches for a block in the bar with the provided tag.\n
                    Tags may be a categorical label added to the block, or a user-provided value.
                @param tag The tag to look for.
                @returns An iterator to the first block with the given tag,
                    or `GetBlocks().cend()` if not found.*/
            std::vector<BarBlock>::const_iterator FindBlock(const wxString& tag) const noexcept
                {
                return std::find_if(GetBlocks().cbegin(), GetBlocks().cend(), [&](const auto& block)
                                    { return block.GetTag().CmpNoCase(tag) == 0; });
                }

            /** @brief Searches for the first block in the bar whose tag matches the
                    provided regular expression.\n
                    (Tags may be a categorical label added to the block, or a user-provided value.)
                @param pattern The regex to match against.
                @returns An iterator to the first matching block,
                    or `GetBlocks().cend()` if not found.*/
            std::vector<BarBlock>::const_iterator
            FindFirstBlockRE(const wxString& pattern) const noexcept
                {
                wxRegEx re(pattern);
                if (!re.IsValid())
                    {
                    return GetBlocks().cend();
                    }

                return std::find_if(GetBlocks().cbegin(), GetBlocks().cend(),
                                    [&](const auto& block) { return re.Matches(block.GetTag()); });
                }

            /** @brief Searches for the last block in the bar whose tag matches the
                    provided regular expression.\n
                    (Tags may be a categorical label added to the block, or a user-provided value.)
                @param pattern The regex to match against.
                @returns A reverse iterator to the last matching block,
                    or `GetBlocks().crend()` if not found.*/
            std::vector<BarBlock>::const_reverse_iterator
            FindLastBlockRE(const wxString& pattern) const noexcept
                {
                wxRegEx re(pattern);
                if (!re.IsValid())
                    {
                    return GetBlocks().crend();
                    }

                return std::find_if(GetBlocks().crbegin(), GetBlocks().crend(),
                                    [&](const auto& block) { return re.Matches(block.GetTag()); });
                }

            /// @}

            /** @name Positioning Functions
                @brief Functions related to positioning the bar on its parent axis.*/
            /// @{

            /// @brief The position on the bar axis that the bar should be placed on.
            /// @return The axis position.
            [[nodiscard]]
            double GetAxisPosition() const noexcept
                {
                return m_axisPosition;
                }

            /** @brief Sets The position on the bar axis that the bar should be placed on.
                @param position The place on the axis to position the bar.*/
            void SetAxisPosition(const double position) noexcept { m_axisPosition = position; }

            /// @brief The custom position on the **scaling** axis to start drawing the bar.
            /// @returns The custom position on the scaling axis.
            [[nodiscard]]
            std::optional<double> GetCustomScalingAxisStartPosition() const noexcept
                {
                return m_customScalingStartPosition;
                }

            /** @brief Sets a custom position on the **scaling** axis to start drawing the bar.
                @details Normally a bar begins at the start of the scaling axis, so this can be
                    used to make it start higher/more to the right.

                    As an example, if a bar's length is 40 and you specify its axis start position
                    as 80, then it will start at 80 and end at 120. (And 0-79 will be a blank spot
                    along the bar.)
                @param position The position to start drawing the bar. Set this to NaN (the default)
                    to disable this.*/
            void SetCustomScalingAxisStartPosition(const std::optional<double> position)
                {
                m_customScalingStartPosition = position;
                // sanity check
                if (m_customScalingStartPosition.has_value() &&
                    std::isnan(m_customScalingStartPosition.value()))
                    {
                    m_customScalingStartPosition = std::nullopt;
                    }
                }

            /// @}

            /// @private
            [[nodiscard]]
            const Wisteria::GraphItems::Label& GetLabel() const noexcept
                {
                return m_barLabel;
                }

            /// @private
            [[nodiscard]]
            const Wisteria::GraphItems::Label& GetAxisLabel() const noexcept
                {
                return m_axisLabel;
                }

            /// @private
            const std::vector<BarBlock>& GetBlocks() const noexcept { return m_blocks; }

            /// @private
            std::vector<BarBlock>::iterator FindBlock(const wxString& tag) noexcept
                {
                return std::find_if(GetBlocks().begin(), GetBlocks().end(), [&](const auto& block)
                                    { return block.GetTag().CmpNoCase(tag) == 0; });
                }

          private:
            std::vector<BarBlock> m_blocks;
            uint8_t m_opacity{ wxALPHA_OPAQUE };
            BoxEffect m_barEffect{ BoxEffect::Solid };
            BarShape m_barShape{ BarShape::Rectangle };
            Wisteria::GraphItems::Label m_axisLabel;
            Wisteria::GraphItems::Label m_barLabel;
            // cached from bar blocks
            double m_length{ 0 };
            // only used if a bar must be a specific width
            std::optional<double> m_customWidth{ std::nullopt };
            // only used if a bar is somewhere other than the start of the scaling axis
            std::optional<double> m_customScalingStartPosition{ std::nullopt };
            double m_axisPosition{ 0 };
            };

        /** @brief Defines a bar group, which is where a contiguous series of bars have a curly
               brace wrapped around them (within the plotting area). On the other side of the curly
               brace, a bar is drawn representing the length of children bars (within the group)
               combined.*/
        struct BarGroup
            {
            /// @brief The start and end positions
            ///  (in terms of index into the bars, not axis position) of the group.
            std::pair<size_t, size_t> m_barPositions{ 0, 0 };
            /// @brief A label to draw on the group bar.
            wxString m_barDecal;
            /// @brief The brush to paint the group bar with.
            wxBrush m_barBrush{ wxNullBrush };
            /// @brief The base color to paint under the group bar's brush.
            /// @details This is useful if the brush is using a hatched pattern.
            wxColour m_barColor{ wxNullColour };
            /// @brief Position on the axis next to the furthest out bar in the group.
            /// @details This is relevant only when the group label placement is `NextToParent`.
            std::optional<wxCoord> m_maxBarPos{ std::nullopt };
            };

        /** @brief Constructor.
            @param canvas The parent canvas for the chart to be drawn on.*/
        explicit BarChart(Wisteria::Canvas* canvas);

        /// @name %Bar Functions
        /// @brief Functions relating to adding and editing bars.
        /// @note Visual effects can also be controlled at the Bar level.
        /// @{

        /** @brief Adds a bar to the chart.
            @param bar The bar to add.
            @param adjustScalingAxis @c true to adjust the scaling axis to fit the bar.\n
                @c false is only recommended if you will be setting the scaling axis manually
                and don't want the chart adjusting it for you.*/
        void AddBar(Bar bar, const bool adjustScalingAxis = true);

        /// @brief Removes all bars from the chart.
        /// @param resetAxes @c true to reset axes. @c true is recommended if you will be adding
        ///    new bars and want the chart to adjust the axes as you add them. @c false is
        ///    recommended only if you are manually setting the axes prior to adding new bars.
        void ClearBars(const bool resetAxes = true)
            {
            m_bars.clear();
            if (resetAxes)
                {
                m_longestBarLength = 0;
                m_lowestBarAxisPosition = std::numeric_limits<double>::max();
                m_highestBarAxisPosition = std::numeric_limits<double>::lowest();
                // Gridlines are reset in SetBarOrientation(), so remember how this
                // was set from before and then restore it.
                const wxPen gridlinePen = GetBarAxis().GetGridlinePen();
                GetLeftYAxis().Reset();
                GetRightYAxis().Reset();
                GetBottomXAxis().Reset();
                GetTopXAxis().Reset();
                GetBarAxis().GetGridlinePen() = gridlinePen;
                }
            }

        /// @returns The opacity of the bar.
        [[nodiscard]]
        uint8_t GetBarOpacity() const noexcept
            {
            return m_barOpacity;
            }

        /// @brief Sets the bar opacity.
        /// @param opacity The level of opacity to use.
        void SetBarOpacity(const uint8_t opacity) noexcept
            {
            m_barOpacity = opacity;
            for (auto& bar : GetBars())
                {
                bar.SetOpacity(opacity);
                }
            }

        /// @returns The effect drawn across the bar.
        [[nodiscard]]
        BoxEffect GetBarEffect() const noexcept
            {
            return m_barEffect;
            }

        /// @brief Sets the bar effect.
        /// @param effect The bar effect to apply.
        /// @sa SetImageScheme().
        void SetBarEffect(const BoxEffect effect) noexcept
            {
            m_barEffect = effect;
            for (auto& bar : GetBars())
                {
                bar.SetEffect(effect);
                }
            }

        /** @brief Sets the specified bars (by custom axis label) to be fully opaque,
                and all others to a lighter opacity.
            @param labels The bars to showcase.
            @note Call SetGhostOpacity() prior to this to control how translucent
                the non-showcased (i.e., "ghosted") bars are.
            @sa SetGhostOpacity().*/
        void ShowcaseBars(const std::vector<wxString>& labels);
        /** @brief Sets the specified bars (by axis position) to be fully opaque,
                and all others to a lighter opacity.
            @param positions The axis positions of the bars to showcase.
            @note Call SetGhostOpacity() prior to this to control how translucent
                the non-showcased (i.e., "ghosted") bars are.
            @sa SetGhostOpacity().*/
        void ShowcaseBars(const std::vector<double>& positions);

        /// @returns The opacity level applied to "ghosted" slices.
        [[nodiscard]]
        uint8_t GetGhostOpacity() const noexcept
            {
            return m_ghostOpacity;
            }

        /** @brief Sets the opacity level for "ghosted" bars.\n
                This is only used if ShowcaseBars() is called; this is the
                opacity applied to bars not being showcased.
            @param opacity The opacity level (should be between @c 0 to @c 255).
            @sa ShowcaseBars().*/
        void SetGhostOpacity(const uint8_t opacity) noexcept { m_ghostOpacity = opacity; }

        /** @brief Adds an image next to the axis label of a bar.
            @param bar The bar to add the icon to. This should be the axis label of the bar.
            @param img The image to add to the bar's label.*/
        void AddBarIcon(const wxString& bar, const wxBitmapBundle& img);

        /// @returns Direct access to the bars.
        /// @note If changing the length of the bar directly, then you will need
        ///    to adjust the scaling axis as well.\n
        ///    If changing the axis position of the bar, then the bar axis may also
        ///    need to be changed manually.
        [[nodiscard]]
        std::vector<Bar>& GetBars() noexcept
            {
            return m_bars;
            }

        /// @brief Searches for a bar by custom axis label.
        /// @param axisLabel The custom axis label of the bar to search for.
        /// @returns The index of the bar if found, @c std::nullopt otherwise.
        [[nodiscard]]
        std::optional<size_t> FindBar(const wxString& axisLabel);
        /// @brief Searches for a bar by axis position.
        /// @param axisPosition The axis position of the bar to search for.
        /// @returns The index of the bar if found, @c std::nullopt otherwise.
        [[nodiscard]]
        std::optional<size_t> FindBar(const double axisPosition);
        /** @brief Finds the start of a bar block.
            @param barIndex Which bar to use.
            @param blockTag The tag to identify the bar block to find.
            @returns The position along the scaling axis where the block begins, or @c std::nullopt
                if not found.*/
        [[nodiscard]]
        std::optional<double> FindBarBlockStart(const size_t barIndex,
                                                const wxString& blockTag) const;
        /** @brief Finds the end of a bar block.
            @param barIndex Which bar to use.
            @param blockTag The tag to identify the bar block to find.
            @returns The position along the scaling axis where the block ends, or @c std::nullopt
                if not found.*/
        [[nodiscard]]
        std::optional<double> FindBarBlockEnd(const size_t barIndex,
                                              const wxString& blockTag) const;

        /** @brief Adds a bracket (inside the plotting area) around a range of bars
                and draws a bar above that showing the length of the children bars combined.
            @details This is useful for giving attention to a block of smaller bars
                that may be eclipsed by a larger bar.
            @param firstBarLabel The (custom) axis label of the first bar in the group.
            @param lastBarLabel The (custom) axis label of the last bar in the group.
            @param decal The label to show on the grouped bar.
            @param color The color of the grouped bar. If @c std::nullopt, the first color from
                the color scheme will be used (or transparent color, if the color scheme is null).
            @param brush The brush of the grouped bar. If @c std::nullopt, the first brush from
                the brush scheme will be used.
            @throws std::runtime_error If a provided label isn't found, throws an exception.*/
        void AddBarGroup(const wxString& firstBarLabel, const wxString& lastBarLabel,
                         std::optional<wxString> decal = std::nullopt,
                         std::optional<wxColour> color = std::nullopt,
                         std::optional<wxBrush> brush = std::nullopt);
        /** @brief Adds a bracket (inside the plotting area) around a range of bars
                and draws a bar above that showing the length of the children bars combined.
            @details This is useful for giving attention to a block of smaller bars
                that may be eclipsed by a larger bar.
            @param firstBarAxisPosition The axis position of the first bar in the group.
            @param lastBarAxisPosition The axis position of the last bar in the group.
            @param decal The label to show on the grouped bar.
            @param color The color of the grouped bar. If @c std::nullopt, the first color from
                the color scheme will be used (or transparent color, if the color scheme is null).
            @param brush The brush of the grouped bar. If @c std::nullopt, the first brush from
                the brush scheme will be used.
            @throws std::runtime_error If a provided axis positions doesn't have a bar,
                throws an exception.*/
        void AddBarGroup(const double firstBarAxisPosition, const double lastBarAxisPosition,
                         std::optional<wxString> decal = std::nullopt,
                         std::optional<wxColour> color = std::nullopt,
                         std::optional<wxBrush> brush = std::nullopt);

        /** @brief Adds a bracket around a range of bars and draws a bar above that
                showing the length of the children bars combined.
            @details This is useful for giving attention to a block of smaller bars
                that may be eclipsed by a larger bar.
            @param barGroup The bar group information.*/
        void AddBarGroup(BarGroup barGroup)
            {
            m_barGroups.push_back(std::move(barGroup));
            AdjustScalingAxisFromBarGroups();
            }

        /// @brief Removes all bar groups from the bar chart.
        void ClearBarGroups() { m_barGroups.clear(); }

        /// @returns How the bar groups (brackets and cumulative bars) are aligned with their
        /// respective bars.
        [[nodiscard]]
        LabelPlacement GetBarGroupPlacement() const noexcept
            {
            return m_barGroupPlacement;
            }

        /** @brief Sets how the bar groups (brackets and cumulative bars) are aligned with their
           respective bars.
            @details `NextToParent` will position the bar groups brackets next to their respective
               bars.\n `Flush` will align all bar groups together, rather than next to their bars.
               `Flush` is recommended if you want to easily compare the groups' cumulative bars
               with each other.
            @param barGroupPlacement How to position the bar groups.*/
        void SetBarGroupPlacement(LabelPlacement barGroupPlacement) noexcept
            {
            m_barGroupPlacement = barGroupPlacement;
            }

        /// @}

        /// @name Axis Functions
        /// @brief Functions relating to the axes and how the chart is oriented.
        /// @{

        /// @returns The axis with the scaling, which is the axis perpendicular
        ///    to the axis with the bars on it.
        [[nodiscard]]
        Wisteria::GraphItems::Axis& GetScalingAxis() noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetLeftYAxis() :
                                                                    GetBottomXAxis();
            }

        /// @returns The opposite side scaling axis,
        ///    which is the axis perpendicular to the axis with the bars on it.
        [[nodiscard]]
        Wisteria::GraphItems::Axis& GetOppositeScalingAxis() noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetRightYAxis() : GetTopXAxis();
            }

        /// @returns The axis that the bars are being spread across.
        [[nodiscard]]
        Wisteria::GraphItems::Axis& GetBarAxis() noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetBottomXAxis() :
                                                                    GetLeftYAxis();
            }

        /// @returns The opposite side axis,
        ///     which is the axis parallel to the axis with the bars on it.
        [[nodiscard]]
        Wisteria::GraphItems::Axis& GetOppositeBarAxis() noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetTopXAxis() : GetRightYAxis();
            }

        /// @returns Whether the bars are laid out vertically or
        ///    horizontally across the chart.
        [[nodiscard]]
        Orientation GetBarOrientation() const noexcept
            {
            return m_barOrientation;
            }

        /// @brief Sets whether the bars are laid out vertically or horizontally across the chart.
        /// @param orient Which orientation to use for the bars.
        /// @note All axis brackets will be removed when changing bar orientation, so all bracket
        ///     operations should be called after calling this.
        /// @warning Call this prior to any calls to AddBar() (or SetData() in derived classes).\n
        ///     Also, if aligning with a common X axis, then set this to @c Vertical.
        ///     (Set to @c Horizontal if aligning with a common Y axis.)
        void SetBarOrientation(const Orientation orient);
        /** @brief Adds an axis bracket (scaling axis) referencing the first bar.
            @param firstBarBlock The tag of the bar block of the first bar where the
                bracket should begin.\n
                The bracket will be drawn at the start of this block.
            @param lastBarBlock The tag of the bar block of the first bar where the
                bracket should end.\n
                The bracket will be drawn to the end of this block.
            @param bracketLabel The label for the bracket.
            @note This should be called after all bars have been added and
                bar orientation has been set.
            @throws std::runtime_error If a provided label isn't found, throws an exception.
            @todo Add support for reversed scaling axis.*/
        void AddFirstBarBracket(const wxString& firstBarBlock, const wxString& lastBarBlock,
                                const wxString& bracketLabel);
        /** @brief Adds an axis bracket (scaling axis) referencing the first bar.
            @param firstBarBlockPattern A regular expression matching the tag of the bar block
                of the first bar where the bracket should begin.\n
                The bracket will be drawn at the start of this block.
            @param lastBarBlockPattern A regular expression matching the tag of the bar block
                of the first bar where the bracket should end.\n
                The bracket will be drawn to the end of this block.
            @param bracketLabel The label for the bracket.
            @note This should be called after all bars have been added and
                bar orientation has been set.
            @throws std::runtime_error If a provided label isn't found, throws an exception.
            @todo Add support for reversed scaling axis.*/
        void AddFirstBarBracketRE(const wxString& firstBarBlockPattern,
                                  const wxString& lastBarBlockPattern,
                                  const wxString& bracketLabel);
        /** @brief Adds an axis bracket (opposite scaling axis) referencing the last bar.
            @param firstBarBlock The bar block of the last bar where the bracket should begin.\n
                The bracket will be drawn at the start of this block.
            @param lastBarBlock The bar block of the last bar where the bracket should end.\n
                The bracket will be drawn to the end of this block.
            @param bracketLabel The label for the bracket.
            @note This should be called after all bars have been added and
                bar orientation has been set.
            @throws std::runtime_error If a provided label isn't found, throws an exception.
            @todo Add support for reversed scaling axis.*/
        void AddLastBarBracket(const wxString& firstBarBlock, const wxString& lastBarBlock,
                               const wxString& bracketLabel);
        /** @brief Adds an axis bracket (scaling axis) referencing the first bar.
            @param firstBarBlockPattern A regular expression matching the tag of the bar block
                of the last bar where the bracket should begin.\n
                The bracket will be drawn at the start of this block.
            @param lastBarBlockPattern A regular expression matching the tag of the bar block
                of the last bar where the bracket should end.\n
                The bracket will be drawn to the end of this block.
            @param bracketLabel The label for the bracket.
            @note This should be called after all bars have been added and
                bar orientation has been set.
            @throws std::runtime_error If a provided label isn't found, throws an exception.
            @todo Add support for reversed scaling axis.*/
        void AddLastBarBracketRE(const wxString& firstBarBlockPattern,
                                 const wxString& lastBarBlockPattern, const wxString& bracketLabel);

        /// @brief Forces the scaling axis to the length of the longest bar, resetting any custom
        ///     or calculated range.
        /// @details This is only recommended for when the scaling axis is not being shown and the
        ///     bars are being used more for artistic purposes.\n
        ///     Call this after all manual calls to @c AddBar or various @c SetData() derived
        ///     functions.
        /// @warning Bar labels should be set to @c BinLabelDisplay::NoDisplay as they will go
        ///     outside of the plotting area.
        void ConstrainScalingAxisToBars()
            {
            GetScalingAxis().SetRange(GetScalingAxis().GetRange().first, m_longestBarLength,
                                      GetScalingAxis().GetPrecision(),
                                      GetScalingAxis().GetInterval(),
                                      GetScalingAxis().GetDisplayInterval());
            }

        /// @}

        /// @name Sort Functions
        /// @brief Functions relating to sorting the bars.
        /// @{

        /// @brief Sorts the bars (based on bar size or axis label).
        /// @param sortMethod How to sort the bars.
        /// @param direction SortAscending to sort smallest-to-largest (A-Z),
        ///    going top-to-bottom or left-to-right.
        ///    SortDescending to sort largest-to-smallest (A-Z).
        /// @warning If there are bar groups or brackets along the bar axis, then those
        ///     will be removed when sorting.
        virtual void SortBars(const BarSortComparison sortMethod, const SortDirection direction);
        /// @brief Sorts the bars (based on a specified order of axis labels).
        ///     This is similar to what @c forcats::fct_relevel() does in R.
        /// @param labels The axis labels of the bars, sorted in the new order that they should
        ///     appear along the axis.
        /// @param direction SortAscending to sort going top-to-bottom or left-to-right.
        ///    SortDescending to sort bottom-to-top or right-to-left.
        /// @note If not all bar labels are provided, then the given labels will be grouped
        ///     to the top (or left) and the rest of the bars will be sorted after those
        ///     alphabetically.\n
        ///     Also, if no bar can be found for a provided label, then an empty bar
        ///     with that label will be added.\n
        ///     Finally, all bar labels must be unique for the label ordering to work;
        ///     otherwise, the sort could not be deterministic. If there are multiple bars
        ///     with the same axis label, then this will return without sorting.
        /// @warning If there are bar groups or brackets along the bar axis, then those
        ///     will be removed when sorting.
        virtual void SortBars(std::vector<wxString> labels, const SortDirection direction);

        /// @returns @c true if the bars can be sorted (i.e., reordered) in terms of bar length.
        virtual bool IsSortable() const noexcept { return m_isSortable; }

        /// @returns The direction that the bars are being sorted.
        [[nodiscard]]
        SortDirection GetSortDirection() const noexcept
            {
            return m_sortDirection;
            }

        /// @brief Sets the direction that the bars should be sorted.
        /// @param direction The direction that the bars should be sorted.
        void SetSortDirection(const SortDirection direction) noexcept
            {
            m_sortDirection = direction;
            }

        /// @brief Sets the bar axis so that it can be sorted
        ///    (based on bar size or axis label).
        /// @param sortable Whether the bars should be sortable.
        /// @note This needs to turn off bar axis reversal and only use
        ///    custom labels on the bar axis.\n
        ///    When sorting the bars, it only makes sense if the bars are
        ///    categories/observations with text labels.
        void SetSortable(const bool sortable)
            {
            m_isSortable = sortable;
            if (sortable)
                {
                GetBarAxis().ReverseScale(false);
                GetBarAxis().SetLabelDisplay(AxisLabelDisplay::DisplayOnlyCustomLabels);
                }
            }

        /// @}

        /// @name Label Functions
        /// @brief Functions relating to how labels and shadows are drawn.
        /// @{

        /** @brief Sets whether to include spaces between the bars when drawn.
            @param includeSpaces Whether to include spaces between the bars when drawn.
            @note If using custom widths for bars, then this is ignored.*/
        void IncludeSpacesBetweenBars(bool includeSpaces = true) noexcept
            {
            m_includeSpacesBetweenBars = includeSpaces;
            }

        /// @returns The type of labels being shown on the bars.
        [[nodiscard]]
        BinLabelDisplay GetBinLabelDisplay() const noexcept
            {
            return m_binLabelDisplay;
            }

        /// @brief Sets which type of labels to display for the bars.
        /// @param display The bin label display to use.
        void SetBinLabelDisplay(const BinLabelDisplay display)
            {
            m_binLabelDisplay = display;
            // update the bars' labels
            for (auto& bar : GetBars())
                {
                UpdateBarLabel(bar);
                }
            }

        /// @returns The suffix being added to each bar label.
        [[nodiscard]]
        wxString GetBinLabelSuffix() const
            {
            return m_binLabelSuffix;
            }

        /// @brief Sets the suffix being added to each bar label.
        /// @details This is useful for adding something like `%` or ` units`
        ///     to the bin labels.
        /// @param suffix The suffix to add to label.
        void SetBinLabelSuffix(wxString suffix)
            {
            m_binLabelSuffix = std::move(suffix);
            // update the bars' labels
            for (auto& bar : GetBars())
                {
                UpdateBarLabel(bar);
                }
            }

        /// @}

        /// @private
        [[nodiscard]]
        const std::vector<Bar>& GetBars() const noexcept
            {
            return m_bars;
            }

        /// @private
        [[nodiscard]]
        const Wisteria::GraphItems::Axis& GetBarAxis() const noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetBottomXAxis() :
                                                                    GetLeftYAxis();
            }

        /// @private
        [[nodiscard]]
        const Wisteria::GraphItems::Axis& GetScalingAxis() const noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetLeftYAxis() :
                                                                    GetBottomXAxis();
            }

        /// @private
        [[nodiscard]]
        const Wisteria::GraphItems::Axis& GetOppositeScalingAxis() const noexcept
            {
            return (GetBarOrientation() == Orientation::Vertical) ? GetRightYAxis() : GetTopXAxis();
            }

      protected:
        /// @brief Updates the label at the top (or right) of a bar.
        ///     This is controlled by the current bin label display and will update it accordingly.
        /// @param bar The bar to update.
        void UpdateBarLabel(Bar& bar);

        /// @returns The number of slots that can hold a bar.
        ///    This is used for calculating the width of the bars.
        ///    Using the number of bars to calculate the widths may be inaccurate if
        ///    there are missing bars along the bar axis, so this (by default) will use
        ///    the number of axis labels that would appear under each place where a bar would go.\n
        ///    (Note that the outer labels are not counted, just the labels under the bars.)
        /// @note The default behavior assumes that labels are appearing beneath the middle of
        ///    each bar (instead of cutpoints, like in histograms), so override this if relying
        ///    on bar axis labels won't work.\n
        ///    This should be overridden if bar axis labelling is being done in a different way,
        ///    or if bars are being stacked on top of each other or other interesting situations.
        [[nodiscard]]
        virtual size_t GetBarSlotCount() const noexcept
            {
            return GetBarAxis().GetAxisPoints().size() - 2;
            }

        /// @brief Recalculates the layout of the elements on the chart.
        /// @details Call this after adding all of your bars.
        /// @param dc The DC to measure content with.
        void RecalcSizes(wxDC& dc) override;
        /** @brief Recalculates the scaling axis based on the size and positioning on a given bar.
            @param bar The bar to review.*/
        void UpdateScalingAxisFromBar(const Bar& bar);

        /** @brief Sets the DPI scaling.
            @param scaling The DPI scaling.*/
        void SetDPIScaleFactor(const double scaling) override
            {
            Graph2D::SetDPIScaleFactor(scaling);
            for (auto& bar : m_bars)
                {
                bar.m_barLabel.SetDPIScaleFactor(scaling);
                bar.m_axisLabel.SetDPIScaleFactor(scaling);
                }
            }

      private:
        struct BarRenderInfo
            {
            explicit BarRenderInfo(wxDC& dc) : m_dc(dc) {}

            wxCoord m_barSpacing{ 0 };
            wxCoord m_scaledShadowOffset{ 0 };
            int m_defaultFontPointSize{ 0 };
            wxCoord m_labelSpacingFromLine{ 0 };
            wxImage m_scaledCommonImg;
            std::vector<std::unique_ptr<GraphItems::Label>> m_decals;
            wxRect m_barRect{};
            double m_barWidth{ 0 };
            wxDC& m_dc;
            std::vector<wxPoint> m_barMiddleEndPositions;
            };

        struct BarBlockRenderInfo
            {
            wxPoint m_middlePointOfBarEnd;
            wxCoord m_axisOffset{ 0 };
            };

        wxPoint DrawBar(Bar& bar, const size_t barIndex, BarRenderInfo& barRenderInfo,
                        const bool measureOnly = false);
        wxPoint DrawBarBlockHorizontal(const Bar& bar, const size_t barIndex,
                                       const BarBlock& barBlock, BarRenderInfo& barRenderInfo,
                                       BarBlockRenderInfo& barBlockRenderInfo,
                                       const bool measureOnly = false);
        wxPoint DrawBarBlockVertical(const Bar& bar, const size_t barIndex,
                                     const BarBlock& barBlock, BarRenderInfo& barRenderInfo,
                                     BarBlockRenderInfo& barBlockRenderInfo,
                                     const bool measureOnly = false);
        void DrawBarGroups(BarRenderInfo& barRenderInfo);

        [[nodiscard]]
        wxRect GetDrawArea() const;

        void AdjustScalingAxisFromBarLength(const double barLength);
        void AdjustScalingAxisFromBarGroups();
        std::vector<Bar> m_bars;
        uint8_t m_barOpacity{ wxALPHA_OPAQUE };
        uint8_t m_ghostOpacity{ 32 }; // used for showcasing
        BoxEffect m_barEffect{ BoxEffect::Solid };
        BinLabelDisplay m_binLabelDisplay{ BinLabelDisplay::BinValue };
        wxString m_binLabelSuffix;

        double m_longestBarLength{ 0 };
        double m_lowestBarAxisPosition{ std::numeric_limits<double>::max() };
        double m_highestBarAxisPosition{ std::numeric_limits<double>::lowest() };
        bool m_includeSpacesBetweenBars{ false };
        bool m_isSortable{ false };
        LabelPlacement m_barGroupPlacement{ LabelPlacement::NextToParent };
        std::vector<BarGroup> m_barGroups;
        Wisteria::SortDirection m_sortDirection{ SortDirection::NoSort };
        Orientation m_barOrientation{ Orientation::Vertical };
        };
    } // namespace Wisteria::Graphs

/** @}*/

#endif // WISTERIA_BARCHART_H
