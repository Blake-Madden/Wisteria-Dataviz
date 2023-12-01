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
            @note Multimarkdown, YAML, and Pandoc sections are supported.
            @warning @c md_text must be the start of the Markdown document.*/
        [[nodiscard]]
        bool has_metadata_section(const wchar_t* md_text) const
            {
            if (!md_text)
                { return false; }
            // will just be reviewing the first line of text
            auto eol = string_util::strcspn_pointer(md_text, L"\n\r", 2);
            if (!eol)
                { return false; }

            return std::regex_match(md_text, eol, m_metadataSectionStart);
            }

        /** @brief Metadata sections end on the first blank like, so moves to that.
            @param md_text The Markdown text, starting anywhere in the metadata section.
            @returns The start of the document's body.
            @sa has_metadata_section().*/
        [[nodiscard]]
        static const wchar_t* find_metadata_section_end(const wchar_t* md_text) noexcept
            {
            if (!md_text)
                { return nullptr; }
            while (md_text[0] != 0 && md_text[1] != 0)
                {
                // if a Windows \r\n combination, then if followed by either 
                // \r or \n means we have a blank line
                if (md_text[0] == L'\r' && md_text[1] == L'\n')
                    {
                    if (md_text[2] == 0)
                        { return md_text+2; }
                    else if (string_util::is_either(md_text[2], L'\n', L'\r'))
                        { return md_text+3; }
                    }
                // otherwise, if 2 consecutive breaks then we have a blank line
                else if (string_util::is_either(md_text[0], L'\n', L'\r') &&
                    string_util::is_either(md_text[1], L'\n', L'\r'))
                    { return md_text+2; }
                ++md_text;
                }
            // if stopped on the last character because the whole file is just metadata,
            // then step to the end.
            if (md_text[0] != 0 && md_text[1] == 0)
                { return ++md_text; }

            return md_text;
            }

        /** Metadata section types start with these:
            - Pandoc: % and a space.
            - YAML header: ---*/
        std::wregex m_metadataSectionStart
            { LR"(^(%[[:space:]]+|---).*$)" };

        std::unique_ptr<markdown_extract_text> m_subParser{ nullptr };
        };
    }

/** @}*/

#endif //__MARKDOWN_TEXT_EXTRACT_H__
