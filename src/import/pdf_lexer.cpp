///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_lexer.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_lexer.h"
#include <charconv>
#include <tuple>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    bool pdf_lexer::is_whitespace(const char character)
        {
        return (character == 0 || character == '\t' || character == '\n' || character == '\f' ||
                character == '\r' || character == ' ');
        }

    //------------------------------------------------------------------
    bool pdf_lexer::is_delimiter(const char character)
        {
        return (character == '(' || character == ')' || character == '<' || character == '>' ||
                character == '[' || character == ']' || character == '{' || character == '}' ||
                character == '/' || character == '%');
        }

    //------------------------------------------------------------------
    bool pdf_lexer::is_token_end(const char character)
        {
        return (pdf_lexer::is_whitespace(character) || pdf_lexer::is_delimiter(character));
        }

    //------------------------------------------------------------------
    bool pdf_lexer::is_digit(const char character)
        {
        return (character >= '0' && character <= '9');
        }

    //------------------------------------------------------------------
    int pdf_lexer::hex_digit_value(const char character)
        {
        if (character >= '0' && character <= '9')
            {
            return character - '0';
            }
        if (character >= 'A' && character <= 'F')
            {
            return (character - 'A') + 10;
            }
        if (character >= 'a' && character <= 'f')
            {
            return (character - 'a') + 10;
            }
        return -1;
        }

    //------------------------------------------------------------------
    void pdf_lexer::skip_whitespace(const std::string_view source, size_t& pos)
        {
        while (pos < source.length())
            {
            if (pdf_lexer::is_whitespace(source[pos]))
                {
                ++pos;
                }
            else if (source[pos] == '%')
                {
                // comment; skip to the end of the line
                while (pos < source.length() && source[pos] != '\n' && source[pos] != '\r')
                    {
                    ++pos;
                    }
                }
            else
                {
                break;
                }
            }
        }

    //------------------------------------------------------------------
    std::string_view pdf_lexer::trim(std::string_view value)
        {
        while (!value.empty() && pdf_lexer::is_whitespace(value.front()))
            {
            value.remove_prefix(1);
            }
        while (!value.empty() && pdf_lexer::is_whitespace(value.back()))
            {
            value.remove_suffix(1);
            }
        return value;
        }

    //------------------------------------------------------------------
    bool pdf_lexer::read_int(const std::string_view source, size_t& pos, long& valueOut)
        {
        pdf_lexer::skip_whitespace(source, pos);
        bool negative{ false };
        if (pos < source.length() && (source[pos] == '+' || source[pos] == '-'))
            {
            negative = (source[pos] == '-');
            ++pos;
            }
        // the digit run is parsed separately from the sign, since std::from_chars would
        // otherwise apply the sign itself, causing it to be double-applied below
        const size_t digitStart{ pos };
        while (pos < source.length() && pdf_lexer::is_digit(source[pos]))
            {
            ++pos;
            }
        if (pos == digitStart)
            {
            return false;
            }
        long magnitude{ 0 };
        const auto convResult =
            std::from_chars(source.data() + digitStart, source.data() + pos, magnitude);
        if (convResult.ec != std::errc{})
            {
            return false;
            }
        valueOut = negative ? -magnitude : magnitude;
        return true;
        }

    //------------------------------------------------------------------
    bool pdf_lexer::to_int(const std::string_view token, long& valueOut)
        {
        size_t pos{ 0 };
        return (!token.empty() && pdf_lexer::read_int(token, pos, valueOut) &&
                pos == token.length());
        }

    //------------------------------------------------------------------
    std::string pdf_lexer::read_literal_string(const std::string_view source, size_t& pos)
        {
        if (pos >= source.length() || source[pos] != '(')
            {
            return {};
            }
        ++pos;
        int parenDepth{ 1 };
        std::string bytes;
        while (pos < source.length())
            {
            const char curChar{ source[pos] };
            if (curChar == '\\')
                {
                ++pos;
                if (pos >= source.length())
                    {
                    break;
                    }
                const char escChar{ source[pos] };
                if (escChar == 'n')
                    {
                    bytes += '\n';
                    ++pos;
                    }
                else if (escChar == 'r')
                    {
                    bytes += '\r';
                    ++pos;
                    }
                else if (escChar == 't')
                    {
                    bytes += '\t';
                    ++pos;
                    }
                else if (escChar == 'b')
                    {
                    bytes += '\b';
                    ++pos;
                    }
                else if (escChar == 'f')
                    {
                    bytes += '\f';
                    ++pos;
                    }
                else if (escChar == '(' || escChar == ')' || escChar == '\\')
                    {
                    bytes += escChar;
                    ++pos;
                    }
                // an escaped EOL is a line continuation (ignored)
                else if (escChar == '\r' || escChar == '\n')
                    {
                    ++pos;
                    if (escChar == '\r' && pos < source.length() && source[pos] == '\n')
                        {
                        ++pos;
                        }
                    }
                // octal character code (1-3 digits)
                else if (escChar >= '0' && escChar <= '7')
                    {
                    int octalValue{ 0 };
                    size_t digitCount{ 0 };
                    while (digitCount < 3 && pos < source.length() && source[pos] >= '0' &&
                           source[pos] <= '7')
                        {
                        octalValue = (octalValue * 8) + (source[pos] - '0');
                        ++pos;
                        ++digitCount;
                        }
                    bytes += static_cast<char>(octalValue & 0xFF);
                    }
                else
                    {
                    // unknown escape; take the character as-is
                    bytes += escChar;
                    ++pos;
                    }
                }
            else if (curChar == '(')
                {
                ++parenDepth;
                bytes += curChar;
                ++pos;
                }
            else if (curChar == ')')
                {
                --parenDepth;
                ++pos;
                if (parenDepth == 0)
                    {
                    break;
                    }
                bytes += curChar;
                }
            // a bare (unescaped) end-of-line marker is normalized to a single '\n',
            // regardless of whether it was a CR, LF, or CRLF pair
            else if (curChar == '\r')
                {
                bytes += '\n';
                ++pos;
                if (pos < source.length() && source[pos] == '\n')
                    {
                    ++pos;
                    }
                }
            else
                {
                bytes += curChar;
                ++pos;
                }
            }
        return bytes;
        }

    //------------------------------------------------------------------
    std::string pdf_lexer::read_hex_string(const std::string_view source, size_t& pos)
        {
        std::string bytes;
        if (pos >= source.length() || source[pos] != '<')
            {
            return bytes;
            }
        ++pos;
        int highNibble{ -1 };
        while (pos < source.length() && source[pos] != '>')
            {
            const int digitValue{ pdf_lexer::hex_digit_value(source[pos]) };
            if (digitValue >= 0)
                {
                if (highNibble < 0)
                    {
                    highNibble = digitValue;
                    }
                else
                    {
                    bytes += static_cast<char>((highNibble << 4) | digitValue);
                    highNibble = -1;
                    }
                }
            ++pos;
            }
        // an odd trailing digit is treated as if followed by a zero
        if (highNibble >= 0)
            {
            bytes += static_cast<char>(highNibble << 4);
            }
        if (pos < source.length())
            {
            // step over '>'
            ++pos;
            }
        return bytes;
        }

    //------------------------------------------------------------------
    std::string_view pdf_lexer::read_value(const std::string_view source, size_t& pos,
                                           const int depth)
        {
        pdf_lexer::skip_whitespace(source, pos);
        const size_t startPos{ pos };
        if (pos >= source.length() || depth > 64)
            {
            return {};
            }
        // dictionary
        if (source.compare(pos, 2, "<<") == 0)
            {
            pos += 2;
            while (pos < source.length())
                {
                pdf_lexer::skip_whitespace(source, pos);
                if (source.compare(pos, 2, ">>") == 0)
                    {
                    pos += 2;
                    break;
                    }
                if (pos >= source.length() || pdf_lexer::read_value(source, pos, depth + 1).empty())
                    {
                    break;
                    }
                }
            }
        // hex string
        else if (source[pos] == '<')
            {
            std::ignore = pdf_lexer::read_hex_string(source, pos);
            }
        // literal string
        else if (source[pos] == '(')
            {
            std::ignore = pdf_lexer::read_literal_string(source, pos);
            }
        // array
        else if (source[pos] == '[')
            {
            ++pos;
            while (pos < source.length())
                {
                pdf_lexer::skip_whitespace(source, pos);
                if (pos < source.length() && source[pos] == ']')
                    {
                    ++pos;
                    break;
                    }
                if (pos >= source.length() || pdf_lexer::read_value(source, pos, depth + 1).empty())
                    {
                    break;
                    }
                }
            }
        // name
        else if (source[pos] == '/')
            {
            ++pos;
            while (pos < source.length() && !pdf_lexer::is_token_end(source[pos]))
                {
                ++pos;
                }
            }
        // stray delimiter (malformed document); step over it
        else if (pdf_lexer::is_delimiter(source[pos]))
            {
            ++pos;
            }
        // number or keyword
        else
            {
            while (pos < source.length() && !pdf_lexer::is_token_end(source[pos]))
                {
                ++pos;
                }
            // if this was an integer, look ahead for "GEN R" (an indirect reference)
            bool allDigits{ pos > startPos };
            for (size_t digitPos = startPos; digitPos < pos; ++digitPos)
                {
                if (!pdf_lexer::is_digit(source[digitPos]))
                    {
                    allDigits = false;
                    break;
                    }
                }
            if (allDigits)
                {
                size_t probePos{ pos };
                pdf_lexer::skip_whitespace(source, probePos);
                const size_t genStart{ probePos };
                while (probePos < source.length() && pdf_lexer::is_digit(source[probePos]))
                    {
                    ++probePos;
                    }
                if (probePos > genStart)
                    {
                    pdf_lexer::skip_whitespace(source, probePos);
                    if (probePos < source.length() && source[probePos] == 'R' &&
                        (probePos + 1 == source.length() ||
                         pdf_lexer::is_token_end(source[probePos + 1])))
                        {
                        pos = probePos + 1;
                        }
                    }
                }
            }
        return source.substr(startPos, pos - startPos);
        }

    //------------------------------------------------------------------
    std::string_view pdf_lexer::find_dictionary_value(const std::string_view dictionary,
                                                      const std::string_view keyName)
        {
        if (dictionary.compare(0, 2, "<<") != 0)
            {
            return {};
            }
        size_t pos{ 2 };
        while (pos < dictionary.length())
            {
            pdf_lexer::skip_whitespace(dictionary, pos);
            if (dictionary.compare(pos, 2, ">>") == 0 || pos >= dictionary.length())
                {
                break;
                }
            if (dictionary[pos] != '/')
                {
                // malformed entry; step over one value and try again
                if (pdf_lexer::read_value(dictionary, pos).empty())
                    {
                    break;
                    }
                continue;
                }
            ++pos;
            const size_t keyStart{ pos };
            while (pos < dictionary.length() && !pdf_lexer::is_token_end(dictionary[pos]))
                {
                ++pos;
                }
            const std::string_view currentKey{ dictionary.substr(keyStart, pos - keyStart) };
            const std::string_view value{ pdf_lexer::read_value(dictionary, pos) };
            if (currentKey == keyName)
                {
                return value;
                }
            if (value.empty())
                {
                break;
                }
            }
        return {};
        }

    //------------------------------------------------------------------
    std::vector<std::pair<std::string_view, std::string_view>>
    pdf_lexer::get_dictionary_entries(const std::string_view dictionary)
        {
        std::vector<std::pair<std::string_view, std::string_view>> entries;
        if (dictionary.compare(0, 2, "<<") != 0)
            {
            return entries;
            }
        size_t pos{ 2 };
        while (pos < dictionary.length())
            {
            pdf_lexer::skip_whitespace(dictionary, pos);
            if (dictionary.compare(pos, 2, ">>") == 0 || pos >= dictionary.length())
                {
                break;
                }
            if (dictionary[pos] != '/')
                {
                if (pdf_lexer::read_value(dictionary, pos).empty())
                    {
                    break;
                    }
                continue;
                }
            ++pos;
            const size_t keyStart{ pos };
            while (pos < dictionary.length() && !pdf_lexer::is_token_end(dictionary[pos]))
                {
                ++pos;
                }
            const std::string_view currentKey{ dictionary.substr(keyStart, pos - keyStart) };
            const std::string_view value{ pdf_lexer::read_value(dictionary, pos) };
            if (value.empty())
                {
                break;
                }
            entries.emplace_back(currentKey, value);
            }
        return entries;
        }

    //------------------------------------------------------------------
    bool pdf_lexer::get_reference(std::string_view value, long& objectNumberOut)
        {
        value = pdf_lexer::trim(value);
        size_t pos{ 0 };
        long objectNumber{ 0 }, generationNumber{ 0 };
        if (!pdf_lexer::read_int(value, pos, objectNumber))
            {
            return false;
            }
        if (!pdf_lexer::read_int(value, pos, generationNumber))
            {
            return false;
            }
        pdf_lexer::skip_whitespace(value, pos);
        if (pos + 1 == value.length() && value[pos] == 'R')
            {
            objectNumberOut = objectNumber;
            return true;
            }
        return false;
        }
    } // namespace lily_of_the_valley
