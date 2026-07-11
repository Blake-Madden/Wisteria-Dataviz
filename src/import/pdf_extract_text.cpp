///////////////////////////////////////////////////////////////////////////////
// Name:        pdf_extract_text.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdf_extract_text.h"
#include "pdf_content_parser.h"
#include "pdf_document.h"
#include "text_matrix.h"
#include <cstdint>
#include <memory>
#include <set>
#include <tuple>
#include <vector>

namespace lily_of_the_valley
    {
    //------------------------------------------------------------------
    void pdf_extract_text::load_cid_to_unicode_table(const std::string_view registryOrdering,
                                                     const std::wstring_view text)
        {
        if (registryOrdering.empty())
            {
            return;
            }
        cid_to_unicode_table table;
        if (!text.empty())
            {
            // the CMap resource file is plain ASCII, so narrowing each character
            // to a char (rather than relying on an implicit conversion) is lossless
            std::string cmapText;
            cmapText.reserve(text.length());
            for (const wchar_t character : text)
                {
                cmapText += static_cast<char>(character);
                }
            pdf_text_decoder::parse_cid_to_unicode_cmap(cmapText, table);
            }
        m_cid_to_unicode_tables[std::string{ registryOrdering }] = std::move(table);
        }

    //------------------------------------------------------------------
    void pdf_extract_text::load_glyph_name_table(const std::wstring_view text)
        {
        m_glyph_name_table.clear();
        if (text.empty())
            {
            return;
            }
        // strip comment ('#'...) and blank lines; text_matrix::read() needs to know
        // the row count ahead of time, so count what remains as we go
        std::wstring filtered;
        filtered.reserve(text.length());
        size_t lineCount{ 0 };
        size_t pos{ 0 };
        while (pos < text.length())
            {
            const size_t lineEnd{ text.find(L'\n', pos) };
            const bool haveMore{ lineEnd != std::wstring_view::npos };
            std::wstring_view line{ text.substr(pos, haveMore ? (lineEnd - pos) :
                                                                std::wstring_view::npos) };
            // trim a trailing '\r' (CRLF line endings)
            if (!line.empty() && line.back() == L'\r')
                {
                line.remove_suffix(1);
                }
            if (!line.empty() && line.front() != L'#')
                {
                filtered.append(line);
                filtered += L'\n';
                ++lineCount;
                }
            pos = haveMore ? (lineEnd + 1) : text.length();
            }
        if (lineCount == 0)
            {
            return;
            }

        // each line is "glyphname;XXXX[ XXXX...]"; a single ';'-delimited column
        // definition, repeated, fills both fields of the row
        std::vector<std::vector<std::wstring>> matrix;
        text_matrix<std::wstring> parser{ &matrix };
        text_row<std::wstring> row;
        text_column<text_column_delimited_character_parser> delimitedColumn{
            text_column_delimited_character_parser{ L';' }
        };
        row.add_column(delimitedColumn);
        parser.add_row_definition(row);
        const size_t rowsRead{ parser.read(filtered.c_str(), lineCount, 2) };

        for (size_t i = 0; i < rowsRead; ++i)
            {
            if (matrix[i].size() < 2 || matrix[i][0].empty() || matrix[i][1].empty())
                {
                continue;
                }
            const std::wstring_view codepoints{ matrix[i][1] };
            std::wstring unicodeValue;
            size_t codePos{ 0 };
            while (codePos < codepoints.length())
                {
                while (codePos < codepoints.length() && codepoints[codePos] == L' ')
                    {
                    ++codePos;
                    }
                const size_t groupStart{ codePos };
                while (codePos < codepoints.length() && codepoints[codePos] != L' ')
                    {
                    ++codePos;
                    }
                if (codePos == groupStart)
                    {
                    continue;
                    }
                const std::wstring_view hexGroup{ codepoints.substr(groupStart,
                                                                    codePos - groupStart) };
                // AGL-format Unicode values are always exactly four hex digits
                if (hexGroup.length() != 4)
                    {
                    continue;
                    }
                uint32_t value{ 0 };
                bool validGroup{ true };
                for (const wchar_t hexChar : hexGroup)
                    {
                    const int digitValue{ pdf_lexer::hex_digit_value(static_cast<char>(hexChar)) };
                    if (digitValue < 0)
                        {
                        validGroup = false;
                        break;
                        }
                    value = (value << 4) | static_cast<uint32_t>(digitValue);
                    }
                if (validGroup)
                    {
                    unicodeValue += static_cast<wchar_t>(value);
                    }
                }
            if (!unicodeValue.empty())
                {
                m_glyph_name_table[matrix[i][0]] = unicodeValue;
                }
            }
        }

    //------------------------------------------------------------------
    std::wstring pdf_extract_text::read_string_value(const pdf_document& document,
                                                     const pdf_object& owningObject,
                                                     std::string_view value)
        {
        value = pdf_lexer::trim(document.resolve_value(value));
        size_t pos{ 0 };
        if (!value.empty() && value.front() == '(')
            {
            return pdf_text_decoder::decode_metadata_string(
                document.decrypt(owningObject, pdf_lexer::read_literal_string(value, pos)));
            }
        if (!value.empty() && value.front() == '<')
            {
            return pdf_text_decoder::decode_metadata_string(
                document.decrypt(owningObject, pdf_lexer::read_hex_string(value, pos)));
            }
        return {};
        }

    //------------------------------------------------------------------
    std::string pdf_extract_text::read_raw_string(std::string_view value)
        {
        value = pdf_lexer::trim(value);
        size_t pos{ 0 };
        if (!value.empty() && value.front() == '(')
            {
            return pdf_lexer::read_literal_string(value, pos);
            }
        if (!value.empty() && value.front() == '<')
            {
            return pdf_lexer::read_hex_string(value, pos);
            }
        return {};
        }

    //------------------------------------------------------------------
    std::unique_ptr<pdf_decryptor> pdf_extract_text::setup_decryption(
        const pdf_document& document, const std::string_view encryptValue,
        const std::string_view idValue, const aes_cbc_functor& aesFunction,
        const sha2_functor& hashFunction)
        {
        const std::string_view encryptDict{ pdf_lexer::trim(document.resolve_value(encryptValue)) };
        if (encryptDict.empty() || encryptDict.compare(0, 2, "<<") != 0)
            {
            return nullptr;
            }

        const std::string_view filter{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(encryptDict, "Filter")) };
        if (!filter.empty() && filter != "/Standard")
            {
            return nullptr;
            }

        long revision{ 0 };
        if (!pdf_lexer::to_int(pdf_lexer::trim(pdf_lexer::find_dictionary_value(encryptDict, "R")),
                               revision))
            {
            return nullptr;
            }

        long version{ 0 };
        std::ignore = pdf_lexer::to_int(
            pdf_lexer::trim(pdf_lexer::find_dictionary_value(encryptDict, "V")), version);

        // RC4 (CFM /V2) and AES (CFM /AESV2 or /AESV3) are supported. V4 resolves
        // its crypt filter from /CF/StdCF/CFM; V5 always uses AES-256 (/AESV3).
        std::string cryptFilterMethod;
        if (version == 4)
            {
            const std::string_view cfDict{ pdf_lexer::trim(
                pdf_lexer::find_dictionary_value(encryptDict, "CF")) };
            const std::string_view stdCf{ pdf_lexer::trim(
                pdf_lexer::find_dictionary_value(cfDict, "StdCF")) };
            cryptFilterMethod = pdf_lexer::trim(pdf_lexer::find_dictionary_value(stdCf, "CFM"));
            if (cryptFilterMethod != "/V2" && cryptFilterMethod != "/AESV2")
                {
                return nullptr;
                }
            }
        else if (version == 5)
            {
            cryptFilterMethod = "/AESV3";
            }
        else if (version != 1 && version != 2)
            {
            return nullptr;
            }

        long permissions{ 0 };
        std::ignore = pdf_lexer::to_int(
            pdf_lexer::trim(pdf_lexer::find_dictionary_value(encryptDict, "P")), permissions);

        long keyLengthBits{ 40 };
        std::ignore = pdf_lexer::to_int(
            pdf_lexer::trim(pdf_lexer::find_dictionary_value(encryptDict, "Length")),
            keyLengthBits);
        if (keyLengthBits < 40 || keyLengthBits > 128 || (keyLengthBits % 8) != 0)
            {
            keyLengthBits = 40;
            }

        const std::string_view encryptMetadataValue{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(encryptDict, "EncryptMetadata")) };
        const bool encryptMetadata{ encryptMetadataValue != "false" };

        const std::string ownerKey{ pdf_extract_text::read_raw_string(
            pdf_lexer::find_dictionary_value(encryptDict, "O")) };
        const std::string userKey{ pdf_extract_text::read_raw_string(
            pdf_lexer::find_dictionary_value(encryptDict, "U")) };
        const std::string userEncryptionKey{ pdf_extract_text::read_raw_string(
            pdf_lexer::find_dictionary_value(encryptDict, "UE")) };

        // the document ID is the first element of the trailer's /ID array
        std::string documentId;
        const std::string_view idArray{ pdf_lexer::trim(idValue) };
        if (!idArray.empty() && idArray.front() == '[')
            {
            size_t pos{ 1 };
            pdf_lexer::skip_whitespace(idArray, pos);
            documentId = pdf_extract_text::read_raw_string(idArray.substr(pos));
            }

        return pdf_decryptor::create(ownerKey, userKey, userEncryptionKey, documentId,
                                     static_cast<int32_t>(permissions), static_cast<int>(revision),
                                     static_cast<size_t>(keyLengthBits / 8), encryptMetadata,
                                     cryptFilterMethod, aesFunction, hashFunction);
        }

    std::vector<std::string_view>
    pdf_extract_text::collect_trailers(const std::string_view fileContent,
                                       const pdf_document& document)
        {
        std::vector<std::string_view> trailers;
        size_t searchPos{ 0 };
        while ((searchPos = fileContent.find("trailer", searchPos)) != std::string_view::npos)
            {
            size_t pos{ searchPos + 7 };
            pdf_lexer::skip_whitespace(fileContent, pos);
            if (fileContent.compare(pos, 2, "<<") == 0)
                {
                trailers.push_back(pdf_lexer::read_value(fileContent, pos));
                }
            searchPos += 7;
            }
        // cross-reference streams (PDF 1.5+) act as trailers as well
        for (const long objectNumber : document.get_scan_order())
            {
            const pdf_object* currentObject{ document.find_object(objectNumber) };
            if (currentObject != nullptr && !currentObject->m_dictionary.empty() &&
                pdf_lexer::trim(pdf_lexer::find_dictionary_value(currentObject->m_dictionary,
                                                                 "Type")) == "/XRef")
                {
                trailers.push_back(currentObject->m_dictionary);
                }
            }
        return trailers;
        }

    void pdf_extract_text::walk_page_tree(const pdf_document& document, const long nodeNumber,
                                          std::set<long>& visitedNodes,
                                          std::vector<long>& pageOrder, const int depth)
        {
        if (depth > 64 || !visitedNodes.insert(nodeNumber).second)
            {
            return;
            }
        const pdf_object* nodeObject{ document.find_object(nodeNumber) };
        if (nodeObject == nullptr || nodeObject->m_dictionary.empty())
            {
            return;
            }
        const std::string_view nodeType{ pdf_lexer::trim(
            pdf_lexer::find_dictionary_value(nodeObject->m_dictionary, "Type")) };
        if (nodeType == "/Page")
            {
            pageOrder.push_back(nodeNumber);
            return;
            }
        const std::string_view kidsValue{ pdf_lexer::trim(document.resolve_value(
            pdf_lexer::find_dictionary_value(nodeObject->m_dictionary, "Kids"))) };
        if (kidsValue.empty() || kidsValue.front() != '[')
            {
            return;
            }
        size_t pos{ 1 };
        while (pos < kidsValue.length())
            {
            pdf_lexer::skip_whitespace(kidsValue, pos);
            if (pos >= kidsValue.length() || kidsValue[pos] == ']')
                {
                break;
                }
            const std::string_view kidValue{ pdf_lexer::read_value(kidsValue, pos) };
            if (kidValue.empty())
                {
                break;
                }
            long kidNumber{ 0 };
            if (pdf_lexer::get_reference(kidValue, kidNumber))
                {
                pdf_extract_text::walk_page_tree(document, kidNumber, visitedNodes, pageOrder,
                                                 depth + 1);
                }
            }
        }

    //------------------------------------------------------------------
    const wchar_t* pdf_extract_text::operator()(const char* pdf_buffer, const size_t text_length)
        {
        clear_log();
        clear();
        m_title.clear();
        m_author.clear();
        m_subject.clear();
        m_keywords.clear();
        if (pdf_buffer == nullptr || text_length == 0)
            {
            return nullptr;
            }
        const std::string_view fileContent{ pdf_buffer, text_length };

        // see if this is a valid PDF file (the header should be at, or at least
        // very near, the start of the file)
        if (fileContent.substr(0, 1024).find("%PDF-") == std::string_view::npos)
            {
            throw pdf_header_not_found();
            }

        allocate_text_buffer(text_length);

        pdf_document document{ fileContent, m_decompress,
                               [this](const std::wstring& message) { log_message(message); },
                               &m_glyph_name_table, &m_cid_to_unicode_tables };
        document.set_charset_converter(m_charset_convert);
        document.catalog_objects();

        // review the trailer(s) for encryption, the document ID, the page tree root,
        // and the metadata. This must happen before expand_object_streams(), since an
        // encrypted file's compressed object streams need to be decrypted before they
        // can be expanded.
        std::string_view rootValue, infoValue, encryptValue, idValue;
        for (const std::string_view trailerDict :
             pdf_extract_text::collect_trailers(fileContent, document))
            {
            const std::string_view currentEncrypt{ pdf_lexer::find_dictionary_value(trailerDict,
                                                                                    "Encrypt") };
            if (!currentEncrypt.empty())
                {
                encryptValue = currentEncrypt;
                }
            const std::string_view currentId{ pdf_lexer::find_dictionary_value(trailerDict, "ID") };
            if (!currentId.empty())
                {
                idValue = currentId;
                }
            const std::string_view currentRoot{ pdf_lexer::find_dictionary_value(trailerDict,
                                                                                 "Root") };
            if (!currentRoot.empty())
                {
                rootValue = currentRoot;
                }
            const std::string_view currentInfo{ pdf_lexer::find_dictionary_value(trailerDict,
                                                                                 "Info") };
            if (!currentInfo.empty())
                {
                infoValue = currentInfo;
                }
            }

        // if the document is encrypted, only continue if we can authenticate against
        // it using an empty user password (the common case of a document that is
        // merely restricted from editing/printing, but opens for reading)
        std::unique_ptr<pdf_decryptor> decryptor;
        if (!encryptValue.empty())
            {
            decryptor = pdf_extract_text::setup_decryption(document, encryptValue, idValue,
                                                           m_aes_decrypt, m_hash);
            if (decryptor == nullptr)
                {
                throw pdf_encrypted();
                }
            document.set_decryptor(decryptor.get());
            }

        document.expand_object_streams();

        // read the document's metadata
        const pdf_object* infoObject{ document.resolve_to_object(infoValue) };
        if (infoObject != nullptr && !infoObject->m_dictionary.empty())
            {
            m_title = pdf_extract_text::read_string_value(
                document, *infoObject,
                pdf_lexer::find_dictionary_value(infoObject->m_dictionary, "Title"));
            m_author = pdf_extract_text::read_string_value(
                document, *infoObject,
                pdf_lexer::find_dictionary_value(infoObject->m_dictionary, "Author"));
            m_subject = pdf_extract_text::read_string_value(
                document, *infoObject,
                pdf_lexer::find_dictionary_value(infoObject->m_dictionary, "Subject"));
            m_keywords = pdf_extract_text::read_string_value(
                document, *infoObject,
                pdf_lexer::find_dictionary_value(infoObject->m_dictionary, "Keywords"));
            }

        // put the pages into document order by walking the page tree from the catalog
        std::vector<long> pageOrder;
        const pdf_object* rootObject{ document.resolve_to_object(rootValue) };
        if (rootObject != nullptr && !rootObject->m_dictionary.empty())
            {
            long pagesRootNumber{ 0 };
            if (pdf_lexer::get_reference(
                    pdf_lexer::find_dictionary_value(rootObject->m_dictionary, "Pages"),
                    pagesRootNumber))
                {
                std::set<long> visitedNodes;
                pdf_extract_text::walk_page_tree(document, pagesRootNumber, visitedNodes, pageOrder,
                                                 0);
                }
            }
        // ...or fall back to the order that the page objects appear in the file
        if (pageOrder.empty())
            {
            for (const long objectNumber : document.get_scan_order())
                {
                const pdf_object* currentObject{ document.find_object(objectNumber) };
                if (currentObject != nullptr && !currentObject->m_dictionary.empty() &&
                    pdf_lexer::trim(pdf_lexer::find_dictionary_value(currentObject->m_dictionary,
                                                                     "Type")) == "/Page")
                    {
                    pageOrder.push_back(objectNumber);
                    }
                }
            }

        // extract the text from each page
        std::wstring text;
        text.reserve(text_length / 4);
        pdf_content_parser parser{ document, text };
        for (size_t i = 0; i < pageOrder.size(); ++i)
            {
            const pdf_object* pageObject{ document.find_object(pageOrder[i]) };
            if (pageObject != nullptr)
                {
                parser.parse_page(*pageObject);
                // close out any RTL run left open by the page's last line
                parser.flush_rtl_run();
                }
            // Separate pages with a blank line. A form feed would be more precise,
            // but most text controls have no glyph for it and render it as garbage
            // (e.g., the legacy CP437 glyph for 0x0C is the Venus symbol).
            if ((i + 1) < pageOrder.size())
                {
                text += L'\n';
                }
            }
        if (pageOrder.empty())
            {
            log_message(L"No pages found in PDF file.");
            }

        if (m_normalize)
            {
            text = m_normalize(text);
            }

        add_characters(text);
        trim();
        return get_filtered_text();
        }
    } // namespace lily_of_the_valley
