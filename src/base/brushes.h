/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_BRUSHSCHEME_H__
#define __WISTERIA_BRUSHSCHEME_H__

#include "colorbrewer.h"

/// @brief Additional brush features, such as brush schemes.
namespace Wisteria::Brushes
    {
    /// @brief Brush schemes.
    namespace Schemes
        {
        /// @brief Base class for a list of brushes to use for groups.
        class BrushScheme
            {
        public:
            /// @brief Constructor.
            /// @param brushes The vector of brushes to fill the scheme with.
            explicit BrushScheme(const std::vector<wxBrush>& brushes) : m_brushes(brushes)
                {}
            /// @brief Constructor, which builds a list of brushes from a list of
            ///     brush styles and a color scheme.
            /// @param brushStyles The vector of brush styles to fill the scheme with.
            /// @param colorScheme A color scheme to use for the brushes.
            explicit BrushScheme(const std::vector<wxBrushStyle>& brushStyles,
                                 const Colors::Schemes::ColorScheme& colorScheme)
                {
                // fill with brushes and colors
                // (which may be the default black if much less colors than brush styles)
                if (brushStyles.size() >= colorScheme.GetColors().size())
                    {
                    for (size_t i = 0; i < brushStyles.size(); ++i)
                        { m_brushes.push_back(wxBrush(colorScheme.GetColor(i), brushStyles[i])); }
                    }
                // more colors than brushes, so fill with colors and recycle brushes
                else if (brushStyles.size())
                    {
                    for (size_t i = 0; i < colorScheme.GetColors().size(); ++i)
                        {
                        m_brushes.push_back(wxBrush(colorScheme.GetColor(i),
                                                    brushStyles.at(i % brushStyles.size())) );
                        }
                    }
                }
            /// @brief Constructor, which builds a list of brushes from a color scheme
            ///     and solid patterns.
            /// @param colorScheme A color scheme to use for the brushes.
            explicit BrushScheme(const Colors::Schemes::ColorScheme& colorScheme)
                {
                for (size_t i = 0; i < colorScheme.GetColors().size(); ++i)
                    {
                    m_brushes.push_back(wxBrush(colorScheme.GetColor(i),
                                                wxBrushStyle::wxBRUSHSTYLE_SOLID));
                    }
                }
            /// @private
            explicit BrushScheme(std::vector<wxBrush>&& brushes) : m_brushes(std::move(brushes))
                {}
            /// @brief Constructor.
            /// @param brushes The initializer list of brushes to fill the scheme with.
            explicit BrushScheme(const std::initializer_list<wxBrush>& brushes) : m_brushes(brushes)
                {}
            /// @returns The list of brushes from the scheme.
            [[nodiscard]]
            const std::vector<wxBrush>& GetBrushes() const noexcept
                { return m_brushes; }
            /** @returns The brush from a given index.\n
                    If no brushes are available, returns a blank icon.
                @param index The index into the brush list to return. If index is outside
                    number of brushes, then it will recycle (i.e., wrap around).
                    For example, if there are 2 brushes, index 1 will return 1;
                    however, index 2 will wrap around and return brush 0 and
                    index 3 will return brush 1.*/
            [[nodiscard]]
            wxBrush GetBrush(const size_t index) const
                {
                return (m_brushes.size() == 0) ?
                    *wxBLACK_BRUSH : m_brushes.at(index % m_brushes.size());
                }
            /** @brief Adds a brush to the scheme.
                @param brush The brush to add.*/
            void AddBrush(const wxBrush& brush)
                { m_brushes.push_back(brush); }
            /// @private
            void AddBrush(wxBrush&& brush)
                { m_brushes.push_back(brush); }

            /// @brief Removes all brushes from the collection.
            void Clear() noexcept
                { m_brushes.clear(); }
        private:
            std::vector<wxBrush> m_brushes;
            };
        }
    }

/** @}*/

#endif //__WISTERIA_BRUSHSCHEME_H__
