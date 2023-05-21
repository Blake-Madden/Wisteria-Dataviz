/** @addtogroup Graphics
    @brief Graphing classes.
    @date 2005-2023
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
    /** @brief Debug settings used throughout the library.
        @details This is a bitmask which can be used to control multiple flags.\n
            The following preprocessors can be defined to control which settings are enabled.
    
        - @c DEBUG_LOG_INFO: enables @ LogExtraInfo.
        - @c DEBUG_BOXES: enables @ DrawBoundingBoxesOnSelection.
        - @c DEBUG_DRAW_INFO: enables @ DrawInformationOnSelection.
        - @c DEBUG_DRAW_EXTRA_INFO enables @ DrawExtraInformation.
        - @c DEBUG_DRAW_EXP_CODE enables @ IncludeExperimentalCode.
        - @c DEBUG_FILE_IO enables @ AllowFileIO.

            By default, if @c wxDEBUG_LEVEL is @c 2, then @c DEBUG_BOXES, @c DEBUG_FILE_IO,
            and @c DEBUG_LOG_INFO are enabled. Otherwise, all debugging features are disabled.

            Note that these are %Wisteria specific debugging features (e.g., bounding boxes
            being rendered). If running in debug mode, other debugging features (e.g., asserts)
            will still be in effect.
    
        @internal Developer Note: this is used as a bitmask, don't strongly type it.*/
    enum DebugSettings
        {
        /** @brief No %Wisteria-specific debugging should be enabled.*/
        DebugNone = 0,
        /** @brief Draw a bounding box around objects when they are selected.*/
        DrawBoundingBoxesOnSelection = (1 << 0),
        /** @brief Write additional information on the screen when an object is selected
                (e.g., the scaling value).*/
        DrawInformationOnSelection = (1 << 1),
        /** @brief Draw more verbose information, even when objects aren't selected.\n
                This is only recommended when designing a new graph type.*/
        DrawExtraInformation = (1 << 2),
        /** @brief Run experimental code.\n
                Code being used to test a new graph type during the design stage should
                be wrapped in @c IncludeExperimentalCode blocks.\n
                This is a preferred replacement for `#ifdef 0` code blocks as this
                can easily be enabled/disabled globally
                (based on how @c DEBUG_DRAW_EXP_CODE is defined).*/
        IncludeExperimentalCode = (1 << 3),
        /** @brief Allows various file output options that should not be available in
                production releases. For example, allowing configuration files to
                export dataset silently for debugging purposes.*/
        AllowFileIO = (1 << 4),
        /** @brief Logs various information for additional messages.*/
        LogExtraInfo = (1 << 5)
        };

#if wxDEBUG_LEVEL >= 2
    #define DEBUG_LOG_INFO
    #define DEBUG_BOXES
    #define DEBUG_FILE_IO
