/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2010-2020
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __RTF_ENCODE_H__
#define __RTF_ENCODE_H__

#include "../util/stringutil.h"

namespace lily_of_the_valley
    {
    /// @brief Class to format text into RTF.
    class rtf_encode_text
        {
    public:
        /** @brief Encodes a regular string into RTF.
            @details This includes escaping '\', '{', and '}' symbols,
                and encoding tabs, newlines, and Unicode values.
            @param text The text to encode.
            @param length The length of the text to encode. Pass @c -1 for the function
                to determine the length.
            @returns A string encoded to RTF.*/
        [[nodiscard]]
        std::wstring operator()(const wchar_t* text, size_t length) const
            {
            if (std::wstring::npos == length)
                { length = std::wcslen(text); }
            std::wstring encoded_text;
            if (text == nullptr || length == 0)
                { return encoded_text; }
            encoded_text.reserve(length*2);
            wchar_t formatBuffer[256]; std::memset(formatBuffer,0,sizeof(formatBuffer));
            const size_t formatBufferSize = sizeof(formatBuffer)/sizeof(wchar_t);
            for (size_t i = 0; i < length; ++i)
                {
                //insert a '\' in front of RTF escape characters
                if (string_util::is_one_of(text[i], L"\\{}") )
                    {
                    encoded_text += L'\\';
                    encoded_text += text[i];
                    }
                //extended ASCII and Unicode have to be encoded differently due to RTF quirkiness
                else if (text[i] >= 127 && text[i] <= 255)
                    {
                    const int retVal = std::swprintf(formatBuffer, formatBufferSize, L"\\\'%02X", text[i]);
                    assert(retVal != -1);
                    if (retVal != -1)
                        { encoded_text.append(formatBuffer, retVal); }
                    }
                else if (text[i] > 255)
                    {
                    const int retVal = std::swprintf(formatBuffer, formatBufferSize, L"\\u%04d?", static_cast<int>(text[i]));
                    assert(retVal != -1);
                    if (retVal != -1)
                        { encoded_text.append(formatBuffer, retVal); }
                    }
                //encode tabs
                else if (text[i] == 9)
                    { encoded_text += L"\\tab "; }
                //turn carriage return/line feeds into RTF breaks
                else if (text[i] == 10 ||
                        text[i] == 13)
                    {
                    //Treats CRLF combo as one break
                    if (i < length-1 &&
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
            @param length The length of @c text.
            @returns @c true if text should be encoded.*/
        [[nodiscard]]
        bool needs_to_be_encoded(const wchar_t* text, const size_t length) const
            {
            if (text == nullptr || length == 0)
                { return false; }
            for (size_t scanCounter = 0; scanCounter < length; ++scanCounter)
                {
                if (text[scanCounter] >= 127 ||
                    string_util::is_one_of(text[scanCounter], L"\\{}\r\n\t"))
                    { return true; }
                }
            return false;
            }
        };
    }

/** @} */

#endif //__RTF_ENCODE_H__
