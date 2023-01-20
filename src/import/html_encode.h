/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2010-2020
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __HTML_ENCODE_H__
#define __HTML_ENCODE_H__

#include "../util/stringutil.h"

namespace lily_of_the_valley
    {
    /// @brief Class to encode a string into HTML.
    class html_encode_text
        {
    public:
        /** @brief Encodes a regular string into HTML.
            @details This includes escaping quotes and angle symbols,
            and encoding tabs, newlines, and Unicode values.
            @param text The text to encode.
            @param length The length of the text to encode.\n
                Pass @c -1 for the function to determine the length.
            @param encodeSpaces @c true to preserve consecutive spaces with `&#nbsp;`.
            @returns A string encoded to HTML.*/
        [[nodiscard]] std::wstring operator()(const wchar_t* text, size_t length,
                                              const bool encodeSpaces) const
            {
            if (std::wstring::npos == length)
                { length = std::wcslen(text); }
            std::wstring encoded_text;
            if (text == nullptr || length == 0)
                { return encoded_text; }
            encoded_text.reserve(length*2);
            for (size_t i = 0; i < length; ++i)
                {
                if (text[i] >= 127)
                    {
                    encoded_text.append(L"&#").
                        append(std::to_wstring(static_cast<uint32_t>(text[i]))).
                        append(L";");
                    }
                else if (text[i] == L'<')
                    { encoded_text += L"&#60;"; }
                else if (text[i] == L'>')
                    { encoded_text += L"&#62;"; }
                else if (text[i] == L'\"')
                    { encoded_text += L"&#34;"; }
                else if (text[i] == L'&')
                    { encoded_text += L"&#38;"; }
                else if (text[i] == L'\'')
                    { encoded_text += L"&#39;"; }
                // turn carriage return/line feeds into HTML breaks
                else if (text[i] == 10 ||
                         text[i] == 13)
                    {
                    // treats CRLF combo as one break
                    if (i < length-1 &&
                        (text[i+1] == 10 ||
                        text[i+1] == 13) )
                        {
                        encoded_text += L"<p></p>";
                        // make one extra step for CRLF combination so
                        // that it counts as only one line break
                        ++i;
                        }
                    else
                        { encoded_text += L"<p></p>"; }
                    }
                else if (encodeSpaces && text[i] == L'\t')
                    { encoded_text += L"&nbsp;&nbsp;&nbsp;"; }
                else if (encodeSpaces && text[i] == L' ')
                    {
                    if (i > 0 && text[i-1] == L' ')
                        {
                        encoded_text += L"&nbsp;";
                        while (i+1 < length)
                            {
                            if (text[i+1] == L' ')
                                {
                                encoded_text += L"&nbsp;";
                                ++i;
                                }
                            else
                                { break; }
                            }
                        }
                    else
                        { encoded_text += text[i]; }
                    }
                else
                    { encoded_text += text[i]; }
                }
            return encoded_text;
            }
        /** @brief Determines if a block of text has characters in it that
                need to be encoded to be HTML compliant.
            @param text The text to be reviewed.
            @param length The length of @c text.
            @returns @c true if text should be HTML encoded.*/
        [[nodiscard]]
        static bool needs_to_be_encoded(const wchar_t* text, const size_t length)
            {
            if (text == nullptr || length == 0)
                { return false; }
            for (size_t scanCounter = 0; scanCounter < length; ++scanCounter)
                {
                if (text[scanCounter] >= 127 ||
                    string_util::is_one_of(text[scanCounter], L"&\"\'<>\n\r\t") ||
                    // consecutive spaces
                    (scanCounter > 0 && text[scanCounter] == L' ' && text[scanCounter-1] == L' '))
                    { return true; }
                }
            return false;
            }
        };
    }

/** @} */

#endif //__HTML_ENCODE_H__
