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
#include <regex>

namespace lily_of_the_valley
    {
    /// @brief Extracts plain text from a Markdown file.
    class markdown_extract_text final : public html_extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from a Markdown file.
            @param md_text The markdown text to parse.
            @returns The parsed text from the Markdown stream.*/
        const wchar_t* operator()(const std::wstring_view md_text);
#ifndef __UNITTEST
      private:
#endif
        /** @returns @c true if text marks the start of a Markdown metadata section.
            @param md_text The start of the Markdown text.
            @note YAML sections are supported.
            @warning @c md_text must be the start of the Markdown document.*/
        [[nodiscard]]
        static bool has_metadata_section(const wchar_t* md_text)
            {
            if (md_text == nullptr)
                {
                return false;
                }
            // does the first line start with ---
            return (std::wcsncmp(md_text, L"---", 3) == 0);
            }

        bool parse_styled_text(wchar_t& previousChar, const wchar_t tag);

        [[nodiscard]]
        bool parse_html_block(const std::wstring_view tag, const std::wstring_view endTag);

        /** @brief Metadata sections end on the first blank like, so moves to that.
            @param md_text The Markdown text, starting anywhere in the metadata section.
            @returns The start of the document's body.
            @sa has_metadata_section().*/
        [[nodiscard]]
        static const wchar_t* find_metadata_section_end(const wchar_t* md_text) noexcept
            {
            if (md_text == nullptr)
                {
                return nullptr;
                }
            // step over first line
            const wchar_t* eol = string_util::strcspn_pointer(md_text, L"\r\n", 2);
            if (eol == nullptr)
                {
                return md_text;
                }
            // ...and find the terminating --- line
            const wchar_t* endOfYaml = std::wcsstr(eol, L"\n---");
            if (endOfYaml == nullptr)
                {
                return md_text;
                }
            endOfYaml = string_util::strcspn_pointer(endOfYaml + 4, L"\r\n", 2);
            if (endOfYaml == nullptr)
                {
                return md_text;
                }
            while (*endOfYaml == L'\r' || *endOfYaml == L'\n')
                {
                ++endOfYaml;
                }

            return endOfYaml;
            }

        std::unique_ptr<markdown_extract_text> m_subParser{ nullptr };
        const wchar_t* m_currentStart{ nullptr };
        const wchar_t* m_currentEndSentinel{ nullptr };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // MARKDOWN_TEXT_EXTRACT_H
