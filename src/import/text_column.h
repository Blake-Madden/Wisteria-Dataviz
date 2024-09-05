/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2023
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef __TEXT_COLUMN_H__
#define __TEXT_COLUMN_H__

#include "text_functional.h"

namespace lily_of_the_valley
    {
    /// @brief Base interface for column parsing.
    class text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_parser(const bool read_text) noexcept : m_read_text(read_text) {}

        /** @brief Reads the next column(s) from the current row of text.
            @param text The current row of text to parse.
            @returns The text after the column(s) that were just read.*/
        [[nodiscard]]
        virtual const wchar_t*
        operator()(const wchar_t* text) const noexcept
            {
            if (text == nullptr || text[0] == 0)
                {
                return nullptr;
                }
            int32_t quoteStack{ 0 };
            while (text[0] && !is_eol(text[0]) &&
                   // allow delims if inside of set of double quotes
                   (!is_delimiter(text[0]) || (quoteStack % 2 != 0)))
                {
                if (text[0] == L'\"')
                    {
                    // just step over doubled up (i.e., escaped) quote
                    if (text[1] == L'\"')
                        {
                        text += 2;
                        }
                    else
                        {
                        ++text;
                        ++quoteStack;
                        }
                    }
                else
                    {
                    ++text;
                    }
                }
            return text;
            }

        /** @brief Indicates whether this parser is actually reading anything
                back into the parent parser.
            @details If this is @c false, then the parser is simply skipping this column.
            @returns @c true if text that is parsed for this column is fed back
                into the parent parser.*/
        [[nodiscard]]
        inline bool is_reading_text() const noexcept
            {
            return m_read_text;
            }

        /** @brief Determines if a character is a delimiter.
            @param character The character to review.
            @returns @c true if @c character is a delimiter.*/
        [[nodiscard]]
        virtual bool is_delimiter(const wchar_t character) const noexcept = 0;

      protected:
        /// @brief Functor for determining and end-of-line.
        is_end_of_line is_eol;

      private:
        bool m_read_text{ true };
        };

    /// @brief Parser that "slices" the text into fixed-width columns.
    class text_column_fixed_parser : public text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param width The number of characters in the fixed-width column.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_fixed_parser(const size_t width, const bool read_text = true)
            : text_column_parser(read_text), m_width(width)
            {
            }

        /// @brief Feeds text in from the current row to scan to the next column.
        /// @param text The text from the current row to splice.
        /// @returns The position in the current row of text after the next column.
        [[nodiscard]]
        inline const wchar_t*
        operator()(const wchar_t* text) const noexcept final
            {
            if (text == nullptr || text[0] == 0)
                {
                return nullptr;
                }
            for (size_t i = 0; i < m_width; ++i, ++text)
                {
                // stop if we hit the end of the buffer or an end of line
                if (text[0] == 0 || is_eol(text[0]))
                    {
                    break;
                    }
                }
            return text;
            }

        /** @private
            @details This is just here to fulfill the pure virtual contract, not actually used.*/
        [[nodiscard]]
        inline bool is_delimiter([[maybe_unused]] const wchar_t character) const noexcept final
            {
            return false;
            }

      private:
        size_t m_width{ 0 };
        };

    /// @brief Parser that finds a space, semicolon, or comma as a column delimiter.
    class text_column_standard_delimiter_parser : public text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_standard_delimiter_parser(const bool read_text = true) noexcept
            : text_column_parser(read_text)
            {
            }

        /** @brief Determines if a character is a delimiter.
            @param character The character to review.
            @returns @c true if @c character is a delimiter.*/
        [[nodiscard]]
        inline bool is_delimiter(const wchar_t character) const noexcept final
            {
            return is_delim(character);
            }

      private:
        is_standard_delimiters is_delim;
        };

    /// @brief Parser that finds a single-character delimiter.
    class text_column_delimited_character_parser : public text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param delim The delimiter to determine where a column ends.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_delimited_character_parser(const wchar_t delim,
                                                        const bool read_text = true) noexcept
            : text_column_parser(read_text), is_delim(delim)
            {
            }

        /** @brief Determines if a character is a delimiter.
            @param character The character to review.
            @returns @c true if @c character is a delimiter.*/
        [[nodiscard]]
        inline bool is_delimiter(const wchar_t character) const noexcept final
            {
            return is_delim(character);
            }

      private:
        is_single_delimiter is_delim;
        };

    /// @brief Parser that finds a single-character delimiter from a set of possible characters.
    class text_column_delimited_multiple_character_parser : public text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param delims The delimiters to determine where a column ends.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_delimited_multiple_character_parser(const wchar_t* delims,
                                                                 const bool read_text = true)
            : text_column_parser(read_text), is_delim(delims)
            {
            }

        /** @brief Determines if a character is a delimiter.
            @param character The character to review.
            @returns @c true if @c character is a delimiter.*/
        [[nodiscard]]
        inline bool is_delimiter(const wchar_t character) const noexcept final
            {
            return is_delim(character);
            }

      private:
        is_one_of_multiple_delimiters is_delim;
        };

    /// @brief Parser that simply reads to the end of the line (reads in each line as one string).
    class text_column_to_eol_parser : public text_column_parser
        {
      public:
        /// @brief Constructor.
        /// @param read_text Set to @c true to feed the parsed text back to the parent parser,
        ///     or @c false to simply skip to the end of line.
        explicit text_column_to_eol_parser(const bool read_text = true) noexcept
            : text_column_parser(read_text)
            {
            }

        /** @brief Reads the current row of text to the end of line.
            @param text The current row of text to read.
            @returns The start of the next row.*/
        [[nodiscard]]
        inline const wchar_t*
        operator()(const wchar_t* text) const noexcept final
            {
            if (text == nullptr || text[0] == 0)
                {
                return nullptr;
                }
            while (text[0] && !is_eol(text[0]))
                {
                ++text;
                }
            return text;
            }

        /** @private
            @details This is just here to fulfill the pure virtual contract, not actually used.*/
        [[nodiscard]]
        inline bool is_delimiter([[maybe_unused]] const wchar_t character) const noexcept final
            {
            return false;
            }
        };

    /// @brief Class representing a column of text.
    template<typename Tparser>
    class text_column
        {
      public:
        /// @brief Constructor.
        /// @param parser The parser to use to determine how to read a column.
        /// @param repeatCount The number of times this column type should be read consecutively.
        ///     Set to `std::nullopt` to repeat the column until the end of line is reached.
        explicit text_column(const Tparser& parser,
                             std::optional<size_t> repeatCount = std::nullopt) noexcept
            : m_parser(parser), m_repeat_count(repeatCount)
            {
            }

        /// @private
        text_column() noexcept = default;

        /** @brief Reads the next column(s) from the current row of text.
            @param text The current row of text to parse.
            @returns The text after the column(s) that were just read.*/
        [[nodiscard]]
        inline const wchar_t* read(const wchar_t* text)
            {
            const wchar_t* end = m_parser(text);
            /* if this is null then we are at the end of the file,
               so just copy over the rest of the file into this column*/
            if (end == nullptr)
                {
                return nullptr;
                }
            return end;
            }

        /// @returns The number of times this column should repeat in the current row.
        /// @note This is optional and if not specified then this column
        ///     definition should be repeated
        ///     by the parser until the end of line is reached.
        inline std::optional<size_t> get_repeat_count() const noexcept { return m_repeat_count; }

        /// @returns The parser used by this column.
        inline Tparser& get_parser() noexcept { return m_parser; }

      private:
        Tparser m_parser;
        std::optional<size_t> m_repeat_count{ std::nullopt };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif //__TEXT_COLUMN_H__
