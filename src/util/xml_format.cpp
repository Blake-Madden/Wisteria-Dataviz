///////////////////////////////////////////////////////////////////////////////
// Name:        xml_format.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "xml_format.h"

using namespace lily_of_the_valley;

//------------------------------------------------
wxString XmlFormat::FormatColorAttributes(const wxColour& color)
    {
    wxString attributeText;
    attributeText.append(L" ")
        .append(GetRed())
        .append(L"=\"")
        .append(std::to_wstring(color.Red()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetGreen())
        .append(L"=\"")
        .append(std::to_wstring(color.Green()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetBlue())
        .append(L"=\"")
        .append(std::to_wstring(color.Blue()))
        .append(L"\"");
    return attributeText;
    }

//------------------------------------------------
wxString XmlFormat::FormatColorAttributeWithInclusionTag(const wxColour& color, const bool include)
    {
    wxString attributeText;
    attributeText.append(L" ")
        .append(GetRed())
        .append(L"=\"")
        .append(std::to_wstring(color.Red()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetGreen())
        .append(L"=\"")
        .append(std::to_wstring(color.Green()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetBlue())
        .append(L"=\"")
        .append(std::to_wstring(color.Blue()))
        .append(L"\"");
    attributeText.append(L" ").append(GetInclude()).append(L"=\"");
    attributeText += include ? GetTrue() : GetFalse();
    attributeText += (L"\"");

    return attributeText;
    }

//------------------------------------------------
wxString XmlFormat::FormatFontAttributes(const wxFont& font)
    {
    wxString attributeText;
    attributeText.append(L" ")
        .append(GetFontPointSize())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetPointSize())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetFontStyle())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetStyle())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetFontWeight())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetWeight())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetFontUnderline())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetUnderlined())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GetFontFaceName())
        .append(L"=\"")
        .append(font.GetFaceName())
        .append(L"\"");
    return attributeText;
    }

//------------------------------------------------
long XmlFormat::GetAttributeLongValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                      const wchar_t* attributeTag)
    {
    assert(sectionStart && sectionEnd && attributeTag &&
           L"Invalid pointer passed to GetAttributeLongValue()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || attributeTag == nullptr)
        {
        return 0;
        }

    const wchar_t* currentPos = std::wcsstr(sectionStart, attributeTag);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos = std::wcschr(currentPos, L'\"');
        if (currentPos)
            {
            return std::wcstol(++currentPos, nullptr, 10);
            }
        else
            {
            return 0;
            }
        }
    return 0;
    }

//------------------------------------------------
double XmlFormat::GetAttributeDoubleValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                          const wxString& entityTag, const wxString& attributeTag,
                                          const double defaultValue)
    {
    assert(sectionStart && sectionEnd && entityTag.length() && attributeTag.length() &&
           L"Invalid pointer passed to GetAttributeDoubleValue()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty() ||
        attributeTag.empty())
        {
        return 0.0;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    if (currentPos)
        {
        const auto dValueStr =
            wxString(lily_of_the_valley::html_extract_text::read_attribute_as_string(
                currentPos, { attributeTag.data(), attributeTag.length() }, false, false));
        if (dValueStr.length())
            {
            double dValue{ 0 };
            if (dValueStr.ToCDouble(&dValue))
                {
                return dValue;
                }
            else
                {
                return defaultValue;
                }
            }
        }
    return defaultValue;
    }

//------------------------------------------------
long XmlFormat::GetAttributeLongValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                      const wxString& entityTag, const wxString& attributeTag,
                                      const long defaultValue)
    {
    assert(sectionStart && sectionEnd && entityTag.length() && attributeTag.length() &&
           L"Invalid pointer passed to GetAttributeLongValue()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty() ||
        attributeTag.empty())
        {
        return 0;
        }

    const wchar_t* currentPos = std::wcsstr(sectionStart, entityTag);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos = std::wcsstr(currentPos, attributeTag);
        if (currentPos && (currentPos < sectionEnd))
            {
            currentPos = std::wcschr(currentPos, L'\"');
            if (currentPos)
                {
                return std::wcstol(++currentPos, nullptr, 10);
                }
            else
                {
                return defaultValue;
                }
            }
        else
            {
            return defaultValue;
            }
        }
    return defaultValue;
    }

//------------------------------------------------
wxString XmlFormat::GetAttributeString(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                       const wxString& attributeTag)
    {
    assert(sectionStart && sectionEnd && attributeTag.length() &&
           L"Invalid pointer passed to GetAttributeString()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || attributeTag.empty())
        {
        return wxString{};
        }

    const wchar_t* currentPos = std::wcsstr(sectionStart, attributeTag);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos = std::wcschr(currentPos, L'\"');
        if (currentPos && (currentPos < sectionEnd))
            {
            ++currentPos;
            const wchar_t* endPos = std::wcschr(currentPos, L'\"');
            if (!endPos || (endPos > sectionEnd))
                {
                return wxString{};
                }
            return wxString(currentPos, (endPos - currentPos));
            }
        else
            {
            return wxString{};
            }
        }
    return wxString{};
    }

