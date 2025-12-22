/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_NUMBERFORMAT_H
#define WISTERIA_NUMBERFORMAT_H

namespace Wisteria
    {
    /// @brief Specification for formatting a value into a string.
    struct NumberFormatInfo
        {
        /// @brief Basic number formatting types.
        enum class NumberFormatType
            {
            /// @brief A custom format, defined by a NumberFormat-derived class.
            CustomFormatting,
            /// @brief Basic formatting, including a decimal and
            ///     (possible) thousands separator.
            StandardFormatting,
            /// @brief Formatting something like `0.5` to `50%`.
            PercentageFormatting
            };

        /// @brief Constructor.
        /// @param type How to format values.
        explicit NumberFormatInfo(const NumberFormatType type) noexcept : m_type(type) {}

        /// @brief Constructor.
        /// @param type How to format values.
        /// @param precision The precision to show.
        /// @param displayThousandsSeparator @c true to show the thousands separator.
        NumberFormatInfo(const NumberFormatType type, const uint8_t precision,
                         const bool displayThousandsSeparator = false) noexcept
            : m_type(type), m_precision(precision),
              m_displayThousandsSeparator(displayThousandsSeparator)
            {
            }

        /// @brief The specification for how to format the values.
        NumberFormatType m_type{ NumberFormatType::StandardFormatting };
        /// @brief The precision.
        uint8_t m_precision{ 0 };
        /// @brief Whether to include the thousands separator.
        bool m_displayThousandsSeparator{ false };
        };

    /// @brief Formats numbers (and strings) into a specialized string format.
    /// @details This can support for percentages and possibly custom formats
    ///     in derived classes. Support for parsing a string and returning it
    ///     in a different format is also available.
    template<typename stringT>
    class NumberFormat
        {
      public:
        /// @private
        virtual ~NumberFormat() = default;

        /// @brief Formats a string into a different format.
        /// @param value The string to reformat.
        /// @param format The format specification.
        /// @returns The value, formatted as a string.
        [[nodiscard]]
        virtual stringT GetFormattedValue(const stringT& value,
                                          const NumberFormatInfo& format) const = 0;
        /// @brief Formats a number into a string.
        /// @param value The number to format.
        /// @param format The format specification.
        /// @returns The value, formatted as a string.
        [[nodiscard]]
        virtual stringT GetFormattedValue(double value, const NumberFormatInfo& format) const = 0;
        };
    } // namespace Wisteria

/** @}*/

#endif // WISTERIA_NUMBERFORMAT_H
