/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef TEXT_FUNCTIONAL_H
#define TEXT_FUNCTIONAL_H

#include <algorithm>
#include <cwctype>
#include <string>

namespace lily_of_the_valley
    {
    /// @brief Replaces "" with a single ".
    /// @details In CSV files, embedded quotes are doubled like this to preserve them,
    ///     so this interface will convert those back to a single ".
    template<typename T>
    class cell_collapse_quotes
        {
      public:
        /// @brief Converts doubled up " with a single ".
        /// @param[in, out] text The string to convert.
        void operator()(T& text) const
            {
            size_t start{ 0 };
            while (start != T::npos)
                {
                start = text.find(L"\"\"", start);
                if (start == T::npos)
                    {
                    break;
                    }
                text.replace(start, 2, L"\"");
                }
            }
        };

    /// @brief Trims whitespace and quotes from around a string.
    /// @details This is needed for reading cells from a CSV file where some cells may be quoted.
    class cell_trim
        {
      public:
        /** @brief Finds the string inside a larger string, ignoring the
                spaces and quotes around it.
            @returns Position into the string buffer where the first non-space/quote is.
            @param value The string to trim.
            @param length The length of @c value.\n
                Call get_trimmed_string_length() to see how much to read from there to
                see where the last non-space is at the end.*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* value, size_t length = std::wstring::npos) noexcept
            {
            m_trimmed_string_length = 0;
            if (value == nullptr)
                {
                return nullptr;
                }
            if (length == std::wstring::npos)
                {
                length = std::wcslen(value);
                }
            if (length == 0)
                {
                return value;
                }
            // end is last valid character in the string, not the null terminator
            const wchar_t* end = value + (length - 1);
            // trim leading quote
            const wchar_t* valueStart = value;
            if (valueStart[0] == L'\"')
                {
                ++valueStart;
                --length;
                }
            const wchar_t* start =
                std::find_if_not(valueStart, valueStart + length,
                                 [](const auto ch) noexcept { return std::iswspace(ch); });
            // remove trailing quote (just the last one)
            if (end > start && end[0] == L'\"')
                {
                --end;
                }
            while (end > start)
                {
                if (std::iswspace(end[0]))
                    {
                    --end;
                    }
                else
                    {
                    break;
                    }
                }
            // if start overran end, then this string was all spaces
            m_trimmed_string_length = (start > end) ? 0 : (end - start) + 1;
            return start;
            }

        /// @returns The length of the string buffer,
        ///     ignoring spaces and quotes on the left and right.
        [[nodiscard]]
        size_t get_trimmed_string_length() const noexcept
            {
            return m_trimmed_string_length;
            }

      private:
        size_t m_trimmed_string_length{ 0 };
        };

    // EOL determinant functors used by the row parser
    //-------------------------------------------------

    /// @brief EOL determinant.
    class is_end_of_line
        {
      public:
        /// @brief Determines if character is either CR or LF.
        /// @param character The character to review.
        /// @returns @c true if character is either CR or LF.
        [[nodiscard]]
        inline constexpr bool operator()(const wchar_t character) const noexcept
            {
            return (character == 10 || character == 13);
            }
        };

    /// @brief Column delimiter determinant, where the first whitespace, comma,
    ///     or semicolon is the end of the current column.
    class is_standard_delimiters
        {
      public:
        /// @brief Determines if character is either whitespace, a semicolon, or comma.
        /// @param character The character to review.
        /// @returns @c true if character is either whitespace, a semicolon, or comma.
        [[nodiscard]]
        inline bool operator()(const wchar_t character) const noexcept
            {
            return (std::iswspace(character) || character == L';' || character == L',');
            }
        };

    /// @brief Column delimiter determinant (a specified single character).
    class is_single_delimiter
        {
      public:
        /// @brief Constructor.
        /// @param delim The delimiter to use.
        explicit is_single_delimiter(const wchar_t delim) noexcept : m_delim(delim) {}

        /// @private
        is_single_delimiter() = delete;

        /// @brief Determines if character is a delimiter.
        /// @param character The character to review.
        /// @returns @c true if character is a delimiter.
        [[nodiscard]]
        inline constexpr bool operator()(const wchar_t character) const noexcept
            {
            return (character == m_delim);
            }

      private:
        wchar_t m_delim{ L';' };
        };

    /// @brief Column delimiter determinant (one of multiple specified characters).
    class is_one_of_multiple_delimiters
        {
      public:
        /// @brief Constructor.
        /// @param delims The delimiters to use.
        explicit is_one_of_multiple_delimiters(const std::wstring_view delims) : m_delims(delims) {}

        /// @private
        is_one_of_multiple_delimiters() = delete;

        /// @brief Determines if character is a delimiter.
        /// @param character The character to review.
        /// @returns @c true if character is a delimiter.
        [[nodiscard]]
        inline bool operator()(const wchar_t character) const noexcept
            {
            return (m_delims.find(character) != std::wstring::npos);
            }

      private:
        std::wstring m_delims;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // TEXT_FUNCTIONAL_H
