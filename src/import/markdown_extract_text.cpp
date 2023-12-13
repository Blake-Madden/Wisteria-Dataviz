#include "markdown_extract_text.h"

const wchar_t* lily_of_the_valley::markdown_extract_text::operator()(const std::wstring_view md_text)
    {
    clear_log();
    if (md_text.empty())
        {
        set_filtered_text_length(0);
        return nullptr;
        }

    if (m_subParser == nullptr)
        { m_subParser = std::make_unique<markdown_extract_text>(); }

    if (!allocate_text_buffer(md_text.length() * 2))
        {
        set_filtered_text_length(0);
        return nullptr;
        }

    // find the start of the text body and set up where we halt our searching
    const wchar_t* const endSentinel = md_text.data() + md_text.length();
    const wchar_t* start = md_text.data();
    const wchar_t* metaEnd{ nullptr };
    if (has_metadata_section(start))
        { metaEnd = find_metadata_section_end(start); }
    if (metaEnd != nullptr)
        { start = metaEnd; }
    // in case metadata section ate up the whole file
    // (or at least the part of the file requested to be reviewed)
    if (start >= endSentinel)
        { return endSentinel; }

    const std::wstring_view QUARTO_PAGEBREAK{ L"{{< pagebreak >}}" };
    const std::wstring_view BEGIN_FIGURE{ L"\\begin{figure}" };
    const std::wstring_view END_FIGURE{ L"\\end{figure}" };

    bool isEscaping{ false };
    bool headerMode{ false };
    wchar_t previousChar{ L'\n' };

    const auto parseStyledText = [&](const wchar_t tag)
        {
        // not styling text, just an orphan character that should be processed as-is
        if (*start == tag &&
            start + 1 < endSentinel)
            {
            if (start[1] != tag &&
                !std::iswalnum(start[1]) &&
                start[1] != L'`')
                {
                add_character(*start);
                previousChar = *start;
                ++start;
                return true;
                }
            }
        while (*start == tag &&
            start < endSentinel)
            { ++start; }
        auto endOfTag =
            string_util::find_unescaped_char_same_line_n(start, tag, std::distance(start, endSentinel));
        if (endOfTag == nullptr || (endOfTag >= endSentinel))
            {
            log_message(L"Missing matching styling tag in markdown file.");
            return false;
            }
        while (*endOfTag == tag &&
            (endOfTag < endSentinel))
            { ++endOfTag; }
        while (endOfTag < endSentinel &&
            (std::iswalnum(*endOfTag) || *endOfTag == L'`'))
            {
            ++endOfTag;
            endOfTag =
                string_util::find_unescaped_char_same_line_n(endOfTag, tag,
                                                   std::distance(endOfTag, endSentinel));
            if (endOfTag == nullptr ||
                endOfTag + 1 >= endSentinel)
                {
                log_message(L"Missing matching styling tag in markdown file.");
                return false;
                }
            ++endOfTag;
            }
        // in case we stepped ahead, step back to the last closing tag
        if (*endOfTag != tag)
            {
            --endOfTag;
            }
        // ...and then move back to the first tag of the consecutive closing tags
        while (endOfTag > start && *endOfTag == tag)
            {
            --endOfTag;
            }
        if (*endOfTag != tag)
            {
            ++endOfTag;
            }

        [[maybe_unused]] auto retval = m_subParser->operator()(
            { start, static_cast<size_t>(std::distance(start, endOfTag)) });
        add_characters(
            { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
        start = endOfTag;
        while (*start == tag &&
            (start < endSentinel))
            { ++start; }
        return true;
        };

    while (start != nullptr && *start != 0 && (start < endSentinel))
        {
        if (*start == L'\\')
            {
            // Previous character was not \, but this one is.
            // Skip and get ready to escape the next character.
            if (!isEscaping)
                {
                // remove \index{} tags
                if (std::wcsncmp(start, L"\\index{", 7) == 0)
                    {
                    start += 7;
                    auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line(start, L'{', L'}');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad index{} command in markdown file.");
                        break;
                        }
                    start = ++endOfTag;
                    continue;
                    }
                else if (std::wcsncmp(start, BEGIN_FIGURE.data(), BEGIN_FIGURE.length()) == 0)
                    {
                    start += BEGIN_FIGURE.length();
                    continue;
                    }
                else if (std::wcsncmp(start, END_FIGURE.data(), END_FIGURE.length()) == 0)
                    {
                    start += END_FIGURE.length();
                    continue;
                    }
                else if (std::wcsncmp(start, L"\\@ref(", 6) == 0)
                    {
                    start += 6;
                    auto endOfTag = string_util::find_unescaped_matching_close_tag_same_line(start, L'(', L')');
                    if (endOfTag == nullptr)
                        {
                        log_message(L"Bad cross reference command in markdown file.");
                        break;
                        }
                    start = ++endOfTag;
                    continue;
                    }
                else if (std::wcsncmp(start, L"\\newpage", 8) == 0)
                    {
                    start += 8;
                    add_characters(L"\n\n");
                    continue;
                    }
                else if (start < endSentinel &&
                    (start[1] == L'\n' || start[1] == L'\r'))
                    {
                    headerMode = true;
                    ++start;
                    continue;
                    }
                // actually is an escape character
                isEscaping = true;
                previousChar = *start;
                ++start;
                continue;
                }
            }
        // skip over header tags
        else if (*start == L'#')
            {
            if (!isEscaping &&
                (previousChar == L'\n' || previousChar == L'\r'))
                {
                while (*start == L'#' &&
                       (start < endSentinel))
                    { ++start; }
                // space between # and header text
                while ((*start == L' ' || *start == L'\t') &&
                       (start < endSentinel))
                    { ++start; }
                previousChar = *start;
                headerMode = true;
                continue;
                }
            }
        // RMarkdown div fences
        else if (*start == L':')
            {
            if (!isEscaping &&
                (previousChar == L'\n' || previousChar == L'\r'))
                {
                // space between > and quote text
                while ((*start == L':') &&
                       (start < endSentinel))
                    { ++start; }
                continue;
                }
            }
        // block quotes
        else if (*start == L'>')
            {
            if (!isEscaping &&
                (previousChar == L'\n' || previousChar == L'\r'))
                {
                size_t tabCount{ 0 };
                while (*start == L'>' &&
                       (start < endSentinel))
                    {
                    ++tabCount;
                    ++start;
                    }
                // space between > and quote text
                while ((*start == L' ' || *start == L'\t') &&
                       (start < endSentinel))
                    { ++start; }
                add_character(L'\t', tabCount);
                // Flags that we are still at the start of the line,
                // so that headers and list items can still be parsed correctly.
                previousChar = L'\n';
                continue;
                }
            }
        // block quotes
        else if (*start == L'&')
            {
            if (!isEscaping)
                {
                auto endOfTag = std::wcsstr(start, L";");
                if (endOfTag != nullptr &&
                    (endOfTag < endSentinel) &&
                    std::distance(start, endOfTag) <= 6)
                    {
                    if (start[1] == L'#')
                        {
                        const auto decodedChar = static_cast<wchar_t>(string_util::atoi(start + 2));
                        add_character(decodedChar);
                        previousChar = decodedChar;
                        }
                    else
                        {
                        const auto decodedChar =
                            html_extract_text::HTML_TABLE_LOOKUP.find(
                                { start + 1, static_cast<size_t>(std::distance(start, endOfTag) - 1) });
                        add_character(decodedChar);
                        previousChar = decodedChar;
                        }
                    start += std::distance(start, endOfTag) + 1;
                    continue;
                    }
                // not an HTML entity, treat as an ampersand at the end of the loop
                }
            }
        // code blocks
        else if (*start == L'`')
            {
            // fenced section
            if (!isEscaping && std::wcsncmp(start, L"```", 3) == 0)
                {
                start += 3;
                auto endOfTag = std::wcsstr(start, L"```");
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad fenced code block in markdown file.");
                    break;
                    }
                bool isMultiline{ false };
                auto scanAhead{ start };
                while (scanAhead < endOfTag)
                    {
                    if (*scanAhead == L'\r' ||
                        *scanAhead == L'\n')
                        {
                        isMultiline = true;
                        break;
                        }
                    ++scanAhead;
                    }
                bool pastFirstLine{ false };
                // tab over each line inside of the code block
                while (start < endOfTag)
                    {
                    if (*start == L'\r' ||
                        *start == L'\n')
                        {
                        while (start < endOfTag &&
                            (*start == L'\r' ||
                             *start == L'\n'))
                            {
                            add_character(*start);
                            ++start;
                            }
                        add_character(L'\t');
                        pastFirstLine = true;
                        continue;
                        }
                    if (!isMultiline || pastFirstLine)
                        {
                        add_character(*start);
                        }
                    ++start;
                    }
                start = endOfTag + 3;
                // if code block is not inline, then force a line break later after it
                if (start < endSentinel &&
                    ((*start == L'\r' ||
                     *start == L'\n')))
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
                if (std::wcsncmp(start, L"`r keys(", 8) == 0)
                    {
                    start += 8;
                    if (start + 1 < endSentinel &&
                        (start[1] == L'\'' || start[1] == L'"'))
                        { ++start; }
                    if (*start == L'\'' || *start == L'"')
                        {
                        const auto quoteChar{ *start };
                        ++start;
                        auto endOfTag =
                            string_util::find_unescaped_char_same_line_n(start, quoteChar, std::distance(start, endSentinel));
                        if (endOfTag == nullptr || (endOfTag >= endSentinel))
                            {
                            log_message(L"Bad 'r keys' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]] auto retval = m_subParser->operator()(
                            { start, static_cast<size_t>(std::distance(start, endOfTag)) });
                        add_character(L'"');
                        add_characters(
                            { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                        add_character(L'"');
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                            }
                        endOfTag = string_util::find_unescaped_char_same_line_n(start, L'`', std::distance(start, endSentinel));
                        if (endOfTag == nullptr || (endOfTag >= endSentinel))
                            {
                            log_message(L"Bad 'r keys' code block in markdown file.");
                            break;
                            }
                        start = ++endOfTag;
                        }
                    continue;
                    }
                else if (std::wcsncmp(start, L"`r drop_cap(", 11) == 0)
                    {
                    start += 11;
                    if (start + 1 < endSentinel &&
                        (start[1] == L'\'' || start[1] == L'"'))
                        { ++start; }
                    if (*start == L'\'' || *start == L'"')
                        {
                        const auto quoteChar{ *start };
                        ++start;
                        auto endOfTag =
                            string_util::find_unescaped_char_same_line_n(start, quoteChar, std::distance(start, endSentinel));
                        if (endOfTag == nullptr || (endOfTag >= endSentinel))
                            {
                            log_message(L"Bad 'r dropcap' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]] auto retval = m_subParser->operator()(
                            { start, static_cast<size_t>(std::distance(start, endOfTag)) });
                        add_character(L'"');
                        add_characters(
                            { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                        add_character(L'"');
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                            }
                        endOfTag = string_util::find_unescaped_char_same_line_n(start, L'`', std::distance(start, endSentinel));
                        if (endOfTag == nullptr || (endOfTag >= endSentinel))
                            {
                            log_message(L"Bad 'r dropcap' code block in markdown file.");
                            break;
                            }
                        start = ++endOfTag;
                        }
                    continue;
                    }
                else if (std::wcsncmp(start, L"`r menu(", 8) == 0)
                    {
                    start += 8;
                    if (start + 1 < endSentinel &&
                        (start[0] == L'c') &&
                        (start[1] == L'('))
                        { start += 2; }
                    if (*start == L'\'' || *start == L'"')
                        {
                        auto endOfTag = std::wcsstr(start, L")`");
                        if (endOfTag == nullptr)
                            {
                            log_message(L"Bad 'r menu' code block in markdown file.");
                            break;
                            }
                        [[maybe_unused]] auto retval = m_subParser->operator()(
                            { start, static_cast<size_t>(std::distance(start, endOfTag-1)) });
                        add_characters(
                            { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                            }
                        start = endOfTag + 2;
                        }
                    continue;
                    }
                // read content as-is otherwise
                else
                    {
                    if (std::wcsncmp(start, L"`r ", 3) == 0)
                        { start += 3; }
                    else if (std::wcsncmp(start, L"`python ", 8) == 0)
                        { start += 8; }
                    else
                        {
                        ++start;
                        }
                    // `` section, which can have embedded backticks
                    if (*start == '`')
                        {
                        auto endOfTag = std::wcsstr(++start, L"``");
                        if (endOfTag == nullptr ||
                            endOfTag > endSentinel)
                            {
                            log_message(L"Bad inline `` code block in markdown file.");
                            break;
                            }
                        // read in content verbatim
                        while (start < endOfTag)
                            {
                            previousChar = *start;
                            add_character(*start);
                            ++start;
                            }
                        start += 2;
                        }
                    // just a single backtick block, should be on one line
                    else
                        {
                        while (start < endSentinel && *start != '`')
                            {
                            // inline blocks should be on one line, so bail if we hit a new line
                            // as this would probably be a missing closing backtick
                            if (*start == L'\n' || *start == L'\r')
                                {
                                log_message(L"Unterminated inline ` code block in markdown file.");
                                previousChar = *start;
                                add_character(*start);
                                ++start;
                                break;
                                }
                            previousChar = *start;
                            add_character(*start);
                            ++start;
                            }
                        if (*start == '`')
                            {
                            ++start;
                            }
                        }
                    continue;
                    }
                }
            }
        // images (we don't read in the alt text inside of the [], just skip everything)
        else if (*start == L'!')
            {
            if (!isEscaping &&
                (start +1 < endSentinel) &&
                start[1] == L'[')
                {
                start += 2;
                auto endOfTag =
                    string_util::find_unescaped_matching_close_tag_same_line_n(start, L'[', L']',
                        std::distance(start, endSentinel));
                if (endOfTag == nullptr || (endOfTag >= endSentinel))
                    {
                    log_message(L"Bad image command in markdown file.");
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                start = ++endOfTag;
                if (*start == L'(')
                    {
                    ++start;
                    endOfTag =
                        string_util::find_unescaped_matching_close_tag_same_line_n(start, L'(', L')',
                            std::distance(start, endSentinel));
                    if (endOfTag == nullptr || (endOfTag >= endSentinel))
                        {
                        log_message(L"Bad image command in markdown file.");
                        previousChar = L'(';
                        add_character(L'(');
                        continue;
                        }
                    start = ++endOfTag;
                    }
                continue;
                }
            }
        // links
        else if (*start == L'[')
            {
            if (!isEscaping)
                {
                auto labelStart{ ++start };
                auto endOfTag =
                    string_util::find_unescaped_matching_close_tag_same_line_n(start, L'[', L']',
                        std::distance(start, endSentinel));
                if (endOfTag == nullptr || (endOfTag >= endSentinel))
                    {
                    log_message(L"Bad link command in markdown file. Missing closing ']'.");
                    // just treat it like a stray '[' and keep going
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                start = ++endOfTag;
                if (*start == L'(')
                    {
                    auto labelEnd{ start - 1};
                    ++start;
                    endOfTag =
                        string_util::find_unescaped_matching_close_tag_same_line_n(start, L'(', L')',
                            std::distance(start, endSentinel));
                    if (endOfTag == nullptr || (endOfTag >= endSentinel))
                        {
                        log_message(L"Bad link command in markdown file. Missing closing ')'.");
                        // read in the label and '(' section after it as-is if closing ')' is missing
                        start = labelStart;
                        previousChar = L'[';
                        add_character(L'[');
                        continue;
                        }
                    start = ++endOfTag;
                    if (labelStart < labelEnd)
                        {
                        [[maybe_unused]] auto retval = m_subParser->operator()(
                            { labelStart, static_cast<size_t>(std::distance(labelStart, labelEnd)) });
                        add_characters(
                            { m_subParser->get_filtered_text(), m_subParser->get_filtered_text_length() });
                        if (m_subParser->get_filtered_text_length())
                            {
                            previousChar =
                                m_subParser->get_filtered_text()[m_subParser->get_filtered_text_length() - 1];
                            }
                        }
                    }
                else
                    {
                    log_message(L"Bad link command in markdown file. Missing '()' section.");
                    // read in the label and '(' section after it as-is if closing ')' is missing
                    start = labelStart;
                    previousChar = L'[';
                    add_character(L'[');
                    continue;
                    }
                continue;
                }
            }
        // IDs
        else if (*start == L'{')
            {
            if (!isEscaping)
                {
                // if quarto syntax
                if (std::wcsncmp(start, QUARTO_PAGEBREAK.data(), QUARTO_PAGEBREAK.length()) == 0)
                    {
                    start += QUARTO_PAGEBREAK.length();
                    add_characters(L"\n\n");
                    }
                else
                    {
                    ++++start;
                    auto endOfTag =
                        string_util::find_unescaped_matching_close_tag_same_line_n(start, L'{', L'}',
                            std::distance(start, endSentinel));
                    if (endOfTag == nullptr || (endOfTag >= endSentinel))
                        {
                        log_message(L"Bad ID command in markdown file.");
                        break;
                        }
                    start = ++endOfTag;
                    }
                continue;
                }
            }
        // superscript (just read as-is)
        else if (!isEscaping &&
                 *start == L'^')
            {
            ++start;
            continue;
            }
        // RMarkdown (Pandoc) comment
        else if (!isEscaping &&
            std::wcsncmp(start, L"<!--", 4) == 0)
            {
            const auto endOfTag = std::wcsstr(start, L"-->");
            if (endOfTag == nullptr)
                {
                log_message(L"Bad comment block in markdown file.");
                break;
                }
            start = endOfTag + 3;
            continue;
            }
        // newline hacks found in tables (just replace with space to keep the table structure).
        else if (!isEscaping &&
            std::wcsncmp(start, L"<br>\\linebreak", 14) == 0)
            {
            start += 14;
            previousChar = L' ';
            add_character(L' ');
            continue;
            }
        // HTML newline
        else if (!isEscaping &&
            std::wcsncmp(start, L"<br>", 4) == 0)
            {
            start += 4;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping &&
            std::wcsncmp(start, L"<br/>", 5) == 0)
            {
            start += 5;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (!isEscaping &&
            (std::wcsncmp(start, L"< br/>", 6) == 0 ||
             std::wcsncmp(start, L"<br />", 6) == 0))
            {
            start += 6;
            previousChar = L'\n';
            add_characters(L"\n\n");
            continue;
            }
        else if (*start == L'<')
            {
            if (!isEscaping &&
                start + 1 < endSentinel &&
                (start[1] == L'/' ||
                 start[1] == L'p' ||
                 std::wcsncmp(start + 1, L"a ", 2) == 0 ||
                 std::wcsncmp(start + 1, L"b>", 2) == 0 ||
                 std::wcsncmp(start + 1, L"i>", 2) == 0 ||
                 std::wcsncmp(start + 1, L"u>", 2) == 0 ||
                 std::wcsncmp(start + 1, L"div", 3) == 0 ||
                 std::wcsncmp(start + 1, L"dl>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"dt>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"dd>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"em>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"tt>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"ul>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"ol>", 3) == 0 ||
                 std::wcsncmp(start + 1, L"li>", 3) == 0) )
                {
                ++start;
                auto endOfTag =
                        string_util::find_unescaped_matching_close_tag_same_line_n(start, L'<', L'>',
                            std::distance(start, endSentinel));
                if (endOfTag == nullptr)
                    {
                    log_message(L"Bad <> pair in markdown file.");
                    break;
                    }
                start = ++endOfTag;
                continue;
                }
            }
        // newlines
        else if (*start == L'\n' || *start == L'\r')
            {
            // two (or more) spaces at the end of a line indicates a paragraph break
            size_t newlineCount{ 0 };
            if (previousChar == L' ' &&
                std::distance(md_text.data(), start) > 2 &&
                *(start-2) == L' ')
                { ++newlineCount; }
            // count the newlines (taking CRLF combos into account)
            while ((*start == L'\n' || *start == L'\r') &&
                    (start < endSentinel))
                {
                if (*start == L'\r' &&
                    (start + 1 < endSentinel) &&
                    start[1] == L'\n')
                    {
                    ++start;
                    continue;
                    }
                ++newlineCount;
                ++start;
                // If the next line is a header line divider, then
                // skip that, switch to header mode, and keep reading
                // any more newlines
                if (start + 1 < endSentinel &&
                    ((start[0] == L'=' && start[1] == L'=') ||
                    (start[0] == L'-' && start[1] == L'-')) )
                    {
                    while ((*start == L'=' || *start == L'-') &&
                        (start < endSentinel))
                        { ++start; }
                    headerMode = true;
                    }
                }

            auto scanAhead{ start };
            size_t leadingScaces{ 0 };
            while ((scanAhead < endSentinel) &&
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
                add_character(L'\n', newlineCount);
                previousChar = L'\n';
                }
            // same for an ordered list
            else if (newlineCount == 1 &&
                (start < endSentinel) &&
                std::iswdigit(*start))
                {
                auto scanAheadDigit{ start };
                while ((scanAheadDigit < endSentinel) &&
                    std::iswdigit(*scanAheadDigit))
                    { ++scanAheadDigit; }
                if (*scanAheadDigit == L'.')
                    {
                    add_character(L'\n', newlineCount);
                    previousChar = L'\n';
                    }
                // not an ordered list, default behavior to read as space
                else
                    {
                    add_character(L' ');
                    previousChar = L' ';
                    }
                }
            // a single newline not a end of a self-contained line
            // (e.g., a header) is seen as a space
            else if (newlineCount == 1)
                {
                add_character(L' ');
                previousChar = L' ';
                }
            else
                {
                add_character(L'\n', newlineCount);
                previousChar = L'\n';
                }
            headerMode = false;
            isEscaping = false;
            continue;
            }
        // styling tags that just get removed from raw text
        else if (!isEscaping &&
                 std::iswspace(previousChar) &&
                 *start == L'*')
            {
            parseStyledText(L'*');
            continue;
             }
        else if (!isEscaping &&
                 std::iswspace(previousChar) &&
                 *start == L'_')
            {
            parseStyledText(L'_');
            continue;
            }
        else if (!isEscaping &&
                 std::iswspace(previousChar) &&
                 *start == L'~')
            {
            parseStyledText(L'~');
            continue;
            }
        // table
        else if (!isEscaping &&
                 (*start == L'|') )
            {
            previousChar = L'|';
            add_characters(L"\t|");
            ++start;
            continue;
            }
        // turn off escaping and load the character
        isEscaping = false;
        if (start < endSentinel)
            {
            previousChar = *start;
            add_character(*start);
            ++start;
            }
        else
            { break; }
        }

    return get_filtered_text();
    }
