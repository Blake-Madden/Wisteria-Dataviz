///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_text_decoder.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_extract_text.h"
#include <algorithm>
#include <array>
#include <cstdint>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    // text decoding helpers
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    wchar_t pdf_text_decoder::cp1252_to_unicode(const unsigned char byteValue)
        {
        // 0x80-0x9F are the only values that differ from Latin-1
        static constexpr std::array<wchar_t, 32> cp1252Table{
            0x20AC, 0x0081, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021, 0x02C6, 0x2030, 0x0160,
            0x2039, 0x0152, 0x008D, 0x017D, 0x008F, 0x0090, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022,
            0x2013, 0x2014, 0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0x009D, 0x017E, 0x0178
        };
        if (byteValue >= 0x80 && byteValue <= 0x9F)
            {
            return cp1252Table[byteValue - 0x80];
            }
        return static_cast<wchar_t>(byteValue);
        }

    //------------------------------------------------------------------
    wchar_t pdf_text_decoder::mac_roman_to_unicode(const unsigned char byteValue)
        {
        // Unlike WinAnsi/CP1252, MacRoman diverges from Latin-1 across the entire
        // 0x80-0xFF range, so (unlike cp1252_to_unicode) the full range is tabulated.
        // A 0 entry means the code point is undefined in this encoding
        static constexpr std::array<wchar_t, 128> macRomanTable{
            0x00C4, 0x00C5, 0x00C7, 0x00C9, 0x00D1, 0x00D6, 0x00DC, 0x00E1, 0x00E0, 0x00E2, 0x00E4,
            0x00E3, 0x00E5, 0x00E7, 0x00E9, 0x00E8, 0x00EA, 0x00EB, 0x00ED, 0x00EC, 0x00EE, 0x00EF,
            0x00F1, 0x00F3, 0x00F2, 0x00F4, 0x00F6, 0x00F5, 0x00FA, 0x00F9, 0x00FB, 0x00FC, 0x2020,
            0x00B0, 0x00A2, 0x00A3, 0x00A7, 0x2022, 0x00B6, 0x00DF, 0x00AE, 0x00A9, 0x2122, 0x00B4,
            0x00A8, 0x0000, 0x00C6, 0x00D8, 0x0000, 0x00B1, 0x0000, 0x0000, 0x00A5, 0x00B5, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x00AA, 0x00BA, 0x0000, 0x00E6, 0x00F8, 0x00BF, 0x00A1,
            0x00AC, 0x0000, 0x0192, 0x0000, 0x0000, 0x00AB, 0x00BB, 0x2026, 0x00A0, 0x00C0, 0x00C3,
            0x00D5, 0x0152, 0x0153, 0x2013, 0x2014, 0x201C, 0x201D, 0x2018, 0x2019, 0x00F7, 0x0000,
            0x00FF, 0x0178, 0x2044, 0x00A4, 0x2039, 0x203A, 0xFB01, 0xFB02, 0x2021, 0x00B7, 0x201A,
            0x201E, 0x2030, 0x00C2, 0x00CA, 0x00C1, 0x00CB, 0x00C8, 0x00CD, 0x00CE, 0x00CF, 0x00CC,
            0x00D3, 0x00D4, 0x0000, 0x00D2, 0x00DA, 0x00DB, 0x00D9, 0x0131, 0x02C6, 0x02DC, 0x00AF,
            0x02D8, 0x02D9, 0x02DA, 0x00B8, 0x02DD, 0x02DB, 0x02C7
        };
        if (byteValue < 0x80)
            {
            return static_cast<wchar_t>(byteValue);
            }
        const wchar_t mapped{ macRomanTable[byteValue - 0x80] };
        return (mapped != 0) ? mapped : static_cast<wchar_t>(byteValue);
        }

    //------------------------------------------------------------------
    wchar_t pdf_text_decoder::standard_to_unicode(const unsigned char byteValue)
        {
        // Adobe StandardEncoding; like MacRoman, this diverges from Latin-1 across the
        // whole 0x80-0xFF range, so (unlike cp1252_to_unicode) it needs a full table.
        // A 0 entry means the code point is undefined in this encoding
        static constexpr std::array<wchar_t, 128> standardTable{
            0x0080, 0x0081, 0x0082, 0x0083, 0x0084, 0x0085, 0x0086, 0x0087, 0x0088, 0x0089, 0x008A,
            0x008B, 0x008C, 0x008D, 0x008E, 0x008F, 0x0090, 0x0091, 0x0092, 0x0093, 0x0094, 0x0095,
            0x0096, 0x0097, 0x0098, 0x0099, 0x009A, 0x009B, 0x009C, 0x009D, 0x009E, 0x009F, 0x00A0,
            0x00A1, 0x00A2, 0x00A3, 0x2044, 0x00A5, 0x0192, 0x00A7, 0x00A4, 0x0027, 0x201C, 0x00AB,
            0x2039, 0x203A, 0xFB01, 0xFB02, 0x0000, 0x2013, 0x2020, 0x2021, 0x00B7, 0x0000, 0x00B6,
            0x2022, 0x201A, 0x201E, 0x201D, 0x00BB, 0x2026, 0x2030, 0x0000, 0x00BF, 0x0000, 0x0060,
            0x00B4, 0x02C6, 0x02DC, 0x00AF, 0x02D8, 0x02D9, 0x00A8, 0x0000, 0x02DA, 0x00B8, 0x0000,
            0x02DD, 0x02DB, 0x02C7, 0x2014, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x00C6, 0x0000, 0x00AA,
            0x0000, 0x0000, 0x0000, 0x0000, 0x0141, 0x00D8, 0x0152, 0x00BA, 0x0000, 0x0000, 0x0000,
            0x0000, 0x0000, 0x00E6, 0x0000, 0x0000, 0x0000, 0x0131, 0x0000, 0x0000, 0x0142, 0x00F8,
            0x0153, 0x00DF, 0x0000, 0x0000, 0x0000, 0x0000
        };
        if (byteValue < 0x80)
            {
            return static_cast<wchar_t>(byteValue);
            }
        const wchar_t mapped{ standardTable[byteValue - 0x80] };
        return (mapped != 0) ? mapped : static_cast<wchar_t>(byteValue);
        }

    //------------------------------------------------------------------
    std::wstring pdf_text_decoder::utf16_units_to_wstring(const std::vector<char16_t>& units)
        {
        std::wstring result;
        result.reserve(units.size());
        for (size_t i = 0; i < units.size(); ++i)
            {
            const char16_t unit{ units[i] };
            // a high surrogate followed by a low surrogate is a valid pair
            if (unit >= 0xD800 && unit <= 0xDBFF && (i + 1) < units.size() &&
                units[i + 1] >= 0xDC00 && units[i + 1] <= 0xDFFF)
                {
                if constexpr (sizeof(wchar_t) > 2)
                    {
                    const char32_t codePoint = 0x10000 +
                                               ((static_cast<char32_t>(unit) - 0xD800) << 10) +
                                               (static_cast<char32_t>(units[i + 1]) - 0xDC00);
                    result += static_cast<wchar_t>(codePoint);
                    }
                else
                    {
                    result += static_cast<wchar_t>(unit);
                    result += static_cast<wchar_t>(units[i + 1]);
                    }
                ++i;
                continue;
                }
            // skip lone surrogates (both high and low)
            if (unit >= 0xD800 && unit <= 0xDFFF)
                {
                continue;
                }
            result += static_cast<wchar_t>(unit);
            }
        return result;
        }

    //------------------------------------------------------------------
    std::vector<char16_t> pdf_text_decoder::hex_to_utf16_units(const std::string_view hexDigits)
        {
        std::vector<char16_t> units;
        char16_t currentUnit{ 0 };
        size_t digitCount{ 0 };
        for (const char digit : hexDigits)
            {
            const int digitValue{ pdf_lexer::hex_digit_value(digit) };
            if (digitValue < 0)
                {
                continue;
                }
            currentUnit = static_cast<char16_t>((currentUnit << 4) | digitValue);
            if (++digitCount == 4)
                {
                units.push_back(currentUnit);
                currentUnit = 0;
                digitCount = 0;
                }
            }
        // a trailing partial unit (e.g., <20>, meaning a single byte) is taken as-is
        if (digitCount > 0)
            {
            units.push_back(currentUnit);
            }
        return units;
        }

    //------------------------------------------------------------------
    uint32_t pdf_text_decoder::hex_to_uint(const std::string_view hexDigits)
        {
        uint32_t value{ 0 };
        size_t digitCount{ 0 };
        for (const char digit : hexDigits)
            {
            const int digitValue{ pdf_lexer::hex_digit_value(digit) };
            if (digitValue < 0)
                {
                continue;
                }
            if (++digitCount > 8)
                {
                break;
                }
            value = (value << 4) | static_cast<uint32_t>(digitValue);
            }
        return value;
        }

    //------------------------------------------------------------------
    std::wstring pdf_text_decoder::decode_metadata_string(const std::string_view bytes)
        {
        if (bytes.length() >= 2 && static_cast<unsigned char>(bytes[0]) == 0xFE &&
            static_cast<unsigned char>(bytes[1]) == 0xFF)
            {
            std::vector<char16_t> units;
            units.reserve((bytes.length() - 2) / 2);
            for (size_t i = 2; (i + 1) < bytes.length(); i += 2)
                {
                units.push_back(static_cast<char16_t>((static_cast<unsigned char>(bytes[i]) << 8) |
                                                      static_cast<unsigned char>(bytes[i + 1])));
                }
            return pdf_text_decoder::utf16_units_to_wstring(units);
            }
        std::wstring result;
        result.reserve(bytes.length());
        for (const char byteValue : bytes)
            {
            result += pdf_text_decoder::cp1252_to_unicode(static_cast<unsigned char>(byteValue));
            }
        return result;
        }

    //------------------------------------------------------------------
    std::wstring pdf_text_decoder::glyph_name_to_unicode(const std::string_view glyphName,
                                                         const glyph_name_table& glyphTable)
        {
        if (glyphName.empty())
            {
            return {};
            }
        // widen the (always ASCII) glyph name to look it up
        std::wstring wideName;
        wideName.reserve(glyphName.length());
        for (const char nameChar : glyphName)
            {
            wideName += static_cast<wchar_t>(static_cast<unsigned char>(nameChar));
            }
        const auto tablePos = glyphTable.find(wideName);
        if (tablePos != glyphTable.cend())
            {
            return tablePos->second;
            }

        // fall back to Adobe's glyph-naming convention: a suffix after a period is a
        // variant tag and is ignored (e.g., "A.sc" is just "A")
        std::string_view baseName{ glyphName };
        const size_t dotPos{ baseName.find('.') };
        if (dotPos != std::string_view::npos)
            {
            baseName = baseName.substr(0, dotPos);
            }
        // "uniXXXX", optionally repeated ("uniXXXXYYYY...") for a sequence of code points
        if (baseName.compare(0, 3, "uni") == 0 && baseName.length() >= 7 &&
            ((baseName.length() - 3) % 4) == 0)
            {
            return pdf_text_decoder::utf16_units_to_wstring(
                pdf_text_decoder::hex_to_utf16_units(baseName.substr(3)));
            }
        // "uXXXX" through "uXXXXXX": a single code point, possibly astral
        if (baseName.compare(0, 1, "u") == 0 && baseName.length() >= 5 && baseName.length() <= 7)
            {
            const std::string_view hexDigits{ baseName.substr(1) };
            for (const char hexChar : hexDigits)
                {
                if (pdf_lexer::hex_digit_value(hexChar) < 0)
                    {
                    return {};
                    }
                }
            const uint32_t codePoint{ pdf_text_decoder::hex_to_uint(hexDigits) };
            if (codePoint <= 0xFFFF || sizeof(wchar_t) > 2)
                {
                return std::wstring(1, static_cast<wchar_t>(codePoint));
                }
            // encode as a UTF-16 surrogate pair (16-bit wchar_t platforms)
            const uint32_t adjusted{ codePoint - 0x10000 };
            std::wstring result;
            result += static_cast<wchar_t>(0xD800 + (adjusted >> 10));
            result += static_cast<wchar_t>(0xDC00 + (adjusted & 0x3FF));
            return result;
            }
        return {};
        }

    //------------------------------------------------------------------
    void pdf_text_decoder::parse_differences_array(const std::string_view differencesArray,
                                                   const glyph_name_table& glyphTable,
                                                   pdf_font_decoder& decoder)
        {
        size_t pos{ 0 };
        if (pos < differencesArray.length() && differencesArray[pos] == '[')
            {
            ++pos;
            }
        long currentCode{ 0 };
        bool haveCode{ false };
        bool addedAny{ false };
        while (pos < differencesArray.length())
            {
            pdf_lexer::skip_whitespace(differencesArray, pos);
            if (pos >= differencesArray.length() || differencesArray[pos] == ']')
                {
                break;
                }
            if (differencesArray[pos] == '/')
                {
                ++pos;
                const size_t nameStart{ pos };
                while (pos < differencesArray.length() &&
                       !pdf_lexer::is_token_end(differencesArray[pos]))
                    {
                    ++pos;
                    }
                if (haveCode)
                    {
                    const std::string_view glyphName{
                        differencesArray.substr(nameStart, pos - nameStart)
                    };
                    const std::wstring unicodeValue{
                        pdf_text_decoder::glyph_name_to_unicode(glyphName, glyphTable)
                    };
                    if (!unicodeValue.empty())
                        {
                        decoder.m_code_map[static_cast<uint32_t>(currentCode)] = unicodeValue;
                        addedAny = true;
                        }
                    ++currentCode;
                    }
                }
            else if (pdf_lexer::is_digit(differencesArray[pos]) || differencesArray[pos] == '+' ||
                     differencesArray[pos] == '-')
                {
                long code{ 0 };
                if (pdf_lexer::read_int(differencesArray, pos, code))
                    {
                    currentCode = code;
                    haveCode = true;
                    }
                else
                    {
                    ++pos;
                    }
                }
            else
                {
                // stray token in a malformed array; step over it
                ++pos;
                }
            }
        if (addedAny)
            {
            decoder.m_has_unicode_map = true;
            }
        }

    //------------------------------------------------------------------
    void pdf_text_decoder::parse_unicode_cmap(const std::string_view cmap,
                                              pdf_font_decoder& decoder)
        {
        // local helper: read the next <hex> token, returning its digits
        const auto readHexToken = [](const std::string_view source, size_t& pos,
                                     std::string_view& digitsOut) -> bool
        {
            pdf_lexer::skip_whitespace(source, pos);
            if (pos >= source.length() || source[pos] != '<')
                {
                return false;
                }
            ++pos;
            const size_t digitStart{ pos };
            while (pos < source.length() && source[pos] != '>')
                {
                ++pos;
                }
            digitsOut = source.substr(digitStart, pos - digitStart);
            if (pos < source.length())
                {
                ++pos;
                }
            return true;
        };

        // determine the code width(s) from the codespace range(s); a CMap may declare
        // more than one range, each with its own byte length (e.g., 1-byte codes for
        // ASCII mixed with 2-byte, 3-byte, or 4-byte codes for other glyphs)
        size_t searchPos{ cmap.find("begincodespacerange") };
        if (searchPos != std::string_view::npos)
            {
            size_t pos{ searchPos + 19 };
            while (pos < cmap.length())
                {
                pdf_lexer::skip_whitespace(cmap, pos);
                if (cmap.compare(pos, 17, "endcodespacerange") == 0 || pos >= cmap.length())
                    {
                    break;
                    }
                std::string_view lowDigits, highDigits;
                if (!readHexToken(cmap, pos, lowDigits) || !readHexToken(cmap, pos, highDigits) ||
                    lowDigits.empty())
                    {
                    break;
                    }
                const size_t byteLength{ std::clamp<size_t>(lowDigits.length() / 2, 1, 4) };
                decoder.m_codespace_ranges.push_back(
                    { pdf_text_decoder::hex_to_uint(lowDigits),
                      pdf_text_decoder::hex_to_uint(highDigits), byteLength });
                }
            if (!decoder.m_codespace_ranges.empty())
                {
                // used as the fallback width if a code doesn't match any range
                decoder.m_bytes_per_code = decoder.m_codespace_ranges.front().m_byte_length;
                }
            }

        // bfchar sections: <src> <dst>
        searchPos = 0;
        while ((searchPos = cmap.find("beginbfchar", searchPos)) != std::string_view::npos)
            {
            size_t pos{ searchPos + 11 };
            while (pos < cmap.length())
                {
                pdf_lexer::skip_whitespace(cmap, pos);
                if (cmap.compare(pos, 9, "endbfchar") == 0 || pos >= cmap.length())
                    {
                    break;
                    }
                std::string_view srcDigits, dstDigits;
                if (!readHexToken(cmap, pos, srcDigits) || !readHexToken(cmap, pos, dstDigits))
                    {
                    break;
                    }
                const std::wstring destination{ pdf_text_decoder::utf16_units_to_wstring(
                    pdf_text_decoder::hex_to_utf16_units(dstDigits)) };
                if (!destination.empty())
                    {
                    decoder.m_code_map[pdf_text_decoder::hex_to_uint(srcDigits)] = destination;
                    }
                }
            searchPos += 11;
            }

        // bfrange sections: <lo> <hi> <dst>  or  <lo> <hi> [<dst1> <dst2> ...]
        searchPos = 0;
        while ((searchPos = cmap.find("beginbfrange", searchPos)) != std::string_view::npos)
            {
            size_t pos{ searchPos + 12 };
            while (pos < cmap.length())
                {
                pdf_lexer::skip_whitespace(cmap, pos);
                if (cmap.compare(pos, 10, "endbfrange") == 0 || pos >= cmap.length())
                    {
                    break;
                    }
                std::string_view lowDigits, highDigits;
                if (!readHexToken(cmap, pos, lowDigits) || !readHexToken(cmap, pos, highDigits))
                    {
                    break;
                    }
                const uint32_t lowCode{ pdf_text_decoder::hex_to_uint(lowDigits) };
                const uint32_t highCode{ pdf_text_decoder::hex_to_uint(highDigits) };
                if (highCode < lowCode || (highCode - lowCode) > 65535)
                    {
                    break;
                    }
                pdf_lexer::skip_whitespace(cmap, pos);
                if (pos < cmap.length() && cmap[pos] == '[')
                    {
                    ++pos;
                    for (uint32_t code = lowCode; code <= highCode; ++code)
                        {
                        std::string_view dstDigits;
                        if (!readHexToken(cmap, pos, dstDigits))
                            {
                            break;
                            }
                        const std::wstring destination{ pdf_text_decoder::utf16_units_to_wstring(
                            pdf_text_decoder::hex_to_utf16_units(dstDigits)) };
                        if (!destination.empty())
                            {
                            decoder.m_code_map[code] = destination;
                            }
                        }
                    pdf_lexer::skip_whitespace(cmap, pos);
                    if (pos < cmap.length() && cmap[pos] == ']')
                        {
                        ++pos;
                        }
                    }
                else
                    {
                    std::string_view dstDigits;
                    if (!readHexToken(cmap, pos, dstDigits))
                        {
                        break;
                        }
                    std::vector<char16_t> units{ pdf_text_decoder::hex_to_utf16_units(dstDigits) };
                    if (!units.empty())
                        {
                        for (uint32_t code = lowCode; code <= highCode; ++code)
                            {
                            decoder.m_code_map[code] =
                                pdf_text_decoder::utf16_units_to_wstring(units);
                            units.back() = static_cast<char16_t>(units.back() + 1);
                            }
                        }
                    }
                }
            searchPos += 12;
            }
        decoder.m_has_unicode_map = !decoder.m_code_map.empty();
        }

    //------------------------------------------------------------------
    std::string_view pdf_text_decoder::predefined_cmap_charset(const std::string_view cmapName)
        {
        // Adobe's predefined CJK CMaps (PDF spec, "Predefined CJK CMap names").
        // A Type0 font using one of these as its /Encoding stores its string bytes
        // as text in the CMap's underlying platform charset; the CMap exists to map
        // those charset bytes to the font vendor's glyph IDs (CIDs), so the bytes
        // themselves can simply be converted from that charset.
        //
        // Japanese: the RKSJ family (83pv-, 90ms-, 90msp-, 90pv-, Add-, Ext-RKSJ)
        // is Shift-JIS; plain EUC-H/V is EUC-JP.
        if (cmapName.find("RKSJ") != std::string_view::npos)
            {
            return "CP932";
            }
        if (cmapName == "EUC-H" || cmapName == "EUC-V")
            {
            return "EUC-JP";
            }
        // Traditional Chinese: the B5 family (B5pc-, ETen-B5, ETenms-B5, HKscs-B5)
        // is Big5. (CNS-EUC-H/V is EUC-TW, which is too rarely supported by
        // converters to map here.)
        if (cmapName.find("-B5") != std::string_view::npos ||
            cmapName.compare(0, 4, "B5pc") == 0)
            {
            return "CP950";
            }
        // Simplified Chinese: GBK2K is GB18030, the other GBK CMaps are GBK,
        // and the older GB-EUC/GBpc-EUC are EUC-CN (GB2312).
        if (cmapName.compare(0, 5, "GBK2K") == 0)
            {
            return "GB18030";
            }
        if (cmapName.compare(0, 3, "GBK") == 0)
            {
            return "CP936";
            }
        if (cmapName.compare(0, 6, "GB-EUC") == 0 || cmapName.compare(0, 8, "GBpc-EUC") == 0)
            {
            return "GB2312";
            }
        // Korean: the UHC family (KSCms-UHC-H, -HW-H, -V) is Unified Hangul Code
        // (an extension of EUC-KR); KSC-EUC/KSCpc-EUC are EUC-KR.
        if (cmapName.find("UHC") != std::string_view::npos)
            {
            return "CP949";
            }
        if (cmapName.compare(0, 7, "KSC-EUC") == 0 || cmapName.compare(0, 9, "KSCpc-EUC") == 0)
            {
            return "EUC-KR";
            }
        return {};
        }

    //------------------------------------------------------------------
    bool pdf_text_decoder::is_unicode_cmap_name(const std::string_view cmapName)
        {
        // Adobe's predefined Unicode CMaps: UniJIS-UCS2-H, UniGB-UTF16-H,
        // UniCNS-UCS2-HW-V, UniKS-UTF16-H, UniJIS2004-UTF16-H, etc.
        // Strings shown with such an encoding are UTF-16BE.
        return (cmapName.compare(0, 3, "Uni") == 0 &&
                (cmapName.find("UCS2") != std::string_view::npos ||
                 cmapName.find("UTF16") != std::string_view::npos));
        }

    //------------------------------------------------------------------
    size_t pdf_text_decoder::determine_code_length(const std::string& bytes, const size_t pos,
                                                    const pdf_font_decoder* fontDecoder)
        {
        const size_t remaining{ bytes.length() - pos };
        if (fontDecoder == nullptr)
            {
            return 1;
            }
        for (const auto& range : fontDecoder->m_codespace_ranges)
            {
            if (range.m_byte_length > remaining)
                {
                continue;
                }
            uint32_t code{ 0 };
            for (size_t byteIndex = 0; byteIndex < range.m_byte_length; ++byteIndex)
                {
                code = (code << 8) | static_cast<unsigned char>(bytes[pos + byteIndex]);
                }
            if (code >= range.m_low && code <= range.m_high)
                {
                return range.m_byte_length;
                }
            }
        // no (or no matching) codespace range; fall back to the font's fixed width
        return std::clamp<size_t>(fontDecoder->m_bytes_per_code, 1, remaining);
        }

    //------------------------------------------------------------------
    std::wstring pdf_text_decoder::decode_string_bytes(const std::string& bytes,
                                                       const pdf_font_decoder* fontDecoder)
        {
        // A font using one of Adobe's predefined legacy CJK CMaps as its /Encoding
        // (e.g., /ETenms-B5-H) has string bytes that are text in the corresponding
        // platform charset (Big5, Shift-JIS, etc.). Convert the whole string through
        // the connected charset converter, which natively handles the mix of
        // single-byte and double-byte characters those charsets allow. A ToUnicode
        // CMap, when the font also has one, is more authoritative and is preferred.
        if (fontDecoder != nullptr && !fontDecoder->m_has_unicode_map &&
            !fontDecoder->m_charset.empty() && fontDecoder->m_charset_converter)
            {
            return fontDecoder->m_charset_converter(bytes, fontDecoder->m_charset);
            }
        std::wstring result;
        result.reserve(bytes.length());
        size_t i{ 0 };
        while (i < bytes.length())
            {
            const size_t codeSize{
                pdf_text_decoder::determine_code_length(bytes, i, fontDecoder)
            };
            uint32_t code{ 0 };
            for (size_t byteIndex = 0; byteIndex < codeSize; ++byteIndex)
                {
                code = (code << 8) | static_cast<unsigned char>(bytes[i + byteIndex]);
                }
            if (fontDecoder != nullptr && fontDecoder->m_has_unicode_map)
                {
                const auto mappedPos = fontDecoder->m_code_map.find(code);
                if (mappedPos != fontDecoder->m_code_map.cend())
                    {
                    result += mappedPos->second;
                    i += codeSize;
                    continue;
                    }
                }
            if (codeSize == 2)
                {
                // No mapping is available for this code. Some subset fonts (particularly
                // ones supporting astral-plane characters, such as emoji) assign CIDs
                // equal to the UTF-16 code units of the underlying text, even with no
                // ToUnicode CMap at all. If this is a high surrogate immediately followed
                // by a low surrogate (also resolved as a 2-byte code), combine the pair
                // into the single code point they represent, rather than dropping both as
                // "unpaired". A genuine surrogate pair is an unambiguous signal that's very
                // unlikely to occur by coincidence in an icon font's arbitrary glyph-index
                // numbering, so this check is safe to make unconditionally.
                const size_t nextPos{ i + codeSize };
                if (code >= 0xD800 && code <= 0xDBFF && (nextPos + 1) < bytes.length() &&
                    pdf_text_decoder::determine_code_length(bytes, nextPos, fontDecoder) == 2)
                    {
                    const uint32_t nextCode{
                        (static_cast<uint32_t>(static_cast<unsigned char>(bytes[nextPos])) << 8) |
                        static_cast<unsigned char>(bytes[nextPos + 1])
                    };
                    if (nextCode >= 0xDC00 && nextCode <= 0xDFFF)
                        {
                        if constexpr (sizeof(wchar_t) > 2)
                            {
                            const char32_t codePoint{
                                0x10000 + ((static_cast<char32_t>(code) - 0xD800) << 10) +
                                (static_cast<char32_t>(nextCode) - 0xDC00)
                            };
                            result += static_cast<wchar_t>(codePoint);
                            }
                        else
                            {
                            result += static_cast<wchar_t>(code);
                            result += static_cast<wchar_t>(nextCode);
                            }
                        i += (codeSize * 2);
                        continue;
                        }
                    }
                // Not a surrogate pair. Only fall back to taking the code value directly
                // as a Unicode code point if the font declared its codes to be UTF-16
                // (a predefined /UniXX-UCS2 or -UTF16 encoding), or its ToUnicode CMap
                // maps *some* other codes to real text, which is evidence its CIDs are
                // Unicode-based (just missing an entry for this particular one). A font
                // with neither (e.g., an embedded icon/dingbat font such as Font Awesome)
                // has arbitrary glyph-index CIDs with no relationship to Unicode, so
                // there's no basis to guess a character for it; drop it instead of
                // emitting the raw CID.
                if (fontDecoder != nullptr &&
                    (fontDecoder->m_has_unicode_map || fontDecoder->m_codes_are_utf16) &&
                    (code < 0xD800 || code > 0xDFFF))
                    {
                    result += static_cast<wchar_t>(code);
                    }
                }
            else if (codeSize == 1)
                {
                if (fontDecoder != nullptr && fontDecoder->m_symbol_font && code == 0xB7)
                    {
                    result += L'\x2022'; // bullet
                    }
                else
                    {
                    const pdf_font_decoder::base_encoding_type baseEncoding{
                        (fontDecoder != nullptr) ? fontDecoder->m_base_encoding :
                                                   pdf_font_decoder::base_encoding_type::win_ansi
                    };
                    result += (baseEncoding == pdf_font_decoder::base_encoding_type::mac_roman) ?
                                  pdf_text_decoder::mac_roman_to_unicode(
                                      static_cast<unsigned char>(code)) :
                              (baseEncoding == pdf_font_decoder::base_encoding_type::standard) ?
                                  pdf_text_decoder::standard_to_unicode(
                                      static_cast<unsigned char>(code)) :
                                  pdf_text_decoder::cp1252_to_unicode(
                                      static_cast<unsigned char>(code));
                    }
                }
            // else: a 3-byte or 4-byte code with no ToUnicode mapping; there's no
            // fallback encoding table for widths like this, so it's dropped
            i += codeSize;
            }
        return result;
        }
    } // namespace lily_of_the_valley
