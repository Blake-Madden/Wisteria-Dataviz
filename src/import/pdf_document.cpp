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
                               const glyph_name_table* glyphTable)
        : m_file(fileContent), m_decompress(decompressor), m_log(std::move(logFunction)),
          m_glyph_name_table(glyphTable)
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
        const std::string_view encoding{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(fontDictionary, "Encoding")) };
        if (encoding.compare(0, 9, "/Identity") == 0)
            {
            decoder->m_bytes_per_code = 2;
            }
        // a Type0 font's /Encoding may instead name one of Adobe's other predefined
        // CMaps, which determine how the font's string bytes are encoded
        else if (encoding.compare(0, 1, "/") == 0)
            {
            const std::string_view encodingName{ encoding.substr(1) };
            if (pdf_text_decoder::is_unicode_cmap_name(encodingName))
                {
                // string bytes are UTF-16BE
                decoder->m_codes_are_utf16 = true;
                decoder->m_bytes_per_code = 2;
                }
            else
                {
                // string bytes are text in a legacy CJK charset (Big5, Shift-JIS,
                // etc.), which the connected charset converter (if any) can decode
                const std::string_view charsetName{ pdf_text_decoder::predefined_cmap_charset(
                    encodingName) };
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

        const std::string_view baseFont{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(fontDictionary, "BaseFont")) };
        if (baseFont.find("Symbol") != std::string_view::npos ||
            baseFont.find("Dingbat") != std::string_view::npos ||
            baseFont.find("Wingding") != std::string_view::npos)
            {
            decoder->m_symbol_font = true;
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
        return decoder;
        }

    //------------------------------------------------------------------
    const std::vector<long>& pdf_document::get_scan_order() const noexcept
        {
        return m_object_scan_order;
        }
    } // namespace lily_of_the_valley
