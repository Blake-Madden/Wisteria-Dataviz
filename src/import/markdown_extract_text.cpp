///////////////////////////////////////////////////////////////////////////////
// Name:        markdown_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "markdown_extract_text.h"

//--------------------------------------------
bool lily_of_the_valley::markdown_extract_text::parse_styled_text(wchar_t& previousChar,
                                                                  const wchar_t tag)
    {
    // not styling text, just an orphan character that should be processed as-is
    if (*m_currentStart == tag && std::next(m_currentStart) < m_currentEndSentinel)
        {
        if (m_currentStart[1] != tag && !std::iswalnum(m_currentStart[1]) &&
            m_currentStart[1] != L'`')
            {
            add_character(*m_currentStart);
            previousChar = *m_currentStart;
            std::advance(m_currentStart, 1);
            return true;
            }
        }
    const wchar_t* const tagStart{ m_currentStart };
    while (*m_currentStart == tag && m_currentStart < m_currentEndSentinel)
        {
        std::advance(m_currentStart, 1);
        }
    std::wstring_view currentTag{ tagStart,
                                  static_cast<size_t>(std::distance(tagStart, m_currentStart)) };
    const wchar_t* endOfTag = string_util::find_unescaped_char_same_line_n(
        m_currentStart, tag, std::distance(m_currentStart, m_currentEndSentinel));
    if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
        {
        log_message(L"Missing matching styling tag in markdown file.");
        return false;
        }
    // if a bold tag (**), then move to the matching (terminating) tag
    while (currentTag == L"**" && (std::next(endOfTag) < m_currentEndSentinel) &&
           *std::next(endOfTag) != L'*')
        {
        std::advance(endOfTag, 1);
        endOfTag = string_util::find_unescaped_char_same_line_n(
            endOfTag, tag, std::distance(endOfTag, m_currentEndSentinel));
        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
            {
            log_message(L"Missing matching styling tag in markdown file.");
            return false;
            }
        }
    // or an italic tag(*), then move to the matching (*), skipping over any
    // embedded bold tags (**)
    while (currentTag == L"*" && (std::next(endOfTag) < m_currentEndSentinel) &&
           *std::next(endOfTag) == L'*')
        {
        std::advance(endOfTag, 2);
        endOfTag = string_util::find_unescaped_char_same_line_n(
            endOfTag, tag, std::distance(endOfTag, m_currentEndSentinel));
        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
            {
            log_message(L"Missing matching styling tag in markdown file.");
            return false;
            }
        }
    while (*endOfTag == tag && (endOfTag < m_currentEndSentinel))
        {
        std::advance(endOfTag, 1);
        }
    // in case we stepped ahead, step back to the last closing tag
    if (*endOfTag != tag)
        {
        std::advance(endOfTag, -1);
        }
    // ...and then move back to the first tag of the consecutive closing tags
    while (endOfTag > m_currentStart && *endOfTag == tag)
        {
        std::advance(endOfTag, -1);
        }
    if (*endOfTag != tag)
        {
        std::advance(endOfTag, 1);
        }

    [[maybe_unused]]
    auto retval = m_subParser->operator()(
        { m_currentStart, static_cast<size_t>(std::distance(m_currentStart, endOfTag)) });
    add_characters({ m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
    m_currentStart = endOfTag;
    while (*m_currentStart == tag && (m_currentStart < m_currentEndSentinel))
        {
        std::advance(m_currentStart, 1);
        }
    return true;
    }

//--------------------------------------------
bool lily_of_the_valley::markdown_extract_text::parse_html_block(const std::wstring_view tag,
                                                                 const std::wstring_view endTag)
    {
    const auto* originalStart{ m_currentStart };
    std::advance(m_currentStart, tag.length() + 1); // step over '<' also
    auto* endOfTag = string_util::find_matching_close_tag(
        { m_currentStart,
          static_cast<size_t>(std::distance(m_currentStart, m_currentEndSentinel)) },
        { tag.data() }, { endTag.data() });
    if (endOfTag == nullptr)
        {
        log_message(L"Bad HTML section in markdown file.");
        return false;
        }
    std::advance(endOfTag, endTag.length());
    if (endOfTag >= m_currentEndSentinel)
        {
        log_message(L"Bad HTML section in markdown file.");
        return false;
        }
    html_extract_text hext;
    hext(originalStart, std::distance(originalStart, endOfTag), false, false);
    add_characters({ hext.get_filtered_text(), hext.get_filtered_text_length() });
    std::advance(m_currentStart, std::distance(m_currentStart, endOfTag));
    return true;
    }

//--------------------------------------------
const wchar_t*
lily_of_the_valley::markdown_extract_text::operator()(const std::wstring_view md_text)
    {
    clear_log();
    clear();
    m_currentStart = m_currentEndSentinel = nullptr;

    if (md_text.empty())
        {
        return nullptr;
        }

    if (m_subParser == nullptr)
        {
        m_subParser = std::make_unique<markdown_extract_text>();
        }

    allocate_text_buffer(md_text.length() * 2);

    // find the m_currentStart of the text body and set up where we halt our searching
    m_currentStart = md_text.data();
    m_currentEndSentinel = md_text.data() + md_text.length();

    const wchar_t* metaEnd{ nullptr };
    if (has_metadata_section(m_currentStart))
        {
        metaEnd = find_metadata_section_end(m_currentStart);
        }
    if (metaEnd != nullptr)
        {
        m_currentStart = metaEnd;
        }
    // in case metadata section ate up the whole file
    // (or at least the part of the file requested to be reviewed)
    if (m_currentStart >= m_currentEndSentinel)
        {
        return m_currentEndSentinel;
        }

    const std::wstring_view QUARTO_PAGEBREAK{ L"{{< pagebreak >}}" };
    const std::wstring_view BEGIN_FIGURE{ L"\\begin{figure}" };
    const std::wstring_view END_FIGURE{ L"\\end{figure}" };

    const std::wstring_view TABLE{ L"table" };
    const std::wstring_view TABLE_END{ L"</table>" };

    const std::wstring_view UNORDERED_LIST{ L"ul" };
    const std::wstring_view UNORDERED_LIST_END{ L"</ul>" };

    const std::wstring_view ORDERED_LIST{ L"ol" };
    const std::wstring_view ORDERED_LIST_END{ L"</ol>" };

    const std::wstring_view SUP{ L"sup" };
    const std::wstring_view SUP_END{ L"</sup>" };

    bool isEscaping{ false };
    bool headerMode{ false };
    wchar_t previousChar{ L'\n' };

    while (m_currentStart != nullptr && *m_currentStart != 0 &&
           (m_currentStart < m_currentEndSentinel))
        {
        if (*m_currentStart == L'\\')
            {
            // Previous character was not \, but this one is.
            // Skip and get ready to escape the next character.
            if (!isEscaping)
                {
                // remove \index{} tags
                if (std::wcsncmp(m_currentStart, L"\\index{", 7) == 0)
                    {
                    m_currentStart += 7;
                    auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line(
                        m_currentStart, L'{', L'}');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad index{} command in markdown file.");
                        break;
                        }
                    m_currentStart = ++endOfTag;
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, BEGIN_FIGURE.data(), BEGIN_FIGURE.length()) ==
                         0)
                    {
                    m_currentStart += BEGIN_FIGURE.length();
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, END_FIGURE.data(), END_FIGURE.length()) == 0)
                    {
                    m_currentStart += END_FIGURE.length();
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, L"\\@ref(", 6) == 0)
                    {
                    m_currentStart += 6;
                    auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line(
                        m_currentStart, L'(', L')');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad cross reference command in markdown file.");
                        break;
                        }
                    m_currentStart = ++endOfTag;
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, L"\\newpage", 8) == 0)
                    {
                    m_currentStart += 8;
                    add_characters(L"\n\n");
                    continue;
                    }
                else if (m_currentStart < m_currentEndSentinel &&
                         (m_currentStart[1] == L'\n' || m_currentStart[1] == L'\r'))
                    {
                    headerMode = true;
                    ++m_currentStart;
                    continue;
                    }
                // actually is an escape character
                isEscaping = true;
                previousChar = *m_currentStart;
                ++m_currentStart;
                continue;
                }
            }
        // skip over header tags
        else if (*m_currentStart == L'#')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                while (*m_currentStart == L'#' && (m_currentStart < m_currentEndSentinel))
                    {
                    ++m_currentStart;
                    }
                // space between # and header text
                while ((*m_currentStart == L' ' || *m_currentStart == L'\t') &&
                       (m_currentStart < m_currentEndSentinel))
                    {
                    ++m_currentStart;
                    }
                previousChar = *m_currentStart;
                headerMode = true;
                continue;
                }
            }
        // RMarkdown div fences
        else if (*m_currentStart == L':')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                // space between > and quote text
                while ((*m_currentStart == L':') && (m_currentStart < m_currentEndSentinel))
                    {
                    ++m_currentStart;
                    }
                continue;
                }
            }
        // block quotes
        else if (*m_currentStart == L'>')
            {
            if (!isEscaping && (previousChar == L'\n' || previousChar == L'\r'))
                {
                size_t tabCount{ 0 };
                while (*m_currentStart == L'>' && (m_currentStart < m_currentEndSentinel))
                    {
                    ++tabCount;
                    ++m_currentStart;
                    }
                // space between > and quote text
                while ((*m_currentStart == L' ' || *m_currentStart == L'\t') &&
                       (m_currentStart < m_currentEndSentinel))
                    {
                    ++m_currentStart;
                    }
                fill_with_character(tabCount, L'\t');
                // Flags that we are still at the m_currentStart of the line,
                // so that headers and list items can still be parsed correctly.
                previousChar = L'\n';
                continue;
                }
            }
        // block quotes
        else if (*m_currentStart == L'&')
            {
            if (!isEscaping)
                {
                auto endOfTag = std::wcsstr(m_currentStart, L";");
                if (endOfTag != nullptr && (endOfTag < m_currentEndSentinel) &&
                    std::distance(m_currentStart, endOfTag) <= 6)
                    {
                    if (m_currentStart + 3 < endOfTag && m_currentStart[1] == L'#')
                        {
                        wchar_t* dummy{ nullptr };
                        const wchar_t decodedChar =
                            string_util::is_either(m_currentStart[2], L'x', L'X') ?
                                // if it is hex encoded (e.g., '&#xFF')
                                static_cast<wchar_t>(std::wcstol(m_currentStart + 3, &dummy, 16)) :
                                // else it is a plain numeric value (e.g., '&#79')
                                static_cast<wchar_t>(std::wcstol(m_currentStart + 2, &dummy, 10));
                        if (decodedChar != 0)
                            {
                            add_character(decodedChar);
                            }
                        previousChar = decodedChar;
                        }
                    else
                        {
                        const auto decodedChar = html_extract_text::HTML_TABLE_LOOKUP.find(
                            { m_currentStart + 1,
                              static_cast<size_t>(std::distance(m_currentStart, endOfTag) - 1) });
                        add_character(decodedChar);
                        previousChar = decodedChar;
                        }
                    m_currentStart += std::distance(m_currentStart, endOfTag) + 1;
                    continue;
                    }
                // not an HTML entity, treat as an ampersand at the end of the loop
                }
            }
        // code blocks
        else if (*m_currentStart == L'`')
            {
            // fenced section
            if (!isEscaping && std::wcsncmp(m_currentStart, L"```", 3) == 0)
                {
                m_currentStart += 3;
                auto endOfTag = std::wcsstr(m_currentStart, L"```");
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad fenced code block in markdown file.");
                    break;
                    }
                bool isMultiline{ false };
                auto scanAhead{ m_currentStart };
                while (scanAhead < endOfTag)
                    {
                    if (*scanAhead == L'\r' || *scanAhead == L'\n')
                        {
                        isMultiline = true;
                        break;
                        }
                    ++scanAhead;
                    }
                // if Quarto and this block is not getting echoed, then don't include
                // it in the parsed text
                if (const auto includeFalse = std::wcsstr(m_currentStart, L"#| include: false");
                    includeFalse != nullptr && includeFalse < endOfTag)
                    {
                    m_currentStart = endOfTag + 3;
                    }
                else if (const auto echoFalse = std::wcsstr(m_currentStart, L"#| echo: false");
                         echoFalse != nullptr && echoFalse < endOfTag)
                    {
                    m_currentStart = endOfTag + 3;
                    }
                else
                    {
                    bool pastFirstLine{ false };
                    // tab over each line inside the code block
                    while (m_currentStart < endOfTag)
                        {
                        if (*m_currentStart == L'\r' || *m_currentStart == L'\n')
                            {
                            while (m_currentStart < endOfTag &&
                                   (*m_currentStart == L'\r' || *m_currentStart == L'\n'))
                                {
                                add_character(*m_currentStart);
                                ++m_currentStart;
                                }
                            add_character(L'\t');
                            pastFirstLine = true;
                            continue;
                            }
                        if (!isMultiline || pastFirstLine)
                            {
                            // step over line if Quarto code block directive
                            if (std::wcsncmp(m_currentStart, L"#| ", 3) == 0)
                                {
                                while (m_currentStart < endOfTag &&
                                       !(*m_currentStart == L'\r' || *m_currentStart == L'\n'))
                                    {
                                    ++m_currentStart;
                                    }
                                continue;
                                }
                            else
                                {
                                add_character(*m_currentStart);
                                }
                            }
                        ++m_currentStart;
                        }
                    m_currentStart = endOfTag + 3;
                    }
                // if code block is not inline, then force a line break later after it
                if (m_currentStart < m_currentEndSentinel &&
                    ((*m_currentStart == L'\r' || *m_currentStart == L'\n')))
                    {
                    headerMode = true;
                    }
                continue;
                }
            // verbatim (inline) code
            else if (!isEscaping)
                {
                // RMarkdown code should left as-is, but with the 'r' prefix removed
                // (or processed for known functions)
                if (std::wcsncmp(m_currentStart, L"`r keys(", 8) == 0)
                    {
                    m_currentStart += 8;
                    if (m_currentStart + 1 < m_currentEndSentinel &&
                        (m_currentStart[1] == L'\'' || m_currentStart[1] == L'"'))
                        {
                        ++m_currentStart;
                        }
                    if (*m_currentStart == L'\'' || *m_currentStart == L'"')
                        {
                        const auto quoteChar{ *m_currentStart };
                        ++m_currentStart;
                        auto endOfTag = string_util::find_unescaped_char_same_line_n(
                            m_currentStart, quoteChar,
                            std::distance(m_currentStart, m_currentEndSentinel));
                        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                            {
                            log_message(L"Bad 'r keys' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]]
                        auto retval = m_subParser->operator()(
                            { m_currentStart,
                              static_cast<size_t>(std::distance(m_currentStart, endOfTag)) });
                        add_character(L'"');
                        add_characters({ m_subParser->get_filtered_text(),
                                         m_subParser->get_filtered_text_length() });
                        add_character(L'"');
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser
                                    ->get_filtered_text()[m_subParser->get_filtered_text_length() -
                                                          1];
                            }
                        endOfTag = string_util::find_unescaped_char_same_line_n(
                            m_currentStart, L'`',
                            std::distance(m_currentStart, m_currentEndSentinel));
                        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                            {
                            log_message(L"Bad 'r keys' code block in markdown file.");
                            break;
                            }
                        m_currentStart = ++endOfTag;
                        }
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, L"`r drop_cap(", 11) == 0)
                    {
                    m_currentStart += 11;
                    if (m_currentStart + 1 < m_currentEndSentinel &&
                        (m_currentStart[1] == L'\'' || m_currentStart[1] == L'"'))
                        {
                        ++m_currentStart;
                        }
                    if (*m_currentStart == L'\'' || *m_currentStart == L'"')
                        {
                        const auto quoteChar{ *m_currentStart };
                        ++m_currentStart;
                        auto endOfTag = string_util::find_unescaped_char_same_line_n(
                            m_currentStart, quoteChar,
                            std::distance(m_currentStart, m_currentEndSentinel));
                        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                            {
                            log_message(L"Bad 'r dropcap' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]]
                        auto retval = m_subParser->operator()(
                            { m_currentStart,
                              static_cast<size_t>(std::distance(m_currentStart, endOfTag)) });
                        add_character(L'"');
                        add_characters({ m_subParser->get_filtered_text(),
                                         m_subParser->get_filtered_text_length() });
                        add_character(L'"');
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser
                                    ->get_filtered_text()[m_subParser->get_filtered_text_length() -
                                                          1];
                            }
                        endOfTag = string_util::find_unescaped_char_same_line_n(
                            m_currentStart, L'`',
                            std::distance(m_currentStart, m_currentEndSentinel));
                        if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                            {
                            log_message(L"Bad 'r dropcap' code block in markdown file.");
                            break;
                            }
                        m_currentStart = ++endOfTag;
                        }
                    continue;
                    }
                else if (std::wcsncmp(m_currentStart, L"`r menu(", 8) == 0)
                    {
                    m_currentStart += 8;
                    if (m_currentStart + 1 < m_currentEndSentinel && (m_currentStart[0] == L'c') &&
                        (m_currentStart[1] == L'('))
                        {
                        m_currentStart += 2;
                        }
                    if (*m_currentStart == L'\'' || *m_currentStart == L'"')
                        {
                        auto endOfTag = std::wcsstr(m_currentStart, L")`");
                        if (endOfTag == nullptr)
                            {
                            log_message(L"Bad 'r menu' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]]
                        auto retval = m_subParser->operator()(
                            { m_currentStart,
                              static_cast<size_t>(std::distance(m_currentStart, endOfTag - 1)) });
                        add_characters({ m_subParser->get_filtered_text(),
                                         m_subParser->get_filtered_text_length() });
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser
                                    ->get_filtered_text()[m_subParser->get_filtered_text_length() -
                                                          1];
                            }
                        m_currentStart = endOfTag + 2;
                        }
                    continue;
                    }
                // read content as-is otherwise
                else
                    {
                    if (std::wcsncmp(m_currentStart, L"`r ", 3) == 0)
                        {
                        m_currentStart += 3;
                        }
                    else if (std::wcsncmp(m_currentStart, L"`python ", 8) == 0)
                        {
                        m_currentStart += 8;
                        }
                    else
                        {
                        ++m_currentStart;
                        }
                    // `` section, which can have embedded backticks
                    if (*m_currentStart == '`')
                        {
                        auto endOfTag = std::wcsstr(++m_currentStart, L"``");
                        if (endOfTag == nullptr || endOfTag > m_currentEndSentinel)
                            {
                            log_message(L"Bad inline `` code block in markdown file.");
                            break;
                            }
                        // read in content verbatim
                        while (m_currentStart < endOfTag)
                            {
                            previousChar = *m_currentStart;
                            add_character(*m_currentStart);
                            ++m_currentStart;
                            }
                        m_currentStart += 2;
                        }
                    // just a single backtick block, should be on one line
                    else
                        {
                        while (m_currentStart < m_currentEndSentinel && *m_currentStart != '`')
                            {
                            // inline blocks should be on one line, so bail if we hit a new line
                            // as this would probably be a missing closing backtick
                            if (*m_currentStart == L'\n' || *m_currentStart == L'\r')
                                {
                                log_message(L"Unterminated inline ` code block in markdown file.");
                                previousChar = *m_currentStart;
                                add_character(*m_currentStart);
                                ++m_currentStart;
                                break;
                                }
                            previousChar = *m_currentStart;
                            add_character(*m_currentStart);
                            ++m_currentStart;
                            }
                        if (*m_currentStart == '`')
                            {
                            ++m_currentStart;
                            }
                        }
                    continue;
                    }
                }
            }
        // images (we don't read in the alt text inside the [], just skip everything)
        else if (*m_currentStart == L'!')
            {
            if (!isEscaping && (m_currentStart + 1 < m_currentEndSentinel) &&
                m_currentStart[1] == L'[')
                {
                m_currentStart += 2;
                auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    m_currentStart, L'[', L']',
                    std::distance(m_currentStart, m_currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                    {
                    log_message(L"Bad image command in markdown file.");
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                m_currentStart = ++endOfTag;
                if (*m_currentStart == L'(')
                    {
                    ++m_currentStart;
                    endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                        m_currentStart, L'(', L')',
                        std::distance(m_currentStart, m_currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                        {
                        log_message(L"Bad image command in markdown file.");
                        previousChar = L'(';
                        add_character(L'(');
                        continue;
                        }
                    m_currentStart = ++endOfTag;
                    }
                continue;
                }
            }
        // links
        else if (*m_currentStart == L'[')
            {
            if (!isEscaping)
                {
                auto labelStart{ ++m_currentStart };
                auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    m_currentStart, L'[', L']',
                    std::distance(m_currentStart, m_currentEndSentinel));
                if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                    {
                    log_message(L"Bad link command in markdown file. Missing closing ']'.");
                    // just treat it like a stray '[' and keep going
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                m_currentStart = ++endOfTag;
                if (*m_currentStart == L'(')
                    {
                    auto labelEnd{ m_currentStart - 1 };
                    ++m_currentStart;
                    endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                        m_currentStart, L'(', L')',
                        std::distance(m_currentStart, m_currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                        {
                        log_message(L"Bad link command in markdown file. Missing closing ')'.");
                        // read in the label and '(' section after it as-is if closing ')'
                        // is missing
                        m_currentStart = labelStart;
                        previousChar = L'[';
                        add_character(L'[');
                        continue;
                        }
                    m_currentStart = ++endOfTag;
                    if (labelStart < labelEnd)
                        {
                        [[maybe_unused]]
                        auto retval = m_subParser->operator()(
                            { labelStart,
                              static_cast<size_t>(std::distance(labelStart, labelEnd)) });
                        add_characters({ m_subParser->get_filtered_text(),
                                         m_subParser->get_filtered_text_length() });
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser
                                    ->get_filtered_text()[m_subParser->get_filtered_text_length() -
                                                          1];
                            }
                        }
                    }
                else
                    {
                    log_message(L"Bad link command in markdown file. Missing '()' section.");
                    // read in the label and '(' section after it as-is if closing ')' is missing
                    m_currentStart = labelStart;
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                continue;
                }
            }
        // IDs
        else if (*m_currentStart == L'{')
            {
            if (!isEscaping)
                {
                // if quarto syntax
                if (std::wcsncmp(m_currentStart, QUARTO_PAGEBREAK.data(),
                                 QUARTO_PAGEBREAK.length()) == 0)
                    {
                    m_currentStart += QUARTO_PAGEBREAK.length();
                    add_characters(L"\n\n");
                    }
                else
                    {
                    std::advance(m_currentStart, 1);
                    auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                        m_currentStart, L'{', L'}',
                        std::distance(m_currentStart, m_currentEndSentinel));
                    if (endOfTag == nullptr || (endOfTag >= m_currentEndSentinel))
                        {
                        log_message(L"Bad ID command in markdown file.");
                        break;
                        }
                    m_currentStart = ++endOfTag;
                    }
                continue;
                }
            }
        // superscript (just read as-is)
        else if (!isEscaping && *m_currentStart == L'^')
            {
            ++m_currentStart;
            continue;
            }
        // RMarkdown (Pandoc) comment
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"<!--", 4) == 0)
            {
            const auto endOfTag = std::wcsstr(m_currentStart, L"-->");
            if (endOfTag == nullptr)
                {
                log_message(L"Bad comment block in markdown file.");
                break;
                }
            m_currentStart = endOfTag + 3;
            continue;
            }
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"<p>", 3) == 0)
            {
            m_currentStart += 3;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"</p>", 4) == 0)
            {
            m_currentStart += 4;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        // newline hacks found in tables (just replace with space to keep the table structure).
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"<br>\\linebreak", 14) == 0)
            {
            m_currentStart += 14;
            previousChar = L' ';
            add_character(L' ');
            continue;
            }
        // HTML newline
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"<br>", 4) == 0)
            {
            m_currentStart += 4;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && std::wcsncmp(m_currentStart, L"<br/>", 5) == 0)
            {
            m_currentStart += 5;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping && (std::wcsncmp(m_currentStart, L"< br/>", 6) == 0 ||
                                 std::wcsncmp(m_currentStart, L"<br />", 6) == 0))
            {
            m_currentStart += 6;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (*m_currentStart == L'<')
            {
            if (!isEscaping &&
                static_cast<size_t>(std::distance(m_currentStart, m_currentEndSentinel)) >=
                    TABLE.length() + 1 &&
                std::wcsncmp(std::next(m_currentStart), TABLE.data(), TABLE.length()) == 0)
                {
                if (!parse_html_block(TABLE, TABLE_END))
                    {
                    break;
                    }
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(m_currentStart, m_currentEndSentinel)) >=
                         UNORDERED_LIST.length() + 1 &&
                     std::wcsncmp(std::next(m_currentStart), UNORDERED_LIST.data(),
                                  UNORDERED_LIST.length()) == 0)
                {
                if (!parse_html_block(UNORDERED_LIST, UNORDERED_LIST_END))
                    {
                    break;
                    }
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(m_currentStart, m_currentEndSentinel)) >=
                         ORDERED_LIST.length() + 1 &&
                     std::wcsncmp(std::next(m_currentStart), ORDERED_LIST.data(),
                                  ORDERED_LIST.length()) == 0)
                {
                if (!parse_html_block(ORDERED_LIST, ORDERED_LIST_END))
                    {
                    break;
                    }
                }
            else if (!isEscaping &&
                     static_cast<size_t>(std::distance(m_currentStart, m_currentEndSentinel)) >=
                         SUP.length() + 1 &&
                     std::wcsncmp(std::next(m_currentStart), SUP.data(), SUP.length()) == 0)
                {
                if (!parse_html_block(SUP, SUP_END))
                    {
                    break;
                    }
                }
            else if (!isEscaping && std::next(m_currentStart) < m_currentEndSentinel &&
                     (*std::next(m_currentStart) == L'/' || *std::next(m_currentStart) == L'p' ||
                      std::wcsncmp(std::next(m_currentStart), L"a ", 2) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"b>", 2) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"i>", 2) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"u>", 2) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"code", 4) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"span", 4) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"strong", 6) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"div", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"dl>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"dt>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"dd>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"em>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"tt>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"ul>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"ol>", 3) == 0 ||
                      std::wcsncmp(std::next(m_currentStart), L"li>", 3) == 0))
                {
                ++m_currentStart;
                auto* endOfTag = string_util::find_unescaped_matching_close_tag_same_line_n(
                    m_currentStart, L'<', L'>',
                    std::distance(m_currentStart, m_currentEndSentinel));
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad <> pair in markdown file.");
                    break;
                    }
                m_currentStart = ++endOfTag;
                continue;
                }
            }
        // newlines
        else if (*m_currentStart == L'\n' || *m_currentStart == L'\r')
            {
            // two (or more) spaces at the end of a line indicates a paragraph break
            size_t newlineCount{ 0 };
            if (previousChar == L' ' && std::distance(md_text.data(), m_currentStart) > 2 &&
                *(m_currentStart - 2) == L' ')
                {
                ++newlineCount;
                }
            // count the newlines (taking CRLF combos into account)
            while ((*m_currentStart == L'\n' || *m_currentStart == L'\r') &&
                   (m_currentStart < m_currentEndSentinel))
                {
                if (*m_currentStart == L'\r' && (m_currentStart + 1 < m_currentEndSentinel) &&
                    m_currentStart[1] == L'\n')
                    {
                    ++m_currentStart;
                    continue;
                    }
                ++newlineCount;
                ++m_currentStart;
                // If the next line is a header line divider, then
                // skip that, switch to header mode, and keep reading
                // any more newlines
                if (m_currentStart + 1 < m_currentEndSentinel &&
                    ((m_currentStart[0] == L'=' && m_currentStart[1] == L'=') ||
                     (m_currentStart[0] == L'-' && m_currentStart[1] == L'-')))
                    {
                    while ((*m_currentStart == L'=' || *m_currentStart == L'-') &&
                           (m_currentStart < m_currentEndSentinel))
                        {
                        ++m_currentStart;
                        }
                    headerMode = true;
                    }
                }

            auto scanAhead{ m_currentStart };
            size_t leadingScaces{ 0 };
            while ((scanAhead < m_currentEndSentinel) &&
                   (*scanAhead == L' ' || *scanAhead == L'\t'))
                {
                ++scanAhead;
                ++leadingScaces;
                }
            if (newlineCount == 1 && headerMode)
                {
                add_characters(L"\n\n");
                previousChar = L'\n';
                }
            // next line starts a list item, quoteblock, table, etc., so keep the newline as-is
            else if (newlineCount == 1 &&
                     (string_util::is_one_of(*scanAhead, L">-*+|:^") || leadingScaces >= 4))
                {
                fill_with_character(newlineCount, L'\n');
                previousChar = L'\n';
                }
            // same for an ordered list
            else if (newlineCount == 1 && (m_currentStart < m_currentEndSentinel) &&
                     std::iswdigit(*m_currentStart))
                {
                auto scanAheadDigit{ m_currentStart };
                while ((scanAheadDigit < m_currentEndSentinel) && std::iswdigit(*scanAheadDigit))
                    {
                    ++scanAheadDigit;
                    }
                if (*scanAheadDigit == L'.')
                    {
                    fill_with_character(newlineCount, L'\n');
                    previousChar = L'\n';
                    }
                // not an ordered list, default behavior to read as space
                else
                    {
                    add_character(L' ');
                    previousChar = L' ';
                    }
                }
            // a single newline not at end of a self-contained line
            // (e.g., a header) is seen as a space
            else if (newlineCount == 1)
                {
                if (*m_currentStart == L'#' || *m_currentStart == L'-' || *m_currentStart == L'|')
                    {
                    add_character(L'\n');
                    previousChar = L'\n';
                    }
                else
                    {
                    add_character(L' ');
                    previousChar = L' ';
                    }
                }
            else
                {
                fill_with_character(newlineCount, L'\n');
                previousChar = L'\n';
                }
            headerMode = false;
            isEscaping = false;
            continue;
            }
        // styling tags that just get removed from raw text
        else if (!isEscaping && *m_currentStart == L'*')
            {
            parse_styled_text(previousChar, L'*');
            continue;
            }
        else if (!isEscaping && *m_currentStart == L'_')
            {
            parse_styled_text(previousChar, L'_');
            continue;
            }
        else if (!isEscaping && *m_currentStart == L'~')
            {
            parse_styled_text(previousChar, L'~');
            continue;
            }
        // table
        else if (!isEscaping && (*m_currentStart == L'|'))
            {
            previousChar = L'|';
            auto scanAhead{ std::next(m_currentStart) };
            // if the line is table column format specifiers (e.g., ":--"),
            // then step over the whole line
            while (scanAhead < m_currentEndSentinel && (*scanAhead == L' ' || *scanAhead == L'\t'))
                {
                ++scanAhead;
                }
            if ((*scanAhead == L'-' && *std::next(scanAhead) == L'-') ||
                (*scanAhead == L':' && *std::next(scanAhead) == L'-'))
                {
                // move to the end of the line...
                while (scanAhead < m_currentEndSentinel &&
                       !(*scanAhead == L'\r' || *scanAhead == L'\n'))
                    {
                    ++scanAhead;
                    }
                // ...and then step of the newlines
                while (scanAhead < m_currentEndSentinel &&
                       (*scanAhead == L'\r' || *scanAhead == L'\n'))
                    {
                    ++scanAhead;
                    }
                m_currentStart = scanAhead;
                }
            else
                {
                add_characters(L"\t|");
                ++m_currentStart;
                }
            continue;
            }
        // turn off escaping and load the character
        isEscaping = false;
        if (m_currentStart < m_currentEndSentinel)
            {
            previousChar = *m_currentStart;
            add_character(*m_currentStart);
            ++m_currentStart;
            }
        else
            {
            break;
            }
        }

    return get_filtered_text();
    }
