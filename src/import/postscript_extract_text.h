/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __POSTSCRIPT_EXTRACT_TEXT_H__
#define __POSTSCRIPT_EXTRACT_TEXT_H__

#include "extract_text.h"

namespace lily_of_the_valley
    {
    /** @brief Class to extract text from a <b>Postscript</b> stream.
        @par Example:
        @code
        std::ifstream fs("C:\\users\\Mistletoe\\CheckupReport.ps",
                         std::ios::in|std::ios::binary|std::ios::ate);
        if (fs.is_open())
            {
            // read a Postscript file into a char* buffer
            size_t fileSize = fs.tellg();
            char* fileContents = new char[fileSize+1];
            std::unique_ptr<char> deleteBuffer(fileContents);
            std::memset(fileContents, 0, fileSize+1);
            fs.seekg(0, std::ios::beg);
            fs.read(fileContents, fileSize);
            // convert the Postscript data into raw text
            lily_of_the_valley::postscript_extract_text psExtract;
            psExtract(fileContents, fileSize);
            // The raw text from the file is now in a Unicode buffer.
            // This buffer can be accessed from get_filtered_text() and its length
            // from get_filtered_text_length(). Call these to copy the text into
            // a wide string.
            std::wstring fileText(psExtract.get_filtered_text(), psExtract.get_filtered_text_length());
            }
        @endcode*/
    class postscript_extract_text final : public extract_text
        {
    public:
        /** @brief Main interface for extracting plain text from a Postscript buffer.
            @details Supports Postscript up to version 2.
            @param ps_buffer The Postscript text to convert to plain text.
            @param text_length The length of the Postscript buffer.
            @returns A pointer to the parsed text, or null upon failure.\n
                Call get_filtered_text_length() to get the length of the parsed text.
            @throws postscript_header_not_found If an invalid document.
            @throws postscript_version_not_supported if document is a newer version of 
                Postscript that is not supported.*/
        [[nodiscard]] const wchar_t* operator()(const char* ps_buffer, const size_t text_length);
        /** @returns The title from the metadata file or stream.
            @note Must be called after calling operator() or read_meta_data()
                (depending on which parser you are using).*/
        [[nodiscard]] const std::string& get_title() const noexcept
            { return m_title; }
        /// @brief Exception thrown when a Postscript is missing its header
        ///     (more than likely an invalid Postscript file).
        class postscript_header_not_found : public std::exception {};
        /// @brief Exception thrown when an unsupported version of Postscript is being parsed.
        class postscript_version_not_supported : public std::exception {};
    private:
        std::string m_title;
        };
    }

/** @}*/

#endif //__POSTSCRIPT_EXTRACT_TEXT_H__
