/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __CPP_TEXT_EXTRACT_H__
#define __CPP_TEXT_EXTRACT_H__

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text (Doxygen/Javadoc comments and GETTEXT resources) from a C++
     *      source code stream.*/
    class cpp_extract_text final : public extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from C++ source code.
            @details This will include Doxygen/Javadoc comments and GETTEXT strings.
            @param cpp_text The C++ code text to extract text from.
            @param text_length The length of the text.
            @returns A pointer to the parsed text, or null upon failure.
            @note To include all comments (not just Doxygen content),
                call `include_all_comments(true)`.*/
        const wchar_t* operator()(const wchar_t* cpp_text, const size_t text_length);

        /// @brief Sets whether all comments should be included (not just Doxygen-style comments).
        /// @param includeAll Set to @c true to include all comments.
        void include_all_comments(const bool includeAll) noexcept
            {
            m_include_all_comments = includeAll;
            }

        /// @returns @c true if all comments are being included (not just Doxygen-style comments).
        /// @note The default is to only include Doxygen comments.
        [[nodiscard]]
        bool is_including_all_comments() const noexcept
            {
            return m_include_all_comments;
            }

        /** @returns The author from the document summary.
            @note Must be called after calling operator().*/
        [[nodiscard]]
        const std::wstring& get_author() const noexcept
            {
            return m_author;
            }

      protected:
        /// @returns @c true if a character is an English letter, number, or underscore.
        /// @param ch The character to review.
        [[nodiscard]]
        static constexpr bool is_valid_char(const wchar_t ch) noexcept
            {
            return (ch >= L'a' && ch <= L'z') || (ch >= L'A' && ch <= L'Z') ||
                   (ch >= L'0' && ch <= L'9') || (ch == L'_');
            }

        /** @brief Replaces `\n \t \r` characters with spaces while
                calling add_characters().
            @param characters The string to add.
            @param length The length of the string to add.*/
        void add_characters_strip_escapes(const wchar_t* characters, const size_t length);
        /// @brief Adds the text to the buffer, but strips markup (e.g., Doxygen tags) from the
        ///     stream first.
        /// @param cpp_text The text to read from.
        /// @param text_length The length of @c cpp_text.
        /// @todo add rest of the tags that should be skipped.
        void add_characters_strip_markup(const wchar_t* cpp_text, const size_t text_length);

      private:
        html_extract_text html_extract;
        bool m_include_all_comments{ false };

        // meta data
        std::wstring m_author;
        // doxygen tags that are removed (but their following text is read in).
        // Note that 'param' is not included here so that 'in|out' info is handled separately.
        std::set<std::wstring> m_doxygen_tags = {
            L"{",       L"}",       L"a",           L"b",          L"c",       L"cond",
            L"e",       L"p",       L"em",          L"brief",      L"short",   L"code",
            L"endcode", L"endcond", L"note",        L"return",     L"returns", L"result",
            L"remark",  L"remarks", L"retval",      L"warning",    L"sa",      L"see",
            L"related", L"relates", L"relatedalso", L"relatesalso"
        };
        // doxygen tags that are removed (but their following text is read in)
        // and newlines are wrapped around their text because their text
        // should be single-line content. These would be things like names
        // of classes, groups, functions, etc.
        std::set<std::wstring> m_doxygen_tags_single_line = {
            L"class", L"struct",    L"union",   L"var",       L"enum", L"def",  L"typedef",
            L"file",  L"namespace", L"package", L"interface", L"fn",   L"name", L"addtogroup"
        };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__CPP_TEXT_EXTRACT_H__