#endif

    /// @brief Class for managing global library settings.
    class Settings
        {
    public:
        /// @returns The default point radius.
        [[nodiscard]]
        static size_t GetPointRadius() noexcept
            { return m_pointRadius; }
        /// @brief Sets the default point radius.
        /// @param radius The default point radius.
        void SetPointRadius(const size_t radius) noexcept
            { m_pointRadius = radius; }
        /// @returns The opacity value to use when making a color translucent.
        [[nodiscard]]
        static uint8_t GetTranslucencyValue() noexcept
            { return m_translucencyValue; }
        /// @brief Sets the opacity value to use when making a color translucent.
        ///     Default is 100;
        /// @param value The opacity level (should be between 0 [transparent] to 255 [opaque]).
        static void SetTranslucencyValue(const uint8_t value) noexcept
            { m_translucencyValue = std::clamp<uint8_t>(value, 0, 255); }
        /// @brief Gets the maximum number of items that can be displayed in a legend.
        /// @returns The maximum number of items that can be displayed in a legend.
        [[nodiscard]]
        static uint8_t GetMaxLegendItemCount() noexcept
            { return m_maxLegendItems; }
        /// @brief Sets the maximum number of items that can be displayed in a legend.
        /// @details If there are more items in the legend, then an ellipsis will be shown.
        ///     The default number of items is 20.
        /// @param maxItems The maximum number of items that can be displayed in a legend.
        static void SetMaxLegendItemCount(const uint8_t maxItems) noexcept
            { m_maxLegendItems = maxItems; }
        /// @brief Gets the maximum text length for legend labels.
        /// @returns The maximum text length.
        [[nodiscard]]
        static size_t GetMaxLegendTextLength() noexcept
            { return m_maxLegendTextLength; }

        /// @brief Gets the maximum number of observations to show as a label in a bin.
        /// @returns The maximum number of observations to show in a bin label.
        [[nodiscard]]
        static size_t GetMaxObservationInBin() noexcept
            { return m_maxObservationsInBin; }

        /// @brief Sets the radius of the rounded corner, which is used when using rounded
        ///     corners for labels, box plots, etc.
        /// @param roundedCornerRadius The rounded corner radius.
        static void SetBoxRoundedCornerRadius(const double roundedCornerRadius) noexcept
            { m_roundedCornerRadius = roundedCornerRadius; }
        /// @returns The radius of the rounded corner, which is used when using rounded
        ///     corners for labels, box plots, etc.
        [[nodiscard]]
        static double GetBoxRoundedCornerRadius() noexcept
            { return m_roundedCornerRadius; }
        /// @brief Sets the maximum text length for legend labels.
        /// @details The default length is 40.
        /// @details If a label is longer than this,
        ///     then it will be truncated with an ellipsis at the end.
        /// @param length The maximum text length.
        static void SetMaxLegendTextLength(const size_t length) noexcept
            {
            wxASSERT_MSG(length > 0,
                         L"Max legend label lengths should be at least 1!");
            m_maxLegendTextLength = std::max<size_t>(1, length); // at least length of one
            }
        /// @brief Determines if a debug flag is enabled.
        /// @param flag The flag to check for.
        /// @returns @c true if the given flag is enabled.
        /// @note Calls to this can be `if constexpr`ed so that the @c if block's code
        ///     will be compiled out when the flag is not enabled.
        [[nodiscard]]
        static constexpr bool IsDebugFlagEnabled(const int flag) noexcept
            { return (m_debugSettings & flag) == flag; }
        /// @returns No trailing zeroes and thousands separator format
        ///     for calls to @c wxNumberFormatter::ToString().
        [[nodiscard]]
        static auto GetDefaultNumberFormat() noexcept
            {
            return wxNumberFormatter::Style::Style_WithThousandsSep|
                   wxNumberFormatter::Style::Style_NoTrailingZeroes;
            }
        /// @returns The default color scheme to use for groups with the graphs.
        [[nodiscard]]
        static std::shared_ptr<Colors::Schemes::ColorScheme> GetDefaultColorScheme()
            { return std::make_shared<Colors::Schemes::ColorScheme>(Colors::Schemes::Dusk()); }
    private:
        inline static uint8_t m_translucencyValue{ 100 };
        inline static uint8_t m_maxLegendItems{ 20 };
        inline static size_t m_maxLegendTextLength{ 40 };
        inline static size_t m_pointRadius{ 4 };
        inline static double m_roundedCornerRadius{ 5 };
        inline static size_t m_maxObservationsInBin{ 25 };
        static constexpr int m_debugSettings
        {
#ifdef DEBUG_LOG_INFO
        DebugSettings::LogExtraInfo|
#endif
#ifdef DEBUG_BOXES
        DebugSettings::DrawBoundingBoxesOnSelection|
#endif
#ifdef DEBUG_DRAW_INFO
        DebugSettings::DrawInformationOnSelection|
#endif
#ifdef DEBUG_DRAW_EXTRA_INFO
        DebugSettings::DrawExtraInformation|
#endif
#ifdef DEBUG_DRAW_EXP_CODE
        DebugSettings::IncludeExperimentalCode|
#endif
#ifdef DEBUG_FILE_IO
        DebugSettings::AllowFileIO|
#endif
        DebugSettings::DebugNone
        };
        std::shared_ptr<Colors::Schemes::ColorScheme> m_defaultColorScheme;
        };
    }

/** @}*/

#endif // __WISTERIA_SETTINGS_H__
