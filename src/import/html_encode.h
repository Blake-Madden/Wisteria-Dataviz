/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef HTML_ENCODE_H
#define HTML_ENCODE_H

#include "../util/string_util.h"
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
        std::wstring operator()(const std::wstring_view text, const bool encodeSpaces) const
            {
            std::wstring encodedText;
            if (text.empty())
                {
                return encodedText;
                }
            encodedText.reserve(text.length() * 2);
            for (size_t i = 0; i < text.length(); ++i)
                {
                if (text[i] >= 127)
                    {
                    uint32_t codePoint{ static_cast<uint32_t>(text[i]) };
                    // on Windows wchar_t is UTF-16, so supplementary plane characters
                    // (e.g., emoji) arrive as surrogate pairs; combine them into the
                    // full code point so the entity is correct (e.g., &#128027; not
                    // &#55357;&#56350;)
                    if constexpr (sizeof(wchar_t) == 2)
                        {
                        if (codePoint >= 0xD800 && codePoint <= 0xDBFF && i + 1 < text.length())
                            {
                            const uint32_t low{ static_cast<uint32_t>(text[i + 1]) };
                            if (low >= 0xDC00 && low <= 0xDFFF)
                                {
                                codePoint = 0x10000 + ((codePoint - 0xD800) << 10) + (low - 0xDC00);
                                ++i;
                                }
                            }
                        }
                    encodedText.append(L"&#").append(std::to_wstring(codePoint)).append(L";");
                    }
                else if (text[i] == L'<')
                    {
                    encodedText += L"&#60;";
                    }
                else if (text[i] == L'>')
                    {
                    encodedText += L"&#62;";
                    }
                else if (text[i] == L'\"')
                    {
                    encodedText += L"&#34;";
                    }
                else if (text[i] == L'&')
                    {
                    encodedText += L"&#38;";
                    }
                else if (text[i] == L'\'')
                    {
                    encodedText += L"&#39;";
                    }
                // turn carriage return/line feeds into HTML breaks
                else if (text[i] == 10 || text[i] == 13)
                    {
                    // treats CRLF combo as one break
                    if (i < text.length() - 1 && (text[i + 1] == 10 || text[i + 1] == 13))
                        {
                        encodedText += L"<p></p>";
                        // make one extra step for CRLF combination so
                        // that it counts as only one line break
                        ++i;
                        }
                    else
                        {
                        encodedText += L"<p></p>";
                        }
                    }
                else if (encodeSpaces && text[i] == L'\t')
                    {
                    encodedText += L"&nbsp;&nbsp;&nbsp;";
                    }
                else if (encodeSpaces && text[i] == L' ')
                    {
                    if (i > 0 && text[i - 1] == L' ')
                        {
                        encodedText += L"&nbsp;";
                        while (i + 1 < text.length())
                            {
                            if (text[i + 1] == L' ')
                                {
                                encodedText += L"&nbsp;";
                                ++i;
                                }
                            else
                                {
                                break;
                                }
                            }
                        }
                    else
                        {
                        encodedText += text[i];
                        }
                    }
                else
                    {
                    encodedText += text[i];
                    }
                }
            return encodedText;
            }

        /** @brief Simplified version that encodes a regular string into HTML.
            @details This only encodes `<`, `>`, and `&`.
            @param text The text to encode.
            @returns A string (partially) encoded to HTML.*/
        [[nodiscard]]
        static std::wstring simple_encode(const std::wstring_view text)
            {
            std::wstring encodedText;
            if (text.empty())
                {
                return encodedText;
                }
            encodedText.reserve(text.length() * 2);
            for (const auto character : text)
                {
                if (character == L'<')
                    {
                    encodedText += L"&#60;";
                    }
                else if (character == L'>')
                    {
                    encodedText += L"&#62;";
                    }
                else if (character == L'&')
                    {
                    encodedText += L"&#38;";
                    }
                else
                    {
                    encodedText += character;
                    }
                }
            return encodedText;
            }

        /** @brief Determines if a block of text has characters in it that
                need to be encoded to be HTML compliant.
            @details Only checks for `<`. `>`, and `&`.
            @param text The text to be reviewed.
            @returns @c true if text should be (simple) HTML encoded.*/
        [[nodiscard]]
        static bool needs_to_be_simple_encoded(const std::wstring_view text)
            {
            return text.find_first_of(L"<>&") != std::wstring_view::npos;
            }

        /** @brief Determines if a block of text has characters in it that
                need to be encoded to be HTML compliant.
            @param text The text to be reviewed.
            @returns @c true if text should be HTML encoded.*/
        [[nodiscard]]
        static bool needs_to_be_encoded(const std::wstring_view text)
            {
            if (text.empty())
                {
                return false;
                }
            for (size_t scanCounter = 0; scanCounter < text.length(); ++scanCounter)
                {
                if (text[scanCounter] >= 127 ||
                    string_util::is_one_of(text[scanCounter], L"&\"\'<>\n\r\t") ||
                    // consecutive spaces
                    (scanCounter > 0 && text[scanCounter] == L' ' && text[scanCounter - 1] == L' '))
                    {
                    return true;
                    }
                }
            return false;
            }
        };

    /// @brief HTML formatting helper.
    class html_format
        {
      public:
        /// @brief Adds a title to an HTML block.
        /// @param[in,out] htmlText The HTML to set the title within.
        /// @param title The title to use.
        /// @note @c string_typeT must be a @c std::wstring-compatible string type.
        template<typename string_typeT>
        static void set_title(string_typeT& htmlText, const std::wstring& title)
            {
            auto titleStart = htmlText.find(L"<title>");
            if (titleStart == string_typeT::npos)
                {
                auto headStart = htmlText.find(L"<head>");
                // add <head> section if needed
                if (headStart == string_typeT::npos)
                    {
                    auto htmlStart = htmlText.find(L"<html");
                    if (htmlStart == string_typeT::npos)
                        {
                        return;
                        } // give up if this is bogus HTML
                    // find the end of the <html> tag
                    htmlStart = htmlText.find(L'>', htmlStart);
                    // give up if this is bogus HTML
                    if (htmlStart == string_typeT::npos)
                        {
                        return;
                        }
                    headStart = htmlStart + 1;
                    htmlText.insert(headStart, L"\n<head></head>\n");
                    // skip newline in front of <head>
                    ++headStart;
                    }
                htmlText.insert(headStart + 6, L"\n<title></title>");
                // skip over '<head>\n'
                titleStart = headStart + 7;
                }
            // skip over <title>
            titleStart += 7;
            const auto titleEnd = htmlText.find(L"</title>", titleStart);
            if (titleEnd == string_typeT::npos)
                {
                return;
                }
            htmlText.replace(titleStart, (titleEnd - titleStart), title.c_str());
            }

        /// @brief Specifies the encoding of an HTML block.
        /// @param[in,out] htmlText The HTML to set edit.
        /// @param encoding The encoding to use.
        /// @note @c string_typeT must be a @c std::wstring-compatible string type.
        template<typename string_typeT>
        static void set_encoding(string_typeT& htmlText, const std::wstring& encoding = L"UTF-8")
            {
            auto headStart = htmlText.find(L"<head");
            // add <head> section if needed
            if (headStart == string_typeT::npos)
                {
                auto htmlStart = htmlText.find(L"<html");
                // give up if this is bogus HTML
                if (htmlStart == string_typeT::npos)
                    {
                    return;
                    }
                // find the end of the <html> tag
                htmlStart = htmlText.find(L'>', htmlStart);
                // give up if this is bogus HTML
                if (htmlStart == string_typeT::npos)
                    {
                    return;
                    }
                headStart = htmlStart + 1;
                htmlText.insert(headStart, L"\n<head></head>\n");
                // skip newline in front of <head>
                ++headStart;
                }
            headStart += 5;
            headStart = htmlText.find(L'>', headStart);
            if (headStart == string_typeT::npos)
                {
                return;
                }
            ++headStart;

            const size_t metaStart = htmlText.find(L"<meta", headStart);
            if (metaStart == string_typeT::npos)
                {
                const std::wstring encodingDef =
                    L"<meta http-equiv=\"content-type\" content=\"text/html; charset=" + encoding +
                    L"\" />";
                htmlText.insert(headStart, encodingDef.c_str());
                }
            /// @todo if meta section is actually found then update it
            }

        /** @brief Removes any hyperlinks in a file, and optionally preserve bookmarks
                that are in the same file.
            @details This is mostly used for HTML windows that have application-related
                bookmarks in them that need to be removed prior to printing or saving them.
            @param[in,out] htmlText The text to strip hyperlinks from.
            @param preserveInPageBookmarks Whether to preserve hyperlinks to bookmarks that
                happen to be in the current block of text.
            @note @c string_typeT must be a @c std::wstring-compatible string type.*/
        template<typename string_typeT>
        static void strip_hyperlinks(string_typeT& htmlText,
                                     const bool preserveInPageBookmarks = true)
            {
            std::set<std::wstring> bookmarksInCurrentPage;
            std::pair<const wchar_t*, std::wstring> foundBookMark{ htmlText.c_str(),
                                                                   std::wstring() };
            assert(foundBookMark.first);
            if (foundBookMark.first == nullptr)
                {
                return;
                }
            const wchar_t* const htmlEnd = foundBookMark.first + htmlText.length();
            while (preserveInPageBookmarks && (foundBookMark.first != nullptr))
                {
                foundBookMark = html_extract_text::find_bookmark(foundBookMark.first, htmlEnd);
                if (foundBookMark.first != nullptr)
                    {
                    bookmarksInCurrentPage.insert(foundBookMark.second);
                    foundBookMark.first += foundBookMark.second.length();
                    }
                else
                    {
                    break;
                    }
                }

            size_t start{ 0 };
            while (start != string_typeT::npos)
                {
                start = htmlText.find(L"<a href=", start);
                if (start == string_typeT::npos)
                    {
                    break;
                    }
                const auto endOfTag = htmlText.find(L'>', start);
                if (endOfTag == string_typeT::npos)
                    {
                    break;
                    }
                const auto startOfLink = start + 8;
                string_typeT link = htmlText.substr(startOfLink, (endOfTag - startOfLink));
                if (!link.empty() && link[0] == '\"')
                    {
                    link.erase(0, 1);
                    }
                if (!link.empty() && link[link.length() - 1] == '\"')
                    {
                    link.erase(link.length() - 1, 1);
                    }
                // see if it's a bookmark into the current page
                if (!link.empty() && link[0] == '#')
                    {
                    link.erase(0, 1);
                    // if the bookmark isn't found in this file then remove the link to it
                    if (!bookmarksInCurrentPage.contains(std::wstring{ link.c_str() }))
                        {
                        htmlText.erase(start, (endOfTag - start) + 1);
                        const size_t endOfAnchor = htmlText.find(L"</a>", start);
                        if (endOfAnchor == string_typeT::npos)
                            {
                            continue;
                            }
                        htmlText.erase(endOfAnchor, 4);
                        }
                    // bookmark was found and we didn't delete this link, so move over it instead
                    else
                        {
                        start = endOfTag;
                        }
                    }
                // not an internal bookmark, so just remove it
                else
                    {
                    htmlText.erase(start, (endOfTag - start) + 1);
                    const auto endOfAnchor = htmlText.find(L"</a>", start);
                    if (endOfAnchor == string_typeT::npos)
                        {
                        continue;
                        }
                    htmlText.erase(endOfAnchor, 4);
                    }
                }
            }

        /// @brief Removes any image tags from an HTML block.
        /// @param[in,out] htmlText The HTML to strip.
        /// @param removePadding @c true to remove padding (i.e., `&nbsp;`) around the images.
        /// @note @c string_typeT must be a @c std::wstring-compatible string type.
        template<typename string_typeT>
        static void strip_images(string_typeT& htmlText, const bool removePadding = true)
            {
            size_t start{ 0 };
            while (start != string_typeT::npos)
                {
                start = htmlText.find(L"<img ", start);
                if (start == string_typeT::npos)
                    {
                    break;
                    }
                const size_t endOfTag = htmlText.find(L'>', start);
                if (endOfTag == string_typeT::npos)
                    {
                    break;
                    }
                // remove padding that was around the image
                if (removePadding && start > 6 && htmlText.substr(start - 6, 6) == L"&nbsp;")
                    {
                    start -= 6;
                    }
                htmlText.erase(start, (endOfTag - start) + 1);
                // remove padding that was around the image
                if (removePadding && (htmlText.length() - start) >= 6 &&
                    htmlText.substr(start, 6) == L"&nbsp;")
                    {
                    htmlText.erase(start, 6);
                    }
                }
            }

        /// @brief Removes any attributes in the `<body>` element.
        /// @param[in,out] htmlText The HTML to strip.
        /// @note @c string_typeT must be a @c std::wstring-compatible string type.
        template<typename string_typeT>
        static void strip_body_attributes(string_typeT& htmlText)
            {
            auto start = htmlText.find(L"<body ");
            if (start != string_typeT::npos)
                {
                const auto endOfTag = htmlText.find(L'>', start);
                if (endOfTag == string_typeT::npos)
                    {
                    return;
                    }
                start += 5;
                htmlText.erase(start, endOfTag - start);
                }
            }
        };
    } // namespace lily_of_the_valley

/** @} */

#endif // HTML_ENCODE_H
