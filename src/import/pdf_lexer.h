/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PDF_LEXER_H
#define PDF_LEXER_H

#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief Low-level PDF tokenization and value-reading utilities.
    class pdf_lexer
        {
      public:
        /// @param character The character to check.
        /// @returns @c true if @p character is PDF whitespace.
        [[nodiscard]]
        static bool is_whitespace(char character);

        /// @param character The character to check.
        /// @returns @c true if @p character is a PDF delimiter.
        [[nodiscard]]
        static bool is_delimiter(char character);

        /// @param character The character to check.
        /// @returns @c true if @p character ends a token (whitespace or delimiter).
        [[nodiscard]]
        static bool is_token_end(char character);

        /// @param character The character to check.
        /// @returns @c true if @p character is an ASCII digit.
        [[nodiscard]]
        static bool is_digit(char character);

        /// @param character The character to check.
        /// @returns The numeric value of a hex digit (or @c -1 if not a hex digit).
        [[nodiscard]]
        static int hex_digit_value(char character);

        /// @brief Moves @p pos past any whitespace and comments.
        /// @param source The PDF source content.
        /// @param[in,out] pos The position to move past whitespace and comments.
        static void skip_whitespace(std::string_view source, size_t& pos);

        /// @param value The value to trim.
        /// @returns @p value with leading and trailing whitespace removed.
        [[nodiscard]]
        static std::string_view trim(std::string_view value);

        /// @brief Reads a (base 10) integer at @p pos, moving @p pos past it.
        /// @param source The PDF source content.
        /// @param[in,out] pos The position to read the integer from.
        /// @param[out] valueOut The integer that was read.
        /// @returns @c true if an integer was read.
        static bool read_int(std::string_view source, size_t& pos, long& valueOut);

        /// @brief Converts a trimmed token to an integer.
        /// @param token The token to convert.
        /// @param[out] valueOut The integer that was read.
        /// @returns @c true if the (full) token was an integer.
        [[nodiscard]]
        static bool to_int(std::string_view token, long& valueOut);

        /// @brief Reads a literal `(...)` string at @p pos
        ///     (which must be at the opening parenthesis).
        /// @param source The PDF source content.
        /// @param[in,out] pos The position of the opening parenthesis.
        /// @returns The string's (raw, still possibly font-encoded) bytes.
        [[nodiscard]]
        static std::string read_literal_string(std::string_view source, size_t& pos);

        /// @brief Reads a hexadecimal `<...>` string at @p pos
        ///     (which must be at the opening angle bracket).
        /// @param source The PDF source content.
        /// @param[in,out] pos The position of the opening angle bracket.
        /// @returns The string's (raw, still possibly font-encoded) bytes.
        [[nodiscard]]
        static std::string read_hex_string(std::string_view source, size_t& pos);

        /// @brief Reads one complete PDF value (dictionary, array, string, name, number,
        ///     indirect reference, or keyword) at @p pos.
        /// @details The @p depth parameter guards against infinite recursion from
        ///     malformed documents (e.g., a dictionary that contains itself).
        /// @param source The PDF source content.
        /// @param[in,out] pos The position of the value to read.
        /// @param depth The current recursion depth.
        /// @returns A view of the raw value.
        [[nodiscard]]
        static std::string_view read_value(std::string_view source, size_t& pos, int depth = 0);

        /// @brief Searches a dictionary (the view must include the enclosing `<< >>`)
        ///     for a key (specified without the leading slash).
        /// @param dictionary The dictionary to search.
        /// @param keyName The key to search for, without the leading slash.
        /// @returns The key's raw value, or empty if not found.
        [[nodiscard]]
        static std::string_view find_dictionary_value(std::string_view dictionary,
                                                      std::string_view keyName);

        /// @param dictionary The dictionary to read.
        /// @returns All key/value pairs from a dictionary
        ///     (the view must include the enclosing `<< >>`).
        [[nodiscard]]
        static std::vector<std::pair<std::string_view, std::string_view>>
        get_dictionary_entries(std::string_view dictionary);

        /// @param arrayValue The array to read.
        /// @returns Each element's raw value from an array (the view must include the
        ///     enclosing `[ ]`), in order. Empty if @p arrayValue isn't an array.
        [[nodiscard]]
        static std::vector<std::string_view> read_array_elements(std::string_view arrayValue);

        /// @brief Determines whether a (trimmed) value is an indirect reference (`N G R`).
        /// @param value The (trimmed) value to check.
        /// @param[out] objectNumberOut The referenced object's number, if it is one.
        /// @returns @c true (filling in @p objectNumberOut) if it is.
        [[nodiscard]]
        static bool get_reference(std::string_view value, long& objectNumberOut);
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_LEXER_H
