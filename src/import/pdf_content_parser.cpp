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
#include <cstdlib>
#include <cwctype>
#include <tuple>
#include <utility>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    pdf_content_parser::pdf_content_parser(pdf_document& document, std::wstring& text,
                                           const std::set<long>& hiddenOCGs)
        : m_document(document), m_text(text), m_hidden_ocgs(hiddenOCGs)
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
        m_horizScale = 100;
        m_atLineStart = true;
        m_haveShownText = false;
        m_freshTextObject = true;
        m_matrixA = 1;
        m_matrixB = 0;
        m_matrixC = 0;
        m_matrixD = 1;
        m_verticalWritingMode = false;
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
                character == 0x00B7 || character == 0x2043 || character == 0x2219 ||
                character == 0x25A1 /* checkbox */ || character == 0x2610);
        }

    //------------------------------------------------------------------
    bool pdf_content_parser::is_strong_rtl(const wchar_t character)
        {
        // Arabic-Indic and Extended Arabic-Indic digits aren't mirrored in RTL
        // text, so (like Latin digits) they must close a run rather than join
        // it; exclude them from the Arabic block below.
        if ((character >= 0x0660 && character <= 0x0669) ||
            (character >= 0x06F0 && character <= 0x06F9))
            {
            return false;
            }
        return (character >= 0x0590 && character <= 0x05FF) || // Hebrew
               (character >= 0x0600 && character <= 0x06FF) || // Arabic
               (character >= 0x0750 && character <= 0x077F) || // Arabic Supplement
               (character >= 0xFB1D && character <= 0xFB4F) || // Hebrew Presentation Forms
               (character >= 0xFB50 && character <= 0xFDFF) || // Arabic Presentation Forms-A
               (character >= 0xFE70 && character <= 0xFEFF);   // Arabic Presentation Forms-B
        }

    //------------------------------------------------------------------
    bool pdf_content_parser::is_combining_mark(const wchar_t character)
        {
        return (character >= 0x0591 && character <= 0x05BD) || // Hebrew points
               character == 0x05BF || (character >= 0x05C1 && character <= 0x05C2) ||
               (character >= 0x05C4 && character <= 0x05C5) || character == 0x05C7 ||
               (character >= 0x0610 && character <= 0x061A) || // Arabic marks
               (character >= 0x064B && character <= 0x065F) || // Arabic harakat
               character == 0x0670 || (character >= 0x06D6 && character <= 0x06DC) ||
               (character >= 0x06DF && character <= 0x06E4) ||
               (character >= 0x06E7 && character <= 0x06E8) ||
               (character >= 0x06EA && character <= 0x06ED);
        }

    //------------------------------------------------------------------
    void pdf_content_parser::flush_rtl_run()
        {
        if (m_rtlRunStart == std::wstring::npos)
            {
            return;
            }
        // Reverse the run's cluster order (a base character plus any combining
        // marks that follow it), not individual codepoints, so a diacritic
        // doesn't get swapped ahead of the base letter it attaches to.
        std::wstring reordered;
        reordered.reserve(m_rtlRunEnd - m_rtlRunStart);
        size_t clusterEnd{ m_rtlRunEnd };
        while (clusterEnd > m_rtlRunStart)
            {
            size_t clusterStart{ clusterEnd - 1 };
            while (clusterStart > m_rtlRunStart && is_combining_mark(m_text[clusterStart]))
                {
                --clusterStart;
                }
            reordered.append(m_text, clusterStart, clusterEnd - clusterStart);
            clusterEnd = clusterStart;
            }
        m_text.replace(m_rtlRunStart, m_rtlRunEnd - m_rtlRunStart, reordered);
        m_rtlRunStart = std::wstring::npos;
        m_rtlRunEnd = std::wstring::npos;
        }

    //------------------------------------------------------------------
    double pdf_content_parser::line_height() const
        {
        return std::max({ m_leading, m_fontSize, 4.0 });
        }

    //------------------------------------------------------------------
    void pdf_content_parser::add_newline(const bool paragraphBreak)
        {
        // a line break always closes an in-progress RTL run
        flush_rtl_run();
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
            // expand ligatures (Latin, so this always closes an open RTL run)
            switch (character)
                {
            case 0xFB00:
                flush_rtl_run();
                m_text += L"ff";
                break;
            case 0xFB01:
                flush_rtl_run();
                m_text += L"fi";
                break;
            case 0xFB02:
                flush_rtl_run();
                m_text += L"fl";
                break;
            case 0xFB03:
                flush_rtl_run();
                m_text += L"ffi";
                break;
            case 0xFB04:
                flush_rtl_run();
                m_text += L"ffl";
                break;
            default:
                if (is_strong_rtl(character))
                    {
                    if (m_rtlRunStart == std::wstring::npos)
                        {
                        m_rtlRunStart = m_text.length();
                        }
                    m_text += character;
                    m_rtlRunEnd = m_text.length();
                    }
                // whitespace doesn't break an open run: if more RTL text
                // follows, it (and this whitespace) join the run; otherwise
                // it's left outside the run when the run is next flushed
                else if (character == L' ' || character == L'\t')
                    {
                    m_text += character;
                    }
                else
                    {
                    flush_rtl_run();
                    m_text += character;
                    }
                }
            }
        }

    //------------------------------------------------------------------
    void pdf_content_parser::handle_relative_move(const double moveX, const double moveY)
        {
        // BT resets the text line matrix to identity, so a Td/TD right after it is
        // setting an absolute position (like Tm), not stepping from the current
        // position by a small amount. Route it through the same
        // delta-from-last-position check as Tm, rather than treating its raw
        // operands as the delta itself.
        if (m_freshTextObject)
            {
            const bool wroteNewline{ handle_absolute_move(moveX, moveY * m_fontScale) };
            // Landed on the same line as the previous (separate) text object (e.g., a
            // bullet glyph followed by its label, drawn with two different fonts). The
            // gap between them has no space character of its own, since it's expressed
            // purely through Td's offset, so add one to keep the words from running
            // together.
            if (!wroteNewline)
                {
                add_space();
                }
            return;
            }
        // Once a Tm or Td/TD has already run in this text object, later Td/TD
        // operands are offsets in that matrix's local space, which may carry
        // rotation or scale from an earlier Tm. They must be transformed through
        // the matrix before they're meaningful as a page-space delta. A purely
        // horizontal step in a rotated font's local space can land anywhere in
        // page space, not just along the page's x-axis.
        const double pageDeltaX{ (m_matrixA * moveX) + (m_matrixC * moveY) };
        const double pageDeltaY{ (m_matrixB * moveX) + (m_matrixD * moveY) };
        handle_absolute_move(m_currentX + pageDeltaX, m_currentY + pageDeltaY);
        }

    //------------------------------------------------------------------
    bool pdf_content_parser::handle_absolute_move(const double newX, const double newY)
        {
        m_freshTextObject = false;
        // If nothing has been shown on this page yet, this move is establishing the
        // page's starting position, not landing next to already-placed text.
        // Skip the newline/space decision so the page doesn't start with stray leading whitespace.
        const bool isFirstPositionOnPage{ !m_haveY && !m_haveShownText };
        m_haveY = true;
        if (isFirstPositionOnPage)
            {
            m_currentX = newX;
            m_currentY = newY;
            return false;
            }
        const double deltaX{ newX - m_currentX };
        const double deltaY{ newY - m_currentY };
        // In vertical writing mode, glyphs advance down a column and a new line is
        // a step across columns along x, the reverse of horizontal mode's
        // top-to-bottom line stepping along y.
        const double lineAxisDelta{ m_verticalWritingMode ? deltaX : deltaY };
        const double scaledLineHeight{ line_height() * m_fontScale };
        bool wroteNewline{ false };
        if (std::abs(lineAxisDelta) > (0.25 * scaledLineHeight))
            {
            add_newline(std::abs(lineAxisDelta) > (1.8 * scaledLineHeight));
            wroteNewline = true;
            }
        // Landed on (roughly) the same line, but far enough away that this is a
        // separate run, not a continuation of the previous glyph. The gap can be
        // horizontal, or diagonal for rotated text. This happens when text is drawn
        // as several independent BT/Tm blocks (one per label) instead of one Tj/TJ
        // run. Without this check, such runs would be glued directly together.
        else if (std::hypot(deltaX, deltaY) > scaledLineHeight)
            {
            add_space();
            }
        m_currentX = newX;
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
                const double adjustment{ extract_text::to_double(token) };
                // the adjustment is in unscaled text space, but Tz stretches or
                // compresses how much actual displayed width it corresponds to
                if ((adjustment * (m_horizScale / 100)) < -150)
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
        const std::string_view propertiesDict{ pdf_lexer::trim(m_document.resolve_value(
            pdf_lexer::find_dictionary_value(resourcesDict, "Properties"))) };
        for (const auto& [propertyName, propertyValue] :
             pdf_lexer::get_dictionary_entries(propertiesDict))
            {
            long propertyObjectNumber{ 0 };
            if (pdf_lexer::get_reference(propertyValue, propertyObjectNumber))
                {
                resources.m_ocg_visible.insert(std::make_pair(
                    std::string{ propertyName },
                    m_hidden_ocgs.find(propertyObjectNumber) == m_hidden_ocgs.cend()));
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
            for (const std::string_view element : pdf_lexer::read_array_elements(contentsValue))
                {
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
        std::string secondLastName;
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
                secondLastName = std::move(lastName);
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
                const double numericValue{ extract_text::to_double(
                    content.substr(numberStart, pos - numberStart)) };
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
                if (keyword == "BDC" || keyword == "BMC")
                    {
                    bool levelHidden{ !m_markedContentHidden.empty() &&
                                      m_markedContentHidden.back() };
                    if (!levelHidden && keyword == "BDC" && secondLastName == "OC")
                        {
                        const auto visiblePos{ resources.m_ocg_visible.find(lastName) };
                        levelHidden =
                            (visiblePos != resources.m_ocg_visible.cend() && !visiblePos->second);
                        }
                    m_markedContentHidden.push_back(levelHidden);
                    }
                else if (keyword == "EMC")
                    {
                    if (!m_markedContentHidden.empty())
                        {
                        m_markedContentHidden.pop_back();
                        }
                    }
                else if (keyword == "BI")
                    {
                    // Prefer the dictionary's /L (or /Length) entry, if given, to
                    // jump straight to the real "EI" instead of scanning for one,
                    // since the binary data can contain its own decoy "EI" bytes.
                    // Runs regardless of OCG visibility, to stay correctly positioned.
                    long declaredLength{ -1 };
                    bool foundId{ false };
                    size_t idPos{ pos };
                    size_t dictPos{ pos };
                    while (dictPos < content.length())
                        {
                        pdf_lexer::skip_whitespace(content, dictPos);
                        if (content.compare(dictPos, 2, "ID") == 0 &&
                            ((dictPos + 2) >= content.length() ||
                             pdf_lexer::is_token_end(content[dictPos + 2])))
                            {
                            idPos = dictPos;
                            foundId = true;
                            break;
                            }
                        if (dictPos >= content.length() || content[dictPos] != '/')
                            {
                            break;
                            }
                        const std::string_view key{ pdf_lexer::read_value(content, dictPos) };
                        const std::string_view value{ pdf_lexer::read_value(content, dictPos) };
                        if (key == "/L" || key == "/Length")
                            {
                            long lengthValue{ 0 };
                            if (pdf_lexer::to_int(value, lengthValue) && lengthValue >= 0)
                                {
                                declaredLength = lengthValue;
                                }
                            }
                        }
                    if (foundId && declaredLength >= 0)
                        {
                        size_t dataStart{ idPos + 2 };
                        if (dataStart < content.length() &&
                            pdf_lexer::is_whitespace(content[dataStart]))
                            {
                            ++dataStart;
                            }
                        const size_t dataEnd{ dataStart + static_cast<size_t>(declaredLength) };
                        size_t eiPos{ dataEnd };
                        if (dataEnd <= content.length())
                            {
                            pdf_lexer::skip_whitespace(content, eiPos);
                            }
                        if (dataEnd <= content.length() && content.compare(eiPos, 2, "EI") == 0 &&
                            ((eiPos + 2) >= content.length() ||
                             pdf_lexer::is_token_end(content[eiPos + 2])))
                            {
                            pos = eiPos + 2;
                            }
                        else
                            {
                            declaredLength = -1;
                            }
                        }
                    else
                        {
                        declaredLength = -1;
                        }
                    // no (usable) /L was declared; fall back to scanning for a
                    // whitespace-delimited "EI"
                    if (declaredLength < 0)
                        {
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
                    }
                else if (!m_markedContentHidden.empty() && m_markedContentHidden.back())
                    {
                    // inside a hidden optional-content group; every other operator
                    // (text showing, positioning, font/graphics state) is ignored
                    }
                else if (keyword == "BT")
                    {
                    // Resets the text line matrix to identity. The next Td/TD/Tm
                    // establishes a fresh position rather than stepping from the last one.
                    m_freshTextObject = true;
                    m_matrixA = 1;
                    m_matrixB = 0;
                    m_matrixC = 0;
                    m_matrixD = 1;
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
                    m_verticalWritingMode =
                        (currentFont != nullptr && currentFont->m_vertical_writing_mode);
                    }
                else if (keyword == "TL")
                    {
                    if (!numbers.empty())
                        {
                        m_leading = std::abs(numbers.back());
                        }
                    }
                else if (keyword == "Tz")
                    {
                    if (!numbers.empty())
                        {
                        m_horizScale = numbers.back();
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
                        m_matrixA = numbers[numbers.size() - 6];
                        m_matrixB = numbers[numbers.size() - 5];
                        m_matrixC = numbers[numbers.size() - 4];
                        m_matrixD = numbers[numbers.size() - 3];
                        // the matrix's vertical scale (fonts are often set at
                        // size 1 and then scaled up through the text matrix)
                        const double verticalScale{ std::abs(m_matrixD) };
                        if (verticalScale > 0.001)
                            {
                            m_fontScale = verticalScale;
                            }
                        handle_absolute_move(numbers[numbers.size() - 2],
                                             numbers[numbers.size() - 1]);
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
