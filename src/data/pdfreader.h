/** @addtogroup Data
    @brief Data management classes for graphs.
    @date 2005-2026
    @copyright Blake Madden
    @author Blake Madden
    @details This program is free software; you can redistribute it and/or modify
     it under the terms of the 3-Clause BSD License.

     SPDX-License-Identifier: BSD-3-Clause
@{*/

#ifndef WISTERIA_PDF_H
#define WISTERIA_PDF_H

#include "../import/pdf_extract_text.h"
#include "../util/memorymappedfile.h"
#include <string>
#include <string_view>
#include <wx/string.h>

namespace Wisteria::Data
    {
    /** @brief Interface for reading the text from a PDF file.
        @details This connects the parsing logic to wxWidgets so that it can handle
            compressed PDF content and text written in CJK (Chinese, Japanese, Korean) charsets.
        @par Example:
        @code
            Wisteria::Data::PdfReader pdfReader;
            // Optional: some PDFs use custom, non-standard fonts that need
            // an extra lookup table to translate their text correctly.
            // If you run into a PDF with garbled or missing text, try loading
            // a glyph name table (e.g., the Adobe Glyph List) like this first.
            pdfReader.LoadGlyphNameTableFromFile(
                L"/Users/kdaly/Documents/Data/glyphlist.txt");
            // Optional: needed only for CJK PDFs whose text still comes out
            // empty or garbled after trying without this. Load one of these
            // for each language you need to support:
            //     "Adobe-Japan1" -> UniJIS-UTF16-H  (Japanese)
            //     "Adobe-GB1"    -> UniGB-UTF16-H   (Simplified Chinese)
            //     "Adobe-CNS1"   -> UniCNS-UTF16-H  (Traditional Chinese)
            //     "Adobe-Korea1" -> UniKS-UTF16-H   (Korean, older ordering)
            //     "Adobe-KR"     -> UniAKR-UTF16-H  (Korean, current ordering)
            pdfReader.LoadCidToUnicodeTableFromFile(
                L"Adobe-Japan1", L"/Users/kdaly/Documents/Data/UniJIS-UTF16-H");
            pdfReader.LoadCidToUnicodeTableFromFile(
                L"Adobe-CNS1", L"/Users/kdaly/Documents/Data/UniCNS-UTF16-H");

            try
                {
                const wxString pdfText{ pdfReader.ReadFile(
                    L"/Users/kdaly/Documents/Letters/Reconnecting with My Wife.pdf") };

                wxLogMessage(L"Title: %s", pdfReader.GetTitle());
                wxLogMessage(L"Author: %s", pdfReader.GetAuthor());
                wxLogMessage(L"%s", pdfText);
                }
            catch (const std::runtime_error& readError)
                {
                wxLogError(wxString::FromUTF8(readError.what()));
                }
        @endcode*/
    class PdfReader
        {
      public:
        /// @brief Constructor; connects the wxWidgets zlib decompressor,
        ///     charset converter, and AES/SHA-2 crypto functors to the parser.
        PdfReader()
            {
            m_pdfTextExtractor.set_stream_decompressor(&PdfReader::Inflate);
            m_pdfTextExtractor.set_charset_converter(&PdfReader::ConvertCharset);
            m_pdfTextExtractor.set_aes_decryptor(&PdfReader::AesCbcCrypt);
            m_pdfTextExtractor.set_hash_functor(&PdfReader::Sha2Hash);
            }

        /** @brief Reads the raw text from a PDF file.
            @param filePath The path to the PDF file to read.
            @returns The document's text.
            @throws std::runtime_error If the file is invalid or encrypted,
                throws an exception.\n
                The exception's @c what() message is UTF-8 encoded, so pass it to
                @c wxString::FromUTF8() when formatting it for an error message.*/
        [[nodiscard]]
        wxString ReadFile(const wxString& filePath);

        /** @brief Loads a lookup table that helps translate text from
                PDFs that use custom, non-standard fonts. (This is optional.)
            @details The file is expected in the same format as Adobe's AGL data files:
                semicolon-delimited lines of `glyphname;XXXX[ XXXX...]`, where each `XXXX`
                is a four-digit hexadecimal Unicode value; lines starting with `#`
                (and blank lines) are ignored.
            @param filePath The path to the glyph name table file.
            @returns @c true if file is found and read successfully.
            @sa https://github.com/adobe-type-tools/agl-aglfn */
        bool LoadGlyphNameTableFromFile(const wxString& filePath);

        /** @brief Loads a lookup table that helps translate text from PDFs
                whose CJK text has no built-in Unicode mapping of its own.
                (This is optional.)
            @details Call this once for each language/character set you need to
                support. A PDF only needs this if its text still comes out empty or
                garbled after trying without it.
            @param registryOrdering The character collection the table is for, e.g.
                "Adobe-Japan1" for Japanese, "Adobe-GB1" for Simplified Chinese,
                "Adobe-CNS1" for Traditional Chinese, or "Adobe-Korea1"/"Adobe-KR"
                for Korean.
            @param filePath The path to the lookup table file for that collection
                (e.g., "UniJIS-UTF16-H" for "Adobe-Japan1").
            @returns @c true if file is found and read successfully.
            @sa https://github.com/adobe-type-tools/cmap-resources */
        bool LoadCidToUnicodeTableFromFile(const wxString& registryOrdering,
                                           const wxString& filePath);

        /// @returns The title from the document's metadata.
        /// @note Must be called after calling @c ReadFile().
        [[nodiscard]]
        wxString GetTitle() const
            {
            return m_pdfTextExtractor.get_title();
            }

        /// @returns The author from the document's metadata.
        /// @note Must be called after calling @c ReadFile().
        [[nodiscard]]
        wxString GetAuthor() const
            {
            return m_pdfTextExtractor.get_author();
            }

        /// @returns The subject from the document's metadata.
        /// @note Must be called after calling @c ReadFile().
        [[nodiscard]]
        wxString GetSubject() const
            {
            return m_pdfTextExtractor.get_subject();
            }

        /// @returns The keywords from the document's metadata.
        /// @note Must be called after calling @c ReadFile().
        [[nodiscard]]
        wxString GetKeywords() const
            {
            return m_pdfTextExtractor.get_keywords();
            }

        /// @returns A report of any issues encountered while reading the file.
        [[nodiscard]]
        wxString GetLogReport() const
            {
            return m_pdfTextExtractor.get_log();
            }

        /** @brief Decompresses a zlib (DEFLATE) stream section
                (using @c wxZlibInputStream).
            @details This is the decompression functor that the constructor connects
                to the underlying lily_of_the_valley::pdf_extract_text parser; it is
                @c public so that it can also be connected to standalone instances
                of that class.
            @param compressedStream The compressed bytes of a stream section.
            @returns The uncompressed bytes, or an empty string upon failure.*/
        [[nodiscard]]
        static std::string Inflate(std::string_view compressedStream);

        /** @brief Converts legacy multibyte charset bytes (e.g., Big5, Shift-JIS)
                to Unicode (using @c wxCSConv).
            @details This is the charset conversion functor that the constructor
                connects to the underlying lily_of_the_valley::pdf_extract_text
                parser (used to decode CJK text that uses one of Adobe's predefined
                legacy CMap encodings); it is @c public so that it can also be
                connected to standalone instances of that class.
            @param sourceBytes The raw bytes of a string in @c charsetName.
            @param charsetName The name of the charset the bytes are encoded in
                (e.g., "CP950").
            @returns The equivalent Unicode text, or an empty string if the charset
                isn't supported by the platform or the bytes are invalid for it.*/
        [[nodiscard]]
        static std::wstring ConvertCharset(std::string_view sourceBytes,
                                           std::string_view charsetName);

        /** @brief Performs AES-CBC encryption/decryption (using @c wxPdfRijndael).
            @details This is the AES functor that the constructor connects to the
                underlying `lily_of_the_valley::pdf_extract_text` parser (used to
                decrypt AES-encrypted PDFs).
            @param key The AES key. Its length (16 or 32 bytes) selects AES-128
                or AES-256.
            @param initVector The 16-byte initialization vector.
            @param data The bytes to transform (must be a multiple of 16 bytes long.)
            @param direction Whether to encrypt or decrypt @c data.
            @returns The transformed bytes, or an empty string upon failure.*/
        [[nodiscard]]
        static std::string AesCbcCrypt(std::string_view key, std::string_view initVector,
                                       std::string_view data,
                                       lily_of_the_valley::cipher_direction direction);

        /** @brief Computes a SHA-2 message digest (using wxpdfdoc's @c crypto helpers).
            @details This is the hashing functor that the constructor connects to
                the underlying `lily_of_the_valley::pdf_extract_text` parser (used
                for revision 5/6 encrypted PDFs' key derivation).
            @param data The bytes to hash.
            @param digestBits Which SHA-2 variant to use: 256, 384, or 512.
            @returns The binary digest, or an empty string if @c digestBits isn't
                256, 384, or 512.*/
        [[nodiscard]]
        static std::string Sha2Hash(std::string_view data, int digestBits);

      private:
        lily_of_the_valley::pdf_extract_text m_pdfTextExtractor;
        inline static bool m_loadedGlyphTable{ false };
        inline static bool m_loadedCidTable{ false };
        };
    } // namespace Wisteria::Data

/** @}*/

#endif // WISTERIA_PDF_H
