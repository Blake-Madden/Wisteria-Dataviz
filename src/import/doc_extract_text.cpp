///////////////////////////////////////////////////////////////////////////////
// Name:        doc_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2025 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "doc_extract_text.h"
#include "../utfcpp/source/utf8.h"
#include "html_extract_text.h"
#include "rtf_extract_text.h"

namespace lily_of_the_valley
    {
    const std::string word1997_extract_text::file_system_entry::ROOT_ENTRY = "Root Entry";
    const std::string word1997_extract_text::RTF_SIGNATURE = "{\\rtf";
    const uint8_t word1997_extract_text::UTF8_SIGNATURE[] = { 0xEF, 0xBB, 0xBF };
    const std::string word1997_extract_text::MAGIC_NUMBER = {
        -48, -49, 17, -32, -95, -79, 26, -31
    };
    const std::string word1997_extract_text::MAGIC_NUMBER_BETA = {
        14, 17, -4, 13, -48, -49, 17, 14
    };

    //----------------------------------------------------
    const wchar_t* word1997_extract_text::operator()(const char* const doc_buffer,
                                                     const size_t text_length)
        {
        clear_log();
        clear();
        reset();
        if (doc_buffer == nullptr || text_length == 0)
            {
            log_message(L"Empty buffer sent to DOC parser.");
            return nullptr;
            }

        m_file_end_sentinel = doc_buffer + text_length;

        auto input = cfb_iostream(doc_buffer, text_length + 1);

        allocate_text_buffer(text_length);

        std::vector<char> buffer(128, 0);
        if (input.read(buffer.data(), 8) < 8)
            {
            // couldn't read in the header for some reason
            log_message(L"DOC parser: error reading file header.");
            return nullptr;
            }

        if (MAGIC_NUMBER.compare(0, MAGIC_NUMBER.length(), buffer.data(), MAGIC_NUMBER.length()) ==
                0 ||
            MAGIC_NUMBER_BETA.compare(0, MAGIC_NUMBER_BETA.length(), buffer.data(),
                                      MAGIC_NUMBER_BETA.length()) == 0)
            {
            if (load_header(&input))
                {
                std::shared_ptr<file_system_entry> cfbObj;
                while ((cfbObj = read_next_file_system_entry(&input)) != nullptr)
                    {
                    if (cfbObj->open() == 0)
                        {
                        // the actual text body
                        if (cfbObj->m_name == "WordDocument")
                            {
                            try
                                {
                                load_document(cfbObj.get());
                                }
                            catch (...)
                                {
                                log_message(L"DOC parser: error loading main body of document.");
                                throw;
                                }
                            }
                        // document meta data
                        else if (cfbObj->m_name == "\005SummaryInformation")
                            {
                            load_summary_information(cfbObj.get());
                            }
                        }
                    }
                }
            else
                {
                return nullptr;
                }
            }
        // Really an RTF file? Happens with ancient files.
        else if (std::strncmp(buffer.data(), RTF_SIGNATURE.c_str(), RTF_SIGNATURE.length()) == 0)
            {
            log_message(L"DOC file appears to be RTF. Parsing file as RTF.");
            rtf_extract_text filter_rtf;
            const wchar_t* const rtfText = filter_rtf(doc_buffer, text_length);
            add_characters({ rtfText, filter_rtf.get_filtered_text_length() });
            return get_filtered_text();
            }
        // ... or HTML? It happens.
        else
            {
            size_t bomStartLength{ 0 };
            if (text_length > 3 && static_cast<uint8_t>(doc_buffer[0]) == UTF8_SIGNATURE[0] &&
                static_cast<uint8_t>(doc_buffer[1]) == UTF8_SIGNATURE[1] &&
                static_cast<uint8_t>(doc_buffer[2]) == UTF8_SIGNATURE[2])
                {
                bomStartLength = 3;
                }
            size_t firstChar =
                std::string_view{ doc_buffer + bomStartLength, text_length - bomStartLength }
                    .find_first_not_of(" \t\n\r");
            if (firstChar == std::string_view::npos)
                {
                return nullptr;
                }
            firstChar += bomStartLength;

            // See if maybe it's HTML (based on if first character is a '<').
            // We are grasping at straws here, but people actually have files like this.
            if (firstChar < text_length && doc_buffer[firstChar] == '<')
                {
                log_message(L"DOC file appears to be HTML. Parsing file as HTML.");
                html_extract_text filter_html;
                // convert from UTF-8
                if (utf8::is_valid(doc_buffer, doc_buffer + text_length))
                    {
                    auto u16Val = utf8::utf8to16(std::string(doc_buffer, doc_buffer + text_length));
                    std::wstring convertedBuffer;
                    for (auto ch : u16Val)
                        {
                        convertedBuffer += static_cast<wchar_t>(ch);
                        }
                    const wchar_t* const htmText =
                        filter_html(convertedBuffer.c_str(), convertedBuffer.length(), true, false);
                    add_characters({ htmText, filter_html.get_filtered_text_length() });
                    }
                else
                    {
                    const size_t cvtBufferSize = text_length * 2;
                    const auto convertedBuffer = std::make_unique<wchar_t[]>(cvtBufferSize + 1);
#ifdef _MSC_VER
                    size_t cvtSize{ 0 };
                    mbstowcs_s(&cvtSize, convertedBuffer.get(),
                               static_cast<size_t>(cvtBufferSize) + 1, doc_buffer, cvtBufferSize);
#else
                    const size_t cvtSize =
                        std::mbstowcs(convertedBuffer.get(), doc_buffer, cvtBufferSize);
#endif
                    const wchar_t* const htmText =
                        filter_html(convertedBuffer.get(), cvtSize, true, false);
                    add_characters({ htmText, filter_html.get_filtered_text_length() });
                    }
                return get_filtered_text();
                }
            else
                {
                log_message(L"DOC parser: file header not found.");
                throw msword_header_not_found();
                }
            }

        return get_filtered_text();
        }

    //----------------------------------------------------
    void word1997_extract_text::load_stream(file_system_entry* cfbObj)
        {
        assert(cfbObj);
        if (!cfbObj)
            {
            return;
            }

        parse_state currentState;

        size_t offset{ 0 };
        // cppcheck-suppress unreadVariable
        wchar_t currentChar{ 0 };
        std::wstring paragraphBuffer;
        std::vector<unsigned char> currentSectorBuffer(SECTOR_SIZE, 0);

        // process the stream
        while (!cfbObj->eof() && offset < m_text_body_stream_length)
            {
            paragraphBuffer.clear();
            currentState.m_non_printable_char_detected = false;
            // cppcheck-suppress unreadVariable
            currentState.m_force_output_write = false;
            currentState.m_consecutive_table_tabs_detected = false;
            currentState.m_at_start_of_new_block = false;

            while (!cfbObj->eof() && offset < m_text_body_stream_length &&
                   !paragraph_ends_with_crlf(paragraphBuffer))
                {
                auto currSectorPos = safe_modulus(offset, SECTOR_SIZE);
                currentState.m_at_start_of_new_block = (currSectorPos == 0);

                // read in the next sector
                if (currentState.m_at_start_of_new_block)
                    {
                    size_t lastRead{ 0 };
                    std::memset(currentSectorBuffer.data(), 0, currentSectorBuffer.size());
                    // read it
                    for (;;)
                        {
                        lastRead = read_stream(currentSectorBuffer.data(), SECTOR_SIZE, cfbObj);
                        lastRead = std::clamp<size_t>(lastRead, 0, SECTOR_SIZE);
                        std::memset(currentSectorBuffer.data() + lastRead, 0,
                                    SECTOR_SIZE - lastRead);
                        // skip binary BAT blocks--more than likely are images or something
                        // (make sure we have a full 256-byte block to skip)
                        if (offset + SECTOR_SIZE < m_text_body_stream_length &&
                            is_buffer_binary_stream(currentSectorBuffer.data(),
                                                    std::clamp<size_t>(lastRead, 0, SECTOR_SIZE)))
                            {
                            log_message(L"DOC parser: binary stream intermixed with text body; "
                                        "file may be corrupt.");
                            offset += SECTOR_SIZE;
                            }
                        else
                            {
                            break;
                            }
                        }

                    if ((offset + lastRead) > m_text_body_stream_length)
                        {
                        lastRead = m_text_body_stream_length - offset;
                        }
                    assert(lastRead <= SECTOR_SIZE);
                    lastRead = std::clamp<size_t>(lastRead, 0, SECTOR_SIZE);

                    // Sector may be UTF-16 or 8-bit extended ASCII, try to figure out which it is.
                    // Note that files can have a mixture of the two, with no indicator
                    // (at least none that I can find) of how a particular sector is encoded.
                    // All we can do is a heuristic check here.
                    m_read_type =
                        string_util::is_extended_ascii(currentSectorBuffer.data(), lastRead) ?
                            charset_type::mbcs :
                            charset_type::utf16;
                    currSectorPos = safe_modulus(offset, SECTOR_SIZE);
                    }
                // reset
                currentState.m_non_printable_char_detected = false;
                currentState.m_force_output_write = false;

                if (m_read_type == charset_type::utf16)
                    {
                    currentChar = static_cast<decltype(currentChar)>(
                        read_short(currentSectorBuffer.data(), static_cast<int>(currSectorPos)));
                    offset += 2;
                    }
                else
                    {
                    currentChar = currentSectorBuffer[currSectorPos];
                    ++offset;
                    }

                // If null, then see if this block simply has its
                // end zeroed out--happens if you have an embedded image.
                // Scan past any more zeros until we reach either the end of the
                // block (in which case we would write out our current paragraph buffer)
                // or a non-zero character.
                if (currentChar == 0)
                    {
                    // while NOT at the start of a section
                    while (safe_modulus(offset, SECTOR_SIZE) != 0)
                        {
                        if (m_read_type == charset_type::utf16)
                            {
                            currentChar = static_cast<decltype(currentChar)>(read_short(
                                currentSectorBuffer.data(), static_cast<int>(currSectorPos)));
                            offset += 2;
                            }
                        else
                            {
                            currentChar = currentSectorBuffer[currSectorPos];
                            ++offset;
                            }
                        if (currentChar != 0)
                            {
                            break;
                            }
                        }
                    // see if we are at the end of the BAT block
                    currentState.m_at_start_of_new_block = (safe_modulus(offset, SECTOR_SIZE) == 0);
                    }

                // first, see if we are in a table
                if (currentState.m_is_in_table)
                    {
                    // if there are two consecutive 0x007, then this is the end of the table row
                    if (currentChar == 0x007)
                        {
                        paragraphBuffer += L'\t';
                        currentState.m_consecutive_table_tabs_detected = true;
                        continue;
                        }
                    else
                        {
                        if (currentState.m_consecutive_table_tabs_detected)
                            {
                            paragraphBuffer[paragraphBuffer.length() - 1] = L'\n';
                            }
                        currentState.m_consecutive_table_tabs_detected = false;
                        currentState.m_is_in_table = false;
                        }
                    }
                // special handling for control and Latin-1 surrogate characters
                if (currentChar < 0x20 || (currentChar >= 0x80 && currentChar <= 0x9F))
                    {
                    switch (currentChar)
                        {
                    // first character in front of HYPERLINK or PAGEREF tags.
                    case 0x13:
                        // throw out PAGE # information
                        if (paragraph_begins_with(paragraphBuffer, L"PAGE"))
                            {
                            paragraphBuffer.clear();
                            }
                        currentState.m_hyperlink_begin_char_detected = true;
                        currentState.m_force_output_write = true;
                        break;
                    // characters after the HYPERLINK tag
                    case 0x01:
                        /* this character proceeds 0x14 sometimes
                           in pageref & hyperlinks*/
                        if (currentState.m_hyperlink_begin_char_detected)
                            {
                            continue;
                            }
                        else
                            {
                            currentState.m_non_printable_char_detected = false;
                            currentState.m_force_output_write = true;
                            }
                        break;
                    case 0x14:
                        if (currentState.m_hyperlink_begin_char_detected &&
                            (paragraph_begins_with(paragraphBuffer, L"HYPERLINK") ||
                             paragraph_begins_with(paragraphBuffer, L"SEQ Table") ||
                             paragraph_begins_with(paragraphBuffer, L"REF") ||
                             paragraph_begins_with(paragraphBuffer, L"TOC") ||
                             paragraph_begins_with(paragraphBuffer, L"EMBED") ||
                             paragraph_begins_with(paragraphBuffer, L"PAGEREF") ||
                             // German for "PAGEREF".
                             // German version of some program (probably OpenOffice.org)
                             // translated this by accident at some point :)
                             paragraph_begins_with(paragraphBuffer, L"SEITENREF")))
                            {
                            currentState.m_hyperlink_is_valid = true;
                            paragraphBuffer.clear();
                            }
                        else if (paragraph_begins_with(paragraphBuffer, L"PAGE"))
                            {
                            currentState.m_hyperlink_is_valid = false;
                            paragraphBuffer.clear();
                            }
                        currentState.m_non_printable_char_detected = true;
                        break;
                    // for hyperlinks (after the hyperlinked text)
                    case 0x15:
                        if (currentState.m_hyperlink_is_valid)
                            {
                            // write out the text
                            currentState.m_force_output_write = true;
                            }
                        // throw out PAGE # information
                        else if (paragraph_begins_with(paragraphBuffer, L"PAGE"))
                            {
                            currentState.m_non_printable_char_detected = true;
                            paragraphBuffer.clear();
                            }
                        /* this has nothing to do with a hyperlink,
                           it's just regular text*/
                        else if (!currentState.m_hyperlink_begin_char_detected)
                            {
                            // write out the text
                            currentState.m_force_output_write = true;
                            }
                        else
                            {
                            // it's bogus trash; ignore it
                            currentState.m_non_printable_char_detected = true;
                            }
                        currentState.m_hyperlink_begin_char_detected = false;
                        currentState.m_hyperlink_is_valid = false;
                        break;
                    // for reviewed section
                    case 0x05:
                        currentState.m_hyperlink_begin_char_detected = false;
                        break;
                    // table cell
                    case 0x07:
                        currentState.m_hyperlink_begin_char_detected = false;
                        currentState.m_is_in_table = true;
                        paragraphBuffer += L'\t';
                        break;
                    case 0x0D:
                        // regular line feed
                        currentState.m_hyperlink_begin_char_detected = false;
                        paragraphBuffer += 0x000A;
                        break;
                    case 0x0B:
                        // hard return (Shift+Enter)
                        currentState.m_hyperlink_begin_char_detected = false;
                        paragraphBuffer += 0x000A;
                        break;
                    case 0x0C:
                        currentState.m_hyperlink_begin_char_detected = false;
                        paragraphBuffer += L'\n';
                        currentState.m_force_output_write = true;
                        break;
                    case 0x1E:
                        currentState.m_hyperlink_begin_char_detected = false;
                        paragraphBuffer += L'-';
                        break;
                    case 0x02:
                        currentState.m_hyperlink_begin_char_detected = false;
                        break;
                    case 0x1F:
                        currentState.m_hyperlink_begin_char_detected = false;
                        // Soft hyphen, we just strip these out
                        break;
                    case 0x09:
                        currentState.m_hyperlink_begin_char_detected = false;
                        paragraphBuffer += currentChar;
                        break;
                    case 0x08:
                        currentState.m_hyperlink_begin_char_detected = false;
                        break;
                    // MS 1252 surrogates need to be converted to unicode values
                    case 0x80:
                        paragraphBuffer += 0x20AC;
                        break;
                    case 0x82:
                        paragraphBuffer += 0x201A;
                        break;
                    case 0x83:
                        paragraphBuffer += 0x0192;
                        break;
                    case 0x84:
                        paragraphBuffer += 0x00A4;
                        break;
                    case 0x85:
                        paragraphBuffer += 0x2026;
                        break;
                    case 0x86:
                        paragraphBuffer += 0x2020;
                        break;
                    case 0x87:
                        paragraphBuffer += 0x2021;
                        break;
                    case 0x88:
                        paragraphBuffer += 0x02C6;
                        break;
                    case 0x89:
                        paragraphBuffer += 0x2030;
                        break;
                    case 0x8A:
                        paragraphBuffer += 0x0160;
                        break;
                    case 0x8B:
                        paragraphBuffer += 0x2039;
                        break;
                    case 0x8C:
                        paragraphBuffer += 0x0152;
                        break;
                    case 0x8E:
                        paragraphBuffer += 0x017D;
                        break;
                    case 0x91:
                        paragraphBuffer += 0x2018;
                        break;
                    case 0x92:
                        paragraphBuffer += 0x2019;
                        break;
                    case 0x93:
                        paragraphBuffer += 0x201C;
                        break;
                    case 0x94:
                        paragraphBuffer += 0x201D;
                        break;
                    case 0x95:
                        paragraphBuffer += 0x2022;
                        break;
                    case 0x96:
                        paragraphBuffer += 0x2013;
                        break;
                    case 0x97:
                        paragraphBuffer += 0x2014;
                        break;
                    case 0x98:
                        paragraphBuffer += 0x02DC;
                        break;
                    case 0x99:
                        paragraphBuffer += 0x2122;
                        break;
                    case 0x9A:
                        paragraphBuffer += 0x0161;
                        break;
                    case 0x9B:
                        paragraphBuffer += 0x203A;
                        break;
                    case 0x9C:
                        paragraphBuffer += 0x0153;
                        break;
                    case 0x9E:
                        paragraphBuffer += 0x017E;
                        break;
                    case 0x9F:
                        paragraphBuffer += 0x0178;
                        break;
                    default:
                        currentState.m_hyperlink_begin_char_detected = false;
                        // Any other control char - discard paragraph
                        currentState.m_non_printable_char_detected = true;
                        };
                    }
                // private-use unicode values used for ligatures
                else if (currentChar == 0xF001)
                    {
                    paragraphBuffer += L"fi";
                    }
                else if (currentChar == 0xF002)
                    {
                    paragraphBuffer += L"fl";
                    }
                // regular ligatures
                else if (currentChar >= 0xFB00 && currentChar <= 0xFB06)
                    {
                    switch (currentChar)
                        {
                    case 0xFB00:
                        paragraphBuffer += L"ff";
                        break;
                    case 0xFB01:
                        paragraphBuffer += L"fi";
                        break;
                    case 0xFB02:
                        paragraphBuffer += L"fl";
                        break;
                    case 0xFB03:
                        paragraphBuffer += L"ffi";
                        break;
                    case 0xFB04:
                        paragraphBuffer += L"ffl";
                        break;
                    case 0xFB05:
                        paragraphBuffer += L"ft";
                        break;
                    case 0xFB06:
                        paragraphBuffer += L"st";
                        break;
                    default:
                        break;
                        };
                    }
                else if (currentChar != 0xFEFF)
                    {
                    // skip zero width no-break space, output anything else to the next paragraph.
                    paragraphBuffer += currentChar;
                    }
                if (currentState.m_non_printable_char_detected || currentState.m_force_output_write)
                    {
                    break;
                    }
                }

            if (!currentState.m_non_printable_char_detected || currentState.m_at_start_of_new_block)
                {
                add_characters({ paragraphBuffer.c_str(), paragraphBuffer.length() });
                }
            }
        }

    //----------------------------------------------------
    void word1997_extract_text::load_summary_information(file_system_entry* cfbObj)
        {
        std::vector<char> propBuffer(4096, 0);
        // read the header
        auto readBytes = read_stream(propBuffer.data(), 28, cfbObj);
        const auto headerSignature = read_short(propBuffer.data(), 0);
        if (headerSignature != 0xFFFE && headerSignature != 0xFEFF)
            {
            log_message(L"DOC parser: SummaryInformation has an invalid signature. "
                        "Document properties will not be loaded.");
            return;
            }
        // skip the ClassID GUID, just read the section count
        auto sectionCount = static_cast<size_t>(read_int(propBuffer.data(), 24));
        if ((sectionCount * 20) + 28 /*header*/ > propBuffer.size())
            {
            log_message(L"DOC parser: unusual number of sections in SummaryInformation, "
                        "only first one will be read. File may be corrupt.");
            sectionCount = 1;
            }

        // read the section list
        std::vector<size_t> sectorStarts;
        for (size_t i = 0; i < sectionCount; ++i)
            {
            readBytes += read_stream(propBuffer.data(), 20, cfbObj);
            // skip GUID, just read offset
            if (read_int(propBuffer.data(), 16) < 0)
                {
                log_message(L"DOC parser: invalid property offset. File may be corrupt.");
                continue;
                }
            sectorStarts.push_back(static_cast<size_t>(read_int(propBuffer.data(), 16)));
            }

        for (auto sectionStart : sectorStarts)
            {
            // move to where we need to be
            if (readBytes < sectionStart)
                {
                readBytes += read_stream(propBuffer.data(), sectionStart - readBytes, cfbObj);
                }
            readBytes += read_stream(propBuffer.data(), 4, cfbObj);
            if (read_int(propBuffer.data(), 0) <= 0)
                {
                log_message(L"DOC parser: invalid property section size. File may be corrupt.");
                continue;
                }
            const size_t sectionSize = static_cast<size_t>(read_int(propBuffer.data(), 0));
            if (sectionSize > propBuffer.size())
                {
                propBuffer.resize(sectionSize);
                }
            // read in the rest of it (preserve the size DWORD that we just read)
            readBytes += read_stream(propBuffer.data() + 4, sectionSize - 4, cfbObj);
            if (read_int(propBuffer.data(), 4) < 0)
                {
                log_message(L"DOC parser: invalid property count. File may be corrupt.");
                continue;
                }
            const size_t propertyCount = static_cast<size_t>(read_int(propBuffer.data(), 4));
            std::vector<std::pair<int32_t, int32_t>> properties;
            size_t pos = 8;
            for (size_t i = 0; i < propertyCount; ++i, pos += 8)
                {
                if (pos + 8 > sectionSize)
                    {
                    log_message(L"DOC parser: error in property count. File may be corrupt.");
                    break;
                    }
                if (read_int(propBuffer.data(), pos + 4) < 0)
                    {
                    log_message(L"DOC parser: invalid property offset, property will be skipped. "
                                "File may be corrupt.");
                    continue;
                    }
                properties.emplace_back(read_int(propBuffer.data(), pos + 4), // Offset
                                        read_int(propBuffer.data(), pos));    // ID
                }
            // read the properties
            for (const auto& property : properties)
                {
                // should be able to at least read the data type (4 bytes)
                // and value (always multiple of 4)
                if (static_cast<size_t>(property.first) + 8 > sectionSize)
                    {
                    log_message(L"DOC parser: error in property offset. File may be corrupt.");
                    break;
                    }
                const auto dataType = read_int(propBuffer.data(), property.first);
                std::wstring propertyValue;
                // Multi-byte character string
                if (dataType == static_cast<int32_t>(property_data_type::vt_bstr) ||
                    dataType == static_cast<int32_t>(property_data_type::vt_lpstr))
                    {
                    const auto strByteCount =
                        read_int(propBuffer.data(), static_cast<size_t>(property.first) + 4);
                    if ((static_cast<size_t>(property.first) + 8 + strByteCount) > sectionSize)
                        {
                        log_message(
                            L"DOC parser: error in property MBCS value. File may be corrupt.");
                        break;
                        }
                    // convert from UTF-8
                    if (utf8::is_valid(propBuffer.data() + property.first + 8,
                                       propBuffer.data() + property.first + 8 + strByteCount))
                        {
                        auto u16Val = utf8::utf8to16(
                            std::string(propBuffer.data() + property.first + 8,
                                        propBuffer.data() + property.first + 8 + strByteCount));
                        for (auto ch : u16Val)
                            {
                            propertyValue += static_cast<wchar_t>(ch);
                            }
                        }
                    // or current code page
                    else
                        {
                        auto wideBuff =
                            std::make_unique<wchar_t[]>(static_cast<size_t>(strByteCount) + 1);
#ifdef _MSC_VER
                        size_t convertedAmt{ 0 };
                        mbstowcs_s(&convertedAmt, wideBuff.get(),
                                   static_cast<size_t>(strByteCount) + 1,
                                   propBuffer.data() + property.first + 8, strByteCount);
#else
                        std::mbstowcs(wideBuff.get(), propBuffer.data() + property.first + 8,
                                      strByteCount);
#endif
                        propertyValue = wideBuff.get();
                        }
                    }
                // UTF-16 string
                if (dataType == static_cast<int32_t>(property_data_type::vt_lpwstr))
                    {
                    const auto strByteCount =
                        read_int(propBuffer.data(), static_cast<size_t>(property.first) + 4);
                    if ((static_cast<size_t>(property.first) + 8 + strByteCount) > sectionSize)
                        {
                        log_message(
                            L"DOC parser: error in property WCS value. File may be corrupt.");
                        break;
                        }
                    for (auto i = 0; i < safe_divide(strByteCount, 2); ++i)
                        {
                        propertyValue += read_short(propBuffer.data() + property.first + 8, i * 2);
                        }
                    }
                // Set the value to the respective property.
                // Note that there may be embedded nulls in the string if the input was trash,
                // so copy it in as a wchar_t* buffer so that it stops on the null terminator.
                if (property.second == static_cast<int32_t>(property_format_id::pid_title))
                    {
                    m_title.assign(propertyValue.c_str());
                    }
                else if (property.second == static_cast<int32_t>(property_format_id::pid_subject))
                    {
                    m_subject.assign(propertyValue.c_str());
                    }
                else if (property.second == static_cast<int32_t>(property_format_id::pid_author))
                    {
                    m_author.assign(propertyValue.c_str());
                    }
                else if (property.second == static_cast<int32_t>(property_format_id::pid_keywords))
                    {
                    m_keywords.assign(propertyValue.c_str());
                    }
                else if (property.second == static_cast<int32_t>(property_format_id::pid_comments))
                    {
                    m_comments.assign(propertyValue.c_str());
                    }
                }
            }
        }

    //----------------------------------------------------
    void word1997_extract_text::load_document(file_system_entry* cfbObj)
        {
        if (!cfbObj)
            {
            log_message(L"DOC parser: file system entry missing.");
            throw msword_header_not_found();
            }
        // process document flags
        std::vector<char> headerBuffer(128, 0);
        const long offset =
            static_cast<long>(read_stream(headerBuffer.data(), headerBuffer.size(), cfbObj));
        const auto flags = read_short(headerBuffer.data(), 10);
        if (flags & fComplex)
            {
            log_message(L"DOC parser: fast-saved (complex) files are not supported.");
            throw msword_fastsaved();
            }
        if (flags & fEncrypted)
            {
            log_message(L"DOC parser: encrypted files are not supported.");
            throw msword_encrypted();
            }
        // Document is using Unicode
        if (flags & fExtChar || // MS docs says this "MUST always be 1,"
                                // so this check is probably only relevant with ancient files.
            flags & fFarEast)   // Saved from CJK version of Word? Probably Unicode.
            {
            m_read_type = charset_type::utf16;
            }
        // or MBCS
        if (m_read_type == charset_type::type_unknown)
            {
            m_read_type = charset_type::mbcs;
            }

        // jump to the end of the stream boundary
        size_t strStart = read_int(headerBuffer.data(), 24);
        m_text_body_stream_length = static_cast<decltype(m_text_body_stream_length)>(
            read_int(headerBuffer.data(), 28) - strStart);
        strStart -= offset; // step back into the header

        // move the iostream into position
        char readBuf[2]{ 0 };
        for (size_t i{ 0 }; i < strStart; ++i)
            {
            read_stream(readBuf, 1, cfbObj);
            if (cfbObj->eof())
                {
                log_message(
                    L"DOC parser: stream ends before the document's body; file is corrupt.");
                throw msword_corrupted();
                }
            }

        load_stream(cfbObj);
        }

    //----------------------------------------------------
    bool word1997_extract_text::is_buffer_binary_stream(const unsigned char* buffer,
                                                        const size_t size) noexcept
        {
        assert(buffer);
        // if nothing to look at, then let's not consider it binary
        if (buffer == nullptr || size == 0)
            {
            return false;
            }
        // Look for a Unicode null terminator (both bytes are zero)
        // and if there is more data after it, then it probably is a binary stream.
        // It wouldn't make sense to have an embedded terminator in the middle of a text block.
        size_t index = 0;
        while (index < size - 1)
            {
            if (buffer[index] == 0 && buffer[index + 1] == 0)
                {
                // see if there is any more data after the null inside the block
                index += 2;
                while (index < size - 1)
                    {
                    if (buffer[index] != 0)
                        {
                        return true;
                        }
                    ++index;
                    }
                break;
                }
            ++index;
            }
        return false;
        }

    //----------------------------------------------------
    bool word1997_extract_text::load_header(cfb_iostream* str)
        {
        if (!str)
            {
            return false;
            }

        str->seek(0, cfb_iostream::cfb_strem_seek_type::seek_end);
        m_file_length = str->tell();
        str->seek(0, cfb_iostream::cfb_strem_seek_type::seek_beg);
        // read in the first BAT (512 byte block) and verify that it's a valid CFB stream
        std::vector<char> cfbBuf(BAT_SECTOR_SIZE + 1, 0);
        if (str->read(cfbBuf.data(), BAT_SECTOR_SIZE) != BAT_SECTOR_SIZE)
            {
            return false;
            }
        if (std::memcmp(cfbBuf.data(), MAGIC_NUMBER.c_str(), 8) != 0 &&
            std::memcmp(cfbBuf.data(), MAGIC_NUMBER_BETA.c_str(), 8) != 0)
            {
            return false;
            }

        // should be 512 normally
        m_sector_size = static_cast<decltype(m_sector_size)>(1ull << read_short(cfbBuf.data(), 30));
        // should be 64 normally
        m_short_sector_size =
            static_cast<decltype(m_short_sector_size)>(1ull << read_short(cfbBuf.data(), 32));

        if (m_file_length == 0 || m_sector_size == 0)
            {
            return false;
            }
        m_sector_count = safe_divide(m_file_length, m_sector_size);

        // get BAT info
        m_bat_sector_count = read_uint(cfbBuf.data(), 44);
        // 64 is the number of 512 sectors that are needed to hold the
        // SBAT, but we need the number of small (64) sectors,
        // so multiply by 8 (64*8 = 512).
        m_sbat_sector_count = static_cast<size_t>(read_uint(cfbBuf.data(), 64)) * 8;

        if (m_bat_sector_count == 0)
            {
            log_message(L"DOC parser: no content in file?");
            return false;
            }
        // Do a check on the number of BAT sectors to make sure they don't go beyond
        // the size of the document
        if (m_bat_sector_count * m_sector_size > m_file_length)
            {
            log_message(L"DOC parser: unable to read Block Allocation Table entry.");
            throw cfb_bad_bat_entry();
            }

        // get XBAT info (if there are any [only medium and large files have these])
        const auto numOfXBATs = read_uint(cfbBuf.data(), 72);
        const auto XBATstart = read_uint(cfbBuf.data(), 68);
        if (numOfXBATs * m_sector_size > m_file_length)
            {
            log_message(L"DOC parser: unable to read eXtended Block Allocation Table entry.");
            throw cfb_bad_bat_entry();
            }
        m_BAT.resize(m_bat_sector_count * m_sector_size, 0);

        int32_t currSector{ static_cast<int32_t>(XBATstart) };
        size_t i{ 0 };
        std::vector<char> tmpBuffer(DIFAT_SIZE);
        std::memcpy(tmpBuffer.data(), cfbBuf.data() + 0x4C, DIFAT_SIZE);
        while ((currSector >= 0) && (i < numOfXBATs))
            {
            tmpBuffer.resize(m_sector_size * (i + 1) + DIFAT_SIZE);
            str->seek(
                static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ + currSector * m_sector_size),
                cfb_iostream::cfb_strem_seek_type::seek_beg);
            const auto readSCount =
                str->read(tmpBuffer.data() + DIFAT_SIZE + (m_sector_size - 4) * i, m_sector_size);
            if (readSCount != m_sector_size)
                {
                log_message(L"DOC parser: unable to read Block Allocation Table entry.");
                throw cfb_bad_bat_entry();
                }
            ++i;
            currSector = read_int(tmpBuffer.data(), DIFAT_SIZE + (m_sector_size - 4) * i);
            };

        // read in the initial 108 (regular) BATs (Block Allocation Tables)
        size_t currentBATSector{ 0 };
        for (i = 0; i < m_bat_sector_count && i < 109; ++i, ++currentBATSector)
            {
            auto batSector = read_int(tmpBuffer.data(), 4 * i);

            if (batSector < 0 || static_cast<size_t>(batSector) >= m_sector_count)
                {
                // bad BAT entry
                log_message(L"DOC parser: unable to read Block Allocation Table entry.");
                throw cfb_bad_bat_entry();
                }
            str->seek(
                static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ + batSector * m_sector_size),
                cfb_iostream::cfb_strem_seek_type::seek_beg);
            if (str->read(m_BAT.data() + currentBATSector * m_sector_size, m_sector_size) !=
                m_sector_size)
                {
                // can't read BAT
                log_message(L"DOC parser: unable to read Block Allocation Table entry.");
                throw cfb_bad_bat();
                }
            }

        // read the XBATs (eXtended Block Allocation Tables), which are present in larger files
        if (numOfXBATs > 0)
            {
            // read in the first XBAT
            char cfbBuffer2[BAT_SECTOR_SIZE]{ 0 };
            str->seek(
                static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ + XBATstart * m_sector_size),
                cfb_iostream::cfb_strem_seek_type::seek_beg);
            if (str->read(cfbBuffer2, BAT_SECTOR_SIZE) != BAT_SECTOR_SIZE)
                {
                // Bad XBAT entry
                log_message(L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                throw cfb_bad_xbat_entry();
                }
            for (size_t k{ 0 }; k < 128 && (k + 109) < m_bat_sector_count; ++k, ++currentBATSector)
                {
                auto batSector = read_int(cfbBuffer2, 4 * k);

                if (batSector < 0 || static_cast<size_t>(batSector) >= m_sector_count)
                    {
                    // Bad XBAT entry
                    log_message(
                        L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                    throw cfb_bad_xbat_entry();
                    }
                str->seek(static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ +
                                            batSector * m_sector_size),
                          cfb_iostream::cfb_strem_seek_type::seek_beg);
                if (str->read(m_BAT.data() + currentBATSector * m_sector_size, m_sector_size) !=
                    m_sector_size)
                    {
                    // Can't read XBAT
                    log_message(
                        L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                    throw cfb_bad_xbat();
                    }
                }

            // read in the rest of the XBATS
            --currentBATSector; /*step back to 236 (108+128)*/
            for (i = 1; i < numOfXBATs; ++i)
                {
                // last value in the XBAT tells where the next one is
                auto batSector = read_int(cfbBuffer2, 127 * 4);
                if (batSector < 0 || static_cast<size_t>(batSector) >= m_sector_count)
                    {
                    break;
                    }
                // read in the next (X)BAT, which is always 512
                str->seek(static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ +
                                            batSector * m_sector_size),
                          cfb_iostream::cfb_strem_seek_type::seek_beg);
                if (str->read(cfbBuffer2, BAT_SECTOR_SIZE) != BAT_SECTOR_SIZE)
                    {
                    // Bad XBAT entry
                    log_message(
                        L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                    throw cfb_bad_xbat_entry();
                    }
                // read in the next XBAT (stop when we have read in the number of BATs)
                for (size_t k = 0; k < 128 && currentBATSector < m_bat_sector_count;
                     ++k, ++currentBATSector)
                    {
                    batSector = read_int(cfbBuffer2, 4 * k);

                    if (batSector < 0 || static_cast<size_t>(batSector) >= m_sector_count)
                        {
                        // Bad XBAT entry
                        log_message(
                            L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                        throw cfb_bad_xbat_entry();
                        }
                    str->seek(static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ +
                                                batSector * m_sector_size),
                              cfb_iostream::cfb_strem_seek_type::seek_beg);
                    if (str->read(m_BAT.data() + currentBATSector * m_sector_size, m_sector_size) !=
                        m_sector_size)
                        {
                        // can't read XBAT
                        log_message(
                            L"DOC parser: unable to read eXtended Block Allocation Table entry.");
                        throw cfb_bad_xbat();
                        }
                    }
                }
            }

        // Read SBAT (Small Block Allocation Table)
        long sbatCurrent = read_int(cfbBuf.data(), 60);
        // Make sure current BAT block is within range of possible 512-byte sectors in the file.
        // The sectors are zero-indexed, but can't start at zero because that's the file header.
        if (sbatCurrent > 0 && static_cast<size_t>(sbatCurrent) < m_sector_count)
            {
            size_t sbatBigSectorsRead{ 0 };
            // 8 SBATs for each 512 sector, so get enough space for that
            m_SBAT.resize(m_sector_size * (safe_divide<size_t>(m_sbat_sector_count, 8)));
            for (;;)
                {
                // don't read too far
                if ((sbatBigSectorsRead * m_sector_size) + m_sector_size > m_SBAT.size())
                    {
                    log_message(L"DOC parser: Small Block Allocation Table corrupted, "
                                "some data may be lost.");
                    break;
                    }
                str->seek(static_cast<long>(BAT_SECTOR_SIZE /*skip first BAT*/ +
                                            sbatCurrent * m_sector_size),
                          cfb_iostream::cfb_strem_seek_type::seek_beg);
                str->read(m_SBAT.data() + sbatBigSectorsRead * m_sector_size, m_sector_size);
                ++sbatBigSectorsRead;
                if ((static_cast<size_t>(sbatCurrent) * 4) > (m_bat_sector_count * m_sector_size))
                    {
                    log_message(L"DOC parser: Small Block Allocation Table corrupted.");
                    return false;
                    }
                // walk the BAT to the next sector
                sbatCurrent = read_int(m_BAT.data(), static_cast<size_t>(sbatCurrent) * 4);
                if (sbatCurrent < 0 || static_cast<size_t>(sbatCurrent) >= m_sector_count)
                    {
                    break;
                    }
                }
            if ((sbatBigSectorsRead * m_sector_size) == 0 || m_short_sector_size == 0)
                {
                log_message(L"DOC parser: Small Block Allocation Table corrupted.");
                return false;
                }
            }
        else
            {
            log_message(L"DOC parser: Small Block Allocation Table entry out of range. "
                        "File may be corrupted.");
            m_SBAT.clear();
            }

        /* Raw block index of where the entries start.
           It may go to the end of the stream, but not always.*/
        const auto entriesStart = read_uint(cfbBuf.data(), 48);
        const size_t remainingBlocks = m_sector_count - entriesStart;
        m_file_system_entry_count = remainingBlocks * 4;
        // set the entries pointer to the specified block index
        m_file_system_entries =
            str->get_start() + ((static_cast<size_t>(entriesStart) + 1) * m_sector_size);
        m_current_file_system_entry = m_file_system_entries;

        // move to the root storage
        for (auto cfbObj = read_next_file_system_entry(str); cfbObj != nullptr;
             cfbObj = read_next_file_system_entry(str))
            {
            if (cfbObj->is_root_entry())
                {
                m_root_storage = cfbObj;
                break;
                }
            }
        if (!m_root_storage)
            {
            log_message(L"DOC parser: document entry point not found.");
            throw msword_root_enrty_not_found();
            }
        m_current_file_system_entry = m_file_system_entries;
        str->seek(0, cfb_iostream::cfb_strem_seek_type::seek_beg);
        return true;
        }

    //----------------------------------------------------
    std::shared_ptr<word1997_extract_text::file_system_entry>
    word1997_extract_text::read_next_file_system_entry(const cfb_iostream* str)
        {
        if (m_file_system_entries == nullptr ||
            m_current_file_system_entry >=
                (m_file_system_entries + m_file_system_entry_count * ENTRY_SECTOR_SIZE) ||
            m_current_file_system_entry + ENTRY_SECTOR_SIZE > m_file_end_sentinel || str == nullptr)
            {
            if (m_current_file_system_entry + ENTRY_SECTOR_SIZE > m_file_end_sentinel)
                {
                log_message(
                    L"DOC parser: file-system entry beyond file length. File may be corrupted.");
                }
            return nullptr;
            }

        auto cfbObj = std::make_shared<file_system_entry>(*str);
        cfbObj->m_strorage_offset = m_current_file_system_entry;
        cfbObj->m_type = static_cast<file_system_entry_type>(m_current_file_system_entry[66]);
        cfbObj->m_color = static_cast<file_system_entry_color>(m_current_file_system_entry[67]);
        cfbObj->m_previous_property = read_uint(m_current_file_system_entry, 68);
        cfbObj->m_next_property = read_uint(m_current_file_system_entry, 72);
        cfbObj->m_size = read_uint(m_current_file_system_entry, 120);

        cfbObj->m_sectors.clear();
        cfbObj->m_sectors.reserve(256);
        cfbObj->m_name.clear();
        cfbObj->m_name.reserve(32);

        // read the storage area's name
        const auto nameLength = safe_divide<size_t>(read_short(m_current_file_system_entry, 64), 2);
        // length is 32 (64 bits, drop the high bit)
        if (nameLength > 32)
            {
            log_message(L"DOC parser: corrupt name in property detected. Skipping property.");
            return nullptr;
            }
        for (size_t i{ 0 }; i < nameLength; ++i)
            {
            // skip leading zero in Unicode characters
            const char currentChar = m_current_file_system_entry[i * 2];
            if (currentChar == 0)
                {
                break;
                }
            cfbObj->m_name.append(1, currentChar);
            }

        // read sector list
        auto currentSector = read_uint(m_current_file_system_entry, 116);
        const auto sectorCount = safe_divide(
            m_file_length, (cfbObj->is_in_small_blocks() ? m_short_sector_size : m_sector_size));
        while (currentSector <= sectorCount)
            {
            // break if sector count or sector list size are beyond the boundaries of the file
            if (currentSector >= safe_divide<size_t>(cfbObj->is_in_small_blocks() ?
                                                         m_sbat_sector_count * m_short_sector_size :
                                                         m_bat_sector_count * m_sector_size,
                                                     4) ||
                (cfbObj->m_sectors.size() >
                 safe_divide(cfbObj->m_size,
                             (cfbObj->is_in_small_blocks() ? m_short_sector_size : m_sector_size))))
                {
                break;
                }

            cfbObj->m_sectors.push_back(currentSector);

            // read next sector value from associated buffer
            const auto nextSector =
                (!cfbObj->is_in_small_blocks() &&
                 m_BAT.size() > (static_cast<size_t>(currentSector) * 4) + 4) ?
                    read_int(m_BAT.data(), static_cast<size_t>(currentSector) * 4) :
                (m_SBAT.size() > (static_cast<size_t>(currentSector) * 4) + 4) ?
                    read_int(m_SBAT.data(), static_cast<size_t>(currentSector) * 4) :
                    -1;
            if (nextSector < 0)
                {
                break;
                }
            currentSector = nextSector;
            }

        // fix size of file object if larger than sum of its sector sizes
        cfbObj->m_size = std::clamp<size_t>(
            cfbObj->m_size, 0,
            (cfbObj->is_in_small_blocks() ? m_short_sector_size : m_sector_size) *
                cfbObj->m_sectors.size());

        // move to next property
        m_current_file_system_entry += ENTRY_SECTOR_SIZE;

        return cfbObj;
        }

    //----------------------------------------------------
    size_t word1997_extract_text::read_stream(void* buffer, size_t bufferSize,
                                              file_system_entry* cfbObj)
        {
        if (buffer == nullptr || cfbObj == nullptr || bufferSize == 0)
            {
            return 0;
            }

        // returns the offset of a CFB object based on the sector
        const auto get_offset = [this](const file_system_entry* cfbObjEntry,
                                       const size_t sectorIndex) noexcept -> size_t
        {
            if (cfbObjEntry == nullptr)
                {
                return 0;
                }
            if (!cfbObjEntry->is_in_small_blocks())
                {
                return BAT_SECTOR_SIZE +
                       // step over initial sector
                       (cfbObjEntry->m_sectors[sectorIndex] * m_sector_size);
                }
            else
                {
                const size_t sbatSectorNumber = safe_divide<size_t>(
                    cfbObjEntry->m_sectors[sectorIndex], get_sbats_per_sector());
                return BAT_SECTOR_SIZE +
                       (m_root_storage->m_sectors[sbatSectorNumber] * m_sector_size) +
                       (safe_modulus<size_t>(cfbObjEntry->m_sectors[sectorIndex],
                                             get_sbats_per_sector()) *
                        m_short_sector_size);
                }
        };

        // if buffer size goes beyond the end of the stream, then "shrink" the buffer size
        if (cfbObj->m_internal_offset + bufferSize > cfbObj->m_size)
            {
            if (cfbObj->m_size <= cfbObj->m_internal_offset)
                {
                return 0;
                }
            bufferSize = cfbObj->m_size - cfbObj->m_internal_offset;
            }

        // see where we are and ensure the object's sector is where it should be
        const size_t sectorSize =
            (cfbObj->is_in_small_blocks() ? m_short_sector_size : m_sector_size);
        size_t sectorCount = safe_divide(cfbObj->m_internal_offset, sectorSize);
        if (sectorCount >= cfbObj->m_sectors.size())
            {
            return 0;
            }

        // if the offset doesn't align evenly with the sector sizes,
        // then factor that in when adjusting the offset to the parent parser
        const size_t extraBytesOffset = safe_modulus(cfbObj->m_internal_offset, sectorSize);
        // get into position (adjusting if necessary)
        if (const auto offset = get_offset(cfbObj, sectorCount) + extraBytesOffset;
            cfbObj->m_stream_offset != offset)
            {
            cfbObj->m_stream_offset = offset;
            cfbObj->seek(static_cast<long>(cfbObj->m_stream_offset),
                         cfb_iostream::cfb_strem_seek_type::seek_beg);
            }

        const size_t remainingBytesInSector = sectorSize - extraBytesOffset;
        // the main call to read
        size_t readSize =
            cfbObj->read(buffer, std::min<size_t>(bufferSize, remainingBytesInSector));
        cfbObj->m_stream_offset += readSize;

        // if partial read was done because offset was at a weird place, then see if
        // there is more space left in the buffer to read in more sectors
        const size_t sectorsToRead =
            (remainingBytesInSector < bufferSize) ?
                safe_divide((bufferSize - remainingBytesInSector), sectorSize) :
                0;
        for (size_t i{ 0 }; i < sectorsToRead; ++i)
            {
            ++sectorCount;
            if (const size_t offset = get_offset(cfbObj, sectorCount);
                offset != cfbObj->m_stream_offset)
                {
                cfbObj->m_stream_offset = offset;
                cfbObj->seek(static_cast<long>(cfbObj->m_stream_offset),
                             cfb_iostream::cfb_strem_seek_type::seek_beg);
                }
            const auto readbytes =
                cfbObj->read(static_cast<char*>(buffer) + readSize,
                             std::min<size_t>(bufferSize - readSize, sectorSize));
            readSize += readbytes;
            cfbObj->m_stream_offset += readbytes;
            }

        /* Also, if a weird offset caused a partial read and there is more space in the buffer
           (even if more sectors had been read), then read any remaining bytes into the buffer.
           (This would be a partial read of the next sector.)*/
        if (const size_t bytesToRead =
                (remainingBytesInSector < bufferSize) ?
                    safe_modulus((bufferSize - remainingBytesInSector), sectorSize) :
                    0;
            bytesToRead > 0)
            {
            ++sectorCount;
            cfbObj->m_stream_offset = get_offset(cfbObj, sectorCount);
            cfbObj->seek(static_cast<long>(cfbObj->m_stream_offset),
                         cfb_iostream::cfb_strem_seek_type::seek_beg);
            const auto readbytes = cfbObj->read(static_cast<char*>(buffer) + readSize, bytesToRead);
            readSize += readbytes;
            cfbObj->m_stream_offset += readbytes;
            }

        cfbObj->m_internal_offset += readSize;
        return readSize;
        }
    } // namespace lily_of_the_valley
