/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef HHC_HHK_EXTRACT_TEXT_H
#define HHC_HHK_EXTRACT_TEXT_H

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
            @param hhcText The HHK/HHC text to extract from.
            @param textLength The length of the text.
            @returns A pointer to the parsed text, or null upon failure.*/
        const wchar_t* operator()(const wchar_t* hhcText, const size_t textLength)
            {
            clear_log();
            clear();
            if (hhcText == nullptr || hhcText[0] == 0 || textLength == 0)
                {
                return nullptr;
                }
            assert(textLength <= std::wcslen(hhcText));

            allocate_text_buffer(textLength);

            // find the first < and set up where we halt our searching
            const wchar_t* start = std::wcschr(hhcText, L'<');
            const wchar_t* const endSentinel = hhcText + textLength;
            string_util::case_insensitive_wstring currentTag;
            std::wstring attribValue;
            while (start != nullptr && (start < endSentinel))
                {
                currentTag.assign(get_element_name(std::next(start), true));

                if (currentTag == L"param")
                    {
                    if (read_attribute_as_string(std::next(start, 6) /*skip "<param"*/, L"name",
                                                 false, false) == L"Name")
                        {
                        attribValue =
                            read_attribute_as_string(std::next(start, 6), L"value", false, true);
                        parse_raw_text(attribValue.c_str(), attribValue.length());
                        add_character(L'\n');
                        add_character(L'\n');
                        }
                    }
                // find the next starting tag
                start = std::wcschr(std::next(start), L'<');
                if (start == nullptr)
                    {
                    break;
                    }
                }

            return get_filtered_text();
            }
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // HHC_HHK_EXTRACT_TEXT_H
