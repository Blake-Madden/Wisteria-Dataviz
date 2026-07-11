///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_document.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_document.h"
#include "pdf_extract_text.h"
#include <algorithm>
#include <array>
#include <charconv>
#include <cstdint>
#include <memory>
#include <tuple>
#include <utility>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    // stream filters (other than FlateDecode, which is delegated to the
    // client-provided decompression functor)
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    std::string pdf_document::ascii_hex_decode(const std::string_view data)
        {
        std::string bytes;
        bytes.reserve(data.length() / 2);
        int highNibble{ -1 };
        for (const char curChar : data)
            {
            if (curChar == '>')
                {
                break;
                }
            const int digitValue{ pdf_lexer::hex_digit_value(curChar) };
            if (digitValue < 0)
                {
                continue;
                }
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
        if (highNibble >= 0)
            {
            bytes += static_cast<char>(highNibble << 4);
            }
        return bytes;
        }

    //------------------------------------------------------------------
    std::string pdf_document::ascii85_decode(const std::string_view data)
        {
        std::string bytes;
        bytes.reserve((data.length() * 4) / 5);
        uint32_t group{ 0 };
        size_t groupSize{ 0 };
        size_t pos{ 0 };
        // an optional "<~" header
        if (data.compare(0, 2, "<~") == 0)
            {
            pos = 2;
            }
        for (/*initialized*/; pos < data.length(); ++pos)
            {
            const char curChar{ data[pos] };
            if (pdf_lexer::is_whitespace(curChar))
                {
                continue;
                }
            if (curChar == '~')
                {
                break;
                }
            if (curChar == 'z' && groupSize == 0)
                {
                bytes.append(4, '\0');
                continue;
                }
            if (curChar < '!' || curChar > 'u')
                {
                continue;
                }
            group = (group * 85) + static_cast<uint32_t>(curChar - '!');
            if (++groupSize == 5)
                {
                for (int shift = 24; shift >= 0; shift -= 8)
                    {
                    bytes += static_cast<char>((group >> shift) & 0xFF);
                    }
                group = 0;
                groupSize = 0;
                }
            }
        // partial final group
        if (groupSize > 1)
            {
            const size_t padCount{ 5 - groupSize };
            for (size_t i = 0; i < padCount; ++i)
                {
                group = (group * 85) + 84; // pad with 'u'
                }
            for (size_t i = 0; i < (groupSize - 1); ++i)
                {
                bytes += static_cast<char>((group >> (24 - (i * 8))) & 0xFF);
                }
            }
        return bytes;
        }

    //------------------------------------------------------------------
    std::string pdf_document::lzw_decode(const std::string_view data)
        {
        // the (rare) alternate LZW flavor that some very old encoders emit;
        // Adobe's implementation doesn't support it either
        if (data.length() >= 2 && static_cast<unsigned char>(data[0]) == 0 &&
            static_cast<unsigned char>(data[1]) == 1)
            {
            return {};
            }

        constexpr int clearTableCode{ 256 };
        constexpr int eodCode{ 257 };
        constexpr size_t stringTableSize{ 4096 };
        constexpr std::array<int, 4> andTable{ 511, 1023, 2047, 4095 };

        std::vector<std::string> stringTable(stringTableSize);
        int tableIndex{ 0 };
        int bitsToGet{ 9 };
        const auto initializeStringTable = [&stringTable, &tableIndex, &bitsToGet]()
        {
            for (int byteValue = 0; byteValue < 256; ++byteValue)
                {
                stringTable[static_cast<size_t>(byteValue)].assign(1, static_cast<char>(byteValue));
                }
            tableIndex = 258;
            bitsToGet = 9;
        };
        initializeStringTable();

        size_t bytePointer{ 0 };
        uint32_t nextData{ 0 };
        int nextBits{ 0 };
        const auto getNextCode = [&]() -> int
        {
            if (bytePointer >= data.length())
                {
                return eodCode;
                }
            nextData = (nextData << 8) | static_cast<unsigned char>(data[bytePointer++]);
            nextBits += 8;
            if (nextBits < bitsToGet)
                {
                if (bytePointer >= data.length())
                    {
                    return eodCode;
                    }
                nextData = (nextData << 8) | static_cast<unsigned char>(data[bytePointer++]);
                nextBits += 8;
                }
            const int code{ static_cast<int>((nextData >> (nextBits - bitsToGet)) &
                                             static_cast<uint32_t>(andTable[bitsToGet - 9])) };
            nextBits -= bitsToGet;
            return code;
        };
        // appends a new entry (the old code's string, plus one byte) to the table and
        // widens the code size once the table grows past the current width's capacity
        // ("early change," as required by the PDF spec's LZWDecode filter)
        const auto addStringToTable = [&](const int previousCode, const char newByteValue)
        {
            if (tableIndex >= static_cast<int>(stringTableSize))
                {
                return;
                }
            stringTable[static_cast<size_t>(tableIndex)] =
                stringTable[static_cast<size_t>(previousCode)] + newByteValue;
            ++tableIndex;
            if (tableIndex == 511)
                {
                bitsToGet = 10;
                }
            else if (tableIndex == 1023)
                {
                bitsToGet = 11;
                }
            else if (tableIndex == 2047)
                {
                bitsToGet = 12;
                }
        };

        std::string output;
        output.reserve(data.length() * 3);
        int oldCode{ 0 };
        for (int code = getNextCode(); code != eodCode; code = getNextCode())
            {
            if (code == clearTableCode)
                {
                initializeStringTable();
                code = getNextCode();
                if (code == eodCode)
                    {
                    break;
                    }
                output += stringTable[static_cast<size_t>(code)];
                oldCode = code;
                }
            else if (code < tableIndex)
                {
                output += stringTable[static_cast<size_t>(code)];
                addStringToTable(oldCode, stringTable[static_cast<size_t>(code)].front());
                oldCode = code;
                }
            else if (!stringTable[static_cast<size_t>(oldCode)].empty())
                {
                // "KwK" case: the code refers to a table entry not yet written
                // (it is being defined by this very step), so it must be built
                // from the previous code's string
                const int newIndex{ tableIndex };
                addStringToTable(oldCode, stringTable[static_cast<size_t>(oldCode)].front());
                if (newIndex >= static_cast<int>(stringTableSize))
                    {
                    break;
                    }
                output += stringTable[static_cast<size_t>(newIndex)];
                oldCode = code;
                }
            else
                {
                // malformed stream; stop rather than reference an empty entry
                break;
                }
            }
        return output;
        }

    //------------------------------------------------------------------
    std::string pdf_document::apply_png_predictor(const std::string& data, const size_t columns,
                                                  const size_t colors,
                                                  const size_t bitsPerComponent)
        {
        const size_t bytesPerPixel{ std::max<size_t>(1, (colors * bitsPerComponent) / 8) };
        const size_t rowLength{ ((columns * colors * bitsPerComponent) + 7) / 8 };
        if (rowLength == 0)
            {
            return {};
            }
        std::string output;
        output.reserve(data.length());
        std::vector<unsigned char> previousRow(rowLength, 0);
        std::vector<unsigned char> currentRow(rowLength, 0);
        size_t pos{ 0 };
        while ((pos + 1) <= data.length())
            {
            const unsigned char filterType{ static_cast<unsigned char>(data[pos++]) };
            const size_t available{ std::min(rowLength, data.length() - pos) };
            for (size_t i = 0; i < available; ++i)
                {
                currentRow[i] = static_cast<unsigned char>(data[pos + i]);
                }
            for (size_t i = available; i < rowLength; ++i)
                {
                currentRow[i] = 0;
                }
            pos += available;
            for (size_t i = 0; i < rowLength; ++i)
                {
                const unsigned char left{ (i >= bytesPerPixel) ? currentRow[i - bytesPerPixel] :
                                                                 static_cast<unsigned char>(0) };
                const unsigned char above{ previousRow[i] };
                const unsigned char upperLeft{ (i >= bytesPerPixel) ?
                                                   previousRow[i - bytesPerPixel] :
                                                   static_cast<unsigned char>(0) };
                switch (filterType)
                    {
                case 0: // none
                    break;
                case 1: // sub
                    currentRow[i] = static_cast<unsigned char>(currentRow[i] + left);
                    break;
                case 2: // up
                    currentRow[i] = static_cast<unsigned char>(currentRow[i] + above);
                    break;
                case 3: // average
                    currentRow[i] =
                        static_cast<unsigned char>(currentRow[i] + ((left + above) / 2));
                    break;
                case 4: // Paeth
                    {
                    const int paethBase{ left + above - upperLeft };
                    const int distLeft{ std::abs(paethBase - left) };
                    const int distAbove{ std::abs(paethBase - above) };
                    const int distUpperLeft{ std::abs(paethBase - upperLeft) };
                    const unsigned char predictor{
                        (distLeft <= distAbove && distLeft <= distUpperLeft) ? left :
                        (distAbove <= distUpperLeft)                         ? above :
                                                                               upperLeft
                    };
                    currentRow[i] = static_cast<unsigned char>(currentRow[i] + predictor);
                    break;
                    }
                default:
                    break;
                    }
                }
            output.append(reinterpret_cast<const char*>(currentRow.data()), rowLength);
            previousRow = currentRow;
            }
        return output;
        }

    //------------------------------------------------------------------
    // document catalog
    //------------------------------------------------------------------

    //------------------------------------------------------------------
    pdf_document::pdf_document(const std::string_view fileContent,
                               const stream_decompress_functor& decompressor,
                               std::function<void(const std::wstring&)> logFunction,
                               const glyph_name_table* glyphTable,
                               const cid_to_unicode_registry* cidTables)
        : m_file(fileContent), m_decompress(decompressor), m_log(std::move(logFunction)),
          m_glyph_name_table(glyphTable), m_cid_to_unicode_tables(cidTables)
        {
        }

    //------------------------------------------------------------------
    void pdf_document::catalog_objects()
        {
        size_t pos{ 0 };
        while ((pos = m_file.find("obj", pos)) != std::string_view::npos)
            {
            const size_t keywordPos{ pos };
            pos += 3;
            // the keyword must be followed by whitespace or a delimiter
            if (pos < m_file.length() && !pdf_lexer::is_token_end(m_file[pos]))
                {
                continue;
                }
            // Walk back over "N G " to verify (and read) the object number.
            // The expected pattern is:
            //     <whitespace> digit+ <whitespace> digit+ <whitespace> "obj"
            size_t backPos{ keywordPos };
            while (backPos > 0 && pdf_lexer::is_whitespace(m_file[backPos - 1]))
                {
                --backPos;
                }
            const size_t genEnd{ backPos };
            while (backPos > 0 && pdf_lexer::is_digit(m_file[backPos - 1]))
                {
                --backPos;
                }
            const size_t genStart{ backPos };
            while (backPos > 0 && pdf_lexer::is_whitespace(m_file[backPos - 1]))
                {
                --backPos;
                }
            const size_t numEnd{ backPos };
            while (backPos > 0 && pdf_lexer::is_digit(m_file[backPos - 1]))
                {
                --backPos;
                }
            const size_t numStart{ backPos };
            if (genStart == genEnd || numStart == numEnd || genEnd == keywordPos ||
                numEnd == genStart ||
                (numStart > 0 && !pdf_lexer::is_token_end(m_file[numStart - 1])))
                {
                continue;
                }
            long objectNumber{ 0 };
            if (std::from_chars(m_file.data() + numStart, m_file.data() + numEnd, objectNumber)
                    .ec != std::errc{})
                {
                continue;
                }
            // a malformed generation number just falls back to 0, rather than
            // abandoning an otherwise well-formed object
            long generationNumber{ 0 };
            std::ignore =
                std::from_chars(m_file.data() + genStart, m_file.data() + genEnd, generationNumber);

            pdf_object currentObject;
            currentObject.m_object_number = objectNumber;
            currentObject.m_generation = generationNumber;
            pdf_lexer::skip_whitespace(m_file, pos);
            if (m_file.compare(pos, 2, "<<") == 0)
                {
                currentObject.m_dictionary = pdf_lexer::read_value(m_file, pos);
                pdf_lexer::skip_whitespace(m_file, pos);
                if (m_file.compare(pos, 6, "stream") == 0)
                    {
                    pos += 6;
                    if (pos < m_file.length() && m_file[pos] == '\r')
                        {
                        ++pos;
                        }
                    if (pos < m_file.length() && m_file[pos] == '\n')
                        {
                        ++pos;
                        }
                    const size_t dataStart{ pos };
                    size_t dataEnd{ std::string_view::npos };
                    // Prefer the /Length entry for finding the end of the payload.
                    // Payload is binary and could coincidentally contain
                    // the "endstream" keyword.
                    long streamLength{ 0 };
                    if (pdf_lexer::to_int(
                            pdf_lexer::trim(resolve_value(pdf_lexer::find_dictionary_value(
                                currentObject.m_dictionary, "Length"))),
                            streamLength) &&
                        streamLength >= 0 &&
                        (dataStart + static_cast<size_t>(streamLength)) <= m_file.length())
                        {
                        size_t probePos{ dataStart + static_cast<size_t>(streamLength) };
                        pdf_lexer::skip_whitespace(m_file, probePos);
                        if (m_file.compare(probePos, 9, "endstream") == 0)
                            {
                            dataEnd = dataStart + static_cast<size_t>(streamLength);
                            pos = probePos + 9;
                            }
                        }
                    if (dataEnd == std::string_view::npos)
                        {
                        const size_t endKeyword{ m_file.find("endstream", dataStart) };
                        if (endKeyword == std::string_view::npos)
                            {
                            // This object's stream is unusable, but later objects in the
                            // file may still be well-formed. Skip it and keep scanning
                            // (rather than abandoning the rest of the file).
                            m_log(L"\"endstream\" keyword missing in PDF file.");
                            continue;
                            }
                        dataEnd = endKeyword;
                        // don't include the EOL in front of "endstream" in the payload
                        if (dataEnd > dataStart && m_file[dataEnd - 1] == '\n')
                            {
                            --dataEnd;
                            }
                        if (dataEnd > dataStart && m_file[dataEnd - 1] == '\r')
                            {
                            --dataEnd;
                            }
                        pos = endKeyword + 9;
                        }
                    currentObject.m_stream_data = m_file.substr(dataStart, dataEnd - dataStart);
                    }
                }
            else
                {
                // a non-dictionary object; its value runs up to "endobj"
                const size_t endKeyword{ m_file.find("endobj", pos) };
                if (endKeyword == std::string_view::npos)
                    {
                    currentObject.m_value = pdf_lexer::trim(m_file.substr(pos));
                    pos = m_file.length();
                    }
                else
                    {
                    currentObject.m_value = pdf_lexer::trim(m_file.substr(pos, endKeyword - pos));
                    pos = endKeyword + 6;
                    }
                }
            // incrementally-updated PDFs (re-saved by Acrobat, form-filled, annotated, etc.)
            // append newer bodies for the same object number later in the file. This scan
            // runs forward, so the later (i.e., newest) occurrence must win, not the first.
            const bool isNewObject{ m_objects.find(objectNumber) == m_objects.cend() };
            m_objects[objectNumber] = currentObject;
            if (isNewObject)
                {
                m_object_scan_order.push_back(objectNumber);
                }
            }
        exclude_free_objects();
        }

    //------------------------------------------------------------------
    void pdf_document::exclude_free_objects()
        {
        // object number -> free status; a later xref record for the same
        // object number overwrites an earlier one, just as catalog_objects()
        // lets a later object body win over an earlier one
        std::map<long, bool> freeStatus;

        // classic cross-reference tables: an "xref" keyword followed by one or
        // more subsections, each a "firstObjectNumber count" header followed by
        // `count` entries of the form "offset generation n_or_f"
        size_t pos{ 0 };
        while ((pos = m_file.find("xref", pos)) != std::string_view::npos)
            {
            const size_t keywordPos{ pos };
            pos += 4;
            // skip "startxref" (the pointer to the last xref section's file
            // offset, not a table itself) and any non-standalone match
            if ((keywordPos >= 5 && m_file.compare(keywordPos - 5, 5, "start") == 0) ||
                (keywordPos > 0 && !pdf_lexer::is_token_end(m_file[keywordPos - 1])) ||
                (pos < m_file.length() && !pdf_lexer::is_token_end(m_file[pos])))
                {
                continue;
                }
            while (true)
                {
                const size_t beforeHeader{ pos };
                long firstObjectNumber{ 0 }, entryCount{ 0 };
                if (!pdf_lexer::read_int(m_file, pos, firstObjectNumber) ||
                    !pdf_lexer::read_int(m_file, pos, entryCount) || entryCount < 0)
                    {
                    // not a subsection header; the table has ended
                    // (a "trailer" keyword or another "xref" section follows)
                    pos = beforeHeader;
                    break;
                    }
                for (long i = 0; i < entryCount; ++i)
                    {
                    // the offset and generation fields aren't needed here (object
                    // bodies are located independently by the "obj" scan above),
                    // just validated and skipped over to reach the status flag
                    [[maybe_unused]]
                    long entryOffset{ 0 };
                    [[maybe_unused]]
                    long entryGeneration{ 0 };
                    if (!pdf_lexer::read_int(m_file, pos, entryOffset) ||
                        !pdf_lexer::read_int(m_file, pos, entryGeneration))
                        {
                        break;
                        }
                    pdf_lexer::skip_whitespace(m_file, pos);
                    if (pos >= m_file.length())
                        {
                        break;
                        }
                    const char statusChar{ m_file[pos] };
                    if (statusChar != 'f' && statusChar != 'n')
                        {
                        break;
                        }
                    freeStatus[firstObjectNumber + i] = (statusChar == 'f');
                    ++pos;
                    }
                }
            }

        // cross-reference streams (/Type /XRef, PDF 1.5+); these were already
        // cataloged as ordinary objects above, since they're just another
        // "N G obj ... endobj" body with a dictionary and a stream
        for (const long objectNumber : m_object_scan_order)
            {
            const pdf_object* xrefObject{ find_object(objectNumber) };
            if (xrefObject == nullptr || xrefObject->m_dictionary.empty() ||
                xrefObject->m_stream_data.empty() ||
                pdf_lexer::trim(
                    pdf_lexer::find_dictionary_value(xrefObject->m_dictionary, "Type")) != "/XRef")
                {
                continue;
                }
            apply_xref_stream_free_entries(*xrefObject, freeStatus);
            }

        for (const auto& [objectNumber, isFree] : freeStatus)
            {
            if (isFree)
                {
                m_objects.erase(objectNumber);
                m_object_scan_order.erase(std::remove(m_object_scan_order.begin(),
                                                      m_object_scan_order.end(), objectNumber),
                                          m_object_scan_order.end());
                }
            }
        }

    //------------------------------------------------------------------
    void pdf_document::apply_xref_stream_free_entries(const pdf_object& xrefObject,
                                                      std::map<long, bool>& freeStatus) const
        {
        // /W is an array of 3 field widths (in bytes): [type, field2, field3]
        std::array<long, 3> widths{ 0, 0, 0 };
        const std::string_view widthValue{ pdf_lexer::trim(
            resolve_value(pdf_lexer::find_dictionary_value(xrefObject.m_dictionary, "W"))) };
        if (widthValue.empty() || widthValue.front() != '[')
            {
            return;
            }
        size_t widthPos{ 1 };
        for (long& width : widths)
            {
            pdf_lexer::skip_whitespace(widthValue, widthPos);
            if (!pdf_lexer::read_int(widthValue, widthPos, width) || width < 0 || width > 8)
                {
                return;
                }
            }
        const size_t entryWidth{ static_cast<size_t>(widths[0] + widths[1] + widths[2]) };
        if (entryWidth == 0)
            {
            return;
            }

        // /Index is pairs of (firstObjectNumber, count) subsections;
        // defaults to a single [0, /Size] subsection when absent
        std::vector<std::pair<long, long>> subsections;
        const std::string_view indexValue{ pdf_lexer::trim(
            resolve_value(pdf_lexer::find_dictionary_value(xrefObject.m_dictionary, "Index"))) };
        if (!indexValue.empty() && indexValue.front() == '[')
            {
            size_t indexPos{ 1 };
            while (true)
                {
                pdf_lexer::skip_whitespace(indexValue, indexPos);
                if (indexPos >= indexValue.length() || indexValue[indexPos] == ']')
                    {
                    break;
                    }
                long first{ 0 }, count{ 0 };
                if (!pdf_lexer::read_int(indexValue, indexPos, first) ||
                    !pdf_lexer::read_int(indexValue, indexPos, count) || count < 0)
                    {
                    break;
                    }
                subsections.emplace_back(first, count);
                }
            }
        else
            {
            long size{ 0 };
            if (pdf_lexer::to_int(pdf_lexer::trim(resolve_value(pdf_lexer::find_dictionary_value(
                                      xrefObject.m_dictionary, "Size"))),
                                  size) &&
                size > 0)
                {
                subsections.emplace_back(0, size);
                }
            }
        if (subsections.empty())
            {
            return;
            }

        const std::string decoded{ decode_stream(xrefObject) };
        if (decoded.empty())
            {
            return;
            }

        size_t streamPos{ 0 };
        for (const auto& [firstObjectNumber, count] : subsections)
            {
            for (long i = 0; i < count; ++i)
                {
                if ((streamPos + entryWidth) > decoded.length())
                    {
                    return;
                    }
                // field 1 (the entry type) defaults to 1 (in use) when its
                // width is 0, per the spec. Widened to 64 bits while
                // accumulating since widths[0] can be up to 8 bytes.
                uint64_t type{ 1 };
                if (widths[0] > 0)
                    {
                    type = 0;
                    for (long byteIndex = 0; byteIndex < widths[0]; ++byteIndex)
                        {
                        type =
                            (type << 8) | static_cast<unsigned char>(
                                              decoded[streamPos + static_cast<size_t>(byteIndex)]);
                        }
                    }
                streamPos += entryWidth;
                // type 0 = free, type 1 = in use (uncompressed). Type 2 (in use,
                // packed in an object stream) needs no entry here: it's neither
                // free nor governed by this table, it's handled separately when
                // expand_object_streams() unpacks its containing /ObjStm.
                if (type == 0 || type == 1)
                    {
                    freeStatus[firstObjectNumber + i] = (type == 0);
                    }
                }
            }
        }

    //------------------------------------------------------------------
    void pdf_document::expand_object_streams()
        {
        const std::vector<long> topLevelObjects{ m_object_scan_order };
        for (const long objectNumber : topLevelObjects)
            {
            const pdf_object* streamObject{ find_object(objectNumber) };
            if (streamObject == nullptr || streamObject->m_dictionary.empty() ||
                streamObject->m_stream_data.empty() ||
                pdf_lexer::trim(pdf_lexer::find_dictionary_value(streamObject->m_dictionary,
                                                                 "Type")) != "/ObjStm")
                {
                continue;
                }
            std::string decodedData{ decode_stream(*streamObject) };
            if (decodedData.empty())
                {
                m_log(L"Unable to decode a compressed object stream in PDF file.");
                continue;
                }
            long innerCount{ 0 }, firstOffset{ 0 };
            if (!pdf_lexer::to_int(pdf_lexer::trim(resolve_value(pdf_lexer::find_dictionary_value(
                                       streamObject->m_dictionary, "N"))),
                                   innerCount) ||
                !pdf_lexer::to_int(pdf_lexer::trim(resolve_value(pdf_lexer::find_dictionary_value(
                                       streamObject->m_dictionary, "First"))),
                                   firstOffset) ||
                innerCount <= 0 || firstOffset <= 0 ||
                std::cmp_greater_equal(firstOffset, decodedData.length()))
                {
                continue;
                }
            // take ownership of the decompressed data so that views into it stay valid
            m_decompressed_buffers.push_back(std::move(decodedData));
            const std::string_view streamView{ m_decompressed_buffers.back() };

            // the header is N pairs of "objectnumber offset"
            std::vector<std::pair<long, long>> innerObjects;
            size_t headerPos{ 0 };
            const std::string_view headerView{ streamView.substr(
                0, static_cast<size_t>(firstOffset)) };
            for (long i = 0; i < innerCount; ++i)
                {
                long innerNumber{ 0 }, innerOffset{ 0 };
                if (!pdf_lexer::read_int(headerView, headerPos, innerNumber) ||
                    !pdf_lexer::read_int(headerView, headerPos, innerOffset))
                    {
                    break;
                    }
                innerObjects.emplace_back(innerNumber, innerOffset);
                }
            for (size_t i = 0; i < innerObjects.size(); ++i)
                {
                const size_t valueStart{ static_cast<size_t>(firstOffset) +
                                         static_cast<size_t>(innerObjects[i].second) };
                if (valueStart >= streamView.length())
                    {
                    continue;
                    }
                const size_t valueEnd{ ((i + 1) < innerObjects.size()) ?
                                           std::min(streamView.length(),
                                                    static_cast<size_t>(firstOffset) +
                                                        static_cast<size_t>(
                                                            innerObjects[i + 1].second)) :
                                           streamView.length() };
                if (valueEnd <= valueStart)
                    {
                    continue;
                    }
                const std::string_view valueView{ pdf_lexer::trim(
                    streamView.substr(valueStart, valueEnd - valueStart)) };
                pdf_object innerObject;
                innerObject.m_object_number = innerObjects[i].first;
                innerObject.m_from_object_stream = true;
                if (valueView.compare(0, 2, "<<") == 0)
                    {
                    innerObject.m_dictionary = valueView;
                    }
                else
                    {
                    innerObject.m_value = valueView;
                    }
                if (m_objects.insert(std::make_pair(innerObjects[i].first, innerObject)).second)
                    {
                    m_object_scan_order.push_back(innerObjects[i].first);
                    }
                }
            }
        }

    //------------------------------------------------------------------
    const pdf_object* pdf_document::find_object(const long objectNumber) const
        {
        const auto objectPos = m_objects.find(objectNumber);
        return (objectPos == m_objects.cend()) ? nullptr : &objectPos->second;
        }

    //------------------------------------------------------------------
    const pdf_object* pdf_document::resolve_to_object(const std::string_view value) const
        {
        long objectNumber{ 0 };
        if (pdf_lexer::get_reference(value, objectNumber))
            {
            return find_object(objectNumber);
            }
        return nullptr;
        }

    //------------------------------------------------------------------
    std::string pdf_document::decrypt(const pdf_object& owningObject,
                                      const std::string_view bytes) const
        {
        if (m_decryptor == nullptr || owningObject.m_from_object_stream)
            {
            return std::string{ bytes };
            }
        return m_decryptor->decrypt(owningObject.m_object_number, owningObject.m_generation, bytes);
        }

    //------------------------------------------------------------------
    std::string_view pdf_document::resolve_value(std::string_view value) const
        {
        for (int depth = 0; depth < 8; ++depth)
            {
            const pdf_object* referencedObject{ resolve_to_object(value) };
            if (referencedObject == nullptr)
                {
                return value;
                }
            value = !referencedObject->m_dictionary.empty() ? referencedObject->m_dictionary :
                                                              referencedObject->m_value;
            }
        return value;
        }

    //------------------------------------------------------------------
    std::string pdf_document::decode_stream(const pdf_object& streamObject) const
        {
        if (streamObject.m_stream_data.empty())
            {
            return {};
            }
        // gather the filter chain (a single name or an array of names)
        std::vector<std::string_view> filters;
        const std::string_view filterValue{ pdf_lexer::trim(
            resolve_value(pdf_lexer::find_dictionary_value(streamObject.m_dictionary, "Filter"))) };
        if (!filterValue.empty() && filterValue.front() == '[')
            {
            size_t pos{ 1 };
            while (pos < filterValue.length())
                {
                pdf_lexer::skip_whitespace(filterValue, pos);
                if (pos >= filterValue.length() || filterValue[pos] == ']')
                    {
                    break;
                    }
                const std::string_view element{ pdf_lexer::trim(
                    pdf_lexer::read_value(filterValue, pos)) };
                if (element.empty())
                    {
                    break;
                    }
                filters.push_back(element);
                }
            }
        else if (!filterValue.empty())
            {
            filters.push_back(filterValue);
            }

        // gather the parallel /DecodeParms (or abbreviated /DP) entries, if any. This can be
        // a single dictionary (applying to the lone filter) or an array running parallel to
        // /Filter (one entry, possibly `null`, per filter in the chain).
        std::vector<std::string_view> filterParms;
        std::string_view parmsValue{ pdf_lexer::trim(resolve_value(
            pdf_lexer::find_dictionary_value(streamObject.m_dictionary, "DecodeParms"))) };
        if (parmsValue.empty())
            {
            parmsValue = pdf_lexer::trim(
                resolve_value(pdf_lexer::find_dictionary_value(streamObject.m_dictionary, "DP")));
            }
        if (!parmsValue.empty() && parmsValue.front() == '[')
            {
            size_t pos{ 1 };
            while (pos < parmsValue.length())
                {
                pdf_lexer::skip_whitespace(parmsValue, pos);
                if (pos >= parmsValue.length() || parmsValue[pos] == ']')
                    {
                    break;
                    }
                const std::string_view element{ pdf_lexer::trim(
                    pdf_lexer::read_value(parmsValue, pos)) };
                if (element.empty())
                    {
                    break;
                    }
                filterParms.push_back(element);
                }
            }
        else if (!parmsValue.empty())
            {
            filterParms.push_back(parmsValue);
            }

        // encryption (if any) is applied to the raw stream bytes before its filter
        // chain runs, so it must be reversed first
        std::string data{ decrypt(streamObject, streamObject.m_stream_data) };
        for (size_t filterIndex = 0; filterIndex < filters.size(); ++filterIndex)
            {
            const std::string_view filter{ filters[filterIndex] };
            if (filter == "/FlateDecode" || filter == "/Fl")
                {
                if (!m_decompress)
                    {
                    m_log(L"Compressed section in PDF file skipped "
                          "(no stream decompressor was connected).");
                    return {};
                    }
                data = m_decompress(data);
                if (data.empty())
                    {
                    m_log(L"Unable to decompress a stream section in PDF file.");
                    return {};
                    }
                }
            else if (filter == "/ASCIIHexDecode" || filter == "/AHx")
                {
                data = pdf_document::ascii_hex_decode(data);
                }
            else if (filter == "/ASCII85Decode" || filter == "/A85")
                {
                data = pdf_document::ascii85_decode(data);
                }
            else if (filter == "/LZWDecode" || filter == "/LZW")
                {
                data = pdf_document::lzw_decode(data);
                if (data.empty())
                    {
                    m_log(L"Unable to decode an LZW-compressed stream section in PDF file.");
                    return {};
                    }
                }
            else
                {
                // image (DCTDecode et al.) streams are not supported
                m_log(L"Unsupported stream filter in PDF file; "
                      "content from this section will be skipped.");
                return {};
                }

            // reverse any PNG predictor that was applied (for this specific filter step)
            // before compression
            const std::string_view parms{ (filterIndex < filterParms.size()) ?
                                              pdf_lexer::trim(
                                                  resolve_value(filterParms[filterIndex])) :
                                              std::string_view{} };
            if (parms.compare(0, 2, "<<") == 0)
                {
                long predictor{ 0 };
                if (pdf_lexer::to_int(pdf_lexer::trim(resolve_value(
                                          pdf_lexer::find_dictionary_value(parms, "Predictor"))),
                                      predictor) &&
                    predictor >= 10)
                    {
                    long columns{ 1 }, colors{ 1 }, bitsPerComponent{ 8 };
                    std::ignore =
                        pdf_lexer::to_int(pdf_lexer::trim(resolve_value(
                                              pdf_lexer::find_dictionary_value(parms, "Columns"))),
                                          columns);
                    std::ignore =
                        pdf_lexer::to_int(pdf_lexer::trim(resolve_value(
                                              pdf_lexer::find_dictionary_value(parms, "Colors"))),
                                          colors);
                    std::ignore = pdf_lexer::to_int(
                        pdf_lexer::trim(resolve_value(
                            pdf_lexer::find_dictionary_value(parms, "BitsPerComponent"))),
                        bitsPerComponent);
                    if (columns > 0 && colors > 0 && bitsPerComponent > 0)
                        {
                        data = pdf_document::apply_png_predictor(
                            data, static_cast<size_t>(columns), static_cast<size_t>(colors),
                            static_cast<size_t>(bitsPerComponent));
                        }
                    }
                }
            }
        return data;
        }

    //------------------------------------------------------------------
    std::shared_ptr<pdf_font_decoder> pdf_document::load_font(const long objectNumber)
        {
        const auto cachedPos = m_font_cache.find(objectNumber);
        if (cachedPos != m_font_cache.cend())
            {
            return cachedPos->second;
            }
        std::shared_ptr<pdf_font_decoder> decoder;
        const pdf_object* fontObject{ find_object(objectNumber) };
        if (fontObject != nullptr && !fontObject->m_dictionary.empty())
            {
            decoder = load_font_from_dictionary(fontObject->m_dictionary);
            }
        else
            {
            decoder = std::make_shared<pdf_font_decoder>();
            }
        m_font_cache.insert(std::make_pair(objectNumber, decoder));
        return decoder;
        }

    //------------------------------------------------------------------
    std::shared_ptr<pdf_font_decoder>
    pdf_document::load_font_from_dictionary(const std::string_view fontDictionary)
        {
        auto decoder = std::make_shared<pdf_font_decoder>();
        if (pdf_lexer::trim(pdf_lexer::find_dictionary_value(fontDictionary, "Subtype")) ==
            "/Type0")
            {
            decoder->m_bytes_per_code = 2;
            }
        // applies the semantics of one of Adobe's predefined CMap names
        // (Identity, Unicode, or a legacy CJK charset) to the font's decoder
        const auto applyPredefinedCmapName = [this, &decoder](const std::string_view cmapName)
        {
            decoder->m_vertical_writing_mode = pdf_text_decoder::is_vertical_cmap_name(cmapName);
            if (pdf_text_decoder::is_unicode_cmap_name(cmapName))
                {
                // string bytes are UTF-16BE
                decoder->m_codes_are_utf16 = true;
                decoder->m_bytes_per_code = 2;
                return;
                }
            // string bytes are text in a legacy CJK charset (Big5, Shift-JIS,
            // etc.), which the connected charset converter (if any) can decode
            const std::string_view charsetName{ pdf_text_decoder::predefined_cmap_charset(
                cmapName) };
            if (!charsetName.empty())
                {
                decoder->m_charset = charsetName;
                decoder->m_charset_converter = m_charset_convert;
                if (!m_charset_convert)
                    {
                    m_log(L"Text encoded as " +
                          std::wstring(charsetName.cbegin(), charsetName.cend()) +
                          L" in PDF file skipped (no charset converter was connected).");
                    }
                }
        };

        const std::string_view encoding{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(fontDictionary, "Encoding")) };
        if (encoding.compare(0, 9, "/Identity") == 0)
            {
            decoder->m_bytes_per_code = 2;
            decoder->m_vertical_writing_mode =
                pdf_text_decoder::is_vertical_cmap_name(encoding.substr(1));
            }
        // a Type0 font's /Encoding may instead name one of Adobe's other predefined
        // CMaps, which determine how the font's string bytes are encoded
        else if (encoding.compare(0, 1, "/") == 0)
            {
            applyPredefinedCmapName(encoding.substr(1));
            }
        // ...or it may be an indirect reference to a font-embedded CMap stream (common
        // for subsetted CID fonts). Such a stream may itself chain to one of Adobe's
        // predefined CMaps via a "usecmap" directive, whose semantics we can then reuse.
        else if (!encoding.empty() && encoding.compare(0, 2, "<<") != 0)
            {
            const pdf_object* cmapStreamObject{ resolve_to_object(encoding) };
            if (cmapStreamObject != nullptr && !cmapStreamObject->m_stream_data.empty())
                {
                const std::string cmapStreamData{ decode_stream(*cmapStreamObject) };
                const std::string_view usecmapName{ pdf_text_decoder::parse_usecmap_name(
                    cmapStreamData) };
                if (!usecmapName.empty())
                    {
                    applyPredefinedCmapName(usecmapName);
                    }
                // The stream may also declare its writing mode directly via /WMode.
                if (pdf_text_decoder::parse_wmode_from_cmap_stream(cmapStreamData))
                    {
                    decoder->m_vertical_writing_mode = true;
                    }
                }
            }
        // the base encoding is either the /Encoding name directly, or (for a simple
        // font with custom glyph mappings) the /BaseEncoding entry inside an
        // /Encoding dictionary
        std::string_view baseEncodingName{ encoding };
        if (baseEncodingName.compare(0, 2, "<<") == 0)
            {
            baseEncodingName = pdf_lexer::trim(
                resolve_value(pdf_lexer::find_dictionary_value(baseEncodingName, "BaseEncoding")));
            }
        if (baseEncodingName == "/MacRomanEncoding")
            {
            decoder->m_base_encoding = pdf_font_decoder::base_encoding_type::mac_roman;
            }
        else if (baseEncodingName == "/StandardEncoding")
            {
            decoder->m_base_encoding = pdf_font_decoder::base_encoding_type::standard;
            }
        // ...otherwise leave the default (WinAnsiEncoding), which covers an explicit
        // /WinAnsiEncoding as well as the common case of no /Encoding entry at all.

        // A simple font may remap individual codes to named glyphs via a /Differences
        // array inside its /Encoding dictionary. Resolve those through the glyph
        // name table (if one was loaded)
        if (m_glyph_name_table != nullptr && !m_glyph_name_table->empty() &&
            encoding.compare(0, 2, "<<") == 0)
            {
            const std::string_view differences{ pdf_lexer::trim(
                resolve_value(pdf_lexer::find_dictionary_value(encoding, "Differences"))) };
            if (!differences.empty())
                {
                pdf_text_decoder::parse_differences_array(differences, *m_glyph_name_table,
                                                          *decoder);
                }
            }

        // A Symbol or ZapfDingbats font's codes are that font's own private encoding,
        // not Latin text, so its BaseFont name is used to pick the matching built-in
        // encoding table when the font itself gives no /Encoding to override it. (An
        // explicit /Encoding is respected as-is above, since a PDF may legitimately
        // remap even one of these fonts' codes.) Wingdings is excluded: unlike Symbol
        // and ZapfDingbats, its code-to-glyph mapping isn't a published Adobe
        // encoding, so there's no table to look it up in.
        if (encoding.empty())
            {
            const std::string_view baseFont{ pdf_lexer::trim(
                pdf_lexer::find_dictionary_value(fontDictionary, "BaseFont")) };
            if (baseFont.find("Symbol") != std::string_view::npos)
                {
                decoder->m_base_encoding = pdf_font_decoder::base_encoding_type::symbol;
                }
            else if (baseFont.find("Dingbat") != std::string_view::npos)
                {
                decoder->m_base_encoding = pdf_font_decoder::base_encoding_type::zapf_dingbats;
                }
            }
        const pdf_object* cmapObject{ resolve_to_object(
            pdf_lexer::find_dictionary_value(fontDictionary, "ToUnicode")) };
        if (cmapObject != nullptr && !cmapObject->m_stream_data.empty())
            {
            const std::string cmapData{ decode_stream(*cmapObject) };
            if (!cmapData.empty())
                {
                pdf_text_decoder::parse_unicode_cmap(cmapData, *decoder);
                }
            }
        // a /Type0 font's descendant is either a lone dictionary or the first
        // (and only) entry of a /DescendantFonts array
        const auto resolveDescendantFont = [this, &fontDictionary]() -> const pdf_object*
        {
            const std::string_view descendantFontsValue{ pdf_lexer::trim(
                pdf_lexer::find_dictionary_value(fontDictionary, "DescendantFonts")) };
            std::string_view descendantFontRef{ descendantFontsValue };
            if (!descendantFontsValue.empty() && descendantFontsValue.front() == '[')
                {
                size_t pos{ 1 };
                descendantFontRef = pdf_lexer::read_value(descendantFontsValue, pos);
                }
            return resolve_to_object(descendantFontRef);
        };

        // A Type0 font with no ToUnicode CMap has CIDs that are only interpretable
        // via an external CID-to-Unicode table for its /CIDSystemInfo character
        // collection, if the client loaded one. Skipped for fonts already resolved
        // via decoder->m_charset (a legacy CJK CMap encoding), since their codes
        // aren't CIDs and would map to unrelated, wrong-looking table entries.
        if (!decoder->m_has_unicode_map && decoder->m_charset.empty() &&
            m_cid_to_unicode_tables != nullptr && !m_cid_to_unicode_tables->empty())
            {
            const pdf_object* descendantFont{ resolveDescendantFont() };
            if (descendantFont != nullptr && !descendantFont->m_dictionary.empty())
                {
                const std::string_view cidSystemInfo{ pdf_lexer::trim(
                    resolve_value(pdf_lexer::find_dictionary_value(descendantFont->m_dictionary,
                                                                   "CIDSystemInfo"))) };
                // reads a /Registry or /Ordering literal string value
                const auto readName = [](const std::string_view value) -> std::string
                {
                    const std::string_view trimmed{ pdf_lexer::trim(value) };
                    size_t pos{ 0 };
                    return (!trimmed.empty() && trimmed.front() == '(') ?
                               pdf_lexer::read_literal_string(trimmed, pos) :
                               std::string{};
                };
                const std::string registryOrdering{
                    readName(pdf_lexer::find_dictionary_value(cidSystemInfo, "Registry")) + "-" +
                    readName(pdf_lexer::find_dictionary_value(cidSystemInfo, "Ordering"))
                };
                const auto tablePos = m_cid_to_unicode_tables->find(registryOrdering);
                if (tablePos != m_cid_to_unicode_tables->cend())
                    {
                    for (const auto& [cid, unicodeText] : tablePos->second)
                        {
                        decoder->m_code_map[cid] = unicodeText;
                        }
                    decoder->m_has_unicode_map = !decoder->m_code_map.empty();
                    }
                }
            }

        // A /Type0 font whose descendant is an embedded /CIDFontType2 (subsetted
        // TrueType) with no ToUnicode CMap and no external CID-to-Unicode table
        // still has one more source of truth: the embedded font's own cmap table.
        if (!decoder->m_has_unicode_map && decoder->m_charset.empty())
            {
            const pdf_object* descendantFont{ resolveDescendantFont() };
            if (descendantFont != nullptr && !descendantFont->m_dictionary.empty() &&
                pdf_lexer::trim(pdf_lexer::find_dictionary_value(descendantFont->m_dictionary,
                                                                 "Subtype")) == "/CIDFontType2")
                {
                const pdf_object* descriptorObject{ resolve_to_object(
                    pdf_lexer::find_dictionary_value(descendantFont->m_dictionary,
                                                     "FontDescriptor")) };
                const pdf_object* fontFileObject{
                    (descriptorObject != nullptr && !descriptorObject->m_dictionary.empty()) ?
                        resolve_to_object(pdf_lexer::find_dictionary_value(
                            descriptorObject->m_dictionary, "FontFile2")) :
                        nullptr
                };
                if (fontFileObject != nullptr && !fontFileObject->m_stream_data.empty())
                    {
                    const std::string fontProgram{ decode_stream(*fontFileObject) };
                    if (!fontProgram.empty())
                        {
                        // /CIDToGIDMap: absent/Identity, or a stream of 2-byte GIDs indexed by CID
                        std::string cidToGidMapData;
                        const pdf_object* cidToGidMapStream{ resolve_to_object(
                            pdf_lexer::find_dictionary_value(descendantFont->m_dictionary,
                                                             "CIDToGIDMap")) };
                        if (cidToGidMapStream != nullptr &&
                            !cidToGidMapStream->m_stream_data.empty())
                            {
                            cidToGidMapData = decode_stream(*cidToGidMapStream);
                            }
                        pdf_text_decoder::parse_embedded_cid_truetype_cmap(
                            fontProgram, cidToGidMapData, *decoder);
                        }
                    }
                }
            }

        // A simple TrueType font with no ToUnicode CMap and no (resolvable)
        // /Differences has no way to interpret its codes as text via the PDF
        // constructs above. As a last resort, fall back to the font's own embedded
        // cmap table (its /FontFile2 program), which can recover a code -> Unicode
        // mapping directly from the font.
        if (!decoder->m_has_unicode_map && pdf_lexer::trim(pdf_lexer::find_dictionary_value(
                                               fontDictionary, "Subtype")) == "/TrueType")
            {
            const pdf_object* descriptorObject{ resolve_to_object(
                pdf_lexer::find_dictionary_value(fontDictionary, "FontDescriptor")) };
            if (descriptorObject != nullptr && !descriptorObject->m_dictionary.empty())
                {
                const pdf_object* fontFileObject{ resolve_to_object(
                    pdf_lexer::find_dictionary_value(descriptorObject->m_dictionary,
                                                     "FontFile2")) };
                if (fontFileObject != nullptr && !fontFileObject->m_stream_data.empty())
                    {
                    const std::string fontProgram{ decode_stream(*fontFileObject) };
                    if (!fontProgram.empty())
                        {
                        pdf_text_decoder::parse_embedded_truetype_cmap(fontProgram, *decoder);
                        }
                    }
                }
            }
        return decoder;
        }

    //------------------------------------------------------------------
    const std::vector<long>& pdf_document::get_scan_order() const noexcept
        {
        return m_object_scan_order;
        }
    } // namespace lily_of_the_valley
