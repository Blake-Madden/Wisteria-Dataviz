///////////////////////////////////////////////////////////////////////////////
// Name:        xml_format.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// Licence:     3-Clause BSD licence
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "xml_format.h"
#include "wx/fontenum.h"

//------------------------------------------------
wxString XmlFormat::FormatColorAttributes(const wxColour& color)
    {
    wxString attributeText;
    attributeText.append(L" ")
        .append(RED_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(color.Red()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GREEN_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(color.Green()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(BLUE_TAG.data())
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
        .append(RED_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(color.Red()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(GREEN_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(color.Green()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(BLUE_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(color.Blue()))
        .append(L"\"");
    attributeText.append(L" ").append(INCLUDE_TAG.data()).append(L"=\"");
    attributeText += include ? wxString{ TRUE_TAG } : wxString{ FALSE_TAG };
    attributeText += (L"\"");

    return attributeText;
    }

//------------------------------------------------
wxString XmlFormat::FormatFontAttributes(const wxFont& font)
    {
    wxString attributeText;
    attributeText.append(L" ")
        .append(FONT_POINT_SIZE_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(font.GetPointSize()))
        .append(L"\"");
    attributeText.append(L" ")
        .append(FONT_STYLE_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetStyle())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(FONT_WEIGHT_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetWeight())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(FONT_UNDERLINE_TAG.data())
        .append(L"=\"")
        .append(std::to_wstring(static_cast<int>(font.GetUnderlined())))
        .append(L"\"");
    attributeText.append(L" ")
        .append(FONT_FACE_NAME_TAG.data())
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
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos = std::wcschr(currentPos, L'\"');
        if (currentPos != nullptr)
            {
            return std::wcstol(++currentPos, nullptr, 10);
            }

        return 0;
        }
    return 0;
    }

//------------------------------------------------
double XmlFormat::GetAttributeDoubleValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                          const wxString& entityTag, const wxString& attributeTag,
                                          const double defaultValue)
    {
    assert(sectionStart && sectionEnd && !entityTag.empty() && !attributeTag.empty() &&
           L"Invalid pointer passed to GetAttributeDoubleValue()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty() ||
        attributeTag.empty())
        {
        return 0.0;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    if (currentPos != nullptr)
        {
        const auto dValueStr =
            wxString(lily_of_the_valley::html_extract_text::read_attribute_as_string(
                currentPos, { attributeTag.data(), attributeTag.length() }, false, false));
        if (!dValueStr.empty())
            {
            double dValue{ 0 };
            if (dValueStr.ToCDouble(&dValue))
                {
                return dValue;
                }

            return defaultValue;
            }
        }
    return defaultValue;
    }

//------------------------------------------------
long XmlFormat::GetAttributeLongValue(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                      const wxString& entityTag, const wxString& attributeTag,
                                      const long defaultValue)
    {
    assert(sectionStart && sectionEnd && !entityTag.empty() && !attributeTag.empty() &&
           L"Invalid pointer passed to GetAttributeLongValue()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty() ||
        attributeTag.empty())
        {
        return 0;
        }

    const wchar_t* currentPos = std::wcsstr(sectionStart, entityTag);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos = std::wcsstr(currentPos, attributeTag);
        if ((currentPos != nullptr) && (currentPos < sectionEnd))
            {
            currentPos = std::wcschr(currentPos, L'\"');
            if (currentPos != nullptr)
                {
                return std::wcstol(++currentPos, nullptr, 10);
                }

            return defaultValue;
            }

        return defaultValue;
        }
    return defaultValue;
    }

//------------------------------------------------
wxString XmlFormat::GetAttributeString(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                                       const wxString& attributeTag)
    {
    assert(sectionStart && sectionEnd && !attributeTag.empty() &&
           L"Invalid pointer passed to GetAttributeString()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || attributeTag.empty())
        {
        return {};
        }

    const wchar_t* currentPos = std::wcsstr(sectionStart, attributeTag);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos = std::wcschr(currentPos, L'\"');
        if ((currentPos != nullptr) && (currentPos < sectionEnd))
            {
            ++currentPos;
            const wchar_t* endPos = std::wcschr(currentPos, L'\"');
            if ((endPos == nullptr) || (endPos > sectionEnd))
                {
                return {};
                }
            return { currentPos, static_cast<size_t>(endPos - currentPos) };
            }

        return {};
        }
    return {};
    }

//------------------------------------------------
wxFont XmlFormat::GetFont(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                          const wxString& entityTag, const wxFont& defaultFont /*= wxNullFont*/)
    {
    wxFont font =
        defaultFont.IsOk() ? defaultFont : wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT);

    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetFont()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return font;
        }

    // get the font
    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    wchar_t* dummy{ nullptr };
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        // point size
        wxString attribute{ FONT_POINT_SIZE_TAG };
        attribute += L"=\"";
        const wchar_t* pos = std::wcsstr(currentPos, attribute);
        if (pos != nullptr)
            {
            pos += attribute.length();
            const int pointSize = static_cast<int>(std::wcstol(pos, &dummy, 10));
            font.SetFractionalPointSize(
                (pointSize > 0) ?
                    pointSize :
                    wxSystemSettings::GetFont(wxSYS_DEFAULT_GUI_FONT).GetFractionalPointSize());
            }
        // style
        attribute = FONT_STYLE_TAG;
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos != nullptr)
            {
            pos += attribute.length();
            font.SetStyle(static_cast<wxFontStyle>(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // weight
        attribute = FONT_WEIGHT_TAG;
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos != nullptr)
            {
            pos += attribute.length();
            font.SetWeight(
                static_cast<wxFontWeight>(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // underlined
        attribute = FONT_UNDERLINE_TAG;
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos != nullptr)
            {
            pos += attribute.length();
            font.SetUnderlined(int_to_bool(static_cast<int>(std::wcstol(pos, &dummy, 10))));
            }
        // face name
        attribute = FONT_FACE_NAME_TAG;
        attribute += L"=\"";
        pos = std::wcsstr(currentPos, attribute);
        if (pos != nullptr)
            {
            pos += attribute.length();
            const wchar_t* posEnd = std::wcsstr(pos, L"\"");
            if ((posEnd != nullptr) && (pos < posEnd))
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

    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetColor()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return color;
        }

    // get the color
    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    int red = defaultValue.Red(), green = defaultValue.Green(), blue = defaultValue.Blue();
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        const wchar_t* entityEnd = std::wcschr(currentPos, L'>');
        if ((entityEnd == nullptr) || (entityEnd > sectionEnd))
            {
            return defaultValue;
            }
        currentPos += entityTag.length() + 1;
        wchar_t* dummy{ nullptr };
        // red
        wxString colorAttribute = wxString{ RED_TAG } + L"=\"";
        const wchar_t* colorPos = std::wcsstr(currentPos, colorAttribute);
        if ((colorPos != nullptr) && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            red = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // green
        colorAttribute = wxString{ GREEN_TAG } + L"=\"";
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if ((colorPos != nullptr) && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            green = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // blue
        colorAttribute = wxString{ BLUE_TAG } + L"=\"";
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if ((colorPos != nullptr) && (colorPos < entityEnd))
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

    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetColorWithInclusionTag()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return color;
        }

    // get the color
    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.data(), entityTag.length() }, true);
    int red = defaultValue.Red(), green = defaultValue.Green(), blue = defaultValue.Blue();
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        const wchar_t* entityEnd = std::wcschr(currentPos, L'>');
        if ((entityEnd == nullptr) || (entityEnd > sectionEnd))
            {
            return defaultValue;
            }
        currentPos += entityTag.length() + 1;
        // red
        wxString colorAttribute = wxString{ RED_TAG } + L"=\"";
        const wchar_t* colorPos = std::wcsstr(currentPos, colorAttribute);
        wchar_t* dummy{ nullptr };
        if ((colorPos != nullptr) && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            red = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // green
        colorAttribute = wxString{ GREEN_TAG } + L"=\"";
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if ((colorPos != nullptr) && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            green = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // blue
        colorAttribute = wxString{ BLUE_TAG } + L"=\"";
        colorPos = std::wcsstr(currentPos, colorAttribute);
        if ((colorPos != nullptr) && (colorPos < entityEnd))
            {
            colorPos += colorAttribute.length();
            blue = static_cast<int>(std::wcstol(colorPos, &dummy, 10));
            }
        // include
        const wxString includeAttribute = wxString{ INCLUDE_TAG } + L"=\"";
        const wchar_t* includePos = std::wcsstr(currentPos, includeAttribute);
        if ((includePos != nullptr) && (includePos < entityEnd))
            {
            includePos += includeAttribute.length();
            include = (std::wcsncmp(includePos, WTRUE_TAG.data(), WTRUE_TAG.length()) == 0);
            }
        }
    color.Set(red, green, blue);
    return color;
    }

//------------------------------------------------
bool XmlFormat::GetBoolean(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                           const wxString& entityTag, const bool defaultValue)
    {
    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetBoolean()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, '>');
        if ((currentPos != nullptr) && (endPos != nullptr) && (currentPos < endPos))
            {
            ++currentPos;
            // if an empty tag
            if (currentPos == endPos)
                {
                return defaultValue;
                }
            return (std::wcsncmp(currentPos, WTRUE_TAG.data(), WTRUE_TAG.length()) == 0);
            }

        wxMessageBox(
            wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
            _(L"Warning"), wxOK | wxICON_INFORMATION);
        return defaultValue;
        }
    return defaultValue;
    }

//------------------------------------------------
void XmlFormat::GetStringsWithExtraInfo(
    const wchar_t* sectionStart, const wchar_t* sectionEnd, const wxString& entityTag,
    const wxString& attributeTag, std::vector<comparable_first_pair<wxString, wxString>>& strings)
    {
    strings.clear();

    assert(sectionStart && sectionEnd && !entityTag.empty() && !attributeTag.empty());
    if ((sectionStart == nullptr) || (sectionEnd == nullptr) || entityTag.empty() ||
        attributeTag.empty())
        {
        return;
        }

    wxString value;

    const wchar_t* start{ sectionStart };
    while ((start != nullptr) && (start < sectionEnd))
        {
        start = lily_of_the_valley::html_extract_text::find_element(
            start, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
        if ((start != nullptr) && (start < sectionEnd))
            {
            const auto attributeStart = lily_of_the_valley::html_extract_text::read_attribute(
                start, { attributeTag.wc_str(), attributeTag.length() }, false, true);
            // move into the actual element data now
            start += entityTag.length() + 1;
            const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
                start, sectionEnd, { entityTag.wc_str(), entityTag.length() });
            start = std::wcschr(start, L'>');
            if ((start != nullptr) && (endPos != nullptr) && (start < endPos))
                {
                ++start;
                value.assign(start, (endPos - start));
                lily_of_the_valley::html_extract_text filterHtml;
                value = filterHtml(value.wc_str(), value.length(), true, false);
                const wxString attributeValue =
                    (attributeStart.first != nullptr) ?
                        wxString(
                            filterHtml(attributeStart.first, attributeStart.second, true, false)) :
                        wxString{};
                strings.emplace_back(value, attributeValue);
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

    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetStrings()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return;
        }

    wxString value;

    const wchar_t* start = sectionStart;
    while ((start != nullptr) && (start < sectionEnd))
        {
        start = lily_of_the_valley::html_extract_text::find_element(
            start, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
        if ((start != nullptr) && (start < sectionEnd))
            {
            start += entityTag.length() + 1;
            const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
                start, sectionEnd, { entityTag.wc_str(), entityTag.length() });
            start = std::wcschr(start, L'>');
            if ((start != nullptr) && (endPos != nullptr) && (start < endPos))
                {
                ++start;
                value.assign(start, (endPos - start));
                lily_of_the_valley::html_extract_text filterHtml;
                value = filterHtml(value.wc_str(), value.length(), true, false);
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
    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetString()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if ((currentPos != nullptr) && (endPos != nullptr) && (currentPos < endPos))
            {
            wxString value;
            ++currentPos;
            value.assign(currentPos, (endPos - currentPos));
            lily_of_the_valley::html_extract_text filterHtml;
            value = filterHtml(value.wc_str(), value.length(), true, true);
            return value;
            }
        wxMessageBox(
            wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
            _(L"Warning"), wxOK | wxICON_INFORMATION);
        return defaultValue;
        }
    return defaultValue;
    }

//------------------------------------------------
long XmlFormat::GetLong(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                        const wxString& entityTag, const long defaultValue)
    {
    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetLong()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if ((currentPos != nullptr) && (endPos != nullptr) && (currentPos < endPos))
            {
            ++currentPos;
            // if an empty tag
            if (currentPos == endPos)
                {
                return defaultValue;
                }

            return std::wcstol(currentPos, nullptr, 10);
            }

        wxMessageBox(
            wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
            _(L"Warning"), wxOK | wxICON_INFORMATION);
        return defaultValue;
        }
    return defaultValue;
    }

//------------------------------------------------
double XmlFormat::GetDouble(const wchar_t* sectionStart, const wchar_t* sectionEnd,
                            const wxString& entityTag, const double defaultValue)
    {
    assert(sectionStart && sectionEnd && !entityTag.empty() &&
           L"Invalid pointer passed to GetDouble()!");
    if (sectionStart == nullptr || sectionEnd == nullptr || entityTag.empty())
        {
        return defaultValue;
        }

    const wchar_t* currentPos = lily_of_the_valley::html_extract_text::find_element(
        sectionStart, sectionEnd, { entityTag.wc_str(), entityTag.length() }, true);
    if ((currentPos != nullptr) && (currentPos < sectionEnd))
        {
        currentPos += entityTag.length() + 1;
        const wchar_t* endPos = lily_of_the_valley::html_extract_text::find_closing_element(
            currentPos, sectionEnd, { entityTag.wc_str(), entityTag.length() });
        currentPos = std::wcschr(currentPos, L'>');
        if ((currentPos != nullptr) && (endPos != nullptr) && (currentPos < endPos))
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

        wxMessageBox(
            wxString::Format(_(L"Warning: %s section of file is ill-formatted."), entityTag),
            _(L"Warning"), wxOK | wxICON_INFORMATION);
        return defaultValue;
        }
    return defaultValue;
    }
