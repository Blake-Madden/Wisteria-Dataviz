/** @addtogroup Exporting
    @brief Classes for formatting and exporting text.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
* @{*/

#ifndef __DOCX_EXTRACTOR_H__
#define __DOCX_EXTRACTOR_H__

#include "html_extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from a Microsoft® Word (2007+) stream
            (specifically, the "document.xml" file).
        @par Example:
        @code
            // Assuming that the contents of "document.xml" from a DOCX file is in a
            // wchar_t* buffer named "fileContents" and "fileSize" is set to the size
            // of this document.xml. Note that you should use the specified character set
            // in the XML file (usually UTF-8) when loading this file into the wchar_t buffer.
            lily_of_the_valley::word2007_extract_text docxExtract;
            docxExtract(fileContents, fileSize);

            // The raw text from the file is now in a Unicode buffer.
            // This buffer can be accessed from get_filtered_text() and its length
            // from get_filtered_text_length(). Call these to copy the text into
            // a wide string.
            std::wstring fileText(docxExtract.get_filtered_text(),
                                  docxExtract.get_filtered_text_length());
        @endcode
        @date 2010*/
    class word2007_extract_text final : public html_extract_text
        {
      public:
        word2007_extract_text() noexcept : m_preserve_text_table_layout(false) {}

        /** @brief Specifies how to import tables.
            @param preserve Set to @c true to import tables as tab-delimited cells of text.\n
                Set to @c false to simply import each cell as a separate paragraph,
                the tabbed structure of the rows may be lost.\n
                The default is to import cells as separate paragraphs.*/
        void preserve_text_table_layout(const bool preserve) noexcept
            {
            m_preserve_text_table_layout = preserve;
            }

        /** @brief Main interface for extracting plain text from a DOCX stream.
            @param html_text The "document.xml" text to extract text from.\n
                Note that "document.xml" should be extracted from a DOCX file
                (from the "word" folder).\n
                DOCX files are zip files, which can be opened by a library such as zlib.
            @param text_length The length of the "document.xml" stream.
            @returns A pointer to the parsed text, or null upon failure.\n
                Call get_filtered_text_length() to get the length of the parsed text.*/
        const wchar_t* operator()(const wchar_t* html_text, const size_t text_length);
        /** @brief Reads the "docProps/core.xml" file and extracts meta data from the file
                (e.g., subject, title).
            @param html_text The "docProps/core.xml" text to extract text from.\n
                "docProps/core.xml" is extracted from a DOCX file.\n
                DOCX files are zip files, which can be opened by a library such as zlib.
            @param text_length The length of the "docProps/core.xml" stream.
            @sa get_title(), get_subject(), etc.*/
        void read_meta_data(const wchar_t* html_text, const size_t text_length);

      private:
        bool m_preserve_text_table_layout{ false };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__DOCX_EXTRACTOR_H__
