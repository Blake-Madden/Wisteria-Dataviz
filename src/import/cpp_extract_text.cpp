///////////////////////////////////////////////////////////////////////////////
// Name:        cpp_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "cpp_extract_text.h"

using namespace lily_of_the_valley;

//-----------------------------------------
const wchar_t* cpp_extract_text::operator()(const wchar_t* cpp_text, const size_t text_length)
    {
    clear_log();
    clear();
    m_author.clear();
    if (cpp_text == nullptr || text_length == 0)
        {
        return nullptr;
        }
    assert(std::wcslen(cpp_text) == text_length);
    const wchar_t* const endSentinel = cpp_text + text_length;
    allocate_text_buffer(text_length);

    while (std::next(cpp_text, 2) < endSentinel && *cpp_text != 0)
        {
        // if a comment...
        if (*cpp_text == L'/')
            {
            // see if a doxygen block comment (/**comment*/ or /*!comment*/)
            // (or simple comment if all are being included).
            if (cpp_text[1] == L'*' && (is_including_all_comments() ||
                                        string_util::is_either<wchar_t>(cpp_text[2], L'*', L'!')))
                {
                std::advance(cpp_text, 1);
                while (cpp_text < endSentinel &&
                       string_util::is_either<wchar_t>(*cpp_text, L'*', L'!'))
                    {
                    std::advance(cpp_text, 1);
                    }
                // skip over empty comments
                if (cpp_text < endSentinel && *(cpp_text - 1) == L'*' && *cpp_text == L'/')
                    {
                    std::advance(cpp_text, 1);
                    continue;
                    }
                if (const wchar_t* const end = std::wcsstr(cpp_text, L"*/");
                    end && end < endSentinel)
                    {
                    add_characters_strip_markup(cpp_text, end - cpp_text);
                    add_characters({ L"\n\n", 2 });
                    cpp_text = std::next(end, 2);
                    }
                // can't find ending tag, so just read in the rest of the text
                else
                    {
                    add_characters_strip_markup(cpp_text, endSentinel - cpp_text);
                    trim();
                    return get_filtered_text();
                    }
                }
            // or a single line comment
            else if (cpp_text[1] == L'/' &&
                     (is_including_all_comments() ||
                      string_util::is_either<wchar_t>(cpp_text[2], L'/', L'!')))
                {
                while (cpp_text < endSentinel &&
                       string_util::is_either<wchar_t>(*cpp_text, L'/', L'!'))
                    {
                    std::advance(cpp_text, 1);
                    }
                const size_t end = std::wcscspn(cpp_text, L"\n\r");
                add_characters_strip_markup(cpp_text, end);
                add_character(L'\n');
                cpp_text += end;
                /*  Scan ahead and see if the next line is another comment.
                    If so, then we will allow these lines to be in the same paragraph; however,
                    if there is code after this comment then it must be a separate paragraph.*/
                const wchar_t* scanAhead = cpp_text;
                while (scanAhead < endSentinel && string_util::is_one_of(*scanAhead, L" \t\n\r"))
                    {
                    ++scanAhead;
                    }
                if (scanAhead < endSentinel && *scanAhead != L'/')
                    {
                    add_character(L'\n');
                    }
                }
            else
                {
                std::advance(cpp_text, 1);
                }
            }
        // ...or gettext resources
        else if (*cpp_text == L'_' && cpp_text[1] == L'(')
            {
            if (endSentinel && cpp_text[2] == L'\"')
                {
                std::advance(cpp_text, 3);
                }
            else if (cpp_text + 3 < endSentinel && cpp_text[2] == L'L' && cpp_text[3] == L'\"')
                {
                std::advance(cpp_text, 4);
                }
            else if (cpp_text + 4 < endSentinel && cpp_text[2] == L'L' && cpp_text[3] == L'R' &&
                     cpp_text[4] == L'\"')
                {
                std::advance(cpp_text, 5);
                }
            else
                {
                std::advance(cpp_text, 1);
                continue;
                }
            const wchar_t* const end = string_util::find_unescaped_char(cpp_text, L'\"');
            if (end && end < endSentinel)
                {
                add_characters_strip_escapes(cpp_text, end - cpp_text);
                add_characters({ L"\n\n", 2 });
                cpp_text = std::next(end);
                }
            else
                {
                break;
                }
            }
        // if a quote, then make sure we don't pick up what looks like comments later inside it
        else if (*cpp_text == L'\"')
            {
            cpp_text = string_util::find_unescaped_char(++cpp_text, L'\"');
            if (!cpp_text || cpp_text >= endSentinel)
                {
                break;
                }
            std::advance(cpp_text, 1);
            }
        else
            {
            std::advance(cpp_text, 1);
            }
        }
    trim();
    return get_filtered_text();
    }

