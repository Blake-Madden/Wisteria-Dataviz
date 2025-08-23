/********************************************************************************
 * Copyright (c) 2021-2025 Blake Madden
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Blake Madden - initial implementation
 ********************************************************************************/

/** @addtogroup Internationalization
    @brief i18n classes.
@{*/

#ifndef I18N_EXTRACT_H
#define I18N_EXTRACT_H

#include <regex>
#include <string>
#include <string_view>

/// @brief Helper functions for reviewing i18n/l10n related strings.
namespace i18n_string_util
    {
    /** @brief Determines if a string is a local file path,
            file name, email address, or internet address.
        @returns @c true if text block is a local file or Internet address.
        @param text The text block to analyze.\n
            This will be the start of the text block up to the end of the
            suspected file address.*/
    [[nodiscard]]
    bool is_file_address(std::wstring_view text);

    /** @brief Determines if a string is an internet address.
        @returns @c true if text block is an Internet address.
        @param text The text block to analyze.\n
            This will be the start of the text block up to the end of the
            suspected file address.*/
    [[nodiscard]]
    bool is_url(std::wstring_view text);

    /** @returns Whether a character is a number (narrow [0-9] characters only).
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_numeric_7bit(const wchar_t ch) noexcept
        {
        return (ch >= L'0' && ch <= L'9') ? true : false;
        }

    /** @returns @c true if a character is a letter
            (English alphabet only, and no full-width characters).
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_alpha_7bit(const wchar_t ch) noexcept
        {
        return (((ch >= 0x41 /*'A'*/) && (ch <= 0x5A /*'Z'*/)) ||
                ((ch >= 0x61 /*'a'*/) && (ch <= 0x7A /*'z'*/)));
        }

    /** @returns @c true if a character is an apostrophe (includes straight single quotes).
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_apostrophe(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L'\'') ?       // '
            true : (ch == 146) ?    // apostrophe
            true : (ch == 180) ?    // apostrophe
            true : (ch == 0xFF07) ? // full-width apostrophe
            true : (ch == 0x2019) ? // right single apostrophe
            true : false;
        // clang-format on
        }

    /** @returns @c true if a character is a period.
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_period(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L'.') ?       // .
            true : (ch == 0x2024) ? // one dot leader
            true : (ch == 0x3002) ? // Japanese full stop
            true : (ch == 0xFF61) ? // halfwidth full stop
            true : (ch == 0xFF0E) ? // fullwidth full stop
            true : (ch == 0xFE12) ? // vertical full stop
            true : (ch == 0x06D4) ? // Arabic full stop
            true : (ch == 0x2026) ? // ellipsis
            true : false;
        // clang-format on
        }

    /** @returns @c true if a character is an exclamation mark.
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_exclamation(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L'!') ?       // !
            true : (ch == 0xFE15) ? // presentation
            true : (ch == 0xFE57) ? // small
            true : (ch == 0x00A1) ? // inverted
            true : (ch == 0xFF01) ? // fullwidth
            true : (ch == 0xFE57) ? // small
            true : (ch == 0xFE15) ? // presentation
            true : false;
        // clang-format on
        }

    /** @returns @c true if a character is a question mark.
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_question(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L'?') ?       // !
            true : (ch == 0x061F) ? // Arabic
            true : (ch == 0xFF1F) ? // fullwidth
            true : false;
        // clang-format on
        }

    /** @returns @c true if a character is a colon.
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_colon(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L':') ?       // !
            true : (ch == 0xFF1A) ? // fullwidth
            true : false;
        // clang-format on
        }

    /** @returns @c true if a character is a closing parenthesis.
        @param ch The letter to be reviewed.*/
    [[nodiscard]]
    constexpr static bool is_close_parenthesis(const wchar_t ch) noexcept
        {
        // clang-format off
        return (ch == L')') ?       // )
            true : (ch == 0xFF09) ? // fullwidth
            true : false;
        // clang-format on
        }

    /** @returns @c a full-width number converted to its 7-bit counterpart.
        @note Will return the original character if it cannot be converted.
        @param chr The letter to be converted.*/
    [[nodiscard]]
    constexpr static wchar_t full_width_number_to_7bit(const wchar_t chr) noexcept
        {
        return (chr >= 0xFF10 && chr <= 0xFF19) ? (chr - 65248) : chr;
        }

    /** @returns @c a 7-bit number converted to its full-width counterpart.
        @note Will return the original character if it cannot be converted.
        @param chr The letter to be converted.*/
    [[nodiscard]]
    constexpr static wchar_t seven_bit_number_to_full_width(const wchar_t chr) noexcept
        {
        return (chr >= 0x0030 && chr <= 0x0039) ? (chr + 65248) : chr;
        }

    /** @returns @c a Devanagari (char set for languages such as Hindi) number
            converted to its 7-bit counterpart.
        @note Will return the original character if it cannot be converted.
        @param chr The letter to be converted.*/
    [[nodiscard]]
    constexpr static wchar_t devanagari_number_to_7bit(const wchar_t chr) noexcept
        {
        return (chr >= 0x0966 && chr <= 0x096F) ? (chr - 2358) : chr;
        }

    /** @returns @c a 7-bit number converted to its Devanagari counterpart.
        @note Will return the original character if it cannot be converted.
        @param chr The letter to be converted.*/
    [[nodiscard]]
    constexpr static wchar_t seven_bit_number_to_devanagari(const wchar_t chr) noexcept
        {
        return (chr >= 0x0030 && chr <= 0x0039) ? (chr + 2358) : chr;
        }

    /// @brief Removes printf commands in @c str (in-place).
    /// @param str The string to have printf commands removed from.
    inline void remove_printf_commands(std::wstring& str)
        {
        // Y H M are also included, as they are for similar datetime formatting functions
        static const std::wregex printfRegex(
            L"([^%\\\\]|^|\\b)%[-+0 #]{0,4}[.[:digit:]]*"
            "(?:c|C|d|i|o|u|lu|ld|lx|lX|lo|llu|lld|x|X|e|E|f|g|G|a|A|n|p|s|S|Z|zu|Y|H|M)");
        // The % command (not following another % or \),
        // flags ("-+0 #", optionally can have up to 4 of these),
        // width and precision (".0-9", optional), and the specifier.
        try
            {
            str = std::regex_replace(str, printfRegex, L"$1");
            return;
            }
        catch (...)
            {
            return;
            }
        }

    /// @brief Removes hex color values (e.g., "#FF00AA") in @c str (in-place).
    /// @param str The string to have color values removed from.
    inline void remove_hex_color_values(std::wstring& str)
        {
        const std::wregex colorRegex(L"#[[:xdigit:]]{6}");
        try
            {
            str = std::regex_replace(str, colorRegex, L"");
            return;
            }
        catch (...)
            {
            return;
            }
        }

    /// @brief Removes escaped Unicode values in @c str.
    ///     (e.g., "\u266f" will be replaced with spaces).
    /// @param[out] str The string being escaped.
    void remove_escaped_unicode_values(std::wstring& str);

    /** @brief Converts a string to wstring (assuming that the string is simple 8-bit ASCII).
        @param str The string to convert.
        @returns The string, converted to a wstring.
        @warning This assumes 8-bit ASCII strings and does not perform any sort
            of charset conversion. This should only be used for very simple strings,
            such as `what()` from an untranslated `std::exception`.*/
    [[nodiscard]]
    inline std::wstring lazy_string_to_wstring(const std::string& str)
        {
        std::wstring retVal;
        retVal.reserve(str.length());
        for (const auto& ch : str)
            {
            retVal += static_cast<wchar_t>(ch);
            }
        return retVal;
        }

    /// @brief Converts escaped control characters (e.g., "\n")
    ///     inside a string into spaces.
    /// @param[out] str The string being escaped.
    template<typename string_typeT>
    void replace_escaped_control_chars(string_typeT& str)
        {
        for (size_t i = 0; i < str.length(); ++i)
            {
            if (str[i] == L'\\' &&
                (str[i + 1] == L'n' || str[i + 1] == L'r' || str[i + 1] == L't') &&
                (i == 0 || str[i - 1] != L'\\'))
                {
                str[i] = str[i + 1] = L' ';
                }
            }
        }
    } // namespace i18n_string_util

/** @}*/

#endif // I18N_EXTRACT_H
