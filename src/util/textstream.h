/** @addtogroup Utilities
    @brief Utility classes.
    @date 2005-2022
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __TEXTRESTREAM_H__
#define __TEXTRESTREAM_H__

#include <memory>
#include <wx/wx.h>
#include <wx/string.h>
#include <wx/regex.h>
#include <wx/file.h>
#include <wx/string.h>
#include <wx/filename.h>
#include <vector>
#include "memorymappedfile.h"
#include "../i18n-check/src/string_util.h"
#include "../i18n-check/src/donttranslate.h"
#include "../utfcpp/source/utf8.h"
#include "../math/mathematics.h"
#include "../import/html_extract_text.h"
#include "../import/unicode_extract_text.h"

namespace Wisteria
    {
    /// @brief Class for streaming (and manipulating) text.
    class TextStream
        {
    public:
        /** Copies a broken UTF-8 stream (where it contains incorrect UTF-8 sequences,
                like regular extended ASCII characters) into a Unicode buffer,
                where the incorrect sequences are removed.
           @param[out] dest The buffer to copy the corrected text into.
           @param destLength the length of the destination buffer.
           @param text The UTF-8 stream to read.
           @param length The length of the UTF-8 stream.
           @returns Whether corrected text could be copied to the buffer.*/
        static bool FixBrokenUtf8Stream(wchar_t* dest, size_t destLength,
                                        const char* text, size_t length);
        /** @brief Converts a `char*` stream into a Unicode stream.
            @param[out] dest The buffer to copy the Unicode text into.
            @param destLength the length of the destination buffer.
            @param text The `char*` stream to read.
            @param length The length of the `char*` stream.
            @param srcCharSet The character set to convert the text from.
            @returns Whether the text could be copied to the buffer.*/
        static bool CharStreamToUnicode(wchar_t* dest, const size_t destLength,
                            const char* text, const size_t length,
                            wxString srcCharSet = wxEmptyString);
        /** @brief Converts a `char*` stream into a Unicode stream.
            @param text The `char*` stream to read.
            @param length The length of the `char*` stream.
            @param srcCharSet The character set to convert the text from.
            @returns The converted text.*/
        static wxString CharStreamToUnicode(const char* text,
                           size_t length,
                           wxString srcCharSet = wxEmptyString);
        /** @brief Converts a `char*` stream that may contain embedded nulls into a Unicode stream.
            @param text The `char*` stream to read.
            @param length The length of the `char*` stream.
            @param srcCharSet The character set to convert the text from.
            @returns The converted text.*/
        static wxString CharStreamWithEmbeddedNullsToUnicode(const char* text,
                           const size_t length,
                           wxString srcCharSet = wxEmptyString);
        /** @brief Reads a file into a string buffer.
            @details This supports UTF-8 and double-byte Unicode files. For HTML and XML files,
                can also support reading the character set from the file's definition and
                will use that.
            @warning This function assumes that the file may not exist and will prompt
                the client for the correct path if not found. Therefore, it should not be
                used when possible client interaction is not expected.
            @param[in,out] filePath The file path to read. This may be altered if the original
                path didn't exist and the client is prompted to enter a new one.
            @param[out] fileText The buffer to write the file's content to.
            @param srcCharSet The (optional) character set to convert the file from.
            @returns @c true if the file was read successfully.*/
        static bool ReadFile(wxString& filePath, wxString& fileText,
                             const wxString& srcCharSet = wxEmptyString);
        };
    }

/** @}*/

#endif //__TEXTRESTREAM_H__