//-----------------------------------------
void cpp_extract_text::add_characters_strip_escapes(const wchar_t* characters, const size_t length)
    {
    assert(characters);
    if (characters == nullptr || length == 0)
        {
        return;
        }
    const auto* start = characters;
    const auto* end = std::next(characters, length);
    size_t currentBlockSize = 0;
    for (size_t i = 0; i < length - 1; /*in loop*/)
        {
        if (*std::next(characters, i) == L'\\')
            {
            if (*std::next(characters, i + 1) == L'n' || *std::next(characters, i + 1) == L'r')
                {
                add_characters({ start, currentBlockSize });
                add_character(L'\n');
                std::advance(start, currentBlockSize + 2); // skip '\n'
                i += 2;
                currentBlockSize = 0;
                continue;
                }
            if (*std::next(characters, i + 1) == L't')
                {
                add_characters({ start, currentBlockSize });
                add_character(L'\t');
                std::advance(start, currentBlockSize + 2); // skip '\t'
                i += 2;
                currentBlockSize = 0;
                continue;
                }
            // copy over what is being escaped
            // if escaping an escape character, then copy that over now
            // so that it doesn't get lost in the next loop
            if (*std::next(characters, i + 1) == L'\\')
                {
                add_characters({ start, currentBlockSize + 1 });
                std::advance(start, currentBlockSize + 2); // skip escape character
                i += 2;
                }
            // or just skip escape char and feed in the next char in the next loop
            else
                {
                add_characters({ start, currentBlockSize });
                std::advance(start, currentBlockSize + 1); // skip escape character
                ++i;
                }
            currentBlockSize = 0;
            continue;
            }
        ++i;
        ++currentBlockSize;
        }
    // add final block of text
    assert(start <= end);
    if (end > start)
        {
        add_characters({ start, static_cast<size_t>(end - start) });
        }
    }

