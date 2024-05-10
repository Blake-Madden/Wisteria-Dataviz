/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2023
    @copyright Oleander Software, Ltd.
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __ODF_TEXT_EXTRACT_H__
#define __ODF_TEXT_EXTRACT_H__

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from a Open Document Text or
            Open Document Presentation stream (specifically, the "content.xml" file).
        @par Example:
        @code
            // Assuming that the contents of "content.xml" from an ODT file is in a
            // wchar_t* buffer named "fileContents" and "fileSize" is set to the size
            // of this content.xml. Note that you should use the specified character set
            // in the XML file (usually UTF-8) when loading this file into the wchar_t buffer.
            lily_of_the_valley::odt_odp_extract_text odtExtract;
            odtExtract(fileContents, fileSize);

            // The raw text from the file is now in a Unicode buffer.
            // This buffer can be accessed from get_filtered_text() and its length
            // from get_filtered_text_length(). Call these to copy the text into
            // a wide string.
            std::wstring fileText(odtExtract.get_filtered_text(),
                                  odtExtract.get_filtered_text_length());
        @endcode*/
    class odt_odp_extract_text final : public html_extract_text
        {
      public:
        odt_odp_extract_text() noexcept : m_preserve_text_table_layout(false) {}

        /** @brief Specifies how to import tables.
            @param preserve Set to @c true to not import text cells as separate paragraphs,
                but instead as cells of text with tabs between them.
                Set to @c false to simply import each cell as a separate paragraph,
                the tabbed structure of the rows will be lost.*/
        void preserve_text_table_layout(const bool preserve) noexcept
            {
            m_preserve_text_table_layout = preserve;
            }

        /** @brief Main interface for extracting plain text from a "content.xml" buffer.
            @param html_text The "content.xml" text to extract text from.\n
                "content.xml" is extracted from an ODT file.\n
                ODT files are zip files, which can be opened by a library such as zlib.
            @param text_length The length of the "content.xml" stream.
            @returns The plain text from the ODT stream.*/
        [[nodiscard]]
        const wchar_t*
        operator()(const wchar_t* html_text, const size_t text_length);
        /** @brief Reads the "meta.xml" file and extracts meta data from
                the file (e.g., subject, title).
            @param html_text The "meta.xml" text to extract text from.\n
                "meta.xml" is extracted from an ODT file.\n
                ODT files are zip files, which can be opened by a library such as zlib.
            @param text_length The length of the "meta.xml" stream.
            @sa get_title(), get_subject(), etc.*/
        void read_meta_data(const wchar_t* html_text, const size_t text_length);

      private:
        /// Reads in all of the paragraph styles, looking for any styles
        /// that involve text alignment.
        void read_paragraph_styles(const wchar_t* text, const wchar_t* textEnd);
        std::set<std::wstring> m_indented_paragraph_styles;
        std::set<std::wstring> m_page_break_paragraph_styles;

        bool m_preserve_text_table_layout{ false };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__ODF_TEXT_EXTRACT_H__
