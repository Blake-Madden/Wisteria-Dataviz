/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __PPTX_TEXT_EXTRACT_H__
#define __PPTX_TEXT_EXTRACT_H__

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from a Microsoft® PowerPoint (2007+) slide.
        @par Example:
        @code
            // Assuming that the contents of "slide[PAGENUMBER].xml" from a PPTX
            // file is in a wchar_t* buffer named "fileContents" and
            // "fileSize" is set to the size of this xml file.
            // Note that you should use the specified character set
            // in the XML file (usually UTF-8) when loading this file into the wchar_t buffer.
            lily_of_the_valley::pptx_extract_text pptxExtract;
            pptxExtract(fileContents, fileSize);

            // The raw text from the file is now in a Unicode buffer.
            // This buffer can be accessed from get_filtered_text() and its length
            // from get_filtered_text_length(). Call these to copy the text into
            // a wide string.
            std::wstring fileText(pptxExtract.get_filtered_text(),
                                  pptxExtract.get_filtered_text_length());
        @endcode
        @date 2010*/
    class pptx_extract_text final : public html_extract_text
        {
      public:
        /** @brief Main interface for extracting plain text from a PowerPoint (2007+) slide.
            @param html_text The slide text to parse. Pass in the text from a
                "slide[PAGENUMBER].xml" file from a PPTX file. PPTX files are zip files,
                which can be opened by a library such as zlib.
            @param text_length The length of the text.
            @returns The parsed text from the slide.*/
        [[nodiscard]]
        const wchar_t*
        operator()(const wchar_t* html_text, const size_t text_length)
            {
            clear_log();
            clear();
            if (html_text == nullptr || html_text[0] == 0 || text_length == 0)
                {
                return nullptr;
                }
            assert(text_length == std::wcslen(html_text));

            allocate_text_buffer(text_length);

            // use "preserve spaces" logic in this XML
            m_is_in_preformatted_text_block_stack = 1;

            // find the first paragraph and set up where we halt our searching
            const wchar_t* const endSentinel = html_text + text_length;
            const wchar_t* start = find_element(html_text, endSentinel, L"a:p", false);
            const wchar_t* paragraphEnd = nullptr;
            const wchar_t* rowEnd = nullptr;
            const wchar_t* paragraphProperties = nullptr;
            const wchar_t* paragraphPropertiesEnd = nullptr;
            const wchar_t* textEnd = nullptr;
            const wchar_t* nextBreak = nullptr;
            bool isBulletedParagraph = true;

            while (start && (start < endSentinel))
                {
                bool isBulletedPreviousParagraph = isBulletedParagraph;
                isBulletedParagraph = true;
                paragraphEnd = find_closing_element(start, endSentinel, L"a:p");
                if (!paragraphEnd)
                    {
                    break;
                    }
                paragraphProperties = find_element(start, paragraphEnd, L"a:pPr", true);
                if (paragraphProperties)
                    {
                    paragraphPropertiesEnd =
                        find_closing_element(paragraphProperties, paragraphEnd, L"a:pPr");
                    if (paragraphPropertiesEnd)
                        {
                        // see if the paragraphs in here are bullet points or real lines of text.
                        const wchar_t* bulletNoneTag = find_element(
                            paragraphProperties, paragraphPropertiesEnd, L"a:buNone", true);
                        if (bulletNoneTag)
                            {
                            isBulletedParagraph = false;
                            }
                        }
                    // if the paragraph is indented, then put a tab in front of it.
                    const std::wstring levelDepth =
                        read_attribute_as_string(paragraphProperties, L"lvl", false, false);
                    if (!levelDepth.empty())
                        {
                        wchar_t* dummy = nullptr;
                        const double levelDepthValue = std::wcstod(levelDepth.c_str(), &dummy);
                        if (levelDepthValue >= 1)
                            {
                            add_character(L'\t');
                            }
                        }
                    }
                // if last paragraph as not a bullet point, but this one is then add an
                // extra newline between them to differentiate them
                if (isBulletedParagraph && !isBulletedPreviousParagraph)
                    {
                    add_character(L'\n');
                    }
                for (;;)
                    {
                    // go to the next row
                    nextBreak = find_element(start, paragraphEnd, L"a:br", true);
                    start = find_element(start, paragraphEnd, L"a:r", false);
                    if (!start || start > endSentinel)
                        {
                        // if no more runs in this paragraph,
                        // just see if there are any trailing breaks
                        if (nextBreak)
                            {
                            add_character(L'\n');
                            }
                        break;
                        }
                    rowEnd = find_closing_element(++start, paragraphEnd, L"a:r");
                    if (!rowEnd || rowEnd > endSentinel)
                        {
                        break;
                        }
                    // See if there is a break before this row.
                    // If so, then add some newlines to the output first.
                    if (nextBreak && nextBreak < start)
                        {
                        add_character(L'\n');
                        }
                    // Read the text section inside of it. If no valid text section, then
                    // just add a space (which an empty run implies) and skip to the next run.
                    start = find_element(start, rowEnd, L"a:t", false);
                    if (!start || start > endSentinel)
                        {
                        if (get_filtered_text_length() > 0 &&
                            !std::iswspace(get_filtered_text()[get_filtered_text_length() - 1]))
                            {
                            add_character(L' ');
                            }
                        start = rowEnd;
                        continue;
                        }
                    start = std::wcschr(start, L'>');
                    if (!start || start > endSentinel)
                        {
                        start = rowEnd;
                        continue;
                        }
                    textEnd = find_closing_element(++start, rowEnd, L"a:t");
                    if (!textEnd || textEnd > endSentinel)
                        {
                        start = rowEnd;
                        continue;
                        }
                    parse_raw_text(start, textEnd - start);
                    }
                // force bullet points to have two lines between them to show they are
                // independent of each other
                if (isBulletedParagraph)
                    {
                    add_character(L'\n');
                    add_character(L'\n');
                    }
                // ...otherwise, lines might actually be paragraphs split to fit inside of a box
                else
                    {
                    add_character(L'\n');
                    }
                // go to the next paragraph
                start = find_element(paragraphEnd, endSentinel, L"a:p", false);
                }

            return get_filtered_text();
            }

        /** @brief Reads the "docProps/core.xml" file and extracts meta data from
                the file (e.g., subject, title).
            @param html_text The "docProps/core.xml" text to extract text from.
                "docProps/core.xml" is extracted from a PPTX file.\n
                PPTX files are zip files, which can be opened by a library such as zlib.
            @param text_length The length of the "docProps/core.xml" stream.
            @sa get_title(), get_subject(), etc.*/
        void read_meta_data(const wchar_t* html_text, const size_t text_length)
            {
            // reset meta data from last call
            reset_meta_data();

            static const std::wstring_view OFFICE_META(L"cp:coreProperties");
            static const std::wstring_view SUBJECT(L"dc:subject");
            static const std::wstring_view TITLE(L"dc:title");
            static const std::wstring_view DESCRIPTION(L"dc:description");
            static const std::wstring_view KEYWORDS(L"cp:keywords");
            static const std::wstring_view AUTHOR(L"dc:creator");
            const wchar_t* const textEnd = html_text + text_length;

            const wchar_t* const officMetaStart =
                find_element(html_text, textEnd, OFFICE_META, true);
            if (!officMetaStart)
                {
                return;
                }
            m_title = read_element_as_string(officMetaStart, textEnd, TITLE);
            m_subject = read_element_as_string(officMetaStart, textEnd, SUBJECT);
            m_description = read_element_as_string(officMetaStart, textEnd, DESCRIPTION);
            m_keywords = read_element_as_string(officMetaStart, textEnd, KEYWORDS);
            m_author = read_element_as_string(officMetaStart, textEnd, AUTHOR);
            }
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__PPTX_TEXT_EXTRACT_H__
