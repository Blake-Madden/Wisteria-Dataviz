/** @addtogroup Importing
    @brief Classes for importing and parsing text.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __MARKDOWN_TEXT_EXTRACT_H__
#define __MARKDOWN_TEXT_EXTRACT_H__

#include <regex>
#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /// @brief Extracts plain text from a Markdown file.
    class markdown_extract_text final : public html_extract_text
        {
    public:
        /** @brief Main interface for extracting plain text from a Markdown file.
            @param md_text The markdown text to parse.
            @returns The parsed text from the Markdown stream.*/
        [[nodiscard]]
        const wchar_t* operator()(const std::wstring_view md_text);
#ifndef __UNITTEST
    private:
#endif
        /** @returns @c true if text marks the start of a Markdown metadata section.
            @param md_text The start of the Markdown text.
            @note YAML sections are supported.
            @warning @c md_text must be the start of the Markdown document.*/
        [[nodiscard]]
        bool has_metadata_section(const wchar_t* md_text) const
            {
            if (md_text == nullptr)
                { return false; }
            // does the first line start with ---
            return (std::wcsncmp(md_text, L"---", 3) == 0);
            }

        /** @brief Metadata sections end on the first blank like, so moves to that.
            @param md_text The Markdown text, starting anywhere in the metadata section.
            @returns The start of the document's body.
            @sa has_metadata_section().*/
        [[nodiscard]]
        static const wchar_t* find_metadata_section_end(const wchar_t* md_text) noexcept
            {
            if (md_text == nullptr)
                { return nullptr; }
            // step over first line
            auto eol = string_util::strcspn_pointer(md_text, L"\r\n", 2);
            if (eol == nullptr)
                {
                return md_text;
                }
            // ...and find the terminating --- line
            auto endOfYaml = std::wcsstr(eol, L"\n---");
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
        };
    }

/** @}*/

#endif //__MARKDOWN_TEXT_EXTRACT_H__
