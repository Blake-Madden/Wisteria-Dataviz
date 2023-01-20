/** @addtogroup Importing
    @brief Classes for importing and parsing text.
    @date 2005-2020
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __IDL_EXTRACT_TEXT_H__
#define __IDL_EXTRACT_TEXT_H__

#include "extract_text.h"

namespace lily_of_the_valley
    {
    /// @brief Functor to extract text from an IDL stream.
    class idl_extract_text final : public extract_text
        {
    public:
        /** @brief Main interface for extracting plain text from an IDL buffer.
            @param idl_buffer The IDL text to strip.
            @param text_length The length of the text.
            @returns A pointer to the parsed text, or null upon failure.*/
        [[nodiscard]]
        const wchar_t* operator()(const wchar_t* idl_buffer, const size_t text_length)
            {
            clear_log();
            if (idl_buffer == nullptr || idl_buffer[0] == 0 || text_length == 0)
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            if (!allocate_text_buffer(text_length))
                {
                set_filtered_text_length(0);
                return nullptr;
                }

            const wchar_t* nextHelpStr = std::wcsstr(idl_buffer, L"helpstring(\"");
            const wchar_t* endOfHelpStr = nullptr;
            while (nextHelpStr || nextHelpStr > (idl_buffer+text_length))
                {
                nextHelpStr += 12;
                endOfHelpStr = std::wcschr(nextHelpStr, L'\"');
                if (!endOfHelpStr || endOfHelpStr > (idl_buffer+text_length))
                    { break; }
                add_characters(nextHelpStr, (endOfHelpStr-nextHelpStr));
                add_character(L'\n');
                add_character(L'\n');
                nextHelpStr = std::wcsstr(nextHelpStr, L"helpstring(\"");
                }

            // returns the text buffer
            return get_filtered_text();
            }
        };
    }

/** @}*/

#endif //__IDL_EXTRACT_TEXT_H__
