/** @addtogroup Formatting
    @brief Classes for parsing and formatting results output.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __XML_FORMAT_H__
#define __XML_FORMAT_H__

#include "../import/html_encode.h"
#include "../math/mathematics.h"
#include "../util/donttranslate.h"
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <set>
#include <vector>
#include <wx/arrstr.h>
#include <wx/dir.h>
#include <wx/file.h>
#include <wx/filefn.h>
#include <wx/filename.h>
#include <wx/fontenum.h>
#include <wx/progdlg.h>
#include <wx/regex.h>
#include <wx/string.h>
#include <wx/tokenzr.h>
#include <wx/utils.h>
#include <wx/wx.h>

/// @brief Class to format XML files.
/// @private
class XmlFormat
    {
  public:
    [[nodiscard]]
    static wxString GetInclude()
        {
        return _DT(L"include");
        }

    [[nodiscard]]
    static wxString GetTrue()
        {
        return _DT(L"1");
        }

    [[nodiscard]]
    static wxString GetFalse()
        {
        return _DT(L"0");
        }

    [[nodiscard]]
    static wxString GetRed()
        {
        return _DT(L"red");
        }

    [[nodiscard]]
    static wxString GetGreen()
        {
        return _DT(L"green");
        }

    [[nodiscard]]
    static wxString GetBlue()
        {
        return _DT(L"blue");
        }

    [[nodiscard]]
    static wxString GetFontFamily()
        {
        return _DT(L"font-family");
        }

    [[nodiscard]]
    static wxString GetFontPointSize()
        {
        return _DT(L"font-point-size");
        }

    [[nodiscard]]
    static wxString GetFontStyle()
        {
        return _DT(L"font-style");
        }

    [[nodiscard]]
    static wxString GetFontWeight()
        {
        return _DT(L"font-weight");
        }

    [[nodiscard]]
    static wxString GetFontUnderline()
        {
        return _DT(L"font-underline");
        }

    [[nodiscard]]
    static wxString GetFontFaceName()
        {
        return _DT(L"font-face-name");
        }

    [[nodiscard]]
    static long GetLong(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                        const wxString& entityTag, const long defaultValue);
    [[nodiscard]]
    static double GetDouble(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                            const wxString& entityTag, const double defaultValue);
    [[nodiscard]]
    static wxString GetString(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                              const wxString& entityTag, const wxString& defaultValue = wxString{});
    /// loads values when the same tag is used more than once in a block of text
    static void GetStrings(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                           const wxString& entityTag, std::vector<wxString>& strings);
    static void
    GetStringsWithExtraInfo(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                            const wxString& entityTag, const wxString& attributeTag,
                            std::vector<comparable_first_pair<wxString, wxString>>& strings);
    [[nodiscard]]
    static bool GetBoolean(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                           const wxString& entityTag, const bool defaultValue);
    [[nodiscard]]
    static wxColour GetColor(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                             const wxString& entityTag, const wxColour& defaultValue);
    [[nodiscard]]
    static wxColour GetColorWithInclusionTag(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                             const wxString& entityTag, bool& include,
                                             const wxColour& defaultValue,
                                             const bool includeDefaultValue);
    [[nodiscard]]
    static wxFont GetFont(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                          const wxString& entityTag, const wxFont& defaultFont = wxNullFont);
    [[nodiscard]]
    static long GetAttributeLongValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                      const wchar_t* attributeTag);
    [[nodiscard]]
    static long GetAttributeLongValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                      const wxString& entityTag, const wxString& attributeTag,
                                      const long defaultValue);
    [[nodiscard]]
    static double GetAttributeDoubleValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                          const wxString& entityTag, const wxString& attributeTag,
                                          const double defaultValue);
    [[nodiscard]]
    static wxString GetAttributeString(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                       const wxString& attributeTag);
    [[nodiscard]]
    static wxString FormatFontAttributes(const wxFont& font);
    [[nodiscard]]
    static wxString FormatColorAttributes(const wxColour& color);
    [[nodiscard]]
    static wxString FormatColorAttributeWithInclusionTag(const wxColour& color, const bool include);

    template<typename T>
    static void FormatSection(wxString& text, const wxString& elementName, T value,
                              const size_t tabCount = 0)
        {
        text.clear();
        text.append(tabCount, L'\t').append(L"<").append(elementName).append(L">");
        text << value;
        text.append(L"</").append(elementName).append(L">\n");
        }

    [[nodiscard]]
    static wxString
    FormatSectionWithAttribute(const wxString& elementName, const wxString& elementValue,
                               const wxString& attributeName, const wxString& attributeValue,
                               const size_t tabCount = 0)
        {
        wxString textBuffer;
        lily_of_the_valley::html_encode_text enc;
        textBuffer.append(tabCount, L'\t')
            .append(L"<")
            .append(elementName)
            .append(L" ")
            .append(attributeName)
            .append(L"=\"")
            .append(enc({ attributeValue.wc_str(), attributeValue.length() }, false))
            .append(L"\">")
            .append(enc({ elementValue.wc_str(), elementValue.length() }, false))
            .append(L"</")
            .append(elementName)
            .append(L">\n");
        return textBuffer;
        }

    template<typename T>
    static void FormatAttribute(wxString& text, const wxString& elementName, const T value)
        {
        text.Clear();
        text.append(L" ").append(elementName).append(L"=\"");
        text << value;
        text.append(L"\"");
        }
    };

    /** @}*/

#endif //__XML_FORMAT_H__
