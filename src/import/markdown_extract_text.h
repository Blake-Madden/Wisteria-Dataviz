/** @addtogroup Importing
    @brief Classes for importing and parsing text.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef MARKDOWN_TEXT_EXTRACT_H
#define MARKDOWN_TEXT_EXTRACT_H

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /// @brief Extracts plain text from a Markdown file.
    class markdown_extract_text final : public html_extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from a Markdown file.
            @param md_text The Markdown text to parse.
            @returns The parsed text from the Markdown stream.*/
        const wchar_t* operator()(std::wstring_view md_text);
#ifndef __UNITTEST
      private:
#endif
        /** @returns @c true if text marks the start of a Markdown metadata section.
            @param md_text The start of the Markdown text.
            @note YAML sections are supported.
            @warning @c md_text must be the start of the Markdown document.*/
        [[nodiscard]]
        static bool has_metadata_section(const std::wstring_view md_text) noexcept
            {
            return md_text.starts_with(L"---");
            }

        /// @returns number of characters consumed, or npos on failure
        size_t parse_styled_text(std::wstring_view input, wchar_t& previousChar, wchar_t tag);

        /// @returns number of characters consumed on success, or std::wstring_view::npos on failure
        [[nodiscard]]
        size_t parse_html_block(std::wstring_view input, std::wstring_view tag,
                                std::wstring_view endTag);

        bool parse_code_block(const bool isEscaping, const wchar_t* currentEndSentinel,
                              const wchar_t*& currentStart, wchar_t& previousChar,
                              bool& headerMode);

        /** @brief Metadata sections end on the first blank like, so moves to that.
            @param md_text The Markdown text, starting anywhere in the metadata section.
            @returns The start of the document's body.
            @sa has_metadata_section().*/
        [[nodiscard]]
        static size_t find_metadata_section_end(std::wstring_view mdText);

        std::unique_ptr<markdown_extract_text> m_subParser{ nullptr };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // MARKDOWN_TEXT_EXTRACT_H
