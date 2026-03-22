///////////////////////////////////////////////////////////////////////////////
// Name:        colorbrewer.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "colorbrewer.h"

using namespace Wisteria::Colors::Schemes;

// clang-format off
wxIMPLEMENT_DYNAMIC_CLASS(ColorScheme, wxObject)
wxIMPLEMENT_DYNAMIC_CLASS(Dusk, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(EarthTones, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1920s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1940s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1950s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1960s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1970s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1980s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade1990s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Decade2000s, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(October, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Slytherin, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Campfire, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(CoffeeShop, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(ArcticChill, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(BackToSchool, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(BoxOfChocolates, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Cosmopolitan, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(DayAndNight, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(FreshFlowers, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(IceCream, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(UrbanOasis, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Typewriter, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(TastyWaves, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Spring, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(ShabbyChic, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(RollingThunder, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(ProduceSection, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Nautical, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(MeadowSunset, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Semesters, ColorScheme)
wxIMPLEMENT_DYNAMIC_CLASS(Seasons, ColorScheme)
    // clang-format on

    namespace Wisteria::Colors
    {
    //-------------------------------------------
    wxColour ColorBrewer::GetColor(const Color color)
        {
        if (static_cast<uint16_t>(color) >= static_cast<uint16_t>(Color::COLOR_COUNT))
            {
            return wxNullColour;
            }
        wxColour theColor;
        theColor.SetRGBA(m_colors[static_cast<uint16_t>(color)]);
        return theColor;
        }

    //-------------------------------------------
    wxColour ColorBrewer::BrewColor(const double value) const
        {
        wxASSERT_MSG((std::isnan(value) || (value >= m_range.first && value <= m_range.second)),
                     L"Value passed to BrewColor() should be within established data range "
                     "from previous call to BrewColors()!");
        // return invalid color or NaN
        if (std::isnan(value))
            {
            return wxNullColour;
            }
        // verify that we have a valid spectrum initialized
        NON_UNIT_TEST_ASSERT(m_colorSpectrum.size() > 1);
        if (m_colorSpectrum.size() < 2)
            {
            throw std::length_error(
                _(L"Color scale has not been initialized in color brewer.").ToUTF8());
            }
        double normalizedValue =
            statistics::normalize<double>(m_range.first, m_range.second, value);

        // Our desired color will be between these two indexes in "colorSpectrum".
        int idx1{ 0 }, idx2{ 0 };
        // Fraction between "idx1" and "idx2" where our value is.
        double fractBetween{ 0 };

        if (compare_doubles_less_or_equal(normalizedValue, 0))
            {
            idx1 = idx2 = 0;
            }
        else if (compare_doubles_greater_or_equal(normalizedValue, 1))
            {
            idx1 = idx2 = m_colorSpectrum.size() - 1;
            }
        else
            {
            normalizedValue = normalizedValue * (m_colorSpectrum.size() - 1);
            // Our desired color will be after this index.
            idx1 = std::floor(normalizedValue);
            // ... and before this index (inclusive).
            idx2 = idx1 + 1;
            // Distance between the two indexes (0-1).
            fractBetween = normalizedValue - static_cast<double>(idx1);
            }

        const wxColour brewedColor(
            ((m_colorSpectrum[idx2].Red() - m_colorSpectrum[idx1].Red()) * fractBetween) +
                m_colorSpectrum[idx1].Red(),
            ((m_colorSpectrum[idx2].Green() - m_colorSpectrum[idx1].Green()) * fractBetween) +
                m_colorSpectrum[idx1].Green(),
            ((m_colorSpectrum[idx2].Blue() - m_colorSpectrum[idx1].Blue()) * fractBetween) +
                m_colorSpectrum[idx1].Blue());

        return brewedColor;
        }

    //-------------------------------------------
    wxColour ColorContrast::Contrast(const wxColour& color) const
        {
        wxASSERT_MSG(color.IsOk(), L"Invalid color passed to Contrast().");
        const auto bgLuminance = m_baseColor.GetLuminance();
        const auto colorLuminance = color.GetLuminance();
        const auto luminanceDifference = std::abs(bgLuminance - colorLuminance);
        const auto adjustmentNeeded = M_TOLERANCE - luminanceDifference;
        if (!compare_doubles(adjustmentNeeded, 0))
            {
            // if background is lighter or the same...
            if (bgLuminance >= colorLuminance)
                {
                // ...and color can be made darker by the full amount of the adjustment,
                // then make it darker
                if ((colorLuminance - adjustmentNeeded) >= 0)
                    {
                    return color.ChangeLightness(100 - (adjustmentNeeded * 100));
                    }
                // ...otherwise, make it lighter. Going the other way will require lightening it
                // up to the other color, then lightening by the full tolerance amount.
                // An example of this would be a dark gray background and even darker gray text
                // that's too close to black to make it useful as black. Instead, you lighten
                // the text color to the same level as the background and then lighten it by the
                // full tolerance value.
                return color.ChangeLightness(100 + ((adjustmentNeeded + M_TOLERANCE) * 100));
                }
            // or if background is darker,
            // make the text lighter
            if ((colorLuminance + adjustmentNeeded) <= 1)
                {
                return color.ChangeLightness(100 + (adjustmentNeeded * 100));
                }
            // or darker
            return color.ChangeLightness(100 - ((adjustmentNeeded + M_TOLERANCE) * 100));
            }

        return color;
        }
    } // namespace Wisteria::Colors