//-----------------------------------------
void cpp_extract_text::add_characters_strip_markup(const wchar_t* cpp_text,
                                                   const size_t text_length)
    {
    assert(cpp_text);
    if (cpp_text == nullptr || text_length == 0)
        {
        return;
        }
    const wchar_t* const endSentinel = std::next(cpp_text, text_length);
    const wchar_t* const startSentinel = cpp_text;
    // step over any whitespace
    while (cpp_text < endSentinel && string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
        {
        std::advance(cpp_text, 1);
        }
    const wchar_t* startPos = cpp_text;
    while (cpp_text < endSentinel && *cpp_text != 0)
        {
        if (string_util::is_either<wchar_t>(*cpp_text, L'\n', L'\r'))
            {
            // step over the newlines, we will copy the previous text and these newlines
            while (cpp_text < endSentinel &&
                   string_util::is_either<wchar_t>(*cpp_text, L'\n', L'\r'))
                {
                std::advance(cpp_text, 1);
                }
            add_characters({ startPos, static_cast<size_t>(cpp_text - startPos) });
            // skip any space in front of this line
            while (cpp_text < endSentinel &&
                   string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
                {
                std::advance(cpp_text, 1);
                }
            startPos = cpp_text;
            }
        // before handling doxygen @ and \ symbols, make sure they aren't part of a file path
        else if (string_util::is_either<wchar_t>(*cpp_text, L'@', L'\\') &&
                 (startSentinel == cpp_text || string_util::is_one_of(*(cpp_text - 1), L" \t\n\r")))
            {
            if (*cpp_text == L'\\' && cpp_text + 1 < endSentinel &&
                string_util::is_either<wchar_t>(cpp_text[1], L'\'', L'\"'))
                {
                std::advance(cpp_text, 1);
                continue;
                }
            // copy over any text before the current @ or \ tag
            add_characters({ startPos, static_cast<size_t>(cpp_text - startPos) });
            std::advance(cpp_text, 1);
            const wchar_t* tagEnd = cpp_text;
            while (*tagEnd != 0 && tagEnd < endSentinel &&
                   (is_valid_char(*tagEnd) || string_util::is_either(*tagEnd, L'{', L'}')))
                {
                std::advance(tagEnd, 1);
                }
            if (*tagEnd == 0 || tagEnd >= endSentinel)
                {
                startPos = endSentinel;
                break;
                }
            const std::wstring doxygenTag(cpp_text, tagEnd - cpp_text);
            // param tag
            if (doxygenTag == L"param" || doxygenTag == L"tparam")
                {
                add_character(L'\n');
                std::advance(cpp_text, doxygenTag.length());
                while (cpp_text < endSentinel &&
                       string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
                    {
                    std::advance(cpp_text, 1);
                    }
                // skip any "[in,out]" argument
                if (*cpp_text == L'[')
                    {
                    const wchar_t* const endBracket = std::wcschr(cpp_text, L']');
                    if (endBracket != nullptr && endBracket < endSentinel)
                        {
                        cpp_text = std::next(endBracket);
                        while (cpp_text < endSentinel &&
                               string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
                            {
                            std::advance(cpp_text, 1);
                            }
                        }
                    }
                const wchar_t* const paramLabel = cpp_text;
                // read in the param name and add a colon after it
                while (cpp_text < endSentinel && is_valid_char(*cpp_text))
                    {
                    std::advance(cpp_text, 1);
                    }
                add_characters({ paramLabel, static_cast<size_t>(cpp_text - paramLabel) });
                add_character(L':');

                startPos = cpp_text;
                }
            /* Tags that should be skipped (i.e., not copied into the text) and that should
               also have a newline added before and after their text.*/
            else if (m_doxygen_tags_single_line.contains(doxygenTag))
                {
                add_character(L'\n');
                std::advance(cpp_text, doxygenTag.length());
                // scan over space(s)
                while (cpp_text < endSentinel &&
                       string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
                    {
                    std::advance(cpp_text, 1);
                    }
                startPos = cpp_text;
                const size_t end = std::wcscspn(cpp_text, L"\n\r");
                if (cpp_text + end < endSentinel)
                    {
                    cpp_text += end;
                    add_characters({ startPos, static_cast<size_t>(cpp_text - startPos) });
                    add_character(L'\n');
                    startPos = cpp_text;
                    }
                continue;
                }
            // tags that we want to skip over, but preserve the surrounding text, just step over it
            else if (m_doxygen_tags.contains(doxygenTag))
                {
                std::advance(cpp_text, doxygenTag.length());
                // scan over space(s)
                while (cpp_text < endSentinel &&
                       string_util::is_either<wchar_t>(*cpp_text, L' ', L'\t'))
                    {
                    std::advance(cpp_text, 1);
                    }
                startPos = cpp_text;
                }
            else if (doxygenTag == L"htmlonly")
                {
                // scan until we get to a space (skipping the tag)
                std::advance(cpp_text, doxygenTag.length());
                startPos = cpp_text;
                // go to the end of the HTML block
                const wchar_t* const endBlock =
                    string_util::strnistr(cpp_text, L"endhtmlonly", endSentinel - cpp_text);
                if (endBlock != nullptr && endBlock < endSentinel)
                    {
                    if (html_extract(cpp_text, (endBlock - 1) - cpp_text, true, false) != nullptr)
                        {
                        add_characters({ html_extract.get_filtered_text(),
                                         html_extract.get_filtered_text_length() });
                        }
                    add_character(L'\n');
                    startPos = cpp_text = std::next(endBlock, 11);
                    }
                }
            // ...or a tag name that we want to copy over as part of the text
            else
                {
                const bool authorCommand = (doxygenTag == L"author" || doxygenTag == L"authors");
                const bool singleLineCommand =
                    (authorCommand || doxygenTag == L"date" || doxygenTag == L"copyright" ||
                     doxygenTag == L"version");

                // step over command
                startPos = cpp_text;
                std::advance(cpp_text, doxygenTag.length());
                add_characters({ startPos, static_cast<size_t>(cpp_text - startPos) });
                // if a recognized command
                if (singleLineCommand)
                    {
                    add_character(L':');
                    }
                startPos = cpp_text;
                const size_t end = std::wcscspn(cpp_text, L"\n\r");
                if (cpp_text + end < endSentinel)
                    {
                    std::advance(cpp_text, end);
                    add_characters({ startPos, static_cast<size_t>(cpp_text - startPos) });
                    if (authorCommand)
                        {
                        m_author.assign(startPos, static_cast<size_t>(cpp_text - startPos));
                        string_util::trim(m_author);
                        }
                    if (singleLineCommand)
                        {
                        add_character(L'\n');
                        }
                    startPos = cpp_text;
                    }
                continue;
                }
            }
        else
            {
            std::advance(cpp_text, 1);
            }
        }
    // add any remaining text
    add_characters({ startPos, static_cast<size_t>(endSentinel - startPos) });
    }
