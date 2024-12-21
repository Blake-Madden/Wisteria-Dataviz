/********************************************************************************
 * Copyright (c) 2021-2025 Blake Madden
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License 2.0 which is available at
 * https://www.eclipse.org/legal/epl-2.0.
 *
 * SPDX-License-Identifier: EPL-2.0
 *
 * Contributors:
 *   Blake Madden - initial implementation
 ********************************************************************************/

#include "i18n_string_util.h"

namespace i18n_string_util
    {
    //--------------------------------------------------
    bool is_url(std::wstring_view text)
        {
        if (text.length() < 5) // NOLINT
            {
            return false;
            }
        // protocols
        if (string_util::strnicmp(text, std::wstring_view{ L"http://" }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"https://" }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"ftp://" }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"www." }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"mailto:" }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"file://" }) == 0)
            {
            return true;
            }
        if (string_util::strnicmp(text, std::wstring_view{ L"local://" }) == 0)
            {
            return true;
            }
        // relic from the '90s
        if (string_util::strnicmp(text, std::wstring_view{ L"gopher://" }) == 0)
            {
            return true;
            }

        // an URL that is missing the "www" prefix (e.g, ibm.com/index.html)
        const size_t firstSlash = text.find(L'/');
        if (firstSlash != std::wstring_view::npos)
            {
            const size_t lastDotPos = text.find_last_of(L'.', firstSlash);
            if (lastDotPos != std::wstring_view::npos && lastDotPos > 0 &&
                (lastDotPos + 4 == firstSlash) &&
                static_cast<bool>(std::iswalpha(text[lastDotPos - 1])) &&
                static_cast<bool>(std::iswalpha(text[lastDotPos + 1])) &&
                static_cast<bool>(std::iswalpha(text[lastDotPos + 2])) &&
                static_cast<bool>(std::iswalpha(text[lastDotPos + 3])))
                {
                return true;
                }
            }

        // cut off possessive form
        if (text.length() >= 3 && is_apostrophe(text[text.length() - 2]) &&
            string_util::is_either(text[text.length() - 1], L's', L'S'))
            {
            text.remove_suffix(2);
            }

        static const std::set<std::wstring> knownWebExtensions = { L"au",  L"biz", L"ca",
                                                                   L"com", L"edu", L"gov",
                                                                   L"ly",  L"org", L"uk" };

        size_t periodPos = text.find_last_of(L'.', text.length() - 1);
        if (periodPos != std::wstring::npos && periodPos < text.length() - 1)
            {
            ++periodPos;
            if (knownWebExtensions.find(
                    std::wstring(text.data() + periodPos, text.length() - periodPos)) !=
                knownWebExtensions.cend())
                {
                const size_t numberOfSpaces = std::count_if(
                    text.cbegin(), text.cend(), [](const auto chr) { return chr == L' '; });
                // Has a suffix like ".com" but is lengthy and has not slash in it?
                // Probably not really an URL then (may be a sentence missing a period).
                if (firstSlash == std::wstring_view::npos && text.length() > 64 &&
                    numberOfSpaces > 5)
                    {
                    return false;
                    }
                return true;
                }
            }

        return false;
        }

    //--------------------------------------------------
    bool is_file_address(std::wstring_view text)
        {
        constexpr size_t fileAddressMinLength{ 5 };
        constexpr size_t basicFileExtMinLength{ 3 };

        // Basic network and drive letter checks

        // UNC path
        if (text.length() >= basicFileExtMinLength && text[0] == L'\\' && text[1] == L'\\')
            {
            return true;
            }
        // Windows file path
        if (text.length() >= basicFileExtMinLength && static_cast<bool>(std::iswalpha(text[0])) &&
            text[1] == L':' && (text[2] == L'\\' || text[2] == L'/'))
            {
            return true;
            }

        // start looking at longer paths
        if (text.length() < fileAddressMinLength)
            {
            return false;
            }
        // protocols
        if (is_url(text))
            {
            return true;
            }
        // UNIX paths (including where the '/' at the front is missing
        if (text.length() >= basicFileExtMinLength && text[0] == L'/' &&
            string_util::strnchr(text.data() + 2, L'/', text.length() - 2) != nullptr)
            {
            return true;
            }
#if __cplusplus >= 202002L
        if (string_util::strnchr(text.data(), L'/', text.length()) != nullptr &&
            (text.starts_with(L"usr/") || text.starts_with(L"var/") || text.starts_with(L"tmp/") ||
             text.starts_with(L"sys/") || text.starts_with(L"srv/") || text.starts_with(L"mnt/") ||
             text.starts_with(L"etc/") || text.starts_with(L"dev/") || text.starts_with(L"bin/") ||
             text.starts_with(L"usr/") || text.starts_with(L"sbin/") ||
             text.starts_with(L"root/") || text.starts_with(L"proc/") ||
             text.starts_with(L"boot/") || text.starts_with(L"home/")))
            {
            return true;
            }
#endif
        // email address
        if (text.length() >= fileAddressMinLength)
            {
            const wchar_t* spaceInStr =
                string_util::strnchr(text.data() + 1, L' ', text.length() - 1);
            const wchar_t* atSign = string_util::strnchr(text.data() + 1, L'@', text.length() - 1);
            // no spaces and an '@' symbol
            if ((atSign != nullptr) && (spaceInStr == nullptr))
                {
                const wchar_t* dotSign =
                    string_util::strnchr(atSign, L'.', text.length() - (atSign - text.data()));
                if (dotSign != nullptr &&
                    static_cast<size_t>(dotSign - text.data()) < text.length() - 1)
                    {
                    return true;
                    }
                }
            }

        // If a longer string that did not start with a UNIX / or Windows drive letter
        // then this is likely not a file name. It could be filename, but even if it
        // ends with a valid file extension, it would more than likely be a filename
        // at the end of legit sentence if it's this long.
        constexpr size_t maxFileLength{ 128 };
        if (text.length() > maxFileLength)
            {
            return false;
            }

        // cut off possessive form
        if (text.length() >= basicFileExtMinLength && is_apostrophe(text[text.length() - 2]) &&
            string_util::is_either(text[text.length() - 1], L's', L'S'))
            {
            text.remove_suffix(2);
            }

        // we will start to review the extension now; if there is no period, then we are done.
        if (text.find(L'.') == std::wstring_view::npos)
            {
            return false;
            }
        // Large number of spaces? This is unlikely to be a filepath then.
        const size_t numberOfSpaces =
            std::count_if(text.cbegin(), text.cend(), [](const auto chr) { return chr == L' '; });
        if (numberOfSpaces >= 5)
            {
            return false;
            }

        // look at extensions now
        // 3-letter file name
        if (text.length() >= 4 && text[text.length() - 4] == L'.' &&
            static_cast<bool>(std::iswalpha(text[text.length() - 3])) &&
            static_cast<bool>(std::iswalpha(text[text.length() - 2])) &&
            static_cast<bool>(std::iswalpha(text[text.length() - 1])))
            {
            // Space followed by extension is probably not a file name,
            // but something referring to a file extension instead.
            if (text.length() >= 5 && text[text.length() - 5] == L' ')
                {
                return false;
                }
            // see if it is really a typo (missing space after a sentence).
            if (static_cast<bool>(std::iswupper(text[text.length() - 3])) &&
                !static_cast<bool>(std::iswupper(text[text.length() - 2])))
                {
                return false;
                }
            // see if a file filter/wildcard (e.g., "*.txt", "Rich Text Format (*.rtf)|*.rtf")
            // and not a file path
            if (text.length() >= fileAddressMinLength &&
                text[text.length() - fileAddressMinLength] == L'*')
                {
                return false;
                }
            return true;
            }
        // 4-letter (Microsoft XML-based) file name
        if (text.length() >= fileAddressMinLength &&
            text[text.length() - fileAddressMinLength] == L'.' &&
            static_cast<bool>(std::iswalpha(text[text.length() - (fileAddressMinLength - 1)])) &&
            static_cast<bool>(std::iswalpha(text[text.length() - (fileAddressMinLength - 2)])) &&
            static_cast<bool>(std::iswalpha(text[text.length() - (fileAddressMinLength - 3)])) &&
            string_util::is_either(text[text.length() - 1], L'x', L'X'))
            {
            if (text.length() >= 6 && text[text.length() - 6] == L' ')
                {
                return false;
                }

            // see if it is really a typo (missing space after a sentence)
            if (static_cast<bool>(
                    std::iswupper(text[text.length() - (fileAddressMinLength - 1)])) &&
                !static_cast<bool>(std::iswupper(text[text.length() - (fileAddressMinLength - 2)])))
                {
                return false;
                }
            if (text.length() >= 6 && text[text.length() - 6] == L'*')
                {
                return false;
                }
            return true;
            }
        // 4-letter extensions (HTML)
        if (text.length() >= fileAddressMinLength &&
            text[text.length() - fileAddressMinLength] == L'.' &&
            string_util::strnicmp(text.substr(text.length() - (fileAddressMinLength - 1)),
                                  std::wstring_view{ L"html" }) == 0)
            {
            if (text.length() >= 6 &&
                (text[text.length() - 6] == L'*' || text[text.length() - 6] == L' '))
                {
                return false;
                }
            return true;
            }
        // 2-letter extensions
        if (text.length() >= basicFileExtMinLength &&
            text[text.length() - basicFileExtMinLength] == L'.' &&
            // translation, source, and doc files
            (string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"mo" }) ==
                 0 ||
             string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"po" }) ==
                 0 ||
             string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"cs" }) ==
                 0 ||
             string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"js" }) ==
                 0 ||
             string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"db" }) ==
                 0 ||
             string_util::strnicmp(text.substr(text.length() - 2), std::wstring_view{ L"md" }) ==
                 0))
            {
            return true;
            }
        // tarball file name
        if (text.length() >= 7 && string_util::strnicmp(text.substr(text.length() - 7),
                                                        std::wstring_view{ L".tar." }) == 0)
            {
            // see if it is really a typo (missing space after a sentence).
            if (static_cast<bool>(std::iswupper(text[text.length() - 4])) &&
                !static_cast<bool>(std::iswupper(text[text.length() - 3])))
                {
                return false;
                }
            return true;
            }
        // C header/source files, which only have a letter in the extension,
        // but are common in documentation
        if (text.length() >= basicFileExtMinLength && text[text.length() - 2] == L'.' &&
            string_util::is_either(text[text.length() - 1], L'h', L'c'))
            {
            return true;
            }

        return false;
        }

    //--------------------------------------------------
    void remove_escaped_unicode_values(std::wstring& str)
        {
        for (size_t i = 0; i < str.length(); /* in loop*/)
            {
            // '\' that is not escaped by a proceeding '\'
            if (str[i] == L'\\' && (i == 0 || str[(i - 1)] != L'\\'))
                {
                // "\u266F" format
                if (i + 5 < str.length() && str[i + 1] == L'u' &&
                    string_util::is_hex_digit(str[i + 2]) &&
                    string_util::is_hex_digit(str[i + 3]) &&
                    string_util::is_hex_digit(str[i + 4]) && string_util::is_hex_digit(str[i + 5]))
                    {
                    str.replace(i, 6, 6, ' ');
                    i += 6;
                    continue;
                    }
                // "\U000FF254" format
                if (i + 9 < str.length() && str[i + 1] == L'U' &&
                    string_util::is_hex_digit(str[i + 2]) &&
                    string_util::is_hex_digit(str[i + 3]) &&
                    string_util::is_hex_digit(str[i + 4]) &&
                    string_util::is_hex_digit(str[i + 5]) &&
                    string_util::is_hex_digit(str[i + 6]) &&
                    string_util::is_hex_digit(str[i + 7]) &&
                    string_util::is_hex_digit(str[i + 8]) && string_util::is_hex_digit(str[i + 9]))
                    {
                    str.replace(i, 10, 10, ' ');
                    i += 10;
                    continue;
                    }
                // "\xFFFF" format (can be variable number of hex digits)
                if (i + 5 < str.length() && str[i + 1] == L'x' &&
                    // at least two hex digits needed
                    string_util::is_hex_digit(str[i + 2]) &&
                    string_util::is_hex_digit(str[i + 3]) &&
                    string_util::is_hex_digit(str[i + 4]) && string_util::is_hex_digit(str[i + 5]))
                    {
                    str.replace(i, 6, 6, ' ');
                    i += 6;
                    continue;
                    }
                // "\xFF" format (can be variable number of hex digits)
                if (i + 3 < str.length() && str[i + 1] == L'x' &&
                    // at least two hex digits needed
                    string_util::is_hex_digit(str[i + 2]) && string_util::is_hex_digit(str[i + 3]))
                    {
                    str.replace(i, 4, 4, ' ');
                    i += 4;
                    continue;
                    }
                }
            ++i;
            }
        }
    } // namespace i18n_string_util
