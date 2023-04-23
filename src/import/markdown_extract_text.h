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
    /// @warning This is in alpha state. Do not use in productino.
    class markdown_extract_text final : public html_extract_text
        {
    public:
        /** @brief Main interface for extracting plain text from a Markdown file.
            @param md_text The markdown text to parse.
            @param text_length The length of the text.
            @returns The parsed text from the Markdown stream.*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* md_text,
                                  const size_t text_length)
            {
            clear_log();
            if (md_text == nullptr || md_text[0] == 0 || text_length == 0)
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            if (!allocate_text_buffer(text_length))
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            // find the start of the text body and set up where we halt our searching
            const wchar_t* const endSentinel = md_text+text_length;
            if (is_metadata_section(md_text))
                { md_text = find_metadata_section_end(md_text); }
            while (md_text < endSentinel && md_text[0] != 0 && std::iswspace(md_text[0]))
                { ++md_text; }
            // in case metadata section ate up the whole file
            // (or at least the part of the file requested to be reviewed)
            if (md_text >= endSentinel)
                { return endSentinel; }
            const wchar_t* start = md_text;
            const wchar_t* end = md_text;

            while (end != nullptr  && end[0] != 0 && (end < endSentinel))
                { ++end; }
            // pick up any remaining text at the end of the text stream..
            if (end != nullptr && end[0] == 0)
                { add_characters(start, end-start); }
            // ...or if end is beyond where we should go in the buffer,
            // copy up to the end of the buffer
            else if (end >= endSentinel)
                { add_characters(start, endSentinel-start); }

            return get_filtered_text();
            }
#ifndef __UNITTEST
    private:
#endif
        /** @returns @c true if text marks the start of a Markdown metadata section.
            @param md_text The start of the Markdown text.
            @note Multimarkdown, YAML, and Pandoc sections are supported.
            @warning @c md_text must be the start of the Markdown document.*/
        [[nodiscard]]
        bool is_metadata_section(const wchar_t* md_text) const
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
            @sa is_metadata_section().*/
        [[nodiscard]]
        const wchar_t* find_metadata_section_end(const wchar_t* md_text) const noexcept
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
            - YAML header: ---
            - Multimarkdown: word, followed by an optional space and other word,
              followed by colon.*/
        std::wregex m_metadataSectionStart
            { LR"(^(%[[:space:]]+|---|[[:alpha:]]+([[:space:]]{1}[[:alpha:]]+)?[:]).*$)" };
        };
    }

/** @}*/

#endif //__MARKDOWN_TEXT_EXTRACT_H__
