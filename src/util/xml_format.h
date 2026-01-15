/** @addtogroup Formatting
    @brief Classes for parsing and formatting results output.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef XML_FORMAT_H
#define XML_FORMAT_H

#include "../import/html_encode.h"
#include "../math/mathematics.h"
#include "../util/donttranslate.h"
#include <string_view>
#include <vector>
#include <wx/string.h>
#include <wx/wx.h>

/// @brief Class to format XML files.
/// @private
class XmlFormat
    {
  public:
    [[deprecated("Use INCLUDE_TAG instead")]] [[nodiscard]]
    static wxString GetInclude()
        {
        return _DT(L"include");
        }

    [[deprecated("Use TRUE_TAG or WTRUE_TAG instead")]] [[nodiscard]]
    static wxString GetTrue()
        {
        return _DT(L"1");
        }

    [[deprecated("Use FALSE_TAG instead")]] [[nodiscard]]
    static wxString GetFalse()
        {
        return _DT(L"0");
        }

    [[deprecated("Use RED_TAG instead")]] [[nodiscard]]
    static wxString GetRed()
        {
        return _DT(L"red");
        }

    [[deprecated("Use GREEN_TAG instead")]] [[nodiscard]]
    static wxString GetGreen()
        {
        return _DT(L"green");
        }

    [[deprecated("Use BLUE_TAG instead")]] [[nodiscard]]
    static wxString GetBlue()
        {
        return _DT(L"blue");
        }

    [[deprecated("Use FONT_FAMILY_TAG instead")]] [[nodiscard]]
    static wxString GetFontFamily()
        {
        return _DT(L"font-family");
        }

    [[deprecated("Use FONT_POINT_SIZE_TAG instead")]] [[nodiscard]]
    static wxString GetFontPointSize()
        {
        return _DT(L"font-point-size");
        }

    [[deprecated("Use FONT_STYLE_TAG instead")]] [[nodiscard]]
    static wxString GetFontStyle()
        {
        return _DT(L"font-style");
        }

    [[deprecated("Use FONT_WEIGHT_TAG instead")]] [[nodiscard]]
    static wxString GetFontWeight()
        {
        return _DT(L"font-weight");
        }

    [[deprecated("Use FONT_UNDERLINE_TAG instead")]] [[nodiscard]]
    static wxString GetFontUnderline()
        {
        return _DT(L"font-underline");
        }

    [[deprecated("Use FONT_FACE_NAME_TAG instead")]] [[nodiscard]]
    static wxString GetFontFaceName()
        {
        return _DT(L"font-face-name");
        }

    inline constexpr static std::string_view INCLUDE_TAG = _DT("include");
    inline constexpr static std::string_view TRUE_TAG = "1";
    inline constexpr static std::wstring_view WTRUE_TAG = L"1";
    inline constexpr static std::string_view FALSE_TAG = "0";
    inline constexpr static std::string_view RED_TAG = _DT("red");
    inline constexpr static std::string_view GREEN_TAG = _DT("green");
    inline constexpr static std::string_view BLUE_TAG = _DT("blue");
    inline constexpr static std::string_view FONT_FAMILY_TAG = "font-family";
    inline constexpr static std::string_view FONT_POINT_SIZE_TAG = "font-point-size";
    inline constexpr static std::string_view FONT_STYLE_TAG = "font-style";
    inline constexpr static std::string_view FONT_WEIGHT_TAG = "font-weight";
    inline constexpr static std::string_view FONT_UNDERLINE_TAG = "font-underline";
    inline constexpr static std::string_view FONT_FACE_NAME_TAG = "font-face-name";

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
        const lily_of_the_valley::html_encode_text enc;
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

#endif // XML_FORMAT_H
