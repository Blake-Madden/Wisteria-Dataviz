/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __RTF_ENCODE_H__
#define __RTF_ENCODE_H__

#include "../i18n-check/src/string_util.h"

namespace lily_of_the_valley
    {
    /// @brief Class to format text into RTF.
    /// @date 2010
    class rtf_encode_text
        {
    public:
        /** @brief Encodes a regular string into RTF.
            @details This includes escaping '\', '{', and '}' symbols,
                and encoding tabs, newlines, and Unicode values.
            @param text The text to encode.
            @returns A string encoded to RTF.
            @todo Replace @c swprintf() code with `std::format()` when upgrading to C++20.*/
        [[nodiscard]]
        std::wstring operator()(const std::wstring_view text) const
            {
            std::wstring encoded_text;
            if (text.empty())
                { return encoded_text; }
            encoded_text.reserve(text.length() * 2);
            std::array<wchar_t, 256> formatBuffer{ 0 };
            for (size_t i = 0; i < text.length(); ++i)
                {
                // insert a '\' in front of RTF escape characters
                if (string_util::is_one_of(text[i], L"\\{}") )
                    {
                    encoded_text += L'\\';
                    encoded_text += text[i];
                    }
                // extended ASCII and Unicode have to be encoded differently due to RTF quirkiness
                else if (text[i] >= 127 && text[i] <= 255)
                    {
                    const int retVal = std::swprintf(formatBuffer.data(), formatBuffer.size(), L"\\\'%02X", text[i]);
                    assert(retVal != -1);
                    if (retVal != -1)
                        { encoded_text.append(formatBuffer.data(), retVal); }
                    }
                else if (text[i] > 255)
                    {
                    const int retVal = std::swprintf(formatBuffer.data(), formatBuffer.size(), L"\\u%04d?", static_cast<int>(text[i]));
                    assert(retVal != -1);
                    if (retVal != -1)
                        { encoded_text.append(formatBuffer.data(), retVal); }
                    }
                // encode tabs
                else if (text[i] == 9)
                    { encoded_text += L"\\tab "; }
                // turn carriage return/line feeds into RTF breaks
                else if (text[i] == 10 ||
                        text[i] == 13)
                    {
                    // treats CRLF combo as one break
                    if (i < text.length() - 1 &&
                        (text[i+1] == 10 ||
                        text[i+1] == 13) )
                        {
                        encoded_text += L"\\par\n";
                        // make one extra step for CRLF combination
                        // so that it counts as only one line break
                        ++i;
                        }
                    else
                        { encoded_text += L"\\par\n"; }
                    }
                else
                    { encoded_text += text[i]; }
                }
            return encoded_text;
            }

        /** @brief Determines if a block of text has characters in it that need to
                be encoded to be RTF compliant.
            @param text The text to review.
            @returns @c true if text should be encoded.*/
        [[nodiscard]]
        bool needs_to_be_encoded(const std::wstring_view text) const
            {
            if (text.empty())
                { return false; }
            const auto foundPos = std::find_if(text.cbegin(), text.cend(),
                [](const auto& ch) noexcept
                {
                return (ch >= 127 || string_util::is_one_of(ch, L"\\{}\r\n\t"));
                });
            return (foundPos != text.cend());
            }
        };
    }

/** @} */

#endif //__RTF_ENCODE_H__