//------------------------------------------------
wxFont XmlFormat::GetFont(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                          const wxString& entityTag, const wxFont& defaultFont /*= wxNullFont*/)
    {
    wxFont font =
        defaultFont.IsOk() ? defaultFont : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetFont()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return font;
        }

    // get the font
    const wchar_t* currentPos = html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    wchar_t* dummy{ nullptr };
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        // point size
        wxString attribute = GetFontPointSize();
        attribute += L"=\"";
        const wchar_t* pos = std::wcsstr(currentPos, attribute);
        if (pos)
            {
            pos += attribute.length();
            const int pointSize = static_cast<int>(std::wcstol(pos, &dummy, 10));
            font.SetPointSize((pointSize > 0) ?
                                  pointSize :
                                  wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetPointSize());
            }
        // style
        attribute = GetFontStyle();
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos)
            {
            pos += attribute.length();
            font.SetStyle(static_cast<wxFontStyle>(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // weight
        attribute = GetFontWeight();
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos)
            {
            pos += attribute.length();
            font.SetWeight(
                static_cast<wxFontWeight>(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // underlined
        attribute = GetFontUnderline();
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos)
            {
            pos += attribute.length();
            font.SetUnderlined(int_to_bool(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // face name
        attribute = GetFontFaceName();
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos)
            {
            pos += attribute.length();
            const wchar_t* posEnd = std::wcsstr(pos, L"\"");
            if (posEnd && (pos < posEnd))
                {
                wxString faceName(pos, posEnd - pos);
                // fall back to system font if unknown font name
                if (!wxFontEnumerator::IsValidFacename(faceName))
                    {
                    faceName = wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFaceName();
                    }
                font.SetFaceName(faceName);
                }
            }
        }
    return font;
    }

//------------------------------------------------
wxColour XmlFormat::GetColor(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                             const wxString& entityTag, const wxColour& defaultValue)
    {
    wxColour color = defaultValue;

    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetColor()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return color;
        }

    // get the color
    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    int red = defaultValue.Red(), green = defaultValue.Green(), blue = defaultValue.Blue();
    if (currentPos && (currentPos < sectionEnd))
        {
        const wchar_t* entityEnd = std::wcschr(currentPos, L'>');
        if (!entityEnd || (entityEnd > sectionEnd))
            {
            return defaultValue;
            }
        currentPos += entityTag.length() + 1;
        wchar_t* dummy{ nullptr };
        // red
        wxString colorAttribute = wxString::Format(L"%s=\"", GetRed());
        const wchar_t* colorPos = std::wcsstr(currentPos, colorAttribute);
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            red = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // green
        colorAttribute = wxString::Format(L"%s=\"", GetGreen());
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            green = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // blue
        colorAttribute = wxString::Format(L"%s=\"", GetBlue());
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            blue = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        }
    color.Set(red, green, blue);
    return color;
    }

//------------------------------------------------
wxColour XmlFormat::GetColorWithInclusionTag(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                             const wxString& entityTag, bool& include,
                                             const wxColour& defaultValue,
                                             const bool includeDefaultValue)
    {
    wxColour color = defaultValue;
    include = includeDefaultValue;

    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetColorWithInclusionTag()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return color;
        }

    // get the color
    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    int red = defaultValue.Red(), green = defaultValue.Green(), blue = defaultValue.Blue();
    if (currentPos && (currentPos < sectionEnd))
        {
        const wchar_t* entityEnd = std::wcschr(currentPos, L'>');
        if (!entityEnd || (entityEnd > sectionEnd))
            {
            return defaultValue;
            }
        currentPos += entityTag.length() + 1;
        // red
        wxString colorAttribute = wxString::Format(L"%s=\"", GetRed());
        const wchar_t* colorPos = std::wcsstr(currentPos, colorAttribute);
        wchar_t* dummy{ nullptr };
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            red = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // green
        colorAttribute = wxString::Format(L"%s=\"", GetGreen());
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            green = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // blue
        colorAttribute = wxString::Format(L"%s=\"", GetBlue());
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if (colorPos && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            blue = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // include
        wxString includeAttribute = wxString::Format(L"%s=\"", GetInclude());
        const wchar_t* includePos = std::wcsstr(currentPos, includeAttribute);
        if (includePos && (includePos < entityEnd))
            {
            includePos += includeAttribute.length();
            include = (std::wcsncmp(includePos, GetTrue().c_str(), GetTrue().length()) == 0);
            }
        }
    color.Set(red, green, blue);
    return color;
    }

//------------------------------------------------
bool XmlFormat::GetBoolean(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                           const wxString& entityTag, const bool defaultValue)
    {
    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetBoolean()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, '>');
        if (currentPos && endPos && (currentPos < endPos))
            {
            ++currentPos;
            // if an empty tag
            if (currentPos == endPos)
                {
                return defaultValue;
                }
            return (std::wcsncmp(currentPos, GetTrue().c_str(), GetTrue().length()) == 0) ? true :
                                                                                            false;
            }
        else
            {
            wxMessageBox(
                wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
                _(L"Warning"), wxOK | wxICON_INFORMATION);
            return defaultValue;
            }
        }
    return defaultValue;
    }

//------------------------------------------------
void XmlFormat::GetStringsWithExtraInfo(
    const wchar_t* sectionStart, const wchar_t* sectionEnd, const wxString& entityTag,
    const wxString& attributeTag, std::vector<comparable_first_pair<wxString, wxString>>& strings)
    {
    strings.clear();

    assert(sectionStart && sectionEnd && entityTag.length() && attributeTag.length());
    if (!sectionStart || !sectionEnd || entityTag.empty() || attributeTag.empty())
        {
        return;
        }

    wxString value;

    const wchar_t* start{ sectionStart };
    while (start && (start < sectionEnd))
        {
        start = lily_of_the_valley::html_extract_text::find_element(
            start, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
        if (start && (start < sectionEnd))
            {
            const auto attributeStart = lily_of_the_valley::html_extract_text::read_attribute(
                start, { attributeTag.wc_str(), attributeTag.length() }, false, true);
            // move into the actual element data now
            start += entityTag.length() + 1;
            const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
                start, sectionEnd, { entityTag.wc_str(), entityTag.length() });
            start = std::wcschr(start, L'>');
            if (start && endPos && (start < endPos))
                {
                ++start;
                value.assign(start, (endPos - start));
                lily_of_the_valley::html_extract_text filter_html;
                value = filter_html(value.wc_str(), value.length(), true, false);
                const wxString attributeValue =
                    attributeStart.first ?
                        wxString(
                            filter_html(attributeStart.first, attributeStart.second, true, false)) :
                        wxString{};
                strings.push_back(comparable_first_pair<wxString, wxString>(value, attributeValue));
                }
            else
                {
                wxMessageBox(wxString::Format(_(L"Warning: %s section of file is ill-formatted."),
                                              entityTag),
                             _(L"Warning"), wxOK | wxICON_INFORMATION);
                return;
                }
            }
        else
            {
            return;
            }
        }
    }

//------------------------------------------------
void XmlFormat::GetStrings(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                           const wxString& entityTag, std::vector<wxString>& strings)
    {
    strings.clear();

    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetStrings()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return;
        }

    wxString value;

    const wchar_t* start = sectionStart;
    while (start && (start < sectionEnd))
        {
        start = lily_of_the_valley::html_extract_text::find_element(
            start, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
        if (start && (start < sectionEnd))
            {
            start += entityTag.length() + 1;
            const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
                start, sectionEnd, { entityTag.wc_str(), entityTag.length() });
            start = std::wcschr(start, L'>');
            if (start && endPos && (start < endPos))
                {
                ++start;
                value.assign(start, (endPos - start));
                lily_of_the_valley::html_extract_text filter_html;
                value = filter_html(value.wc_str(), value.length(), true, false);
                strings.push_back(value);
                }
            else
                {
                wxMessageBox(wxString::Format(_(L"Warning: %s section of file is ill-formatted."),
                                              entityTag),
                             _(L"Warning"), wxOK | wxICON_INFORMATION);
                return;
                }
            }
        else
            {
            return;
            }
        }
    }

