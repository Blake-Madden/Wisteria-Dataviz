/** @addtogroup Importing
    @brief Classes for importing and parsing text.
    @date 2005-2023
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
            @returns A pointer to the parsed text, or null upon failure.*/
        [[nodiscard]]
        const wchar_t*
        operator()(const std::wstring_view idl_buffer)
            {
            static const std::wstring HELP_STRING{ L"helpstring(\"" };
            clear_log();
            clear();
            if (idl_buffer.empty())
                {
                return nullptr;
                }

            allocate_text_buffer(idl_buffer.length());

            auto nextHelpStr = idl_buffer.find(HELP_STRING.c_str());
            while (nextHelpStr != std::wstring_view::npos)
                {
                nextHelpStr += HELP_STRING.length();
                const auto endOfHelpStr = idl_buffer.find(L'\"', nextHelpStr);
                if (endOfHelpStr == std::wstring_view::npos)
                    {
                    break;
                    }
                add_characters(idl_buffer.substr(nextHelpStr, (endOfHelpStr - nextHelpStr)));
                add_character(L'\n');
                add_character(L'\n');
                nextHelpStr = idl_buffer.find(HELP_STRING.c_str(), nextHelpStr);
                }

            // returns the text buffer
            return get_filtered_text();
            }
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__IDL_EXTRACT_TEXT_H__
