///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_content_parser.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_content_parser.h"
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cwctype>
#include <tuple>
#include <utility>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    pdf_content_parser::pdf_content_parser(pdf_document& document, std::wstring& text)
        : m_document(document), m_text(text)
        {
        }

    //------------------------------------------------------------------
    void pdf_content_parser::parse_page(const pdf_object& pageObject)
        {
        const std::string pageContent{ load_page_content(pageObject) };
        if (pageContent.empty())
            {
            return;
            }
        const pdf_page_resources resources{ load_resources(
            find_inherited_value(pageObject, "Resources")) };
        // reset the text-positioning state for the new page
        m_currentY = 0;
        m_haveY = false;
        m_fontSize = 12;
        m_fontScale = 1;
        m_leading = 0;
        m_atLineStart = true;
        m_haveShownText = false;
        m_freshTextObject = true;
        parse_content(pageContent, resources, 0);
        // End the page on its own line so that whatever comes next (the next page's
        // text, or nothing) never runs directly into this page's last glyph.
        // This matters most for trailing content like a page footer. Combined with the
        // newline inserted between pages, this yields a blank line at each page break.
        if (m_haveShownText)
            {
            add_newline(false);
            }
        }

    //------------------------------------------------------------------
    bool pdf_content_parser::is_bullet(const wchar_t character)
        {
        return (character == 0x2022 /* bullet */ || character == 0x2023 || character == 0x25AA ||
                character == 0x25CF || character == 0x25E6 || character == 0x25CB ||
                character == 0x00B7 || character == 0x2043 || character == 0x2219);
        }

    //------------------------------------------------------------------
    double pdf_content_parser::line_height() const
        {
        return std::max({ m_leading, m_fontSize, 4.0 });
        }

    //------------------------------------------------------------------
    void pdf_content_parser::add_newline(const bool paragraphBreak)
        {
        // remove trailing spaces from the line that is ending
        while (!m_text.empty() && (m_text.back() == L' ' || m_text.back() == L'\t'))
            {
            m_text.pop_back();
            }
        if (!m_text.empty() && m_text.back() != L'\n')
            {
            m_text += L'\n';
            }
        if (paragraphBreak && m_text.length() >= 2 &&
            m_text.compare(m_text.length() - 2, 2, L"\n\n") != 0)
            {
            m_text += L'\n';
            }
        m_atLineStart = true;
        }

    //------------------------------------------------------------------
    void pdf_content_parser::add_space()
        {
        if (!m_text.empty() && !std::iswspace(m_text.back()))
            {
            m_text += L' ';
            }
        }

    //------------------------------------------------------------------
    void pdf_content_parser::add_text(const std::wstring& decodedText)
        {
        for (const wchar_t curChar : decodedText)
            {
            wchar_t character{ curChar };
            // symbol fonts in the F0xx Private Use Area (e.g., Wingdings bullets)
            if (character == 0xF0A7 || character == 0xF06C || character == 0xF06E)
                {
                character = 0x25AA; // small black square
                }
            else if (character == 0xF0B7 || character == 0xF0A8)
                {
                character = 0x2022; // bullet
                }
            else if (character == 0xF0FC)
                {
                character = 0x2713; // check mark
                }
            if (character == 0 ||
                (character < 32 && character != L'\t' && character != L'\n' && character != L'\r'))
                {
                continue;
                }
            if (m_atLineStart)
                {
                if (character == L' ' || character == L'\t')
                    {
                    // wait for the line's first real glyph
                    continue;
                    }
                m_haveShownText = true;
                // a line starting with a bullet glyph is a list item;
                // format it like the other extract_text parsers do (tab + bullet)
                if (is_bullet(character))
                    {
                    m_text += L'\t';
                    m_text += L'\x2022';
                    m_atLineStart = false;
                    continue;
                    }
                m_atLineStart = false;
                }
            // expand ligatures
            switch (character)
                {
            case 0xFB00:
                m_text += L"ff";
                break;
            case 0xFB01:
                m_text += L"fi";
                break;
            case 0xFB02:
                m_text += L"fl";
                break;
            case 0xFB03:
                m_text += L"ffi";
                break;
            case 0xFB04:
                m_text += L"ffl";
                break;
            default:
                m_text += character;
                }
            }
        }

    //------------------------------------------------------------------
    void pdf_content_parser::handle_relative_move(const double moveX, const double moveY)
        {
        if (std::abs(moveY) > 0.001)
            {
            // BT resets the text line matrix to identity, so a Td/TD right after it
            // is setting an absolute position (like Tm), not stepping down by a small
            // incremental line move. Route it through the same delta-from-last-position
            // check as Tm, rather than treating its raw operand as the delta itself.
            if (m_freshTextObject)
                {
                const bool wroteNewline{ handle_absolute_move(moveY * m_fontScale) };
                // Landed on the same line as the previous (separate) text object (e.g., a
                // bullet glyph followed by its label, drawn with two different fonts). The
                // gap between them has no space character of its own, since it's expressed
                // purely through Td's x-offset, so add one to keep the words from running
                // together.
                if (!wroteNewline)
                    {
                    add_space();
                    }
                }
            else
                {
                add_newline(std::abs(moveY) > (1.8 * line_height()));
                // establish (or continue tracking) the baseline so that a later Tm
                // (handle_absolute_move) can correctly compute its delta, rather than
                // treating itself as the page's first position and skipping a newline
                m_currentY += (moveY * m_fontScale);
                m_haveY = true;
                }
            m_freshTextObject = false;
            }
        else if (moveX > line_height())
            {
            add_space();
            }
        }

    //------------------------------------------------------------------
    bool pdf_content_parser::handle_absolute_move(const double newY)
        {
        m_freshTextObject = false;
        // If nothing has been shown on this page yet, this move is establishing the
        // page's starting position, not landing next to already-placed text.
        // Skip the newline/space decision so the page doesn't start with stray leading whitespace.
        const bool isFirstPositionOnPage{ !m_haveY && !m_haveShownText };
        m_haveY = true;
        if (isFirstPositionOnPage)
            {
            m_currentY = newY;
            return false;
            }
        const double deltaY{ newY - m_currentY };
        const double scaledLineHeight{ line_height() * m_fontScale };
        bool wroteNewline{ false };
        if (std::abs(deltaY) > (0.25 * scaledLineHeight))
            {
            add_newline(std::abs(deltaY) > (1.8 * scaledLineHeight));
            wroteNewline = true;
            }
        m_currentY = newY;
        return wroteNewline;
        }

    //------------------------------------------------------------------
    void pdf_content_parser::show_string(const std::string& stringBytes,
                                         const pdf_font_decoder* currentFont)
        {
        add_text(pdf_text_decoder::decode_string_bytes(stringBytes, currentFont));
        }

    //------------------------------------------------------------------
    void pdf_content_parser::show_array(const std::string_view arrayValue,
                                        const pdf_font_decoder* currentFont)
        {
        size_t pos{ 0 };
        if (pos < arrayValue.length() && arrayValue[pos] == '[')
            {
            ++pos;
            }
        while (pos < arrayValue.length())
            {
            pdf_lexer::skip_whitespace(arrayValue, pos);
            if (pos >= arrayValue.length() || arrayValue[pos] == ']')
                {
                break;
                }
            if (arrayValue[pos] == '(')
                {
                show_string(pdf_lexer::read_literal_string(arrayValue, pos), currentFont);
                }
            else if (arrayValue[pos] == '<')
                {
                show_string(pdf_lexer::read_hex_string(arrayValue, pos), currentFont);
                }
            else
                {
                const std::string_view token{ pdf_lexer::read_value(arrayValue, pos) };
                if (token.empty())
                    {
                    break;
                    }
                // a large negative adjustment (in thousandths of an em)
                // is a gap between words
                double adjustment{ 0 };
                std::from_chars(token.data(), token.data() + token.length(), adjustment);
                if (adjustment < -150)
                    {
                    add_space();
                    }
                }
            }
        }

    //------------------------------------------------------------------
    pdf_page_resources pdf_content_parser::load_resources(const std::string_view resourcesValue)
        {
        pdf_page_resources resources;
        const std::string_view resourcesDict{ pdf_lexer::trim(
            m_document.resolve_value(resourcesValue)) };
        if (resourcesDict.compare(0, 2, "<<") != 0)
            {
            return resources;
            }
        const std::string_view fontsDict{ pdf_lexer::trim(
            m_document.resolve_value(pdf_lexer::find_dictionary_value(resourcesDict, "Font"))) };
        for (const auto& [fontName, fontValue] : pdf_lexer::get_dictionary_entries(fontsDict))
            {
            long fontObjectNumber{ 0 };
            if (pdf_lexer::get_reference(fontValue, fontObjectNumber))
                {
                resources.m_fonts.insert(std::make_pair(std::string{ fontName },
                                                        m_document.load_font(fontObjectNumber)));
                }
            else if (pdf_lexer::trim(fontValue).compare(0, 2, "<<") == 0)
                {
                resources.m_fonts.insert(std::make_pair(
                    std::string{ fontName },
                    m_document.load_font_from_dictionary(pdf_lexer::trim(fontValue))));
                }
            }
        const std::string_view xobjectsDict{ pdf_lexer::trim(
            m_document.resolve_value(pdf_lexer::find_dictionary_value(resourcesDict, "XObject"))) };
        for (const auto& [xobjectName, xobjectValue] :
             pdf_lexer::get_dictionary_entries(xobjectsDict))
            {
            long xobjectNumber{ 0 };
            if (pdf_lexer::get_reference(xobjectValue, xobjectNumber))
                {
                resources.m_xobjects.insert(
                    std::make_pair(std::string{ xobjectName }, xobjectNumber));
                }
            }
        return resources;
        }

    //------------------------------------------------------------------
    std::string_view pdf_content_parser::find_inherited_value(const pdf_object& pageObject,
                                                              const std::string_view keyName) const
        {
        std::string_view currentDict{ pageObject.m_dictionary };
        for (int guard = 0; guard < 32; ++guard)
            {
            const std::string_view value{ pdf_lexer::find_dictionary_value(currentDict, keyName) };
            if (!value.empty())
                {
                return value;
                }
            const pdf_object* parentObject{ m_document.resolve_to_object(
                pdf_lexer::find_dictionary_value(currentDict, "Parent")) };
            if (parentObject == nullptr || parentObject->m_dictionary.empty())
                {
                break;
                }
            currentDict = parentObject->m_dictionary;
            }
        return {};
        }

    //------------------------------------------------------------------
    std::string pdf_content_parser::load_page_content(const pdf_object& pageObject) const
        {
        std::string pageContent;
        const std::string_view contentsValue{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(pageObject.m_dictionary, "Contents")) };
        const auto appendStream = [this, &pageContent](const std::string_view value)
        {
            const pdf_object* contentObject{ m_document.resolve_to_object(value) };
            if (contentObject != nullptr && !contentObject->m_stream_data.empty())
                {
                const std::string decodedData{ m_document.decode_stream(*contentObject) };
                if (!decodedData.empty())
                    {
                    if (!pageContent.empty())
                        {
                        pageContent += '\n';
                        }
                    pageContent += decodedData;
                    }
                }
        };
        if (!contentsValue.empty() && contentsValue.front() == '[')
            {
            size_t pos{ 1 };
            while (pos < contentsValue.length())
                {
                pdf_lexer::skip_whitespace(contentsValue, pos);
                if (pos >= contentsValue.length() || contentsValue[pos] == ']')
                    {
                    break;
                    }
                const std::string_view element{ pdf_lexer::read_value(contentsValue, pos) };
                if (element.empty())
                    {
                    break;
                    }
                appendStream(element);
                }
            }
        else if (!contentsValue.empty())
            {
            appendStream(contentsValue);
            }
        return pageContent;
        }

    //------------------------------------------------------------------
    void pdf_content_parser::parse_content(const std::string_view content,
                                           const pdf_page_resources& resources, const int depth)
        {
        if (depth > 8)
            {
            return;
            }
        size_t pos{ 0 };
        std::vector<double> numbers;
        std::string lastName;
        std::string pendingString;
        bool havePendingString{ false };
        std::string_view pendingArray;
        const pdf_font_decoder* currentFont{ nullptr };
        while (pos < content.length())
            {
            pdf_lexer::skip_whitespace(content, pos);
            if (pos >= content.length())
                {
                break;
                }
            const char curChar{ content[pos] };
            if (curChar == '(')
                {
                pendingString = pdf_lexer::read_literal_string(content, pos);
                havePendingString = true;
                }
            else if (curChar == '<' && content.compare(pos, 2, "<<") == 0)
                {
                // a dictionary operand (e.g., for BDC); not needed
                std::ignore = pdf_lexer::read_value(content, pos);
                }
            else if (curChar == '<')
                {
                pendingString = pdf_lexer::read_hex_string(content, pos);
                havePendingString = true;
                }
            else if (curChar == '[')
                {
                pendingArray = pdf_lexer::read_value(content, pos);
                }
            else if (curChar == '/')
                {
                const std::string_view nameValue{ pdf_lexer::read_value(content, pos) };
                lastName.assign(nameValue.length() > 1 ? nameValue.substr(1) : std::string_view{});
                }
            else if (pdf_lexer::is_digit(curChar) || curChar == '+' || curChar == '-' ||
                     curChar == '.')
                {
                const size_t numberStart{ pos };
                while (pos < content.length() && !pdf_lexer::is_token_end(content[pos]))
                    {
                    ++pos;
                    }
                double numericValue{ 0 };
                std::from_chars(content.data() + numberStart, content.data() + pos, numericValue);
                numbers.push_back(numericValue);
                // operands are only cleared by operators
                continue;
                }
            else if (pdf_lexer::is_delimiter(curChar))
                {
                // stray delimiter in a malformed stream
                ++pos;
                continue;
                }
            else
                {
                // an operator keyword
                const size_t operatorStart{ pos };
                while (pos < content.length() && !pdf_lexer::is_token_end(content[pos]))
                    {
                    ++pos;
                    }
                const std::string_view keyword{ content.substr(operatorStart,
                                                               pos - operatorStart) };
                if (keyword == "BT")
                    {
                    // Resets the text line matrix to identity. The next Td/TD/Tm
                    // establishes a fresh position rather than stepping from the last one.
                    m_freshTextObject = true;
                    }
                else if (keyword == "Tf")
                    {
                    if (!numbers.empty())
                        {
                        m_fontSize = std::abs(numbers.back());
                        }
                    const auto fontPos = resources.m_fonts.find(lastName);
                    currentFont =
                        (fontPos != resources.m_fonts.cend()) ? fontPos->second.get() : nullptr;
                    }
                else if (keyword == "TL")
                    {
                    if (!numbers.empty())
                        {
                        m_leading = std::abs(numbers.back());
                        }
                    }
                else if (keyword == "Td" || keyword == "TD")
                    {
                    if (numbers.size() >= 2)
                        {
                        const double moveX{ numbers[numbers.size() - 2] };
                        const double moveY{ numbers[numbers.size() - 1] };
                        if (keyword == "TD")
                            {
                            m_leading = std::abs(moveY);
                            }
                        handle_relative_move(moveX, moveY);
                        }
                    }
                else if (keyword == "Tm")
                    {
                    if (numbers.size() >= 6)
                        {
                        // the matrix's vertical scale (fonts are often set at
                        // size 1 and then scaled up through the text matrix)
                        const double verticalScale{ std::abs(numbers[numbers.size() - 3]) };
                        if (verticalScale > 0.001)
                            {
                            m_fontScale = verticalScale;
                            }
                        handle_absolute_move(numbers[numbers.size() - 1]);
                        }
                    }
                else if (keyword == "T*")
                    {
                    add_newline(false);
                    if (m_haveY)
                        {
                        m_currentY -= line_height();
                        }
                    m_freshTextObject = false;
                    }
                else if (keyword == "Tj")
                    {
                    if (havePendingString)
                        {
                        show_string(pendingString, currentFont);
                        }
                    }
                else if (keyword == "'" || keyword == "\"")
                    {
                    add_newline(false);
                    if (m_haveY)
                        {
                        m_currentY -= line_height();
                        }
                    if (havePendingString)
                        {
                        show_string(pendingString, currentFont);
                        }
                    }
                else if (keyword == "TJ")
                    {
                    if (!pendingArray.empty())
                        {
                        show_array(pendingArray, currentFont);
                        }
                    }
                else if (keyword == "Do")
                    {
                    // a form XObject can contain more text; recurse into it
                    const auto xobjectPos = resources.m_xobjects.find(lastName);
                    if (xobjectPos != resources.m_xobjects.cend() &&
                        m_visited_xobjects.insert(xobjectPos->second).second)
                        {
                        const pdf_object* xobject{ m_document.find_object(xobjectPos->second) };
                        if (xobject != nullptr && !xobject->m_stream_data.empty() &&
                            pdf_lexer::trim(pdf_lexer::find_dictionary_value(xobject->m_dictionary,
                                                                             "Subtype")) == "/Form")
                            {
                            const std::string formContent{ m_document.decode_stream(*xobject) };
                            if (!formContent.empty())
                                {
                                const std::string_view formResources{
                                    pdf_lexer::find_dictionary_value(xobject->m_dictionary,
                                                                     "Resources")
                                };
                                parse_content(formContent,
                                              formResources.empty() ? resources :
                                                                      load_resources(formResources),
                                              depth + 1);
                                }
                            }
                        m_visited_xobjects.erase(xobjectPos->second);
                        }
                    }
                else if (keyword == "BI")
                    {
                    // inline image; skip to the "EI" keyword
                    while ((pos = content.find("EI", pos)) != std::string_view::npos)
                        {
                        if (pos > 0 && pdf_lexer::is_whitespace(content[pos - 1]) &&
                            ((pos + 2) >= content.length() ||
                             pdf_lexer::is_token_end(content[pos + 2])))
                            {
                            pos += 2;
                            break;
                            }
                        pos += 2;
                        }
                    if (pos == std::string_view::npos)
                        {
                        break;
                        }
                    }
                // any operator consumes the pending operands
                numbers.clear();
                pendingString.clear();
                havePendingString = false;
                pendingArray = std::string_view{};
                continue;
                }
            }
        }
    } // namespace lily_of_the_valley
