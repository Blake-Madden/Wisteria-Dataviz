/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __HHC_HHK_EXTRACT_TEXT_H__
#define __HHC_HHK_EXTRACT_TEXT_H__

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from an HHK (Microsoft® HTML Workshop Index) &
            HHC (Microsoft® HTML Workshop Table of Contents) stream.*/
    class hhc_hhk_extract_text final : public html_extract_text
        {
    public:
        /** @brief Main interface for extracting plain text from an HTML Workshop index or
                table of contents buffer.
            @details This text is the labels shown in the TOC and index.
            @param html_text The HHK/HHC text to extract from.
            @param text_length The length of the text.
            @returns A pointer to the parsed text, or null upon failure.*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* html_text, const size_t text_length)
            {
            clear_log();
            if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
                {
                set_filtered_text_length(0);
                return nullptr;
                }
            assert(text_length <= std::wcslen(html_text) );

            if (!allocate_text_buffer(text_length))
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            // find the first < and set up where we halt our searching
            const wchar_t* start = std::wcschr(html_text, L'<');
            const wchar_t* endSentinel = html_text + text_length;
            string_util::case_insensitive_wstring currentTag;
            std::wstring attribValue;
            while (start && (start < endSentinel))
                {
                currentTag.assign(get_element_name(start+1));

                if (currentTag == L"param")
                    {
                    if (read_attribute_as_string(start + 6/*skip "<param"*/, L"name", 4, false) == L"Name")
                        {
                        attribValue =
                            read_attribute_as_string(start + 6, L"value", 5, false, true);
                        parse_raw_text(attribValue.c_str(), attribValue.length());
                        add_character(L'\n');
                        add_character(L'\n');
                        }
                    }
                // find the next starting tag
                start = std::wcschr(start + 1, L'<');
                if (!start)
                    { break; }
                }

            return get_filtered_text();
            }
        };
    }

/** @}*/
    
#endif //__HHC_HHK_EXTRACT_TEXT_H__
