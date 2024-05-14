///////////////////////////////////////////////////////////////////////////////
// Name:        docx_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2023 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "docx_extract_text.h"

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    void word2007_extract_text::read_meta_data(const wchar_t* html_text, const size_t text_length)
        {
        static const std::wstring_view OFFICE_META(L"cp:coreProperties");
        static const std::wstring_view SUBJECT(L"dc:subject");
        static const std::wstring_view TITLE(L"dc:title");
        static const std::wstring_view DESCRIPTION(L"dc:description");
        static const std::wstring_view KEYWORDS(L"cp:keywords");
        static const std::wstring_view AUTHOR(L"dc:creator");

        // reset meta data from last call
        reset_meta_data();

        const wchar_t* const textEnd = html_text + text_length;

        const wchar_t* const officMetaStart = find_element(html_text, textEnd, OFFICE_META, true);
        if (!officMetaStart)
            {
            return;
            }

        html_extract_text parseHtml;

        m_title = read_element_as_string(officMetaStart, textEnd, TITLE);
        auto metaVal = parseHtml(m_title.c_str(), m_title.length(), true, false);
        if (metaVal)
            {
            m_title.assign(metaVal);
            }

        m_subject = read_element_as_string(officMetaStart, textEnd, SUBJECT);
        metaVal = parseHtml(m_subject.c_str(), m_subject.length(), true, false);
        if (metaVal)
            {
            m_subject.assign(metaVal);
            }

        m_description = read_element_as_string(officMetaStart, textEnd, DESCRIPTION);
        metaVal = parseHtml(m_description.c_str(), m_description.length(), true, false);
        if (metaVal)
            {
            m_description.assign(metaVal);
            }

        m_keywords = read_element_as_string(officMetaStart, textEnd, KEYWORDS);
        metaVal = parseHtml(m_keywords.c_str(), m_keywords.length(), true, false);
        if (metaVal)
            {
            m_keywords.assign(metaVal);
            }

        m_author = read_element_as_string(officMetaStart, textEnd, AUTHOR);
        metaVal = parseHtml(m_author.c_str(), m_author.length(), true, false);
        if (metaVal)
            {
            m_author.assign(metaVal);
            }
        }

    //------------------------------------------------------------------
    const wchar_t* word2007_extract_text::operator()(const wchar_t* html_text,
                                                     const size_t text_length)
        {
        clear_log();
        reset_meta_data();

        if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
            {
            clear();
            return nullptr;
            }
        assert(text_length == std::wcslen(html_text));

        allocate_text_buffer(text_length);

        // use "preserve spaces" logic in this XML
        m_is_in_preformatted_text_block_stack = 1;

        // find the first < and set up where we halt our searching
        const wchar_t* start = std::wcschr(html_text, L'<');
        const wchar_t* end = nullptr;
        const wchar_t* const endSentinel = html_text + text_length;

        bool insideOfTableCell = false;
        string_util::case_insensitive_wstring currentTag;
        while (start && (start < endSentinel))
            {
            currentTag.assign(get_element_name(start + 1, true));
            bool textSectionFound = false;
            // if it's a comment then look for matching comment ending sequence
            if (currentTag == L"!--")
                {
                end = std::wcsstr(start + 1, L"-->");
                if (!end)
                    {
                    break;
                    }
                end += 3; // -->
                }
            /* If it's an instruction command then skip it.
               Note that annotations for DOCX files are stored separately in "comments.xml",
               so we don't need to worry about these in here.*/
            else if (currentTag == L"w:instrText")
                {
                end = std::wcsstr(start + 1, L"</w:instrText>");
                if (!end)
                    {
                    break;
                    }
                end += 14;
                }
            // if it's an offset command then skip it
            else if (currentTag == L"wp:posOffset")
                {
                end = std::wcsstr(start + 1, L"</wp:posOffset>");
                if (!end)
                    {
                    break;
                    }
                end += 15;
                }
            else
                {
                // see if this should be treated as a new paragraph
                if (currentTag == L"w:p")
                    {
                    if (!m_preserve_text_table_layout || !insideOfTableCell)
                        {
                        add_character(L'\n');
                        add_character(L'\n');
                        }
                    }
                // if paragraph style indicates a list item
                else if (currentTag == L"w:pStyle")
                    {
                    if (read_attribute_as_string(start + 1, L"w:val", false, false) ==
                        L"ListParagraph")
                        {
                        add_character(L'\t');
                        }
                    }
                // or a tab
                else if (currentTag == L"w:tab")
                    {
                    add_character(L'\t');
                    }
                // hard breaks
                else if (currentTag == L"w:cr")
                    {
                    add_character(L'\n');
                    }
                else if (currentTag == L"w:br")
                    {
                    const std::wstring breakType =
                        read_attribute_as_string(start + 1, L"w:type", false, false);
                    if (breakType == L"page")
                        {
                        add_character(L'\f');
                        }
                    else
                        {
                        add_character(L'\n');
                        }
                    }
                // other type of page break
                else if (currentTag == L"w:pageBreakBefore")
                    {
                    add_character(L'\f');
                    }
                // or if it's aligned center or right
                else if (currentTag == L"w:jc")
                    {
                    const std::wstring alignment =
                        read_attribute_as_string(start + 1, L"w:val", false, false);
                    if (alignment == L"center" || alignment == L"right" || alignment == L"both" ||
                        alignment == L"list-tab")
                        {
                        add_character(L'\t');
                        }
                    }
                // or if it's indented
                else if (currentTag == L"w:ind")
                    {
                    const auto alignment = read_attribute_as_long(start + 1, L"w:left", false);
                    if (alignment > 0.0f)
                        {
                        add_character(L'\t');
                        }
                    }
                // tab over table cell and newline for table rows
                else if (currentTag == L"w:tr")
                    {
                    add_character(L'\n');
                    add_character(L'\n');
                    }
                else if (compare_element_case_sensitive(start + 1, L"w:tc", true))
                    {
                    add_character(L'\t');
                    insideOfTableCell = true;
                    }
                else if (compare_element_case_sensitive(start + 1, L"/w:tc", false))
                    {
                    insideOfTableCell = false;
                    }
                else if (compare_element_case_sensitive(start + 1, L"w:t", false))
                    {
                    textSectionFound = true;
                    }
                /* find the matching >, but watch out for an errant < also in case
                   the previous < wasn't terminated properly*/
                end = string_util::strcspn_pointer<wchar_t>(start + 1, L"<>", 2);
                if (!end)
                    {
                    break;
                    }
                /* If the < tag that we started from is not terminated then feed that in as
                   text instead of treating it like a valid HTML tag.
                   Not common, but it happens.*/
                else if (end[0] == L'<')
                    {
                    /* copy over the text from the unterminated < to the currently found
                        < (that we will start from in the next loop*/
                    parse_raw_text(start, end - start);
                    // set the starting point to the next < that we already found
                    start = end;
                    continue;
                    }
                // more normal behavior, where tag is properly terminated
                else
                    {
                    ++end;
                    }
                }
            // find the next starting tag
            start = std::wcschr(end, L'<');
            if (!start)
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
    } // namespace lily_of_the_valley
