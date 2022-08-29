/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_ICONS_H__
#define __WISTERIA_ICONS_H__

#include <vector>
#include <set>
#include <initializer_list>
#include <array>
#include <memory>
#include <optional>
#include <wx/wx.h>
#include <wx/gdicmn.h>
#include <wx/dcgraph.h>
#include <wx/string.h>
#include <wx/uilocale.h>
#include <wx/numformatter.h>
#include "settings.h"
#include "../math/mathematics.h"

/// @brief Icons and icon schemes.
namespace Wisteria::Icons
    {
    /// @brief The types of shapes that can be drawn on a legend or plot.
    /// @internal Update the constant map in @c ReportBuilder::LoadIconScheme when added a new icon.
    enum class IconShape
        {
        BlankIcon,                     /*!< Don't draw any icon.*/
        HorizontalLineIcon,            /*!< A horizontal line.*/
        ArrowRightIcon,                /*!< An arrow pointing right.*/
        CircleIcon,                    /*!< A circle.*/
        ImageIcon,                     /*!< An image, shrunk down to the height of its line
                                            (legend) or point size (point).*/
        HorizontalSeparator,           /*!< A horizontal line going across the entire legend
                                            and text area of a label.\n
                                            Only applicable for legends.*/
        HorizontalArrowRightSeparator, /*!< A horizontal arrow going across the entire legend
                                            and text area of a label.\n
                                            Only applicable for legends.*/
        ColorGradientIcon,             /*!< A color gradient which fills the entire legend.
                                            Only applicable for legends.*/
        SquareIcon,                    /*!< A square.*/
        TriangleUpwardIcon,            /*!< A triangle pointing upward.*/
        TriangleDownwardIcon,          /*!< A triangle pointing downward.*/
        TriangleRightIcon,             /*!< A triangle pointing right.*/
        TriangleLeftIcon,              /*!< A triangle pointing left.*/
        DiamondIcon,                   /*!< A diamond.*/
        PlusIcon,                      /*!< A plus sign.*/
        AsteriskIcon,                  /*!< An asterisk.*/
        HexagonIcon,                   /*!< A hexagon.*/
        BoxPlotIcon,                   /*!< A box & whisker plot.*/
        LocationMarker,                /*!< A marker indicating a location on a map.*/
        GoRoadSign,                    /*!< A circular sign that says 'Go' on it,
                                            with a sign post beneath it.*/
        WarningRoadSign,               /*!< A triangular sign containing an exclamation point,
                                            with a sign post beneath it.*/
        SunIcon,                       /*!< A sun (with sunbeams).*/
        FlowerIcon,                    /*!< A flower (stigma and petals).*/
        FallLeafIcon                   /*!< A red leaf.*/
        };

    /// @brief Item to draw on a legend.
    /// @details This can include shapes, images, or blanks for the shape
    ///     and also includes control of the color.
    struct LegendIcon
        {
        /** @brief Constructs a color gradient legend.
            @param colors The color gradient to use (must contain at least two colors).
            @note The colors are drawn top to bottom, starting from the first color.*/
        explicit LegendIcon(const std::vector<wxColour>& colors) :
            m_shape(IconShape::ColorGradientIcon), m_colors(colors)
            {
            wxASSERT_LEVEL_2_MSG(m_colors.size() >= 2,
                L"Color gradient legend created with only one color!");
            }
        /** @brief Constructs legend icon.
            @param img The image to draw as an icon.*/
        LegendIcon(const wxImage& img) :
            m_shape(IconShape::ImageIcon), m_img(img)
            {}
        /** @brief Constructor.
            @param icon The icon type.
            @param pen The pen to outline the icon with.
            @param brush The brush to paint with.*/
        LegendIcon(const IconShape icon, const wxPen& pen, const wxBrush& brush) :
            m_shape(icon), m_pen(pen), m_brush(brush), m_img(wxNullImage)
            {}

        IconShape m_shape{ IconShape::BlankIcon }; /*!< The icon type.*/
        wxPen m_pen;                               /*!< The pen to draw with.*/
        wxBrush m_brush;                           /*!< The brush to paint with.*/
        /// @brief The image to draw (if shape is set to ImageIcon).
        wxImage m_img;
        /// @brief The color gradient to draw (if shape is set to @c ColorGradientIcon).
        std::vector<wxColour> m_colors;

        /// @returns The minimum width that should be used for legend icons.
        /// @note  This is usually used by Label::SetLeftPadding() or Label::GetMinLegendWidth(),
        ///     which use DIPs.
        [[nodiscard]] static constexpr double GetIconWidthDIPs() noexcept
            { return 16; }
        /// @returns The size of arrowheads (if shape is set to @c HorizontalArrowSeparator)
        ///     in DIPs.
        [[nodiscard]] static wxSize GetArrowheadSizeDIPs()
            { return wxSize(2, 2); }
        };

    /// @brief Icons schemese for use on plots and legends.
    namespace Schemes
        {
        /// @brief Base class for a list of shapes to use for groups.
        class IconScheme
            {
        public:
            /// @brief Constructor.
            /// @param shapes The vector of shapes to fill the scheme with.
            explicit IconScheme(const std::vector<IconShape>& shapes) : m_shapes(shapes)
                {}
            /// @private
            explicit IconScheme(std::vector<IconShape>&& shapes) : m_shapes(std::move(shapes))
                {}
            /// @brief Constructor.
            /// @param shapes The initializer list of shapes to fill the scheme with.
            explicit IconScheme(const std::initializer_list<IconShape>& shapes) : m_shapes(shapes)
                {}
            /// @brief Constructor.
            /// @param shapes The list of shapes to fill the scheme with.
            /// @param images The list of images to use for the points if point is
            ///     using IconShape::ImageIcon.
            IconScheme(const std::vector<IconShape>& shapes,
                const std::vector<wxBitmapBundle>& images) :
                m_shapes(shapes), m_iconImages(images)
                {}
            /// @private
            IconScheme(std::vector<IconShape>&& shapes,
                std::vector<wxBitmapBundle>&& images) :
                m_shapes(std::move(shapes)), m_iconImages(std::move(images))
                {}
            /// @returns The list of shapes from the scheme.
            [[nodiscard]] const std::vector<IconShape>& GetShapes() const noexcept
                { return m_shapes; }
            /** @returns The shape from a given index.\n
                    If no shapes are available, returns a blank icon.
                @param index The index into the shape list to return. If index is outside
                    number of shapes, then it will recycle (i.e., wrap around).
                    For example, if there are 2 shapes, index 1 will return 1;
                    however, index 2 will wrap around and return shape 0 and
                    index 3 will return shape 1.*/
            [[nodiscard]] IconShape GetShape(const size_t index) const
                {
                return (m_shapes.size() == 0) ?
                    IconShape::BlankIcon : m_shapes.at(index % m_shapes.size());
                }
            /** @brief Adds a shape to the scheme.
                @param shape The shape to add.*/
            void AddShape(const IconShape shape)
                { m_shapes.push_back(shape); }
            /** @returns The image used for icons (if shape is set to @c IconShape::ImageIcon).\n
                    If no image(s) is available, returns an empty image (be sure to call @c IsOK()).
                @param index The index into the image list to return. If index is outside
                    number of images, then it will recycle (i.e., wrap around).
                    For example, if there are 2 images, index 1 will return 1;
                    however, index 2 will wrap around and return image 0 and
                    index 3 will return image 1.*/
            [[nodiscard]] const wxBitmapBundle& GetImage(const size_t index) const noexcept
                {
                return (m_iconImages.size() == 0) ?
                    m_emptyImage :
                    m_iconImages.at(index % m_iconImages.size());
                }
            /// @brief Removes all shapes from the collection.
            void Clear() noexcept
                { m_shapes.clear(); }
        private:
            std::vector<IconShape> m_shapes;
            std::vector<wxBitmapBundle> m_iconImages;
            wxBitmapBundle m_emptyImage;
            };

        /// @brief Standard shapes scheme.
        class StandardShapes : public IconScheme
            {
        public:
            StandardShapes() : IconScheme({ IconShape::CircleIcon,
                                            IconShape::SquareIcon,
                                            IconShape::HexagonIcon,
                                            IconShape::DiamondIcon,
                                            IconShape::TriangleUpwardIcon,
                                            IconShape::TriangleDownwardIcon,
                                            IconShape::PlusIcon,
                                            IconShape::AsteriskIcon,
                                            IconShape::TriangleRightIcon,
                                            IconShape::TriangleLeftIcon })
                {}
            };

        /// @brief Semesters (fall, spring, and summer) icon scheme.
        class Semesters : public IconScheme
            {
        public:
            Semesters() : IconScheme({ IconShape::FallLeafIcon,
                                       IconShape::FlowerIcon,
                                       IconShape::SunIcon })
                {}
            };
        }
    }

/** @}*/

#endif //__WISTERIA_ICONS_H__
