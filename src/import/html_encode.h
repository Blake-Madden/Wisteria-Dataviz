/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2020
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __HTML_ENCODE_H__
#define __HTML_ENCODE_H__

#include "../i18n-check/src/string_util.h"
#include "html_extract_text.h"

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
            @param encodeSpaces @c true to preserve consecutive spaces with `&#nbsp;`.
            @returns A string encoded to HTML.*/
        [[nodiscard]]
        std::wstring operator()(const std::wstring_view text,
                                const bool encodeSpaces) const
            {
            std::wstring encoded_text;
            if (text.empty())
                { return encoded_text; }
            encoded_text.reserve(text.length() * 2);
            for (size_t i = 0; i < text.length(); ++i)
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
                    if (i < text.length() - 1 &&
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
                        while (i+1 < text.length())
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

        /** @brief Simplified version that encodes a regular string into HTML.
            @details This only encodes `<`, `>`, and `&`.
            @param text The text to encode.
            @returns A string (partially) encoded to HTML.*/
        [[nodiscard]]
        static std::wstring simple_encode(const std::wstring_view text)
            {
            std::wstring encoded_text;
            if (text.empty())
                { return encoded_text; }
            encoded_text.reserve(text.length() * 2);
            for (const auto character : text)
                {
                if (character == L'<')
                    { encoded_text += L"&#60;"; }
                else if (character == L'>')
                    { encoded_text += L"&#62;"; }
                else if (character == L'&')
                    { encoded_text += L"&#38;"; }
                else
                    { encoded_text += character; }
                }
            return encoded_text;
            }

        /** @brief Determines if a block of text has characters in it that
                need to be encoded to be HTML compliant.
            @param text The text to be reviewed.
            @returns @c true if text should be HTML encoded.*/
        [[nodiscard]]
        static bool needs_to_be_encoded(const std::wstring_view text)
            {
            if (text.empty())
                { return false; }
            for (size_t scanCounter = 0; scanCounter < text.length(); ++scanCounter)
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

    /// @brief HTML formatting helper.
    class html_format
        {
    public:
        /// @brief Adds a title to an HTML buffer.
        /// @param[in,out] HtmlText The HTML to set the title within.
        /// @param Title The title to use.
        /// @todo needs unit test.
        static void set_title(std::wstring& HtmlText, const std::wstring& Title)
            {
            auto titleStart = HtmlText.find(L"<title>");
            if (titleStart == std::wstring::npos)
                {
                auto headStart = HtmlText.find(L"<head>");
                // add <head> section if needed
                if (headStart == std::wstring::npos)
                    {
                    auto htmlStart = HtmlText.find(L"<html");
                    if (htmlStart == std::wstring::npos)
                        { return; }//give up if this is bogus HTML
                    // find the end of the <html> tag
                    htmlStart = HtmlText.find(L">", htmlStart);
                    // give up if this is bogus HTML
                    if (htmlStart == std::wstring::npos)
                        { return; }
                    headStart = htmlStart + 1;
                    HtmlText.insert(headStart, L"\n<head></head>\n");
                    // skip newline in front of <head>
                    ++headStart;
                    }
                HtmlText.insert(headStart + 6, L"\n<title></title>");
                // skip over '<head>\n'
                titleStart = headStart + 7;
                }
            // skip over <title>
            titleStart += 7;
            const auto titleEnd = HtmlText.find(L"</", titleStart);
            if (titleEnd == std::wstring::npos)
                { return; }
            HtmlText.replace(titleStart, (titleEnd-titleStart), Title);
            }
        /// @brief Specifies the encoding of an HTML buffer.
        /// @param[in,out] HtmlText The HTML to set edit.
        /// @param encoding The encoding to use.
        /// @todo needs unit test.
        static void set_encoding(std::wstring& HtmlText, const std::wstring& encoding = L"UTF-8")
            {
            auto headStart = HtmlText.find(L"<head");
            // add <head> section if needed
            if (headStart == std::wstring::npos)
                {
                auto htmlStart = HtmlText.find(L"<html");
                // give up if this is bogus HTML
                if (htmlStart == std::wstring::npos)
                    { return; }
                // find the end of the <html> tag
                htmlStart = HtmlText.find(L">", htmlStart);
                // give up if this is bogus HTML
                if (htmlStart == std::wstring::npos)
                    { return; }
                headStart = htmlStart+1;
                HtmlText.insert(headStart, L"\n<head></head>\n");
                // skip newline in front of <head>
                ++headStart;
                }
            headStart += 5;
            headStart = HtmlText.find(L">", headStart);
            if (headStart == std::wstring::npos)
                { return; }
            ++headStart;

            const size_t metaStart = HtmlText.find(L"<meta", headStart);
            if (metaStart == std::wstring::npos)
                {
                const std::wstring encodingDef =
                    L"<meta http-equiv=\"content-type\" content=\"text/html; charset=" +
                    encoding + L"\" />";
                HtmlText.insert(headStart, encodingDef);
                }
            /// @todo if meta section is actually found then update it
            }
        /** @brief Removes any hyperlinks in a file, and optionally preserve bookmarks
                that are in the same file.
            @details This is mostly used for HTML windows that have application-related
                bookmarks in them that need to be removed prior to printing or saving them.
            @param[in,out] HtmlText The text to strip hyperlinks from.
            @param preserveInPageBookmarks Whether or not to preserve hyperlinks to bookmarks that
                happen to be in the current block of text.
            @todo needs unit test.*/
        static void strip_hyperlinks(std::wstring& HtmlText,
                                     const bool preserveInPageBookmarks = true)
            {
            std::set<std::wstring> BookmarksInCurrentPage;
            std::pair<const wchar_t*, std::wstring> foundBookMark{ HtmlText.c_str(), std::wstring() };
            assert(foundBookMark.first);
            if (!foundBookMark.first)
                { return; }
            const wchar_t* const htmlEnd = foundBookMark.first + HtmlText.length();
            while (preserveInPageBookmarks && foundBookMark.first)
                {
                foundBookMark = html_extract_text::find_bookmark(foundBookMark.first, htmlEnd);
                if (foundBookMark.first)
                    {
                    BookmarksInCurrentPage.insert(foundBookMark.second);
                    foundBookMark.first += foundBookMark.second.length();
                    }
                else
                    { break; }
                }

            size_t start = 0;
            while (start != std::wstring::npos)
                {
                start = HtmlText.find(L"<a href=", start);
                if (start == std::wstring::npos)
                    { break; }
                const auto endOfTag = HtmlText.find(L">", start);
                if (endOfTag == std::wstring::npos)
                    { break; }
                const auto startOfLink = start + 8;
                std::wstring link = HtmlText.substr(startOfLink, (endOfTag-startOfLink));
                if (link.length() && link[0] == '\"')
                    { link.erase(0, 1); }
                if (link.length() && link[link.length()-1] == '\"')
                    { link.erase(link.length()-1, 1); }
                // see if it's a bookmark into the current page
                if (link.length() && link[0] == '#')
                    {
                    link.erase(0, 1);
                    // if the bookmark isn't found in this file then remove the link to it
                    if (BookmarksInCurrentPage.find(link) == BookmarksInCurrentPage.cend())
                        {
                        HtmlText.erase(start, (endOfTag-start)+1);
                        const size_t endOfAnchor = HtmlText.find(L"</a>", start);
                        if (endOfAnchor == std::wstring::npos)
                            { continue; }
                        HtmlText.erase(endOfAnchor, 4);
                        }
                    // bookmark was found and we didn't delete this link, so move over it instead 
                    else
                        { start = endOfTag; }
                    }
                // not an internal bookmark, so just remove it
                else
                    {
                    HtmlText.erase(start, (endOfTag-start)+1);
                    const auto endOfAnchor = HtmlText.find(L"</a>", start);
                    if (endOfAnchor == std::wstring::npos)
                        { continue; }
                    HtmlText.erase(endOfAnchor, 4);
                    }
                }
            }
        /// @brief Removes any image tags from an HTML block
        /// @param[in,out] HtmlText The HTML to strip.
        /// @param removePadding @c true to remove padding (i.e., `&nbsp;`) around the images.
        /// @todo needs unit test.
        /// @todo use wstring_view.
        static void strip_images(std::wstring& HtmlText, const bool removePadding = true)
            {
            size_t start = 0;
            while (start != std::wstring::npos)
                {
                start = HtmlText.find(L"<img ", start);
                if (start == std::wstring::npos)
                    { break; }
                const size_t endOfTag = HtmlText.find(L">", start);
                if (endOfTag == std::wstring::npos)
                    { break; }
                // remove padding that was around the image
                if (removePadding &&
                    start > 6 &&
                    HtmlText.substr(start-6, 6) == L"&nbsp;")
                    { start -= 6; }
                HtmlText.erase(start, (endOfTag-start)+1);
                // remove padding that was around the image
                if (removePadding &&
                    (HtmlText.length()-start) >= 6 &&
                    HtmlText.substr(start, 6) == L"&nbsp;")
                    {HtmlText.erase(start, 6); }
                }
            }
        /// @brief Removes any attributes in the `<body>` element.
        /// @param[in,out] HtmlText The HTML to strip.
        /// @todo needs unit test.
        static void strip_body_atributes(std::wstring& HtmlText)
            {
            auto start = HtmlText.find(L"<body ");
            if (start != std::wstring::npos)
                {
                const auto endOfTag = HtmlText.find(L">", start);
                if (endOfTag == std::wstring::npos)
                    { return; }
                start += 5;
                HtmlText.erase(start, endOfTag-start);
                }
            }
        };
    }

/** @} */

#endif //__HTML_ENCODE_H__
