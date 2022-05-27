/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_SHAPES_H__
#define __WISTERIA_SHAPES_H__

#include "graphitems.h"

namespace Wisteria::GraphItems
    {
    /** @brief Helper class to draw shapes.
        @details This class accepts a GraphItemInfo object, which will be used
            when any @c DrawX() functions.\n
            This class is not an embeddable object placed on a canvas like
            `GraphItemBase`-deirived classes; rather, it is used by these classes
            as a way to draw commonly used shapes.*/
    class Shapes
        {
    public:
        /** @brief Constructor.
            @param itemInfo Extended information to construct this item with.*/
        explicit Shapes(const GraphItemInfo& info) : m_graphInfo(info)
            {}
        /// @private
        Shapes() = delete;
        /// @brief Draws a circle filled with the shape's brush, draws a black
        ///     outline, and draws the shape's text value in the center of it.
        /// @param rect The area to bind the circle within.
        /// @param dc The DC to draw to.
        void DrawCircularSign(const wxRect rect, wxDC& dc);
    private:
        [[nodiscard]] double GetScaling() const noexcept
            { return m_graphInfo.m_scaling; }

        double GetDPIScaleFactor() const noexcept
            {
            wxASSERT_LEVEL_2_MSG(m_graphInfo.m_dpiScaleFactor.has_value(),
                                 L"Graph item should have a proper DPI scaling.");
            return (m_graphInfo.m_dpiScaleFactor.has_value() ?
                    m_graphInfo.m_dpiScaleFactor.value() : 1);
            }

        [[nodiscard]] double ScaleToScreenAndCanvas(const double value) const noexcept
            { return value * GetScaling() * GetDPIScaleFactor(); }
        GraphItemInfo m_graphInfo;
        };
    };

/** @}*/

#endif //__WISTERIA_SHAPES_H__
