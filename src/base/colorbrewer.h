/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_COLORBREWER_H
#define WISTERIA_COLORBREWER_H

#include "../math/statistics.h"
#include "colors.h"
#include <concepts>
#include <initializer_list>
#include <vector>
#include <wx/string.h>
#include <wx/wx.h>

// @private
template<typename T>
concept CssChar = std::same_as<T, char> || std::same_as<T, wchar_t>;

/// @brief Color management features (building, brewing, contrasting, etc.).
/// @sa The [color management](../../Colors.md) overview for more information.
namespace Wisteria::Colors
    {
    /** @brief Constructs a color scale for a given range of values.
        @details Brews values within that range to a color representing its
            position on the color scale.

        @code
         using namespace Wisteria::Colors;

         ColorBrewer cb;
         cb.SetColorScale(
             {
             // the color for the min value
             Colors::ColorBrewer::GetColor(Colors::Color::Blue),
             // the color for the max value (because it's the last color added)
             ColorBrewer::GetColor(Color::Red)
             });

         std::vector<double> data =
            {
            50,   // max value (will be red)
            1,    // min value (will be blue)
            25.5  // in between value (will be purple)
            };

         const auto res = cb.BrewColors(data.cbegin(), data.cend());

         // an initializer list could also be used:
         // const auto res = cb.BrewColors({ 50.0, 1.0, 25.5 });

         // res[0] will be red, res[1] will be blue, and res[2] will be purple
        @endcode
       */
    class ColorBrewer
        {
      public:
        /// @brief Converts RGBA values into a @c wxUint32 that can be
        ///     used with @c wxColour::SetRGBA().
        /// @param red The red channel.
        /// @param green The green channel.
        /// @param blue The blue channel.
        /// @param alpha The alpha channel.
        /// @returns The value to pass to @c wxColour::SetRGBA().
        [[nodiscard]]
        constexpr static wxUint32 RGBA(const unsigned char red, const unsigned char green,
                                       const unsigned char blue, const unsigned char alpha) noexcept
            {
            wxUint32 color = 0;
            // note that channels go in reverse order
            color |= static_cast<wxUint32>(alpha) << 24;
            color |= static_cast<wxUint32>(blue) << 16;
            color |= static_cast<wxUint32>(green) << 8;
            color |= static_cast<wxUint32>(red);
            return color;
            }

        /// @brief Convert a CSS-style hex color string literal to a long value.
        /// @tparam charT Character type (`char` or `wchar_t`).
        /// @tparam N Array size (deduced from the string literal).
        /// @param css CSS color string literal (must be exactly "#RRGGBB").
        /// @return Encoded color value (0xBBGGRR), or `-1` if invalid.
        /// @note This performs compile-time validation of string length.
        /// @code
        /// constexpr long red = ColorBrewer::CSS_HEX_TO_LONG("#FF0000"); // OK
        /// constexpr long bad = ColorBrewer::CSS_HEX_TO_LONG("#123");    // Compile error
        /// @endcode
        // NOLINTBEGIN(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)
        template<CssChar charT, std::size_t N>
        constexpr static long CSS_HEX_TO_LONG(const charT (&css)[N])
            {
            if (css == nullptr)
                {
                return -1;
                }
            if (css[0] != static_cast<charT>('#'))
                {
                return -1;
                }

            // hex conversion
            constexpr auto hexDigit = [](charT c) -> int
            {
                if (c >= static_cast<charT>('0') && c <= static_cast<charT>('9'))
                    {
                    return c - static_cast<charT>('0');
                    }
                if (c >= static_cast<charT>('a') && c <= static_cast<charT>('f'))
                    {
                    return 10 + (c - static_cast<charT>('a'));
                    }
                if (c >= static_cast<charT>('A') && c <= static_cast<charT>('F'))
                    {
                    return 10 + (c - static_cast<charT>('A'));
                    }
                return -1;
            };

            const int r1 = hexDigit(css[1]), r2 = hexDigit(css[2]);
            const int g1 = hexDigit(css[3]), g2 = hexDigit(css[4]);
            const int b1 = hexDigit(css[5]), b2 = hexDigit(css[6]);

            if (r1 < 0 || r2 < 0 || g1 < 0 || g2 < 0 || b1 < 0 || b2 < 0)
                {
                return -1;
                }

            const int r = r1 * 16 + r2;
            const int g = g1 * 16 + g2;
            const int b = b1 * 16 + b2;

            return (static_cast<long>(b) << 16) | (static_cast<long>(g) << 8) |
                   static_cast<long>(r);
            }

        // NOLINTEND(cppcoreguidelines-avoid-c-arrays,hicpp-avoid-c-arrays,modernize-avoid-c-arrays,cppcoreguidelines-pro-bounds-array-to-pointer-decay,hicpp-no-array-decay)

        /** @brief Creates a color from a Colors::Color value.
            @returns A color from a list of known colors.
            @param color The color ID to use.*/
        [[nodiscard]]
        static wxColour GetColor(Colors::Color color);

        /** @brief Creates a color from a Colors::Color value and applies an opacity to it.
            @returns A color from a list of known colors.
            @param color The color ID to use.
            @param opacity The opacity to set the color.*/
        [[nodiscard]]
        static wxColour GetColor(const Colors::Color color, const uint8_t opacity)
            {
            auto foundColor = GetColor(color);
            return { foundColor.Red(), foundColor.Green(), foundColor.Blue(), opacity };
            }

        /** @brief Initializes the color scale to map to the range of values.
            @param start The start of a range of color objects (this will be the min value).
            @param end The end of a range of color objects (this will be the max value).*/
        template<typename T>
        void SetColorScale(const T start, const T end)
            {
            m_colorSpectrum.clear();
            m_colorSpectrum.insert(m_colorSpectrum.begin(), start, end);
            }

        /** @brief Initializes the color scale to map to the range of values.
            @param colors The colors to map values to. The first color in the list
             will map to the data's min value, and the last color will map to the data's max
           value.*/
        void SetColorScale(const std::initializer_list<wxColour>& colors)
            {
            m_colorSpectrum.clear();
            m_colorSpectrum.insert(m_colorSpectrum.begin(), colors.begin(), colors.end());
            }

        /** @brief Converts a range of numbers into a sequence of color values.
            @details The color values for each number represent where it falls on the color scale,
                relative to the overall range of values.
            @param start The start of the data.
            @param end The end of the data.
            @returns A vector of colors respective to each value in the data.
            @note Any NaN values in the range will be mapped to an invalid `wxColour`,
                so be sure to call `IsOk()` when using the returned colors.*/
        template<typename T>
        [[nodiscard]]
        std::vector<wxColour> BrewColors(const T start, const T end)
            {
            std::vector<double> validColorData;
            std::copy_if(start, end, std::back_inserter(validColorData),
                         [](auto x) { return std::isfinite(x); });
            m_range.first = *std::ranges::min_element(std::as_const(validColorData));
            m_range.second = *std::ranges::max_element(std::as_const(validColorData));

            const size_t rangeSize = std::distance(start, end);

            std::vector<wxColour> colors(rangeSize, wxColour{});
            for (size_t i = 0; i < rangeSize; ++i)
                {
                colors[i] = BrewColor(start[i]);
                }
            return colors;
            }

        /** @brief Converts a range of numbers into a sequence of color values.
            @details The color values for each number represent where it falls on the color scale,
                relative to the overall range of values.
            @param values The data to convert into a series of colors.
            @returns A vector of colors respective to each value in the data.
            @note Any NaN values in the range will be mapped to an invalid `wxColour`,
                so be sure to call `IsOk()` when using the returned colors.*/
        template<typename T>
        [[nodiscard]]
        std::vector<wxColour> BrewColors(const std::initializer_list<T>& values)
            {
            std::vector<double> validColorData;
            std::copy_if(values.cbegin(), values.cend(), std::back_inserter(validColorData),
                         [](auto x) { return std::isfinite(x); });
            m_range.first = *std::ranges::min_element(std::as_const(validColorData));
            m_range.second = *std::ranges::max_element(std::as_const(validColorData));

            std::vector<wxColour> colors;
            colors.reserve(values.size());
            for (const auto& value : values)
                {
                colors.push_back(BrewColor(value));
                }
            return colors;
            }

        /** @brief Returns the calculated min and max of the values from the last
                call to BrewColors().
            @returns The min and max of the values represented by the color scale.
            @sa BrewColors().*/
        [[nodiscard]]
        std::pair<double, double> GetRange() const noexcept
            {
            return m_range;
            }

        /** @brief Converts a value from the range into a color laying on the
                color scale mapped to that range.
            @details This should be called after a call to BrewColors(), which will
                establish the color spectrum across a range of values.
            @param value The value (should be within the original range) to convert.
            @returns The color that represents the value on our color scale.
                Will return an empty color if @c value is NaN;
                be sure to call `IsOk()` on the returned color.
            @warning The value passed here should be within the range of data previously
                passed to BrewColors(); otherwise, it will re-adjust the color/value mapping
                and invalidate previous calls to BrewColor(s).
            @note This code is adapted from
           http://andrewnoske.com/wiki/Code_-_heatmaps_and_color_gradients.*/
        [[nodiscard]]
        wxColour BrewColor(double value) const;

        /// @returns The official shade of navy blue for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseNavyBlue()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#000966") };
            }

        /// @returns The official shade of royal blue for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseRoyalBlue()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#2C1CD8") };
            }

        /// @returns The official shade of light blue for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseLightBlue()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#E2F4FF") };
            }

        /// @returns The official shade of mint for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseMint()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#54FFD4") };
            }

        /// @returns The official shade of orange for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseOrange()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#F06C02") };
            }

        /// @returns The official shade of black for Eclipse branding.
        [[nodiscard]]
        static wxColour GetEclipseBlack()
            {
            return wxColour{ Wisteria::Colors::ColorBrewer::CSS_HEX_TO_LONG(L"#000000") };
            }

      private:
        std::pair<double, double> m_range{ 0, 0 };
        std::vector<wxColour> m_colorSpectrum;
        static const std::vector<wxUint32> m_colors;
        };

    /// @brief Adjusts a color to contrast against another color.
    class ColorContrast
        {
      public:
        /// @brief Constructor.
        /// @param color The base color to contrast other colors against.
        explicit ColorContrast(const wxColour& color) : m_baseColor(color)
            {
            wxASSERT_MSG(m_baseColor.IsOk(), L"Invalid base color passed to ColorContrast.");
            }

        /// @returns A variation of @c color that is adjusted to contrast against the base color
        ///     (that was set in the constructor).
        /// @param color The color to adjust so that it contrasts.
        [[nodiscard]]
        wxColour Contrast(const wxColour& color) const;

        /// @returns A variation of @c color with a different opacity.
        /// @param color The base color to apply an opacity to.
        /// @param opacity The opacity to use for the new color.
        [[nodiscard]]
        static wxColour ChangeOpacity(const wxColour& color, const uint8_t opacity)
            {
            wxASSERT_MSG(color.IsOk(), L"Invalid color passed to ChangeOpacity().");
            return (color.IsOk() ? wxColor(color.Red(), color.Green(), color.Blue(), opacity) :
                                   color);
            }

        /// @brief Determines whether a color is dark.
        /// @details "Dark" is defined as luminance being less than 50% and
        ///     opacity higher than 32. For example, black having an opacity of 32
        ///     would mean it has 1/8 of the opacity of a fully opaque black;
        ///     this would appear more like a very light gray, rather than black, and would
        ///     be considered not dark.
        /// @param color The color to review.
        /// @returns @c true if the color is dark.
        [[nodiscard]]
        static bool IsDark(const wxColour& color)
            {
            wxASSERT_MSG(color.IsOk(), L"Invalid color passed to IsDark().");
            return (color.IsOk() && color.Alpha() > 32 &&
                    color.GetLuminance() < math_constants::half);
            }

        /// @brief Determines whether a color is light
        ///     (i.e., luminance is >= 50% and not heavily translucent).
        /// @param color The color to review.
        /// @returns @c true if the color's luminance is >= 50%.
        [[nodiscard]]
        static bool IsLight(const wxColour& color)
            {
            return !IsDark(color);
            }

        /// @returns A darkened version of a color.
        /// @param color The base color to darken.
        /// @param minimumLuminance The minimum darkness of the color,
        ///     ranging from @c 0.0 to @c 1.0 (the lower, the darker).
        [[nodiscard]]
        static wxColour Shade(wxColour color, double minimumLuminance = math_constants::half)
            {
            wxASSERT_MSG(color.IsOk(), L"Invalid color passed to Shade().");
            minimumLuminance =
                std::clamp(minimumLuminance, math_constants::empty, math_constants::full);
            int darkenValue{ 100 };
            while (color.GetLuminance() > minimumLuminance && darkenValue > 0)
                {
                color = color.ChangeLightness(--darkenValue);
                }
            return color;
            }

        /// @returns A lightened version of a color.
        /// @param color The base color to darken.
        /// @param maximumLuminance The maximum lightness of the color,
        ///     ranging from @c 0.0 to @c 1.0 (the higher, the lighter).
        [[nodiscard]]
        static wxColour Tint(wxColour color,
                             double maximumLuminance = math_constants::three_quarters)
            {
            wxASSERT_MSG(color.IsOk(), L"Invalid color passed to Shade().");
            maximumLuminance =
                std::clamp(maximumLuminance, math_constants::empty, math_constants::half);
            int lightenValue{ 100 };
            while (color.GetLuminance() < maximumLuminance)
                {
                color = color.ChangeLightness(++lightenValue);
                }
            return color;
            }

        /// @brief Returns a darker (shaded) or lighter (tinted) version of a color,
        ///     depending on how dark it is to begin with.
        ///     For example, black will be returned as dark gray,
        ///     while white will return as an eggshell white.
        /// @param color The color to shade.
        /// @param shadeOrTintValue How much to lighten or darken a color
        ///      (should be between @c 0.0 to @c 1.0.)
        /// @returns The shaded or tinted color.
        [[nodiscard]]
        static wxColour ShadeOrTint(const wxColour& color,
                                    const double shadeOrTintValue = math_constants::fifth)
            {
            return (IsDark(color) ?
                        color.ChangeLightness(
                            100 + std::clamp(static_cast<int>(shadeOrTintValue * 100), 0, 100)) :
                        color.ChangeLightness(
                            100 - std::clamp(static_cast<int>(shadeOrTintValue * 100), 0, 100)));
            }

        /// @brief Returns either black or white, depending on which better contrasts
        ///     against the specified color.
        /// @param color The color to contrast against to see if white or black should go on it.
        /// @returns Black or white; whichever contrasts better against @c color.
        [[nodiscard]]
        static wxColour BlackOrWhiteContrast(const wxColour& color)
            {
            return (IsDark(color) ? ColorBrewer::GetColor(Color::White) :
                                    ColorBrewer::GetColor(Color::Black));
            }

        /// @returns @c true if two colors' luminance values are close.
        /// @param color1 First color to compare.
        /// @param color2 Second color to compare.
        /// @param delta The difference threshold to use when comparing.
        ///     Should be between @c 0.0 to @c 1.0.
        /// @note Returns @c false if either color is invalid.
        [[nodiscard]]
        static bool AreColorsClose(const wxColour& color1, const wxColour& color2,
                                   const double delta = math_constants::tenth)
            {
            wxASSERT_MSG(color1.IsOk() && color2.IsOk(),
                         L"Invalid color passed to AreColorsClose().");
            return (color1.IsOk() && color2.IsOk() &&
                    (std::abs(color1.GetLuminance() - color2.GetLuminance())) <=
                        std::clamp(delta, math_constants::empty, math_constants::full));
            }

        /// @brief Shades or tints a color if close to another color (e.g., a background color).
        /// @param mainColor The color to adjust (if necessary).
        /// @param secondaryColor The base color to compare against.
        /// @returns If @c mainColor is close to @c secondaryColor,
        ///     then returns a shaded or tinted version of @c mainColor; otherwise,
        ///     returns the original @c mainColor.
        [[nodiscard]]
        static wxColour ShadeOrTintIfClose(const wxColour& mainColor,
                                           const wxColour& secondaryColor)
            {
            return AreColorsClose(mainColor, secondaryColor) ? ShadeOrTint(mainColor, .40f) :
                                                               mainColor;
            }

      private:
        wxColour m_baseColor{ ColorBrewer::GetColor(Color::White) };
        constexpr static double M_TOLERANCE{ math_constants::half };
        };

    /// @brief Color schemes to use for grouped data.
    /// @details Some schemes created by Paul Gernale and shared on
    ///     <a href='https://www.canva.com'>www.canva.com</a>.
    namespace Schemes
        {
        /// @brief Base class for creating a color scheme.
        class ColorScheme
            {
          public:
            /** @brief Constructor.
                @param colors The initializer list of colors to fill the scheme with.
                @note A series of shaded or tinted versions of these colors will also
                    be added to this list of colors, essentially double the color count.*/
            ColorScheme(std::initializer_list<wxColour> colors) : m_colors(colors) {}

            /** @brief Constructor.
                @param colors The initializer list of colors to fill the scheme with.
                @note A series of shaded or tinted versions of these colors will also
                    be added to this list of colors, essentially double the color count.*/
            explicit ColorScheme(const std::vector<wxColour>& colors) : m_colors(colors) {}

            /// @private
            explicit ColorScheme(std::vector<wxColour>&& colors) : m_colors(std::move(colors)) {}

            /** @brief Gets the list of colors from the scheme.
                @returns The scheme's colors.*/
            [[nodiscard]]
            const std::vector<wxColour>& GetColors() const noexcept
                {
                return m_colors;
                }

            /** @brief Gets the color from a given index.
                @param index The index into the color list to return.
                    If index is outside the color scheme but within double the
                    size of the scheme, then scheme will "wrap around" and
                    return a shaded or tinted version.
                    If outside twice the number of colors, then returns white.

                    For example, if you have 8 colors and pass in an index of 7,
                    then it will return the last color.\n
                    If you pass in index 9, then will return a shaded/tinted version
                    of the second color.\n
                    Index 15 will return a shaded/tinted version of the last color,
                    and index 16 will return white.
                @sa GetRecycledColor().
                @returns The color from a given index,
                    or white if scheme is empty.*/
            [[nodiscard]]
            wxColour GetColor(const size_t index) const
                {
                return (m_colors.empty()              ? ColorBrewer::GetColor(Color::White) :
                        (index < m_colors.size())     ? m_colors.at(index) :
                        (index < m_colors.size() * 2) ? ColorContrast::ShadeOrTint(
                                                            m_colors.at(index % m_colors.size())) :
                                                        ColorBrewer::GetColor(Color::White));
                }

            /** @brief Gets the color from a given index and applies an opacity value to it.
                @param index The index into the color list to return.
                @param opacity The opacity to set the color.
                @returns The color from given index, or white if index is invalid.*/
            [[nodiscard]]
            wxColour GetColor(const size_t index, const uint8_t opacity) const
                {
                auto color = GetColor(index);
                return { color.Red(), color.Green(), color.Blue(), opacity };
                }

            /** @brief Gets the color from a given index.
                @param index The index into the color scheme to return. If index is outside
                    number of colors, then it will recycle (i.e., wrap around).
                    For example, if there are 2 colors, index 1 will return 1;
                    however, index 2 will wrap around and return colors 0 and
                    colors 3 will return colors 1.
                @sa GetColor().
                @returns The (possibly recycled) color from a given index,
                    or white if scheme is empty.*/
            [[nodiscard]]
            wxColour GetRecycledColor(const size_t index) const
                {
                return (m_colors.empty() ? ColorBrewer::GetColor(Color::White) :
                                           m_colors.at(index % m_colors.size()));
                }

            /** @brief Adds a color to the scheme.
                @param color The color to add.*/
            void AddColor(const wxColour& color)
                {
                assert(color.IsOk() && L"Invalid color passed to AddColor().");
                m_colors.push_back(color);
                }

            /// @brief Removes all colors from the collection.
            void Clear() noexcept { m_colors.clear(); }

          private:
            /// @brief The colors in the scheme.
            std::vector<wxColour> m_colors;
            };

        /** @brief An equidistant color scheme, with darker tones reminiscent
             of a sunset.
             \htmlonly
             <div style='background-color:#003F5C; width:50px;'>&nbsp;</div>
             <div style='background-color:#2F4B7C; width:50px;'>&nbsp;</div>
             <div style='background-color:#665191; width:50px;'>&nbsp;</div>
             <div style='background-color:#A05195; width:50px;'>&nbsp;</div>
             <div style='background-color:#D45087; width:50px;'>&nbsp;</div>
             <div style='background-color:#F95D6A; width:50px;'>&nbsp;</div>
             <div style='background-color:#FF7C43; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFA600; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Dusk : public ColorScheme
            {
          public:
            Dusk()
                : ColorScheme({ wxColour(L"#003F5C"), wxColour(L"#2F4B7C"), wxColour(L"#665191"),
                                wxColour(L"#A05195"), wxColour(L"#D45087"), wxColour(L"#F95D6A"),
                                wxColour(L"#FF7C43"), wxColour(L"#FFA600") })
                {
                }
            };

        /** @brief An Earth tones themed color scheme.
             \htmlonly
             <div style='background-color:rgb(186,150,155); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(110,80,69); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(202,80,69); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(102,131,145); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(154,131,97); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(41,109,91); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(140,74,86); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(238,221,130); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(176,48,96); width:50px;'>&nbsp;</div>
             <div style='background-color:rgb(205,150,205); width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class EarthTones : public ColorScheme
            {
          public:
            EarthTones()
                : ColorScheme({ wxColour(186, 150, 155), wxColour(110, 80, 69),
                                wxColour(202, 80, 69), wxColour(102, 131, 145),
                                wxColour(154, 131, 97), wxColour(41, 109, 91),
                                wxColour(140, 74, 86), wxColour(238, 221, 130),
                                wxColour(176, 48, 96), wxColour(205, 150, 205) })
                {
                }
            };

        /** @brief A 1920s themed color scheme.
             \htmlonly
             <div style='background-color:#9E3E33; width:50px;'>&nbsp;</div>
             <div style='background-color:#F1BFB1; width:50px;'>&nbsp;</div>
             <div style='background-color:#CBD0C2; width:50px;'>&nbsp;</div>
             <div style='background-color:#598C74; width:50px;'>&nbsp;</div>
             <div style='background-color:#AB7878; width:50px;'>&nbsp;</div>
             <div style='background-color:#C6B9B8; width:50px;'>&nbsp;</div>
             <div style='background-color:#ABD1C9; width:50px;'>&nbsp;</div>
             <div style='background-color:#014E4C; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1920s : public ColorScheme
            {
          public:
            Decade1920s()
                : ColorScheme({ ColorBrewer::GetColor(Color::ChineseRed),
                                ColorBrewer::GetColor(Color::JazzAgeCoral),
                                ColorBrewer::GetColor(Color::Frostwork),
                                ColorBrewer::GetColor(Color::Alexandrite),
                                ColorBrewer::GetColor(Color::SalonRose),
                                ColorBrewer::GetColor(Color::StudioMauve),
                                ColorBrewer::GetColor(Color::BlueSky),
                                ColorBrewer::GetColor(Color::HunterGreen) })
                {
                }
            };

        /** @brief A 1940s themed color scheme.
             \htmlonly
             <div style='background-color:#4B5645; width:50px;'>&nbsp;</div>
             <div style='background-color:#ACB19F; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0CDA0; width:50px;'>&nbsp;</div>
             <div style='background-color:#C2CCC4; width:50px;'>&nbsp;</div>
             <div style='background-color:#CD9C85; width:50px;'>&nbsp;</div>
             <div style='background-color:#DEC3B9; width:50px;'>&nbsp;</div>
             <div style='background-color:#BC9C9E; width:50px;'>&nbsp;</div>
             <div style='background-color:#623F45; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1940s : public ColorScheme
            {
          public:
            Decade1940s()
                : ColorScheme({ ColorBrewer::GetColor(Color::VogueGreen),
                                ColorBrewer::GetColor(Color::CascadeGreen),
                                ColorBrewer::GetColor(Color::BelvedereCream),
                                ColorBrewer::GetColor(Color::Cream),
                                ColorBrewer::GetColor(Color::RoseTan),
                                ColorBrewer::GetColor(Color::PinkShadow),
                                ColorBrewer::GetColor(Color::Orchid),
                                ColorBrewer::GetColor(Color::Maroon) })
                {
                }
            };

        /** @brief A 1950s themed color scheme.
             \htmlonly
             <div style='background-color:#E1D286; width:50px;'>&nbsp;</div>
             <div style='background-color:#A489A0; width:50px;'>&nbsp;</div>
             <div style='background-color:#8AC6BD; width:50px;'>&nbsp;</div>
             <div style='background-color:#DAB5B4; width:50px;'>&nbsp;</div>
             <div style='background-color:#888782; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0D39D; width:50px;'>&nbsp;</div>
             <div style='background-color:#C9AA98; width:50px;'>&nbsp;</div>
             <div style='background-color:#CD717B; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1950s : public ColorScheme
            {
          public:
            Decade1950s()
                : ColorScheme({ ColorBrewer::GetColor(Color::Lime),
                                ColorBrewer::GetColor(Color::RadiantLilac),
                                ColorBrewer::GetColor(Color::Turquoise),
                                ColorBrewer::GetColor(Color::Appleblossom),
                                ColorBrewer::GetColor(Color::ClassicFrenchGray),
                                ColorBrewer::GetColor(Color::SunbeamYellow),
                                ColorBrewer::GetColor(Color::PinkyBeige),
                                ColorBrewer::GetColor(Color::PinkFlamingo) })
                {
                }
            };

        /** @brief A 1960s themed color scheme.
             \htmlonly
             <div style='background-color:#EC8430; width:50px;'>&nbsp;</div>
             <div style='background-color:#FED340; width:50px;'>&nbsp;</div>
             <div style='background-color:#B54D7F; width:50px;'>&nbsp;</div>
             <div style='background-color:#D9C661; width:50px;'>&nbsp;</div>
             <div style='background-color:#716998; width:50px;'>&nbsp;</div>
             <div style='background-color:#2F2F30; width:50px;'>&nbsp;</div>
             <div style='background-color:#EDECE6; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1960s : public ColorScheme
            {
          public:
            Decade1960s()
                : ColorScheme({ ColorBrewer::GetColor(Color::Navel),
                                ColorBrewer::GetColor(Color::Daisy),
                                ColorBrewer::GetColor(Color::ExuberantPink),
                                ColorBrewer::GetColor(Color::Frolic),
                                ColorBrewer::GetColor(Color::ForgetMeNot),
                                ColorBrewer::GetColor(Color::TricornBlack),
                                ColorBrewer::GetColor(Color::PureWhite) })
                {
                }
            };

        /** @brief A 1970s themed color scheme.
             \htmlonly
             <div style='background-color:#857C5D; width:50px;'>&nbsp;</div>
             <div style='background-color:#D28240; width:50px;'>&nbsp;</div>
             <div style='background-color:#D69969; width:50px;'>&nbsp;</div>
             <div style='background-color:#815D40; width:50px;'>&nbsp;</div>
             <div style='background-color:#C9B29C; width:50px;'>&nbsp;</div>
             <div style='background-color:#FBCB78; width:50px;'>&nbsp;</div>
             <div style='background-color:#B1975F; width:50px;'>&nbsp;</div>
             <div style='background-color:#DA9100; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFDB58; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1970s : public ColorScheme
            {
          public:
            Decade1970s()
                : ColorScheme({ ColorBrewer::GetColor(Color::Avocado),
                                ColorBrewer::GetColor(Color::AmberWave),
                                ColorBrewer::GetColor(Color::FolksyGold),
                                ColorBrewer::GetColor(Color::JuteBrown),
                                ColorBrewer::GetColor(Color::PracticalBeige),
                                ColorBrewer::GetColor(Color::Afternoon),
                                ColorBrewer::GetColor(Color::EdgyGold),
                                ColorBrewer::GetColor(Color::HarvestGold),
                                ColorBrewer::GetColor(Color::Mustard) })
                {
                }
            };

        /** @brief A 1980s themed color scheme.
             \htmlonly
             <div style='background-color:#b89d9a; width:50px;'>&nbsp;</div>
             <div style='background-color:#ede1ce; width:50px;'>&nbsp;</div>
             <div style='background-color:#8aa3b1; width:50px;'>&nbsp;</div>
             <div style='background-color:#c6c0b6; width:50px;'>&nbsp;</div>
             <div style='background-color:#f4d3b3; width:50px;'>&nbsp;</div>
             <div style='background-color:#75b9ae; width:50px;'>&nbsp;</div>
             <div style='background-color:#ebcecb; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1980s : public ColorScheme
            {
          public:
            Decade1980s()
                : ColorScheme({ ColorBrewer::GetColor(Color::DressyRose),
                                ColorBrewer::GetColor(Color::Cream),
                                ColorBrewer::GetColor(Color::FavoriteJeans),
                                ColorBrewer::GetColor(Color::Cream),
                                ColorBrewer::GetColor(Color::FlatteringPeach),
                                ColorBrewer::GetColor(Color::CooledBlue),
                                ColorBrewer::GetColor(Color::RosyOutlook) })
                {
                }
            };

        /** @brief A 1990s themed color scheme.
             \htmlonly
             <div style='background-color:#E2B6A7; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDBFB0; width:50px;'>&nbsp;</div>
             <div style='background-color:#7B3730; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0EADC; width:50px;'>&nbsp;</div>
             <div style='background-color:#CFC0AB; width:50px;'>&nbsp;</div>
             <div style='background-color:#B2AC96; width:50px;'>&nbsp;</div>
             <div style='background-color:#C0A98B; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDB592; width:50px;'>&nbsp;</div>
             <div style='background-color:#B0785C; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade1990s : public ColorScheme
            {
          public:
            Decade1990s()
                : ColorScheme({ ColorBrewer::GetColor(Color::SmokySalmon),
                                ColorBrewer::GetColor(Color::BungalowBeige),
                                ColorBrewer::GetColor(Color::Fireweed),
                                ColorBrewer::GetColor(Color::DoverWhite),
                                ColorBrewer::GetColor(Color::UrbanPutty),
                                ColorBrewer::GetColor(Color::SvelteSage),
                                ColorBrewer::GetColor(Color::BasketBeige),
                                ColorBrewer::GetColor(Color::WholeWheat),
                                ColorBrewer::GetColor(Color::SpicedCider) })
                {
                }
            };

        /** @brief A 2000s themed color scheme.
             \htmlonly
             <div style='background-color:#564537; width:50px;'>&nbsp;</div>
             <div style='background-color:#BAA185; width:50px;'>&nbsp;</div>
             <div style='background-color:#D7C5AE; width:50px;'>&nbsp;</div>
             <div style='background-color:#E5E2DA; width:50px;'>&nbsp;</div>
             <div style='background-color:#88ABB4; width:50px;'>&nbsp;</div>
             <div style='background-color:#CDD2CA; width:50px;'>&nbsp;</div>
             <div style='background-color:#C2CFCF; width:50px;'>&nbsp;</div>
             <div style='background-color:#B4CCC9; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Decade2000s : public ColorScheme
            {
          public:
            Decade2000s()
                : ColorScheme({ ColorBrewer::GetColor(Color::HickorySmoke),
                                ColorBrewer::GetColor(Color::Latte),
                                ColorBrewer::GetColor(Color::Khaki),
                                ColorBrewer::GetColor(Color::OrigamiWhite),
                                ColorBrewer::GetColor(Color::Aquitaine),
                                ColorBrewer::GetColor(Color::Seashell),
                                ColorBrewer::GetColor(Color::Tradewind),
                                ColorBrewer::GetColor(Color::Watery) })
                {
                }
            };

        /** @brief An %October themed color scheme, including traditional
             Halloween colors.
             \htmlonly
             <div style='background-color:#FFA500; width:50px;'>&nbsp;</div>
             <div style='background-color:#000000; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFFF00; width:50px;'>&nbsp;</div>
             <div style='background-color:#A52A2A; width:50px;'>&nbsp;</div>
             <div style='background-color:#DC143C; width:50px;'>&nbsp;</div>
             <div style='background-color:#B6B8A5; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class October : public ColorScheme
            {
          public:
            October()
                : ColorScheme(
                      { ColorBrewer::GetColor(Color::Orange), ColorBrewer::GetColor(Color::Black),
                        ColorBrewer::GetColor(Color::Yellow), ColorBrewer::GetColor(Color::Brown),
                        ColorBrewer::GetColor(Color::Crimson),
                        ColorBrewer::GetColor(Color::OctoberMist) })
                {
                }
            };

        /** @brief %Slytherin's house color scheme.
             \htmlonly
             <div style='background-color:#1A472A; width:50px;'>&nbsp;</div>
             <div style='background-color:#2A623D; width:50px;'>&nbsp;</div>
             <div style='background-color:#5D5D5D; width:50px;'>&nbsp;</div>
             <div style='background-color:#AAAAAA; width:50px;'>&nbsp;</div>
             <div style='background-color:#000000; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Slytherin : public ColorScheme
            {
          public:
            Slytherin()
                : ColorScheme({ ColorBrewer::GetColor(Color::Slytherin1),
                                ColorBrewer::GetColor(Color::Slytherin2),
                                ColorBrewer::GetColor(Color::Slytherin3),
                                ColorBrewer::GetColor(Color::Slytherin4),
                                ColorBrewer::GetColor(Color::Black) })
                {
                }
            };

        /** @brief %Campfire color scheme.
             \htmlonly
             <div style='background-color:#F55449; width:50px;'>&nbsp;</div>
             <div style='background-color:#1B4B5A; width:50px;'>&nbsp;</div>
             <div style='background-color:#0F1F38; width:50px;'>&nbsp;</div>
             <div style='background-color:#8E7970; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Campfire : public ColorScheme
            {
          public:
            Campfire()
                : ColorScheme({ ColorBrewer::GetColor(Color::Firework),
                                ColorBrewer::GetColor(Color::Oceanic),
                                ColorBrewer::GetColor(Color::Onyx),
                                ColorBrewer::GetColor(Color::Taupe) })
                {
                }
            };

        /** @brief %Coffee shop color scheme.
             \htmlonly
             <div style='background-color:#CDCDC0; width:50px;'>&nbsp;</div>
             <div style='background-color:#B38867; width:50px;'>&nbsp;</div>
             <div style='background-color:#DDBC95; width:50px;'>&nbsp;</div>
             <div style='background-color:#626D71; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class CoffeeShop : public ColorScheme
            {
          public:
            CoffeeShop()
                : ColorScheme(
                      { ColorBrewer::GetColor(Color::Ceramic), ColorBrewer::GetColor(Color::Coffee),
                        ColorBrewer::GetColor(Color::Latte), ColorBrewer::GetColor(Color::Slate) })
                {
                }
            };

        /** @brief Arctic color scheme.
             \htmlonly
             <div style='background-color:#1995AD; width:50px;'>&nbsp;</div>
             <div style='background-color:#A1D6E2; width:50px;'>&nbsp;</div>
             <div style='background-color:#F1F1F2; width:50px;'>&nbsp;</div>
             <div style='background-color:#BCBABE; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class ArcticChill : public ColorScheme
            {
          public:
            ArcticChill()
                : ColorScheme({ ColorBrewer::GetColor(Color::GlacierBlue),
                                ColorBrewer::GetColor(Color::Ice),
                                ColorBrewer::GetColor(Color::Overcast),
                                ColorBrewer::GetColor(Color::WarmGray) })
                {
                }
            };

        /** @brief School inspired color scheme (i.e., pencil and eraser colors).
             \htmlonly
             <div style='background-color:#E38533; width:50px;'>&nbsp;</div>
             <div style='background-color:#FAAE3D; width:50px;'>&nbsp;</div>
             <div style='background-color:#E4535E; width:50px;'>&nbsp;</div>
             <div style='background-color:#81715E; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class BackToSchool : public ColorScheme
            {
          public:
            BackToSchool()
                : ColorScheme({ ColorBrewer::GetColor(Color::Orange),
                                ColorBrewer::GetColor(Color::PencilYellow),
                                ColorBrewer::GetColor(Color::PinkEraser),
                                ColorBrewer::GetColor(Color::Wood) })
                {
                }
            };

        /** @brief %Colors from a box of chocolates.
             \htmlonly
             <div style='background-color:#523634; width:50px;'>&nbsp;</div>
             <div style='background-color:#301B28; width:50px;'>&nbsp;</div>
             <div style='background-color:#DDC5A2; width:50px;'>&nbsp;</div>
             <div style='background-color:#B6452C; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class BoxOfChocolates : public ColorScheme
            {
          public:
            BoxOfChocolates()
                : ColorScheme({ ColorBrewer::GetColor(Color::Chocolate), wxColour(L"#301B28"),
                                ColorBrewer::GetColor(Color::Frosting),
                                ColorBrewer::GetColor(Color::Toffee) })
                {
                }
            };

        /** @brief %Cosmopolitan color scheme.
             \htmlonly
             <div style='background-color:#8593AE; width:50px;'>&nbsp;</div>
             <div style='background-color:#DDA288; width:50px;'>&nbsp;</div>
             <div style='background-color:#4F4A45; width:50px;'>&nbsp;</div>
             <div style='background-color:#5A4E4D; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Cosmopolitan : public ColorScheme
            {
          public:
            Cosmopolitan()
                : ColorScheme({ wxColour(L"#8593AE"), ColorBrewer::GetColor(Color::Blush),
                                ColorBrewer::GetColor(Color::Pewter),
                                ColorBrewer::GetColor(Color::Steel) })
                {
                }
            };

        /** @brief Day & night color scheme.
             \htmlonly
             <div style='background-color:#063852; width:50px;'>&nbsp;</div>
             <div style='background-color:#E6DF44; width:50px;'>&nbsp;</div>
             <div style='background-color:#011A27; width:50px;'>&nbsp;</div>
             <div style='background-color:#F0810F; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class DayAndNight : public ColorScheme
            {
          public:
            DayAndNight()
                : ColorScheme({ ColorBrewer::GetColor(Color::Blueberry),
                                ColorBrewer::GetColor(Color::Daffodil), wxColour(L"#011A27"),
                                ColorBrewer::GetColor(Color::Tangerine) })
                {
                }
            };

        /** @brief %Colors from a bouquet of flowers.
             \htmlonly
             <div style='background-color:#98DBC6; width:50px;'>&nbsp;</div>
             <div style='background-color:#E6D72A; width:50px;'>&nbsp;</div>
             <div style='background-color:#F18D9E; width:50px;'>&nbsp;</div>
             <div style='background-color:#5BC8AC; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class FreshFlowers : public ColorScheme
            {
          public:
            FreshFlowers()
                : ColorScheme({ ColorBrewer::GetColor(Color::Aquamarine), wxColour(L"#E6D72A"),
                                ColorBrewer::GetColor(Color::PinkTulip),
                                ColorBrewer::GetColor(Color::Turquoise) })
                {
                }
            };

        /** @brief Icecream color scheme.
             \htmlonly
             <div style='background-color:#C9A66B; width:50px;'>&nbsp;</div>
             <div style='background-color:#AF4425; width:50px;'>&nbsp;</div>
             <div style='background-color:#662E1C; width:50px;'>&nbsp;</div>
             <div style='background-color:#EBDCB2; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class IceCream : public ColorScheme
            {
          public:
            IceCream()
                : ColorScheme({ ColorBrewer::GetColor(Color::Caramel),
                                ColorBrewer::GetColor(Color::Cayenne),
                                ColorBrewer::GetColor(Color::Cinnamon),
                                ColorBrewer::GetColor(Color::Cream) })
                {
                }
            };

        /** @brief Downtown color scheme.
             \htmlonly
             <div style='background-color:#2A2922; width:50px;'>&nbsp;</div>
             <div style='background-color:#7D5642; width:50px;'>&nbsp;</div>
             <div style='background-color:#506D2F; width:50px;'>&nbsp;</div>
             <div style='background-color:#F3EBDD; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class UrbanOasis : public ColorScheme
            {
          public:
            UrbanOasis()
                : ColorScheme({ ColorBrewer::GetColor(Color::Bark),
                                ColorBrewer::GetColor(Color::Brownstone),
                                ColorBrewer::GetColor(Color::Leaves),
                                ColorBrewer::GetColor(Color::Marble) })
                {
                }
            };

        /** @brief %Colors from a typewriter.
             \htmlonly
             <div style='background-color:#080706; width:50px;'>&nbsp;</div>
             <div style='background-color:#D1B280; width:50px;'>&nbsp;</div>
             <div style='background-color:#EFEFEF; width:50px;'>&nbsp;</div>
             <div style='background-color:#594D46; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Typewriter : public ColorScheme
            {
          public:
            Typewriter()
                : ColorScheme({ wxColour(L"#080706"), ColorBrewer::GetColor(Color::GoldLeaf),
                                ColorBrewer::GetColor(Color::Paper),
                                ColorBrewer::GetColor(Color::Silver) })
                {
                }
            };

        /** @brief Surfing color scheme.
             \htmlonly
             <div style='background-color:#003B46; width:50px;'>&nbsp;</div>
             <div style='background-color:#C4DFE6; width:50px;'>&nbsp;</div>
             <div style='background-color:#07575B; width:50px;'>&nbsp;</div>
             <div style='background-color:#66A5AD; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class TastyWaves : public ColorScheme
            {
          public:
            TastyWaves()
                : ColorScheme({ wxColour(L"#003B46"), ColorBrewer::GetColor(Color::LightSeafoam),
                                ColorBrewer::GetColor(Color::Ocean),
                                ColorBrewer::GetColor(Color::Wave) })
                {
                }
            };

        /** @brief Springtime color scheme.
             \htmlonly
             <div style='background-color:#F98866; width:50px;'>&nbsp;</div>
             <div style='background-color:#FF420E; width:50px;'>&nbsp;</div>
             <div style='background-color:#89DA59; width:50px;'>&nbsp;</div>
             <div style='background-color:#80BD9E; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Spring : public ColorScheme
            {
          public:
            Spring()
                : ColorScheme({ ColorBrewer::GetColor(Color::Petal),
                                ColorBrewer::GetColor(Color::Poppy),
                                ColorBrewer::GetColor(Color::SpringGreen),
                                ColorBrewer::GetColor(Color::Stem) })
                {
                }
            };

        /** @brief Shabby chic color scheme.
             \htmlonly
             <div style='background-color:#CDAB81; width:50px;'>&nbsp;</div>
             <div style='background-color:#6C5F5B; width:50px;'>&nbsp;</div>
             <div style='background-color:#DAC3B3; width:50px;'>&nbsp;</div>
             <div style='background-color:#4F4A45; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class ShabbyChic : public ColorScheme
            {
          public:
            ShabbyChic()
                : ColorScheme({ wxColour(L"#CDAB81"), ColorBrewer::GetColor(Color::Metal),
                                ColorBrewer::GetColor(Color::Newsprint),
                                ColorBrewer::GetColor(Color::Pewter) })
                {
                }
            };

        /** @brief Thunderstorm color scheme.
             \htmlonly
             <div style='background-color:#598234; width:50px;'>&nbsp;</div>
             <div style='background-color:#AEBD38; width:50px;'>&nbsp;</div>
             <div style='background-color:#505160; width:50px;'>&nbsp;</div>
             <div style='background-color:#68829E; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class RollingThunder : public ColorScheme
            {
          public:
            RollingThunder()
                : ColorScheme({ ColorBrewer::GetColor(Color::Meadow),
                                ColorBrewer::GetColor(Color::Moss),
                                ColorBrewer::GetColor(Color::ThunderCloud),
                                ColorBrewer::GetColor(Color::Waterfall) })
                {
                }
            };

        /** @brief Vegetable colors.
             \htmlonly
             <div style='background-color:#31A9B8; width:50px;'>&nbsp;</div>
             <div style='background-color:#258039; width:50px;'>&nbsp;</div>
             <div style='background-color:#CF3721; width:50px;'>&nbsp;</div>
             <div style='background-color:#F5BE41; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class ProduceSection : public ColorScheme
            {
          public:
            ProduceSection()
                : ColorScheme({ wxColour(L"#31A9B8"), ColorBrewer::GetColor(Color::Avocado),
                                ColorBrewer::GetColor(Color::RedTomato),
                                ColorBrewer::GetColor(Color::YellowPepper) })
                {
                }
            };

        /** @brief %Nautical color scheme.
             \htmlonly
             <div style='background-color:#F62A00; width:50px;'>&nbsp;</div>
             <div style='background-color:#F1F3CE; width:50px;'>&nbsp;</div>
             <div style='background-color:#00293C; width:50px;'>&nbsp;</div>
             <div style='background-color:#1E656D; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Nautical : public ColorScheme
            {
          public:
            Nautical()
                : ColorScheme({ ColorBrewer::GetColor(Color::CandyApple),
                                ColorBrewer::GetColor(Color::Ivory),
                                ColorBrewer::GetColor(Color::Navy),
                                ColorBrewer::GetColor(Color::PeacockBlue) })
                {
                }
            };

        /** @brief Meadow sunset color scheme.
             \htmlonly
             <div style='background-color:#3F681C; width:50px;'>&nbsp;</div>
             <div style='background-color:#375E97; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFBB00; width:50px;'>&nbsp;</div>
             <div style='background-color:#FB6542; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class MeadowSunset : public ColorScheme
            {
          public:
            MeadowSunset()
                : ColorScheme({ ColorBrewer::GetColor(Color::Grass),
                                ColorBrewer::GetColor(Color::Sky),
                                ColorBrewer::GetColor(Color::Sunflower),
                                ColorBrewer::GetColor(Color::Sunset) })
                {
                }
            };

        /** @brief Semesters color scheme, representing fall, spring, and summer.
            @details Note that there is no such thing as a winter semester.
             \htmlonly
             <div style='background-color:#FF7518; width:50px;'>&nbsp;</div>
             <div style='background-color:#00FF7F; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFBB00; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Semesters : public ColorScheme
            {
          public:
            Semesters()
                : ColorScheme({ ColorBrewer::GetColor(Color::Pumpkin),
                                ColorBrewer::GetColor(Color::SpringGreen),
                                ColorBrewer::GetColor(Color::Sunflower) })
                {
                }
            };

        /** @brief Seasons color scheme, representing fall, winter, spring, and summer.
             \htmlonly
             <div style='background-color:#FF7518; width:50px;'>&nbsp;</div>
             <div style='background-color:#A1D6E2; width:50px;'>&nbsp;</div>
             <div style='background-color:#00FF7F; width:50px;'>&nbsp;</div>
             <div style='background-color:#FFBB00; width:50px;'>&nbsp;</div>
             \endhtmlonly*/
        class Seasons : public ColorScheme
            {
          public:
            Seasons()
                : ColorScheme({ ColorBrewer::GetColor(Color::Pumpkin),
                                ColorBrewer::GetColor(Color::Ice),
                                ColorBrewer::GetColor(Color::SpringGreen),
                                ColorBrewer::GetColor(Color::Sunflower) })
                {
                }
            };
        } // namespace Schemes
    } // namespace Wisteria::Colors

/** @}*/

#endif // WISTERIA_COLORBREWER_H
