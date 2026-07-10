///////////////////////////////////////////////////////////////////////////////
// Name:        pdfreader.cpp
// Author:      Blake Madden
// Copyright:   (c) 2005-2026 Blake Madden
// License:     3-Clause BSD license
// SPDX-License-Identifier: BSD-3-Clause
///////////////////////////////////////////////////////////////////////////////

#include "pdfreader.h"
#include "../util/textstream.h"
#include <stdexcept>
#include <wx/intl.h>
#include <wx/mstream.h>
#include <wx/strconv.h>
#include <wx/zstream.h>

namespace Wisteria::Data
    {
    //------------------------------------------------------------------
    wxString PdfReader::ReadFile(const wxString& filePath)
        {
        try
            {
            const MemoryMappedFile sourceFile(filePath, true, true);
            const wchar_t* extractedText{ m_pdfTextExtractor(
                static_cast<const char*>(sourceFile.GetStream()), sourceFile.GetMapSize()) };
            if (extractedText == nullptr)
                {
                return {};
                }
            return wxString{ extractedText, m_pdfTextExtractor.get_filtered_text_length() };
            }
        catch (const lily_of_the_valley::pdf_extract_text::pdf_encrypted&)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': PDF file is encrypted and cannot be read."), filePath)
                    .ToUTF8());
            }
        catch (const lily_of_the_valley::pdf_extract_text::pdf_header_not_found&)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': invalid PDF file."), filePath).ToUTF8());
            }
        catch (...)
            {
            throw std::runtime_error(
                wxString::Format(_(L"'%s': error reading PDF file."), filePath).ToUTF8());
            }
        }

    //------------------------------------------------------------------
    bool PdfReader::LoadGlyphNameTableFromFile(const wxString& filePath)
        {
        wxString resolvedPath{ filePath };
        wxString fileText;
        if (!Wisteria::TextStream::ReadFile(
                resolvedPath, fileText, wxString{},
                Wisteria::TextStream::ReadFileInteractivityMode::NoInteractivity))
            {
            return false;
            }
        m_pdfTextExtractor.load_glyph_name_table(fileText.ToStdWstring());
        return true;
        }

    //------------------------------------------------------------------
    std::wstring PdfReader::ConvertCharset(const std::string_view sourceBytes,
                                           const std::string_view charsetName)
        {
        if (sourceBytes.empty() || charsetName.empty())
            {
            return {};
            }
        const wxCSConv charsetConverter{ wxString{ charsetName.data(), charsetName.length() } };
        if (!charsetConverter.IsOk())
            {
            return {};
            }
        const wxString convertedText{ sourceBytes.data(), charsetConverter, sourceBytes.length() };
        return { convertedText.wc_str(), convertedText.length() };
        }

    //------------------------------------------------------------------
    std::string PdfReader::Inflate(const std::string_view compressedStream)
        {
        if (compressedStream.empty())
            {
            return {};
            }
        wxMemoryInputStream memoryStream(compressedStream.data(), compressedStream.length());
        wxZlibInputStream zlibStream(memoryStream, wxZLIB_AUTO);
        if (!zlibStream.IsOk())
            {
            return {};
            }
        std::string uncompressed;
        uncompressed.reserve(compressedStream.length() * 4);
        constexpr size_t READ_BLOCK_SIZE{ 16384 };
        char readBuffer[READ_BLOCK_SIZE]{};
        while (!zlibStream.Eof())
            {
            zlibStream.Read(readBuffer, READ_BLOCK_SIZE);
            const size_t lastReadSize{ zlibStream.LastRead() };
            if (lastReadSize == 0)
                {
                break;
                }
            uncompressed.append(readBuffer, lastReadSize);
            }
        return uncompressed;
        }
    } // namespace Wisteria::Data
