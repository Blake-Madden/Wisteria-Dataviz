/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2025
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __EXTRACT_TEXT_H__
#define __EXTRACT_TEXT_H__

#include "../math/safe_math.h"
#include "../util/char_traits.h"
#include "../util/donttranslate.h"
#include "../util/string_util.h"
#include <exception>
#include <string_view>
#include <unordered_map>

namespace lily_of_the_valley
    {
    /** @brief Base class for text extraction (from marked-up formats).
        @details Derived classes will usually implement operator() to parse a formatted
            buffer and then store the raw text in here.*/
    class extract_text
        {
      public:
        /// @brief Default constructor.
        extract_text() = default;
        /// @private
        extract_text(const extract_text&) = delete;
        /// @private
        void operator=(const extract_text&) = delete;

        /// @returns The text that has been extracted from the formatted stream.
        [[nodiscard]]
        const wchar_t* get_filtered_text() const
            {
            return m_text_buffer.c_str();
            }

        /// @returns The length of the parsed text.
        [[nodiscard]]
        size_t get_filtered_text_length() const
            {
            return m_text_buffer.length();
            }

        /// @returns A report of any issues with the last read block.
        [[nodiscard]]
        const std::wstring& get_log() const noexcept
            {
            return m_log;
            }

        /// @returns The internal buffer that stores the parsed text.
        /// @warning This grants direct access to the buffer and should generally be used
        ///     for operating with the results after a parse is complete.\n
        ///     For example, this can be useful for performing a `std::move` to copy the results
        ///     to another string efficiently.
        [[nodiscard]]
        std::wstring& get_filtered_buffer() noexcept
            {
            return m_text_buffer;
            }

        /** @brief Sets the string used to separate the messages in the log report.
            @details By default, messages are separated by newlines, so call this to separate them
                by something like commas (if you are needing a single-line report).
            @param separator The separator character to use.*/
        void set_log_message_separator(const std::wstring& separator)
            {
            m_log_message_separator = separator;
            }
#ifndef __UNITTEST
      protected:
#endif
        /** @brief Allocates (or resizes) the buffer to hold the parsed text.
            @details This must be called before using add_character() or add_characters().
                If the new size is smaller than the current size, then the size remains the same.
            @param text_length The new size of the buffer.*/
        void allocate_text_buffer(const size_t text_length)
            {
            m_text_buffer.clear();
            m_text_buffer.reserve(text_length);
            }

        /** @brief Adds a character to the parsed buffer.
            @param character The character to add.*/
        void add_character(const wchar_t character) { m_text_buffer.append(1, character); }

        /** @brief Adds a character to the parsed buffer a specified number of times.
            @param repeatCount The number of times to add the character.
            @param character The character to add.*/
        void fill_with_character(const size_t repeatCount, const wchar_t character)
            {
            m_text_buffer.append(repeatCount, character);
            }

        /** @brief Adds a string to the parsed buffer.
            @param characters The string to add.*/
        void add_characters(const std::wstring_view characters)
            {
            m_text_buffer.append(characters);
            }

        /** @brief Trims any trailing whitespace from the end of the parsed text.*/
        void trim() { string_util::rtrim(m_text_buffer); }

        /** @brief Clears any text.*/
        void clear() { m_text_buffer.clear(); }

        /** @brief Resizes the buffer.
            @param newSize The new size of the buffer.*/
        void resize_buffer(const size_t newSize) { m_text_buffer.resize(newSize); }

        /// @brief Empties the log of any previous parsing issues.
        void clear_log() { m_log.clear(); }

        /** @brief Adds a message to the report logging system.
            @param message The message to log.*/
        void log_message(const std::wstring& message) const
            {
            if (m_log.empty())
                {
                m_log.append(message);
                } // first message won't need a separator in front of it
            else
                {
                m_log.append(m_log_message_separator + message);
                }
            }

      private:
        mutable std::wstring m_log;
        std::wstring m_log_message_separator{ L"\n" };
        // data
        std::wstring m_text_buffer;
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__EXTRACT_TEXT_H__
