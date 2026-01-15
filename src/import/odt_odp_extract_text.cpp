///////////////////////////////////////////////////////////////////////////////
// Name:        odt_odp_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "odt_odp_extract_text.h"
#include <algorithm>

namespace lily_of_the_valley
    {
    void odt_odp_extract_text::read_meta_data(const wchar_t* html_text, const size_t text_length)
        {
        // reset metadata from last call
        reset_meta_data();
        if (html_text == nullptr || text_length == 0)
            {
            return;
            }

        constexpr std::wstring_view OFFICE_META(L"office:meta");
        constexpr std::wstring_view SUBJECT(L"dc:subject");
        constexpr std::wstring_view TITLE(L"dc:title");
        constexpr std::wstring_view DESCRIPTION(L"dc:description");
        constexpr std::wstring_view KEYWORDS(L"meta:keyword");
        constexpr std::wstring_view AUTHOR(L"meta:initial-creator");
        const wchar_t* const textEnd = html_text + text_length;

        const wchar_t* const officeMetaStart = find_element(html_text, textEnd, OFFICE_META, true);
        if (officeMetaStart == nullptr)
            {
            return;
            }

        html_extract_text parseHtml;

        m_title = read_element_as_string(officeMetaStart, textEnd, TITLE);
        const auto* metaVal = parseHtml(m_title.c_str(), m_title.length(), true, false);
        if (metaVal != nullptr)
            {
            m_title.assign(metaVal);
            }

        m_subject = read_element_as_string(officeMetaStart, textEnd, SUBJECT);
        metaVal = parseHtml(m_subject.c_str(), m_subject.length(), true, false);
        if (metaVal != nullptr)
            {
            m_subject.assign(metaVal);
            }

        m_description = read_element_as_string(officeMetaStart, textEnd, DESCRIPTION);
        metaVal = parseHtml(m_description.c_str(), m_description.length(), true, false);
        if (metaVal != nullptr)
            {
            m_description.assign(metaVal);
            }

        m_keywords = read_element_as_string(officeMetaStart, textEnd, KEYWORDS);
        metaVal = parseHtml(m_keywords.c_str(), m_keywords.length(), true, false);
        if (metaVal != nullptr)
            {
            m_keywords.assign(metaVal);
            }

        m_author = read_element_as_string(officeMetaStart, textEnd, AUTHOR);
        metaVal = parseHtml(m_author.c_str(), m_author.length(), true, false);
        if (metaVal != nullptr)
            {
            m_author.assign(metaVal);
            }
        }

    const wchar_t* odt_odp_extract_text::operator()(const wchar_t* html_text,
                                                    const size_t text_length)
        {
        // reset metadata from last call
        reset_meta_data();

        constexpr static std::wstring_view OFFICE_ANNOTATION(L"office:annotation");
        constexpr static std::wstring_view OFFICE_ANNOTATION_OOO(L"officeooo:annotation");
        // text section tags
        constexpr static std::wstring_view TEXT_P(L"text:p");
        constexpr static std::wstring_view TEXT_P_END(L"</text:p>");
        constexpr static std::wstring_view TEXT_H(L"text:h");
        constexpr static std::wstring_view TEXT_H_END(L"</text:h>");
        constexpr static std::wstring_view TEXT_S(L"text:s");
        constexpr static std::wstring_view TEXT_C(L"text:c");
        // tables
        constexpr static std::wstring_view TABLE_ROW(L"table:table-row");
        // paragraph info
        constexpr static std::wstring_view TEXT_STYLE_NAME(L"text:style-name");

        clear_log();
        clear();
        if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
            {
            return nullptr;
            }
        assert(text_length <= std::wcslen(html_text));

        allocate_text_buffer(text_length);

        // use "preserve spaces" logic in this XML
        m_is_in_preformatted_text_block_stack = 1;

        // find the first < and set up where we halt our searching
        const wchar_t* start = std::wcschr(html_text, L'<');
        const wchar_t* end = nullptr;
        const wchar_t* const endSentinel = html_text + text_length;

        read_paragraph_styles(html_text, endSentinel);

        int textSectionDepth = 0;
        bool insideOfListItemOrTableCell = false;
        while ((start != nullptr) && (start < endSentinel))
            {
            bool textSectionFound = true;
            // if it's a comment then look for matching comment ending sequence
            if ((endSentinel - start) >= 4 && start[0] == L'<' && start[1] == L'!' &&
                start[2] == L'-' && start[3] == L'-')
                {
                end = std::wcsstr(start, L"-->");
                if (end == nullptr)
                    {
                    break;
                    }
                end += 3; // -->
                }
            // if it's an annotation, then skip it
            else if (compare_element_case_sensitive(start + 1, OFFICE_ANNOTATION, false))
                {
                end = find_closing_element(start, endSentinel, OFFICE_ANNOTATION);
                if (end == nullptr)
                    {
                    break;
                    }
                end = find_close_tag(end + 1);
                if (end == nullptr)
                    {
                    break;
                    }

                ++end;
                }
            else if (compare_element_case_sensitive(start + 1, OFFICE_ANNOTATION_OOO, false))
                {
                end = find_closing_element(start, endSentinel, OFFICE_ANNOTATION_OOO);
                if (end == nullptr)
                    {
                    break;
                    }
                end = find_close_tag(end + 1);
                if (end == nullptr)
                    {
                    break;
                    }

                ++end;
                }
            else if (compare_element_case_sensitive(start + 1, TEXT_S, true))
                {
                auto spacesCount = read_attribute_as_long(start + 1, TEXT_C, false);
                // if unreasonable number of spaces value is found, then use 10
                // (more than 10 spaces could mean that this tag is messed up)
                spacesCount = std::min<long>(spacesCount, 10);
                // if no space count specified, then default to one space
                fill_with_character(static_cast<size_t>(std::max(spacesCount, 1L)), L' ');
                end = find_close_tag(start + 1);
                if (end == nullptr)
                    {
                    break;
                    }

                ++end;
                }
            else
                {
                // see if this should be treated as a new paragraph
                if (compare_element_case_sensitive(start + 1, TEXT_P, true) ||
                    compare_element_case_sensitive(start + 1, TEXT_H, true))
                    {
                    const std::wstring styleName =
                        read_attribute_as_string(start + 1, TEXT_STYLE_NAME, false, false);
                    // page breaks
                    if (m_page_break_paragraph_styles.contains(styleName))
                        {
                        add_character(L'\f');
                        }
                    // if this paragraph's style is indented, then include a tab in front of it
                    if (!m_preserve_text_table_layout || !insideOfListItemOrTableCell)
                        {
                        if (m_indented_paragraph_styles.contains(styleName))
                            {
                            add_character(L'\n');
                            add_character(L'\n');
                            add_character(L'\t');
                            }
                        else
                            {
                            add_character(L'\n');
                            add_character(L'\n');
                            }
                        }
                    ++textSectionDepth;
                    }
                else if (compare_element_case_sensitive(start + 1, L"text:span", true))
                    {
                    ++textSectionDepth;
                    }
                // or end of a section
                else if (std::wcsncmp(start, TEXT_P_END.data(), TEXT_P_END.length()) == 0 ||
                         std::wcsncmp(start, TEXT_H_END.data(), TEXT_H_END.length()) == 0 ||
                         std::wcsncmp(start, L"</text:span>", 12) == 0)
                    {
                    --textSectionDepth;
                    }
                // beginning of a list item
                else if (compare_element_case_sensitive(start + 1, L"text:list-item", false))
                    {
                    add_character(L'\n');
                    add_character(L'\t');
                    insideOfListItemOrTableCell = true;
                    }
                // end of a list item
                else if (compare_element_case_sensitive(start + 1, L"/text:list-item", false))
                    {
                    insideOfListItemOrTableCell = false;
                    }
                // tab over table cell and newline for table rows
                else if (compare_element_case_sensitive(start + 1, TABLE_ROW, false))
                    {
                    add_character(L'\n');
                    add_character(L'\n');
                    }
                // tab over for a cell
                else if (compare_element_case_sensitive(start + 1, L"table:table-cell", false))
                    {
                    add_character(L'\t');
                    insideOfListItemOrTableCell = true;
                    }
                else if (compare_element_case_sensitive(start + 1, L"/table:table-cell", false))
                    {
                    insideOfListItemOrTableCell = false;
                    }
                // or a tab
                else if (compare_element_case_sensitive(start + 1, L"text:tab", true))
                    {
                    add_character(L'\t');
                    }
                // hard breaks
                else if (compare_element_case_sensitive(start + 1, L"text:line-break", true))
                    {
                    add_character(L'\n');
                    }
                // a new page (only in ODP files)
                else if (compare_element_case_sensitive(start + 1, L"draw:page", true))
                    {
                    add_character(L'\f');
                    }
                else
                    {
                    textSectionFound = textSectionDepth > 0;
                    }
                /* find the matching >, but watch out for an errant < also in case
                   the previous < wasn't terminated properly*/
                end = string_util::strcspn_pointer<wchar_t>(start + 1, L"<>");
                if (end == nullptr)
                    {
                    break;
                    }
                /* if the < tag that we started from is not terminated then feed that in as
                   text instead of treating it like a valid HTML tag.  Not common, but it happens.*/
                if (end[0] == L'<')
                    {
                    /* copy over the text from the unterminated < to the currently found
                       < (that we will start from in the next loop*/
                    parse_raw_text(start, end - start);
                    // set the starting point to the next < that we already found
                    start = end;
                    continue;
                    }
                // more normal behavior, where tag is properly terminated

                ++end;
                }
            // find the next starting tag
            start = std::wcschr(end, L'<');
            if (start == nullptr)
                {
                break;
                }
            // copy over the text between the tags
            if (textSectionFound)
                {
                parse_raw_text(end, start - end);
                }
            }

        return get_filtered_text();
        }

    void odt_odp_extract_text::read_paragraph_styles(const wchar_t* text, const wchar_t* textEnd)
        {
        // items for reading in general style information
        constexpr static std::wstring_view STYLE_STYLE_END(L"</style:style>");
        constexpr static std::wstring_view STYLE_NAME(L"style:name");
        constexpr static std::wstring_view ALIGNMENT(L"fo:text-align");
        constexpr static std::wstring_view BREAK_BEFORE(L"fo:break-before");
        constexpr static std::wstring_view MARGIN_ALIGNMENT(L"fo:margin-left");

        const wchar_t* const officeStyleStart =
            find_element(text, textEnd, L"office:automatic-styles", true);
        if (officeStyleStart == nullptr)
            {
            return;
            }
        const wchar_t* const officeStyleEnd =
            find_closing_element(officeStyleStart, textEnd, L"office:automatic-styles");
        if (officeStyleEnd != nullptr)
            {
            // go through all the styles in the office styles section
            const wchar_t* currentStyleStart =
                find_element(officeStyleStart, textEnd, L"style:style", true);
            while (currentStyleStart != nullptr)
                {
                const wchar_t* currentStyleEnd =
                    find_closing_element(currentStyleStart, textEnd, L"style:style");
                if ((currentStyleEnd != nullptr) && (currentStyleStart < currentStyleEnd))
                    {
                    // read in the name of the current style
                    const std::wstring styleName =
                        read_attribute_as_string(currentStyleStart, STYLE_NAME, false, true);
                    if (styleName.empty())
                        {
                        currentStyleStart = currentStyleEnd + STYLE_STYLE_END.length();
                        continue;
                        }
                    const wchar_t* paragraphProperties = find_element(
                        currentStyleStart, currentStyleEnd, L"style:paragraph-properties", true);
                    if ((paragraphProperties == nullptr) || paragraphProperties > currentStyleEnd)
                        {
                        currentStyleStart = currentStyleEnd + STYLE_STYLE_END.length();
                        continue;
                        }
                    currentStyleStart = paragraphProperties;
                    // read in the paragraph alignment and if it's indented then
                    // add it to our collection of indented styles
                    const std::wstring alignment =
                        read_attribute_as_string(currentStyleStart, ALIGNMENT, false, true);
                    if (alignment == L"center" || alignment == L"end")
                        {
                        m_indented_paragraph_styles.insert(styleName);
                        }
                    else
                        {
                        const auto alignmentValue =
                            read_attribute_as_long(currentStyleStart, MARGIN_ALIGNMENT, false);
                        if (alignmentValue > 0)
                            {
                            m_indented_paragraph_styles.insert(styleName);
                            }
                        }
                    // page breaks
                    const std::wstring pageBreak =
                        read_attribute_as_string(currentStyleStart, BREAK_BEFORE, false, true);
                    if (pageBreak == L"page")
                        {
                        m_page_break_paragraph_styles.insert(styleName);
                        }
                    }
                else
                    {
                    break;
                    }
                currentStyleStart = currentStyleEnd + STYLE_STYLE_END.length();
                }
            }
        }
    } // namespace lily_of_the_valley