//------------------------------------------------
wxString XmlFormat::GetString(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                              const wxString& entityTag,
                              const wxString& defaultValue /*= wxString{}*/)
    {
    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetString()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    wxString value;

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if (currentPos && endPos && (currentPos < endPos))
            {
            ++currentPos;
            value.assign(currentPos, (endPos - currentPos));
            lily_of_the_valley::html_extract_text filter_html;
            value = filter_html(value.wc_str(), value.length(), true, true);
            return value;
            }
        else
            {
            wxMessageBox(
                wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
                _(L"Warning"), wxOK | wxICON_INFORMATION);
            return defaultValue;
            }
        }
    return defaultValue;
    }

//------------------------------------------------
long XmlFormat::GetLong(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                        const wxString& entityTag, const long defaultValue)
    {
    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetLong()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if (currentPos && endPos && (currentPos < endPos))
            {
            ++currentPos;
            // if an empty tag
            if (currentPos == endPos)
                {
                return defaultValue;
                }

            return std::wcstol(currentPos, nullptr, 10);
            }
        else
            {
            wxMessageBox(
                wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
                _(L"Warning"), wxOK | wxICON_INFORMATION);
            return defaultValue;
            }
        }
    return defaultValue;
    }

//------------------------------------------------
double XmlFormat::GetDouble(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                            const wxString& entityTag, const double defaultValue)
    {
    assert(sectionStart && sectionEnd && entityTag.length() &&
           L"Invalid pointer passed to GetDouble()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if (currentPos && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if (currentPos && endPos && (currentPos < endPos))
            {
            ++currentPos;
            // if an empty tag
            if (currentPos == endPos)
                {
                return defaultValue;
                }
            wchar_t* dummy = nullptr;
            return std::wcstod(currentPos, &dummy);
            }
        else
            {
            wxMessageBox(
                wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
                _(L"Warning"), wxOK | wxICON_INFORMATION);
            return defaultValue;
            }
        }
    return defaultValue;
    }
