/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __WISTERIA_SETTINGS_H__
#define __WISTERIA_SETTINGS_H__

#include "colorbrewer.h"
#include <wx/numformatter.h>

namespace Wisteria
    {
    /// @brief Debug settings used throughout the library.
    ///  This is a bitmask which can be used to control multiple flags.
    /// @internal Developer Note: this is used as a bitmask, don't strongly type it.
    /// @note These are %Wisteria specific debugging features
    ///  (e.g., bounding boxes being rendered). If running in debug mode,
    ///  other debugging features (e.g., asserts) will still be in effect.
    enum DebugSettings
        {
        None = 0x00,                         /*!< No %Wisteria-specific debugging should be enabled.*/
        DrawBoundingBoxesOnSelection = 0x01, /*!< Draw a bounding box around objects when they are selected.*/
        DrawInformationOnSelection = 0x02,   /*!< Write additional information on the screen when an object is selected
                                                  (e.g., the scaling value).*/
        DrawExtraInformation = 0x04,         /*!< Draw more verbose information, even when objects aren't selected.
                                                  This is only recommended when first designing a graph.*/
        IncludeExperimentalCode = 0x08       /*!< Run experimental code.
                                                  Code being used to test a new graph type during the design stage should
                                                  be wrapped in IncludeExperimentalCode blocks.
                                                  This is a preferred replacement for "#ifdef 0" code blocks as finalized code
                                                  can be scanned for IncludeExperimentalCode and removed.*/
        };

    /// @brief Class for managing global library settings.
    class Settings
        {
    public:
        /// @returns The default point radius.
        [[nodiscard]] static size_t GetPointRadius() noexcept
            { return m_pointRadius; }
        /// @brief Sets the default point radius.
        /// @param radius The default point radius.
        void SetPointRadius(const size_t radius) noexcept
            { m_pointRadius = radius; }
        /// @returns The opacity value to use when making a color translucent.
        [[nodiscard]] static uint8_t GetTranslucencyValue() noexcept
            { return m_translucencyValue; }
        /// @brief Sets the opacity value to use when making a color translucent.
        ///  Default is 100;
        /// @param value The opacity level (should be between 0 [transparent] to 255 [opaque]).
        static void SetTranslucencyValue(const uint8_t value) noexcept
            { m_translucencyValue = std::clamp<uint8_t>(value, 0, 255); }
        /// @brief Gets the maximum number of items that can be displayed in a legend.
        /// @returns The maximum number of items that can be displayed in a legend.
        [[nodiscard]] static uint8_t GetMaxLegendItemCount() noexcept
            { return m_maxLegendItems; }
        /// @brief Sets the maximum number of items that can be displayed in a legend.
        /// @details If there are more items in the legend, then an ellipsis will be shown.
        ///  The default number of items is 20.
        /// @param maxItems The maximum number of items that can be displayed in a legend.
        static void SetMaxLegendItemCount(const uint8_t maxItems) noexcept
            { m_maxLegendItems = maxItems; }
        /// @brief Gets the maximum text length for legend labels.
        /// @returns The maximum text length.
        [[nodiscard]] static size_t GetMaxLegendTextLength() noexcept
            { return m_maxLegendTextLength; }

        /// @brief Gets the maximum number of observations to show as a label in a bin.
        /// @returns The maximum number of observations to show in a bin label.
        [[nodiscard]] static size_t GetMaxObservationInBin() noexcept
            { return m_maxObservationsInBin; }
 
        /// @brief Sets the radius of the rounded corner, which is used when using rounded
        ///  corners for labels, box plots, etc.
        /// @param roundedCornerRadius The rounded corner radius.
        static void SetBoxRoundedCornerRadius(const double roundedCornerRadius) noexcept
            { m_roundedCornerRadius = roundedCornerRadius; }
        /// @returns The radius of the rounded corner, which is used when using rounded
        ///  corners for labels, box plots, etc.
        [[nodiscard]] static double GetBoxRoundedCornerRadius() noexcept
            { return m_roundedCornerRadius; }
        /// @brief Sets the maximum text length for legend labels.
        /// @details The default length is 32.
        /// @details If a label is longer than this,
        ///  then it will be truncated with an ellipsis at the end.
        /// @param length The maximum text length.
        static void SetMaxLegendTextLength(const size_t length) noexcept
            {
            wxASSERT_MSG(length > 0,
                                 L"Max legend label lengths should be at least 1!");
            m_maxLegendTextLength = std::max<size_t>(1, length); // at least length of one
            }
        /// @brief Enables or disables a debug flag.
        /// @param flag Which debug flag to enable.
        /// @param enable Whether to enable or disable the flag.
        /// @note DebugSettings::DrawBoundingBoxesOnSelection is enabled by default
        ///  if @c wxDEBUG_LEVEL is set to 2; otherwise, all flags are disabled.
        static void EnableDebugFlag(const int flag, const bool enable) noexcept
            {
            if (enable)
                { m_debugSettings |= flag; }
            else
                { m_debugSettings &= ~flag; }
            }
        /// @brief Turns off all debugging flags (specific to %Wisteria).
        static void DisableAllDebugFlags() noexcept
            { m_debugSettings = 0; }
        /// @brief Turns on all debugging flags (specific to %Wisteria).
        static void EnableAllDebugFlags() noexcept
            {
            m_debugSettings = DebugSettings::DrawBoundingBoxesOnSelection|
                              DebugSettings::DrawInformationOnSelection|
                              DebugSettings::DrawExtraInformation|
                              DebugSettings::IncludeExperimentalCode;
            }
        /// @brief Determines if a debug flag is enabled.
        /// @param flag The flag to check for.
        /// @returns `true` if the given flag is enabled.
        [[nodiscard]] static bool IsDebugFlagEnabled(const int flag) noexcept
            { return (m_debugSettings & flag) == flag; }
        /// @returns No trailing zeroes and thousands separator format
        ///  for calls to @c wxNumberFormatter::ToString().
        [[nodiscard]] static auto GetDefaultNumberFormat() noexcept
            {
            return wxNumberFormatter::Style::Style_WithThousandsSep|
                   wxNumberFormatter::Style::Style_NoTrailingZeroes;
            }
        /// @returns The default color scheme to use for groups with the graphs.
        [[nodiscard]] static std::shared_ptr<Colors::Schemes::ColorScheme> GetDefaultColorScheme()
            { return std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::Dusk()); }
    private:
        inline static uint8_t m_translucencyValue{ 100 };
        inline static uint8_t m_maxLegendItems{ 20 };
        inline static size_t m_maxLegendTextLength{ 32 };
        inline static size_t m_pointRadius{ 4 };
        inline static double m_roundedCornerRadius{ 5 };
        inline static size_t m_maxObservationsInBin{ 25 };
        inline static int m_debugSettings
#if wxDEBUG_LEVEL >= 2
        { DebugSettings::DrawBoundingBoxesOnSelection };
#else
        { DebugSettings::None };
#endif
        std::shared_ptr<Colors::Schemes::ColorScheme> m_defaultColorScheme;
        };
    }

/** @}*/

#endif // __WISTERIA_SETTINGS_H__
