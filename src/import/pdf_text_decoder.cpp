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
#include <unordered_map>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    // text decoding helpers
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    wchar_t pdf_text_decoder::cp1252_to_unicode(const unsigned char byteValue)
        {
        // 0x80-0x9F are the only values that differ from Latin-1
        constexpr static std::array<wchar_t, 32> cp1252Table{
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
        constexpr static std::array<wchar_t, 128> macRomanTable{
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
        constexpr static std::array<wchar_t, 128> standardTable{
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
    wchar_t pdf_text_decoder::symbol_to_unicode(const unsigned char byteValue)
        {
        // Unlike the Latin encodings above, Adobe's SymbolEncoding remaps the entire
        // printable range (not just 0x80-0xFF) to the Symbol font's own glyph set
        // (Greek letters, mathematical operators, and the like), so the full
        // 0x20-0xFF span is tabulated here. A 0 entry means the code point is
        // undefined in this encoding.
        constexpr static auto symbolTable{ std::to_array<wchar_t>(
            { 0x0020, 0x0021, 0x2200, 0x0023, 0x2203, 0x0025, 0x0026, 0x220B, 0x0028, 0x0029,
              0x2217, 0x002B, 0x002C, 0x2212, 0x002E, 0x002F, 0x0030, 0x0031, 0x0032, 0x0033,
              0x0034, 0x0035, 0x0036, 0x0037, 0x0038, 0x0039, 0x003A, 0x003B, 0x003C, 0x003D,
              0x003E, 0x003F, 0x2245, 0x0391, 0x0392, 0x03A7, 0x2206, 0x0395, 0x03A6, 0x0393,
              0x0397, 0x0399, 0x03D1, 0x039A, 0x039B, 0x039C, 0x039D, 0x039F, 0x03A0, 0x0398,
              0x03A1, 0x03A3, 0x03A4, 0x03A5, 0x03C2, 0x2126, 0x039E, 0x03A8, 0x0396, 0x005B,
              0x2234, 0x005D, 0x22A5, 0x005F, 0xF8E5, 0x03B1, 0x03B2, 0x03C7, 0x03B4, 0x03B5,
              0x03C6, 0x03B3, 0x03B7, 0x03B9, 0x03D5, 0x03BA, 0x03BB, 0x00B5, 0x03BD, 0x03BF,
              0x03C0, 0x03B8, 0x03C1, 0x03C3, 0x03C4, 0x03C5, 0x03D6, 0x03C9, 0x03BE, 0x03C8,
              0x03B6, 0x007B, 0x007C, 0x007D, 0x223C, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x20AC, 0x03D2,
              0x2032, 0x2264, 0x2044, 0x221E, 0x0192, 0x2663, 0x2666, 0x2665, 0x2660, 0x2194,
              0x2190, 0x2191, 0x2192, 0x2193, 0x00B0, 0x00B1, 0x2033, 0x2265, 0x00D7, 0x221D,
              0x2202, 0x2022, 0x00F7, 0x2260, 0x2261, 0x2248, 0x2026, 0xF8E6, 0xF8E7, 0x21B5,
              0x2135, 0x2111, 0x211C, 0x2118, 0x2297, 0x2295, 0x2205, 0x2229, 0x222A, 0x2283,
              0x2287, 0x2284, 0x2282, 0x2286, 0x2208, 0x2209, 0x2220, 0x2207, 0xF6DA, 0xF6D9,
              0xF6DB, 0x220F, 0x221A, 0x22C5, 0x00AC, 0x2227, 0x2228, 0x21D4, 0x21D0, 0x21D1,
              0x21D2, 0x21D3, 0x25CA, 0x2329, 0xF8E8, 0xF8E9, 0xF8EA, 0x2211, 0xF8EB, 0xF8EC,
              0xF8ED, 0xF8EE, 0xF8EF, 0xF8F0, 0xF8F1, 0xF8F2, 0xF8F3, 0xF8F4, 0x0000, 0x232A,
              0x222B, 0x2320, 0xF8F5, 0x2321, 0xF8F6, 0xF8F7, 0xF8F8, 0xF8F9, 0xF8FA, 0xF8FB,
              0xF8FC, 0xF8FD, 0xF8FE, 0x0000 }) };
        // one entry per code point from 0x20 through 0xFF
        static_assert(symbolTable.size() == (0x100 - 0x20));
        if (byteValue < 0x20)
            {
            return static_cast<wchar_t>(byteValue);
            }
        const wchar_t mapped{ symbolTable[byteValue - 0x20] };
        return (mapped != 0) ? mapped : static_cast<wchar_t>(byteValue);
        }

    //------------------------------------------------------------------
    wchar_t pdf_text_decoder::zapf_dingbats_to_unicode(const unsigned char byteValue)
        {
        // Same shape as symbol_to_unicode(): ZapfDingbatsEncoding remaps the whole
        // printable range to the ZapfDingbats font's own pictographic glyph set.
        // A 0 entry means the code point is undefined in this encoding.
        constexpr static auto zapfDingbatsTable{ std::to_array<wchar_t>(
            { 0x0020, 0x2701, 0x2702, 0x2703, 0x2704, 0x260E, 0x2706, 0x2707, 0x2708, 0x2709,
              0x261B, 0x261E, 0x270C, 0x270D, 0x270E, 0x270F, 0x2710, 0x2711, 0x2712, 0x2713,
              0x2714, 0x2715, 0x2716, 0x2717, 0x2718, 0x2719, 0x271A, 0x271B, 0x271C, 0x271D,
              0x271E, 0x271F, 0x2720, 0x2721, 0x2722, 0x2723, 0x2724, 0x2725, 0x2726, 0x2727,
              0x2605, 0x2729, 0x272A, 0x272B, 0x272C, 0x272D, 0x272E, 0x272F, 0x2730, 0x2731,
              0x2732, 0x2733, 0x2734, 0x2735, 0x2736, 0x2737, 0x2738, 0x2739, 0x273A, 0x273B,
              0x273C, 0x273D, 0x273E, 0x273F, 0x2740, 0x2741, 0x2742, 0x2743, 0x2744, 0x2745,
              0x2746, 0x2747, 0x2748, 0x2749, 0x274A, 0x274B, 0x25CF, 0x274D, 0x25A0, 0x274F,
              0x2750, 0x2751, 0x2752, 0x25B2, 0x25BC, 0x25C6, 0x2756, 0x25D7, 0x2758, 0x2759,
              0x275A, 0x275B, 0x275C, 0x275D, 0x275E, 0x0000, 0x2768, 0x2769, 0x276A, 0x276B,
              0x276C, 0x276D, 0x276E, 0x276F, 0x2770, 0x2771, 0x2772, 0x2773, 0x2774, 0x2775,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000,
              0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x2761,
              0x2762, 0x2763, 0x2764, 0x2765, 0x2766, 0x2767, 0x2663, 0x2666, 0x2665, 0x2660,
              0x2460, 0x2461, 0x2462, 0x2463, 0x2464, 0x2465, 0x2466, 0x2467, 0x2468, 0x2469,
              0x2776, 0x2777, 0x2778, 0x2779, 0x277A, 0x277B, 0x277C, 0x277D, 0x277E, 0x277F,
              0x2780, 0x2781, 0x2782, 0x2783, 0x2784, 0x2785, 0x2786, 0x2787, 0x2788, 0x2789,
              0x278A, 0x278B, 0x278C, 0x278D, 0x278E, 0x278F, 0x2790, 0x2791, 0x2792, 0x2793,
              0x2794, 0x2192, 0x2194, 0x2195, 0x2798, 0x2799, 0x279A, 0x279B, 0x279C, 0x279D,
              0x279E, 0x279F, 0x27A0, 0x27A1, 0x27A2, 0x27A3, 0x27A4, 0x27A5, 0x27A6, 0x27A7,
              0x27A8, 0x27A9, 0x27AA, 0x27AB, 0x27AC, 0x27AD, 0x27AE, 0x27AF, 0x0000, 0x27B1,
              0x27B2, 0x27B3, 0x27B4, 0x27B5, 0x27B6, 0x27B7, 0x27B8, 0x27B9, 0x27BA, 0x27BB,
              0x27BC, 0x27BD, 0x27BE, 0x0000 }) };
        // one entry per code point from 0x20 through 0xFF
        static_assert(zapfDingbatsTable.size() == (0x100 - 0x20));
        if (byteValue < 0x20)
            {
            return static_cast<wchar_t>(byteValue);
            }
        const wchar_t mapped{ zapfDingbatsTable[byteValue - 0x20] };
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
            const std::string_view hexDigits{ baseName.substr(3) };
            bool allHex{ true };
            for (const char hexChar : hexDigits)
                {
                if (pdf_lexer::hex_digit_value(hexChar) < 0)
                    {
                    allHex = false;
                    break;
                    }
                }
            // reject names that merely start with "uni" (e.g., "unicorn"), rather than
            // silently skipping their non-hex characters and returning a garbage code point
            if (allHex)
                {
                return pdf_text_decoder::utf16_units_to_wstring(
                    pdf_text_decoder::hex_to_utf16_units(hexDigits));
                }
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
                    const std::string_view glyphName{ differencesArray.substr(nameStart,
                                                                              pos - nameStart) };
                    const std::wstring unicodeValue{ pdf_text_decoder::glyph_name_to_unicode(
                        glyphName, glyphTable) };
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
    uint16_t pdf_text_decoder::truetype_read_uint16(const std::string_view data, const size_t pos)
        {
        if (pos + 2 > data.size())
            {
            return 0;
            }
        return static_cast<uint16_t>((static_cast<unsigned char>(data[pos]) << 8) |
                                     static_cast<unsigned char>(data[pos + 1]));
        }

    //------------------------------------------------------------------
    uint32_t pdf_text_decoder::truetype_read_uint32(const std::string_view data, const size_t pos)
        {
        if (pos + 4 > data.size())
            {
            return 0;
            }
        return (static_cast<uint32_t>(static_cast<unsigned char>(data[pos])) << 24) |
               (static_cast<uint32_t>(static_cast<unsigned char>(data[pos + 1])) << 16) |
               (static_cast<uint32_t>(static_cast<unsigned char>(data[pos + 2])) << 8) |
               static_cast<uint32_t>(static_cast<unsigned char>(data[pos + 3]));
        }

    //------------------------------------------------------------------
    bool pdf_text_decoder::find_cmap_table(const std::string_view fontProgram,
                                           std::string_view& cmapTableOut)
        {
        constexpr size_t SFNT_HEADER_SIZE{ 12 };
        constexpr size_t TABLE_RECORD_SIZE{ 16 };
        if (fontProgram.size() < SFNT_HEADER_SIZE)
            {
            return false;
            }
        const uint16_t numTables{ pdf_text_decoder::truetype_read_uint16(fontProgram, 4) };
        for (uint16_t tableIndex = 0; tableIndex < numTables; ++tableIndex)
            {
            const size_t recordPos{ SFNT_HEADER_SIZE +
                                    (static_cast<size_t>(tableIndex) * TABLE_RECORD_SIZE) };
            if (recordPos + TABLE_RECORD_SIZE > fontProgram.size())
                {
                break;
                }
            if (fontProgram.compare(recordPos, 4, "cmap") == 0)
                {
                const size_t cmapOffset{ pdf_text_decoder::truetype_read_uint32(fontProgram,
                                                                                recordPos + 8) };
                const size_t cmapLength{ pdf_text_decoder::truetype_read_uint32(fontProgram,
                                                                                recordPos + 12) };
                if (cmapOffset > fontProgram.size() ||
                    cmapLength > (fontProgram.size() - cmapOffset))
                    {
                    return false;
                    }
                cmapTableOut = fontProgram.substr(cmapOffset, cmapLength);
                return true;
                }
            }
        return false;
        }

    //------------------------------------------------------------------
    bool pdf_text_decoder::find_cmap_subtable_offset(const std::string_view cmapTable,
                                                     const uint16_t platformID,
                                                     const uint16_t encodingID, size_t& offsetOut)
        {
        constexpr size_t CMAP_HEADER_SIZE{ 4 };
        constexpr size_t ENCODING_RECORD_SIZE{ 8 };
        const uint16_t subtableCount{ pdf_text_decoder::truetype_read_uint16(cmapTable, 2) };
        for (uint16_t recordIndex = 0; recordIndex < subtableCount; ++recordIndex)
            {
            const size_t recordPos{ CMAP_HEADER_SIZE +
                                    (static_cast<size_t>(recordIndex) * ENCODING_RECORD_SIZE) };
            if (recordPos + ENCODING_RECORD_SIZE > cmapTable.size())
                {
                break;
                }
            if (pdf_text_decoder::truetype_read_uint16(cmapTable, recordPos) == platformID &&
                pdf_text_decoder::truetype_read_uint16(cmapTable, recordPos + 2) == encodingID)
                {
                offsetOut = pdf_text_decoder::truetype_read_uint32(cmapTable, recordPos + 4);
                return true;
                }
            }
        return false;
        }

    //------------------------------------------------------------------
    std::unordered_map<uint32_t, wchar_t>
    pdf_text_decoder::invert_format4_unicode_subtable(const std::string_view cmapTable,
                                                      const size_t subtableOffset)
        {
        std::unordered_map<uint32_t, wchar_t> glyphToUnicode;
        if (pdf_text_decoder::truetype_read_uint16(cmapTable, subtableOffset) != 4)
            {
            return glyphToUnicode;
            }
        const size_t segCount{ pdf_text_decoder::truetype_read_uint16(cmapTable,
                                                                      subtableOffset + 6) /
                               static_cast<size_t>(2) };
        const size_t endCodeArray{ subtableOffset + 14 };
        const size_t startCodeArray{ endCodeArray + (segCount * 2) + 2 };
        const size_t idDeltaArray{ startCodeArray + (segCount * 2) };
        const size_t idRangeOffsetArray{ idDeltaArray + (segCount * 2) };
        if (segCount == 0 || idRangeOffsetArray + (segCount * 2) > cmapTable.size())
            {
            return glyphToUnicode;
            }
        // a segment's code range is at most 0xFFFF wide; this bounds the total work
        // a malicious/malformed font (with many wide, overlapping segments) can force
        size_t codesProcessed{ 0 };
        constexpr size_t MAX_CODES_PROCESSED{ 0x10000 };
        for (size_t segment = 0; segment < segCount && codesProcessed < MAX_CODES_PROCESSED;
             ++segment)
            {
            const uint16_t endCode{ pdf_text_decoder::truetype_read_uint16(
                cmapTable, endCodeArray + (segment * 2)) };
            const uint16_t startCode{ pdf_text_decoder::truetype_read_uint16(
                cmapTable, startCodeArray + (segment * 2)) };
            const auto idDelta{ static_cast<int16_t>(
                pdf_text_decoder::truetype_read_uint16(cmapTable, idDeltaArray + (segment * 2))) };
            const uint16_t idRangeOffset{ pdf_text_decoder::truetype_read_uint16(
                cmapTable, idRangeOffsetArray + (segment * 2)) };
            if (idRangeOffset != 0 || startCode > endCode)
                {
                continue;
                }
            for (uint32_t code = startCode; code <= endCode && codesProcessed < MAX_CODES_PROCESSED;
                 ++code)
                {
                ++codesProcessed;
                const auto glyphID{ static_cast<uint16_t>(static_cast<int32_t>(code) + idDelta) };
                if (glyphID == 0)
                    {
                    continue;
                    }
                glyphToUnicode.try_emplace(glyphID, static_cast<wchar_t>(code));
                }
            if (endCode == 0xFFFF)
                {
                break;
                }
            }
        return glyphToUnicode;
        }

    //------------------------------------------------------------------
    void pdf_text_decoder::parse_embedded_truetype_cmap(const std::string_view fontProgram,
                                                        pdf_font_decoder& decoder)
        {
        std::string_view cmapTable;
        if (!pdf_text_decoder::find_cmap_table(fontProgram, cmapTable))
            {
            return;
            }

        // find the (1, 0) [Mac Roman] and (3, 1) [Windows Unicode BMP] subtables
        size_t macSubtableOffset{ 0 };
        size_t winSubtableOffset{ 0 };
        if (!pdf_text_decoder::find_cmap_subtable_offset(cmapTable, 1, 0, macSubtableOffset) ||
            !pdf_text_decoder::find_cmap_subtable_offset(cmapTable, 3, 1, winSubtableOffset))
            {
            return;
            }

        // format 0: a flat 256-byte array, code -> glyph index
        constexpr size_t FORMAT0_HEADER_SIZE{ 6 };
        constexpr size_t FORMAT0_ARRAY_SIZE{ 256 };
        if (pdf_text_decoder::truetype_read_uint16(cmapTable, macSubtableOffset) != 0 ||
            macSubtableOffset + FORMAT0_HEADER_SIZE + FORMAT0_ARRAY_SIZE > cmapTable.size())
            {
            return;
            }
        std::array<uint8_t, FORMAT0_ARRAY_SIZE> codeToGlyph{};
        for (size_t code = 0; code < FORMAT0_ARRAY_SIZE; ++code)
            {
            codeToGlyph[code] =
                static_cast<uint8_t>(cmapTable[macSubtableOffset + FORMAT0_HEADER_SIZE + code]);
            }

        // format 4: segmented Unicode BMP coverage, inverted into glyph -> Unicode
        const std::unordered_map<uint32_t, wchar_t> glyphToUnicode{
            pdf_text_decoder::invert_format4_unicode_subtable(cmapTable, winSubtableOffset)
        };
        if (glyphToUnicode.empty())
            {
            return;
            }

        // chain: code -> glyph index (format 0) -> Unicode (inverted format 4)
        bool addedAny{ false };
        for (uint32_t code = 0; code < FORMAT0_ARRAY_SIZE; ++code)
            {
            const uint8_t glyphID{ codeToGlyph[code] };
            if (glyphID == 0)
                {
                continue;
                }
            const auto unicodePos = glyphToUnicode.find(glyphID);
            if (unicodePos != glyphToUnicode.cend())
                {
                decoder.m_code_map[code] = std::wstring(1, unicodePos->second);
                addedAny = true;
                }
            }
        if (addedAny)
            {
            decoder.m_has_unicode_map = true;
            }
        }

    //------------------------------------------------------------------
    void pdf_text_decoder::parse_embedded_cid_truetype_cmap(const std::string_view fontProgram,
                                                            const std::string_view cidToGidMapData,
                                                            pdf_font_decoder& decoder)
        {
        std::string_view cmapTable;
        if (!pdf_text_decoder::find_cmap_table(fontProgram, cmapTable))
            {
            return;
            }

        size_t winSubtableOffset{ 0 };
        if (!pdf_text_decoder::find_cmap_subtable_offset(cmapTable, 3, 1, winSubtableOffset))
            {
            return;
            }

        const std::unordered_map<uint32_t, wchar_t> glyphToUnicode{
            pdf_text_decoder::invert_format4_unicode_subtable(cmapTable, winSubtableOffset)
        };
        if (glyphToUnicode.empty())
            {
            return;
            }

        bool addedAny{ false };
        // no /CIDToGIDMap stream: CID -> GID is Identity
        if (cidToGidMapData.empty())
            {
            for (const auto& [glyphID, unicodeChar] : glyphToUnicode)
                {
                decoder.m_code_map[glyphID] = std::wstring(1, unicodeChar);
                addedAny = true;
                }
            }
        else
            {
            const size_t cidCount{ cidToGidMapData.length() / 2 };
            for (size_t cid = 0; cid < cidCount; ++cid)
                {
                const uint16_t glyphID{ pdf_text_decoder::truetype_read_uint16(cidToGidMapData,
                                                                               cid * 2) };
                if (glyphID == 0)
                    {
                    continue;
                    }
                const auto unicodePos = glyphToUnicode.find(glyphID);
                if (unicodePos != glyphToUnicode.cend())
                    {
                    decoder.m_code_map[static_cast<uint32_t>(cid)] =
                        std::wstring(1, unicodePos->second);
                    addedAny = true;
                    }
                }
            }
        if (addedAny)
            {
            decoder.m_has_unicode_map = true;
            }
        }

    //------------------------------------------------------------------
    bool pdf_text_decoder::read_hex_token(const std::string_view source, size_t& pos,
                                          std::string_view& digitsOut)
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
        }

    //------------------------------------------------------------------
    void pdf_text_decoder::parse_unicode_cmap(const std::string_view cmap,
                                              pdf_font_decoder& decoder)
        {
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
                if (!read_hex_token(cmap, pos, lowDigits) ||
                    !read_hex_token(cmap, pos, highDigits) || lowDigits.empty())
                    {
                    break;
                    }
                const size_t byteLength{ std::clamp<size_t>(lowDigits.length() / 2, 1, 4) };
                decoder.m_codespace_ranges.push_back({ pdf_text_decoder::hex_to_uint(lowDigits),
                                                       pdf_text_decoder::hex_to_uint(highDigits),
                                                       byteLength });
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
                if (!read_hex_token(cmap, pos, srcDigits) || !read_hex_token(cmap, pos, dstDigits))
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
                if (!read_hex_token(cmap, pos, lowDigits) || !read_hex_token(cmap, pos, highDigits))
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
                        if (!read_hex_token(cmap, pos, dstDigits))
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
                    if (!read_hex_token(cmap, pos, dstDigits))
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
    void pdf_text_decoder::parse_cid_to_unicode_cmap(const std::string_view cmap,
                                                     cid_to_unicode_table& table)
        {
        // reads the next (base 10) integer token
        const auto readDecimalToken = [](const std::string_view source, size_t& pos,
                                         uint32_t& valueOut) -> bool
        {
            pdf_lexer::skip_whitespace(source, pos);
            if (pos >= source.length() || !pdf_lexer::is_digit(source[pos]))
                {
                return false;
                }
            valueOut = 0;
            while (pos < source.length() && pdf_lexer::is_digit(source[pos]))
                {
                valueOut = (valueOut * 10) + static_cast<uint32_t>(source[pos] - '0');
                ++pos;
                }
            return true;
        };

        // cidchar sections: <src> cid
        size_t searchPos{ 0 };
        while ((searchPos = cmap.find("begincidchar", searchPos)) != std::string_view::npos)
            {
            size_t pos{ searchPos + 12 };
            while (pos < cmap.length())
                {
                pdf_lexer::skip_whitespace(cmap, pos);
                if (cmap.compare(pos, 10, "endcidchar") == 0 || pos >= cmap.length())
                    {
                    break;
                    }
                std::string_view srcDigits;
                uint32_t cid{ 0 };
                if (!read_hex_token(cmap, pos, srcDigits) || !readDecimalToken(cmap, pos, cid))
                    {
                    break;
                    }
                const std::wstring unicodeText{ pdf_text_decoder::utf16_units_to_wstring(
                    pdf_text_decoder::hex_to_utf16_units(srcDigits)) };
                if (!unicodeText.empty())
                    {
                    table[cid] = unicodeText;
                    }
                }
            searchPos += 12;
            }

        // cidrange sections: <lo> <hi> cidStart
        searchPos = 0;
        while ((searchPos = cmap.find("begincidrange", searchPos)) != std::string_view::npos)
            {
            size_t pos{ searchPos + 13 };
            while (pos < cmap.length())
                {
                pdf_lexer::skip_whitespace(cmap, pos);
                if (cmap.compare(pos, 11, "endcidrange") == 0 || pos >= cmap.length())
                    {
                    break;
                    }
                std::string_view lowDigits, highDigits;
                uint32_t cidStart{ 0 };
                if (!read_hex_token(cmap, pos, lowDigits) ||
                    !read_hex_token(cmap, pos, highDigits) ||
                    !readDecimalToken(cmap, pos, cidStart))
                    {
                    break;
                    }
                const uint32_t lowCode{ pdf_text_decoder::hex_to_uint(lowDigits) };
                const uint32_t highCode{ pdf_text_decoder::hex_to_uint(highDigits) };
                if (highCode < lowCode || (highCode - lowCode) > 65535)
                    {
                    break;
                    }
                // Walk the source range's UTF-16 units alongside the destination CIDs,
                // incrementing just the low code unit at each step. This also keeps a
                // surrogate-pair source range within its low-surrogate block, matching
                // how parse_unicode_cmap()'s bfrange handling steps its destination units.
                std::vector<char16_t> srcUnits{ pdf_text_decoder::hex_to_utf16_units(lowDigits) };
                if (!srcUnits.empty())
                    {
                    for (uint32_t code = lowCode, cid = cidStart; code <= highCode; ++code, ++cid)
                        {
                        const std::wstring unicodeText{ pdf_text_decoder::utf16_units_to_wstring(
                            srcUnits) };
                        if (!unicodeText.empty())
                            {
                            table[cid] = unicodeText;
                            }
                        srcUnits.back() = static_cast<char16_t>(srcUnits.back() + 1);
                        }
                    }
                }
            searchPos += 13;
            }
        }

    //------------------------------------------------------------------
    std::string_view pdf_text_decoder::parse_usecmap_name(const std::string_view cmap)
        {
        // An embedded CMap stream may chain to one of Adobe's predefined CMaps via
        // "/<Name> usecmap" (PDF spec, 9.7.5.3, "Use of a CMap"). When present, the
        // referenced CMap's semantics (Identity/Unicode/legacy charset) apply here too.
        const size_t useCmapPos{ cmap.find("usecmap") };
        if (useCmapPos == std::string_view::npos)
            {
            return {};
            }
        size_t pos{ useCmapPos };
        while (pos > 0 && pdf_lexer::is_whitespace(cmap[pos - 1]))
            {
            --pos;
            }
        const size_t nameEnd{ pos };
        while (pos > 0 && !pdf_lexer::is_whitespace(cmap[pos - 1]) &&
               !pdf_lexer::is_delimiter(cmap[pos - 1]))
            {
            --pos;
            }
        if (pos == 0 || cmap[pos - 1] != '/')
            {
            return {};
            }
        return cmap.substr(pos, nameEnd - pos);
        }

    //------------------------------------------------------------------
    bool pdf_text_decoder::parse_wmode_from_cmap_stream(const std::string_view cmap)
        {
        // An embedded CMap stream may declare "/WMode 1 def" for vertical writing
        // mode (PDF spec, 9.7.5.2, "CMap Dictionaries").
        const size_t wmodePos{ cmap.find("/WMode") };
        if (wmodePos == std::string_view::npos)
            {
            return false;
            }
        size_t pos{ wmodePos + 6 };
        pdf_lexer::skip_whitespace(cmap, pos);
        return pos < cmap.length() && cmap[pos] == '1';
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
        if (cmapName.find("-B5") != std::string_view::npos || cmapName.compare(0, 4, "B5pc") == 0)
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
    bool pdf_text_decoder::is_vertical_cmap_name(const std::string_view cmapName)
        {
        // Every one of Adobe's predefined CMap names (Identity-H/V and the CJK
        // families) ends in "-H" or "-V" (optionally preceded by "-HW" for the
        // fixed-pitch variants), naming the writing mode the CMap lays glyphs out
        // in. A "-V" font advances glyphs down a column and steps to a new column
        // horizontally, the reverse of the default horizontal layout.
        return cmapName.size() >= 2 && cmapName.compare(cmapName.size() - 2, 2, "-V") == 0;
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
            const size_t codeSize{ pdf_text_decoder::determine_code_length(bytes, i, fontDecoder) };
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
                const pdf_font_decoder::base_encoding_type baseEncoding{
                    (fontDecoder != nullptr) ? fontDecoder->m_base_encoding :
                                               pdf_font_decoder::base_encoding_type::win_ansi
                };
                result +=
                    (baseEncoding == pdf_font_decoder::base_encoding_type::mac_roman) ?
                        pdf_text_decoder::mac_roman_to_unicode(static_cast<unsigned char>(code)) :
                    (baseEncoding == pdf_font_decoder::base_encoding_type::standard) ?
                        pdf_text_decoder::standard_to_unicode(static_cast<unsigned char>(code)) :
                    (baseEncoding == pdf_font_decoder::base_encoding_type::symbol) ?
                        pdf_text_decoder::symbol_to_unicode(static_cast<unsigned char>(code)) :
                    (baseEncoding == pdf_font_decoder::base_encoding_type::zapf_dingbats) ?
                        pdf_text_decoder::zapf_dingbats_to_unicode(
                            static_cast<unsigned char>(code)) :
                        pdf_text_decoder::cp1252_to_unicode(static_cast<unsigned char>(code));
                }
            // else: a 3-byte or 4-byte code with no ToUnicode mapping; there's no
            // fallback encoding table for widths like this, so it's dropped
            i += codeSize;
            }
        return result;
        }
    } // namespace lily_of_the_valley
