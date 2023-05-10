/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __UNICODE_EXTRACT_TEXT_H__
#define __UNICODE_EXTRACT_TEXT_H__

#include "extract_text.h"
#include <memory>
#include <cstdint>

namespace lily_of_the_valley
    {
    /**@brief Class to convert a (16-bit) Unicode char* stream to a wchar_t* buffer.
       @details Works with both little and big-endian Unicode.
       @par Example:
       @code
        std::ifstream fs("C:\\Users\\Mistletoe\\Unicode.txt", std::ios::in|std::ios::binary|std::ios::ate);
        if (fs.is_open())
            {
            //read a unicode file into a char* buffer
            size_t fileSize = fs.tellg();
            char* fileContents = new char[fileSize+1];
            std::unique_ptr<char> deleteBuffer(fileContents);
            std::memset(fileContents, 0, fileSize+1);
            fs.seekg(0, std::ios::beg);
            fs.read(fileContents, fileSize);
            //convert the char* data into double-byte text
            lily_of_the_valley::unicode_extract_text UnicodeExtract;
            UnicodeExtract(fileContents, fileSize, true);
            //The text from the file is now in a fully Unicode buffer.
            //This buffer can be accessed from get_filtered_text() and its length
            //from get_filtered_text_length(). Call these to copy this text into
            //a wide string.
            std::wstring fileText(UnicodeExtract.get_filtered_text(),
                                  UnicodeExtract.get_filtered_text_length());
            }
        @endcode*/
    class unicode_extract_text final : public extract_text
        {
    public:
        /// @returns The UTF-8 leading signature.
        [[nodiscard]]
        static const char* get_bom_utf8() noexcept { return "\357\273\277"; }
        /// @returns The 16-bit Unicode byte order marker (little endian).
        [[nodiscard]]
        static const char* get_bom_utf16le() noexcept { return "\377\376"; }
        /// @returns The 16-bit Unicode byte order marker (big endian).
        [[nodiscard]]
        static const char* get_bom_utf16be() noexcept { return "\376\377"; }
        /** @returns Whether a text stream is Unicode
                (by seeing if it has a leading Byte Order Mark).
            @param text The text stream to analyze.*/
        [[nodiscard]]
        static bool is_unicode(const char* text) noexcept
            {
            return (std::strncmp(get_bom_utf16le(), text, 2) == 0 ||
                    std::strncmp(get_bom_utf16be(), text, 2) == 0);
            }
        /** @returns Whether a text stream is little endian Unicode (based on the BoM).
            @param text The text stream to analyze.*/
        [[nodiscard]]
        static bool is_little_endian(const char* text) noexcept
            { return (std::strncmp(get_bom_utf16le(), text, 2) == 0); }
        /** @returns Whether a text stream is big endian Unicode (based on the BoM).
            @param text The text stream to analyze.*/
        [[nodiscard]]
        static bool is_big_endian(const char* text) noexcept
            { return (std::strncmp(get_bom_utf16be(), text, 2) == 0); }
        /** @brief Main interface for taking a Unicode (char*) stream and converting
                it into a wide Unicode stream.
            @param unicodeText The char* (raw) stream of test.
            @param length: The length of the raw string.
            @param systemIsLittleEndian Whether the current system is little endian.
                This is used to determine whether the bits need to be flipped or not.
            @returns A wchar_t* pointer of the raw (char*) stream converted into a wchar_t buffer.
                Call get_filtered_text_length() to get the length of this buffer.*/
        const wchar_t* operator()(const char* unicodeText,
                               const size_t length,
                               const bool systemIsLittleEndian = true)
            {
            clear_log();
            if (!unicodeText || length == 0)
                {
                set_filtered_text_length(0);
                return nullptr;
                }
            // first see if the file is an even number of bytes (a unicode file would have to be)
            if (length%2 != 0)
                {
                log_message(L"Invalid Unicode stream, uneven number of bytes.");
                return nullptr;
                }
            // prepare the wide buffer
            if (!allocate_text_buffer((length/2)+1/*Null terminator*/))
                {
                set_filtered_text_length(0);
                return nullptr;
                }
            /* If unicode stream is the native endian format,
               then just copy it over into the wide buffer.*/
            if (std::strncmp(systemIsLittleEndian ? get_bom_utf16le() :
                             get_bom_utf16be(), unicodeText, 2) == 0)
                {
                // note that we skip the BoM
                convert_unicode_char_stream(get_writable_buffer(), unicodeText+2, length-2);
                }
            // ...otherwise, start flipping the bytes around to make it the native endian type
            else if (std::strncmp(systemIsLittleEndian ? get_bom_utf16be() :
                                  get_bom_utf16le(), unicodeText, 2) == 0)
                { get_flipped_buffer(get_writable_buffer(), unicodeText+2, length-2); }
            else
                { return nullptr; }
            set_filtered_text_length(std::wcslen(get_filtered_text()));

            return get_filtered_text();
            }
    private:
        /** @brief Flips the bytes of unicodeText and copy them into destination.
                Destination should be length+1 and zeroed out beforehand.
            @param[out] destination wchar_t buffer to write the flipped text.
            @param unicodeText The raw char* unicode stream.
            @param length The length of the raw char* stream.*/
        static void get_flipped_buffer(wchar_t* destination,
                                       const char* unicodeText,
                                       const size_t length)
            {
            auto flippedBuffer = std::make_unique<char[]>(length+1);
            // copy over the words (two byte pair), but flip the bytes around
            for (size_t i = 0; i < length; i += 2)
                {
                flippedBuffer[i] = unicodeText[i+1];
                flippedBuffer[i+1] = unicodeText[i];
                }
            convert_unicode_char_stream(destination,flippedBuffer.get(),length);
            }
        /** @brief Copies the bytes of unicodeText into destination.
                Destination should be length+1 and zeroed out beforehand.
            @param[out] destination wchar_t buffer to write the flipped text.
            @param unicodeText The raw char* unicode stream.
            @param length The length of the raw char* stream.*/
        static void convert_unicode_char_stream(wchar_t* destination,
                                                const char* unicodeText,
                                                size_t length) noexcept
            {
            const uint16_t* doubleByteBuffer = reinterpret_cast<const uint16_t*>(unicodeText);
            if (sizeof(wchar_t) == 2)
                {
                std::memcpy(destination, unicodeText,
                            length /* in this case, length is the number of bytes
                                      (not characters) to copy over*/);
                }
            // Text needs to be copied over letter by letter if sizeof(wchar_t) isn't 2.
            // (on UNIX it is usually 4 bytes).
            else
                {
                length /= 2;
                for (size_t i = 0; i < length; ++i)
                    { destination[i] = doubleByteBuffer[i]; }
                }
            }
        };
    }

/** @}*/

#endif //__UNICODE_EXTRACT_TEXT_H__
