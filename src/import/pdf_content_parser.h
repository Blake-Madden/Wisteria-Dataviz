/** @addtogroup Importing
    @brief Classes for importing data.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef PDF_CONTENT_PARSER_H
#define PDF_CONTENT_PARSER_H

#include "pdf_extract_text.h"
#include <map>
#include <memory>
#include <set>
#include <string>
#include <string_view>
#include <vector>

namespace lily_of_the_valley
    {
    /// @brief The resources (fonts, form XObjects) available to a content stream.
    struct pdf_page_resources
        {
        /// Resource name (e.g., "F1") -> font decoder.
        std::map<std::string, std::shared_ptr<pdf_font_decoder>, std::less<>> m_fonts;
        /// Resource name -> object number of a form XObject.
        std::map<std::string, long, std::less<>> m_xobjects;
        };

    /// @brief Parses page content streams, appending the extracted text to a buffer.
    class pdf_content_parser
        {
      public:
        /// @brief Constructs a parser that appends extracted text to @c text.
        /// @param document The document whose objects and streams are accessed.
        /// @param text The output buffer to append decoded text to.
        pdf_content_parser(pdf_document& document, std::wstring& text);

        /// @brief Extracts the text from one page.
        /// @param pageObject The page object (must have a dictionary).
        void parse_page(const pdf_object& pageObject);

      private:
        /// @returns @c true if @c character is a bullet-point glyph.
        [[nodiscard]]
        static bool is_bullet(wchar_t character);

        /// @returns The effective line height in text-space units.
        [[nodiscard]]
        double line_height() const;

        /// @brief Appends a line (or paragraph) break to the output.
        void add_newline(bool paragraphBreak);
        /// @brief Appends a space (if the output doesn't already end in whitespace).
        void add_space();
        /// @brief Appends decoded text to the output, normalizing ligatures and
        ///     formatting bullet points that start a line as list items.
        void add_text(const std::wstring& decodedText);
        /// @brief Handles a relative text-position move (Td/TD operators).
        /// @details A vertical move larger than 1.8x the line height is treated as
        ///     a paragraph break rather than a simple line break.
        void handle_relative_move(double moveX, double moveY);
        /// @brief Handles an absolute text-position move (Tm operator).
        /// @details Uses the same 1.8x line-height threshold as Td/TD to
        ///     distinguish line breaks from paragraph breaks.
        ///     If the move lands on the same line but far enough away, a space is
        ///     inserted so the two runs don't run together. This can happen with a
        ///     separate BT/Tm/Tj block placed well to the right, or diagonally for
        ///     rotated text.
        /// @returns @c true if a newline was written (i.e., @c newY landed on a
        ///     different line than the current position).
        bool handle_absolute_move(double newX, double newY);
        /// @brief Decodes and appends a shown string (the Tj, ', " operators).
        void show_string(const std::string& stringBytes, const pdf_font_decoder* currentFont);
        /// @brief Decodes and appends a TJ array (strings mixed with kerning values).
        /// @details A kerning value's displayed width is scaled by the current
        ///     horizontal scaling (Tz) before being checked against the word-gap threshold.
        void show_array(std::string_view arrayValue, const pdf_font_decoder* currentFont);

        /// @brief Loads the resources (fonts and form XObjects) available
        ///     to a content stream.
        [[nodiscard]]
        pdf_page_resources load_resources(std::string_view resourcesValue);

        /// @brief Finds a key in a page's dictionary, walking up the page tree
        ///     if the page inherits it.
        [[nodiscard]]
        std::string_view find_inherited_value(const pdf_object& pageObject,
                                              std::string_view keyName) const;

        /// @returns A page's (decoded) content, concatenated from its
        ///     `/Contents` stream(s).
        [[nodiscard]]
        std::string load_page_content(const pdf_object& pageObject) const;

        /// @brief Tokenizes a content stream, tracking the text operators.
        /// @details PDF content streams are stacks of operands followed by an operator
        ///     (e.g., `12 Tf` sets the font). This function accumulates numeric and
        ///     string operands until an operator consumes them.
        ///     @p depth limits recursion into form XObjects to prevent stack overflows
        ///     on malformed files.
        void parse_content(std::string_view content, const pdf_page_resources& resources,
                           int depth);

        pdf_document& m_document;          ///< Document being parsed.
        std::wstring& m_text;              ///< Output buffer (owned by the caller).
        std::set<long> m_visited_xobjects; ///< XObjects already recursed into (cycle guard).
        double m_currentX{ 0 };            ///< Current horizontal position in user-space.
        double m_currentY{ 0 };            ///< Current vertical position in user-space.
        double m_fontSize{ 12 };           ///< Current font size (from Tf operator).
        double m_fontScale{ 1 };           ///< Vertical scale factor from the text matrix.
        double m_leading{ 0 };             ///< Current leading (line spacing, from TL).
        double m_charSpacing{ 0 };         ///< Current character spacing (from Tc).
        double m_wordSpacing{ 0 };         ///< Current word spacing (from Tw).
        double m_horizScale{ 100 };        ///< Current horizontal scaling percent (from Tz).
        bool m_haveY{ false };             ///< Whether m_currentY has been initialized.
        bool m_atLineStart{ true };        ///< True when no glyphs emitted since the last newline.
        bool m_haveShownText{ false };     ///< Whether any glyph has been shown on this page yet.
        /// True between a `BT` operator and the first `Td`/`TD`/`Tm` after it (i.e.,
        /// while the text line matrix is still at its just-reset identity value).
        bool m_freshTextObject{ true };
        };
    } // namespace lily_of_the_valley

/** @}*/

#endif // PDF_CONTENT_PARSER_H
